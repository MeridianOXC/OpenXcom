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
#include <string>
#include <yaml-cpp/yaml.h>
#include "RuleBaseFacilityFunctions.h"
#include "ModScript.h"

namespace OpenXcom
{

typedef std::vector<std::vector<int> > RuleCraftDeployment;

class RuleTerrain;
class Mod;
class ModScript;
class ScriptParserBase;

/**
 * Battle statistic of craft type and bonus from craft weapons.
 */
struct RuleCraftStats
{
	int fuelMax, damageMax, speedMax, accel, radarRange, radarChance, sightRange, hitBonus, avoidBonus, powerBonus, armor, shieldCapacity, shieldRecharge, shieldRechargeInGeoscape, shieldBleedThrough, soldiers, vehicles;

	/// Default constructor.
	RuleCraftStats() :
		fuelMax(0), damageMax(0), speedMax(0), accel(0),
		radarRange(0), radarChance(0), sightRange(0),
		hitBonus(0), avoidBonus(0), powerBonus(0), armor(0),
		shieldCapacity(0), shieldRecharge(0), shieldRechargeInGeoscape(0), shieldBleedThrough(0),
		soldiers(0), vehicles(0)
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
		shieldRechargeInGeoscape += r.shieldRechargeInGeoscape;
		shieldBleedThrough += r.shieldBleedThrough;
		soldiers += r.soldiers;
		vehicles += r.vehicles;
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
		shieldRechargeInGeoscape -= r.shieldRechargeInGeoscape;
		shieldBleedThrough -= r.shieldBleedThrough;
		soldiers -= r.soldiers;
		vehicles -= r.vehicles;
		return *this;
	}
	/// Gets negative values of stats.
	RuleCraftStats operator-() const
	{
		RuleCraftStats s;
		s -= *this;
		return s;
	}
	/// Loads stats from YAML.
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
		shieldRechargeInGeoscape = node["shieldRechargeInGeoscape"].as<int>(shieldRechargeInGeoscape);
		shieldBleedThrough = node["shieldBleedThrough"].as<int>(shieldBleedThrough);
		soldiers = node["soldiers"].as<int>(soldiers);
		vehicles = node["vehicles"].as<int>(vehicles);
	}

	template<auto Stat, typename TBind>
	static void addGetStatsScript(TBind& b, std::string prefix)
	{
		b.template addField<Stat, &RuleCraftStats::fuelMax>(prefix + "getFuelMax");
		b.template addField<Stat, &RuleCraftStats::damageMax>(prefix + "getDamageMax");
		b.template addField<Stat, &RuleCraftStats::speedMax>(prefix + "getSpeedMax");
		b.template addField<Stat, &RuleCraftStats::accel>(prefix + "getAccel");
		b.template addField<Stat, &RuleCraftStats::radarRange>(prefix + "getRadarRange");
		b.template addField<Stat, &RuleCraftStats::radarChance>(prefix + "getRadarChance");
		b.template addField<Stat, &RuleCraftStats::sightRange>(prefix + "getSightRange");
		b.template addField<Stat, &RuleCraftStats::hitBonus>(prefix + "getHitBonus");
		b.template addField<Stat, &RuleCraftStats::avoidBonus>(prefix + "getAvoidBonus");
		b.template addField<Stat, &RuleCraftStats::powerBonus>(prefix + "getPowerBonus");
		b.template addField<Stat, &RuleCraftStats::armor>(prefix + "getArmor");
		b.template addField<Stat, &RuleCraftStats::shieldCapacity>(prefix + "getShieldCapacity");
		b.template addField<Stat, &RuleCraftStats::shieldRecharge>(prefix + "getShieldRecharge");
		b.template addField<Stat, &RuleCraftStats::shieldRechargeInGeoscape>(prefix + "getShieldRechargeInGeoscape");
		b.template addField<Stat, &RuleCraftStats::shieldBleedThrough>(prefix + "getShieldBleedThrough");
		b.template addField<Stat, &RuleCraftStats::soldiers>(prefix + "getMaxUnits");
		b.template addField<Stat, &RuleCraftStats::vehicles>(prefix + "getMaxVehiclesAndLargeSoldiers");
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
	/// Maximum of different types in one weapon slot.
	static const int WeaponTypeMax = 8;

	/// Default craft preview ID.
	static const std::string DEFAULT_CRAFT_DEPLOYMENT_PREVIEW;
	/// Dummy craft ID.
	static const int DUMMY_CRAFT_ID = -42; // a negative integer

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleCraft";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

private:
	std::string _type;
	std::vector<std::string> _requires;
	RuleBaseFacilityFunctions _requiresBuyBaseFunc;
	std::string _requiresBuyCountry;
	int _sprite, _marker;
	std::vector<int> _skinSprites;
	int _weapons, _pilots;
	int _maxSmallSoldiers, _maxLargeSoldiers, _maxSmallVehicles, _maxLargeVehicles;
	int _maxSmallUnits, _maxLargeUnits, _maxSoldiers, _maxVehicles, _maxUnitsLimit;
	int _monthlyBuyLimit;
	int _costBuy, _costRent, _costSell;
	char _weaponTypes[WeaponMax][WeaponTypeMax];
	std::string _refuelItem;
	std::string _weaponStrings[WeaponMax];
	std::string _fixedWeaponNames[WeaponMax];
	int _repairRate, _refuelRate, _transferTime, _score;
	RuleTerrain *_battlescapeTerrainData;
	int _maxSkinIndex;
	bool _keepCraftAfterFailedMission, _allowLanding, _spacecraft, _notifyWhenRefueled, _autoPatrol, _undetectable;
	int _listOrder, _maxItems, _maxAltitude;
	double _maxStorageSpace;
	RuleCraftDeployment _deployment;
	std::vector<int> _craftInventoryTile;
	RuleCraftStats _stats;
	int _shieldRechargeAtBase;
	bool _mapVisible, _forceShowInMonthlyCosts;
	bool _useAllStartTiles;
	std::string _customPreview;
	std::vector<int> _selectSound, _takeoffSound;

	ModScript::CraftScripts::Container _craftScripts;
	ScriptValues<RuleCraft> _scriptValues;

	/// Gets a random sound from a given vector.
	int getRandomSound(const std::vector<int>& vector, int defaultValue = -1) const;

public:
	/// Creates a blank craft ruleset.
	RuleCraft(const std::string &type, int listOrder);
	/// Cleans up the craft ruleset.
	~RuleCraft();
	/// Loads craft data from YAML.
	void load(const YAML::Node& node, Mod *mod, const ModScript &parsers);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);
	/// Gets the craft's type.
	const std::string &getType() const;
	/// Gets the craft's requirements.
	const std::vector<std::string> &getRequirements() const;
	/// Gets the base functions required to buy craft.
	RuleBaseFacilityFunctions getRequiresBuyBaseFunc() const { return _requiresBuyBaseFunc; }
	/// Gets the allied country name required to buy this craft.
	const std::string& getRequiresBuyCountry() const { return _requiresBuyCountry; }
	/// Gets the craft's sprite.
	int getSprite(int skinIndex) const;
	const std::vector<int> &getSkinSpritesRaw() const { return _skinSprites; }
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
	/// Gets the craft's maximum unit capacity (soldiers and vehicles, small and large).
	int getMaxUnits() const;
	/// Gets the craft's pilot capacity/requirement.
	int getPilots() const;
	/// Gets the craft's maximum vehicle capacity (incl. 2x2 soldiers).
	int getMaxVehiclesAndLargeSoldiers() const;
	/// Gets the craft's maximum supported number of small (size=1) soldiers.
	int getMaxSmallSoldiers() const { return _maxSmallSoldiers; }
	/// Gets the craft's maximum supported number of large (size=2) soldiers.
	int getMaxLargeSoldiers() const { return _maxLargeSoldiers; }
	/// Gets the craft's maximum supported number of small (size=1) vehicles (HWPs/SWSs).
	int getMaxSmallVehicles() const { return _maxSmallVehicles; }
	/// Gets the craft's maximum supported number of large (size=2) vehicles (HWPs/SWSs).
	int getMaxLargeVehicles() const { return _maxLargeVehicles; }
	/// Gets the craft's maximum supported number of small (size=1) units (soldiers + vehicles).
	int getMaxSmallUnits() const { return _maxSmallUnits; }
	/// Gets the craft's maximum supported number of large (size=2) units (soldiers + vehicles).
	int getMaxLargeUnits() const { return _maxLargeUnits; }
	/// Gets the craft's maximum supported number of soldiers (small + large).
	int getMaxSoldiers() const { return _maxSoldiers; }
	/// Gets the craft's maximum supported number of vehicles (small + large).
	int getMaxVehicles() const { return _maxVehicles; }
	/// Gets the craft's unit capacity limit (soldiers and vehicles, small and large).
	int getMaxUnitsLimit() const { return _maxUnitsLimit; }
	/// Gets the craft's monthly buy limit.
	int getMonthlyBuyLimit() const { return _monthlyBuyLimit; }
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
	RuleTerrain *getBattlescapeTerrainData() const;
	/// Gets the craft's maximum skin index.
	int getMaxSkinIndex() const { return _maxSkinIndex; }
	/// Checks if this craft is lost after a failed mission or not.
	bool keepCraftAfterFailedMission() const;
	/// Checks if this craft is capable of landing (on missions).
	bool getAllowLanding() const;
	/// Checks if this craft is capable of travelling to mars.
	bool getSpacecraft() const;
	/// Should notification be displayed when the craft is refueled?
	bool notifyWhenRefueled() const;
	/// Does this craft support auto-patrol?
	bool canAutoPatrol() const;
	/// Is this craft immune to detection by HKs and alien bases?
	bool isUndetectable() const { return _undetectable; }
	/// Gets the list weight for this craft.
	int getListOrder() const;
	/// Gets the deployment priority for the craft.
	const RuleCraftDeployment &getDeployment() const;
	/// Gets the craft inventory tile position.
	const std::vector<int> &getCraftInventoryTile() const;
	/// Gets the item limit for this craft.
	int getMaxItems() const;
	/// Gets the item storage space limit for this craft.
	double getMaxStorageSpace() const;
	/// Test for possibility of usage of weapon type in weapon slot.
	bool isValidWeaponSlot(int slot, int weaponType) const;
	int getWeaponTypesRaw(int slot, int subslot) const;
	/// Get description string of weapon slot.
	const std::string &getWeaponSlotString(int slot) const;
	/// Gets the string ID of a fixed weapon in a given slot.
	const std::string &getFixedWeaponInSlot(int slot) const;
	/// Get basic statistic of craft.
	const RuleCraftStats& getStats() const;
	/// Gets how high this craft can go.
	int getMaxAltitude() const;
	/// Gets if this craft only fights on water.
	bool isWaterOnly() const;
	/// Get how many shield points are recharged per hour at base
	int getShieldRechargeAtBase() const;
	/// Get whether the craft's map should be visible at the start of a battle
	bool isMapVisible() const;
	/// Gets whether or not the craft type should be displayed in Monthly Costs even if not present in the base.
	bool forceShowInMonthlyCosts() const;
	/// Can the player utilize all start tiles on a craft or only the ones specified in the '_deployment' list?
	bool useAllStartTiles() const;
	/// Gets the craft's custom preview type.
	const std::string& getCustomPreviewType() const;
	const std::string& getCustomPreviewTypeRaw() const;

	/// Calculate the theoretical range of the craft in nautical miles
	int calculateRange(int type);

	/// Gets the sound played when the player directly selects a craft on the globe.
	int getSelectSound() const;
	const std::vector<int>& getSelectSoundRaw() const { return _selectSound; }
	/// Gets the sound played when a craft takes off from a base.
	int getTakeoffSound() const;
	const std::vector<int>& getTakeoffSoundRaw() const { return _takeoffSound; }

	/// Gets script.
	template<typename Script>
	const typename Script::Container &getScript() const { return _craftScripts.get<Script>(); }
	/// Get all script values.
	const ScriptValues<RuleCraft>& getScriptValuesRaw() const { return _scriptValues; }
};

}
