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

#include "TouchState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"

namespace OpenXcom
{

void TouchState::touchComponentsCreate(Text* txtTitlePtr, bool hideGroup100, int horizontalOffset, int verticalOffset)
{
	// Reset touch flags
	_game->resetTouchButtonFlags();

	if (!Options::oxceBaseTouchButtons)
		return;

	_hideGroup100 = hideGroup100;
	_txtTitlePtr = txtTitlePtr;

	_btnTouch = new TextButton(40, 16, horizontalOffset + 273, 7 + verticalOffset);

	_btn1     = new       TextButton(25, 16, horizontalOffset + 61 + 0 * 26, 7 + verticalOffset);
	_btn10    = new       TextButton(25, 16, horizontalOffset + 61 + 1 * 26, 7 + verticalOffset);
	_btn100   = new       TextButton(25, 16, horizontalOffset + 61 + 2 * 26, 7 + verticalOffset);
	_owner100 = _btn1;

	_btnLMB   = new       TextButton(25, 16, horizontalOffset + 65 + 3 * 26, 7 + verticalOffset);
	_btnRMB   = new       TextButton(25, 16, horizontalOffset + 65 + 4 * 26, 7 + verticalOffset);
	_btnMMB   = new       TextButton(25, 16, horizontalOffset + 65 + 5 * 26, 7 + verticalOffset);
	_ownerLRM = _btnLMB;

	_btnCtrl  = new ToggleTextButton(27, 16, horizontalOffset + 69 + 6 * 26, 7 + verticalOffset);
	_btnAlt   = new ToggleTextButton(27, 16, horizontalOffset + 71 + 7 * 26, 7 + verticalOffset);
	_btnShift = new ToggleTextButton(32, 16, horizontalOffset + 73 + 8 * 26, 7 + verticalOffset);
}

void TouchState::touchComponentsAdd(const std::string& id, const std::string& category, Surface* parent)
{
	if (!Options::oxceBaseTouchButtons)
		return;

	add(_btnTouch, "touchButton", category, parent);

	add(_btn1, id, category);
	add(_btn10, id, category);
	add(_btn100, id, category);

	add(_btnLMB, id, category);
	add(_btnRMB, id, category);
	add(_btnMMB, id, category);

	add(_btnCtrl, id, category);
	add(_btnAlt, id, category);
	add(_btnShift, id, category);
}

void TouchState::touchComponentsConfigure()
{
	if (!Options::oxceBaseTouchButtons)
		return;

	_btn1->setText(tr("STR_BUTTON_1"));
	_btn10->setText(tr("STR_BUTTON_10"));
	_btn100->setText(tr("STR_BUTTON_100"));
	_btnLMB->setText(tr("STR_BUTTON_LMB"));
	_btnRMB->setText(tr("STR_BUTTON_RMB"));
	_btnMMB->setText(tr("STR_BUTTON_MMB"));
	_btnCtrl->setText(tr("STR_BUTTON_CTRL"));
	_btnAlt->setText(tr("STR_BUTTON_ALT"));
	_btnShift->setText(tr("STR_BUTTON_SHIFT"));

	_btn1->setGroup(&_owner100);
	_btn10->setGroup(&_owner100);
	_btn100->setGroup(&_owner100);
	_btnLMB->setGroup(&_ownerLRM);
	_btnRMB->setGroup(&_ownerLRM);
	_btnMMB->setGroup(&_ownerLRM);

	_btn1->onMousePress((ActionHandler)&TouchState::btnGroup100Press);
	_btn10->onMousePress((ActionHandler)&TouchState::btnGroup100Press);
	_btn100->onMousePress((ActionHandler)&TouchState::btnGroup100Press);
	_btnLMB->onMousePress((ActionHandler)&TouchState::btnGroupLRMPress);
	_btnRMB->onMousePress((ActionHandler)&TouchState::btnGroupLRMPress);
	_btnMMB->onMousePress((ActionHandler)&TouchState::btnGroupLRMPress);

	_btn1->setVisible(false);
	_btn10->setVisible(false);
	_btn100->setVisible(false);
	_btnLMB->setVisible(false);
	_btnRMB->setVisible(false);
	_btnMMB->setVisible(false);
	_btnCtrl->setVisible(false);
	_btnAlt->setVisible(false);
	_btnShift->setVisible(false);

	_btnTouch->setText(tr("STR_BUTTON_TOUCH"));
	_btnTouch->setVisible(Options::oxceBaseTouchButtons);
	_btnTouch->onMouseClick((ActionHandler)&TouchState::btnTouchClick);

	_btnCtrl->onMouseClick((ActionHandler)&TouchState::btnCtrlClick);
	_btnAlt->onMouseClick((ActionHandler)&TouchState::btnAltClick);
	_btnShift->onMouseClick((ActionHandler)&TouchState::btnShiftClick);
}

void TouchState::touchComponentsRefresh()
{
	if (!Options::oxceBaseTouchButtons)
		return;

	_owner100 = _game->getScrollStep() == 100 ? _btn100 : (_game->getScrollStep() == 10 ? _btn10 : _btn1);

	_ownerLRM = _game->getMMBFlag() ? _btnMMB : (_game->getRMBFlag() ? _btnRMB : _btnLMB);

	_btnCtrl->setPressed(_game->getCtrlPressedFlag());
	_btnAlt->setPressed(_game->getAltPressedFlag());
	_btnShift->setPressed(_game->getShiftPressedFlag());
}

void TouchState::btnTouchClick(Action* action)
{
	if (_txtTitlePtr)
	{
		_txtTitlePtr->setVisible(false);
	}
	_btnTouch->setVisible(false);

	if (!_hideGroup100)
	{
		_btn1->setVisible(true);
		_btn10->setVisible(true);
		_btn100->setVisible(true);
	}
	_btnCtrl->setVisible(true);
	_btnAlt->setVisible(true);
	_btnShift->setVisible(true);
	_btnLMB->setVisible(true);
	_btnRMB->setVisible(true);
	_btnMMB->setVisible(true);
}

void TouchState::btnGroup100Press(Action*)
{
	int step = (_owner100 == _btn100 ? 100 : (_owner100 == _btn10 ? 10 : 1));
	_game->setScrollStep(step);
}

void TouchState::btnGroupLRMPress(Action*)
{
	_game->setRMBFlag(_ownerLRM == _btnRMB);
	_game->setMMBFlag(_ownerLRM == _btnMMB);
}

void TouchState::btnCtrlClick(Action*)
{
	_game->setCtrlPressedFlag(_btnCtrl->getPressed());
}

void TouchState::btnAltClick(Action*)
{
	_game->setAltPressedFlag(_btnAlt->getPressed());
}

void TouchState::btnShiftClick(Action*)
{
	_game->setShiftPressedFlag(_btnShift->getPressed());
}

}
