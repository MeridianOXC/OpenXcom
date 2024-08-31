/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include "RuleManufacture.h"
#include "RuleManufactureShortcut.h"
#include "RuleResearch.h"
#include "RuleCraft.h"
#include "RuleItem.h"
#include "Mod.h"
#include "../Engine/Collections.h"

namespace OpenXcom
{
/**
 * Creates a new Manufacture.
 * @param name The unique manufacture name.
 */
RuleManufacture::RuleManufacture(const std::string &name, int listOrder) :
	_name(name), _space(0), _time(0), _cost(0), _points(0), _refund(false), _producedCraft(0), _listOrder(listOrder)
{
	_producedItemsNames[name] = 1;
}

/**
 * Loads the manufacture project from a YAML file.
 * @param node YAML node.
 * @param listOrder The list weight for this manufacture.
 */
void RuleManufacture::load(const YAML::Node &node, Mod* mod)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod);
	}

	_category = node["category"].as<std::string>(_category);
	mod->loadUnorderedNames(_name, _requiresName, node["requires"]);
	mod->loadBaseFunction(_name, _requiresBaseFunc, node["requiresBaseFunc"]);
	_space = node["space"].as<int>(_space);
	_time = node["time"].as<int>(_time);
	_cost = node["cost"].as<int>(_cost);
	_points = node["points"].as<int>(_points);
	_refund = node["refund"].as<bool>(_refund);
	mod->loadUnorderedNamesToInt(_name, _requiredItemsNames, node["requiredItems"]);
	mod->loadUnorderedNamesToInt(_name, _producedItemsNames, node["producedItems"]);
	_randomProducedItemsNames = node["randomProducedItems"].as< std::vector<std::pair<int, std::map<std::string, int> > > >(_randomProducedItemsNames);
	_spawnedPersonType = node["spawnedPersonType"].as<std::string>(_spawnedPersonType);
	_spawnedPersonName = node["spawnedPersonName"].as<std::string>(_spawnedPersonName);
	if (node["spawnedSoldier"])
	{
		_spawnedSoldier = node["spawnedSoldier"];
	}
	_transferTimes = node["transferTimes"].as< std::vector<int> >(_transferTimes);
	_listOrder = node["listOrder"].as<int>(_listOrder);
}

/**
 * Cross link with other Rules.
 */
void RuleManufacture::afterLoad(const Mod* mod)
{
	if (_time <= 0)
	{
		throw Exception("Manufacturing time must be greater than zero.");
	}

	_requires = mod->getResearch(_requiresName);
	if (_category == "STR_CRAFT")
	{
		auto item = _producedItemsNames.begin();
		if (item == _producedItemsNames.end())
		{
			throw Exception("No craft defined for production");
		}
		else if (item->second != 1)
		{
			throw Exception("Only one craft can be build in production");
		}
		else
		{
			_producedCraft = mod->getCraft(item->first, true);
		}
	}
	else
	{
		for (auto& i : _producedItemsNames)
		{
			_producedItems[mod->getItem(i.first, true)] = i.second;
		}
	}
	for (auto& i : _requiredItemsNames)
	{
		auto* itemRule = mod->getItem(i.first, false);
		auto* craftRule = mod->getCraft(i.first, false);
		if (itemRule)
		{
			_requiredItems[itemRule] = i.second;
		}
		else if (craftRule)
		{
			_requiredCrafts[craftRule] = i.second;
		}
		else
		{
			throw Exception("Unknown required item '" + i.first + "'");
		}
	}

	for (auto& itemSet : _randomProducedItemsNames)
	{
		std::map<const RuleItem*, int> tmp;
		for (auto& i : itemSet.second)
		{
			tmp[mod->getItem(i.first, true)] = i.second;
		}
		_randomProducedItems.push_back(std::make_pair(itemSet.first, tmp));
	}

	//remove not needed data
	Collections::removeAll(_requiresName);
	Collections::removeAll(_producedItemsNames);
	Collections::removeAll(_requiredItemsNames);
	Collections::removeAll(_randomProducedItemsNames);
}

/**
 * Change the name and break down the sub-projects into simpler components.
 */
void RuleManufacture::breakDown(const Mod* mod, const RuleManufactureShortcut* recipe)
{
	// 0. rename
	_name = recipe->getName();

	// 1. init temp variables
	std::map<const RuleItem*, int> tempRequiredItems = _requiredItems;
	std::map<const RuleResearch*, bool> tempRequires;
	for (auto* r : _requires)
		tempRequires[r] = true;
	RuleBaseFacilityFunctions tempRequiresBaseFunc = _requiresBaseFunc;

	// 2. break down iteratively
	bool doneSomething = false;
	do
	{
		doneSomething = false;
		for (auto& projectName : recipe->getBreakDownItems())
		{
			RuleItem* itemRule = mod->getItem(projectName, true);
			RuleManufacture* projectRule = mod->getManufacture(projectName, true);
			int count = tempRequiredItems[itemRule];
			if (count > 0)
			{
				tempRequiredItems[itemRule] = 0;
				doneSomething = true;

				_space = std::max(_space, projectRule->getRequiredSpace());
				_time += count * projectRule->getManufactureTime();
				_cost += count * projectRule->getManufactureCost();

				for (auto& ri : projectRule->getRequiredItems())
					tempRequiredItems[ri.first] += count * ri.second;
				for (auto* r : projectRule->getRequirements())
					tempRequires[r] = true;

				tempRequiresBaseFunc |= projectRule->getRequireBaseFunc();
			}
		}
	}
	while (doneSomething);

	// 3. update
	{
		_requiredItems.clear();
		for (auto& item : tempRequiredItems)
			if (item.second > 0)
				_requiredItems[item.first] = item.second;
	}
	if (recipe->getBreakDownRequires())
	{
		_requires.clear();
		for (auto& item : tempRequires)
			if (item.second)
				_requires.push_back(item.first);
	}
	if (recipe->getBreakDownRequiresBaseFunc())
	{
		_requiresBaseFunc = tempRequiresBaseFunc;
	}
}

/**
 * Gets the unique name of the manufacture.
 * @return The name.
 */
const std::string &RuleManufacture::getName() const
{
	return _name;
}

/**
 * Gets the category shown in the manufacture list.
 * @return The category.
 */
const std::string &RuleManufacture::getCategory() const
{
	return _category;
}

/**
 * Gets the list of research required to
 * manufacture this object.
 * @return A list of research IDs.
 */
const std::vector<const RuleResearch*> &RuleManufacture::getRequirements() const
{
	return _requires;
}

/**
 * Gets the required workspace to start production.
 * @return The required workspace.
 */
int RuleManufacture::getRequiredSpace() const
{
	return _space;
}

/**
 * Gets the time needed to manufacture one object.
 * @return The time needed to manufacture one object (in man/hour).
 */
int RuleManufacture::getManufactureTime() const
{
	return _time;
}


/**
 * Gets the cost of manufacturing one object.
 * @return The cost of manufacturing one object.
 */
int RuleManufacture::getManufactureCost() const
{
	return _cost;
}

/**
 * Checks if there's enough funds to manufacture one object.
 * @param funds Current funds.
 * @return True if manufacture is possible.
 */
bool RuleManufacture::haveEnoughMoneyForOneMoreUnit(int64_t funds) const
{
	// either we have enough money, or the production doesn't cost anything
	return funds >= _cost || _cost <= 0;
}

/**
 * Should all resources of a cancelled project be refunded?
 * @return True, if all resources should be refunded. False otherwise.
 */
bool RuleManufacture::getRefund() const
{
	return _refund;
}

/**
 * Gets the list of items required to manufacture one object.
 * @return The list of items required to manufacture one object.
 */
const std::map<const RuleItem*, int> &RuleManufacture::getRequiredItems() const
{
	return _requiredItems;
}

/**
 * Gets the list of crafts required to manufacture one object.
 */
const std::map<const RuleCraft*, int> &RuleManufacture::getRequiredCrafts() const
{
	return _requiredCrafts;
}

/**
 * Gets the list of items produced by completing "one object" of this project.
 * @return The list of items produced by completing "one object" of this project.
 */
const std::map<const RuleItem*, int> &RuleManufacture::getProducedItems() const
{
	return _producedItems;
}

/*
 * Gets craft build by this project. null if is not craft production.
 * @return Craft rule set.
 */
const RuleCraft* RuleManufacture::getProducedCraft() const
{
	return _producedCraft;
}

/**
 * Gets the random manufacture rules.
 * @return The random manufacture rules.
 */
const std::vector<std::pair<int, std::map<const RuleItem*, int> > > &RuleManufacture::getRandomProducedItems() const
{
	return _randomProducedItems;
}

/**
* Gets the "manufactured person", i.e. person spawned when manufacturing project ends.
* @return The person type ID.
*/
const std::string &RuleManufacture::getSpawnedPersonType() const
{
	return _spawnedPersonType;
}

/**
* Gets the custom name of the "manufactured person".
* @return The person name translation code.
*/
const std::string &RuleManufacture::getSpawnedPersonName() const
{
	return _spawnedPersonName;
}

/**
 * Is it possible to use auto-sell feature for this manufacturing project?
 * @return True for normal projects, false for special projects.
 */
bool RuleManufacture::canAutoSell() const
{
	if (!_spawnedPersonType.empty())
	{
		// can't sell people
		return false;
	}
	if (!_randomProducedItems.empty())
	{
		// sell profit label can't show reasonable info
		return false;
	}
	return true;
}

/**
 * Gets the list weight for this manufacture item.
 * @return The list weight for this manufacture item.
 */
int RuleManufacture::getListOrder() const
{
	return _listOrder;
}

}
