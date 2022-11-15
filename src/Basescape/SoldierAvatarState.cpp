/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "SoldierAvatarState.h"
#include <sstream>
#include <string>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Base.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleSoldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Avatar window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierAvatarState::SoldierAvatarState(Base *base, size_t soldier) : _base(base), _soldier(soldier)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 240, 160, 40, 24, POPUP_BOTH);
	_soldierSurface = new Surface(320, 200, 0, 0);
	_btnCancel = new TextButton(100, 16, 55, 160);
	_btnOk = new TextButton(100, 16, 165, 160);
	_txtTitle = new Text(182, 16, 69, 32);
	_txtType = new Text(90, 9, 122, 58);
	_lstAvatar = new TextList(132, 80, 115, 72);

	// Set palette
	setStandardPalette("PAL_BATTLESCAPE");

	add(_window, "window", "soldierAvatar");
	add(_soldierSurface);
	add(_btnCancel, "button", "soldierAvatar");
	add(_btnOk, "button", "soldierAvatar");
	add(_txtTitle, "text", "soldierAvatar");
	add(_txtType, "text", "soldierAvatar");
	add(_lstAvatar, "list", "soldierAvatar");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierAvatar");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierAvatarState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierAvatarState::btnCancelClick, Options::keyCancel);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&SoldierAvatarState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SoldierAvatarState::btnOkClick, Options::keyOk);

	Soldier *s = _base->getSoldiers()->at(_soldier);
	_origAvatar = SoldierAvatar("original", s->getGender(), s->getLook(), s->getLookVariant());
	initPreview(s);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_AVATAR_FOR").arg(s->getName()));

	_txtType->setText(tr("STR_TYPE"));

	_lstAvatar->setColumns(1, 125);
	_lstAvatar->setSelectable(true);
	_lstAvatar->setBackground(_window);
	_lstAvatar->setMargin(8);

	std::string prefix = "STR_AVATAR_NAME_";
	for  (int variant = 0; variant <= _game->getMod()->getMaxLookVariant(); ++variant)
	{
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 1), GENDER_MALE,   LOOK_BLONDE,    variant));
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 2), GENDER_MALE,   LOOK_BROWNHAIR, variant));
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 3), GENDER_MALE,   LOOK_ORIENTAL,  variant));
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 4), GENDER_MALE,   LOOK_AFRICAN,   variant));
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 5), GENDER_FEMALE, LOOK_BLONDE,    variant));
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 6), GENDER_FEMALE, LOOK_BROWNHAIR, variant));
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 7), GENDER_FEMALE, LOOK_ORIENTAL,  variant));
		_avatars.push_back(SoldierAvatar(prefix + std::to_string(variant*8 + 8), GENDER_FEMALE, LOOK_AFRICAN,   variant));
	}

	for (std::vector<SoldierAvatar>::const_iterator i = _avatars.begin(); i != _avatars.end(); ++i)
	{
		_lstAvatar->addRow(1, tr(i->getAvatarName()).c_str());
	}
	_lstAvatar->onMouseClick((ActionHandler)&SoldierAvatarState::lstAvatarClick);

	// switch to battlescape theme if called from inventory
	applyBattlescapeTheme("soldierAvatar");
}

/**
 * Shows the preview
 * @param s Soldier.
 */
void SoldierAvatarState::initPreview(Soldier *s)
{
	_soldierSurface->clear();

	if (!s)
	{
		return;
	}

	if (s->getArmor()->hasLayersDefinition())
	{
		for (const auto& layer : s->getArmorLayers())
		{
			auto surf = _game->getMod()->getSurface(layer, true);
			surf->blitNShade(_soldierSurface, 0, 0);
		}
	}
	else
	{
		const std::string look = s->getArmor()->getSpriteInventory();
		const std::string gender = s->getGender() == GENDER_MALE ? "M" : "F";
		std::stringstream ss;
		Surface *surf = 0;

		for (int i = 0; i <= RuleSoldier::LookVariantBits; ++i)
		{
			ss.str("");
			ss << look;
			ss << gender;
			ss << (int)s->getLook() + (s->getLookVariant() & (RuleSoldier::LookVariantMask >> i)) * 4;
			ss << ".SPK";
			std::string debug = ss.str();
			surf = _game->getMod()->getSurface(ss.str(), false);
			if (surf)
			{
				break;
			}
		}
		if (!surf)
		{
			ss.str("");
			ss << look;
			ss << ".SPK";
			surf = _game->getMod()->getSurface(ss.str(), false);
		}
		if (!surf)
		{
			surf = _game->getMod()->getSurface(look, true);
		}
		surf->blitNShade(_soldierSurface, 0, 0);
	}
}

/**
 *
 */
SoldierAvatarState::~SoldierAvatarState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierAvatarState::btnCancelClick(Action *)
{
	Soldier *soldier = _base->getSoldiers()->at(_soldier);

	// revert the avatar to original
	soldier->setGender(_origAvatar.getGender());
	soldier->setLook(_origAvatar.getLook());
	soldier->setLookVariant(_origAvatar.getLookVariant());

	_game->popState();
}

/**
 * Changes the avatar on the soldier and returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierAvatarState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Changes the avatar on the soldier and returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierAvatarState::lstAvatarClick(Action *)
{
	Soldier *soldier = _base->getSoldiers()->at(_soldier);

	// change the avatar
	soldier->setGender(_avatars[_lstAvatar->getSelectedRow()].getGender());
	soldier->setLook(_avatars[_lstAvatar->getSelectedRow()].getLook());
	soldier->setLookVariant(_avatars[_lstAvatar->getSelectedRow()].getLookVariant());

	initPreview(soldier);
}

}
