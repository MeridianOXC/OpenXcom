/*
 * Copyright 2010-2020 OpenXcom Developers.
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
#include <sstream>
#include "MapEditorFindTileState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Battlescape/MapEditor.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/Mod.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/RuleTerrain.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the find and replace tiles window for the map editor
 * @param game Pointer to the core game.
 */
MapEditorFindTileState::MapEditorFindTileState(int selectedTilePart, int selectedTileIndex) : _selectedTileFind(selectedTileIndex), _selectedTileReplace(-1)
{
    _screen = false;

    int windowX = 32;
    int windowY = 0;
    _window = new Window(this, 256, 200, 32, 0, POPUP_BOTH);

    _txtFind = new Text(256, 17, windowX, windowY + 9);

    _txtWithMCDEntry = new Text(118, 10, windowX + 9, windowY + 26);
    _txtInTilePartFind = new Text(118, 10, windowX + 43, windowY + 26);
    _txtActionFind = new Text(118, 10, windowX + 43, windowY + 72);

    _cbxTilePartFind = new ComboBox(this, 120, 16, windowX + 41, windowY + 36, false);
    _cbxCurrentSelection = new ComboBox(this, 120, 16, windowX + 41, windowY + 54, false);
    _cbxHandleSelection = new ComboBox(this, 120, 16, windowX + 41, windowY + 82, false);

    _btnFind = new TextButton(84, 16, windowX + 165, windowY + 82);

    _txtReplace = new Text(256, 17, windowX, windowY + 109);

    _txtInTilePartReplace = new Text(118, 10, windowX + 43, windowY + 144);

    _cbxClipBoardOrNot = new ComboBox(this, 120, 16, windowX + 41, windowY + 126, true);
    _cbxTilePartReplace = new ComboBox(this, 120, 16, windowX + 41, windowY + 154, true);
    _cbxHandleTileContents = new ComboBox(this, 120, 16, windowX + 41, windowY + 172, true);

    _btnReplace = new TextButton(84, 16, windowX + 165, windowY + 154);

    _btnCancel = new TextButton(84, 16, windowX + 165, windowY + 172);

	SurfaceSet *icons = _game->getMod()->getSurfaceSet("MapEditorIcons");

	_backgroundTileSelection = new InteractiveSurface(32, 160, windowX + 7, windowY + 36);
	icons->getFrame(15)->blitNShade(_backgroundTileSelection, 0, 0);
	icons->getFrame(15)->blitNShade(_backgroundTileSelection, 0, 126 - 36);

	/**_backgroundTileSelectionNavigation = new InteractiveSurface(96, 40, 32, 0);
	for (int i = 0; i < 3; ++i)
	{
		icons->getFrame(i + 16)->blitNShade(_backgroundTileSelectionNavigation, i * 32, 0);
	}
	_tileSelectionColumns = screenWidth / 2 / 32;
	_tileSelectionColumns = std::max(2, std::min(_tileSelectionColumns, Options::mapEditorMaxTileSelectionColumns));
	_tileSelectionRows = (screenHeight - 2 * 40) / 40;
	_tileSelectionRows = std::max(2, std::min(_tileSelectionRows, Options::mapEditorMaxTileSelectionRows));
	int tileSelectionWidth = _tileSelectionColumns * 32;
	int tileSelectionHeight = _tileSelectionRows * 40;
	_tileSelection = new InteractiveSurface(32, 40, 0, 0);
	_panelTileSelection = new InteractiveSurface(tileSelectionWidth, tileSelectionHeight, 0, 40);
	_tileSelectionPageCount = new BattlescapeButton(32, 12, 64, 28);
	_tileSelectionLeftArrow = new BattlescapeButton(32, 12, 32, 28);
	_tileSelectionRightArrow = new BattlescapeButton(32, 12, 96, 28);
	_txtSelectionPageCount = new Text(32, 12, 64, 28);
	_tileSelectionGrid.clear();
	for (int i = 0; i < _tileSelectionRows; ++i)
	{
		for (int j = 0; j < _tileSelectionColumns; ++j)
		{
			// select which of the background panel frames is appropriate for this position on the grid
			int panelSpriteOffset = 20;
			if (i % (_tileSelectionRows - 1) != 0) // we're in a middle row
				panelSpriteOffset += 3;
			else if (i / (_tileSelectionRows - 1) == 1) // we're on the bottom row
				panelSpriteOffset += 6;
			// else we're on the top row

			if (j % (_tileSelectionColumns - 1) != 0) // we're in a middle column
				panelSpriteOffset += 1;
			else if (j / (_tileSelectionColumns - 1) == 1) // we're on the right edge
				panelSpriteOffset += 2;
			// else we're on the left edge

			// draw the background
			icons->getFrame(panelSpriteOffset)->blitNShade(_panelTileSelection, j * 32, i * 40);

			_tileSelectionGrid.push_back(new InteractiveSurface(32, 40, j * 32, i * 40 + 40));
		}
	}**/

	//TextButton *_btnFind;
    //BattlescapeButton *_tileObjectSelectedFind;
    //Text *_txtReplace, *_txtInTilePartReplace;
    //TextButton *_btnReplace *_btnCancel;
    //BattlescapeButton *_tileObjectSelectedReplace;
    //ComboBox *_cbxClipBoardOrNot, *_cbxTilePartReplace, *_cbxHandleTileContents;
	//InteractiveSurface *_backgroundTileSelection, *_tileSelection;
	//InteractiveSurface *_panelTileSelection;
	//InteractiveSurface *_backgroundTileSelectionNavigation;
	//BattlescapeButton *_tileSelectionLeftArrow, *_tileSelectionRightArrow, *_tileSelectionPageCount;
	//Text *_txtSelectionPageCount;
	//std::vector<InteractiveSurface*> _tileSelectionGrid;

	// Set palette
	setInterface("optionsMenu", false, _game->getSavedGame()->getSavedBattle());

    add(_window, "window", "optionsMenu");
    add(_txtFind, "text", "optionsMenu");
    add(_txtWithMCDEntry, "text", "optionsMenu");
    add(_txtInTilePartFind, "text", "optionsMenu");
    add(_txtActionFind, "text", "optionsMenu");
    add(_btnFind, "button", "optionsMenu");
    add(_txtReplace, "text", "optionsMenu");
    add(_txtInTilePartReplace, "text", "optionsMenu");
    add(_btnReplace, "button", "optionsMenu");
	add(_backgroundTileSelection);
    // add combo boxes in reverse order so they properly layer over each other when open
    add(_cbxClipBoardOrNot, "infoBoxOKButton", "battlescape");
    add(_cbxTilePartReplace, "infoBoxOKButton", "battlescape");
    add(_cbxHandleTileContents, "infoBoxOKButton", "battlescape");
	add(_cbxHandleSelection, "infoBoxOKButton", "battlescape");
	add(_cbxCurrentSelection, "infoBoxOKButton", "battlescape");
	add(_cbxTilePartFind, "infoBoxOKButton", "battlescape");
    add(_btnCancel, "button", "optionsMenu");

    centerAllSurfaces();

    // Set up objects
	applyBattlescapeTheme();
	setWindowBackground(_window, "mainMenu");

	_txtFind->setAlign(ALIGN_CENTER);
	_txtFind->setBig();
	_txtFind->setText("FIND TILES"); //tr("STR_MAP_EDITOR_FIND_TILES"));

    _txtWithMCDEntry->setText("With:"); //tr("STR_MAP_EDITOR_FIND_TILES_WITH")
    _txtInTilePartFind->setText("In Tile Part:");
    _txtActionFind->setText("Then:");

    _btnFind->setText("Find");

	_txtReplace->setAlign(ALIGN_CENTER);
	_txtReplace->setBig();
	_txtReplace->setText("AND REPLACE WITH");

    _txtInTilePartReplace->setText("In Tile Part:");

    _btnReplace->setText("Replace");

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorFindTileState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorFindTileState::btnCancelClick, Options::keyCancel);

    std::vector<std::string> lstOptions;
    lstOptions.clear();
    lstOptions.push_back("Floor");
    lstOptions.push_back("West Wall");
    lstOptions.push_back("North Wall");
    lstOptions.push_back("Object");
    lstOptions.push_back("Any");
    _cbxTilePartFind->setOptions(lstOptions, false);
    _cbxTilePartFind->setSelected(selectedTilePart);
    _cbxTilePartReplace->setOptions(lstOptions, false);
    _cbxTilePartReplace->setSelected(selectedTilePart);

    lstOptions.clear();
    lstOptions.push_back("Anywhere");
    lstOptions.push_back("Inside Selection"); // disable if no selection
    lstOptions.push_back("Outside Selection");
    _cbxCurrentSelection->setOptions(lstOptions, false);
    _cbxCurrentSelection->setSelected(0);

    lstOptions.clear();
    lstOptions.push_back("Create New Selection");
    lstOptions.push_back("Add to Selection"); // disable if no selection
    lstOptions.push_back("Remove from Selection");
    _cbxHandleSelection->setOptions(lstOptions, false);
    _cbxHandleSelection->setSelected(0);
    
    lstOptions.clear();
    lstOptions.push_back("Selected Tile from MCD");
    lstOptions.push_back("First Tile in Clipboard"); // disable in clipboard empty
    _cbxClipBoardOrNot->setOptions(lstOptions, false);
    _cbxClipBoardOrNot->setSelected(0);

    lstOptions.clear();
    lstOptions.push_back("Don't Clear First");
    lstOptions.push_back("Clear Tile First");
    _cbxHandleTileContents->setOptions(lstOptions, false);
    _cbxHandleTileContents->setSelected(0);
}

/**
 * Cleans up the state
 */
MapEditorFindTileState::~MapEditorFindTileState()
{

}

/**
 * Returns to the previous menu
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::btnCancelClick(Action *)
{
    _game->popState();
}

}