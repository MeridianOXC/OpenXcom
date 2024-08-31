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
#include "AlienMission.h"
#include "AlienBase.h"
#include "Base.h"
#include "../fmath.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"
#include "../Geoscape/Globe.h"
#include "../Mod/RuleAlienMission.h"
#include "../Mod/RuleRegion.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleUfo.h"
#include "../Mod/UfoTrajectory.h"
#include "../Mod/RuleGlobe.h"
#include "../Mod/Texture.h"
#include "SavedGame.h"
#include "MissionSite.h"
#include "Ufo.h"
#include "Craft.h"
#include "Region.h"
#include "Country.h"
#include "Waypoint.h"
#include <assert.h>
#include <algorithm>
#include <functional>
#include "../Mod/AlienDeployment.h"

namespace OpenXcom
{

AlienMission::AlienMission(const RuleAlienMission &rule) : _rule(rule), _nextWave(0), _nextUfoCounter(0), _spawnCountdown(0), _liveUfos(0),
	_interrupted(false), _multiUfoRetaliationInProgress(false), _uniqueID(0), _missionSiteZoneArea(-1), _base(0)
{
	// Empty by design.
}

AlienMission::~AlienMission()
{
	// Empty by design.
}

/**
 * Loads the alien mission from a YAML file.
 * @param node The YAML node containing the data.
 * @param game The game data, required to locate the alien base.
 */
void AlienMission::load(const YAML::Node& node, SavedGame &game, const Mod* mod)
{
	_region = node["region"].as<std::string>(_region);
	_race = node["race"].as<std::string>(_race);
	_nextWave = node["nextWave"].as<size_t>(_nextWave);
	_nextUfoCounter = node["nextUfoCounter"].as<size_t>(_nextUfoCounter);
	_spawnCountdown = node["spawnCountdown"].as<size_t>(_spawnCountdown);
	_liveUfos = node["liveUfos"].as<size_t>(_liveUfos);
	_interrupted = node["interrupted"].as<bool>(_interrupted);
	_multiUfoRetaliationInProgress = node["multiUfoRetaliationInProgress"].as<bool>(_multiUfoRetaliationInProgress);
	_uniqueID = node["uniqueID"].as<int>(_uniqueID);
	if (const YAML::Node &base = node["alienBase"])
	{
		int id = base.as<int>(-1);
		std::string type = "STR_ALIEN_BASE";
		// New format
		if (id == -1)
		{
			id = base["id"].as<int>();
			type = base["type"].as<std::string>();
		}
		AlienBase* found = nullptr;
		for (auto* ab : *game.getAlienBases())
		{
			if (ab->getId() == id && ab->getDeployment()->getMarkerName() == type)
			{
				found = ab;
				break;
			}
		}
		if (!found)
		{
			throw Exception("Corrupted save: Invalid base for mission.");
		}
		_base = found;
	}
	_missionSiteZoneArea = node["missionSiteZone"].as<int>(_missionSiteZoneArea);

	// fix invalid saves
	RuleRegion* region = mod->getRegion(_region, false);
	if (!region)
	{
		Log(LOG_ERROR) << "Corrupted save: Mission with uniqueID: " << _uniqueID << " has an invalid region: " << _region;
		_interrupted = true;
		if (_missionSiteZoneArea > -1)
		{
			_missionSiteZoneArea = 0;
		}
		_region = mod->getRegionsList().front();
		if (_liveUfos > 0)
		{
			Log(LOG_ERROR) << "Mission still has some live UFOs, please leave them be until they disappear.";
		}
		Log(LOG_ERROR) << "Mission was interrupted! Temporarily assigned to a new region: " << _region;
	}
	AlienRace* race = mod->getAlienRace(_race, false);
	if (!race)
	{
		Log(LOG_ERROR) << "Corrupted save: Mission with uniqueID: " << _uniqueID << " has an invalid alien race: " << _race;
		_interrupted = true;
		_race = mod->getAlienRacesList().front();
		if (_liveUfos > 0)
		{
			Log(LOG_ERROR) << "Mission still has some live UFOs, please leave them be until they disappear.";
		}
		Log(LOG_ERROR) << "Mission was interrupted! Temporarily assigned a new alien race: " << _race;
	}
}

/**
 * Saves the alien mission to a YAML file.
 * @return YAML node.
 */
YAML::Node AlienMission::save() const
{
	YAML::Node node;
	node["type"] = _rule.getType();
	node["region"] = _region;
	node["race"] = _race;
	node["nextWave"] = _nextWave;
	node["nextUfoCounter"] = _nextUfoCounter;
	node["spawnCountdown"] = _spawnCountdown;
	node["liveUfos"] = _liveUfos;
	if (_interrupted)
	{
		node["interrupted"] = _interrupted;
	}
	if (_multiUfoRetaliationInProgress)
	{
		node["multiUfoRetaliationInProgress"] = _multiUfoRetaliationInProgress;
	}
	node["uniqueID"] = _uniqueID;
	if (_base)
	{
		node["alienBase"] = _base->saveId();
	}
	node["missionSiteZone"] = _missionSiteZoneArea;
	return node;
}

/**
 * Check if a mission is over and can be safely removed from the game.
 * A mission is over if it will spawn no more UFOs and it has no UFOs still in
 * the game.
 * @return If the mission can be safely removed from game.
 */
bool AlienMission::isOver() const
{
	if (_interrupted && !_liveUfos)
	{
		return true;
	}
	if (_rule.getObjective() == OBJECTIVE_INFILTRATION && _rule.isEndlessInfiltration())
	{
		//Infiltrations continue for ever.
		return false;
	}
	if (_nextWave == _rule.getWaveCount() && !_liveUfos)
	{
		return true;
	}
	return false;
}

void AlienMission::think(Game &engine, const Globe &globe)
{
	// if interrupted, don't generate any more UFOs or anything else
	if (_interrupted || (_multiUfoRetaliationInProgress && !_rule.isMultiUfoRetaliationExtra()))
	{
		return;
	}

	const Mod &mod = *engine.getMod();
	SavedGame &game = *engine.getSavedGame();
	if (_nextWave >= _rule.getWaveCount())
		return;
	if (_spawnCountdown > 30)
	{
		_spawnCountdown -= 30;
		return;
	}
	const MissionWave &wave = _rule.getWave(_nextWave);
	const UfoTrajectory &trajectory = *mod.getUfoTrajectory(wave.trajectory, true);
	Ufo *ufo = spawnUfo(game, mod, globe, wave, trajectory);
	if (ufo)
	{
		//Some missions may not spawn a UFO!
		ufo->setMissionWaveNumber(_nextWave);
		game.getUfos()->push_back(ufo);
	}
	else if ((mod.getDeployment(wave.ufoType) && !mod.getUfo(wave.ufoType) && !mod.getDeployment(wave.ufoType)->getMarkerName().empty()) // a mission site that we want to spawn directly
			|| (_rule.getObjective() == OBJECTIVE_SITE && wave.objective)) // or we want to spawn one at random according to our terrain
	{
		RuleRegion* regionRules = mod.getRegion(_region, true);
		std::vector<MissionArea> areas = regionRules->getMissionZones().at((_rule.getSpawnZone() == -1) ? trajectory.getZone(0) : _rule.getSpawnZone()).areas;
		MissionArea area = areas.at((_missionSiteZoneArea == -1) ? RNG::generate(0, areas.size() - 1) : _missionSiteZoneArea);

		if (wave.objectiveOnXcomBase)
		{
			Base* xcombase = selectXcomBase(game, *regionRules);
			// no xcom base = don't spawn a mission site
			if (xcombase)
			{
				area.lonMin = xcombase->getLongitude();
				area.lonMax = xcombase->getLongitude();
				area.latMin = xcombase->getLatitude();
				area.latMax = xcombase->getLatitude();
				// area.texture is taken from the ruleset
				area.name = ""; // remove the city name, if there was any
				spawnMissionSite(game, mod, area, 0, mod.getDeployment(wave.ufoType, false));
			}
		}
		else
		{
			spawnMissionSite(game, mod, area, 0, mod.getDeployment(wave.ufoType, false));
		}
	}

	++_nextUfoCounter;
	if (_nextUfoCounter >= wave.ufoCount)
	{
		_nextUfoCounter = 0;
		++_nextWave;
	}
	if (_rule.getObjective() == OBJECTIVE_INFILTRATION && _nextWave == _rule.getWaveCount())
	{
		std::vector<Country*> countriesCopy = *game.getCountries();
		if (mod.getInfiltrateRandomCountryInTheRegion())
		{
			RNG::shuffle(countriesCopy);
		}
		for (auto* c : countriesCopy)
		{
			RuleRegion *region = mod.getRegion(_region, true);
			if (c->canBeInfiltrated() && region->insideRegion(c->getRules()->getLabelLongitude(), c->getRules()->getLabelLatitude()))
			{
				std::pair<double, double> pos;
				int tries = 0;
				bool dynamicBaseType = _rule.getSiteType().empty();
				AlienDeployment *alienBaseType = nullptr;
				bool wantsToSpawnFakeUnderwater = false;
				bool found = false;
				if (!mod.getBuildInfiltrationBaseCloseToTheCountry())
				{
					std::vector<MissionArea> areas = region->getMissionZones().at(_rule.getSpawnZone()).areas;
					while (!found)
					{
						MissionArea &area = areas.at(RNG::generate(0, areas.size() - 1));
						pos.first = RNG::generate(std::min(area.lonMin, area.lonMax), std::max(area.lonMin, area.lonMax));
						pos.second = RNG::generate(std::min(area.latMin, area.latMax), std::max(area.latMin, area.latMax));

						if (tries == 0 || dynamicBaseType)
						{
							alienBaseType = chooseAlienBaseType(mod, area);
							wantsToSpawnFakeUnderwater = RNG::percent(alienBaseType->getFakeUnderwaterSpawnChance());
						}
						++tries;

						if (tries == 100)
						{
							found = true; // forced spawn on invalid location
						}
						else if (globe.insideLand(pos.first, pos.second) && region->insideRegion(pos.first, pos.second))
						{
							bool isFakeUnderwater = globe.insideFakeUnderwaterTexture(pos.first, pos.second);
							if (wantsToSpawnFakeUnderwater)
							{
								if (isFakeUnderwater) found = true; // found spawn point on fakeUnderwater texture
							}
							else
							{
								if (!isFakeUnderwater) found = true; // found spawn point on land
							}
						}
					}
				}
				else
				{
					dynamicBaseType = false; // just to make absolutely clear this feature is not supported in this case
					MissionArea dummyArea;
					alienBaseType = chooseAlienBaseType(mod, dummyArea);
					wantsToSpawnFakeUnderwater = RNG::percent(alienBaseType->getFakeUnderwaterSpawnChance());

					const RuleCountry* cRule = c->getRules();
					int pick = 0;
					double lonMini, lonMaxi, latMini, latMaxi;
					while (!found)
					{
						pick = RNG::generate(0, cRule->getLonMin().size() - 1);
						lonMini = cRule->getLonMin()[pick];
						lonMaxi = cRule->getLonMax()[pick];
						if (lonMini > lonMaxi)
						{
							// UK, France, Spain, or anything crossed by the prime meridian
							if (RNG::percent(50)) { lonMini = Deg2Rad(0.0); } else { lonMaxi = Deg2Rad(359.99); }
						}
						latMini = cRule->getLatMin()[pick];
						latMaxi = cRule->getLatMax()[pick];
						pos.first = RNG::generate(std::min(lonMini, lonMaxi), std::max(lonMini, lonMaxi));
						pos.second = RNG::generate(std::min(latMini, latMaxi), std::max(latMini, latMaxi));
						++tries;

						if (tries == 100)
						{
							found = true; // forced spawn on invalid location
						}
						else if (globe.insideLand(pos.first, pos.second) && cRule->insideCountry(pos.first, pos.second))
						{
							bool isFakeUnderwater = globe.insideFakeUnderwaterTexture(pos.first, pos.second);
							if (wantsToSpawnFakeUnderwater)
							{
								if (isFakeUnderwater) found = true; // found spawn point on fakeUnderwater texture
							}
							else
							{
								if (!isFakeUnderwater) found = true; // found spawn point on land
							}
						}
					}
				}
				if (tries < 100 || mod.getAllowAlienBasesOnWrongTextures())
				{
					// only create a pact if the base is going to be spawned too
					c->setNewPact();

					spawnAlienBase(c, engine, pos, alienBaseType);

					// if the base can't be spawned for this country, try the next country
					break;
				}
			}
		}
		if (_rule.isEndlessInfiltration())
		{
			// Infiltrations loop for ever.
			_nextWave = 0;
		}
	}
	if (_rule.getObjective() == OBJECTIVE_BASE && _nextWave == _rule.getWaveCount() && !wave.objectiveOnTheLandingSite)
	{
		RuleRegion *region = mod.getRegion(_region, true);
		std::vector<MissionArea> areas = region->getMissionZones().at(_rule.getSpawnZone()).areas;
		std::pair<double, double> pos;
		int tries = 0;
		bool dynamicBaseType = _rule.getSiteType().empty();
		AlienDeployment* alienBaseType = nullptr;
		bool wantsToSpawnFakeUnderwater = false;
		bool found = false;
		while (!found)
		{
			MissionArea &area = areas.at(RNG::generate(0, areas.size()-1));
			pos.first = RNG::generate(std::min(area.lonMin, area.lonMax), std::max(area.lonMin, area.lonMax));
			pos.second = RNG::generate(std::min(area.latMin, area.latMax), std::max(area.latMin, area.latMax));

			if (tries == 0 || dynamicBaseType)
			{
				alienBaseType = chooseAlienBaseType(mod, area);
				wantsToSpawnFakeUnderwater = RNG::percent(alienBaseType->getFakeUnderwaterSpawnChance());
			}
			++tries;

			if (tries == 100)
			{
				found = true; // forced spawn on invalid location
			}
			else if (globe.insideLand(pos.first, pos.second) && region->insideRegion(pos.first, pos.second, true))
			{
				bool isFakeUnderwater = globe.insideFakeUnderwaterTexture(pos.first, pos.second);
				if (wantsToSpawnFakeUnderwater)
				{
					if (isFakeUnderwater) found = true; // found spawn point on fakeUnderwater texture
				}
				else
				{
					if (!isFakeUnderwater) found = true; // found spawn point on land
				}
			}
		}
		if (tries < 100 || mod.getAllowAlienBasesOnWrongTextures())
		{
			spawnAlienBase(0, engine, pos, alienBaseType);
		}
	}

	if (_nextWave != _rule.getWaveCount())
	{
		size_t origSpawnTimerValue = _rule.getWave(_nextWave).spawnTimer;
		size_t spawnTimer = origSpawnTimerValue / 30;
		_spawnCountdown = (spawnTimer/2 + RNG::generate(0, spawnTimer)) * 30;

		if (origSpawnTimerValue == 0)
		{
			// spawn more UFOs immediately
			think(engine, globe);
		}
	}
}

/**
 * Selects an xcom base in a given region.
 * @param game The saved game information.
 * @param regionRules The rule for the given region.
 * @return Pointer to the selected xcom base or nullptr.
 */
Base* AlienMission::selectXcomBase(SavedGame& game, const RuleRegion& regionRules)
{
	std::vector<Base*> validxcombases;
	for (auto* xb : *game.getBases())
	{
		if (regionRules.insideRegion(xb->getLongitude(), xb->getLatitude()))
		{
			if (_rule.getObjective() == OBJECTIVE_RETALIATION)
			{
				// only discovered xcom bases!
				if (xb->getRetaliationTarget())
				{
					validxcombases.push_back(xb);
					if (_rule.isMultiUfoRetaliation())
					{
						continue; // non-vanilla: let's consider all, for fun
					}
					break; // vanilla: the first is enough
				}
			}
			else // instant retaliation; or a mission wave with `objectiveOnXcomBase: true`
			{
				validxcombases.push_back(xb);
			}
		}
	}
	Base* xcombase = nullptr;
	if (!validxcombases.empty())
	{
		if (validxcombases.size() == 1)
		{
			// take the first (don't mess with the RNG seed)
			xcombase = validxcombases.front();
		}
		else
		{
			int rngpick = RNG::generate(0, validxcombases.size() - 1);
			xcombase = validxcombases[rngpick];
		}
	}
	return xcombase;
}

/**
 * This function will spawn a UFO according the mission rules.
 * Some code is duplicated between cases, that's ok for now. It's on different
 * code paths and the function is MUCH easier to read written this way.
 * @param game The saved game information.
 * @param mod The mod.
 * @param globe The globe, for land checks.
 * @param wave The wave for the desired UFO.
 * @param trajectory The rule for the desired trajectory.
 * @return Pointer to the spawned UFO. If the mission does not desire to spawn a UFO, 0 is returned.
 */
Ufo *AlienMission::spawnUfo(SavedGame &game, const Mod &mod, const Globe &globe, const MissionWave &wave, const UfoTrajectory &trajectory)
{
	auto logUfo = [](Ufo* u, SavedGame& g, AlienMission* a)
	{
		if (Options::oxceGeoscapeDebugLogMaxEntries > 0)
		{
			std::ostringstream ss;
			ss << "gameTime: " << g.getTime()->getFullString();
			ss << " ufoId: " << u->getUniqueId();
			ss << " ufoType: " << u->getRules()->getType();
			ss << " race: " << u->getAlienRace();
			ss << " region: " << a->getRegion();
			ss << " trajectory: " << u->getTrajectory().getID();
			if (u->isHunterKiller())
			{
				ss << " hk: true";
			}
			ss << " missionId: " << a->getId();
			ss << " missionType: " << a->getRules().getType();
			g.getGeoscapeDebugLog().push_back(ss.str());
		}
	};

	RuleUfo *ufoRule = mod.getUfo(wave.ufoType);
	int hunterKillerPercentage = wave.hunterKillerPercentage;
	if (hunterKillerPercentage == -1 && ufoRule)
	{
		hunterKillerPercentage = ufoRule->getHunterKillerPercentage();
	}
	int huntMode = wave.huntMode;
	if (huntMode == -1 && ufoRule)
	{
		huntMode = ufoRule->getHuntMode();
	}
	int huntBehavior = wave.huntBehavior;
	if (huntBehavior == -1 && ufoRule)
	{
		huntBehavior = ufoRule->getHuntBehavior();
	}
	if (_rule.getObjective() == OBJECTIVE_RETALIATION || _rule.getObjective() == OBJECTIVE_INSTANT_RETALIATION)
	{
		const RuleRegion &regionRules = *mod.getRegion(_region, true);
		Base* xcombase = nullptr;

		// skip the scouting phase of a retaliation mission
		if (_rule.skipScoutingPhase() && _rule.getObjective() == OBJECTIVE_RETALIATION)
		{
			for (auto* xbase : *game.getBases())
			{
				if (regionRules.insideRegion(xbase->getLongitude(), xbase->getLatitude()))
				{
					xcombase = xbase;
					break;
				}
			}
		}
		else
		{
			xcombase = selectXcomBase(game, regionRules);
		}

		if (xcombase)
		{
			// Spawn a battleship straight for the XCOM base.
			const RuleUfo &battleshipRule = _rule.getSpawnUfo().empty() ? *ufoRule : *mod.getUfo(_rule.getSpawnUfo(), true);
			const UfoTrajectory &assaultTrajectory = *mod.getUfoTrajectory(UfoTrajectory::RETALIATION_ASSAULT_RUN, true);
			Ufo *ufo = new Ufo(&battleshipRule, game.getId("STR_UFO_UNIQUE"));
			ufo->setMissionInfo(this, &assaultTrajectory);
			std::pair<double, double> pos;
			if (_rule.getObjective() == OBJECTIVE_INSTANT_RETALIATION)
			{
				pos.first = xcombase->getLongitude();
				pos.second = xcombase->getLatitude();
			}
			else if (_rule.getOperationType() != AMOT_SPACE && _base)
			{
				pos.first = _base->getLongitude();
				pos.second = _base->getLatitude();
			}
			else if (trajectory.getAltitude(0) == "STR_GROUND")
			{
				pos = getLandPoint(globe, regionRules, trajectory.getZone(0), *ufo);
			}
			else
			{
				pos = regionRules.getRandomPoint(trajectory.getZone(0));
			}
			ufo->setAltitude(assaultTrajectory.getAltitude(0));
			ufo->setSpeed(assaultTrajectory.getSpeedPercentage(0) * ufo->getCraftStats().speedMax);
			ufo->setLongitude(pos.first);
			ufo->setLatitude(pos.second);
			Waypoint *wp = new Waypoint();
			wp->setLongitude(xcombase->getLongitude());
			wp->setLatitude(xcombase->getLatitude());
			ufo->setDestination(wp);
			logUfo(ufo, game, this);
			return ufo;
		}
		else if (_rule.getObjective() == OBJECTIVE_INSTANT_RETALIATION)
		{
			// no xcom base found, nothing to do, terminate
			_interrupted = true;
			return 0;
		}
	}
	else if (_rule.getObjective() == OBJECTIVE_SUPPLY)
	{
		if (ufoRule == 0 || (wave.objective && !_base))
		{
			// No base to supply!
			return 0;
		}
		// Our destination is always an alien base.
		Ufo *ufo = 0;
		if (wave.objective)
		{
			ufo = new Ufo(ufoRule, game.getId("STR_UFO_UNIQUE"));
		}
		else
		{
			ufo = new Ufo(ufoRule, game.getId("STR_UFO_UNIQUE"), hunterKillerPercentage, huntMode, huntBehavior);
		}
		ufo->setMissionInfo(this, &trajectory);
		const RuleRegion &regionRules = *mod.getRegion(_region, true);
		std::pair<double, double> pos;
		if (_rule.getOperationType() != AMOT_SPACE && _base)
		{
			pos.first = _base->getLongitude();
			pos.second = _base->getLatitude();
		}
		else if (trajectory.getAltitude(0) == "STR_GROUND")
		{
			pos = getLandPoint(globe, regionRules, trajectory.getZone(0), *ufo);
		}
		else
		{
			pos = regionRules.getRandomPoint(trajectory.getZone(0));
		}
		ufo->setAltitude(trajectory.getAltitude(0));
		ufo->setSpeed(trajectory.getSpeedPercentage(0) * ufo->getCraftStats().speedMax);
		ufo->setLongitude(pos.first);
		ufo->setLatitude(pos.second);
		Waypoint *wp = new Waypoint();
		if (trajectory.getAltitude(1) == "STR_GROUND")
		{
			if (wave.objective)
			{
				// Supply ships on supply missions land on bases, ignore trajectory zone.
				pos.first = _base->getLongitude();
				pos.second = _base->getLatitude();
			}
			else
			{
				// Other ships can land where they want.
				pos = getLandPoint(globe, regionRules, trajectory.getZone(1), *ufo);
			}
		}
		else
		{
			pos = regionRules.getRandomPoint(trajectory.getZone(1));
		}
		wp->setLongitude(pos.first);
		wp->setLatitude(pos.second);
		ufo->setDestination(wp);

		// Only hunter-killers can escort
		if (ufo->isHunterKiller() && wave.escort && !wave.objective)
		{
			ufo->setEscort(true);
			// Find a UFO to escort
			for (auto* ufoToBeEscorted : *game.getUfos())
			{
				// From the same mission
				if (ufoToBeEscorted->getMission()->getId() == ufo->getMission()->getId())
				{
					// But not another hunter-killer, we escort only normal UFOs
					if (!ufoToBeEscorted->isHunterKiller())
					{
						ufo->setLongitude(ufoToBeEscorted->getLongitude());
						ufo->setLatitude(ufoToBeEscorted->getLatitude());
						ufo->setEscortedUfo(ufoToBeEscorted);
						break;
					}
				}
			}
		}
		logUfo(ufo, game, this);
		return ufo;
	}
	if (ufoRule == 0)
		return 0;
	// Spawn according to sequence.
	Ufo *ufo = new Ufo(ufoRule, game.getId("STR_UFO_UNIQUE"), hunterKillerPercentage, huntMode, huntBehavior);
	ufo->setMissionInfo(this, &trajectory);
	const RuleRegion &regionRules = *mod.getRegion(_region, true);
	std::pair<double, double> pos = getWaypoint(wave, trajectory, 0, globe, regionRules, *ufo);
	ufo->setAltitude(trajectory.getAltitude(0));
	if (trajectory.getAltitude(0) == "STR_GROUND")
	{
		ufo->setSecondsRemaining(trajectory.groundTimer()*5);
	}
	ufo->setSpeed(trajectory.getSpeedPercentage(0) * ufo->getCraftStats().speedMax);
	ufo->setLongitude(pos.first);
	ufo->setLatitude(pos.second);
	if (_rule.getOperationType() != AMOT_SPACE && _base)
	{
		// override starting location for Earth-based alien operations
		ufo->setLongitude(_base->getLongitude());
		ufo->setLatitude(_base->getLatitude());
	}
	Waypoint *wp = new Waypoint();
	if (trajectory.getWaypointCount() > 1)
	{
		pos = getWaypoint(wave, trajectory, 1, globe, regionRules, *ufo);
	}
	else
	{
		std::stringstream ss;
		ss << "Missing second waypoint! Error occurred while trying to determine UFO waypoint for mission type: " << _rule.getType();
		ss << " in region: " << regionRules.getType() << "; ufo type: " << ufo->getRules()->getType() << ", trajectory ID: " << trajectory.getID() << ".";
		throw Exception(ss.str());
	}
	wp->setLongitude(pos.first);
	wp->setLatitude(pos.second);
	ufo->setDestination(wp);

	// Only hunter-killers can escort
	if (ufo->isHunterKiller() && wave.escort)
	{
		ufo->setEscort(true);
		// Find a UFO to escort
		for (auto* ufoToBeEscorted : *game.getUfos())
		{
			// From the same mission
			if (ufoToBeEscorted->getMission()->getId() == ufo->getMission()->getId())
			{
				// But not another hunter-killer, we escort only normal UFOs
				if (!ufoToBeEscorted->isHunterKiller())
				{
					ufo->setLongitude(ufoToBeEscorted->getLongitude());
					ufo->setLatitude(ufoToBeEscorted->getLatitude());
					ufo->setEscortedUfo(ufoToBeEscorted);
					break;
				}
			}
		}
	}
	logUfo(ufo, game, this);
	return ufo;
}

void AlienMission::start(Game &engine, const Globe &globe, size_t initialCount)
{
	_nextWave = 0;
	_nextUfoCounter = 0;
	_liveUfos = 0;
	if (initialCount == 0)
	{
		size_t spawnTimer = _rule.getWave(0).spawnTimer / 30;
		_spawnCountdown = (spawnTimer / 2 + RNG::generate(0, spawnTimer)) * 30;
	}
	else
	{
		_spawnCountdown = initialCount;
	}

	// Earth-based alien operations
	if (_rule.getOperationType() != AMOT_SPACE && !_base)
	{
		const Mod &mod = *engine.getMod();
		SavedGame &game = *engine.getSavedGame();

		std::vector<AlienBase*> possibilities;
		if (_rule.getOperationType() == AMOT_REGION_NEW_BASE)
		{
			// new base, leave possibilities empty
		}
		else
		{
			if (_rule.getOperationType() == AMOT_REGION_EXISTING_BASE || _rule.getOperationType() == AMOT_REGION_NEW_BASE_IF_NECESSARY)
			{
				// region only
				auto* missionRegion = mod.getRegion(_region, true);
				for (auto* ab : *game.getAlienBases())
				{
					if (missionRegion->insideRegion(ab->getLongitude(), ab->getLatitude()))
					{
						possibilities.push_back(ab);
					}
				}
			}
			else
			{
				// whole Earth
				possibilities = *game.getAlienBases();
			}
		}
		if (!possibilities.empty())
		{
			// 1. existing base selected
			int selected = RNG::generate(0, possibilities.size() - 1);
			_base = possibilities[selected];
		}
		else
		{
			if (_rule.getOperationType() == AMOT_REGION_EXISTING_BASE || _rule.getOperationType() == AMOT_EARTH_EXISTING_BASE)
			{
				// 2. no base available, mission aborted!
				_interrupted = true;
			}
			else
			{
				// 3. spawn a new base
				RuleRegion *region = mod.getRegion(_region, true);
				std::vector<MissionArea> areas = region->getMissionZones().at(_rule.getOperationSpawnZone()).areas;
				std::pair<double, double> pos;
				int tries = 0;
				AlienDeployment* operationBaseType = mod.getDeployment(_rule.getOperationBaseType(), true);
				bool wantsToSpawnFakeUnderwater = RNG::percent(operationBaseType->getFakeUnderwaterSpawnChance());
				bool found = false;
				while (!found)
				{
					MissionArea &area = areas.at(RNG::generate(0, areas.size() - 1));
					pos.first = RNG::generate(std::min(area.lonMin, area.lonMax), std::max(area.lonMin, area.lonMax));
					pos.second = RNG::generate(std::min(area.latMin, area.latMax), std::max(area.latMin, area.latMax));
					++tries;

					if (tries == 100)
					{
						found = true; // forced spawn on invalid location
					}
					else if (globe.insideLand(pos.first, pos.second) && region->insideRegion(pos.first, pos.second, true))
					{
						bool isFakeUnderwater = globe.insideFakeUnderwaterTexture(pos.first, pos.second);
						if (wantsToSpawnFakeUnderwater)
						{
							if (isFakeUnderwater) found = true; // found spawn point on fakeUnderwater texture
						}
						else
						{
							if (!isFakeUnderwater) found = true; // found spawn point on land
						}
					}
				}
				AlienBase* newAlienOperationBase = nullptr;
				if (tries < 100 || mod.getAllowAlienBasesOnWrongTextures())
				{
					newAlienOperationBase = spawnAlienBase(0, engine, pos, operationBaseType);
				}
				if (newAlienOperationBase)
				{
					_base = newAlienOperationBase;
				}
				else
				{
					_interrupted = true;
				}
			}
		}
	}
}

/**
 * This function is called when one of the mission's UFOs arrives at it's current destination.
 * It takes care of sending the UFO to the next waypoint, landing UFOs and
 * marking them for removal as required. It must set the game data in a way that the rest of the code
 * understands what to do.
 * @param ufo The UFO that reached it's waypoint.
 * @param engine The game engine, required to get access to game data and game rules.
 * @param globe The earth globe, required to get access to land checks.
 */
void AlienMission::ufoReachedWaypoint(Ufo &ufo, Game &engine, const Globe &globe)
{
	if (_interrupted)
	{
		ufo.setDetected(false);
		ufo.setStatus(Ufo::DESTROYED);
		return;
	}

	const Mod &mod = *engine.getMod();
	SavedGame &game = *engine.getSavedGame();
	const size_t curWaypoint = ufo.getTrajectoryPoint();
	const size_t nextWaypoint = curWaypoint + 1;
	const UfoTrajectory &trajectory = ufo.getTrajectory();
	int waveNumber = ufo.getMissionWaveNumber() > -1 ? ufo.getMissionWaveNumber() : _nextWave - 1;
	if (waveNumber < 0)
	{
		waveNumber = _rule.getWaveCount() - 1;
	}

	const MissionWave &wave = _rule.getWave(waveNumber);
	if (nextWaypoint >= trajectory.getWaypointCount())
	{
		ufo.setDetected(false);
		ufo.setStatus(Ufo::DESTROYED);
		return;
	}
	ufo.setAltitude(trajectory.getAltitude(nextWaypoint));
	ufo.setTrajectoryPoint(nextWaypoint);
	const RuleRegion &regionRules = *mod.getRegion(_region, true);

	{
		std::pair<double, double> pos = getWaypoint(wave, trajectory, nextWaypoint, globe, regionRules, ufo);

		Waypoint *wp = new Waypoint();
		wp->setLongitude(pos.first);
		wp->setLatitude(pos.second);
		ufo.setDestination(wp);
	}

	if (ufo.getAltitude() != "STR_GROUND")
	{
		if (ufo.getLandId() != 0)
		{
			ufo.setLandId(0);
		}
		// Set next waypoint.
		ufo.setSpeed((int)(ufo.getCraftStats().speedMax * trajectory.getSpeedPercentage(nextWaypoint)));
	}
	else
	{
		// UFO landed.
		if (_missionSiteZoneArea != -1 && wave.objective && trajectory.getZone(curWaypoint) == (size_t)(_rule.getSpawnZone()))
		{
			// Remove UFO, replace with MissionSite.
			addScore(ufo.getLongitude(), ufo.getLatitude(), game);

			MissionArea area = regionRules.getMissionZones().at(trajectory.getZone(curWaypoint)).areas.at(_missionSiteZoneArea);
			if (wave.objectiveOnTheLandingSite)
			{
				// Note: 'area' is a local variable; we're not changing the ruleset
				area.lonMin = ufo.getLongitude();
				area.lonMax = ufo.getLongitude();
				area.latMin = ufo.getLatitude();
				area.latMax = ufo.getLatitude();
			}
			MissionSite *missionSite = spawnMissionSite(game, mod, area, &ufo);
			if (missionSite && _rule.respawnUfoAfterSiteDespawn())
			{
				ufo.setStatus(Ufo::IGNORE_ME);
				missionSite->setUfo(&ufo);
			}
			else
			{
				ufo.setStatus(Ufo::DESTROYED);
			}
			if (missionSite)
			{
				for (auto* follower : ufo.getCraftFollowers())
				{
					if (follower->getNumTotalUnits() > 0)
					{
						follower->setDestination(missionSite);
					}
					else if (ufo.getStatus() == Ufo::IGNORE_ME)
					{
						follower->returnToBase(); // no craft is allowed to follow an ignored UFO
					}
				}
			}
		}
		else if (trajectory.getID() == UfoTrajectory::RETALIATION_ASSAULT_RUN)
		{
			// Ignore what the trajectory might say, this is a base assault.
			// Remove UFO, replace with Base defense.
			ufo.setDetected(false);
			Base* found = nullptr;
			for (auto* xbase : *game.getBases())
			{
				if (AreSame(xbase->getLongitude(), ufo.getLongitude()) && AreSame(xbase->getLatitude(), ufo.getLatitude()))
				{
					found = xbase;
					break;
				}
			}
			if (!found)
			{
				ufo.setStatus(Ufo::DESTROYED);
				// Only spawn mission if the base is still there.
				return;
			}
			ufo.setDestination(found);
		}
		else
		{
			bool landingAllowed = true;
			if (!globe.insideLand(ufo.getLongitude(), ufo.getLatitude()))
			{
				landingAllowed = false; // real water
			}
			else if (globe.insideFakeUnderwaterTexture(ufo.getLongitude(), ufo.getLatitude()))
			{
				// decision to land on fake water was done earlier already
				// most of the time it's a proper decision, but sometimes it's a forced decision (i.e. no other option left)
				// because of forced decisions, let's check if the UFO can (at least theoretically) land on fake water...
				// ...and if not, don't land!
				if (ufo.getRules()->getFakeWaterLandingChance() <= 0)
				{
					landingAllowed = false; // UFO was forced to go here, but it's not going to land!
				}
			}
			if (landingAllowed)
			{
				// Set timer for UFO on the ground.
				ufo.setSecondsRemaining(trajectory.groundTimer() * 5);
				if (ufo.getDetected() && ufo.getLandId() == 0)
				{
					ufo.setLandId(engine.getSavedGame()->getId("STR_LANDING_SITE"));
				}

				// Many players wanted this over the years... you're welcome
				if (_rule.getObjective() == OBJECTIVE_BASE && wave.objectiveOnTheLandingSite && trajectory.getZone(curWaypoint) == (size_t)(_rule.getSpawnZone()))
				{
					std::pair<double, double> pos;
					pos.first = ufo.getLongitude();
					pos.second = ufo.getLatitude();

					MissionArea dummyArea;
					AlienDeployment* alienBaseType = chooseAlienBaseType(mod, dummyArea);

					spawnAlienBase(0, engine, pos, alienBaseType);
				}
			}
			else
			{
				// There's nothing to land on
				ufo.setSecondsRemaining(5);
			}
		}
	}
}

/**
 * This function is called when one of the mission's UFOs is shot down (crashed or destroyed).
 * Currently the only thing that happens is delaying the next UFO in the mission sequence.
 * @param ufo The UFO that was shot down.
 */
void AlienMission::ufoShotDown(Ufo &ufo)
{
	switch (ufo.getStatus())
	{
	case Ufo::FLYING:
	case Ufo::LANDED:
		assert(0 && "Ufo seems ok!");
		break;
	case Ufo::CRASHED:
	case Ufo::DESTROYED:
		if (_nextWave != _rule.getWaveCount())
		{
			// Delay next wave
			_spawnCountdown += 30 * (RNG::generate(0, 400) + 48);
		}
		break;
	case Ufo::IGNORE_ME:
		break;
	}
}

/**
 * This function is called when one of the mission's UFOs has finished it's time on the ground.
 * It takes care of sending the UFO to the next waypoint and marking them for removal as required.
 * It must set the game data in a way that the rest of the code understands what to do.
 * @param ufo The UFO that reached it's waypoint.
 * @param game The saved game information.
 */
void AlienMission::ufoLifting(Ufo &ufo, SavedGame &game)
{
	switch (ufo.getStatus())
	{
	case Ufo::FLYING:
		assert(0 && "Ufo is already on the air!");
		break;
	case Ufo::LANDED:
	case Ufo::IGNORE_ME:
		{
			// base missions only get points when they are completed.
			if (_rule.getPoints() > 0 && _rule.getObjective() != OBJECTIVE_BASE)
			{
				addScore(ufo.getLongitude(), ufo.getLatitude(), game);
			}
			ufo.setAltitude("STR_VERY_LOW");
			ufo.setSpeed((int)(ufo.getCraftStats().speedMax * ufo.getTrajectory().getSpeedPercentage(ufo.getTrajectoryPoint())));
		}
		break;
	case Ufo::CRASHED:
		// Mission expired
		ufo.setDetected(false);
		ufo.setStatus(Ufo::DESTROYED);
		break;
	case Ufo::DESTROYED:
		assert(0 && "UFO can't fly!");
		break;
	}
}

/**
 * The new time must be a multiple of 30 minutes, and more than 0.
 * Calling this on a finished mission has no effect.
 * @param minutes The minutes until the next UFO wave will spawn.
 */
void AlienMission::setWaveCountdown(size_t minutes)
{
	assert(minutes != 0 && minutes % 30 == 0);
	if (isOver())
	{
		return;
	}
	_spawnCountdown = minutes;
}

/**
 * Assigns a unique ID to this mission.
 * It is an error to assign two IDs to the same mission.
 * @param id The UD to assign.
 */
void AlienMission::setId(int id)
{
	assert(_uniqueID == 0 && "Reassigning ID!");
	_uniqueID = id;
}

/**
 * @return The unique ID assigned to this mission.
 */
int AlienMission::getId() const
{
	assert(_uniqueID != 0 && "Uninitialized mission!");
	return _uniqueID;
}

/**
 * Sets the alien base associated with this mission.
 * Only the alien supply missions care about this.
 * @param base A pointer to an alien base.
 */
void AlienMission::setAlienBase(const AlienBase *base)
{
	_base = base;
}

/**
 * Only alien supply missions ever have a valid pointer.
 * @return A pointer (possibly 0) of the AlienBase for this mission.
 */
const AlienBase *AlienMission::getAlienBase() const
{
	return _base;
}

/**
 * Add alien points to the country and region at the coordinates given.
 * @param lon Longitudinal coordinates to check.
 * @param lat Latitudinal coordinates to check.
 * @param game The saved game information.
 */
void AlienMission::addScore(double lon, double lat, SavedGame &game) const
{
	if (_rule.getObjective() == OBJECTIVE_INFILTRATION)
		return; // pact score is a special case
	for (auto* region : *game.getRegions())
	{
		if (region->getRules()->insideRegion(lon, lat))
		{
			region->addActivityAlien(_rule.getPoints());
			break;
		}
	}
	for (auto* country : *game.getCountries())
	{
		if (country->getRules()->insideCountry(lon, lat))
		{
			country->addActivityAlien(_rule.getPoints());
			break;
		}
	}
}

/**
 * Spawn an alien base.
 * @param pactCountry A country that has signed a pact with the aliens and allowed them to build this base.
 * @param engine The game engine, required to get access to game data and game rules.
 * @param pos The base coordinates.
 * @param deployment The base type.
 * @return Pointer to the spawned alien base.
 */
AlienBase *AlienMission::spawnAlienBase(Country *pactCountry, Game &engine, std::pair<double, double> pos, AlienDeployment *deployment)
{
	SavedGame &game = *engine.getSavedGame();
	AlienBase *ab = new AlienBase(deployment, game.getMonthsPassed());
	if (pactCountry)
	{
		ab->setPactCountry(pactCountry->getRules()->getType());
	}
	ab->setAlienRace(_race);
	ab->setId(game.getId(deployment->getMarkerName()));
	ab->setLongitude(pos.first);
	ab->setLatitude(pos.second);
	game.getAlienBases()->push_back(ab);
	addScore(ab->getLongitude(), ab->getLatitude(), game);

	if (Options::oxceGeoscapeDebugLogMaxEntries > 0)
	{
		std::ostringstream ss;
		ss << "gameTime: " << game.getTime()->getFullString();
		ss << " baseId: " << ab->getId();
		ss << " baseType: " << ab->getType();
		ss << " race: " << _race;
		ss << " region: " << _region;
		ss << " deployment: " << deployment->getType();
		ss << " missionId: " << _uniqueID;
		ss << " missionType: " << _rule.getType();
		game.getGeoscapeDebugLog().push_back(ss.str());
	}

	return ab;
}

/**
 * Chooses a mission type for a new alien base.
 */
AlienDeployment *AlienMission::chooseAlienBaseType(const Mod &mod, const MissionArea &area)
{
	AlienDeployment *deployment = nullptr;
	if (!_rule.getSiteType().empty())
	{
		deployment = mod.getDeployment(_rule.getSiteType(), true);
	}
	if (!deployment)
	{
		Texture *texture = mod.getGlobe()->getTexture(area.texture);
		if (texture && !texture->getDeployments().empty())
		{
			// Note: only used in Area51 mod (as of April 2020)
			deployment = mod.getDeployment(texture->getRandomDeployment(), true);
		}
	}
	if (!deployment)
	{
		// Note: emergency fall-back
		deployment = mod.getDeployment("STR_ALIEN_BASE_ASSAULT", true);
	}
	return deployment;
}

/*
 * Sets the mission's region. if the region is incompatible with
 * actually carrying out an attack, use the "fallback" region as
 * defined in the ruleset.
 * (this is a slight difference from the original, which just
 * defaulted them to zone[0], North America)
 * @param region the region we want to try to set the mission to.
 * @param mod the mod, in case we need to swap out the region.
 */
void AlienMission::setRegion(const std::string &region, const Mod &mod)
{
	RuleRegion *r = mod.getRegion(region, true);
	if (!r->getMissionRegion().empty())
	{
		_region = r->getMissionRegion();
	}
	else
	{
		_region = region;
	}
}

/**
 * Select a destination based on the criteria of our trajectory and desired waypoint.
 * @param trajectory the trajectory in question.
 * @param nextWaypoint the next logical waypoint in sequence (0 for newly spawned UFOs)
 * @param globe The earth globe, required to get access to land checks.
 * @param region the ruleset for the region of our mission.
 * @param ufo Required when making landing decisions on fake water.
 * @return a set of lon and lat coordinates based on the criteria of the trajectory.
 */
std::pair<double, double> AlienMission::getWaypoint(const MissionWave &wave, const UfoTrajectory &trajectory, const size_t nextWaypoint, const Globe &globe, const RuleRegion &region, const Ufo &ufo)
{
	if (trajectory.getZone(nextWaypoint) >= region.getMissionZones().size())
	{
		logMissionError(trajectory.getZone(nextWaypoint), region);
	}

	if (_missionSiteZoneArea != -1 && wave.objective && trajectory.getZone(nextWaypoint) == (size_t)(_rule.getSpawnZone()))
	{
		if (wave.objectiveOnTheLandingSite)
		{
			return getLandPointForMissionSite(globe, region, _rule.getSpawnZone(), _missionSiteZoneArea, ufo);
		}
		const MissionArea *area = &region.getMissionZones().at(_rule.getSpawnZone()).areas.at(_missionSiteZoneArea);
		return std::make_pair(area->lonMin, area->latMin);
	}

	// if we are an Earth-based operation
	if (_rule.getOperationType() != AMOT_SPACE && _base)
	{
		// and this is our last waypoint
		if (nextWaypoint >= trajectory.getWaypointCount() - 1)
		{
			// finish our business at the operation base
			return std::make_pair(_base->getLongitude(), _base->getLatitude());
		}
	}
	// Note: if the operation base is destroyed in the meantime, remaining live UFOs will follow the original trajectory...
	// ...therefore it is important to have a valid last trajectory waypoint even for such missions

	if (trajectory.getWaypointCount() > nextWaypoint + 1 && trajectory.getAltitude(nextWaypoint + 1) == "STR_GROUND")
	{
		return getLandPoint(globe, region, trajectory.getZone(nextWaypoint), ufo);
	}
	return region.getRandomPoint(trajectory.getZone(nextWaypoint));
}

/**
 * Get a random point inside the given region zone.
 * The point will be used to land a UFO, so it HAS to be on land (UNLESS it's landing on a city).
 * @param globe reference to the globe data.
 * @param region reference to the region we want a land point in.
 * @param zone the missionZone set within the region to find a landing zone in.
 * @param ufo Required when making landing decisions on fake water.
 * @return a set of longitudinal and latitudinal coordinates.
 */
std::pair<double, double> AlienMission::getLandPoint(const Globe &globe, const RuleRegion &region, size_t zone, const Ufo &ufo)
{
	if (zone >= region.getMissionZones().size() || region.getMissionZones().at(zone).areas.size() == 0)
	{
		logMissionError(zone, region);
	}

	std::pair<double, double> pos;

	if (region.getMissionZones().at(zone).areas.at(0).isPoint()) // if a UFO wants to land on a city, let it.
	{
		pos = region.getRandomPoint(zone);
	}
	else
	{
		int tries = 0;
		bool wantsToLandOnFakeWater = RNG::percent(ufo.getRules()->getFakeWaterLandingChance());
		bool found = false;
		while (!found)
		{
			pos = region.getRandomPoint(zone);
			++tries;

			if (tries == 100)
			{
				found = true; // forced decision
			}
			else if (globe.insideLand(pos.first, pos.second) && region.insideRegion(pos.first, pos.second))
			{
				bool isFakeWater = globe.insideFakeUnderwaterTexture(pos.first, pos.second);
				if (wantsToLandOnFakeWater)
				{
					if (isFakeWater)
					{
						found = true; // found landing point on fake water
					}
				}
				else
				{
					if (!isFakeWater)
					{
						found = true; // found landing point on land
					}
				}
			}
		}

		if (tries == 100)
		{
			Log(LOG_WARNING) << "Region: " << region.getType() << " Longitude: " << pos.first << " Latitude: " << pos.second << " invalid zone: " << zone << " ufo forced to land on water (or invalid texture)!";
			if (wantsToLandOnFakeWater)
			{
				Log(LOG_WARNING) << "UFO: " << ufo.getRules()->getType() << " wanted to land on fake water.";
			}
		}
	}
	return pos;
}

/**
 * Get a random point for the given region, zone and area.
 * The point doesn't necessarily need to be inside the region boundaries.
 * The point will be used to land a UFO, which immediately converts into a mission site, so it HAS to be on land (or fake water).
 */
std::pair<double, double> AlienMission::getLandPointForMissionSite(const Globe& globe, const RuleRegion& region, size_t zone, int area, const Ufo& ufo)
{
	// Full error report in case of corrupted saves and/or modding-related issues
	// Note: checking only areas; zones were already checked earlier in getWaypoint()
	if (region.getMissionZones().at(zone).areas.empty())
	{
		std::stringstream ss;
		ss << "Error occurred while trying to determine land+site point for mission type: " << _rule.getType() << " in region: " << region.getType() << ", in zone: " << zone;
		ss << ", zone has no valid areas.";
		throw Exception(ss.str());
	}
	else if ((size_t)area >= region.getMissionZones().at(zone).areas.size())
	{
		std::stringstream ss;
		ss << "Error occurred while trying to determine land+site point for mission type: " << _rule.getType() << " in region: " << region.getType() << ", in zone: " << zone;
		ss << ", mission tried to find a land+site point in area " << area;
		ss << ", but this zone only has areas valid up to " << region.getMissionZones().at(zone).areas.size() - 1 << ".";
		throw Exception(ss.str());
	}

	std::pair<double, double> pos;
	{
		int tries = 0;
		bool wantsToLandOnFakeWater = RNG::percent(ufo.getRules()->getFakeWaterLandingChance());
		bool found = false;
		while (!found)
		{
			pos = region.getRandomPoint(zone, area); // pass the area as a parameter too!
			++tries;

			if (tries == 100)
			{
				found = true; // forced decision
			}
			else if (globe.insideLand(pos.first, pos.second) /* && region.insideRegion(pos.first, pos.second) */) // doesn't need to be inside the region!
			{
				bool isFakeWater = globe.insideFakeUnderwaterTexture(pos.first, pos.second);
				if (wantsToLandOnFakeWater)
				{
					if (isFakeWater)
					{
						found = true; // found landing point on fake water
					}
				}
				else
				{
					if (!isFakeWater)
					{
						found = true; // found landing point on land
					}
				}
			}
		}

		if (tries == 100)
		{
			Log(LOG_WARNING) << "Region: " << region.getType() << " Longitude: " << pos.first << " Latitude: " << pos.second << " zone: " << zone << " area: " << area << " ufo forced to land on water (or invalid texture)!";
			if (wantsToLandOnFakeWater)
			{
				Log(LOG_WARNING) << "UFO: " << ufo.getRules()->getType() << " wanted to land on fake water.";
			}
		}
	}
	return pos;
}

/**
 * Attempt to spawn a Mission Site at a given location.
 * @param game reference to the saved game.
 * @param rules reference to the game rules.
 * @param area the point on the globe at which to spawn this site.
 * @param ufo ufo that spawn that mission.
 * @return a pointer to the mission site.
 */
MissionSite *AlienMission::spawnMissionSite(SavedGame &game, const Mod &mod, const MissionArea &area, const Ufo *ufo, AlienDeployment *missionOveride)
{
	Texture *texture = mod.getGlobe()->getTexture(area.texture);
	AlienDeployment *deployment = nullptr;
	AlienDeployment *alienCustomDeploy = ufo ? mod.getDeployment(ufo->getCraftStats().missionCustomDeploy) : 0;

	if (missionOveride)
	{
		deployment = missionOveride;
	}
	else if (mod.getDeployment(_rule.getSiteType()))
	{
		deployment = mod.getDeployment(_rule.getSiteType());
	}
	else
	{
		if (!texture)
		{
			throw Exception("Error occurred while spawning mission site: " + _rule.getType());
		}
		deployment = mod.getDeployment(texture->getRandomDeployment(), true);
	}

	if (deployment)
	{
		MissionSite *missionSite = new MissionSite(&_rule, deployment, alienCustomDeploy);
		missionSite->setLongitude(RNG::generate(area.lonMin, area.lonMax));
		missionSite->setLatitude(RNG::generate(area.latMin, area.latMax));
		missionSite->setId(game.getId(deployment->getMarkerName()));
		missionSite->setSecondsRemaining(RNG::generate(deployment->getDurationMin(), deployment->getDurationMax()) * 3600);
		missionSite->setAlienRace(_race);
		missionSite->setTexture(area.texture);
		missionSite->setCity(area.name);
		game.getMissionSites()->push_back(missionSite);

		if (Options::oxceGeoscapeDebugLogMaxEntries > 0)
		{
			std::ostringstream ss;
			ss << "gameTime: " << game.getTime()->getFullString();
			ss << " siteId: " << missionSite->getId();
			ss << " siteType: " << missionSite->getType();
			ss << " race: " << _race;
			ss << " region: " << _region;
			if (!missionSite->getCity().empty())
			{
				ss << " city: " << missionSite->getCity();
			}
			ss << " deployment: " << deployment->getType();
			if (alienCustomDeploy)
			{
				ss << " / " << alienCustomDeploy->getType();
			}
			ss << " missionId: " << _uniqueID;
			ss << " missionType: " << _rule.getType();
			game.getGeoscapeDebugLog().push_back(ss.str());
		}

		return missionSite;
	}
	return 0;
}

/**
 * Tell the mission which entry in the 'areas' array we're targetting for our missionSite payload.
 * @param area the number of the area to target, synonymous with a city.
 */
void AlienMission::setMissionSiteZoneArea(int area)
{
	_missionSiteZoneArea = area;
}

void AlienMission::logMissionError(int zone, const RuleRegion &region)
{
	if (!region.getMissionZones().empty())
	{
		std::stringstream ss, ss2;
		ss << zone;
		ss2 << region.getMissionZones().size() - 1;
		throw Exception("Error occurred while trying to determine waypoint for mission type: " + _rule.getType() + " in region: " + region.getType() + ", mission tried to find a waypoint in zone " + ss.str() + " but this region only has zones valid up to " + ss2.str() + ".");
	}
	else
	{
		throw Exception("Error occurred while trying to determine waypoint for mission type: " + _rule.getType() + " in region: " + region.getType() + ", region has no valid zones.");
	}
}

}
