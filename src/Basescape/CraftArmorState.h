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
#include "SoldierSortUtil.h"

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class ComboBox;
class Base;
class Soldier;
struct SortFunctor;

/**
 * Select Armor screen that lets the player
 * pick armor for the soldiers on the craft.
 */
class CraftArmorState : public TouchState
{
private:
	TextButton *_btnOk;
	Window *_window;
	Text *_txtTitle, *_txtName, *_txtCraft, *_txtArmor;
	ComboBox *_cbxSortBy;
	TextList *_lstSoldiers;

	Base *_base;
	size_t _craft, _savedScrollPosition;
	std::vector<Soldier *> _origSoldierOrder;
	std::vector<SortFunctor *> _sortFunctors;
	getStatFn_t _dynGetter;
	///initializes the display list based on the craft soldier's list and the position to display
	void initList(size_t scrl);
public:
	/// Creates the Craft Armor state.
	CraftArmorState(Base *base, size_t craft);
	/// Cleans up the Craft Armor state.
	~CraftArmorState();
	/// Handler for changing the sort by combobox.
	void cbxSortByChange(Action *action);
	/// Updates the soldier armors.
	void init() override;
	/// Handler for clicking the Soldiers reordering button.
	void lstItemsLeftArrowClick(Action *action);
	/// Moves a soldier up.
	void moveSoldierUp(Action *action, unsigned int row, bool max = false);
	/// Handler for clicking the Soldiers reordering button.
	void lstItemsRightArrowClick(Action *action);
	/// Moves a soldier down.
	void moveSoldierDown(Action *action, unsigned int row, bool max = false);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Soldiers list.
	void lstSoldiersClick(Action *action);
	/// Handler for pressing-down a mouse-button in the list.
	void lstSoldiersMousePress(Action *action);
	/// Handler for clicking the De-equip All Armor button.
	void btnDeequipAllArmorClick(Action *action);
	void btnDeequipCraftArmorClick(Action *action);
};

}
