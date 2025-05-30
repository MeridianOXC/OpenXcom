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
#include <sstream>
#include <algorithm>
#include "MapBlock.h"
#include "../Battlescape/Position.h"
#include "../Engine/Exception.h"

namespace OpenXcom
{

/**
 * MapBlock construction.
 */
MapBlock::MapBlock(const std::string &name): _name(name), _size_x(10), _size_y(10), _size_z(4)
{
	_groups.push_back(0);
}

/**
 * MapBlock destruction.
 */
MapBlock::~MapBlock()
{
}

/**
 * Loads the map block from a YAML file.
 * @param node YAML node.
 */
void MapBlock::load(const YAML::YamlNodeReader& reader)
{
	reader.tryRead("name", _name);
	reader.tryRead("width", _size_x);
	reader.tryRead("length", _size_y);
	reader.tryRead("height", _size_z);
	if ((_size_x % 10) != 0 || (_size_y % 10) != 0)
	{
		std::ostringstream ss;
		ss << "Error: MapBlock " << _name << ": Size must be divisible by ten";
		throw Exception(ss.str());
	}
	if (const auto& map = reader["groups"])
	{
		_groups.clear();
		if (map.isSeq())
		{
			map.tryReadVal(_groups);
		}
		else
		{
			_groups.push_back(map.readVal(0));
		}
	}
	if (const auto& map = reader["revealedFloors"])
	{
		_revealedFloors.clear();
		if (map.isSeq())
		{
			map.tryReadVal(_revealedFloors);
		}
		else
		{
			_revealedFloors.push_back(map.readVal(0));
		}
	}
	reader.tryRead("items", _items);
	reader.tryRead("fuseTimers", _itemsFuseTimer);
	reader.tryRead("randomizedItems", _randomizedItems);
	reader.tryRead("extendedItems", _extendedItems);
	reader.tryRead("craftInventoryTile", _craftInventoryTile);
}

/**
 * Gets the MapBlock name (string).
 * @return The name.
 */
const std::string& MapBlock::getName() const
{
	return _name;
}

/**
 * Gets the MapBlock size x.
 * @return The size x in tiles.
 */
int MapBlock::getSizeX() const
{
	return _size_x;
}

/**
 * Gets the MapBlock size y.
 * @return The size y in tiles.
 */
int MapBlock::getSizeY() const
{
	return _size_y;
}

/**
 * Sets the MapBlock size z.
 * @param size_z The size z.
 */
void MapBlock::setSizeZ(int size_z)
{
	_size_z = size_z;
}

/**
 * Gets the MapBlock size z.
 * @return The size z.
 */
int MapBlock::getSizeZ() const
{
	return _size_z;
}

/**
 * Gets the type of mapblock.
 * @return The mapblock's type.
 */
bool MapBlock::isInGroup(int group)
{
	return std::find(_groups.begin(), _groups.end(), group) != _groups.end();
}

/**
 * Gets if this floor should be revealed or not.
 */
bool MapBlock::isFloorRevealed(int floor)
{
	return std::find(_revealedFloors.begin(), _revealedFloors.end(), floor) != _revealedFloors.end();
}

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, RandomizedItems* val)
{
	YAML::YamlNodeReader reader(n);
	reader.tryRead("position", val->position);
	reader.tryRead("amount", val->amount);
	reader.tryRead("fuseTimerMin", val->fuseTimerMin);
	reader.tryRead("fuseTimerMax", val->fuseTimerMax);
	reader.tryRead("mixed", val->mixed);
	reader.tryRead("itemList", val->itemList);
	return true;
}

bool read(ryml::ConstNodeRef const& n, ExtendedItems* val)
{
	YAML::YamlNodeReader reader(n);
	reader.tryRead("type", val->type);
	reader.tryRead("pos", val->pos);
	reader.tryRead("fuseTimerMin", val->fuseTimerMin);
	reader.tryRead("fuseTimerMax", val->fuseTimerMax);
	reader.tryRead("ammoDef", val->ammoDef);
	return true;
}

}
