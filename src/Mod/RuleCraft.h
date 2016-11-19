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
#ifndef OPENXCOM_RULECRAFT_H
#define OPENXCOM_RULECRAFT_H

#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

class RuleTerrain;
class Mod;

/**
 * Battle statistic of craft type and bonus form craft weapons.
 */
struct RuleCraftStats
{
	int fuelMax, damageMax, speedMax, accel, radarRange, radarChance, sightRange, hitBonus, avoidBonus, powerBonus, armor, shieldCapacity, shieldRecharge, shieldBleedThrough;

	/// Default constructor.
	RuleCraftStats() :
		fuelMax(0), damageMax(0), speedMax(0), accel(0),
		radarRange(0), radarChance(0), sightRange(0),
		hitBonus(0), avoidBonus(0), powerBonus(0), armor(0),
		shieldCapacity(0), shieldRecharge(0), shieldBleedThrough(0)
	{

	}
	/// Add different stats.
	RuleCraftStats& operator+=(const RuleCraftStats& r)
	{
		fuelMax += r.fuelMax;
		damageMax += r.damageMax;
		speedMax += r.speedMax;
		accel += r.accel;
		radarRange += r.radarRange;
		radarChance += r.radarChance;
		sightRange += r.sightRange;
		hitBonus += r.hitBonus;
		avoidBonus += r.avoidBonus;
		powerBonus += r.powerBonus;
		armor += r.armor;
		shieldCapacity += r.shieldCapacity;
		shieldRecharge += r.shieldRecharge;
		shieldBleedThrough += r.shieldBleedThrough;
		return *this;
	}
	/// Subtract different stats.
	RuleCraftStats& operator-=(const RuleCraftStats& r)
	{
		fuelMax -= r.fuelMax;
		damageMax -= r.damageMax;
		speedMax -= r.speedMax;
		accel -= r.accel;
		radarRange -= r.radarRange;
		radarChance -= r.radarChance;
		sightRange -= r.sightRange;
		hitBonus -= r.hitBonus;
		avoidBonus -= r.avoidBonus;
		powerBonus -= r.powerBonus;
		armor -= r.armor;
		shieldCapacity -= r.shieldCapacity;
		shieldRecharge -= r.shieldRecharge;
		shieldBleedThrough -= r.shieldBleedThrough;
		return *this;
	}
	/// Gets negative values of stats.
	RuleCraftStats operator-() const
	{
		RuleCraftStats s;
		s -= *this;
		return s;
	}
	/// Loads stats form YAML.
	void load(const YAML::Node &node)
	{
		fuelMax = node["fuelMax"].as<int>(fuelMax);
		damageMax = node["damageMax"].as<int>(damageMax);
		speedMax = node["speedMax"].as<int>(speedMax);
		accel = node["accel"].as<int>(accel);
		radarRange = node["radarRange"].as<int>(radarRange);
		radarChance = node["radarChance"].as<int>(radarChance);
		sightRange = node["sightRange"].as<int>(sightRange);
		hitBonus = node["hitBonus"].as<int>(hitBonus);
		avoidBonus = node["avoidBonus"].as<int>(avoidBonus);
		powerBonus = node["powerBonus"].as<int>(powerBonus);
		armor = node["armor"].as<int>(armor);
		shieldCapacity = node["shieldCapacity"].as<int>(shieldCapacity);
		shieldRecharge = node["shieldRecharge"].as<int>(shieldRecharge);
		shieldBleedThrough = node["shieldBleedThrough"].as<int>(shieldBleedThrough);
	}
};

/**
 * Represents a specific type of craft.
 * Contains constant info about a craft like
 * costs, speed, capacities, consumptions, etc.
 * @sa Craft
 */
class RuleCraft
{
public:
	/// Maximum number of weapon slots on craft.
	static const int WeaponMax = 4;
	/// Maximum of diffrernt types in one weapon slot.
	static const int WeaponTypeMax = 4;

private:
	std::string _type;
	std::vector<std::string> _requires;
	int _sprite, _marker;
	int _weapons, _soldiers, _pilots, _vehicles, _costBuy, _costRent, _costSell;
	char _weaponTypes[WeaponMax][WeaponTypeMax];
	std::string _refuelItem;
	std::string _weaponStrings[WeaponMax];
	int _repairRate, _refuelRate, _transferTime, _score;
	RuleTerrain *_battlescapeTerrainData;
	bool _spacecraft, _notifyWhenRefueled, _autoPatrol;
	int _listOrder, _maxItems, _maxDepth;
	std::vector<std::vector <int> > _deployment;
	std::vector<int> _craftInventoryTile;
	RuleCraftStats _stats;
	int _shield, _shieldRecharge, _shieldBleedThrough, _shieldRechargeAtBase;
public:
	/// Creates a blank craft ruleset.
	RuleCraft(const std::string &type);
	/// Cleans up the craft ruleset.
	~RuleCraft();
	/// Loads craft data from YAML.
	void load(const YAML::Node& node, Mod *mod, int listOrder);
	/// Gets the craft's type.
	const std::string &getType() const;
	/// Gets the craft's requirements.
	const std::vector<std::string> &getRequirements() const;
	/// Gets the craft's sprite.
	int getSprite() const;
	/// Gets the craft's globe marker.
	int getMarker() const;
	/// Gets the craft's maximum fuel.
	int getMaxFuel() const;
	/// Gets the craft's maximum damage.
	int getMaxDamage() const;
	/// Gets the craft's maximum speed.
	int getMaxSpeed() const;
	/// Gets the craft's acceleration.
	int getAcceleration() const;
	/// Gets the craft's weapon capacity.
	int getWeapons() const;
	/// Gets the craft's soldier capacity.
	int getSoldiers() const;
	/// Gets the craft's pilot capacity/requirement.
	int getPilots() const;
	/// Gets the craft's vehicle capacity.
	int getVehicles() const;
	/// Gets the craft's cost.
	int getBuyCost() const;
	/// Gets the craft's rent for a month.
	int getRentCost() const;
	/// Gets the craft's value.
	int getSellCost() const;
	/// Gets the craft's refuel item.
	const std::string &getRefuelItem() const;
	/// Gets the craft's repair rate.
	int getRepairRate() const;
	/// Gets the craft's refuel rate.
	int getRefuelRate() const;
	/// Gets the craft's radar range.
	int getRadarRange() const;
	/// Gets the craft's radar chance.
	int getRadarChance() const;
	/// Gets the craft's sight range.
	int getSightRange() const;
	/// Gets the craft's transfer time.
	int getTransferTime() const;
	/// Gets the craft's score.
	int getScore() const;
	/// Gets the craft's terrain data.
	RuleTerrain *getBattlescapeTerrainData();
	/// Checks if this craft is capable of travelling to mars.
	bool getSpacecraft() const;
	/// Should notification be displayed when the craft is refueled?
	bool notifyWhenRefueled() const;
	/// Does this craft support auto patrol?
	bool canAutoPatrol() const;
	/// Gets the list weight for this craft.
	int getListOrder() const;
	/// Gets the deployment priority for the craft.
	std::vector<std::vector<int> > &getDeployment();
	/// Gets the craft inventory tile position.
	std::vector<int> &getCraftInventoryTile();
	/// Gets the item limit for this craft.
	int getMaxItems() const;
	/// Test for possibility of usage of weapon type in weapon slot.
	bool isValidWeaponSlot(int slot, int weaponType) const;
	/// Get description string of weapon slot.
	const std::string &getWeaponSlotString(int slot) const;
	/// Get basic statistic of craft.
	const RuleCraftStats& getStats() const;
	/// checks how deep this craft can go.
	int getMaxDepth() const;
	/// Get the shield capacity for the craft
	int getShieldCapacity() const;
	/// Get the shield recharge rate for the craft
	int getShieldRecharge() const;
	/// Get how much damage bleeds through the shield when it goes down
	int getShieldBleedThrough() const;
	/// Get how many shield points are recharged per hour at base
	int getShieldRechargeAtBase() const;
};

}

#endif
