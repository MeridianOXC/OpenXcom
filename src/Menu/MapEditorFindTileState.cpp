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
#include "../Battlescape/Map.h"
#include "../Battlescape/MapEditor.h"
#include "../Battlescape/MapEditorState.h"
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
MapEditorFindTileState::MapEditorFindTileState(MapEditorState *mapEditorState, int selectedTilePart, int selectedTileIndex) : _mapEditorState(mapEditorState), _selectedTileFind(selectedTileIndex), _selectedTileReplace(0), _tileSelectionCurrentPage(0)
{
    _screen = false;

    int windowX = 32;
    int windowY = 0;
    _window = new Window(this, 256, 200, 32, 0, POPUP_BOTH);

    _save = _game->getSavedGame()->getSavedBattle();

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

	_backgroundTileSelectionNavigation = new InteractiveSurface(96, 40, 112, -14);
	for (int i = 0; i < 3; ++i)
	{
		icons->getFrame(i + 16)->blitNShade(_backgroundTileSelectionNavigation, i * 32, 0);
	}
	_tileSelectionColumns = 10;
	_tileSelectionRows = 4;
	int tileSelectionWidth = _tileSelectionColumns * 32;
	int tileSelectionHeight = _tileSelectionRows * 40;
	_tileObjectSelectedFind = new InteractiveSurface(32, 40, windowX + 7, windowY + 36);
	_tileObjectSelectedReplace = new InteractiveSurface(32, 40, windowX + 7, windowY + 126);
	_panelTileSelection = new InteractiveSurface(tileSelectionWidth, tileSelectionHeight, 0, 26);
	_tileSelectionPageCount = new BattlescapeButton(32, 12, 144, 14);
	_tileSelectionLeftArrow = new BattlescapeButton(32, 12, 112, 14);
	_tileSelectionRightArrow = new BattlescapeButton(32, 12, 176, 14);
	_txtSelectionPageCount = new Text(32, 12, 144, 14);
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

			_tileSelectionGrid.push_back(new InteractiveSurface(32, 40, j * 32, i * 40 + 26));
		}
	}

	// Set palette
	setInterface("optionsMenu", false, _save);

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

	add(_backgroundTileSelectionNavigation);
	add(_tileObjectSelectedFind, "", "battlescape", _backgroundTileSelection);
	add(_tileObjectSelectedReplace, "", "battlescape", _backgroundTileSelection);
	add(_panelTileSelection);
	add(_tileSelectionPageCount, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_tileSelectionLeftArrow, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_tileSelectionRightArrow, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_txtSelectionPageCount);
	for (auto i : _tileSelectionGrid)
	{
		add(i);
		i->onMouseClick((ActionHandler)&MapEditorFindTileState::tileSelectionGridClick);
	}

    centerAllSurfaces();

    // Set up objects
	applyBattlescapeTheme();
	setWindowBackground(_window, "mainMenu");

	_txtFind->setAlign(ALIGN_CENTER);
	_txtFind->setBig();
	_txtFind->setText("FIND TILES"); //tr("STR_MAP_game->getMapEditor()_FIND_TILES"));

    _txtWithMCDEntry->setText("With:"); //tr("STR_MAP_game->getMapEditor()_FIND_TILES_WITH")
    _txtInTilePartFind->setText("In Tile Part:");
    _txtActionFind->setText("Then:");

    _btnFind->setText("Find");
	_btnFind->onMouseClick((ActionHandler)&MapEditorFindTileState::btnFindClick);
	_btnFind->onKeyboardPress((ActionHandler)&MapEditorFindTileState::btnFindClick, Options::keyOk);

	_txtReplace->setAlign(ALIGN_CENTER);
	_txtReplace->setBig();
	_txtReplace->setText("AND REPLACE WITH");

    _txtInTilePartReplace->setText("In Tile Part:");

    _btnReplace->setText("Replace");

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorFindTileState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorFindTileState::btnCancelClick, Options::keyCancel);

	//_tileSelection->setColor(232); // Goal of background color 235
	_tileObjectSelectedFind->onMouseClick((ActionHandler)&MapEditorFindTileState::tileSelectionClick);
	_tileObjectSelectedReplace->onMouseClick((ActionHandler)&MapEditorFindTileState::tileSelectionClick);

	int mapDataObjects = 0;
	for (auto mapDataSet : *_save->getMapDataSets())
	{
		mapDataObjects += mapDataSet->getSize();
	}
	_tileSelectionLastPage = mapDataObjects / (_tileSelectionColumns * _tileSelectionRows);
	_txtSelectionPageCount->setSmall();
	_txtSelectionPageCount->setAlign(ALIGN_CENTER);
	_txtSelectionPageCount->setVerticalAlign(ALIGN_MIDDLE);
	_txtSelectionPageCount->setWordWrap(true);
	_txtSelectionPageCount->setColor(224);
	_txtSelectionPageCount->setHighContrast(true);
	std::ostringstream ss;
	ss << _tileSelectionCurrentPage + 1 << "/" << _tileSelectionLastPage + 1;
	_txtSelectionPageCount->setText(ss.str().c_str());
	_txtSelectionPageCount->setVisible(false);

	_tileSelectionPageCount->setVisible(false);
	_tileSelectionPageCount->onMousePress((ActionHandler)&MapEditorFindTileState::tileSelectionMousePress);

	if (_tileSelectionCurrentPage == _tileSelectionLastPage)
	{
		size_t arrowColor = 1;
		icons->getFrame(16)->blitNShade(_tileSelectionLeftArrow, 0, -28, 0, false, arrowColor);
		icons->getFrame(18)->blitNShade(_tileSelectionRightArrow, 0, -28, 0, false, arrowColor);
	}

	_tileSelectionLeftArrow->onMouseClick((ActionHandler)&MapEditorFindTileState::tileSelectionLeftArrowClick);
	_tileSelectionLeftArrow->onMousePress((ActionHandler)&MapEditorFindTileState::tileSelectionMousePress);
	_tileSelectionLeftArrow->setVisible(false);

	_tileSelectionRightArrow->onMouseClick((ActionHandler)&MapEditorFindTileState::tileSelectionRightArrowClick);
	_tileSelectionRightArrow->onMousePress((ActionHandler)&MapEditorFindTileState::tileSelectionMousePress);
	_tileSelectionRightArrow->setVisible(false);

	//_panelTileSelection->setColor(232); // nice purple
	_panelTileSelection->onMousePress((ActionHandler)&MapEditorFindTileState::tileSelectionMousePress);
	drawTileSelectionGrid();
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(false);
	}
	_panelTileSelection->setVisible(false);

	_backgroundTileSelectionNavigation->setVisible(false);

    _selectedTileFind = std::max(0, _selectedTileFind);
    drawTileSpriteOnSurface(_tileObjectSelectedFind, _selectedTileFind);
    drawTileSpriteOnSurface(_tileObjectSelectedReplace, _selectedTileReplace);

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
 * Handles clicking the find button
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::btnFindClick(Action *action)
{
    selectTiles();
    _game->popState();
}

/**
 * Handles clicking the replace button
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::btnReplaceClick(Action *action)
{
    selectTiles();
    replaceTiles();
    _game->popState();
}

/**
 * Returns to the previous menu
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::btnCancelClick(Action *action)
{
	// Pressing escape closes the tile selection UI first
	if (_panelTileSelection->getVisible() && action->getDetails()->type == SDL_KEYDOWN)
	{
		tileSelectionClick(action);
		return;
	}

    _game->popState();
}

/**
 * Selects tiles according to the parameters chosen
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::selectTiles()
{
    // clear the current selection if that's what was selected
    if (_cbxHandleSelection->getSelected() == 0)
    {
        _game->getMapEditor()->getSelectedTiles()->clear();
    }

    int dataIDToCheck = -1;
    int dataSetIDToCheck = -1;
    _game->getMapEditor()->getMapDataFromIndex(_selectedTileFind, &dataSetIDToCheck, &dataIDToCheck);

    for (int z = 0; z < _save->getMapSizeZ(); z++)
    {
        for (int y = 0; y < _save->getMapSizeY(); y++)
        {
            for (int x = 0; x < _save->getMapSizeX(); x++)
            {
                Tile *tile = _save->getTile(Position(x, y, z));
                std::vector<Tile*>::iterator it = std::find_if(_game->getMapEditor()->getSelectedTiles()->begin(), _game->getMapEditor()->getSelectedTiles()->end(), 
                                                        [&](const Tile *t){ return t == tile; });

                // We can skip this tile if we're looking only inside the current selection and it's outside
                // or we're looking outside the current selection and it's inside
                if ((_cbxCurrentSelection->getSelected() == 1 && it == _game->getMapEditor()->getSelectedTiles()->end())
                    || (_cbxCurrentSelection->getSelected() == 2 && it != _game->getMapEditor()->getSelectedTiles()->end()))
                {
                    continue;
                }

                // Check the tile part filter and selected MCD
                bool isTileAMatch = false;
                std::vector<TilePart> tileParts;
                if (_cbxTilePartFind->getSelected() == O_MAX)
                {
                    tileParts = {O_FLOOR, O_WESTWALL, O_NORTHWALL, O_OBJECT};
                }
                else
                {
                    tileParts = {(TilePart)_cbxTilePartFind->getSelected()};
                }
                for (auto part : tileParts)
                {
                    int dataID;
                    int dataSetID;
                    tile->getMapData(&dataID, &dataSetID, part);

                    isTileAMatch = isTileAMatch || (dataID == dataIDToCheck && dataSetID == dataSetIDToCheck);
                }

                // this tile is what we're looking for! now add/remove from selection as requested
                if (isTileAMatch)
                {
                    // add to current selection
                    if (_cbxHandleSelection->getSelected() != 2 && it == _game->getMapEditor()->getSelectedTiles()->end())
                    {
                        _game->getMapEditor()->getSelectedTiles()->push_back(tile);
                    }
                    // remove from current selection
                    else if (_cbxHandleSelection->getSelected() == 2 && it != _game->getMapEditor()->getSelectedTiles()->end())
                    {
                        _game->getMapEditor()->getSelectedTiles()->erase(it);
                    }
                }
            }
        }
    }

    _mapEditorState->getMap()->resetObstacles();
    if (Options::mapEditorSelectedTilesKeepFlashing)
    {
        for (auto tile : *_game->getMapEditor()->getSelectedTiles())
        {
            for (int i = 0; i < O_MAX; ++i)
            {
                tile->setObstacle(i);
            }
        }							
    }
}

/**
 * Replaces tiles according to the parameters chosen
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::replaceTiles()
{

}

/**
 * Toggles the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::tileSelectionClick(Action *action)
{
    bool closePanel = _panelTileSelection->getVisible();

    if (!closePanel && (action->getSender() == _tileObjectSelectedFind || action->getSender() == _tileObjectSelectedReplace))
    {
        _clickedTileButton = action->getSender();
    }
    else
    {
        _clickedTileButton = 0;
    }

	_btnFind->setVisible(closePanel);
    _cbxTilePartFind->setVisible(closePanel);
    _cbxCurrentSelection->setVisible(closePanel);
    _cbxHandleSelection->setVisible(closePanel);
    _btnReplace->setVisible(closePanel);
    int screenHeight = Options::baseYResolution;
    _btnCancel->setY(closePanel ? (screenHeight - _window->getHeight()) / 2 + 172 : -screenHeight / 2); // just move the cancel button off the screen so it can still handle pressing escape to close the panel
    _cbxClipBoardOrNot->setVisible(closePanel);
    _cbxTilePartReplace->setVisible(closePanel);
    _cbxHandleTileContents->setVisible(closePanel);

    _tileObjectSelectedFind->setVisible(closePanel);
    _tileObjectSelectedReplace->setVisible(closePanel);

	_backgroundTileSelectionNavigation->setVisible(!closePanel);
	_txtSelectionPageCount->setVisible(!closePanel);
	_tileSelectionPageCount->setVisible(!closePanel);
	_tileSelectionLeftArrow->setVisible(!closePanel);
	_tileSelectionRightArrow->setVisible(!closePanel);
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(!closePanel);
	}
	_panelTileSelection->setVisible(!closePanel);

	drawTileSpriteOnSurface(_tileObjectSelectedFind, _selectedTileFind);
	drawTileSpriteOnSurface(_tileObjectSelectedReplace, _selectedTileReplace);
}

/**
 * Draws the tile images on the selection grid
 */
void MapEditorFindTileState::drawTileSelectionGrid()
{
	for (int i = 0; i < (int)_tileSelectionGrid.size(); ++i)
	{
		_tileSelectionGrid.at(i)->draw();
		drawTileSpriteOnSurface(_tileSelectionGrid.at(i), i + _tileSelectionCurrentPage * _tileSelectionRows * _tileSelectionColumns);
	}
}

/**
 * Moves the tile selection UI left one page
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::tileSelectionLeftArrowClick(Action *action)
{
	if (_tileSelectionCurrentPage == 0)
		return;

	--_tileSelectionCurrentPage;
	drawTileSelectionGrid();
	std::ostringstream ss;
	ss << _tileSelectionCurrentPage + 1 << "/" << _tileSelectionLastPage + 1;
	_txtSelectionPageCount->setText(ss.str().c_str());
}

/**
 * Moves the tile selection UI right one page
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::tileSelectionRightArrowClick(Action *action)
{
	if (_tileSelectionCurrentPage == _tileSelectionLastPage)
		return;

	++_tileSelectionCurrentPage;
	drawTileSelectionGrid();
	std::ostringstream ss;
	ss << _tileSelectionCurrentPage + 1 << "/" << _tileSelectionLastPage + 1;
	_txtSelectionPageCount->setText(ss.str().c_str());
}

/**
 * Selects the tile from the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::tileSelectionGridClick(Action *action)
{
    int index = 0;
    for (auto i : _tileSelectionGrid)
    {
        if (i == action->getSender())
        {
            break;
        }

        ++index;
    }
	index += _tileSelectionCurrentPage * _tileSelectionRows * _tileSelectionColumns;

	if (drawTileSpriteOnSurface(_clickedTileButton, index))
	{
		if (_clickedTileButton == _tileObjectSelectedFind)
        {
            _selectedTileFind = index;
        }
        else if (_clickedTileButton == _tileObjectSelectedReplace)
        {
            _selectedTileReplace = index;
        }
	}

	tileSelectionClick(action);
}

/**
 * Handles mouse wheel scrolling of the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorFindTileState::tileSelectionMousePress(Action *action)
{
    if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
    {
        tileSelectionLeftArrowClick(action);
        // Consume the event so the map doesn't scroll up or down
        //action->getDetails()->type = SDL_NOEVENT;
    }
    else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
    {
        tileSelectionRightArrowClick(action);
        // Consume the event so the map doesn't scroll up or down
        //action->getDetails()->type = SDL_NOEVENT;
    }
}

/**
 * Draws a tile sprite on a given surface
 * @param surface Pointer to the surface.
 * @param index Index of the tile object.
 * @return Whether or not we found and drew the proper tile object.
 */
bool MapEditorFindTileState::drawTileSpriteOnSurface(Surface *surface, int index)
{
	int mapDataSetID = 0;
	int mapDataID = 0;
	MapData *mapData = _game->getMapEditor()->getMapDataFromIndex(index, &mapDataSetID, &mapDataID);

	if (surface && mapData)
	{
		surface->draw();
		_save->getMapDataSets()->at(mapDataSetID)->getSurfaceset()->getFrame(mapData->getSprite(0))->blitNShade(surface, 0, 0);
		return true;
	}

	return false;
}

}