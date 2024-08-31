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
#include <vector>
#include <map>
#include <string>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

enum AlienMissionOperationType {
	AMOT_SPACE,
	AMOT_REGION_EXISTING_BASE,
	AMOT_REGION_NEW_BASE,
	AMOT_REGION_NEW_BASE_IF_NECESSARY,
	AMOT_EARTH_EXISTING_BASE,
	AMOT_EARTH_NEW_BASE_IF_NECESSARY,
	AMOT_EXISTING_BASE_HUNT_MISSION
};

class WeightedOptions;

/**
 * @brief Information about a mission wave.
 * Mission waves control the UFOs that will be generated during an alien mission.
 */
struct MissionWave
{
	/// The type of the spawned UFOs.
	std::string ufoType;
	/// The number of UFOs that will be generated.
	/**
	 * The UFOs are generated sequentially, one every @a spawnTimer minutes.
	 */
	size_t ufoCount;
	/// The trajectory ID for this wave's UFOs.
	/**
	 * Trajectories control the way UFOs fly around the Geoscape.
	 */
	std::string trajectory;
	/// Number of minutes between UFOs in the wave.
	/**
	 * The actual value used is spawnTimer/4 or 3*spawnTimer/4.
	 */
	size_t spawnTimer;
	/// This wave performs the mission objective.
	/**
	 * The UFO executes a special action based on the mission objective.
	 */
	bool objective;
	/// This wave performs the mission objective in a rectangular (non-point) area.
	/**
	 * Make the UFO land on a random landing site (instead of always top left). Make the mission site spawn exactly on the landing site.
	 */
	bool objectiveOnTheLandingSite;
	/**
	 * Make the mission site spawn on an xcom base (or not at all).
	 */
	bool objectiveOnXcomBase;
	/// The chance to become a hunter-killer UFO upon spawning.
	/**
	 * -1 (default): take the info from RuleUfo
	 *  0: not a hunter-killer
	 *  1..99: percentage chance to be flagged as hunter-killer upon spawn
	 *  100: always a hunter-killer
	 */
	int hunterKillerPercentage;
	/// Algorithm to use when prioritizing xcom targets
	/**
	 * -1 (default): take the info from RuleUfo
	 * 0: prefer hunting xcom interceptors
	 * 1: prefer hunting xcom transports
	 * 2: random preference (0 or 1) determined at spawn
	 */
	int huntMode;
	/// Algorithm to use when considering retreating from the dogfight
	/**
	* -1 (default): take the info from RuleUfo
	* 0: flee if you're losing
	* 1: never flee, never crash (is destroyed instead of crashing)
	* 2: random preference (0 or 1) determined at spawn
	*/
	int huntBehavior;
	/// Does this wave escort/protect the previous wave(s)?
	/**
	* The UFO escorts other UFO(s) from the same mission.
	*/
	bool escort;
	/// The chance to interrupt the alien mission when successfully shooting down a UFO from this wave.
	/**
	*  0 (default): cannot interrupt
	*  1..99: percentage chance to be interrupted
	*  100: always interrupted
	*/
	int interruptPercentage;
};

enum MissionObjective { OBJECTIVE_SCORE, OBJECTIVE_INFILTRATION, OBJECTIVE_BASE, OBJECTIVE_SITE, OBJECTIVE_RETALIATION, OBJECTIVE_SUPPLY, OBJECTIVE_INSTANT_RETALIATION };

/**
 * Stores fixed information about a mission type.
 * It stores the mission waves and the distribution of the races that can
 * undertake the mission based on game date.
 */
class RuleAlienMission
{
public:
	RuleAlienMission(const std::string &type);
	/// Releases all resources held by the mission.
	~RuleAlienMission();
	/// Gets the mission's type.
	const std::string &getType() const { return _type; }
	/// Does this mission have raceWeights?
	bool hasRaceWeights() const;
	/// Gets a race based on the game time and the racial distribution.
	std::string generateRace(const size_t monthsPassed) const;
	/// Loads alien mission data from YAML.
	void load(const YAML::Node &node);
	/// Gets the number of waves.
	size_t getWaveCount() const { return _waves.size(); }
	/// Gets the full wave information.
	const MissionWave &getWave(size_t index) const { return _waves[index]; }
	/// Gets the score for this mission.
	int getPoints() const;
	/// Gets the objective for this mission.
	MissionObjective getObjective() const { return _objective; }
	/// Gets the UFO type for special spawns.
	const std::string &getSpawnUfo() const { return _spawnUfo; }
	/// Should the scouting phase of the retaliation mission be skipped?
	bool skipScoutingPhase() const { return _skipScoutingPhase; }
	/// Gets the zone for spawning an alien site or base.
	int getSpawnZone() const { return _spawnZone; }
	/// Gets the chances of this mission based on the game time.
	int getWeight(const size_t monthsPassed) const;
	/// Gets the inherent odds of this mission spawning a retaliation mission.
	int getRetaliationOdds() const;
	/// Should the infiltration end after first cycle or continue indefinitely?
	bool isEndlessInfiltration() const;
	/// Should the retaliation mission end after the first base defense or continue until all already spawned UFOs disappear?
	bool isMultiUfoRetaliation() const { return _multiUfoRetaliation; }
	/// Should the retaliation mission continue generating UFOs even after the first base defense?
	bool isMultiUfoRetaliationExtra() const { return _multiUfoRetaliationExtra; }
	/// Should the retaliation UFO ignore xcom base defenses?
	bool ignoreBaseDefenses() const { return _ignoreBaseDefenses; }
	/// Should the mission site despawn even if targeted?
	bool despawnEvenIfTargeted() const { return _despawnEvenIfTargeted; }
	/// Should the original UFO be respawned when the mission site despawns?
	bool respawnUfoAfterSiteDespawn() const { return _respawnUfoAfterSiteDespawn; }
	/// Should the spawned alien base be revealed immediately?
	bool showAlienBase() const { return _showAlienBase; }
	/// Gets the ID of the research topic that interrupts this mission (if any).
	const std::string &getInterruptResearch() const { return _interruptResearch; }
	/// the type of missionSite to spawn (if any)
	std::string getSiteType() const { return _siteType; }
	/// From where does this mission operate?
	AlienMissionOperationType getOperationType() const { return _operationType; }
	/// Gets the mission zone for spawning the operation base (if necessary).
	int getOperationSpawnZone() const { return _operationSpawnZone; }
	/// Gets the type of operation base to spawn (if any).
	const std::string &getOperationBaseType() const { return _operationBaseType; }
	/// Gets the odds of this mission targetting an xcom base. Works only for "gen missions" spawned by an alien base.
	int getTargetBaseOdds() const { return _targetBaseOdds; }
	/// Does this mission have region weights? Works only for "gen missions" spawned by an alien base.
	bool hasRegionWeights() const;
	/// Gets a region based on the game time and the region distribution. Works only for "gen missions" spawned by an alien base.
	std::string generateRegion(const size_t monthsPassed) const;
private:
	/// The mission's type ID.
	std::string _type;
	/// The UFO to use for spawning.
	std::string _spawnUfo;
	/// Should the scouting phase of the retaliation mission be skipped?
	bool _skipScoutingPhase;
	/// The race distribution over game time.
	std::vector<std::pair<size_t, WeightedOptions*> > _raceDistribution;
	/// The mission's weights.
	std::map<size_t, int> _weights;
	/// The mission's waves.
	std::vector<MissionWave> _waves;
	/// The mission's points.
	int _points;
	/// The mission's objective.
	MissionObjective _objective;
	/// The mission zone to use for spawning.
	int _spawnZone;
	/// The odds that this mission will result in retaliation
	int _retaliationOdds;
	/// Should the infiltration end after first cycle or continue indefinitely?
	bool _endlessInfiltration;
	/// Should the retaliation mission end after the first base defense or continue until all already spawned UFOs disappear?
	bool _multiUfoRetaliation;
	/// Should the retaliation mission continue generating UFOs even after the first base defense?
	bool _multiUfoRetaliationExtra;
	/// Should the retaliation UFO ignore xcom base defenses?
	bool _ignoreBaseDefenses;
	/// Should the mission site despawn even if targeted?
	bool _despawnEvenIfTargeted;
	/// Should the original UFO be respawned when the mission site despawns?
	bool _respawnUfoAfterSiteDespawn;
	/// Should the spawned alien base be revealed immediately?
	bool _showAlienBase;
	/// the research topic that interrupts this mission type (when discovered)
	std::string _interruptResearch;
	/// the type of missionSite to spawn (if any)
	std::string _siteType;
	/// From where does this mission operate?
	AlienMissionOperationType _operationType;
	/// The mission zone for spawning the operation base (if necessary).
	int _operationSpawnZone;
	/// The type of operation base to spawn (if any).
	std::string _operationBaseType;
	/// The odds of this mission targetting an xcom base. Works only for "gen missions" spawned by an alien base.
	int _targetBaseOdds;
	/// The region distribution over game time. Works only for "gen missions" spawned by an alien base.
	std::vector<std::pair<size_t, WeightedOptions*> > _regionWeights;
};

}
