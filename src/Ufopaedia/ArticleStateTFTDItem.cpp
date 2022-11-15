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
#include <algorithm>
#include "Ufopaedia.h"
#include "ArticleStateTFTDItem.h"
#include "../Mod/Mod.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/RuleItem.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Engine/Unicode.h"
#include "../Interface/TextList.h"
#include <algorithm>

namespace OpenXcom
{

	ArticleStateTFTDItem::ArticleStateTFTDItem(ArticleDefinitionTFTD *defs, std::shared_ptr<ArticleCommonState> state) : ArticleStateTFTD(defs, std::move(state))
	{
		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		RuleItem *item = _game->getMod()->getItem(defs->id, true);

		auto ammoSlot = defs->getAmmoSlotForPage(_state->current_page);
		auto ammoSlotPrevUsage = defs->getAmmoSlotPrevUsageForPage(_state->current_page);
		const std::vector<const RuleItem*> dummy;
		const std::vector<const RuleItem*> *ammo_data = ammoSlot != RuleItem::AmmoSlotSelfUse ? item->getCompatibleAmmoForSlot(ammoSlot) : &dummy;

		// SHOT STATS TABLE (for firearms only)
		if (item->getBattleType() == BT_FIREARM)
		{
			_txtShotType = new Text(53, 17, 8, 157);
			add(_txtShotType);
			_txtShotType->setColor(_textColor);
			_txtShotType->setWordWrap(true);
			_txtShotType->setText(tr("STR_SHOT_TYPE"));

			_txtAccuracy = new Text(57, 17, 61, 157);
			add(_txtAccuracy);
			_txtAccuracy->setColor(_textColor);
			_txtAccuracy->setWordWrap(true);
			_txtAccuracy->setText(tr("STR_ACCURACY_UC"));

			_txtTuCost = new Text(56, 17, 118, 157);
			add(_txtTuCost);
			_txtTuCost->setColor(_textColor);
			_txtTuCost->setWordWrap(true);
			_txtTuCost->setText(tr("STR_TIME_UNIT_COST"));

			_lstInfo = new TextList(140, 55, 8, 170);
			add(_lstInfo);

			_lstInfo->setColor(_listColor2); // color for % data!
			_lstInfo->setColumns(3, 70, 40, 30);


			auto addAttack = [&](int& row, const std::string& name, const RuleItemUseCost& cost, const RuleItemUseCost& flat, const RuleItemAction *config, const RuleItem* weapon)
			{
				if (row < 3 && cost.Time > 0 && config->ammoSlot == ammoSlot)
				{
					std::string tu = Unicode::formatPercentage(cost.Time);
					if (flat.Time)
					{
						tu.erase(tu.end() - 1);
					}
					int range = std::min(config->range, weapon->getMaxRange());
					std::string label = config->shortName.empty() ? tr(name).arg(config->shots).arg(range) : tr(config->shortName).arg(config->shots).arg(range);
					_lstInfo->addRow(3,
						label.c_str(),
						Unicode::formatPercentage(config->accuracy).c_str(),
						tu.c_str());
					_lstInfo->setCellColor(row, 0, _listColor1);
					row++;
				}
			};

			int current_row = 0;

			addAttack(current_row, "STR_SHOT_TYPE_AUTO", item->getCostAuto(), item->getFlatAuto(), item->getConfigAuto(), item);

			addAttack(current_row, "STR_SHOT_TYPE_SNAP", item->getCostSnap(), item->getFlatSnap(), item->getConfigSnap(), item);

			addAttack(current_row, "STR_SHOT_TYPE_AIMED", item->getCostAimed(), item->getFlatAimed(), item->getConfigAimed(), item);

			//optional melee
			addAttack(current_row, "STR_SHOT_TYPE_MELEE", item->getCostMelee(), item->getFlatMelee(), item->getConfigMelee(), item);
		}

		// AMMO column
		std::ostringstream ss;

		for (int i = 0; i<3; ++i)
		{
			_txtAmmoType[i] = new Text(120, 9, 168, 144 + i*10);
			add(_txtAmmoType[i]);
			_txtAmmoType[i]->setColor(_textColor);
			_txtAmmoType[i]->setWordWrap(true);

			_txtAmmoDamage[i] = new Text(20, 9, 300, 144 + i*10);
			add(_txtAmmoDamage[i]);
			_txtAmmoDamage[i]->setColor(_ammoColor);
		}

		auto addAmmoDamagePower = [&](int pos, const RuleItem *rule)
		{
			_txtAmmoType[pos]->setText(tr(getDamageTypeText(rule->getDamageType()->ResistType)));

			ss.str("");ss.clear();
			ss << rule->getPower();
			if (rule->getShotgunPellets())
			{
				ss << "x" << rule->getShotgunPellets();
			}
			_txtAmmoDamage[pos]->setText(ss.str());
		};

		switch (item->getBattleType())
		{
			case BT_FIREARM:
				if (item->getHidePower()) break;
				if (ammo_data->empty())
				{
					addAmmoDamagePower(0, item);
				}
				else
				{
					int maxShow = 3;
					int skipShow = maxShow * ammoSlotPrevUsage;
					int currShow = 0;
					for (auto& type : *ammo_data)
					{
						ArticleDefinition *ammo_article = _game->getMod()->getUfopaediaArticle(type->getType(), true);
						if (Ufopaedia::isArticleAvailable(_game->getSavedGame(), ammo_article))
						{
							if (skipShow > 0)
							{
								--skipShow;
								continue;
							}
							addAmmoDamagePower(currShow, type);

							++currShow;
							if (currShow == maxShow)
							{
								break;
							}
						}
					}
				}
				break;
			case BT_AMMO:
			case BT_GRENADE:
			case BT_PROXIMITYGRENADE:
			case BT_MELEE:
				if (item->getHidePower()) break;
				addAmmoDamagePower(0, item);
				break;
			default: break;
		}

		if (!_txtAmmoType[0]->getText().empty())
			_txtInfo->setHeight(112);

		// multi-page indicator
		_txtArrows = new Text(32, 9, 277, 134);
		add(_txtArrows);
		_txtArrows->setColor(_arrowColor);
		_txtArrows->setAlign(ALIGN_RIGHT);
		std::ostringstream ss2;
		if (_state->hasPrevArticlePage()) ss2 << "<<";
		if (_state->hasNextArticlePage()) ss2 << " >>";
		_txtArrows->setText(ss2.str());

		centerAllSurfaces();
	}

	ArticleStateTFTDItem::~ArticleStateTFTDItem()
	{}

}
