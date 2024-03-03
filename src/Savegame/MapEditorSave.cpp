/*
 * Copyright 2010-2024 OpenXcom Developers.
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
#include "MapEditorSave.h"
#include <sstream>
#include <yaml-cpp/yaml.h>
#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/Logger.h"

namespace OpenXcom
{

const std::string MapEditorSave::AUTOSAVE_MAPEDITOR = "_auto_mapeditor_.asav",
				  MapEditorSave::MAINSAVE_MAPEDITOR = "mapeditor.sav";

const std::string MapEditorSave::MAP_DIRECTORY = "MAPS/",
				  MapEditorSave::RMP_DIRECTORY = "ROUTES/";

/**
 * Initializes the class for storing information for saving or editing maps
 */
MapEditorSave::MapEditorSave()
{
    _savedMapFiles.clear();
    Log(LOG_INFO) << _currentMapFile.name;
    Log(LOG_INFO) << _currentMapFile.baseDirectory;
    Log(LOG_INFO) << _currentMapFile.terrain;
}

/**
 * Deletes data on edited map files from memory
 */
MapEditorSave::~MapEditorSave()
{

}

/**
 * Loads the data on edited map files from the user directory
 */
void MapEditorSave::load()
{
    std::string filename = MAINSAVE_MAPEDITOR;
	std::string filepath = Options::getMasterUserFolder() + filename;

    if(CrossPlatform::fileExists(filepath))
    {
        std::vector<YAML::Node> file = YAML::LoadAll(*CrossPlatform::readFile(filepath));
        YAML::Node doc = file[0];

        for (YAML::const_iterator it = doc["savedMapFiles"].begin(); it != doc["savedMapFiles"].end(); ++it)
        {
            MapFileInfo mapFile;
            mapFile.name = (*it)["name"].as<std::string>();
            mapFile.baseDirectory = (*it)["baseDirectory"].as<std::string>();
            mapFile.mods = (*it)["mods"].as< std::vector<std::string> >();
            mapFile.terrain = (*it)["terrain"].as<std::string>();
            mapFile.mcds = (*it)["mcds"].as< std::vector<std::string> >();

            _savedMapFiles.push_back(mapFile);
        }
    }

}

/**
 * Saves the data on edited map files to the user directory
 */
void MapEditorSave::save()
{
	YAML::Emitter out;

    // Saves the data on the edited maps
    YAML::Node node;
    for (auto mapFile : _savedMapFiles)
    {
        YAML::Node fileData;
        fileData["name"] = mapFile.name;
        fileData["baseDirectory"] = mapFile.baseDirectory;
        for (auto mod : mapFile.mods)
        {
            fileData["mods"].push_back(mod);
        }
        fileData["terrain"] = mapFile.terrain;
        for (auto mcd : mapFile.mcds)
        {
            fileData["mcds"].push_back(mcd);
        }

        node["savedMapFiles"].push_back(fileData);
    }

    out << node;

    std::string filename = MAINSAVE_MAPEDITOR;
	std::string filepath = Options::getMasterUserFolder() + filename;
	if (!CrossPlatform::writeFile(filepath, out.c_str()))
	{
		throw Exception("Failed to save " + filepath);
	}
}

/**
 * Adds the data on a new edited map file to the list
 * @param fileInfo information on the new edited map file
 */
void MapEditorSave::addMap(MapFileInfo fileInfo)
{
    MapFileInfo existingMap = getMapFileInfo(fileInfo.baseDirectory, fileInfo.name);

    // Only add a new entry if one doesn't already exist
    if(existingMap.name.size() == 0)
    {
        _savedMapFiles.push_back(fileInfo);
    }

    _currentMapFile = fileInfo;
}

/**
 * Gets the data for the current map being edited
 * @return pointer to the current map info
 */
MapFileInfo *MapEditorSave::getCurrentMapFile()
{
    return &_currentMapFile;
}

/**
 * Search for a specific entry in the list of edited map files
 * @return copy of the data for the map file
 */
MapFileInfo MapEditorSave::getMapFileInfo(std::string mapDirectory, std::string mapName)
{
    MapFileInfo fileInfo;
    fileInfo.name = "";

    // TODO: more efficient search?
    for (size_t i = 0; i != _savedMapFiles.size(); ++i)
    {
        if (mapName == _savedMapFiles.at(i).name && mapDirectory == _savedMapFiles.at(i).baseDirectory)
        {
            fileInfo = _savedMapFiles.at(i);
            break;
        }
    }

    return fileInfo;
}

}