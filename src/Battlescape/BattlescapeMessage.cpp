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
#include "BattlescapeMessage.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/ProgressBar.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

namespace OpenXcom
{

const int BattlescapeMessage::HORIZONTAL_OFFSET = 200;
const int BattlescapeMessage::VERTICAL_OFFSET = 20;

/**
 * Sets up a blank Battlescape message with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
BattlescapeMessage::BattlescapeMessage(int width, int height, int x, int y) : Surface(width, height, x, y)
{
	_window = new Window(0, width, height, x, y, POPUP_NONE);
	_window->setColor(Palette::blockOffset(0)-1);
	_window->setHighContrast(true);

	_text = new Text(width, height, x, y);
	_text->setColor(Palette::blockOffset(0)-1);
	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
	_text->setHighContrast(true);

	_txtThinking = new Text(102, 9, HORIZONTAL_OFFSET, VERTICAL_OFFSET - 10);
	_txtThinking->setColor(Palette::blockOffset(0) - 1);
	_txtThinking->setAlign(ALIGN_CENTER);
	_txtThinking->setHighContrast(true);

	_progressBar = new ProgressBar(102, 5, HORIZONTAL_OFFSET, VERTICAL_OFFSET);

	if (Options::oxceDisableThinkingProgressBar)
	{
		_txtThinking->setVisible(false);
		_progressBar->setVisible(false);
	}
}

/**
 * Deletes surfaces.
 */
BattlescapeMessage::~BattlescapeMessage()
{
	delete _window;
	delete _text;
	delete _txtThinking;
	delete _progressBar;
}

/**
 * Changes the position of the surface in the X axis.
 * @param x X position in pixels.
 */
void BattlescapeMessage::setX(int x)
{
	Surface::setX(x);
	_window->setX(x);
	_text->setX(x);
	_txtThinking->setX(x + HORIZONTAL_OFFSET);
	_progressBar->setX(x + HORIZONTAL_OFFSET);
}

/**
 * Changes the position of the surface in the Y axis.
 * @param y Y position in pixels.
 */
void BattlescapeMessage::setY(int y)
{
	Surface::setY(y);
	_window->setY(y);
	_text->setY(y);
	_txtThinking->setY(y + VERTICAL_OFFSET - 10);
	_progressBar->setY(y + VERTICAL_OFFSET);
}

/**
 * Changes the message background.
 * @param background Pointer to background surface.
 */
void BattlescapeMessage::setBackground(Surface *background)
{
	_window->setBackground(background);
}

/**
 * Changes the message text.
 * @param message Message string.
 */
void BattlescapeMessage::setText(const std::string &message, const std::string& message2)
{
	_text->setText(message);
	_txtThinking->setText(message2);
}

/**
 * Changes the progress bar value.
 * @param message New value.
 */
void BattlescapeMessage::setProgressValue(int progress)
{
	_progressBar->setValue(progress);
}

/**
 * Changes the various resources needed for text rendering.
 * The different fonts need to be passed in advance since the
 * text size can change mid-text, and the language affects
 * how the text is rendered.
 * @param big Pointer to large-size font.
 * @param small Pointer to small-size font.
 * @param lang Pointer to current language.
 */
void BattlescapeMessage::initText(Font *big, Font *small, Language *lang)
{
	_text->initText(big, small, lang);
	_text->setBig();
	_txtThinking->initText(big, small, lang);
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void BattlescapeMessage::setPalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_window->setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
	_txtThinking->setPalette(colors, firstcolor, ncolors);
	_progressBar->setPalette(colors, firstcolor, ncolors);
}

/**
 * Blits the warning message.
 */
void BattlescapeMessage::blit(SDL_Surface *surface)
{
	Surface::blit(surface);
	_window->blit(surface);
	_text->blit(surface);
	_txtThinking->blit(surface);
	_progressBar->blit(surface);
}

/*
 * Special handling for setting the height of the battlescape message.
 * @param height the new height.
 */
void BattlescapeMessage::setHeight(int height)
{
	Surface::setHeight(height);
	_window->setHeight(height);
	_text->setHeight(height);
}

/*
 * Sets the text color of the battlescape message.
 * @param color the new color.
 */
void BattlescapeMessage::setTextColor(Uint8 color)
{
	_text->setColor(color);
	_txtThinking->setColor(color);
}

/*
 * Sets the colors of the progress bar.
 * @param color the new color.
 * @param borderColor the new border color.
 */
void BattlescapeMessage::setProgressBarColor(Uint8 color, Uint8 borderColor)
{
	_progressBar->setColor(color);
	_progressBar->setBorderColor(borderColor);
}

}
