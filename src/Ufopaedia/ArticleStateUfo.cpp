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

#include "ArticleStateUfo.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleUfo.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Unicode.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

	ArticleStateUfo::ArticleStateUfo(ArticleDefinitionUfo *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		RuleUfo *ufo = _game->getMod()->getUfo(defs->id, true);

		// add screen elements
		_txtTitle = new Text(155, 32, 5, 24);

		// Set palette
		setStandardPalette("PAL_GEOSCAPE");

		ArticleState::initLayout();

		// add other elements
		add(_txtTitle);

		// Set up objects
		_game->getMod()->getSurface("BACK11.SCR")->blitNShade(_bg, 0, 0);
		_btnOk->setColor(Palette::blockOffset(8)+5);
		_btnPrev->setColor(Palette::blockOffset(8)+5);
		_btnNext->setColor(Palette::blockOffset(8)+5);
		_btnInfo->setColor(Palette::blockOffset(8)+5);
		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		_txtTitle->setColor(Palette::blockOffset(8)+5);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

		_image = new Surface(160, 52, 160, 6);
		add(_image);

		RuleInterface *dogfightInterface = _game->getMod()->getInterface("dogfight");

		auto crop = _game->getMod()->getSurface("INTERWIN.DAT")->getCrop();
		crop.setX(0);
		crop.setY(0);
		crop.getCrop()->x = 0;
		crop.getCrop()->y = 0;
		crop.getCrop()->w = _image->getWidth();
		crop.getCrop()->h = _image->getHeight();
		_image->drawRect(crop.getCrop(), 15);
		crop.blit(_image);

		if (ufo->getModSprite().empty())
		{
			crop.getCrop()->y = dogfightInterface->getElement("previewMid")->y + dogfightInterface->getElement("previewMid")->h * ufo->getSprite();
			crop.getCrop()->h = dogfightInterface->getElement("previewMid")->h;
		}
		else
		{
			crop = _game->getMod()->getSurface(ufo->getModSprite())->getCrop();
		}
		crop.setX(0);
		crop.setY(0);
		crop.blit(_image);

		_txtInfo = new Text(300, 50, 10, 140);
		add(_txtInfo);

		_txtInfo->setColor(Palette::blockOffset(8)+5);
		_txtInfo->setSecondaryColor(Palette::blockOffset(8) + 10);
		_txtInfo->setWordWrap(true);
		_txtInfo->setScrollable(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		_lstInfo = new TextList(310, 64, 10, 68);
		add(_lstInfo);

		centerAllSurfaces();

		_lstInfo->setColor(Palette::blockOffset(8)+5);
		_lstInfo->setColumns(2, 200, 110);
//		_lstInfo->setCondensed(true);
		_lstInfo->setBig();
		_lstInfo->setDot(true);

		_lstInfo->addRow(2, tr("STR_DAMAGE_CAPACITY").c_str(), Unicode::formatNumber(ufo->getStats().damageMax).c_str());

		_lstInfo->addRow(2, tr("STR_WEAPON_POWER").c_str(), Unicode::formatNumber(ufo->getWeaponPower()).c_str());

		_lstInfo->addRow(2, tr("STR_WEAPON_RANGE").c_str(), tr("STR_KILOMETERS").arg(ufo->getWeaponRange()).c_str());

		_lstInfo->addRow(2, tr("STR_MAXIMUM_SPEED").c_str(), tr("STR_KNOTS").arg(Unicode::formatNumber(ufo->getStats().speedMax)).c_str());
	}

	ArticleStateUfo::~ArticleStateUfo()
	{}

}
