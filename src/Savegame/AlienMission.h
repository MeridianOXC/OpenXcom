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
#include "../Engine/Yaml.h"

namespace OpenXcom
{

class RuleAlienMission;
class Ufo;
class Globe;
class Game;
class SavedGame;
class Mod;
class RuleRegion;
struct MissionWave;
class UfoTrajectory;
class AlienBase;
class MissionSite;
struct MissionArea;
class AlienDeployment;
class Country;
class Base;

/**
 * Represents an ongoing alien mission.
 * Contains variable info about the mission, like spawn counter, target region
 * and current wave.
 * @sa RuleAlienMission
 */
class AlienMission
{
private:
	const RuleAlienMission &_rule;
	std::string _region, _race;
	size_t _nextWave;
	size_t _nextUfoCounter;
	size_t _spawnCountdown;
	size_t _liveUfos;
	bool _interrupted, _multiUfoRetaliationInProgress;
	int _uniqueID, _missionSiteZoneArea;
	const AlienBase *_base;
public:
	// Data

	/// Creates a mission of the specified type.
	AlienMission(const RuleAlienMission &rule);
	/// Cleans up the mission info.
	~AlienMission();
	/// Loads the mission from YAML.
	void load(const YAML::YamlNodeReader& reader, SavedGame &game, const Mod* mod);
	/// Saves the mission to YAML.
	void save(YAML::YamlNodeWriter writer) const;
	/// Gets the mission's ruleset.
	const RuleAlienMission &getRules() const { return _rule; }
	/// Gets the mission's region.
	const std::string &getRegion() const { return _region; }
	/// Sets the mission's region.
	void setRegion(const std::string &region, const Mod &rules);
	/// Gets the mission's race.
	const std::string &getRace() const { return _race; }
	/// Sets the mission's race.
	void setRace(const std::string &race) { _race = race; }
	/// Gets the minutes until next wave spawns.
	size_t getWaveCountdown() const { return _spawnCountdown; }
	/// Sets the minutes until next wave spawns.
	void setWaveCountdown(size_t minutes);
	/// Sets the unique ID for this mission.
	void setId(int id);
	/// Gets the unique ID for this mission.
	int getId() const;
	/// Gets the alien base for this mission.
	const AlienBase *getAlienBase() const;
	/// Sets the alien base for this mission.
	void setAlienBase(const AlienBase *base);

	// Behaviour

	/// Is this mission over?
	bool isOver() const;
	/// Handle UFO spawning for the mission.
	void think(Game &engine, const Globe &globe);
	/// Initialize with values from rules.
	void start(Game &engine, const Globe &globe, size_t initialCount = 0);
	/// Increase number of live UFOs.
	void increaseLiveUfos() { ++_liveUfos; }
	/// Decrease number of live UFOs.
	void decreaseLiveUfos() { --_liveUfos; }
	/// Sets the interrupted flag.
	void setInterrupted(bool interrupted) { _interrupted = interrupted; }
	/// Sets the multiUfoRetaliationInProgress flag.
	void setMultiUfoRetaliationInProgress(bool multiUfoRetaliationInProgress) { _multiUfoRetaliationInProgress = multiUfoRetaliationInProgress; }
	/// Handle UFO reaching a waypoint.
	void ufoReachedWaypoint(Ufo &ufo, Game &engine, const Globe &globe);
	/// Handle UFO lifting from the ground.
	void ufoLifting(Ufo &ufo, SavedGame &game);
	/// Handle UFO shot down.
	void ufoShotDown(Ufo &ufo);
	/// Handle Points for mission successes.
	void addScore(double lon, double lat, SavedGame &game) const;
	/// Keep track of the city/whatever that we're going to target.
	void setMissionSiteZoneArea(int area);
private:
	/// Selects an xcom base in a given region.
	Base* selectXcomBase(SavedGame& game, const RuleRegion& regionRules);
	/// Spawns a UFO, based on mission rules.
	Ufo *spawnUfo(SavedGame &game, const Mod &mod, const Globe &globe, const MissionWave &wave, const UfoTrajectory &trajectory);
	/// Spawn an alien base
	AlienBase *spawnAlienBase(Country *pactCountry, Game &engine, std::pair<double, double> pos, AlienDeployment *deployment);
	/// Chooses a mission type for a new alien base.
	AlienDeployment *chooseAlienBaseType(const Mod &mod, const MissionArea &area);
	/// Select a destination (lon/lat) based on the criteria of our trajectory and desired waypoint.
	std::pair<double, double> getWaypoint(const MissionWave &wave, const UfoTrajectory &trajectory, const size_t nextWaypoint, const Globe &globe, const RuleRegion &region, const Ufo &ufo);
	/// Get a random landing point inside the given region zone.
	std::pair<double, double> getLandPoint(const Globe &globe, const RuleRegion &region, size_t zone, const Ufo &ufo);
	/// Get a random landing point inside the given region zone and area.
	std::pair<double, double> getLandPointForMissionSite(const Globe& globe, const RuleRegion& region, size_t zone, int area, const Ufo& ufo);
	/// Spawns a MissionSite at a specific location.
	MissionSite *spawnMissionSite(SavedGame &game, const Mod &mod, const MissionArea &area, const Ufo *ufo = 0, AlienDeployment *missionOveride = 0);
	/// Provides some error information for bad mission definitions
	void logMissionError(int zone, const RuleRegion &region);

};

}
