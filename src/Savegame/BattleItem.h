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
#include <yaml-cpp/yaml.h>
#include "../Mod/RuleItem.h"
#include "../Engine/Script.h"

namespace OpenXcom
{

class RuleItem;
class RuleInventory;
class BattleUnit;
class Tile;
class SurfaceSet;
class Surface;
class ScriptWorkerBlit;
class ScriptParserBase;
class SavedBattleGame;
class RuleItemAction;

enum BattleActionType : Uint8;

/**
 * Represents a single item in the battlescape.
 * Contains battle-related info about an item like the position, ammo quantity, ...
 * @sa RuleItem
 * @sa Item
 */
class BattleItem
{
private:
	int _id;
	RuleItem *_rules;
	BattleUnit *_owner, *_previousOwner;
	BattleUnit *_unit;
	Tile *_tile;
	RuleInventory *_inventorySlot;
	int _inventoryX, _inventoryY;
	BattleItem *_ammoItem[RuleItem::AmmoSlotMax] = { };
	int _fuseTimer, _ammoQuantity;
	int _painKiller, _heal, _stimulant;
	bool _XCOMProperty, _droppedOnAlienTurn, _isAmmo, _isWeaponWithAmmo;
	const RuleItemAction *_confAimedOrLaunch = nullptr;
	const RuleItemAction *_confSnap = nullptr;
	const RuleItemAction *_confAuto = nullptr;
	const RuleItemAction *_confMelee = nullptr;
	ScriptValues<BattleItem> _scriptValues;

public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "BattleItem";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);
	/// Init all required data in script using object data.
	static void ScriptFill(ScriptWorkerBlit* w, BattleItem* item, int part, int anim_frame, int shade);

	/// Creates a item of the specified type.
	BattleItem(RuleItem *rules, int *id);
	/// Cleans up the item.
	~BattleItem();
	/// Loads the item from YAML.
	void load(const YAML::Node& node, const ScriptGlobal *shared);
	/// Saves the item to YAML.
	YAML::Node save(const ScriptGlobal *shared) const;
	/// Gets the item's ruleset.
	const RuleItem *getRules() const;
	/// Gets the item's ammo quantity
	int getAmmoQuantity() const;
	/// Sets the item's ammo quantity.
	void setAmmoQuantity(int qty);
	/// Gets the turn until explosion
	int getFuseTimer() const;
	/// Sets the turns until explosion.
	void setFuseTimer(int turns);
	/// Spend one bullet.
	bool spendBullet();
	/// Gets the item's owner.
	BattleUnit *getOwner();
	/// Gets the item's owner.
	const BattleUnit *getOwner() const;
	/// Gets the item's previous owner.
	BattleUnit *getPreviousOwner();
	/// Gets the item's previous owner.
	const BattleUnit *getPreviousOwner() const;
	/// Sets the owner.
	void setOwner(BattleUnit *owner);
	/// Sets the item's previous owner.
	void setPreviousOwner(BattleUnit *owner);
	/// Removes the item from previous owner and moves to new owner.
	void moveToOwner(BattleUnit *owner);
	/// Gets the item's inventory slot.
	RuleInventory *getSlot() const;
	/// Sets the item's inventory slot.
	void setSlot(RuleInventory *slot);
	/// Gets the item's inventory X position.
	int getSlotX() const;
	/// Sets the item's inventory X position.
	void setSlotX(int x);
	/// Gets the item's inventory Y position.
	int getSlotY() const;
	/// Sets the item's inventory Y position.
	void setSlotY(int y);
	/// Checks if the item is occupying a slot.
	bool occupiesSlot(int x, int y, BattleItem *item = 0) const;
	/// Gets the item's floor sprite.
	Surface *getFloorSprite(SurfaceSet *set) const;
	/// Gets the item's inventory sprite.
	Surface *getBigSprite(SurfaceSet *set) const;
    /// FIXME: Is there a reason for there being two getAmmoItem functions?
	/// Check if item can use any ammo.
	bool isWeaponWithAmmo() const;
	/// Check if weapon is armed.
	bool haveAnyAmmo() const;
	/// Check if weapon have all ammo slot filled.
	bool haveAllAmmo() const;
	/// Sets the item's ammo item based on it type.
	bool setAmmoPreMission(BattleItem *item);
	/// Get ammo slot for action.
	const RuleItemAction *getActionConfNullable(BattleActionType action) const;
	/// Get ammo slot for action.
	const RuleItemAction *getActionConf(BattleActionType action) const;
	/// Get ammo for action.
	const BattleItem *getAmmoForAction(BattleActionType action) const;
	/// Get ammo for action.
	BattleItem *getAmmoForAction(BattleActionType action, std::string* message = nullptr);
	/// Spend weapon ammo.
	void spendAmmoForAction(BattleActionType action, SavedBattleGame *save);
	/// How many auto shots does this weapon fire.
	bool haveNextShotsForAction(BattleActionType action, int shotCount) const;
	/// Determines if this item uses ammo.
	bool needsAmmoForSlot(int slot) const;
	/// Set the item's ammo slot.
	BattleItem *setAmmoForSlot(int slot, BattleItem *item);
	/// Gets the item's ammo item.
	BattleItem *getAmmoForSlot(int slot);
	/// Gets the item's ammo item.
	const BattleItem *getAmmoForSlot(int slot) const;
	/// Get total weight (with ammo).
	int getTotalWeight() const;
	/// Get waypoints count.
	int getCurrentWaypoints() const;

	/// Gets the item's tile.
	Tile *getTile() const;
	/// Sets the tile.
	void setTile(Tile *tile);
	/// Gets it's unique id.
	int getId() const;
	/// Gets the corpse's unit.
	BattleUnit *getUnit();
	/// Gets the corpse's unit.
	const BattleUnit *getUnit() const;
	/// Sets the corpse's unit.
	void setUnit(BattleUnit *unit);
	/// Set medikit Heal quantity
	void setHealQuantity (int heal);
	/// Get medikit heal quantity
	int getHealQuantity() const;
	/// Set medikit pain killers quantity
	void setPainKillerQuantity (int pk);
	/// Get medikit pain killers quantity
	int getPainKillerQuantity() const;
	/// Set medikit stimulant quantity
	void setStimulantQuantity (int stimulant);
	/// Get medikit stimulant quantity
	int getStimulantQuantity() const;
	/// Set xcom property flag
	void setXCOMProperty (bool flag);
	/// Get xcom property flag
	bool getXCOMProperty() const;
	/// get the flag representing "not dropped on player turn"
	bool getTurnFlag() const;
	/// set the flag representing "not dropped on player turn"
	void setTurnFlag(bool flag);
	/// Sets the item's ruleset.
	void convertToCorpse(RuleItem *rules);
	/// Get if item can glow.
	bool getGlow() const;
	/// Gets range of glow in tiles.
	int getGlowRange() const;
	/// Calculate range need to be updated by changing this weapon.
	int getVisibilityUpdateRange() const;
	/// Sets a flag on the item indicating if this is a clip in a weapon or not.
	void setIsAmmo(bool ammo);
	/// Checks a flag on the item to see if it's a clip in a weapon or not.
	bool isAmmo() const;
};

}
