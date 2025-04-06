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
#include "State.h"

namespace OpenXcom
{

class Text;
class TextButton;
class ToggleTextButton;

/**
 * A State with touch buttons support.
 */

class TouchState : public State
{
protected:
	bool _hideGroup100;
	Text *_txtTitlePtr;
	TextButton *_btnTouch;
	TextButton *_btn1, *_btn10, *_btn100, *_owner100;
	TextButton *_btnLMB, *_btnRMB, *_btnMMB, *_ownerLRM;
	ToggleTextButton *_btnCtrl, *_btnAlt, *_btnShift;
public:
	/// Creates a new state.
	TouchState() = default;
	void touchComponentsCreate(Text* txtTitlePtr, bool hideGroup100 = false, int horizontalOffset = 0, int verticalOffset = 0);
	void touchComponentsAdd(const std::string& id, const std::string& category, Surface* parent);
	void touchComponentsConfigure();
	/// Cleans up the state.
	virtual ~TouchState() = default;

	void touchComponentsRefresh();

	/// Handler for clicking the Touch button.
	void btnTouchClick(Action* action);
	/// Handler for clicking 1-10-100 group buttons.
	void btnGroup100Press(Action* action);
	/// Handler for clicking LMB-RMB-MMB group buttons.
	void btnGroupLRMPress(Action* action);
	/// Handler for clicking the CTRL button.
	void btnCtrlClick(Action* action);
	/// Handler for clicking the ALT button.
	void btnAltClick(Action* action);
	/// Handler for clicking the SHIFT button.
	void btnShiftClick(Action* action);
};

}
