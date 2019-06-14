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
#include "Position.h"
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
MapEditor::MapEditor(SavedBattleGame *save) : _save(save),
    _selectedMapDataID(-1), _editRegisterPosition(0)
{
    _editRegister.clear();
    _selectedTiles.clear();
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
        _selectedTiles.push_back(tile);
        changeTiles(MET_DO);
        _selectedTiles.clear();
    }
}

/**
 * Changes tile data according to the selected tiles and map data
 * @action Type of action we're taking
 */
void MapEditor::changeTiles(EditType action)
{
    std::vector<TileEdit> changes;
    changes.clear();
    int tileIndex = 0;

    for (std::vector<Tile*>::iterator tile = _selectedTiles.begin(); tile != _selectedTiles.end(); )
    {
        // Validate to make sure we're not working on non-existant tiles
        if (*tile == 0)
        {
            tile = _selectedTiles.erase(tile);
            ++tileIndex;
            if (tile == _selectedTiles.end())
                break;
        }

        MapData *mapData[O_MAX] = {0};
        MapDataSet *mapDataSet[O_MAX] = {0};
        int mapDataID[O_MAX] = {-1, -1, -1, -1};
        int mapDataSetID[O_MAX] = {-1, -1, -1, -1};
        std::vector<TilePart> parts;
        parts.clear();

        switch (action)
        {
            case MET_DO:
                {
                    // TODO change this to actual IDs later
                    int selectedIndex = _selectedMapDataID;

                    // If no index is selected, we're clearing the tile
                    if (selectedIndex == -1)
                    {
                        for (int part = O_FLOOR; part < O_MAX; part++)
                        {
                            parts.push_back((TilePart)part);
                        }
                    }
                    // Otherwise, we need to find which object the index is pointing to
                    else
                    {
                        int selectedMapDataSetID = 0;
                        MapData *selectedMapData = getMapDataFromIndex(_selectedMapDataID, &selectedMapDataSetID, &selectedIndex);

                        if (selectedMapData)
                        {
                            // TODO: try-catch error handling for changing sprites using the undo/redo register
                            MapDataSet *selectedMapDataSet = _save->getMapDataSets()->at(selectedMapDataSetID);
                            parts.push_back(selectedMapData->getObjectType());

                            int partIndex = (int)selectedMapData->getObjectType();
                            mapData[partIndex] = selectedMapData;
                            mapDataSet[partIndex] = selectedMapDataSet;
                            mapDataID[partIndex] = selectedIndex;
                            mapDataSetID[partIndex] = selectedMapDataSetID;
                        }
                    }

                    // Save the data from the previous tile so we can undo/redo changes later
                    int beforeDataID[O_MAX] = {-1, -1, -1, -1};
                    int beforeDataSetID[O_MAX] = {-1, -1, -1, -1};
                    for (int part = O_FLOOR; part < O_MAX; part++)
                    {
                        (*tile)->getMapData(&beforeDataID[part], &beforeDataSetID[part], (TilePart)part);
                    }
                    changes.push_back(TileEdit((*tile)->getPosition(), beforeDataID, beforeDataSetID, mapDataID, mapDataSetID));
                }

                break;

            case MET_UNDO:
                {
                    for (int part = O_FLOOR; part < O_MAX; part++)
                    {
                        mapDataSetID[part] = _editRegister.at(_editRegisterPosition).at(tileIndex).tileBeforeDataSetIDs[part];
                        mapDataID[part] = _editRegister.at(_editRegisterPosition).at(tileIndex).tileBeforeDataIDs[part];

                        if (mapDataSetID[part] != -1)
                            mapDataSet[part] = _save->getMapDataSets()->at(mapDataSetID[part]);
                        if (mapDataID[part] != -1)
                        mapData[part] = mapDataSet[part]->getObjects()->at(mapDataID[part]);

                        parts.push_back((TilePart)part);
                    }
                }

                break;
            case MET_REDO:
                {
                    for (int part = O_FLOOR; part < O_MAX; part++)
                    {
                        mapDataSetID[part] = _editRegister.at(_editRegisterPosition).at(tileIndex).tileAfterDataSetIDs[part];
                        mapDataID[part] = _editRegister.at(_editRegisterPosition).at(tileIndex).tileAfterDataIDs[part];

                        if (mapDataSetID[part] != -1)
                            mapDataSet[part] = _save->getMapDataSets()->at(mapDataSetID[part]);
                        if (mapDataID[part] != -1)
                        mapData[part] = mapDataSet[part]->getObjects()->at(mapDataID[part]);

                        parts.push_back((TilePart)part);
                    }
                }

                break;
            default:

                break;
        }

        for (auto part : parts)
        {
            int partIndex = (int)part;
            (*tile)->setMapData(mapData[partIndex], mapDataID[partIndex], mapDataSetID[partIndex], (TilePart)part);
        }

        // Save the data on the changed tile for undo/redo later
        if (action == MET_DO)
        {
            for (int part = O_FLOOR; part < O_MAX; part++)
            {
                (*tile)->getMapData(&changes.back().tileAfterDataIDs[part], &changes.back().tileAfterDataSetIDs[part], (TilePart)part);
            }
        }

        ++tile;
        ++tileIndex;
    }

    if (action == MET_DO)
    {
        if (_editRegisterPosition < (int)_editRegister.size())
        {
            _editRegister.erase(_editRegister.begin() + _editRegisterPosition, _editRegister.end());
        }
        _editRegister.push_back(changes);
        ++_editRegisterPosition;
    }
}

/**
 * Un-does an action pointed to by the current position in the edit register
 */
void MapEditor::undo()
{
    if (_editRegisterPosition == 0)
        return;

    _selectedTiles.clear();

    --_editRegisterPosition;
    for (auto edit : _editRegister.at(_editRegisterPosition))
    {
        _selectedTiles.push_back(_save->getTile(edit.position));
    }

    changeTiles(MET_UNDO);
    _selectedTiles.clear();
}

/**
 * Re-does an action pointed to by the current position in the edit register
 */
void MapEditor::redo()
{
    if (_editRegisterPosition >= (int)_editRegister.size())
        return;

    _selectedTiles.clear();

    for (auto edit : _editRegister.at(_editRegisterPosition))
    {
        _selectedTiles.push_back(_save->getTile(edit.position));
    }

    changeTiles(MET_REDO);
    ++_editRegisterPosition;
    _selectedTiles.clear();
}

/**
 * Helper function for getting MapData, MapDataSetIDs, and MapDataIDs from index of a tile
 * @param index The index of the tile object data we want
 * @param mapDataSetID The index of the MapDataSet
 * @param mapDataID The index of the MapData within the MapDataSet
 */
MapData *MapEditor::getMapDataFromIndex(int index, int *mapDataSetID, int *mapDataID)
{
    MapDataSet *mapDataSet = 0;
    int dataSetID = 0;

    // This causes a segfault when called at the initialization of BattlescapeState after changing resolution in video options- why?
    for (auto i : *_save->getMapDataSets())
    {
        if (index < (int)i->getSize())
        {
            mapDataSet = i;
            break;
        }
        else
        {
            index -= i->getSize();
            ++dataSetID;
        }
    }

    if (mapDataSet)
    {
        *mapDataSetID = dataSetID;
        *mapDataID = index;
        return mapDataSet->getObjects()->at(index);
    }

    // If the index is greater than the number of available tile objects, return that we've found nothing
    *mapDataSetID = -1;
    *mapDataID = -1;
    return 0;
}

/**
 * Gets the current position of the edit register
 */
int MapEditor::getEditRegisterPosition()
{
    return _editRegisterPosition;
}

/**
 * Gets the number of edits in the register
 */
int MapEditor::getEditRegisterSize()
{
    return (int)_editRegister.size();
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