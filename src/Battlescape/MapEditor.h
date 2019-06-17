#pragma once
/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include "Position.h"
#include "../Mod/MapData.h"

namespace OpenXcom
{

enum EditType {MET_DO, MET_UNDO, MET_REDO};

struct TileEdit
{
    Position position;
    int tileBeforeDataIDs[O_MAX];
    int tileBeforeDataSetIDs[O_MAX];
    int tileAfterDataIDs[O_MAX];
    int tileAfterDataSetIDs[O_MAX];

    TileEdit(Position pos, int beforeDataIDs[], int beforeDataSetIDs[], int afterDataIDs[], int afterDataSetIDs[])
    {
        position = pos;
        for (int i = 0; i < O_MAX; ++i)
        {
            tileBeforeDataIDs[i] = beforeDataIDs[i];
            tileBeforeDataSetIDs[i] = beforeDataSetIDs[i];
            tileAfterDataIDs[i] = afterDataIDs[i];
            tileAfterDataSetIDs[i] = afterDataSetIDs[i];
        }
    }
};

class Action;
class SavedBattleGame;
class Tile;

class MapEditor
{
private :
    SavedBattleGame *_save;
    std::vector< std::vector< TileEdit > > _editRegister;
    int _selectedMapDataID, _editRegisterPosition;
    std::vector< Tile* > _selectedTiles;
    std::string _mapname;

public :
    /// Creates the Map Editor
    MapEditor(SavedBattleGame *save);
    /// Cleans up the Map Editor
    ~MapEditor();
    /// Handles input passed to the Editor from the BattlescapeState
    void handleEditorInput(Action *action, Tile *tile);
    /// Changes tile data according to the selected tiles and map data
    void changeTiles(EditType action);
    /// Un-does an action pointed to by the current position in the edit register
    void undo();
    /// Re-does an action pointed to by the current position in the edit register
    void redo();
    /// Helper function for getting MapData, MapDataSetIDs, and MapDataIDs from index of a tile
    MapData *getMapDataFromIndex(int index, int *mapDataSetID, int *mapDataID);
    /// Gets the current position of the edit register
    int getEditRegisterPosition();
    /// Gets the number of edits in the register
    int getEditRegisterSize();
    /// Sets the map data ID index selected
    void setSelectedMapDataID(int selectedIndex);
    /// Gets the map data ID index selected
    int getSelectedMapDataID();
    /// Sets the SavedBattleGame
    void setSave(SavedBattleGame *save);
    /// Sets the name of the map we're editing
    void setMapName(std::string mapname);
    /// Gets the name of the map we're editing
    std::string getMapName();
    /// Saves the map file
    void saveMapFile(std::string filename);

};

}