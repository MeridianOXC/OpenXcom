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
#include "../Engine/State.h"
#include <vector>
#include <string>

namespace OpenXcom
{

class TextButton;
class TextList;
class Window;
class Text;
class Frame;
class Position;

class MapEditorMenuState : public State
{
private :
	Window *_window;
	Text *_txtTitle, *_txtSelectedMap, *_txtSelectedMapTerrain;
    TextButton *_filterTerrain, *_filterCraft, *_filterUFOs, *_mapFilter;
	TextButton *_btnOk, *_btnCancel, *_btnNew;
    TextList *_lstMaps;
    Frame *_frameLeft, *_frameRight;
    std::vector< std::pair<std::string, std::string> > _mapsList;
    int _selectedMap;
    bool _newMapMode;
    std::string _newMapName;
    int _newMapX, _newMapY, _newMapZ;

public :
    /// Creates the Map Editor Menu State
    MapEditorMenuState();
    /// Cleans up the Map Editor Menu State
    ~MapEditorMenuState();
    /// Initializes the data in the Map Editor Menu
    void init();
    /// Populates the list of available maps
    void populateMapsList();
    /// Populates the list of available terrains
    void populateTerrainsList();
    /// Handles clicking on the filter buttons for available maps
    void btnMapFilterClick(Action *action);
    /// Handles clicking on the list of available maps
    void lstMapsClick(Action *action);
    /// Handles clicking the OK button
    void btnOkClick(Action *action);
    /// Starts the Map Editor
    void startEditor();
    /// Returns to the Main Menu
    void btnCancelClick(Action *action);
    /// Handles toggling the new map mode
    void btnNewMapClick(Action *action);
    /// Sets the information necessary for a new map
    void setNewMapInformation(std::string newMapName, int newMapX, int newMapY, int newMapZ);

};

}