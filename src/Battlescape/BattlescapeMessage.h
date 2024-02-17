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
#include <string>
#include "../Engine/Surface.h"

namespace OpenXcom
{

class Window;
class Text;
class Font;
class ProgressBar;

/**
 * Generic window used to display messages
 * over the Battlescape map.
 */
class BattlescapeMessage : public Surface
{
private:
	static const int HORIZONTAL_OFFSET;
	static const int VERTICAL_OFFSET;

	Window *_window;
	Text *_text;
	Text *_txtThinking;
	ProgressBar *_progressBar;
public:
	/// Creates a new Battlescape message with the specified size and position.
	BattlescapeMessage(int width, int height, int x = 0, int y = 0);
	/// Cleans up the Battlescape message.
	~BattlescapeMessage();
	/// Sets the X position of the surface.
	void setX(int x) override;
	/// Sets the Y position of the surface.
	void setY(int y) override;
	/// Sets the Battlescape message's background.
	void setBackground(Surface *background);
	/// Sets the Battlescape message's text.
	void setText(const std::string &message, const std::string& message2);
	/// Sets the progress bar value.
	void setProgressValue(int progress);
	/// Initializes the Battlescape message's resources.
	void initText(Font *big, Font *small, Language *lang) override;
	/// Sets the Battlescape message's palette.
	void setPalette(const SDL_Color *colors, int firstcolor = 0, int ncolors = 256) override;
	/// Blits the warning message.
	void blit(SDL_Surface *surface) override;
	/// Special handling for setting the height of the battlescape message.
	void setHeight(int height) override;
	/// Sets the text color of the battlescape message.
	void setTextColor(Uint8 color);
	/// Sets the colors of the progress bar.
	void setProgressBarColor(Uint8 color, Uint8 borderColor);
};

}
