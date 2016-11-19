/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#ifndef OPENXCOM_RULEUFO_H
#define OPENXCOM_RULEUFO_H

#include <string>
#include <map>
#include <yaml-cpp/yaml.h>
#include "RuleCraft.h"

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
	/// Loads stats form YAML.
	void load(const YAML::Node &node)
	{
		(*(RuleCraftStats*)this).load(node);
		craftCustomDeploy = node["craftCustomDeploy"].as<std::string>(craftCustomDeploy);
		missionCustomDeploy = node["missionCustomDeploy"].as<std::string>(missionCustomDeploy);
	}
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
	int _sprite, _marker;
	int _power, _range, _score, _reload, _breakOffTime;
	int _fireSound;
	RuleTerrain *_battlescapeTerrainData;
	RuleUfoStats _stats;
	std::map<std::string, RuleUfoStats> _statsRaceBonus;
	std::string _modSprite;
public:
	/// Creates a blank UFO ruleset.
	RuleUfo(const std::string &type);
	/// Cleans up the UFO ruleset.
	~RuleUfo();
	/// Loads UFO data from YAML.
	void load(const YAML::Node& node, Mod *mod);
	/// Gets the UFO's type.
	const std::string &getType() const;
	/// Gets the UFO's size.
	const std::string &getSize() const;
	/// Gets the UFO's radius.
	int getRadius() const;
	/// Gets the UFO's sprite.
	int getSprite() const;
	/// Gets the UFO's globe marker.
	int getMarker() const;
	/// Gets the UFO's shield capacity.
	int getShieldCapacity() const;
	/// Gets the UFO's shield recharge rate.
	int getShieldRecharge() const;
	/// Get how much damage bleeds through the UFO's shield when downed
	int getShieldBleedThrough() const;
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
	/// Gets the name of the surface that represents this UFO.
	const std::string &getModSprite() const;
	/// Get basic statistic of UFO.
	const RuleUfoStats& getStats() const;
	/// Get race bonus of statistic of UFO.
	const RuleUfoStats& getRaceBonus(const std::string& s) const;
	/// Gets the UFO's radar range.
	int getSightRange() const;
	/// Gets the UFO's firing sound.
	int getFireSound() const;
};

}

#endif
