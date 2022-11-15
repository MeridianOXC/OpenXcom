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
#include "../Savegame/Transfer.h"
#include <vector>
#include <string>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextEdit;
class TextList;
class ComboBox;
class Timer;
class Base;
class CannotReequipState;
class RuleItem;

/**
 * Purchase/Hire screen that lets the player buy
 * new items for a base.
 */
class PurchaseState : public State
{
private:
	Base *_base;
	CannotReequipState *_parent;
	bool _autoBuyDone;
	std::map<RuleItem*, int> _missingItemsMap;

	TextButton *_btnOk, *_btnCancel;
	TextEdit *_btnQuickSearch;
	Window *_window;
	Text *_txtTitle, *_txtFunds, *_txtPurchases, *_txtCost, *_txtQuantity, *_txtSpaceUsed;
	ComboBox *_cbxCategory;
	TextList *_lstItems;
	std::vector<TransferRow> _items;
	std::vector<int> _rows;
	std::vector<std::string> _cats;
	size_t _vanillaCategories;
	size_t _sel;
	int _total, _pQty, _cQty;
	double _iQty;
	std::map<int, int> _iPrisonQty;
	Uint8 _ammoColor;
	Timer *_timerInc, *_timerDec;
	/// Gets the category of the current selection.
	std::string getCategory(int sel) const;
	/// Determines if the current selection belongs to a given category.
	bool belongsToCategory(int sel, const std::string &cat) const;
	/// Checks for hidden items
	bool isHidden(int sel) const;
	/// Checks for missing items
	int getMissingQty(int sel) const;
	/// Gets the row of the current selection.
	TransferRow &getRow() { return _items[_rows[_sel]]; }
public:
	/// Creates the Purchase state.
	PurchaseState(Base *base, CannotReequipState *parent = nullptr);
	/// Cleans up the Purchase state.
	~PurchaseState();
	/// Runs the timers.
	void think() override;
	/// Updates the item list.
	void updateList();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action *action);
	void btnQuickSearchApply(Action *action);
	/// Handler for pressing an Increase arrow in the list.
	void lstItemsLeftArrowPress(Action *action);
	/// Handler for releasing an Increase arrow in the list.
	void lstItemsLeftArrowRelease(Action *action);
	/// Handler for clicking an Increase arrow in the list.
	void lstItemsLeftArrowClick(Action *action);
	/// Handler for pressing a Decrease arrow in the list.
	void lstItemsRightArrowPress(Action *action);
	/// Handler for releasing a Decrease arrow in the list.
	void lstItemsRightArrowRelease(Action *action);
	/// Handler for clicking a Decrease arrow in the list.
	void lstItemsRightArrowClick(Action *action);
	/// Handler for pressing-down a mouse-button in the list.
	void lstItemsMousePress(Action *action);
	/// Increases the quantity of an item by one.
	void increase();
	/// Increases the quantity of an item by the given value.
	void increaseByValue(int change);
	/// Decreases the quantity of an item by one.
	void decrease();
	/// Decreases the quantity of an item by the given value.
	void decreaseByValue(int change);
	/// Updates the quantity-strings of the selected item.
	void updateItemStrings();
	/// Handler for changing the category filter.
	void cbxCategoryChange(Action *action);
};

}
