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
#include "TestPaletteState.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the test palette screen.
 */
TestPaletteState::TestPaletteState(const std::string &palette, PaletteActionType action)
{
	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_btnCancel = new TextButton(60, 9, 240, 190);

	// Set palette
	setStandardPalette(palette);
	int maxColors = _game->getMod()->getPalette(palette)->getColorCount();

	if (_game->isCtrlPressed())
	{
		// export the palette
		const std::string exportName = palette + ".jasc.pal";
		_game->getMod()->getPalette(palette)->savePalJasc(exportName);
	}

	add(_bg);
	add(_btnCancel);

	centerAllSurfaces();

	_btnCancel->onMouseClick((ActionHandler)&TestPaletteState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&TestPaletteState::btnCancelClick, Options::keyCancel);

	// basic palette
	if (action == PAT_PREVIEW)
	{
		Surface surf = Surface(20, 11, 0, 0);
		surf.setPalette(_bg->getPalette());
		for (int row = 0; row < 16; ++row)
		{
			for (int column = 0; column < 16; ++column)
			{
				int index = row * 16 + column;
				if (index >= maxColors) return;
				surf.drawRect(0, 0, 20, 11, index);
				surf.blitNShade(_bg, column * 20, row * 11);
			}
		}
		return;
	}

	// small digits without/with border
	if (action == PAT_TINY_BORDERLESS || action == PAT_TINY_BORDER)
	{
		NumberText text = NumberText(25, 9, 0, 0);
		text.setPalette(_bg->getPalette());
		text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
		text.setBordered((action == PAT_TINY_BORDER));
		for (int row = 0; row < 22; ++row)
		{
			for (int column = 0; column < 12; ++column)
			{
				int index = row * 12 + column;
				if (index >= maxColors) return;
				text.setColor(index);
				text.setX(column * 26);
				text.setY(row * 9);
				text.setValue(index);
				text.blit(_bg->getSurface());
			}
		}
		return;
	}

	// big text without/with high contrast
	if (action == PAT_BIG_LOW || action == PAT_BIG_HIGH)
	{
		Text text = Text(13, 17, 0, 0);
		text.setPalette(_bg->getPalette());
		text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
		text.setHighContrast((action == PAT_BIG_HIGH));
		text.setBig();
		for (int row = 0; row < 11; ++row)
		{
			for (int column = 0; column < 24; ++column)
			{
				int index = row * 24 + column;
				if (index >= maxColors) return;
				text.setColor(index);
				text.setX(column * 13);
				text.setY(row * 17);
				std::ostringstream ss;
				ss << index % 10;
				text.setText(ss.str().c_str());
				text.blit(_bg->getSurface());
			}
		}
		return;
	}

	// normal text without/with high contrast
	Text text = Text(25, 9, 0, 0);
	text.setPalette(_bg->getPalette());
	text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	text.setHighContrast((action == PAT_SMALL_HIGH));
	for (int row = 0; row < 22; ++row)
	{
		for (int column = 0; column < 12; ++column)
		{
			int index = row * 12 + column;
			if (index >= maxColors) return;
			text.setColor(index);
			text.setX(column * 26);
			text.setY(row * 9);
			std::ostringstream ss;
			ss << index;
			text.setText(ss.str().c_str());
			text.blit(_bg->getSurface());
		}
	}
}

TestPaletteState::~TestPaletteState()
{
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestPaletteState::btnCancelClick(Action *action)
{
	_game->popState();
}

}
