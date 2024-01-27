#pragma once
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
#include "../Engine/State.h"
#include "OptionsBaseState.h"
#include <string>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextEdit;
class TextList;
class ToggleTextButton;

/**
 * Allows the player to take notes.
 */
class NotesState : public State
{
protected:
	Window* _window;
	Text* _txtTitle;
	Text* _txtDelete;
	TextList* _lstNotes;
	TextEdit* _edtNote;
	TextButton* _btnSave;
	TextButton* _btnCancel;
	ToggleTextButton* _btnDelete;

	OptionsOrigin _origin;
	std::string _selectedNote;
	int _previousSelectedRow, _selectedRow;

	/// Updates the Notes list.
	void updateList();
public:
	/// Creates the Notes state.
	NotesState(OptionsOrigin origin);
	/// Cleans up the Notes state.
	~NotesState();
	/// Refreshes the Notes state.
	void init() override;
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action* action);
	/// Handler for clicking the Notes list.
	void lstNotesPress(Action* action);
	/// Handler for pressing a key on the Note edit.
	void edtNoteKeyPress(Action* action);
	/// Handler for clicking on the Save button.
	void btnSaveClick(Action* action);
};

}
