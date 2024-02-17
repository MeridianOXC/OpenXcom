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
#include "InventoryPersonalState.h"
#include <algorithm>
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the InventoryPersonalState.
 */
InventoryPersonalState::InventoryPersonalState(Soldier* soldier)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 196, 160, 64, 20, POPUP_BOTH);
	_txtTitle = new Text(186, 16, 69, 30);
	_lstLayout = new TextList(164, 104, 72, 44);
	_btnCancel = new TextButton(144, 16, 90, 156);

	setStandardPalette("PAL_BATTLESCAPE");

	add(_window, "messageWindowBorder", "battlescape");
	add(_txtTitle, "messageWindows", "battlescape");
	add(_lstLayout, "optionLists", "battlescape");
	add(_btnCancel, "messageWindowButtons", "battlescape");

	centerAllSurfaces();

	// Set up objects
	_window->setHighContrast(true);
	_window->setBackground(_game->getMod()->getSurface("TAC00.SCR"));

	_txtTitle->setHighContrast(true);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PERSONAL_EQUIPMENT"));

	_lstLayout->setHighContrast(true);
	_lstLayout->setColumns(2, 136, 12);
	_lstLayout->setSelectable(true);
	_lstLayout->setBackground(_window);
	_lstLayout->setMargin(8);

	_btnCancel->setHighContrast(true);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&InventoryPersonalState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&InventoryPersonalState::btnCancelClick, Options::keyCancel);

	// 1. tally items
	std::map<const RuleItem*, int> summary;
	for (const auto* layoutItem : *soldier->getPersonalEquipmentLayout())
	{
		// item
		summary[layoutItem->getItemType()] += 1;

		// ammo
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			const auto* loadedAmmoType = layoutItem->getAmmoItemForSlot(slot);
			if (loadedAmmoType != nullptr)
			{
				summary[loadedAmmoType] += 1;
			}
		}
	}

	// 2. sort items
	std::vector<const RuleItem*> sorted;
	sorted.reserve(summary.size());
	for (auto& info : summary)
	{
		sorted.push_back(info.first);
	}
	std::sort(sorted.begin(), sorted.end(), [](const RuleItem* a, const RuleItem* b)
		{
			return a->getListOrder() < b->getListOrder();
		}
	);

	// display armor
	if (Options::oxcePersonalLayoutIncludingArmor)
	{
		std::ostringstream ss1, ss2;
		auto* armor = soldier->getPersonalEquipmentArmor();
		if (armor)
		{
			ss1 << tr(armor->getType());
		}
		_lstLayout->addRow(2, ss1.str().c_str(), ss2.str().c_str());
	}

	// 3. display items
	for (auto* ruleItem : sorted)
	{
		std::ostringstream ss1, ss2;
		if (ruleItem->getBattleType() == BT_AMMO)
		{
			ss1 << "  ";
		}
		ss1 << tr(ruleItem->getType());
		ss2 << summary[ruleItem];
		_lstLayout->addRow(2, ss1.str().c_str(), ss2.str().c_str());
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InventoryPersonalState::btnCancelClick(Action*)
{
	_game->popState();
}

}
