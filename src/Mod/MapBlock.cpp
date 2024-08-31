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

namespace YAML
{
	template<>
	struct convert<OpenXcom::RandomizedItems>
	{
		static Node encode(const OpenXcom::RandomizedItems& rhs)
		{
			Node node;
			node["position"] = rhs.position;
			node["amount"] = rhs.amount;
			node["mixed"] = rhs.mixed;
			node["itemList"] = rhs.itemList;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::RandomizedItems& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.position = node["position"].as<OpenXcom::Position>(rhs.position);
			rhs.amount = node["amount"].as<int>(rhs.amount);
			rhs.mixed = node["mixed"].as<bool>(rhs.mixed);
			rhs.itemList = node["itemList"].as< std::vector<std::string> >(rhs.itemList);
			return true;
		}
	};
	template<>
	struct convert<OpenXcom::ExtendedItems>
	{
		static Node encode(const OpenXcom::ExtendedItems& rhs)
		{
			Node node;
			node["type"] = rhs.type;
			node["pos"] = rhs.pos;
			node["fuseTimerMin"] = rhs.fuseTimerMin;
			node["fuseTimerMax"] = rhs.fuseTimerMax;
			node["ammoDef"] = rhs.ammoDef;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::ExtendedItems& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.type = node["type"].as<std::string>(rhs.type);
			rhs.pos = node["pos"].as< std::vector<OpenXcom::Position> >(rhs.pos);
			rhs.fuseTimerMin = node["fuseTimerMin"].as<int>(rhs.fuseTimerMin);
			rhs.fuseTimerMax = node["fuseTimerMax"].as<int>(rhs.fuseTimerMax);
			rhs.ammoDef = node["ammoDef"].as< std::vector<std::pair<std::string, int> > >(rhs.ammoDef);
			return true;
		}
	};
}

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
void MapBlock::load(const YAML::Node &node)
{
	_name = node["name"].as<std::string>(_name);
	_size_x = node["width"].as<int>(_size_x);
	_size_y = node["length"].as<int>(_size_y);
	_size_z = node["height"].as<int>(_size_z);
	if ((_size_x % 10) != 0 || (_size_y % 10) != 0)
	{
		std::ostringstream ss;
		ss << "Error: MapBlock " << _name << ": Size must be divisible by ten";
		throw Exception(ss.str());
	}
	if (const YAML::Node &map = node["groups"])
	{
		_groups.clear();
		if (map.Type() == YAML::NodeType::Sequence)
		{
			_groups = map.as<std::vector<int> >(_groups);
		}
		else
		{
			_groups.push_back(map.as<int>(0));
		}
	}
	if (const YAML::Node &map = node["revealedFloors"])
	{
		_revealedFloors.clear();
		if (map.Type() == YAML::NodeType::Sequence)
		{
			_revealedFloors = map.as<std::vector<int> >(_revealedFloors);
		}
		else
		{
			_revealedFloors.push_back(map.as<int>(0));
		}
	}
	_items = node["items"].as<std::map<std::string, std::vector<Position> > >(_items);
	_itemsFuseTimer = node["fuseTimers"].as<std::map<std::string, std::pair<int, int> > >(_itemsFuseTimer);
	_randomizedItems = node["randomizedItems"].as< std::vector<RandomizedItems> >(_randomizedItems);
	_extendedItems = node["extendedItems"].as< std::vector<ExtendedItems> >(_extendedItems);
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

}
