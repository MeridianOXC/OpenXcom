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
#include <map>
#include "../Engine/Yaml.h"
#include "RuleCraft.h"
#include "ModScript.h"

namespace OpenXcom
{

class RuleTerrain;
class Mod;

struct RuleUfoStats : RuleCraftStats
{
	std::string craftCustomDeploy, missionCustomDeploy;

	/// Add different stats.
	RuleUfoStats& operator+=(const RuleUfoStats& r)
	{
		*(RuleCraftStats*)this += r;
		if (!r.craftCustomDeploy.empty()) craftCustomDeploy = r.craftCustomDeploy;
		if (!r.missionCustomDeploy.empty()) missionCustomDeploy = r.missionCustomDeploy;
		return *this;
	}
	/// Loads stats from YAML.
	void load(const YAML::YamlNodeReader& reader)
	{
		(*(RuleCraftStats*)this).load(reader);
		reader.tryRead("craftCustomDeploy", craftCustomDeploy);
		reader.tryRead("missionCustomDeploy", missionCustomDeploy);
	}

	template<auto Stat, typename TBind>
	static void addGetStatsScript(TBind& b, std::string prefix)
	{
		RuleCraftStats::addGetStatsScript<Stat>(b, prefix);
//		b.template addField<Stat, &RuleUfoStats::?>(prefix + "?");
	}

	/// Helper function needed by scripts
	const RuleCraftStats& getBase() const { return *this; }
};

/**
 * Represents a specific type of UFO.
 * Contains constant info about a UFO like
 * speed, weapons, scores, etc.
 * @sa Ufo
 */
class RuleUfo
{
private:
	std::string _type, _size;
	int _radius, _visibility, _blobSize;
	int _sprite, _marker, _markerLand, _markerCrash;
	int _power, _range, _score, _reload, _breakOffTime, _missionScore;
	int _hunterKillerPercentage, _huntMode, _huntSpeed, _huntBehavior, _softlockThreshold;
	int _missilePower;
	bool _unmanned;
	bool _instaHyper;
	int _splashdownSurvivalChance;
	int _fakeWaterLandingChance;
	int _fireSound;
	int _alertSound;
	int _huntAlertSound;
	int _hitSound;
	RuleTerrain *_battlescapeTerrainData;
	RuleUfoStats _stats;
	std::map<std::string, RuleUfoStats> _statsRaceBonus;
	std::string _modSprite;
	std::string _hitImage;

	ModScript::UfoScripts::Container _ufoScripts;
	ScriptValues<RuleUfo> _scriptValues;
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleUfo";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Creates a blank UFO ruleset.
	RuleUfo(const std::string &type);
	/// Cleans up the UFO ruleset.
	~RuleUfo();
	/// Loads UFO data from YAML.
	void load(const YAML::YamlNodeReader& reader, Mod *mod, const ModScript &parsers);
	/// Gets the UFO's type.
	const std::string &getType() const;
	/// Gets the UFO's size.
	const std::string &getSize() const;
	/// Gets the UFO's radius.
	int getRadius() const;
	/// Gets the UFO's default visibility.
	int getDefaultVisibility() const;
	/// Gets the UFO's blob size.
	int getBlobSize() const;
	/// Gets the UFO's sprite.
	int getSprite() const;
	/// Gets the UFO's globe marker when flying.
	int getMarker() const;
	/// Gets the UFO's globe marker when landed.
	int getLandMarker() const;
	/// Gets the UFO's globe marker when crashed.
	int getCrashMarker() const;
	/// Gets the UFO's weapon power.
	int getWeaponPower() const;
	/// Gets the UFO's weapon range.
	int getWeaponRange() const;
	/// Gets the UFO's score.
	int getScore() const;
	/// Sets the battlescape terrain data ruleset for this UFO
	RuleTerrain *getBattlescapeTerrainData() const;
	/// Gets the reload time of the UFO's weapon.
	int getWeaponReload() const;
	/// Gets the UFO's escape time.
	int getBreakOffTime() const;
	/// Gets the UFO's fire sound.
	int getFireSound() const;
	/// Gets the alert sound for this UFO (UFO detected alert).
	int getAlertSound() const;
	/// Gets the alert sound for this UFO (UFO on intercept course alert).
	int getHuntAlertSound() const;
	/// Gets the hit sound for this UFO (to be used in the BaseDestroyedState UI).
	int getHitSound() const { return _hitSound; }
	/// Gets the name of the surface that represents this UFO.
	const std::string &getModSprite() const;
	/// Gets the name of the surface to be displayed in the BaseDestroyedState UI.
	const std::string& getHitImage() const { return _hitImage; }
	/// Get basic statistic of UFO.
	const RuleUfoStats& getStats() const;
	/// Get race bonus of statistic of UFO.
	const RuleUfoStats& getRaceBonus(const std::string& s) const;
	const std::map<std::string, RuleUfoStats> &getRaceBonusRaw() const;
	/// Gets the UFO's mission score.
	int getMissionScore() const;
	/// Gets the UFO's chance to become a hunter-killer.
	int getHunterKillerPercentage() const;
	/// Gets the UFO's hunting preferences.
	int getHuntMode() const;
	/// Gets the UFO's hunting speed (in percent of maximum speed).
	int getHuntSpeed() const;
	/// Gets the UFO's hunting behavior (normal, kamikaze or random).
	int getHuntBehavior() const;
	/// Gets the amount of shots fired before a softlock is detected.
	int getSoftlockThreshold() const { return _softlockThreshold; }
	/// Gets the missile power (of a UFO that represents one or more missiles).
	int getMissilePower() const { return _missilePower; }
	/// Is this an unmanned UFO (drone, missile, etc.)?
	bool isUnmanned() const { return _unmanned; }
	/// Show extra info during base defense even if the UFO was not yet hyper-detected classically?
	bool isInstaHyper() const { return _instaHyper; }
	/// Gets the chance of surviving a splashdown on fake water.
	int getSplashdownSurvivalChance() const { return _splashdownSurvivalChance; }
	/// Gets the chance for deciding to land on fake water texture.
	int getFakeWaterLandingChance() const { return _fakeWaterLandingChance; }
	/// Gets script.
	template<typename Script>
	const typename Script::Container &getScript() const { return _ufoScripts.get<Script>(); }
	/// Get all script values.
	const ScriptValues<RuleUfo> &getScriptValuesRaw() const { return _scriptValues; }
};

}
