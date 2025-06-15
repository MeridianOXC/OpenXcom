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
#include "RuleArcScript.h"
#include <climits>

namespace OpenXcom
{

/**
 * RuleArcScript: the (optional) rules for the high-level game progression.
 * Each script element is independent, and the saved game will probe the list of these each month to determine what's going to happen.
 * Arc scripts are executed just before the mission scripts and unlock research topics that can then be used by mission scripts.
 */
RuleArcScript::RuleArcScript(const std::string& type) :
	_type(type), _firstMonth(0), _lastMonth(-1), _executionOdds(100), _maxArcs(-1), _minDifficulty(0), _maxDifficulty(4),
	_minScore(INT_MIN), _maxScore(INT_MAX), _minFunds(INT64_MIN), _maxFunds(INT64_MAX)
{
}

/**
 *
 */
RuleArcScript::~RuleArcScript()
{
}

/**
 * Loads an arcScript from a YML file.
 * @param node the node within the file we're reading.
 */
void RuleArcScript::load(const YAML::YamlNodeReader& node)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("sequentialArcs", _sequentialArcs);
	if (reader["randomArcs"])
	{
		_randomArcs.load(reader["randomArcs"]);
	}
	reader.tryRead("firstMonth", _firstMonth);
	reader.tryRead("lastMonth", _lastMonth);
	reader.tryRead("executionOdds", _executionOdds);
	reader.tryRead("maxArcs", _maxArcs);
	reader.tryRead("minDifficulty", _minDifficulty);
	reader.tryRead("maxDifficulty", _maxDifficulty);
	reader.tryRead("minScore", _minScore);
	reader.tryRead("maxScore", _maxScore);
	reader.tryRead("minFunds", _minFunds);
	reader.tryRead("maxFunds", _maxFunds);
	reader.tryRead("missionVarName", _missionVarName);
	reader.tryRead("missionMarkerName", _missionMarkerName);
	reader.tryRead("counterMin", _counterMin);
	reader.tryRead("counterMax", _counterMax);

	reader.tryRead("researchTriggers", _researchTriggers);
	reader.tryRead("itemTriggers", _itemTriggers);
	reader.tryRead("facilityTriggers", _facilityTriggers);
	reader.tryRead("soldierTypeTriggers", _soldierTypeTriggers);
	reader.tryRead("xcomBaseInRegionTriggers", _xcomBaseInRegionTriggers);
	reader.tryRead("xcomBaseInCountryTriggers", _xcomBaseInCountryTriggers);
	reader.tryRead("pactCountryTriggers", _pactCountryTriggers);
}

}
