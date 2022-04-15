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
#include "SellState.h"
#include "ManufactureDependenciesTreeState.h"
#include <algorithm>
#include <locale>
#include <sstream>
#include <climits>
#include <cmath>
#include <iomanip>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Vehicle.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Unicode.h"
#include "../Mod/RuleInterface.h"
#include "../Battlescape/DebriefingState.h"
#include "TransferBaseState.h"
#include "TechTreeViewerState.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param origin Game section that originated this state.
 */
SellState::SellState(Base *base, DebriefingState *debriefingState, OptionsOrigin origin) : _base(base), _debriefingState(debriefingState), _sel(0), _total(0), _spaceChange(0), _origin(origin),
	_reset(false), _sellAllButOne(false), _delayedInitDone(false), _previousSort(TransferSortDirection::BY_LIST_ORDER), _currentSort(TransferSortDirection::BY_LIST_ORDER)
{
	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&SellState::increase);
	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&SellState::decrease);
}

/**
 * Delayed constructor functionality.
 */
void SellState::delayedInit()
{
	if (_delayedInitDone)
	{
		return;
	}
	_delayedInitDone = true;

	bool overfull = _debriefingState == 0 && Options::storageLimitsEnforced && _base->storesOverfull();
	bool overfullCritical = overfull ? _base->storesOverfullCritical() : false;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 13);
	//_btnOk = new TextButton(overfull? 288:148, 16, overfull? 16:8, 176);
	_btnOk = new TextButton(148, 16, 8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_btnTransfer = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtSales = new Text(150, 9, 10, 24);
	_txtFunds = new Text(150, 9, 160, 24);
	_txtSpaceUsed = new Text(150, 9, 160, 34);
	_txtQuantity = new Text(54, 9, 136, 44);
	_txtSell = new Text(96, 9, 190, 44);
	_txtValue = new Text(40, 9, 270, 44);
	_cbxCategory = new ComboBox(this, 120, 16, 10, 36);
	_lstItems = new TextList(287, 120, 8, 54);

	// Set palette
	setInterface("sellMenu");

	_ammoColor = _game->getMod()->getInterface("sellMenu")->getElement("ammoColor")->color;

	add(_window, "window", "sellMenu");
	add(_btnQuickSearch, "button", "sellMenu");
	add(_btnOk, "button", "sellMenu");
	add(_btnCancel, "button", "sellMenu");
	add(_btnTransfer, "button", "sellMenu");
	add(_txtTitle, "text", "sellMenu");
	add(_txtSales, "text", "sellMenu");
	add(_txtFunds, "text", "sellMenu");
	add(_txtSpaceUsed, "text", "sellMenu");
	add(_txtQuantity, "text", "sellMenu");
	add(_txtSell, "text", "sellMenu");
	add(_txtValue, "text", "sellMenu");
	add(_lstItems, "list", "sellMenu");
	add(_cbxCategory, "text", "sellMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "sellMenu");

	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)&SellState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SellState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&SellState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SellState::btnCancelClick, Options::keyCancel);

	_btnTransfer->setText(tr("STR_GO_TO_TRANSFERS"));
	_btnTransfer->onMouseClick((ActionHandler)&SellState::btnTransferClick);

	_btnCancel->setVisible(!overfull);
	_btnOk->setVisible(!overfull);
	_btnTransfer->setVisible(overfull);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

	_txtFunds->setText(tr("STR_FUNDS").arg(Unicode::formatFunding(_game->getSavedGame()->getFunds())));

	_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);

	std::ostringstream ss;
	ss << _base->getUsedStores() << ":" << _base->getAvailableStores();
	_txtSpaceUsed->setText(ss.str());
	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss.str()));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSell->setText(tr("STR_SELL_SACK"));

	_txtValue->setText(tr("STR_VALUE"));

	_lstItems->setArrowColumn(182, ARROW_VERTICAL);
	_lstItems->setColumns(4, 156, 54, 24, 53);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(2);
	_lstItems->onLeftArrowPress((ActionHandler)&SellState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)&SellState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)&SellState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)&SellState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)&SellState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)&SellState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)&SellState::lstItemsMousePress);

	_cats.push_back("STR_ALL_ITEMS");

	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if (_debriefingState) break;
		if ((*i)->getCraft() == 0)
		{
			TransferRow row = { TRANSFER_SOLDIER, (*i), (*i)->getName(true), 0, 1, 0, 0, -4, 0, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	for (std::vector<Craft*>::iterator i = _base->getCrafts()->begin(); i != _base->getCrafts()->end(); ++i)
	{
		if (_debriefingState) break;
		if ((*i)->getStatus() != "STR_OUT")
		{
			TransferRow row = { TRANSFER_CRAFT, (*i), (*i)->getName(_game->getLanguage()), (*i)->getRules()->getSellCost(), 1, 0, 0, -3, 0, 0, (*i)->getRules()->getSellCost() };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	if (_base->getAvailableScientists() > 0 && _debriefingState == 0)
	{
		TransferRow row = { TRANSFER_SCIENTIST, 0, tr("STR_SCIENTIST"), 0, _base->getAvailableScientists(), 0, 0, -2, 0, 0, 0 };
		_items.push_back(row);
		std::string cat = getCategory(_items.size() - 1);
		if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
		{
			_cats.push_back(cat);
		}
	}
	if (_base->getAvailableEngineers() > 0 && _debriefingState == 0)
	{
		TransferRow row = { TRANSFER_ENGINEER, 0, tr("STR_ENGINEER"), 0, _base->getAvailableEngineers(), 0, 0, -1, 0, 0, 0 };
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
		const RuleItem *rule = _game->getMod()->getItem(*i, true);
		int qty = 0;
		if (_debriefingState != 0)
		{
			qty = _debriefingState->getRecoveredItemCount(rule);
		}
		else
		{
			qty = _base->getStorageItems()->getItem(rule);
			if (Options::storageLimitsEnforced && (_origin == OPT_BATTLESCAPE || overfullCritical))
			{
				for (std::vector<Transfer*>::iterator j = _base->getTransfers()->begin(); j != _base->getTransfers()->end(); ++j)
				{
					if ((*j)->getItems() == *i)
					{
						qty += (*j)->getQuantity();
					}
					else if ((*j)->getCraft())
					{
						qty += overfullCritical ? (*j)->getCraft()->getTotalItemCount(rule) : (*j)->getCraft()->getItems()->getItem(rule);
					}
				}
				for (std::vector<Craft*>::iterator j = _base->getCrafts()->begin(); j != _base->getCrafts()->end(); ++j)
				{
					qty +=  overfullCritical ? (*j)->getTotalItemCount(rule) : (*j)->getItems()->getItem(rule);
				}
			}
		}
		if (qty > 0 && (Options::canSellLiveAliens || !rule->isAlien()))
		{
			TransferRow row = { TRANSFER_ITEM, rule, tr(*i), rule->getSellCost(), qty, 0, 0, rule->getListOrder(), rule->getSize(), qty * rule->getSize(), (int64_t)qty * rule->getSellCost() };
			if ((_debriefingState != 0) && (_game->getSavedGame()->getAutosell(rule)))
			{
				row.amount = qty;
				_total += row.cost * qty;
				_spaceChange -= qty * rule->getSize();
			}
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

	int64_t adjustedTotal = _total * _game->getSavedGame()->getSellPriceCoefficient() / 100;
	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Unicode::formatFunding(adjustedTotal)));

	_cbxCategory->setOptions(_cats, true);
	_cbxCategory->onChange((ActionHandler)&SellState::cbxCategoryChange);
	_cbxCategory->onKeyboardPress((ActionHandler)&SellState::btnSellAllClick, Options::keySellAll);
	_cbxCategory->onKeyboardPress((ActionHandler)&SellState::btnSellAllButOneClick, Options::keySellAllButOne);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SellState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	// OK button is not always visible, so bind it here
	_cbxCategory->onKeyboardRelease((ActionHandler)&SellState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	updateList();
}

/**
 *
 */
SellState::~SellState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
* Resets stuff when coming back from other screens.
*/
void SellState::init()
{
	delayedInit();

	State::init();

	if (_reset)
	{
		_game->popState();
		_game->pushState(new SellState(_base, _debriefingState, _origin));
	}
}

/**
 * Runs the arrow timers.
 */
void SellState::think()
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
std::string SellState::getCategory(int sel) const
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
bool SellState::belongsToCategory(int sel, const std::string &cat) const
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
void SellState::btnQuickSearchToggle(Action *action)
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
void SellState::btnQuickSearchApply(Action *)
{
	updateList();
}

/**
 * Filters the current list of items.
 */
void SellState::updateList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_lstItems->clearList();
	_rows.clear();

	int sellPriceCoefficient = _game->getSavedGame()->getSellPriceCoefficient();

	size_t selCategory = _cbxCategory->getSelected();
	const std::string selectedCategory = _cats[selCategory];
	bool categoryFilterEnabled = (selectedCategory != "STR_ALL_ITEMS");
	bool categoryUnassigned = (selectedCategory == "STR_UNASSIGNED");

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
			else if (categoryFilterEnabled && !belongsToCategory(i, selectedCategory))
			{
				continue;
			}
		}
		else
		{
			if (categoryFilterEnabled && selectedCategory != getCategory(i))
			{
				continue;
			}
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
		std::ostringstream ssQty, ssAmount;
		ssQty << _items[i].qtySrc - _items[i].amount;
		ssAmount << _items[i].amount;
		int64_t adjustedCost = _items[i].cost;
		adjustedCost = adjustedCost * sellPriceCoefficient / 100;
		_lstItems->addRow(4, name.c_str(), ssQty.str().c_str(), ssAmount.str().c_str(), Unicode::formatFunding(adjustedCost).c_str());
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
 * Sells the selected items.
 * @param action Pointer to an action.
 */
void SellState::btnOkClick(Action *)
{
	int64_t adjustedTotal = _total * _game->getSavedGame()->getSellPriceCoefficient() / 100;
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + adjustedTotal);
	Soldier *soldier;
	Craft *craft;

	auto cleanUpContainer = [&](ItemContainer* container, const RuleItem* rule, int toRemove) -> int
	{
		auto curr = container->getItem(rule);
		if (curr >= toRemove)
		{
			container->removeItem(rule, toRemove);
			return 0;
		}
		else
		{
			container->removeItem(rule, INT_MAX);
			return toRemove - curr;
		}
	};

	auto cleanUpCraft = [&](Craft* craft2, const RuleItem* rule, int toRemove) -> int
	{
		struct S
		{
			int ToRemove, ToSave;
			const RuleItem* rule;
		};

		auto tryRemove = [&toRemove, rule](int curr, const RuleItem* i) -> S
		{
			if (i == rule)
			{
				auto r = std::min(toRemove, curr);
				toRemove -= r;
				curr -= r;
				return S{ r, curr, i };
			}
			else
			{
				return S{ 0, curr, i };
			}
		};
		auto tryStore = [&](S s)
		{
			if (s.ToSave > 0)
			{
				_base->getStorageItems()->addItem(s.rule, s.ToSave);
			}
		};

		for (auto*& w :* craft2->getWeapons())
		{
			if (w != nullptr)
			{
				auto* wr = w->getRules();

				auto launcher = tryRemove(1, wr->getLauncherItem());
				auto clip = tryRemove(w->getClipsLoaded(), wr->getClipItem());
				if (launcher.ToRemove || clip.ToRemove)
				{
					tryStore(launcher);
					tryStore(clip);

					delete w;
					w = nullptr;
				}
			}
		}

		Collections::deleteIf(
			*craft2->getVehicles(),
			[&](Vehicle* v)
			{
				auto clipType = v->getRules()->getVehicleClipAmmo();

				auto launcher = tryRemove(1, v->getRules());
				auto clip = tryRemove(v->getRules()->getVehicleClipsLoaded(), clipType);

				if (launcher.ToRemove || clip.ToRemove)
				{
					tryStore(launcher);
					tryStore(clip);

					return true;
				}
				else
				{
					return false;
				}
			}
		);

		return toRemove;
	};

	for (std::vector<TransferRow>::const_iterator i = _items.begin(); i != _items.end(); ++i)
	{
		if (i->amount > 0)
		{
			switch (i->type)
			{
			case TRANSFER_SOLDIER:
				soldier = (Soldier*)i->rule;
				for (std::vector<Soldier*>::iterator s = _base->getSoldiers()->begin(); s != _base->getSoldiers()->end(); ++s)
				{
					if (*s == soldier)
					{
						if ((*s)->getArmor()->getStoreItem())
						{
							_base->getStorageItems()->addItem((*s)->getArmor()->getStoreItem()->getType());
						}
						_base->getSoldiers()->erase(s);
						break;
					}
				}
				delete soldier;
				break;
			case TRANSFER_CRAFT:
				craft = (Craft*)i->rule;
				_base->removeCraft(craft, true);
				delete craft;
				break;
			case TRANSFER_SCIENTIST:
				_base->setScientists(_base->getScientists() - i->amount);
				break;
			case TRANSFER_ENGINEER:
				_base->setEngineers(_base->getEngineers() - i->amount);
				break;
			case TRANSFER_ITEM:
				RuleItem *item = (RuleItem*)i->rule;
				{
					// remove all of said items from base
					int toRemove = cleanUpContainer(_base->getStorageItems(), item, i->amount);

					// if we still need to remove any, remove them from the crafts first, and keep a running tally
					for (std::vector<Craft*>::iterator j = _base->getCrafts()->begin(); j != _base->getCrafts()->end() && toRemove; ++j)
					{
						toRemove = cleanUpContainer((*j)->getItems(), item, toRemove);
						if (toRemove > 0)
						{
							toRemove = cleanUpCraft((*j), item, toRemove);
						}
					}

					// if there are STILL any left to remove, take them from the transfers, and if necessary, delete it.
					for (std::vector<Transfer*>::iterator j = _base->getTransfers()->begin(); j != _base->getTransfers()->end() && toRemove;)
					{
						if ((*j)->getItems() == item->getType())
						{
							if ((*j)->getQuantity() <= toRemove)
							{
								toRemove -= (*j)->getQuantity();
								delete *j;
								j = _base->getTransfers()->erase(j);
							}
							else
							{
								(*j)->setItems((*j)->getItems(), (*j)->getQuantity() - toRemove);
								toRemove = 0;
							}
						}
						else
						{
							if ((*j)->getCraft())
							{
								toRemove = cleanUpContainer((*j)->getCraft()->getItems(), item, toRemove);
								if (toRemove > 0)
								{
									toRemove = cleanUpCraft((*j)->getCraft(), item, toRemove);
								}
							}
							++j;
						}
					}
				}

				// Note: this only updates a helper map, it doesn't affect real item recovery (that has already happened and all items are already in the base)
				if (_debriefingState != 0)
				{
					// remember the decreased amount for next sell/transfer
					_debriefingState->decreaseRecoveredItemCount(item, i->amount);

					// set autosell status if we sold all of the item
					_game->getSavedGame()->setAutosell(item, (i->qtySrc == i->amount));
				}

				break;
			}
		}
		else
		{
			if (_debriefingState != 0 && i->type == TRANSFER_ITEM)
			{
				// disable autosell since we haven't sold any of the item.
				_game->getSavedGame()->setAutosell((RuleItem*)i->rule, false);
			}
		}
	}
	if (_debriefingState != 0 && _debriefingState->getTotalRecoveredItemCount() <= 0)
	{
		_debriefingState->hideSellTransferButtons();
	}
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SellState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
* Opens the Transfer UI and gives the player an option to transfer stuff instead of selling it.
* Returns back to this screen when finished.
* @param action Pointer to an action.
*/
void SellState::btnTransferClick(Action *)
{
	_reset = true;
	_game->pushState(new TransferBaseState(_base, nullptr));
}

/**
* Increase all items to max, i.e. sell everything.
* @param action Pointer to an action.
*/
void SellState::btnSellAllClick(Action *)
{
	bool allItemsSelected = true;
	for (size_t i = 0; i < _lstItems->getTexts(); ++i)
	{
		if (_items[_rows[i]].qtySrc > _items[_rows[i]].amount)
		{
			allItemsSelected = false;
			break;
		}
	}
	int dir = allItemsSelected ? -1 : 1;

	size_t backup = _sel;
	for (size_t i = 0; i < _lstItems->getTexts(); ++i)
	{
		_sel = i;
		changeByValue(INT_MAX, dir);
	}
	_sel = backup;
}

/**
* Increase all items to max - 1, i.e. sell everything but one.
* @param action Pointer to an action.
*/
void SellState::btnSellAllButOneClick(Action *)
{
	_sellAllButOne = true;
	btnSellAllClick(nullptr);
	_sellAllButOne = false;
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerInc->isRunning()) _timerInc->start();
}

/**
 * Stops increasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected item;
 * by one on left-click, to max on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) changeByValue(INT_MAX, 1);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1,1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerDec->isRunning()) _timerDec->start();
}

/**
 * Stops decreasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected item;
 * by one on left-click, to 0 on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) changeByValue(INT_MAX, -1);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1,-1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void SellState::lstItemsMousePress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, 1);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, -1);
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
				const RuleResearch *selectedTopic = _game->getMod()->getResearch(articleId, false);
				bool ctrlPressed = _game->isCtrlPressed();
				if (selectedTopic && !ctrlPressed)
				{
					_game->pushState(new TechTreeViewerState(selectedTopic, 0));
				}
				else
				{
					Ufopaedia::openArticle(_game, articleId);
				}
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
 * Increases the quantity of the selected item to sell by one.
 */
void SellState::increase()
{
	_timerDec->setInterval(50);
	_timerInc->setInterval(50);
	changeByValue(1,1);
}

/**
 * Increases or decreases the quantity of the selected item to sell.
 * @param change How much we want to add or remove.
 * @param dir Direction to change, +1 to increase or -1 to decrease.
 */
void SellState::changeByValue(int change, int dir)
{
	if (dir > 0)
	{
		if (0 >= change || getRow().qtySrc <= getRow().amount) return;
		change = std::min(getRow().qtySrc - getRow().amount, change);
		if (_sellAllButOne && change > 0)
		{
			--change;
		}
	}
	else
	{
		if (0 >= change || 0 >= getRow().amount) return;
		change = std::min(getRow().amount, change);
	}
	getRow().amount += dir * change;
	_total += dir * getRow().cost * change;

	// Calculate the change in storage space.
	Soldier *soldier;
	const RuleItem *item;
	switch (getRow().type)
	{
	case TRANSFER_SOLDIER:
		soldier = (Soldier*)getRow().rule;
		if (soldier->getArmor()->getStoreItem())
		{
			_spaceChange += dir * soldier->getArmor()->getStoreItem()->getSize();
		}
		break;
	case TRANSFER_CRAFT:
		// Note: in OXCE, there is no storage space change, everything on the craft is already included in the base storage space calculations
		break;
	case TRANSFER_ITEM:
		item = (const RuleItem*)getRow().rule;
		_spaceChange -= dir * change * item->getSize();
		break;
	default:
		//TRANSFER_SCIENTIST and TRANSFER_ENGINEER do not own anything that takes storage
		break;
	}

	updateItemStrings();
}

/**
 * Decreases the quantity of the selected item to sell by one.
 */
void SellState::decrease()
{
	_timerInc->setInterval(50);
	_timerDec->setInterval(50);
	changeByValue(1,-1);
}

/**
 * Updates the quantity-strings of the selected item.
 */
void SellState::updateItemStrings()
{
	std::ostringstream ss, ss2, ss3;
	ss << getRow().amount;
	_lstItems->setCellText(_sel, 2, ss.str());
	ss2 << getRow().qtySrc - getRow().amount;
	_lstItems->setCellText(_sel, 1, ss2.str());
	int64_t adjustedTotal = _total * _game->getSavedGame()->getSellPriceCoefficient() / 100;
	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Unicode::formatFunding(adjustedTotal)));

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

	ss3 << _base->getUsedStores();
	if (std::abs(_spaceChange) > 0.05)
	{
		ss3 << "(";
		if (_spaceChange > 0.05)
			ss3 << "+";
		ss3 << std::fixed << std::setprecision(1) << _spaceChange << ")";
	}
	ss3 << ":" << _base->getAvailableStores();
	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss3.str()));
	if (_debriefingState == 0 && Options::storageLimitsEnforced)
	{
		_btnOk->setVisible(!_base->storesOverfull(_spaceChange));
	}
}

/**
* Updates the production list to match the category filter.
*/
void SellState::cbxCategoryChange(Action *)
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
