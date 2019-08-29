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
#include <algorithm>
#include <sstream>
#include <string> // TODO remove later
#include <iomanip>
#include <SDL_gfxPrimitives.h>
#include "Map.h"
#include "Camera.h"
#include "TileEngine.h"
#include "MapEditor.h"
#include "MapEditorState.h"
#include "../lodepng.h"
#include "../fmath.h"
#include "../Geoscape/SelectMusicTrackState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Action.h"
#include "../Engine/Script.h"
#include "../Engine/Logger.h"
#include "../Engine/Timer.h"
#include "../Engine/CrossPlatform.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Menu/MapEditorOptionsState.h"
#include "../Menu/MapEditorInfoState.h"
#include "../Menu/MapEditorSaveAsState.h"
#include "../Mod/MapData.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/Mod.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Mod/RuleInterface.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Initializes all the elements in the Map Editor screen.
 * @param game Pointer to the core game.
 * @param editor Pointer to the data structure for the in-game map editor
 */
MapEditorState::MapEditorState(MapEditor *editor) : _firstInit(true), _isMouseScrolling(false), _isMouseScrolled(false), _xBeforeMouseScrolling(0), _yBeforeMouseScrolling(0), _totalMouseMoveX(0), _totalMouseMoveY(0), _mouseMovedOverThreshold(0), _mouseOverIcons(false), _autosave(false),
	_editor(editor), _tileSelectionColumns(5), _tileSelectionRows(3), _tileSelectionCurrentPage(0), _tileSelectionLastPage(0), _selectedTileIndex(0)
{
	const int screenWidth = Options::baseXResolution;
	const int screenHeight = Options::baseYResolution;

	_tooltipDefaultColor = _game->getMod()->getInterface("battlescape")->getElement("textTooltip")->color;

	// Create the battlemap view
	_map = new Map(_game, screenWidth, screenHeight, 0, 0, screenHeight);

	// Create buttons
	//_btnMapUp = new BattlescapeButton(32, 16, x + 80, y);
	//_btnMapDown = new BattlescapeButton(32, 16, x + 80, y + 16);

	_txtTooltip = new Text(300, 10, 2, screenHeight - 50);
	_txtDebug = new Text(158, 30, screenWidth - 160, 40);

	SurfaceSet *icons = _game->getMod()->getSurfaceSet("MapEditorIcons");

	// TODO: create interface ruleset for the map editor for all these hardcoded colors
	_iconsLowerLeft = new InteractiveSurface(160, 40, 0, screenHeight - 40);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i)->blitNShade(_iconsLowerLeft, i * 32, 0);
	}
	_btnOptions = new BattlescapeButton(32, 40, 0, screenHeight - 40);
	_btnOptions->setColor(232);
	_btnSave = new BattlescapeButton(32, 40, 32, screenHeight - 40);
	_btnSave->setColor(232);
	_btnLoad = new BattlescapeButton(32, 40, 64, screenHeight - 40);
	_btnLoad->setColor(232);
	_btnUndo = new BattlescapeButton(32, 40, 96, screenHeight - 40);
	_btnUndo->setColor(232);
	_btnRedo = new BattlescapeButton(32, 40, 128, screenHeight - 40);
	_btnRedo->setColor(232);

	_iconsLowerRight = new InteractiveSurface(160, 40, screenWidth - 160, screenHeight - 40);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i + 5)->blitNShade(_iconsLowerRight, i * 32, 0);
	}
	_btnFill = new BattlescapeButton(32, 40, screenWidth - 160, screenHeight - 40);
	_btnFill->setColor(232);
	_btnClear = new BattlescapeButton(32, 40, screenWidth - 128, screenHeight - 40);
	_btnClear->setColor(232);
	_btnCut = new BattlescapeButton(32, 40, screenWidth - 96, screenHeight - 40);
	_btnCut->setColor(232);
	_btnCopy = new BattlescapeButton(32, 40, screenWidth - 64, screenHeight - 40);
	_btnCopy->setColor(232);
	_btnPaste = new BattlescapeButton(32, 40, screenWidth - 32, screenHeight - 40);
	_btnPaste->setColor(232);

	_tileEditMode = _btnClear;
	_btnFill->setGroup(&_tileEditMode);
	_btnClear->setGroup(&_tileEditMode);

	_iconsUpperRight = new InteractiveSurface(160, 40, screenWidth - 160, 0);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i + 10)->blitNShade(_iconsUpperRight, i * 32, 0);
	}
	_btnSelectedTile = new BattlescapeButton(32, 40, screenWidth - 160, 0);
	_btnSelectedTile->setColor(232);
	_btnTileFilterGround = new BattlescapeButton(32, 40, screenWidth - 128, 0);
	_btnTileFilterGround->setColor(232);
	_btnTileFilterWestWall = new BattlescapeButton(32, 40, screenWidth - 96, 0);
	_btnTileFilterWestWall->setColor(232);
	_btnTileFilterNorthWall = new BattlescapeButton(32, 40, screenWidth - 64, 0);
	_btnTileFilterNorthWall->setColor(232);
	_btnTileFilterObject = new BattlescapeButton(32, 40, screenWidth - 32, 0);
	_btnTileFilterObject->setColor(232);

	_tileFilters[O_FLOOR] = _btnTileFilterGround;
	_tileFilters[O_WESTWALL] = _btnTileFilterWestWall;
	_tileFilters[O_NORTHWALL] = _btnTileFilterNorthWall;
	_tileFilters[O_OBJECT] = _btnTileFilterObject;

	_backgroundTileSelection = new InteractiveSurface(32, 40, 0, 0);
	icons->getFrame(15)->blitNShade(_backgroundTileSelection, 0, 0);
	_backgroundTileSelectionNavigation = new InteractiveSurface(96, 40, 32, 0);
	for (int i = 0; i < 3; ++i)
	{
		icons->getFrame(i + 16)->blitNShade(_backgroundTileSelectionNavigation, i * 32, 0);
	}
	_tileSelectionColumns = screenWidth / 2 / 32;
	_tileSelectionRows = (screenHeight - 2 * 40) / 40;
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
	}

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_map);
	add(_txtTooltip, "textTooltip", "battlescape");
	add(_txtDebug, "textTooltip", "battlescape");
	add(_iconsLowerLeft);
	add(_iconsLowerRight);
	add(_iconsUpperRight);
	add(_btnOptions, "", "battlescape", _iconsLowerLeft);
	add(_btnSave, "", "battlescape", _iconsLowerLeft);
	add(_btnLoad, "", "battlescape", _iconsLowerLeft);
	add(_btnUndo, "", "battlescape", _iconsLowerLeft);
	add(_btnRedo, "", "battlescape", _iconsLowerLeft);
	add(_btnFill, "", "battlescape", _iconsLowerRight);
	add(_btnClear, "", "battlescape", _iconsLowerRight);
	add(_btnCut, "", "battlescape", _iconsLowerRight);
	add(_btnCopy, "", "battlescape", _iconsLowerRight);
	add(_btnPaste, "", "battlescape", _iconsLowerRight);
	add(_btnSelectedTile, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterGround, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterWestWall, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterNorthWall, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterObject, "", "battlescape", _iconsUpperRight);
	add(_backgroundTileSelection);
	add(_backgroundTileSelectionNavigation);
	add(_tileSelection, "", "battlescape", _backgroundTileSelection);
	add(_panelTileSelection);
	add(_tileSelectionPageCount, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_tileSelectionLeftArrow, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_tileSelectionRightArrow, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_txtSelectionPageCount);
	for (auto i : _tileSelectionGrid)
	{
		add(i);
		i->onMouseClick((ActionHandler)&MapEditorState::tileSelectionGridClick);
	}

	// Set up objects
	_save = _game->getSavedGame()->getSavedBattle();
	_editor->setSave(_save);
	_editor->setSelectedMapDataID(-1);

	_map->init();
	_map->onMouseOver((ActionHandler)&MapEditorState::mapOver);
	_map->onMousePress((ActionHandler)&MapEditorState::mapPress);
	_map->onMouseClick((ActionHandler)&MapEditorState::mapClick, 0);
	_map->onMouseIn((ActionHandler)&MapEditorState::mapIn);

	//_btnMapUp->onMouseClick((ActionHandler)&MapEditorState::btnMapUpClick);
	//_btnMapUp->onKeyboardPress((ActionHandler)&MapEditorState::btnMapUpClick, Options::keyBattleLevelUp);
	//_btnMapUp->setTooltip("STR_VIEW_LEVEL_ABOVE");
	//_btnMapUp->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	//_btnMapUp->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnMapDown->onMouseClick((ActionHandler)&MapEditorState::btnMapDownClick);
	//_btnMapDown->onKeyboardPress((ActionHandler)&MapEditorState::btnMapDownClick, Options::keyBattleLevelDown);
	//_btnMapDown->setTooltip("STR_VIEW_LEVEL_BELOW");
	//_btnMapDown->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	//_btnMapDown->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_txtTooltip->setHighContrast(true);

	updateDebugText();
	_txtDebug->setColor(_tooltipDefaultColor);
	_txtDebug->setHighContrast(true);
	_txtDebug->setAlign(ALIGN_RIGHT);

	// Set music
	if (!Options::oxcePlayBriefingMusicDuringEquipment)
	{
		if (_save->getMusic().empty())
		{
			_game->getMod()->playMusic("GMTACTIC");
		}
		else
		{
			_game->getMod()->playMusic(_save->getMusic());
		}
	}

	_iconsLowerLeft->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsLowerLeft->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsLowerRight->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsLowerRight->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsUpperRight->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsUpperRight->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_backgroundTileSelection->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_backgroundTileSelection->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_backgroundTileSelectionNavigation->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_backgroundTileSelectionNavigation->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_panelTileSelection->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_panelTileSelection->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsMousedOver.clear();

	_btnOptions->onMouseClick((ActionHandler)&MapEditorState::btnOptionsClick);
	_btnOptions->onKeyboardPress((ActionHandler)&MapEditorState::btnOptionsClick, Options::keyBattleOptions);
	_btnOptions->setTooltip("STR_OPTIONS");
	_btnOptions->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnOptions->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnSave->onMouseClick((ActionHandler)&MapEditorState::btnSaveClick);
	//_btnSave->onKeyboardPress((ActionHandler)&MapEditorState::btnSaveClick, SDLK_s); // change to options
	_btnSave->setTooltip("STR_TOOLTIP_SAVE_MAP");
	_btnSave->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnSave->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnLoad->onMouseClick((ActionHandler)&MapEditorState::btnLoadClick);
	//_btnLoad->onKeyboardPress((ActionHandler)&MapEditorState::btnLoadClick, SDLK_o); // change to options
	_btnLoad->setTooltip("STR_TOOLTIP_LOAD_MAP");
	_btnLoad->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnLoad->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnUndo->onMouseClick((ActionHandler)&MapEditorState::btnUndoClick);
	//_btnUndo->onKeyboardPress((ActionHandler)&MapEditorState::btnUndoClick, SDLK_z); // change to options
	_btnUndo->setTooltip("STR_TOOLTIP_UNDO");
	_btnUndo->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnUndo->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnRedo->onMouseClick((ActionHandler)&MapEditorState::btnRedoClick);
	//_btnRedo->onKeyboardPress((ActionHandler)&MapEditorState::btnRedoClick, SDLK_z); // change to options
	_btnRedo->setTooltip("STR_TOOLTIP_REDO");
	_btnRedo->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnRedo->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnFill->onMouseClick((ActionHandler)&MapEditorState::btnFillClick);
	//_btnFill->onKeyboardPress((ActionHandler)&MapEditorState::btnFillClick, SDLK_z); // change to options
	_btnFill->setTooltip("STR_TOOLTIP_FILL");
	_btnFill->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnFill->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnClear->onMouseClick((ActionHandler)&MapEditorState::btnClearClick);
	//_btnClear->onKeyboardPress((ActionHandler)&MapEditorState::btnClearClick, SDLK_z); // change to options
	_btnClear->setTooltip("STR_TOOLTIP_CLEAR");
	_btnClear->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnClear->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnCut->onMouseClick((ActionHandler)&MapEditorState::btnCutClick);
	//_btnCut->onKeyboardPress((ActionHandler)&MapEditorState::btnCutClick, SDLK_x); // change to options
	_btnCut->setTooltip("STR_TOOLTIP_CUT");
	_btnCut->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnCut->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnCopy->onMouseClick((ActionHandler)&MapEditorState::btnCopyClick);
	//_btnCopy->onKeyboardPress((ActionHandler)&MapEditorState::btnCopyClick, SDLK_c); // change to options
	_btnCopy->setTooltip("STR_TOOLTIP_COPY");
	_btnCopy->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnCopy->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnPaste->onMouseClick((ActionHandler)&MapEditorState::btnPasteClick);
	//_btnPaste->onKeyboardPress((ActionHandler)&MapEditorState::btnPasteClick, SDLK_v); // change to options
	_btnPaste->setTooltip("STR_TOOLTIP_PASTE");
	_btnPaste->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnPaste->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnSelectedTile->onMouseClick((ActionHandler)&MapEditorState::btnSelectedTileClick);
	//_btnSelectedTile->onKeyboardPress((ActionHandler)&MapEditorState::btnSelectedTileClick, SDLK_v); // change to options
	_btnSelectedTile->setTooltip("STR_TOOLTIP_SELECTED_TILE");
	_btnSelectedTile->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnSelectedTile->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterGround->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterGround->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_1); // change to options
	_btnTileFilterGround->setTooltip("STR_TOOLTIP_TILE_GROUND");
	_btnTileFilterGround->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterGround->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterWestWall->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterWestWall->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_2); // change to options
	_btnTileFilterWestWall->setTooltip("STR_TOOLTIP_TILE_WESTWALL");
	_btnTileFilterWestWall->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterWestWall->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterNorthWall->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterNorthWall->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_3); // change to options
	_btnTileFilterNorthWall->setTooltip("STR_TOOLTIP_TILE_NORTHWALL");
	_btnTileFilterNorthWall->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterNorthWall->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterObject->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterObject->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_4); // change to options
	_btnTileFilterObject->setTooltip("STR_TOOLTIP_TILE_OBJECT");
	_btnTileFilterObject->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterObject->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_tileSelection->setColor(232); // Goal of background color 235
	_tileSelection->onMouseClick((ActionHandler)&MapEditorState::tileSelectionClick);
	_tileSelection->setTooltip("STR_TOOLTIP_TILE_SELECTION");
	_tileSelection->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_tileSelection->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

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
	_tileSelectionPageCount->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);

	if (_tileSelectionCurrentPage == _tileSelectionLastPage)
	{
		size_t arrowColor = 1;
		icons->getFrame(16)->blitNShade(_tileSelectionLeftArrow, 0, -28, 0, false, arrowColor);
		icons->getFrame(18)->blitNShade(_tileSelectionRightArrow, 0, -28, 0, false, arrowColor);
	}

	_tileSelectionLeftArrow->onMouseClick((ActionHandler)&MapEditorState::tileSelectionLeftArrowClick);
	_tileSelectionLeftArrow->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	_tileSelectionLeftArrow->setVisible(false);

	_tileSelectionRightArrow->onMouseClick((ActionHandler)&MapEditorState::tileSelectionRightArrowClick);
	_tileSelectionRightArrow->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	_tileSelectionRightArrow->setVisible(false);

	//_panelTileSelection->setColor(232); // nice purple
	_panelTileSelection->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_panelTileSelection->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_panelTileSelection->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	drawTileSelectionGrid();
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(false);
	}
	_panelTileSelection->setVisible(false);

	_backgroundTileSelectionNavigation->setVisible(false);

	_animTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_animTimer->onTimer((StateHandler)&MapEditorState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_gameTimer->onTimer((StateHandler)&MapEditorState::handleState);
}


/**
 * Deletes the MapEditorState.
 */
MapEditorState::~MapEditorState()
{
	delete _animTimer;
	delete _gameTimer;
}

/**
 * Initializes the battlescapestate.
 */
void MapEditorState::init()
{
	State::init();
	_animTimer->start();
	_gameTimer->start();
	_map->setFocus(true);
	_map->draw();

	int midpointX = _save->getMapSizeX() / 2;
	int midpointY = _save->getMapSizeY() / 2;

	if (_firstInit)
	{
		// Set music
		if (Options::oxcePlayBriefingMusicDuringEquipment)
		{
			if (_save->getMusic() == "")
			{
				_game->getMod()->playMusic("GMTACTIC");
			}
			else
			{
				_game->getMod()->playMusic(_save->getMusic());
			}
		}

		_map->getCamera()->centerOnPosition(Position(midpointX, midpointY, 0), true);
		_map->refreshSelectorPosition();
		_map->setCursorType(CT_NORMAL);

		_editor->setSelectedObject(O_MAX);

		_firstInit = false;
	}
	_txtTooltip->setText("");

	//if (_autosave)
	//{
	//	_autosave = false;
	//	if (_game->getSavedGame()->isIronman())
	//	{
	//		_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_IRONMAN, _palette));
	//	}
	//	else if (Options::autosave)
	//	{
	//		_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_AUTO_BATTLESCAPE, _palette));
	//	}
	//}
}

/**
 * Runs the timers and handles popups.
 */
void MapEditorState::think()
{
	static bool popped = false;


	if (_gameTimer->isRunning())
	{
        State::think();
        _animTimer->think(this, 0);
        _gameTimer->think(this, 0);
	}

	if (_editor->getEditRegisterPosition() == 0 && _btnUndo->getColor() != 8)
	{
		_btnUndo->offset(8 - 232, 0, 255, 1);
		_btnUndo->setColor(8); // change to disabled button color
	}
	else if (_editor->getEditRegisterPosition() > 0 && _btnUndo->getColor() != 232)
	{
		_btnUndo->offset(232 - 8, 0, 255, 1);
		_btnUndo->setColor(232); // change to default color
	}

	if (_editor->getEditRegisterPosition() == _editor->getEditRegisterSize() && _btnRedo->getColor() != 8)
	{
		_btnRedo->offset(8 - 232, 0, 255, 1);
		_btnRedo->setColor(8);
	}
	else if (_editor->getEditRegisterPosition() < _editor->getEditRegisterSize() && _btnRedo->getColor() != 232)
	{
		_btnRedo->offset(232 - 8, 0, 255, 1);
		_btnRedo->setColor(232);
	}

	updateDebugText();
}

/**
 * Processes any mouse moving over the map.
 * @param action Pointer to an action.
 */
void MapEditorState::mapOver(Action *action)
{
	if (_isMouseScrolling && action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// (checking: is the dragScroll-mouse-button still pressed?)
		// However if the SDL is also missed the release event, then it is to no avail :(
		if ((SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{ // so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}
			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action);
			return;
		}

		_isMouseScrolled = true;

		if (Options::touchEnabled == false)
		{
			// Set the mouse cursor back
			SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
			SDL_WarpMouse(_game->getScreen()->getWidth() / 2, _game->getScreen()->getHeight() / 2 - _map->getIconHeight() / 2);
			SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
		}

		// Check the threshold
		_totalMouseMoveX += action->getDetails()->motion.xrel;
		_totalMouseMoveY += action->getDetails()->motion.yrel;
		if (!_mouseMovedOverThreshold)
		{
			_mouseMovedOverThreshold = ((std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance) || (std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance));
		}

		// Scrolling
		if (Options::battleDragScrollInvert)
		{
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			int scrollX = -(int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = -(int)((double)_totalMouseMoveY / action->getYScale());
			Position delta2 = _map->getCamera()->getMapOffset();
			_map->getCamera()->scrollXY(scrollX, scrollY, true);
			delta2 = _map->getCamera()->getMapOffset() - delta2;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = -(int) (delta2.x * action->getXScale());
				_totalMouseMoveY = -(int) (delta2.y * action->getYScale());
			}

			if (Options::touchEnabled == false)
			{
				action->getDetails()->motion.x = _xBeforeMouseScrolling;
				action->getDetails()->motion.y = _yBeforeMouseScrolling;
			}
			_map->setCursorType(CT_NONE);
		}
		else
		{
			Position delta = _map->getCamera()->getMapOffset();
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			int scrollX = (int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = (int)((double)_totalMouseMoveY / action->getYScale());
			Position delta2 = _map->getCamera()->getMapOffset();
			_map->getCamera()->scrollXY(scrollX, scrollY, true);
			delta2 = _map->getCamera()->getMapOffset() - delta2;
			delta = _map->getCamera()->getMapOffset() - delta;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = (int) (delta2.x * action->getXScale());
				_totalMouseMoveY = (int) (delta2.y * action->getYScale());
			}

			int barWidth = _game->getScreen()->getCursorLeftBlackBand();
			int barHeight = _game->getScreen()->getCursorTopBlackBand();
			int cursorX = _cursorPosition.x + Round(delta.x * action->getXScale());
			int cursorY = _cursorPosition.y + Round(delta.y * action->getYScale());
			_cursorPosition.x = Clamp(cursorX, barWidth, _game->getScreen()->getWidth() - barWidth - (int)(Round(action->getXScale())));
			_cursorPosition.y = Clamp(cursorY, barHeight, _game->getScreen()->getHeight() - barHeight - (int)(Round(action->getYScale())));

			if (Options::touchEnabled == false)
			{
				action->getDetails()->motion.x = _cursorPosition.x;
				action->getDetails()->motion.y = _cursorPosition.y;
			}
		}

		// We don't want to look the mouse-cursor jumping :)
		_game->getCursor()->handle(action);
	}
}

/**
 * Processes any presses on the map.
 * @param action Pointer to an action.
 */
void MapEditorState::mapPress(Action *action)
{
	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons) return;

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;
		SDL_GetMouseState(&_xBeforeMouseScrolling, &_yBeforeMouseScrolling);
		_mapOffsetBeforeMouseScrolling = _map->getCamera()->getMapOffset();
		if (!Options::battleDragScrollInvert && _cursorPosition.z == 0)
		{
			_cursorPosition.x = action->getDetails()->motion.x;
			_cursorPosition.y = action->getDetails()->motion.y;
			// the Z is irrelevant to our mouse position, but we can use it as a boolean to check if the position is set or not
			_cursorPosition.z = 1;
		}
		_totalMouseMoveX = 0; _totalMouseMoveY = 0;
		_mouseMovedOverThreshold = false;
		_mouseScrollingStartTime = SDL_GetTicks();
	}
}

/**
 * Processes any clicks on the map to edit it
 * @param action Pointer to an action.
 */
void MapEditorState::mapClick(Action *action)
{
	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now an other button is used)
	if (_isMouseScrolling)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
		&& (SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{   // so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}
			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action);
		}
	}

	// DragScroll-Button release: release mouse-scroll-mode
	if (_isMouseScrolling)
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_isMouseScrolling = false;
			stopScrolling(action);
		}
		else
		{
			return;
		}
		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
		{
			_isMouseScrolled = false;
			stopScrolling(action);
		}
		if (_isMouseScrolled) return;
	}

	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons) return;


	// don't accept leftclicks if there is no cursor or there is an action busy
	if (_map->getCursorType() == CT_NONE) return;

	Position pos;
	_map->getSelectorPosition(&pos);

	// Have the map editor capture the mouse input here for editing tiles
	if (_editor)
	{
		Tile *selectedTile = _save->getTile(pos);
		_editor->handleEditorInput(action, selectedTile);
		return;
	}
}

/**
 * Handles mouse entering the map surface.
 * @param action Pointer to an action.
 */
void MapEditorState::mapIn(Action *)
{
	_isMouseScrolling = false;
	_map->setButtonsPressed(Options::battleDragScrollButton, false);
}

/**
 * Shows the next map layer.
 * @param action Pointer to an action.
 */
//void MapEditorState::btnMapUpClick(Action *)
//{
//	_map->getCamera()->up();
//}

/**
 * Shows the previous map layer.
 * @param action Pointer to an action.
 */
//void MapEditorState::btnMapDownClick(Action *)
//{
//	_map->getCamera()->down();
//}

/**
 * Shows/hides all map layers.
 * @param action Pointer to an action.
 */
//void MapEditorState::btnShowLayersClick(Action *)
//{
//	_numLayers->setValue(_map->getCamera()->toggleShowAllLayers());
//}

/**
 * Shows options.
 * @param action Pointer to an action.
 */
void MapEditorState::btnOptionsClick(Action *action)
{
	// Pressing escape closes the tile selection UI first
	if (_panelTileSelection->getVisible() && action->getDetails()->type == SDL_KEYDOWN)
	{
		tileSelectionClick(action);
		return;
	}

	_game->pushState(new MapEditorOptionsState(OPT_MAPEDITOR));
}

/**
 * Opens the jukebox.
 * @param action Pointer to an action.
 */
void MapEditorState::btnSelectMusicTrackClick(Action *)
{
	_game->pushState(new SelectMusicTrackState(SMT_BATTLESCAPE));
}

/**
 * Saves the edited map.
 * @param action Pointer to an action.
 */
void MapEditorState::btnSaveClick(Action *action)
{
	_editor->saveMapFile(_editor->getMapName());
}

/**
 * Un-does the action on the top of the editor's register
 * @param action Pointer to an action.
 */
void MapEditorState::btnUndoClick(Action *action)
{
	_editor->undo();
	if (_txtTooltip->getVisible())
		txtTooltipIn(action);
	_map->draw();
}

/**
 * Re-does the next action on the editor's register
 * @param action Pointer to an action.
 */
void MapEditorState::btnRedoClick(Action *action)
{
	_editor->redo();
	if (_txtTooltip->getVisible())
		txtTooltipIn(action);
	_map->draw();
}

/**
 * Handles pressing the fill button
 * @param action Pointer to an action.
 */
void MapEditorState::btnFillClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	_editor->setSelectedMapDataID(_selectedTileIndex);
	_tileEditMode = _btnFill;
}

/**
 * Handles pressing the clear button
 * @param action Pointer to an action.
 */
void MapEditorState::btnClearClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	_editor->setSelectedMapDataID(-1);
	_tileEditMode = _btnClear;
}

/**
 * Handles pressing the tile filter buttons
 * @param action Pointer to an action.
 */
void MapEditorState::btnTileFilterClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	TilePart clickedFilter = O_MAX;

	if (action->getSender() == _btnTileFilterGround)
	{
		clickedFilter = O_FLOOR;
	}
	else if (action->getSender() == _btnTileFilterWestWall)
	{
		clickedFilter = O_WESTWALL;
	}
	else if (action->getSender() == _btnTileFilterNorthWall)
	{
		clickedFilter = O_NORTHWALL;
	}
	else if (action->getSender() == _btnTileFilterObject)
	{
		clickedFilter = O_OBJECT;
	}

	// Clicking on the currently selected filter: de-select the filter
	if (_editor->getSelectedObject() == clickedFilter)
	{
		_editor->setSelectedObject(O_MAX);

		_btnTileFilterGround->setGroup(0);
		_btnTileFilterWestWall->setGroup(0);
		_btnTileFilterNorthWall->setGroup(0);
		_btnTileFilterObject->setGroup(0);
		
		action->getDetails()->type = SDL_MOUSEBUTTONUP;
		_tileObjectSelected->mouseRelease(action, this);
		_tileObjectSelected->draw();
		_tileObjectSelected = 0;

		return;
	}

	_tileObjectSelected = _tileFilters[clickedFilter];
	// If no filter was previously selected, the filters need to be re-grouped
	if (_editor->getSelectedObject() == O_MAX)
	{
		_btnTileFilterGround->setGroup(&_tileObjectSelected);
		_btnTileFilterWestWall->setGroup(&_tileObjectSelected);
		_btnTileFilterNorthWall->setGroup(&_tileObjectSelected);
		_btnTileFilterObject->setGroup(&_tileObjectSelected);
	}

	_editor->setSelectedObject(clickedFilter);

	// consume the event to keep the button held down
	action->getDetails()->type = SDL_NOEVENT;
}

/**
 * Animates map objects on the map
 */
void MapEditorState::animate()
{
	_map->animate(true);
}

/**
 * Handles the battle game state.
 */
void MapEditorState::handleState()
{
    _map->invalidate();
	//_battleGame->handleState();
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval An interval in ms.
 */
void MapEditorState::setStateInterval(Uint32 interval)
{
	_gameTimer->setInterval(interval);
}

/**
 * Gets pointer to the game. Some states need this info.
 * @return Pointer to game.
 */
Game *MapEditorState::getGame() const
{
	return _game;
}

/**
 * Gets pointer to the map. Some states need this info.
 * @return Pointer to map.
 */
Map *MapEditorState::getMap() const
{
	return _map;
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
inline void MapEditorState::handle(Action *action)
{
	if (!_firstInit)
	{
		if (_game->getCursor()->getVisible() || ((action->getDetails()->type == SDL_MOUSEBUTTONDOWN || action->getDetails()->type == SDL_MOUSEBUTTONUP) && action->getDetails()->button.button == SDL_BUTTON_RIGHT))
		{
			State::handle(action);

			if (Options::touchEnabled == false && _isMouseScrolling && !Options::battleDragScrollInvert)
			{
				_map->setSelectorPosition((_cursorPosition.x - _game->getScreen()->getCursorLeftBlackBand()) / action->getXScale(), (_cursorPosition.y - _game->getScreen()->getCursorTopBlackBand()) / action->getYScale());
			}

			if (action->getDetails()->type == SDL_KEYDOWN)
			{
				SDLKey key = action->getDetails()->key.keysym.sym;
				bool ctrlPressed = (SDL_GetModState() & KMOD_CTRL) != 0;
				bool shiftPressed = (SDL_GetModState() & KMOD_SHIFT) != 0;

				if (key == SDLK_z && ctrlPressed) // change z to options
				{
					if (shiftPressed)
						btnRedoClick(action);
					else
						btnUndoClick(action);
				}
				else if (key == SDLK_s && ctrlPressed) // change s to options
				{
					if (shiftPressed || _editor->getMapName().size() == 0)
						_game->pushState(new MapEditorSaveAsState());
					else
						btnSaveClick(action);
				}
				else if (key == SDLK_d && ctrlPressed) // change d to options
				{
					updateDebugText();
					_txtDebug->setVisible(!_txtDebug->getVisible());
				}
				else if (key == SDLK_i) // change i to options
				{
					_game->pushState(new MapEditorInfoState());
				}

				// quick save and quick load
				//if (!_game->getSavedGame()->isIronman())
				//{
				//	if (key == Options::keyQuickSave)
				//	{
				//		_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
				//	}
				//	else if (key == Options::keyQuickLoad)
				//	{
				//		_game->pushState(new LoadGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
				//	}
				//}
			}
		}
	}
}

/**
 * Updates the debug text
 */
void MapEditorState::updateDebugText()
{
	std::string selectedMode;
	if (_editor->getSelectedMapDataID() == -1)
	{
		selectedMode = tr("STR_DEBUG_CLEAR");
	}
	else
	{
		selectedMode = tr("STR_DEBUG_PLACE");
	}

	std::string selectedObject;
	switch (_editor->getSelectedObject())
	{
		case O_FLOOR:
			selectedObject = tr("STR_DEBUG_O_FLOOR");
			break;
		case O_WESTWALL:
			selectedObject = tr("STR_DEBUG_O_WESTWALL");
			break;
		case O_NORTHWALL:
			selectedObject = tr("STR_DEBUG_O_NORTHWALL");
			break;
		case O_OBJECT:
			selectedObject = tr("STR_DEBUG_O_OBJECT");
			break;
		default:
			selectedObject = tr("STR_DEBUG_O_MAX");
			break;
	}

	Position pos;
	_map->getSelectorPosition(&pos);
	Tile *selectedTile = _save->getTile(pos);
	if (!selectedTile)
	{
		pos = Position(-1, -1, -1);
	}

	_txtDebug->setText(tr("STR_DEBUG_MAP_EDITOR").arg(selectedMode).arg(selectedObject).arg(pos));
}

/**
 * Clears mouse-scrolling state (isMouseScrolling).
 */
void MapEditorState::clearMouseScrollingState()
{
	_isMouseScrolling = false;
}

/**
 * Handler for the mouse moving over the icons, disabling the tile selection cube.
 * @param action Pointer to an action.
 */
void MapEditorState::mouseInIcons(Action *action)
{
	InteractiveSurface *sender = action->getSender();
	if (sender && std::find_if(_iconsMousedOver.begin(), _iconsMousedOver.end(),
		[&](const InteractiveSurface* s){ return s == sender; }) == _iconsMousedOver.end())
	{
		_iconsMousedOver.push_back(sender);
	}

	_mouseOverIcons = _iconsMousedOver.size() != 0;
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 * @param action Pointer to an action.
 */
void MapEditorState::mouseOutIcons(Action *action)
{
	InteractiveSurface *sender = action->getSender();
	if (sender)
	{
		std::vector<InteractiveSurface*>::iterator it = std::find_if(_iconsMousedOver.begin(), _iconsMousedOver.end(),
		[&](const InteractiveSurface* s){ return s == sender; });

		if (it != _iconsMousedOver.end())
		{
			_iconsMousedOver.erase(it);
		}
	}

	_mouseOverIcons = _iconsMousedOver.size() != 0;
}

/**
 * Checks if the mouse is over the icons.
 * @return True, if the mouse is over the icons.
 */
bool MapEditorState::getMouseOverIcons() const
{
	return _mouseOverIcons;
}

/**
* Shows a tooltip for the appropriate button.
* @param action Pointer to an action.
*/
void MapEditorState::txtTooltipIn(Action *action)
{
	if (Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		if (action->getSender() == _btnUndo || action->getSender() == _btnRedo)
		{
			std::ostringstream ss;
			ss << tr(_currentTooltip).arg(std::to_string(_editor->getEditRegisterPosition())).arg(std::to_string(_editor->getEditRegisterSize()));
			_txtTooltip->setText(ss.str());
		}
		else
		{
			_txtTooltip->setText(tr(_currentTooltip));
		}
	}
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void MapEditorState::txtTooltipOut(Action *action)
{
	// reset color
	_txtTooltip->setColor(_tooltipDefaultColor);

	if (Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_txtTooltip->setText("");
		}
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void MapEditorState::resize(int &dX, int &dY)
{
	dX = Options::baseXResolution;
	dY = Options::baseYResolution;
	int divisor = 1;
	double pixelRatioY = 1.0;

	if (Options::nonSquarePixelRatio)
	{
		pixelRatioY = 1.2;
	}
	switch (Options::battlescapeScale)
	{
	case SCALE_SCREEN_DIV_6:
		divisor = 6;
		break;
	case SCALE_SCREEN_DIV_5:
		divisor = 5;
		break;
	case SCALE_SCREEN_DIV_4:
		divisor = 4;
		break;
	case SCALE_SCREEN_DIV_3:
		divisor = 3;
		break;
	case SCALE_SCREEN_DIV_2:
		divisor = 2;
		break;
	case SCALE_SCREEN:
		break;
	default:
		dX = 0;
		dY = 0;
		return;
	}

	Options::baseXResolution = std::max(Screen::ORIGINAL_WIDTH, Options::displayWidth / divisor);
	Options::baseYResolution = std::max(Screen::ORIGINAL_HEIGHT, (int)(Options::displayHeight / pixelRatioY / divisor));

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;
	_map->setWidth(Options::baseXResolution);
	_map->setHeight(Options::baseYResolution);
	_map->getCamera()->resize();
	_map->getCamera()->jumpXY(dX/2, dY/2);

	for (std::vector<Surface*>::const_iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		if (*i != _map)
		{
			(*i)->setX((*i)->getX() + dX / 2);
			(*i)->setY((*i)->getY() + dY);
		}
		else if (*i != _map)
		{
			(*i)->setX((*i)->getX() + dX);
		}
	}

}

/**
 * Move the mouse back to where it started after we finish drag scrolling.
 * @param action Pointer to an action.
 */
void MapEditorState::stopScrolling(Action *action)
{
	if (Options::battleDragScrollInvert)
	{
		SDL_WarpMouse(_xBeforeMouseScrolling, _yBeforeMouseScrolling);
		action->setMouseAction(_xBeforeMouseScrolling, _yBeforeMouseScrolling, _map->getX(), _map->getY());
		_map->setCursorType(CT_NORMAL);
	}
	else
	{
		SDL_WarpMouse(_cursorPosition.x, _cursorPosition.y);
		action->setMouseAction(_cursorPosition.x/action->getXScale(), _cursorPosition.y/action->getYScale(), 0, 0);
		_map->setSelectorPosition(_cursorPosition.x / action->getXScale(), _cursorPosition.y / action->getYScale());
	}
	// reset our "mouse position stored" flag
	_cursorPosition.z = 0;
}

/**
 * Autosave the game the next time the battlescape is displayed.
 */
//void MapEditorState::autosave()
//{
//	_autosave = true;
//}

/**
 * Gets the pointer to the Map Editor
 */
MapEditor *MapEditorState::getMapEditor()
{
	return _editor;
}

/**
 * Toggles the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionClick(Action *action)
{
	_backgroundTileSelectionNavigation->setVisible(!_backgroundTileSelectionNavigation->getVisible());
	_txtSelectionPageCount->setVisible(!_txtSelectionPageCount->getVisible());
	_tileSelectionPageCount->setVisible(!_tileSelectionPageCount->getVisible());
	_tileSelectionLeftArrow->setVisible(!_tileSelectionLeftArrow->getVisible());
	_tileSelectionRightArrow->setVisible(!_tileSelectionRightArrow->getVisible());
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(!_panelTileSelection->getVisible());
	}
	_panelTileSelection->setVisible(!_panelTileSelection->getVisible());

	if (_panelTileSelection->getVisible())
		_txtTooltip->setX(_panelTileSelection->getWidth() + 2);
	else
		_txtTooltip->setX(2);

	drawTileSpriteOnSurface(_tileSelection, _selectedTileIndex);

	// Make sure the hidden surfaces are no longer stored as "moused over"
	InteractiveSurface *surface = _backgroundTileSelectionNavigation;
	action->setSender(surface);
	mouseOutIcons(action);
	surface = _panelTileSelection;
	action->setSender(surface);
	mouseOutIcons(action);
}

/**
 * Draws the tile images on the selection grid
 */
void MapEditorState::drawTileSelectionGrid()
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
void MapEditorState::tileSelectionLeftArrowClick(Action *action)
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
void MapEditorState::tileSelectionRightArrowClick(Action *action)
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
void MapEditorState::tileSelectionGridClick(Action *action)
{
	int mouseX = (int)action->getAbsoluteXMouse();
	int mouseY = (int)action->getAbsoluteYMouse();

	int index = _tileSelectionColumns * ((mouseY - 40) / 40) + (mouseX / 32);
	index += _tileSelectionCurrentPage * _tileSelectionRows * _tileSelectionColumns;
	if (drawTileSpriteOnSurface(_tileSelection, index))
	{
		_selectedTileIndex = index;
		if (_editor->getSelectedMapDataID() != -1)
			_editor->setSelectedMapDataID(index);
	}
}

/**
 * Handles mouse wheel scrolling of the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionMousePress(Action *action)
{
	if (Options::battleDragScrollButton != SDL_BUTTON_MIDDLE || (SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		{
			tileSelectionLeftArrowClick(action);
			// Consume the event so the map doesn't scroll up or down
			action->getDetails()->type = SDL_NOEVENT;
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		{
			tileSelectionRightArrowClick(action);
			// Consume the event so the map doesn't scroll up or down
			action->getDetails()->type = SDL_NOEVENT;
		}
	}
}

/**
 * Draws a tile sprite on a given surface
 * @param surface Pointer to the surface.
 * @param index Index of the tile object.
 * @return Whether or not we found and drew the proper tile object.
 */
bool MapEditorState::drawTileSpriteOnSurface(Surface *surface, int index)
{
	int mapDataSetID = 0;
	int mapDataID = 0;
	MapData *mapData = _editor->getMapDataFromIndex(index, &mapDataSetID, &mapDataID);

	if (mapData)
	{
		surface->draw();
		_save->getMapDataSets()->at(mapDataSetID)->getSurfaceset()->getFrame(mapData->getSprite(0))->blitNShade(surface, 0, 0);
		return true;
	}

	return false;
}

}