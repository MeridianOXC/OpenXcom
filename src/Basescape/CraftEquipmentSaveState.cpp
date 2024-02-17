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
#include "CraftEquipmentSaveState.h"
#include "CraftEquipmentState.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Engine/Language.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
* Initializes all the elements in the Save Craft Loadout window.
*/
CraftEquipmentSaveState::CraftEquipmentSaveState(CraftEquipmentState *parent) : _parent(parent), _previousSelectedRow(-1), _selectedRow(-1)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 240, 136, 40, 36 + 1, POPUP_BOTH);
	_txtTitle = new Text(230, 16, 45, 44 + 3);
	_lstLoadout = new TextList(208, 80, 48, 60);
	_btnCancel = new TextButton(80, 16, 165, 148);
	_btnSave = new TextButton(80, 16, 75, 148);
	_edtSave = new TextEdit(this, 188, 9, 0, 0);

	// Set palette
	setInterface("craftEquipmentSave");

	add(_window, "window", "craftEquipmentSave");
	add(_txtTitle, "text", "craftEquipmentSave");
	add(_lstLoadout, "list", "craftEquipmentSave");
	add(_btnCancel, "button", "craftEquipmentSave");
	add(_btnSave, "button", "craftEquipmentSave");
	add(_edtSave);

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftEquipmentSave");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SAVE_CRAFT_LOADOUT_TEMPLATE"));

	_lstLoadout->setColumns(1, 192);
	_lstLoadout->setSelectable(true);
	_lstLoadout->setBackground(_window);
	_lstLoadout->setMargin(8);
	_lstLoadout->onMousePress((ActionHandler)&CraftEquipmentSaveState::lstLoadoutPress);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&CraftEquipmentSaveState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&CraftEquipmentSaveState::btnCancelClick, Options::keyCancel);

	_btnSave->setText(tr("STR_SAVE_UC"));
	_btnSave->onMouseClick((ActionHandler)&CraftEquipmentSaveState::btnSaveClick);

	_edtSave->setColor(_lstLoadout->getSecondaryColor());
	_edtSave->setVisible(false);
	_edtSave->onKeyboardPress((ActionHandler)&CraftEquipmentSaveState::edtSaveKeyPress);

	for (int i = 0; i < SavedGame::MAX_CRAFT_LOADOUT_TEMPLATES; ++i)
	{
		ItemContainer *item = _game->getSavedGame()->getGlobalCraftLoadout(i);
		if (item->empty())
		{
			_lstLoadout->addRow(1, tr("STR_EMPTY_SLOT_N").arg(i + 1).c_str());
		}
		else
		{
			const std::string &itemName = _game->getSavedGame()->getGlobalCraftLoadoutName(i);
			if (itemName.empty())
			{
				_lstLoadout->addRow(1, tr("STR_UNNAMED_SLOT_N").arg(i + 1).c_str());
			}
			else
			{
				_lstLoadout->addRow(1, itemName.c_str());
			}
		}
	}
}

/**
*
*/
CraftEquipmentSaveState::~CraftEquipmentSaveState()
{

}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void CraftEquipmentSaveState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
* Saves the selected template.
* @param action Pointer to an action.
*/
void CraftEquipmentSaveState::btnSaveClick(Action *)
{
	if (_selectedRow != -1)
	{
		saveTemplate();
	}
}

/**
* Names the selected template.
* @param action Pointer to an action.
*/
void CraftEquipmentSaveState::lstLoadoutPress(Action *action)
{
	_previousSelectedRow = _selectedRow;
	_selectedRow = _lstLoadout->getSelectedRow();
	if (_previousSelectedRow > -1)
	{
		_lstLoadout->setCellText(_previousSelectedRow, 0, _selected);
	}
	_selected = _lstLoadout->getCellText(_selectedRow, 0);

	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT && _edtSave->isFocused())
	{
		_previousSelectedRow = -1;
		_selectedRow = -1;

		_edtSave->setText("");
		_edtSave->setVisible(false);
		_edtSave->setFocus(false, false);
		_lstLoadout->setScrolling(true);
	}
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_lstLoadout->setCellText(_selectedRow, 0, "");

		_edtSave->setText(_selected);
		_edtSave->setX(_lstLoadout->getColumnX(0));
		_edtSave->setY(_lstLoadout->getRowY(_selectedRow));
		_edtSave->setVisible(true);
		_edtSave->setFocus(true, false);
		_lstLoadout->setScrolling(false);
	}
}


/**
* Saves the selected template.
* @param action Pointer to an action.
*/
void CraftEquipmentSaveState::edtSaveKeyPress(Action *action)
{
	if (action->getDetails()->key.keysym.sym == SDLK_RETURN ||
		action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		saveTemplate();
	}
}

/**
* Saves the selected template.
*/
void CraftEquipmentSaveState::saveTemplate()
{
	if (_selectedRow >= 0 && _selectedRow < SavedGame::MAX_CRAFT_LOADOUT_TEMPLATES)
	{
		_game->getSavedGame()->setGlobalCraftLoadoutName(_selectedRow, _edtSave->getText());
		_parent->saveGlobalLoadout(_selectedRow);

		_game->popState();
	}
}

}
