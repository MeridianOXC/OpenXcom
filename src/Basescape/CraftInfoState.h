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
#include "../Mod/RuleCraft.h"

namespace OpenXcom
{

class Base;
class InteractiveSurface;
class TextButton;
class Window;
class Text;
class TextEdit;
class Surface;
class Craft;

/**
 * Craft Info screen that shows all the
 * info of a specific craft.
 */
class CraftInfoState : public State
{
private:
	Base *_base;
	size_t _craftId;
	Craft *_craft;
	int _weaponNum;

	TextButton *_btnOk, *_btnW[RuleCraft::WeaponMax], *_btnCrew, *_btnEquip, *_btnArmor, *_btnPilots, *_btnNewBattle;
	Window *_window;
	TextEdit *_edtCraft;
	Text *_txtDamage, *_txtShield, *_txtFuel, *_txtSkin;
	Text *_txtWName[RuleCraft::WeaponMax], *_txtWAmmo[RuleCraft::WeaponMax];
	InteractiveSurface *_sprite, *_weapon[RuleCraft::WeaponMax];
	Surface *_crew, *_equip;
	/// Formats an amount of time.
	std::string formatTime(int time);
public:
	/// Creates the Craft Info state.
	CraftInfoState(Base *base, size_t craftId);
	/// Cleans up the Craft Info state.
	~CraftInfoState();
	/// Updates the craft info.
	void init() override;
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Ufopedia button.
	void btnUfopediaClick(Action *action);
	/// Handler for clicking the New Battle button.
	void btnNewBattleClick(Action *action);
	/// Handler for clicking the weapon button.
	void btnWClick(Action *action);
	/// Handler for clicking one of the weapon icons.
	void btnWIconClick(Action *action);
	void btnWIconMiddleClick(Action* action);
	/// Handler for clicking one of the craft icon.
	void btnCraftIconClick(Action *action);
	/// Handler for clicking the Crew button.
	void btnCrewClick(Action *action);
	/// Handler for clicking the Equipment button.
	void btnEquipClick(Action *action);
	/// Handler for clicking the Armor button.
	void btnArmorClick(Action *action);
	/// Handler for clicking the Pilots button.
	void btnPilotsClick(Action *action);
	/// Handler for changing the text on the Name edit.
	void edtCraftChange(Action *action);
};

}
