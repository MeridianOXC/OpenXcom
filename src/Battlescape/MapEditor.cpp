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
    _selectedMapDataID(-1), _tileRegisterPosition(0), _nodeRegisterPosition(0), _mapname(""), _selectedObject(O_MAX), _numberOfActiveNodes(0)
{
    _tileRegister.clear();
    _proposedTileEdits.clear();
    _nodeRegister.clear();
    _proposedNodeEdits.clear();
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
 * Changes the data of a specific tile according to the given MCD data
 * @param action specifies this as a new edit or undo/redo
 * @param tile pointer to the tile
 * @param dataIDs index of the mapData objects we're changing to, within their map data sets
 * @param dataSetIDs index of the map data sets
 */
void MapEditor::changeTileData(EditType action, Tile *tile, int dataIDs[4], int dataSetIDs[4])
{
    int beforeDataIDs[O_MAX];
    int beforeDataSetIDs[O_MAX];
    int afterDataIDs[O_MAX];
    int afterDataSetIDs[O_MAX];

    std::vector<TilePart> parts = {O_FLOOR, O_WESTWALL, O_NORTHWALL, O_OBJECT};
    for (auto part : parts)
    {
        int partIndex = (int)part;
        // grab the tile's data from before the change in case we need to save it
        tile->getMapData(&beforeDataIDs[partIndex], &beforeDataSetIDs[partIndex], part);

        // perform the change on the current part
        if (dataIDs[partIndex] != beforeDataIDs[partIndex] || dataSetIDs[partIndex] != beforeDataSetIDs[partIndex])
        {
            MapData *mapData = 0;
            if (dataIDs[partIndex] != -1 && dataSetIDs[partIndex] != -1)
            {
                mapData = _save->getMapDataSets()->at(dataSetIDs[partIndex])->getObject((size_t)dataIDs[partIndex]);
            }
            tile->setMapData(mapData, dataIDs[partIndex], dataSetIDs[partIndex], (TilePart)part);
        }

        // grab the tile's data from after the change for saving
        tile->getMapData(&afterDataIDs[partIndex], &afterDataSetIDs[partIndex], part);
    }

    TileEdit change = TileEdit(tile->getPosition(), beforeDataIDs, beforeDataSetIDs, afterDataIDs, afterDataSetIDs);
    if (action == MET_DO && !change.isEditEmpty())
    {
        _proposedTileEdits.push_back(change);
    }

    // Recalculate lighting so it updates properly for display
    _save->getTileEngine()->calculateLighting(LL_AMBIENT, tile->getPosition(), 1, true);
}

/**
 * Changes the data of a specific node according to the selected route data
 * @param action specifies this as a new edit or undo/redo
 * @param node pointer to the node
 * @param changeType which data field we're changing
 * @param data the new data being passed to the editor
 */
void MapEditor::changeNodeData(EditType action, Node *node, NodeChangeType changeType, std::vector<int> data)
{
    NodeEdit change = NodeEdit(-1, changeType);
    // make sure we have a node to get its ID
    // leaving the ID as -1 means we're probably making a new node, and the ID will be set later
    if (node)
    {
        change.nodeID = node->getID();
    }

    switch(changeType)
    {
        case NCT_NEW:
            {
                // creating a new node: only when doing a new change
                // undoing/redoing node creation/deletion will just be marking as active or inactive
                if (action == MET_DO)
                {
                    std::vector<int> beforeData(data.size(), -1);
                    change.nodeBeforeData.insert(change.nodeBeforeData.begin(), beforeData.begin(), beforeData.end());
                    change.nodeAfterData.insert(change.nodeAfterData.begin(), data.begin(), data.end());

                    int nodeID = _save->getNodes()->size();
                    Position pos = Position(data.at(0), data.at(1), data.at(2));
                    int segment = data.at(3);
                    int type = data.at(4);
                    int rank = data.at(5);
                    int flags = data.at(6);
                    int reserved = data.at(7);
                    int priority = data.at(8);
                    node = new Node(nodeID, pos, segment, type, rank, flags, reserved, priority);
                    for (int i = 0; i < 5; ++i)
                    {
                        node->getNodeLinks()->push_back(data.at(9 + i));
                        node->getLinkTypes()->push_back(data.at(14 + i));
                    }

                    _save->getNodes()->push_back(node);

                    change.nodeID = nodeID;
                    setNodeAsActive(node, true);
                }
                // undo or redo - just toggle the active/inactive marker for the node
                else
                {
                    bool active = action == MET_REDO;
                    setNodeAsActive(node, active);

                    std::vector<Node*>::iterator it = std::find(_selectedNodes.begin(), _selectedNodes.end(), node);
                    if (it != _selectedNodes.end() && !active)
                    {
                        _selectedNodes.erase(it);
                    }
                }

            }

            break;

        case NCT_DELETE:
            {
                // "deleting" a node in the editor just marks it as inactive
                // inactive nodes are not drawn, links to/from them are ignored, and they won't be saved to the RMP file
                bool active = action == MET_UNDO;
                setNodeAsActive(node, active);

                std::vector<Node*>::iterator it = std::find(_selectedNodes.begin(), _selectedNodes.end(), node);
                if (it != _selectedNodes.end() && !active && action != MET_DO) // let the MapEditorState clear selected nodes when deleting directly
                {
                    _selectedNodes.erase(it);
                }
            }

            break;

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

    // if this is a new edit, then save it for the register
    if (action == MET_DO && !change.isEditEmpty())
    {
        _proposedNodeEdits.push_back(change);
    }
}

/**
 * Confirm recent changes and advance the register index
 * @param nodeChange whether we're confirming tile or node edits
 */
void MapEditor::confirmChanges(bool nodeChange)
{
    if (!nodeChange)
    {
        // make sure we have changes to commit
        if (!_proposedTileEdits.empty())
        {
            // if we've un-done some changes recently, then we need to clear from the current position forward in the register
            if (_tileRegisterPosition < (int)_tileRegister.size())
            {
                _tileRegister.erase(_tileRegister.begin() + _tileRegisterPosition, _tileRegister.end());
            }
            _tileRegister.push_back(_proposedTileEdits);
            ++_tileRegisterPosition;
        }

        _proposedTileEdits.clear();
    }
    else
    {
        // make sure we have changes to commit
        if (!_proposedNodeEdits.empty())
        {
            // if we've un-done some changes recently, then we need to clear from the current position forward in the register
            if (_nodeRegisterPosition < (int)_nodeRegister.size())
            {
                _nodeRegister.erase(_nodeRegister.begin() + _nodeRegisterPosition, _nodeRegister.end());
            }
            _nodeRegister.push_back(_proposedNodeEdits);
            ++_nodeRegisterPosition;
        }

        _proposedNodeEdits.clear();
    }
}

/**
 * Changes tile data for undo or redo actions
 * @param action Type of action we're taking
 */
void MapEditor::undoRedoTiles(EditType action)
{
    for (auto edit : _tileRegister.at(_tileRegisterPosition))
    {
        Tile *tile = _save->getTile(edit.position);
        // Make sure the tile exists
        if (tile == 0)
        {
            continue;
        }

        if (action == MET_UNDO)
        {
            changeTileData(action, tile, edit.tileBeforeDataIDs, edit.tileBeforeDataSetIDs);
        }
        else if (action == MET_REDO)
        {
            changeTileData(action, tile, edit.tileAfterDataIDs, edit.tileAfterDataSetIDs);
        }
    }
}

/**
 * Changes node data for undo or redo actions
 * @action Type of action we're taking
 */
void MapEditor::undoRedoNodes(EditType action)
{
    // TODO: refactor for adding/removing nodes - that's where we'll need to know whether we're undoing or redoing
    // redoing means re-creating or re-deleting nodes, undoing means we swap create/delete actions
    for (auto edit : _nodeRegister.at(_nodeRegisterPosition))
    {
        Node *node = _save->getNodes()->at(edit.nodeID);
        // Make sure we're not working on non-existant nodes
        if (node == 0)
        {
            continue;
        }

        if (action == MET_UNDO)
        {
            changeNodeData(action, node, edit.nodeChangeType, edit.nodeBeforeData);
        }
        else if (action == MET_REDO)
        {
            changeNodeData(action, node, edit.nodeChangeType, edit.nodeAfterData);
        }
    }
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
 * Helper function for figuring out whether a node is linked to a certain other
 * Returns the index of the link if nodeToCheck is linked to the given ID or exit direction
 * Returns -1 if nodeToCheck is not linked to the given ID or exit direction
 */
int MapEditor::getConnectionIndex(Node *nodeToCheck, int nodeOrExitID)
{
    int connectionIndex = -1;
    for (int i = 0; i < 5; ++i)
    {
        if (nodeToCheck->getNodeLinks()->at(i) == nodeOrExitID)
        {
            connectionIndex = i;
            break;
        }
    }

    return connectionIndex;
}

/**
 * Helper to determine which direction outside the map a clicked position is
 * @param pos the clicked position
 * @return the direction: -1 = not a cardinal direction, -2 = north, -3 = east, -4 = south, -5 = west
 * matches direction of node links for exit directions
 */
int MapEditor::getExitLinkDirection(Position pos)
{
	int linkDirection = -1;
	// exit north
	if (pos.y < 0 && pos.x >= 0 && pos.x < _save->getMapSizeX())
	{
		linkDirection = -2;
	}
	// exit east
	else if (pos.x > _save->getMapSizeX() - 1 && pos.y >= 0 && pos.y < _save->getMapSizeY())
	{
		linkDirection = -3;
	}
	// exit south
	else if (pos.y > _save->getMapSizeY() - 1 && pos.x >= 0 && pos.x < _save->getMapSizeX())
	{
		linkDirection = -4;
	}
	// exit west
	else if (pos.x < 0 && pos.y >= 0 && pos.y < _save->getMapSizeY())
	{
		linkDirection = -5;
	}

	return linkDirection;
}

/**
 * Un-does an action pointed to by the current position in the edit register
 */
void MapEditor::undo(bool node)
{
    if (!node)
    {
        if (_tileRegisterPosition == 0)
        {
            return;
        }

        --_tileRegisterPosition;
        undoRedoTiles(MET_UNDO);
    }
    else
    {
        if (_nodeRegisterPosition == 0)
        {
            return;
        }

        --_nodeRegisterPosition;
        undoRedoNodes(MET_UNDO);
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
        {
            return;
        }

        undoRedoTiles(MET_REDO);
        ++_tileRegisterPosition;
    }
    else
    {
        if (_nodeRegisterPosition >= (int)_nodeRegister.size())
        {
            return;
        }

        undoRedoNodes(MET_REDO);
        ++_nodeRegisterPosition;
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
 * Marks a node as active or inactive
 * @param active true if the node should be marked as active
 */
void MapEditor::setNodeAsActive(Node *node, bool active)
{
    if (node)
    {
        _activeNodes[node->getID()] = active;
        _numberOfActiveNodes += active ? 1 : -1;
    }
}

/**
 * Gets whether a node is marked as active or not
 */
bool MapEditor::isNodeActive(Node *node)
{
    bool active = false;

    if (node)
    {
        std::map<int, bool>::iterator it = _activeNodes.find(node->getID());
        if (it != _activeNodes.end())
        {
            active = it->second;
        }
        // if we don't have the node in the list, mark it as active
        else
        {
            active = true;
            _activeNodes[node->getID()] = active;
            ++_numberOfActiveNodes;
        }
    }

    return active;
}

/**
 * Gets whether a node won't be saved due to being over the ID 250 limit
 * Links can only go up to ID 250 since they're saved as 8-bit numbers, and 251-255 are reserved values
 */
bool MapEditor::isNodeOverIDLimit(Node *node)
{
    if (_numberOfActiveNodes < 252) // ID is 0-indexed, so the 252nd node would be ID 251
    {
        return false;
    }
    else
    {
        int activeCount = 0;
        int lastIDToBeSaved = 0;
        for (size_t i = 0; i < _activeNodes.size(); i++) // we can iterate like this because node IDs are consecutive in the SavedBattleGame
        {
            if (_activeNodes[i])
            {
                ++activeCount;
                lastIDToBeSaved = i;
            }

            if (activeCount == 251)
            {
                return node->getID() > lastIDToBeSaved;
            }
        }
    }

    return false;
}

/**
 * Gets the number of nodes that are active
 */
int MapEditor::getNumberOfActiveNodes()
{
    return _numberOfActiveNodes;
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

    // Start by finding how many 24-byte nodes we need to allocate and which nodes are "active" and should be saved
    std::vector<Node*> nodesToSave;
    // Node IDs aren't saved into RMP files, so the index has to match position of the node in the file
    // We'll also create a map of current node IDs to where they should be in the RMP file and the proper connections for them
    std::map<int, int> nodeIDMap;
    int offset = 0;
    for (auto node : *_save->getNodes())
    {

        int currentID = node->getID();
        if (isNodeOverIDLimit(node))
        {
            nodeIDMap[currentID] = -1;
        }
        else if (isNodeActive(node))
        {
            nodesToSave.push_back(node);
            nodeIDMap[currentID] = currentID - offset;
        }
        else
        {
            nodeIDMap[currentID] = -1;
            ++offset;
        }
    }

    // Assign vector to hold the node data, 24 bytes per node (up to the max ID number we just found)
    data.clear();
    data.resize(24 * nodesToSave.size());

    for (auto node : nodesToSave)
    {
        size_t nodeIDBytePosition = 24 * nodeIDMap[node->getID()];
        data.at(nodeIDBytePosition + 0) = (unsigned char)node->getPosition().y;
        data.at(nodeIDBytePosition + 1) = (unsigned char)node->getPosition().x;
        data.at(nodeIDBytePosition + 2) = (unsigned char)(_save->getMapSizeZ() - 1 - node->getPosition().z);

        for (int i = 0; i < 5; ++i)
        {
            int link = node->getNodeLinks()->at(i);
            // remap the links to nodes so we match the positions of the nodes in the RMP file
            if (link >= 0)
            {
                link = nodeIDMap[link];
            }
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