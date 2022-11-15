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
#include "CraftPilotsState.h"
#include "CraftPilotSelectState.h"
#include <sstream>
#include <algorithm>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleSoldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Pilots screen.
 */
CraftPilotsState::CraftPilotsState(Base *base, size_t craft) : _base(base), _craft(craft)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(300, 20, 10, 170);
	_txtTitle = new Text(310, 17, 5, 12);
	_txtFiringAcc = new Text(108, 9, 74, 32);
	_txtReactions = new Text(58, 9, 182, 32);
	_txtBravery = new Text(58, 9, 240, 32);
	_txtPilots = new Text(150, 9, 10, 40);
	_lstPilots = new TextList(288, 32, 10, 48);
	_btnAdd = new TextButton(146, 20, 10, 96);
	_btnRemoveAll = new TextButton(146, 20, 164, 96);

	_txtRequired = new Text(288, 9, 10, 84);

	_txtAccuracyBonus = new Text(100, 9, 10, 128);
	_txtAccuracyBonusValue = new Text(150, 9, 110, 128);
	_txtDodgeBonus = new Text(100, 9, 10, 140);
	_txtDodgeBonusValue = new Text(150, 9, 110, 140);
	_txtApproachSpeed = new Text(100, 9, 10, 152);
	_txtApproachSpeedValue = new Text(150, 9, 110, 152);

	// Set palette
	setInterface("craftPilots");

	add(_window, "window", "craftPilots");
	add(_btnOk, "button", "craftPilots");
	add(_txtTitle, "text1", "craftPilots");
	add(_txtFiringAcc, "text1", "craftPilots");
	add(_txtReactions, "text1", "craftPilots");
	add(_txtBravery, "text1", "craftPilots");
	add(_txtPilots, "text1", "craftPilots");
	add(_lstPilots, "list", "craftPilots");
	add(_btnAdd, "button", "craftPilots");
	add(_btnRemoveAll, "button", "craftPilots");
	add(_txtRequired, "text1", "craftPilots");
	add(_txtAccuracyBonus, "text1", "craftPilots");
	add(_txtAccuracyBonusValue, "text2", "craftPilots");
	add(_txtDodgeBonus, "text1", "craftPilots");
	add(_txtDodgeBonusValue, "text2", "craftPilots");
	add(_txtApproachSpeed, "text1", "craftPilots");
	add(_txtApproachSpeedValue, "text2", "craftPilots");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftPilots");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftPilotsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftPilotsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&CraftPilotsState::btnOkClick, Options::keyCancel);

	Craft *c = _base->getCrafts()->at(_craft);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PILOTS_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtFiringAcc->setText(tr("STR_FIRING_ACCURACY"));
	_txtFiringAcc->setAlign(ALIGN_RIGHT);

	_txtReactions->setText(tr("STR_REACTIONS"));
	_txtReactions->setAlign(ALIGN_RIGHT);

	_txtBravery->setText(tr("STR_BRAVERY"));
	_txtBravery->setAlign(ALIGN_RIGHT);

	_txtPilots->setText(tr("STR_PILOTS"));

	_lstPilots->setColumns(5, 114, 58, 58, 58, 0);
	_lstPilots->setAlign(ALIGN_RIGHT);
	_lstPilots->setAlign(ALIGN_LEFT, 0);
	_lstPilots->setDot(true);

	_txtRequired->setText(tr("STR_PILOTS_REQUIRED").arg(c->getRules()->getPilots()));

	_btnAdd->setText(tr("STR_ADD_PILOT"));
	_btnAdd->onMouseClick((ActionHandler)&CraftPilotsState::btnAddClick);
	_btnRemoveAll->setText(tr("STR_REMOVE_ALL_PILOTS"));
	_btnRemoveAll->onMouseClick((ActionHandler)&CraftPilotsState::btnRemoveAllClick);

	_txtAccuracyBonus->setText(tr("STR_ACCURACY_BONUS"));
	_txtDodgeBonus->setText(tr("STR_DODGE_BONUS"));
	_txtApproachSpeed->setText(tr("STR_APPROACH_SPEED"));

	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getCraft() == c)
		{
			(*i)->prepareStatsWithBonuses(_game->getMod()); // refresh soldier bonuses
		}
	}
}

/**
 *
 */
CraftPilotsState::~CraftPilotsState()
{

}

/**
* Initializes the state.
*/
void CraftPilotsState::init()
{
	State::init();
	updateUI();
}

/**
* Updates the data on the screen.
*/
void CraftPilotsState::updateUI()
{
	_lstPilots->clearList();

	Craft *c = _base->getCrafts()->at(_craft);

	const std::vector<Soldier*> pilots = c->getPilotList(false);
	for (std::vector<Soldier*>::const_iterator i = pilots.begin(); i != pilots.end(); ++i)
	{
		std::ostringstream ss1;
		ss1 << (*i)->getStatsWithSoldierBonusesOnly()->firing;
		std::ostringstream ss2;
		ss2 << (*i)->getStatsWithSoldierBonusesOnly()->reactions;
		std::ostringstream ss3;
		ss3 << (*i)->getStatsWithSoldierBonusesOnly()->bravery;
		_lstPilots->addRow(5, (*i)->getName(false).c_str(), ss1.str().c_str(), ss2.str().c_str(), ss3.str().c_str(), "");
	}

	std::ostringstream ss1;
	int accBonus = c->getPilotAccuracyBonus(pilots, _game->getMod());
	ss1 << (accBonus > 0 ? "+" : "") << accBonus << "%";
	_txtAccuracyBonusValue->setText(ss1.str().c_str());

	std::ostringstream ss2;
	int dodgeBonus = c->getPilotDodgeBonus(pilots, _game->getMod());
	ss2 << (dodgeBonus > 0 ? "+" : "") << dodgeBonus << "%";
	_txtDodgeBonusValue->setText(ss2.str().c_str());

	std::ostringstream ss3;
	int approachSpeed = c->getPilotApproachSpeedModifier(pilots, _game->getMod());
	switch (approachSpeed)
	{
	case 1:
		ss3 << tr("STR_COWARDLY");
		break;
	case 2:
		ss3 << tr("STR_NORMAL");
		break;
	case 3:
		ss3 << tr("STR_BOLD");
		break;
	case 4:
		ss3 << tr("STR_VERY_BOLD");
		break;
	default:
		ss3 << tr("STR_UNKNOWN");
		break;
	}
	_txtApproachSpeedValue->setText(ss3.str().c_str());

	_btnAdd->setVisible((int)(_lstPilots->getTexts()) < c->getRules()->getPilots());

	int availablePilots = 0;
	for (auto soldier : *_base->getSoldiers())
	{
		// must be on board & able to drive
		if (soldier->getCraft() == c && soldier->getRules()->getAllowPiloting())
		{
			availablePilots++;
		}
	}
	_btnRemoveAll->setVisible(availablePilots != c->getRules()->getPilots());
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftPilotsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
* Opens a Pilot Selection screen.
* @param action Pointer to an action.
*/
void CraftPilotsState::btnAddClick(Action *)
{
	_game->pushState(new CraftPilotSelectState(_base, _craft));
}

/**
* Clears the pilot list.
* @param action Pointer to an action.
*/
void CraftPilotsState::btnRemoveAllClick(Action *)
{
	Craft *c = _base->getCrafts()->at(_craft);

	c->removeAllPilots();

	updateUI();
}

}
