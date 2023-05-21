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
#include "StoresState.h"
#include <sstream>
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Transfer.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleResearch.h"
#include "../Ufopaedia/Ufopaedia.h"
#include <algorithm>
#include <locale>

namespace OpenXcom
{

struct compareItemName
{
	typedef StoredItem& first_argument_type;
	typedef StoredItem& second_argument_type;
	typedef bool result_type;

	bool _reverse;

	compareItemName(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return Unicode::naturalCompare(a.name, b.name);
	}
};

struct compareItemQuantity
{
	typedef StoredItem& first_argument_type;
	typedef StoredItem& second_argument_type;
	typedef bool result_type;

	bool _reverse;

	compareItemQuantity(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return (a.quantity < b.quantity) || ((a.quantity == b.quantity) && Unicode::naturalCompare(a.name, b.name));
	}
};

struct compareItemSize
{
	typedef StoredItem& first_argument_type;
	typedef StoredItem& second_argument_type;
	typedef bool result_type;

	bool _reverse;

	compareItemSize(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return (a.size < b.size) || ((a.size == b.size) && Unicode::naturalCompare(a.name, b.name));
	}
};

struct compareItemSpaceUsed
{
	typedef StoredItem& first_argument_type;
	typedef StoredItem& second_argument_type;
	typedef bool result_type;

	bool _reverse;

	compareItemSpaceUsed(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return (a.spaceUsed < b.spaceUsed) || ((a.spaceUsed == b.spaceUsed) && Unicode::naturalCompare(a.name, b.name));
	}
};


/**
 * Initializes all the elements in the Stores window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
StoresState::StoresState(Base *base) : _base(base)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 20);
	_btnOk = new TextButton(148, 16, 164, 176);
	_btnGrandTotal = new ToggleTextButton(148, 16, 8, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtItem = new Text(142, 9, 10, 32);
	_txtQuantity = new Text(54, 9, 152, 32);
	_txtSize = new Text(54, 9, 212, 32);
	_txtSpaceUsed = new Text(54, 9, 248, 32);
	_lstStores = new TextList(288, 128, 8, 40);
	_sortName = new ArrowButton(ARROW_NONE, 11, 8, 10, 32);
	_sortQuantity = new ArrowButton(ARROW_NONE, 11, 8, 152, 32);
	_sortSize = new ArrowButton(ARROW_NONE, 11, 8, 212, 32);
	_sortSpaceUsed = new ArrowButton(ARROW_NONE, 11, 8, 248, 32);

	// Set palette
	setInterface("storesInfo");

	add(_window, "window", "storesInfo");
	add(_btnQuickSearch, "button", "storesInfo");
	add(_btnOk, "button", "storesInfo");
	add(_btnGrandTotal, "button", "storesInfo");
	add(_txtTitle, "text", "storesInfo");
	add(_txtItem, "text", "storesInfo");
	add(_txtQuantity, "text", "storesInfo");
	add(_txtSize, "text", "storesInfo");
	add(_txtSpaceUsed, "text", "storesInfo");
	add(_lstStores, "list", "storesInfo");
	add(_sortName, "text", "storesInfo");
	add(_sortQuantity, "text", "storesInfo");
	add(_sortSize, "text", "storesInfo");
	add(_sortSpaceUsed, "text", "storesInfo");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "storesInfo");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&StoresState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&StoresState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&StoresState::btnOkClick, Options::keyCancel);

	_btnGrandTotal->setText(tr("STR_GRAND_TOTAL"));
	_btnGrandTotal->onMouseClick((ActionHandler)&StoresState::btnGrandTotalClick);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_STORES"));

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtSize->setText(tr("STR_SIZE_UC"));
	_txtSpaceUsed->setText(tr("STR_SPACE_USED_UC"));

	_lstStores->setColumns(4, 162, 40, 50, 34);
	_lstStores->setSelectable(true);
	_lstStores->setBackground(_window);
	_lstStores->setMargin(2);
	_lstStores->onMouseClick((ActionHandler)&StoresState::lstStoresClick, SDL_BUTTON_MIDDLE);

	_sortName->setX(_sortName->getX() + _txtItem->getTextWidth() + 4);
	_sortName->onMouseClick((ActionHandler)&StoresState::sortNameClick);

	_sortQuantity->setX(_sortQuantity->getX() + _txtQuantity->getTextWidth() + 4);
	_sortQuantity->onMouseClick((ActionHandler)&StoresState::sortQuantityClick);

	_sortSize->setX(_sortSize->getX() + _txtSize->getTextWidth() + 4);
	_sortSize->onMouseClick((ActionHandler)&StoresState::sortSizeClick);

	_sortSpaceUsed->setX(_sortSpaceUsed->getX() + _txtSpaceUsed->getTextWidth() + 4);
	_sortSpaceUsed->onMouseClick((ActionHandler)&StoresState::sortSpaceUsedClick);

	itemOrder = ITEM_SORT_NONE;
	updateArrows();

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&StoresState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&StoresState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
}

/**
 *
 */
StoresState::~StoresState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void StoresState::btnOkClick(Action *)
{
	_game->popState();
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void StoresState::btnQuickSearchToggle(Action *action)
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
void StoresState::btnQuickSearchApply(Action *)
{
	initList(_btnGrandTotal->getPressed());
}

/**
 * Reloads the item list.
 */
void StoresState::initList(bool grandTotal)
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	// clear everything
	_lstStores->clearList();
	_itemList.clear();

	// find relevant items
	for (auto& itemType : _game->getMod()->getItemsList())
	{
		// quick search
		if (!searchString.empty())
		{
			std::string projectName = tr(itemType);
			Unicode::upperCase(projectName);
			if (projectName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		int qty = 0;
		auto* rule = _game->getMod()->getItem(itemType, true);
		if (!grandTotal)
		{
			// items in stores from this base only
			qty += _base->getStorageItems()->getItem(itemType);
		}
		else
		{

			// items from all bases
			for (auto* xbase : *_game->getSavedGame()->getBases())
			{
				// 1. items in base stores
				qty += xbase->getStorageItems()->getItem(rule);

				// 2. items from craft
				for (const auto* craft : *xbase->getCrafts())
				{
					qty += craft->getTotalItemCount(rule);
				}

				// 3. armor in use (worn by soldiers)
				for (const auto* soldier : *xbase->getSoldiers())
				{
					if (soldier->getArmor()->getStoreItem() == rule)
					{
						qty += 1;
					}
				}

				// 4. items/aliens in research
				for (const auto* research : xbase->getResearch())
				{
					if (research->getRules()->needItem() && research->getRules()->getName() == itemType)
					{
						if (research->getRules()->destroyItem())
						{
							qty += 1;
							break;
						}
					}
				}

				// 5. items in transfer
				for (auto* transfer : *xbase->getTransfers())
				{
					if (transfer->getCraft())
					{
						// 5a. craft equipment, weapons, vehicles
						qty += transfer->getCraft()->getTotalItemCount(rule);
					}
					else if (transfer->getSoldier())
					{
						// 5c. armor in use (worn by soldiers)
						if (transfer->getSoldier()->getArmor()->getStoreItem() == rule)
						{
							qty += 1;
						}
					}
					else if (transfer->getItems() == itemType)
					{
						// 5b. items in transfer
						qty += transfer->getQuantity();
					}
				}
			}
		}

		if (qty > 0)
		{
			_itemList.push_back(StoredItem(rule, tr(itemType), qty, rule->getSize(), qty * rule->getSize()));
		}
	}

	sortList(itemOrder);
}

/**
 * Refreshes the item list.
 */
void StoresState::init()
{
	State::init();

	initList(false);
}

/**
 * Includes items from all bases.
 * @param action Pointer to an action.
 */
void StoresState::btnGrandTotalClick(Action *action)
{
	initList(_btnGrandTotal->getPressed());
}

/**
 * Updates the sorting arrows based
 * on the current setting.
 */
void StoresState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	_sortQuantity->setShape(ARROW_NONE);
	_sortSize->setShape(ARROW_NONE);
	_sortSpaceUsed->setShape(ARROW_NONE);
	switch (itemOrder)
	{
	case ITEM_SORT_NONE:
		break;
	case ITEM_SORT_NAME_ASC:
		_sortName->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_NAME_DESC:
		_sortName->setShape(ARROW_SMALL_DOWN);
		break;
	case ITEM_SORT_QUANTITY_ASC:
		_sortQuantity->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_QUANTITY_DESC:
		_sortQuantity->setShape(ARROW_SMALL_DOWN);
		break;
	case ITEM_SORT_SIZE_ASC:
		_sortSize->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_SIZE_DESC:
		_sortSize->setShape(ARROW_SMALL_DOWN);
		break;
	case ITEM_SORT_SPACE_USED_ASC:
		_sortSpaceUsed->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_SPACE_USED_DESC:
		_sortSpaceUsed->setShape(ARROW_SMALL_DOWN);
		break;
	default:
		break;
	}
}

/**
 * Sorts the item list.
 * @param sort Order to sort the items in.
 */
void StoresState::sortList(ItemSort sort)
{
	switch (sort)
	{
	case ITEM_SORT_NONE:
		break;
	case ITEM_SORT_NAME_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemName(false));
		break;
	case ITEM_SORT_NAME_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemName(true));
		break;
	case ITEM_SORT_QUANTITY_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemQuantity(false));
		break;
	case ITEM_SORT_QUANTITY_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemQuantity(true));
		break;
	case ITEM_SORT_SIZE_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemSize(false));
		break;
	case ITEM_SORT_SIZE_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemSize(true));
		break;
	case ITEM_SORT_SPACE_USED_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemSpaceUsed(false));
		break;
	case ITEM_SORT_SPACE_USED_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemSpaceUsed(true));
		break;
	}
	updateList();
}

/**
 * Updates the item list with the current list
 * of available items.
 */
void StoresState::updateList()
{
	for (const auto& item : _itemList)
	{
		std::ostringstream ss, ss2, ss3;
		ss << item.quantity;
		ss2 << item.size;
		ss3 << item.spaceUsed;
		_lstStores->addRow(4, item.name.c_str(), ss.str().c_str(), ss2.str().c_str(), ss3.str().c_str());
	}
}

/**
 * Sorts the items by name.
 * @param action Pointer to an action.
 */
void StoresState::sortNameClick(Action *)
{
	if (itemOrder == ITEM_SORT_NAME_ASC)
	{
		itemOrder = ITEM_SORT_NAME_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_NAME_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

/**
 * Sorts the items by quantity.
 * @param action Pointer to an action.
 */
void StoresState::sortQuantityClick(Action *)
{
	if (itemOrder == ITEM_SORT_QUANTITY_ASC)
	{
		itemOrder = ITEM_SORT_QUANTITY_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_QUANTITY_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

/**
 * Sorts the items by size.
 * @param action Pointer to an action.
 */
void StoresState::sortSizeClick(Action *)
{
	if (itemOrder == ITEM_SORT_SIZE_ASC)
	{
		itemOrder = ITEM_SORT_SIZE_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_SIZE_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

/**
 * Sorts the items by space used.
 * @param action Pointer to an action.
 */
void StoresState::sortSpaceUsedClick(Action *)
{
	if (itemOrder == ITEM_SORT_SPACE_USED_ASC)
	{
		itemOrder = ITEM_SORT_SPACE_USED_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_SPACE_USED_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

/**
 * Handles mouse clicks.
 * @param action Pointer to an action.
 */
void StoresState::lstStoresClick(Action* action)
{
	if (_game->isMiddleClick(action))
	{
		auto* rule = _itemList[_lstStores->getSelectedRow()].rule;

		std::string articleId = rule->getUfopediaType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

}
