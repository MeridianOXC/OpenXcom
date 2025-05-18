#pragma once
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
#include "../Engine/State.h"

namespace OpenXcom
{

class Window;
class Text;
class TextButton;
class TextList;
class RuleItem;

/**
 * Window which displays item locations (i.e. which base has how much).
 */
class ItemLocationsState : public State
{
private:
	Window *_window;
	Text *_txtTitle;
	Text *_txtBase, *_txtQuantity;
	TextList *_lstLocations;
	TextButton *_btnOk;
public:
	/// Creates the ItemLocationsState state.
	ItemLocationsState(const RuleItem* selectedItem);
	/// Cleans up the ItemLocationsState state
	~ItemLocationsState() = default;
	/// Handler for clicking the OK button.
	void btnOkClick(Action* action);
};
}
