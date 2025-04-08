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
class Timer;

/**
 * Select Squad screen that lets the player
 * pick the soldiers to assign to a craft.
 */
class CraftSoldiersState : public TouchState
{
private:
	TextButton *_btnOk;
	TextButton *_btnPreview;
	Window *_window;
	Text *_txtTitle, *_txtName, *_txtRank, *_txtCraft, *_txtAvailable, *_txtUsed;
	ComboBox *_cbxSortBy;
	TextList *_lstSoldiers;

	Base *_base;
	size_t _craft;
	Uint8 _otherCraftColor;
	std::vector<Soldier *> _origSoldierOrder;
	std::vector<SortFunctor *> _sortFunctors;
	getStatFn_t _dynGetter;
	/// initializes the display list based on the craft soldier's list and the position to display
	void initList(size_t scrl);
	/// Stores previously selected soldier position
	int _pselSoldier;
	/// (De)selection guard
	bool _wasDragging;
#ifdef __MOBILE__
	/// Timer for handling long presses as right clicks
	Timer *_longPressTimer;
	/// Click guard for state transitions
	bool _clickGuard;
#endif
public:
	/// Creates the Craft Soldiers state.
	CraftSoldiersState(Base *base, size_t craft);
	/// Cleans up the Craft Soldiers state.
	~CraftSoldiersState();
	/// Handler for changing the sort by combobox.
	void cbxSortByChange(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Preview button.
	void btnPreviewClick(Action *action);
	/// Updates the soldiers list.
	void init() override;
	/// Handler for clicking the Soldiers reordering button.
	void lstItemsLeftArrowClick(Action *action);
	/// Moves a soldier up.
	void moveSoldierUp(Action *action, unsigned int row, bool max = false);
	/// Handler for clicking the Soldiers reordering button.
	void lstItemsRightArrowClick(Action *action);
	/// Moves a soldier down.
	void moveSoldierDown(Action *action, unsigned int row, bool max = false);
	/// Handler for clicking the Soldiers list.
	void lstSoldiersClick(Action *action);
	/// Handler for pressing-down a mouse-button in the list.
	void lstSoldiersMousePress(Action *action);
	/// Handler for mousewheel action.
	void lstSoldiersMouseWheel(Action *action);
	/// Handler for mouseover (drag-drop) action
	void lstSoldiersMouseOver(Action *action);
#ifdef __MOBILE__
	/// Pokes the timer
	void think() override;
	/// Handler for mouse releases
	void lstSoldiersMouseRelease(Action *action);
	/// Handler for long presses
	void lstSoldiersLongPress();
#endif
	/// Handler for clicking the De-assign All Soldiers button.
	void btnDeassignAllSoldiersClick(Action *action);
	void btnDeassignCraftSoldiersClick(Action *action);
};

}
