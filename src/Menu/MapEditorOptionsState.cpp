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
#include "MapEditorOptionsState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Battlescape/MapEditor.h"
#include "AbandonGameState.h"
#include "OptionsVideoState.h"
#include "OptionsGeoscapeState.h"
#include "OptionsBattlescapeState.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the map editor options window
 * @param game Pointer to the core game.
 * @param parent Pointer to the map editor menu
 */
MapEditorOptionsState::MapEditorOptionsState(OptionsOrigin origin) : _origin(origin)
{
    _screen = false;

    _window = new Window(this, 216, 160, 52, 20, POPUP_BOTH);
	_txtTitle = new Text(206, 17, 57, 32);

	_btnLoad = new TextButton(180, 18, 70, 52);
	_btnSave = new TextButton(180, 18, 70, 74);
	_btnAbandon = new TextButton(180, 18, 70, 96);
	_btnOptions = new TextButton(180, 18, 70, 122);
	_btnCancel = new TextButton(180, 18, 70, 150);

	// Set palette
	setInterface("mainMenu");

    add(_window, "window", "mainMenu");
    add(_txtTitle, "text", "mainMenu");
	add(_btnLoad, "button", "mainMenu");
	add(_btnSave, "button", "mainMenu");
	add(_btnAbandon, "button", "mainMenu");
	add(_btnOptions, "button", "mainMenu");
	add(_btnCancel, "button", "mainMenu");

    centerAllSurfaces();

    // Set up objects
	setWindowBackground(_window, "mainMenu");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_OPTIONS_UC"));

	_btnLoad->setText(tr("STR_LOAD_MAP"));
	_btnLoad->onMouseClick((ActionHandler)&MapEditorOptionsState::btnLoadClick);

	_btnSave->setText(tr("STR_SAVE_MAP"));
	_btnSave->onMouseClick((ActionHandler)&MapEditorOptionsState::btnSaveClick);

	_btnAbandon->setText(tr("STR_ABANDON_GAME"));
	_btnAbandon->onMouseClick((ActionHandler)&MapEditorOptionsState::btnAbandonClick);

	_btnOptions->setText(tr("STR_GAME_OPTIONS"));
	_btnOptions->onMouseClick((ActionHandler)&MapEditorOptionsState::btnOptionsClick);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorOptionsState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorOptionsState::btnCancelClick, Options::keyCancel);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorOptionsState::btnCancelClick, Options::keyBattleOptions);
}

/**
 * Cleans up the state
 */
MapEditorOptionsState::~MapEditorOptionsState()
{

}

/**
 * Opens the load map screen
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnLoadClick(Action *)
{

}

/**
 * Saves the current map
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnSaveClick(Action *)
{
    _game->getMapEditor()->saveMapFile(_game->getMapEditor()->getMapName());
}

/**
 * Opens the Game Options screen.
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnOptionsClick(Action *)
{
	Options::backupDisplay();
	_game->pushState(new OptionsVideoState(_origin));
}

/**
 * Opens the Abandon Game window.
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnAbandonClick(Action *)
{
	_game->pushState(new AbandonGameState(_origin));
}

/**
 * Returns to the map editor state
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnCancelClick(Action *)
{
    _game->popState();
}

}