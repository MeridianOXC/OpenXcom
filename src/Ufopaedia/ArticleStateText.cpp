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

#include "../Mod/ArticleDefinition.h"
#include "ArticleStateText.h"
#include "../Engine/Game.h"
#include "../Engine/Surface.h"
#include "../Mod/Mod.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

	ArticleStateText::ArticleStateText(ArticleDefinitionText *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		// add screen elements
		_txtTitle = new Text(296, 17, 5, 23);
		_txtInfo = new Text(296, 150, 10, 48);

		// Set palette
		setStandardPalette("PAL_UFOPAEDIA");

		_buttonColor = _game->getMod()->getInterface("articleText")->getElement("button")->color;
		_titleColor = _game->getMod()->getInterface("articleText")->getElement("title")->color;
		_textColor1 = _game->getMod()->getInterface("articleText")->getElement("text")->color;
		_textColor2 = _game->getMod()->getInterface("articleText")->getElement("text")->color2;

		ArticleState::initLayout();

		// add other elements
		add(_txtTitle);
		add(_txtInfo);

		centerAllSurfaces();

		// Set up objects
		_game->getMod()->getSurface("BACK10.SCR")->blitNShade(_bg, 0, 0);
		_btnOk->setColor(_buttonColor);
		_btnPrev->setColor(_buttonColor);
		_btnNext->setColor(_buttonColor);

		_txtTitle->setColor(_titleColor);
		_txtTitle->setBig();
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

		_txtInfo->setColor(_textColor1);
		_txtInfo->setSecondaryColor(_textColor2);
		_txtInfo->setWordWrap(true);
		_txtInfo->setScrollable(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));
	}

	ArticleStateText::~ArticleStateText()
	{}

}
