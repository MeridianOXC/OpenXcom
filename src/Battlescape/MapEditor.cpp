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
#include "MapEditor.h"
#include "../Engine/Action.h"
#include "../Mod/MapData.h"
#include "../Mod/MapDataSet.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include <sstream>

namespace OpenXcom
{

/**
 * Initializes all the Map Editor.
 */
MapEditor::MapEditor(SavedBattleGame *save) : _save(save), _selectedMapDataID(-1)
{
    _editRegister.clear();

}

/**
 * Cleans up the Map Editor.
 */
MapEditor::~MapEditor()
{

}

/**
 * Handles inputs passed to the editor from the BattlescapeState
 * @param action Pointer to the action
 * @param tile Pointer to a selected tile
 */
void MapEditor::handleEditorInput(Action *action, Tile *tile)
{
    if (tile != 0 && action->getDetails()->button.button == SDL_BUTTON_RIGHT)
    {
        // TODO change this to actual IDs later
        int selectedIndex = _selectedMapDataID;

        if (selectedIndex == -1)
        {
            for (int part = O_FLOOR; part < O_MAX; part++)
            {
                TilePart tp = (TilePart)part;
                tile->setMapData(0, -1, -1, tp);
            }
        }
        else
        {
            MapDataSet *mapDataSet;
            int mapDataSetID = 0;
            for (auto i : *_save->getMapDataSets())
            {
                if (selectedIndex < i->getSize())
                {
                    mapDataSet = i;
                    break;
                }
                else
                {
                    selectedIndex -= i->getSize();
                    ++mapDataSetID;
                }
            }

            if (mapDataSet)
            {
                // TODO: try-catch error handling for changing sprites using the undo/redo register
                MapData *mapData = mapDataSet->getObjects()->at(selectedIndex);
                tile->setMapData(mapData, selectedIndex, mapDataSetID, mapData->getObjectType());
            }
        }
    }
}

/**
 * Sets the selected Map Data ID
 * @param selectedIndex the Map Data ID
 */
void MapEditor::setSelectedMapDataID(int selectedIndex)
{
    _selectedMapDataID = selectedIndex;
}

/**
 * Gets the selected Map Data ID
 */
 int MapEditor::getSelectedMapDataID()
{
    return _selectedMapDataID;
}

}