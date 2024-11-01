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
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "SkillMenuState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSkill.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierBonus.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "ActionMenuItem.h"
#include "Pathfinding.h"
#include "TileEngine.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Skill Menu window.
 * @param action Pointer to the action.
 * @param x Position on the x-axis.
 * @param y position on the y-axis.
 */
SkillMenuState::SkillMenuState(BattleAction *action, int x, int y) : ActionMenuState(action)
{
	// Attention: back up the current _action members
	BattleActionType currentActionType = _action->type;
	BattleItem* currentWeapon = _action->weapon;
	const RuleSkill* currentSkill = _action->skillRules;

	_screen = false;

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	for (int i = 0; i < (int)std::size(_actionMenu); ++i)
	{
		_actionMenu[i] = new ActionMenuItem(i, _game, x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&SkillMenuState::btnActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;

	std::vector<SDL_Keycode> hotkeys = {
		Options::keyBattleActionItem5,
		Options::keyBattleActionItem4,
		Options::keyBattleActionItem3,
		Options::keyBattleActionItem2,
		Options::keyBattleActionItem1
	};
	auto* soldier = _action->actor->getGeoscapeSoldier();
	for (auto* skill : soldier->getRules()->getSkills())
	{
		if (!hotkeys.empty()
			&& soldierHasAllRequiredBonusesForSkill(soldier, skill)
			&& (skill->getCost().Time > 0 || skill->getCost().Mana > 0)
			&& (!skill->isPsiRequired() || _action->actor->getBaseStats()->psiSkill > 0))
		{
			// Attention: we are modifying _action here
			_action->skillRules = skill;
			_action->type = skill->getTargetMode();

			// Attention: we are modifying _action->weapon inside!
			chooseWeaponForSkill(_action, skill);

			// Attention: here the modified values are consumed
			addItem(skill, &id, hotkeys.back());

			hotkeys.pop_back();
		}
	}

	// Attention: restore the original values
	_action->type = currentActionType;
	_action->weapon = currentWeapon;
	_action->skillRules = currentSkill;
}

/**
 * Deletes the SkillMenuState.
 */
SkillMenuState::~SkillMenuState()
{

}

/**
 * Check if the given soldier has all the required soldier bonuses for this soldier skill.
 * @param soldier Soldier to check.
 * @param skillRules Skill rules.
 */
bool SkillMenuState::soldierHasAllRequiredBonusesForSkill(Soldier *soldier, const RuleSkill *skillRules)
{
	for (auto* requiredBonusRule : skillRules->getRequiredBonuses())
	{
		bool found = false;
		for (auto* bonusRule : *soldier->getBonuses(nullptr))
		{
			if (bonusRule == requiredBonusRule)
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

/**
 * Adds a new menu item for an action.
 * @param ba Action type.
 * @param name Action description.
 * @param id Pointer to the new item ID.
 */
void SkillMenuState::addItem(const RuleSkill* skill, int *id, SDL_Keycode key)
{
	BattleActionType ba = skill->getTargetMode();

	std::string s1, s2;
	RuleItemUseCost cost = _action->actor->getActionTUs(ba, _action->skillRules);

	if (_action->weapon)
	{
		int acc = BattleUnit::getFiringAccuracy(BattleActionAttack::GetBeforeShoot(ba, _action->actor, _action->weapon, _action->skillRules), _game->getMod());
		if (ba == BA_THROW || ba == BA_AIMEDSHOT || ba == BA_SNAPSHOT || ba == BA_AUTOSHOT || ba == BA_LAUNCH || ba == BA_HIT)
			s1 = tr("STR_ACCURACY_SHORT").arg(Unicode::formatPercentage(acc));
	}

	if (cost.Time > 0)
	{
		s2 = tr("STR_TIME_UNITS_SHORT").arg(cost.Time);
	}
	else if (cost.Mana > 0)
	{
		s2 = tr("STR_MANA_SHORT").arg(cost.Mana);
	}

	_actionMenu[*id]->setAction(ba, tr(skill->getType()), s1, s2, cost.Time);
	_actionMenu[*id]->setSkill(skill);
	_actionMenu[*id]->setVisible(true);

	if (key != SDLK_UNKNOWN)
	{
		_actionMenu[*id]->onKeyboardPress((ActionHandler)&SkillMenuState::btnActionMenuItemClick, key);
	}
	(*id)++;
}


/**
 * Executes the action corresponding to this action menu item.
 * @param action Pointer to an action.
 */
void SkillMenuState::btnActionMenuItemClick(Action *action)
{
	_game->getSavedGame()->getSavedBattle()->getPathfinding()->removePreview();

	int btnID = -1;

	if (_game->getSavedGame()->getSavedBattle()->isPreview())
	{
		_action->result = "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED";
		_game->popState();
		return;
	}

	// got to find out which button was pressed
	for (size_t i = 0; i < std::size(_actionMenu) && btnID == -1; ++i)
	{
		if (action->getSender() == _actionMenu[i])
		{
			btnID = i;
			break;
		}
	}

	if (btnID != -1)
	{
		TileEngine *tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();
		const RuleSkill *selectedSkill = _actionMenu[btnID]->getSkill();
		_action->skillRules = selectedSkill;
		_action->type = _actionMenu[btnID]->getAction();
		chooseWeaponForSkill(_action, selectedSkill);
		_action->updateTU();

		bool continueAction = tileEngine->skillUse(_action, selectedSkill);

		if (!continueAction || _action->type == BA_NONE)
		{
			_action->targeting = false;
			_action->type = BA_NONE;
			_action->skillRules = nullptr;
			_game->popState();
			return;
		}

		// check if the skill needs an item, if so check if an item was found
		BattleItem *item = _action->weapon;
		if (_action->type != BA_NONE)
		{
			if (!item)
			{
				_action->result = "STR_SKILL_NEEDS_ITEM";
				if (_action->type == BA_HIT) _action->type = BA_NONE; // OXC merge
				_game->popState();
				return;
			}
		}

		if (_action->type == BA_THROW && _action->weapon->getRules()->getBattleType() == BT_GRENADE)
		{
			// instant grenades
			_action->weapon->setFuseTimer(0);
			_action->weapon->setFuseEnabled(true);
		}
		else if (_action->type == BA_PRIME || _action->type == BA_UNPRIME)
		{
			// nothing, skill menu only supports instant grenades for now
			_game->popState();
			return;
		}

		// default handling from here on...
		ActionMenuState::handleAction();
	}
}

void SkillMenuState::chooseWeaponForSkill(BattleAction* action, const RuleSkill* skillRules)
{
	auto* unit = action->actor;
	action->weapon = nullptr;

	if (action->type == BA_NONE)
	{
		return;
	}

	// 1. choose by weapon's name
	if (!skillRules->getCompatibleWeapons().empty())
	{
		for (auto* itemRule : skillRules->getCompatibleWeapons())
		{
			// check both hands, right first
			if (unit->getRightHandWeapon() && unit->getRightHandWeapon()->getRules() == itemRule)
			{
				action->weapon = unit->getRightHandWeapon();
				return;
			}
			else if (unit->getLeftHandWeapon() && unit->getLeftHandWeapon()->getRules() == itemRule)
			{
				action->weapon = unit->getLeftHandWeapon();
				return;
			}
			if (!skillRules->checkHandsOnly())
			{
				// check special weapons
				BattleItem *item = unit->getSpecialWeapon(itemRule);
				if (item)
				{
					action->weapon = item;
					return;
				}
				// check inventory
				for (auto* invItem : *unit->getInventory())
				{
					if (invItem->getRules() == itemRule)
					{
						action->weapon = invItem;
						return;
					}
				}
			}
		}
	}

	// 2. if not found, try by weapon's battle type
	if (skillRules->getCompatibleBattleType() != BT_NONE)
	{
		// check both hands, right first
		if (unit->getRightHandWeapon() && unit->getRightHandWeapon()->getRules()->getBattleType() == skillRules->getCompatibleBattleType())
		{
			action->weapon = unit->getRightHandWeapon();
			return;
		}
		else if (unit->getLeftHandWeapon() && unit->getLeftHandWeapon()->getRules()->getBattleType() == skillRules->getCompatibleBattleType())
		{
			action->weapon = unit->getLeftHandWeapon();
			return;
		}
		if (!skillRules->checkHandsOnly2())
		{
			// check special weapons
			BattleItem* item = unit->getSpecialWeapon(skillRules->getCompatibleBattleType());
			if (item)
			{
				action->weapon = item;
				return;
			}
			// check inventory
			for (auto* invItem : *unit->getInventory())
			{
				if (invItem->getRules()->getBattleType() == skillRules->getCompatibleBattleType())
				{
					action->weapon = invItem;
					return;
				}
			}
		}
	}
}

}
