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
#include "ArticleStateTFTDVehicle.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextList.h"
#include "../Mod/Mod.h"
#include "../Mod/Unit.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleItem.h"

namespace OpenXcom
{

	ArticleStateTFTDVehicle::ArticleStateTFTDVehicle(ArticleDefinitionTFTD *defs, std::shared_ptr<ArticleCommonState> state) : ArticleStateTFTD(defs, std::move(state))
	{
		_txtInfo->setHeight(72);

		RuleItem *item = _game->getMod()->getItem(defs->id, true);
		Unit *unit = item->getVehicleUnit();
		if (!unit)
		{
			throw Exception("ArticleStateTFTDVehicle: Item " + defs->id + " is missing a vehicle unit definition!");
		}
		Armor *armor = unit->getArmor();

		_lstStats = new TextList(150, 65, 168, 106);

		add(_lstStats);

		_lstStats->setColor(_listColor1);
		_lstStats->setColumns(2, 100, 50);
		_lstStats->setDot(true);

		_lstStats2 = new TextList(195, 33, 25, 166);

		add(_lstStats2);

		_lstStats2->setColor(_listColor1);
		_lstStats2->setColumns(2, 65, 130);
		_lstStats2->setDot(true);

		std::ostringstream ss;
		ss << unit->getStats()->tu;
		_lstStats->addRow(2, tr("STR_TIME_UNITS").c_str(), ss.str().c_str());

		std::ostringstream ss2;
		ss2 << unit->getStats()->health;
		_lstStats->addRow(2, tr("STR_HEALTH").c_str(), ss2.str().c_str());

		std::ostringstream ss3;
		ss3 << armor->getFrontArmor();
		_lstStats->addRow(2, tr("STR_FRONT_ARMOR").c_str(), ss3.str().c_str());

		std::ostringstream ss4;
		ss4 << armor->getLeftSideArmor();
		_lstStats->addRow(2, tr("STR_LEFT_ARMOR").c_str(), ss4.str().c_str());

		std::ostringstream ss5;
		ss5 << armor->getRightSideArmor();
		_lstStats->addRow(2, tr("STR_RIGHT_ARMOR").c_str(), ss5.str().c_str());

		std::ostringstream ss6;
		ss6 << armor->getRearArmor();
		_lstStats->addRow(2, tr("STR_REAR_ARMOR").c_str(), ss6.str().c_str());

		std::ostringstream ss7;
		ss7 << armor->getUnderArmor();
		_lstStats->addRow(2, tr("STR_UNDER_ARMOR").c_str(), ss7.str().c_str());

		_lstStats2->addRow(2, tr("STR_WEAPON").c_str(), tr(defs->weapon).c_str());

		if (item->getVehicleClipAmmo())
		{
			const RuleItem *ammo = item->getVehicleClipAmmo();

			std::ostringstream ss8;
			ss8 << ammo->getPower();
			_lstStats2->addRow(2, tr("STR_WEAPON_POWER").c_str(), ss8.str().c_str());

			_lstStats2->addRow(2, tr("STR_AMMUNITION").c_str(), tr(ammo->getName()).c_str());

			std::ostringstream ss9;
			ss9 << item->getVehicleClipSize();

			_lstStats2->addRow(2, tr("STR_ROUNDS").c_str(), ss9.str().c_str());
		}
		else
		{
			std::ostringstream ss8;
			ss8 << item->getPower();
			_lstStats2->addRow(2, tr("STR_WEAPON_POWER").c_str(), ss8.str().c_str());
		}

		for (size_t i = 0; i != _lstStats->getTexts(); ++i)
		{
			_lstStats->setCellColor(i, 1, _listColor2);
		}
		for (size_t i = 0; i != _lstStats2->getTexts(); ++i)
		{
			_lstStats2->setCellColor(i, 1, _listColor2);
		}

		centerAllSurfaces();
	}

	ArticleStateTFTDVehicle::~ArticleStateTFTDVehicle()
	{}

}
