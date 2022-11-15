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
#include "../Mod/Mod.h"
#include "../Mod/RuleCraft.h"
#include "ArticleStateTFTD.h"
#include "ArticleStateTFTDCraft.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Engine/Unicode.h"
#include "../Interface/Text.h"

namespace OpenXcom
{

	ArticleStateTFTDCraft::ArticleStateTFTDCraft(ArticleDefinitionTFTD *defs, std::shared_ptr<ArticleCommonState> state) : ArticleStateTFTD(defs, std::move(state))
	{
		_txtInfo->setHeight(80);

		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		RuleCraft *craft = _game->getMod()->getCraft(defs->id, true);
		_txtStats = new Text(131, 56, 187, 116);
		add(_txtStats);

		_txtStats->setColor(_textColor);
		_txtStats->setSecondaryColor(_textColor2);

		std::ostringstream ss;
		ss << tr("STR_MAXIMUM_SPEED_UC").arg(Unicode::formatNumber(craft->getMaxSpeed())) << '\n';
		ss << tr("STR_ACCELERATION").arg(craft->getAcceleration()) << '\n';
		int range;
		switch (_game->getMod()->getPediaReplaceCraftFuelWithRangeType())
		{
			// Both max range alone and average range get rounded
			case 0:
			case 2:
				range = craft->calculateRange(_game->getMod()->getPediaReplaceCraftFuelWithRangeType());
				if (range == -1)
				{
					ss << tr("STR_MAXIMUM_RANGE").arg(tr("STR_INFINITE_RANGE")) << '\n';
					break;
				}

				// Round the answer to
				if (range < 100)
				{
					// don't round if it's small!
				}
				else if (range < 1000)
				{
					// nearest 10 nautical miles
					range += 10 / 2;
					range -= range % 10;
				}
				else
				{
					// nearest 100 nautical miles
					range += 100 / 2;
					range -= range % 100;
				}

				ss << tr("STR_MAXIMUM_RANGE").arg(Unicode::formatNumber(range)) << '\n';
				break;
			// Min-maxxers can fret over exact numbers
			case 1:
				if (craft->calculateRange(0) == -1)
				{
					ss << tr("STR_MAXIMUM_RANGE").arg(tr("STR_INFINITE_RANGE")) << '\n';
					break;
				}

				ss << tr("STR_MINIMUM_RANGE").arg(Unicode::formatNumber(craft->calculateRange(1))) << '\n';
				ss << tr("STR_MAXIMUM_RANGE").arg(Unicode::formatNumber(craft->calculateRange(0))) << '\n';
				break;
			default :
				ss << tr("STR_FUEL_CAPACITY").arg(Unicode::formatNumber(craft->getMaxFuel())) << '\n';
				break;
		}
		ss << tr("STR_WEAPON_PODS").arg(craft->getWeapons()) << '\n';
		ss << tr("STR_DAMAGE_CAPACITY_UC").arg(Unicode::formatNumber(craft->getMaxDamage())) << '\n';
		ss << tr("STR_CARGO_SPACE").arg(craft->getMaxUnits()) << '\n';
		ss << tr("STR_HWP_CAPACITY").arg(craft->getMaxVehiclesAndLargeSoldiers());
		_txtStats->setText(ss.str());

		centerAllSurfaces();
	}

	ArticleStateTFTDCraft::~ArticleStateTFTDCraft()
	{}

}
