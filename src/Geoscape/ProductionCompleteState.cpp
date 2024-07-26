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
#include <assert.h>
#include "ProductionCompleteState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleManufacture.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "GeoscapeState.h"
#include "../Engine/Options.h"
#include "../Basescape/BasescapeState.h"
#include "../Basescape/ManufactureState.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in a Production Complete window.
 * @param game Pointer to the core game.
 * @param base Pointer to base the production belongs to.
 * @param item Item that finished producing.
 * @param state Pointer to the Geoscape state.
 * @param endType What ended the production.
 * @param production Pointer to the production details.
 */
ProductionCompleteState::ProductionCompleteState(Base *base, const std::string &item, GeoscapeState *state, productionProgress_e endType, Production *production) : _base(base), _state(state), _endType(endType)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_btnOk = new TextButton(118, 18, 40, 154);
	_btnGotoBase = new TextButton(118, 18, 162, 154);
	_btnSummary = new TextButton(118, 18, 162, 154);
	_txtMessage = new Text(246, 110, 37, 35);
	_txtItem = new Text(160, 9, 47, 35);
	_txtQuantity = new Text(70, 9, 209, 35);
	_lstSummary = new TextList(224, 96, 39, 45);

	// Set palette
	setInterface("geoManufactureComplete");

	add(_window, "window", "geoManufactureComplete");
	add(_btnOk, "button", "geoManufactureComplete");
	add(_btnGotoBase, "button", "geoManufactureComplete");
	add(_btnSummary, "button", "geoManufactureComplete");
	add(_txtMessage, "text1", "geoManufactureComplete");
	add(_txtItem, "text1", "geoManufactureComplete");
	add(_txtQuantity, "text1", "geoManufactureComplete");
	add(_lstSummary, "list", "geoManufactureComplete");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "geoManufactureComplete");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ProductionCompleteState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ProductionCompleteState::btnOkClick, Options::keyCancel);

	if (_endType != PROGRESS_CONSTRUCTION)
	{
		_btnGotoBase->setText(tr("STR_ALLOCATE_MANUFACTURE"));
	}
	else
	{
		_btnGotoBase->setText(tr("STR_GO_TO_BASE"));
	}
	_btnGotoBase->onMouseClick((ActionHandler)&ProductionCompleteState::btnGotoBaseClick);

	_btnSummary->setText(tr("STR_RANDOM_PRODUCTION_SUMMARY"));
	_btnSummary->onMouseClick((ActionHandler)&ProductionCompleteState::btnSummaryClick);

	_txtItem->setText(tr("STR_ITEM"));
	_txtItem->setVisible(false);

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtQuantity->setVisible(false);

	_lstSummary->setColumns(2, 162, 46);
	_lstSummary->setBackground(_window);
	_lstSummary->setSelectable(true);
	_lstSummary->setMargin(8);
	_lstSummary->setVisible(false);
	_lstSummary->onMouseClick((ActionHandler)&ProductionCompleteState::lstSummaryClick, SDL_BUTTON_RIGHT);

	if (production && !production->getRules()->getRandomProducedItems().empty())
	{
		_btnGotoBase->setVisible(false);
		_randomProductionInfo = production->getRandomProductionInfo(); // local copy is necessary
		_index.reserve(_randomProductionInfo.size());
		for (auto& each : production->getRandomProductionInfo())
		{
			std::ostringstream ss;
			ss << each.second;
			_lstSummary->addRow(2, tr(each.first).c_str(), ss.str().c_str());
			_index.push_back(each.first);
		}
	}
	else
	{
		_btnSummary->setVisible(false);
	}

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap(true);
	std::string s;
	switch(_endType)
	{
	case PROGRESS_CONSTRUCTION:
		s = tr("STR_CONSTRUCTION_OF_FACILITY_AT_BASE_IS_COMPLETE").arg(item).arg(base->getName());
		break;
	case PROGRESS_COMPLETE:
		s = tr("STR_PRODUCTION_OF_ITEM_AT_BASE_IS_COMPLETE").arg(item).arg(base->getName());
		break;
	case PROGRESS_NOT_ENOUGH_MONEY:
		s = tr("STR_NOT_ENOUGH_MONEY_TO_PRODUCE_ITEM_AT_BASE").arg(item).arg(base->getName());
		break;
	case PROGRESS_NOT_ENOUGH_MATERIALS:
		s = tr("STR_NOT_ENOUGH_SPECIAL_MATERIALS_TO_PRODUCE_ITEM_AT_BASE").arg(item).arg(base->getName());
		break;
	case PROGRESS_NOT_ENOUGH_LIVING_SPACE:
		s = tr("STR_NOT_ENOUGH_LIVING_SPACE_AT_BASE").arg(item).arg(base->getName());
		break;
	default:
		assert(false);
	}
	_txtMessage->setText(s);
}

/**
 *
 */
ProductionCompleteState::~ProductionCompleteState()
{

}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void ProductionCompleteState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Goes to the base for the respective production.
 * @param action Pointer to an action.
 */
void ProductionCompleteState::btnGotoBaseClick(Action *)
{
	_state->timerReset();
	_game->popState();
	if (_endType != PROGRESS_CONSTRUCTION)
	{
		_game->pushState(new ManufactureState(_base));
	}
	else
	{
		_game->pushState(new BasescapeState(_base, _state->getGlobe()));
	}
}

/**
 * Displays the random production info.
 * @param action Pointer to an action.
 */
void ProductionCompleteState::btnSummaryClick(Action *)
{
	_btnSummary->setVisible(false);
	_txtMessage->setVisible(false);

	_btnGotoBase->setVisible(true);
	_txtItem->setVisible(true);
	_txtQuantity->setVisible(true);
	_lstSummary->setVisible(true);
}

/**
 * Sells the selected item(s)... unless they were sold, transferred or otherwise disposed of earlier already.
 * @param action Pointer to an action.
 */
void ProductionCompleteState::lstSummaryClick(Action *)
{
	// 1. remember stuff
	auto scrollPos = _lstSummary->getScroll();
	unsigned int sel = _lstSummary->getSelectedRow();
	std::string itemName = _index[sel];
	int itemCount = _randomProductionInfo[itemName];

	// 2. deal with it
	auto* itemRule = _game->getMod()->getItem(itemName, false);
	if (itemRule)
	{
		// check if we sold something in the meantime
		if (_base->getStorageItems()->getItem(itemRule) < itemCount)
		{
			itemCount = _base->getStorageItems()->getItem(itemRule);
			_randomProductionInfo[itemName] -= itemCount; // just decrease amount by the maximum we can sell
		}
		else
		{
			// remove completely
			_index.erase(_index.begin() + sel);
		}

		if (itemCount > 0)
		{
			int64_t adjustedSellValue = itemRule->getSellCostAdjusted(_base, _game->getSavedGame());
			adjustedSellValue *= itemCount;
			_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + adjustedSellValue);
			_base->getStorageItems()->removeItem(itemRule, itemCount);
		}
	}

	// 3. refresh UI
	_lstSummary->clearList();
	for (const auto& name : _index)
	{
		std::ostringstream ss;
		ss << _randomProductionInfo[name];
		_lstSummary->addRow(2, tr(name).c_str(), ss.str().c_str());
	}
	_lstSummary->scrollTo(scrollPos);
}

}
