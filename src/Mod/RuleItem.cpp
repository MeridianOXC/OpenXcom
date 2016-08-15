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
#include <algorithm>
#include "Unit.h"
#include "RuleItem.h"
#include "RuleInventory.h"
#include "RuleDamageType.h"
#include "../Savegame/BattleUnit.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Surface.h"
#include "../Engine/ScriptBind.h"
#include "Mod.h"
#include <algorithm>

namespace OpenXcom
{

const float VexelsToTiles = 0.0625f;
const float TilesToVexels = 16.0f;

/**
 * Creates a blank ruleset for a certain type of item.
 * @param type String defining the type.
 */
RuleItem::RuleItem(const std::string &type) :
	_type(type), _name(type), _size(0.0), _costBuy(0), _costSell(0), _transferTime(24), _weight(3),
	_bigSprite(-1), _floorSprite(-1), _handSprite(120), _bulletSprite(-1),
	_fireSound(-1),
	_hitSound(-1), _hitAnimation(0), _hitMissSound(-1), _hitMissAnimation(-1),
	_meleeSound(39), _meleeAnimation(0), _meleeMissSound(-1), _meleeMissAnimation(-1),
	_meleeHitSound(-1), _explosionHitSound(-1),
	_psiSound(-1), _psiAnimation(-1), _psiMissSound(-1), _psiMissAnimation(-1),
	_power(0), _powerRangeReduction(0), _powerRangeThreshold(0),
	_accuracyAimed(0), _accuracyAuto(0), _accuracySnap(0), _accuracyMelee(0), _accuracyUse(0), _accuracyMind(0), _accuracyPanic(20), _accuracyThrow(100),
	_costAimed(0), _costAuto(0, -1), _costSnap(0, -1), _costMelee(0), _costUse(25), _costMind(-1, -1), _costPanic(-1, -1), _costThrow(25), _costPrime(50),
	_clipSize(0), _specialChance(100), _tuLoad(15), _tuUnload(8),
	_battleType(BT_NONE), _fuseType(BFT_NONE), _twoHanded(false), _blockBothHands(false), _waypoint(false), _fixedWeapon(false), _fixedWeaponShow(false), _allowSelfHeal(false), _isConsumable(false), _isFireExtinguisher(false), _invWidth(1), _invHeight(1),
	_painKiller(0), _heal(0), _stimulant(0), _medikitType(BMT_NORMAL), _woundRecovery(0), _healthRecovery(0), _stunRecovery(0), _energyRecovery(0), _moraleRecovery(0), _painKillerRecovery(1.0f), _recoveryPoints(0), _armor(20), _turretType(-1),
	_aiUseDelay(-1), _aiMeleeHitCount(25),
	_recover(true), _liveAlien(false), _attraction(0), _flatUse(0, 1), _flatMelee(-1, -1), _flatThrow(0, 1), _flatPrime(0, 1), _arcingShot(false), _experienceTrainingMode(ETM_DEFAULT), _listOrder(0),
	_maxRange(200), _aimRange(200), _snapRange(15), _autoRange(7), _minRange(0), _dropoff(2), _bulletSpeed(0), _explosionSpeed(0), _autoShots(3), _shotgunPellets(0), _shotgunSpread(100), _shotgunBehavior(0), _shotgunChoke(100),
	_LOSRequired(false), _underwaterOnly(false), _psiReqiured(false),
	_meleePower(0), _specialType(-1), _vaporColor(-1), _vaporDensity(0), _vaporProbability(15),
	_customItemPreviewIndex(0),
	_kneelBonus(-1), _oneHandedPenalty(-1),
	_monthlySalary(0), _monthlyMaintenance(0)
{
	_accuracyMulti.setFiring();
	_meleeMulti.setMelee();
	_throwMulti.setThrowing();
}

/**
 *
 */
RuleItem::~RuleItem()
{
}

/**
 * Get optional value (not equal -1) or default one.
 * @param a Optional cost value.
 * @param b Default cost value.
 * @return Final cost.
 */
RuleItemUseCost RuleItem::getDefault(const RuleItemUseCost& a, const RuleItemUseCost& b) const
{
	RuleItemUseCost n;
	n.Time = a.Time >= 0 ? a.Time : b.Time;
	n.Energy = a.Energy >= 0 ? a.Energy : b.Energy;
	n.Morale = a.Morale >= 0 ? a.Morale : b.Morale;
	n.Health = a.Health >= 0 ? a.Health : b.Health;
	n.Stun = a.Stun >= 0 ? a.Stun : b.Stun;
	return n;
}

/**
 * Load nullable bool value and store it in int (with null as -1).
 * @param a value to set.
 * @param node YAML node.
 */
void RuleItem::loadBool(int& a, const YAML::Node& node) const
{
	if (node)
	{
		if (node.IsNull())
		{
			a = -1;
		}
		else
		{
			a = node.as<bool>();
		}
	}
}

/**
 * Load nullable int (with null as -1).
 * @param a value to set.
 * @param node YAML node.
 */
void RuleItem::loadInt(int& a, const YAML::Node& node) const
{
	if (node)
	{
		if (node.IsNull())
		{
			a = -1;
		}
		else
		{
			a = node.as<int>();
		}
	}
}

/**
 * Load item use cost type (flat or percent).
 * @param a Item use type.
 * @param node YAML node.
 * @param name Name of action type.
 */
void RuleItem::loadPercent(RuleItemUseCost& a, const YAML::Node& node, const std::string& name) const
{
	if (const YAML::Node& cost = node["flat" + name])
	{
		if (cost.IsScalar())
		{
			loadBool(a.Time, cost);
		}
		else
		{
			loadBool(a.Time, cost["time"]);
			loadBool(a.Energy, cost["energy"]);
			loadBool(a.Morale, cost["morale"]);
			loadBool(a.Health, cost["health"]);
			loadBool(a.Stun, cost["stun"]);
		}
	}
}

/**
 * Load item use cost.
 * @param a Item use cost.
 * @param node YAML node.
 * @param name Name of action type.
 */
void RuleItem::loadCost(RuleItemUseCost& a, const YAML::Node& node, const std::string& name) const
{
	loadInt(a.Time, node["tu" + name]);
	if (const YAML::Node& cost = node["cost" + name])
	{
		loadInt(a.Time, cost["time"]);
		loadInt(a.Energy, cost["energy"]);
		loadInt(a.Morale, cost["morale"]);
		loadInt(a.Health, cost["health"]);
		loadInt(a.Stun, cost["stun"]);
	}
}

/**
* Updates item categories based on replacement rules.
* @param replacementRules The list replacement rules.
*/
void RuleItem::updateCategories(std::map<std::string, std::string> *replacementRules)
{
	for (auto it = replacementRules->begin(); it != replacementRules->end(); ++it)
	{
		std::replace(_categories.begin(), _categories.end(), it->first, it->second);
	}
}

/**
 * Loads the item from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the item.
 * @param listOrder The list weight for this item.
 */
void RuleItem::load(const YAML::Node &node, Mod *mod, int listOrder, const ModScript& parsers)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod, listOrder, parsers);
	}
	_type = node["type"].as<std::string>(_type);
	_name = node["name"].as<std::string>(_name);
	_requires = node["requires"].as< std::vector<std::string> >(_requires);
	_requiresBuy = node["requiresBuy"].as< std::vector<std::string> >(_requiresBuy);
	_categories = node["categories"].as< std::vector<std::string> >(_categories);
	_size = node["size"].as<double>(_size);
	_costBuy = node["costBuy"].as<int>(_costBuy);
	_costSell = node["costSell"].as<int>(_costSell);
	_transferTime = node["transferTime"].as<int>(_transferTime);
	_weight = node["weight"].as<int>(_weight);
	if (node["bigSprite"])
	{
		_bigSprite = mod->getSpriteOffset(node["bigSprite"].as<int>(_bigSprite), "BIGOBS.PCK");
	}
	if (node["floorSprite"])
	{
		_floorSprite = mod->getSpriteOffset(node["floorSprite"].as<int>(_floorSprite), "FLOOROB.PCK");
	}
	if (node["handSprite"])
	{
		_handSprite = mod->getSpriteOffset(node["handSprite"].as<int>(_handSprite), "HANDOB.PCK");
	}
	if (node["bulletSprite"])
	{
		// Projectiles: 385 entries ((105*33) / (3*3)) (35 sprites per projectile(0-34), 11 projectiles (0-10))
		_bulletSprite = node["bulletSprite"].as<int>(_bulletSprite) * 35;
		if (_bulletSprite >= 385)
			_bulletSprite += mod->getModOffset();
	}
	if (node["fireSound"])
	{
		_fireSound = mod->getSoundOffset(node["fireSound"].as<int>(_fireSound), "BATTLE.CAT");
	}
	if (node["hitSound"])
	{
		_hitSound = mod->getSoundOffset(node["hitSound"].as<int>(_hitSound), "BATTLE.CAT");
	}
	if (node["hitMissSound"])
	{
		_hitMissSound = mod->getSoundOffset(node["hitMissSound"].as<int>(_hitMissSound), "BATTLE.CAT");
	}
	if (node["meleeSound"])
	{
		_meleeSound = mod->getSoundOffset(node["meleeSound"].as<int>(_meleeSound), "BATTLE.CAT");
	}
	if (node["meleeMissSound"])
	{
		_meleeMissSound = mod->getSoundOffset(node["meleeMissSound"].as<int>(_meleeMissSound), "BATTLE.CAT");
	}
	if (node["psiSound"])
	{
		_psiSound = mod->getSoundOffset(node["psiSound"].as<int>(_psiSound), "BATTLE.CAT");
	}
	if (node["psiMissSound"])
	{
		_psiMissSound = mod->getSoundOffset(node["psiMissSound"].as<int>(_psiMissSound), "BATTLE.CAT");
	}
	if (node["hitAnimation"])
	{
		_hitAnimation = mod->getSpriteOffset(node["hitAnimation"].as<int>(_hitAnimation), "SMOKE.PCK");
	}
	if (node["hitMissAnimation"])
	{
		_hitMissAnimation = mod->getSpriteOffset(node["hitMissAnimation"].as<int>(_hitMissAnimation), "SMOKE.PCK");
	}
	if (node["meleeAnimation"])
	{
		_meleeAnimation = mod->getSpriteOffset(node["meleeAnimation"].as<int>(_meleeAnimation), "HIT.PCK");
	}
	if (node["meleeMissAnimation"])
	{
		_meleeMissAnimation = mod->getSpriteOffset(node["meleeMissAnimation"].as<int>(_meleeMissAnimation), "HIT.PCK");
	}
	if (node["psiAnimation"])
	{
		_psiAnimation = mod->getSpriteOffset(node["psiAnimation"].as<int>(_psiAnimation), "HIT.PCK");
	}
	if (node["psiMissAnimation"])
	{
		_psiMissAnimation = mod->getSpriteOffset(node["psiMissAnimation"].as<int>(_psiMissAnimation), "HIT.PCK");
	}
	if (node["meleeHitSound"])
	{
		_meleeHitSound = mod->getSoundOffset(node["meleeHitSound"].as<int>(_meleeHitSound), "BATTLE.CAT");
	}
	if (node["explosionHitSound"])
	{
		_explosionHitSound = mod->getSoundOffset(node["explosionHitSound"].as<int>(_explosionHitSound), "BATTLE.CAT");
	}

	if (node["battleType"])
	{
		_battleType = (BattleType)node["battleType"].as<int>(_battleType);
		if (_battleType == BT_PSIAMP)
		{
			_psiReqiured = true;
			_dropoff = 1;
			_aimRange = 0;
			_accuracyMulti.setPsiAttack();
		}
		else
		{
			_psiReqiured = false;
		}
		if (_battleType == BT_PROXIMITYGRENADE)
		{
			_fuseType = BFT_INSTANT;
		}
		else if (_battleType == BT_GRENADE)
		{
			_fuseType = BFT_SET;
		}
		else
		{
			_fuseType = BFT_NONE;
		}

		if (_battleType == BT_CORPSE)
		{
			//compatibility hack for corpse explosion, that didn't have defined damage type
			_damageType = *mod->getDamageType(DT_HE);
		}
		_meleeType = *mod->getDamageType(DT_MELEE);
	}

	if (const YAML::Node &type = node["damageType"])
	{
		//load predefined damage type
		_damageType = *mod->getDamageType((ItemDamageType)type.as<int>());
	}
	_damageType.FixRadius = node["blastRadius"].as<int>(_damageType.FixRadius);
	if (const YAML::Node &alter = node["damageAlter"])
	{
		_damageType.load(alter);
	}

	if (const YAML::Node &type = node["meleeType"])
	{
		//load predefined damage type
		_meleeType = *mod->getDamageType((ItemDamageType)type.as<int>());
	}
	if (const YAML::Node &alter = node["meleeAlter"])
	{
		_meleeType.load(alter);
	}

	if (node["skillApplied"])
	{
		if (node["skillApplied"].as<int>(false))
		{
			_meleeMulti.setMelee();
		}
		else
		{
			_meleeMulti.setFlatHundred();
		}
	}
	if (node["strengthApplied"].as<bool>(false))
	{
		_damageBonus.setStrength();
	}

	_power = node["power"].as<int>(_power);
	_psiAttackName = node["psiAttackName"].as<std::string>(_psiAttackName);
	_compatibleAmmo = node["compatibleAmmo"].as< std::vector<std::string> >(_compatibleAmmo);
	_fuseType = (BattleFuseType)node["fuseType"].as<int>(_fuseType);

	_accuracyAimed = node["accuracyAimed"].as<int>(_accuracyAimed);
	_accuracyAuto = node["accuracyAuto"].as<int>(_accuracyAuto);
	_accuracySnap = node["accuracySnap"].as<int>(_accuracySnap);
	_accuracyMelee = node["accuracyMelee"].as<int>(_accuracyMelee);
	_accuracyUse = node["accuracyUse"].as<int>(_accuracyUse);
	_accuracyMind = node["accuracyMindControl"].as<int>(_accuracyMind);
	_accuracyPanic = node["accuracyPanic"].as<int>(_accuracyPanic);
	_accuracyThrow = node["accuracyThrow"].as<int>(_accuracyThrow);

	loadCost(_costAimed, node, "Aimed");
	loadCost(_costAuto, node, "Auto");
	loadCost(_costSnap, node, "Snap");
	loadCost(_costMelee, node, "Melee");
	loadCost(_costUse, node, "Use");
	loadCost(_costMind, node, "MindControl");
	loadCost(_costPanic, node, "Panic");
	loadCost(_costThrow, node, "Throw");
	loadCost(_costPrime, node, "Prime");

	loadBool(_flatUse.Time, node["flatRate"]);
	loadPercent(_flatUse, node, "Use");
	loadPercent(_flatMelee, node, "Melee");
	loadPercent(_flatThrow, node, "Throw");
	loadPercent(_flatPrime, node, "Prime");

	_clipSize = node["clipSize"].as<int>(_clipSize);
	_specialChance = node["specialChance"].as<int>(_specialChance);
	_tuLoad = node["tuLoad"].as<int>(_tuLoad);
	_tuUnload = node["tuUnload"].as<int>(_tuUnload);
	_twoHanded = node["twoHanded"].as<bool>(_twoHanded);
	_blockBothHands = node["blockBothHands"].as<bool>(_blockBothHands);
	_waypoint = node["waypoint"].as<bool>(_waypoint);
	_fixedWeapon = node["fixedWeapon"].as<bool>(_fixedWeapon);
	_fixedWeaponShow = node["fixedWeaponShow"].as<bool>(_fixedWeaponShow);
	_defaultInventorySlot = node["defaultInventorySlot"].as<std::string>(_defaultInventorySlot);
	_allowSelfHeal = node["allowSelfHeal"].as<bool>(_allowSelfHeal);
	_isConsumable = node["isConsumable"].as<bool>(_isConsumable);
	_isFireExtinguisher = node["isFireExtinguisher"].as<bool>(_isFireExtinguisher);
	_invWidth = node["invWidth"].as<int>(_invWidth);
	_invHeight = node["invHeight"].as<int>(_invHeight);

	_painKiller = node["painKiller"].as<int>(_painKiller);
	_heal = node["heal"].as<int>(_heal);
	_stimulant = node["stimulant"].as<int>(_stimulant);
	_woundRecovery = node["woundRecovery"].as<int>(_woundRecovery);
	_healthRecovery = node["healthRecovery"].as<int>(_healthRecovery);
	_stunRecovery = node["stunRecovery"].as<int>(_stunRecovery);
	_energyRecovery = node["energyRecovery"].as<int>(_energyRecovery);
	_moraleRecovery = node["moraleRecovery"].as<int>(_moraleRecovery);
	_painKillerRecovery = node["painKillerRecovery"].as<float>(_painKillerRecovery);
	_medikitType = (BattleMediKitType)node["medikitType"].as<int>(_medikitType);

	_recoveryPoints = node["recoveryPoints"].as<int>(_recoveryPoints);
	_armor = node["armor"].as<int>(_armor);
	_turretType = node["turretType"].as<int>(_turretType);
	if (const YAML::Node &nodeAI = node["ai"])
	{
		_aiUseDelay = nodeAI["useDelay"].as<int>(_aiUseDelay);
		_aiMeleeHitCount = nodeAI["meleeHitCount"].as<int>(_aiMeleeHitCount);
	}
	_recover = node["recover"].as<bool>(_recover);
	_liveAlien = node["liveAlien"].as<bool>(_liveAlien);
	_attraction = node["attraction"].as<int>(_attraction);
	_arcingShot = node["arcingShot"].as<bool>(_arcingShot);
	_experienceTrainingMode = (ExperienceTrainingMode)node["experienceTrainingMode"].as<int>(_experienceTrainingMode);
	_listOrder = node["listOrder"].as<int>(_listOrder);
	_maxRange = node["maxRange"].as<int>(_maxRange);
	_aimRange = node["aimRange"].as<int>(_aimRange);
	_snapRange = node["snapRange"].as<int>(_snapRange);
	_autoRange = node["autoRange"].as<int>(_autoRange);
	_minRange = node["minRange"].as<int>(_minRange);
	_dropoff = node["dropoff"].as<int>(_dropoff);
	_bulletSpeed = node["bulletSpeed"].as<int>(_bulletSpeed);
	_explosionSpeed = node["explosionSpeed"].as<int>(_explosionSpeed);
	_autoShots = node["autoShots"].as<int>(_autoShots);
	_shotgunPellets = node["shotgunPellets"].as<int>(_shotgunPellets);
	_shotgunSpread = node["shotgunSpread"].as<int>(_shotgunSpread);
	_shotgunBehavior = node["shotgunBehavior"].as<int>(_shotgunBehavior);
	_shotgunChoke = node["shotgunChoke"].as<int>(_shotgunChoke);
	_zombieUnit = node["zombieUnit"].as<std::string>(_zombieUnit);
	_LOSRequired = node["LOSRequired"].as<bool>(_LOSRequired);
	_meleePower = node["meleePower"].as<int>(_meleePower);
	_underwaterOnly = node["underwaterOnly"].as<bool>(_underwaterOnly);
	_specialType = node["specialType"].as<int>(_specialType);
	_vaporColor = node["vaporColor"].as<int>(_vaporColor);
	_vaporDensity = node["vaporDensity"].as<int>(_vaporDensity);
	_vaporProbability = node["vaporProbability"].as<int>(_vaporProbability);
	_customItemPreviewIndex = node["customItemPreviewIndex"].as<int>(_customItemPreviewIndex);
	_kneelBonus = node["kneelBonus"].as<int>(_kneelBonus);
	_oneHandedPenalty = node["oneHandedPenalty"].as<int>(_oneHandedPenalty);
	_monthlySalary = node["monthlySalary"].as<int>(_monthlySalary);
	_monthlyMaintenance = node["monthlyMaintenance"].as<int>(_monthlyMaintenance);

	_damageBonus.load(node["damageBonus"]);
	_meleeBonus.load(node["meleeBonus"]);
	_accuracyMulti.load(node["accuracyMultiplier"]);
	_meleeMulti.load(node["meleeMultiplier"]);
	_throwMulti.load(node["throwMultiplier"]);

	_powerRangeReduction = node["powerRangeReduction"].as<float>(_powerRangeReduction);
	_powerRangeThreshold = node["powerRangeThreshold"].as<float>(_powerRangeThreshold);

	_psiReqiured = node["psiRequired"].as<bool>(_psiReqiured);
	_scriptValues.load(node, parsers.getShared());

	_recolorScript.load(_type, node, parsers.recolorItemSprite);
	_spriteScript.load(_type, node, parsers.selectItemSprite);

	_reacActionScript.load(_type, node, parsers.reactionWeaponAction);

	if (!_listOrder)
	{
		_listOrder = listOrder;
	}
}

/**
 * Gets the item type. Each item has a unique type.
 * @return The item's type.
 */
const std::string &RuleItem::getType() const
{
	return _type;
}

/**
 * Gets the language string that names
 * this item. This is not necessarily unique.
 * @return  The item's name.
 */
const std::string &RuleItem::getName() const
{
	return _name;
}

/**
 * Gets the list of research required to
 * use this item.
 * @return The list of research IDs.
 */
const std::vector<std::string> &RuleItem::getRequirements() const
{
	return _requires;
}

/**
 * Gets the list of research required to
 * buy this item from market.
 * @return The list of research IDs.
 */
const std::vector<std::string> &RuleItem::getBuyRequirements() const
{
	return _requiresBuy;
}

/**
* Gets the list of categories
* this item belongs to.
* @return The list of category IDs.
*/
const std::vector<std::string> &RuleItem::getCategories() const
{
	return _categories;
}

/**
* Checks if the item belongs to a category.
* @param category Category name.
* @return True if item belongs to the category, False otherwise.
*/
bool RuleItem::belongsToCategory(const std::string &category) const
{
	return std::find(_categories.begin(), _categories.end(), category) != _categories.end();
}

/**
 * Gets the amount of space this item
 * takes up in a storage facility.
 * @return The storage size.
 */
double RuleItem::getSize() const
{
	return _size;
}

/**
 * Gets the amount of money this item
 * costs to purchase (0 if not purchasable).
 * @return The buy cost.
 */
int RuleItem::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the amount of money this item
 * is worth to sell.
 * @return The sell cost.
 */
int RuleItem::getSellCost() const
{
	return _costSell;
}

/**
 * Gets the amount of time this item
 * takes to arrive at a base.
 * @return The time in hours.
 */
int RuleItem::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets the weight of the item.
 * @return The weight in strength units.
 */
int RuleItem::getWeight() const
{
	return _weight;
}

/**
 * Gets the reference in BIGOBS.PCK for use in inventory.
 * @return The sprite reference.
 */
int RuleItem::getBigSprite() const
{
	return _bigSprite;
}

/**
 * Gets the reference in FLOOROB.PCK for use in battlescape.
 * @return The sprite reference.
 */
int RuleItem::getFloorSprite() const
{
	return _floorSprite;
}

/**
 * Gets the reference in HANDOB.PCK for use in inventory.
 * @return The sprite reference.
 */
int RuleItem::getHandSprite() const
{
	return _handSprite;
}

/**
 * Returns whether this item is held with two hands.
 * @return True if it is two-handed.
 */
bool RuleItem::isTwoHanded() const
{
	return _twoHanded;
}

/**
 * Returns whether this item must be used with both hands.
 * @return True if requires both hands.
 */
bool RuleItem::isBlockingBothHands() const
{
	return _blockBothHands;
}

/**
 * Returns whether this item uses waypoints.
 * @return True if it uses waypoints.
 */
bool RuleItem::isWaypoint() const
{
	return _waypoint;
}

/**
 * Returns whether this item is a fixed weapon.
 * You can't move/throw/drop fixed weapons - e.g. HWP turrets.
 * @return True if it is a fixed weapon.
 */
bool RuleItem::isFixed() const
{
	return _fixedWeapon;
}

/**
 * Do show fixed item on unit.
 * @return true if show even is fixed.
 */
bool RuleItem::getFixedShow() const
{
	return _fixedWeaponShow;
}

/**
 * Gets the name of the default inventory slot.
 * @return String Id.
 */
const std::string &RuleItem::getDefaultInventorySlot() const
{
	return _defaultInventorySlot;
}

/**
 * Gets the item's bullet sprite reference.
 * @return The sprite reference.
 */
int RuleItem::getBulletSprite() const
{
	return _bulletSprite;
}

/**
 * Gets the item's fire sound.
 * @return The fire sound id.
 */
int RuleItem::getFireSound() const
{
	return _fireSound;
}

/**
 * Gets the item's hit sound.
 * @return The hit sound id.
 */
int RuleItem::getHitSound() const
{
	return _hitSound;
}

/**
 * Gets the item's hit animation.
 * @return The hit animation id.
 */
int RuleItem::getHitAnimation() const
{
	return _hitAnimation;
}

/**
 * Gets the item's miss sound.
 * @return The miss sound id.
 */
int RuleItem::getHitMissSound() const
{
	return _hitMissSound;
}

/**
 * Gets the item's miss animation.
 * @return The miss animation id.
 */
int RuleItem::getHitMissAnimation() const
{
	return _hitMissAnimation;
}


/**
 * What sound does this weapon make when you swing this at someone?
 * @return The weapon's melee attack sound.
 */
int RuleItem::getMeleeSound() const
{
	return _meleeSound;
}

/**
 * What is the starting frame offset in hit.pck to use for the animation?
 * @return the starting frame offset in hit.pck to use for the animation.
 */
int RuleItem::getMeleeAnimation() const
{
	return _meleeAnimation;
}

/**
 * What sound does this weapon make when you miss a swing?
 * @return The weapon's melee attack miss sound.
 */
int RuleItem::getMeleeMissSound() const
{
	return _meleeMissSound;
}

/**
 * What is the starting frame offset in hit.pck to use for the animation?
 * @return the starting frame offset in hit.pck to use for the animation.
 */
int RuleItem::getMeleeMissAnimation() const
{
	return _meleeMissAnimation;
}

/**
 * What sound does this weapon make when you punch someone in the face with it?
 * @return The weapon's melee hit sound.
 */
int RuleItem::getMeleeHitSound() const
{
	return _meleeHitSound;
}

/**
 * What sound does explosion sound?
 * @return The weapon's explosion sound.
 */
int RuleItem::getExplosionHitSound() const
{
	return _explosionHitSound;
}

/**
 * Gets the item's psi hit sound.
 * @return The hit sound id.
 */
int RuleItem::getPsiSound() const
{
	return _psiSound;
}

/**
 * What is the starting frame offset in hit.pck to use for the animation?
 * @return the starting frame offset in hit.pck to use for the animation.
 */
int RuleItem::getPsiAnimation() const
{
	return _psiAnimation;
}

/**
 * Gets the item's psi miss sound.
 * @return The miss sound id.
 */
int RuleItem::getPsiMissSound() const
{
	return _psiMissSound;
}

/**
 * What is the starting frame offset in hit.pck to use for the animation?
 * @return the starting frame offset in hit.pck to use for the animation.
 */
int RuleItem::getPsiMissAnimation() const
{
	return _psiMissAnimation;
}

/**
 * Gets the item's power.
 * @return The power.
 */
int RuleItem::getPower() const
{
	return _power;
}

/**
 * Gets amount of power dropped for range in voxels.
 * @return Power reduction.
 */
float RuleItem::getPowerRangeReduction(float range) const
{
	range -= _powerRangeThreshold * TilesToVexels;
	return (_powerRangeReduction * VexelsToTiles) * (range > 0 ? range : 0);
}

/**
 * Get amount of psi accuracy dropped for range in voxels.
 * @param range
 * @return Psi accuracy reduction.
 */
float RuleItem::getPsiAccuracyRangeReduction(float range) const
{
	range -= _aimRange * TilesToVexels;
	return (_dropoff * VexelsToTiles) * (range > 0 ? range : 0);
}

/**
 * Gets the item's accuracy for snapshots.
 * @return The snapshot accuracy.
 */
int RuleItem::getAccuracySnap() const
{
	return _accuracySnap;
}

/**
 * Gets the item's accuracy for autoshots.
 * @return The autoshot accuracy.
 */
int RuleItem::getAccuracyAuto() const
{
	return _accuracyAuto;
}

/**
 * Gets the item's accuracy for aimed shots.
 * @return The aimed accuracy.
 */
int RuleItem::getAccuracyAimed() const
{
	return _accuracyAimed;
}

/**
 * Gets the item's accuracy for melee attacks.
 * @return The melee accuracy.
 */
int RuleItem::getAccuracyMelee() const
{
	return _accuracyMelee;
}

/**
 * Gets the item's accuracy for use psi-amp.
 * @return The psi-amp accuracy.
 */
int RuleItem::getAccuracyUse() const
{
	return _accuracyUse;
}

/**
 * Gets the item's accuracy for mind control use.
 * @return The mind control accuracy.
 */
int RuleItem::getAccuracyMind() const
{
	return _accuracyMind;
}

/**
 * Gets the item's accuracy for panic use.
 * @return The panic accuracy.
 */
int RuleItem::getAccuracyPanic() const
{
	return _accuracyPanic;
}

/**
 * Gets the item's accuracy for throw.
 * @return The throw accuracy.
 */
int RuleItem::getAccuracyThrow() const
{
	return _accuracyThrow;
}

/**
 * Gets the item's time unit percentage for aimed shots.
 * @return The aimed shot TU percentage.
 */
RuleItemUseCost RuleItem::getCostAimed() const
{
	return _costAimed;
}

/**
 * Gets the item's time unit percentage for autoshots.
 * @return The autoshot TU percentage.
 */
RuleItemUseCost RuleItem::getCostAuto() const
{
	return getDefault(_costAuto, _costAimed);
}

/**
 * Gets the item's time unit percentage for snapshots.
 * @return The snapshot TU percentage.
 */
RuleItemUseCost RuleItem::getCostSnap() const
{
	return getDefault(_costSnap, _costAimed);
}

/**
 * Gets the item's time unit percentage for melee attacks.
 * @return The melee TU percentage.
 */
RuleItemUseCost RuleItem::getCostMelee() const
{
	return _costMelee;
}

/**
 * Gets the number of Time Units needed to use this item.
 * @return The number of Time Units needed to use this item.
 */
RuleItemUseCost RuleItem::getCostUse() const
{
	if (_battleType != BT_PSIAMP || !_psiAttackName.empty())
	{
		return _costUse;
	}
	else
	{
		return RuleItemUseCost();
	}
}

/**
 * Gets the number of Time Units needed to use mind control action.
 * @return The number of Time Units needed to mind control.
 */
RuleItemUseCost RuleItem::getCostMind() const
{
	return getDefault(_costMind, _costUse);
}

/**
 * Gets the number of Time Units needed to use panic action.
 * @return The number of Time Units needed to panic.
 */
RuleItemUseCost RuleItem::getCostPanic() const
{
	return getDefault(_costPanic, _costUse);
}

/**
 * Gets the item's time unit percentage for throwing.
 * @return The throw TU percentage.
 */
RuleItemUseCost RuleItem::getCostThrow() const
{
	return _costThrow;
}

/**
 * Gets the item's time unit percentage for prime grenade.
 * @return The prime TU percentage.
 */
RuleItemUseCost RuleItem::getCostPrime() const
{
	return _costPrime;
}

/**
 * Gets the item's time unit for loading weapon ammo.
 * @return The throw TU.
 */
int RuleItem::getTULoad() const
{
	return _tuLoad;
}

/**
 * Gets the item's time unit for unloading weapon ammo.
 * @return The throw TU.
 */
int RuleItem::getTUUnload() const
{
	return _tuUnload;
}

/**
 * Gets a list of compatible ammo.
 * @return Pointer to a list of compatible ammo.
 */
const std::vector<std::string> *RuleItem::getCompatibleAmmo() const
{
	return &_compatibleAmmo;
}

/**
 * Gets the item's damage type.
 * @return The damage type.
 */
const RuleDamageType *RuleItem::getDamageType() const
{
	return &_damageType;
}

/**
 * Gets the item's melee damage type for range weapons.
 * @return The damage type.
 */
const RuleDamageType *RuleItem::getMeleeType() const
{
	return &_meleeType;
}

/**
 * Gets the item's battlye type.
 * @return The battle type.
 */
BattleType RuleItem::getBattleType() const
{
	return _battleType;
}

/**
 * Gets the item's fuse timer type.
 * @return Fuse Timer Type.
 */
BattleFuseType RuleItem::getFuseTimerType() const
{
	return _fuseType;
}

/**
 * Gets the item's default fuse timer.
 * @return Time in turns.
 */
int RuleItem::getFuseTimerDefault() const
{
	if (_fuseType >= BFT_FIX_MIN && _fuseType < BFT_FIX_MAX)
	{
		return (int)_fuseType;
	}
	else if (_fuseType == BFT_SET || _fuseType == BFT_INSTANT)
	{
		return 0;
	}
	else
	{
		return -1; //can't prime
	}
}

/**
 * Gets the item's width in a soldier's inventory.
 * @return The width.
 */
int RuleItem::getInventoryWidth() const
{
	return _invWidth;
}

/**
 * Gets the item's height in a soldier's inventory.
 * @return The height.
 */
int RuleItem::getInventoryHeight() const
{
	return _invHeight;
}

/**
 * Gets the item's ammo clip size.
 * @return The ammo clip size.
 */
int RuleItem::getClipSize() const
{
	return _clipSize;
}

/**
 * Gets the chance of special effect like zombify or corpse explosion or mine triggering.
 * @return Percent value.
 */
int RuleItem::getSpecialChance() const
{
	return _specialChance;
}
/**
 * Draws and centers the hand sprite on a surface
 * according to its dimensions.
 * @param texture Pointer to the surface set to get the sprite from.
 * @param surface Pointer to the surface to draw to.
 */
void RuleItem::drawHandSprite(SurfaceSet *texture, Surface *surface, BattleItem *item) const
{
	Surface *frame = nullptr;
	if (item)
	{
		frame = item->getBigSprite(texture);
		ScriptWorkerBlit scr;
		BattleItem::ScriptFill(&scr, item, BODYPART_ITEM_INVENTORY, 0, 0);
		scr.executeBlit(frame, surface, this->getHandSpriteOffX(), this->getHandSpriteOffY(), 0);
	}
	else
	{
		frame = texture->getFrame(this->getBigSprite());
		frame->setX(this->getHandSpriteOffX());
		frame->setY(this->getHandSpriteOffY());
		frame->blit(surface);
	}
}

/**
 * item's hand spite x offset
 * @return x offset
 */
int RuleItem::getHandSpriteOffX() const
{
	return (RuleInventory::HAND_W - getInventoryWidth()) * RuleInventory::SLOT_W/2;
}

/**
 * item's hand spite y offset
 * @return y offset
 */
int RuleItem::getHandSpriteOffY() const
{
	return (RuleInventory::HAND_H - getInventoryHeight()) * RuleInventory::SLOT_H/2;
}

/**
 * Gets the heal quantity of the item.
 * @return The new heal quantity.
 */
int RuleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Gets the pain killer quantity of the item.
 * @return The new pain killer quantity.
 */
int RuleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Gets the stimulant quantity of the item.
 * @return The new stimulant quantity.
 */
int RuleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Gets the amount of fatal wound healed per usage.
 * @return The amount of fatal wound healed.
 */
int RuleItem::getWoundRecovery() const
{
	return _woundRecovery;
}

/**
 * Gets the amount of health added to a wounded soldier's health.
 * @return The amount of health to add.
 */
int RuleItem::getHealthRecovery() const
{
	return _healthRecovery;
}

/**
 * Gets the amount of energy added to a soldier's energy.
 * @return The amount of energy to add.
 */
int RuleItem::getEnergyRecovery() const
{
	return _energyRecovery;
}

/**
 * Gets the amount of stun removed from a soldier's stun level.
 * @return The amount of stun removed.
 */
int RuleItem::getStunRecovery() const
{
	return _stunRecovery;
}

/**
 * Gets the amount of morale added to a solders's morale.
 * @return The amount of morale to add.
 */
int RuleItem::getMoraleRecovery() const
{
	return _moraleRecovery;
}

/**
 * Gets the medikit morale recovered based on missing health.
 * @return The ratio of how much restore.
 */
float RuleItem::getPainKillerRecovery() const
{
	return _painKillerRecovery;
}

/**
 * Gets the medikit morale recovered based on missing health.
 * @return True if you can use medkit on self.
 */
bool RuleItem::getAllowSelfHeal() const
{
	return _allowSelfHeal;
}

/**
 * Is this (medikit-type) item consumable?
 * @return True if the item is consumable.
 */
bool RuleItem::isConsumable() const
{
	return _isConsumable;
}

/**
 * Does this item extinguish fire?
 * @return True if the item extinguishes fire.
 */
bool RuleItem::isFireExtinguisher() const
{
	return _isFireExtinguisher;
}

/**
 * Gets the medikit type of how it operate.
 * @return Type of medikit.
 */
BattleMediKitType RuleItem::getMediKitType() const
{
	return _medikitType;
}
/**
 * Returns the item's max explosion radius. Small explosions don't have a restriction.
 * Larger explosions are restricted using a formula, with a maximum of radius 10 no matter how large the explosion is.
 * @param stats unit stats
 * @return The radius.
 */
int RuleItem::getExplosionRadius(const BattleUnit *unit) const
{
	int radius = 0;

	if (_damageType.FixRadius == -1)
	{
		radius = getPowerBonus(unit) * _damageType.RadiusEffectiveness;
		if (_damageType.FireBlastCalc)
		{
			radius += 1;
		}
		// cap the formula to 11
		if (radius > 11)
		{
			radius = 11;
		}
		if (radius <= 0)
		{
			radius = 1;
		}
	}
	else
	{
		// unless a blast radius is actually defined.
		radius = _damageType.FixRadius;
	}

	return radius;
}

/**
 * Returns the item's recovery points.
 * This is used during the battlescape debriefing score calculation.
 * @return The recovery points.
 */
int RuleItem::getRecoveryPoints() const
{
	return _recoveryPoints;
}

/**
 * Returns the item's armor.
 * The item is destroyed when an explosion power bigger than its armor hits it.
 * @return The armor.
 */
int RuleItem::getArmor() const
{
	return _armor;
}

/**
 * Returns if the item should be recoverable
 * from the battlescape.
 * @return True if it is recoverable.
 */
bool RuleItem::isRecoverable() const
{
	return _recover;
}


/**
 * Returns the item's Turret Type.
 * @return The turret index (-1 for no turret).
 */
int RuleItem::getTurretType() const
{
	return _turretType;
}

/**
 * Returns first turn when AI can use item.
 * @param Pointer to the mod. 0 by default.
 * @return First turn when AI can use item.
 *	if mod == 0 returns only local defined aiUseDelay
 *	else takes into account global define of aiUseDelay for this item
 */
int RuleItem::getAIUseDelay(const Mod *mod) const
{
	if (mod == 0 || _aiUseDelay >= 0)
		return _aiUseDelay;

	switch (getBattleType())
	{
	case BT_FIREARM:
		if (isWaypoint())
		{
			return mod->getAIUseDelayBlaster();
		}
		else
		{
			return mod->getAIUseDelayFirearm();
		}

	case BT_MELEE:
		return mod->getAIUseDelayMelee();

	case BT_GRENADE:
	case BT_PROXIMITYGRENADE:
		return mod->getAIUseDelayGrenade();

	case BT_PSIAMP:
		return mod->getAIUseDelayPsionic();

	default:
		return _aiUseDelay;
	}
}

/**
 * Resturns number of melee hits AI should do when attacking enemy.
 * @return Number of hits.
 */
int RuleItem::getAIMeleeHitCount() const
{
	return _aiMeleeHitCount;
}

/**
 * Returns if this is a live alien.
 * @return True if this is a live alien.
 */
bool RuleItem::isAlien() const
{
	return _liveAlien;
}

/**
 * Returns whether this item charges a flat rate of use and attack cost.
 * @return True if this item charges a flat rate of use and attack cost.
 */
RuleItemUseCost RuleItem::getFlatUse() const
{
	return _flatUse;
}

/**
 * Returns whether this item charges a flat rate for costThrow.
 * @return True if this item charges a flat rate for costThrow.
 */
RuleItemUseCost RuleItem::getFlatMelee() const
{
	return getDefault(_flatMelee, _flatUse);
}

/**
 * Returns whether this item charges a flat rate for costThrow.
 * @return True if this item charges a flat rate for costThrow.
 */
RuleItemUseCost RuleItem::getFlatThrow() const
{
	return _flatThrow;
}

/**
 * Returns whether this item charges a flat rate for costPrime.
 * @return True if this item charges a flat rate for costPrime.
 */
RuleItemUseCost RuleItem::getFlatPrime() const
{
	return _flatPrime;
}

/**
 * Returns if this weapon should arc its shots.
 * @return True if this weapon should arc its shots.
 */
bool RuleItem::getArcingShot() const
{
	return _arcingShot;
}

/**
 * Returns the experience training mode configured for this weapon.
 * @return The mode ID.
 */
ExperienceTrainingMode RuleItem::getExperienceTrainingMode() const
{
	return _experienceTrainingMode;
}

/**
 * Gets the attraction value for this item (for AI).
 * @return The attraction value.
 */
int RuleItem::getAttraction() const
{
	return _attraction;
}

/**
 * Gets the list weight for this research item
 * @return The list weight.
 */
int RuleItem::getListOrder() const
{
	return _listOrder;
}

/**
 * Gets the maximum range of this weapon
 * @return The maximum range.
 */
int RuleItem::getMaxRange() const
{
	return _maxRange;
}

/**
 * Gets the maximum effective range of this weapon when using Aimed Shot.
 * @return The maximum range.
 */
int RuleItem::getAimRange() const
{
	return _aimRange;
}

/**
 * Gets the maximim effective range of this weapon for Snap Shot.
 * @return The maximum range.
 */
int RuleItem::getSnapRange() const
{
	return _snapRange;
}

/**
 * Gets the maximim effective range of this weapon for Auto Shot.
 * @return The maximum range.
 */
int RuleItem::getAutoRange() const
{
	return _autoRange;
}

/**
 * Gets the minimum effective range of this weapon.
 * @return The minimum effective range.
 */
int RuleItem::getMinRange() const
{
	return _minRange;
}

/**
 * Gets the accuracy dropoff value of this weapon.
 * @return The per-tile dropoff.
 */
int RuleItem::getDropoff() const
{
	return _dropoff;
}

/**
 * Gets the speed at which this bullet travels.
 * @return The speed.
 */
int RuleItem::getBulletSpeed() const
{
	return _bulletSpeed;
}

/**
 * Gets the speed at which this bullet explodes.
 * @return The speed.
 */
int RuleItem::getExplosionSpeed() const
{
	return _explosionSpeed;
}

/**
 * Gets the amount of auto shots fired by this weapon.
 * @return The shots.
 */
int RuleItem::getAutoShots() const
{
	return _autoShots;
}

/**
 * Gets the name of psi attack for action popup list.
 * @return String Id.
 */
const std::string &RuleItem::getPsiAttackName() const
{
	return _psiAttackName;
}

/**
 * is this item a rifle?
 * @return whether or not it is a rifle.
 */
bool RuleItem::isRifle() const
{
	return (_battleType == BT_FIREARM || _battleType == BT_MELEE) && _twoHanded;
}

/**
 * is this item a pistol?
 * @return whether or not it is a pistol.
 */
bool RuleItem::isPistol() const
{
	return (_battleType == BT_FIREARM || _battleType == BT_MELEE) && !_twoHanded;
}

/**
 * Gets the number of projectiles this ammo shoots at once.
 * @return The number of projectiles.
 */
int RuleItem::getShotgunPellets() const
{
	return _shotgunPellets;
}

/**
 * Gets the spread of projectiles from this ammo for use in shotgun behavior.
 * @return The spread.
 */
int RuleItem::getShotgunSpread() const
{
	return _shotgunSpread;
}

/**
 * Do the shotgun pellets follow a grouping, or just fired in a cone-like behavior?
 * @return 0 => cone-like spread, 1 => grouping
 */
int RuleItem::getShotgunBehavior() const
{
	return _shotgunBehavior;
}

/**
 * Gets the shotgun choke value for modifying pellet spread
 * @return weapon shotgun choke value
 */
int RuleItem::getShotgunChoke() const
{
	return _shotgunChoke;
}

/**
 * Gets the unit that the victim is morphed into when attacked.
 * @return The weapon's zombie unit.
 */
const std::string &RuleItem::getZombieUnit() const
{
	return _zombieUnit;
}

/**
 * How much damage does this weapon do when you punch someone in the face with it?
 * @return The weapon's melee power.
 */
int RuleItem::getMeleePower() const
{
	return _meleePower;
}

/**
 * Is line of sight required for this psionic weapon to function?
 * @return If line of sight is required.
 */
bool RuleItem::isLOSRequired() const
{
	return _LOSRequired;
}

/**
 * Can this item be used on land or is it underwater only?
 * @return if this is an underwater weapon or not.
 */
bool RuleItem::isWaterOnly() const
{
	return _underwaterOnly;
}

/**
 * Is psi skill is required to use this weapon.
 * @return If psi skill is required.
 */
bool RuleItem::isPsiRequired() const
{
	return _psiReqiured;
}

/**
 * Compute power bonus based on unit stats.
 * @param stats unit stats
 * @return bonus power.
 */
int RuleItem::getPowerBonus(const BattleUnit *unit) const
{
	return _power + _damageBonus.getBonus(unit);
}

/**
 * Compute power bonus based on unit stats.
 * @param stats unit stats
 * @return bonus power.
 */
int RuleItem::getMeleeBonus(const BattleUnit *unit) const
{
	return _meleePower + _meleeBonus.getBonus(unit);
}

/**
 * Compute multiplier of melee hit chance based on unit stats.
 * @param stats unit stats
 * @return multiplier.
 */
int RuleItem::getMeleeMultiplier(const BattleUnit *unit) const
{
	return _meleeMulti.getBonus(unit);
}

/**
 * Compute multiplier of accuracy based on unit stats.
 * @param stats unit stats
 * @return multiplier.
 */
int RuleItem::getAccuracyMultiplier(const BattleUnit *unit) const
{
	return _accuracyMulti.getBonus(unit);
}

/**
 * Compute multiplier of throw accuracy based on unit stats.
 * @param stats unit stats
 * @return multiplier.
 */
int RuleItem::getThrowMultiplier(const BattleUnit *unit) const
{
	return _throwMulti.getBonus(unit);
}
/**
 * Gets the associated special type of this item.
 * note that type 14 is the alien brain, and types
 * 0 and 1 are "regular tile" and "starting point"
 * so try not to use those ones.
 * @return special type.
 */
int RuleItem::getSpecialType() const
{
	return _specialType;
}

/**
 * Gets the color offset to use for the vapor trail.
 * @return the color offset.
 */
int RuleItem::getVaporColor() const
{
	return _vaporColor;
}

/**
 * Gets the vapor cloud density for the vapor trail.
 * @return the vapor density.
 */
int RuleItem::getVaporDensity() const
{
	return _vaporDensity;
}

/**
 * Gets the vapor cloud probability for the vapor trail.
 * @return the vapor probability.
 */
int RuleItem::getVaporProbability() const
{
	return _vaporProbability;
}

/**
 * Get recoloring script.
 * @return Script for recoloring.
 */
const ModScript::RecolorItemParser::Container &RuleItem::getRecolorScript() const
{
	return _recolorScript;
}

/**
 * Get switch sprite script.
 * @return Script for switching.
 */
const ModScript::SelectItemParser::Container &RuleItem::getSpriteScript() const
{
	return _spriteScript;
}
/**
 * Get script that caclualte reaction based on item that is used for action.
 * @return Script that calculate reaction.
 */
const ModScript::ReactionUnitParser::Container &RuleItem::getReacActionScript() const
{
	return _reacActionScript;
}

namespace
{

void getBattleTypeScript(RuleItem *ri, int &ret)
{
	if (ri)
	{
		ret = (int)ri->getBattleType();
		return;
	}
	ret = (int)BT_NONE;
}

}


/**
 * Register RuleItem in script parser.
 * @param parser Script parser.
 */
void RuleItem::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<Mod>();

	Bind<RuleItem> ri = { parser };

	ri.addCustomConst("BT_NONE", BT_NONE);
	ri.addCustomConst("BT_FIREARM", BT_FIREARM);
	ri.addCustomConst("BT_AMMO", BT_AMMO);
	ri.addCustomConst("BT_MELEE", BT_MELEE);
	ri.addCustomConst("BT_GRENADE", BT_GRENADE);
	ri.addCustomConst("BT_PROXIMITYGRENADE", BT_PROXIMITYGRENADE);
	ri.addCustomConst("BT_MEDIKIT", BT_MEDIKIT);
	ri.addCustomConst("BT_SCANNER", BT_SCANNER);
	ri.addCustomConst("BT_MINDPROBE", BT_MINDPROBE);
	ri.addCustomConst("BT_PSIAMP", BT_PSIAMP);
	ri.addCustomConst("BT_FLARE", BT_FLARE);
	ri.addCustomConst("BT_CORPSE", BT_CORPSE);

	ri.add<&RuleItem::getAccuracyAimed>("getAccuracyAimed");
	ri.add<&RuleItem::getAccuracyAuto>("getAccuracyAuto");
	ri.add<&RuleItem::getAccuracyMelee>("getAccuracyMelee");
	ri.add<&RuleItem::getAccuracyMind>("getAccuracyMind");
	ri.add<&RuleItem::getAccuracyPanic>("getAccuracyPanic");
	ri.add<&RuleItem::getAccuracySnap>("getAccuracySnap");
	ri.add<&RuleItem::getAccuracyThrow>("getAccuracyThrow");
	ri.add<&RuleItem::getAccuracyUse>("getAccuracyUse");

	ri.add<&RuleItem::getArmor>("getArmorValue");
	ri.add<&RuleItem::getWeight>("getWeight");
	ri.add<&getBattleTypeScript>("getBattleType");
	ri.add<&RuleItem::isWaypoint>("isWaypoint");
	ri.add<&RuleItem::isWaterOnly>("isWaterOnly");
	ri.add<&RuleItem::isTwoHanded>("isTwoHanded");
	ri.add<&RuleItem::isBlockingBothHands>("isBlockingBothHands");

	ri.addScriptValue<&RuleItem::_scriptValues>(false);
}

/**
 * Gets the index of the sprite in the CustomItemPreview sprite set.
 * @return Sprite index.
 */
int RuleItem::getCustomItemPreviewIndex() const
{
	return _customItemPreviewIndex;
}

/**
* Gets the kneel bonus (15% bonus is encoded as 100+15 = 115).
* @return Kneel bonus.
*/
int RuleItem::getKneelBonus(Mod *mod) const
{
	return _kneelBonus != -1 ? _kneelBonus : mod->getKneelBonusGlobal();
}

/**
* Gets the one-handed penalty (20% penalty is encoded as 100-20 = 80).
* @return One-handed penalty.
*/
int RuleItem::getOneHandedPenalty(Mod *mod) const
{
	return _oneHandedPenalty != -1 ? _oneHandedPenalty : mod->getOneHandedPenaltyGlobal();
}

/**
* Gets the monthly salary.
* @return Monthly salary.
*/
int RuleItem::getMonthlySalary() const
{
	return _monthlySalary;
}

/**
* Gets the monthly maintenance.
* @return Monthly maintenance.
*/
int RuleItem::getMonthlyMaintenance() const
{
	return _monthlyMaintenance;
}

}
