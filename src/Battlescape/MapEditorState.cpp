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
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Menu/PauseState.h"
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
	_editor(editor), _tileSelectionColumns(5), _tileSelectionRows(3), _tileSelectionCurrentPage(0), _tileSelectionLastPage(0)
{
	const int screenWidth = Options::baseXResolution;
	const int screenHeight = Options::baseYResolution;
	//const int iconsWidth = _game->getMod()->getInterface("battlescape")->getElement("icons")->w;
	//const int iconsHeight = _game->getMod()->getInterface("battlescape")->getElement("icons")->h;
	//const int visibleMapHeight = screenHeight - iconsHeight;
	//const int x = screenWidth/2 - iconsWidth/2;
	//const int y = screenHeight - iconsHeight;

	_tooltipDefaultColor = _game->getMod()->getInterface("battlescape")->getElement("textTooltip")->color;

	// Create the battlemap view
	_map = new Map(_game, screenWidth, screenHeight, 0, 0, screenHeight);

	// Create buttons
	//_btnMapUp = new BattlescapeButton(32, 16, x + 80, y);
	//_btnMapDown = new BattlescapeButton(32, 16, x + 80, y + 16);

	// Create soldier stats summary
	_txtDebug = new Text(300, 10, 20, 0);
	_txtTooltip = new Text(300, 10, 2, screenHeight - 50);

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_map);
	add(_txtDebug);
	add(_txtTooltip, "textTooltip", "battlescape");

	// Set up objects
	_save = _game->getSavedGame()->getSavedBattle();
	_map->init();
	_map->onMouseOver((ActionHandler)&MapEditorState::mapOver);
	_map->onMousePress((ActionHandler)&MapEditorState::mapPress);
	_map->onMouseClick((ActionHandler)&MapEditorState::mapClick, 0);
	_map->onMouseIn((ActionHandler)&MapEditorState::mapIn);

	//_icons->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	//_icons->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);

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

	_txtDebug->setColor(Palette::blockOffset(8));
	_txtDebug->setHighContrast(true);

	_txtTooltip->setHighContrast(true);

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

	_animTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_animTimer->onTimer((StateHandler)&MapEditorState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_gameTimer->onTimer((StateHandler)&MapEditorState::handleState);

	//_battleGame = new BattlescapeGame(_save, this);

	// TODO: create interface ruleset for the map editor for all these hardcoded colors
	_btnOptions = new TextButton(32, 40, screenWidth - 32, screenHeight - 40);
	_btnOptions->setColor(232);
	
	_editor->setSave(_save);
	_editor->setSelectedMapDataID(-1);

	_txtSelectedIndex = new Text(100, 10, 32, 10);
	_edtSelectedIndex = new TextEdit(this, 50, 10, 32, 20);
	_txtEditRegister = new Text(200, 10, 32, 30);

	_tileSelectionColumns = screenWidth / 2 / 32;
	_tileSelectionRows = (screenHeight - 2 * 40) / 40;
	int tileSelectionWidth = _tileSelectionColumns * 32;
	int tileSelectionHeight = _tileSelectionRows * 40;
	// TODO: change text buttons that are only images to ImageButton?
	_tileSelection = new TextButton(32, 40, 0, 0);
	_panelTileSelection = new TextButton(tileSelectionWidth, tileSelectionHeight, 0, 40);
	_tileSelectionPageCount = new TextButton(32, 12, 64, 28);
	_tileSelectionLeftArrow = new TextButton(32, 12, 32, 28);
	_tileSelectionRightArrow = new TextButton(32, 12, 96, 28);
	_tileSelectionGrid.clear();
	for (int i = 0; i < _tileSelectionRows; ++i)
	{
		for (int j = 0; j < _tileSelectionColumns; ++j)
		{
			_tileSelectionGrid.push_back(new InteractiveSurface(32, 40, j * 32, i * 40 + 40));
		}
	}

	add(_btnOptions);
	add(_txtSelectedIndex);
	add(_edtSelectedIndex);
	add(_txtEditRegister);
	add(_tileSelection);
	add(_panelTileSelection);
	add(_tileSelectionPageCount);
	add(_tileSelectionLeftArrow);
	add(_tileSelectionRightArrow);
	for (auto i : _tileSelectionGrid)
	{
		add(i);
		i->onMouseClick((ActionHandler)&MapEditorState::tileSelectionGridClick);
	}

	_btnOptions->onMouseClick((ActionHandler)&MapEditorState::btnOptionsClick);
	_btnOptions->onKeyboardPress((ActionHandler)&MapEditorState::btnOptionsClick, Options::keyBattleOptions);
	_btnOptions->setTooltip("STR_OPTIONS");
	_btnOptions->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnOptions->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_txtSelectedIndex->setColor(Palette::blockOffset(8));
	_txtSelectedIndex->setHighContrast(true);
	std::string txtIndex = "Index:";
	_txtSelectedIndex->setText(txtIndex.c_str());

	_edtSelectedIndex->setColor(Palette::blockOffset(8));
	_edtSelectedIndex->setHighContrast(true);
	_edtSelectedIndex->onChange((ActionHandler)&MapEditorState::edtSelectedIndexChange);
	_edtSelectedIndex->setConstraint(TEC_NUMERIC);
	std::string edtIndex = "-1";
	_edtSelectedIndex->setText(edtIndex.c_str());

	_txtEditRegister->setColor(Palette::blockOffset(8));
	_txtEditRegister->setHighContrast(true);
	std::ostringstream ss;
	ss << _editor->getEditRegisterPosition() << "/" << _editor->getEditRegisterSize();
	_txtEditRegister->setText(ss.str().c_str());

	_tileSelection->setColor(232); // Goal of background color 235
	_tileSelection->onMouseClick((ActionHandler)&MapEditorState::tileSelectionClick);

	_tileSelectionPageCount->setColor(224);
	_tileSelectionPageCount->setHighContrast(true);
	int mapDataObjects = 0;
	for (auto mapDataSet : *_save->getMapDataSets())
	{
		mapDataObjects += mapDataSet->getSize();
	}
	_tileSelectionLastPage = mapDataObjects / (_tileSelectionColumns * _tileSelectionRows);
	ss.str(std::string());
	ss << _tileSelectionCurrentPage + 1 << "/" << _tileSelectionLastPage + 1;
	_tileSelectionPageCount->setText(ss.str().c_str());
	_tileSelectionPageCount->setVisible(false);
	_tileSelectionPageCount->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);

	size_t arrowColor = 8;
	if (_tileSelectionCurrentPage != _tileSelectionLastPage)
	{
		arrowColor = 224;
		_tileSelectionLeftArrow->setHighContrast(true);
		_tileSelectionRightArrow->setHighContrast(true);
	}

	_tileSelectionLeftArrow->setColor(arrowColor);
	_tileSelectionLeftArrow->onMouseClick((ActionHandler)&MapEditorState::tileSelectionLeftArrowClick);
	_tileSelectionLeftArrow->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	_tileSelectionLeftArrow->setText(std::string("<<").c_str());
	_tileSelectionLeftArrow->setVisible(false);

	_tileSelectionRightArrow->setColor(arrowColor);
	_tileSelectionRightArrow->onMouseClick((ActionHandler)&MapEditorState::tileSelectionRightArrowClick);
	_tileSelectionRightArrow->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	_tileSelectionRightArrow->setText(std::string(">>").c_str());
	_tileSelectionRightArrow->setVisible(false);

	_panelTileSelection->setColor(232); // nice purple
	_panelTileSelection->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_panelTileSelection->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_panelTileSelection->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	drawTileSelectionGrid();
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(false);
	}
	_panelTileSelection->setVisible(false);
}


/**
 * Deletes the MapEditorState.
 */
MapEditorState::~MapEditorState()
{
	delete _animTimer;
	delete _gameTimer;
	//delete _battleGame;
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

	if (_save->getDebugMode())
	{
		std::ostringstream ss;
		ss << "Clicked " << pos;
		debug(ss.str());
	}

	// Have the map editor capture the mouse input here for editing tiles
	if (_editor)
	{
		Tile *selectedTile = _save->getTile(pos);
		_editor->handleEditorInput(action, selectedTile);
		std::ostringstream ss;
		ss << _editor->getEditRegisterPosition() << "/" << _editor->getEditRegisterSize();
		_txtEditRegister->setText(ss.str().c_str());
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
void MapEditorState::btnOptionsClick(Action *)
{
	_game->pushState(new PauseState(OPT_MAPEDITOR));
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
 * Shows a debug message in the topleft corner.
 * @param message Debug message.
 */
void MapEditorState::debug(const std::string &message)
{
	if (_save->getDebugMode())
	{
		_txtDebug->setText(message);
	}
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
				bool altPressed = (SDL_GetModState() & KMOD_ALT) != 0;

				// Map Editor undo/redo
				if (key == SDLK_z && ctrlPressed)
				{
					if (shiftPressed)
					{
						_editor->redo();
					}
					else
					{
						_editor->undo();
					}

					_map->draw();

					std::ostringstream ss;
					ss << _editor->getEditRegisterPosition() << "/" << _editor->getEditRegisterSize();
					_txtEditRegister->setText(ss.str().c_str());
				}
				// Map Editor save
				else if (key == SDLK_s && ctrlPressed)
				{
					_editor->saveMapFile(_editor->getMapName());
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
void MapEditorState::mouseInIcons(Action *)
{
	_mouseOverIcons = true;
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 * @param action Pointer to an action.
 */
void MapEditorState::mouseOutIcons(Action *)
{
	_mouseOverIcons = false;
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
		_txtTooltip->setText(tr(_currentTooltip));
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
		if (*i != _map && *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX / 2);
			(*i)->setY((*i)->getY() + dY);
		}
		else if (*i != _map && *i != _txtDebug)
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
 * Changes which map tile index the map editor has selected
 * @param action Pointer to an action.
 */
void MapEditorState::edtSelectedIndexChange(Action *action)
{
	if (!_editor)
		return;

	if (_edtSelectedIndex->getText().size() == 0 &&
		(action->getDetails()->key.keysym.sym == SDLK_RETURN ||
		action->getDetails()->key.keysym.sym == SDLK_KP_ENTER))
	{
		_edtSelectedIndex->setText(std::to_string(_editor->getSelectedMapDataID()));
		_edtSelectedIndex->setFocus(false);
	}
	else
	{
		std::string input = _edtSelectedIndex->getText();
		int selectedIndex = 0;

		// Try to convert input to a number.
		try
		{
			selectedIndex = stoi(input);

			if (selectedIndex < -1)
			{
				selectedIndex = -1;
			}
			
			int maxIndex = 0;
			for (auto i : *_save->getMapDataSets())
			{
				maxIndex += i->getSize();
			}
			if (selectedIndex > maxIndex - 1)
			{
				selectedIndex = maxIndex;
			}

			if (action->getDetails()->key.keysym.sym == SDLK_RETURN ||
				action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
			{
				_edtSelectedIndex->setText(std::to_string(selectedIndex));
			}

			_editor->setSelectedMapDataID(selectedIndex);
			drawTileSpriteOnSurface(_tileSelection, selectedIndex);
		}
		catch (...)
		{
			if (action->getDetails()->key.keysym.sym == SDLK_RETURN ||
				action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
			{
				_edtSelectedIndex->setText(std::to_string(selectedIndex));
			}
		}
	}
}

/**
 * Toggles the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionClick(Action *action)
{
	_tileSelectionPageCount->setVisible(!_tileSelectionPageCount->getVisible());
	_tileSelectionLeftArrow->setVisible(!_tileSelectionLeftArrow->getVisible());
	_tileSelectionRightArrow->setVisible(!_tileSelectionRightArrow->getVisible());
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(!_panelTileSelection->getVisible());
	}
	_panelTileSelection->setVisible(!_panelTileSelection->getVisible());

	if (_editor->getSelectedMapDataID() > -0)
		drawTileSpriteOnSurface(_tileSelection, _editor->getSelectedMapDataID());
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
	_tileSelectionPageCount->setText(ss.str().c_str());
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
	_tileSelectionPageCount->setText(ss.str().c_str());
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
		_editor->setSelectedMapDataID(index);
		_edtSelectedIndex->setText(std::to_string(index));
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