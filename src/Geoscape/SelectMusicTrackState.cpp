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
#include "SelectMusicTrackState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Music.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/RuleInterface.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Select Music Track window.
 * @param origin Where is the dialog called from?
 */
SelectMusicTrackState::SelectMusicTrackState(SelectMusicTrackOrigin origin) : _origin(origin)
{
	_screen = false;

	int x;
	if (_origin == SMT_BATTLESCAPE)
	{
		x = 52;
	}
	else
	{
		x = 20;
	}

	// Create objects
	_window = new Window(this, 216, 160, x, 20, POPUP_BOTH);
	_txtTitle = new Text(206, 17, x + 5, 32);
	_btnCancel = new TextButton(140, 16, x + 38, 156);
	_lstTracks = new TextList(180, 96, x + 13, 52);

	// Set palette
	setInterface("selectMusicTrack", false, _game->getSavedGame() ? _game->getSavedGame()->getSavedBattle() : 0);

	add(_window, "window", "selectMusicTrack");
	add(_txtTitle, "text", "selectMusicTrack");
	add(_btnCancel, "button", "selectMusicTrack");
	add(_lstTracks, "list", "selectMusicTrack");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "selectMusicTrack");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_MUSIC_TRACK"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SelectMusicTrackState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SelectMusicTrackState::btnCancelClick, Options::keyCancel);

	_lstTracks->setColumns(1, 172);
	_lstTracks->setSelectable(true);
	_lstTracks->setBackground(_window);
	_lstTracks->setMargin(8);
	_lstTracks->setAlign(ALIGN_CENTER);
	_lstTracks->onMouseClick((ActionHandler)&SelectMusicTrackState::lstTrackClick);

	const std::string search = _origin == SMT_BATTLESCAPE ? "GMTAC" : "GMGEO";
	const std::string& currentName = _game->getMod()->getCurrentMusicTrack();
	int currentTrackIndex = -1;
	for (auto& pair : _game->getMod()->getMusicTrackList())
	{
		if (pair.first == currentName || pair.first.find(search) != std::string::npos)
		{
			_lstTracks->addRow(1, tr(pair.first).c_str());
			_tracks.push_back(pair.second);
			_trackNames.push_back(pair.first);
			if (pair.first == currentName)
			{
				currentTrackIndex = _lstTracks->getLastRowIndex();
				_lstTracks->setRowColor(currentTrackIndex, _lstTracks->getSecondaryColor());
			}
		}
	}

	// switch to battlescape theme if called from battlestate
	if (_origin == SMT_BATTLESCAPE)
	{
		applyBattlescapeTheme("selectMusicTrack");
		if (currentTrackIndex > -1)
		{
			const Element* element = _game->getMod()->getInterface("battlescape")->getElement("optionLists");
			_lstTracks->setRowColor(currentTrackIndex, element->color2);
		}
	}
}

/**
 *
 */
SelectMusicTrackState::~SelectMusicTrackState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SelectMusicTrackState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Starts playing the selected track and returns to the previous screen.
 * @param action Pointer to an action.
 */
void SelectMusicTrackState::lstTrackClick(Action *)
{
	Music *selected = _tracks[_lstTracks->getSelectedRow()];
	selected->play();
	_game->getMod()->setCurrentMusicTrack(_trackNames[_lstTracks->getSelectedRow()]);

	_game->popState();
}

}
