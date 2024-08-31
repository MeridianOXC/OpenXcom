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
#include "RuleAlienMission.h"
#include "../Savegame/WeightedOptions.h"

namespace YAML
{
	template<>
	struct convert<OpenXcom::MissionWave>
	{
		static Node encode(const OpenXcom::MissionWave& rhs)
		{
			Node node;
			node["ufo"] = rhs.ufoType;
			node["count"] = rhs.ufoCount;
			node["trajectory"] = rhs.trajectory;
			node["timer"] = rhs.spawnTimer;
			node["objective"] = rhs.objective;
			node["objectiveOnTheLandingSite"] = rhs.objectiveOnTheLandingSite;
			node["objectiveOnXcomBase"] = rhs.objectiveOnXcomBase;
			node["hunterKillerPercentage"] = rhs.hunterKillerPercentage;
			node["huntMode"] = rhs.huntMode;
			node["huntBehavior"] = rhs.huntBehavior;
			node["escort"] = rhs.escort;
			node["interruptPercentage"] = rhs.interruptPercentage;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::MissionWave& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.ufoType = node["ufo"].as<std::string>();
			rhs.ufoCount = node["count"].as<size_t>();
			rhs.trajectory = node["trajectory"].as<std::string>();
			rhs.spawnTimer = node["timer"].as<size_t>();
			rhs.objective = node["objective"].as<bool>(false);
			rhs.objectiveOnTheLandingSite = node["objectiveOnTheLandingSite"].as<bool>(false);
			rhs.objectiveOnXcomBase = node["objectiveOnXcomBase"].as<bool>(false);
			rhs.hunterKillerPercentage = node["hunterKillerPercentage"].as<int>(-1);
			rhs.huntMode = node["huntMode"].as<int>(-1);
			rhs.huntBehavior = node["huntBehavior"].as<int>(-1);
			rhs.escort = node["escort"].as<bool>(false);
			rhs.interruptPercentage = node["interruptPercentage"].as<int>(0);
			return true;
		}
	};
}

namespace OpenXcom
{

RuleAlienMission::RuleAlienMission(const std::string &type) :
	_type(type), _skipScoutingPhase(false), _points(0), _objective(OBJECTIVE_SCORE), _spawnZone(-1),
	_retaliationOdds(-1), _endlessInfiltration(true), _multiUfoRetaliation(false), _multiUfoRetaliationExtra(false),
	_ignoreBaseDefenses(false), _despawnEvenIfTargeted(false), _respawnUfoAfterSiteDespawn(false), _showAlienBase(false),
	_operationType(AMOT_SPACE), _operationSpawnZone(-1),
	_targetBaseOdds(0)
{
}

/**
 * Ensures the allocated memory is released.
 */
RuleAlienMission::~RuleAlienMission()
{
	for (auto& pair : _raceDistribution)
	{
		delete pair.second;
	}
	for (auto& pair : _regionWeights)
	{
		delete pair.second;
	}
}

/**
 * Loads the mission data from a YAML node.
 * @param node YAML node.
 */
void RuleAlienMission::load(const YAML::Node &node)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent);
	}

	_points = node["points"].as<int>(_points);
	_waves = node["waves"].as< std::vector<MissionWave> >(_waves);
	_objective = (MissionObjective)node["objective"].as<int>(_objective);
	_spawnUfo = node["spawnUfo"].as<std::string>(_spawnUfo);
	_skipScoutingPhase = node["skipScoutingPhase"].as<bool>(_skipScoutingPhase);
	_spawnZone = node["spawnZone"].as<int>(_spawnZone);
	_weights = node["missionWeights"].as< std::map<size_t, int> >(_weights);
	_retaliationOdds = node["retaliationOdds"].as<int>(_retaliationOdds);
	_endlessInfiltration = node["endlessInfiltration"].as<bool>(_endlessInfiltration);
	_multiUfoRetaliation = node["multiUfoRetaliation"].as<bool>(_multiUfoRetaliation);
	_multiUfoRetaliationExtra = node["multiUfoRetaliationExtra"].as<bool>(_multiUfoRetaliationExtra);
	if (_multiUfoRetaliationExtra)
	{
		// yes, I should have changed _multiUfoRetaliation to int instead, but I'm lazy
		// I also don't want to break existing mods or handle it in a complicated way
		_multiUfoRetaliation = true;
	}
	_ignoreBaseDefenses = node["ignoreBaseDefenses"].as<bool>(_ignoreBaseDefenses);
	_despawnEvenIfTargeted = node["despawnEvenIfTargeted"].as<bool>(_despawnEvenIfTargeted);
	_respawnUfoAfterSiteDespawn = node["respawnUfoAfterSiteDespawn"].as<bool>(_respawnUfoAfterSiteDespawn);
	_showAlienBase = node["showAlienBase"].as<bool>(_showAlienBase);
	_interruptResearch = node["interruptResearch"].as<std::string>(_interruptResearch);
	_siteType = node["siteType"].as<std::string>(_siteType);
	_operationType = (AlienMissionOperationType)node["operationType"].as<int>(_operationType);
	_operationSpawnZone = node["operationSpawnZone"].as<int>(_operationSpawnZone);
	_operationBaseType = node["operationBaseType"].as<std::string>(_operationBaseType);
	_targetBaseOdds = node["targetBaseOdds"].as<int>(_targetBaseOdds);

	if (const YAML::Node &regWeights = node["regionWeights"])
	{
		for (YAML::const_iterator nn = regWeights.begin(); nn != regWeights.end(); ++nn)
		{
			WeightedOptions *nw = new WeightedOptions();
			nw->load(nn->second);
			_regionWeights.push_back(std::make_pair(nn->first.as<size_t>(0), nw));
		}
	}

	//Only allow full replacement of mission racial distribution.
	if (const YAML::Node &weights = node["raceWeights"])
	{
		typedef std::map<size_t, WeightedOptions*> Associative;
		typedef std::vector< std::pair<size_t, WeightedOptions*> > Linear;

		Associative assoc;
		//Place in the associative container so we can index by month and keep entries sorted.
		for (Linear::iterator ii = _raceDistribution.begin(); ii != _raceDistribution.end(); ++ii)
		{
			assoc.insert(*ii);
		}

		// Now go through the node contents and merge with existing data.
		for (YAML::const_iterator nn = weights.begin(); nn != weights.end(); ++nn)
		{
			size_t month = nn->first.as<size_t>();
			Associative::iterator existing = assoc.find(month);
			if (assoc.end() == existing)
			{
				// New entry, load and add it.
				WeightedOptions *nw = new WeightedOptions();
				nw->load(nn->second);
				assoc.insert(std::make_pair(month, nw));
			}
			else
			{
				// Existing entry, update it.
				existing->second->load(nn->second);
			}
		}

		// Now replace values in our actual member variable!
		_raceDistribution.clear();
		_raceDistribution.reserve(assoc.size());
		for (Associative::iterator ii = assoc.begin(); ii != assoc.end(); ++ii)
		{
			if (ii->second->empty())
			{
				// Don't keep empty lists.
				delete ii->second;
			}
			else
			{
				// Place it
				_raceDistribution.push_back(*ii);
			}
		}
	}
}

/**
 * @return if this mission uses a weighted distribution to pick a race.
 */
bool RuleAlienMission::hasRaceWeights() const
{
	return !_raceDistribution.empty();
}

/**
 * Chooses one of the available races for this mission.
 * The racial distribution may vary based on the current game date.
 * @param monthsPassed The number of months that have passed in the game world.
 * @return The string id of the race.
 */
std::string RuleAlienMission::generateRace(const size_t monthsPassed) const
{
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rc;
	for (rc = _raceDistribution.rbegin(); rc != _raceDistribution.rend() && monthsPassed < rc->first; ++rc);
	if (rc == _raceDistribution.rend())
		return "";
	return rc->second->choose();
}

/**
 * Returns the Alien score for this mission.
 * @return Amount of points.
 */
int RuleAlienMission::getPoints() const
{
	return _points;
}

/**
 * Returns the chances of this mission being generated based on the current game date.
 * @param monthsPassed The number of months that have passed in the game world.
 * @return The weight.
 */
int RuleAlienMission::getWeight(const size_t monthsPassed) const
{
	if (_weights.empty())
	{
		return 1;
	}
	int weight = 0;
	for (auto& pair : _weights)
	{
		if (pair.first > monthsPassed)
		{
			break;
		}
		weight = pair.second;
	}
	return weight;
}

/**
 * Returns the Alien score for this mission.
 * @return Amount of points.
 */
int RuleAlienMission::getRetaliationOdds() const
{
	return _retaliationOdds;
}

/**
 * Should the infiltration end after first cycle or continue indefinitely?
 * @return True, if infiltration should continue indefinitely.
 */
bool RuleAlienMission::isEndlessInfiltration() const
{
	return _endlessInfiltration;
}

/**
 * Does this mission have region weights?
 * @return if this mission uses a weighted distribution to pick a region.
 */
bool RuleAlienMission::hasRegionWeights() const
{
	return !_regionWeights.empty();
}

/**
 * Chooses one of the available regions for this mission.
 * The region distribution may vary based on the current game date.
 * @param monthsPassed The number of months that have passed in the game world.
 * @return The string id of the region.
 */
std::string RuleAlienMission::generateRegion(const size_t monthsPassed) const
{
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rc = _regionWeights.rbegin();
	while (monthsPassed < rc->first)
		++rc;
	return rc->second->choose();
}

}
