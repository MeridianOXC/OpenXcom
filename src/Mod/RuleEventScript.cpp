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
#include "RuleEventScript.h"
#include <climits>

namespace OpenXcom
{

/**
 * RuleEventScript: the (optional) rules for generating custom Geoscape events.
 * Each script element is independent, and the saved game will probe the list of these each month to determine what's going to happen.
 * Event scripts are executed just after the mission scripts.
 */
RuleEventScript::RuleEventScript(const std::string &type) :
	_type(type), _firstMonth(0), _lastMonth(-1), _executionOdds(100), _minDifficulty(0), _maxDifficulty(4),
	_minScore(INT_MIN), _maxScore(INT_MAX), _minFunds(INT64_MIN), _maxFunds(INT64_MAX),
	_counterMin(0), _counterMax(-1),
	_affectsGameProgression(false)
{
}

/**
 * Cleans up the event script ruleset.
 */
RuleEventScript::~RuleEventScript()
{
	for (auto& pair : _eventWeights)
	{
		delete pair.second;
	}
}

/**
 * Loads an event script from YAML.
 * @param node YAML node.
 */
void RuleEventScript::load(const YAML::YamlNodeReader& node)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("oneTimeSequentialEvents", _oneTimeSequentialEvents);
	if (reader["oneTimeRandomEvents"])
	{
		_oneTimeRandomEvents.load(reader["oneTimeRandomEvents"]);
	}
	for (const auto& monthWeights : reader["eventWeights"].children())
	{
		WeightedOptions *nw = new WeightedOptions();
		nw->load(monthWeights);
		_eventWeights.push_back(std::make_pair(monthWeights.readKey<size_t>(0), nw));
	}
	reader.tryRead("firstMonth", _firstMonth);
	reader.tryRead("lastMonth", _lastMonth);
	reader.tryRead("executionOdds", _executionOdds);
	reader.tryRead("minDifficulty", _minDifficulty);
	reader.tryRead("maxDifficulty", _maxDifficulty);
	reader.tryRead("minScore", _minScore);
	reader.tryRead("maxScore", _maxScore);
	reader.tryRead("minFunds", _minFunds);
	reader.tryRead("maxFunds", _maxFunds);
	reader.tryRead("missionVarName", _missionVarName);
	reader.tryRead("missionMarkerName", _missionMarkerName);
	{
		// deprecated, remove after July 2022
		reader.tryRead("missionMinRuns", _counterMin);
		reader.tryRead("missionMaxRuns", _counterMax);
	}
	reader.tryRead("counterMin", _counterMin);
	reader.tryRead("counterMax", _counterMax);

	reader.tryRead("researchTriggers", _researchTriggers);
	reader.tryRead("itemTriggers", _itemTriggers);
	reader.tryRead("facilityTriggers", _facilityTriggers);
	reader.tryRead("soldierTypeTriggers", _soldierTypeTriggers);
	reader.tryRead("xcomBaseInRegionTriggers", _xcomBaseInRegionTriggers);
	reader.tryRead("xcomBaseInCountryTriggers", _xcomBaseInCountryTriggers);
	reader.tryRead("pactCountryTriggers", _pactCountryTriggers);

	reader.tryRead("affectsGameProgression", _affectsGameProgression);
}

/**
 * Chooses one of the available events for this command.
 * @param monthsPassed The number of months that have passed in the game world.
 * @return The string id of the event.
 */
std::string RuleEventScript::generate(const size_t monthsPassed) const
{
	if (_eventWeights.empty())
		return std::string();

	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rw;
	rw = _eventWeights.rbegin();
	while (monthsPassed < rw->first)
		++rw;
	return rw->second->choose();
}

}
