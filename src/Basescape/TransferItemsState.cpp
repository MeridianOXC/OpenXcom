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
#include "TransferItemsState.h"
#include "ManufactureDependenciesTreeState.h"
#include <sstream>
#include <climits>
#include <algorithm>
#include <locale>
#include "../Engine/CrossPlatform.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleItem.h"
#include "../Engine/Timer.h"
#include "../Menu/ErrorMessageState.h"
#include "TransferConfirmState.h"
#include "../Engine/Options.h"
#include "../fmath.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/Armor.h"
#include "../Interface/ComboBox.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Battlescape/DebriefingState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Transfer screen.
 * @param game Pointer to the core game.
 * @param baseFrom Pointer to the source base.
 * @param baseTo Pointer to the destination base.
 */
TransferItemsState::TransferItemsState(Base *baseFrom, Base *baseTo, DebriefingState *debriefingState) :
	_baseFrom(baseFrom), _baseTo(baseTo), _debriefingState(debriefingState),
	_sel(0), _total(0), _pQty(0), _cQty(0), _aQty(0), _iQty(0.0), _distance(0.0), _ammoColor(0),
	_previousSort(TransferSortDirection::BY_LIST_ORDER), _currentSort(TransferSortDirection::BY_LIST_ORDER), _errorShown(false)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 13);
	_btnOk = new TextButton(148, 16, 8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtQuantity = new Text(50, 9, 150, 24);
	_txtAmountTransfer = new Text(60, 17, 200, 24);
	_txtAmountDestination = new Text(60, 17, 260, 24);
	_cbxCategory = new ComboBox(this, 120, 16, 10, 24);
	_lstItems = new TextList(287, 128, 8, 44);

	// Set palette
	setInterface("transferMenu");

	_ammoColor = _game->getMod()->getInterface("transferMenu")->getElement("ammoColor")->color;

	add(_window, "window", "transferMenu");
	add(_btnQuickSearch, "button", "transferMenu");
	add(_btnOk, "button", "transferMenu");
	add(_btnCancel, "button", "transferMenu");
	add(_txtTitle, "text", "transferMenu");
	add(_txtQuantity, "text", "transferMenu");
	add(_txtAmountTransfer, "text", "transferMenu");
	add(_txtAmountDestination, "text", "transferMenu");
	add(_lstItems, "list", "transferMenu");
	add(_cbxCategory, "text", "transferMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "transferMenu");

	_btnOk->setText(tr("STR_TRANSFER"));
	_btnOk->onMouseClick((ActionHandler)&TransferItemsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&TransferItemsState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&TransferItemsState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&TransferItemsState::btnCancelClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFER"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtAmountTransfer->setText(tr("STR_AMOUNT_TO_TRANSFER"));
	_txtAmountTransfer->setWordWrap(true);

	_txtAmountDestination->setText(tr("STR_AMOUNT_AT_DESTINATION"));
	_txtAmountDestination->setWordWrap(true);

	_lstItems->setArrowColumn(193, ARROW_VERTICAL);
	_lstItems->setColumns(4, 162, 58, 40, 27);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(2);
	_lstItems->onLeftArrowPress((ActionHandler)&TransferItemsState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)&TransferItemsState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)&TransferItemsState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)&TransferItemsState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)&TransferItemsState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)&TransferItemsState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)&TransferItemsState::lstItemsMousePress);

	_distance = getDistance();

	_cats.push_back("STR_ALL_ITEMS");
	_cats.push_back("STR_ITEMS_AT_DESTINATION");

	for (std::vector<Soldier*>::iterator i = _baseFrom->getSoldiers()->begin(); i != _baseFrom->getSoldiers()->end(); ++i)
	{
		if (_debriefingState) break;
		if ((*i)->getCraft() == 0)
		{
			TransferRow row = { TRANSFER_SOLDIER, (*i), (*i)->getName(true), (int)(5 * _distance), 1, 0, 0, -4, 0, 0, (int)(5 * _distance) };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	for (std::vector<Craft*>::iterator i = _baseFrom->getCrafts()->begin(); i != _baseFrom->getCrafts()->end(); ++i)
	{
		if (_debriefingState) break;
		if ((*i)->getStatus() != "STR_OUT" || (Options::canTransferCraftsWhileAirborne && (*i)->getFuel() >= (*i)->getFuelLimit(_baseTo)))
		{
			TransferRow row = { TRANSFER_CRAFT, (*i), (*i)->getName(_game->getLanguage()),  (int)(25 * _distance), 1, 0, 0, -3, 0, 0, (int)(25 * _distance) };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	if (_baseFrom->getAvailableScientists() > 0 && _debriefingState == 0)
	{
		TransferRow row = { TRANSFER_SCIENTIST, 0, tr("STR_SCIENTIST"),  (int)(5 * _distance), _baseFrom->getAvailableScientists(), _baseTo->getAvailableScientists(), 0, -2, 0, 0, _baseFrom->getAvailableScientists() * (int)(5 * _distance) };
		_items.push_back(row);
		std::string cat = getCategory(_items.size() - 1);
		if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
		{
			_cats.push_back(cat);
		}
	}
	if (_baseFrom->getAvailableEngineers() > 0 && _debriefingState == 0)
	{
		TransferRow row = { TRANSFER_ENGINEER, 0, tr("STR_ENGINEER"),  (int)(5 * _distance), _baseFrom->getAvailableEngineers(), _baseTo->getAvailableEngineers(), 0, -1, 0, 0, _baseFrom->getAvailableEngineers() * (int)(5 * _distance) };
		_items.push_back(row);
		std::string cat = getCategory(_items.size() - 1);
		if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
		{
			_cats.push_back(cat);
		}
	}
	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		int qty = _baseFrom->getStorageItems()->getItem(*i);
		RuleItem *rule = _game->getMod()->getItem(*i, true);
		if (_debriefingState != 0)
		{
			qty = _debriefingState->getRecoveredItemCount(rule);
		}
		if (qty > 0)
		{
			TransferRow row = { TRANSFER_ITEM, rule, tr(*i),  (int)(1 * _distance), qty, _baseTo->getStorageItems()->getItem(*i), 0, rule->getListOrder(), rule->getSize(), qty * rule->getSize(), qty * (int)(1 * _distance) };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	_vanillaCategories = _cats.size();
	if (_game->getMod()->getDisplayCustomCategories() > 0)
	{
		bool hasUnassigned = false;

		// first find all relevant item categories
		std::vector<std::string> tempCats;
		for (std::vector<TransferRow>::iterator i = _items.begin(); i != _items.end(); ++i)
		{
			if ((*i).type == TRANSFER_ITEM)
			{
				RuleItem *rule = (RuleItem*)((*i).rule);
				if (rule->getCategories().empty())
				{
					hasUnassigned = true;
				}
				for (std::vector<std::string>::const_iterator j = rule->getCategories().begin(); j != rule->getCategories().end(); ++j)
				{
					if (std::find(tempCats.begin(), tempCats.end(), (*j)) == tempCats.end())
					{
						tempCats.push_back((*j));
					}
				}
			}
		}
		// then use them nicely in order
		if (_game->getMod()->getDisplayCustomCategories() == 1)
		{
			_cats.clear();
			_cats.push_back("STR_ALL_ITEMS");
			_cats.push_back("STR_ITEMS_AT_DESTINATION");
			_vanillaCategories = _cats.size();
		}
		const std::vector<std::string> &categories = _game->getMod()->getItemCategoriesList();
		for (std::vector<std::string>::const_iterator k = categories.begin(); k != categories.end(); ++k)
		{
			if (std::find(tempCats.begin(), tempCats.end(), (*k)) != tempCats.end())
			{
				_cats.push_back((*k));
			}
		}
		if (hasUnassigned)
		{
			_cats.push_back("STR_UNASSIGNED");
		}
	}

	_cbxCategory->setOptions(_cats, true);
	_cbxCategory->onChange((ActionHandler)&TransferItemsState::cbxCategoryChange);
	_cbxCategory->onKeyboardPress((ActionHandler)&TransferItemsState::btnTransferAllClick, Options::keyTransferAll);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&TransferItemsState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&TransferItemsState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	updateList();

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&TransferItemsState::increase);
	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&TransferItemsState::decrease);
}

/**
 *
 */
TransferItemsState::~TransferItemsState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void TransferItemsState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Determines the category a row item belongs in.
 * @param sel Selected row.
 * @returns Item category.
 */
std::string TransferItemsState::getCategory(int sel) const
{
	RuleItem *rule = 0;
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		return "STR_PERSONNEL";
	case TRANSFER_CRAFT:
		return "STR_CRAFT_ARMAMENT";
	case TRANSFER_ITEM:
		rule = (RuleItem*)_items[sel].rule;
		if (rule->getBattleType() == BT_CORPSE || rule->isAlien())
		{
			if (rule->getVehicleUnit())
				return "STR_PERSONNEL"; // OXCE: critters fighting for us
			if (rule->isAlien())
				return "STR_PRISONERS"; // OXCE: live aliens
			return "STR_ALIENS";
		}
		if (rule->getBattleType() == BT_NONE)
		{
			if (_game->getMod()->isCraftWeaponStorageItem(rule))
				return "STR_CRAFT_ARMAMENT";
			if (_game->getMod()->isArmorStorageItem(rule))
				return "STR_ARMORS"; // OXCE: armors
			return "STR_COMPONENTS";
		}
		return "STR_EQUIPMENT";
	}
	return "STR_ALL_ITEMS";
}

/**
 * Determines if a row item belongs to a given category.
 * @param sel Selected row.
 * @param cat Category.
 * @returns True if row item belongs to given category, otherwise False.
 */
bool TransferItemsState::belongsToCategory(int sel, const std::string &cat) const
{
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
	case TRANSFER_CRAFT:
		return false;
	case TRANSFER_ITEM:
		RuleItem *rule = (RuleItem*)_items[sel].rule;
		return rule->belongsToCategory(cat);
	}
	return false;
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void TransferItemsState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText("");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
* Quick search.
* @param action Pointer to an action.
*/
void TransferItemsState::btnQuickSearchApply(Action *)
{
	updateList();
}

/**
* Filters the current list of items.
*/
void TransferItemsState::updateList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_lstItems->clearList();
	_rows.clear();

	size_t selCategory = _cbxCategory->getSelected();
	const std::string cat = _cats[selCategory];
	bool allItems = (cat == "STR_ALL_ITEMS");
	bool onlyItemsAtDestination = (cat == "STR_ITEMS_AT_DESTINATION");
	bool categoryUnassigned = (cat == "STR_UNASSIGNED");
	bool specialCategory = allItems || onlyItemsAtDestination;

	if (_previousSort != _currentSort)
	{
		switch (_currentSort)
		{
		case TransferSortDirection::BY_TOTAL_COST: std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.totalCost > b.totalCost; }); break;
		case TransferSortDirection::BY_UNIT_COST:  std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.cost > b.cost; }); break;
		case TransferSortDirection::BY_TOTAL_SIZE: std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.totalSize > b.totalSize; }); break;
		case TransferSortDirection::BY_UNIT_SIZE:  std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.size > b.size; }); break;
		default:                                   std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.listOrder < b.listOrder; }); break;
		}
	}

	for (size_t i = 0; i < _items.size(); ++i)
	{
		// filter
		if (selCategory >= _vanillaCategories)
		{
			if (categoryUnassigned && _items[i].type == TRANSFER_ITEM)
			{
				RuleItem* rule = (RuleItem*)_items[i].rule;
				if (!rule->getCategories().empty())
				{
					continue;
				}
			}
			else if (!specialCategory && !belongsToCategory(i, cat))
			{
				continue;
			}
		}
		else
		{
			if (!specialCategory && cat != getCategory(i))
			{
				continue;
			}
		}

		// "items at destination" filter
		if (onlyItemsAtDestination && _items[i].qtyDst <= 0)
		{
			continue;
		}

		// quick search
		if (!searchString.empty())
		{
			std::string projectName = _items[i].name;
			Unicode::upperCase(projectName);
			if (projectName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		std::string name = _items[i].name;
		bool ammo = false;
		if (_items[i].type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)_items[i].rule;
			ammo = (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0));
			if (ammo)
			{
				name.insert(0, "  ");
			}
		}
		std::ostringstream ssQtySrc, ssQtyDst, ssAmount;
		ssQtySrc << _items[i].qtySrc - _items[i].amount;
		ssQtyDst << _items[i].qtyDst;
		ssAmount << _items[i].amount;
		_lstItems->addRow(4, name.c_str(), ssQtySrc.str().c_str(), ssAmount.str().c_str(), ssQtyDst.str().c_str());
		_rows.push_back(i);
		if (_items[i].amount > 0)
		{
			_lstItems->setRowColor(_rows.size() - 1, _lstItems->getSecondaryColor());
		}
		else if (ammo)
		{
			_lstItems->setRowColor(_rows.size() - 1, _ammoColor);
		}
	}
}

/**
 * Transfers the selected items.
 * @param action Pointer to an action.
 */
void TransferItemsState::btnOkClick(Action *)
{
	if (Options::storageLimitsEnforced && !AreSame(_iQty, 0.0))
	{
		// check again (because of items with negative size)
		// But only check the base whose available space is decreasing.
		double freeStoresTo = _baseTo->getAvailableStores() - _baseTo->getUsedStores() - _iQty;
		double freeStoresFrom = _baseFrom->getAvailableStores() - _baseFrom->getUsedStores() + _iQty;
		if (_iQty > 0.0 ? freeStoresTo < -0.00001 : freeStoresFrom < -0.00001)
		{
			RuleInterface *menuInterface = _game->getMod()->getInterface("transferMenu");
			_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_STORE_SPACE"), _palette, menuInterface->getElement("errorMessage")->color, "BACK13.SCR", menuInterface->getElement("errorPalette")->color));
			return;
		}
	}

	_game->pushState(new TransferConfirmState(_baseTo, this));
}

/**
 * Completes the transfer between bases.
 */
void TransferItemsState::completeTransfer()
{
	int time = (int)floor(6 + _distance / 10.0);
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _total);
	for (std::vector<TransferRow>::const_iterator i = _items.begin(); i != _items.end(); ++i)
	{
		if (i->amount > 0)
		{
			Transfer *t = 0;
			Craft *craft = 0;
			switch (i->type)
			{
			case TRANSFER_SOLDIER:
				for (std::vector<Soldier*>::iterator s = _baseFrom->getSoldiers()->begin(); s != _baseFrom->getSoldiers()->end(); ++s)
				{
					if (*s == i->rule)
					{
						(*s)->setPsiTraining(false);
						(*s)->setTraining(false);
						t = new Transfer(time);
						t->setSoldier(*s);
						_baseTo->getTransfers()->push_back(t);
						_baseFrom->getSoldiers()->erase(s);
						break;
					}
				}
				break;
			case TRANSFER_CRAFT:
				craft = (Craft*)i->rule;
				// Transfer soldiers inside craft
				for (std::vector<Soldier*>::iterator s = _baseFrom->getSoldiers()->begin(); s != _baseFrom->getSoldiers()->end();)
				{
					if ((*s)->getCraft() == craft)
					{
						(*s)->setPsiTraining(false);
						(*s)->setTraining(false);
						if (craft->getStatus() == "STR_OUT")
						{
							_baseTo->getSoldiers()->push_back(*s);
						}
						else
						{
							t = new Transfer(time);
							t->setSoldier(*s);
							_baseTo->getTransfers()->push_back(t);
						}
						s = _baseFrom->getSoldiers()->erase(s);
					}
					else
					{
						++s;
					}
				}

				// Transfer craft
				_baseFrom->removeCraft(craft, false);
				if (craft->getStatus() == "STR_OUT")
				{
					bool returning = (craft->getDestination() == (Target*)craft->getBase());
					_baseTo->getCrafts()->push_back(craft);
					craft->setBase(_baseTo, false);
					if (craft->getFuel() <= craft->getFuelLimit(_baseTo))
					{
						craft->setLowFuel(true);
						craft->returnToBase();
					}
					else if (returning)
					{
						craft->setLowFuel(false);
						craft->returnToBase();
					}
				}
				else
				{
					t = new Transfer(time);
					t->setCraft(craft);
					_baseTo->getTransfers()->push_back(t);
				}
				break;
			case TRANSFER_SCIENTIST:
				_baseFrom->setScientists(_baseFrom->getScientists() - i->amount);
				t = new Transfer(time);
				t->setScientists(i->amount);
				_baseTo->getTransfers()->push_back(t);
				break;
			case TRANSFER_ENGINEER:
				_baseFrom->setEngineers(_baseFrom->getEngineers() - i->amount);
				t = new Transfer(time);
				t->setEngineers(i->amount);
				_baseTo->getTransfers()->push_back(t);
				break;
			case TRANSFER_ITEM:
				RuleItem *item = (RuleItem*)i->rule;
				_baseFrom->getStorageItems()->removeItem(item, i->amount);
				t = new Transfer(time);
				t->setItems(item->getType(), i->amount);
				_baseTo->getTransfers()->push_back(t);
				if (_debriefingState != 0)
				{
					// remember the decreased amount for next sell/transfer
					_debriefingState->decreaseRecoveredItemCount(item, i->amount);
				}
				break;
			}
		}
	}

	if (_debriefingState != 0 && _debriefingState->getTotalRecoveredItemCount() <= 0)
	{
		_debriefingState->hideSellTransferButtons();
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void TransferItemsState::btnCancelClick(Action *)
{
	_game->popState();
	_game->popState();
}

/**
 * Increase all items to max, i.e. transfer everything.
 * @param action Pointer to an action.
 */
void TransferItemsState::btnTransferAllClick(Action *)
{
	bool allItemsSelected = true;
	for (size_t i = 0; i < _lstItems->getTexts(); ++i)
	{
		if (_items[_rows[i]].type == TRANSFER_ITEM && _items[_rows[i]].amount < _items[_rows[i]].qtySrc)
		{
			allItemsSelected = false;
			break;
		}
	}

	size_t backup = _sel;
	_errorShown = false;
	for (size_t i = 0; i < _lstItems->getTexts(); ++i)
	{
		if (_items[_rows[i]].type == TRANSFER_ITEM)
		{
			_sel = i;
			allItemsSelected ? decreaseByValue(INT_MAX) : increaseByValue(INT_MAX);
			if (_errorShown)
			{
				break; // stop on first error
			}
		}
	}
	_sel = backup;
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsLeftArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerInc->isRunning()) _timerInc->start();
}

/**
 * Stops increasing the item.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected item;
 * by one on left-click; to max on right-click.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) increaseByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		increaseByValue(1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the item.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsRightArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerDec->isRunning()) _timerDec->start();
}

/**
 * Stops decreasing the item.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected item;
 * by one on left-click; to 0 on right-click.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) decreaseByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		decreaseByValue(1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void TransferItemsState::lstItemsMousePress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			increaseByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			decreaseByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			return;
		}
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				_game->pushState(new ManufactureDependenciesTreeState(rule->getType()));
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
		else if (getRow().type == TRANSFER_CRAFT)
		{
			Craft *rule = (Craft*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getRules()->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
	}
}

/**
 * Increases the quantity of the selected item to transfer by one.
 */
void TransferItemsState::increase()
{
	_timerDec->setInterval(50);
	_timerInc->setInterval(50);
	increaseByValue(1);
}

/**
 * Increases the quantity of the selected item to transfer by "change".
 * @param change How much we want to add.
 */
void TransferItemsState::increaseByValue(int change)
{
	if (0 >= change || getRow().qtySrc <= getRow().amount) return;
	std::string errorMessage;
	RuleItem *selItem = 0;
	Craft *craft = 0;

	switch (getRow().type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		if (_pQty + 1 > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
		{
			errorMessage = tr("STR_NO_FREE_ACCOMODATION");
		}
		break;
	case TRANSFER_CRAFT:
		craft = (Craft*)getRow().rule;
		if (_cQty + 1 > _baseTo->getAvailableHangars() - _baseTo->getUsedHangars())
		{
			errorMessage = tr("STR_NO_FREE_HANGARS_FOR_TRANSFER");
		}
		else if (craft->getNumTotalSoldiers() > 0 && _pQty + craft->getNumTotalSoldiers() > _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters())
		{
			errorMessage = tr("STR_NO_FREE_ACCOMODATION_CREW");
		}
		else if (Options::storageLimitsEnforced)
		{
			auto used = craft->getTotalItemStorageSize(_game->getMod());
			if (used > 0.0 && _baseTo->storesOverfull(_iQty + used))
			{
				errorMessage = tr("STR_NOT_ENOUGH_STORE_SPACE_FOR_CRAFT");
			}
		}
		break;
	case TRANSFER_ITEM:
		selItem = (RuleItem*)getRow().rule;
		if (selItem->getSize() > 0.0 && _baseTo->storesOverfull(selItem->getSize() + _iQty))
		{
			errorMessage = tr("STR_NOT_ENOUGH_STORE_SPACE");
		}
		if (selItem->isAlien())
		{
			if (Options::storageLimitsEnforced * _aQty + 1 > _baseTo->getAvailableContainment(selItem->getPrisonType()) - Options::storageLimitsEnforced * _baseTo->getUsedContainment(selItem->getPrisonType()))
			{
				errorMessage = trAlt("STR_NO_ALIEN_CONTAINMENT_FOR_TRANSFER", selItem->getPrisonType());
			}
		}
		break;
	}

	if (errorMessage.empty())
	{
		int freeQuarters = _baseTo->getAvailableQuarters() - _baseTo->getUsedQuarters() - _pQty;
		switch (getRow().type)
		{
		case TRANSFER_SOLDIER:
		case TRANSFER_SCIENTIST:
		case TRANSFER_ENGINEER:
			change = std::min(std::min(freeQuarters, getRow().qtySrc - getRow().amount), change);
			_pQty += change;
			getRow().amount += change;
			_total += getRow().cost * change;
			break;
		case TRANSFER_CRAFT:
			_cQty++;
			_pQty += craft->getNumTotalSoldiers();
			_iQty += craft->getTotalItemStorageSize(_game->getMod());
			getRow().amount++;
			if (!Options::canTransferCraftsWhileAirborne || craft->getStatus() != "STR_OUT")
				_total += getRow().cost;
			break;
		case TRANSFER_ITEM:
			if (selItem->isAlien())
			{
				int freeContainment = Options::storageLimitsEnforced ? _baseTo->getAvailableContainment(selItem->getPrisonType()) - _baseTo->getUsedContainment(selItem->getPrisonType()) - _aQty : INT_MAX;
				change = std::min(std::min(freeContainment, getRow().qtySrc - getRow().amount), change);
			}
			// both aliens and items
			{
				double storesNeededPerItem = ((RuleItem*)getRow().rule)->getSize();
				double freeStores = _baseTo->getAvailableStores() - _baseTo->getUsedStores() - _iQty;
				double freeStoresForItem = (double)(INT_MAX);
				if (!AreSame(storesNeededPerItem, 0.0) && storesNeededPerItem > 0.0)
				{
					freeStoresForItem = (freeStores + 0.05) / storesNeededPerItem;
				}
				change = std::min(std::min((int)freeStoresForItem, getRow().qtySrc - getRow().amount), change);
				_iQty += change * storesNeededPerItem;
			}
			if (selItem->isAlien())
			{
				_aQty += change;
			}
			getRow().amount += change;
			_total += getRow().cost * change;
			break;
		}
		updateItemStrings();
	}
	else
	{
		_timerInc->stop();
		RuleInterface *menuInterface = _game->getMod()->getInterface("transferMenu");
		_game->pushState(new ErrorMessageState(errorMessage, _palette, menuInterface->getElement("errorMessage")->color, "BACK13.SCR", menuInterface->getElement("errorPalette")->color));
		_errorShown = true;
	}
}

/**
 * Decreases the quantity of the selected item to transfer by one.
 */
void TransferItemsState::decrease()
{
	_timerInc->setInterval(50);
	_timerDec->setInterval(50);
	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected item to transfer by "change".
 * @param change How much we want to remove.
 */
void TransferItemsState::decreaseByValue(int change)
{
	if (0 >= change || 0 >= getRow().amount) return;
	Craft *craft = 0;
	change = std::min(getRow().amount, change);

	switch (getRow().type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		_pQty -= change;
		break;
	case TRANSFER_CRAFT:
		craft = (Craft*)getRow().rule;
		_cQty--;
		_pQty -= craft->getNumTotalSoldiers();
		_iQty -= craft->getTotalItemStorageSize(_game->getMod());
		break;
	case TRANSFER_ITEM:
		const RuleItem *selItem = (RuleItem*)getRow().rule;
		_iQty -= selItem->getSize() * change;
		if (selItem->isAlien())
		{
			_aQty -= change;
		}
		break;
	}
	getRow().amount -= change;
	if (!Options::canTransferCraftsWhileAirborne || 0 == craft || craft->getStatus() != "STR_OUT")
		_total -= getRow().cost * change;
	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void TransferItemsState::updateItemStrings()
{
	std::ostringstream ss1, ss2;
	ss1 << getRow().qtySrc - getRow().amount;
	ss2 << getRow().amount;
	_lstItems->setCellText(_sel, 1, ss1.str());
	_lstItems->setCellText(_sel, 2, ss2.str());

	if (getRow().amount > 0)
	{
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	}
	else
	{
		_lstItems->setRowColor(_sel, _lstItems->getColor());
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, _ammoColor);
			}
		}
	}
}

/**
 * Gets the total cost of the current transfer.
 * @return Total cost.
 */
int TransferItemsState::getTotal() const
{
	return _total;
}

/**
 * Gets the shortest distance between the two bases.
 * @return Distance.
 */
double TransferItemsState::getDistance() const
{
	double x[3], y[3], z[3], r = 51.2;
	Base *base = _baseFrom;
	for (int i = 0; i < 2; ++i) {
		x[i] = r * cos(base->getLatitude()) * cos(base->getLongitude());
		y[i] = r * cos(base->getLatitude()) * sin(base->getLongitude());
		z[i] = r * -sin(base->getLatitude());
		base = _baseTo;
	}
	x[2] = x[1] - x[0];
	y[2] = y[1] - y[0];
	z[2] = z[1] - z[0];
	return sqrt(x[2] * x[2] + y[2] * y[2] + z[2] * z[2]);
}

/**
* Updates the production list to match the category filter.
*/
void TransferItemsState::cbxCategoryChange(Action *)
{
	_previousSort = _currentSort;

	if (_game->isCtrlPressed())
	{
		_currentSort = _game->isShiftPressed() ? TransferSortDirection::BY_UNIT_SIZE : TransferSortDirection::BY_TOTAL_SIZE;
	}
	else if (_game->isAltPressed())
	{
		_currentSort = _game->isShiftPressed() ? TransferSortDirection::BY_UNIT_COST : TransferSortDirection::BY_TOTAL_COST;
	}
	else
	{
		_currentSort = TransferSortDirection::BY_LIST_ORDER;
	}

	updateList();
}

}
