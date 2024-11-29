#pragma once
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
#include <map>
#include <string>
#include "../Engine/Yaml.h"

namespace OpenXcom
{
class Mod;
class Armor;

struct EnvironmentalCondition
{
	int globalChance;
	int chancePerTurn;
	int firstTurn, lastTurn;
	std::string message;
	int color;
	std::string weaponOrAmmo;
	int side;
	int bodyPart;
	EnvironmentalCondition() : globalChance(100), chancePerTurn(0), firstTurn(1), lastTurn(1000), color(29), side(-1), bodyPart(-1) { /*Empty by Design*/ };
};

/**
 * Represents a specific Starting Condition.
 */
class RuleEnviroEffects
{
private:
	std::string _type;
	std::map<std::string, EnvironmentalCondition> _environmentalConditions;
	std::map<std::string, std::string> _paletteTransformations;
	std::map<std::string, std::string> _armorTransformationsName;
	std::map<const Armor*, Armor*> _armorTransformations;
	int _mapBackgroundColor;
	bool _ignoreAutoNightVisionUserSetting;
	std::string _inventoryShockIndicator;
	std::string _mapShockIndicator;
public:
	/// Creates a blank EnviroEffects ruleset.
	RuleEnviroEffects(const std::string& type);
	/// Cleans up the EnviroEffects ruleset.
	~RuleEnviroEffects();
	/// Loads EnviroEffects data from YAML.
	void load(const YAML::YamlNodeReader& reader, const Mod* mod);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);
	/// Gets the EnviroEffects's type.
	const std::string& getType() const { return _type; }
	/// Gets the palette transformations.
	const std::map<std::string, std::string>& getPaletteTransformations() const { return _paletteTransformations; }
	/// Gets the environmental condition for a given faction.
	EnvironmentalCondition getEnvironmetalCondition(const std::string& faction) const;
	/// Gets the transformed armor.
	Armor* getArmorTransformation(const Armor* sourceArmor) const;
	/// Gets the battlescape map background color.
	int getMapBackgroundColor() const { return _mapBackgroundColor; }
	/// Gets the corresponding flag.
	bool ignoreAutoNightVisionUserSetting() const { return _ignoreAutoNightVisionUserSetting; }
	/// Gets the inventory shock indicator sprite name.
	const std::string& getInventoryShockIndicator() const { return _inventoryShockIndicator; }
	/// Gets the map shock indicator sprite name.
	const std::string& getMapShockIndicator() const { return _mapShockIndicator; }
};

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, EnvironmentalCondition* val);

}
