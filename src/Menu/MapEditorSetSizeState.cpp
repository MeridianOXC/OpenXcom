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
#include "MapEditorSetSizeState.h"
#include "MapEditorMenuState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Battlescape/MapEditor.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the window for setting the size of a new map in the editor
 * @param game Pointer to the core game.
 * @param parent Pointer to the map editor menu
 */
MapEditorSetSizeState::MapEditorSetSizeState(MapEditorMenuState *parent) : _parent(parent), _x(10), _y(10), _z(4), _mapName("")
{
    _screen = false;

    _window = new Window(this, 200, 100, 60, 50, POPUP_BOTH);
    _txtTitle = new Text(200, 17, 60, 59);

    _btnOk = new TextButton(90 , 16, 68, 126);
    _btnCancel = new TextButton(90 , 16, 160, 126);

    _txtX = new Text(100, 8, 68, 80);
    _txtY = new Text(100, 8, 68, 90);
    _txtZ = new Text(100, 8, 68, 100);
    _txtMapName = new Text(100, 8, 68, 110);

    _edtX = new TextEdit(this, 30, 8, 160, 80);
    _edtY = new TextEdit(this, 30, 8, 160, 90);
    _edtZ = new TextEdit(this, 30, 8, 160, 100);
    _edtMapName = new TextEdit(this, 80, 8, 160, 110);

	// Set palette
	setInterface("mainMenu");

    add(_window, "window", "mainMenu");
    add(_txtTitle, "text", "mainMenu");
    add(_btnOk, "button", "mainMenu");
    add(_btnCancel, "button", "mainMenu");
    add(_txtX, "text", "mainMenu");
    add(_txtY, "text", "mainMenu");
    add(_txtZ, "text", "mainMenu");
    add(_txtMapName, "text", "mainMenu");
    add(_edtX, "text", "mainMenu");
    add(_edtY, "text", "mainMenu");
    add(_edtZ, "text", "mainMenu");
    add(_edtMapName, "text", "mainMenu");

    centerAllSurfaces();

    // Set up objects
	setWindowBackground(_window, "mainMenu");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SET_MAP_SIZES"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&MapEditorSetSizeState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&MapEditorSetSizeState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorSetSizeState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorSetSizeState::btnCancelClick, Options::keyCancel);

    _txtX->setText(tr("STR_MAP_SIZE_X"));
    _txtY->setText(tr("STR_MAP_SIZE_Y"));
    _txtZ->setText(tr("STR_MAP_SIZE_Z"));
    _txtMapName->setText(tr("STR_ENTER_MAP_NAME"));

    _edtX->onChange((ActionHandler)&MapEditorSetSizeState::edtSizeOnChange);
    _edtX->setConstraint(TEC_NUMERIC_POSITIVE);
    _edtX->setText(std::to_string(_x));

    _edtY->onChange((ActionHandler)&MapEditorSetSizeState::edtSizeOnChange);
    _edtY->setConstraint(TEC_NUMERIC_POSITIVE);
    _edtY->setText(std::to_string(_y));

    _edtZ->onChange((ActionHandler)&MapEditorSetSizeState::edtSizeOnChange);
    _edtZ->setConstraint(TEC_NUMERIC_POSITIVE);
    _edtZ->setText(std::to_string(_z));

    _edtMapName->onChange((ActionHandler)&MapEditorSetSizeState::edtMapNameOnChange);
    _edtMapName->onMouseClick((ActionHandler)&MapEditorSetSizeState::edtMapNameOnClick);
    _edtMapName->setText(tr("STR_EMPTY_MAP_NAME"));

    _editMap[_edtX] = &_x;
    _editMap[_edtY] = &_y;
    _editMap[_edtZ] = &_z;
}

/**
 * Cleans up the state
 */
MapEditorSetSizeState::~MapEditorSetSizeState()
{

}

/**
 * Handles text inputs for the map sizes
 * @param action Pointer to an action.
 */
void MapEditorSetSizeState::edtSizeOnChange(Action *action)
{
    TextEdit *sender;
    if (action->getSender() == _edtX)
        sender = _edtX;
    else if (action->getSender() == _edtY)
        sender = _edtY;
    else if (action->getSender() == _edtZ)
        sender = _edtZ;

    if (action->getDetails()->key.keysym.sym == Options::keyOk)
    {
        if (sender->getText().size() == 0)
        {
            sender->setText(std::to_string(*_editMap[sender]));
            sender->setFocus(false);
        }
        else
        {
            std::string input = sender->getText();
            int newSize = *_editMap[sender];

            // Try to convert input to a number
            // If we have an error, return to the previous number
            try
            {
                newSize = std::stoi(input);

                // Round to the nearest 10 for width or length
                if ((sender == _edtX || sender == _edtY) && newSize % 10 != 0)
                {
                    newSize += newSize % 10 < 5 ? 0 : 10;
                    newSize -= newSize % 10;
                }

                *_editMap[sender] = newSize;
                sender->setText(std::to_string(newSize));
            }
            catch (...)
            {
                sender->setText(std::to_string(newSize));
            }
        }
    }
    else if (action->getDetails()->key.keysym.sym == Options::keyCancel)
    {
        sender->setText(std::to_string(*_editMap[sender]));
        sender->setFocus(false);
    }

}

/**
 * Handles changing the map name
 * @param action Pointer to an action.
 */
void MapEditorSetSizeState::edtMapNameOnChange(Action *action)
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
}

/**
 * Clears the map name text when clicking on it and no string is set
 * @param action Pointer to an action.
 */
void MapEditorSetSizeState::edtMapNameOnClick(Action *)
{
    if (_edtMapName->getText().compare(std::string(tr("STR_EMPTY_MAP_NAME"))) == 0)
    {
        _edtMapName->setText("");
        _edtMapName->setFocus(true);
    }
}

/**
 * Confirms the sizes and returns to the map editor menu to start editing
 * @param action Pointer to an action.
 */
void MapEditorSetSizeState::btnOkClick(Action *)
{
    _game->getSavedGame()->getSavedBattle()->initMap(_x, _y, _z, true);
    _game->getMapEditor()->setMapName(_mapName);
    _game->popState();
    _parent->startEditor();
}

/**
 * Cancels setting the sizes and returns to the map editor menu
 * @param action Pointer to an action
 */
void MapEditorSetSizeState::btnCancelClick(Action *)
{
    // Clean up the save and map editor created by clicking OK on the Map Editor menu
    _game->setSavedGame(0);
    _game->setMapEditor(0);
    _game->popState();
}

}