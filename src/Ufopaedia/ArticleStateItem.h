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
#include "ArticleState.h"

namespace OpenXcom
{
	class Game;
	class Surface;
	class NumberText;
	class Text;
	class TextList;
	class ArticleDefinitionItem;

	/**
	 * ArticleStateItem has a caption, text, preview image and a stats block.
	 * The facility image is found using the RuleBasefacility class.
	 */

	class ArticleStateItem : public ArticleState
	{
	public:
		ArticleStateItem(ArticleDefinitionItem *article_defs, std::shared_ptr<ArticleCommonState> state);
		virtual ~ArticleStateItem();

	protected:
		Surface *_image;
		NumberText *_txtWeaponClipSize;
		Text *_txtTitle;
		Text *_txtWeight;
		Text *_txtInfo;
		Text *_txtAccuracyModifier;
		Text *_txtPowerBonus;
		TextList *_lstInfo;
		Text *_txtShotType;
		Text *_txtAccuracy;
		Text *_txtTuCost;
		Text *_txtDamage;
		Text *_txtAmmo;
		Text *_txtAmmoType[3];
		Text *_txtAmmoDamage[3];
		Surface *_imageAmmo[3];
		NumberText *_txtAmmoClipSize[3];
		Text * _txtArrows;
		Uint8 _buttonColor, _textColor, _textColor2, _listColor1, _listColor2, _ammoColor, _arrowColor;
		std::string addRuleStatBonus(const RuleStatBonus &value);
		int getDamageTypeTextColor(ItemDamageType dt);
	};
}
