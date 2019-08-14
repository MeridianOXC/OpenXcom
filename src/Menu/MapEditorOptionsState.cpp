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
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "AbandonGameState.h"
#include "MapEditorInfoState.h"
#include "MapEditorSaveAsState.h"
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

	_btnInfo = new TextButton(180, 18, 70, 52);
	_btnLoad = new TextButton(180, 18, 70, 72);
	_btnSave = new TextButton(180, 18, 70, 92);
	_btnAbandon = new TextButton(180, 18, 70, 112);
	_btnOptions = new TextButton(180, 18, 70, 132);
	_btnCancel = new TextButton(180, 18, 70, 150);

	// Set palette
	setInterface("optionsMenu", false, _game->getSavedGame()->getSavedBattle());

    add(_window, "window", "optionsMenu");
    add(_txtTitle, "text", "optionsMenu");
	add(_btnInfo, "text", "optionsMenu");
	add(_btnLoad, "button", "optionsMenu");
	add(_btnSave, "button", "optionsMenu");
	add(_btnAbandon, "button", "optionsMenu");
	add(_btnOptions, "button", "optionsMenu");
	add(_btnCancel, "button", "optionsMenu");

    centerAllSurfaces();

    // Set up objects
	applyBattlescapeTheme();
	setWindowBackground(_window, "mainMenu");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_OPTIONS_UC"));

	_btnInfo->setText(tr("STR_MAP_INFO"));
	_btnInfo->onMouseClick((ActionHandler)&MapEditorOptionsState::btnInfoClick);

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
 * Opens the info screen
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnInfoClick(Action *)
{
	_game->pushState(new MapEditorInfoState());
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
	if (_game->getMapEditor()->getMapName().size() == 0)
	{
		_game->pushState(new MapEditorSaveAsState());
	}
	else
	{
    	_game->getMapEditor()->saveMapFile(_game->getMapEditor()->getMapName());
	}
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