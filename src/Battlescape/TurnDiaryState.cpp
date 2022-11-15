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
#include "TurnDiaryState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/Mod.h"
#include "../Savegame/HitLog.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Turn Diary window.
 */
TurnDiaryState::TurnDiaryState(const HitLog *hitLog)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 216, 160, 52, 20, POPUP_BOTH);
	_txtTitle = new Text(206, 17, 57, 32);
	_btnCancel = new TextButton(140, 16, 90, 156);
	_lstTurnDiary = new TextList(180, 96, 65, 52);

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_window, "messageWindowBorder", "battlescape");
	add(_txtTitle, "messageWindows", "battlescape");
	add(_btnCancel, "messageWindowButtons", "battlescape");
	add(_lstTurnDiary, "optionLists", "battlescape");

	centerAllSurfaces();

	// Set up objects
	_window->setHighContrast(true);
	_window->setBackground(_game->getMod()->getSurface("TAC00.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setHighContrast(true);
	_txtTitle->setText(tr("STR_HIT_LOG"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->setHighContrast(true);
	_btnCancel->onMouseClick((ActionHandler)&TurnDiaryState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&TurnDiaryState::btnCancelClick, Options::keyCancel);

	_lstTurnDiary->setColumns(1, 172);
	_lstTurnDiary->setSelectable(true);
	_lstTurnDiary->setBackground(_window);
	_lstTurnDiary->setMargin(8);
	_lstTurnDiary->setAlign(ALIGN_CENTER);
	_lstTurnDiary->setHighContrast(true);
	_lstTurnDiary->setWordWrap(true);

	// in reverse order
	_lstTurnDiary->addRow(1, hitLog->getHitLogText(true).c_str());
	size_t row = 1;
	for (std::vector<std::string>::const_reverse_iterator i = hitLog->getTurnDiary().rbegin(); i != hitLog->getTurnDiary().rend(); ++i)
	{
		_lstTurnDiary->addRow(1, (*i).c_str());
		if (row % 2 != 0)
		{
			_lstTurnDiary->setRowColor(row, _lstTurnDiary->getSecondaryColor());
		}
		++row;
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void TurnDiaryState::btnCancelClick(Action *)
{
	_game->popState();
}

}
