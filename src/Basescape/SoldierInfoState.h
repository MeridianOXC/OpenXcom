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
#include <vector>

namespace OpenXcom
{

class Base;
class Surface;
class TextButton;
class Text;
class TextEdit;
class Bar;
class Soldier;

/**
 * Soldier Info screen that shows all the
 * info of a specific soldier.
 */
class SoldierInfoState : public State
{
private:
	Base *_base;
	size_t _soldierId;
	bool _forceLimits;
	bool _readOnly;
	bool _noTransformations;
	Soldier *_soldier;
	std::vector<Soldier*> *_list;

	Surface *_bg;
	InteractiveSurface *_rank;
	InteractiveSurface *_flag;
	TextButton *_btnOk, *_btnPrev, *_btnNext, *_btnArmor, *_btnSack, *_btnDiary, *_btnBonuses, *_btnTransformations;
	Text *_txtRank, *_txtMissions, *_txtKills, *_txtCraft, *_txtRecovery, *_txtPsionic, *_txtDead;
	Text *_txtStuns;
	TextEdit *_edtSoldier;

	Text *_txtTimeUnits, *_txtStamina, *_txtHealth, *_txtBravery, *_txtReactions, *_txtFiring, *_txtThrowing, *_txtMelee, *_txtStrength, *_txtPsiStrength, *_txtPsiSkill, *_txtMana;
	Text *_numTimeUnits, *_numStamina, *_numHealth, *_numBravery, *_numReactions, *_numFiring, *_numThrowing, *_numMelee, *_numStrength, *_numPsiStrength, *_numPsiSkill, *_numMana;
	Bar *_barTimeUnits, *_barStamina, *_barHealth, *_barBravery, *_barReactions, *_barFiring, *_barThrowing, *_barMelee, *_barStrength, *_barPsiStrength, *_barPsiSkill, *_barMana;

public:
	/// Creates the Soldier Info state.
	SoldierInfoState(Base *base, size_t soldierId, bool forceLimits = true, bool readOnly = false);
	/// Cleans up the Soldier Info state.
	~SoldierInfoState();
	/// Updates the soldier info.
	void init() override;
	/// Handles keypresses.
	void handle(Action* action) override;
	/// Set the soldier Id.
	void setSoldierId(size_t soldier);
	/// Handler for pressing on the Name edit.
	void edtSoldierPress(Action *action);
	/// Handler for changing text on the Name edit.
	void edtSoldierChange(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Previous button.
	void btnPrevClick(Action *action);
	/// Handler for clicking the Next button.
	void btnNextClick(Action *action);
	/// Handler for clicking the Armor button.
	void btnArmorClick(Action *action);
	/// Handler for clicking the Bonuses button.
	void btnBonusesClick(Action *action);
	/// Handler for clicking the Transformations button.
	void btnTransformationsClick(Action* action);
	/// Handler for clicking the Sack button.
	void btnSackClick(Action *action);
	/// Handler for clicking the Diary button.
	void btnDiaryClick(Action *action);
	/// Handler for clicking the flag.
	void btnFlagClick(Action *action);
	/// Handler for clicking the rank.
	void btnRankClick(Action *action);
};

}
