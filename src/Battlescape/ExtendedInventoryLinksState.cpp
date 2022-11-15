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

#include "ExtendedInventoryLinksState.h"
#include "InventoryState.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Menu/NotesState.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the ExtendedInventoryLinksState screen.
 */
ExtendedInventoryLinksState::ExtendedInventoryLinksState(InventoryState* parent, SavedBattleGame* save, bool inBase, bool beforeMission) : _parent(parent), _save(save)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 256, inBase ? 180 : 180-14, 32, inBase ? 10 : 10+14, POPUP_BOTH);
	_txtTitle = new Text(220, 17, 50, inBase ? 33 : 33+23);
	if (Options::oxceFatFingerLinks)
	{
		_btnArmor = new TextButton(116, 25, 44, 50);
		_btnAvatar = new TextButton(116, 25, 161, 50);
		_btnEquipmentSave = new TextButton(116, 25, 44, 76);
		_btnEquipmentLoad = new TextButton(116, 25, 161, 76);
		_btnPersonalSave = new TextButton(116, 25, 44, 102);
		_btnPersonalLoad = new TextButton(116, 25, 161, 102);
		_btnNotes = new TextButton(116, 25, 44, 128);
		_btnUfopedia = new TextButton(116, 25, 161, 128);
		_btnAutoEquip = new TextButton(116, 25, 44, 154);
		_btnOk = new TextButton(116, 25, 161, 154);
	}
	else
	{
		_btnArmor = new TextButton(220, 12, 50, 50);
		_btnAvatar = new TextButton(220, 12, 50, 63);
		_btnEquipmentSave = new TextButton(220, 12, 50, 76);
		_btnEquipmentLoad = new TextButton(220, 12, 50, 89);
		_btnPersonalSave = new TextButton(220, 12, 50, 102);
		_btnPersonalLoad = new TextButton(220, 12, 50, 115);
		_btnNotes = new TextButton(220, 12, 50, 128);
		_btnUfopedia = new TextButton(220, 12, 50, 141);
		_btnAutoEquip = new TextButton(220, 12, 50, 154);
		_btnOk = new TextButton(220, 12, 50, 167);
	}

	// Set palette
	setInterface("oxceLinks", false, _save);

	add(_window, "window", "oxceLinks");
	add(_txtTitle, "text", "oxceLinks");
	add(_btnOk, "button", "oxceLinks");

	add(_btnArmor, "button", "oxceLinks");
	add(_btnAvatar, "button", "oxceLinks");
	add(_btnEquipmentSave, "button", "oxceLinks");
	add(_btnEquipmentLoad, "button", "oxceLinks");
	add(_btnPersonalSave, "button", "oxceLinks");
	add(_btnPersonalLoad, "button", "oxceLinks");
	add(_btnNotes, "button", "oxceLinks");
	add(_btnUfopedia, "button", "oxceLinks");
	add(_btnAutoEquip, "button", "oxceLinks");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "oxceLinks");

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_EXTENDED_LINKS"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ExtendedInventoryLinksState::btnOkClick, Options::keyCancel);

	_btnArmor->setText(tr("STR_INVENTORY_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnArmorClick);
	_btnArmor->setVisible(inBase);

	_btnAvatar->setText(tr("STR_INVENTORY_AVATAR"));
	_btnAvatar->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnAvatarClick);
	_btnAvatar->setVisible(inBase);

	_btnEquipmentSave->setText(tr("STR_SAVE_EQUIPMENT_TEMPLATE"));
	_btnEquipmentSave->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnEquipmentSaveClick);
	_btnEquipmentSave->setVisible(beforeMission);

	_btnEquipmentLoad->setText(tr("STR_LOAD_EQUIPMENT_TEMPLATE"));
	_btnEquipmentLoad->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnEquipmentLoadClick);
	_btnEquipmentLoad->setVisible(beforeMission);

	_btnPersonalSave->setText(tr("STR_SAVE_PERSONAL_EQUIPMENT"));
	_btnPersonalSave->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnPersonalSaveClick);
	_btnPersonalSave->setVisible(beforeMission);

	_btnPersonalLoad->setText(tr("STR_LOAD_PERSONAL_EQUIPMENT"));
	_btnPersonalLoad->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnPersonalLoadClick);
	_btnPersonalLoad->setVisible(beforeMission);

	_btnNotes->setText(tr("STR_NOTES"));
	_btnNotes->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnNotesClick);

	_btnUfopedia->setText(tr("STR_UFOPAEDIA"));
	_btnUfopedia->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnUfopediaClick);

	_btnAutoEquip->setText(tr("STR_AUTO_EQUIP"));
	_btnAutoEquip->onMouseClick((ActionHandler)&ExtendedInventoryLinksState::btnAutoEquipClick);
	_btnAutoEquip->setVisible(beforeMission);

	applyBattlescapeTheme("oxceLinks");
}

void ExtendedInventoryLinksState::btnArmorClick(Action *)
{
	_game->popState();
	_parent->btnArmorClick(nullptr);
}

void ExtendedInventoryLinksState::btnAvatarClick(Action *)
{
	_game->popState();
	_parent->btnArmorClickRight(nullptr);
}

void ExtendedInventoryLinksState::btnEquipmentSaveClick(Action *)
{
	_game->popState();
	_parent->btnInventorySaveClick(nullptr);
}

void ExtendedInventoryLinksState::btnEquipmentLoadClick(Action *)
{
	_game->popState();
	_parent->btnInventoryLoadClick(nullptr);
}

void ExtendedInventoryLinksState::btnPersonalSaveClick(Action *)
{
	_game->popState();
	_parent->btnCreatePersonalTemplateClick(nullptr);
}

void ExtendedInventoryLinksState::btnPersonalLoadClick(Action *)
{
	_game->popState();
	_parent->btnApplyPersonalTemplateClick(nullptr);
}

void ExtendedInventoryLinksState::btnNotesClick(Action *)
{
	_game->popState();
	_game->pushState(new NotesState(OPT_BATTLESCAPE));
}

void ExtendedInventoryLinksState::btnUfopediaClick(Action *)
{
	_game->popState();
	_parent->btnUfopaediaClick(nullptr);
}

void ExtendedInventoryLinksState::btnAutoEquipClick(Action *)
{
	_game->popState();
	_parent->onAutoequip(nullptr);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ExtendedInventoryLinksState::btnOkClick(Action *)
{
	_game->popState();
}

}
