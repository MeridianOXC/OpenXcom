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
#include "../Menu/OptionsBaseState.h"
#include <vector>
#include <string>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class Timer;
class Base;

/**
 * ManageAlienContainment screen that lets the player manage
 * alien numbers in a particular base.
 */
class ManageAlienContainmentState : public State
{
private:
	Base *_base;
	int _prisonType;
	OptionsOrigin _origin;
	TextButton *_btnOk, *_btnSell, *_btnCancel, *_btnTransfer, *_btnCleanup;
	Window *_window;
	Text *_txtTitle, *_txtUsed, *_txtAvailable, *_txtValueOfSales, *_txtItem, *_txtLiveAliens, *_txtDeadAliens, *_txtInterrogatedAliens;
	TextList *_lstAliens;
	Timer *_timerInc, *_timerDec;
	std::vector<int> _qtys;
	std::vector<std::string> _aliens;
	size_t _sel;
	int _aliensSold;
	int64_t _total;
	bool _doNotReset, _threeButtons;

	/// Gets selected quantity.
	int getQuantity();
	/// Deals with the selected aliens.
	void dealWithSelectedAliens(bool sell);
public:
	/// Creates the ManageAlienContainment state.
	ManageAlienContainmentState(Base *base, int prisonType, OptionsOrigin origin);
	/// Cleans up the ManageAlienContainment state.
	~ManageAlienContainmentState();
	/// Resets state.
	void init() override;
	/// Resets the list and the totals, updates button visibility.
	void resetListAndTotals();
	/// Runs the timers.
	void think() override;
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for opening the Global Alien Containment UI.
	void onGlobalAlienContainmentClick(Action *action);
	/// Handler for clicking the Sell button.
	void btnSellClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for clicking the Transfer button.
	void btnTransferClick(Action *action);
	/// Handler for clicking the Cleanup button.
	void btnCleanupClick(Action *action);
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
	/// Handler for mousewheeling over the list.
	void lstItemsMouseWheel(Action *action);
	/// Increases the quantity of an alien by one.
	void increase();
	/// Increases the quantity of an alien by the given value.
	void increaseByValue(int change);
	/// Decreases the quantity of an alien by one.
	void decrease();
	/// Decreases the quantity of an alien by the given value.
	void decreaseByValue(int change);
	/// Updates the quantity-strings of the selected alien.
	void updateStrings();
};

}
