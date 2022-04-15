#pragma once
/*
 * Copyright 2010-2020 OpenXcom Developers.
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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */
#include "ActionMenuState.h"

namespace OpenXcom
{

/**
 * Window that allows the player
 * to select a battlescape action.
 */
class SkillMenuState : public ActionMenuState
{
private:
	/// Check if the given soldier has all the required soldier bonuses for this soldier skill.
	bool soldierHasAllRequiredBonusesForSkill(Soldier *soldier, const RuleSkill *skillRules);
	/// Adds a new menu item for an action.
	void addItem(const RuleSkill* skill, int *id, SDLKey key);
	/// Choose an action weapon based on given parameters.
	void chooseWeaponForSkill(BattleAction* action, const std::vector<const RuleItem*> &compatibleWeaponTypes, BattleType compatibleWeaponType, bool checkHandsOnly);
public:
	/// Creates the Skill Menu state.
	SkillMenuState(BattleAction *action, int x, int y);
	/// Cleans up the Skill Menu state.
	~SkillMenuState();
	/// Handler for clicking a skill menu item.
	void btnActionMenuItemClick(Action *action) override;
};

}
