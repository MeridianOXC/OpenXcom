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
#include "RuleInventory.h"
#include <cmath>
#include "RuleItem.h"
#include "../Engine/Screen.h"
#include "../Engine/ScriptBind.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain
 * type of inventory section.
 * @param id String defining the id.
 */
RuleInventory::RuleInventory(const std::string &id, int listOrder): _id(id), _x(0), _y(0), _type(INV_SLOT), _listOrder(listOrder), _hand(0)
{
}

RuleInventory::~RuleInventory()
{
}

/**
 * Loads the inventory from a YAML file.
 * @param node YAML node.
 * @param listOrder The list weight for this inventory.
 */
void RuleInventory::load(const YAML::YamlNodeReader& reader)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("x", _x);
	reader.tryRead("y", _y);
	reader.tryRead("type", _type);
	reader.tryRead("slots", _slots);
	reader.tryRead("costs", _costs);
	reader.tryRead("listOrder", _listOrder);
	if (_id == "STR_RIGHT_HAND")
	{
		_hand = 2;
	}
	else if (_id == "STR_LEFT_HAND")
	{
		_hand = 1;
	}
	else
	{
		_hand = 0;
	}
}

/**
 * Gets the language string that names
 * this inventory section. Each section has a unique name.
 * @return The section name.
 */
const std::string& RuleInventory::getId() const
{
	return _id;
}

/**
 * Gets the X position of the inventory section on the screen.
 * @return The X position in pixels.
 */
int RuleInventory::getX() const
{
	return _x;
}

/**
 * Gets the Y position of the inventory section on the screen.
 * @return The Y position in pixels.
 */
int RuleInventory::getY() const
{
	return _y;
}

/**
 * Gets the type of the inventory section.
 * Slot-based contain a limited number of slots.
 * Hands only contain one slot but can hold any item.
 * Ground can hold infinite items but don't attach to soldiers.
 * @return The inventory type.
 */
InventoryType RuleInventory::getType() const
{
	return _type;
}

/**
 * Gets if this slot is right hand.
 * @return This is right hand.
 */
bool RuleInventory::isRightHand() const
{
	return _hand == 2;
}

/**
 * Gets if this slot is left hand.
 * @return This is wrong hand.
 */
bool RuleInventory::isLeftHand() const
{
	return _hand == 1;
}

/**
 * Gets all the slots in the inventory section.
 * @return The list of slots.
 */
const std::vector<struct RuleSlot> *RuleInventory::getSlots() const
{
	return &_slots;
}

/**
 * Gets the slot located in the specified mouse position.
 * @param x Mouse X position. Returns the slot's X position.
 * @param y Mouse Y position. Returns the slot's Y position.
 * @return True if there's a slot there.
 */
bool RuleInventory::checkSlotInPosition(int *x, int *y) const
{
	int mouseX = *x, mouseY = *y;
	if (_type == INV_HAND)
	{
		for (int xx = 0; xx < HAND_W; ++xx)
		{
			for (int yy = 0; yy < HAND_H; ++yy)
			{
				if (mouseX >= _x + xx * SLOT_W && mouseX < _x + (xx + 1) * SLOT_W &&
					mouseY >= _y + yy * SLOT_H && mouseY < _y + (yy + 1) * SLOT_H)
				{
					*x = 0;
					*y = 0;
					return true;
				}
			}
		}
	}
	else if (_type == INV_GROUND)
	{
		if (mouseX >= _x && mouseX < Screen::ORIGINAL_WIDTH && mouseY >= _y && mouseY < Screen::ORIGINAL_HEIGHT)
		{
			*x = (int)floor(double(mouseX - _x) / SLOT_W);
			*y = (int)floor(double(mouseY - _y) / SLOT_H);
			return true;
		}
	}
	else
	{
		for (const auto& coord : _slots)
		{
			if (mouseX >= _x + coord.x * SLOT_W && mouseX < _x + (coord.x + 1) * SLOT_W &&
				mouseY >= _y + coord.y * SLOT_H && mouseY < _y + (coord.y + 1) * SLOT_H)
			{
				*x = coord.x;
				*y = coord.y;
				return true;
			}
		}
	}
	return false;
}

/**
 * Checks if an item completely fits when
 * placed in a certain slot.
 * @param item Pointer to item ruleset.
 * @param x Slot X position.
 * @param y Slot Y position.
 * @return True if there's a slot there.
 */
bool RuleInventory::fitItemInSlot(const RuleItem *item, int x, int y) const
{
	if (_type == INV_HAND)
	{
		return true;
	}
	else if (_type == INV_GROUND)
	{
		int width = (320 - _x) / SLOT_W;
		int height = (200 - _y) / SLOT_H;
		int xOffset = 0;
		while (x >= xOffset + width)
			xOffset += width;
		for (int xx = x; xx < x + item->getInventoryWidth(); ++xx)
		{
			for (int yy = y; yy < y + item->getInventoryHeight(); ++yy)
			{
				if (!(xx >= xOffset && xx < xOffset + width && yy >= 0 && yy < height))
					return false;
			}
		}
		return true;
	}
	else
	{
		int totalSlots = item->getInventoryWidth() * item->getInventoryHeight();
		int foundSlots = 0;
		for (const auto& coord : _slots)
		{
			if (foundSlots >= totalSlots)
			{
				break; // loop finished
			}
			if (coord.x >= x && coord.x < x + item->getInventoryWidth() &&
				coord.y >= y && coord.y < y + item->getInventoryHeight())
			{
				foundSlots++;
			}
		}
		return (foundSlots == totalSlots);
	}
}

/**
 * Gets the time unit cost to place an item in another section.
 * @param slot The new section id.
 * @return The time unit cost.
 */
int RuleInventory::getCost(const RuleInventory* slot) const
{
	if (slot == this)
		return 0;
	return _costs.find(slot->getId())->second;
}

int RuleInventory::getListOrder() const
{
	return _listOrder;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

void getIdScript(const RuleInventory* bu, ScriptText& txt)
{
	if (bu)
	{
		txt = { bu->getId().c_str() };
		return;
	}
	else
	{
		txt = ScriptText::empty;
	}
}

void getTypeScript(const RuleInventory* bu, int& type)
{
	if (bu)
	{
		type = (int)bu->getType();
	}
	else
	{
		type = 0;
	}
}

std::string debugDisplayScript(const RuleInventory* bu)
{
	if (bu)
	{
		std::string s;
		s += RuleInventory::ScriptName;
		s += "(id: \"";
		s += bu->getId();
		s += "\")";
		return s;
	}
	else
	{
		return "null";
	}
}

} // namespace



/**
 * Register RuleInventory in script parser.
 * @param parser Script parser.
 */
void RuleInventory::ScriptRegister(ScriptParserBase* parser)
{
	Bind<RuleInventory> bu = { parser };

	bu.add<&getIdScript>("getId");
	bu.add<&getTypeScript>("getType");
	bu.add<&RuleInventory::isRightHand>("isRightHand");
	bu.add<&RuleInventory::isLeftHand>("isLeftHand");
	bu.add<&RuleInventory::getCost>("getMoveToCost", "Cost of moving item from slot in first arg to slot from last arg");

	bu.addDebugDisplay<&debugDisplayScript>();

	bu.addCustomConst("INV_GROUND", INV_GROUND);
	bu.addCustomConst("INV_SLOT", INV_SLOT);
	bu.addCustomConst("INV_HAND", INV_HAND);
}

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, RuleSlot* val)
{
	YAML::YamlNodeReader reader(n);
	val->x = reader[0].readVal<int>();
	val->y = reader[1].readVal<int>();
	return true;
}

}
