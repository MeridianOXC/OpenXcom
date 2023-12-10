/*
 * Copyright 2010-2016 OpenXcom Developers.
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
#include "CraftNotEnoughPilotsState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Engine/Options.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Basescape/CraftInfoState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in a Craft Not Enough Pilots window.
 * @param craft Relevant craft.
 */
CraftNotEnoughPilotsState::CraftNotEnoughPilotsState(Craft *craft) : _craft(craft)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_btnOk = new TextButton(108, 18, 48, 150);
	_btnAssignPilots = new TextButton(108, 18, 164, 150);
	_txtMessage = new Text(246, 96, 37, 42);

	// Set palette
	setInterface("craftPilotError");

	add(_window, "window", "craftPilotError");
	add(_btnOk, "button", "craftPilotError");
	add(_btnAssignPilots, "button", "craftPilotError");
	add(_txtMessage, "text1", "craftPilotError");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftPilotError");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftNotEnoughPilotsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftNotEnoughPilotsState::btnOkClick, Options::keyCancel);

	_btnAssignPilots->setText(tr("STR_ASSIGN_PILOTS"));
	_btnAssignPilots->onMouseClick((ActionHandler)&CraftNotEnoughPilotsState::btnAssignPilotsClick);
	_btnAssignPilots->onKeyboardPress((ActionHandler)&CraftNotEnoughPilotsState::btnAssignPilotsClick, Options::keyOk);
	if (_craft->getMissionComplete() || _craft->getStatus() == "STR_OUT")
	{
		_btnAssignPilots->setVisible(false);
	}

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap(true);
	_txtMessage->setText(tr("STR_NOT_ENOUGH_PILOTS").arg(_craft->getRules()->getPilots()));
}

/**
 *
 */
CraftNotEnoughPilotsState::~CraftNotEnoughPilotsState()
{

}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void CraftNotEnoughPilotsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens CraftInfo window.
 * @param action Pointer to an action.
 */
void CraftNotEnoughPilotsState::btnAssignPilotsClick(Action *)
{
	Base *b = _craft->getBase();
	if (b)
	{
		for (size_t i = 0; i < b->getCrafts()->size(); ++i)
		{
			if (b->getCrafts()->at(i) == _craft)
			{
				_game->popState();
				_game->pushState(new CraftInfoState(_craft->getBase(), i));
				break;
			}
		}
	}
}

}
