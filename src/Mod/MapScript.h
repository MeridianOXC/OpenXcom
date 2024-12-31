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
#include <vector>
#include <string>
#include "../Engine/Yaml.h"
#include <SDL_video.h>
#include "MapBlock.h"
#include "../Engine/Logger.h"

namespace OpenXcom
{

enum MapDirection {MD_NONE, MD_VERTICAL, MD_HORIZONTAL, MD_BOTH};
struct MCDReplacement {	int set, entry;};
struct TunnelData
 {
	std::map<std::string, MCDReplacement> replacements;
	int level;
	TunnelData() : level(0) { }
	MCDReplacement *getMCDReplacement(const std::string& type)
	{
		if (replacements.find(type) == replacements.end())
		{
			return 0;
		}

		return &replacements[type];
	}
 };
enum MapScriptCommand {MSC_UNDEFINED = -1, MSC_ADDBLOCK, MSC_ADDLINE, MSC_ADDCRAFT, MSC_ADDUFO, MSC_DIGTUNNEL, MSC_FILLAREA, MSC_CHECKBLOCK, MSC_REMOVE, MSC_RESIZE};

class MapBlock;
class RuleTerrain;

// Structure for containing multiple levels of map blocks inside a command
enum VerticalLevelType {VLT_GROUND, VLT_MIDDLE, VLT_CEILING, VLT_EMPTY, VLT_DECORATION, VLT_CRAFT, VLT_LINE};
struct VerticalLevel
{
	VerticalLevelType levelType;
	std::vector<int> levelGroups, levelBlocks;
	int levelSizeX, levelSizeY, levelSizeZ;
	int maxRepeats;
	std::string levelTerrain;

	// Default constructor
	VerticalLevel() :
		levelType(VLT_MIDDLE), levelSizeX(1), levelSizeY(1), levelSizeZ(-1), maxRepeats(-1), levelTerrain("")
	{

	}

	// Load in the data for a VerticalLevel from a YAML file, has to load similar data to a full mapscript command
	void load(const YAML::YamlNodeReader& reader)
	{
		std::string type = reader["type"].readVal<std::string>("");
		if (type == "ground")
		{
			levelType = VLT_GROUND;
		}
		else if (type == "middle")
		{
			levelType = VLT_MIDDLE;
		}
		else if (type == "ceiling")
		{
			levelType = VLT_CEILING;
		}
		else if (type == "empty")
		{
			levelType = VLT_EMPTY;
		}
		else if (type == "decoration")
		{
			levelType = VLT_DECORATION;
			levelSizeZ = 0;
		}
		else if (type == "craft")
		{
			levelType = VLT_CRAFT;
		}
		else if (type == "line")
		{
			levelType = VLT_LINE;
		}
		else
		{
			Log(LOG_WARNING) << "'" << type << "'" << " does not resolve into a valid verticalLevel type, loading as 'middle'.";
			levelType = VLT_MIDDLE;
		}

		if (const auto& map = reader["size"])
		{
			if (map.isSeq())
			{
				int *sizes[3] = {&levelSizeX, &levelSizeY, &levelSizeZ};
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
				map.tryReadVal(levelSizeX);
				levelSizeY = levelSizeX;
			}
		}

		reader.tryRead("maxRepeats", maxRepeats);

		if (const auto& map = reader["groups"])
		{
			levelGroups.clear();
			if (map.isSeq())
			{
				for (const auto& group : map.children())
				{
					levelGroups.push_back(group.readVal(0));
				}
			}
			else
			{
				levelGroups.push_back(map.readVal(0));
			}
		}

		if (const auto& map = reader["blocks"])
		{
			levelGroups.clear();
			if (map.isSeq())
			{
				for (const auto& block : map.children())
				{
					levelBlocks.push_back(block.readVal(0));
				}
			}
			else
			{
				levelBlocks.push_back(map.readVal(0));
			}

		}

		reader.tryRead("terrain", levelTerrain);
	}
};

class MapScript
{
private:
	MapScriptCommand _type;
	bool _canBeSkipped, _markAsReinforcementsBlock;
	std::vector<SDL_Rect*> _rects;
	std::vector<int> _craftGroups;
	std::vector<int> _groups, _blocks, _frequencies, _maxUses, _conditionals;
	int _verticalGroup, _horizontalGroup, _crossingGroup;
	std::vector<int> _groupsTemp, _blocksTemp, _frequenciesTemp, _maxUsesTemp;
	int _sizeX, _sizeY, _sizeZ, _executionChances, _executions, _cumulativeFrequency, _label;
	MapDirection _direction;
	TunnelData *_tunnelData;
	std::string _ufoName, _craftName;
	std::vector<std::string> _randomTerrain;
	std::vector<VerticalLevel> _verticalLevels;

	/// Randomly generate a group from within the array.
	int getGroupNumber();
	/// Randomly generate a block number from within the array.
	int getBlockNumber();
public:
	MapScript();
	~MapScript();
	/// Loads information from a ruleset file.
	void load(const YAML::YamlNodeReader& reader);
	/// Initializes all the variables and junk for a mapscript command.
	void init();
	/// Initializes the variables for a mapscript command from a VerticalLevel
	void initVerticalLevel(VerticalLevel level);
	/// Gets what type of command this is.
	MapScriptCommand getType() const {return _type;};
	/// Can this command be skipped if unsuccessful?
	bool canBeSkipped() const { return _canBeSkipped; };
	/// Should blocks added by this command be used as reinforcements blocks?
	bool markAsReinforcementsBlock() const { return _markAsReinforcementsBlock; }
	/// Gets the rects, describing the areas this command applies to.
	const std::vector<SDL_Rect*> *getRects() const {return &_rects;};
	/// Gets the craft groups vector for iteration.
	const std::vector<int>& getCraftGroups() const { return _craftGroups; };
	/// Gets the X size for this command.
	int getSizeX() const {return _sizeX;};
	/// Gets the Y size for this command.
	int getSizeY() const {return _sizeY;};
	/// Gets the Z size for this command.
	int getSizeZ() const {return _sizeZ;};
	/// Get the chances of this command executing.
	int getChancesOfExecution() const {return _executionChances;};
	/// Gets the label for this command.
	int getLabel() const {return _label;};
	/// Gets how many times this command repeats (1 repeat means 2 executions)
	int getExecutions() const {return _executions;};
	/// Gets what conditions apply to this command.
	const std::vector<int> *getConditionals() const {return &_conditionals;};
	/// Gets the groups vector for iteration.
	const std::vector<int> *getGroups() const {return &_groups;};
	/// Gets the blocks vector for iteration.
	const std::vector<int> *getBlocks() const {return &_blocks;};
	/// Gets the verticalGroup for this command.
	int getVerticalGroup() const { return _verticalGroup; };
	/// Gets the horizontalGroup for this command.
	int getHorizontalGroup() const { return _horizontalGroup; };
	/// Gets the crossingGroup for this command.
	int getCrossingGroup() const { return _crossingGroup; };
	/// Gets the direction this command goes (for lines and tunnels).
	MapDirection getDirection() const {return _direction;};
	/// Gets the mcd replacement data for tunnel replacements.
	TunnelData *getTunnelData() {return _tunnelData;};
	/// Randomly generate a block from within either the array of groups or blocks.
	MapBlock *getNextBlock(RuleTerrain *terrain);
	/// Gets the UFO's name (for setUFO)
	std::string getUFOName() const;
	/// Gets the craft's name (for addCraft)
	std::string getCraftName();
	/// Gets the alternate terrain list for this command.
	const std::vector<std::string> &getRandomAlternateTerrain() const;
	/// Gets the vertical levels for a command
	const std::vector<VerticalLevel> &getVerticalLevels() const;
	/// Sets the vertical levels for a command from a base facility's vertical levels
	void setVerticalLevels(const std::vector<VerticalLevel> &verticalLevels, int sizeX, int sizeY);
};

}
