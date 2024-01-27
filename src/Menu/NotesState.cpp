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
#include "NotesState.h"
#include <algorithm>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Notes screen.
 * @param origin Game section that originated this state.
 */
NotesState::NotesState(OptionsOrigin origin) : _origin(origin), _previousSelectedRow(-1), _selectedRow(-1)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0, POPUP_NONE);
	_txtTitle = new Text(310, 17, 5, 7);
	_txtDelete = new Text(310, 9, 5, 23);
	_lstNotes = new TextList(288, 120, 8, 42);
	_edtNote = new TextEdit(this, 268, 9, 0, 0);
	_btnSave = new TextButton(80, 16, 60, 172);
	_btnCancel = new TextButton(80, 16, 180, 172);
	_btnDelete = new ToggleTextButton(288, 16, 16, 23);

	// Set palette
	setInterface("geoscape", true, _game->getSavedGame() ? _game->getSavedGame()->getSavedBattle() : 0);

	add(_window, "window", "noteMenu");
	add(_txtTitle, "text", "noteMenu");
	add(_txtDelete, "text", "noteMenu");
	add(_lstNotes, "list", "noteMenu");
	add(_edtNote);
	add(_btnSave, "button", "noteMenu");
	add(_btnCancel, "button", "noteMenu");
	add(_btnDelete, "button", "noteMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "noteMenu");

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_NOTES"));

#ifdef __MOBILE__
	_txtDelete->setVisible(false);
	_btnDelete->setText(tr("STR_RIGHT_CLICK_TO_DELETE"));
#else
	_btnDelete->setVisible(false);
	_txtDelete->setAlign(ALIGN_CENTER);
	_txtDelete->setText(tr("STR_RIGHT_CLICK_TO_DELETE"));
#endif

	_lstNotes->setColumns(1, 288);
	_lstNotes->setSelectable(true);
	_lstNotes->setBackground(_window);
	_lstNotes->setMargin(8);
	_lstNotes->setWordWrap(true); // just in case someone modifies the save manually
	_lstNotes->onMousePress((ActionHandler)&NotesState::lstNotesPress);

	_edtNote->setColor(_lstNotes->getSecondaryColor());
	_edtNote->setVisible(false);
	_edtNote->onKeyboardPress((ActionHandler)&NotesState::edtNoteKeyPress);

	_btnSave->setText(tr("STR_SAVE_UC"));
	_btnSave->onMouseClick((ActionHandler)&NotesState::btnSaveClick);
	//_btnSave->onKeyboardPress((ActionHandler)&NotesState::btnSaveClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&NotesState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&NotesState::btnCancelClick, Options::keyCancel);
}

/**
 *
 */
NotesState::~NotesState()
{

}

/**
 * Refreshes the Notes state.
 */
void NotesState::init()
{
	State::init();

	if (_origin == OPT_BATTLESCAPE)
	{
		applyBattlescapeTheme("noteMenu");
	}

	updateList();
}

/**
 * Updates the Notes list.
 */
void NotesState::updateList()
{
	_lstNotes->clearList();

	int row = 0;
	int color = _lstNotes->getSecondaryColor();

	for (const auto& note : _game->getSavedGame()->getUserNotes())
	{
		_lstNotes->addRow(1, note.c_str());
		row++;
	}

	_lstNotes->addRow(1, tr("STR_NEW_NOTE").c_str());
	if (_origin != OPT_BATTLESCAPE)
	{
		_lstNotes->setRowColor(_lstNotes->getLastRowIndex(), color);
	}
	_lstNotes->scrollDown(true);
}

/**
 * Returns to the previous screen without saving anything.
 * @param action Pointer to an action.
 */
void NotesState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Allows to enter, edit and delete notes.
 * @param action Pointer to an action.
 */
void NotesState::lstNotesPress(Action* action)
{
	// ignore scrolling, process only LMB and RMB
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT || action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_previousSelectedRow = _selectedRow;
		_selectedRow = _lstNotes->getSelectedRow();

		// restore previous
		if (_previousSelectedRow > -1)
		{
			_lstNotes->setCellText(_previousSelectedRow, 0, _selectedNote);
		}

		// back up current
		_selectedNote = _lstNotes->getCellText(_selectedRow, 0);
	}

	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT || _btnDelete->getPressed())
	{
		if (_edtNote->isFocused())
		{
			// cancel editing the current note
			_edtNote->setText("");
			_edtNote->setVisible(false);
			_edtNote->setFocus(false, false);
			_lstNotes->setScrolling(true);
		}
		else
		{
			// any row except for the last
			if (_selectedRow >= 0 && _selectedRow < _lstNotes->getLastRowIndex())
			{
				// delete the selected note
				_selectedNote = "";
				_lstNotes->setCellText(_selectedRow, 0, _selectedNote);
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		// temporarily set to empty during editing
		_lstNotes->setCellText(_selectedRow, 0, "");

		// set the initial text for editing
		if (_selectedRow == _lstNotes->getLastRowIndex())
		{
			_edtNote->setText("");
		}
		else
		{
			_edtNote->setText(_selectedNote);
		}
		_edtNote->setX(_lstNotes->getColumnX(0));
		_edtNote->setY(_lstNotes->getRowY(_selectedRow));
		_edtNote->setVisible(true);
		_edtNote->setFocus(true, false);
		_lstNotes->setScrolling(false);
	}
}

/**
 * Updates the currently edited note.
 * @param action Pointer to an action.
 */
void NotesState::edtNoteKeyPress(Action* action)
{
	if (action->getDetails()->key.keysym.sym == SDLK_RETURN ||
		action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		// update the selected note
		_selectedNote = _edtNote->getText();
		_lstNotes->setCellText(_selectedRow, 0, _selectedNote);

		// clean up
		_edtNote->setText("");
		_edtNote->setVisible(false);
		_edtNote->setFocus(false, false);
		_lstNotes->setScrolling(true);

		// if we're adding a new note...
		if (_selectedRow == _lstNotes->getLastRowIndex())
		{
			// change color to normal
			_lstNotes->setRowColor(_lstNotes->getLastRowIndex(), _lstNotes->getColor());

			// add a new empty note
			_lstNotes->addRow(1, tr("STR_NEW_NOTE").c_str());
			if (_origin != OPT_BATTLESCAPE)
			{
				_lstNotes->setRowColor(_lstNotes->getLastRowIndex(), _lstNotes->getSecondaryColor());
			}
			_lstNotes->scrollDown(true);
		}
	}
}

/**
 * Saves all changes and returns to the previous screen.
 * @param action Pointer to an action.
 */
void NotesState::btnSaveClick(Action*)
{
	if (_edtNote->isFocused())
	{
		// edit still in progress
		return;
	}

	// overwrite everything, no way back :)
	auto& notes = _game->getSavedGame()->getUserNotes();
	notes.clear();
	if (_lstNotes->getTexts() > 1)
	{
		// ignore last row
		for (int i = 0; i < _lstNotes->getLastRowIndex(); ++i)
		{
			std::string note = _lstNotes->getCellText(i, 0);
			if (!note.empty())
			{
				notes.push_back(note);
			}
		}
	}

	_game->popState();
}

}
