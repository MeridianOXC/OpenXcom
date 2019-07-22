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
#include "MapEditorMenuState.h"
#include <sstream>
#include "../version.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleTerrain.h"
#include "../Mod/MapBlock.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleUfo.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/Frame.h"
#include "../Battlescape/MapEditor.h"
#include "../Battlescape/MapEditorState.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/Camera.h"
#include "../Battlescape/Map.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Map Editor Menu window.
 * @param game Pointer to the core game.
 */
MapEditorMenuState::MapEditorMenuState() : _selectedMap(-1)
{
	// Create objects
    // TODO: add filters for terrain/craft/UFO
    // TODO: clean up and make it look pretty
	_window = new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_txtTitle = new Text(320, 17, 0, 9);

	_btnOk = new TextButton(100, 16, 8, 176);
	_btnCancel = new TextButton(100, 16, 110, 176);

    _filterTerrain = new TextButton(48, 16, 8, 28);
    _filterCraft = new TextButton(48, 16, 58, 28);
    _filterUFOs = new TextButton(48, 16, 108, 28);
    _mapFilter = _filterTerrain;

    _filterTerrain->setGroup(&_mapFilter);
    _filterCraft->setGroup(&_mapFilter);
    _filterUFOs->setGroup(&_mapFilter);

    _lstMaps = new TextList(119, 104, 14, 52);
    
    _txtSelectedMap = new Text(100, 8, 170, 52);
    _txtSelectedMapTerrain = new Text(100, 8, 170, 62);

    _frameLeft = new Frame(148, 116, 8, 46);
    _frameRight = new Frame(148, 116, 164, 46);

	// Set palette
	setInterface("mainMenu");

	add(_window, "window", "mainMenu");
	add(_txtTitle, "text", "mainMenu");
	add(_btnOk, "button", "mainMenu");
	add(_btnCancel, "button", "mainMenu");
    add(_filterTerrain, "button", "mainMenu");
    add(_filterCraft, "button", "mainMenu");
    add(_filterUFOs, "button", "mainMenu");
    add(_lstMaps, "list", "saveMenus");
    add(_txtSelectedMap, "text", "mainMenu");
    add(_txtSelectedMapTerrain, "text", "mainMenu");
    add(_frameLeft, "frames", "newBattleMenu");
    add(_frameRight, "frames", "newBattleMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "mainMenu");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_MAP_EDITOR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&MapEditorMenuState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&MapEditorMenuState::btnOkClick, Options::keyOk);
    _btnOk->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorMenuState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorMenuState::btnCancelClick, Options::keyCancel);

    _filterTerrain->setText(tr("STR_TERRAIN"));
    _filterTerrain->onMouseClick((ActionHandler)&MapEditorMenuState::btnMapFilterClick);

    _filterCraft->setText(tr("STR_CRAFT"));
    _filterCraft->onMouseClick((ActionHandler)&MapEditorMenuState::btnMapFilterClick);

    _filterUFOs->setText(tr("STR_UFOS"));
    _filterUFOs->onMouseClick((ActionHandler)&MapEditorMenuState::btnMapFilterClick);

    _lstMaps->setColumns(1, 120);
    _lstMaps->setAlign(ALIGN_LEFT);
    _lstMaps->setSelectable(true);
	_lstMaps->setBackground(_window);
    _lstMaps->onMouseClick((ActionHandler)&MapEditorMenuState::lstMapsClick);
}

MapEditorMenuState::~MapEditorMenuState()
{

}

void MapEditorMenuState::init()
{
	State::init();

    populateMapsList();
}

void MapEditorMenuState::populateMapsList()
{
    _mapsList.clear();
    _lstMaps->clearList();

    if (_mapFilter == _filterTerrain)
    {
        for (auto &i : _game->getMod()->getTerrainList())
        {
            for (auto j : *_game->getMod()->getTerrain(i)->getMapBlocks())
            {
                _lstMaps->addRow(1, j->getName().c_str());
                _mapsList.push_back(std::make_pair(j->getName(), i));
            }
        }
    }
    else if (_mapFilter == _filterCraft)
    {
        for (auto &i : _game->getMod()->getCraftsList())
        {
            if (_game->getMod()->getCraft(i)->getBattlescapeTerrainData() == 0)
                continue;

            for (auto j : *_game->getMod()->getCraft(i)->getBattlescapeTerrainData()->getMapBlocks())
            {
                _lstMaps->addRow(1, j->getName().c_str());
                _mapsList.push_back(std::make_pair(j->getName(), i));
            }
        }
    }
    else if (_mapFilter == _filterUFOs)
    {
        for (auto &i : _game->getMod()->getUfosList())
        {
            if (_game->getMod()->getUfo(i)->getBattlescapeTerrainData() == 0)
                continue;

            for (auto j : *_game->getMod()->getUfo(i)->getBattlescapeTerrainData()->getMapBlocks())
            {
                _lstMaps->addRow(1, j->getName().c_str());
                _mapsList.push_back(std::make_pair(j->getName(), i));
            }
        }
    }

    _selectedMap = -1;
    _txtSelectedMap->setText("");
    _txtSelectedMapTerrain->setText("");
    _btnOk->setVisible(false);
}

/**
 * Handles clicking on the filter buttons for the type of map to display
 * @param action Pointer to an action.
 */
void MapEditorMenuState::btnMapFilterClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

    if (action->getSender() == _filterTerrain)
    {
        _mapFilter = _filterTerrain;
    }
    else if (action->getSender() == _filterCraft)
    {
        _mapFilter = _filterCraft;
    }
    else if (action->getSender() == _filterUFOs)
    {
        _mapFilter = _filterUFOs;
    }

    populateMapsList();

    // consume the action to keep the selected filter button pressed
    action->getDetails()->type = SDL_NOEVENT;
}

/**
 * Handles clicking on the list of maps.
 * @param action Pointer to an action.
 */
void MapEditorMenuState::lstMapsClick(Action *)
{
    // TODO: searching the list for a certain map
    _selectedMap = _lstMaps->getSelectedRow();
    if (_selectedMap > -1 && _selectedMap < _mapsList.size() + 1)
    {
        _txtSelectedMap->setText(_mapsList.at(_selectedMap).first);
        _txtSelectedMapTerrain->setText(_mapsList.at(_selectedMap).second);
        _btnOk->setVisible(true);
    }
    else
    {
        _selectedMap = -1;
        _txtSelectedMap->setText("");
        _txtSelectedMapTerrain->setText("");
        _btnOk->setVisible(false);
    }
}

/**
 * Starts the Map Editor.
 * @param action Pointer to an action.
 */
void MapEditorMenuState::btnOkClick(Action *)
{
    // TODO: Move handling of any battlescape objects to the MapEditor class, this menu should only be for initializing that class

	SavedGame *save = new SavedGame();
    _game->setSavedGame(save);

	SavedBattleGame *savedBattleGame = new SavedBattleGame(_game->getMod());
	_game->getSavedGame()->setBattleGame(savedBattleGame);
	BattlescapeGenerator battlescapeGenerator = BattlescapeGenerator(_game);

    RuleTerrain *terrain = 0;
    if (_mapFilter == _filterTerrain)
        terrain = _game->getMod()->getTerrain(_mapsList.at(_selectedMap).second);
    else if (_mapFilter == _filterCraft)
        terrain = _game->getMod()->getCraft(_mapsList.at(_selectedMap).second)->getBattlescapeTerrainData();
    else if (_mapFilter == _filterUFOs)
        terrain = _game->getMod()->getUfo(_mapsList.at(_selectedMap).second)->getBattlescapeTerrainData();

	battlescapeGenerator.setTerrain(terrain);
	battlescapeGenerator.setWorldShade(0);
    MapBlock *block = terrain->getMapBlock(_mapsList.at(_selectedMap).first);
	battlescapeGenerator.loadMapForEditing(block);

	_game->popState();
	_game->popState();

	Options::baseXResolution = Options::baseXBattlescape;
	Options::baseYResolution = Options::baseYBattlescape;
	_game->getScreen()->resetDisplay(false);

	MapEditor *editor = new MapEditor(savedBattleGame);
	editor->setMapName(block->getName());
	_game->setMapEditor(editor);
	MapEditorState *mapEditorState = new MapEditorState(editor);
	_game->pushState(mapEditorState);
	_game->getSavedGame()->getSavedBattle()->setMapEditorState(mapEditorState);
}

/**
 * Returns to the main menu.
 * @param action Pointer to an action.
 */
void MapEditorMenuState::btnCancelClick(Action *)
{
	//_game->setSavedGame(0);
	_game->popState();
}

}