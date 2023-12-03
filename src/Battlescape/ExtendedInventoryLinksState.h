#pragma once
/*
 * Copyright 2010-2021 OpenXcom Developers.
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

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class InventoryState;
class SavedBattleGame;

/**
 * A screen with links to the OXCE inventory functionality.
 */
class ExtendedInventoryLinksState : public State
{
private:
	TextButton *_btnOk;
	TextButton *_btnArmor, *_btnAvatar, *_btnEquipmentSave, *_btnEquipmentLoad, *_btnPersonalSave, *_btnPersonalLoad, *_btnNotes, *_btnUfopedia, *_btnAutoEquip;
	Window *_window;
	Text *_txtTitle;
	InventoryState *_parent;
	SavedBattleGame* _save;
public:
	/// Creates the ExtendedInventoryLinks state.
	ExtendedInventoryLinksState(InventoryState* parent, SavedBattleGame* save, bool inBase, bool beforeMission);
	/// Cleans up the ExtendedInventoryLinks state.
	~ExtendedInventoryLinksState() = default;
	/// Handlers for clicking the buttons.
	void btnArmorClick(Action* action);
	void btnAvatarClick(Action* action);
	void btnEquipmentSaveClick(Action* action);
	void btnEquipmentLoadClick(Action* action);
	void btnPersonalSaveClick(Action* action);
	void btnPersonalLoadClick(Action* action);
	void btnNotesClick(Action* action);
	void btnUfopediaClick(Action* action);
	void btnAutoEquipClick(Action* action);
	void btnGenerateBackstoryClick(Action* action);
	void btnOkClick(Action* action);
};

}
