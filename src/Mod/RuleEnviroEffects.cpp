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
#include "RuleEnviroEffects.h"
#include "../Engine/Collections.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"

namespace YAML
{
	template<>
	struct convert<OpenXcom::EnvironmentalCondition>
	{
		static Node encode(const OpenXcom::EnvironmentalCondition& rhs)
		{
			Node node;
			node["globalChance"] = rhs.globalChance;
			node["chancePerTurn"] = rhs.chancePerTurn;
			node["firstTurn"] = rhs.firstTurn;
			node["lastTurn"] = rhs.lastTurn;
			node["message"] = rhs.message;
			node["color"] = rhs.color;
			node["weaponOrAmmo"] = rhs.weaponOrAmmo;
			node["side"] = rhs.side;
			node["bodyPart"] = rhs.bodyPart;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::EnvironmentalCondition& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.globalChance = node["globalChance"].as<int>(rhs.globalChance);
			rhs.chancePerTurn = node["chancePerTurn"].as<int>(rhs.chancePerTurn);
			rhs.firstTurn = node["firstTurn"].as<int>(rhs.firstTurn);
			rhs.lastTurn = node["lastTurn"].as<int>(rhs.lastTurn);
			rhs.message = node["message"].as<std::string>(rhs.message);
			rhs.color = node["color"].as<int>(rhs.color);
			rhs.weaponOrAmmo = node["weaponOrAmmo"].as<std::string>(rhs.weaponOrAmmo);
			rhs.side = node["side"].as<int>(rhs.side);
			rhs.bodyPart = node["bodyPart"].as<int>(rhs.bodyPart);
			return true;
		}
	};
}

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of EnviroEffects.
 * @param type String defining the type.
 */
RuleEnviroEffects::RuleEnviroEffects(const std::string& type) : _type(type), _mapBackgroundColor(15), _ignoreAutoNightVisionUserSetting(false)
{
}

/**
 *
 */
RuleEnviroEffects::~RuleEnviroEffects()
{
}

/**
 * Loads the EnviroEffects from a YAML file.
 * @param node YAML node.
 */
void RuleEnviroEffects::load(const YAML::Node& node, const Mod* mod)
{
	if (const YAML::Node& parent = node["refNode"])
	{
		load(parent, mod);
	}
	_type = node["type"].as<std::string>(_type);
	_environmentalConditions = node["environmentalConditions"].as< std::map<std::string, EnvironmentalCondition> >(_environmentalConditions);
	mod->loadUnorderedNamesToNames(_type, _paletteTransformations, node["paletteTransformations"]);
	mod->loadUnorderedNamesToNames(_type, _armorTransformationsName, node["armorTransformations"]);
	_mapBackgroundColor = node["mapBackgroundColor"].as<int>(_mapBackgroundColor);
	_ignoreAutoNightVisionUserSetting = node["ignoreAutoNightVisionUserSetting"].as<bool>(_ignoreAutoNightVisionUserSetting);
	_inventoryShockIndicator = node["inventoryShockIndicator"].as<std::string>(_inventoryShockIndicator);
	_mapShockIndicator = node["mapShockIndicator"].as<std::string>(_mapShockIndicator);
}

/**
 * Cross link with other rules.
 */
void RuleEnviroEffects::afterLoad(const Mod* mod)
{
	for (auto& pair : _armorTransformationsName)
	{
		auto src = mod->getArmor(pair.first, true);
		auto dest = mod->getArmor(pair.second, true);
		_armorTransformations[src] = dest;
	}

	//remove not needed data
	Collections::removeAll(_armorTransformationsName);
}

/**
 * Gets the environmental condition for a given faction.
 * @param faction Faction code (STR_FRIENDLY, STR_HOSTILE or STR_NEUTRAL).
 * @return Environmental condition definition.
 */
EnvironmentalCondition RuleEnviroEffects::getEnvironmetalCondition(const std::string& faction) const
{
	if (!_environmentalConditions.empty())
	{
		std::map<std::string, EnvironmentalCondition>::const_iterator i = _environmentalConditions.find(faction);
		if (i != _environmentalConditions.end())
		{
			return i->second;
		}
	}

	return EnvironmentalCondition();
}

/**
 * Gets the transformed armor.
 * @param sourceArmor Existing/old armor type.
 * @return Transformed armor type (or null if there is no transformation).
 */
Armor* RuleEnviroEffects::getArmorTransformation(const Armor* sourceArmor) const
{
	if (!_armorTransformations.empty())
	{
		std::map<const Armor*, Armor*>::const_iterator i = _armorTransformations.find(sourceArmor);
		if (i != _armorTransformations.end())
		{
			// cannot switch into a bigger armor size!
			if (sourceArmor->getSize() >= i->second->getSize())
			{
				return i->second;
			}
		}
	}

	return nullptr;
}

}
