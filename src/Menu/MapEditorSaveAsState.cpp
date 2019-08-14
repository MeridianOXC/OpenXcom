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
#include <sstream>
#include "MapEditorSaveAsState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Battlescape/MapEditor.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the Save Map As window
 * Yes, I see the thing in the state name.
 * @param game Pointer to the core game.
 */
MapEditorSaveAsState::MapEditorSaveAsState() : _mapName("")
{
    _screen = false;

    _window = new Window(this, 200, 72, 60, 64, POPUP_BOTH);
    _txtTitle = new Text(200, 17, 68, 73);

    _btnOk = new TextButton(90, 16, 68, 109);
    _btnCancel = new TextButton(90, 16, 160, 109);

    _txtMapName = new Text(92, 8, 68, 93);

    _edtMapName = new TextEdit(this, 100, 8, 160, 93);

	// Set palette
	setInterface("optionsMenu", false, _game->getSavedGame()->getSavedBattle());

    add(_window, "window", "optionsMenu");
    add(_txtTitle, "text", "optionsMenu");
    add(_btnOk, "button", "optionsMenu");
    add(_btnCancel, "button", "optionsMenu");
    add(_txtMapName, "text", "optionsMenu");
    add(_edtMapName, "text", "optionsMenu");

    centerAllSurfaces();

    // Set up objects
	applyBattlescapeTheme();
	setWindowBackground(_window, "mainMenu");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_MAP_EDITOR_SAVEAS"));

    _txtMapName->setText(tr("STR_ENTER_MAP_NAME"));

    _mapName = _game->getMapEditor()->getMapName();
    std::string name = _mapName.size() == 0 ? std::string(tr("STR_EMPTY_MAP_NAME")) : _mapName;

    _edtMapName->onChange((ActionHandler)&MapEditorSaveAsState::edtMapNameOnChange);
    _edtMapName->onMouseClick((ActionHandler)&MapEditorSaveAsState::edtMapNameOnClick);
    _edtMapName->setText(name);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&MapEditorSaveAsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&MapEditorSaveAsState::btnOkClick, Options::keyOk);
    _btnOk->setVisible(_mapName.size() != 0);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorSaveAsState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorSaveAsState::btnCancelClick, Options::keyCancel);
}

/**
 * Cleans up the state
 */
MapEditorSaveAsState::~MapEditorSaveAsState()
{

}

/**
 * Handles changing the map name
 * @param action Pointer to an action.
 */
void MapEditorSaveAsState::edtMapNameOnChange(Action *action)
{
    if (action->getDetails()->key.keysym.sym == Options::keyOk)
    {
        if (_edtMapName->getText().size() == 0)
        {
            if (_mapName.size() == 0)
            {
                _edtMapName->setText(tr("STR_EMPTY_MAP_NAME"));
            }
            else
            {
                _edtMapName->setText(_mapName);
            }
            _edtMapName->setFocus(false);
        }
        else
        {
            std::string input = _edtMapName->getText();

            // Sanitize the input for saving to a file
            // TODO much later - move this function to a file manager/helper for maps?
            std::string forbidden = "\\/:?\"<>|";
            for (std::string::iterator it = input.begin(); it != input.end(); ++it)
            {
                if (forbidden.find(*it) != std::string::npos)
                {
                    *it = '_';
                }
            }

            _mapName = input;
            _edtMapName->setText(input);
        }
    }
    else if (action->getDetails()->key.keysym.sym == Options::keyCancel)
    {
        if (_mapName.size() == 0)
        {
            _edtMapName->setText(tr("STR_EMPTY_MAP_NAME"));
        }
        else
        {
            _edtMapName->setText(_mapName);
        }
        _edtMapName->setFocus(false);
    }

    _btnOk->setVisible(_mapName.size() != 0);
}

/**
 * Clears the map name text when clicking on it and no string is set
 * @param action Pointer to an action.
 */
void MapEditorSaveAsState::edtMapNameOnClick(Action *)
{
    if (_edtMapName->getText().compare(std::string(tr("STR_EMPTY_MAP_NAME"))) == 0)
    {
        _edtMapName->setText("");
        _edtMapName->setFocus(true);
    }
}

/**
 * Saves and returns to the previous menu
 *
 */
void MapEditorSaveAsState::btnOkClick(Action *)
{
    _game->getMapEditor()->setMapName(_mapName);
    _game->getMapEditor()->saveMapFile(_mapName);
    _game->popState();
}

/**
 * Returns to the previous menu without saving
 * @param action Pointer to an action.
 */
void MapEditorSaveAsState::btnCancelClick(Action *)
{
    _game->popState();
}

}