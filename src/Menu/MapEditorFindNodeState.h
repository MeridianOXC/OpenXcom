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

class MapEditorState;
class Window;
class SavedBattleGame;
class Text;
class TextButton;
class BattlescapeButton;
class ComboBox;
class InteractiveSurface;

class MapEditorFindNodeState : public State
{
private :
    MapEditorState *_mapEditorState;
	Window *_window;
    SavedBattleGame *_save;
	Text *_txtFind, *_txtWithMCDEntry, *_txtInTilePartFind, *_txtActionFind;
	TextButton *_btnFind;
    InteractiveSurface *_tileObjectSelectedFind;
    ComboBox *_cbxTilePartFind, *_cbxCurrentSelection, *_cbxHandleSelection;
    Text *_txtReplace, *_txtInTilePartReplace;
    TextButton *_btnReplace, *_btnCancel;
    InteractiveSurface *_tileObjectSelectedReplace;
    ComboBox *_cbxClipBoardOrNot, *_cbxTilePartReplace, *_cbxHandleTileContents;
	InteractiveSurface *_backgroundTileSelection;
	InteractiveSurface *_panelTileSelection;
	InteractiveSurface *_backgroundTileSelectionNavigation;
	BattlescapeButton *_tileSelectionLeftArrow, *_tileSelectionRightArrow, *_tileSelectionPageCount;
	Text *_txtSelectionPageCount;
	std::vector<InteractiveSurface*> _tileSelectionGrid;
    InteractiveSurface *_clickedTileButton;
    int _selectedTileFind, _selectedTileReplace;
    int _tileSelectionRows, _tileSelectionColumns, _tileSelectionCurrentPage, _tileSelectionLastPage;

public :
    /// Creates the Map Editor Info window
    MapEditorFindNodeState(MapEditorState *mapEditorState, int selectedTilePart, int selectedTileIndex);
    /// Creates the Map Editor Info window
    ~MapEditorFindNodeState();
    /// Handles clicking the find or replace buttons
    void btnFindClick(Action *action);
    /// Returns to the previous menu
    void btnCancelClick(Action *action);
    /// Selects tiles according to the parameters chosen
    void selectTiles(); 
    /// Replaces tiles according to the parameters chosen
    void replaceTiles();
	/// Toggles the tile selection UI
	void tileSelectionClick(Action *action);
	/// Draws the tile sprites on the selection grid
	void drawTileSelectionGrid();
	/// Moves the tile selection UI left one page
	void tileSelectionLeftArrowClick(Action *action);
	/// Moves the tile selection UI right one page
	void tileSelectionRightArrowClick(Action *action);
	/// Selects the tile from the tile selection UI
	void tileSelectionGridClick(Action *action);
	/// Handles mouse wheel scrolling for the tile selectionUI
	void tileSelectionMousePress(Action *action);
	/// Draws a tile sprite on a given surface
	bool drawTileSpriteOnSurface(Surface *surface, int index);
};

}