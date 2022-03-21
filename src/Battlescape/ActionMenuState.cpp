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
#include "ActionMenuState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Action.h"
#include "../Engine/Sound.h"
#include "../Engine/Unicode.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/Mod.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleItem.h"
#include "ActionMenuItem.h"
#include "PrimeGrenadeState.h"
#include "MedikitState.h"
#include "ScannerState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/HitLog.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "../Interface/Text.h"

namespace OpenXcom
{

/**
 * Default constructor, used by SkillMenuState.
 */
ActionMenuState::ActionMenuState(BattleAction *action) : _action(action)
{
}

/**
 * Initializes all the elements in the Action Menu window.
 * @param game Pointer to the core game.
 * @param action Pointer to the action.
 * @param x Position on the x-axis.
 * @param y position on the y-axis.
 */
ActionMenuState::ActionMenuState(BattleAction *action, int x, int y) : _action(action)
{
	_screen = false;

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	for (int i = 0; i < 6; ++i)
	{
		_actionMenu[i] = new ActionMenuItem(i, _game, x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&ActionMenuState::btnActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;
	const RuleItem *weapon = _action->weapon->getRules();

	// throwing (if not a fixed weapon)
	if (!weapon->isFixed() && weapon->getCostThrow().Time > 0)
	{
		addItem(BA_THROW, "STR_THROW", &id, Options::keyBattleActionItem5);
	}

	if (weapon->isPsiRequired() && _action->actor->getBaseStats()->psiSkill <= 0)
	{
		return;
	}

	if (weapon->isManaRequired() && _action->actor->getOriginalFaction() == FACTION_PLAYER)
	{
		if (!_game->getMod()->isManaFeatureEnabled() || !_game->getSavedGame()->isManaUnlocked(_game->getMod()))
		{
			return;
		}
	}

	// priming
	if (weapon->getFuseTimerType() != BFT_NONE)
	{
		auto normalWeapon = weapon->getBattleType() != BT_GRENADE && weapon->getBattleType() != BT_FLARE && weapon->getBattleType() != BT_PROXIMITYGRENADE;
		if (_action->weapon->getFuseTimer() == -1)
		{
			if (weapon->getCostPrime().Time > 0)
			{
				addItem(BA_PRIME, weapon->getPrimeActionName(), &id, normalWeapon ? SDLK_UNKNOWN : Options::keyBattleActionItem1);
			}
		}
		else
		{
			if (weapon->getCostUnprime().Time > 0 && !weapon->getUnprimeActionName().empty())
			{
				addItem(BA_UNPRIME, weapon->getUnprimeActionName(), &id, normalWeapon ? SDLK_UNKNOWN : Options::keyBattleActionItem2);
			}
		}
	}

	if (weapon->getBattleType() == BT_FIREARM)
	{
		auto isLauncher = _action->weapon->getCurrentWaypoints() != 0;
		auto slotLauncher = _action->weapon->getActionConf(BA_LAUNCH)->ammoSlot;
		auto slotSnap = _action->weapon->getActionConf(BA_SNAPSHOT)->ammoSlot;
		auto slotAuto = _action->weapon->getActionConf(BA_AUTOSHOT)->ammoSlot;

		if ((!isLauncher || slotLauncher != slotAuto) && weapon->getCostAuto().Time > 0)
		{
			addItem(BA_AUTOSHOT, weapon->getConfigAuto()->name, &id, Options::keyBattleActionItem3);
		}

		if ((!isLauncher || slotLauncher != slotSnap) && weapon->getCostSnap().Time > 0)
		{
			addItem(BA_SNAPSHOT,  weapon->getConfigSnap()->name, &id, Options::keyBattleActionItem2);
		}

		if (isLauncher)
		{
			addItem(BA_LAUNCH, "STR_LAUNCH_MISSILE", &id, Options::keyBattleActionItem1);
		}
		else if (weapon->getCostAimed().Time > 0)
		{
			addItem(BA_AIMEDSHOT,  weapon->getConfigAimed()->name, &id, Options::keyBattleActionItem1);
		}
	}

	if (weapon->getCostMelee().Time > 0)
	{
		std::string name = weapon->getConfigMelee()->name;
		if (name.empty())
		{
			// stun rod
			if (weapon->getBattleType() == BT_MELEE && weapon->getDamageType()->ResistType == DT_STUN)
			{
				name = "STR_STUN";
			}
			else
			// melee weapon
			{
				name = "STR_HIT_MELEE";
			}
		}
		addItem(BA_HIT, name, &id, Options::keyBattleActionItem4);
	}

	// special items
	if (weapon->getBattleType() == BT_MEDIKIT)
	{
		addItem(BA_USE, weapon->getMedikitActionName(), &id, Options::keyBattleActionItem1);
	}
	else if (weapon->getBattleType() == BT_SCANNER)
	{
		addItem(BA_USE, weapon->getPsiAttackName().empty() ? "STR_USE_SCANNER" : weapon->getPsiAttackName(), &id, Options::keyBattleActionItem1);
	}
	else if (weapon->getBattleType() == BT_PSIAMP)
	{
		if (weapon->getCostMind().Time > 0)
		{
			addItem(BA_MINDCONTROL, "STR_MIND_CONTROL", &id, Options::keyBattleActionItem3);
		}
		if (weapon->getCostPanic().Time > 0)
		{
			addItem(BA_PANIC, "STR_PANIC_UNIT", &id, Options::keyBattleActionItem2);
		}
		if (weapon->getCostUse().Time > 0)
		{
			addItem(BA_USE, weapon->getPsiAttackName(), &id, Options::keyBattleActionItem1);
		}
	}
	else if (weapon->getBattleType() == BT_MINDPROBE)
	{
		addItem(BA_USE, weapon->getPsiAttackName().empty() ? "STR_USE_MIND_PROBE" : weapon->getPsiAttackName(), &id, Options::keyBattleActionItem1);
	}

}

/**
 * Deletes the ActionMenuState.
 */
ActionMenuState::~ActionMenuState()
{

}

/**
 * Init function.
 */
void ActionMenuState::init()
{
	if (!_actionMenu[0]->getVisible())
	{
		// Item don't have any actions, close popup.
		_game->popState();
	}
}

/**
 * Adds a new menu item for an action.
 * @param ba Action type.
 * @param name Action description.
 * @param id Pointer to the new item ID.
 */
void ActionMenuState::addItem(BattleActionType ba, const std::string &name, int *id, SDLKey key)
{
	std::string s1, s2;
	int acc = BattleUnit::getFiringAccuracy(BattleActionAttack::GetBeforeShoot(ba, _action->actor, _action->weapon), _game->getMod());
	int tu = _action->actor->getActionTUs(ba, _action->weapon).Time;

	if (ba == BA_THROW || ba == BA_AIMEDSHOT || ba == BA_SNAPSHOT || ba == BA_AUTOSHOT || ba == BA_LAUNCH || ba == BA_HIT)
		s1 = tr("STR_ACCURACY_SHORT").arg(Unicode::formatPercentage(acc));
	s2 = tr("STR_TIME_UNITS_SHORT").arg(tu);
	_actionMenu[*id]->setAction(ba, tr(name), s1, s2, tu);
	_actionMenu[*id]->setVisible(true);
	if (key != SDLK_UNKNOWN)
	{
		_actionMenu[*id]->onKeyboardPress((ActionHandler)&ActionMenuState::btnActionMenuItemClick, key);
	}
	(*id)++;
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void ActionMenuState::handle(Action *action)
{
	State::handle(action);
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN && _game->isRightClick(action))
	{
		_game->popState();
	}
	else if (action->getDetails()->type == SDL_KEYDOWN &&
		(action->getDetails()->key.keysym.sym == Options::keyCancel ||
		action->getDetails()->key.keysym.sym == Options::keyBattleUseLeftHand ||
		action->getDetails()->key.keysym.sym == Options::keyBattleUseRightHand))
	{
		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action Pointer to an action.
 */
void ActionMenuState::btnActionMenuItemClick(Action *action)
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
		}
	}

	if (btnID != -1)
	{
		_action->type = _actionMenu[btnID]->getAction();
		_action->skillRules = nullptr;
		_action->updateTU();

		handleAction();
	}
}

void ActionMenuState::handleAction()
{
	// reset potential garbage from the previous action
	_action->terrainMeleeTilePart = 0;

	{
		const RuleItem *weapon = _action->weapon->getRules();
		bool newHitLog = false;
		std::string actionResult = "STR_UNKNOWN"; // needs a non-empty default/fall-back !

		if (_action->type != BA_THROW &&
			_action->actor->getOriginalFaction() == FACTION_PLAYER &&
			!_game->getSavedGame()->isResearched(weapon->getRequirements()))
		{
			_action->result = "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED";
			_game->popState();
		}
		else if (_action->type != BA_THROW &&
			!_game->getSavedGame()->getSavedBattle()->canUseWeapon(_action->weapon, _action->actor, false, _action->type, &actionResult))
		{
			_action->result = actionResult;
			_game->popState();
		}
		else if (_action->type == BA_PRIME)
		{
			const BattleFuseType fuseType = weapon->getFuseTimerType();
			if (fuseType == BFT_SET)
			{
				_game->pushState(new PrimeGrenadeState(_action, false, 0));
			}
			else
			{
				_action->value = weapon->getFuseTimerDefault();
				_game->popState();
			}
		}
		else if (_action->type == BA_UNPRIME)
		{
			_game->popState();
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit *targetUnit = 0;
			TileEngine *tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();
			const std::vector<BattleUnit*> *units = _game->getSavedGame()->getSavedBattle()->getUnits();
			for (std::vector<BattleUnit*>::const_iterator i = units->begin(); i != units->end() && !targetUnit; ++i)
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if ((*i)->getPosition() == _action->actor->getPosition() && *i != _action->actor && (*i)->getStatus() == STATUS_UNCONSCIOUS && ((*i)->isWoundable() || weapon->getAllowTargetImmune()) && weapon->getAllowTargetGround())
				{
					if ((*i)->getArmor()->getSize() != 1)
					{
						// never EVER apply anything to 2x2 units on the ground
						continue;
					}
					if ((weapon->getAllowTargetFriendGround() && (*i)->getOriginalFaction() == FACTION_PLAYER) ||
						(weapon->getAllowTargetFriendGround() && (*i)->getOriginalFaction() == FACTION_ALIEN_PLAYER) ||
						(weapon->getAllowTargetNeutralGround() && (*i)->getOriginalFaction() == FACTION_NEUTRAL) ||
						(weapon->getAllowTargetHostileGround() && (*i)->getOriginalFaction() == FACTION_HOSTILE))
					{
						targetUnit = *i;
					}
				}
			}
			if (!targetUnit && weapon->getAllowTargetStanding())
			{
				if (tileEngine->validMeleeRange(
					_action->actor->getPosition(),
					_action->actor->getDirection(),
					_action->actor,
					0, &_action->target, false))
				{
					Tile *tile = _game->getSavedGame()->getSavedBattle()->getTile(_action->target);
					if (tile != 0 && tile->getUnit() && (tile->getUnit()->isWoundable() || weapon->getAllowTargetImmune()))
					{
						if ((weapon->getAllowTargetFriendStanding() && tile->getUnit()->getOriginalFaction() == FACTION_PLAYER) ||
							(weapon->getAllowTargetFriendStanding() && tile->getUnit()->getOriginalFaction() == FACTION_ALIEN_PLAYER) ||
							(weapon->getAllowTargetNeutralStanding() && tile->getUnit()->getOriginalFaction() == FACTION_NEUTRAL) ||
							(weapon->getAllowTargetHostileStanding() && tile->getUnit()->getOriginalFaction() == FACTION_HOSTILE))
						{
							targetUnit = tile->getUnit();
						}
					}
				}
			}
			if (!targetUnit && weapon->getAllowTargetSelf())
			{
				targetUnit = _action->actor;
			}
			if (targetUnit)
			{
				_game->popState();
				BattleMediKitType type = weapon->getMediKitType();
				if (type)
				{
					if ((type == BMT_HEAL && _action->weapon->getHealQuantity() > 0) ||
						(type == BMT_STIMULANT && _action->weapon->getStimulantQuantity() > 0) ||
						(type == BMT_PAINKILLER && _action->weapon->getPainKillerQuantity() > 0))
					{
						if (_action->spendTU(&_action->result))
						{
							switch (type)
							{
							case BMT_HEAL:
								if (targetUnit->getFatalWounds())
								{
									for (int i = 0; i < BODYPART_MAX; ++i)
									{
										if (targetUnit->getFatalWound((UnitBodyPart)i))
										{
											tileEngine->medikitUse(_action, targetUnit, BMA_HEAL, (UnitBodyPart)i);
											tileEngine->medikitRemoveIfEmpty(_action);
											break;
										}
									}
								}
								else
								{
									tileEngine->medikitUse(_action, targetUnit, BMA_HEAL, BODYPART_TORSO);
									tileEngine->medikitRemoveIfEmpty(_action);
								}
								break;
							case BMT_STIMULANT:
								tileEngine->medikitUse(_action, targetUnit, BMA_STIMULANT, BODYPART_TORSO);
								tileEngine->medikitRemoveIfEmpty(_action);
								break;
							case BMT_PAINKILLER:
								tileEngine->medikitUse(_action, targetUnit, BMA_PAINKILLER, BODYPART_TORSO);
								tileEngine->medikitRemoveIfEmpty(_action);
								break;
							case BMT_NORMAL:
								break;
							}
						}
					}
					else
					{
						_action->result = "STR_NO_USES_LEFT";
					}
				}
				else
				{
					_game->pushState(new MedikitState(targetUnit, _action, tileEngine));
				}
			}
			else
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
				_game->popState();
			}
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_SCANNER)
		{
			// spend TUs first, then show the scanner
			if (_action->spendTU(&_action->result))
			{
				_game->popState();
				_game->pushState (new ScannerState(_action));
			}
			else
			{
				_game->popState();
			}
		}
		else if (_action->type == BA_LAUNCH)
		{
			// check beforehand if we have enough time units
			if (!_action->haveTU(&_action->result))
			{
				//nothing
			}
			else if (!_action->weapon->getAmmoForAction(BA_LAUNCH, &_action->result))
			{
				//nothing
			}
			else
			{
				_action->targeting = true;
				newHitLog = true;
			}
			_game->popState();
		}
		else if (_action->type == BA_HIT)
		{
			// check beforehand if we have enough time units
			if (!_action->haveTU(&_action->result))
			{
				//nothing
			}
			else if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
				_action->actor->getPosition(),
				_action->actor->getDirection(),
				_action->actor,
				0, &_action->target))
			{
				if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validTerrainMeleeRange(_action))
				{
					_action->result = "STR_THERE_IS_NO_ONE_THERE";
				}
			}
			else
			{
				newHitLog = true;
			}
			_game->popState();
		}
		else
		{
			_action->targeting = true;
			newHitLog = true;
			_game->popState();
		}

		// meleeAttackBState won't be available to clear the action type, do it here instead.
		if (_action->type == BA_HIT && !_action->result.empty())
		{
			_action->type = BA_NONE;
		}

		if (newHitLog)
		{
			_game->getSavedGame()->getSavedBattle()->appendToHitLog(HITLOG_PLAYER_FIRING, FACTION_PLAYER, tr(weapon->getType())); // Joper HITLOG
		}
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void ActionMenuState::resize(int &dX, int &dY)
{
	State::recenter(dX, dY * 2);
}

}
