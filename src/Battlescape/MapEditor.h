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

struct TileEdit
{
    Position position;
    int tileBeforeDataIDs[O_MAX];
    int tileBeforeDataSetIDs[O_MAX];
    int tileAfterDataIDs[O_MAX];
    int tileAfterDataSetIDs[O_MAX];
};

class MapEditor
{
private :
    std::vector< std::vector< TileEdit > > _editRegister;
    int _selectedMapDataID;

public :
    /// Creates the Map Editor
    MapEditor();
    /// Cleans up the Map Editor
    ~MapEditor();
    /// Sets the map data ID index selected
    void setSelectedMapDataID(int selectedIndex);
    /// Gets the map data ID index selected
    int getSelectedMapDataID();

};

}