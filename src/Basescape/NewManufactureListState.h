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
#include "../Engine/TouchState.h"
#include <vector>

namespace OpenXcom
{

class Base;
class TextButton;
class ToggleTextButton;
class Window;
class Text;
class TextEdit;
class TextList;
class RuleManufacture;
class ComboBox;

/**
 * Screen which list possible productions.
 */
class NewManufactureListState : public TouchState
{
private:
	Base *_base;
	bool _showRequirements, _refreshCategories, _doInit;
	TextButton *_btnOk;
	ToggleTextButton *_btnShowOnlyNew;
	TextEdit *_btnQuickSearch;
	Window *_window;
	Text *_txtTitle, *_txtItem, *_txtCategory;
	TextList *_lstManufacture;
	size_t _lstScroll;
	ComboBox *_cbxCategory;
	ComboBox *_cbxFilter;
	std::vector<RuleManufacture *> _possibleProductions;
	std::vector<std::string> _catStrings;
	std::vector<std::string> _displayedStrings;
	Uint8 _colorNormal, _colorNew;
	Uint8 _colorHidden, _colorFacilityRequired;

public:
	/// Creates the state.
	NewManufactureListState(Base *base);
	/// Initializes state.
	void init() override;
	/// Handler for clicking the OK button.
	void btnOkClick(Action * action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action *action);
	void btnQuickSearchApply(Action *action);
	/// Handlers for clicking on the list.
	void lstProdClick(Action* action);
	void lstProdClickLeft (Action * action);
	void lstProdClickRight(Action * action);
	void lstProdClickMiddle(Action * action);
	/// Handler for changing the category filter
	void cbxCategoryChange (Action * action);
	/// Handler for changing the basic filter
	void cbxFilterChange(Action * action);
	/// Handler for clicking the [Show Only New] button.
	void btnShowOnlyNewClick(Action *action);
	/// Handler for clicking the [Mark All As Seen] button.
	void btnMarkAllAsSeenClick(Action *action);
	/// Fills the list of possible productions.
	void fillProductionList(bool refreshCategories);
};

}
