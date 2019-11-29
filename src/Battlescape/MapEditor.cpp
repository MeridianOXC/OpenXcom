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
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
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
    _selectedMapDataID(-1), _editRegisterPosition(0), _mapname(""), _selectedObject(O_MAX)
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

                    // If no index is selected, we're clearing something from the tile
                    if (selectedIndex == -1)
                    {
                        // No particular object selected, clear the whole tile
                        if (_selectedObject == O_MAX)
                        {
                            for (int part = O_FLOOR; part < O_MAX; part++)
                            {
                                parts.push_back((TilePart)part);
                            }
                        }
                        // Clear just the selected part of the tile
                        else
                        {
                            parts.push_back(_selectedObject);
                        }
                    }
                    // Otherwise, we need to find which object the index is pointing to
                    else
                    {
                        int selectedMapDataSetID = 0;
                        MapData *selectedMapData = getMapDataFromIndex(_selectedMapDataID, &selectedMapDataSetID, &selectedIndex);

                        if (selectedMapData)
                        {
                            MapDataSet *selectedMapDataSet = _save->getMapDataSets()->at(selectedMapDataSetID);
                            if (_selectedObject == O_MAX)
                            {
                                parts.push_back(selectedMapData->getObjectType());
                            }
                            else
                            {
                                parts.push_back(_selectedObject);
                            }

                            int partIndex = (int)parts.back();
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
                        mapData[part] = mapDataSet[part]->getObject(mapDataID[part]);

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
                        mapData[part] = mapDataSet[part]->getObject(mapDataID[part]);

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

            if (changes.back().isEditEmpty())
            {
                changes.pop_back();
            }
        }

        ++tile;
        ++tileIndex;
    }

    if (action == MET_DO && changes.size() != 0)
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

    for (const auto &i : *_save->getMapDataSets())
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
        return mapDataSet->getObject(index);
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

/**
 * Sets the selected tile part
 * @param part the part of the tile to edit
 */
void MapEditor::setSelectedObject(TilePart part)
{
    _selectedObject = part;
}

/**
 * Gets the selected part of the tile
 */
TilePart MapEditor::getSelectedObject()
{
    return _selectedObject;
}

/**
 * Sets the SavedBattleGame for the editor
 * Used when restarting the battle state when changing video options
 * @param save Pointer to the saved game
 */
void MapEditor::setSave(SavedBattleGame *save)
{
    _save = save;
}

/**
 * Sets the name of the map file we're editing
 * @param mapname String of the map's name
 */
void MapEditor::setMapName(std::string mapname)
{
    _mapname = mapname;
}

/**
 * Gets the name of the map file we're editing
 */
std::string MapEditor::getMapName()
{
    return _mapname;
}

/**
 * Saves the map file
 * @param filename String for the name of the file to save
 */
void MapEditor::saveMapFile(std::string filename)
{
    std::string filepath = Options::getMasterUserFolder() + filename + ".MAP";
    std::string logInfo = "    mapDataSets:\n";
    for (auto i : *_save->getMapDataSets())
    {
        logInfo += "      - " + i->getName() + "\n";
    }
    logInfo += "    mapBlocks:\n";
    logInfo += "      - name: " + filename + "\n";
    logInfo += "        width: " + std::to_string(_save->getMapSizeX()) + "\n";
    logInfo += "        length: " + std::to_string(_save->getMapSizeY()) + "\n";
    logInfo += "        height: " + std::to_string(_save->getMapSizeZ());
    Log(LOG_INFO) << "Saving edited map file " << filepath << "\n" << logInfo;

    std::vector<unsigned char> data;
    data.clear();

    data.push_back((unsigned char)_save->getMapSizeY()); // x and y are in opposite order in MAP files
    data.push_back((unsigned char)_save->getMapSizeX());
    data.push_back((unsigned char)_save->getMapSizeZ());

    for (int z = _save->getMapSizeZ() - 1; z > -1; --z)
    {
        for (int y = 0; y < _save->getMapSizeY(); ++y)
        {
            for (int x = 0; x < _save->getMapSizeX(); ++x)
            {
                // TODO use tile->isVoid() to determine if we can speed up the process by adding an empty tile?
                Tile *tile = _save->getTile(Position(x, y, z));

                for (int part = O_FLOOR; part < O_MAX; ++part)
                {
                    int mapDataID;
                    int mapDataSetID;
                    tile->getMapData(&mapDataID, &mapDataSetID, (TilePart)part);

                    if (mapDataSetID != -1 && mapDataID != -1)
                    {
                        for (int i = 0; i < mapDataSetID; ++i)
                        {
                            mapDataID += _save->getMapDataSets()->at(i)->getSize();
                        }
                    }
                    else
                    {
                        mapDataID = 0;
                    }

                    data.push_back((unsigned char)mapDataID);
                }
            }
        }
    }

	if (!CrossPlatform::writeFile(filepath, data))
	{
		throw Exception("Failed to save " + filepath);
	}

}

}