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
#include "RuleCraft.h"
#include "RuleTerrain.h"
#include "../Engine/Exception.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain
 * type of craft.
 * @param type String defining the type.
 */
RuleCraft::RuleCraft(const std::string &type) :
    _type(type), _sprite(-1), _marker(-1), _weapons(0), _soldiers(0), _pilots(0), _vehicles(0),
    _costBuy(0), _costRent(0), _costSell(0), _repairRate(1), _refuelRate(1),
	_transferTime(0), _score(0), _battlescapeTerrainData(0),
	_spacecraft(false), _notifyWhenRefueled(false), _autoPatrol(false), _listOrder(0), _maxItems(0), _maxDepth(0), _stats(),
	_shieldRechargeAtBase(1000)
{
	for (int i = 0; i < WeaponMax; ++ i)
	{
		for (int j = 0; j < WeaponTypeMax; ++j)
			_weaponTypes[i][j] = 0;
	}
	_stats.radarRange = 672;
	_stats.radarChance = 100;
	_stats.sightRange = 1696;
	_weaponStrings[0] = "STR_WEAPON_ONE";
	_weaponStrings[1] = "STR_WEAPON_TWO";
}

/**
 *
 */
RuleCraft::~RuleCraft()
{
	delete _battlescapeTerrainData;
}

/**
 * Loads the craft from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the craft.
 * @param modIndex A value that offsets the sounds and sprite values to avoid conflicts.
 * @param listOrder The list weight for this craft.
 */
void RuleCraft::load(const YAML::Node &node, Mod *mod, int listOrder)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod, listOrder);
	}
	_type = node["type"].as<std::string>(_type);
	_requires = node["requires"].as< std::vector<std::string> >(_requires);
	if (node["sprite"])
	{
		_sprite = node["sprite"].as<int>(_sprite);
		// this is an offset in BASEBITS.PCK, and two in INTICONS.PCK
		if (_sprite > 4)
			_sprite += mod->getModOffset();
	}
	_stats.load(node);
	_marker = node["marker"].as<int>(_marker);
	_weapons = node["weapons"].as<int>(_weapons);
	_soldiers = node["soldiers"].as<int>(_soldiers);
	_pilots = node["pilots"].as<int>(_pilots);
	_vehicles = node["vehicles"].as<int>(_vehicles);
	_costBuy = node["costBuy"].as<int>(_costBuy);
	_costRent = node["costRent"].as<int>(_costRent);
	_costSell = node["costSell"].as<int>(_costSell);
	_refuelItem = node["refuelItem"].as<std::string>(_refuelItem);
	_repairRate = node["repairRate"].as<int>(_repairRate);
	_refuelRate = node["refuelRate"].as<int>(_refuelRate);
	_transferTime = node["transferTime"].as<int>(_transferTime);
	_score = node["score"].as<int>(_score);
	if (const YAML::Node &terrain = node["battlescapeTerrainData"])
	{
		RuleTerrain *rule = new RuleTerrain(terrain["name"].as<std::string>());
		rule->load(terrain, mod);
		_battlescapeTerrainData = rule;

		if (const YAML::Node &deployment = node["deployment"])
		{
			_deployment = deployment.as<std::vector<std::vector<int> > >(_deployment);
		}

		if (const YAML::Node &craftInventoryTile = node["craftInventoryTile"])
		{
			_craftInventoryTile = craftInventoryTile.as<std::vector<int> >(_craftInventoryTile);
		}
	}
	_spacecraft = node["spacecraft"].as<bool>(_spacecraft);
	_notifyWhenRefueled = node["notifyWhenRefueled"].as<bool>(_notifyWhenRefueled);
	_autoPatrol = node["autoPatrol"].as<bool>(_autoPatrol);
	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (!_listOrder)
	{
		_listOrder = listOrder;
	}
	_maxDepth = node["maxDepth"].as<int>(_maxDepth);
	_maxItems = node["maxItems"].as<int>(_maxItems);

	if (const YAML::Node &types = node["weaponTypes"])
	{
		for (int i = 0; (size_t)i < types.size() &&  i < WeaponMax; ++i)
		{
			const YAML::Node t = types[i];
			if (t.IsScalar())
			{
				for (int j = 0; j < WeaponTypeMax; ++j)
					_weaponTypes[i][j] = t.as<int>();
			}
			else if (t.IsSequence() && t.size() > 0)
			{
				for (int j = 0; (size_t)j < t.size() && j < WeaponTypeMax; ++j)
					_weaponTypes[i][j] = t[j].as<int>();
				for (int j = t.size(); j < WeaponTypeMax; ++j)
					_weaponTypes[i][j] = _weaponTypes[i][0];
			}
			else
			{
				throw Exception("Invalid weapon type in craft " + _type + ".");
			}
		}
	}
	if (const YAML::Node &str = node["weaponStrings"])
	{
		for (int i = 0; (size_t)i < str.size() &&  i < WeaponMax; ++i)
			_weaponStrings[i] = str[i].as<std::string>();
	}
	_shieldRechargeAtBase = node["shieldRechargedAtBase"].as<int>(_shieldRechargeAtBase);
}

/**
 * Gets the language string that names
 * this craft. Each craft type has a unique name.
 * @return The craft's name.
 */
const std::string &RuleCraft::getType() const
{
	return _type;
}

/**
 * Gets the list of research required to
 * acquire this craft.
 * @return The list of research IDs.
 */
const std::vector<std::string> &RuleCraft::getRequirements() const
{
	return _requires;
}

/**
 * Gets the ID of the sprite used to draw the craft
 * in the Basescape and Equip Craft screens.
 * @return The Sprite ID.
 */
int RuleCraft::getSprite() const
{
	return _sprite;
}

/**
 * Returns the globe marker for the craft type.
 * @return Marker sprite, -1 if none.
 */
int RuleCraft::getMarker() const
{
	return _marker;
}

/**
 * Gets the maximum fuel the craft can contain.
 * @return The fuel amount.
 */
int RuleCraft::getMaxFuel() const
{
	return _stats.fuelMax;
}

/**
 * Gets the maximum damage (damage the craft can take)
 * of the craft.
 * @return The maximum damage.
 */
int RuleCraft::getMaxDamage() const
{
	return _stats.damageMax;
}

/**
 * Gets the maximum speed of the craft flying
 * around the Geoscape.
 * @return The speed in knots.
 */
int RuleCraft::getMaxSpeed() const
{
	return _stats.speedMax;
}

/**
 * Gets the acceleration of the craft for
 * taking off / stopping.
 * @return The acceleration.
 */
int RuleCraft::getAcceleration() const
{
	return _stats.accel;
}

/**
 * Gets the maximum number of weapons that
 * can be equipped onto the craft.
 * @return The weapon capacity.
 */
int RuleCraft::getWeapons() const
{
	return _weapons;
}

/**
 * Gets the maximum number of soldiers that
 * the craft can carry.
 * @return The soldier capacity.
 */
int RuleCraft::getSoldiers() const
{
	return _soldiers;
}

/**
* Gets the number of pilots that the craft requires in order to take off.
* @return The number of pilots.
*/
int RuleCraft::getPilots() const
{
	return _pilots;
}

/**
 * Gets the maximum number of vehicles that
 * the craft can carry.
 * @return The vehicle capacity.
 */
int RuleCraft::getVehicles() const
{
	return _vehicles;
}

/**
 * Gets the cost of this craft for
 * purchase/rent (0 if not purchasable).
 * @return The cost.
 */
int RuleCraft::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the cost of rent for a month.
 * @return The cost.
 */
int RuleCraft::getRentCost() const
{
	return _costRent;
}

/**
 * Gets the sell value of this craft
 * Rented craft should use 0.
 * @return The sell value.
 */
int RuleCraft::getSellCost() const
{
	return _costSell;
}

/**
 * Gets what item is required while
 * the craft is refuelling.
 * @return The item ID or "" if none.
 */
const std::string &RuleCraft::getRefuelItem() const
{
	return _refuelItem;
}

/**
 * Gets how much damage is removed from the
 * craft while repairing.
 * @return The amount of damage.
 */
int RuleCraft::getRepairRate() const
{
	return _repairRate;
}

/**
 * Gets how much fuel is added to the
 * craft while refuelling.
 * @return The amount of fuel.
 */
int RuleCraft::getRefuelRate() const
{
	return _refuelRate;
}

/**
 * Gets the craft's radar range
 * for detecting UFOs.
 * @return The range in nautical miles.
 */
int RuleCraft::getRadarRange() const
{
	return _stats.radarRange;
}

/**
 * Gets the craft's radar chance
 * for detecting UFOs.
 * @return The chance in percentage.
 */
int RuleCraft::getRadarChance() const
{
	return _stats.radarChance;
}

/**
 * Gets the craft's sight range
 * for detecting bases.
 * @return The range in nautical miles.
 */
int RuleCraft::getSightRange() const
{
	return _stats.sightRange;
}

/**
 * Gets the amount of time this item
 * takes to arrive at a base.
 * @return The time in hours.
 */
int RuleCraft::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets the number of points you lose
 * when this craft is destroyed.
 * @return The score in points.
 */
int RuleCraft::getScore() const
{
	return _score;
}

/**
 * Gets the terrain data needed to draw the Craft in the battlescape.
 * @return The terrain data.
 */
RuleTerrain *RuleCraft::getBattlescapeTerrainData()
{
	return _battlescapeTerrainData;
}

/**
 * Checks if this ship is capable of going to mars.
 * @return True if this ship is capable of going to mars.
 */
bool RuleCraft::getSpacecraft() const
{
	return _spacecraft;
}

/**
 * Checks if a notification should be displayed when the craft is refueled.
 * @return True if notification should appear.
 */
bool RuleCraft::notifyWhenRefueled() const
{
	return _notifyWhenRefueled;
}

/**
* Checks if the craft supports auto patrol feature.
* @return True if auto patrol is supported.
*/
bool RuleCraft::canAutoPatrol() const
{
	return _autoPatrol;
}

/**
 * Gets the list weight for this research item.
 * @return The list weight.
 */
int RuleCraft::getListOrder() const
{
	return _listOrder;
}

/**
 * Gets the deployment layout for this craft.
 * @return The deployment layout.
 */
std::vector<std::vector<int> > &RuleCraft::getDeployment()
{
	return _deployment;
}

/**
* Gets the craft inventory tile position.
* @return The tile position.
*/
std::vector<int> &RuleCraft::getCraftInventoryTile()
{
	return _craftInventoryTile;
}

/**
 * Gets the maximum amount of items this craft can store.
 * @return number of items.
 */
int RuleCraft::getMaxItems() const
{
	return _maxItems;
}

/**
 * Test for possibility of usage of weapon type in weapon slot.
 * @param slot value less than WeaponMax.
 * @param weaponType weapon type of weapon that we try insert.
 * @return True if can use.
 */
bool RuleCraft::isValidWeaponSlot(int slot, int weaponType) const
{
	for (int j = 0; j < WeaponTypeMax; ++j)
	{
		if (_weaponTypes[slot][j] == weaponType)
			return true;
	}
	return false;
}

/**
 * Return string ID of weapon slot name for geoscape craft state.
 * @param slot value less than WeaponMax.
 * @return String ID for translation.
 */
const std::string &RuleCraft::getWeaponSlotString(int slot) const
{
	return _weaponStrings[slot];
}
/**
 * Gets basic statistic of craft.
 * @return Basic stats of craft.
 */
const RuleCraftStats& RuleCraft::getStats() const
{
	return _stats;
}

/**
 * Gets the maximum depth this craft can dive to.
 * @return max depth.
 */
int RuleCraft::getMaxDepth() const
{
	return _maxDepth;
}

/**
 * Gets the shield capacity of this craft
 * @return shield capacity.
 */
int RuleCraft::getShieldCapacity() const
{
	return _stats.shieldCapacity;
}

/**
 * Gets the shield recharge rate of this craft in combat
 * @return shield recharge rate.
 */
int RuleCraft::getShieldRecharge() const
{
	return _stats.shieldRecharge;
}

/**
 * Gets how much damage bleeds through the shield when it goes down
 * @return damage bleedthrough percentage.
 */
int RuleCraft::getShieldBleedThrough() const
{
	return _stats.shieldBleedThrough;
}

/**
 * Gets how many shield points are recharged when landed at base per hour
 * @return shield recharged per hour
 */
int RuleCraft::getShieldRechargeAtBase() const
{
	return _shieldRechargeAtBase;
}

}

