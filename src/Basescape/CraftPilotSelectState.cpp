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
#include "CraftPilotSelectState.h"
#include <sstream>
#include <algorithm>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleSoldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Armor window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
CraftPilotSelectState::CraftPilotSelectState(Base *base, size_t craft) : _base(base), _craft(craft)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 232, 156, 44, 28, POPUP_BOTH);
	_btnCancel = new TextButton(180, 16, 70, 158);
	_txtTitle = new Text(222, 9, 49, 36);
	_txtName = new Text(120, 9, 60, 52);
	_txtFiringAcc = new Text(20, 9, 190, 52);
	_txtReactions = new Text(20, 9, 210, 52);
	_txtBravery = new Text(20, 9, 230, 52);
	_lstPilot = new TextList(200, 80, 53, 68);

	setInterface("craftPilotsSelect");

	add(_window, "window", "craftPilotsSelect");
	add(_btnCancel, "button", "craftPilotsSelect");
	add(_txtTitle, "text2", "craftPilotsSelect");
	add(_txtName, "text1", "craftPilotsSelect");
	add(_txtFiringAcc, "text1", "craftPilotsSelect");
	add(_txtReactions, "text1", "craftPilotsSelect");
	add(_txtBravery, "text1", "craftPilotsSelect");
	add(_lstPilot, "list", "craftPilotsSelect");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftPilotsSelect");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&CraftPilotSelectState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&CraftPilotSelectState::btnCancelClick, Options::keyCancel);

	Craft *c = _base->getCrafts()->at(_craft);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_PILOT"));

	_txtName->setText(tr("STR_NAME"));

	_txtFiringAcc->setText(tr("STR_FIRING_ACCURACY_ABBREVIATION"));
	_txtReactions->setText(tr("STR_REACTIONS_ABBREVIATION"));
	_txtBravery->setText(tr("STR_BRAVERY_ABBREVIATION"));

	_lstPilot->setColumns(4, 124, 20, 20, 20);
	_lstPilot->setAlign(ALIGN_RIGHT);
	_lstPilot->setAlign(ALIGN_LEFT, 0);
	_lstPilot->setSelectable(true);
	_lstPilot->setBackground(_window);
	_lstPilot->setMargin(8);

	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		// must be on board & able to drive
		if ((*i)->getCraft() == c && (*i)->getRules()->getAllowPiloting())
		{
			// is not assigned yet
			if (!c->isPilot((*i)->getId()))
			{
				_pilot.push_back((*i)->getId());

				std::ostringstream ss1;
				ss1 << (*i)->getStatsWithSoldierBonusesOnly()->firing;
				std::ostringstream ss2;
				ss2 << (*i)->getStatsWithSoldierBonusesOnly()->reactions;
				std::ostringstream ss3;
				ss3 << (*i)->getStatsWithSoldierBonusesOnly()->bravery;
				_lstPilot->addRow(4, (*i)->getName(false).c_str(), ss1.str().c_str(), ss2.str().c_str(), ss3.str().c_str());
			}
		}
	}

	_lstPilot->onMouseClick((ActionHandler)&CraftPilotSelectState::lstPilotClick);
}

/**
 *
 */
CraftPilotSelectState::~CraftPilotSelectState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftPilotSelectState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Adds the selected pilot to the list and returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftPilotSelectState::lstPilotClick(Action *)
{
	int pilotId = _pilot[_lstPilot->getSelectedRow()];
	Craft *c = _base->getCrafts()->at(_craft);
	c->addPilot(pilotId);

	_game->popState();
}

}
