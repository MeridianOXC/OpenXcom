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
#include "RuleResearch.h"
#include "../Engine/Exception.h"
#include "../Engine/Collections.h"
#include "../Engine/ScriptBind.h"
#include "Mod.h"

namespace OpenXcom
{

RuleResearch::RuleResearch(const std::string &name) : _name(name), _spawnedItemCount(1), _cost(0), _points(0), _sequentialGetOneFree(false), _needItem(false), _destroyItem(false), _listOrder(0)
{
}

/**
 * Loads the research project from a YAML file.
 * @param node YAML node.
 * @param listOrder The list weight for this research.
 */
void RuleResearch::load(const YAML::Node &node, Mod* mod, const ModScript& parsers, int listOrder)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod, parsers, listOrder);
	}
	_name = node["name"].as<std::string>(_name);
	_lookup = node["lookup"].as<std::string>(_lookup);
	_cutscene = node["cutscene"].as<std::string>(_cutscene);
	_spawnedItem = node["spawnedItem"].as<std::string>(_spawnedItem);
	_spawnedItemCount = node["spawnedItemCount"].as<int>(_spawnedItemCount);
	mod->loadUnorderedNames(_name, _spawnedItemList, node["spawnedItemList"]);
	mod->loadUnorderedNames(_name, _decreaseCounter, node["decreaseCounter"]);
	mod->loadUnorderedNames(_name, _increaseCounter, node["increaseCounter"]);
	_spawnedEvent = node["spawnedEvent"].as<std::string>(_spawnedEvent);
	_cost = node["cost"].as<int>(_cost);
	_points = node["points"].as<int>(_points);
	mod->loadUnorderedNames(_name, _dependenciesName, node["dependencies"]);
	mod->loadUnorderedNames(_name, _unlocksName, node["unlocks"]);
	mod->loadUnorderedNames(_name, _disablesName, node["disables"]);
	mod->loadUnorderedNames(_name, _reenablesName, node["reenables"]);
	mod->loadUnorderedNames(_name, _getOneFreeName, node["getOneFree"]);
	mod->loadUnorderedNames(_name, _requiresName, node["requires"]);
	mod->loadBaseFunction(_name, _requiresBaseFunc, node["requiresBaseFunc"]);
	_sequentialGetOneFree = node["sequentialGetOneFree"].as<bool>(_sequentialGetOneFree);
	mod->loadNamesToNames(_name, _getOneFreeProtectedName, node["getOneFreeProtected"]);
	_needItem = node["needItem"].as<bool>(_needItem);
	_destroyItem = node["destroyItem"].as<bool>(_destroyItem);
	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (!_listOrder)
	{
		_listOrder = listOrder;
	}
	// This is necessary, research code assumes it!
	if (!_requiresName.empty() && _cost != 0)
	{
		throw Exception("Research topic " + _name + " has requirements, but the cost is not zero. Sorry, this is not allowed!");
	}
	_scriptValues.load(node, parsers.getShared());
}

/**
 * Cross link with other Rules.
 */
void RuleResearch::afterLoad(const Mod* mod)
{
	if (_lookup == _name)
	{
		_lookup = "";
	}

	_dependencies = mod->getResearch(_dependenciesName);
	_unlocks = mod->getResearch(_unlocksName);
	_disables = mod->getResearch(_disablesName);
	_reenables = mod->getResearch(_reenablesName);
	_getOneFree = mod->getResearch(_getOneFreeName);
	_requires = mod->getResearch(_requiresName);

	_getOneFreeProtected.reserve(_getOneFreeProtectedName.size());
	for (auto& n : _getOneFreeProtectedName)
	{
		auto left = mod->getResearch(n.first, false);
		if (left)
		{
			auto right = mod->getResearch(n.second);
			_getOneFreeProtected.push_back(std::make_pair(left, right));
		}
		else
		{
			throw Exception("Unknown research '" + n.first + "'");
		}
	}

	//remove not needed data
	Collections::removeAll(_dependenciesName);
	Collections::removeAll(_unlocksName);
	Collections::removeAll(_disablesName);
	Collections::removeAll(_reenablesName);
	Collections::removeAll(_getOneFreeName);
	Collections::removeAll(_requiresName);
	Collections::removeAll(_getOneFreeProtectedName);
}

/**
 * Gets the cost of this ResearchProject.
 * @return The cost of this ResearchProject (in man/day).
 */
int RuleResearch::getCost() const
{
	return _cost;
}

/**
 * Gets the name of this ResearchProject.
 * @return The name of this ResearchProject.
 */
const std::string &RuleResearch::getName() const
{
	return _name;
}

/**
 * Gets the list of dependencies, i.e. ResearchProjects, that must be discovered before this one.
 * @return The list of ResearchProjects.
 */
const std::vector<const RuleResearch*> &RuleResearch::getDependencies() const
{
	return _dependencies;
}

/**
 * Checks if this ResearchProject gives free topics in sequential order (or random order).
 * @return True if the ResearchProject gives free topics in sequential order.
 */
bool RuleResearch::sequentialGetOneFree() const
{
	return _sequentialGetOneFree;
}

/**
 * Checks if this ResearchProject needs a corresponding Item to be researched.
 *  @return True if the ResearchProject needs a corresponding item.
 */
bool RuleResearch::needItem() const
{
	return _needItem;
}

/**
 * Checks if this ResearchProject needs a corresponding Item to be researched.
 *  @return True if the ResearchProject needs a corresponding item.
 */
bool RuleResearch::destroyItem() const
{
	return _destroyItem;
}
/**
 * Gets the list of ResearchProjects unlocked by this research.
 * @return The list of ResearchProjects.
 */
const std::vector<const RuleResearch*> &RuleResearch::getUnlocked() const
{
	return _unlocks;
}

/**
 * Gets the list of ResearchProjects disabled by this research.
 * @return The list of ResearchProjects.
 */
const std::vector<const RuleResearch*> &RuleResearch::getDisabled() const
{
	return _disables;
}

/**
 * Gets the list of ResearchProjects reenabled by this research.
 * @return The list of ResearchProjects.
 */
const std::vector<const RuleResearch*> &RuleResearch::getReenabled() const
{
	return _reenables;
}

/**
 * Get the points earned for this ResearchProject.
 * @return The points earned for this ResearchProject.
 */
int RuleResearch::getPoints() const
{
	return _points;
}

/**
 * Gets the list of ResearchProjects granted at random for free by this research.
 * @return The list of ResearchProjects.
 */
const std::vector<const RuleResearch*> &RuleResearch::getGetOneFree() const
{
	return _getOneFree;
}

/**
 * Gets the list(s) of ResearchProjects granted at random for free by this research (if a defined prerequisite is met).
 * @return The list(s) of ResearchProjects.
 */
const std::vector<std::pair<const RuleResearch*, std::vector<const RuleResearch*> > > &RuleResearch::getGetOneFreeProtected() const
{
	return _getOneFreeProtected;
}

/**
 * Gets what article to look up in the ufopedia.
 * @return The article to look up in the ufopaedia
 */
const std::string &RuleResearch::getLookup() const
{
	return _lookup;
}

/**
 * Gets the requirements for this ResearchProject.
 * @return The requirement for this research.
 */
const std::vector<const RuleResearch*> &RuleResearch::getRequirements() const
{
	return _requires;
}

/**
 * Gets the list weight for this research item.
 * @return The list weight for this research item.
 */
int RuleResearch::getListOrder() const
{
	return _listOrder;
}

/**
 * Gets the cutscene to play when this research item is completed.
 * @return The cutscene id.
 */
const std::string & RuleResearch::getCutscene() const
{
	return _cutscene;
}

/**
 * Gets the item to spawn in the base stores when this topic is researched.
 * @return The item id.
 */
const std::string & RuleResearch::getSpawnedItem() const
{
	return _spawnedItem;
}

////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

	std::string debugDisplayScript(const RuleResearch* ru)
	{
		if (ru)
		{
			std::string s;
			s += RuleResearch::ScriptName;
			s += "(name: \"";
			s += ru->getName();
			s += "\")";
			return s;
		}
		else
		{
			return "null";
		}
	}

}

/**
 * Register RuleResearch in script parser.
 * @param parser Script parser.
 */
void RuleResearch::ScriptRegister(ScriptParserBase* parser)
{
	Bind<RuleResearch> ar = { parser };

	ar.add<&RuleResearch::getCost>("getCost");
	ar.add<&RuleResearch::getPoints>("getPoints");

	ar.addScriptValue<BindBase::OnlyGet, &RuleResearch::_scriptValues>();
	ar.addDebugDisplay<&debugDisplayScript>();
}

}
