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
enum NodeChangeType {NCT_NEW, NCT_DELETE, NCT_POS, NCT_TYPE, NCT_RANK, NCT_FLAG, NCT_PRIORITY, NCT_RESERVED, NCT_LINKS, NCT_LINKTYPES, NCT_NONE};

struct TileEdit
{
    Position position;
    int tileBeforeDataIDs[O_MAX];
    int tileBeforeDataSetIDs[O_MAX];
    int tileAfterDataIDs[O_MAX];
    int tileAfterDataSetIDs[O_MAX];

    // TODO: change to populate data from save? (do in actual editor, not here)
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

    // Test for whether an edit does anything
    // Used to prevent pushing back useless changes to the edit register
    bool isEditEmpty()
    {
        bool equal = true;
        for (int i = 0; i < O_MAX; ++i)
        {
            equal &= (tileBeforeDataIDs[i] == tileAfterDataIDs[i]);
            equal &= (tileBeforeDataSetIDs[i] == tileAfterDataSetIDs[i]);
        }
        return equal;
    }
};

struct NodeEdit
{
    // TODO: change magic number 5 to variable like O_MAX
    int nodeID;
    NodeChangeType nodeChangeType;
    std::vector< int > nodeBeforeData, nodeAfterData;

    // Constructor: get the info from the before node for initialization, let the editor populate the changes
    NodeEdit(int id, NodeChangeType changeType)
    {
        nodeID = id;
        nodeChangeType = changeType;
        nodeBeforeData.clear();
        nodeAfterData.clear();
    }

    // Test for whether an edit does anything
    // Used to prevent pushing back useless changes to the edit register
    bool isEditEmpty()
    {
        bool equal = true;
        if (nodeBeforeData.empty() || nodeAfterData.empty())
        {
            return equal;
        }

        for (int i = 0; i < (int)nodeBeforeData.size(); ++i)
        {
            equal &= (nodeBeforeData.at(i) == nodeAfterData.at(i));
        }
        return equal;
    }
};

class Action;
class Node;
class SavedBattleGame;
class Tile;

class MapEditor
{
private :
    SavedBattleGame *_save;
    std::vector< std::vector< TileEdit > > _tileRegister;
    std::vector< std::vector< NodeEdit > > _nodeRegister;
    int _selectedMapDataID, _tileRegisterPosition, _nodeRegisterPosition;
    std::vector< Tile* > _selectedTiles;
    std::vector< Node* > _selectedNodes;
    std::string _mapname;
    TilePart _selectedObject;
    std::map< int, int > _connectionIndexMap;

public :
    /// Creates the Map Editor
    MapEditor(SavedBattleGame *save);
    /// Cleans up the Map Editor
    ~MapEditor();
    /// Handles input passed to the Editor from the BattlescapeState
    void handleEditorInput(Action *action, Tile *tile);
    /// Handles inputs passed to the editor from the node information panels
    void handleNodeInput(Action *action, NodeChangeType changeType, std::vector<int> data);
    /// Changes tile data according to the selected tiles and map data
    void changeTiles(EditType action);
    /// Changes node data according to the selected nodes and route data
    void changeNodes(EditType action, NodeChangeType changeType, std::vector<int> data);
    /// Changes the data of a specific node according to the selected route data
    NodeEdit changeNodeData(Node *node, NodeChangeType changeType, std::vector<int> data);
    /// Helper function for figuring out which link is next open on a node
    int getNextNodeConnectionIndex(Node *node, bool advanceIndex = false);
    /// Un-does an action pointed to by the current position in the edit register
    void undo(bool node = false);
    /// Re-does an action pointed to by the current position in the edit register
    void redo(bool node = false);
    /// Helper function for getting MapData, MapDataSetIDs, and MapDataIDs from index of a tile
    MapData *getMapDataFromIndex(int index, int *mapDataSetID, int *mapDataID);
    /// Gets the current position of the tile edit register
    int getTileRegisterPosition();
    /// Gets the number of edits in the tile register
    int getTileRegisterSize();
    /// Gets the current position of the node edit register
    int getNodeRegisterPosition();
    /// Gets the number of edits in the node register
    int getNodeRegisterSize();
    /// Gets a pointer to the list of selected tiles for editing
    std::vector< Tile* > *getSelectedTiles();
    /// Gets a pointer to the list of selected nodes for editing
    std::vector< Node* > *getSelectedNodes();
    /// Sets the map data ID index selected
    void setSelectedMapDataID(int selectedIndex);
    /// Gets the map data ID index selected
    int getSelectedMapDataID();
    /// Sets the selected object type within a tile
    void setSelectedObject(TilePart part);
    /// Gets the object type within a tile (floor, walls, or object) selected
    TilePart getSelectedObject();
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