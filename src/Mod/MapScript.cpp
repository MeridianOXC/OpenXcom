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

#include "MapScript.h"
#include "../Engine/Yaml.h"
#include "../Engine/RNG.h"
#include "../Engine/Exception.h"
#include "../Engine/Logger.h"
#include "../Mod/RuleTerrain.h"


namespace OpenXcom
{

MapScript::MapScript() :
	_type(MSC_UNDEFINED), _canBeSkipped(true), _markAsReinforcementsBlock(false),
	_verticalGroup(MT_NSROAD), _horizontalGroup(MT_EWROAD), _crossingGroup(MT_CROSSING),
	_sizeX(1), _sizeY(1), _sizeZ(0),
	_executionChances(100), _executions(1), _cumulativeFrequency(0), _label(0),
	_direction(MD_NONE),
	_tunnelData(0), _randomTerrain(), _verticalLevels()
{
}

MapScript::~MapScript()
{
	for (auto* rect : _rects)
	{
		delete rect;
	}
	delete _tunnelData;
}

/**
 * Loads a map script command from YAML.
 * @param node the YAML node from which to read.
 */
void MapScript::load(const YAML::YamlNodeReader& node)
{
	const auto& reader = node.useIndex();
	std::string command;
	if (const auto& map = reader["type"])
	{
		command = map.readVal<std::string>("");
		if (command == "addBlock")
			_type = MSC_ADDBLOCK;
		else if (command == "addLine")
			_type = MSC_ADDLINE;
		else if (command == "addCraft")
		{
			_type = MSC_ADDCRAFT;
			_groups.push_back(1); // this is a default, and can be overridden
		}
		else if (command == "addUFO")
		{
			_type = MSC_ADDUFO;
			_groups.push_back(1); // this is a default, and can be overridden
		}
		else if (command == "digTunnel")
			_type = MSC_DIGTUNNEL;
		else if (command == "fillArea")
			_type = MSC_FILLAREA;
		else if (command == "checkBlock")
			_type = MSC_CHECKBLOCK;
		else if (command == "removeBlock")
			_type = MSC_REMOVE;
		else if (command == "resize")
		{
			_type = MSC_RESIZE;
			_sizeX = _sizeY = 0; // defaults: don't resize anything unless specified.
		}
		else
		{
			throw Exception("Unknown command: " + command);
		}
	}
	else
	{
		throw Exception("Missing command type.");
	}


	for (const auto& rectReader : reader["rects"].children())
	{
		SDL_Rect* rect = new SDL_Rect();
		rect->x = rectReader[0].readVal<int>();
		rect->y = rectReader[1].readVal<int>();
		rect->w = rectReader[2].readVal<int>();
		rect->h = rectReader[3].readVal<int>();
		_rects.push_back(rect);
	}
	if (const auto& craftGroups = reader["craftGroups"])
	{
		craftGroups.tryReadVal(_craftGroups);
	}
	if (const auto& map = reader["tunnelData"])
	{
		_tunnelData = new TunnelData;
		_tunnelData->level = map["level"].readVal(0);
		for (const auto& mcdReplacement : map["MCDReplacements"].children())
		{
			MCDReplacement replacement;
			std::string type = mcdReplacement["type"].readVal<std::string>("");
			replacement.entry = mcdReplacement["entry"].readVal(-1);
			replacement.set = mcdReplacement["set"].readVal(-1);
			_tunnelData->replacements[type] = replacement;
		}
	}
	if (const auto& map = reader["conditionals"])
	{
		if (map.isSeq())
		{
			map.tryReadVal(_conditionals);
		}
		else
		{
			_conditionals.push_back(map.readVal(0));
		}
	}
	if (const auto& map = reader["size"])
	{
		if (map.isSeq())
		{
			int *sizes[3] = {&_sizeX, &_sizeY, &_sizeZ};
			int entry = 0;
			for (const auto& size : map.children())
			{
				*sizes[entry] = size.readVal(1);
				entry++;
				if (entry == 3)
				{
					break;
				}
			}
		}
		else
		{
			map.tryReadVal(_sizeX);
			_sizeY = _sizeX;
		}
	}

	if (const auto& map = reader["groups"])
	{
		_groups.clear();
		if (map.isSeq())
		{
			for (const auto& group : map.children())
			{
				_groups.push_back(group.readVal(0));
			}
		}
		else
		{
			_groups.push_back(map.readVal(0));
		}
	}
	size_t selectionSize = _groups.size();
	if (const auto& map = reader["blocks"])
	{
		_groups.clear();
		if (map.isSeq())
		{
			for (const auto& block : map.children())
			{
				_blocks.push_back(block.readVal(0));
			}
		}
		else
		{
			_blocks.push_back(map.readVal(0));
		}
		selectionSize = _blocks.size();
	}

	_frequencies.resize(selectionSize, 1);
	_maxUses.resize(selectionSize, -1);

	if (const auto& map = reader["freqs"])
	{
		if (map.isSeq())
		{
			size_t entry = 0;
			for (const auto& freq : map.children())
			{
				if (entry == selectionSize)
					break;
				_frequencies.at(entry) = freq.readVal(1);
				entry++;
			}
		}
		else
		{
			_frequencies.at(0) = map.readVal(1);
		}
	}
	if (const auto& map = reader["maxUses"])
	{
		if (map.isSeq())
		{
			size_t entry = 0;
			for (const auto& maxUse : map.children())
			{
				if (entry == selectionSize)
					break;
				_maxUses.at(entry) = maxUse.readVal(-1);
				entry++;
			}
		}
		else
		{
			_maxUses.at(0) = map.readVal(-1);
		}
	}

	if (const auto& map = reader["direction"])
	{
		std::string direction = map.readVal<std::string>("");
		if (!direction.empty())
		{
			char dir = toupper(direction[0]);
			switch (dir)
			{
			case 'V':
				_direction = MD_VERTICAL;
				break;
			case 'H':
				_direction = MD_HORIZONTAL;
				break;
			case 'B':
				_direction = MD_BOTH;
				break;
			default:
				throw Exception("direction must be [V]ertical, [H]orizontal, or [B]oth, what does " + direction + " mean?");
			}
		}
	}

	if (_direction == MD_NONE)
	{
		if (_type == MSC_DIGTUNNEL || _type == MSC_ADDLINE)
		{
			throw Exception("no direction defined for " + command + " command, must be [V]ertical, [H]orizontal, or [B]oth");
		}
	}


	reader.tryRead("verticalGroup", _verticalGroup);
	reader.tryRead("horizontalGroup", _horizontalGroup);
	reader.tryRead("crossingGroup", _crossingGroup);
	reader.tryRead("canBeSkipped", _canBeSkipped);
	reader.tryRead("markAsReinforcementsBlock", _markAsReinforcementsBlock);
	reader.tryRead("executionChances", _executionChances);
	reader.tryRead("executions", _executions);
	reader.tryRead("UFOName", _ufoName);
	reader.tryRead("craftName", _craftName);
	if (reader["terrain"])
	{
		_randomTerrain.clear();
		_randomTerrain.push_back(reader["terrain"].readVal<std::string>());
	}
	reader.tryRead("randomTerrain", _randomTerrain);
	// take no chances, don't accept negative values here.
	_label = std::abs(reader["label"].readVal(_label));

	// Load any VerticalLevels into a map if we have them
	if (reader["verticalLevels"])
	{
		_verticalLevels.clear();
		for (const auto& levelReader : reader["verticalLevels"].children())
		{
			if (levelReader["type"])
			{
				VerticalLevel level;
				level.load(levelReader);
				_verticalLevels.push_back(level);
			}
		}
	}
}

/**
 * Initializes all the various scratch values and such for the command.
 */
void MapScript::init()
{
	_cumulativeFrequency = 0;
	_blocksTemp.clear();
	_groupsTemp.clear();
	_frequenciesTemp.clear();
	_maxUsesTemp.clear();

	for (int freq : _frequencies)
	{
		_cumulativeFrequency += freq;
	}
	_blocksTemp = _blocks;
	_groupsTemp = _groups;
	_frequenciesTemp = _frequencies;
	_maxUsesTemp = _maxUses;
}

/**
 * Initializes scratch values for working in a vertical level
 */
void MapScript::initVerticalLevel(VerticalLevel level)
{
	_cumulativeFrequency = 0;
	_blocksTemp.clear();
	_groupsTemp.clear();
	_frequenciesTemp.clear();
	_maxUsesTemp.clear();

	_blocks = level.levelBlocks;
	_groups = level.levelGroups;
	_cumulativeFrequency = std::max(_blocks.size(), _groups.size());
	_frequenciesTemp.resize(_cumulativeFrequency, 1);
	_maxUsesTemp.resize(_cumulativeFrequency, -1);
	_blocksTemp = _blocks;
	_groupsTemp = _groups;
}

/**
 * Gets a random group number from the array, accounting for frequencies and max uses.
 * If no groups or blocks are defined, this command will return the default" group,
 * If all the max uses are used up, it will return "undefined".
 * @return Group number.
 */
int MapScript::getGroupNumber()
{
	if (!_groups.size())
	{
		return MT_DEFAULT;
	}
	if (_cumulativeFrequency > 0)
	{
		int pick = RNG::generate(0, _cumulativeFrequency-1);
		for (size_t i = 0; i != _groupsTemp.size(); ++i)
		{
			if (pick < _frequenciesTemp.at(i))
			{
				int retVal = _groupsTemp.at(i);

				if (_maxUsesTemp.at(i) > 0)
				{
					if (--_maxUsesTemp.at(i) == 0)
					{
						_groupsTemp.erase(_groupsTemp.begin() + i);
						_cumulativeFrequency -= _frequenciesTemp.at(i);
						_frequenciesTemp.erase(_frequenciesTemp.begin() + i);
						_maxUsesTemp.erase(_maxUsesTemp.begin() + i);
					}
				}
				return retVal;
			}
			pick -= _frequenciesTemp.at(i);
		}
	}
	return MT_UNDEFINED;
}

/**
 * Gets a random block number from the array, accounting for frequencies and max uses.
 * If no blocks are defined, it will use a group instead.
 * @return Block number.
 */
int MapScript::getBlockNumber()
{
	if (_cumulativeFrequency > 0)
	{
		int pick = RNG::generate(0, _cumulativeFrequency-1);
		for (size_t i = 0; i != _blocksTemp.size(); ++i)
		{
			if (pick < _frequenciesTemp.at(i))
			{
				int retVal = _blocksTemp.at(i);

				if (_maxUsesTemp.at(i) > 0)
				{
					if (--_maxUsesTemp.at(i) == 0)
					{
						_blocksTemp.erase(_blocksTemp.begin() + i);
						_cumulativeFrequency -= _frequenciesTemp.at(i);
						_frequenciesTemp.erase(_frequenciesTemp.begin() + i);
						_maxUsesTemp.erase(_maxUsesTemp.begin() + i);
					}
				}
				return retVal;
			}
			pick -= _frequenciesTemp.at(i);
		}
	}
	return MT_UNDEFINED;
}

/**
 * Gets a random map block from a given terrain, using either the groups or the blocks defined.
 * @param terrain the terrain to pick a block from.
 * @return Pointer to a randomly chosen map block, given the options available.
 */
MapBlock *MapScript::getNextBlock(RuleTerrain *terrain)
{
	if (_blocks.empty())
	{
		return terrain->getRandomMapBlock(_sizeX * 10, _sizeY * 10, getGroupNumber());
	}
	int result = getBlockNumber();
	if (result < (int)(terrain->getMapBlocks()->size()) && result != MT_UNDEFINED)
	{
		return terrain->getMapBlocks()->at((size_t)(result));
	}
	return 0;
}

/**
 * Gets the name of the UFO in the case of "setUFO"
 * @return the UFO name.
 */
std::string MapScript::getUFOName() const
{
	return _ufoName;
}

/**
* Gets the name of the craft in the case of "addCraft"
* @return the craft name.
*/
std::string MapScript::getCraftName()
{
	return _craftName;
}

/**
 * Gets the alternate terrain list for this command.
 * @return the vector of terrain names.
 */
const std::vector<std::string> &MapScript::getRandomAlternateTerrain() const
{
	return _randomTerrain;
}

/**
 * Gets the vertical levels defined in a map script command
 * @return the vector of VerticalLevels
 */
const std::vector<VerticalLevel> &MapScript::getVerticalLevels() const
{
	return _verticalLevels;
}

/**
 * For use only with base defense maps as a special case,
 * set _verticalLevels directly for a new MapScript
 * @param verticalLevels the vector of VerticalLevels
 * @param sizeX the X-size of the facility whose VerticalLevels are being loaded
 * @param sizeY the Y-size of the facility whose VerticalLevels are being loaded
 */
void MapScript::setVerticalLevels(const std::vector<VerticalLevel> &verticalLevels, int sizeX, int sizeY)
{
	_verticalLevels = verticalLevels;
	_sizeX = sizeX;
	_sizeY = sizeY;
}

}
