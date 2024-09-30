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
#include "RuleWeaponSet.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a weapon set.
 * @param type String defining the weapon set.
 */
RuleWeaponSet::RuleWeaponSet(const std::string& type) : _type(type)
{
}

/**
 *
 */
RuleWeaponSet::~RuleWeaponSet()
{
}

/**
 * Loads the weapon set from a YAML file.
 * @param node YAML node.
 * @param mod Mod handle.
 */
void RuleWeaponSet::load(const YAML::Node& node, Mod* mod)
{
	if (const YAML::Node& parent = node["refNode"])
	{
		load(parent, mod);
	}

	mod->loadUnorderedNames(_type, _weaponNames, node["weapons"]);
}

/**
 * Cross link with other Rules.
 */
void RuleWeaponSet::afterLoad(const Mod* mod)
{
	mod->linkRule(_weapons, _weaponNames);
}

}
