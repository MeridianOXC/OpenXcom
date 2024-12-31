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
#include <string>
#include <vector>
#include <map>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

struct RuleSlot
{
	int x, y;
};

enum InventoryType { INV_SLOT, INV_HAND, INV_GROUND };

class RuleItem;
class ScriptParserBase;

/**
 * Represents a specific section of the inventory,
 * containing information like available slots and
 * screen position.
 */
class RuleInventory
{
private:
	std::string _id;
	int _x, _y;
	InventoryType _type;
	std::vector<RuleSlot> _slots;
	std::map<std::string, int> _costs;
	int _listOrder;
	int _hand;
public:
	static const int SLOT_W = 16;
	static const int SLOT_H = 16;
	static const int HAND_W = 2;
	static const int HAND_H = 3;
	static const int PAPERDOLL_W = 40;
	static const int PAPERDOLL_H = 70;
	static const int PAPERDOLL_X = 60;
	static const int PAPERDOLL_Y = 65;

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleInventory";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Creates a blank inventory ruleset.
	RuleInventory(const std::string &id, int listOrder);
	/// Cleans up the inventory ruleset.
	~RuleInventory();
	/// Loads inventory data from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the inventory's id.
	const std::string& getId() const;
	/// Gets the X position of the inventory.
	int getX() const;
	/// Gets the Y position of the inventory.
	int getY() const;
	/// Gets the inventory type.
	InventoryType getType() const;
	/// Gets if this slot is right hand;
	bool isRightHand() const;
	/// Gets if this slot is left hand;
	bool isLeftHand() const;
	/// Gets all the slots in the inventory.
	const std::vector<struct RuleSlot> *getSlots() const;
	/// Checks for a slot in a certain position.
	bool checkSlotInPosition(int *x, int *y) const;
	/// Checks if an item fits in a slot.
	bool fitItemInSlot(const RuleItem *item, int x, int y) const;
	/// Gets a certain cost in the inventory.
	int getCost(const RuleInventory *slot) const;
	int getListOrder() const;
};

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, RuleSlot* val);

}
