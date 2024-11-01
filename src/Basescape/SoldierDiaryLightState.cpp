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
#include "SoldierDiaryLightState.h"
#include <sstream>
#include <string>
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDiary.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Diary Light window.
 * @param soldier Pointer to the soldier to get info from.
 */
SoldierDiaryLightState::SoldierDiaryLightState(Soldier* soldier) : _soldier(soldier)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 240, 160, 40, 24, POPUP_BOTH);
	_btnOk = new TextButton(100, 16, 110, 160);
	_txtTitle = new Text(220, 9, 50, 36);
	_lstStats = new TextList(177, 96, 74, 56);

	// Set palette
	setStandardPalette("PAL_BATTLESCAPE");

	add(_window, "window", "soldierDiaryLight");
	add(_btnOk, "button", "soldierDiaryLight");
	add(_txtTitle, "text", "soldierDiaryLight");
	add(_lstStats, "list", "soldierDiaryLight");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierDiaryLight");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&SoldierDiaryLightState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SoldierDiaryLightState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&SoldierDiaryLightState::btnOkClick, Options::keyCancel);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_NEUTRALIZATIONS_BY_WEAPON"));

	_lstStats->setColumns(2, 156, 20);
	_lstStats->setBackground(_window);
	_lstStats->setDot(true);

	for (const auto& mapItem : _soldier->getDiary()->getWeaponTotal())
	{
		std::ostringstream ss;
		ss << mapItem.second;
		_lstStats->addRow(2, tr(mapItem.first).c_str(), ss.str().c_str());
	}

	// switch to battlescape theme if called from inventory
	applyBattlescapeTheme("soldierDiaryLight");
}

/**
 *
 */
SoldierDiaryLightState::~SoldierDiaryLightState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierDiaryLightState::btnOkClick(Action *)
{
	_game->popState();
}

}
