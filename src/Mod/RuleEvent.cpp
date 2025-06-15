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
#include "RuleEvent.h"
#include "Mod.h"

namespace OpenXcom
{

RuleEvent::RuleEvent(const std::string &name) :
	_name(name), _background("BACK13.SCR"), _alignBottom(false),
	_city(false), _points(0), _funds(0), _spawnedPersons(0), _timer(30), _timerRandom(0), _invert(false)
{
}

/**
 * Loads the event definition from YAML.
 * @param node YAML node.
 */
void RuleEvent::load(const YAML::YamlNodeReader& node)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("description", _description);
	reader.tryRead("alignBottom", _alignBottom);
	reader.tryRead("background", _background);
	reader.tryRead("music", _music);
	reader.tryRead("cutscene", _cutscene);
	reader.tryRead("regionList", _regionList);
	reader.tryRead("city", _city);
	reader.tryRead("points", _points);
	reader.tryRead("funds", _funds);
	reader.tryRead("spawnedCraftType", _spawnedCraftType);
	reader.tryRead("spawnedPersons", _spawnedPersons);
	reader.tryRead("spawnedPersonType", _spawnedPersonType);
	reader.tryRead("spawnedPersonName", _spawnedPersonName);
	if (reader["spawnedSoldier"])
	{
		_spawnedSoldier = reader["spawnedSoldier"].emitDescendants(YAML::YamlRootNodeReader(_spawnedSoldier, "(spawned soldier template)"));
	}
	reader.tryRead("everyMultiItemList", _everyMultiItemList);
	reader.tryRead("everyItemList", _everyItemList);
	reader.tryRead("randomItemList", _randomItemList);
	reader.tryRead("randomMultiItemList", _randomMultiItemList);
	if (reader["weightedItemList"])
	{
		_weightedItemList.load(reader["weightedItemList"]);
	}
	reader.tryRead("researchList", _researchNames);
	reader.tryRead("adhocMissionScriptTags", _adhocMissionScriptTags);
	reader.tryRead("interruptResearch", _interruptResearch);
	reader.tryRead("timer", _timer);
	reader.tryRead("timerRandom", _timerRandom);
	reader.tryRead("invert", _invert);

	reader.tryRead("everyMultiSoldierList", _everyMultiSoldierList);
	reader.tryRead("randomMultiSoldierList", _randomMultiSoldierList);
}

/**
 * Cross link with other Rules.
 */
void RuleEvent::afterLoad(const Mod* mod)
{
	mod->linkRule(_research, _researchNames);
}

}
