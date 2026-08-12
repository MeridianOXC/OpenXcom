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
#include "RuleVoiceSet.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a voice set.
 * @param type String defining the voice set.
 */
RuleVoiceSet::RuleVoiceSet(const std::string& type, int listOrder) : _type(type), _listOrder(listOrder)
{
}

/**
 *
 */
RuleVoiceSet::~RuleVoiceSet()
{
}

/**
 * Loads the voice set from a YAML file.
 * @param node YAML node.
 * @param mod Mod handle.
 */
void RuleVoiceSet::load(const YAML::YamlNodeReader& reader, Mod* mod)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	mod->loadSoundOffset(_type, _selectUnitSound, reader["selectUnit"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _startMovingSound, reader["startMoving"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _selectWeaponSound, reader["selectWeapon"], "BATTLE.CAT");
	mod->loadSoundOffset(_type, _annoyedSound, reader["annoyed"], "BATTLE.CAT");

	mod->loadUnorderedInts(_type, _allowedGenders, reader["allowedGenders"]);
	mod->loadUnorderedNames(_type, _allowedSoldierNames, reader["allowedSoldierTypes"]);
	mod->loadUnorderedNames(_type, _allowedUnitNames, reader["allowedUnitTypes"]);
	mod->loadUnorderedNames(_type, _allowedArmorNames, reader["allowedArmorTypes"]);

	reader.tryRead("listOrder", _listOrder);
}

/**
 * Cross link with other Rules.
 */
void RuleVoiceSet::afterLoad(const Mod* mod)
{
	mod->linkRule(_allowedSoldiers, _allowedSoldierNames);
	mod->linkRule(_allowedUnits, _allowedUnitNames);
	mod->linkRule(_allowedArmors, _allowedArmorNames);
}

}
