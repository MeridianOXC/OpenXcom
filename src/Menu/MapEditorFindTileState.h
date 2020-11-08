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

class Window;
class Text;
class TextButton;
class BattlescapeButton;
class ComboBox;
class InteractiveSurface;

class MapEditorFindTileState : public State
{
private :
	Window *_window;
	Text *_txtFind, *_txtWithMCDEntry, *_txtInTilePartFind, *_txtActionFind;
	TextButton *_btnFind;
    InteractiveSurface *_tileObjectSelectedFind;
    ComboBox *_cbxTilePartFind, *_cbxCurrentSelection, *_cbxHandleSelection;
    Text *_txtReplace, *_txtInTilePartReplace;
    TextButton *_btnReplace, *_btnCancel;
    InteractiveSurface *_tileObjectSelectedReplace;
    ComboBox *_cbxClipBoardOrNot, *_cbxTilePartReplace, *_cbxHandleTileContents;
	InteractiveSurface *_backgroundTileSelection, *_tileSelection;
	InteractiveSurface *_panelTileSelection;
	InteractiveSurface *_backgroundTileSelectionNavigation;
	BattlescapeButton *_tileSelectionLeftArrow, *_tileSelectionRightArrow, *_tileSelectionPageCount;
	Text *_txtSelectionPageCount;
	std::vector<InteractiveSurface*> _tileSelectionGrid;
    int _selectedTileFind, _selectedTileReplace;

public :
    /// Creates the Map Editor Info window
    MapEditorFindTileState(int selectedTilePart, int selectedTileIndex);
    /// Creates the Map Editor Info window
    ~MapEditorFindTileState();
    /// Returns to the previous menu
    void btnCancelClick(Action *action);

};

}