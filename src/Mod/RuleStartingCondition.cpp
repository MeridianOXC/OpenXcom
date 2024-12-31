/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include "RuleStartingCondition.h"
#include "RuleItem.h"
#include "../Engine/Logger.h"
#include "../Mod/Mod.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/WeightedOptions.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of Starting Condition.
 * @param type String defining the type.
 */
RuleStartingCondition::RuleStartingCondition(const std::string& type) : _type(type), _destroyRequiredItems(false), _requireCommanderOnboard(false)
{
}

/**
 *
 */
RuleStartingCondition::~RuleStartingCondition()
{
}

/**
 * Loads the Starting Conditions from a YAML file.
 * @param node YAML node.
 */
void RuleStartingCondition::load(const YAML::YamlNodeReader& node, Mod *mod)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	mod->loadUnorderedNamesToNamesToInt(_type, _defaultArmor, reader["defaultArmor"]);
	mod->loadUnorderedNames(_type, _allowedArmors, reader["allowedArmors"]);
	mod->loadUnorderedNames(_type, _forbiddenArmors, reader["forbiddenArmors"]);
	mod->loadUnorderedNames(_type, _forbiddenArmorsInNextStageName, reader["forbiddenArmorsInNextStage"]);
	mod->loadUnorderedNames(_type, _allowedVehicles, reader["allowedVehicles"]);
	mod->loadUnorderedNames(_type, _forbiddenVehicles, reader["forbiddenVehicles"]);
	mod->loadUnorderedNames(_type, _allowedItems, reader["allowedItems"]);
	mod->loadUnorderedNames(_type, _forbiddenItems, reader["forbiddenItems"]);
	mod->loadUnorderedNames(_type, _allowedItemCategories, reader["allowedItemCategories"]);
	mod->loadUnorderedNames(_type, _forbiddenItemCategories, reader["forbiddenItemCategories"]);
	mod->loadUnorderedNames(_type, _allowedCraft, reader["allowedCraft"]);
	mod->loadUnorderedNames(_type, _forbiddenCraft, reader["forbiddenCraft"]);
	mod->loadUnorderedNames(_type, _allowedSoldierTypes, reader["allowedSoldierTypes"]);
	mod->loadUnorderedNames(_type, _forbiddenSoldierTypes, reader["forbiddenSoldierTypes"]);
	mod->loadUnorderedNamesToInt(_type, _requiredItems, reader["requiredItems"]);
	mod->loadUnorderedNamesToNames(_type, _craftTransformationsName, reader["craftTransformations"]);
	reader.tryRead("destroyRequiredItems", _destroyRequiredItems);
	reader.tryRead("requireCommanderOnboard", _requireCommanderOnboard);

	if (reader["environmentalConditions"] || reader["paletteTransformations"] || reader["armorTransformations"]
		|| reader["mapBackgroundColor"] || reader["inventoryShockIndicator"] || reader["mapShockIndicator"])
	{
		Log(LOG_ERROR) << "There are invalid/obsolete attributes in starting condition " << _type << ". Please review the ruleset.";
	}
}

/**
 * Cross link with other rules.
 */
void RuleStartingCondition::afterLoad(const Mod* mod)
{
	mod->linkRule(_forbiddenArmorsInNextStage, _forbiddenArmorsInNextStageName);

	for (auto& pair : _craftTransformationsName)
	{
		auto* src = mod->getCraft(pair.first, true);
		auto* dest = mod->getCraft(pair.second, true);
		_craftTransformations[src] = dest;
	}

	//remove not needed data
	Collections::removeAll(_craftTransformationsName);
}

/**
 * Checks if the craft type is permitted.
 * @param craftType Craft type name.
 * @return True if permitted, false otherwise.
 */
bool RuleStartingCondition::isCraftPermitted(const std::string& craftType) const
{
	if (!_forbiddenCraft.empty())
	{
		return (std::find(_forbiddenCraft.begin(), _forbiddenCraft.end(), craftType) == _forbiddenCraft.end());
	}
	else if (!_allowedCraft.empty())
	{
		return (std::find(_allowedCraft.begin(), _allowedCraft.end(), craftType) != _allowedCraft.end());
	}
	return true;
}

/**
 * Checks if the soldier type is permitted.
 * @param soldierType Soldier type name.
 * @return True if permitted, false otherwise.
 */
bool RuleStartingCondition::isSoldierTypePermitted(const std::string& soldierType) const
{
	if (!_forbiddenSoldierTypes.empty())
	{
		return (std::find(_forbiddenSoldierTypes.begin(), _forbiddenSoldierTypes.end(), soldierType) == _forbiddenSoldierTypes.end());
	}
	else if (!_allowedSoldierTypes.empty())
	{
		return (std::find(_allowedSoldierTypes.begin(), _allowedSoldierTypes.end(), soldierType) != _allowedSoldierTypes.end());
	}
	return true;
}

/**
 * Gets the replacement armor.
 * @param soldierType Soldier type name.
 * @param armorType Existing/old armor type name.
 * @return Replacement armor type name (or empty string if no replacement is needed).
 */
std::string RuleStartingCondition::getArmorReplacement(const std::string& soldierType, const std::string& armorType) const
{
	bool allowed = true;
	if (!_forbiddenArmors.empty())
	{
		allowed = (std::find(_forbiddenArmors.begin(), _forbiddenArmors.end(), armorType) == _forbiddenArmors.end());
	}
	else if (!_allowedArmors.empty())
	{
		allowed = (std::find(_allowedArmors.begin(), _allowedArmors.end(), armorType) != _allowedArmors.end());
	}

	if (!allowed)
	{
		auto j = _defaultArmor.find(soldierType);
		if (j != _defaultArmor.end())
		{
			WeightedOptions w = WeightedOptions();
			for (auto& k : (j->second))
			{
				w.set(k.first, k.second);
			}
			std::string pick = w.choose();
			return pick == "noChange" ? "" : pick;
		}
	}

	return "";
}

/**
 * Gets the replacement craft.
 * @param sourceCraft Existing/old craft type.
 * @param mapScriptCraft Fallback craft type defined by map script (can be null).
 * @return Replacement craft type (or nullptr if no replacement is needed).
 */
const RuleCraft* RuleStartingCondition::getCraftReplacement(const RuleCraft* sourceCraft, const RuleCraft* mapScriptCraft) const
{
	if (!_craftTransformations.empty())
	{
		auto i = _craftTransformations.find(sourceCraft);
		if (i != _craftTransformations.end())
		{
			return i->second;
		}
	}

	return mapScriptCraft;
}

/**
 * Checks if the vehicle type is permitted.
 * @param vehicleType Vehicle type name.
 * @return True if permitted, false otherwise.
 */
bool RuleStartingCondition::isVehiclePermitted(const std::string& vehicleType) const
{
	if (!_forbiddenVehicles.empty())
	{
		return (std::find(_forbiddenVehicles.begin(), _forbiddenVehicles.end(), vehicleType) == _forbiddenVehicles.end());
	}
	else if (!_allowedVehicles.empty())
	{
		return (std::find(_allowedVehicles.begin(), _allowedVehicles.end(), vehicleType) != _allowedVehicles.end());
	}
	return true;
}

/**
 * Checks if the item type is permitted.
 * @param itemType Item type name.
 * @return True if permitted, false otherwise.
 */
bool RuleStartingCondition::isItemPermitted(const std::string& itemType, Mod* mod, Craft* craft) const
{
	bool itemCheckSubResult = true; // if both item lists are empty, item is OK

	if (!_forbiddenItems.empty())
	{
		if (std::find(_forbiddenItems.begin(), _forbiddenItems.end(), itemType) != _forbiddenItems.end())
		{
			return false; // item explicitly forbidden
		}
		else
		{
			itemCheckSubResult = true; // item is OK, unless category rules below say otherwise
		}
	}
	else if (!_allowedItems.empty())
	{
		if (std::find(_allowedItems.begin(), _allowedItems.end(), itemType) != _allowedItems.end())
		{
			return true; // item explicitly allowed
		}
		else
		{
			itemCheckSubResult = false; // item is NOT OK, unless category rules below say otherwise
		}
	}

	bool checkForbiddenCategories = !_forbiddenItemCategories.empty();
	bool checkAllowedCategories = !checkForbiddenCategories && !_allowedItemCategories.empty();

	bool categoryCheckSubResult = true;

	if (checkForbiddenCategories)
		categoryCheckSubResult = true;
	else if (checkAllowedCategories)
		categoryCheckSubResult = false;

	if (checkForbiddenCategories || checkAllowedCategories)
	{
		RuleItem* item = mod->getItem(itemType);
		if (item)
		{
			// primary categories
			std::vector<std::string> itemCategories = item->getCategories();

			// secondary categories ("inherited" from equipped ammo)
			if (mod->getShareAmmoCategories() && item->getBattleType() == BT_FIREARM)
			{
				for (auto* ammoRule : *item->getPrimaryCompatibleAmmo())
				{
					if (craft->getItems()->getItem(ammoRule) > 0)
					{
						for (auto& cat : ammoRule->getCategories())
						{
							itemCategories.push_back(cat);
						}
					}
				}
			}

			if (!itemCategories.empty())
			{
				// check all categories of the item
				for (const auto& catName : itemCategories)
				{
					if (checkForbiddenCategories)
					{
						if (std::find(_forbiddenItemCategories.begin(), _forbiddenItemCategories.end(), catName) != _forbiddenItemCategories.end())
						{
							return false; // found a category that is forbidden
						}
					}
					else if (checkAllowedCategories)
					{
						if (std::find(_allowedItemCategories.begin(), _allowedItemCategories.end(), catName) != _allowedItemCategories.end())
						{
							return true; // found a category that is allowed
						}
					}
				}
			}
		}
	}

	return itemCheckSubResult && categoryCheckSubResult;
}

}
