/*
 * Copyright 2010-2025 OpenXcom Developers.
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
#include "ItemLocationsState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/RuleItem.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"

namespace OpenXcom
{

/**
 * Initializes all the elements on the UI.
 */
ItemLocationsState::ItemLocationsState(const RuleItem* selectedItem)
{
	_screen = false;

	_window = new Window(this, 222, 144, 49, 32);
	_txtTitle = new Text(200, 9, 59, 42);
	_txtBase = new Text(98, 9, 59, 56);
	_txtQuantity = new Text(98, 9, 159, 56);
	_lstLocations = new TextList(186, 64, 57, 70);
	_btnOk = new TextButton(100, 16, 110, 150);

	// Set palette
	setInterface("itemLocations");

	add(_window, "window", "itemLocations");
	add(_txtTitle, "text", "itemLocations");
	add(_txtBase, "text", "itemLocations");
	add(_txtQuantity, "text", "itemLocations");
	add(_btnOk, "button", "itemLocations");
	add(_lstLocations, "list", "itemLocations");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "itemLocations");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TOPIC").arg(tr(selectedItem->getType())));

	_txtBase->setText(tr("STR_BASE"));
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ItemLocationsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ItemLocationsState::btnOkClick, Options::keyCancel);

	_lstLocations->setColumns(3, 100, 40, 30);
	_lstLocations->setBackground(_window);
	_lstLocations->setMargin(2);
	_lstLocations->setSelectable(true);

	for (auto* xbase : *_game->getSavedGame()->getBases())
	{
		std::ostringstream ssInBase, ssInTransfer;
		ssInBase << xbase->getStorageItems()->getItem(selectedItem);
		int total = 0;
		for (auto* xtransfer : *xbase->getTransfers())
		{
			if (xtransfer->getType() == TRANSFER_ITEM && xtransfer->getItems() == selectedItem)
			{
				total += xtransfer->getQuantity();
			}
		}
		ssInTransfer << "+" << total;
		_lstLocations->addRow(3, xbase->getName().c_str(), ssInBase.str().c_str(), ssInTransfer.str().c_str());
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ItemLocationsState::btnOkClick(Action *)
{
	_game->popState();
}

}
