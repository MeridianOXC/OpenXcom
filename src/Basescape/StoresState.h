#pragma once
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
#include "../Engine/State.h"
#include <vector>

namespace OpenXcom
{

class Base;
class RuleItem;
class TextButton;
class ToggleTextButton;
class Window;
class Text;
class TextEdit;
class TextList;
class ArrowButton;

/// Item sorting modes.
enum ItemSort
{
	ITEM_SORT_NONE,
	ITEM_SORT_NAME_ASC,
	ITEM_SORT_NAME_DESC,
	ITEM_SORT_QUANTITY_ASC,
	ITEM_SORT_QUANTITY_DESC,
	ITEM_SORT_SIZE_ASC,
	ITEM_SORT_SIZE_DESC,
	ITEM_SORT_SPACE_USED_ASC,
	ITEM_SORT_SPACE_USED_DESC
};

struct StoredItem
{
	StoredItem(RuleItem* _rule, const std::string &_name, int _quantity, double _size, double _spaceUsed)
		: rule(_rule), name(_name), quantity(_quantity), size(_size), spaceUsed(_spaceUsed)
	{
	}
	RuleItem* rule;
	std::string name;
	int quantity;
	double size;
	double spaceUsed;
};

/**
 * Stores window that displays all
 * the items currently stored in a base.
 */
class StoresState : public State
{
private:
	Base *_base;

	TextButton *_btnOk;
	TextEdit *_btnQuickSearch;
	ToggleTextButton *_btnGrandTotal;
	Window *_window;
	Text *_txtTitle, *_txtItem, *_txtQuantity, *_txtSize, *_txtSpaceUsed;
	TextList *_lstStores;
	ArrowButton *_sortName, *_sortQuantity, *_sortSize, *_sortSpaceUsed;

	std::vector<StoredItem> _itemList;
	void initList();
	ItemSort _itemOrder;
	void updateArrows();
public:
	/// Creates the Stores state.
	StoresState(Base *base);
	/// Cleans up the Stores state.
	~StoresState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action *action);
	void btnQuickSearchApply(Action *action);
	/// Handler for clicking the Grand Total button.
	void btnGrandTotalClick(Action *action);
	/// Sets up the item list.
	void init() override;
	/// Sorts the item list.
	void sortList();
	/// Updates the item list.
	virtual void updateList();
	/// Handler for clicking the Name arrow.
	void sortNameClick(Action *action);
	/// Handler for clicking the Quantity arrow.
	void sortQuantityClick(Action *action);
	/// Handler for clicking the Size arrow.
	void sortSizeClick(Action *action);
	/// Handler for clicking the Space Used arrow.
	void sortSpaceUsedClick(Action *action);
	/// Handler for clicking the Stores list.
	void lstStoresClick(Action* action);
};

}
