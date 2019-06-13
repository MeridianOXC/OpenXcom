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
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Battlescape/MapEditor.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BattlescapeState.h"
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

    _lstMaps = new TextList(148, 128, 8, 46);
    
    _txtSelectedMap = new Text(100, 8, 174, 46);
    _txtSelectedMapTerrain = new Text(100, 8, 174, 56);

	// Set palette
	setInterface("mainMenu");

	add(_window, "window", "mainMenu");
	add(_txtTitle, "text", "mainMenu");
	add(_btnOk, "button", "mainMenu");
	add(_btnCancel, "button", "mainMenu");
    add(_lstMaps, "list", "saveMenus");
    add(_txtSelectedMap, "text", "mainMenu");
    add(_txtSelectedMapTerrain, "text", "mainMenu");

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
    // TODO: populate list according to terrain/craft/ufo filter
    _mapsList.clear();

    for (auto &i : _game->getMod()->getTerrainList())
    {
        for (auto j : *_game->getMod()->getTerrain(i)->getMapBlocks())
        {
            _lstMaps->addRow(1, j->getName().c_str());
            _mapsList.push_back(std::make_pair(j->getName(), i));
        }
    }

    _selectedMap = -1;
    _txtSelectedMap->setText("");
    _txtSelectedMapTerrain->setText("");
    _btnOk->setVisible(false);
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

    RuleTerrain *terrain = _game->getMod()->getTerrain(_mapsList.at(_selectedMap).second);
	battlescapeGenerator.setTerrain(terrain);

	//_game->getSavedGame()->setDifficulty((GameDifficulty)_cbxDifficulty->getSelected());

	battlescapeGenerator.setWorldShade(0);
	//bgen.setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
	//bgen.setAlienItemlevel(_slrAlienTech->getValue());
	//bgame->setDepth(_slrDepth->getValue());

    MapBlock *block = terrain->getMapBlock(_mapsList.at(_selectedMap).first);
	battlescapeGenerator.loadMapForEditing(block);

	_game->popState();
	_game->popState();
	//_game->pushState(new BriefingState(_craft, base));

    savedBattleGame->setRandomHiddenMovementBackground(_game->getMod());
	Options::baseXResolution = Options::baseXBattlescape;
	Options::baseYResolution = Options::baseYBattlescape;
	_game->getScreen()->resetDisplay(false);

	MapEditor *editor = new MapEditor(savedBattleGame);
	BattlescapeState *battlescapeState = new BattlescapeState(editor);
	//int liveAliens = 0, liveSoldiers = 0;
	//bs->getBattleGame()->tallyUnits(liveAliens, liveSoldiers);
	_game->pushState(battlescapeState);
	_game->getSavedGame()->getSavedBattle()->setBattleState(battlescapeState);
	battlescapeState->getMap()->getCamera()->centerOnPosition(Position(0, 0, 0), true);
	battlescapeState->getMap()->refreshSelectorPosition();
	//	_game->pushState(new NextTurnState(_game->getSavedGame()->getSavedBattle(), bs));
	//	_game->pushState(new InventoryState(false, bs, 0));
	//}
	//else
	//{
	//	Options::baseXResolution = Options::baseXGeoscape;
	//	Options::baseYResolution = Options::baseYGeoscape;
	//	_game->getScreen()->resetDisplay(false);
	//	delete bs;
	//	_game->pushState(new AliensCrashState);
	//}
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