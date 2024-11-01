#pragma once
/*
 * Copyright 2010-2015 OpenXcom Developers.
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

class TextButton;
class ToggleTextButton;
class Window;
class Text;
class Base;
class TextList;
class ComboBox;
class Soldier;
struct SortFunctor;

/**
 * Screen shown monthly to allow changing
 * soldiers currently in psi training.
 */
class AllocateTrainingState : public State
{
private:
	TextButton *_btnOk;
	ToggleTextButton *_btnPlus;
	Window *_window;
	Text *_txtTitle, *_txtTraining, *_txtName, *_txtRemaining;
	Text *_txtTu, *_txtStamina, *_txtHealth, *_txtFiring, *_txtThrowing, *_txtMelee, *_txtStrength;
	ComboBox *_cbxSortBy;
	TextList *_lstSoldiers;
	std::vector<Soldier*> _soldiers;
	size_t _sel;
	int _space;
	Base *_base;
	std::vector<Soldier *> _origSoldierOrder;
	std::vector<SortFunctor *> _sortFunctors;
	std::vector<SortFunctor *> _sortFunctorsPlus;
	bool _doNotReset;

	///initializes the display list
	void initList(size_t scrl);
public:
	/// Creates the Psi Training state.
	AllocateTrainingState(Base *base);
	/// Cleans up the Psi Training state.
	~AllocateTrainingState();
	/// Handler for changing the sort by combobox.
	void cbxSortByChange(Action *action);
	/// Updates the soldier info.
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
	/// Handler for clicking the PLUS button.
	void btnPlusClick(Action *action);
	/// Handler for clicking the Soldiers list.
	void lstSoldiersClick(Action *action);
	/// Handler for pressing-down a mouse-button in the list.
	void lstSoldiersMousePress(Action *action);
	/// Handler for clicking the De-assign All Soldiers button.
	void btnDeassignAllSoldiersClick(Action* action);
	/// Handler for clicking the Assign All Soldiers button.
	void btnAssignAllSoldiersClick(Action* action);
};

}
