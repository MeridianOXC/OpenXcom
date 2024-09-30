#pragma once
/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

class Mod;
class RuleItem;

/**
 * Represents a weapon set.
 */
class RuleWeaponSet
{
private:
	std::string _type;
	std::vector<std::string> _weaponNames;
	std::vector<const RuleItem*> _weapons;
public:
	/// Creates a blank weapon set ruleset.
	RuleWeaponSet(const std::string& type);
	/// Cleans up the weapon set ruleset.
	~RuleWeaponSet();
	/// Loads ruleset from YAML.
	void load(const YAML::Node& node, Mod* mod);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);
	/// Gets the weapon set type.
	const std::string& getType() const { return _type; }
	/// Gets the weapon set content.
	const std::vector<const RuleItem*>& getWeapons() const { return _weapons; }

};

}
