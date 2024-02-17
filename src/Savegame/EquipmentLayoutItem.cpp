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
#include "EquipmentLayoutItem.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/Mod.h"
#include "../Engine/Collections.h"
#include "BattleItem.h"

namespace OpenXcom
{

/**
 * Value used for save backward and forward compatibility. Represent empty slot.
 */
const std::string EmptyPlaceHolder = "NONE";

/**
 * Initializes a new soldier-equipment layout item from YAML.
 * @param node YAML node.
 */
EquipmentLayoutItem::EquipmentLayoutItem(const YAML::Node &node, const Mod* mod)
{
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		_ammoItem[slot] = nullptr;
	}
	load(node, mod);
}

/**
 * Initializes a new soldier-equipment layout item.
 * @param itemType Item's type.
 * @param slot Occupied slot's id.
 * @param slotX Position-X in the occupied slot.
 * @param slotY Position-Y in the occupied slot.
 * @param ammoItem The ammo has to be loaded into the item. (it's type)
 * @param fuseTimer The turn until explosion of the item. (if it's an activated grenade-type)
 */
EquipmentLayoutItem::EquipmentLayoutItem(const BattleItem* item) :
	_itemType(item->getRules()),
	_slot(item->getSlot()),
	_slotX(item->getSlotX()), _slotY(item->getSlotY()),
	_ammoItem{}, _fuseTimer(item->getFuseTimer()),
	_fixed(item->getRules()->isFixed())
{
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		if (item->needsAmmoForSlot(slot) && item->getAmmoForSlot(slot))
		{
			_ammoItem[slot] = item->getAmmoForSlot(slot)->getRules();
		}
		else
		{
			_ammoItem[slot] = nullptr;
		}
	}
}

/**
 *
 */
EquipmentLayoutItem::~EquipmentLayoutItem()
{
}

/**
 * Returns the item's type which has to be in a slot.
 * @return item type.
 */
const RuleItem* EquipmentLayoutItem::getItemType() const
{
	return _itemType;
}

/**
 * Returns the slot to be occupied.
 * @return slot name.
 */
const RuleInventory* EquipmentLayoutItem::getSlot() const
{
	return _slot;
}

/**
 * Returns the position-X in the slot to be occupied.
 * @return slot-X.
 */
int EquipmentLayoutItem::getSlotX() const
{
	return _slotX;
}

/**
 * Returns the position-Y in the slot to be occupied.
 * @return slot-Y.
 */
int EquipmentLayoutItem::getSlotY() const
{
	return _slotY;
}

/**
 * Returns the ammo has to be loaded into the item.
 * @return ammo type.
 */
const RuleItem* EquipmentLayoutItem::getAmmoItemForSlot(int slot) const
{
	return _ammoItem[slot];
}

/**
 * Returns the turn until explosion of the item. (if it's an activated grenade-type)
 * @return turn count.
 */
int EquipmentLayoutItem::getFuseTimer() const
{
	return _fuseTimer;
}

/**
 * Is this a fixed weapon entry?
 * @return True, if this is a fixed weapon entry.
 */
bool EquipmentLayoutItem::isFixed() const
{
	return _fixed;
}

/**
 * Loads the soldier-equipment layout item from a YAML file.
 * @param node YAML node.
 */
void EquipmentLayoutItem::load(const YAML::Node &node, const Mod* mod)
{
	_itemType = mod->getItem(node["itemType"].as<std::string>(), true);
	_slot = mod->getInventory(node["slot"].as<std::string>(), true);
	_slotX = node["slotX"].as<int>(0);
	_slotY = node["slotY"].as<int>(0);
	if (const YAML::Node &ammo = node["ammoItem"])
	{
		_ammoItem[0] = mod->getItem(ammo.as<std::string>(), true);
	}
	if (const YAML::Node &ammoSlots = node["ammoItemSlots"])
	{
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			if (ammoSlots[slot])
			{
				auto s = ammoSlots[slot].as<std::string>();
				_ammoItem[slot] = s != EmptyPlaceHolder ? mod->getItem(s, true) : nullptr;
			}
		}
	}
	_fuseTimer = node["fuseTimer"].as<int>(-1);
	_fixed = node["fixed"].as<bool>(false);
}

/**
 * Saves the soldier-equipment layout item to a YAML file.
 * @return YAML node.
 */
YAML::Node EquipmentLayoutItem::save() const
{
	YAML::Node node;
	node.SetStyle(YAML::EmitterStyle::Flow);
	node["itemType"] = _itemType->getType();
	node["slot"] = _slot->getId();
	// only save this info if it's needed, reduce clutter in saves
	if (_slotX != 0)
	{
		node["slotX"] = _slotX;
	}
	if (_slotY != 0)
	{
		node["slotY"] = _slotY;
	}
	if (_ammoItem[0] != nullptr)
	{
		node["ammoItem"] = _ammoItem[0]->getType();
	}
	Collections::untilLastIf(
		_ammoItem,
		[](const RuleItem* s)
		{
			return s != nullptr;
		},
		[&](const RuleItem* s)
		{
			node["ammoItemSlots"].push_back(s ? s->getType() : EmptyPlaceHolder);
		}
	);
	if (_fuseTimer >= 0)
	{
		node["fuseTimer"] = _fuseTimer;
	}
	if (_fixed)
	{
		node["fixed"] = _fixed;
	}
	return node;
}

}
