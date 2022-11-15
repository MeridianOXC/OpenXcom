/*
 * Copyright 2010-2020 OpenXcom Developers.
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
#include "DogfightExperienceState.h"
#include <sstream>
#include <algorithm>
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Dogfight Experience screen.
 */
DogfightExperienceState::DogfightExperienceState()
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 140, 0, 30, POPUP_HORIZONTAL);
	_btnOk = new TextButton(300, 16, 10, 146);
	_txtTitle = new Text(300, 17, 10, 46);
	_txtFiringAcc = new Text(108, 9, 74, 70);
	_txtReactions = new Text(58, 9, 182, 70);
	_txtBravery = new Text(58, 9, 240, 70);
	_txtPilots = new Text(80, 9, 10, 70);
	_lstPilots = new TextList(288, 64, 10, 78);

	// Set palette
	setInterface("dogfightExperience");

	add(_window, "window", "dogfightExperience");
	add(_btnOk, "button", "dogfightExperience");
	add(_txtTitle, "text1", "dogfightExperience");
	add(_txtFiringAcc, "text2", "dogfightExperience");
	add(_txtReactions, "text2", "dogfightExperience");
	add(_txtBravery, "text2", "dogfightExperience");
	add(_txtPilots, "text2", "dogfightExperience");
	add(_lstPilots, "list", "dogfightExperience");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "dogfightExperience");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&DogfightExperienceState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&DogfightExperienceState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&DogfightExperienceState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_DAILY_PILOT_EXPERIENCE"));

	_txtFiringAcc->setText(tr("STR_FIRING_ACCURACY"));
	_txtFiringAcc->setAlign(ALIGN_RIGHT);

	_txtReactions->setText(tr("STR_REACTIONS"));
	_txtReactions->setAlign(ALIGN_RIGHT);

	_txtBravery->setText(tr("STR_BRAVERY"));
	_txtBravery->setAlign(ALIGN_RIGHT);

	_txtPilots->setText(tr("STR_NAME"));

	_lstPilots->setColumns(5, 114, 58, 58, 58, 0);
	_lstPilots->setAlign(ALIGN_RIGHT);
	_lstPilots->setAlign(ALIGN_LEFT, 0);
	_lstPilots->setDot(true);

	_lstPilots->clearList();
	for (auto* base : *_game->getSavedGame()->getBases())
	{
		for (auto* soldier : *base->getSoldiers())
		{
			auto* tmp = soldier->getDailyDogfightExperienceCache();
			if (tmp->firing > 0 || tmp->reactions > 0 || tmp->bravery > 0)
			{
				std::ostringstream ss1;
				if (tmp->firing > 0)
					ss1 << Unicode::TOK_COLOR_FLIP << '+' << tmp->firing << Unicode::TOK_COLOR_FLIP;
				std::ostringstream ss2;
				if (tmp->reactions > 0)
					ss2 << Unicode::TOK_COLOR_FLIP << '+' << tmp->reactions << Unicode::TOK_COLOR_FLIP;
				std::ostringstream ss3;
				if (tmp->bravery > 0)
					ss3 << Unicode::TOK_COLOR_FLIP << '+' << tmp->bravery << Unicode::TOK_COLOR_FLIP;
				_lstPilots->addRow(5, soldier->getName(false).c_str(), ss1.str().c_str(), ss2.str().c_str(), ss3.str().c_str(), "");
			}
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void DogfightExperienceState::btnOkClick(Action *)
{
	_game->popState();
}

}
