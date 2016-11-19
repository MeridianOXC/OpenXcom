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
#define _USE_MATH_DEFINES
#include "Craft.h"
#include <cmath>
#include "../Engine/Language.h"
#include "../Engine/RNG.h"
#include "../Mod/RuleCraft.h"
#include "CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/Mod.h"
#include "SavedGame.h"
#include "ItemContainer.h"
#include "Soldier.h"
#include "../Mod/RuleSoldier.h"
#include "Base.h"
#include "Ufo.h"
#include "Waypoint.h"
#include "MissionSite.h"
#include "AlienBase.h"
#include "Vehicle.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleItem.h"
#include "../Mod/AlienDeployment.h"
#include "SerializationHelper.h"

namespace OpenXcom
{

/**
 * Initializes a craft of the specified type and
 * assigns it the latest craft ID available.
 * @param rules Pointer to ruleset.
 * @param base Pointer to base of origin.
 * @param id ID to assign to the craft (0 to not assign).
 */
Craft::Craft(RuleCraft *rules, Base *base, int id) : MovingTarget(),
	_rules(rules), _base(base), _id(0), _fuel(0), _damage(0), _shield(0),
	_interceptionOrder(0), _takeoff(0), _weapons(),
	_status("STR_READY"), _lowFuel(false), _mission(false),
	_inBattlescape(false), _inDogfight(false), _stats(),
	_isAutoPatrolling(false), _lonAuto(0.0), _latAuto(0.0)
{
	_stats = rules->getStats();
	_items = new ItemContainer();
	if (id != 0)
	{
		_id = id;
	}
	for (int i = 0; i < _rules->getWeapons(); ++i)
	{
		_weapons.push_back(0);
	}
	if (base != 0)
	{
		setBase(base);
	}
}

/**
 * Delete the contents of the craft from memory.
 */
Craft::~Craft()
{
	for (std::vector<CraftWeapon*>::iterator i = _weapons.begin(); i != _weapons.end(); ++i)
	{
		delete *i;
	}
	delete _items;
	for (std::vector<Vehicle*>::iterator i = _vehicles.begin(); i != _vehicles.end(); ++i)
	{
		delete *i;
	}
}

/**
 * Loads the craft from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the saved game.
 * @param save Pointer to the saved game.
 */
void Craft::load(const YAML::Node &node, const Mod *mod, SavedGame *save)
{
	MovingTarget::load(node);
	_id = node["id"].as<int>(_id);
	_fuel = node["fuel"].as<int>(_fuel);
	_damage = node["damage"].as<int>(_damage);
	_shield = node["shield"].as<int>(_shield);

	int j = 0;
	for (YAML::const_iterator i = node["weapons"].begin(); i != node["weapons"].end(); ++i)
	{
		if (_rules->getWeapons() > j)
		{
			std::string type = (*i)["type"].as<std::string>();
			RuleCraftWeapon* weapon = mod->getCraftWeapon(type);
			if (type != "0" && weapon)
			{
				CraftWeapon *w = new CraftWeapon(weapon, 0);
				w->load(*i);
				_weapons[j] = w;
				_stats += weapon->getBonusStats();
			}
			else
			{
				_weapons[j] = 0;
			}
			j++;
		}
	}

	_items->load(node["items"]);
	for (std::map<std::string, int>::iterator i = _items->getContents()->begin(); i != _items->getContents()->end();)
	{
		if (std::find(mod->getItemsList().begin(), mod->getItemsList().end(), i->first) == mod->getItemsList().end())
		{
			_items->getContents()->erase(i++);
		}
		else
		{
			++i;
		}
	}
	for (YAML::const_iterator i = node["vehicles"].begin(); i != node["vehicles"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (mod->getItem(type))
		{
			Vehicle *v = new Vehicle(mod->getItem(type), 0, 4);
			v->load(*i);
			_vehicles.push_back(v);
		}
	}
	_status = node["status"].as<std::string>(_status);
	_lowFuel = node["lowFuel"].as<bool>(_lowFuel);
	_mission = node["mission"].as<bool>(_mission);
	_interceptionOrder = node["interceptionOrder"].as<int>(_interceptionOrder);
	if (const YAML::Node name = node["name"])
	{
		_name = Language::utf8ToWstr(name.as<std::string>());
	}
	if (const YAML::Node &dest = node["dest"])
	{
		std::string type = dest["type"].as<std::string>();
		int id = dest["id"].as<int>();
		if (type == "STR_BASE")
		{
			returnToBase();
		}
		else if (type == "STR_UFO")
		{
			for (std::vector<Ufo*>::iterator i = save->getUfos()->begin(); i != save->getUfos()->end(); ++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else if (type == "STR_WAYPOINT")
		{
			for (std::vector<Waypoint*>::iterator i = save->getWaypoints()->begin(); i != save->getWaypoints()->end(); ++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else if (type == "STR_ALIEN_BASE")
		{
			for (std::vector<AlienBase*>::iterator i = save->getAlienBases()->begin(); i != save->getAlienBases()->end(); ++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else
		{
			// Backwards compatibility
			if (type == "STR_ALIEN_TERROR")
				type = "STR_TERROR_SITE";
			for (std::vector<MissionSite*>::iterator i = save->getMissionSites()->begin(); i != save->getMissionSites()->end(); ++i)
			{
				if ((*i)->getId() == id && (*i)->getDeployment()->getMarkerName() == type)
				{
					setDestination(*i);
					break;
				}
			}
		}
	}
	_takeoff = node["takeoff"].as<int>(_takeoff);
	_inBattlescape = node["inBattlescape"].as<bool>(_inBattlescape);
	_isAutoPatrolling = node["isAutoPatrolling"].as<bool>(_isAutoPatrolling);
	_lonAuto = node["lonAuto"].as<double>(_lonAuto);
	_latAuto = node["latAuto"].as<double>(_latAuto);
	_pilots = node["pilots"].as< std::vector<int> >(_pilots);
	if (_inBattlescape)
		setSpeed(0);
}

/**
 * Saves the craft to a YAML file.
 * @return YAML node.
 */
YAML::Node Craft::save() const
{
	YAML::Node node = MovingTarget::save();
	node["type"] = _rules->getType();
	node["id"] = _id;
	node["fuel"] = _fuel;
	node["damage"] = _damage;
	node["shield"] = _shield;
	for (std::vector<CraftWeapon*>::const_iterator i = _weapons.begin(); i != _weapons.end(); ++i)
	{
		YAML::Node subnode;
		if (*i != 0)
		{
			subnode = (*i)->save();
		}
		else
		{
			subnode["type"] = "0";
		}
		node["weapons"].push_back(subnode);
	}
	node["items"] = _items->save();
	for (std::vector<Vehicle*>::const_iterator i = _vehicles.begin(); i != _vehicles.end(); ++i)
	{
		node["vehicles"].push_back((*i)->save());
	}
	node["status"] = _status;
	if (_lowFuel)
		node["lowFuel"] = _lowFuel;
	if (_mission)
		node["mission"] = _mission;
	if (_inBattlescape)
		node["inBattlescape"] = _inBattlescape;
	if (_interceptionOrder != 0)
		node["interceptionOrder"] = _interceptionOrder;
	if (_takeoff != 0)
		node["takeoff"] = _takeoff;
	if (!_name.empty())
		node["name"] = Language::wstrToUtf8(_name);
	if (_isAutoPatrolling)
		node["isAutoPatrolling"] = _isAutoPatrolling;
	node["lonAuto"] = serializeDouble(_lonAuto);
	node["latAuto"] = serializeDouble(_latAuto);
	for (std::vector<int>::const_iterator i = _pilots.begin(); i != _pilots.end(); ++i)
	{
		node["pilots"].push_back((*i));
	}
	return node;
}

/**
 * Loads a craft unique identifier from a YAML file.
 * @param node YAML node.
 * @return Unique craft id.
 */
CraftId Craft::loadId(const YAML::Node &node)
{
	return std::make_pair(node["type"].as<std::string>(), node["id"].as<int>());
}

/**
 * Saves the craft's unique identifiers to a YAML file.
 * @return YAML node.
 */
YAML::Node Craft::saveId() const
{
	YAML::Node node = MovingTarget::saveId();
	CraftId uniqueId = getUniqueId();
	node["type"] = uniqueId.first;
	node["id"] = uniqueId.second;
	return node;
}

/**
 * Returns the ruleset for the craft's type.
 * @return Pointer to ruleset.
 */
RuleCraft *Craft::getRules() const
{
	return _rules;
}

/**
 * Changes the ruleset for the craft's type.
 * @param rules Pointer to ruleset.
 * @warning ONLY FOR NEW BATTLE USE!
 */
void Craft::changeRules(RuleCraft *rules)
{
	_rules = rules;
	_weapons.clear();
	for (int i = 0; i < _rules->getWeapons(); ++i)
	{
		_weapons.push_back(0);
	}
}

/**
 * Returns the craft's unique ID. Each craft
 * can be identified by its type and ID.
 * @return Unique ID.
 */
int Craft::getId() const
{
	return _id;
}

/**
 * Returns the craft's unique identifying name.
 * If there's no custom name, the language default is used.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::wstring Craft::getName(Language *lang) const
{
	if (_name.empty())
		return lang->getString("STR_CRAFTNAME").arg(lang->getString(_rules->getType())).arg(_id);
	return _name;
}

/**
 * Changes the craft's custom name.
 * @param newName New custom name. If set to blank, the language default is used.
 */
void Craft::setName(const std::wstring &newName)
{
	_name = newName;
}

/**
 * Returns the globe marker for the craft.
 * @return Marker sprite, -1 if none.
 */
int Craft::getMarker() const
{
	if (_status != "STR_OUT")
		return -1;
	else if (_rules->getMarker() == -1)
		return 1;
	return _rules->getMarker();
}

/**
 * Returns the base the craft belongs to.
 * @return Pointer to base.
 */
Base *Craft::getBase() const
{
	return _base;
}

/**
 * Changes the base the craft belongs to.
 * @param base Pointer to base.
 * @param move Move the craft to the base coordinates.
 */
void Craft::setBase(Base *base, bool move)
{
	_base = base;
	if (move)
	{
		_lon = base->getLongitude();
		_lat = base->getLatitude();
	}
}

/**
 * Returns the current status of the craft.
 * @return Status string.
 */
std::string Craft::getStatus() const
{
	return _status;
}

/**
 * Changes the current status of the craft.
 * @param status Status string.
 */
void Craft::setStatus(const std::string &status)
{
	_status = status;
}

/**
 * Returns the current altitude of the craft.
 * @return Altitude.
 */
std::string Craft::getAltitude() const
{
	Ufo *u = dynamic_cast<Ufo*>(_dest);
	if (u && u->getAltitude() != "STR_GROUND")
	{
		return u->getAltitude();
	}
	else
	{
		return "STR_VERY_LOW";
	}
}


/**
 * Changes the destination the craft is heading to.
 * @param dest Pointer to new destination.
 */
void Craft::setDestination(Target *dest)
{
	if (_status != "STR_OUT")
	{
		_takeoff = 60;
	}
	if (dest == 0)
		setSpeed(_stats.speedMax/2);
	else
		setSpeed(_stats.speedMax);
	MovingTarget::setDestination(dest);
}

bool Craft::getIsAutoPatrolling() const
{
	return _isAutoPatrolling;
}

void Craft::setIsAutoPatrolling(bool isAuto)
{
	_isAutoPatrolling = isAuto;
}

double Craft::getLongitudeAuto() const
{
	return _lonAuto;
}

void Craft::setLongitudeAuto(double lon)
{
	_lonAuto = lon;
}

double Craft::getLatitudeAuto() const
{
	return _latAuto;
}

void Craft::setLatitudeAuto(double lat)
{
	_latAuto = lat;
}

/**
 * Returns the amount of weapons currently
 * equipped on this craft.
 * @return Number of weapons.
 */
int Craft::getNumWeapons() const
{
	if (_rules->getWeapons() == 0)
	{
		return 0;
	}

	int total = 0;

	for (std::vector<CraftWeapon*>::const_iterator i = _weapons.begin(); i != _weapons.end(); ++i)
	{
		if ((*i) != 0)
		{
			total++;
		}
	}

	return total;
}

/**
 * Returns the amount of soldiers from a list
 * that are currently attached to this craft.
 * @return Number of soldiers.
 */
int Craft::getNumSoldiers() const
{
	if (_rules->getSoldiers() == 0)
		return 0;

	int total = 0;

	for (Soldier *s : *_base->getSoldiers())
	{
		if (s->getCraft() == this && s->getArmor()->getSize() == 1)
			++total;
	}

	return total;
}

/**
 * Returns the amount of equipment currently
 * equipped on this craft.
 * @return Number of items.
 */
int Craft::getNumEquipment() const
{
	return _items->getTotalQuantity();
}

/**
 * Returns the amount of vehicles currently
 * contained in this craft.
 * @return Number of vehicles.
 */
int Craft::getNumVehicles() const
{
	int total = 0;

	for (Soldier *s : *_base->getSoldiers())
	{
		if (s->getCraft() == this && s->getArmor()->getSize() == 2)
			++total;
	}
	return _vehicles.size() + total;
}

/**
 * Returns the list of weapons currently equipped
 * in the craft.
 * @return Pointer to weapon list.
 */
std::vector<CraftWeapon*> *Craft::getWeapons()
{
	return &_weapons;
}

/**
 * Returns the list of items in the craft.
 * @return Pointer to the item list.
 */
ItemContainer *Craft::getItems()
{
	return _items;
}

/**
 * Returns the list of vehicles currently equipped
 * in the craft.
 * @return Pointer to vehicle list.
 */
std::vector<Vehicle*> *Craft::getVehicles()
{
	return &_vehicles;
}

/**
 * Update stats of craft.
 * @param s
 */
void Craft::addCraftStats(const RuleCraftStats& s)
{
	setDamage(_damage + s.damageMax); //you need "fix" new damage capability first before use.
	_stats += s;

	int overflowFuel = _fuel - _stats.fuelMax;
	if (overflowFuel > 0 && !_rules->getRefuelItem().empty())
	{
		_base->getStorageItems()->addItem(_rules->getRefuelItem(), overflowFuel / _rules->getRefuelRate());
	}
	setFuel(_fuel);
}

/**
 * Gets all basic stats of craft.
 * @return Stats of craft
 */
const RuleCraftStats& Craft::getCraftStats() const
{
	return _stats;
}

/**
 * Returns current max amount of fuel that craft can carry.
 * @return Max amount of fuel.
 */
int Craft::getFuelMax() const
{
	return _stats.fuelMax;
}

/**
 * Returns the amount of fuel currently contained
 * in this craft.
 * @return Amount of fuel.
 */
int Craft::getFuel() const
{
	return _fuel;
}

/**
 * Changes the amount of fuel currently contained
 * in this craft.
 * @param fuel Amount of fuel.
 */
void Craft::setFuel(int fuel)
{
	_fuel = fuel;
	if (_fuel > _stats.fuelMax)
	{
		_fuel = _stats.fuelMax;
	}
	else if (_fuel < 0)
	{
		_fuel = 0;
	}
}

/**
 * Returns the ratio between the amount of fuel currently
 * contained in this craft and the total it can carry.
 * @return Percentage of fuel.
 */
int Craft::getFuelPercentage() const
{
	return (int)floor((double)_fuel / _stats.fuelMax * 100.0);
}

/**
 * Return current max amount of damage this craft can take.
 * @return Max amount of damage.
 */
int Craft::getDamageMax() const
{
	return _stats.damageMax;
}

/**
 * Returns the amount of damage this craft has taken.
 * @return Amount of damage.
 */
int Craft::getDamage() const
{
	return _damage;
}

/**
 * Changes the amount of damage this craft has taken.
 * @param damage Amount of damage.
 */
void Craft::setDamage(int damage)
{
	_damage = damage;
	if (_damage < 0)
	{
		_damage = 0;
	}
}

/**
 * Returns the ratio between the amount of damage this
 * craft can take and the total it can take before it's
 * destroyed.
 * @return Percentage of damage.
 */
int Craft::getDamagePercentage() const
{
	return (int)floor((double)_damage / _stats.damageMax * 100);
}

/**
 * Gets the max shield capacity of this craft
 * @return max shield capacity.
 */
int Craft::getShieldCapacity() const
{
	return  _stats.shieldCapacity;
}

/**
 * Gets the amount of shield this craft has remaining
 * @return shield points remaining.
 */
int Craft::getShield() const
{
	return  _shield;
}

/**
 * Sets the amount of shield for this craft, capped at the capacity plus bonuses
 * @param shield value to set the shield.
 */
void Craft::setShield(int shield)
{
	_shield = std::max(0, std::min(_stats.shieldCapacity, shield));
}

/**
 * Returns whether the craft is currently low on fuel
 * (only has enough to head back to base).
 * @return True if it's low, false otherwise.
 */
bool Craft::getLowFuel() const
{
	return _lowFuel;
}

/**
 * Changes whether the craft is currently low on fuel
 * (only has enough to head back to base).
 * @param low True if it's low, false otherwise.
 */
void Craft::setLowFuel(bool low)
{
	_lowFuel = low;
}

/**
 * Returns whether the craft has just done a ground mission,
 * and is forced to return to base.
 * @return True if it's returning, false otherwise.
 */
bool Craft::getMissionComplete() const
{
	return _mission;
}

/**
 * Changes whether the craft has just done a ground mission,
 * and is forced to return to base.
 * @param mission True if it's returning, false otherwise.
 */
void Craft::setMissionComplete(bool mission)
{
	_mission = mission;
}

/**
 * Returns the current distance between the craft
 * and the base it belongs to.
 * @return Distance in radian.
 */
double Craft::getDistanceFromBase() const
{
	return getDistance(_base);
}

/**
 * Returns the amount of fuel the craft uses up
 * while it's on the air, based on its speed.
 * @return Fuel amount.
 */
int Craft::getFuelConsumption() const
{
	if (!_rules->getRefuelItem().empty())
		return 1;
	return (int)floor(_speed / 100.0);
}

/**
 * Returns the minimum required fuel for the
 * craft to make it back to base.
 * @return Fuel amount.
 */
int Craft::getFuelLimit() const
{
	return getFuelLimit(_base);
}

/**
 * Returns the minimum required fuel for the
 * craft to go to a base.
 * @param base Pointer to target base.
 * @return Fuel amount.
 */
int Craft::getFuelLimit(Base *base) const
{
	return (int)floor(getFuelConsumption() * getDistance(base) / (_speedRadian * 120));
}

/**
 * Sends the craft back to its origin base.
 */
void Craft::returnToBase()
{
	setDestination(_base);
}

/**
 * Moves the craft to its destination.
 */
void Craft::think()
{
	if (_takeoff == 0)
	{
		move();
	}
	else
	{
		_takeoff--;
	}
	if (reachedDestination() && _dest == (Target*)_base)
	{
		setInterceptionOrder(0);
		checkup();
		setDestination(0);
		setSpeed(0);
		_lowFuel = false;
		_mission = false;
		_takeoff = 0;
	}
}

/**
 * Checks the condition of all the craft's systems
 * to define its new status (eg. when arriving at base).
 */
void Craft::checkup()
{
	int available = 0, full = 0;
	for (std::vector<CraftWeapon*>::iterator i = _weapons.begin(); i != _weapons.end(); ++i)
	{
		if ((*i) == 0)
			continue;
		available++;
		if ((*i)->getAmmo() >= (*i)->getRules()->getAmmoMax())
		{
			full++;
		}
		else
		{
			(*i)->setRearming(true);
		}
	}

	if (_damage > 0)
	{
		_status = "STR_REPAIRS";
	}
	else if (available != full)
	{
		_status = "STR_REARMING";
	}
	else if (_fuel < _stats.fuelMax)
	{
		_status = "STR_REFUELLING";
	}
	else
	{
		_status = "STR_READY";
	}
}

/**
 * Returns if a certain target is detected by the craft's
 * radar, taking in account the range and chance.
 * @param target Pointer to target to compare.
 * @return True if it's detected, False otherwise.
 */
bool Craft::detect(Target *target) const
{
	if (_rules->getRadarRange() == 0 || !insideRadarRange(target))
		return false;

	// backward compatibility with vanilla
	if (_rules->getRadarChance() == 100)
		return true;

	Ufo *u = dynamic_cast<Ufo*>(target);
	int chance = _stats.radarChance * (100 + u->getVisibility()) / 100;
	return RNG::percent(chance);
}

/**
 * Returns if a certain target is inside the craft's
 * radar range, taking in account the positions of both.
 * @param target Pointer to target to compare.
 * @return True if inside radar range.
 */
bool Craft::insideRadarRange(Target *target) const
{
	double range = _stats.radarRange * (1 / 60.0) * (M_PI / 180);
	return (getDistance(target) <= range);
}

/**
 * Consumes the craft's fuel every 10 minutes
 * while it's on the air.
 */
void Craft::consumeFuel()
{
	setFuel(_fuel - getFuelConsumption());
}

/**
 * Returns how long in hours until the
 * craft is repaired.
 */
unsigned int Craft::calcRepairTime()
{
	unsigned int repairTime = 0;

	if (_damage > 0)
	{
		repairTime = (int)ceil((double)_damage / _rules->getRepairRate());
	}
	return repairTime;
}

/**
 * Returns how long in hours until the
 * craft is refuelled (assumes fuel is available).
 */
unsigned int Craft::calcRefuelTime()
{
	unsigned int refuelTime = 0;

	int needed = _rules->getMaxFuel() - _fuel;
	if (needed > 0)
	{
		refuelTime = (int)ceil((double)(needed) / _rules->getRefuelRate() / 2.0);
	}
	return refuelTime;
}

/**
 * Returns how long in hours until the
 * craft is re-armed (assumes ammo is available).
 */
unsigned int Craft::calcRearmTime()
{
	unsigned int rearmTime = 0;

	for (int idx = 0; idx < _rules->getWeapons(); idx++)
	{
		CraftWeapon *w1 = _weapons.at(idx);
		if (w1 != 0)
		{
			int needed = w1->getRules()->getAmmoMax() - w1->getAmmo();
			if (needed > 0)
			{
				rearmTime += (int)ceil((double)(needed) / w1->getRules()->getRearmRate());
			}
		}
	}

	return rearmTime;
}

/**
 * Repairs the craft's damage every hour
 * while it's docked in the base.
 */
void Craft::repair()
{
	setDamage(_damage - _rules->getRepairRate());
	if (_damage <= 0)
	{
		_status = "STR_REARMING";
	}
}

/**
 * Refuels the craft every 30 minutes
 * while it's docked in the base.
 */
void Craft::refuel()
{
	setFuel(_fuel + _rules->getRefuelRate());
	if (_fuel >= _stats.fuelMax)
	{
		_status = "STR_READY";
		for (std::vector<CraftWeapon*>::iterator i = _weapons.begin(); i != _weapons.end(); ++i)
		{
			if (*i && (*i)->isRearming())
			{
				_status = "STR_REARMING";
				break;
			}
		}
	}
}

/**
 * Rearms the craft's weapons by adding ammo every hour
 * while it's docked in the base.
 * @param mod Pointer to mod.
 * @return The ammo ID missing for rearming, or "" if none.
 */
std::string Craft::rearm(Mod *mod)
{
	std::string ammo;
	for (std::vector<CraftWeapon*>::iterator i = _weapons.begin(); ; ++i)
	{
		if (i == _weapons.end())
		{
			_status = "STR_REFUELLING";
			break;
		}
		if (*i != 0 && (*i)->isRearming())
		{
			std::string clip = (*i)->getRules()->getClipItem();
			int available = _base->getStorageItems()->getItem(clip);
			if (clip.empty())
			{
				(*i)->rearm(0, 0);
			}
			else if (available > 0)
			{
				int used = (*i)->rearm(available, mod->getItem(clip)->getClipSize());

				if (used == available && (*i)->isRearming())
				{
					ammo = clip;
					(*i)->setRearming(false);
				}

				_base->getStorageItems()->removeItem(clip, used);
			}
			else
			{
				ammo = clip;
				(*i)->setRearming(false);
			}
			break;
		}
	}
	return ammo;
}

/**
 * Returns the craft's battlescape status.
 * @return Is the craft currently in battle?
 */
bool Craft::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Changes the craft's battlescape status.
 * @param inbattle True if it's in battle, False otherwise.
 */
void Craft::setInBattlescape(bool inbattle)
{
	if (inbattle)
		setSpeed(0);
	_inBattlescape = inbattle;
}

/**
 * Returns the craft destroyed status.
 * If the amount of damage the craft take
 * is more than it's health it will be
 * destroyed.
 * @return Is the craft destroyed?
 */
bool Craft::isDestroyed() const
{
	return (_damage >= _stats.damageMax);
}

/**
 * Returns the amount of space available for
 * soldiers and vehicles.
 * @return Space available.
 */
int Craft::getSpaceAvailable() const
{
	return _rules->getSoldiers() - getSpaceUsed();
}

/**
 * Returns the amount of space in use by
 * soldiers and vehicles.
 * @return Space used.
 */
int Craft::getSpaceUsed() const
{
	int vehicleSpaceUsed = 0;
	for (Vehicle* v : _vehicles)
	{
		vehicleSpaceUsed += v->getSize();
	}
	for (Soldier *s : *_base->getSoldiers())
	{
		if (s->getCraft() == this)
		{
			vehicleSpaceUsed += s->getArmor()->getTotalSize();
		}
	}
	return vehicleSpaceUsed;
}

/**
* Checks if there are enough pilots onboard.
* @return True if the craft has enough pilots.
*/
bool Craft::arePilotsOnboard()
{
	if (_rules->getPilots() == 0)
		return true;

	// refresh the list of pilots (must be performed here, list may be out-of-date!)
	const std::vector<Soldier*> pilots = getPilotList();

	return pilots.size() >= _rules->getPilots();
}

/**
* Checks if a pilot is already on the list.
*/
bool Craft::isPilot(int pilotId)
{
	if (std::find(_pilots.begin(), _pilots.end(), pilotId) != _pilots.end())
	{
		return true;
	}

	return false;
}

/**
* Adds a pilot to the list.
*/
void Craft::addPilot(int pilotId)
{
	if (std::find(_pilots.begin(), _pilots.end(), pilotId) == _pilots.end())
	{
		_pilots.push_back(pilotId);
	}
}

/**
* Removes all pilots from the list.
*/
void Craft::removeAllPilots()
{
	_pilots.clear();
}

/**
* Checks if entire crew is made of pilots.
* @return True if all crew members capable of driving must be pilots to satisfy craft requirement.
*/
bool Craft::isCrewPilotsOnly() const
{
	int total = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getCraft() == this && (*i)->getRules()->getAllowPiloting())
		{
			total++;
		}
	}
	if (total == _rules->getPilots())
	{
		return true;
	}
	return false;
}

/**
* Gets the list of craft pilots.
* @return List of pilots.
*/
const std::vector<Soldier*> Craft::getPilotList()
{
	std::vector<Soldier*> result;

	// 1. no pilots
	if (_rules->getPilots() == 0)
		return result;

	// 2. take the pilots from the bottom of the crew list automatically, if option enabled
	if (Options::autoAssignPilots)
	{
		int counter = 0;
		// in reverse order
		for (std::vector<Soldier*>::reverse_iterator i = _base->getSoldiers()->rbegin(); i != _base->getSoldiers()->rend(); ++i)
		{
			if ((*i)->getCraft() == this && (*i)->getRules()->getAllowPiloting())
			{
				result.push_back((*i));
				counter++;
			}
			if (counter >= _rules->getPilots())
			{
				break; // enough pilots found, don't search more
			}
		}
	}
	else
	{
		// 3. only pilots (assign automatically)
		int total = 0;
		for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
		{
			if ((*i)->getCraft() == this && (*i)->getRules()->getAllowPiloting())
			{
				result.push_back((*i));
				total++;
			}
		}
		if (total == _rules->getPilots())
		{
			// nothing more to do
		}
		else
		{
			// 4. pilots and soldiers (pilots must be assigned manually)
			int total2 = 0;
			result.clear();
			for (std::vector<int>::iterator i = _pilots.begin(); i != _pilots.end(); ++i)
			{
				for (std::vector<Soldier*>::iterator j = _base->getSoldiers()->begin(); j != _base->getSoldiers()->end(); ++j)
				{
					if ((*j)->getCraft() == this && (*j)->getRules()->getAllowPiloting() && (*j)->getId() == (*i))
					{
						result.push_back((*j));
						total2++;
						break; // pilot found, don't search anymore
					}
				}
				if (total2 >= _rules->getPilots())
				{
					break; // enough pilots found
				}
			}
		}
	}

	// remember the pilots and return
	removeAllPilots();
	for (std::vector<Soldier*>::iterator i = result.begin(); i != result.end(); ++i)
	{
		addPilot((*i)->getId());
	}
	return result;
}

/**
* Calculates the accuracy bonus based on pilot skills.
* @return Accuracy bonus.
*/
int Craft::getPilotAccuracyBonus(const std::vector<Soldier*> &pilots) const
{
	if (pilots.empty())
		return 0;

	int firingAccuracy = 0;
	for (std::vector<Soldier*>::const_iterator i = pilots.begin(); i != pilots.end(); ++i)
	{
			firingAccuracy += (*i)->getCurrentStats()->firing;
	}
	firingAccuracy = firingAccuracy / pilots.size(); // average firing accuracy of all pilots

	return ((firingAccuracy - 55) * 40) / 100; // -6% to 26% (firing accuracy from 40 to 120, unmodified by armor)
}

/**
* Calculates the dodge bonus based on pilot skills.
* @return Dodge bonus.
*/
int Craft::getPilotDodgeBonus(const std::vector<Soldier*> &pilots) const
{
	if (pilots.empty())
		return 0;

	int reactions = 0;
	for (std::vector<Soldier*>::const_iterator i = pilots.begin(); i != pilots.end(); ++i)
	{
		reactions += (*i)->getCurrentStats()->reactions;
	}
	reactions = reactions / pilots.size(); // average reactions of all pilots

	return ((reactions - 55) * 60) / 100; // -9% to 27% (reactions from 40 to 100, unmodified by armor)
}

/**
* Calculates the approach speed modifier based on pilot skills.
* @return Approach speed modifier.
*/
int Craft::getPilotApproachSpeedModifier(const std::vector<Soldier*> &pilots) const
{
	if (pilots.empty())
		return 2; // vanilla

	int bravery = 0;
	for (std::vector<Soldier*>::const_iterator i = pilots.begin(); i != pilots.end(); ++i)
	{
		bravery += (*i)->getCurrentStats()->bravery;
	}
	bravery = bravery / pilots.size(); // average bravery of all pilots

	if (bravery <= 20)
	{
		return 1; // half the speed
	}
	else if (bravery >= 90)
	{
		return 4; // double the speed
	}
	else if (bravery >= 80)
	{
		return 3; // 50% speed increase
	}

	return 2; // normal speed
}

/**
 * Returns the total amount of vehicles of
 * a certain type stored in the craft.
 * @param vehicle Vehicle type.
 * @return Number of vehicles.
 */
int Craft::getVehicleCount(const std::string &vehicle) const
{
	int total = 0;
	for (std::vector<Vehicle*>::const_iterator i = _vehicles.begin(); i != _vehicles.end(); ++i)
	{
		if ((*i)->getRules()->getType() == vehicle)
		{
			total++;
		}
	}
	return total;
}

/**
 * Returns the craft's dogfight status.
 * @return Is the craft ion a dogfight?
 */
bool Craft::isInDogfight() const
{
	return _inDogfight;
}

/**
 * Changes the craft's dogfight status.
 * @param inDogfight True if it's in dogfight, False otherwise.
 */
void Craft::setInDogfight(bool inDogfight)
{
	_inDogfight = inDogfight;
}

/**
 * Sets interception order (first craft to leave the base gets 1, second 2, etc.).
 * @param order Interception order.
 */
void Craft::setInterceptionOrder(const int order)
{
	_interceptionOrder = order;
}

/**
 * Gets interception order.
 * @return Interception order.
 */
int Craft::getInterceptionOrder() const
{
	return _interceptionOrder;
}

/**
 * Gets the craft's unique id.
 * @return A tuple of the craft's type and per-type id.
 */
CraftId Craft::getUniqueId() const
{
	return std::make_pair(_rules->getType(), _id);
}

}
