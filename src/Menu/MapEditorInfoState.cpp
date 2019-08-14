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
#include "MapEditorInfoState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Battlescape/MapEditor.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/Mod.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/RuleTerrain.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the current map information window for the the map editor
 * @param game Pointer to the core game.
 */
MapEditorInfoState::MapEditorInfoState()
{
    _screen = false;

    _window = new Window(this, 200, 160, 60, 20, POPUP_BOTH);
    _txtTitle = new Text(200, 17, 68, 29);

    _btnReturn = new TextButton(90, 16, 115, 153);

    _txtName = new Text(184, 8, 68, 49);
    _txtSize = new Text(184, 8, 68, 59);
    _txtTerrain = new Text(184, 8, 68, 69);

    _lstTerrain = new TextList(168, 64, 84, 79);

	// Set palette
	setInterface("optionsMenu", false, _game->getSavedGame()->getSavedBattle());

    add(_window, "window", "optionsMenu");
    add(_txtTitle, "text", "optionsMenu");
    add(_btnReturn, "button", "optionsMenu");
    add(_txtName, "text", "optionsMenu");
    add(_txtSize, "text", "optionsMenu");
    add(_txtTerrain, "text", "optionsMenu");
    add(_lstTerrain, "list", "saveMenus");

    centerAllSurfaces();

    // Set up objects
	applyBattlescapeTheme();
	setWindowBackground(_window, "mainMenu");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_MAP_EDITOR_INFO"));

	_btnReturn->setText(tr("STR_RETURN_TO_PREVIOUS_MENU"));
	_btnReturn->onMouseClick((ActionHandler)&MapEditorInfoState::btnReturnClick);
	_btnReturn->onKeyboardPress((ActionHandler)&MapEditorInfoState::btnReturnClick, Options::keyOk);
	_btnReturn->onKeyboardPress((ActionHandler)&MapEditorInfoState::btnReturnClick, Options::keyCancel);

    std::ostringstream text;
    std::string name = _game->getMapEditor()->getMapName().size() == 0 ? std::string(tr("STR_EMPTY_MAP_NAME")) : _game->getMapEditor()->getMapName();
    text << tr("STR_EDITOR_MAP_NAME").arg(name);
    _txtName->setText(text.str());

    text.str(std::string());
    text << tr("STR_EDITOR_MAP_SIZE").arg(_game->getSavedGame()->getSavedBattle()->getMapSizeX()).arg(_game->getSavedGame()->getSavedBattle()->getMapSizeY()).arg(_game->getSavedGame()->getSavedBattle()->getMapSizeZ());
    _txtSize->setText(text.str());

    _txtTerrain->setText(tr("STR_EDITOR_TERRAINS"));

    _lstTerrain->setColumns(1, 90);
    _lstTerrain->setAlign(ALIGN_LEFT);
    _lstTerrain->setSelectable(false);
	_lstTerrain->setBackground(_window);
    _lstTerrain->clearList();
    for (const auto &i : *_game->getSavedGame()->getSavedBattle()->getMapDataSets())
    {
        _lstTerrain->addRow(1, i->getName().c_str());
    }
}

/**
 * Cleans up the state
 */
MapEditorInfoState::~MapEditorInfoState()
{

}

/**
 * Returns to the previous menu
 * @param action Pointer to an action.
 */
void MapEditorInfoState::btnReturnClick(Action *)
{
    _game->popState();
}

}