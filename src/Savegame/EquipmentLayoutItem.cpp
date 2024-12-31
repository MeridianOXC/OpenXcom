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
#include <optional>

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
EquipmentLayoutItem::EquipmentLayoutItem(const YAML::YamlNodeReader& reader, const Mod* mod)
{
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		_ammoItem[slot] = nullptr;
	}
	load(reader, mod);
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
void EquipmentLayoutItem::load(const YAML::YamlNodeReader& reader, const Mod* mod)
{
	_itemType = mod->getItem(reader["itemType"].readVal<std::string>(), true);
	_slot = mod->getInventory(reader["slot"].readVal<std::string>(), true);
	_slotX = reader["slotX"].readVal(0);
	_slotY = reader["slotY"].readVal(0);
	if (const auto& ammoSlots = reader["ammoItemSlots"])
	{
		for (int slot = 0; slot < RuleItem::AmmoSlotMax && ammoSlots[slot]; ++slot)
		{
			auto s = ammoSlots[slot].readVal<std::string>();
			_ammoItem[slot] = s != EmptyPlaceHolder ? mod->getItem(s, true) : nullptr;
		}
	}
	else if (const auto& ammo = reader["ammoItem"])
	{
		_ammoItem[0] = mod->getItem(ammo.readVal<std::string>(), true);
	}
	_fuseTimer = reader["fuseTimer"].readVal(-1);
	_fixed = reader["fixed"].readVal(false);
}

/**
 * Saves the soldier-equipment layout item to a YAML file.
 * @return YAML node.
 */
void EquipmentLayoutItem::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	writer.setFlowStyle();
	writer.write("itemType", _itemType->getType());
	writer.write("slot", _slot->getId());
	// only save this info if it's needed, reduce clutter in saves
	if (_slotX != 0)
		writer.write("slotX", _slotX);
	if (_slotY != 0)
		writer.write("slotY", _slotY);
	if (_ammoItem[0] != nullptr)
		writer.write("ammoItem", _ammoItem[0]->getType());
	std::optional<YAML::YamlNodeWriter> ammoSlotWriter;
	Collections::untilLastIf(
		_ammoItem,
		[](const RuleItem* s)
		{
			return s != nullptr;
		},
		[&](const RuleItem* s)
		{
			if (!ammoSlotWriter.has_value())
			{
				ammoSlotWriter.emplace(writer["ammoItemSlots"]);
				ammoSlotWriter->setAsSeq();
			}
			ammoSlotWriter->write(s ? s->getType() : EmptyPlaceHolder);
		});
	if (_fuseTimer >= 0)
		writer.write("fuseTimer", _fuseTimer);
	if (_fixed)
		writer.write("fixed", _fixed);
}

}
