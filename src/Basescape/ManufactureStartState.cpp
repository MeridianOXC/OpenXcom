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
#include "ManufactureStartState.h"
#include <sstream>
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Menu/ErrorMessageState.h"
#include "../Engine/Unicode.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleManufacture.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "ManufactureInfoState.h"
#include "../Savegame/SavedGame.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the productions start screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param item The RuleManufacture to produce.
 */
ManufactureStartState::ManufactureStartState(Base *base, RuleManufacture *item) :  _base(base), _item(item)
{
	_screen = false;

	_window = new Window(this, 320, 160, 0, 20);
	_btnCancel = new TextButton(136, 16, 16, 155);
	_txtTitle = new Text(320, 17, 0, 30);
	_txtManHour = new Text(290, 9, 16, 50);
	_txtCost = new Text(290, 9, 16, 60);
	_txtWorkSpace = new Text(290, 9, 16, 70);

	_txtRequiredItemsTitle = new Text(290, 9, 16, 84);
	_txtItemNameColumn = new Text(60, 16, 30, 92);
	_txtUnitRequiredColumn = new Text(60, 16, 155, 92);
	_txtUnitAvailableColumn = new Text(60, 16, 230, 92);
	_lstRequiredItems = new TextList(270, 40, 30, 108);

	_btnStart = new TextButton(136, 16, 168, 155);

	// Set palette
	setInterface("allocateManufacture");

	add(_window, "window", "allocateManufacture");
	add(_txtTitle, "text", "allocateManufacture");
	add(_txtManHour, "text", "allocateManufacture");
	add(_txtCost, "text", "allocateManufacture");
	add(_txtWorkSpace, "text", "allocateManufacture");
	add(_btnCancel, "button", "allocateManufacture");

	add(_txtRequiredItemsTitle, "text", "allocateManufacture");
	add(_txtItemNameColumn, "text", "allocateManufacture");
	add(_txtUnitRequiredColumn, "text", "allocateManufacture");
	add(_txtUnitAvailableColumn, "text", "allocateManufacture");
	add(_lstRequiredItems, "list", "allocateManufacture");

	add(_btnStart, "button", "allocateManufacture");

	centerAllSurfaces();

	setWindowBackground(_window, "allocateManufacture");

	_txtTitle->setText(tr(_item->getName()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtManHour->setText(tr("STR_ENGINEER_HOURS_TO_PRODUCE_ONE_UNIT").arg(_item->getManufactureTime()));

	_txtCost->setText(tr("STR_COST_PER_UNIT_").arg(Unicode::formatFunding(_item->getManufactureCost())));

	_txtWorkSpace->setText(tr("STR_WORK_SPACE_REQUIRED").arg(_item->getRequiredSpace()));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&ManufactureStartState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&ManufactureStartState::btnCancelClick, Options::keyCancel);

	bool productionPossible = _item->haveEnoughMoneyForOneMoreUnit(_game->getSavedGame()->getFunds());
	// check available workspace later
	//int availableWorkSpace = _base->getFreeWorkshops();
	//productionPossible &= (availableWorkSpace > 0);

	_txtRequiredItemsTitle->setText(tr("STR_SPECIAL_MATERIALS_REQUIRED"));
	_txtRequiredItemsTitle->setAlign(ALIGN_CENTER);

	_txtItemNameColumn->setText(tr("STR_ITEM_REQUIRED"));
	_txtItemNameColumn->setWordWrap(true);

	_txtUnitRequiredColumn->setText(tr("STR_UNITS_REQUIRED"));
	_txtUnitRequiredColumn->setWordWrap(true);

	_txtUnitAvailableColumn->setText(tr("STR_UNITS_AVAILABLE"));
	_txtUnitAvailableColumn->setWordWrap(true);

	_lstRequiredItems->setColumns(3, 140, 75, 55);
	_lstRequiredItems->setBackground(_window);

	bool hasRequirements = _item->getRequiredCrafts().size() > 0 || _item->getRequiredItems().size() > 0;
	int row = 0;
	for (auto& iter : _item->getRequiredCrafts())
	{
		auto count = base->getCraftCountForProduction(iter.first);

		std::ostringstream s1, s2;
		s1 << iter.second;
		s2 << count;
		productionPossible &= (count >= iter.second);
		_lstRequiredItems->addRow(3, tr(iter.first->getType()).c_str(), s1.str().c_str(), s2.str().c_str());
		_lstRequiredItems->setCellColor(row, 1, _lstRequiredItems->getSecondaryColor());
		_lstRequiredItems->setCellColor(row, 2, _lstRequiredItems->getSecondaryColor());
		row++;
	}
	for (auto& iter : _item->getRequiredItems())
	{
		auto count = base->getStorageItems()->getItem(iter.first);

		std::ostringstream s1, s2;
		s1 << iter.second;
		s2 << count;
		productionPossible &= (count >= iter.second);
		_lstRequiredItems->addRow(3, tr(iter.first->getType()).c_str(), s1.str().c_str(), s2.str().c_str());
		_lstRequiredItems->setCellColor(row, 1, _lstRequiredItems->getSecondaryColor());
		_lstRequiredItems->setCellColor(row, 2, _lstRequiredItems->getSecondaryColor());
		row++;
	}
	if (_item->getSpawnedPersonType() != "")
	{
		if (base->getAvailableQuarters() <= base->getUsedQuarters())
		{
			productionPossible = false;
		}

		// separator line
		_lstRequiredItems->addRow(1, tr("STR_PERSON_JOINING").c_str());
		_lstRequiredItems->setCellColor(row, 0, _lstRequiredItems->getSecondaryColor());
		row++;

		// person joining
		std::ostringstream s1;
		s1 << Unicode::TOK_COLOR_FLIP << 1;
		_lstRequiredItems->addRow(2, tr(_item->getSpawnedPersonName() != "" ? _item->getSpawnedPersonName() : _item->getSpawnedPersonType()).c_str(), s1.str().c_str());
		row++;
	}
	if (!_item->getRandomProducedItems().empty())
	{
		// separator line
		_lstRequiredItems->addRow(1, tr("STR_RANDOM_PRODUCTION_DISCLAIMER").c_str());
		_lstRequiredItems->setCellColor(row, 0, _lstRequiredItems->getSecondaryColor());
		row++;
	}
	bool hasVanillaOutput = false;
	if (_item->getProducedItems().size() == 1)
	{
		const RuleItem* match = _game->getMod()->getItem(_item->getName(), false);
		if (match)
		{
			auto iter = _item->getProducedItems().find(match);
			if (iter != _item->getProducedItems().end())
			{
				hasVanillaOutput = ((*iter).second == 1);
			}
		}
	}
	if (!hasVanillaOutput && !_item->getProducedItems().empty())
	{
		// separator line
		_lstRequiredItems->addRow(1, tr("STR_UNITS_PRODUCED").c_str());
		_lstRequiredItems->setCellColor(row, 0, _lstRequiredItems->getSecondaryColor());
		row++;

		// produced items
		for (auto& iter : _item->getProducedItems())
		{
			std::ostringstream s1;
			s1 << Unicode::TOK_COLOR_FLIP << iter.second;
			_lstRequiredItems->addRow(2, tr(iter.first->getType()).c_str(), s1.str().c_str());
			row++;
		}
	}

	_txtRequiredItemsTitle->setVisible(hasRequirements);
	_txtItemNameColumn->setVisible(hasRequirements);
	_txtUnitRequiredColumn->setVisible(hasRequirements);
	_txtUnitAvailableColumn->setVisible(hasRequirements);
	_lstRequiredItems->setVisible(row);

	_btnStart->setText(tr("STR_START_PRODUCTION"));
	_btnStart->onMouseClick((ActionHandler)&ManufactureStartState::btnStartClick);
	_btnStart->onKeyboardPress((ActionHandler)&ManufactureStartState::btnStartClick, Options::keyOk);
	_btnStart->setVisible(productionPossible);

	if (_item)
	{
		// mark new as normal
		if (_game->getSavedGame()->getManufactureRuleStatus(_item->getName()) == RuleManufacture::MANU_STATUS_NEW)
		{
			_game->getSavedGame()->setManufactureRuleStatus(_item->getName(), RuleManufacture::MANU_STATUS_NORMAL);
		}
	}
}

/**
 * Returns to previous screen.
 * @param action A pointer to an Action.
 */
void ManufactureStartState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Go to the Production settings screen.
 * @param action A pointer to an Action.
 */
void ManufactureStartState::btnStartClick(Action *)
{
	if (_item->getProducedCraft() && _base->getAvailableHangars() - _base->getUsedHangars() <= 0)
	{
		_game->pushState(new ErrorMessageState(tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"), _palette, _game->getMod()->getInterface("basescape")->getElement("errorMessage")->color, "BACK17.SCR", _game->getMod()->getInterface("basescape")->getElement("errorPalette")->color));
	}
	else if (_item->getRequiredSpace() > _base->getFreeWorkshops())
	{
		_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_WORK_SPACE"), _palette, _game->getMod()->getInterface("basescape")->getElement("errorMessage")->color, "BACK17.SCR", _game->getMod()->getInterface("basescape")->getElement("errorPalette")->color));
	}
	else
	{
		_game->pushState(new ManufactureInfoState(_base, _item));
	}
}

}
