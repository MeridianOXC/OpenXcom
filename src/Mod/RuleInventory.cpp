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
#include "../Engine/Exception.h"
#include "../Engine/InteractiveSurface.h"

namespace YAML
{
	template<>
	struct convert<OpenXcom::RuleSlot>
	{
		static Node encode(const OpenXcom::RuleSlot& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, OpenXcom::RuleSlot& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<int>();
			rhs.y = node[1].as<int>();
			return true;
		}
	};
}

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain
 * type of inventory section.
 * @param id String defining the id.
 */
RuleInventory::RuleInventory(const std::string &id): _id(id), _x(0), _y(0), _type(INV_SLOT), _listOrder(0), _hand(0), _handWidth(0), _handHeight(0)
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
void RuleInventory::load(const YAML::Node &node, int listOrder)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, listOrder);
	}
	_id = node["id"].as<std::string>(_id);
	_x = node["x"].as<int>(_x);
	_y = node["y"].as<int>(_y);
	_type = (InventoryType)node["type"].as<int>(_type);
	_slots = node["slots"].as< std::vector<RuleSlot> >(_slots);
	_costs = node["costs"].as< std::map<std::string, int> >(_costs);
	_listOrder = node["listOrder"].as<int>(listOrder);
	bool mainHand = node["mainHand"].as<bool>(false), offHand = node["offHand"].as<bool>(false);
	//coherency checks - error if redefining default hands as something else than hands
	//or new hands without properly qualyfying them as such
	if ((_type == INV_HAND) && (_id != "STR_RIGHT_HAND" && _id != "STR_LEFT_HAND" && !mainHand && !offHand ||
		_id == "STR_RIGHT_HAND" && offHand || _id == "STR_LEFT_HAND" && mainHand ||
		mainHand && offHand) || (_type != INV_HAND) && (mainHand || offHand || _id == "STR_RIGHT_HAND" || _id == "STR_LEFT_HAND"))
		throw Exception("Wrong inventory " + _id + " (re)definition");
	// no another ground definition (no real use, anyways)!!!
	if(_type == INV_GROUND && _id != "STR_GROUND")
		throw Exception("New ground " + _id + " definition");
	if (_id == "STR_RIGHT_HAND" || mainHand)
		_hand = 2;
	else if (_id == "STR_LEFT_HAND" || offHand)
		_hand = 1;
	else
		_hand = 0;
}

/**
 * Gets the language string that names
 * this inventory section. Each section has a unique name.
 * @return The section name.
 */
std::string RuleInventory::getId() const
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
	if (_type == INV_GROUND)
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
		for (std::vector<RuleSlot>::const_iterator i = _slots.begin(); i != _slots.end(); ++i)
		{
			if (mouseX >= _x + i->x * SLOT_W && mouseX < _x + (i->x + 1) * SLOT_W &&
				mouseY >= _y + i->y * SLOT_H && mouseY < _y + (i->y + 1) * SLOT_H)
			{
				if (_type == INV_HAND)
				{
					*x = 0;
					*y = 0;
				}
				else
				{
					*x = i->x;
					*y = i->y;
				}
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
 * @return <0 if not fit or
 *         0 if there's a slot there, or a sum of the following.
 *         +1 * n, n < MAX_HAND_W, in order to horizontally align item
 *         +MAX_HAND_W * m, m < MAX_HAND_H to vertically align item
 */
int RuleInventory::fitItemInSlot(const RuleItem *item, int x, int y) const
{
	if (_type == INV_GROUND)
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
					return -1;
			}
		}
		return 0;
	}
	else if (_type == INV_SLOT)
	{
		int totalSlots = item->getInventoryWidth() * item->getInventoryHeight();
		int foundSlots = 0;
		for (std::vector<RuleSlot>::const_iterator i = _slots.begin(); i != _slots.end() && foundSlots < totalSlots; ++i)
		{
			if (i->x >= x && i->x < x + item->getInventoryWidth() &&
				i->y >= y && i->y < y + item->getInventoryHeight())
			{
				foundSlots++;
			}
		}
		return foundSlots - totalSlots;
	}
	else if (_type == INV_HAND)
	{
		if (item->getInventoryWidth() > _handWidth || item->getInventoryHeight() > _handHeight)
			return -1;
		return _fit[item->getInventoryWidth() - 1][item->getInventoryHeight() - 1];
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
	std::map<std::string, int>::const_iterator i = _costs.find(slot->getId());
	if (i == _costs.cend())
		return -1;
	return i->second;
}

void RuleInventory::draw(Surface* grid_, int color_) const
{
	// Draw grid
	if (_type == INV_GROUND)
	{
		for (int x = _x; x <= 320; x += SLOT_W)
		{
			for (int y = _y; y <= 200; y += SLOT_H)
			{
				SDL_Rect r;
				r.x = x;
				r.y = y;
				r.w = SLOT_W + 1;
				r.h = SLOT_H + 1;
				grid_->drawRect(&r, color_);
				r.x++;
				r.y++;
				r.w -= 2;
				r.h -= 2;
				grid_->drawRect(&r, 0);
			}
		}
	}
	else
	{
		for (auto j : _slots)
		{
			SDL_Rect r;
			r.x = _x + SLOT_W * j.x;
			r.y = _y + SLOT_H * j.y;
			r.w = SLOT_W + 1;
			r.h = SLOT_H + 1;
			grid_->drawRect(&r, color_);
			if (!(j.adj & 1)) r.x++;
			if (!(j.adj & 2)) r.y++;
			if (!(j.adj & 4)) r.w -= ((j.adj & 1) ? 1 : 2);
			if (!(j.adj & 8)) r.h -= ((j.adj & 2) ? 1 : 2);
			grid_->drawRect(&r, 0);
		}
	}
}

void RuleInventory::afterLoad(const Mod* mod_)
{
	if (_hand)
	{
		//compute the adjacency for all hand slots
		for (std::vector<RuleSlot>::iterator i = _slots.begin(); i != _slots.end();i++)
		{
			if (_handWidth < i->x) _handWidth = i->x;
			if (_handHeight < i->y) _handHeight = i->y;
			for (std::vector<RuleSlot>::iterator j = i; j != _slots.end(); j++)
			{
				if (i == j) continue;
				if (i->x == j->x)
				{
					if (i->y == j->y + 1)
					{
						i->adj |= 2;
						j->adj |= 8;
					}
					else if(i->y + 1 == j->y)
					{
						i->adj |= 8;
						j->adj |= 2;
					}
				}
				else if (i->y == j->y)
				{
					if (i->x == j->x + 1)
					{
						i->adj |= 1;
						j->adj |= 4;
					}
					else if (i->x + 1 == j->x)
					{
						i->adj |= 4;
						j->adj |= 1;
					}
				}
			}
		}
		_handWidth++; _handHeight++;
		//now fill the _fit matrix
		for(int i = 0;i < MAX_HAND_W; i++)
			for (int j = 0; j < MAX_HAND_H; j++)
			{
				int maxhfit = -1, maxvfit = -1, minhfit = -1, minvfit = -1;
				for(int x = 0; x < _handWidth; x++)
					for (int y = 0; y < _handHeight; y++)
					{
						int totalSlots = (i+1) * (j+1);
						int foundSlots = 0;
						for (std::vector<RuleSlot>::const_iterator s = _slots.begin(); s != _slots.end() && foundSlots < totalSlots; ++s)
						{
							if (s->x >= x && s->x < x + i + 1 &&
								s->y >= y && s->y < y + j + 1)
							{
								foundSlots++;
							}
						}
						if (foundSlots == totalSlots)
						{
							if (maxhfit < x) maxhfit = x;
							if (minhfit < 0 || minhfit > x) minhfit = x;
							if (maxvfit < y) maxvfit = y;
							if (minvfit < 0 || minvfit > y) minvfit = y;
						}
					}
				_fit[i][j] = (maxhfit + minhfit) * SLOT_W / 2 + (MAX_HAND_W * SLOT_W) * ((maxvfit + minvfit) * SLOT_H / 2);
			}
	}
}

}
