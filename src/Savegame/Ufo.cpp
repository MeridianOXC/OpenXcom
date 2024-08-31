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
#include "Ufo.h"
#include <assert.h>
#include <algorithm>
#include <functional>
#include "../fmath.h"
#include "Craft.h"
#include "AlienMission.h"
#include "../Engine/Exception.h"
#include "../Engine/Language.h"
#include "../Engine/RNG.h"
#include "../Engine/ScriptBind.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleUfo.h"
#include "../Mod/UfoTrajectory.h"
#include "../Mod/RuleAlienMission.h"
#include "Base.h"
#include "SavedGame.h"
#include "Waypoint.h"

namespace OpenXcom
{

const char *Ufo::ALTITUDE_STRING[] = {
	"STR_GROUND",
	"STR_VERY_LOW",
	"STR_LOW_UC",
	"STR_HIGH_UC",
	"STR_VERY_HIGH"
};

/**
 * Initializes a UFO of the specified type.
 * @param rules Pointer to ruleset.
 * @param uniqueId unique ID to assign to the UFO (0 to not assign).
 */
Ufo::Ufo(const RuleUfo *rules, int uniqueId, int hunterKillerPercentage, int huntMode, int huntBehavior) : MovingTarget(),
	_rules(rules), _missionWaveNumber(-1), _crashId(0), _landId(0), _damage(0), _direction("STR_NORTH"),
	_altitude("STR_HIGH_UC"), _status(FLYING), _secondsRemaining(0),
	_inBattlescape(false), _mission(0), _trajectory(0),
	_trajectoryPoint(0), _detected(false), _hyperDetected(false), _processedIntercept(false),
	_shootingAt(0), _hitFrame(0), _fireCountdown(0), _escapeCountdown(0), _stats(), _shield(-1), _shieldRechargeHandle(0),
	_tractorBeamSlowdown(0), _isHunterKiller(false), _isEscort(false), _huntMode(0), _huntBehavior(0),
	_isHunting(false), _isEscorting(false), _softlockShotCounter(0), _origWaypoint(0)
{
	_stats = rules->getStats();
	if (uniqueId != 0)
	{
		_uniqueId = uniqueId;
	}
	if (hunterKillerPercentage > 0)
	{
		_isHunterKiller = RNG::percent(hunterKillerPercentage);
		if (_isHunterKiller)
		{
			_huntMode = huntMode > 1 ? RNG::generate(0, 1) : huntMode;
			_huntBehavior = huntBehavior > 1 ? RNG::generate(0, 1) : huntBehavior;
		}
	}
}

/**
 * Make sure our mission forget's us, and we only delete targets we own (waypoints).
 *
 */
Ufo::~Ufo()
{
	if (_mission)
	{
		_mission->decreaseLiveUfos();
	}

	resetOriginalDestination(false);

	if (_dest)
	{
		Waypoint *wp = dynamic_cast<Waypoint*>(_dest);
		if (wp != 0)
		{
			delete _dest;
			_dest = 0;
		}
	}
}

/**
 * Loads the UFO from a YAML file.
 * @param node YAML node.
 * @param mod The game mod. Use to access the trajectory rules.
 * @param game The game data. Used to find the UFO's mission.
 */
void Ufo::load(const YAML::Node &node, const ScriptGlobal *shared, const Mod &mod, SavedGame &game)
{
	MovingTarget::load(node);
	_uniqueId = node["uniqueId"].as<int>(_uniqueId);
	_missionWaveNumber = node["missionWaveNumber"].as<int>(_missionWaveNumber);
	_crashId = node["crashId"].as<int>(_crashId);
	_landId = node["landId"].as<int>(_landId);
	_damage = node["damage"].as<int>(_damage);
	_shield = node["shield"].as<int>(_shield);
	_shieldRechargeHandle = node["shieldRechargeHandle"].as<int>(_shieldRechargeHandle);
	_altitude = node["altitude"].as<std::string>(_altitude);
	_direction = node["direction"].as<std::string>(_direction);
	_detected = node["detected"].as<bool>(_detected);
	_hyperDetected = node["hyperDetected"].as<bool>(_hyperDetected);
	_secondsRemaining = node["secondsRemaining"].as<size_t>(_secondsRemaining);
	_inBattlescape = node["inBattlescape"].as<bool>(_inBattlescape);
	double lon = _lon;
	double lat = _lat;
	if (const YAML::Node &dest = node["dest"])
	{
		lon = dest["lon"].as<double>();
		lat = dest["lat"].as<double>();
	}
	_dest = new Waypoint();
	_dest->setLongitude(lon);
	_dest->setLatitude(lat);
	if (const YAML::Node &status = node["status"])
	{
		_status = (UfoStatus)status.as<int>();
	}
	if (game.getMonthsPassed() != -1)
	{
		int missionID = node["mission"].as<int>();
		AlienMission* found = nullptr;
		for (auto* am : game.getAlienMissions())
		{
			if (am->getId() == missionID)
			{
				found = am;
				break;
			}
		}
		if (!found)
		{
			// Corrupt save file.
			throw Exception("Unknown UFO mission, save file is corrupt.");
		}
		_mission = found;
		_stats += _rules->getRaceBonus(_mission->getRace());

		std::string tid = node["trajectory"].as<std::string>();
		_trajectory = mod.getUfoTrajectory(tid);
		if (_trajectory == 0)
		{
			// Corrupt save file.
			throw Exception("Unknown UFO trajectory, save file is corrupt.");
		}
		_trajectoryPoint = node["trajectoryPoint"].as<size_t>(_trajectoryPoint);
	}
	_fireCountdown = node["fireCountdown"].as<int>(_fireCountdown);
	_escapeCountdown = node["escapeCountdown"].as<int>(_escapeCountdown);
	if (_inBattlescape)
		setSpeed(0);

	_scriptValues.load(node, shared);
}

/**
 * Finishes loading the UFO from YAML (called after XCOM craft and other UFOs are loaded).
 * @param node YAML node.
 * @param save The game data. Used to find the UFO's target (= XCOM craft or another UFO).
 */
void Ufo::finishLoading(const YAML::Node &node, SavedGame &save)
{
	_isHunterKiller = node["isHunterKiller"].as<bool>(_isHunterKiller);
	_isEscort = node["isEscort"].as<bool>(_isEscort);
	_huntMode = node["huntMode"].as<int>(_huntMode);
	_huntBehavior = node["huntBehavior"].as<int>(_huntBehavior);
	_isHunting = node["isHunting"].as<bool>(_isHunting);
	_isEscorting = node["isEscorting"].as<bool>(_isEscorting);
	_softlockShotCounter = node["softlockShotCounter"].as<int>(_softlockShotCounter);

	if (_isHunting)
	{
		if (const YAML::Node &dest = node["dest"])
		{
			std::string type = dest["type"].as<std::string>();
			int id = dest["id"].as<int>();
			bool found = false;
			for (auto* xbase : *save.getBases())
			{
				for (auto* xcraft : *xbase->getCrafts())
				{
					if (xcraft->getId() == id && xcraft->getRules()->getType() == type)
					{
						if (_dest)
						{
							// this is just a dummy waypoint created during normal loading, not a craft... yet
							delete _dest;
							_dest = 0;
						}
						setDestination(xcraft);
						found = true;
						break;
					}
				}
				if (found) break;
			}
		}
	}
	else if (_isEscorting)
	{
		if (const YAML::Node &dest = node["dest"])
		{
			std::string type = dest["type"].as<std::string>();
			if (type == "STR_UFO")
			{
				int uniqueUfoId = dest["uniqueId"].as<int>(0);
				if (uniqueUfoId > 0)
				{
					for (auto* ufo : *save.getUfos())
					{
						if (ufo->getUniqueId() == uniqueUfoId)
						{
							if (_dest)
							{
								// this is just a dummy waypoint created during normal loading, not a UFO... yet
								delete _dest;
								_dest = 0;
							}
							setDestination(ufo);
							break;
						}
					}
				}
			}
		}
	}
	//if (_isHunting || _isEscorting)
	{
		if (const YAML::Node &origWaypoint = node["origWaypoint"])
		{
			_origWaypoint = new Waypoint();
			_origWaypoint->setLongitude(origWaypoint["lon"].as<double>());
			_origWaypoint->setLatitude(origWaypoint["lat"].as<double>());
		}
	}
}

/**
 * Saves the UFO to a YAML file.
 * @return YAML node.
 */
YAML::Node Ufo::save(const ScriptGlobal *shared, bool newBattle) const
{
	YAML::Node node = MovingTarget::save();
	node["type"] = _rules->getType();
	node["uniqueId"] = _uniqueId;
	node["missionWaveNumber"] = _missionWaveNumber;
	if (_crashId)
	{
		node["crashId"] = _crashId;
	}
	else if (_landId)
	{
		node["landId"] = _landId;
	}
	node["damage"] = _damage;
	node["shield"] = _shield;
	node["shieldRechargeHandle"] = _shieldRechargeHandle;
	node["altitude"] = _altitude;
	node["direction"] = _direction;
	node["status"] = (int)_status;
	if (_detected)
		node["detected"] = _detected;
	if (_hyperDetected)
		node["hyperDetected"] = _hyperDetected;
	if (_secondsRemaining)
		node["secondsRemaining"] = _secondsRemaining;
	if (_inBattlescape)
		node["inBattlescape"] = _inBattlescape;

	if (_isHunterKiller)
		node["isHunterKiller"] = _isHunterKiller;
	if (_isEscort)
		node["isEscort"] = _isEscort;
	node["huntMode"] = _huntMode;
	node["huntBehavior"] = _huntBehavior;
	if (_isHunting)
		node["isHunting"] = _isHunting;
	if (_isEscorting)
		node["isEscorting"] = _isEscorting;
	if (_softlockShotCounter > 0)
		node["softlockShotCounter"] = _softlockShotCounter;
	if (_origWaypoint)
		node["origWaypoint"] = _origWaypoint->save();

	if (!newBattle)
	{
		node["mission"] = _mission->getId();
		node["trajectory"] = _trajectory->getID();
		node["trajectoryPoint"] = _trajectoryPoint;
	}

	node["fireCountdown"] = _fireCountdown;
	node["escapeCountdown"] = _escapeCountdown;

	_scriptValues.save(node, shared);

	return node;
}

/**
 * Saves the UFO's unique identifiers to a YAML file.
 * @return YAML node.
 */
YAML::Node Ufo::saveId() const
{
	YAML::Node node = MovingTarget::saveId();
	// this is needed, because _id is NOT unique until the UFO is detected
	// and UFOs can be referenced by other entities even before they are detected
	// (e.g. when they escort other UFOs)
	node["uniqueId"] = _uniqueId;
	return node;
}

/**
 * Returns the UFO's unique type used for
 * savegame purposes.
 * @return ID.
 */
std::string Ufo::getType() const
{
	return "STR_UFO";
}

/**
 * Returns the ruleset for the UFO's type.
 * @return Pointer to ruleset.
 */
const RuleUfo *Ufo::getRules() const
{
	return _rules;
}

/**
 * Changes the ruleset for the UFO's type.
 * @param rules Pointer to ruleset.
 * @warning ONLY FOR NEW BATTLE USE!
 */
void Ufo::changeRules(const RuleUfo *rules)
{
	_rules = rules;
}

/**
 * Returns the UFO's unique ID.
 * @return Unique ID.
 */
int Ufo::getUniqueId() const
{
	return _uniqueId;
}

/**
 * Returns the UFO's unique default name.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::string Ufo::getDefaultName(Language *lang) const
{
	switch (_status)
	{
	case LANDED:
		return lang->getString(getMarkerName()).arg(_landId);
	case CRASHED:
		return lang->getString(getMarkerName()).arg(_crashId);
	default:
		return lang->getString(getMarkerName()).arg(_id);
	}
}

/**
 * Returns the name on the globe for the UFO.
 * @return String ID.
 */
std::string Ufo::getMarkerName() const
{
	switch (_status)
	{
	case LANDED:
		return "STR_LANDING_SITE_";
	case CRASHED:
		return "STR_CRASH_SITE_";
	default:
		return "STR_UFO_";
	}
}

/**
 * Returns the marker ID on the globe for the UFO.
 * @return Marker ID.
 */
int Ufo::getMarkerId() const
{
	switch (_status)
	{
	case LANDED:
		return _landId;
	case CRASHED:
		return _crashId;
	default:
		return _id;
	}
}

/**
 * Returns the globe marker for the UFO.
 * @return Marker sprite, -1 if none.
 */
int Ufo::getMarker() const
{
	if (!_detected)
		return -1;
	switch (_status)
	{
	case LANDED:
		return _rules->getLandMarker() == -1 ? 3 : _rules->getLandMarker();
	case CRASHED:
		return _rules->getCrashMarker() == -1 ? 4 : _rules->getCrashMarker();
	default:
		return _rules->getMarker() == -1 ? 2 : _rules->getMarker();
	}
}

/**
 * Returns the amount of damage this UFO has taken.
 * @return Amount of damage.
 */
int Ufo::getDamage() const
{
	return _damage;
}

/**
 * Changes the amount of damage this UFO has taken.
 * @param damage Amount of damage.
 */
void Ufo::setDamage(int damage, const Mod *mod)
{
	_damage = damage;
	if (_damage < 0)
	{
		_damage = 0;
	}
	if (isDestroyed())
	{
		_status = DESTROYED;
	}
	else if (isCrashed())
	{
		_status = CRASHED;
	}
	if (_status == CRASHED || _status == DESTROYED)
	{
		int waveNumber = _missionWaveNumber;

		// special case for retaliation UFO attacking the base
		const RuleUfo *battleshipRule = mod->getUfo(_mission->getRules().getSpawnUfo(), true);
		const UfoTrajectory *assaultTrajectory = mod->getUfoTrajectory(UfoTrajectory::RETALIATION_ASSAULT_RUN, true);
		if (_rules == battleshipRule && _trajectory == assaultTrajectory)
		{
			waveNumber = _mission->getRules().getWaveCount() - 1; // last wave
		}

		// backwards save compatibility
		if (waveNumber > -1)
		{
			const MissionWave &wave = _mission->getRules().getWave(waveNumber);
			if (wave.interruptPercentage > 0)
			{
				if (RNG::percent(wave.interruptPercentage))
				{
					_mission->setInterrupted(true);
				}
			}
		}
	}
}

/**
 * Returns the ratio between the amount of damage this
 * ufo can take and the total it can take before it's
 * destroyed.
 * @return Percentage of damage.
 */
int Ufo::getDamagePercentage() const
{
	return (int)floor((double)_damage / _stats.damageMax * 100);
}

/**
 * Returns whether this UFO has been detected by radars.
 * @return Detection status.
 */
bool Ufo::getDetected() const
{
	return _detected;
}

/**
 * Changes whether this UFO has been detected by radars.
 * @param detected Detection status.
 */
void Ufo::setDetected(bool detected)
{
	_detected = detected;
}

/**
 * Returns the amount of remaining seconds the UFO has left on the ground.
 * After this many seconds the UFO will take off, if landed, or disappear, if
 * crashed.
 * @return Amount of seconds.
 */
size_t Ufo::getSecondsRemaining() const
{
	return _secondsRemaining;
}

/**
 * Changes the amount of remaining seconds the UFO has left on the ground.
 * After this many seconds the UFO will take off, if landed, or disappear, if
 * crashed.
 * @param seconds Amount of seconds.
 */
void Ufo::setSecondsRemaining(size_t seconds)
{
	_secondsRemaining = seconds;
}

/**
 * Returns the current direction the UFO is heading in.
 * @return Direction.
 */
std::string Ufo::getDirection() const
{
	return _direction;
}

/**
 * Returns the current altitude of the UFO.
 * @return Altitude as string ID.
 */
std::string Ufo::getAltitude() const
{
	return _altitude;
}

/**
 * Returns the current altitude of the UFO.
 * @return Altitude as integer (0-4).
 */
int Ufo::getAltitudeInt() const
{
	for (size_t i = 0; i < 5; ++i)
	{
		if (ALTITUDE_STRING[i] == _altitude)
		{
			return i;
		}
	}
	return -1;
}

/**
 * Changes the current altitude of the UFO.
 * @param altitude Altitude.
 */
void Ufo::setAltitude(const std::string &altitude)
{
	_altitude = altitude;
	if (_altitude != "STR_GROUND")
	{
		_status = FLYING;
	}
	else
	{
		_status = isCrashed() ? CRASHED : LANDED;
	}
}

/**
 * Returns if this UFO took enough damage
 * to cause it to crash.
 * @return Crashed status.
 */
bool Ufo::isCrashed() const
{
	// Note: yes, this condition is necessary (in OXCE) and cannot be removed!
	if (isDestroyed())
		return true;

	if (_huntBehavior == 1 || _rules->isUnmanned())
	{
		// kamikaze never crash lands; unmanned ditto
		return false;
	}

	return (_damage > _stats.damageMax / 2);
}

/**
 * Returns if this UFO took enough damage
 * to cause it to crash.
 * @return Crashed status.
 */
bool Ufo::isDestroyed() const
{
	return (_damage >= _stats.damageMax);
}

/**
 * Calculates the direction for the UFO based
 * on the current raw speed and destination.
 */
void Ufo::calculateSpeed()
{
	MovingTarget::calculateSpeed();

	double x = _speedLon;
	double y = -_speedLat;

	// This section guards vs. divide-by-zero.
	if (AreSame(x, 0.0) || AreSame(y, 0.0))
	{
		if (AreSame(x, 0.0) && AreSame(y, 0.0))
		{
			_direction = "STR_NONE_UC";
		}
		else if (AreSame(x, 0.0))
		{
			if (y > 0.f)
			{
				_direction = "STR_NORTH";
			}
			else if (y < 0.f)
			{
				_direction = "STR_SOUTH";
			}
		}
		else if (AreSame(y, 0.0))
		{
			if (x > 0.f)
			{
				_direction = "STR_EAST";
			}
			else if (x < 0.f)
			{
				_direction = "STR_WEST";
			}
		}

		return;
	}

	double theta = atan2(y, x); // radians
	theta = theta * 180.f / M_PI; // +/- 180 deg.

	if (22.5f > theta && theta > -22.5f)
	{
		_direction = "STR_EAST";
	}
	else if (-22.5f > theta && theta > -67.5f)
	{
		_direction = "STR_SOUTH_EAST";
	}
	else if (-67.5f > theta && theta > -112.5f)
	{
		_direction = "STR_SOUTH";
	}
	else if (-112.5f > theta && theta > -157.5f)
	{
		_direction = "STR_SOUTH_WEST";
	}
	else if (-157.5f > theta || theta > 157.5f)
	{
		_direction = "STR_WEST";
	}
	else if (157.5f > theta && theta > 112.5f)
	{
		_direction = "STR_NORTH_WEST";
	}
	else if (112.5f > theta && theta > 67.5f)
	{
		_direction = "STR_NORTH";
	}
	else
	{
		_direction = "STR_NORTH_EAST";
	}
}

/**
 * Moves the UFO to its destination.
 */
void Ufo::think()
{
	switch (_status)
	{
	case FLYING:
		move();
		if (reachedDestination() && !isHunting() && !isEscorting())
		{
			// Prevent further movement.
			setSpeed(0);
		}
		break;
	case LANDED:
		assert(_secondsRemaining >= 5 && "Wrong time management.");
		_secondsRemaining -= 5;
		break;
	case CRASHED:
		if (!_detected)
		{
			_detected = true;
		}
		// This gets handled in GeoscapeState::time30Minutes()
		// Because the original game processes it every 30 minutes!
	case DESTROYED:
	case IGNORE_ME:
		// Do nothing
		break;
	}
}

/**
 * Gets the UFO's battlescape status.
 * @return Is the UFO currently in battle?
 */
bool Ufo::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Sets the UFO's battlescape status.
 * @param inbattle True if it's in battle, False otherwise.
 */
void Ufo::setInBattlescape(bool inbattle)
{
	if (inbattle)
		setSpeed(0);
	_inBattlescape = inbattle;
}

/**
 * Returns the alien race currently residing in the UFO.
 * @return Alien race.
 */
const std::string &Ufo::getAlienRace() const
{
	return _mission->getRace();
}

void Ufo::setShotDownByCraftId(const CraftId& craft)
{
	_shotDownByCraftId = craft;
}

CraftId Ufo::getShotDownByCraftId() const
{
	return _shotDownByCraftId;
}

/**
 * Returns a UFO's visibility to radar detection.
 * The UFO's size and altitude affect the chances
 * of it being detected by radars.
 * @return Visibility modifier.
 */
int Ufo::getVisibility() const
{
	int size = 0;
	// size = 15*(3-ufosize);
	if (_rules->getSize() == "STR_VERY_SMALL")
		size = -30;
	else if (_rules->getSize() == "STR_SMALL")
		size = -15;
	else if (_rules->getSize() == "STR_MEDIUM_UC")
		size = 0;
	else if (_rules->getSize() == "STR_LARGE")
		size = 15;
	else if (_rules->getSize() == "STR_VERY_LARGE")
		size = 30;

	int visibility = 0;
	if (_altitude == "STR_GROUND")
		visibility = -30;
	else if (_altitude == "STR_VERY_LOW")
		visibility = size - 20;
	else if (_altitude == "STR_LOW_UC")
		visibility = size - 10;
	else if (_altitude == "STR_HIGH_UC")
		visibility = size;
	else if (_altitude == "STR_VERY_HIGH")
		visibility = size - 10;

	return visibility;
}


/**
 * Returns the Mission type of the UFO.
 * @return Mission.
 */
const std::string &Ufo::getMissionType() const
{
	return _mission->getRules().getType();
}

/**
 * Sets the mission information of the UFO.
 * The UFO will start at the first point of the trajectory. The actual UFO
 * information is not changed here, this only sets the information kept on
 * behalf of the mission.
 * @param mission Pointer to the actual mission object.
 * @param trajectory Pointer to the actual mission trajectory.
 */
void Ufo::setMissionInfo(AlienMission *mission, const UfoTrajectory *trajectory)
{
	assert(!_mission && mission && trajectory);
	_mission = mission;
	_mission->increaseLiveUfos();
	_trajectoryPoint = 0;
	_trajectory = trajectory;
	_stats += _rules->getRaceBonus(_mission->getRace());
}

/**
 * Returns whether this UFO has been detected by hyper-wave.
 * @return Detection status.
 */
bool Ufo::getHyperDetected() const
{
	return _hyperDetected;
}

/**
 * Changes whether this UFO has been detected by hyper-wave.
 * @param hyperdetected Detection status.
 */
void Ufo::setHyperDetected(bool hyperdetected)
{
	_hyperDetected = hyperdetected;
}

/**
 * Gets the Xcom craft targeted by this UFO.
 * @return Pointer to Xcom craft.
 */
Craft *Ufo::getTargetedXcomCraft() const
{
	Craft *craft = dynamic_cast<Craft*>(_dest);
	return craft;
}

/**
 * Resets the original destination if targeting the given craft.
 * @param dest Pointer to Xcom craft.
 */
void Ufo::resetOriginalDestination(Craft *target)
{
	if (target == getTargetedXcomCraft())
	{
		resetOriginalDestination(true);
	}
}

/**
 * Resets the original destination.
 */
void Ufo::resetOriginalDestination(bool debugHelper)
{
	if (_origWaypoint)
	{
		Waypoint *wp = new Waypoint();
		wp->setLatitude(_origWaypoint->getLatitude());
		wp->setLongitude(_origWaypoint->getLongitude());

		setDestination(wp); // hunting and escorting flags will be reset too

		delete _origWaypoint;
		_origWaypoint = 0;
	}
	else if (debugHelper)
	{
		throw Exception("Corrupt state: Unknown original UFO destination.");
	}
}

/**
 * Sets the Xcom craft targeted by this UFO.
 * @param dest Pointer to Xcom craft.
 */
void Ufo::setTargetedXcomCraft(Craft *craft)
{
	if (craft)
	{
		backupOriginalDestination();
		setDestination(craft);
		_isHunting = true; // must be after setDestination()

		// go hunt your target as quickly as you can
		if (_rules->getHuntSpeed() > 0)
		{
			setSpeed(_stats.speedMax * _rules->getHuntSpeed() / 100);
		}
	}
}

/**
 * Sets the UFO escorted by this UFO.
 * @param ufo Pointer to escorted UFO.
 */
void Ufo::setEscortedUfo(Ufo *ufo)
{
	if (ufo)
	{
		backupOriginalDestination();
		setDestination(ufo);
		_isEscorting = true; // must be after setDestination()

		// go protect your target as quickly as you can
		setSpeed(_stats.speedMax);
	}
}

/**
 * Backs up the original destination.
 */
void Ufo::backupOriginalDestination()
{
	// if we're not hunting or escorting yet, remember the original destination waypoint
	// if we're already occupied, keep the original information
	if (!_isHunting && !_isEscorting)
	{
		if (_origWaypoint)
		{
			delete _origWaypoint;
			_origWaypoint = 0;
		}
		_origWaypoint = new Waypoint();
		_origWaypoint->setLatitude(_dest->getLatitude());
		_origWaypoint->setLongitude(_dest->getLongitude());
	}
}

/**
 * Handle destination changes, making sure to delete old waypoint destinations.
 * @param dest Pointer to the new destination.
 */
void Ufo::setDestination(Target *dest)
{
	Waypoint *old = dynamic_cast<Waypoint*>(_dest);
	MovingTarget::setDestination(dest);
	if (old && !_isHunting && !_isEscorting)
	{
		// delete only waypoints, not xcom craft or other UFOs
		delete old;
	}
	_isHunting = false;   // reset flag, will be set in setTargetedXcomCraft()
	_isEscorting = false; // reset flag, will be set in setEscortedUfo()
}

/**
 * Gets which interception window the UFO is active in.
 * @return which interception window the UFO is active in.
 */
int Ufo::getShootingAt() const
{
	return _shootingAt;
}

/**
 * Sets which interception window the UFO is active in.
 * @param target the window the UFO is active in.
 */
void Ufo::setShootingAt(int target)
{
	_shootingAt = target;
}

/**
 * Gets the UFO's landing site ID.
 * @return landing site ID.
 */
int Ufo::getLandId() const
{
	return _landId;
}

/**
 * Sets the UFO's landing site ID.
 * @param id landing site ID.
 */
void Ufo::setLandId(int id)
{
	_landId = id;
}

/**
 * Gets the UFO's crash site ID.
 * @return the UFO's crash site ID.
 */
int Ufo::getCrashId() const
{
	return _crashId;
}

/**
 * Sets the UFO's crash site ID.
 * @param id the UFO's crash site ID.
 */
void Ufo::setCrashId(int id)
{
	_crashId = id;
}

/**
 * Sets the UFO's hit frame.
 * @param frame the hit frame.
 */
void Ufo::setHitFrame(int frame)
{
	_hitFrame = frame;
}

/**
 * Gets the UFO's hit frame.
 * @return the hit frame.
 */
int Ufo::getHitFrame() const
{
	return _hitFrame;
}

/// Gets the UFO's stats.
const RuleUfoStats& Ufo::getCraftStats() const
{
	return _stats;
}

/**
 * Sets the countdown timer for escaping a dogfight.
 * @param time how many ticks until the ship attempts to escape.
 */
void Ufo::setEscapeCountdown(int time)
{
	_escapeCountdown = time;
}

/**
 * Gets the escape timer for dogfights.
 * @return how many ticks until the ship tries to leave.
 */
int Ufo::getEscapeCountdown() const
{
	return _escapeCountdown;
}

/**
 * Sets the number of ticks until the ufo fires its weapon.
 * @param time number of ticks until refire.
 */
void Ufo::setFireCountdown(int time)
{
	_fireCountdown = time;
}

/**
 * Gets the number of ticks until the ufo is ready to fire.
 * @return ticks until weapon is ready.
 */
int Ufo::getFireCountdown() const
{
	return _fireCountdown;
}

/**
 * Sets a flag denoting that this ufo has had its timers decremented.
 * prevents multiple interceptions from decrementing or resetting an already running timer.
 * this flag is reset in advance each time the geoscape processes the dogfights.
 * @param processed whether or not we've had our timers processed.
 */
void Ufo::setInterceptionProcessed(bool processed)
{
	_processedIntercept = processed;
}

/**
 * Gets if the ufo has had its timers decremented on this cycle of interception updates.
 * @return if this ufo has already been processed.
 */
bool Ufo::getInterceptionProcessed() const
{
	return _processedIntercept;
}

/**
 * Gets the UFO's shield level
 * @return the points of shield remaining
 */
int Ufo::getShield() const
{
	return _shield;
}

/**
 * Sets the UFO's shield level
 * @param the shield point value to set
 */
void Ufo::setShield(int shield)
{
	_shield = std::max(0, std::min(_stats.shieldCapacity, shield));
}

/**
 * Sets which _interceptionNumber handles the UFO's shield recharge in a dogfight
 * @param the _interceptionNumber to set
 */
void Ufo::setShieldRechargeHandle(int shieldRechargeHandle)
{
	_shieldRechargeHandle = shieldRechargeHandle;
}

/**
 * Gets which _interceptionNumber handles the UFO's shield recharge in a dogfight
 * @return the _interceptionNumber to handle shield recharge
 */
int Ufo::getShieldRechargeHandle() const
{
	return _shieldRechargeHandle;
}

/**
 * Gets the percent shield remaining
 */
int Ufo::getShieldPercentage() const
{
	return _stats.shieldCapacity != 0 ? _shield * 100 / _stats.shieldCapacity : 0;
}

/**
 * Sets how much this UFO is being slowed down by craft tractor beams
 * @param the _tractorBeamSlowdown to set
 */
void Ufo::setTractorBeamSlowdown(int tractorBeamSlowdown)
{
	_tractorBeamSlowdown = std::max(0, std::min(_stats.speedMax, tractorBeamSlowdown));
}

/**
 * Gets how much this UFO is being slowed down by craft tractor beams
 * @return the tractor beam slowdown
 */
int Ufo::getTractorBeamSlowdown() const
{
	return _tractorBeamSlowdown;
}

/**
 * Is this UFO a hunter-killer?
 * @return True if UFO is a hunter-killer.
 */
bool Ufo::isHunterKiller() const
{
	return _isHunterKiller;
}

/**
 * Sets if the UFO is a hunter-killer.
 * @param isHunterKiller new value to set
 */
void Ufo::setHunterKiller(bool isHunterKiller)
{
	_isHunterKiller = isHunterKiller;
}

/**
* Is this UFO an escort?
* @return True if UFO is an escort.
*/
bool Ufo::isEscort() const
{
	return _isEscort;
}

/**
* Sets if the UFO is an escort.
* @param isEscort new value to set
*/
void Ufo::setEscort(bool isEscort)
{
	_isEscort = isEscort;
}

/**
 * Gets the UFO's hunting preferences.
 * @return Hunt mode ID.
 */
int Ufo::getHuntMode() const
{
	return _huntMode;
}

/**
 * Gets the UFO's hunting behavior.
 * @return Hunt behavior ID.
 */
int Ufo::getHuntBehavior() const
{
	return _huntBehavior;
}

/**
 * Is this UFO actively hunting right now?
 * @return True if UFO is actively hunting.
 */
bool Ufo::isHunting() const
{
	return _isHunting;
}

/**
 * Is this UFO escorting other UFO right now?
 * @return True if UFO is escorting.
 */
bool Ufo::isEscorting() const
{
	return _isEscorting;
}

/**
 * Returns if a certain target is inside the UFO's
 * radar range, taking in account the positions of both.
 * @param target Pointer to target to compare.
 * @return True if inside radar range.
 */
bool Ufo::insideRadarRange(Target *target) const
{
	if (_stats.radarRange == 0)
		return false;

	double range = Nautical(_stats.radarRange);
	return (getDistance(target) <= range);
}

////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

void getDamageMaxScript(const Ufo *u, int &ret)
{
	if (u)
	{
		ret = u->getCraftStats().damageMax;
		return;
	}
	ret = 0;
}

void getStatusScript(const Ufo *u, int &ret)
{
	if (u)
	{
		ret = (int)u->getStatus();
		return;
	}
	ret = 0;
}

std::string debugDisplayScript(const Ufo* u)
{
	if (u)
	{
		std::string s;
		s += Ufo::ScriptName;
		s += "(type: \"";
		s += u->getRules()->getType();
		s += "\" id: ";
		s += std::to_string(u->getId());
		s += "\" damage: ";
		s += std::to_string(u->getDamagePercentage());
		s += "%)";
		return s;
	}
	else
	{
		return "null";
	}
}

} // namespace


/**
 * Register Ufo in script parser.
 * @param parser Script parser.
 */
void Ufo::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<RuleUfo>();

	Bind<Ufo> u = { parser };

	u.add<&Ufo::getAltitudeInt>("getAltitude");
	u.add<&Ufo::getId>("getId");
	u.add<&Ufo::getRules>("getRules");
	u.add<&getStatusScript>("getStatus");
	u.add<&Ufo::getVisibility>("getVisibility");

	u.add<&Ufo::getDamage>("getDamage");
	u.add<&getDamageMaxScript>("getDamageMax");
	u.add<&Ufo::getDamagePercentage>("getDamagePercentage");

	u.add<&Ufo::getShield>("getShield");
	u.addField<&Ufo::_stats, &RuleUfoStats::getBase, &RuleCraftStats::shieldCapacity>("getShieldMax");
	u.add<&Ufo::getShieldPercentage>("getShieldPercentage");

	u.add<&Ufo::isHunterKiller>("isHunterKiller");
	u.add<&Ufo::isHunting>("isHunting");
	u.add<&Ufo::isEscorting>("isEscorting");
	u.add<&Ufo::getHuntBehavior>("getHuntBehavior");
	u.add<&Ufo::getHuntMode>("getHuntMode");

	u.add<&Ufo::getDetected>("getDetected");
	u.add<&Ufo::getHyperDetected>("getHyperDetected");

	u.addRules<RuleUfo, &Ufo::getRules>("getRuleUfo");

	RuleCraftStats::addGetStatsScript<&Ufo::_stats>(u, "Stats.");

	u.addScriptValue<BindBase::OnlyGet, &Ufo::_rules, &RuleUfo::getScriptValuesRaw>();
	u.addScriptValue<&Ufo::_scriptValues>();
	u.addDebugDisplay<&debugDisplayScript>();

	u.addCustomConst("UFO_FLYING", FLYING);
	u.addCustomConst("UFO_LANDED", LANDED);
	u.addCustomConst("UFO_CRASHED", CRASHED);
	u.addCustomConst("UFO_DESTROYED", DESTROYED);
	u.addCustomConst("UFO_IGNORE_ME", IGNORE_ME);

	u.addCustomConst("DETECTION_NONE", DETECTION_NONE);
	u.addCustomConst("DETECTION_RADAR", DETECTION_RADAR);
	u.addCustomConst("DETECTION_HYPERWAVE", DETECTION_HYPERWAVE);
}



ModScript::DetectUfoFromBaseParser::DetectUfoFromBaseParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"detection_type",
	"detection_chance",
	"ufo", "geoscape_game",
	"distance", "already_tracked", "radar_total_strength", "radar_max_distance", "hyperwave_total_strength", "hyperwave_max_distance", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::DetectUfoFromCraftParser::DetectUfoFromCraftParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"detection_type",
	"detection_chance",
	"ufo", "geoscape_game", "craft",
	"distance", "already_tracked", "radar_total_strength", "radar_max_distance", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

}
