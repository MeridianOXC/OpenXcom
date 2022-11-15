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

#include "UfopaediaStartState.h"
#include "UfopaediaSelectState.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ArrowButton.h"
#include "../Mod/Mod.h"
#include "../Engine/Timer.h"
#include "../fmath.h"
#include "../Engine/Screen.h"

namespace OpenXcom
{
	UfopaediaStartState::UfopaediaStartState() : _offset(0), _scroll(0), _maxButtons(0), _heightOffset(0), _windowOffset(0), _cats(_game->getMod()->getUfopaediaCategoryList())
	{
		const int MAX_VANILLA_BUTTONS = 10;
		const int SPACE_PER_BUTTON = 13;
		const int HALF_SPACE_PER_BUTTON = 7;

		_screen = false;

		int extraSpace = (_game->getScreen()->getDY() * 2) + 20; // upper extra + lower extra + 20 pixels from the original
		int maxButtons = (extraSpace / SPACE_PER_BUTTON) + MAX_VANILLA_BUTTONS;
		_maxButtons = std::min(_cats.size(), (size_t)maxButtons);
		maxButtons = (int)_maxButtons;

		int buttonOffset = 0;
		if (maxButtons >= MAX_VANILLA_BUTTONS)
		{
			_heightOffset = (maxButtons - MAX_VANILLA_BUTTONS) * SPACE_PER_BUTTON;
			_windowOffset = ((maxButtons - MAX_VANILLA_BUTTONS) / 2 * SPACE_PER_BUTTON) + ((maxButtons - MAX_VANILLA_BUTTONS) % 2 * HALF_SPACE_PER_BUTTON);
			buttonOffset = _windowOffset + SPACE_PER_BUTTON;
		}

		// set background window
		_window = new Window(this, 256, 180 + _heightOffset, 32, 10 - _windowOffset, POPUP_BOTH);
		_window->setInnerColor(239); // almost black = darkest index from backpals.dat

		// set title
		_txtTitle = new Text(220, 17, 50, 33);

		// Set palette
		setInterface("ufopaedia");

		add(_window, "window", "ufopaedia");
		add(_txtTitle, "text", "ufopaedia");

		// set buttons
		int y = 50 - buttonOffset;

		_btnScrollUp = new ArrowButton(ARROW_BIG_UP, 13, 14, 270, y);
		add(_btnScrollUp, "button1", "ufopaedia");

		for (size_t i = 0; i < _maxButtons; ++i)
		{
			TextButton *button = new TextButton(220, 12, 50, y);
			y += 13;

			add(button, "button1", "ufopaedia");

			button->onMouseClick((ActionHandler)&UfopaediaStartState::btnSectionClick);
			button->onMousePress((ActionHandler)&UfopaediaStartState::btnScrollUpClick, SDL_BUTTON_WHEELUP);
			button->onMousePress((ActionHandler)&UfopaediaStartState::btnScrollDownClick, SDL_BUTTON_WHEELDOWN);

			_btnSections.push_back(button);
		}

		_btnOk = new TextButton(220, 12, 50, y);
		add(_btnOk, "button1", "ufopaedia");

		_btnScrollDown = new ArrowButton(ARROW_BIG_DOWN, 13, 14, 270, y - 15);
		add(_btnScrollDown, "button1", "ufopaedia");

		updateButtons();
		if (!_btnSections.empty())
		{
			int titleY = _btnSections.front()->getY() - _txtTitle->getHeight();
			if (titleY < _window->getY()) titleY = _window->getY();
			_txtTitle->setY(titleY);
		}

		centerAllSurfaces();

		setWindowBackground(_window, "ufopaedia");

		_txtTitle->setBig();
		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setText(tr("STR_UFOPAEDIA"));

		_btnOk->setText(tr("STR_OK"));
		_btnOk->onMouseClick((ActionHandler)&UfopaediaStartState::btnOkClick);
		_btnOk->onKeyboardPress((ActionHandler)&UfopaediaStartState::btnOkClick, Options::keyCancel);
		_btnOk->onKeyboardPress((ActionHandler)&UfopaediaStartState::btnOkClick, Options::keyGeoUfopedia);

		_btnScrollUp->setVisible(_cats.size() > _maxButtons);
		_btnScrollUp->onMousePress((ActionHandler)&UfopaediaStartState::btnScrollUpPress);
		_btnScrollUp->onMouseRelease((ActionHandler)&UfopaediaStartState::btnScrollRelease);
		_btnScrollDown->setVisible(_cats.size() > _maxButtons);
		_btnScrollDown->onMousePress((ActionHandler)&UfopaediaStartState::btnScrollDownPress);
		_btnScrollDown->onMouseRelease((ActionHandler)&UfopaediaStartState::btnScrollRelease);

		_timerScroll = new Timer(50);
		_timerScroll->onTimer((StateHandler)&UfopaediaStartState::scroll);
	}

	/**
	 * Deletes timers.
	 */
	UfopaediaStartState::~UfopaediaStartState()
	{
		delete _timerScroll;
	}

	/**
	 * Run timers.
	 */
	void UfopaediaStartState::think()
	{
		State::think();
		_timerScroll->think(this, 0);
	}

	/**
	 * Returns to the previous screen.
	 * @param action Pointer to an action.
	 */
	void UfopaediaStartState::btnOkClick(Action *)
	{
		_game->popState();
	}

	/**
	 * Displays the list of articles for this section.
	 * @param action Pointer to an action.
	 */
	void UfopaediaStartState::btnSectionClick(Action *action)
	{
		for (size_t i = 0; i < _btnSections.size(); ++i)
		{
			if (action->getSender() == _btnSections[i])
			{
				_game->pushState(new UfopaediaSelectState(_cats[_offset + i], _heightOffset, _windowOffset));
				break;
			}
		}
	}

	/**
	 * Starts scrolling the section buttons up.
	 * @param action Pointer to an action.
	 */
	void UfopaediaStartState::btnScrollUpPress(Action *)
	{
		_scroll = -1;
		_timerScroll->start();
	}

	/**
	 * Scrolls the section buttons up.
	 * @param action Pointer to an action.
	 */
	void UfopaediaStartState::btnScrollUpClick(Action *)
	{
		_scroll = -1;
		scroll();
	}

	/**
	 * Starts scrolling the section buttons down.
	 * @param action Pointer to an action.
	 */
	void UfopaediaStartState::btnScrollDownPress(Action *)
	{
		_scroll = 1;
		_timerScroll->start();
	}

	/**
	 * Scrolls the section buttons down.
	 * @param action Pointer to an action.
	 */
	void UfopaediaStartState::btnScrollDownClick(Action *)
	{
		_scroll = 1;
		scroll();
	}

	/**
	 * Stops scrolling the section buttons.
	 * @param action Pointer to an action.
	 */
	void UfopaediaStartState::btnScrollRelease(Action *)
	{
		_timerScroll->stop();
	}

	/**
	 * Offsets the list of section buttons.
	 */
	void UfopaediaStartState::scroll()
	{
		if (_cats.size() > _maxButtons)
		{
			_offset = Clamp(_offset + _scroll, 0, int(_cats.size() - _maxButtons));
			updateButtons();
		}
	}

	/**
	 * Updates the section button labels based on scroll.
	 */
	void UfopaediaStartState::updateButtons()
	{
		for (size_t i = 0; i < _btnSections.size(); ++i)
		{
			_btnSections[i]->setText(tr(_cats[_offset + i]));
		}
	}
}
