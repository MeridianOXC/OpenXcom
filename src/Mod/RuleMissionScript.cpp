/*
 * Copyright 2010-2016 OpenXcom Developers.
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
#include "RuleMissionScript.h"
#include "../Engine/Exception.h"
#include "../Engine/RNG.h"
#include <climits>
#include <set>

namespace OpenXcom
{

/**
 * RuleMissionScript: the rules for the alien mission progression.
 * Each script element is independent, and the saved game will probe the list of these each month to determine what's going to happen.
 */
RuleMissionScript::RuleMissionScript(const std::string &type) :
	_type(type), _firstMonth(0), _lastMonth(-1), _label(0), _executionOdds(100),
	_targetBaseOdds(0), _minDifficulty(0), _maxDifficulty(4), _maxRuns(-1), _avoidRepeats(0), _delay(0), _randomDelay(0),
	_minScore(INT_MIN), _maxScore(INT_MAX), _minFunds(INT64_MIN), _maxFunds(INT64_MAX),
	_useTable(true), _siteType(false)
{
}

/**
 * Destructor, cleans up the mess we left in ram.
 */
RuleMissionScript::~RuleMissionScript()
{
	for (auto& pair : _missionWeights)
	{
		delete pair.second;
	}
	for (auto& pair : _raceWeights)
	{
		delete pair.second;
	}
	for (auto& pair : _regionWeights)
	{
		delete pair.second;
	}
}

/**
 * Loads a missionScript from a YML file.
 * @param node the node within the file we're reading.
 */
void RuleMissionScript::load(const YAML::YamlNodeReader& node)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("varName", _varName);
	reader.tryRead("firstMonth", _firstMonth);
	reader.tryRead("lastMonth", _lastMonth);
	reader.tryRead("label", _label);
	reader.tryRead("executionOdds", _executionOdds);
	reader.tryRead("targetBaseOdds", _targetBaseOdds);
	reader.tryRead("minDifficulty", _minDifficulty);
	reader.tryRead("maxDifficulty", _maxDifficulty);
	reader.tryRead("maxRuns", _maxRuns);
	reader.tryRead("avoidRepeats", _avoidRepeats);
	reader.tryRead("startDelay", _delay);
	reader.tryRead("randomDelay", _randomDelay);
	reader.tryRead("minScore", _minScore);
	reader.tryRead("maxScore", _maxScore);
	reader.tryRead("minFunds", _minFunds);
	reader.tryRead("maxFunds", _maxFunds);
	reader.tryRead("missionVarName", _missionVarName);
	reader.tryRead("missionMarkerName", _missionMarkerName);
	reader.tryRead("counterMin", _counterMin);
	reader.tryRead("counterMax", _counterMax);
	reader.tryRead("conditionals", _conditionals);
	reader.tryRead("adhocMissionScriptTags", _adhocMissionScriptTags);
	for (const auto& monthWeights : reader["missionWeights"].children())
	{
		WeightedOptions* nw = new WeightedOptions();
		nw->load(monthWeights);
		_missionWeights.push_back(std::make_pair(monthWeights.readKey<size_t>(0), nw));
	}
	for (const auto& monthWeights : reader["raceWeights"].children())
	{
		WeightedOptions* nw = new WeightedOptions();
		nw->load(monthWeights);
		_raceWeights.push_back(std::make_pair(monthWeights.readKey<size_t>(0), nw));
	}
	for (const auto& monthWeights : reader["regionWeights"].children())
	{
		WeightedOptions* nw = new WeightedOptions();
		nw->load(monthWeights);
		_regionWeights.push_back(std::make_pair(monthWeights.readKey<size_t>(0), nw));
	}

	reader.tryRead("researchTriggers", _researchTriggers);
	reader.tryRead("itemTriggers", _itemTriggers);
	reader.tryRead("facilityTriggers", _facilityTriggers);
	reader.tryRead("soldierTypeTriggers", _soldierTypeTriggers);
	reader.tryRead("xcomBaseInRegionTriggers", _xcomBaseInRegionTriggers);
	reader.tryRead("xcomBaseInCountryTriggers", _xcomBaseInCountryTriggers);
	reader.tryRead("pactCountryTriggers", _pactCountryTriggers);

	reader.tryRead("useTable", _useTable);
	if (_varName.empty() && (_maxRuns > 0 || _avoidRepeats > 0))
	{
		throw Exception("Error in mission script: " + _type +": no varName provided for a script with maxRuns or repeatAvoidance.");
	}

}

/**
 * Gets the name of this command.
 * @return the name of the command.
 */
const std::string& RuleMissionScript::getType() const
{
	return _type;
}

/**
 * @return the first month this script should run.
 */
int RuleMissionScript::getFirstMonth() const
{
	return _firstMonth;
}

/**
 * @return the last month this script should run.
 */
int RuleMissionScript::getLastMonth() const
{
	return _lastMonth;
}

/**
 * @return the label this command uses for conditional tracking.
 */
int RuleMissionScript::getLabel() const
{
	return _label;
}

/**
 * @return the odds of this command's execution.
 */
int RuleMissionScript::getExecutionOdds() const
{
	return _executionOdds;
}

/**
 * @return the odds of this command targetting a base.
 */
int RuleMissionScript::getTargetBaseOdds() const
{
	return _targetBaseOdds;
}

/**
 * @return the minimum difficulty for this script to run.
 */
int RuleMissionScript::getMinDifficulty() const
{
	return _minDifficulty;
}

/**
 * @return the maximum runs for scripts tracking our varName.
 */
int RuleMissionScript::getMaxRuns() const
{
	return _maxRuns;
}

/**
 * @return the number of sites to avoid repeating missions against.
 */
int RuleMissionScript::getRepeatAvoidance() const
{
	return _avoidRepeats;
}

/**
 * @return the fixed (or randomized) delay on spawning the first wave (if any) to override whatever's written in the mission definition.
 */
int RuleMissionScript::getDelay() const
{
	if (_randomDelay == 0)
		return _delay;
	else
		return _delay + RNG::generate(0, _randomDelay);
}

/**
 * @return the list of conditions that govern execution of this command.
 */
const std::vector<int> &RuleMissionScript::getConditionals() const
{
	return _conditionals;
}

/**
 * @return if this command uses a weighted distribution to pick a race.
 */
bool RuleMissionScript::hasRaceWeights() const
{
	return !_raceWeights.empty();
}

/**
 * @return if this command uses a weighted distribution to pick a mission.
 */
bool RuleMissionScript::hasMissionWeights() const
{
	return !_missionWeights.empty();
}

/**
 * @return if this command uses a weighted distribution to pick a region.
 */
bool RuleMissionScript::hasRegionWeights() const
{
	return !_regionWeights.empty();
}

/**
 * @return a list of research topics that govern execution of this script.
 */
const std::map<std::string, bool> &RuleMissionScript::getResearchTriggers() const
{
	return _researchTriggers;
}

/**
 * @return a list of item triggers that govern execution of this script.
 */
const std::map<std::string, bool> &RuleMissionScript::getItemTriggers() const
{
	return _itemTriggers;
}

/**
 * @return a list of facility triggers that govern execution of this script.
 */
const std::map<std::string, bool> &RuleMissionScript::getFacilityTriggers() const
{
	return _facilityTriggers;
}

/**
 * @return a list of xcom base triggers that govern execution of this script.
 */
const std::map<std::string, bool> &RuleMissionScript::getXcomBaseInRegionTriggers() const
{
	return _xcomBaseInRegionTriggers;
}

/**
 * @return a list of xcom base triggers that govern execution of this script.
 */
const std::map<std::string, bool> &RuleMissionScript::getXcomBaseInCountryTriggers() const
{
	return _xcomBaseInCountryTriggers;
}

/**
 * @return if this command should remove the mission it generates from the strategy table.
 */
bool RuleMissionScript::getUseTable() const
{
	return _useTable;
}

/**
 * @return the name of the variable we want to use to track in the saved game.
 */
std::string RuleMissionScript::getVarName() const
{
	return _varName;
}

/**
 * @return a complete, unique list of all the mission types this command could possibly generate.
 */
std::set<std::string> RuleMissionScript::getAllMissionTypes() const
{
	std::set<std::string> types;
	for (auto& pair : _missionWeights)
	{
		std::vector<std::string> names = pair.second->getNames();
		for (const auto& name : names)
		{
			types.insert(name);
		}
	}
	return types;
}

/**
 * @param month the month for which we want info.
 * @return a list of the possible missions for the given month.
 */
std::vector<std::string> RuleMissionScript::getMissionTypes(const int month) const
{
	std::vector<std::string> missions;
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rw = _missionWeights.rbegin();
	while (month < (int)(rw->first))
	{
		++rw;
		if (rw == _missionWeights.rend())
		{
			--rw;
			break;
		}
	}
	std::vector<std::string> names = rw->second->getNames();
	for (const auto& name : names)
	{
		missions.push_back(name);
	}
	return missions;
}

/**
 * @param month the month for which we want info.
 * @return the list of regions we have to pick from this month.
 */
std::vector<std::string> RuleMissionScript::getRegions(const int month) const
{
	std::vector<std::string> regions;
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rw = _regionWeights.rbegin();
	while (month < (int)(rw->first))
	{
		++rw;
		if (rw == _regionWeights.rend())
		{
			--rw;
			break;
		}
	}
	std::vector<std::string> names = rw->second->getNames();
	for (const auto& name : names)
	{
		regions.push_back(name);
	}
	return regions;
}

/**
 * Chooses one of the available races, regions, or missions for this command.
 * @param monthsPassed The number of months that have passed in the game world.
 * @param type the type of thing we want to generate, region, mission or race.
 * @return The string id of the thing.
 */
std::string RuleMissionScript::generate(const size_t monthsPassed, const GenerationType type) const
{
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rw;
	if (type == GEN_RACE)
		rw = _raceWeights.rbegin();
	else if (type == GEN_REGION)
		rw = _regionWeights.rbegin();
	else
		rw = _missionWeights.rbegin();
	while (monthsPassed < rw->first)
		++rw;
	return rw->second->choose();
}

/**
 * @param siteType set this command to be a missionSite type or not.
 */
void RuleMissionScript::setSiteType(const bool siteType)
{
	_siteType = siteType;
}

/**
 * @return if this is a mission site type command or not.
 */
bool RuleMissionScript::getSiteType() const
{
	return _siteType;
}

}
