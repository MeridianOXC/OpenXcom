/*
 * Copyright 2010-2024 OpenXcom Developers.
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
#include "ProgressBar.h"
#include <SDL.h>

namespace OpenXcom
{

/**
 * Sets up a blank ProgressBar with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
ProgressBar::ProgressBar(int width, int height, int x, int y) : Surface(width, height, x, y), _color(32), _borderColor(1), _value(0)
{

}

/**
 *
 */
ProgressBar::~ProgressBar()
{
}

/**
 * Changes the color used to draw the border and contents.
 * @param color Color value.
 */
void ProgressBar::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Returns the color used to draw the ProgressBar.
 * @return Color value.
 */
Uint8 ProgressBar::getColor() const
{
	return _color;
}

/**
 * Sets the border color for the ProgressBar.
 * @param bc the color for the outline of the ProgressBar.
 * @note will use base colour + 4 if none is defined here.
 */
void ProgressBar::setBorderColor(Uint8 bc)
{
	_borderColor = bc;
}

/**
 * Changes the value used to draw the inner contents.
 * @param value Current value.
 */
void ProgressBar::setValue(int value)
{
	_value = (value < 0) ? 0 : value;
	_redraw = true;
}

/**
 * Returns the value used to draw the inner contents.
 * @return Current value.
 */
int ProgressBar::getValue() const
{
	return _value;
}

/**
 * Draws the bordered ProgressBar filled according
 * to its value.
 */
void ProgressBar::draw()
{
	Surface::draw();

	SDL_Rect square;

	// border
	square.x = 0;
	square.y = 0;
	square.w = getWidth();
	square.h = getHeight();

	if (_borderColor)
		drawRect(&square, _borderColor);
	else
		drawRect(&square, _color + 4);

	// transparent inner content
	square.x++;
	square.y++;
	square.w -= 2;
	square.h -= 2;

	drawRect(&square, 0);

	// non-transparent inner content
	square.w = square.w * _value / 100;

	drawRect(&square, _color);
}

}
