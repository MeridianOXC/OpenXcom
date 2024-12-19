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

    // define equal to operator to allow matching for the purpose of std::find_if
    bool operator == (const MapFileInfo& otherFile) const
    {
        return baseDirectory == otherFile.baseDirectory
                && name == otherFile.name
                && terrain == otherFile.terrain;
        
        // TODO: determine if there's a use case to check mods; most maps will be saved in the same folder as their mod
        // This will require some refactoring if we want to use SavedGame::sanitizeModName
    }
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
    /// Gets the data for the map we want to load
    MapFileInfo *getMapFileToLoad();
    /// Clears the MapFileInfo for the file we want to load
    void clearMapFileToLoad();
    /// Gets the data for the current map being edited
    MapFileInfo *getCurrentMapFile();
    /// Adds the data on a new edited map file to the list
    void addMap(MapFileInfo fileInfo);
    /// Search for entries matching the given directory + map name
    size_t findMatchingFiles(MapFileInfo *fileInfo);
    /// Gets the list of entries found by the search
    std::vector<MapFileInfo> *getMatchedFiles();

private:
    std::vector<MapFileInfo> _savedMapFiles, _matchedFiles;
    MapFileInfo _mapFileToLoad, _currentMapFile;

};

}