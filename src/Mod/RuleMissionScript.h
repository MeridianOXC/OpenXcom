#pragma once
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
#include <string>
#include <vector>
#include <map>
#include "../Engine/Yaml.h"
#include "../Savegame/WeightedOptions.h"
#include <set>

namespace OpenXcom
{
enum GenerationType { GEN_REGION, GEN_MISSION, GEN_RACE };
class WeightedOptions;

class RuleMissionScript
{
private:
	std::string _type, _varName;
	int _firstMonth, _lastMonth, _label, _executionOdds, _targetBaseOdds, _minDifficulty, _maxDifficulty, _maxRuns, _avoidRepeats, _delay, _randomDelay;
	int _minScore, _maxScore;
	int64_t _minFunds, _maxFunds;
	std::string _missionVarName, _missionMarkerName;
	int _counterMin, _counterMax;
	std::vector<int> _conditionals;
	std::vector<std::string> _adhocMissionScriptTags;
	std::vector<std::pair<size_t, WeightedOptions*> > _regionWeights, _missionWeights, _raceWeights;

	std::map<std::string, bool> _researchTriggers;
	std::map<std::string, bool> _itemTriggers;
	std::map<std::string, bool> _facilityTriggers;
	std::map<std::string, bool> _soldierTypeTriggers;
	std::map<std::string, bool> _xcomBaseInRegionTriggers;
	std::map<std::string, bool> _xcomBaseInCountryTriggers;
	std::map<std::string, bool> _pactCountryTriggers;

	bool _useTable, _siteType;

public:
	/// Creates a new mission script.
	RuleMissionScript(const std::string &type);
	/// Deletes a mission script.
	~RuleMissionScript();
	/// Loads a mission script from yaml.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the name of the script command.
	const std::string& getType() const;
	/// Gets the name of the variable to use for keeping track of... things.
	std::string getVarName() const;
	/// Gets a complete and unique list of all the mission types contained within this command.
	std::set<std::string> getAllMissionTypes() const;
	/// Gets a list of the regions in this command's region weights for a given month.
	std::vector<std::string> getRegions(const int month) const;
	/// Gets a list of the mission types in this command's mission weights for a given month.
	std::vector<std::string> getMissionTypes(const int month) const;
	/// Gets the first month this command will run.
	int getFirstMonth() const;
	/// Gets the last month this command will run.
	int getLastMonth() const;
	/// Gets the label of this command, used for conditionals.
	int getLabel() const;
	/// Gets the odds of this command executing.
	int getExecutionOdds() const;
	/// Gets the odds of this mission targetting an xcom base.
	int getTargetBaseOdds() const;
	/// Gets the minimum difficulty for this command to run
	int getMinDifficulty() const;
	/// Gets the maximum difficulty for this command to run.
	int getMaxDifficulty() const { return _maxDifficulty; }
	/// Gets the maximum number of times to run a command with this varName
	int getMaxRuns() const;
	/// Gets how many previous mission sites to keep track of (to avoid using them again)
	int getRepeatAvoidance() const;
	/// Gets the number of minutes to delay spawning of the first wave of this mission, overrides the spawn delay defined in the mission waves.
	int getDelay() const;
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
	/// Gets the list of conditions this command requires in order to run.
	const std::vector<int> &getConditionals() const;
	/// Gets the list of tags for this command.
	const std::vector<std::string> &getAdhocMissionScriptTags() const { return _adhocMissionScriptTags; }
	/// Does this command have raceWeights?
	bool hasRaceWeights() const;
	/// Does this command have mission weights?
	bool hasMissionWeights() const;
	/// Does this command have region weights?
	bool hasRegionWeights() const;

	/// Gets the research triggers that may apply to this command.
	const std::map<std::string, bool> &getResearchTriggers() const;
	/// Gets the item triggers that may apply to this command.
	const std::map<std::string, bool> &getItemTriggers() const;
	/// Gets the facility triggers that may apply to this command.
	const std::map<std::string, bool> &getFacilityTriggers() const;
	/// Gets the soldier type triggers that may apply to this command.
	const std::map<std::string, bool> &getSoldierTypeTriggers() const { return _soldierTypeTriggers; }
	/// Gets the xcom base triggers that may apply to this command.
	const std::map<std::string, bool> &getXcomBaseInRegionTriggers() const;
	/// Gets the xcom base triggers that may apply to this command.
	const std::map<std::string, bool> &getXcomBaseInCountryTriggers() const;
	/// Gets the country pact triggers that may apply to this command.
	const std::map<std::string, bool> &getPactCountryTriggers() const { return _pactCountryTriggers; }

	/// Delete this mission from the table? stops it coming up again in random selection, but NOT if a missionScript calls it by name.
	bool getUseTable() const;
	/// Sets this script to a terror mission type command or not.
	void setSiteType(const bool siteType);
	/// Checks if this is a terror-type mission or not.
	bool getSiteType() const;
	/// Generates either a region, a mission, or a race based on the month.
	std::string generate(const size_t monthsPassed, const GenerationType type) const;

};

}
