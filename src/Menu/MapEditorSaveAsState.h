
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
#include <string>

namespace OpenXcom
{

class TextButton;
class TextEdit;
class Window;
class Text;

class MapEditorSaveAsState : public State
{
private :
	Window *_window;
	Text *_txtTitle, *_txtMapName;
	TextButton *_btnOk, *_btnCancel;
    TextEdit *_edtMapName;
    std::string _mapName;

public :
    /// Creates the New Map size window
    MapEditorSaveAsState();
    /// Cleans up the New Map size window
    ~MapEditorSaveAsState();
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