#pragma once
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

#include <vector>
#include <string>

namespace OpenXcom
{

/**
 * Container for map file info for saving and loading maps to edit
 */
struct MapFileInfo
{
	std::string name;
	std::string baseDirectory;
	std::vector<std::string> mods;
    std::string terrain;
    std::vector<std::string> mcds;
};

class MapEditorSave
{
public:
    static const std::string AUTOSAVE_MAPEDITOR, MAINSAVE_MAPEDITOR;
    static const std::string MAP_DIRECTORY, RMP_DIRECTORY;

    /// Initializes the class for storing information for saving or editing maps
    MapEditorSave();
    /// Deletes data on edited map files from memory
    ~MapEditorSave();
    /// Loads the data on edited map files from the user directory
    void load();
    /// Saves the data on edited map files to the user directory
    void save();
    /// Adds the data on a new edited map file to the list
    void addMap(MapFileInfo fileInfo);
    /// Gets the data for the current map being edited
    MapFileInfo *getCurrentMapFile();
    /// Search for a specific entry in the list of edited map files
    MapFileInfo getMapFileInfo(std::string mapDirectory, std::string mapName);

private:
    std::vector<MapFileInfo> _savedMapFiles;
    MapFileInfo _currentMapFile;

};

}