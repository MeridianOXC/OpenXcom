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

#include <sstream>
#include "ArticleStateBaseFacility.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Unicode.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

namespace OpenXcom
{

	ArticleStateBaseFacility::ArticleStateBaseFacility(ArticleDefinitionBaseFacility *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		RuleBaseFacility *facility = _game->getMod()->getBaseFacility(defs->id, true);

		// add screen elements
		_txtTitle = new Text(200, 17, 10, 24);

		// Set palette
		setStandardPalette("PAL_BASESCAPE");

		ArticleState::initLayout();

		// add other elements
		add(_txtTitle);

		// Set up objects
		_game->getMod()->getSurface("BACK09.SCR")->blitNShade(_bg, 0, 0);
		_btnOk->setColor(Palette::blockOffset(4));
		_btnPrev->setColor(Palette::blockOffset(4));
		_btnNext->setColor(Palette::blockOffset(4));
		_btnInfo->setColor(Palette::blockOffset(4));
		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		_txtTitle->setColor(Palette::blockOffset(13)+10);
		_txtTitle->setBig();
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

		// build preview image
		int tile_size = 32;
		_image = new Surface(
			tile_size * std::max(1, Mod::PEDIA_FACILITY_RENDER_PARAMETERS[0]),
			tile_size * std::max(1, Mod::PEDIA_FACILITY_RENDER_PARAMETERS[1]),
			232 + Mod::PEDIA_FACILITY_RENDER_PARAMETERS[2],
			16 + Mod::PEDIA_FACILITY_RENDER_PARAMETERS[3]);
		add(_image);

		SurfaceSet *graphic = _game->getMod()->getSurfaceSet("BASEBITS.PCK");
		Surface *frame;
		int x_offset, y_offset;
		int x_pos, y_pos;
		int num;

		// calculate preview offset
		x_offset = (tile_size * std::max(0, Mod::PEDIA_FACILITY_RENDER_PARAMETERS[0] - facility->getSizeX())) / 2;
		y_offset = (tile_size * std::max(0, Mod::PEDIA_FACILITY_RENDER_PARAMETERS[1] - facility->getSizeY())) / 2;

		// render build preview
		num = 0;
		y_pos = y_offset;
		for (int y = 0; y < facility->getSizeY(); ++y)
		{
			x_pos = x_offset;
			for (int x = 0; x < facility->getSizeX(); ++x)
			{
				frame = graphic->getFrame(facility->getSpriteShape() + num);
				frame->blitNShade(_image, x_pos, y_pos);

				if (facility->getSpriteEnabled())
				{
					frame = graphic->getFrame(facility->getSpriteFacility() + num);
					frame->blitNShade(_image, x_pos, y_pos);
				}

				x_pos += tile_size;
				num++;
			}
			y_pos += tile_size;
		}

		_txtInfo = new Text(300, 90, 10, 104);
		add(_txtInfo);

		_txtInfo->setColor(Palette::blockOffset(13)+10);
		_txtInfo->setSecondaryColor(Palette::blockOffset(13));
		_txtInfo->setWordWrap(true);
		_txtInfo->setScrollable(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		_lstInfo = new TextList(200, 42, 10, 42);
		add(_lstInfo);

		_lstInfo->setColor(Palette::blockOffset(13)+10);
		_lstInfo->setColumns(2, 140, 60);
		_lstInfo->setDot(true);

		_lstInfo->addRow(2, tr("STR_CONSTRUCTION_TIME").c_str(), tr("STR_DAY", facility->getBuildTime()).c_str());
		_lstInfo->setCellColor(0, 1, Palette::blockOffset(13)+0);

		std::ostringstream ss;
		ss << Unicode::formatFunding(facility->getBuildCost());
		_lstInfo->addRow(2, tr("STR_CONSTRUCTION_COST").c_str(), ss.str().c_str());
		_lstInfo->setCellColor(1, 1, Palette::blockOffset(13)+0);

		ss.str("");ss.clear();
		ss << Unicode::formatFunding(facility->getMonthlyCost());
		_lstInfo->addRow(2, tr("STR_MAINTENANCE_COST").c_str(), ss.str().c_str());
		_lstInfo->setCellColor(2, 1, Palette::blockOffset(13)+0);

		if (facility->getDefenseValue() > 0)
		{
			ss.str("");ss.clear();
			ss << facility->getDefenseValue();
			_lstInfo->addRow(2, tr("STR_DEFENSE_VALUE").c_str(), ss.str().c_str());
			_lstInfo->setCellColor(3, 1, Palette::blockOffset(13)+0);

			ss.str("");ss.clear();
			ss << Unicode::formatPercentage(facility->getHitRatio());
			_lstInfo->addRow(2, tr("STR_HIT_RATIO").c_str(), ss.str().c_str());
			_lstInfo->setCellColor(4, 1, Palette::blockOffset(13)+0);
		}
		centerAllSurfaces();
	}

	ArticleStateBaseFacility::~ArticleStateBaseFacility()
	{}

}
