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
#include <string>
#include <vector>
#include <map>
#include "../Engine/Yaml.h"
#include "../Savegame/WeightedOptions.h"

namespace OpenXcom
{

class RuleArcScript
{
private:
	std::string _type;
	std::vector<std::string> _sequentialArcs;
	WeightedOptions _randomArcs;
	int _firstMonth, _lastMonth, _executionOdds, _maxArcs, _minDifficulty, _maxDifficulty;
	int _minScore, _maxScore;
	int64_t _minFunds, _maxFunds;
	std::string _missionVarName, _missionMarkerName;
	int _counterMin, _counterMax;

	std::map<std::string, bool> _researchTriggers;
	std::map<std::string, bool> _itemTriggers;
	std::map<std::string, bool> _facilityTriggers;
	std::map<std::string, bool> _soldierTypeTriggers;
	std::map<std::string, bool> _xcomBaseInRegionTriggers;
	std::map<std::string, bool> _xcomBaseInCountryTriggers;
	std::map<std::string, bool> _pactCountryTriggers;

public:
	/// Creates a new arc script.
	RuleArcScript(const std::string& type);
	/// Deletes an arc script.
	~RuleArcScript();
	/// Loads an arc script from yaml.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the name of the script command.
	const std::string &getType() const { return _type; }
	/// Gets the sequential arcs list.
	const std::vector<std::string> &getSequentialArcs() const { return _sequentialArcs; }
	/// Gets the random arcs weighted list.
	const WeightedOptions &getRandomArcs() const { return _randomArcs; }
	/// Gets the first month this command will run.
	int getFirstMonth() const { return _firstMonth; }
	/// Gets the last month this command will run.
	int getLastMonth() const { return _lastMonth; }
	/// Gets the odds of this command executing.
	int getExecutionOdds() const { return _executionOdds; }
	/// Gets the maximum number of arcs this command can generate (during the whole game).
	int getMaxArcs() const { return _maxArcs; }
	/// Gets the minimum difficulty for this command to run.
	int getMinDifficulty() const { return _minDifficulty; }
	/// Gets the maximum difficulty for this command to run.
	int getMaxDifficulty() const { return _maxDifficulty; }
	/// Gets the minimum score (from last month) for this command to run.
	int getMinScore() const { return _minScore; }
	/// Gets the maximum score (from last month) for this command to run.
	int getMaxScore() const { return _maxScore; }
	/// Gets the minimum funds (from current month) for this command to run.
	int64_t getMinFunds() const { return _minFunds; }
	/// Gets the maximum funds (from current month) for this command to run.
	int64_t getMaxFunds() const { return _maxFunds; }
	/// Gets the name of the mission script tracking variable.
	const std::string& getMissionVarName() const { return _missionVarName; }
	/// Gets the name of the mission marker tracking variable.
	const std::string& getMissionMarkerName() const { return _missionMarkerName; }
	/// Gets the minimum number of missions generated for this command to run.
	int getCounterMin() const { return _counterMin; }
	/// Gets the maximum number of missions generated for this command to run.
	int getCounterMax() const { return _counterMax; }

	/// Gets the research triggers that may apply to this command.
	const std::map<std::string, bool> &getResearchTriggers() const { return _researchTriggers; }
	/// Gets the item triggers that may apply to this command.
	const std::map<std::string, bool> &getItemTriggers() const { return _itemTriggers; }
	/// Gets the facility triggers that may apply to this command.
	const std::map<std::string, bool> &getFacilityTriggers() const { return _facilityTriggers; }
	/// Gets the soldier type triggers that may apply to this command.
	const std::map<std::string, bool> &getSoldierTypeTriggers() const { return _soldierTypeTriggers; }
	/// Gets the xcom base triggers that may apply to this command.
	const std::map<std::string, bool> &getXcomBaseInRegionTriggers() const { return _xcomBaseInRegionTriggers; }
	/// Gets the xcom base triggers that may apply to this command.
	const std::map<std::string, bool> &getXcomBaseInCountryTriggers() const { return _xcomBaseInCountryTriggers; }
	/// Gets the country pact triggers that may apply to this command.
	const std::map<std::string, bool> &getPactCountryTriggers() const { return _pactCountryTriggers; }

};

}
