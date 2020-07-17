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
#include "../Battlescape/TileEngine.h"
#include "../Engine/Action.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Mod/MapData.h"
#include "../Mod/MapDataSet.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include <sstream>

namespace OpenXcom
{

/**
 * Initializes all the Map Editor.
 */
MapEditor::MapEditor(SavedBattleGame *save) : _save(save),
    _selectedMapDataID(-1), _tileRegisterPosition(0), _nodeRegisterPosition(0), _mapname(""), _selectedObject(O_MAX)
{
    _tileRegister.clear();
    _nodeRegister.clear();
    _selectedTiles.clear();
    _selectedNodes.clear();
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
 * Handles inputs passed to the editor from the node information panels
 * @param action Pointer to the action
 * @param changeType What type of change we're doing
 * @param data The new value to change in the node
 */
void MapEditor::handleNodeInput(Action *action, NodeChangeType changeType, std::vector<int> data)
{
    if (!_selectedNodes.empty())
    {
        changeNodes(MET_DO, changeType, data);
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
                        mapDataSetID[part] = _tileRegister.at(_tileRegisterPosition).at(tileIndex).tileBeforeDataSetIDs[part];
                        mapDataID[part] = _tileRegister.at(_tileRegisterPosition).at(tileIndex).tileBeforeDataIDs[part];

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
                        mapDataSetID[part] = _tileRegister.at(_tileRegisterPosition).at(tileIndex).tileAfterDataSetIDs[part];
                        mapDataID[part] = _tileRegister.at(_tileRegisterPosition).at(tileIndex).tileAfterDataIDs[part];

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

        // Recalculate lighting
        _save->getTileEngine()->calculateLighting(LL_AMBIENT, (*tile)->getPosition(), 1, true);

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
        if (_tileRegisterPosition < (int)_tileRegister.size())
        {
            _tileRegister.erase(_tileRegister.begin() + _tileRegisterPosition, _tileRegister.end());
        }
        _tileRegister.push_back(changes);
        ++_tileRegisterPosition;
    }
}

/**
 * Changes node data according to the selected nodes and route data
 * @action Type of action we're taking
 */
void MapEditor::changeNodes(EditType action, NodeChangeType changeType, std::vector<int> data)
{
    std::vector<NodeEdit> changes;
    changes.clear();
    int nodeIndex = 0;

    // TODO: refactor for adding/removing nodes
    for (std::vector<Node*>::iterator node = _selectedNodes.begin(); node != _selectedNodes.end(); )
    {
        // Make sure we're not working on non-existant nodes
        if (*node == 0)
        {
            node = _selectedNodes.erase(node);
            ++nodeIndex;
            if (node == _selectedNodes.end())
                break;
        }

        switch (action)
        {
            case MET_DO:
                {
                    changes.push_back(changeNodeData(*node, changeType, data));
                }

                break;

            case MET_UNDO:
                {
                    data = _nodeRegister.at(_nodeRegisterPosition).at(nodeIndex).nodeBeforeData;
                    changeType = _nodeRegister.at(_nodeRegisterPosition).at(nodeIndex).nodeChangeType;
                    changeNodeData(*node, changeType, data);
                }

                break;

            case MET_REDO:
                {
                    data = _nodeRegister.at(_nodeRegisterPosition).at(nodeIndex).nodeAfterData;
                    changeType = _nodeRegister.at(_nodeRegisterPosition).at(nodeIndex).nodeChangeType;
                    changeNodeData(*node, changeType, data);
                }

                break;

            default:

                break;
        }

        // Save the data on the changed node for undo/redo later
        if (action == MET_DO)
        {
            NodeEdit change = changes.back();
            if (change.nodeChangeType != NCT_NEW && change.nodeChangeType != NCT_DELETE && change.isEditEmpty())
            {
                changes.pop_back();
            }
        }

        ++node;
    }

    if (action == MET_DO && changes.size() != 0)
    {
        if (_nodeRegisterPosition < (int)_nodeRegister.size())
        {
            _nodeRegister.erase(_nodeRegister.begin() + _nodeRegisterPosition, _nodeRegister.end());
        }
        _nodeRegister.push_back(changes);
        ++_nodeRegisterPosition;
    }
}

/**
 * Changes the data of a specific node according to the selected route data
 * Populates a NodeEdit for saving later if necessary
 * @node pointer to the node
 * @changeType which data field we're changing
 * @data the new data being passed to the editor
 * @linkID which link number we're changing (default first one)
 * @return the NodeEdit for the change performed
 */
NodeEdit MapEditor::changeNodeData(Node *node, NodeChangeType changeType, std::vector<int> data)
{
    NodeEdit change = NodeEdit(node->getID(), changeType);

    switch(changeType)
    {
        case NCT_POS:
            {
                change.nodeBeforeData.push_back(node->getPosition().x);
                change.nodeBeforeData.push_back(node->getPosition().y);
                change.nodeBeforeData.push_back(node->getPosition().z);
                change.nodeAfterData.push_back(data.at(0));
                change.nodeAfterData.push_back(data.at(1));
                change.nodeAfterData.push_back(data.at(2));

                Position pos = Position(data.at(0), data.at(1), data.at(2));
                node->setPosition(pos);
            }
        
            break;
        
        case NCT_TYPE:
            {
                change.nodeBeforeData.push_back(node->getType());
                change.nodeAfterData.push_back(data.at(0));

                node->setType(data.at(0));
            }
        
            break;
        
        case NCT_RANK:
            {
                change.nodeBeforeData.push_back(node->getRank());
                change.nodeAfterData.push_back(data.at(0));

                node->setRank(data.at(0));
            }

            break;
        
        case NCT_FLAG:
            {
                change.nodeBeforeData.push_back(node->getFlags());
                change.nodeAfterData.push_back(data.at(0));

                node->setFlags(data.at(0));
            }

            break;
        
        case NCT_PRIORITY:
            {
                change.nodeBeforeData.push_back(node->getPriority());
                change.nodeAfterData.push_back(data.at(0));

                node->setPriority(data.at(0));
            }

            break;
        
        case NCT_RESERVED:
            {
                change.nodeBeforeData.push_back(node->isTarget() ? 5 : 0);
                change.nodeAfterData.push_back(data.at(0));

                node->setReserved(data.at(0));
            }

            break;
        
        case NCT_LINKS:
            {
                change.nodeBeforeData.push_back(data.at(0));
                change.nodeBeforeData.push_back(node->getNodeLinks()->at(data.at(0)));
                change.nodeAfterData.push_back(data.at(0));
                change.nodeAfterData.push_back(data.at(1));

                node->getNodeLinks()->at(data.at(0)) = data.at(1);
            }

            break;
        
        case NCT_LINKTYPES:
            {
                change.nodeBeforeData.push_back(data.at(0));
                change.nodeBeforeData.push_back(node->getLinkTypes()->at(data.at(0)));
                change.nodeAfterData.push_back(data.at(0));
                change.nodeAfterData.push_back(data.at(1));

                node->getLinkTypes()->at(data.at(0)) = data.at(1);
            }

            break;
        
        default:

            break;
    }

    return change;
}

/**
 * Helper function for figuring out which link is next open on a node
 * Handles which index is used to make links when not explicitly given
 * @param node pointer to the node
 * @param advanceIndex used when making a connection, advance to the next available index in our data map
 */
int MapEditor::getNextNodeConnectionIndex(Node *node, bool advanceIndex)
{
    int nextAvailableIndex = -1;
    int currentIndex = 0;

    // validate that the node we're trying to examine exists
    if (!node)
    {
        return nextAvailableIndex;
    }

    // check to see if we already have data on the requested node
    std::map<int, int>::iterator it = _connectionIndexMap.find(node->getID());
    if (it != _connectionIndexMap.end())
    {
        currentIndex = it->second;
    }

    // check to see if there are any unused connections on the node first
    for (int i = 0; i < 5; ++i)
    {
        if (node->getNodeLinks()->at((currentIndex + i) % 5) == -1)
        {
            nextAvailableIndex = (currentIndex + i) % 5;
            break;
        }
    }

    // the available index is either the empty one we found or just the next one in line
    nextAvailableIndex = nextAvailableIndex == -1 ? (currentIndex + 1) % 5 : nextAvailableIndex;

    // if we either don't have data on this node next or advancing the stored data was requested, store that now
    if (it == _connectionIndexMap.end() || advanceIndex)
    {
        _connectionIndexMap[node->getID()] = nextAvailableIndex;
    }

    return nextAvailableIndex;
}

/**
 * Un-does an action pointed to by the current position in the edit register
 */
void MapEditor::undo(bool node)
{
    if (!node)
    {
        if (_tileRegisterPosition == 0)
            return;

        _selectedTiles.clear();

        --_tileRegisterPosition;
        for (auto edit : _tileRegister.at(_tileRegisterPosition))
        {
            _selectedTiles.push_back(_save->getTile(edit.position));
        }

        changeTiles(MET_UNDO);
        _selectedTiles.clear();
    }
    else
    {
        if (_nodeRegisterPosition == 0)
            return;

        _selectedNodes.clear();

        --_nodeRegisterPosition;
        for (auto edit : _nodeRegister.at(_nodeRegisterPosition))
        {
            _selectedNodes.push_back(_save->getNodes()->at(edit.nodeID));
        }

        changeNodes(MET_UNDO, NCT_NONE, std::vector<int> {});
        _selectedNodes.clear();
    }
}

/**
 * Re-does an action pointed to by the current position in the edit register
 */
void MapEditor::redo(bool node)
{
    if (!node)
    {
        if (_tileRegisterPosition >= (int)_tileRegister.size())
            return;

        _selectedTiles.clear();

        for (auto edit : _tileRegister.at(_tileRegisterPosition))
        {
            _selectedTiles.push_back(_save->getTile(edit.position));
        }

        changeTiles(MET_REDO);
        ++_tileRegisterPosition;
        _selectedTiles.clear();
    }
    else
    {
        if (_nodeRegisterPosition >= (int)_nodeRegister.size())
            return;

        _selectedNodes.clear();

        for (auto edit : _nodeRegister.at(_nodeRegisterPosition))
        {
            _selectedNodes.push_back(_save->getNodes()->at(edit.nodeID));
        }

        changeNodes(MET_REDO, NCT_NONE, std::vector<int> {});
        ++_nodeRegisterPosition;
        _selectedNodes.clear();
    }
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
 * Gets the current position of the tile edit register
 */
int MapEditor::getTileRegisterPosition()
{
    return _tileRegisterPosition;
}

/**
 * Gets the number of edits in the tile register
 */
int MapEditor::getTileRegisterSize()
{
    return (int)_tileRegister.size();
}

/**
 * Gets the current position of the node edit register
 */
int MapEditor::getNodeRegisterPosition()
{
    return _nodeRegisterPosition;
}

/**
 * Gets the number of edits in the node register
 */
int MapEditor::getNodeRegisterSize()
{
    return (int)_nodeRegister.size();
}

/**
 * Gets a pointer to the list of selected tiles for editing
 */
std::vector<Tile*> *MapEditor::getSelectedTiles()
{
    return &_selectedTiles;
}

/**
 * Gets a pointer to the list of selected nodes for editing
 */
std::vector<Node*> *MapEditor::getSelectedNodes()
{
    return &_selectedNodes;
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
    std::string filepath = Options::getMasterUserFolder() + filename;
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
    Log(LOG_INFO) << logInfo;

    Log(LOG_INFO) << "Saving edited map file " + filepath + ".MAP";

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

	if (!CrossPlatform::writeFile(filepath + ".MAP", data))
	{
		throw Exception("Failed to save " + filepath + ".MAP");
	}

    Log(LOG_INFO) << "Saving edited route file " + filepath + ".RMP";

    // Node IDs aren't saved into RMP files, so the index has to match position of the node in the file
    // Start by finding how many 24-byte nodes we need to allocate by the max node ID
    // We don't use size of the nodes vector (for now, will be fixed later, TODO) since adding/deleting nodes could add empty IDs
    int maxID = 0;
    for (auto node : *_save->getNodes())
    {
        if (node->getID() > maxID)
        {
            maxID = node->getID();
        }
    }

    // Assign vector to hold the node data, 24 bytes per node (up to the max ID number we just found)
    data.clear();
    data.resize(24 * (maxID + 1));

    for (auto node : *_save->getNodes())
    {
        size_t nodeIDBytePosition = 24 * node->getID();
        data.at(nodeIDBytePosition + 0) = (unsigned char)node->getPosition().y;
        data.at(nodeIDBytePosition + 1) = (unsigned char)node->getPosition().x;
        data.at(nodeIDBytePosition + 2) = (unsigned char)(_save->getMapSizeZ() - 1 - node->getPosition().z);

        for (int i = 0; i < 5; ++i)
        {
            int link = node->getNodeLinks()->at(i);
            // negative values are for special links
			// 255/-1 = unused, 254/-2 = north, 253/-3 = east, 252/-4 = south, 251/-5 = west
            if (link < 0)
            {
                link = 256 + link;
            }
            int type = node->getLinkTypes()->at(i);

            data.at(nodeIDBytePosition + 4 + i * 3) = (unsigned char)link;
            data.at(nodeIDBytePosition + 5 + i * 3) = 0;
            data.at(nodeIDBytePosition + 6 + i * 3) = (unsigned char)type;
        }

        data.at(nodeIDBytePosition + 19) = (unsigned char)node->getType();
        data.at(nodeIDBytePosition + 20) = (unsigned char)node->getRank();
        data.at(nodeIDBytePosition + 21) = (unsigned char)node->getFlags();
        data.at(nodeIDBytePosition + 22) = node->isTarget() ? 5 : 0;
        data.at(nodeIDBytePosition + 23) = (unsigned char)node->getPriority();
    }

	if (!CrossPlatform::writeFile(filepath + ".RMP", data))
	{
		throw Exception("Failed to save " + filepath + ".RMP");
	}

}

}