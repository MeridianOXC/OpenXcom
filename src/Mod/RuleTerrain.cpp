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

#include "RuleTerrain.h"
#include "MapBlock.h"
#include "MapDataSet.h"
#include "MapScript.h"
#include "../Engine/RNG.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * RuleTerrain construction.
 */
RuleTerrain::RuleTerrain(const std::string &name) : _name(name), _mapScript("DEFAULT"), _minDepth(0), _maxDepth(0),
	_ambience(-1), _ambientVolume(0.5), _minAmbienceRandomDelay(20), _maxAmbienceRandomDelay(60),
	_lastCraftSkinIndex(0)
{
	_civilianTypes.push_back("MALE_CIVILIAN");
	_civilianTypes.push_back("FEMALE_CIVILIAN");
}

/**
 * RuleTerrain only holds mapblocks. Map datafiles are referenced.
 */
RuleTerrain::~RuleTerrain()
{
	for (auto* mapblock : _mapBlocks)
	{
		delete mapblock;
	}
}

/**
 * Loads the terrain from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the terrain.
 */
void RuleTerrain::load(const YAML::YamlNodeReader& node, Mod *mod)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	bool adding = reader["addOnly"].readVal(false);
	if (const auto& map = reader["mapDataSets"])
	{
		_mapDataSets.clear();
		for (const auto& mapDataSet : map.children())
		{
			_mapDataSets.push_back(mod->getMapDataSet(mapDataSet.readVal<std::string>()));
		}
	}
	if (const auto& map = reader["mapBlocks"])
	{
		if (!adding)
		{
			Collections::deleteAll(_mapBlocks);
		}
		for (const auto& mapBlockReader : map.children())
		{
			MapBlock *mapBlock = new MapBlock(mapBlockReader["name"].readVal<std::string>());
			mapBlock->load(mapBlockReader);
			_mapBlocks.push_back(mapBlock);
		}
	}

	reader.tryRead("enviroEffects", _enviroEffects);
	mod->loadUnorderedNames(_name, _civilianTypes, reader["civilianTypes"]);
	mod->loadUnorderedNames(_name, _music, reader["music"]);
	if (reader["depth"])
	{
		reader["depth"][0].tryReadVal(_minDepth);
		reader["depth"][1].tryReadVal(_maxDepth);
	}
	mod->loadSoundOffset(_name, _ambience, reader["ambience"], "BATTLE.CAT");
	reader.tryRead("ambientVolume", _ambientVolume);
	mod->loadSoundOffset(_name, _ambienceRandom, reader["ambienceRandom"], "BATTLE.CAT");
	if (reader["ambienceRandomDelay"])
	{
		reader["ambienceRandomDelay"][0].tryReadVal(_minAmbienceRandomDelay);
		reader["ambienceRandomDelay"][1].tryReadVal(_maxAmbienceRandomDelay);
	}
	reader.tryRead("script", _mapScript);
	reader.tryRead("mapScripts", _mapScripts);
}

/**
 * Gets the array of mapblocks.
 * @return Pointer to the array of mapblocks.
 */
std::vector<MapBlock*> *RuleTerrain::getMapBlocks()
{
	return &_mapBlocks;
}

/**
 * Gets the array of mapdatafiles.
 * @return Pointer to the array of mapdatafiles.
 */
std::vector<MapDataSet*> *RuleTerrain::getMapDataSets()
{
	return &_mapDataSets;
}

/**
 * Refreshes the terrain's mapdatafiles. Use for craft skins ONLY!
 */
void RuleTerrain::refreshMapDataSets(int craftSkinIndex, Mod *mod)
{
	if (_lastCraftSkinIndex == craftSkinIndex)
	{
		return;
	}

	std::vector<std::string> newNames;
	for (auto* item : _mapDataSets)
	{
		if (item->getName() == "BLANKS")
		{
			newNames.push_back(item->getName());
		}
		else if (_lastCraftSkinIndex == 0)
		{
			newNames.push_back(item->getName() + "_" + std::to_string(craftSkinIndex));
		}
		else
		{
			size_t lastPos = item->getName().find_last_of("_");
			std::string stripped = item->getName().substr(0, lastPos);
			if (craftSkinIndex > 0)
			{
				newNames.push_back(stripped + "_" + std::to_string(craftSkinIndex));
			}
			else
			{
				newNames.push_back(stripped);
			}
		}
	}
	_mapDataSets.clear();
	for (const auto& newName : newNames)
	{
		_mapDataSets.push_back(mod->getMapDataSet(newName));
	}
	newNames.clear();
	_lastCraftSkinIndex = craftSkinIndex;
}

/**
 * Gets the terrain name.
 * @return The terrain name.
 */
std::string RuleTerrain::getName() const
{
	return _name;
}

/**
 * Returns the enviro effects name for this terrain.
 * @return String ID for the enviro effects.
 */
const std::string& RuleTerrain::getEnviroEffects() const
{
	return _enviroEffects;
}

/**
 * Gets a random mapblock within the given constraints.
 * @param maxsize The maximum size of the mapblock (10 or 20 or 999 - don't care).
 * @param type Whether this must be a block of a certain type.
 * @param force Whether to enforce the max size.
 * @return Pointer to the mapblock.
 */
MapBlock* RuleTerrain::getRandomMapBlock(int maxSizeX, int maxSizeY, int group, bool force)
{
	std::vector<MapBlock*> compliantMapBlocks;

	for (auto* mapblock : _mapBlocks)
	{
		if ((mapblock->getSizeX() == maxSizeX ||
			(!force && mapblock->getSizeX() < maxSizeX)) &&
			(mapblock->getSizeY() == maxSizeY ||
			(!force && mapblock->getSizeY() < maxSizeY)) &&
			mapblock->isInGroup(group))
		{
			compliantMapBlocks.push_back(mapblock);
		}
	}

	if (compliantMapBlocks.empty()) return 0;

	size_t n = RNG::generate(0, compliantMapBlocks.size() - 1);

	return compliantMapBlocks[n];
}

/**
 * Gets a mapblock with a given name.
 * @param name The name of the mapblock.
 * @return Pointer to mapblock.
 */
MapBlock* RuleTerrain::getMapBlock(const std::string &name)
{
	for (auto* mapblock : _mapBlocks)
	{
		if (mapblock->getName() == name)
			return mapblock;
	}
	return 0;
}

/**
 * Gets a mapdata object.
 * @param id The id in the terrain.
 * @param mapDataSetID The id of the map data set.
 * @return Pointer to MapData object.
 */
MapData *RuleTerrain::getMapData(unsigned int *id, int *mapDataSetID) const
{
	MapDataSet* mdf = 0;
	auto iter = _mapDataSets.begin();
	for (; iter != _mapDataSets.end(); ++iter)
	{
		mdf = *iter;
		if (*id < mdf->getSize())
		{
			break;
		}
		*id -= mdf->getSize();
		(*mapDataSetID)++;
	}
	if (iter == _mapDataSets.end())
	{
		// oops! someone at microprose made an error in the map!
		// set this broken tile reference to BLANKS 0.
		mdf = _mapDataSets.front();
		*id = 0;
		*mapDataSetID = 0;
	}
	return mdf->getObject(*id);
}

/**
 * Gets the list of civilian types to use on this terrain (default MALE_CIVILIAN and FEMALE_CIVILIAN)
 * @return list of civilian types to use.
 */
std::vector<std::string> RuleTerrain::getCivilianTypes() const
{
	return _civilianTypes;
}

/**
 * Gets the min depth.
 * @return The min depth.
 */
int RuleTerrain::getMinDepth() const
{
	return _minDepth;
}

/**
 * Gets the max depth.
 * @return max depth.
 */
int RuleTerrain::getMaxDepth() const
{
	return _maxDepth;
}

/**
 * Gets The ambient sound effect.
 * @return The ambient sound effect.
 */
int RuleTerrain::getAmbience() const
{
	return _ambience;
}

/**
 * Gets The generation script name.
 * @return The name of the script to use.
 */
const std::string& RuleTerrain::getRandomMapScript() const
{
	if (!_mapScripts.empty())
	{
		size_t pick = RNG::generate(0, _mapScripts.size() - 1);
		return _mapScripts[pick];
	}
	return _mapScript;
}

/**
 * Does any map script use globe terrain?
 * @return 1 = yes, 0 = no, -1 = no map script found.
 */
int RuleTerrain::hasTextureBasedScript(const Mod* mod) const
{
	int ret = -1;
	// iterate _mapScripts
	for (const std::string& script : _mapScripts)
	{
		auto* vec = mod->getMapScript(script);
		if (vec)
		{
			ret = 0;
			// iterate map script commands
			for (auto* ms : *vec)
			{
				// iterate terrains
				for (const std::string& terrain : ms->getRandomAlternateTerrain())
				{
					if (terrain == "globeTerrain" || terrain == "baseTerrain")
					{
						return 1;
					}
				}
				// iterate vertical levels
				for (auto& vlevel : ms->getVerticalLevels())
				{
					if (vlevel.levelTerrain == "globeTerrain" || vlevel.levelTerrain == "baseTerrain")
					{
						return 1;
					}
				}
			}
		}
	}
	// check also _mapScript
	{
		auto* vec = mod->getMapScript(_mapScript);
		if (vec)
		{
			ret = 0;
			// iterate map script commands
			for (auto* ms : *vec)
			{
				// iterate terrains
				for (const std::string& terrain : ms->getRandomAlternateTerrain())
				{
					if (terrain == "globeTerrain" || terrain == "baseTerrain")
					{
						return 1;
					}
				}
				// iterate vertical levels
				for (auto& vlevel : ms->getVerticalLevels())
				{
					if (vlevel.levelTerrain == "globeTerrain" || vlevel.levelTerrain == "baseTerrain")
					{
						return 1;
					}
				}
			}
		}

	}
	return ret;
}

/**
 * Gets The list of musics this terrain has to choose from.
 * @return The list of track names.
 */
const std::vector<std::string> &RuleTerrain::getMusic() const
{
	return _music;
}


double RuleTerrain::getAmbientVolume() const
{
	return _ambientVolume;
}

}
