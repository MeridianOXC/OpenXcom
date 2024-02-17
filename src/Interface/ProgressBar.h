#pragma once
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
#include "../Engine/Surface.h"

namespace OpenXcom
{

/**
 * Progress bar graphic that represents a certain percentage value.
 * Drawn with a coloured border and partially filled inner content.
 */
class ProgressBar : public Surface
{
private:
	Uint8 _color, _borderColor;
	int _value;
public:
	/// Creates a new progress bar with the specified size and position.
	ProgressBar(int width, int height, int x = 0, int y = 0);
	/// Cleans up the progress bar.
	~ProgressBar();
	/// Sets the progress bar's color.
	void setColor(Uint8 color) override;
	/// Gets the progress bar's color.
	Uint8 getColor() const;
	/// Sets the outline color for the progress bar.
	void setBorderColor(Uint8 bc) override;
	/// Sets the progress bar's current value.
	void setValue(int value);
	/// Gets the progress bar's current value.
	int getValue() const;
	/// Draws the progress bar.
	void draw() override;
};

}
