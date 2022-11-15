/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "ConfirmEndMissionState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Engine/Action.h"
#include "../Savegame/SavedBattleGame.h"
#include "BattlescapeState.h"
#include "BattlescapeGame.h"
#include "../Engine/Options.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the ConfirmEndMission window.
 * @param battleGame Pointer to the saved game.
 */
ConfirmEndMissionState::ConfirmEndMissionState(SavedBattleGame *battleGame, int wounded, BattlescapeGame *parent) : _battleGame(battleGame), _wounded(wounded), _parent(parent)
{
	// Create objects
	_screen = false;
	_window = new Window(this, 320, 144, 0, 0);
	_txtTitle = new Text(304, 17, 16, 26);
	_txtWounded = new Text(304, 17, 16, 54);
	_txtConfirm = new Text(320, 17, 0, 80);
	_btnOk = new TextButton(120, 16, 16, 110);
	_btnCancel = new TextButton(120, 16, 184, 110);

	// Set palette
	_battleGame->setPaletteByDepth(this);

	add(_window, "messageWindowBorder", "battlescape");
	add(_txtTitle, "messageWindows", "battlescape");
	add(_txtWounded, "messageWindows", "battlescape");
	add(_txtConfirm, "messageWindows", "battlescape");
	add(_btnOk, "messageWindowButtons", "battlescape");
	add(_btnCancel, "messageWindowButtons", "battlescape");

	// Set up objects
	_window->setHighContrast(true);
	_window->setBackground(_game->getMod()->getSurface("TAC00.SCR"));

	_txtTitle->setBig();
	_txtTitle->setHighContrast(true);
	_txtTitle->setText(tr("STR_MISSION_OVER"));		

	_txtWounded->setBig();
	_txtWounded->setHighContrast(true);
	_txtWounded->setText(tr("STR_UNITS_WITH_FATAL_WOUNDS", _wounded));

	_txtConfirm->setBig();
	_txtConfirm->setAlign(ALIGN_CENTER);
	_txtConfirm->setHighContrast(true);
	_txtConfirm->setText(tr("STR_END_MISSION_QUESTION"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setHighContrast(true);
	_btnOk->onMouseClick((ActionHandler)&ConfirmEndMissionState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ConfirmEndMissionState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->setHighContrast(true);
	_btnCancel->onMouseClick((ActionHandler)&ConfirmEndMissionState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&ConfirmEndMissionState::btnCancelClick, Options::keyCancel);
	_btnCancel->onKeyboardPress((ActionHandler)&ConfirmEndMissionState::btnCancelClick, Options::keyBattleAbort);

	centerAllSurfaces();
}

/**
 *
 */
ConfirmEndMissionState::~ConfirmEndMissionState()
{

}

/**
 * Confirms mission end.
 * @param action Pointer to an action.
 */
void ConfirmEndMissionState::btnOkClick(Action *)
{
	_game->popState();
	_parent->requestEndTurn(false);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ConfirmEndMissionState::btnCancelClick(Action *)
{
	_game->popState();
}


}
