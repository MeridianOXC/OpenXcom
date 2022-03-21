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

#include "BattleItem.h"
#include "BattleUnit.h"
#include "Tile.h"
#include "SavedGame.h"
#include "SavedBattleGame.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleSkill.h"
#include "../Mod/RuleInventory.h"
#include "../Engine/Collections.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Script.h"
#include "../Engine/ScriptBind.h"
#include "../Engine/RNG.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Initializes a item of the specified type.
 * @param rules Pointer to ruleset.
 * @param id The id of the item.
 */
BattleItem::BattleItem(const RuleItem *rules, int *id) : _id(*id), _rules(rules), _owner(0), _previousOwner(0), _unit(0), _tile(0), _inventorySlot(0), _inventoryX(0), _inventoryY(0), _ammoItem{ }, _fuseTimer(-1), _ammoQuantity(0), _painKiller(0), _heal(0), _stimulant(0), _XCOMProperty(false), _droppedOnAlienTurn(false), _isAmmo(false), _isWeaponWithAmmo(false), _fuseEnabled(false)
{
	(*id)++;
	if (_rules)
	{
		_confMelee = _rules->getConfigMelee();
		setAmmoQuantity(_rules->getClipSize());
		if (_rules->getBattleType() == BT_MEDIKIT)
		{
			setHealQuantity (_rules->getHealQuantity());
			setPainKillerQuantity (_rules->getPainKillerQuantity());
			setStimulantQuantity (_rules->getStimulantQuantity());
		}
		// weapon does not need ammo, ammo item points to weapon
		else if (_rules->getBattleType() == BT_FIREARM || _rules->getBattleType() == BT_MELEE)
		{
			_confAimedOrLaunch = _rules->getConfigAimed();
			_confAuto = _rules->getConfigAuto();
			_confSnap = _rules->getConfigSnap();
			bool showSelfAmmo = _rules->getClipSize() > 0;
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				bool used = false;
				used |= (_confAimedOrLaunch && _confAimedOrLaunch->ammoSlot == slot);
				used |= (_confAuto && _confAuto->ammoSlot == slot);
				used |= (_confSnap && _confSnap->ammoSlot == slot);
				used |= (_confMelee && _confMelee->ammoSlot == slot);
				if (_rules->getCompatibleAmmoForSlot(slot)->empty())
				{
					if (used && showSelfAmmo)
					{
						_ammoVisibility[slot] = true;
						showSelfAmmo = false;
					}
					_ammoItem[slot] = this;
				}
				else
				{
					_ammoVisibility[slot] = used;
					_isWeaponWithAmmo = true;
				}
			}
		}
	}
}

/**
 *
 */
BattleItem::~BattleItem()
{
}

/**
 * Loads the item from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the item.
 */
void BattleItem::load(const YAML::Node &node, Mod *mod, const ScriptGlobal *shared)
{
	std::string slot = node["inventoryslot"].as<std::string>("NULL");
	if (slot != "NULL")
	{
		if (mod->getInventory(slot))
		{
			_inventorySlot = mod->getInventory(slot);

		}
		else
		{
			_inventorySlot = mod->getInventoryGround();
		}
	}
	_inventoryX = node["inventoryX"].as<int>(_inventoryX);
	_inventoryY = node["inventoryY"].as<int>(_inventoryY);
	_ammoQuantity = node["ammoqty"].as<int>(_ammoQuantity);
	_painKiller = node["painKiller"].as<int>(_painKiller);
	_heal = node["heal"].as<int>(_heal);
	_stimulant = node["stimulant"].as<int>(_stimulant);
	//_fuseTimer = node["fuseTimer"].as<int>(_fuseTimer);
	if (node["fuseTimer"])
	{
		// needed for compatibility with OXC
		setFuseTimer(node["fuseTimer"].as<int>());
	}
	_fuseEnabled = node["fuseEnabed"].as<bool>(_fuseEnabled);
	_droppedOnAlienTurn = node["droppedOnAlienTurn"].as<bool>(_droppedOnAlienTurn);
	_XCOMProperty = node["XCOMProperty"].as<bool>(_XCOMProperty);
	_scriptValues.load(node, shared);
}

/**
 * Saves the item to a YAML file.
 * @return YAML node.
 */
YAML::Node BattleItem::save(const ScriptGlobal *shared) const
{
	YAML::Node node;
	node["id"] = _id;
	node["type"] = _rules->getType();
	if (_owner)
		node["owner"] = _owner->getId();
	if (_previousOwner)
		node["previousOwner"] = _previousOwner->getId();
	if (_unit)
		node["unit"] = _unit->getId();

	if (_inventorySlot)
		node["inventoryslot"] = _inventorySlot->getId();
	node["inventoryX"] = _inventoryX;
	node["inventoryY"] = _inventoryY;

	if (_tile)
		node["position"] = _tile->getPosition();
	node["ammoqty"] = _ammoQuantity;
	if (_ammoItem[0])
	{
		node["ammoItem"] = _ammoItem[0]->getId();
	}
	Collections::untilLastIf(
		_ammoItem,
		[](BattleItem *i)
		{
			return i != nullptr;
		},
		[&](BattleItem *i)
		{
			node["ammoItemSlots"].push_back(i ? i->getId() : -1);
		}
	);
	if (_rules && _rules->getBattleType() == BT_MEDIKIT)
	{
		node["painKiller"] = _painKiller;
		node["heal"] = _heal;
		node["stimulant"] = _stimulant;
	}
	if (_fuseTimer != -1)
		node["fuseTimer"] = _fuseTimer;
	if (_fuseEnabled)
		node["fuseEnabed"] = _fuseEnabled;
	if (_droppedOnAlienTurn)
		node["droppedOnAlienTurn"] = _droppedOnAlienTurn;
	if (_XCOMProperty)
		node["XCOMProperty"] = _XCOMProperty;
	_scriptValues.save(node, shared);

	return node;
}

/**
 * Gets the ruleset for the item's type.
 * @return Pointer to ruleset.
 */
const RuleItem *BattleItem::getRules() const
{
	return _rules;
}

/**
 * Gets the turns until detonation. -1 = unprimed grenade
 * @return turns until detonation.
 */
int BattleItem::getFuseTimer() const
{
	return _fuseTimer;
}

/**
 * Sets the turn to explode on.
 * @param turns Turns until detonation (player/alien turns, not game turns).
 */
void BattleItem::setFuseTimer(int turns)
{
	auto event = _rules->getFuseTriggerEvent();
	_fuseTimer = turns;
	if (_fuseTimer >= 0)
	{
		if (event->throwTrigger || event->proximityTrigger)
		{
			_fuseEnabled = false;
		}
		else if (event->defaultBehavior)
		{
			_fuseEnabled = true;
		}
		else
		{
			_fuseEnabled = false;
		}
	}
	else
	{
		_fuseEnabled = false;
	}
}

/**
 * Gets if fuse was triggered.
 */
bool BattleItem::isFuseEnabled() const
{
	return _fuseEnabled;
}

/**
 * Set fuse trigger.
 */
void BattleItem::setFuseEnabled(bool enable)
{
	if (getFuseTimer() > -1)
	{
		_fuseEnabled = enable;
	}
}

/**
 * Called at end of turn.
 */
void BattleItem::fuseEndTurnUpdate()
{
	auto event = _rules->getFuseTriggerEvent();
	if (_fuseEnabled && getFuseTimer() > 0)
	{
		if (event->defaultBehavior)
		{
			if (_rules->getFuseTimerType() != BFT_INSTANT)
			{
				--_fuseTimer;
			}
		}
	}
}

/**
 * Get if item can trigger end of turn effect.
 * @return True if grenade should explode or other item removed
 */
bool BattleItem::fuseTimeEvent()
{
	auto event = _rules->getFuseTriggerEvent();
	auto check = [&]
	{
		if (_fuseEnabled && getFuseTimer() == 0)
		{
			if (event->defaultBehavior)
			{
				return _rules->getFuseTimerType() != BFT_INSTANT;
			}
		}
		return false;
	};

	if (check())
	{
		if (RNG::percent(_rules->getSpecialChance()))
		{
			return true;
		}
		else
		{
			//grenade fail to explode or item to get removed.
			if (_rules->getFuseTimerType() == BFT_SET)
			{
				setFuseTimer(1);
			}
			else
			{
				setFuseTimer(-1);
			}
		}
	}
	return false;
}

/**
 * Called when item is throw.
 */
bool BattleItem::fuseThrowEvent()
{
	auto event = _rules->getFuseTriggerEvent();
	auto check = [&]
	{
		if (_fuseEnabled && getFuseTimer() == 0)
		{
			if (event->throwExplode)
			{
				return true;
			}
			else if (event->defaultBehavior)
			{
				return _rules->getBattleType() == BT_GRENADE && (Options::battleInstantGrenade || _rules->getFuseTimerType() == BFT_INSTANT);
			}
		}
		return false;
	};

	if (event->throwTrigger)
	{
		if (_rules->getFuseTimerType() == BFT_NONE)
		{
			_fuseEnabled = true;
			_fuseTimer = 0;
		}
		else if (_fuseTimer >= 0)
		{
			_fuseEnabled = true;
		}
	}

	if (check())
	{
		return RNG::percent(_rules->getSpecialChance());
	}
	return false;
}

/**
 * Called when item is throw.
 */
bool BattleItem::fuseProximityEvent()
{
	auto event = _rules->getFuseTriggerEvent();
	auto check = [&]
	{
		if (_fuseEnabled && getFuseTimer() >= 0)
		{
			if (event->proximityExplode)
			{
				return true;
			}
			else if (event->defaultBehavior)
			{
				return _rules->getBattleType() == BT_PROXIMITYGRENADE;
			}
		}
		return false;
	};

	if (event->proximityTrigger)
	{
		if (_rules->getFuseTimerType() == BFT_NONE)
		{
			_fuseEnabled = true;
			_fuseTimer = 0;
		}
		else if (_fuseTimer >= 0)
		{
			_fuseEnabled = true;
		}
	}

	if (check())
	{
		return RNG::percent(_rules->getSpecialChance());
	}
	return false;
}


/**
 * Gets the quantity of ammo in this item.
 * @return Ammo quantity.
 */
int BattleItem::getAmmoQuantity() const
{
	if (_rules->getClipSize() == -1)
	{
		return 255;
	}
	return _ammoQuantity;
}

/**
 * Changes the quantity of ammo in this item.
 * @param qty Ammo quantity.
 */
void BattleItem::setAmmoQuantity(int qty)
{
	_ammoQuantity = qty;
}

/**
 * Spends a bullet from the ammo in this item.
 * (Or spends a certain amount of energy from this battery item.)
 * @return True if there are bullets left.
 */
bool BattleItem::spendBullet(int spendPerShot)
{
	if (_ammoQuantity >= spendPerShot)
		_ammoQuantity -= spendPerShot;

	if (_ammoQuantity == 0)
		return false;
	else
		return true;
}

void BattleItem::spendHealingItemUse(BattleMediKitAction mediKitAction)
{
	if (mediKitAction == BMA_PAINKILLER)
	{
		setPainKillerQuantity(getPainKillerQuantity() - 1);
	}
	else if (mediKitAction == BMA_STIMULANT)
	{
		setStimulantQuantity(getStimulantQuantity() - 1);
	}
	else if (mediKitAction == BMA_HEAL)
	{
		setHealQuantity(getHealQuantity() - 1);
	}
}


/**
 * Check if owner is removed from game.
 */
bool BattleItem::isOwnerIgnored() const
{
	return _owner && _owner->isIgnored();
}

/**
 * Gets the item's owner.
 * @return Pointer to Battleunit.
 */
BattleUnit *BattleItem::getOwner()
{
	return _owner;
}

/**
 * Gets the item's owner.
 * @return Pointer to Battleunit.
 */
const BattleUnit *BattleItem::getOwner() const
{
	return _owner;
}

/**
 * Gets the item's previous owner.
 * @return Pointer to Battleunit.
 */
BattleUnit *BattleItem::getPreviousOwner()
{
	return _previousOwner;
}

/**
 * Gets the item's previous owner.
 * @return Pointer to Battleunit.
 */
const BattleUnit *BattleItem::getPreviousOwner() const
{
	return _previousOwner;
}

/**
 * Sets the item's owner.
 * @param owner Pointer to Battleunit.
 */
void BattleItem::setOwner(BattleUnit *owner)
{
	_previousOwner = _owner;
	_owner = owner;
}

/**
 * Sets the item's previous owner.
 * @param owner Pointer to Battleunit.
 */
void BattleItem::setPreviousOwner(BattleUnit *owner)
{
	_previousOwner = owner;
}

/**
 * Removes the item from the previous owner and moves it to the new owner.
 * @param owner Pointer to Battleunit.
 */
void BattleItem::moveToOwner(BattleUnit *owner)
{
	if (_tile)
	{
		_tile->removeItem(this);
		_tile = nullptr;
	}
	if (owner != _owner)
	{
		setOwner(owner);

		if (_previousOwner)
		{
			for (std::vector<BattleItem*>::iterator i = _previousOwner->getInventory()->begin(); i != _previousOwner->getInventory()->end(); ++i)
			{
				if ((*i) == this)
				{
					_previousOwner->getInventory()->erase(i);
					break;
				}
			}
		}
		if (_owner)
		{
			_owner->getInventory()->push_back(this);
		}
	}
}

/**
 * Gets the item's inventory slot.
 * @return The slot id.
 */
RuleInventory *BattleItem::getSlot() const
{
	return _inventorySlot;
}

/**
 * Sets the item's inventory slot.
 * @param slot The slot id.
 */
void BattleItem::setSlot(RuleInventory *slot)
{
	_inventorySlot = slot;
}

/**
 * Gets the item's inventory X position.
 * @return X position.
 */
int BattleItem::getSlotX() const
{
	return _inventoryX;
}

/**
 * Sets the item's inventory X position.
 * @param x X position.
 */
void BattleItem::setSlotX(int x)
{
	_inventoryX = x;
}

/**
 * Gets the item's inventory Y position.
 * @return Y position.
 */
int BattleItem::getSlotY() const
{
	return _inventoryY;
}

/**
 * Sets the item's inventory Y position.
 * @param y Y position.
 */
void BattleItem::setSlotY(int y)
{
	_inventoryY = y;
}

/**
 * Checks if the item is covering certain inventory slot(s).
 * @param x Slot X position.
 * @param y Slot Y position.
 * @param item Item to check for overlap, or NULL if none.
 * @return True if it is covering.
 */
bool BattleItem::occupiesSlot(int x, int y, BattleItem *item) const
{
	if (item == this || !_inventorySlot)
		return false;
	if (_inventorySlot->getType() == INV_HAND)
		return true;
	if (item == 0)
	{
		return (x >= _inventoryX && x < _inventoryX + _rules->getInventoryWidth() &&
				y >= _inventoryY && y < _inventoryY + _rules->getInventoryHeight());
	}
	else
	{
		return !(x >= _inventoryX + _rules->getInventoryWidth() ||
				x + item->getRules()->getInventoryWidth() <= _inventoryX ||
				y >= _inventoryY + _rules->getInventoryHeight() ||
				y + item->getRules()->getInventoryHeight() <= _inventoryY);
	}
}

/**
 * Gets the item's floor sprite.
 * @return Return current floor sprite.
 */
const Surface *BattleItem::getFloorSprite(const SurfaceSet *set, const SavedBattleGame *save, int animFrame, int shade) const
{
	int i = _rules->getFloorSprite();
	if (i != -1)
	{
		const Surface *surf = set->getFrame(i);
		//enforce compatibility with basic version
		if (surf == nullptr)
		{
			throw Exception("Image missing in 'FLOOROB.PCK' for item '" + _rules->getType() + "'");
		}

		i = ModScript::scriptFunc2<ModScript::SelectItemSprite>(
			_rules,
			i, 0,
			this, save, BODYPART_ITEM_FLOOR, animFrame, shade
		);
		auto newSurf = set->getFrame(i);
		if (newSurf == nullptr)
		{
			newSurf = surf;
		}
		return newSurf;
	}
	else
	{
		return nullptr;
	}
}

/**
 * Gets the item's inventory sprite.
 * @return Return current inventory sprite.
 */
const Surface *BattleItem::getBigSprite(const SurfaceSet *set, const SavedBattleGame *save, int animFrame) const
{
	int i = _rules->getBigSprite();
	if (i != -1)
	{
		const Surface *surf = set->getFrame(i);
		//enforce compatibility with basic version
		if (surf == nullptr)
		{
			throw Exception("Image missing in 'BIGOBS.PCK' for item '" + _rules->getType() + "'");
		}

		i = ModScript::scriptFunc2<ModScript::SelectItemSprite>(
			_rules,
			i, 0,
			this, save, BODYPART_ITEM_INVENTORY, animFrame, 0
		);

		auto newSurf = set->getFrame(i);
		if (newSurf == nullptr)
		{
			newSurf = surf;
		}
		return newSurf;
	}
	else
	{
		return nullptr;
	}
}

/**
 * Check if item use any ammo.
 * @return True if item accept ammo.
 */
bool BattleItem::isWeaponWithAmmo() const
{
	return _isWeaponWithAmmo;
}

/**
 * Check if weapon has enough ammo to shoot.
 * @return True if has enough ammo.
 */
bool BattleItem::haveAnyAmmo() const
{
	if (!_isWeaponWithAmmo)
	{
		return true;
	}
	auto type = _rules->getBattleType();
	if (type == BT_MELEE)
	{
		return getAmmoForAction(BA_HIT);
	}
	else
	{
		return getAmmoForAction(BA_AIMEDSHOT) ||
			getAmmoForAction(BA_AUTOSHOT) ||
			getAmmoForAction(BA_SNAPSHOT);
	}
}

/**
 * Check if weapon have all ammo slot filled.
 * @return True if all ammo slots are fill.
 */
bool BattleItem::haveAllAmmo() const
{
	for (const auto& a : _ammoItem)
	{
		if (a == nullptr)
		{
			return false;
		}
	}
	return true;
}

/**
 * Sets the item's ammo item.
 * @param item The ammo item.
 * @return True if item fit to weapon.
 */
bool BattleItem::setAmmoPreMission(BattleItem *item)
{
	int slot = _rules->getSlotForAmmo(item->getRules());
	if (slot >= 0)
	{
		if (_ammoItem[slot])
			return false;

		setAmmoForSlot(slot, item);
		return true;
	}

	return false;
}

/**
 * Get configuration of action on that item.
 * @param action Action type.
 * @return Return config of item action or nullptr for wrong action type or item.
 */
const RuleItemAction *BattleItem::getActionConf(BattleActionType action) const
{
	switch (action)
	{
	case BA_LAUNCH:
	case BA_AIMEDSHOT: return _confAimedOrLaunch;
	case BA_AUTOSHOT: return _confAuto;
	case BA_SNAPSHOT: return _confSnap;
	case BA_HIT: return _confMelee;
	default: return nullptr;
	}
}

/**
 * Check if attack shoot in arc.
 */
bool BattleItem::getArcingShot(BattleActionType action) const
{
	if (_rules->getArcingShot())
	{
		return true;
	}

	auto conf = getActionConf(action);
	if (conf && conf->arcing)
	{
		return true;
	}

	return false;
}

/**
 * Determines if this item uses ammo.
 */
bool BattleItem::needsAmmoForAction(BattleActionType action) const
{
	auto conf = getActionConf(action);
	if (!conf || conf->ammoSlot == RuleItem::AmmoSlotSelfUse)
	{
		return false;
	}

	return needsAmmoForSlot(conf->ammoSlot);
}

/**
 * Get ammo used by action.
 * @param action Battle Action done using this item.
 * @return
 */
const BattleItem *BattleItem::getAmmoForAction(BattleActionType action) const
{
	auto conf = getActionConf(action);
	if (!conf)
	{
		return nullptr;
	}
	if (conf->ammoSlot == RuleItem::AmmoSlotSelfUse)
	{
		return this;
	}

	auto ammo = getAmmoForSlot(conf->ammoSlot);
	if (ammo && ammo->getAmmoQuantity() == 0)
	{
		return nullptr;
	}
	return ammo;
}

/**
 * Get ammo used by action.
 * @param action Battle Action done using this item.
 * @param message Error message if weapon don't have ammo.
 * @param spendPerShot How much ammo should be spent for one shot.
 * @return
 */
BattleItem *BattleItem::getAmmoForAction(BattleActionType action, std::string* message, int* spendPerShot)
{
	auto conf = getActionConf(action);
	if (!conf)
	{
		return nullptr;
	}
	if (spendPerShot) *spendPerShot = conf->spendPerShot;
	if (conf->ammoSlot == RuleItem::AmmoSlotSelfUse)
	{
		return this;
	}

	auto ammo = getAmmoForSlot(conf->ammoSlot);
	if (ammo == nullptr)
	{
		if (message) *message = "STR_NO_AMMUNITION_LOADED";
		return nullptr;
	}
	if (ammo->getAmmoQuantity() < conf->spendPerShot)
	{
		if (message)
		{
			*message = "STR_NO_ROUNDS_LEFT"; // no rounds left (or not enough energy left in the battery)
		}
		return nullptr;
	}
	return ammo;
}

/**
 * Spend weapon ammo, if depleted remove clip.
 * @param action Battle Action done using this item.
 * @param save Save game.
 */
void BattleItem::spendAmmoForAction(BattleActionType action, SavedBattleGame* save)
{
	if (save->getDebugMode() || getActionConf(action)->ammoSlot == RuleItem::AmmoSlotSelfUse)
	{
		return;
	}

	int spendPerShot = 1;
	auto ammo = getAmmoForAction(action, nullptr, &spendPerShot);
	if (ammo)
	{
		if (ammo->getRules()->getClipSize() > 0 && ammo->spendBullet(spendPerShot) == false)
		{
			save->removeItem(ammo);
			ammo->setIsAmmo(false);
			if (ammo != this)
			{
				for (auto& a : _ammoItem)
				{
					if (a == ammo)
					{
						a = nullptr;
					}
				}
			}
		}
	}
}

/**
 * Check how many shoots attack can perform.
 * @param action Attack type.
 * @param shotCount Current shot count.
 * @return True if still can shoot.
 */
bool BattleItem::haveNextShotsForAction(BattleActionType action, int shotCount) const
{
	auto conf = getActionConf(action);
	if (conf)
	{
		return shotCount < conf->shots;
	}
	return false;
}

/**
 * Determines if the item uses ammo.
 * @return True if ammo is used.
 */
bool BattleItem::needsAmmoForSlot(int slot) const
{
	return (_isWeaponWithAmmo && _ammoItem[slot] != this); // no ammo for this weapon is needed
}

/**
 * Set ammo slot with new ammo
 * @param slot Ammo slot position.
 * @param item Ammo item.
 * @return Old item if was set.
 */
BattleItem *BattleItem::setAmmoForSlot(int slot, BattleItem* item)
{
	if (!needsAmmoForSlot(slot))
	{
		return nullptr;
	}

	BattleItem *oldItem = _ammoItem[slot];
	if (oldItem)
	{
		oldItem->setIsAmmo(false);
	}
	_ammoItem[slot] = item;
	if (item)
	{
		item->moveToOwner(nullptr);
		item->setSlot(nullptr);
		item->setIsAmmo(true);
	}
	return oldItem;
}

/**
 * Gets the item's ammo item.
 * @return The ammo item.
 */
BattleItem *BattleItem::getAmmoForSlot(int slot)
{
	return _ammoItem[slot];
}

/**
 * Gets the item's ammo item.
 * @return The ammo item.
 */
const BattleItem *BattleItem::getAmmoForSlot(int slot) const
{
	return _ammoItem[slot];
}

/**
 * Get ammo count visibility for slot.
 */
bool BattleItem::isAmmoVisibleForSlot(int slot) const
{
	return _ammoVisibility[slot];
}

/**
 * Get item weight with ammo weight.
 * @return Weight.
 */
int BattleItem::getTotalWeight() const
{
	int weight = _rules->getWeight();
	for (const auto& a : _ammoItem)
	{
		if (a && a != this)
		{
			weight += a->_rules->getWeight();
		}
	}
	return weight;
}

/**
 * Get waypoints count of weapon or from ammo.
 * @return Maximum waypoints count or -1 if unlimited.
 */
int BattleItem::getCurrentWaypoints() const
{
	int waypoints = _rules->getWaypoints();
	auto ammo = getAmmoForAction(BA_LAUNCH);
	if (waypoints == 0 && ammo && ammo != this)
	{
		waypoints = ammo->_rules->getWaypoints();
	}
	return waypoints;
}

/**
 * Gets the item's tile.
 * @return The tile.
 */
Tile *BattleItem::getTile() const
{
	return _tile;
}

/**
 * Sets the item's tile.
 * @param tile The tile.
 */
void BattleItem::setTile(Tile *tile)
{
	_tile = tile;
}

/**
 * Gets the item's id.
 * @return The item's id.
 */
int BattleItem::getId() const
{
	return _id;
}

/**
 * Gets the corpse's unit.
 * @return Pointer to BattleUnit.
 */
BattleUnit *BattleItem::getUnit()
{
	return _unit;
}

/**
 * Gets the corpse's unit.
 * @return Pointer to BattleUnit.
 */
const BattleUnit *BattleItem::getUnit() const
{
	return _unit;
}

/**
 * Sets the corpse's unit.
 * @param unit Pointer to BattleUnit.
 */
void BattleItem::setUnit(BattleUnit *unit)
{
	_unit = unit;
}

/**
 * Sets the heal quantity of the item.
 * @param heal The new heal quantity.
 */
void BattleItem::setHealQuantity (int heal)
{
	_heal = heal;
}

/**
 * Gets the heal quantity of the item.
 * @return The new heal quantity.
 */
int BattleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Sets the pain killer quantity of the item.
 * @param pk The new pain killer quantity.
 */
void BattleItem::setPainKillerQuantity (int pk)
{
	_painKiller = pk;
}

/**
 * Gets the pain killer quantity of the item.
 * @return The new pain killer quantity.
 */
int BattleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Sets the stimulant quantity of the item.
 * @param stimulant The new stimulant quantity.
 */
void BattleItem::setStimulantQuantity (int stimulant)
{
	_stimulant = stimulant;
}

/**
 * Gets the stimulant quantity of the item.
 * @return The new stimulant quantity.
 */
int BattleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Sets the XCom property flag. This is to determine at debriefing what goes into the base/craft.
 * @param flag True if it's XCom property.
 */
void BattleItem::setXCOMProperty (bool flag)
{
	_XCOMProperty = flag;
}

/**
 * Gets the XCom property flag. This is to determine at debriefing what goes into the base/craft.
 * @return True if it's XCom property.
 */
bool BattleItem::getXCOMProperty() const
{
	return _XCOMProperty;
}

/**
 * Gets the "dropped on non-player turn" flag. This is to determine whether or not
 * aliens should attempt to pick this item up, as items dropped by the player may be "honey traps".
 * @return True if the aliens dropped the item.
 */
bool BattleItem::getTurnFlag() const
{
	return _droppedOnAlienTurn;
}

/**
 * Sets the "dropped on non-player turn" flag. This is set when the item is dropped in the battlescape
 * or picked up in the inventory screen.
 * @param flag True if the aliens dropped the item.
 */
void BattleItem::setTurnFlag(bool flag)
{
	_droppedOnAlienTurn = flag;
}

/**
 * Converts an unconscious body into a dead one.
 * @param rules the rules of the corpse item to convert this item into.
 */
void BattleItem::convertToCorpse(const RuleItem *rules)
{
	if (_unit && _rules->getBattleType() == BT_CORPSE && rules->getBattleType() == BT_CORPSE)
	{
		_rules = rules;
	}
}

/**
 * Check if item can glow in darkness.
 * @return True if it glow.
 */
bool BattleItem::getGlow() const
{
	if (_rules->getBattleType() == BT_FLARE)
	{
		return (_rules->getFuseTriggerEvent()->defaultBehavior && _rules->getFuseTimerType() == BFT_NONE) || (_fuseEnabled && getFuseTimer() >= 0);
	}
	else
	{
		return false;
	}
}

/**
 * Gets range of glow in tiles.
 * @return Range.
 */
int BattleItem::getGlowRange() const
{
	auto owner = _unit ? _unit : _previousOwner;
	return _rules->getPowerBonus({ BA_NONE, owner, this, this });
}

/**
 * Gets update range needed by this item. For simplicity we always update to max range if it can glow.
 * @return Range.
 */
int BattleItem::getVisibilityUpdateRange() const
{
	return _rules->getBattleType() == BT_FLARE ? getGlowRange() : 1;
}

/**
 * Sets the flag on this item indicating whether or not it is a clip used in a weapon.
 * @param ammo set the ammo flag to this.
 */
void BattleItem::setIsAmmo(bool ammo)
{
	_isAmmo = ammo;
}

/**
 * Checks if this item is loaded into a weapon.
 * @return if this is loaded into a weapon or not.
 */
bool BattleItem::isAmmo() const
{
	return _isAmmo;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

struct getAmmoForSlotScript
{
	static RetEnum func(BattleItem *weapon, BattleItem *&ammo, int slot)
	{
		if (weapon && slot >= 0 && slot < RuleItem::AmmoSlotMax)
		{
			ammo = weapon->getAmmoForSlot(slot);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

struct getAmmoForSlotConstScript
{
	static RetEnum func(const BattleItem *weapon, const BattleItem *&ammo, int slot)
	{
		if (weapon && slot >= 0 && slot < RuleItem::AmmoSlotMax)
		{
			ammo = weapon->getAmmoForSlot(slot);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

struct getAmmoItemScript
{
	static RetEnum func(BattleItem *weapon, BattleItem *&ammo)
	{
		return getAmmoForSlotScript::func(weapon, ammo, 0);
	}
};

struct getAmmoItemConstScript
{
	static RetEnum func(const BattleItem *weapon, const BattleItem *&ammo)
	{
		return getAmmoForSlotConstScript::func(weapon, ammo, 0);
	}
};

struct getAmmoForActionScript
{
	static RetEnum func(BattleItem *weapon, BattleItem *&ammo, int action)
	{
		BattleActionType bat = (BattleActionType)action;
		if (weapon)
		{
			ammo = weapon->getAmmoForAction(bat);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

struct getAmmoForActionConstScript
{
	static RetEnum func(const BattleItem *weapon, const BattleItem *&ammo, int action)
	{
		BattleActionType bat = (BattleActionType)action;
		if (weapon)
		{
			ammo = weapon->getAmmoForAction(bat);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

struct getRuleInventorySlotScript
{
	static RetEnum func(const BattleItem *weapon, const RuleInventory *&inv)
	{
		if (weapon)
		{
			inv = weapon->getSlot();
		}
		else
		{
			inv = nullptr;
		}
		return RetContinue;
	}
};

std::string debugDisplayScript(const BattleItem* bt)
{
	if (bt)
	{
		auto rule = bt->getRules();
		std::string s;
		s += BattleItem::ScriptName;
		s += "(name: \"";
		s += rule->getName();
		s += "\" id: ";
		s += std::to_string(bt->getId());

		auto clipSize = rule->getClipSize();
		if (clipSize > 0)
		{
			s += " ammo: ";
			s += std::to_string(bt->getAmmoQuantity());
			s += "/";
			s += std::to_string(clipSize);
		}
		s += ")";
		return s;
	}
	else
	{
		return "null";
	}
}

void getActionTUsScript(const BattleItem* bt, int& i, const BattleUnit* bu, const int battle_action)
{
	BattleActionType bat = (BattleActionType)battle_action;
	if (bt && bu)
	{
		i = bu->getActionTUs(bat, bt).Time;
	}
	else
	{
		i = -1;
	}
}

void getFuseTimerDefaultScript(const BattleItem* bt, int& i)
{
	if (bt)
	{
		i = bt->getRules()->getFuseTimerDefault();
	}
	else
	{
		i = -1;
	}
}

void setFuseTimerScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setFuseTimer(Clamp(i, -1, 100));
	}
}

void setAmmoQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setAmmoQuantity(Clamp(i, 1, bt->getRules()->getClipSize()));
	}
}

void setHealQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setHealQuantity(Clamp(i, 0, bt->getRules()->getHealQuantity()));
	}
}

void setPainKillerQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setPainKillerQuantity(Clamp(i, 0, bt->getRules()->getPainKillerQuantity()));
	}
}

void setStimulantQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setStimulantQuantity(Clamp(i, 0, bt->getRules()->getStimulantQuantity()));
	}
}

} // namespace

/**
 * Register BattleItem in script parser.
 * @param parser Script parser.
 */
void BattleItem::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<Mod>();
	parser->registerPointerType<RuleItem>();
	parser->registerPointerType<BattleUnit>();

	Bind<BattleItem> bi = { parser };

	bi.addRules<RuleItem, &BattleItem::getRules>("getRuleItem");
	bi.addPair<BattleUnit, &BattleItem::getUnit, &BattleItem::getUnit>("getBattleUnit");
	bi.addFunc<getAmmoItemScript>("getAmmoItem");
	bi.addFunc<getAmmoItemConstScript>("getAmmoItem");
	bi.addFunc<getAmmoForSlotScript>("getAmmoForSlot");
	bi.addFunc<getAmmoForSlotConstScript>("getAmmoForSlot");
	bi.addFunc<getAmmoForActionScript>("getAmmoForAction");
	bi.addFunc<getAmmoForActionConstScript>("getAmmoForAction");
	bi.addFunc<getRuleInventorySlotScript>("getSlot");
	bi.addPair<BattleUnit, &BattleItem::getPreviousOwner, &BattleItem::getPreviousOwner>("getPreviousOwner");
	bi.addPair<BattleUnit, &BattleItem::getOwner, &BattleItem::getOwner>("getOwner");
	bi.add<&BattleItem::getId>("getId");
	bi.add<&BattleItem::getGlow>("getGlow");
	bi.add<&BattleItem::getTotalWeight>("getTotalWeight");
	bi.add<&BattleItem::isAmmo>("isAmmo");
	bi.add<&BattleItem::isSpecialWeapon>("isSpecialWeapon");

	bi.add<&BattleItem::getAmmoQuantity>("getAmmoQuantity");
	bi.add<&setAmmoQuantityScript>("setAmmoQuantity");

	bi.add<&BattleItem::getFuseTimer>("getFuseTimer");
	bi.add<&getFuseTimerDefaultScript>("getFuseTimerDefault", "get default fuse timer");
	bi.add<&setFuseTimerScript>("setFuseTimer", "set item fuse timer, -1 mean disable it");

	bi.add<&BattleItem::isFuseEnabled>("isFuseEnabled", "check if fuse is triggered (like throw or proxy unit)");
	bi.add<&BattleItem::setFuseEnabled>("setFuseEnabled", "force set or unset fuse trigger state");

	bi.add<&BattleItem::getHealQuantity>("getHealQuantity");
	bi.add<&setHealQuantityScript>("setHealQuantity");

	bi.add<&BattleItem::getPainKillerQuantity>("getPainKillerQuantity");
	bi.add<&setPainKillerQuantityScript>("setPainKillerQuantity");

	bi.add<&BattleItem::getStimulantQuantity>("getStimulantQuantity");
	bi.add<&setStimulantQuantityScript>("setStimulantQuantity");

	bi.add<&getActionTUsScript>("getActionCost.getTimeUnits");

	bi.addScriptValue<BindBase::OnlyGet, &BattleItem::_rules, &RuleItem::getScriptValuesRaw>();
	bi.addScriptValue<&BattleItem::_scriptValues>();
	bi.addDebugDisplay<&debugDisplayScript>();

	bi.addCustomConst("BA_AUTOSHOT", BA_AUTOSHOT);
	bi.addCustomConst("BA_SNAPSHOT", BA_SNAPSHOT);
	bi.addCustomConst("BA_AIMEDSHOT", BA_AIMEDSHOT);
	bi.addCustomConst("BA_LAUNCH", BA_LAUNCH);
	bi.addCustomConst("BA_HIT", BA_HIT);
	bi.addCustomConst("BA_USE", BA_USE);
	bi.addCustomConst("BA_THROW", BA_THROW);
	bi.addCustomConst("BA_MINDCONTROL", BA_MINDCONTROL);
	bi.addCustomConst("BA_PANIC", BA_PANIC);
	bi.addCustomConst("BA_PRIME", BA_PRIME);
	bi.addCustomConst("BA_UNPRIME", BA_UNPRIME);
	bi.addCustomConst("BA_NONE", BA_NONE);
	bi.addCustomConst("BA_TRIGGER_TIMED_GRENADE", BA_TRIGGER_TIMED_GRENADE);
	bi.addCustomConst("BA_TRIGGER_PROXY_GRENADE", BA_TRIGGER_PROXY_GRENADE);
}

namespace
{

void commonImpl(BindBase& b, Mod* mod)
{
	b.addCustomPtr<const Mod>("rules", mod);

	b.addCustomConst("blit_item_righthand", BODYPART_ITEM_RIGHTHAND);
	b.addCustomConst("blit_item_lefthand", BODYPART_ITEM_LEFTHAND);
	b.addCustomConst("blit_item_floor", BODYPART_ITEM_FLOOR);
	b.addCustomConst("blit_item_big", BODYPART_ITEM_INVENTORY);
}

}

/**
 * Constructor of recolor script parser.
 */
ModScript::RecolorItemParser::RecolorItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"new_pixel",
	"old_pixel",

	"item", "battle_game", "blit_part", "anim_frame", "shade" }
{
	BindBase b { this };

	commonImpl(b, mod);

	setDefault("add_shade new_pixel shade; return new_pixel;");
}

/**
 * Constructor of select sprite script parser.
 */
ModScript::SelectItemParser::SelectItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"sprite_index",
	"sprite_offset",

	"item", "battle_game", "blit_part", "anim_frame", "shade" }
{
	BindBase b { this };

	commonImpl(b, mod);

	setDefault("add sprite_index sprite_offset; return sprite_index;");
}

ModScript::CreateItemParser::CreateItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "item", "unit", "battle_game", "turn", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::NewTurnItemParser::NewTurnItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "item", "battle_game", "turn", "side", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::TryPsiAttackItemParser::TryPsiAttackItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"psi_attack_success",

	"item",
	"attacker",
	"victim",
	"skill",
	"attack_strength",
	"defense_strength",
	"battle_action",
	"random",
	"distance",
	"distance_strength_reduction",
	"battle_game",
}
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	setDefault("var int r; random.randomRange r 0 55; add psi_attack_success attack_strength; add psi_attack_success r; sub psi_attack_success defense_strength; sub psi_attack_success distance_strength_reduction; return psi_attack_success;");
}

ModScript::TryMeleeAttackItemParser::TryMeleeAttackItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"melee_attack_success",

	"item",
	"attacker",
	"victim",
	"skill",
	"attack_strength",
	"defense_strength",
	"battle_action",
	"random",
	"arc_to_attacker",
	"defense_strength_penalty",
	"battle_game",
}
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	setDefault(
		"var int r;\n"
		"random.randomRange r 0 99;\n"
		"sub melee_attack_success r;\n"
		"add melee_attack_success attack_strength;\n"
		"sub melee_attack_success defense_strength;\n"
		"add melee_attack_success defense_strength_penalty;\n"
		"return melee_attack_success;\n"
	);
}

/**
 * Init all required data in script using object data.
 */
void BattleItem::ScriptFill(ScriptWorkerBlit* w, const BattleItem* item, const SavedBattleGame* save, int part, int anim_frame, int shade)
{
	w->clear();
	if(item)
	{
		const auto &scr = item->getRules()->getScript<ModScript::RecolorItemSprite>();
		if (scr)
		{
			w->update(scr, item, save, part, anim_frame, shade);
		}
		else
		{
			BattleUnit::ScriptFill(w, item->getUnit(), save, part, anim_frame, shade, 0);
		}
	}
}

}
