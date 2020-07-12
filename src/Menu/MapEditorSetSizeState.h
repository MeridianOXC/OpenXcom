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
#include <map>
#include <string>

namespace OpenXcom
{

class MapEditorMenuState;
class TextButton;
class TextEdit;
class Window;
class Text;

class MapEditorSetSizeState : public State
{
private :
    MapEditorMenuState *_parent;
	Window *_window;
	Text *_txtTitle, *_txtNote, *_txtX, *_txtY, *_txtZ, *_txtMapName;
	TextButton *_btnOk, *_btnCancel;
    TextEdit *_edtX, *_edtY, *_edtZ, *_edtMapName;
    int _x, _y, _z;
    std::map<TextEdit*, int*> _editMap;
    std::string _mapName;

public :
    /// Creates the New Map size window
    MapEditorSetSizeState(MapEditorMenuState *parent);
    /// Cleans up the New Map size window
    ~MapEditorSetSizeState();
    /// Handles editing the text windows for the sizes
    void edtSizeOnChange(Action *action);
    /// Handles entering a name for the map
    void edtMapNameOnChange(Action *action);
    /// Clears the text for the map name when first clicking on it
    void edtMapNameOnClick(Action *action);
    /// Confirms the sizes and starts the map editor
    void btnOkClick(Action *action);
    /// Returns to the map editor menu
    void btnCancelClick(Action *action);

};

}