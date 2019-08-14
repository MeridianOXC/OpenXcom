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
class TextList;

class MapEditorInfoState : public State
{
private :
	Window *_window;
	Text *_txtTitle, *_txtName, *_txtSize, *_txtTerrain;
    TextList *_lstTerrain;
	TextButton *_btnReturn;

public :
    /// Creates the Map Editor Info window
    MapEditorInfoState();
    /// Creates the Map Editor Info window
    ~MapEditorInfoState();
    /// Returns to the previous menu
    void btnReturnClick(Action *action);

};

}