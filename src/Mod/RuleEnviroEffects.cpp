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
void RuleEnviroEffects::load(const YAML::YamlNodeReader& reader, const Mod* mod)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	reader.tryRead("environmentalConditions", _environmentalConditions);
	mod->loadUnorderedNamesToNames(_type, _paletteTransformations, reader["paletteTransformations"]);
	mod->loadUnorderedNamesToNames(_type, _armorTransformationsName, reader["armorTransformations"]);
	reader.tryRead("mapBackgroundColor", _mapBackgroundColor);
	reader.tryRead("ignoreAutoNightVisionUserSetting", _ignoreAutoNightVisionUserSetting);
	reader.tryRead("inventoryShockIndicator", _inventoryShockIndicator);
	reader.tryRead("mapShockIndicator", _mapShockIndicator);
}

/**
 * Cross link with other rules.
 */
void RuleEnviroEffects::afterLoad(const Mod* mod)
{
	for (auto& pair : _armorTransformationsName)
	{
		auto* src = mod->getArmor(pair.first, true);
		auto* dest = mod->getArmor(pair.second, true);
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
		auto i = _environmentalConditions.find(faction);
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
		auto i = _armorTransformations.find(sourceArmor);
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

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, EnvironmentalCondition* val)
{
	YAML::YamlNodeReader reader(nullptr, n);
	reader.tryRead("globalChance", val->globalChance);
	reader.tryRead("chancePerTurn", val->chancePerTurn);
	reader.tryRead("firstTurn", val->firstTurn);
	reader.tryRead("lastTurn", val->lastTurn);
	reader.tryRead("message", val->message);
	reader.tryRead("color", val->color);
	reader.tryRead("weaponOrAmmo", val->weaponOrAmmo);
	reader.tryRead("side", val->side);
	reader.tryRead("bodyPart", val->bodyPart);
	return true;
}

}
