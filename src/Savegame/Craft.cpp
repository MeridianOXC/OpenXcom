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
#include "Craft.h"
#include <algorithm>
#include "../fmath.h"
#include "../Engine/Language.h"
#include "../Engine/RNG.h"
#include "../Engine/ScriptBind.h"
#include "../Mod/RuleCraft.h"
#include "CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/Mod.h"
#include "SavedGame.h"
#include "ItemContainer.h"
#include "Soldier.h"
#include "EquipmentLayoutItem.h"
#include "Transfer.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierBonus.h"
#include "Base.h"
#include "Ufo.h"
#include "Waypoint.h"
#include "MissionSite.h"
#include "AlienBase.h"
#include "Vehicle.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleStartingCondition.h"
#include "../Mod/AlienDeployment.h"
#include "SerializationHelper.h"
#include "../Engine/Logger.h"

namespace YAML
{
	template<>
	struct convert<OpenXcom::VehicleDeploymentData>
	{
		static Node encode(const OpenXcom::VehicleDeploymentData& rhs)
		{
			Node node;
			node["type"] = rhs.type;
			node["pos"] = rhs.pos;
			node["dir"] = rhs.dir;
			//node["used"] = rhs.used; // not needed
			return node;
		}

		static bool decode(const Node& node, OpenXcom::VehicleDeploymentData& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.type = node["type"].as<std::string>(rhs.type);
			rhs.pos = node["pos"].as<OpenXcom::Position>(rhs.pos);
			rhs.dir = node["dir"].as<int>(rhs.dir);
			//rhs.used = node["used"].as<bool>(rhs.used); // not needed
			return true;
		}
	};
}

namespace OpenXcom
{

/**
 * Initializes a craft of the specified type and
 * assigns it the latest craft ID available.
 * @param rules Pointer to ruleset.
 * @param base Pointer to base of origin.
 * @param id ID to assign to the craft (0 to not assign).
 */
Craft::Craft(const RuleCraft *rules, Base *base, int id) : MovingTarget(),
	_rules(rules), _base(base), _fuel(0), _excessFuel(0), _damage(0), _shield(0),
	_interceptionOrder(0), _takeoff(0), _weapons(),
	_status("STR_READY"), _lowFuel(false), _mission(false),
	_inBattlescape(false), _inDogfight(false), _stats(),
	_isAutoPatrolling(false), _lonAuto(0.0), _latAuto(0.0),
	_skinIndex(0)
{
	_stats = rules->getStats();
	_items = new ItemContainer();
	_tempSoldierItems = new ItemContainer();
	_tempExtraItems = new ItemContainer();
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
	recalcSpeedMaxRadian();
}

/**
 * Helper method.
 */
void Craft::recalcSpeedMaxRadian()
{
	_speedMaxRadian = calculateRadianSpeed(_stats.speedMax) * 120;
}

/**
 * Delete the contents of the craft from memory.
 */
Craft::~Craft()
{
	for (auto* cw : _weapons)
	{
		delete cw;
	}
	delete _items;
	delete _tempSoldierItems;
	delete _tempExtraItems;
	for (auto* vehicle : _vehicles)
	{
		delete vehicle;
	}
}

/**
 * Loads the craft from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the saved game.
 * @param save Pointer to the saved game.
 */
void Craft::load(const YAML::Node &node, const ScriptGlobal *shared, const Mod *mod, SavedGame *save)
{
	MovingTarget::load(node);
	_fuel = node["fuel"].as<int>(_fuel);
	_excessFuel = node["excessFuel"].as<int>(_excessFuel);
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
				if (type != "0")
				{
					Log(LOG_ERROR) << "Failed to load craft weapon " << type;
				}
			}
			j++;
		}
	}

	_items->load(node["items"], mod);
	// Some old saves have bad items, better get rid of them to avoid further bugs
	for (auto iter = _items->getContents()->begin(); iter != _items->getContents()->end();)
	{
		auto* ruleItem = iter->first;
		if (!ruleItem->canBeEquippedToCraftInventory())
		{
			Log(LOG_WARNING) << "Item '" << iter->first->getType() << "' cannot be equipped in the craft inventory (" << _rules->getType() << ", " << _id << "). Skipping " << iter->second << " items.";
			auto old = iter++; // avoid erase in `removeItem`
			_items->removeItem(old->first, old->second);
		}
		else
		{
			++iter;
		}
	}
	for (YAML::const_iterator i = node["vehicles"].begin(); i != node["vehicles"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		auto* ruleItem = mod->getItem(type);
		if (ruleItem)
		{
			auto* ruleUnit = ruleItem->getVehicleUnit();
			if (ruleUnit)
			{
				int size = ruleUnit->getArmor()->getTotalSize();
				Vehicle *v = new Vehicle(ruleItem, 0, size);
				v->load(*i);
				_vehicles.push_back(v);
			}
			else
			{
				Log(LOG_ERROR) << "Failed to load vehicle " << type;
			}
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load vehicles item " << type;
		}
	}
	_status = node["status"].as<std::string>(_status);
	_lowFuel = node["lowFuel"].as<bool>(_lowFuel);
	_mission = node["mission"].as<bool>(_mission);
	_interceptionOrder = node["interceptionOrder"].as<int>(_interceptionOrder);
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
			for (auto* ufo : *save->getUfos())
			{
				if (ufo->getId() == id)
				{
					setDestination(ufo);
					break;
				}
			}
		}
		else if (type == "STR_WAY_POINT")
		{
			for (auto* wp : *save->getWaypoints())
			{
				if (wp->getId() == id)
				{
					setDestination(wp);
					break;
				}
			}
		}
		else
		{
			// Backwards compatibility
			if (type == "STR_ALIEN_TERROR")
				type = "STR_TERROR_SITE";
			bool found = false;
			for (auto* ms : *save->getMissionSites())
			{
				if (found) break; // loop finished
				if (ms->getId() == id && ms->getDeployment()->getMarkerName() == type)
				{
					setDestination(ms);
					found = true;
				}
			}
			for (auto* ab : *save->getAlienBases())
			{
				if (found) break; // loop finished
				if (ab->getId() == id && ab->getDeployment()->getMarkerName() == type)
				{
					setDestination(ab);
					found = true;
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
	if (const YAML::Node& customSoldierDeployment = node["customSoldierDeployment"])
	{
		_customSoldierDeployment = customSoldierDeployment.as< std::map<int, SoldierDeploymentData> >();
	}
	if (const YAML::Node& customVehicleDeployment = node["customVehicleDeployment"])
	{
		_customVehicleDeployment = customVehicleDeployment.as< std::vector<VehicleDeploymentData> >();
	}
	_skinIndex = node["skinIndex"].as<int>(_skinIndex);
	if (_skinIndex > _rules->getMaxSkinIndex())
	{
		_skinIndex = 0;
	}
	if (_inBattlescape)
		setSpeed(0);

	recalcSpeedMaxRadian();

	_scriptValues.load(node, shared);
}

/**
 * Finishes loading the craft from YAML (called after all other XCOM craft are loaded too).
 * @param node YAML node.
 * @param save The game data. Used to find the UFO's target (= xcom craft).
 */
void Craft::finishLoading(const YAML::Node &node, SavedGame *save)
{
	if (const YAML::Node &dest = node["dest"])
	{
		std::string type = dest["type"].as<std::string>();
		int id = dest["id"].as<int>();

		bool found = false;
		for (auto* xbase : *save->getBases())
		{
			if (found) break; // loop finished
			for (auto* xcraft : *xbase->getCrafts())
			{
				if (found) break; // loop finished
				if (xcraft->getId() == id && xcraft->getRules()->getType() == type)
				{
					setDestination(xcraft);
					found = true;
				}
			}
		}
	}
}

/**
 * Initializes fixed weapons.
 */
void Craft::initFixedWeapons(const Mod* mod)
{
	for (int i = 0; i < _rules->getWeapons(); ++i)
	{
		if (!_rules->getFixedWeaponInSlot(i).empty())
		{
			RuleCraftWeapon* rule = mod->getCraftWeapon(_rules->getFixedWeaponInSlot(i), true);
			CraftWeapon* w = new CraftWeapon(rule, 0);
			addCraftStats(w->getRules()->getBonusStats());
			_weapons.at(i) = w;
		}
	}

	// Remove craft weapons if needed
	if (getMaxUnitsRaw() < 0 || getMaxVehiclesAndLargeSoldiersRaw() < 0)
	{
		size_t weaponIndex = 0;
		for (auto* current : _weapons)
		{
			addCraftStats(-current->getRules()->getBonusStats());
			setShield(getShield());
			delete current;
			_weapons.at(weaponIndex) = 0;
			weaponIndex++;
		}
	}
}

/**
 * Saves the craft to a YAML file.
 * @return YAML node.
 */
YAML::Node Craft::save(const ScriptGlobal *shared) const
{
	YAML::Node node = MovingTarget::save();
	node["type"] = _rules->getType();
	node["fuel"] = _fuel;
	if (_excessFuel != 0)
		node["excessFuel"] = _excessFuel;
	node["damage"] = _damage;
	node["shield"] = _shield;
	for (const auto* cw : _weapons)
	{
		YAML::Node subnode;
		if (cw != 0)
		{
			subnode = cw->save();
		}
		else
		{
			subnode["type"] = "0";
		}
		node["weapons"].push_back(subnode);
	}
	node["items"] = _items->save();
	for (const auto* vehicle : _vehicles)
	{
		node["vehicles"].push_back(vehicle->save());
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
	if (_isAutoPatrolling)
		node["isAutoPatrolling"] = _isAutoPatrolling;
	node["lonAuto"] = serializeDouble(_lonAuto);
	node["latAuto"] = serializeDouble(_latAuto);
	for (int soldierId : _pilots)
	{
		node["pilots"].push_back(soldierId);
	}
	if (!_customSoldierDeployment.empty())
	{
		node["customSoldierDeployment"] = _customSoldierDeployment;
	}
	if (!_customVehicleDeployment.empty())
	{
		node["customVehicleDeployment"] = _customVehicleDeployment;
	}
	if (_skinIndex != 0)
		node["skinIndex"] = _skinIndex;

	_scriptValues.save(node, shared);

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
 * Returns the craft's unique type used for
 * savegame purposes.
 * @return ID.
 */
std::string Craft::getType() const
{
	return _rules->getType();
}

/**
 * Returns the ruleset for the craft's type.
 * @return Pointer to ruleset.
 */
const RuleCraft *Craft::getRules() const
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
	_stats = rules->getStats();
	_weapons.clear();
	for (int i = 0; i < _rules->getWeapons(); ++i)
	{
		_weapons.push_back(0);
	}
}

/**
 * Returns the craft's unique default name.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::string Craft::getDefaultName(Language *lang) const
{
	return lang->getString("STR_CRAFTNAME").arg(lang->getString(getType())).arg(_id);
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
		return _rules->getDefaultDisplayAltitude();
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
int Craft::getNumWeapons(bool onlyLoaded) const
{
	if (_rules->getWeapons() == 0)
	{
		return 0;
	}

	int total = 0;

	for (const auto* cw : _weapons)
	{
		if (cw != 0)
		{
			if (onlyLoaded && !cw->getAmmo())
			{
				continue;
			}
			total++;
		}
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
 * Returns the list of items in the craft equipped by the soldiers.
 * @return Pointer to the item list.
 */
ItemContainer* Craft::getSoldierItems()
{
	return _tempSoldierItems;
}

/**
 * Returns the list of items in the craft not equipped by the soldiers.
 * @return Pointer to the item list.
 */
ItemContainer* Craft::getExtraItems()
{
	return _tempExtraItems;
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
 * Calculates (and stores) the sum of all equipment of all soldiers on the craft.
 */
void Craft::calculateTotalSoldierEquipment()
{
	_tempSoldierItems->clear();

	for (auto* soldier : *_base->getSoldiers())
	{
		if (soldier->getCraft() == this)
		{
			for (auto* invItem : *soldier->getEquipmentLayout())
			{
				// ignore fixed weapons...
				if (!invItem->isFixed())
				{
					_tempSoldierItems->addItem(invItem->getItemType());
				}
				// ...but not their ammo
				for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
				{
					const auto* invItemAmmo = invItem->getAmmoItemForSlot(slot);
					if (invItemAmmo != nullptr)
					{
						_tempSoldierItems->addItem(invItemAmmo);
					}
				}
			}
		}
	}
}

/**
 * Gets the total storage size of all items in the craft. Including vehicles+ammo and craft weapons+ammo.
 */
double Craft::getTotalItemStorageSize(const Mod* mod) const
{
	double total = _items->getTotalSize(mod);

	for (const auto* v : _vehicles)
	{
		total += v->getRules()->getSize();

		auto* clip = v->getRules()->getVehicleClipAmmo();
		if (clip)
		{
			total += clip->getSize() *  v->getRules()->getVehicleClipsLoaded();
		}
	}

	for (const auto* w : _weapons)
	{
		if (w)
		{
			total += w->getRules()->getLauncherItem()->getSize();

			auto* clip = w->getRules()->getClipItem();
			if (clip)
			{
				total += clip->getSize() * w->getClipsLoaded();
			}
		}
	}

	return total;
}

/**
 * Gets the total number of items of a given type in the craft. Including vehicles+ammo and craft weapons+ammo.
 */
int Craft::getTotalItemCount(const RuleItem* item) const
{
	int qty = _items->getItem(item);

	for (const auto* v : _vehicles)
	{
		if (v->getRules() == item)
		{
			qty += 1;
		}
		else if (v->getRules()->getVehicleClipAmmo() == item)
		{
			qty += v->getRules()->getVehicleClipsLoaded();
		}
	}

	for (const auto* w : _weapons)
	{
		if (w)
		{
			if (w->getRules()->getLauncherItem() == item)
			{
				qty += 1;
			}
			else if (w->getRules()->getClipItem() == item)
			{
				qty += w->getClipsLoaded();
			}
		}
	}

	return qty;
}

/**
 * Update stats of craft.
 * @param s
 */
void Craft::addCraftStats(const RuleCraftStats& s)
{
	setDamage(_damage + s.damageMax); //you need "fix" new damage capability first before use.
	_stats += s;

	setFuel(_fuel);

	recalcSpeedMaxRadian();
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
		int overflowFuel = _fuel - _stats.fuelMax + _excessFuel;
		if (overflowFuel > 0 && _rules->getRefuelItem())
		{
			int returnQty = overflowFuel / _rules->getRefuelRate();
			if (returnQty > 0)
			{
				_base->getStorageItems()->addItem(_rules->getRefuelItem(), returnQty);
			}
			_excessFuel = overflowFuel % _rules->getRefuelRate();
		}
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
	return _stats.shieldCapacity;
}

/**
 * Gets the amount of shield this craft has remaining
 * @return shield points remaining.
 */
int Craft::getShield() const
{
	return _shield;
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
 * Returns the percentage of shields remaining out of the max capacity
 * @return Percentage of shield
 */
int Craft::getShieldPercentage() const
{
	return _stats.shieldCapacity != 0 ? _shield * 100 / _stats.shieldCapacity : 0;
}

/**
 * Returns whether the craft is ignored by hunter-killers.
 * (is returning from a mission or is low on fuel).
 * @return True if it's ignored, false otherwise.
 */
bool Craft::isIgnoredByHK() const
{
	return getMissionComplete() || getLowFuel();
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
 * while it's on the air.
 * @param speed Craft speed for estimation.
 * @return Fuel amount.
 */
int Craft::getFuelConsumption(int speed, int escortSpeed) const
{
	if (_rules->getRefuelItem())
		return 1;
	if (escortSpeed > 0)
	{
		// based on the speed of the escorted craft, but capped between 50% and 100% of escorting craft's speed
		return std::max(_stats.speedMax / 200, std::min(escortSpeed / 100, _stats.speedMax / 100));
	}
	return (int)floor(speed / 100.0);
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
	return (int)floor(getFuelConsumption(_stats.speedMax, 0) * getDistance(base) / _speedMaxRadian);
}

/**
 * Gets the maximum number of units (soldiers and vehicles, small and large) that
 * the craft can carry.
 * @return The maximum unit capacity.
 */
int Craft::getMaxUnitsClamped() const
{
	return Clamp(_stats.soldiers, 0, _rules->getMaxUnitsLimit());
}

/**
 * Gets the maximum number of vehicles (and 2x2 soldiers) that
 * the craft can carry.
 * @return The maximum vehicle capacity (incl. 2x2 soldiers).
 */
int Craft::getMaxVehiclesAndLargeSoldiersClamped() const
{
	return Clamp(_stats.vehicles, 0, _rules->getMaxVehiclesAndLargeSoldiersLimit());
}

/**
 * Returns the maximum range the craft can travel
 * from its origin base on its current fuel.
 * @return Range in radians.
 */
double Craft::getBaseRange() const
{
	return _fuel / 2.0 / getFuelConsumption(_stats.speedMax, 0) * _speedMaxRadian;
}

/**
 * Sends the craft back to its origin base.
 */
void Craft::returnToBase()
{
	setDestination(_base);
}

/**
 * Returns the crew to their base (using transfers).
 */
void Craft::evacuateCrew(const Mod *mod)
{
	for (auto iter = _base->getSoldiers()->begin(); iter != _base->getSoldiers()->end(); )
	{
		Soldier* soldier = (*iter);
		if (soldier->getCraft() == this)
		{
			int survivalChance = isPilot(soldier->getId()) ? mod->getPilotsEmergencyEvacuationSurvivalChance() : mod->getCrewEmergencyEvacuationSurvivalChance();
			if (RNG::percent(survivalChance))
			{
				// remove from craft
				soldier->setCraft(0);
				// remove from training, but remember to return to training when back in the base
				{
					if (soldier->isInTraining())
					{
						soldier->setReturnToTrainingWhenHealed(true);
					}
					soldier->setTraining(false);
				}
				// transfer to base
				Transfer *t = new Transfer(mod->getPersonnelTime());
				t->setSoldier(soldier);
				_base->getTransfers()->push_back(t);
				// next
				iter = _base->getSoldiers()->erase(iter);
			}
			else
			{
				++iter; // will be killed later
			}
		}
		else
		{
			++iter; // next
		}
	}
	removeAllPilots(); // just in case
}

/**
 * Moves the craft to its destination.
 */
bool Craft::think()
{
	if (_takeoff == 0)
	{
		move();
	}
	else
	{
		_takeoff--;
		resetMeetPoint();
	}
	if (reachedDestination() && _dest == (Target*)_base)
	{
		setInterceptionOrder(0); // just to be sure
		checkup();
		setDestination(0);
		setSpeed(0);
		_lowFuel = false;
		_mission = false;
		_takeoff = 0;
		return true;
	}
	return false;
}

/**
 * Is the craft about to take off?
 */
bool Craft::isTakingOff() const
{
	return _takeoff == 60;
}

/**
 * Checks the condition of all the craft's systems
 * to define its new status (eg. when arriving at base).
 */
void Craft::checkup()
{
	int available = 0, full = 0;
	for (auto* cw : _weapons)
	{
		if (cw == 0)
			continue;
		available++;
		if (cw->getAmmo() >= cw->getRules()->getAmmoMax() || cw->isDisabled())
		{
			full++;
		}
		else
		{
			cw->setRearming(true);
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
UfoDetection Craft::detect(const Ufo *target, const SavedGame *save, bool alreadyTracked) const
{
	int distance = XcomDistance(getDistance(target));

	int detectionChance = 0;
	UfoDetection detectionType = DETECTION_NONE;

	if (distance < _stats.radarRange)
	{
		// backward compatibility with vanilla
		if (_stats.radarChance == 100)
		{
			detectionType = DETECTION_RADAR;
			detectionChance = 100;
		}
		else
		{
			detectionType = DETECTION_RADAR;
			if (alreadyTracked)
			{
				detectionChance = 100;
			}
			else
			{
				detectionChance = _stats.radarChance * (100 + target->getVisibility()) / 100;
			}
		}
	}

	ModScript::DetectUfoFromCraft::Output args { detectionType, detectionChance, };
	ModScript::DetectUfoFromCraft::Worker work { target, save, this, distance, alreadyTracked, _stats.radarChance, _stats.radarRange, };

	work.execute(target->getRules()->getScript<ModScript::DetectUfoFromCraft>(), args);

	return RNG::percent(args.getSecond()) ? (UfoDetection)args.getFirst() : DETECTION_NONE;
}

/**
 * Consumes the craft's fuel every 10 minutes
 * while it's on the air.
 */
void Craft::consumeFuel(int escortSpeed)
{
	setFuel(_fuel - getFuelConsumption(_speed, escortSpeed));
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

	int needed = _stats.fuelMax - _fuel;
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
		if (w1 != 0 && !w1->isDisabled())
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
 * @return The item ID missing for refuelling, or "" if none.
 */
std::string Craft::refuel()
{
	std::string fuel;
	if (_fuel < _stats.fuelMax)
	{
		const auto* item = _rules->getRefuelItem();
		if (item == nullptr)
		{
			setFuel(_fuel + _rules->getRefuelRate());
		}
		else
		{
			if (_base->getStorageItems()->getItem(item) > 0)
			{
				_base->getStorageItems()->removeItem(item);
				setFuel(_fuel + _rules->getRefuelRate());
				_lowFuel = false;
			}
			else if (!_lowFuel)
			{
				fuel = item->getType();
				if (_fuel > 0)
				{
					_status = "STR_READY";
				}
				else
				{
					_lowFuel = true;
				}
			}
		}
	}
	if (_fuel >= _stats.fuelMax)
	{
		_status = "STR_READY";
		for (const auto* cw : _weapons)
		{
			if (cw && cw->isRearming())
			{
				_status = "STR_REARMING";
				break;
			}
		}
	}
	return fuel;
}

/**
 * Rearms the craft's weapons by adding ammo every hour
 * while it's docked in the base.
 * @param mod Pointer to mod.
 * @return The ammo ID missing for rearming, or "" if none.
 */
const RuleItem* Craft::rearm()
{
	const RuleItem* ammo = nullptr;
	for (auto iter = _weapons.begin(); ; ++iter)
	{
		if (iter == _weapons.end())
		{
			_status = "STR_REFUELLING";
			break;
		}
		CraftWeapon* cw = (*iter);
		if (cw != 0 && cw->isRearming())
		{
			auto* clip = cw->getRules()->getClipItem();
			int available = _base->getStorageItems()->getItem(clip);
			if (clip == nullptr)
			{
				cw->rearm(0, 0);
			}
			else if (available > 0)
			{
				int used = cw->rearm(available, clip->getClipSize());

				if (used == available && cw->isRearming())
				{
					ammo = clip;
					cw->setRearming(false);
				}

				_base->getStorageItems()->removeItem(clip, used);
			}
			else
			{
				ammo = clip;
				cw->setRearming(false);
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
	return getMaxUnitsClamped() - getSpaceUsed();
}

/**
 * Returns the amount of space in use by
 * soldiers and vehicles.
 * @return Space used.
 */
int Craft::getSpaceUsed() const
{
	int vehicleSpaceUsed = 0;
	for (auto* vehicle : _vehicles)
	{
		vehicleSpaceUsed += vehicle->getTotalSize();
	}
	for (auto* soldier : *_base->getSoldiers())
	{
		if (soldier->getCraft() == this)
		{
			vehicleSpaceUsed += soldier->getArmor()->getTotalSize();
		}
	}
	return vehicleSpaceUsed;
}

/**
 * Checks if the commander is onboard.
 * @return True if the commander is onboard.
 */
bool Craft::isCommanderOnboard() const
{
	for (const auto* soldier : *_base->getSoldiers())
	{
		if (soldier->getCraft() == this && soldier->getRank() == RANK_COMMANDER)
		{
			return true;
		}
	}
	return false;
}

/**
 * Checks if there are only permitted soldier types onboard.
 * @return True if all soldiers onboard are permitted.
 */
bool Craft::areOnlyPermittedSoldierTypesOnboard(const RuleStartingCondition* sc) const
{
	for (const auto* soldier : *_base->getSoldiers())
	{
		if (soldier->getCraft() == this && !sc->isSoldierTypePermitted(soldier->getRules()->getType()))
		{
			return false;
		}
	}
	return true;
}

/**
 * Checks if there are enough required items onboard.
 * @return True if the craft has enough required items.
 */
bool Craft::areRequiredItemsOnboard(const std::map<std::string, int>& requiredItems) const
{
	for (const auto& mapItem : requiredItems)
	{
		if (_items->getItem(mapItem.first) < mapItem.second)
		{
			return false;
		}
	}
	return true;
}

/**
 * Destroys given required items.
 */
void Craft::destroyRequiredItems(const std::map<std::string, int>& requiredItems)
{
	for (const auto& mapItem : requiredItems)
	{
		_items->removeItem(mapItem.first, mapItem.second);
	}
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
	const std::vector<Soldier*> pilots = getPilotList(true);

	return (int)(pilots.size()) >= _rules->getPilots();
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
* Gets the list of craft pilots.
* @return List of pilots.
*/
const std::vector<Soldier*> Craft::getPilotList(bool autoAdd)
{
	std::vector<Soldier*> result;

	// 1. no pilots needed
	if (_rules->getPilots() == 0)
		return result;

	{
		// 2. just enough pilots or pilot candidates onboard (assign them all automatically)
		int total = 0;
		for (auto* soldier : *_base->getSoldiers())
		{
			if (soldier->getCraft() == this && soldier->getRules()->getAllowPiloting())
			{
				result.push_back(soldier);
				total++;
			}
		}
		if (total == _rules->getPilots())
		{
			// nothing more to do
		}
		else
		{
			// 3. mix of manually selected pilots and pilot candidates onboard
			int total2 = 0;
			result.clear();
			// 3a. first take all available (manually selected) pilots
			for (int soldierId : _pilots)
			{
				for (auto* soldier : *_base->getSoldiers())
				{
					if (soldier->getCraft() == this && soldier->getRules()->getAllowPiloting() && soldier->getId() == soldierId)
					{
						result.push_back(soldier);
						total2++;
						break; // pilot found, don't search anymore
					}
				}
				if (total2 >= _rules->getPilots())
				{
					break; // enough pilots found
				}
			}
			if (autoAdd)
			{
				// 3b. if not enough manually selected pilots, take some pilot candidates automatically (take from the rear first)
				for (std::vector<Soldier*>::reverse_iterator iter = _base->getSoldiers()->rbegin(); iter != _base->getSoldiers()->rend(); ++iter)
				{
					Soldier* soldier = (*iter);
					if (soldier->getCraft() == this && soldier->getRules()->getAllowPiloting() && !isPilot(soldier->getId()))
					{
						result.push_back(soldier);
						total2++;
					}
					if (total2 >= _rules->getPilots())
					{
						break; // enough pilots found
					}
				}
			}
		}
	}

	// remember the pilots and return
	removeAllPilots();
	for (auto* soldier : result)
	{
		addPilot(soldier->getId());
	}
	return result;
}

/**
* Calculates the accuracy bonus based on pilot skills.
* @return Accuracy bonus.
*/
int Craft::getPilotAccuracyBonus(const std::vector<Soldier*> &pilots, const Mod *mod) const
{
	if (pilots.empty())
		return 0;

	int firingAccuracy = 0;
	for (const auto* soldier : pilots)
	{
		firingAccuracy += soldier->getStatsWithSoldierBonusesOnly()->firing;
	}
	firingAccuracy = firingAccuracy / pilots.size(); // average firing accuracy of all pilots

	return ((firingAccuracy - mod->getPilotAccuracyZeroPoint()) * mod->getPilotAccuracyRange()) / 100;
}

/**
* Calculates the dodge bonus based on pilot skills.
* @return Dodge bonus.
*/
int Craft::getPilotDodgeBonus(const std::vector<Soldier*> &pilots, const Mod *mod) const
{
	if (pilots.empty())
		return 0;

	int reactions = 0;
	for (const auto* soldier : pilots)
	{
		reactions += soldier->getStatsWithSoldierBonusesOnly()->reactions;
	}
	reactions = reactions / pilots.size(); // average reactions of all pilots

	return ((reactions - mod->getPilotReactionsZeroPoint()) * mod->getPilotReactionsRange()) / 100;
}

/**
* Calculates the approach speed modifier based on pilot skills.
* @return Approach speed modifier.
*/
int Craft::getPilotApproachSpeedModifier(const std::vector<Soldier*> &pilots, const Mod *mod) const
{
	if (pilots.empty())
		return 2; // vanilla

	int bravery = 0;
	for (const auto* soldier : pilots)
	{
		bravery += soldier->getStatsWithSoldierBonusesOnly()->bravery;
	}
	bravery = bravery / pilots.size(); // average bravery of all pilots

	if (bravery >= mod->getPilotBraveryThresholdVeryBold())
	{
		return 4; // double the speed
	}
	else if (bravery >= mod->getPilotBraveryThresholdBold())
	{
		return 3; // 50% speed increase
	}
	else if (bravery >= mod->getPilotBraveryThresholdNormal())
	{
		return 2; // normal speed
	}
	else
	{
		return 1; // half the speed
	}
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
	for (const auto* v : _vehicles)
	{
		if (v->getRules()->getType() == vehicle)
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

/**
 * Unloads all the craft contents to the base.
 * @param mod Pointer to mod.
 */
void Craft::unload()
{
	// Remove weapons
	for (auto*& cw : _weapons)
	{
		if (cw != 0)
		{
			_base->getStorageItems()->addItem(cw->getRules()->getLauncherItem());
			_base->getStorageItems()->addItem(cw->getRules()->getClipItem(), cw->getClipsLoaded());
			delete cw;
			cw = nullptr;
		}
	}

	// Remove items
	for (const auto& pair : *_items->getContents())
	{
		_base->getStorageItems()->addItem(pair.first, pair.second);
	}

	// Remove vehicles
	for (auto*& vehicle : _vehicles)
	{
		_base->getStorageItems()->addItem(vehicle->getRules());
		if (vehicle->getRules()->getVehicleClipAmmo())
		{
			_base->getStorageItems()->addItem(vehicle->getRules()->getVehicleClipAmmo(), vehicle->getRules()->getVehicleClipsLoaded());
		}
		delete vehicle;
		vehicle = nullptr;
	}
	_vehicles.clear();

	// Remove soldiers
	for (auto* soldier : *_base->getSoldiers())
	{
		if (soldier->getCraft() == this)
		{
			soldier->setCraft(0);
		}
	}
}

/**
 * Checks if an item can be reused by the craft and
 * updates its status appropriately.
 * @param item Item ID.
 */
void Craft::reuseItem(const RuleItem* item)
{
	// Note: Craft in-base status hierarchy is repair, rearm, refuel, ready.
	// We only want to interrupt processes that are lower in the hierarchy.
	// (And we don't want to interrupt any out-of-base status.)

	// The only states we are willing to interrupt are "ready" and "refuelling"
	if (_status != "STR_READY" && _status != "STR_REFUELLING")
	{
		return;
	}

	// Check if it's ammo to reload the craft
	for (auto* cw : _weapons)
	{
		if (cw != 0 && item == cw->getRules()->getClipItem() && cw->getAmmo() < cw->getRules()->getAmmoMax() && !cw->isDisabled())
		{
			cw->setRearming(true);
			_status = "STR_REARMING";
		}
	}

	// Only consider refuelling if everything else is complete
	if (_status != "STR_READY")
		return;

	// Check if it's fuel to refuel the craft
	if (item == _rules->getRefuelItem() && _fuel < _stats.fuelMax)
		_status = "STR_REFUELLING";
}

/**
 * Gets the attraction value of the craft for alien hunter-killers.
 * @param huntMode Hunt mode ID.
 * @return Attraction value.
 */
int Craft::getHunterKillerAttraction(int huntMode) const
{
	int attraction = 0;
	if (huntMode == 0)
	{
		// prefer interceptors...
		if (_rules->getAllowLanding())
		{
			// craft that can land (i.e. transports) are not attractive
			attraction += 1000000;
		}
		if (getMaxUnitsClamped() > 0)
		{
			// craft with more crew capacity (i.e. transports) are less attractive
			attraction += 500000 + (getMaxUnitsClamped() * 1000);
		}
		// faster craft (i.e. interceptors) are more attractive
		attraction += 100000 - _stats.speedMax;
		// craft with more damage taken are less attractive
		// this is just to simplify re-targeting when interceptor is fast enough to disengage
		// and another identical but healthier interceptor is waiting for its chance
		attraction += _damage * 100 / _stats.damageMax;
	}
	else
	{
		// prefer transports...
		if (!_rules->getAllowLanding())
		{
			// craft that cannot land (i.e. interceptors) are not attractive
			attraction += 1000000;
		}
		// craft with more crew capacity (i.e. transports) are more attractive
		attraction += 500000 - (getMaxUnitsClamped() * 1000);
		// faster craft (i.e. interceptors) are less attractive
		attraction += 100000 + _stats.speedMax;
	}

	// the higher the number the less attractive the target is for UFO hunter-killers
	return attraction;
}

/**
 * Gets the craft's skin sprite ID.
 * @return Sprite ID.
 */
int Craft::getSkinSprite() const
{
	return getRules()->getSprite(_skinIndex);
}

/**
 * Does this craft have a custom deployment set?
 */
bool Craft::hasCustomDeployment() const
{
	return !_customSoldierDeployment.empty() || !_customVehicleDeployment.empty();
}

/**
 * Resets the craft's custom deployment.
 */
void Craft::resetCustomDeployment()
{
	if (!_customSoldierDeployment.empty())
	{
		_customSoldierDeployment.clear();
	}
	if (!_customVehicleDeployment.empty())
	{
		_customVehicleDeployment.clear();
	}
}

/**
 * Resets the craft's custom deployment of vehicles temp variables.
 */
void Craft::resetTemporaryCustomVehicleDeploymentFlags()
{
	for (auto& depl : _customVehicleDeployment)
	{
		depl.used = false;
	}
}

/**
 * Returns the amount of vehicles and 2x2 soldiers currently contained in this craft.
 * @return Number of vehicles and 2x2 soldiers.
 */
int Craft::getNumVehiclesAndLargeSoldiers() const
{
	return getNumTotalVehicles() + getNumLargeSoldiers();
}

/**
 * Returns the amount of 1x1 soldiers from a list that are currently attached to this craft.
 * @return Number of 1x1 soldiers.
 */
int Craft::getNumSmallSoldiers() const
{
	if (_rules->getMaxUnitsLimit() == 0)
		return 0;

	int total = 0;

	for (const auto* s : *_base->getSoldiers())
	{
		if (s->getCraft() == this && s->getArmor()->getSize() == 1)
			++total;
	}

	return total;
}

/**
 * Returns the amount of 2x2 soldiers from a list that are currently attached to this craft.
 * @return Number of 2x2 soldiers.
 */
int Craft::getNumLargeSoldiers() const
{
	if (_rules->getMaxUnitsLimit() == 0)
		return 0;

	int total = 0;

	for (const auto* s : *_base->getSoldiers())
	{
		if (s->getCraft() == this && s->getArmor()->getSize() == 2)
			++total;
	}

	return total;
}

/**
 * Returns the amount of 1x1 vehicles from a list that are currently attached to this craft.
 * @return Number of 1x1 vehicles.
 */
int Craft::getNumSmallVehicles() const
{
	if (_rules->getMaxUnitsLimit() == 0)
		return 0;

	int total = 0;

	for (const auto* v : _vehicles)
	{
		if (v->getTotalSize() == 1)
			++total;
	}

	return total;
}

/**
 * Returns the amount of 2x2 vehicles from a list that are currently attached to this craft.
 * @return Number of 2x2 vehicles.
 */
int Craft::getNumLargeVehicles() const
{
	if (_rules->getMaxUnitsLimit() == 0)
		return 0;

	int total = 0;

	for (const auto* v : _vehicles)
	{
		if (v->getTotalSize() > 1)
			++total;
	}

	return total;
}

/**
 * Returns the amount of 1x1 units from a list that are currently attached to this craft.
 * @return Number of 1x1 units.
 */
int Craft::getNumSmallUnits() const
{
	return getNumSmallSoldiers() + getNumSmallVehicles();
}

/**
 * Returns the amount of 2x2 units from a list that are currently attached to this craft.
 * @return Number of 2x2 units.
 */
int Craft::getNumLargeUnits() const
{
	return getNumLargeSoldiers() + getNumLargeVehicles();
}

/**
 * Returns the total amount of soldiers from a list that are currently attached to this craft.
 * @return Number of soldiers.
 */
int Craft::getNumTotalSoldiers() const
{
	if (_rules->getMaxUnitsLimit() == 0)
		return 0;

	int total = 0;

	for (const auto* s : *_base->getSoldiers())
	{
		if (s->getCraft() == this)
			++total;
	}

	return total;
}

/**
 * Returns the total amount of vehicles from a list that are currently attached to this craft.
 * @return Number of vehicles.
 */
int Craft::getNumTotalVehicles() const
{
	return _vehicles.size();
}

/**
 * Returns the total amount of units from a list that are currently attached to this craft.
 * @return Number of units.
 */
int Craft::getNumTotalUnits() const
{
	return getNumTotalSoldiers() + getNumTotalVehicles();
}

/**
 * Validates craft space and craft constraints on soldier armor change.
 * @return True, if armor change is allowed.
 */
bool Craft::validateArmorChange(int sizeFrom, int sizeTo) const
{
	if (sizeFrom == sizeTo)
	{
		return true;
	}
	else
	{
		if (sizeFrom < sizeTo)
		{
			if (getSpaceAvailable() < 3)
			{
				return false;
			}
			if (getNumVehiclesAndLargeSoldiers() >= getMaxVehiclesAndLargeSoldiersClamped())
			{
				return false;
			}
			if (_rules->getMaxLargeSoldiers() > -1 && getNumLargeSoldiers() >= _rules->getMaxLargeSoldiers())
			{
				return false;
			}
			if (_rules->getMaxLargeUnits() > -1 && getNumLargeUnits() >= _rules->getMaxLargeUnits())
			{
				return false;
			}
		}
		else if (sizeFrom > sizeTo)
		{
			if (_rules->getMaxSmallSoldiers() > -1 && getNumSmallSoldiers() >= _rules->getMaxSmallSoldiers())
			{
				return false;
			}
			if (_rules->getMaxSmallUnits() > -1 && getNumSmallUnits() >= _rules->getMaxSmallUnits())
			{
				return false;
			}
		}
	}
	return true;
}

/**
 * Validates craft space and craft constraints on adding soldier to a craft.
 */
CraftPlacementErrors Craft::validateAddingSoldier(int space, const Soldier* s) const
{
	if (space < s->getArmor()->getTotalSize())
	{
		return CPE_NotEnoughSpace;
	}
	if (_rules->getMaxSoldiers() > -1 && getNumTotalSoldiers() >= _rules->getMaxSoldiers())
	{
		return CPE_TooManySoldiers;
	}
	if (s->getArmor()->getSize() == 1)
	{
		if (_rules->getMaxSmallSoldiers() > -1 && getNumSmallSoldiers() >= _rules->getMaxSmallSoldiers())
		{
			return CPE_TooManySmallSoldiers;
		}
		if (_rules->getMaxSmallUnits() > -1 && getNumSmallUnits() >= _rules->getMaxSmallUnits())
		{
			return CPE_TooManySmallUnits;
		}
	}
	else // armorSize > 1
	{
		if (getNumVehiclesAndLargeSoldiers() >= getMaxVehiclesAndLargeSoldiersClamped())
		{
			return CPE_TooManyVehiclesAndLargeSoldiers;
		}
		if (_rules->getMaxLargeSoldiers() > -1 && getNumLargeSoldiers() >= _rules->getMaxLargeSoldiers())
		{
			return CPE_TooManyLargeSoldiers;
		}
		if (_rules->getMaxLargeUnits() > -1 && getNumLargeUnits() >= _rules->getMaxLargeUnits())
		{
			return CPE_TooManyLargeUnits;
		}
	}
	auto& allowedSoldierGroups = _rules->getAllowedSoldierGroups();
	if (!allowedSoldierGroups.empty())
	{
		if (std::find(allowedSoldierGroups.begin(), allowedSoldierGroups.end(), s->getRules()->getGroup()) == allowedSoldierGroups.end())
		{
			return CPE_SoldierGroupNotAllowed;
		}
	}
	if (_rules->isOnlyOneSoldierGroupAllowed() && getNumTotalSoldiers() > 0)
	{
		int currentGroup = -1;
		for (const auto* tmpSoldier : *_base->getSoldiers())
		{
			if (tmpSoldier->getCraft() == this)
			{
				currentGroup = tmpSoldier->getRules()->getGroup();
				break;
			}
		}
		if (s->getRules()->getGroup() != currentGroup)
		{
			return CPE_SoldierGroupNotSame;
		}
	}
	return CPE_None;
}

/**
 * Validates craft space and craft constraints on adding vehicles to a craft.
 * @return Maximum allowed number of vehicles to add.
 */
int Craft::validateAddingVehicles(int totalSize) const
{
	int maximumAllowed = getSpaceAvailable() / totalSize;

	{
		maximumAllowed = std::min(maximumAllowed, getMaxVehiclesAndLargeSoldiersClamped() - getNumVehiclesAndLargeSoldiers());
	}
	if (_rules->getMaxVehicles() > -1)
	{
		maximumAllowed = std::min(maximumAllowed, _rules->getMaxVehicles() - getNumTotalVehicles());
	}
	if (totalSize == 1)
	{
		if (_rules->getMaxSmallVehicles() > -1)
		{
			maximumAllowed = std::min(maximumAllowed, _rules->getMaxSmallVehicles() - getNumSmallVehicles());
		}
		if (_rules->getMaxSmallUnits() > -1)
		{
			maximumAllowed = std::min(maximumAllowed, _rules->getMaxSmallUnits() - getNumSmallUnits());
		}
	}
	else // armorSize > 1
	{
		if (_rules->getMaxLargeVehicles() > -1)
		{
			maximumAllowed = std::min(maximumAllowed, _rules->getMaxLargeVehicles() - getNumLargeVehicles());
		}
		if (_rules->getMaxLargeUnits() > -1)
		{
			maximumAllowed = std::min(maximumAllowed, _rules->getMaxLargeUnits() - getNumLargeUnits());
		}
	}
	return maximumAllowed;
}

////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

std::string debugDisplayScript(const Craft* c)
{
	if (c)
	{
		std::string s;
		s += Craft::ScriptName;
		s += "(type: \"";
		s += c->getType();
		s += "\" id: ";
		s += std::to_string(c->getId());
		s += " damage: ";
		s += std::to_string(c->getDamagePercentage());
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
 * Register Type in script parser.
 * @param parser Script parser.
 */
void Craft::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<RuleCraft>();

	Bind<Craft> b = { parser };

	b.add<&Craft::getId>("getId");

	b.add<&Craft::getDamage>("getDamage");
	b.addField<&Craft::_stats, &RuleCraftStats::damageMax>("getDamageMax");
	b.add<&Craft::getDamagePercentage>("getDamagePercentage");

	b.add<&Craft::getShield>("getShield");
	b.addField<&Craft::_stats, &RuleCraftStats::shieldCapacity>("getShieldMax");
	b.add<&Craft::getShieldPercentage>("getShieldPercentage");

	b.addRules<RuleCraft, &Craft::getRules>("getRuleCraft");

	RuleCraftStats::addGetStatsScript<&Craft::_stats>(b, "Stats.");

	b.addScriptValue<BindBase::OnlyGet, &Craft::_rules, &RuleCraft::getScriptValuesRaw>();
	b.addScriptValue<&Craft::_scriptValues>();
	b.addDebugDisplay<&debugDisplayScript>();
}


}
