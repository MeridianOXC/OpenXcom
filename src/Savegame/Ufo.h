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
#include <yaml-cpp/yaml.h>
#include "Craft.h"
#include "MovingTarget.h"
#include "../Mod/RuleUfo.h"

namespace OpenXcom
{

class AlienMission;
class UfoTrajectory;
class SavedGame;
class Mod;
class Waypoint;

enum UfoDetection : int
{
	DETECTION_NONE = 0x00,
	DETECTION_RADAR = 0x01,
	DETECTION_HYPERWAVE = 0x03,
};

/**
 * Represents an alien UFO on the map.
 * Contains variable info about a UFO like
 * position, damage, speed, etc.
 * @sa RuleUfo
 */
class Ufo : public MovingTarget
{
public:
	static const char *ALTITUDE_STRING[];
	enum UfoStatus { FLYING, LANDED, CRASHED, DESTROYED, IGNORE_ME };

	/// Name of class used in script.
	static constexpr const char *ScriptName = "Ufo";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

private:
	const RuleUfo *_rules;
	int _uniqueId;
	int _missionWaveNumber;
	int _crashId, _landId, _damage;
	std::string _direction, _altitude;
	enum UfoStatus _status;
	size_t _secondsRemaining;
	bool _inBattlescape;
	CraftId _shotDownByCraftId;
	AlienMission *_mission;
	const UfoTrajectory *_trajectory;
	size_t _trajectoryPoint;
	bool _detected, _hyperDetected, _processedIntercept;
	int _shootingAt, _hitFrame, _fireCountdown, _escapeCountdown;
	RuleUfoStats _stats;
	/// Calculates a new speed vector to the destination.
	void calculateSpeed() override;
	int _shield, _shieldRechargeHandle;
	int _tractorBeamSlowdown;
	bool _isHunterKiller, _isEscort;
	int _huntMode, _huntBehavior;
	bool _isHunting, _isEscorting;
	int _softlockShotCounter;
	Waypoint *_origWaypoint;
	ScriptValues<Ufo> _scriptValues;

	using MovingTarget::load;
	using MovingTarget::save;

	void backupOriginalDestination();
public:
	/// Creates a UFO of the specified type.
	Ufo(const RuleUfo *rules, int uniqueId, int hunterKillerPercentage = 0, int huntMode = 0, int huntBehavior = 0);
	/// Cleans up the UFO.
	~Ufo();
	/// Loads the UFO from YAML.
	void load(const YAML::Node& node, const ScriptGlobal *shared, const Mod &ruleset, SavedGame &game);
	/// Finishes loading the UFO from YAML (called after XCOM craft are loaded).
	void finishLoading(const YAML::Node& node, SavedGame &save);
	/// Saves the UFO to YAML.
	YAML::Node save(const ScriptGlobal *shared, bool newBattle) const;
	/// Saves the UFO's ID to YAML.
	YAML::Node saveId() const override;
	/// Gets the UFO's type.
	std::string getType() const override;
	/// Gets the UFO's ruleset.
	const RuleUfo *getRules() const;
	/// Sets the UFO's ruleset.
	void changeRules(const RuleUfo *rules);
	/// Gets the (unique) UFO's ID.
	int getUniqueId() const;
	/// Gets the mission wave number that created this UFO.
	int getMissionWaveNumber() const { return _missionWaveNumber; }
	/// Sets the mission wave number that created this UFO.
	void setMissionWaveNumber(int missionWaveNumber) { _missionWaveNumber = missionWaveNumber; }
	/// Gets the UFO's default name.
	std::string getDefaultName(Language *lang) const override;
	/// Gets the UFO's marker name.
	std::string getMarkerName() const override;
	/// Gets the UFO's marker ID.
	int getMarkerId() const override;
	/// Gets the UFO's marker sprite.
	int getMarker() const override;

	/// Gets the UFO's amount of damage.
	int getDamage() const;
	/// Sets the UFO's amount of damage.
	void setDamage(int damage, const Mod *mod);
	/// Gets the UFO's percentage of damage.
	int getDamagePercentage() const;

	/// Gets the UFO's detection status.
	bool getDetected() const;
	/// Sets the UFO's detection status.
	void setDetected(bool detected);
	/// Gets the UFO's seconds left on the ground.
	size_t getSecondsRemaining() const;
	/// Sets the UFO's seconds left on the ground.
	void setSecondsRemaining(size_t seconds);
	/// Gets the UFO's direction.
	std::string getDirection() const;
	/// Gets the UFO's altitude.
	std::string getAltitude() const;
	/// Gets the UFO's altitude.
	int getAltitudeInt() const;
	/// Sets the UFO's altitude.
	void setAltitude(const std::string &altitude);
	/// Gets the UFO status
	enum UfoStatus getStatus() const { return _status; }
	/// Set the UFO's status.
	void setStatus(enum UfoStatus status) {_status = status; }
	/// Gets if the UFO has crashed.
	bool isCrashed() const;
	/// Gets if the UFO has been destroyed.
	bool isDestroyed() const;
	/// Handles UFO logic.
	void think();
	/// Sets the UFO's battlescape status.
	void setInBattlescape(bool inbattle);
	/// Gets if the UFO is in battlescape.
	bool isInBattlescape() const;
	/// Gets the UFO's alien race.
	const std::string &getAlienRace() const;
	/// Sets the ID of craft which shot down the UFO.
	void setShotDownByCraftId(const CraftId& craftId);
	/// Gets the ID of craft which shot down the UFO.
	CraftId getShotDownByCraftId() const;
	/// Gets the UFO's visibility.
	int getVisibility() const;
	/// Gets the UFO's Mission type.
	const std::string &getMissionType() const;
	/// Sets the UFO's mission information.
	void setMissionInfo(AlienMission *mission, const UfoTrajectory *trajectory);
	/// Gets the UFO's hyper detection status.
	bool getHyperDetected() const;
	/// Sets the UFO's hyper detection status.
	void setHyperDetected(bool hyperdetected);
	/// Gets the UFO's progress on the trajectory track.
	size_t getTrajectoryPoint() const { return _trajectoryPoint; }
	/// Sets the UFO's progress on the trajectory track.
	void setTrajectoryPoint(size_t np) { _trajectoryPoint = np; }
	/// Gets the UFO's trajectory.
	const UfoTrajectory &getTrajectory() const { return *_trajectory; }
	/// Gets the UFO's mission object.
	AlienMission *getMission() const { return _mission; }
	/// Gets the Xcom craft targeted by this UFO.
	Craft *getTargetedXcomCraft() const;
	/// Resets the original destination if targeting the given craft.
	void resetOriginalDestination(Craft *target);
	void resetOriginalDestination(bool debugHelper);
	/// Sets the Xcom craft targeted by this UFO.
	void setTargetedXcomCraft(Craft *craft);
	/// Sets the UFO escorted by this UFO.
	void setEscortedUfo(Ufo *ufo);
	/// Sets the UFO's destination.
	void setDestination(Target *dest) override;
	/// Get which interceptor this ufo is engaging.
	int getShootingAt() const;
	/// Set which interceptor this ufo is engaging.
	void setShootingAt(int target);
	/// Gets the UFO's landing site ID.
	int getLandId() const;
	/// Sets the UFO's landing site ID.
	void setLandId(int id);
	/// Gets the UFO's crash site ID.
	int getCrashId() const;
	/// Sets the UFO's crash site ID.
	void setCrashId(int id);
	/// Sets the UFO's hit frame.
	void setHitFrame(int frame);
	/// Gets the UFO's hit frame.
	int getHitFrame() const;
	/// Gets the UFO's stats.
	const RuleUfoStats& getCraftStats() const;
	void setFireCountdown(int time);
	int getFireCountdown() const;
	void setEscapeCountdown(int time);
	int getEscapeCountdown() const;
	void setInterceptionProcessed(bool processed);
	bool getInterceptionProcessed() const;

	/// Sets the UFO's shield
	void setShield(int shield);
	/// Gets the UFO's shield value
	int getShield() const;
	/// Sets which _interceptionNumber in a dogfight handles the UFO shield recharge
	void setShieldRechargeHandle(int shieldRechargeHandle);
	/// Gets which _interceptionNumber in a dogfight handles the UFO shield recharge
	int getShieldRechargeHandle() const;
	/// Gets the percent shield remaining
	int getShieldPercentage() const;

	/// Sets the number of tractor beams locked on to a UFO
	void setTractorBeamSlowdown(int tractorBeamSlowdown);
	/// Gets the number of tractor beams locked on to a UFO
	int getTractorBeamSlowdown() const;
	/// Is this UFO a hunter-killer?
	bool isHunterKiller() const;
	void setHunterKiller(bool isHunterKiller);
	/// Is this UFO an escort?
	bool isEscort() const;
	void setEscort(bool isEscort);
	/// Gets the UFO's hunting preferences.
	int getHuntMode() const;
	/// Gets the UFO's hunting behavior.
	int getHuntBehavior() const;
	/// Is this UFO actively hunting right now?
	bool isHunting() const;
	/// Is this UFO escorting other UFO right now?
	bool isEscorting() const;
	/// Gets the value of the softlock shot counter;
	int getSoftlockShotCounter() const { return _softlockShotCounter; }
	/// Increases the value of the softlock shot counter;
	void increaseSoftlockShotCounter() { _softlockShotCounter++; }
	/// Checks if a target is inside the UFO's radar range.
	bool insideRadarRange(Target *target) const;
};

}
