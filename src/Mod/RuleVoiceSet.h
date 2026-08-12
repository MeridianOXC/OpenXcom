#pragma once
/*
 * Copyright 2010-2026 OpenXcom Developers.
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
#include <string>
#include <vector>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

class Mod;
class RuleSoldier;
class Unit;
class Armor;

/**
 * Represents a voice set.
 */
class RuleVoiceSet
{
private:
	std::string _type;
	std::vector<int> _selectUnitSound;
	std::vector<int> _startMovingSound;
	std::vector<int> _selectWeaponSound;
	std::vector<int> _annoyedSound;

	std::vector<int> _allowedGenders;
	std::vector<std::string> _allowedSoldierNames;
	std::vector<const RuleSoldier*> _allowedSoldiers;
	std::vector<std::string> _allowedUnitNames;
	std::vector<const Unit*> _allowedUnits;
	std::vector<std::string> _allowedArmorNames;
	std::vector<const Armor*> _allowedArmors;

	int _listOrder;
public:
	/// Creates a blank voice set ruleset.
	RuleVoiceSet(const std::string& type, int listOrder);
	/// Cleans up the voice set ruleset.
	~RuleVoiceSet();
	/// Loads ruleset from YAML.
	void load(const YAML::YamlNodeReader& reader, Mod* mod);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);
	/// Gets the voice set type.
	const std::string& getType() const { return _type; }
	/// Gets the "select unit" sounds.
	const std::vector<int>& getSelectUnitSounds() const { return _selectUnitSound; }
	/// Gets the "start moving" sounds.
	const std::vector<int>& getStartMovingSounds() const { return _startMovingSound; }
	/// Gets the "select weapon" sounds.
	const std::vector<int>& getSelectWeaponSounds() const { return _selectWeaponSound; }
	/// Gets the "annoyed" sounds.
	const std::vector<int>& getAnnoyedSounds() const { return _annoyedSound; }

	/// Gets the allowed genders.
	const std::vector<int> &getAllowedGenders() const { return _allowedGenders; }
	/// Gets the allowed soldier types.
	const std::vector<const RuleSoldier*> &getAllowedSoldiers() const { return _allowedSoldiers; }
	/// Gets the allowed unit types.
	const std::vector<const Unit*> &getAllowedUnits() const { return _allowedUnits; }
	/// Gets the allowed armor types.
	const std::vector<const Armor*> &getAllowedArmors() const { return _allowedArmors; }

	/// Gets the voice set's list weight.
	int getListOrder() const { return _listOrder; }
};

}
