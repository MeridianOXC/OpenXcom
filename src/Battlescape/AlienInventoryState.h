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
#include "../Engine/State.h"

namespace OpenXcom
{

class Surface;
class BattlescapeButton;
class AlienInventory;
class BattleItem;
class BattleUnit;
class Text;

/**
 * Screen which displays alien's inventory.
 */
class AlienInventoryState : public State
{
private:
	Surface *_bg, *_soldier;
	BattlescapeButton *_btnArmor;
	Text *_txtName;
	Text *_txtFatalWounds;
	Text *_txtLeftHand, *_txtRightHand;
	AlienInventory *_inv;

	void calculateMeleeWeapon(BattleUnit* unit, BattleItem* weapon, Text* label);
	void calculateRangedWeapon(BattleUnit* unit, BattleItem* weapon, Text* label);
public:
	/// Creates the AlienInventory state.
	AlienInventoryState(BattleUnit *unit);
	/// Cleans up the AlienInventory state.
	~AlienInventoryState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the [Toggle] button.
	void btnToggleClick(Action *action);
	/// Handler for clicking the Armor button.
	void btnArmorClickMiddle(Action *action);
};

}
