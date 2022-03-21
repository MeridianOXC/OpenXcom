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
#include "BattlescapeGame.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "Camera.h"
#include "NextTurnState.h"
#include "BattleState.h"
#include "UnitTurnBState.h"
#include "UnitWalkBState.h"
#include "ProjectileFlyBState.h"
#include "MeleeAttackBState.h"
#include "PsiAttackBState.h"
#include "ExplosionBState.h"
#include "TileEngine.h"
#include "UnitInfoState.h"
#include "UnitDieBState.h"
#include "UnitPanicBState.h"
#include "AIModule.h"
#include "Pathfinding.h"
#include "../Mod/AlienDeployment.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Sound.h"
#include "../Mod/Mod.h"
#include "../Interface/Cursor.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/Armor.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "InfoboxState.h"
#include "InfoboxOKState.h"
#include "UnitFallBState.h"
#include "../Engine/Logger.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "ConfirmEndMissionState.h"
#include "../fmath.h"

namespace OpenXcom
{

bool BattlescapeGame::_debugPlay = false;

/**
 * Update value of TU and Energy
 */
void BattleActionCost::updateTU()
{
	if (actor && skillRules)
	{
		*(RuleItemUseCost*)this = actor->getActionTUs(type, skillRules);
	}
	else if (actor && weapon)
	{
		*(RuleItemUseCost*)this = actor->getActionTUs(type, weapon);
	}
	else
	{
		clearTU();
	}
}

/**
 * Clean up action cost.
 */
void BattleActionCost::clearTU()
{
	*(RuleItemUseCost*)this = RuleItemUseCost();
}

/**
 * Test if action can be performed.
 * @param message optional message with error condition.
 * @return Unit have enough stats to perform action.
 */
bool BattleActionCost::haveTU(std::string *message)
{
	if (!skillRules && Time <= 0)
	{
		//no action, no message
		return false;
	}
	if (actor->getTimeUnits() < Time)
	{
		if (message)
		{
			*message = "STR_NOT_ENOUGH_TIME_UNITS";
		}
		return false;
	}
	if (actor->getEnergy() < Energy)
	{
		if (message)
		{
			*message = "STR_NOT_ENOUGH_ENERGY";
		}
		return false;
	}
	if (actor->getMorale() < Morale)
	{
		if (message)
		{
			*message = "STR_NOT_ENOUGH_MORALE";
		}
		return false;
	}
	if (actor->getHealth() <= Health)
	{
		if (message)
		{
			*message = "STR_NOT_ENOUGH_HEALTH";
		}
		return false;
	}
	if (actor->getMana() < Mana)
	{
		if (message)
		{
			*message = "STR_NOT_ENOUGH_MANA";
		}
		return false;
	}
	if (actor->getHealth() - actor->getStunlevel() <= Stun + Health)
	{
		if (message)
		{
			*message = "STR_NOT_ENOUGH_STUN";
		}
		return false;
	}
	return true;
}

/**
 * Spend cost of action if unit have enough stats.
 * @param message optional message with error condition.
 * @return Action was performed.
 */
bool BattleActionCost::spendTU(std::string *message)
{
	if (haveTU(message))
	{
		actor->spendCost(*this);
		return true;
	}
	return false;
}

/**
 * Initializes all the elements in the Battlescape screen.
 * @param save Pointer to the save game.
 * @param parentState Pointer to the parent battlescape state.
 */
BattlescapeGame::BattlescapeGame(SavedBattleGame *save, BattlescapeState *parentState) :
	_save(save), _parentState(parentState),
	_playerPanicHandled(true), _AIActionCounter(0), _AISecondMove(false), _playedAggroSound(false),
	_endTurnRequested(false), _endConfirmationHandled(false), _allEnemiesNeutralized(false)
{
	if (_save->isPreview())
	{
		_allEnemiesNeutralized = true; // just in case
	}

	_currentAction.actor = 0;
	_currentAction.targeting = false;
	_currentAction.type = BA_NONE;
	_currentAction.skillRules = nullptr;

	_debugPlay = false;

	checkForCasualties(nullptr, BattleActionAttack{ }, true);
	cancelCurrentAction();
}


/**
 * Delete BattlescapeGame.
 */
BattlescapeGame::~BattlescapeGame()
{
	for (std::list<BattleState*>::iterator i = _states.begin(); i != _states.end(); ++i)
	{
		delete *i;
	}
	cleanupDeleted();
}

/**
 * Checks for units panicking or falling and so on.
 */
void BattlescapeGame::think()
{
	// nothing is happening - see if we need some alien AI or units panicking or what have you
	if (_states.empty())
	{
		if (_save->getUnitsFalling())
		{
			statePushFront(new UnitFallBState(this));
			_save->setUnitsFalling(false);
			return;
		}
		// it's a non player side (ALIENS or CIVILIANS)
		if (_save->getSide() != FACTION_PLAYER && _save->getSide() != FACTION_ALIEN_PLAYER) // Joper could be not working properly
		{
			_save->resetUnitHitStates();
			if (!_debugPlay) //( JOPER - this Handles the selection of units)
			{
				if (_save->getSelectedUnit())
				{
					if (!handlePanickingUnit(_save->getSelectedUnit()))
						handleAI(_save->getSelectedUnit());
				}
				else
				{
					if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
					{
						if (!_save->getDebugMode())
						{
							_endTurnRequested = true;
							statePushBack(0); // end AI turn
						}
						else
						{
							_save->selectNextPlayerUnit();
							_debugPlay = true;
						}
					}
				}
			}
		}
		else
		{
			// it's a player side && we have not handled all panicking units
			if (!_playerPanicHandled)
			{
				_playerPanicHandled = handlePanickingPlayer();
				_save->getBattleState()->updateSoldierInfo();
			}
		}
	}
}

/**
 * Initializes the Battlescape game.
 */
void BattlescapeGame::init()
{
	if (_save->getSide() == FACTION_PLAYER && _save->getTurn() > 1)
	{
		_playerPanicHandled = false; // BUG: PLAYER TURN 2, can't use UI button
	}
}


/**
 * Handles the processing of the AI states of a unit.
 * @param unit Pointer to a unit.
 */
void BattlescapeGame::handleAI(BattleUnit *unit)
{
	std::ostringstream ss;

	if (unit->getTimeUnits() <= 5)
	{
		unit->dontReselect();
	}
	if (_AIActionCounter >= 2 || !unit->reselectAllowed() || unit->getTurnsSinceStunned() == 0) //stun check for restoring OXC behavior that AI does not attack after waking up even having full TU
	{
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}
		if (_save->getSelectedUnit())
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
		_AIActionCounter = 0;
		return;
	}

	unit->setVisible(false); //Possible TODO: check number of player unit observers, then hide the unit if no one can see it. Should then be able to skip the next FOV call.

	_save->getTileEngine()->calculateFOV(unit->getPosition(), 1, false); // might need this populate _visibleUnit for a newly-created alien.
		// it might also help chryssalids realize they've zombified someone and need to move on
		// it should also hide units when they've killed the guy spotting them
		// it's also for good luck

	AIModule *ai = unit->getAIModule();
	if (!ai)
	{
		_parentState->debug("Idle");
		_AIActionCounter = 0;
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}
		if (_save->getSelectedUnit())
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
	}
	if (_AIActionCounter == 1)
	{
		_parentState->debug("Idle");
		_AIActionCounter = 0;
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}
		if (_save->getSelectedUnit())
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
	}

	BattleAction action;
	action.actor = unit;
	action.number = _AIActionCounter;
	unit->think(&action);

	if (action.type == BA_RETHINK)
	{
		_parentState->debug("Rethink");
		unit->think(&action);
	}

	_AIActionCounter = action.number;
	BattleItem *weapon = unit->getMainHandWeapon();
	bool pickUpWeaponsMoreActively = unit->getPickUpWeaponsMoreActively();
	bool weaponPickedUp = false;
	bool walkToItem = false;
	if (!weapon || !weapon->haveAnyAmmo())
	{
		_parentState->debug("Idle");
		_AIActionCounter = 0;
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}
		if (_save->getSelectedUnit())
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
	}
	if (action.type == BA_WALK)
	{
		_parentState->debug("Idle");
		_AIActionCounter = 0;
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}
		if (_save->getSelectedUnit())
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
	}

	if (action.type == BA_SNAPSHOT || action.type == BA_AUTOSHOT || action.type == BA_AIMEDSHOT || action.type == BA_THROW || action.type == BA_HIT || action.type == BA_MINDCONTROL || action.type == BA_USE || action.type == BA_PANIC || action.type == BA_LAUNCH)
	{
		_parentState->debug("Idle");
		_AIActionCounter = 0;
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}
		if (_save->getSelectedUnit())
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
	}

	if (action.type == BA_NONE)
	{
		_parentState->debug("Idle");
		_AIActionCounter = 0;
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}
		if (_save->getSelectedUnit())
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
	}
}

/**
 * Toggles the Kneel/Standup status of the unit.
 * @param bu Pointer to a unit.
 * @return If the action succeeded.
 */
bool BattlescapeGame::kneel(BattleUnit *bu)
{
	int tu = bu->getKneelChangeCost();
	if (bu->getArmor()->allowsKneeling(bu->getType() == "SOLDIER") && !bu->isFloating() && ((!bu->isKneeled() && _save->getKneelReserved()) || checkReservedTU(bu, tu, 0)))
	{
		BattleAction kneel;
		kneel.type = BA_KNEEL;
		kneel.actor = bu;
		kneel.Time = tu;
		if (kneel.spendTU())
		{
			bu->kneel(!bu->isKneeled());
			// kneeling or standing up can reveal new terrain or units. I guess.
			getTileEngine()->calculateFOV(bu->getPosition(), 1, false); //Update unit FOV for everyone through this position, skip tiles.
			_parentState->updateSoldierInfo(); //This also updates the tile FOV of the unit, hence why it's skipped above.
			getTileEngine()->checkReactionFire(bu, kneel);
			return true;
		}
		else
		{
			_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
		}
	}
	return false;
}

/**
 * Ends the turn.
 */
void BattlescapeGame::endTurn()
{
	_debugPlay = _save->getDebugMode() && _parentState->getGame()->isCtrlPressed() && (_save->getSide() != FACTION_NEUTRAL);
	_currentAction.type = BA_NONE;
	_currentAction.skillRules = nullptr;
	getMap()->getWaypoints()->clear();
	_currentAction.waypoints.clear();
	_parentState->showLaunchButton(false);
	_currentAction.targeting = false;
	_AISecondMove = false;

	if (_triggerProcessed.tryRun())
	{
		if (_save->getTileEngine()->closeUfoDoors() && Mod::SLIDING_DOOR_CLOSE != -1)
		{
			getMod()->getSoundByDepth(_save->getDepth(), Mod::SLIDING_DOOR_CLOSE)->play(); // ufo door closed
		}

		// if all grenades explode we remove items that expire on that turn too.
		std::vector<std::tuple<BattleItem*, ExplosionBState*>> forRemoval;
		bool exploded = false;

		// check for hot grenades on the ground
		if (_save->getSide() != FACTION_NEUTRAL && !_save->isPreview())
		{
			for (BattleItem *item : *_save->getItems())
			{
				if (item->isOwnerIgnored())
				{
					continue;
				}

				const RuleItem *rule = item->getRules();
				const Tile *tile = item->getTile();
				BattleUnit *unit = item->getOwner();
				if (!tile && unit && rule->isExplodingInHands() && !_allEnemiesNeutralized)
				{
					tile = unit->getTile();
				}
				if (tile)
				{
					if (item->fuseTimeEvent())
					{
						if (rule->getBattleType() == BT_GRENADE) // it's a grenade to explode now
						{
							Position p = tile->getPosition().toVoxel() + Position(8, 8, -tile->getTerrainLevel() + (unit ? unit->getHeight() / 2 : 0));
							forRemoval.push_back(std::tuple(nullptr, new ExplosionBState(this, p, BattleActionAttack::GetBeforeShoot(BA_TRIGGER_TIMED_GRENADE, unit, item))));
							exploded = true;
						}
						else
						{
							forRemoval.push_back(std::tuple(item, nullptr));
						}
					}
				}
			}
			for (auto& p : forRemoval)
			{
				BattleItem* item = std::get<BattleItem*>(p);
				ExplosionBState* expl = std::get<ExplosionBState*>(p);
				if (expl)
				{
					statePushNext(expl);
				}
				else if (item->isSpecialWeapon())
				{
					// we can't remove special weapons, disable the fuse at least
					item->setFuseTimer(-1);
				}
				else
				{
					_save->removeItem(item);
				}
			}
			if (exploded)
			{
				statePushBack(0);
				return;
			}
		}
	}

	// check for terrain explosions
	Tile *t = _save->getTileEngine()->checkForTerrainExplosions();
	if (t)
	{
		Position p = t->getPosition().toVoxel();
		statePushNext(new ExplosionBState(this, p, BattleActionAttack{ }, t));
		statePushBack(0);
		return;
	}

	if (_endTurnProcessed.tryRun())
	{
		if (_save->getSide() != FACTION_NEUTRAL)
		{
			for (BattleItem *item : *_save->getItems())
			{
				if (item->isOwnerIgnored())
				{
					continue;
				}
				item->fuseEndTurnUpdate();
			}
		}


		_save->endTurn();
		t = _save->getTileEngine()->checkForTerrainExplosions();
		if (t)
		{
			Position p = t->getPosition().toVoxel();
			statePushNext(new ExplosionBState(this, p, BattleActionAttack{ }, t));
			statePushBack(0);
			return;
		}
	}

	_triggerProcessed.reset();
	_endTurnProcessed.reset();

	if ((_save->getSide() == FACTION_PLAYER) || (_save->getSide() == FACTION_ALIEN_PLAYER))
	{
		setupCursor();
	}
	else
	{
		getMap()->setCursorType(CT_NONE);
	}
	// JOPER HOST 
	//if ((_save->getSide() == FACTION_PLAYER) || (_save->getSide() == FACTION_ALIEN_PLAYER))
	//{
	//	setupCursor();
	//}
	//else
	//{
	//	getMap()->setCursorType(CT_NONE);
	//}
	// Joper CLIENT
	//if (_save->getSide() == FACTION_ALIEN_PLAYER)
	//{
	//	setupCursor();
	//}
	//else
	//{
	//	getMap()->setCursorType(CT_NONE);
	//}

	checkForCasualties(nullptr, BattleActionAttack{ }, false, false);

	// fires could have been started, stopped or smoke could reveal/conceal units.
	_save->getTileEngine()->calculateLighting(LL_FIRE, TileEngine::invalid, 0, true);
	_save->getTileEngine()->recalculateFOV();

	// Calculate values
	auto tally = _save->getBattleGame()->tallyUnits();

	// if all units from either faction are killed - the mission is over.
	if (_save->allObjectivesDestroyed() && _save->getObjectiveType() == MUST_DESTROY)
	{
		_parentState->finishBattle(false, tally.liveSoldiers);
		return;
	}
	if (_save->getTurnLimit() > 0 && _save->getTurn() > _save->getTurnLimit())
	{
		switch (_save->getChronoTrigger())
		{
		case FORCE_ABORT:
			_save->setAborted(true);
			_parentState->finishBattle(true, tally.inExit);
			return;
		case FORCE_WIN:
		case FORCE_WIN_SURRENDER:
			_parentState->finishBattle(false, tally.liveSoldiers);
			return;
		case FORCE_LOSE:
		default:
			// force mission failure
			_save->setAborted(true);
			_parentState->finishBattle(false, 0);
			return;
		}
	}

	if (tally.liveAliens > 0 && tally.liveSoldiers > 0)
	{
		showInfoBoxQueue();

		_parentState->updateSoldierInfo();

		if (playableUnitSelected())
		{
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			setupCursor();
		}
	}

	// "escort the VIPs" missions don't end when all aliens are neutralized
	// objective type MUST_DESTROY was already handled above
	bool killingAllAliensIsNotEnough = (_save->getVIPSurvivalPercentage() > 0 && _save->getVIPEscapeType() != ESCAPE_NONE);

	bool battleComplete = (!killingAllAliensIsNotEnough && tally.liveAliens == 0) || tally.liveSoldiers == 0;

	if ((_save->getSide() != FACTION_NEUTRAL || battleComplete)
		&& _endTurnRequested)
	{
		_parentState->getGame()->pushState(new NextTurnState(_save, _parentState));
	}
	_endTurnRequested = false;
}


/**
 * Checks for casualties and adjusts morale accordingly.
 * @param damageType Need to know this, for a HE explosion there is an instant death.
 * @param attack This is needed for credits for the kill.
 * @param hiddenExplosion Set to true for the explosions of UFO Power sources at start of battlescape.
 * @param terrainExplosion Set to true for the explosions of terrain.
 */
void BattlescapeGame::checkForCasualties(const RuleDamageType *damageType, BattleActionAttack attack, bool hiddenExplosion, bool terrainExplosion)
{
	auto origMurderer = attack.attacker;
	// If the victim was killed by the murderer's death explosion, fetch who killed the murderer and make HIM the murderer!
	if (origMurderer && (origMurderer->getSpecialAbility() == SPECAB_EXPLODEONDEATH || origMurderer->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
		&& origMurderer->getStatus() == STATUS_DEAD && origMurderer->getMurdererId() != 0)
	{
		for (std::vector<BattleUnit*>::const_iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
		{
			if ((*i)->getId() == origMurderer->getMurdererId())
			{
				origMurderer = (*i);
				break;
			}
		}
	}

	// Fetch the murder weapon
	std::string tempWeapon = "STR_WEAPON_UNKNOWN", tempAmmo = "STR_WEAPON_UNKNOWN";
	if (origMurderer)
	{
		if (attack.weapon_item)
		{
			tempWeapon = attack.weapon_item->getRules()->getName();
		}
		if (attack.damage_item)
		{
			tempAmmo = attack.damage_item->getRules()->getName();
		}
	}

	for (std::vector<BattleUnit*>::iterator j = _save->getUnits()->begin(); j != _save->getUnits()->end(); ++j)
	{
		if ((*j)->isIgnored()) continue;
		BattleUnit *victim = (*j);
		BattleUnit *murderer = origMurderer;

		BattleUnitKills killStat;
		killStat.mission = _parentState->getGame()->getSavedGame()->getMissionStatistics()->size();
		killStat.setTurn(_save->getTurn(), _save->getSide());
		killStat.setUnitStats(victim);
		killStat.faction = victim->getOriginalFaction();
		killStat.side = victim->getFatalShotSide();
		killStat.bodypart = victim->getFatalShotBodyPart();
		killStat.id = victim->getId();
		killStat.weapon = tempWeapon;
		killStat.weaponAmmo = tempAmmo;

		// Determine murder type
		if ((*j)->getStatus() != STATUS_DEAD)
		{
			if ((*j)->getHealth() <= 0)
			{
				killStat.status = STATUS_DEAD;
			}
			else if ((*j)->getStunlevel() >= (*j)->getHealth() && (*j)->getStatus() != STATUS_UNCONSCIOUS)
			{
				killStat.status = STATUS_UNCONSCIOUS;
			}
		}

		// Assume that, in absence of a murderer and an explosion, the laster unit to hit the victim is the murderer.
		// Possible causes of death: bleed out, fire.
		// Possible causes of unconsciousness: wounds, smoke.
		// Assumption : The last person to hit the victim is the murderer.
		if (!murderer && !terrainExplosion)
		{
			for (std::vector<BattleUnit*>::const_iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
			{
				if ((*i)->getId() == victim->getMurdererId())
				{
					murderer = (*i);
					killStat.weapon = victim->getMurdererWeapon();
					killStat.weaponAmmo = victim->getMurdererWeaponAmmo();
					break;
				}
			}
		}

		if (murderer && killStat.status != STATUS_IGNORE_ME)
		{
			if (murderer->getFaction() == FACTION_PLAYER && murderer->getOriginalFaction() != FACTION_PLAYER)
			{
				// This must be a mind controlled unit. Find out who mind controlled him and award the kill to that unit.
				for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
				{
					if ((*i)->getId() == murderer->getMindControllerId() && (*i)->getGeoscapeSoldier())
					{
						if (!victim->isCosmetic())
						{
							(*i)->getStatistics()->kills.push_back(new BattleUnitKills(killStat));
							if (victim->getFaction() == FACTION_HOSTILE || victim->getFaction() == FACTION_ALIEN_PLAYER)
							{
								(*i)->getStatistics()->slaveKills++;
							}
						}
						victim->setMurdererId((*i)->getId());
						break;
					}
				}
			}
			else if (!murderer->getStatistics()->duplicateEntry(killStat.status, victim->getId()))
			{
				if (!victim->isCosmetic())
				{
					murderer->getStatistics()->kills.push_back(new BattleUnitKills(killStat));
				}
				victim->setMurdererId(murderer->getId());
			}
		}

		bool noSound = false;
		if ((*j)->getStatus() != STATUS_DEAD)
		{
			if ((*j)->getHealth() <= 0)
			{
				int moraleLossModifierWhenKilled = _save->getMoraleLossModifierWhenKilled(victim);

				if (murderer)
				{
					murderer->addKillCount();
					victim->killedBy(murderer->getFaction());
					int modifier = murderer->getFaction() == FACTION_PLAYER ? _save->getFactionMoraleModifier(true) : 100;

					// if there is a known murderer, he will get a morale bonus if he is of a different faction (what with neutral?)
					if ((victim->getOriginalFaction() == FACTION_PLAYER && murderer->getFaction() == FACTION_HOSTILE) ||
						(victim->getOriginalFaction() == FACTION_HOSTILE && murderer->getFaction() == FACTION_PLAYER) ||
						(victim->getOriginalFaction() == FACTION_PLAYER && murderer->getFaction() == FACTION_ALIEN_PLAYER) ||
						(victim->getOriginalFaction() == FACTION_ALIEN_PLAYER && murderer->getFaction() == FACTION_PLAYER))
					{
						murderer->moraleChange(20 * modifier / 100);
					}
					// murderer will get a penalty with friendly fire
					if (victim->getOriginalFaction() == murderer->getOriginalFaction())
					{
						// morale loss by friendly fire
						murderer->moraleChange(-(2000 * moraleLossModifierWhenKilled / modifier / 100));
					}
					if (victim->getOriginalFaction() == FACTION_NEUTRAL)
					{
						if (murderer->getOriginalFaction() == FACTION_PLAYER)
						{
							// morale loss by xcom killing civilians
							murderer->moraleChange(-(1000 * moraleLossModifierWhenKilled / modifier / 100));
						}
						else
						{
							murderer->moraleChange(10);
						}
					}
				}

				if (victim->getFaction() != FACTION_NEUTRAL)
				{
					int modifier = _save->getUnitMoraleModifier(victim);
					int loserMod = _save->getFactionMoraleModifier(victim->getOriginalFaction() != FACTION_HOSTILE & victim->getOriginalFaction() != FACTION_ALIEN_PLAYER);
					int winnerMod = _save->getFactionMoraleModifier(victim->getOriginalFaction() == FACTION_HOSTILE || victim->getOriginalFaction() == FACTION_ALIEN_PLAYER);
					for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
					{
						if (!(*i)->isOut() && (*i)->getArmor()->getSize() == 1)
						{
							// the losing squad all get a morale loss
							if ((*i)->getOriginalFaction() == victim->getOriginalFaction())
							{
								// morale loss by losing a team member (not counting mind-controlled units)
								int bravery = (*i)->reduceByBravery(10);
								(*i)->moraleChange(-(modifier * moraleLossModifierWhenKilled * 200 * bravery / loserMod / 100 / 100));

								if (victim->getFaction() == FACTION_HOSTILE && murderer)
								{
									murderer->setTurnsSinceSpotted(0);
								}
							}
							// the winning squad all get a morale increase
							else
							{
								(*i)->moraleChange(10 * winnerMod / 100);
							}
						}
					}
				}
				if (damageType)
				{
					statePushNext(new UnitDieBState(this, (*j), damageType, noSound));
				}
				else
				{
					if (hiddenExplosion)
					{
						// this is instant death from UFO power sources, without screaming sounds
						noSound = true;
						statePushNext(new UnitDieBState(this, (*j), getMod()->getDamageType(DT_HE), noSound));
					}
					else
					{
						if (terrainExplosion)
						{
							// terrain explosion
							statePushNext(new UnitDieBState(this, (*j), getMod()->getDamageType(DT_HE), noSound));
						}
						else
						{
							// no murderer, and no terrain explosion, must be fatal wounds
							statePushNext(new UnitDieBState(this, (*j), getMod()->getDamageType(DT_NONE), noSound));  // DT_NONE = STR_HAS_DIED_FROM_A_FATAL_WOUND
						}
					}
				}
				// one of our own died, record the murderer instead of the victim
				if (victim->getGeoscapeSoldier())
				{
					victim->getStatistics()->KIA = true;
					BattleUnitKills *deathStat = new BattleUnitKills(killStat);
					if (murderer)
					{
						deathStat->setUnitStats(murderer);
						deathStat->faction = murderer->getOriginalFaction();
					}
					_parentState->getGame()->getSavedGame()->killSoldier(false, victim->getGeoscapeSoldier(), deathStat);
				}
			}
			else if ((*j)->getStunlevel() >= (*j)->getHealth() && (*j)->getStatus() != STATUS_UNCONSCIOUS)
			{
				// morale change when an enemy is stunned (only for the first time!)
				if (getMod()->getStunningImprovesMorale() && murderer && !victim->getStatistics()->wasUnconcious)
				{
					if ((victim->getOriginalFaction() == FACTION_PLAYER && murderer->getFaction() == FACTION_HOSTILE) ||
						(victim->getOriginalFaction() == FACTION_HOSTILE && murderer->getFaction() == FACTION_PLAYER) ||
						(victim->getOriginalFaction() == FACTION_PLAYER && murderer->getFaction() == FACTION_ALIEN_PLAYER) ||
						(victim->getOriginalFaction() == FACTION_ALIEN_PLAYER && murderer->getFaction() == FACTION_PLAYER))
					{
						// the murderer gets a morale bonus if he is of a different faction (excluding neutrals)
						murderer->moraleChange(20);

						for (auto winner : *_save->getUnits())
						{
							if (!winner->isOut() && winner->getArmor()->getSize() == 1 && winner->getOriginalFaction() == murderer->getOriginalFaction())
							{
								// the winning squad gets a morale increase (the losing squad is NOT affected)
								winner->moraleChange(10);
							}
						}
					}
				}

				victim->getStatistics()->wasUnconcious = true;
				noSound = true;
				statePushNext(new UnitDieBState(this, (*j), getMod()->getDamageType(DT_NONE), noSound)); // no damage type used there
			}
		}
	}

	BattleUnit *bu = _save->getSelectedUnit();
	if (_save->getSide() == FACTION_PLAYER)
	{
		_parentState->resetUiButton();

		if (bu && !bu->isOut())
		{
			_parentState->updateUiButton(bu);
		}
	}
	if (_save->getSide() == FACTION_ALIEN_PLAYER)
	{
		_parentState->resetUiButton();

		if (bu && !bu->isOut())
		{
			_parentState->updateUiButton(bu);
		}
	}
}

/**
 * Shows the infoboxes in the queue (if any).
 */
void BattlescapeGame::showInfoBoxQueue()
{
	for (std::vector<InfoboxOKState*>::iterator i = _infoboxQueue.begin(); i != _infoboxQueue.end(); ++i)
	{
		_parentState->getGame()->pushState(*i);
	}

	_infoboxQueue.clear();
}

/**
 * Sets up a mission complete notification.
 */
void BattlescapeGame::missionComplete()
{
	Game *game = _parentState->getGame();
	if (game->getMod()->getDeployment(_save->getMissionType()))
	{
		std::string missionComplete = game->getMod()->getDeployment(_save->getMissionType())->getObjectivePopup();
		if (!missionComplete.empty())
		{
			_infoboxQueue.push_back(new InfoboxOKState(game->getLanguage()->getString(missionComplete)));
		}
	}
}

/**
 * Handles the result of non target actions, like priming a grenade.
 */
void BattlescapeGame::handleNonTargetAction()
{
	if (!_currentAction.targeting)
	{
		std::string error;
		_currentAction.cameraPosition = Position(0,0,-1);
		if (!_currentAction.result.empty())
		{
			_parentState->warning(_currentAction.result);
			_currentAction.result = "";
		}
		else if (_currentAction.type == BA_PRIME && _currentAction.value > -1)
		{
			if (_currentAction.spendTU(&error))
			{
				_parentState->warning(_currentAction.weapon->getRules()->getPrimeActionMessage());
				_currentAction.weapon->setFuseTimer(_currentAction.value);
				playSound(_currentAction.weapon->getRules()->getPrimeSound()); // prime sound
				_save->getTileEngine()->calculateLighting(LL_UNITS, _currentAction.actor->getPosition());
				_save->getTileEngine()->calculateFOV(_currentAction.actor->getPosition(), _currentAction.weapon->getVisibilityUpdateRange(), false);
			}
			else
			{
				_parentState->warning(error);
			}
		}
		else if (_currentAction.type == BA_UNPRIME)
		{
			if (_currentAction.spendTU(&error))
			{
				_parentState->warning(_currentAction.weapon->getRules()->getUnprimeActionMessage());
				_currentAction.weapon->setFuseTimer(-1);
				playSound(_currentAction.weapon->getRules()->getUnprimeSound()); // unprime sound
				_save->getTileEngine()->calculateLighting(LL_UNITS, _currentAction.actor->getPosition());
				_save->getTileEngine()->calculateFOV(_currentAction.actor->getPosition(), _currentAction.weapon->getVisibilityUpdateRange(), false);
			}
			else
			{
				_parentState->warning(error);
			}
		}
		else if (_currentAction.type == BA_USE)
		{
			getTileEngine()->updateGameStateAfterScript(BattleActionAttack::GetBeforeShoot(_currentAction), TileEngine::invalid);
		}
		else if (_currentAction.type == BA_HIT)
		{
			if (_currentAction.haveTU(&error))
			{
				statePushBack(new MeleeAttackBState(this, _currentAction));
			}
			else
			{
				_parentState->warning(error);
			}
		}
		if (_currentAction.type != BA_HIT) // don't clear the action type if we're meleeing, let the melee action state take care of that
		{
			_currentAction.type = BA_NONE;
		}
		_parentState->updateSoldierInfo();
	}

	setupCursor();
}

/**
 * Sets the cursor according to the selected action.
 */
void BattlescapeGame::setupCursor()
{
	if (_currentAction.targeting)
	{
		if (_currentAction.type == BA_THROW)
		{
			getMap()->setCursorType(CT_THROW);
		}
		else if (_currentAction.type == BA_MINDCONTROL || _currentAction.type == BA_PANIC || _currentAction.type == BA_USE)
		{
			getMap()->setCursorType(CT_PSI);
		}
		else if (_currentAction.type == BA_LAUNCH)
		{
			getMap()->setCursorType(CT_WAYPOINT);
		}
		else
		{
			getMap()->setCursorType(CT_AIM);
		}
	}
	else if (_currentAction.type != BA_HIT)
	{
		_currentAction.actor = _save->getSelectedUnit();
		if (_currentAction.actor)
		{
			getMap()->setCursorType(CT_NORMAL, _currentAction.actor->getArmor()->getSize());
		}
		else
		{
			getMap()->setCursorType(CT_NORMAL);
		}
	}
}

/**
 * Determines whether a playable unit is selected. Normally only player side units can be selected, but in debug mode one can play with aliens too :)
 * Is used to see if stats can be displayed.
 * @return Whether a playable unit is selected.
 */
bool BattlescapeGame::playableUnitSelected() const
{
	return _save->getSelectedUnit() != 0 && (_save->getSide() == FACTION_ALIEN_PLAYER || _save->getSide() == FACTION_PLAYER || _save->getDebugMode());
	// JOPPER FOR CLIENT 
	// return _save->getSelectedUnit() != 0 && (_save->getSide() == FACTION_ALIEN_PLAYER);
}

/**
 * Gives time slice to the front state.
 */
void BattlescapeGame::handleState()
{
	if (!_states.empty())
	{
		// end turn request?
		if (_states.front() == 0)
		{
			_states.pop_front();
			endTurn();
			return;
		}
		else
		{
			_states.front()->think();
		}
		getMap()->invalidate(); // redraw map
	}
}

/**
 * Pushes a state to the front of the queue and starts it.
 * @param bs Battlestate.
 */
void BattlescapeGame::statePushFront(BattleState *bs)
{
	_states.push_front(bs);
	bs->init();
}

/**
 * Pushes a state as the next state after the current one.
 * @param bs Battlestate.
 */
void BattlescapeGame::statePushNext(BattleState *bs)
{
	if (_states.empty())
	{
		_states.push_front(bs);
		bs->init();
	}
	else
	{
		_states.insert(++_states.begin(), bs);
	}

}

/**
 * Pushes a state to the back.
 * @param bs Battlestate.
 */
void BattlescapeGame::statePushBack(BattleState *bs)
{
	if (_states.empty())
	{
		_states.push_front(bs);
		// end turn request?
		if (_states.front() == 0)
		{
			_states.pop_front();
			endTurn();
			return;
		}
		else
		{
			bs->init();
		}
	}
	else
	{
		_states.push_back(bs);
	}
}

/**
 * Removes the current state.
 *
 * This is a very important function. It is called by a BattleState (walking, projectile is flying, explosions,...) at the moment this state has finished its action.
 * Here we check the result of that action and do all the aftermath.
 * The state is popped off the list.
 */
void BattlescapeGame::popState()
{
	if (Options::traceAI)
	{
		Log(LOG_INFO) << "BattlescapeGame::popState() #" << _AIActionCounter << " with " << (_save->getSelectedUnit() ? _save->getSelectedUnit()->getTimeUnits() : -9999) << " TU";
	}
	bool actionFailed = false;

	if (_states.empty()) return;

	auto first = _states.front();
	BattleAction action = first->getAction();

	if (
		(action.actor && !action.result.empty() && action.actor->getFaction() == FACTION_PLAYER
		&& _playerPanicHandled && (_save->getSide() == FACTION_PLAYER || _debugPlay)) || //jopper
		(action.actor && !action.result.empty() && action.actor->getFaction() == FACTION_ALIEN_PLAYER
		&& _playerPanicHandled && (_save->getSide() == FACTION_ALIEN_PLAYER || _debugPlay))
		) 
	{
		_parentState->warning(action.result);
		actionFailed = true;
	}
	_deleted.push_back(first);
	_states.pop_front();
	first->deinit();

	// handle the end of this unit's actions
	if (action.actor && noActionsPending(action.actor))
	{
		if ((action.actor->getFaction() == FACTION_PLAYER)||(action.actor->getFaction() == FACTION_ALIEN_PLAYER))
		{
			if ((_save->getSide() == FACTION_PLAYER)||(_save->getSide() == FACTION_ALIEN_PLAYER))
			{
				// after throwing the cursor returns to default cursor, after shooting it stays in targeting mode and the player can shoot again in the same mode (autoshot,snap,aimed)
				if ((action.type == BA_THROW || action.type == BA_LAUNCH) && !actionFailed)
				{
					// clean up the waypoints
					if (action.type == BA_LAUNCH)
					{
						_currentAction.waypoints.clear();
					}

					cancelCurrentAction(true);
				}
				_parentState->getGame()->getCursor()->setVisible(true);
				setupCursor();
			}
		}
		else
		{
			if (_save->getSide() != FACTION_PLAYER && _save->getSide() != FACTION_ALIEN_PLAYER && !_debugPlay)
			{
				// AI does three things per unit, before switching to the next, or it got killed before doing the second thing
				if (_AIActionCounter > 2 || _save->getSelectedUnit() == 0 || _save->getSelectedUnit()->isOut())
				{
					_AIActionCounter = 0;
					if (_states.empty() && _save->selectNextPlayerUnit(true) == 0)
					{
						if (!_save->getDebugMode())
						{
							_endTurnRequested = true;
							statePushBack(0); // end AI turn
						}
						else
						{
							_save->selectNextPlayerUnit();
							_debugPlay = true;
						}
					}
					if (_save->getSelectedUnit())
					{
						getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
					}
				}
			}
			else if (_debugPlay)
			{
				_parentState->getGame()->getCursor()->setVisible(true);
				setupCursor();
			}
		}
	}

	if (!_states.empty())
	{
		// end turn request?
		if (_states.front() == 0)
		{
			while (!_states.empty())
			{
				if (_states.front() == 0)
					_states.pop_front();
				else
					break;
			}
			if (_states.empty())
			{
				endTurn();
				return;
			}
			else
			{
				_states.push_back(0);
			}
		}
		// init the next state in queue
		_states.front()->init();
	}

	// the currently selected unit died or became unconscious or disappeared inexplicably
	if (_save->getSelectedUnit() == 0 || _save->getSelectedUnit()->isOut())
	{
		cancelCurrentAction();
		getMap()->setCursorType(CT_NORMAL, 1);
		_parentState->getGame()->getCursor()->setVisible(true);
		if (_save->getSide() == FACTION_PLAYER || _save->getSide() == FACTION_ALIEN_PLAYER)
			_save->setSelectedUnit(0);
		else
			_save->selectNextPlayerUnit(true, true);
	}
	_parentState->updateSoldierInfo();
}

/**
 * Determines whether there are any actions pending for the given unit.
 * @param bu BattleUnit.
 * @return True if there are no actions pending.
 */
bool BattlescapeGame::noActionsPending(BattleUnit *bu)
{
	if (_states.empty()) return true;

	for (std::list<BattleState*>::iterator i = _states.begin(); i != _states.end(); ++i)
	{
		if ((*i) != 0 && (*i)->getAction().actor == bu)
			return false;
	}

	return true;
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval An interval in ms.
 */
void BattlescapeGame::setStateInterval(Uint32 interval)
{
	_parentState->setStateInterval(interval);
}


/**
 * Checks against reserved time units and energy units.
 * @param bu Pointer to the unit.
 * @param tu Number of time units to check.
 * @param energy Number of energy units to check.
 * @param justChecking True to suppress error messages, false otherwise.
 * @return bool Whether or not we got enough time units.
 */
bool BattlescapeGame::checkReservedTU(BattleUnit *bu, int tu, int energy, bool justChecking)
{
	BattleActionCost cost;
	cost.actor = bu;
	cost.type = _save->getTUReserved(); // avoid changing _tuReserved in this method
	cost.weapon = bu->getMainHandWeapon(false); // check TUs against slowest weapon if we have two weapons

	if (_save->getSide() != bu->getFaction() || _save->getSide() == FACTION_NEUTRAL)
	{
		return tu <= bu->getTimeUnits();
	}

	if (_save->getSide() == FACTION_HOSTILE && !_debugPlay) // aliens reserve TUs as a percentage rather than just enough for a single action.
	{
		AIModule *ai = bu->getAIModule();
		if (ai)
		{
			cost.type = ai->getReserveMode();
		}
		cost.updateTU();
		cost.Energy += energy;
		cost.Time = tu; //override original
		switch (cost.type)
		{
		case BA_SNAPSHOT: cost.Time += (bu->getBaseStats()->tu / 3); break; // 33%
		case BA_AUTOSHOT: cost.Time += ((bu->getBaseStats()->tu / 5)*2); break; // 40%
		case BA_AIMEDSHOT: cost.Time += (bu->getBaseStats()->tu / 2); break; // 50%
		default: break;
		}
		return cost.Time <= 0 || cost.haveTU();
	}

	cost.updateTU();
	// if the weapon has no autoshot, reserve TUs for snapshot
	if (cost.Time == 0 && cost.type == BA_AUTOSHOT)
	{
		cost.type = BA_SNAPSHOT;
		cost.updateTU();
	}
	// likewise, if we don't have a snap shot available, try aimed.
	if (cost.Time == 0 && cost.type == BA_SNAPSHOT)
	{
		cost.type = BA_AIMEDSHOT;
		cost.updateTU();
	}
	const int tuKneel = (_save->getKneelReserved() && !bu->isKneeled()  && bu->getArmor()->allowsKneeling(bu->getType() == "SOLDIER")) ? bu->getKneelDownCost() : 0;
	// no aimed shot available? revert to none.
	if (cost.Time == 0 && cost.type == BA_AIMEDSHOT)
	{
		if (tuKneel > 0)
		{
			cost.type = BA_KNEEL;
		}
		else
		{
			return true;
		}
	}

	cost.Time += tuKneel;

	//current TU is less that required for reserved shoot, we can't reserved anything.
	if (!cost.haveTU() && !justChecking)
	{
		return true;
	}

	cost.Time += tu;
	cost.Energy += energy;

	if ((cost.type != BA_NONE || _save->getKneelReserved()) && !cost.haveTU())
	{
		if (!justChecking)
		{
			if (tuKneel)
			{
				switch (cost.type)
				{
				case BA_KNEEL: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING"); break;
				default: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING_AND_FIRING");
				}
			}
			else
			{
				switch (_save->getTUReserved())
				{
				case BA_SNAPSHOT: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_SNAP_SHOT"); break;
				case BA_AUTOSHOT: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AUTO_SHOT"); break;
				case BA_AIMEDSHOT: _parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AIMED_SHOT"); break;
				default: ;
				}
			}
		}
		return false;
	}

	return true;
}



/**
 * Picks the first soldier that is panicking.
 * @return True when all panicking is over.
 */
bool BattlescapeGame::handlePanickingPlayer()
{
	for (std::vector<BattleUnit*>::iterator j = _save->getUnits()->begin(); j != _save->getUnits()->end(); ++j)
	{
		if ((((*j)->getFaction() == FACTION_PLAYER && (*j)->getOriginalFaction() == FACTION_PLAYER) || ((*j)->getFaction() == FACTION_ALIEN_PLAYER && (*j)->getOriginalFaction() == FACTION_ALIEN_PLAYER)) && handlePanickingUnit(*j))
			return false;
	}
	return true;
}

/**
 * Common function for handling panicking units.
 * @return False when unit not in panicking mode.
 */
bool BattlescapeGame::handlePanickingUnit(BattleUnit *unit)
{
	UnitStatus status = unit->getStatus();
	if (status != STATUS_PANICKING && status != STATUS_BERSERK) return false;
	_save->setSelectedUnit(unit);
	_parentState->getMap()->setCursorType(CT_NONE);

	// play panic/berserk sounds first
	bool soundPlayed = false;
	{
		std::vector<int> sounds;
		if (unit->getUnitRules())
		{
			// aliens, civilians, xcom HWPs
			if (status == STATUS_PANICKING)
				sounds = unit->getUnitRules()->getPanicSounds();
			else
				sounds = unit->getUnitRules()->getBerserkSounds();
		}
		else if (unit->getGeoscapeSoldier())
		{
			// xcom soldiers (male/female)
			if (unit->getGeoscapeSoldier()->getGender() == GENDER_MALE)
			{
				if (status == STATUS_PANICKING)
					sounds = unit->getGeoscapeSoldier()->getRules()->getMalePanicSounds();
				else
					sounds = unit->getGeoscapeSoldier()->getRules()->getMaleBerserkSounds();
			}
			else
			{
				if (status == STATUS_PANICKING)
					sounds = unit->getGeoscapeSoldier()->getRules()->getFemalePanicSounds();
				else
					sounds = unit->getGeoscapeSoldier()->getRules()->getFemaleBerserkSounds();
			}
		}
		if (!sounds.empty())
		{
			soundPlayed = true;
			if (sounds.size() > 1)
				playSound(sounds[RNG::generate(0, sounds.size() - 1)]);
			else
				playSound(sounds.front());
		}
	}

	// show a little infobox with the name of the unit and "... is panicking"
	Game *game = _parentState->getGame();
	if (unit->getVisible() || !Options::noAlienPanicMessages)
	{
		getMap()->getCamera()->centerOnPosition(unit->getPosition());
		if (status == STATUS_PANICKING)
		{
			game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_PANICKED", unit->getGender()).arg(unit->getName(game->getLanguage()))));
		}
		else
		{
			game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_GONE_BERSERK", unit->getGender()).arg(unit->getName(game->getLanguage()))));
		}
	}
	else if (soundPlayed)
	{
		// simulate a small pause by using an invisible infobox
		game->pushState(new InfoboxState(""));
	}


	int flee = RNG::generate(0,100);
	BattleAction ba;
	ba.actor = unit;
	if (status == STATUS_PANICKING && flee <= 50) // 1/2 chance to freeze and 1/2 chance try to flee, STATUS_BERSERK is handled in the panic state.
	{
		BattleItem *item = unit->getRightHandWeapon();
		if (item)
		{
			dropItem(unit->getPosition(), item, true);
		}
		item = unit->getLeftHandWeapon();
		if (item)
		{
			dropItem(unit->getPosition(), item, true);
		}
		// let's try a few times to get a tile to run to.
		for (int i= 0; i < 20; i++)
		{
			ba.target = Position(unit->getPosition().x + RNG::generate(-5,5), unit->getPosition().y + RNG::generate(-5,5), unit->getPosition().z);

			if (i >= 10 && ba.target.z > 0) // if we've had more than our fair share of failures, try going down.
			{
				ba.target.z--;
				if (i >= 15 && ba.target.z > 0) // still failing? try further down.
				{
					ba.target.z--;
				}
			}
			if (_save->getTile(ba.target)) // sanity check the tile.
			{
				_save->getPathfinding()->calculate(ba.actor, ba.target, ba.getMoveType());
				if (_save->getPathfinding()->getStartDirection() != -1) // sanity check the path.
				{
					statePushBack(new UnitWalkBState(this, ba));
					break;
				}
			}
		}
	}
	// Time units can only be reset after everything else occurs
	statePushBack(new UnitPanicBState(this, ba.actor));

	return true;
}

/**
  * Cancels the current action the user had selected (firing, throwing,..)
  * @param bForce Force the action to be cancelled.
  * @return Whether an action was cancelled or not.
  */
bool BattlescapeGame::cancelCurrentAction(bool bForce)
{
	bool bPreviewed = Options::battleNewPreviewPath != PATH_NONE;

	if (_save->getPathfinding()->removePreview() && bPreviewed) return true;

	if (_states.empty() || bForce)
	{
		if (_currentAction.targeting)
		{
			if (_currentAction.type == BA_LAUNCH && !_currentAction.waypoints.empty())
			{
				_currentAction.waypoints.pop_back();
				if (!getMap()->getWaypoints()->empty())
				{
					getMap()->getWaypoints()->pop_back();
				}
				if (_currentAction.waypoints.empty())
				{
					_parentState->showLaunchButton(false);
				}
				return true;
			}
			else if (_currentAction.type == BA_AUTOSHOT && _currentAction.sprayTargeting && !_currentAction.waypoints.empty())
			{
				_currentAction.waypoints.pop_back();
				if (!getMap()->getWaypoints()->empty())
				{
					getMap()->getWaypoints()->pop_back();
				}

				if (_currentAction.waypoints.empty())
				{
					_currentAction.sprayTargeting = false;
					getMap()->getWaypoints()->clear();
				}
				return true;
			}
			else
			{
				if (Options::battleConfirmFireMode && !_currentAction.waypoints.empty())
				{
					_currentAction.waypoints.pop_back();
					getMap()->getWaypoints()->pop_back();
					return true;
				}
				_currentAction.targeting = false;
				_currentAction.type = BA_NONE;
				_currentAction.skillRules = nullptr;
				setupCursor();
				_parentState->getGame()->getCursor()->setVisible(true);
				return true;
			}
		}
	}
	else if (!_states.empty() && _states.front() != 0)
	{
		_states.front()->cancel();
		return true;
	}

	return false;
}

/**
  * Cancels all selected user actions.
  */
void BattlescapeGame::cancelAllActions()
{
	_save->getPathfinding()->removePreview();

	_currentAction.waypoints.clear();
	getMap()->getWaypoints()->clear();
	_parentState->showLaunchButton(false);

	_currentAction.targeting = false;
	_currentAction.type = BA_NONE;
	_currentAction.skillRules = nullptr;
	setupCursor();
	_parentState->getGame()->getCursor()->setVisible(true);
}

/**
 * Gets a pointer to access action members directly.
 * @return Pointer to action.
 */
BattleAction *BattlescapeGame::getCurrentAction()
{
	return &_currentAction;
}

/**
 * Determines whether an action is currently going on?
 * @return true or false.
 */
bool BattlescapeGame::isBusy() const
{
	return !_states.empty();
}

/**
 * Activates primary action (left click).
 * @param pos Position on the map.
 */
void BattlescapeGame::primaryAction(Position pos)
{
	bool bPreviewed = Options::battleNewPreviewPath != PATH_NONE;

	getMap()->resetObstacles();

	if (_currentAction.targeting && _save->getSelectedUnit())
	{
		if (_currentAction.type == BA_LAUNCH)
		{
			int maxWaypoints = _currentAction.weapon->getCurrentWaypoints();
			if ((int)_currentAction.waypoints.size() < maxWaypoints || maxWaypoints == -1)
			{
				_parentState->showLaunchButton(true);
				_currentAction.waypoints.push_back(pos);
				getMap()->getWaypoints()->push_back(pos);
			}
		}
		else if (_currentAction.sprayTargeting) // Special "spray" auto shot that allows placing shots between waypoints
		{
			int maxWaypoints = _currentAction.weapon->getRules()->getSprayWaypoints();
			if ((int)_currentAction.waypoints.size() >= maxWaypoints ||
				(_save->isCtrlPressed(true) && _save->isShiftPressed(true)) ||
				(!Options::battleConfirmFireMode && (int)_currentAction.waypoints.size() == maxWaypoints - 1))
			{
				// If we're firing early, pick one last waypoint.
				if ((int)_currentAction.waypoints.size() < maxWaypoints)
				{
					_currentAction.waypoints.push_back(pos);
					getMap()->getWaypoints()->push_back(pos);
				}

				getMap()->setCursorType(CT_NONE);

				// Populate the action's waypoints with the positions we want to fire at
				// Start from the last shot and move to the first, since we'll be using the last element first and then pop_back()
				int numberOfShots = _currentAction.weapon->getRules()->getConfigAuto()->shots;
				int numberOfWaypoints = _currentAction.waypoints.size();
				_currentAction.waypoints.clear();
				for (int i = numberOfShots - 1; i > 0; --i)
				{
					// Evenly space shots along the waypoints according to number of waypoints and the number of shots
					// Use voxel positions to get more uniform spacing
					// We add Position(8, 8, 12) to target middle of tile
					int waypointIndex = std::max(0, std::min(numberOfWaypoints - 1, i * (numberOfWaypoints - 1) / (numberOfShots - 1)));
					Position previousWaypoint = getMap()->getWaypoints()->at(waypointIndex).toVoxel() + TileEngine::voxelTileCenter;
					Position nextWaypoint = getMap()->getWaypoints()->at(std::min((int)getMap()->getWaypoints()->size() - 1, waypointIndex + 1)).toVoxel() + TileEngine::voxelTileCenter;
					Position targetPos;
					targetPos.x = previousWaypoint.x + (nextWaypoint.x - previousWaypoint.x) * (i * (numberOfWaypoints - 1) % (numberOfShots - 1)) / (numberOfShots - 1);
					targetPos.y = previousWaypoint.y + (nextWaypoint.y - previousWaypoint.y) * (i * (numberOfWaypoints - 1) % (numberOfShots - 1)) / (numberOfShots - 1);
					targetPos.z = previousWaypoint.z + (nextWaypoint.z - previousWaypoint.z) * (i * (numberOfWaypoints - 1) % (numberOfShots - 1)) / (numberOfShots - 1);

					_currentAction.waypoints.push_back(targetPos);
				}
				_currentAction.waypoints.push_back(getMap()->getWaypoints()->front().toVoxel() + TileEngine::voxelTileCenter);
				_currentAction.target = _currentAction.waypoints.back().toTile();

				getMap()->getWaypoints()->clear();
				_parentState->getGame()->getCursor()->setVisible(false);
				_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
				_states.push_back(new ProjectileFlyBState(this, _currentAction));
				statePushFront(new UnitTurnBState(this, _currentAction));
				_currentAction.sprayTargeting = false;
				_currentAction.waypoints.clear();
			}
			else if ((int)_currentAction.waypoints.size() < maxWaypoints)
			{
				_currentAction.waypoints.push_back(pos);
				getMap()->getWaypoints()->push_back(pos);
			}
		}
		else if (_currentAction.type == BA_AUTOSHOT &&
			_currentAction.weapon->getRules()->getSprayWaypoints() > 0 &&
			_save->isCtrlPressed(true) &&
			_save->isShiftPressed(true) &&
			_currentAction.waypoints.empty()) // Starts the spray autoshot targeting
		{
			_currentAction.sprayTargeting = true;
			_currentAction.waypoints.push_back(pos);
			getMap()->getWaypoints()->push_back(pos);
		}
		else if (_currentAction.type == BA_USE && _currentAction.weapon->getRules()->getBattleType() == BT_MINDPROBE)
		{
			auto targetUnit = _save->selectUnit(pos);
			if (targetUnit && targetUnit->getFaction() != _save->getSelectedUnit()->getFaction() && targetUnit->getVisible())
			{
				if (!_currentAction.weapon->getRules()->isLOSRequired() ||
					(_currentAction.actor->getFaction() == FACTION_PLAYER && targetUnit->getFaction() != FACTION_HOSTILE && targetUnit->getFaction() != FACTION_ALIEN_PLAYER) ||
					std::find(_currentAction.actor->getVisibleUnits()->begin(), _currentAction.actor->getVisibleUnits()->end(), targetUnit) != _currentAction.actor->getVisibleUnits()->end())
				{
					std::string error;
					if (_currentAction.spendTU(&error))
					{
						_parentState->getGame()->getMod()->getSoundByDepth(_save->getDepth(), _currentAction.weapon->getRules()->getHitSound())->play(-1, getMap()->getSoundAngle(pos));
						_parentState->getGame()->pushState (new UnitInfoState(targetUnit, _parentState, false, true));
						cancelCurrentAction();
					}
					else
					{
						_parentState->warning(error);
					}
				}
				else
				{
					_parentState->warning("STR_LINE_OF_SIGHT_REQUIRED");
				}
			}
		}
		else if ((_currentAction.type == BA_PANIC || _currentAction.type == BA_MINDCONTROL || _currentAction.type == BA_USE) && _currentAction.weapon->getRules()->getBattleType() == BT_PSIAMP)
		{
			auto targetUnit = _save->selectUnit(pos);
			if (targetUnit && targetUnit->getVisible())
			{
				auto targetFaction = targetUnit->getFaction();
				bool psiTargetAllowed = _currentAction.weapon->getRules()->isTargetAllowed(targetFaction);
				if ((_currentAction.actor->getFaction() == FACTION_PLAYER && _currentAction.type == BA_MINDCONTROL && targetFaction == FACTION_PLAYER)
					|| (_currentAction.actor->getFaction() == FACTION_ALIEN_PLAYER && _currentAction.type == BA_MINDCONTROL && targetFaction == FACTION_ALIEN_PLAYER)) // JOPER DOUBLE CHECK IF ALINS/x-com UNIT MIND CONTROL THEIR OWN UNITS BUG
				{
					// no mind controlling allies, unwanted side effects
					psiTargetAllowed = false;
				}
				else if (_currentAction.type == BA_PANIC && targetUnit->getUnitRules() && !targetUnit->getUnitRules()->canPanic())
				{
					psiTargetAllowed = false;
				}
				else if (_currentAction.type == BA_MINDCONTROL && targetUnit->getUnitRules() && !targetUnit->getUnitRules()->canBeMindControlled())
				{
					psiTargetAllowed = false;
				}
				if (psiTargetAllowed)
				{
					_currentAction.updateTU();
					_currentAction.target = pos;
					if (!_currentAction.weapon->getRules()->isLOSRequired() ||
						// old line for controllnig AI units
						// (_currentAction.actor->getFaction() == FACTION_PLAYER && targetFaction != FACTION_HOSTILE) || // Joper. No idea WTF is this. Related to PSIONICKS BUG. A later note. Should allow player factions to mind control eacho other
						// new lines begin
						((_currentAction.actor->getFaction() == FACTION_PLAYER && targetFaction != FACTION_ALIEN_PLAYER) ||
						(_currentAction.actor->getFaction() == FACTION_ALIEN_PLAYER && targetFaction != FACTION_PLAYER)) ||
						std::find(_currentAction.actor->getVisibleUnits()->begin(), _currentAction.actor->getVisibleUnits()->end(), targetUnit) != _currentAction.actor->getVisibleUnits()->end())
					{
						// get the sound/animation started
						getMap()->setCursorType(CT_NONE);
						_parentState->getGame()->getCursor()->setVisible(false);
						_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
						statePushBack(new PsiAttackBState(this, _currentAction));
					}
					else
					{
						_parentState->warning("STR_LINE_OF_SIGHT_REQUIRED");
					}
				}
			}
		}
		else if (Options::battleConfirmFireMode && (_currentAction.waypoints.empty() || pos != _currentAction.waypoints.front()))
		{
			_currentAction.waypoints.clear();
			_currentAction.waypoints.push_back(pos);
			getMap()->getWaypoints()->clear();
			getMap()->getWaypoints()->push_back(pos);
		}
		else
		{
			_currentAction.target = pos;
			getMap()->setCursorType(CT_NONE);

			if (Options::battleConfirmFireMode)
			{
				_currentAction.waypoints.clear();
				getMap()->getWaypoints()->clear();
			}

			_parentState->getGame()->getCursor()->setVisible(false);
			_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
			_states.push_back(new ProjectileFlyBState(this, _currentAction));
			statePushFront(new UnitTurnBState(this, _currentAction)); // first of all turn towards the target
		}
	}
	else
	{
		_currentAction.actor = _save->getSelectedUnit();
		BattleUnit *unit = _save->selectUnit(pos);
		if (unit && unit == _save->getSelectedUnit() && (unit->getVisible() || _debugPlay))
		{
			playUnitResponseSound(unit, 3); // "annoyed" sound
		}
		if (unit && unit != _save->getSelectedUnit() && (unit->getVisible() || _debugPlay))
		{
		//  -= select unit =-
			if (unit->getFaction() == _save->getSide())
			{
				_save->setSelectedUnit(unit);
				_parentState->updateSoldierInfo();
				cancelCurrentAction();
				setupCursor();
				_currentAction.actor = unit;
				playUnitResponseSound(unit, 0); // "select unit" sound
			}
		}
		else if (playableUnitSelected())
		{
			bool isCtrlPressed = Options::strafe && _save->isCtrlPressed(true);
			bool isShiftPressed = _save->isShiftPressed(true);
			if (bPreviewed &&
				(_currentAction.target != pos || (_save->getPathfinding()->isModifierUsed() != isCtrlPressed)))
			{
				_save->getPathfinding()->removePreview();
			}
			_currentAction.target = pos;
			_save->getPathfinding()->calculate(_currentAction.actor, _currentAction.target, BAM_NORMAL); // precalucalte move

			_currentAction.strafe = false;
			_currentAction.run = false;
			if (isCtrlPressed)
			{
				if (_save->getPathfinding()->getPath().size() > 1)
				{
					_currentAction.run = _save->getSelectedUnit()->getArmor()->allowsRunning(_save->getSelectedUnit()->getArmor()->getSize() == 1);
				}
				else
				{
					_currentAction.strafe = _save->getSelectedUnit()->getArmor()->allowsStrafing(_save->getSelectedUnit()->getArmor()->getSize() == 1);
				}
			}

			//recalucate path after setting new move types
			if (BAM_NORMAL != _currentAction.getMoveType())
			{
				_save->getPathfinding()->calculate(_currentAction.actor, _currentAction.target, _currentAction.getMoveType());
			}

			// if running or shifting, ignore spotted enemies (i.e. don't stop)
			_currentAction.ignoreSpottedEnemies = (_currentAction.run && Mod::EXTENDED_RUNNING_COST) || isShiftPressed;

			if (bPreviewed && !_save->getPathfinding()->previewPath() && _save->getPathfinding()->getStartDirection() != -1)
			{
				_save->getPathfinding()->removePreview();
				bPreviewed = false;
			}

			if (!bPreviewed && _save->getPathfinding()->getStartDirection() != -1)
			{
				//  -= start walking =-
				getMap()->setCursorType(CT_NONE);
				_parentState->getGame()->getCursor()->setVisible(false);
				statePushBack(new UnitWalkBState(this, _currentAction));
				playUnitResponseSound(_currentAction.actor, 1); // "start moving" sound
			}
		}
	}
}

/**
 * Activates secondary action (right click).
 * @param pos Position on the map.
 */
void BattlescapeGame::secondaryAction(Position pos)
{
	//  -= turn to or open door =-
	_currentAction.target = pos;
	_currentAction.actor = _save->getSelectedUnit();
	_currentAction.strafe = Options::strafe && _save->isCtrlPressed(true) && _save->getSelectedUnit()->getTurretType() > -1;
	statePushBack(new UnitTurnBState(this, _currentAction));
}

/**
 * Handler for the blaster launcher button.
 */
void BattlescapeGame::launchAction()
{
	_parentState->showLaunchButton(false);
	getMap()->getWaypoints()->clear();
	_currentAction.target = _currentAction.waypoints.front();
	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);
	_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
	_states.push_back(new ProjectileFlyBState(this, _currentAction));
	statePushFront(new UnitTurnBState(this, _currentAction)); // first of all turn towards the target
}

/**
 * Handler for the psi button.
 */
void BattlescapeGame::psiButtonAction()
{
	if (!_currentAction.waypoints.empty()) // in case waypoints were set with a blaster launcher, avoid accidental misclick
		return;
	BattleItem *item = _save->getSelectedUnit()->getSpecialWeapon(BT_PSIAMP);
	_currentAction.type = BA_NONE;
    if ((item->getRules()->getCostMind().Time > 0) & _save->isCtrlPressed(true))
	{
		_currentAction.type = BA_MINDCONTROL;
	}
	else if (item->getRules()->getCostPanic().Time > 0)
	{
		_currentAction.type = BA_PANIC;
	}
	else if (item->getRules()->getCostUse().Time > 0)
	{
		_currentAction.type = BA_USE;
	}
	if (_currentAction.type != BA_NONE)
	{
		_currentAction.targeting = true;
		_currentAction.weapon = item;
		_currentAction.updateTU();
		setupCursor();
	}
}

/**
 * Handler for the psi attack result message.
 */
void BattlescapeGame::psiAttackMessage(BattleActionAttack attack, BattleUnit *victim)
{
	if (victim)
	{
		Game *game = getSave()->getBattleState()->getGame();
		if (attack.attacker->getFaction() == FACTION_HOSTILE || attack.attacker->getFaction() == FACTION_ALIEN_PLAYER)
		{
			// show a little infobox with the name of the unit and "... is under alien control"
			if (attack.type == BA_MINDCONTROL)
				game->pushState(new InfoboxState(game->getLanguage()->getString("STR_IS_UNDER_ALIEN_CONTROL", victim->getGender()).arg(victim->getName(game->getLanguage()))));
			else if (attack.type == BA_PANIC)
				game->pushState(new InfoboxState(game->getLanguage()->getString("STR_MORALE_ATTACK_SUCCESSFUL")));
		}
		else
		{
			// show a little infobox if it's successful
			if (attack.type == BA_PANIC)
				game->pushState(new InfoboxState(game->getLanguage()->getString("STR_MORALE_ATTACK_SUCCESSFUL")));
			else if (attack.type == BA_MINDCONTROL)
				game->pushState(new InfoboxState(game->getLanguage()->getString("STR_MIND_CONTROL_SUCCESSFUL")));
			getSave()->getBattleState()->updateSoldierInfo();
		}
	}
}


/**
 * Moves a unit up or down.
 * @param unit The unit.
 * @param dir Direction DIR_UP or DIR_DOWN.
 */
void BattlescapeGame::moveUpDown(BattleUnit *unit, int dir)
{
	_currentAction.target = unit->getPosition();
	if (dir == Pathfinding::DIR_UP)
	{
		_currentAction.target.z++;
	}
	else
	{
		_currentAction.target.z--;
	}
	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);
	if (_save->getSelectedUnit()->isKneeled())
	{
		kneel(_save->getSelectedUnit());
	}
	_save->getPathfinding()->calculate(_currentAction.actor, _currentAction.target, _currentAction.getMoveType());
	statePushBack(new UnitWalkBState(this, _currentAction));
}

/**
 * Requests the end of the turn (waits for explosions etc to really end the turn).
 */
void BattlescapeGame::requestEndTurn(bool askForConfirmation)
{
	cancelCurrentAction();

	if (askForConfirmation)
	{
		if (_endConfirmationHandled)
			return;

		// check for fatal wounds
		int soldiersWithFatalWounds = 0;
		for (std::vector<BattleUnit*>::iterator it = _save->getUnits()->begin(); it != _save->getUnits()->end(); ++it)
		{
			if ((*it)->getOriginalFaction() == FACTION_PLAYER && (*it)->getStatus() != STATUS_DEAD && (*it)->getFatalWounds() > 0)
				soldiersWithFatalWounds++;
		}

		if (soldiersWithFatalWounds > 0)
		{
			// confirm end of turn/mission
			_parentState->getGame()->pushState(new ConfirmEndMissionState(_save, soldiersWithFatalWounds, this));
			_endConfirmationHandled = true;
		}
		else
		{
			if (!_endTurnRequested)
			{
				_endTurnRequested = true;
				statePushBack(0);
			}
		}
	}
	else
	{
		if (!_endTurnRequested)
		{
			_endTurnRequested = true;
			statePushBack(0);
		}
	}
}

/**
 * Sets the TU reserved type.
 * @param tur A BattleActionType.
 * @param player is this requested by the player?
 */
void BattlescapeGame::setTUReserved(BattleActionType tur)
{
	_save->setTUReserved(tur);
}

/**
 * Drops an item to the floor and affects it with gravity.
 * @param position Position to spawn the item.
 * @param item Pointer to the item.
 * @param newItem Bool whether this is a new item.
 * @param removeItem Bool whether to remove the item from the owner.
 */
void BattlescapeGame::dropItem(Position position, BattleItem *item, bool removeItem, bool updateLight)
{
	_save->getTileEngine()->itemDrop(_save->getTile(position), item, updateLight);
}

/**
 * Converts a unit into a unit of another type.
 * @param unit The unit to convert.
 * @return Pointer to the new unit.
 */
BattleUnit *BattlescapeGame::convertUnit(BattleUnit *unit)
{
	_parentState->resetUiButton();

	return getSave()->convertUnit(unit);
}

/**
 * Spawns a new unit mid-battle
 * @param attack BattleActionAttack that calls to spawn the unit
 * @param position Tile position to try and spawn unit on
 */
void BattlescapeGame::spawnNewUnit(BattleItem *item)
{
	spawnNewUnit(BattleActionAttack{ BA_NONE, nullptr, item, item, }, item->getTile()->getPosition());
}

void BattlescapeGame::spawnNewUnit(BattleActionAttack attack, Position position)
{
	if (!attack.damage_item) // no idea how this happened, but make sure we have an item
		return;

	const RuleItem *item = attack.damage_item->getRules();
	Unit *type = getMod()->getUnit(item->getSpawnUnit(), true);

	// Check which faction the new unit will be
	UnitFaction faction;
	if (item->getSpawnUnitFaction() == -1 && attack.attacker)
	{
		faction = attack.attacker->getFaction();
	}
	else
	{
		switch (item->getSpawnUnitFaction())
		{
			case 0:
				faction = FACTION_PLAYER;
				break;
			case 1:
				faction = FACTION_ALIEN_PLAYER; // Was FACTION_HOSTILE
				break;
			case 2:
				faction = FACTION_NEUTRAL;
				break;
			default:
				faction = FACTION_ALIEN_PLAYER; // was FACTION_HOSTILE (old note)Joper. No need to touch this yet, won't be used in vanilla.Except for zombification perhabs...
				break;
		}
	}

	if (_save->isPreview() && faction != FACTION_PLAYER)
	{
		return;
	}

	// Create the unit
	BattleUnit *newUnit = _save->createTempUnit(type, faction);

	// Validate the position for the unit, checking if there's a surrounding tile if necessary
	int checkDirection = attack.attacker ? (attack.attacker->getDirection() + 4) % 8 : 0;
	bool positionValid = getTileEngine()->isPositionValidForUnit(position, newUnit, true, checkDirection);
	if (positionValid) // Place the unit and initialize it in the battlescape
	{
		int unitDirection = attack.attacker ? attack.attacker->getDirection() : RNG::generate(0, 7);
		// If this is a tank, arm it with its weapon
		if (getMod()->getItem(newUnit->getType()) && getMod()->getItem(newUnit->getType())->isFixed())
		{
			const RuleItem *newUnitWeapon = getMod()->getItem(newUnit->getType());
			if (!_save->isPreview())
			{
				_save->createItemForUnit(newUnitWeapon, newUnit, true);
				if (newUnitWeapon->getVehicleClipAmmo())
				{
					const RuleItem *ammo = newUnitWeapon->getVehicleClipAmmo();
					BattleItem *ammoItem = _save->createItemForUnit(ammo, newUnit);
					if (ammoItem)
					{
						ammoItem->setAmmoQuantity(newUnitWeapon->getVehicleClipSize());
					}
				}
			}
			newUnit->setTurretType(newUnitWeapon->getTurretType());
		}

		// Pick the item sets if the unit has builtInWeaponSets
		auto monthsPassed = _parentState->getGame()->getSavedGame()->getMonthsPassed();
		auto alienItemLevels = getMod()->getAlienItemLevels().size();
		int month;
		if (monthsPassed != -1)
		{
			if ((size_t)monthsPassed > alienItemLevels - 1)
			{
				month = alienItemLevels - 1;
			}
			else
			{
				month = monthsPassed;
			}
		}
		else // For "New Battle" saves
		{
			// We don't have access to the BattlescapeGenerator or the alienItemLevel set on generation at this point, so pick a random one
			month = RNG::generate(0, alienItemLevels - 1);
		}
		size_t itemLevel = (size_t)(getMod()->getAlienItemLevels().at(month).at(RNG::generate(0,9)));

		// Initialize the unit and its position
		newUnit->setTile(_save->getTile(position), _save);
		newUnit->setPosition(position);
		newUnit->setDirection(unitDirection);
		newUnit->clearTimeUnits();
		getSave()->getUnits()->push_back(newUnit);
		bool visible = faction == FACTION_PLAYER;
		newUnit->setVisible(visible);
		getSave()->initUnit(newUnit, itemLevel);

		getTileEngine()->calculateFOV(newUnit->getPosition());  //happens fairly rarely, so do a full recalc for units in range to handle the potential unit visible cache issues.
		getTileEngine()->applyGravity(newUnit->getTile());
	}
	else
	{
		delete newUnit;
	}
}

/**
 * Spawns units from items primed before battle
 */
void BattlescapeGame::spawnFromPrimedItems()
{
	std::vector<BattleItem*> itemsSpawningUnits;

	for (std::vector<BattleItem*>::iterator i = _save->getItems()->begin(); i != _save->getItems()->end(); ++i)
	{
		if ((*i)->isOwnerIgnored() || !(*i)->getTile())
		{
			continue;
		}
		if (!(*i)->getRules()->getSpawnUnit().empty() && !(*i)->getXCOMProperty() && !(*i)->isSpecialWeapon())
		{
			if ((*i)->getRules()->getBattleType() == BT_GRENADE && (*i)->getFuseTimer() == 0 && (*i)->isFuseEnabled())
			{
				itemsSpawningUnits.push_back((*i));
			}
		}
	}

	for (BattleItem *item : itemsSpawningUnits)
	{
		spawnNewUnit(item);
		_save->removeItem(item);
	}
}

/**
 * Removes spawned units that belong to the player to avoid dealing with recovery
 */
void BattlescapeGame::removeSummonedPlayerUnits()
{
	std::vector<Unit*> resummonAsCivilians;

	std::vector<BattleUnit*>::iterator unit = _save->getUnits()->begin();
	while (unit != _save->getUnits()->end())
	{
		if (!(*unit)->isSummonedPlayerUnit())
		{
			++unit;
		}
		else
		{
			if ((*unit)->getStatus() != STATUS_DEAD && (*unit)->getUnitRules())
			{
				if (!(*unit)->getUnitRules()->getCivilianRecoveryType().empty())
				{
					resummonAsCivilians.push_back((*unit)->getUnitRules());
				}
			}

			if ((*unit)->getStatus() == STATUS_UNCONSCIOUS || (*unit)->getStatus() == STATUS_DEAD)
				_save->removeUnconsciousBodyItem((*unit));

			//remove all items form unit
			(*unit)->removeSpecialWeapons(_save);
			auto inv = *(*unit)->getInventory();
			for (auto* bi : inv)
			{
				_save->removeItem(bi);
			}

			(*unit)->setTile(nullptr, _save);
			delete (*unit);
			unit = _save->getUnits()->erase(unit);
		}
	}

	for (auto& type : resummonAsCivilians)
	{
		BattleUnit *newUnit = new BattleUnit(getMod(),
			type,
			FACTION_NEUTRAL,
			_save->getUnits()->back()->getId() + 1,
			_save->getEnviroEffects(),
			type->getArmor(),
			nullptr,
			getDepth());

		// just bare minimum, this unit will never be used for anything except recovery (not even for scoring)
		newUnit->setTile(nullptr, _save);
		newUnit->setPosition(TileEngine::invalid);
		newUnit->markAsResummonedFakeCivilian();
		_save->getUnits()->push_back(newUnit);
	}
}

/**
 * Tally summoned player-controlled VIPs. We may still need to correct this in the Debriefing.
 */
void BattlescapeGame::tallySummonedVIPs()
{
	EscapeType escapeType = _save->getVIPEscapeType();
	for (auto unit : *_save->getUnits())
	{
		if (unit->isVIP() && unit->isSummonedPlayerUnit())
		{
			if (unit->getStatus() == STATUS_DEAD)
			{
				_save->addLostVIP(unit->getValue());
			}
			else if (escapeType == ESCAPE_EXIT)
			{
				if (unit->isInExitArea(END_POINT))
					_save->addSavedVIP(unit->getValue());
				else
					_save->addLostVIP(unit->getValue());
			}
			else if (escapeType == ESCAPE_ENTRY)
			{
				if (unit->isInExitArea(START_POINT))
					_save->addSavedVIP(unit->getValue());
				else
					_save->addLostVIP(unit->getValue());
			}
			else if (escapeType == ESCAPE_EITHER)
			{
				if (unit->isInExitArea(START_POINT) || unit->isInExitArea(END_POINT))
					_save->addSavedVIP(unit->getValue());
				else
					_save->addLostVIP(unit->getValue());
			}
			else //if (escapeType == ESCAPE_NONE)
			{
				if (unit->isInExitArea(START_POINT))
					_save->addSavedVIP(unit->getValue()); // waiting in craft, saved even if aborted
				else
					_save->addWaitingOutsideVIP(unit->getValue()); // waiting outside, lost if aborted
			}
		}
	}
}

/**
 * Gets the map.
 * @return map.
 */
Map *BattlescapeGame::getMap()
{
	return _parentState->getMap();
}

/**
 * Gets the save.
 * @return save.
 */
SavedBattleGame *BattlescapeGame::getSave()
{
	return _save;
}

/**
 * Gets the tile engine.
 * @return tile engine.
 */
TileEngine *BattlescapeGame::getTileEngine()
{
	return _save->getTileEngine();
}

/**
 * Gets the pathfinding.
 * @return pathfinding.
 */
Pathfinding *BattlescapeGame::getPathfinding()
{
	return _save->getPathfinding();
}

/**
 * Gets the mod.
 * @return mod.
 */
Mod *BattlescapeGame::getMod()
{
	return _parentState->getGame()->getMod();
}


/**
 * Tries to find an item and pick it up if possible.
 * @return True if an item was picked up, false otherwise.
 */
bool BattlescapeGame::findItem(BattleAction *action, bool pickUpWeaponsMoreActively, bool& walkToItem)
{
	// terrorists don't have hands.
	if (action->actor->getRankString() != "STR_LIVE_TERRORIST" || pickUpWeaponsMoreActively)
	{
		// pick the best available item
		BattleItem *targetItem = surveyItems(action, pickUpWeaponsMoreActively);
		// make sure it's worth taking
		if (targetItem && worthTaking(targetItem, action, pickUpWeaponsMoreActively))
		{
			// if we're already standing on it...
			if (targetItem->getTile()->getPosition() == action->actor->getPosition())
			{
				// try to pick it up
				if (takeItemFromGround(targetItem, action) == 0)
				{
					// if it isn't loaded or it is ammo
					if (!targetItem->haveAnyAmmo())
					{
						// try to load our weapon
						action->actor->reloadAmmo();
					}
					if (targetItem->getGlow())
					{
						_save->getTileEngine()->calculateLighting(LL_ITEMS, action->actor->getPosition());
						_save->getTileEngine()->calculateFOV(action->actor->getPosition(), targetItem->getVisibilityUpdateRange(), false);
					}
					return true;
				}
			}
			else if (!targetItem->getTile()->getUnit() || targetItem->getTile()->getUnit()->isOut())
			{
				// if we're not standing on it, we should try to get to it.
				action->target = targetItem->getTile()->getPosition();
				action->type = BA_WALK;
				walkToItem = true;
				if (pickUpWeaponsMoreActively)
				{
					// don't end the turn after walking 1-2 tiles... pick up a weapon and shoot!
					action->finalAction = false;
					action->desperate = false;
					action->actor->setHiding(false);
				}
			}
		}
	}
	return false;
}


/**
 * Searches through items on the map that were dropped on an alien turn, then picks the most "attractive" one.
 * @param action A pointer to the action being performed.
 * @return The item to attempt to take.
 */
BattleItem *BattlescapeGame::surveyItems(BattleAction *action, bool pickUpWeaponsMoreActively)
{
	std::vector<BattleItem*> droppedItems;

	// first fill a vector with items on the ground that were dropped on the alien turn, and have an attraction value.
	for (std::vector<BattleItem*>::iterator i = _save->getItems()->begin(); i != _save->getItems()->end(); ++i)
	{
		if ((*i)->isOwnerIgnored())
		{
			continue;
		}

		if ((*i)->getRules()->getAttraction())
		{
			if ((*i)->getTurnFlag() || pickUpWeaponsMoreActively)
			{
				if ((*i)->getSlot() && (*i)->getSlot()->getType() == INV_GROUND && (*i)->getTile() && !(*i)->getTile()->getDangerous())
				{
					droppedItems.push_back(*i);
				}
			}
		}
	}

	BattleItem *targetItem = 0;
	int maxWorth = 0;

	// now select the most suitable candidate depending on attractiveness and distance
	// (are we still talking about items?)
	for (std::vector<BattleItem*>::iterator i = droppedItems.begin(); i != droppedItems.end(); ++i)
	{
		if ((*i)->getTile()->getDangerous())
		{
			continue;
		}
		int currentWorth = (*i)->getRules()->getAttraction() / ((Position::distance2d(action->actor->getPosition(), (*i)->getTile()->getPosition()) * 2)+1);
		if (currentWorth > maxWorth)
		{
			if ((*i)->getTile()->getTUCost(O_OBJECT, action->actor->getMovementType()) == 255)
			{
				// Note: full pathfinding check will be done later, this is just a small optimisation
				(*i)->getTile()->setDangerous(true);
				continue;
			}
			maxWorth = currentWorth;
			targetItem = *i;
		}
	}

	return targetItem;
}


/**
 * Assesses whether this item is worth trying to pick up, taking into account how many units we see,
 * whether or not the Weapon has ammo, and if we have ammo FOR it,
 * or, if it's ammo, checks if we have the weapon to go with it,
 * assesses the attraction value of the item and compares it with the distance to the object,
 * then returns false anyway.
 * @param item The item to attempt to take.
 * @param action A pointer to the action being performed.
 * @return false.
 */
bool BattlescapeGame::worthTaking(BattleItem* item, BattleAction *action, bool pickUpWeaponsMoreActively)
{
	int worthToTake = 0;

	// don't even think about making a move for that gun if you can see a target, for some reason
	// (maybe this should check for enemies spotting the tile the item is on?)
	if (action->actor->getVisibleUnits()->empty() || pickUpWeaponsMoreActively)
	{
		// retrieve an insignificantly low value from the ruleset.
		worthToTake = item->getRules()->getAttraction();

		// it's always going to be worth while to try and take a blaster launcher, apparently
		if (item->getRules()->getBattleType() == BT_FIREARM && item->getCurrentWaypoints() == 0)
		{
			// we only want weapons that HAVE ammo, or weapons that we have ammo FOR
			bool ammoFound = true;
			if (!item->haveAnyAmmo())
			{
				ammoFound = false;
				for (BattleItem *i : *action->actor->getInventory())
				{
					if (i->getRules()->getBattleType() == BT_AMMO)
					{
						if (item->getRules()->getSlotForAmmo(i->getRules()) != -1)
						{
							ammoFound = true;
							break;
						}
					}
				}
			}
			if (!ammoFound)
			{
				return false;
			}
		}

		if (item->getRules()->getBattleType() == BT_AMMO)
		{
			// similar to the above, but this time we're checking if the ammo is suitable for a weapon we have.
			bool weaponFound = false;
			for (BattleItem *i : *action->actor->getInventory())
			{
				if (i->getRules()->getBattleType() == BT_FIREARM)
				{
					if (i->getRules()->getSlotForAmmo(item->getRules()) != -1)
					{
						weaponFound = true;
						break;
					}
				}
			}
			if (!weaponFound)
			{
				return false;
			}
		}
	}

	if (worthToTake)
	{
		// use bad logic to determine if we'll have room for the item
		int freeSlots = 25;
		for (std::vector<BattleItem*>::iterator i = action->actor->getInventory()->begin(); i != action->actor->getInventory()->end(); ++i)
		{
			freeSlots -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
		}
		int size = item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth();
		if (freeSlots < size)
		{
			return false;
		}
	}

	if (pickUpWeaponsMoreActively)
	{
		// Note: always true, the item must have passed this test already in surveyItems()
		return worthToTake > 0;
	}

	// return false for any item that we aren't standing directly on top of with an attraction value less than 6 (aka always)
	return (worthToTake - (Position::distance2d(action->actor->getPosition(), item->getTile()->getPosition())*2)) > 5;
}


/**
 * Picks the item up from the ground.
 *
 * At this point we've decided it's worth our while to grab this item, so we try to do just that.
 * First we check to make sure we have time units, then that we have space (using horrifying logic)
 * then we attempt to actually recover the item.
 * @param item The item to attempt to take.
 * @param action A pointer to the action being performed.
 * @return 0 if successful, 1 for no TUs, 2 for not enough room, 3 for "won't fit" and -1 for "something went horribly wrong".
 */
int BattlescapeGame::takeItemFromGround(BattleItem* item, BattleAction *action)
{
	const int success = 0;
	const int notEnoughTimeUnits = 1;
	const int notEnoughSpace = 2;
	const int couldNotFit = 3;
	int freeSlots = 25;

	// make sure we have time units
	if (action->actor->getTimeUnits() < 6)
	{
		return notEnoughTimeUnits;
	}
	else
	{
		// check to make sure we have enough space by checking all the sizes of items in our inventory
		for (std::vector<BattleItem*>::iterator i = action->actor->getInventory()->begin(); i != action->actor->getInventory()->end(); ++i)
		{
			freeSlots -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
		}
		if (freeSlots < item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth())
		{
			return notEnoughSpace;
		}
		else
		{
			// check that the item will fit in our inventory, and if so, take it
			if (takeItem(item, action))
			{
				return success;
			}
			else
			{
				return couldNotFit;
			}
		}
	}
}


/**
 * Tries to fit an item into the unit's inventory, return false if you can't.
 * @param item The item to attempt to take.
 * @param action A pointer to the action being performed.
 * @return Whether or not the item was successfully retrieved.
 */
bool BattlescapeGame::takeItem(BattleItem* item, BattleAction *action)
{
	bool placed = false;
	Mod *mod = _parentState->getGame()->getMod();
	auto rightWeapon = action->actor->getRightHandWeapon();
	auto leftWeapon = action->actor->getLeftHandWeapon();
	auto unit = action->actor;

	auto reloadWeapon = [&unit](BattleItem* weapon, BattleItem* i)
	{
		if (weapon && weapon->isWeaponWithAmmo() && !weapon->haveAllAmmo())
		{
			auto slot = weapon->getRules()->getSlotForAmmo(i->getRules());
			if (slot != -1)
			{
				BattleActionCost cost{ unit };
				cost.Time += Mod::EXTENDED_ITEM_RELOAD_COST ? i->getSlot()->getCost(weapon->getSlot()) : 0;
				cost.Time += weapon->getRules()->getTULoad(slot);
				if (cost.haveTU() && !weapon->getAmmoForSlot(slot))
				{
					weapon->setAmmoForSlot(slot, i);
					cost.spendTU();
					return true;
				}
			}
		}
		return false;
	};

	auto equipItem = [&unit](RuleInventory *slot, BattleItem* i)
	{
		BattleActionCost cost{ unit };
		cost.Time += i->getSlot()->getCost(slot);
		if (cost.haveTU() && unit->fitItemToInventory(slot, i))
		{
			cost.spendTU();
			return true;
		}
		return false;
	};

	switch (item->getRules()->getBattleType())
	{
	case BT_AMMO:
		// find equipped weapons that can be loaded with this ammo
		if (reloadWeapon(rightWeapon, item))
		{
			placed = true;
		}
		else if (reloadWeapon(leftWeapon, item))
		{
			placed = true;
		}
		else
		{
			placed = equipItem(mod->getInventoryBelt(), item);
		}
		break;
	case BT_GRENADE:
	case BT_PROXIMITYGRENADE:
		placed = equipItem(mod->getInventoryBelt(), item);
		break;
	case BT_FIREARM:
	case BT_MELEE:
		if (!rightWeapon)
		{
			placed = equipItem(mod->getInventoryRightHand(), item);
		}
		break;
	case BT_MEDIKIT:
	case BT_SCANNER:
		placed = equipItem(mod->getInventoryBackpack(), item);
		break;
	case BT_MINDPROBE:
		if (!leftWeapon)
		{
			placed = equipItem(mod->getInventoryLeftHand(), item);
		}
		break;
	default: break;
	}
	return placed;
}

/**
 * Returns the action type that is reserved.
 * @return The type of action that is reserved.
 */
BattleActionType BattlescapeGame::getReservedAction()
{
	return _save->getTUReserved();
}

bool BattlescapeGame::isSurrendering(BattleUnit* bu)
{
	// if we already decided to surrender this turn, don't change our decision (until next turn)
	if (bu->isSurrendering())
	{
		return true;
	}

	int surrenderMode = getMod()->getSurrenderMode();

	// auto-surrender (e.g. units, which won't fight without their masters/controllers)
	if (surrenderMode > 0 && bu->getUnitRules()->autoSurrender())
	{
		bu->setSurrendering(true);
		return true;
	}

	// surrender under certain conditions
	if (surrenderMode == 0)
	{
		// turned off, no surrender
	}
	else if (surrenderMode == 1)
	{
		// all remaining enemy units can surrender and want to surrender now
		if (bu->getUnitRules()->canSurrender() && (bu->getStatus() == STATUS_PANICKING || bu->getStatus() == STATUS_BERSERK))
		{
			bu->setSurrendering(true);
		}
	}
	else if (surrenderMode == 2)
	{
		// all remaining enemy units can surrender and want to surrender now or wanted to surrender in the past
		if (bu->getUnitRules()->canSurrender() && bu->wantsToSurrender())
		{
			bu->setSurrendering(true);
		}
	}
	else if (surrenderMode == 3)
	{
		// all remaining enemy units have empty hands and want to surrender now or wanted to surrender in the past
		if (!bu->getLeftHandWeapon() && !bu->getRightHandWeapon() && bu->wantsToSurrender())
		{
			bu->setSurrendering(true);
		}
	}

	return bu->isSurrendering();
}

/**
 * Tallies the living units in the game and, if required, converts units into their spawn unit.
 */
BattlescapeTally BattlescapeGame::tallyUnits()
{
	BattlescapeTally tally = { };

	for (std::vector<BattleUnit*>::iterator j = _save->getUnits()->begin(); j != _save->getUnits()->end(); ++j)
	{
		//TODO: add handling of stunned units for display purposes in AbortMissionState
		if (!(*j)->isOut() && (!(*j)->isOutThresholdExceed() || ((*j)->getUnitRules() && (*j)->getUnitRules()->getSpawnUnit())))
		{
			if (((*j)->getOriginalFaction() == FACTION_HOSTILE) || (*j)->getOriginalFaction() == FACTION_ALIEN_PLAYER) // JOPER. POSSIBLY A BUG. THIS LINE SHOULD NOT WORK BUT IT WORKS.
			{
				if (Options::allowPsionicCapture && (*j)->getFaction() == FACTION_PLAYER && (*j)->getCapturable())
				{
					// don't count psi-captured units
				}
				else if (isSurrendering((*j)) && (*j)->getCapturable())
				{
					// don't count surrendered units
				}
				else
				{
					tally.liveAliens++;
				}
			}
			else if ((*j)->getOriginalFaction() == FACTION_PLAYER)
			{
				if ((*j)->isSummonedPlayerUnit())
				{
					if ((*j)->isVIP())
					{
						// used only for display purposes in AbortMissionState
						// count only player-controlled VIPs, not civilian VIPs!
						if ((*j)->isInExitArea(START_POINT))
						{
							tally.vipInEntrance++;
						}
						else if ((*j)->isInExitArea(END_POINT))
						{
							tally.vipInExit++;
						}
						else
						{
							tally.vipInField++;
						}
					}
					continue;
				}

				if ((*j)->isInExitArea(START_POINT))
				{
					tally.inEntrance++;
				}
				else if ((*j)->isInExitArea(END_POINT))
				{
					tally.inExit++;
				}
				else
				{
					tally.inField++;
				}

				if ((*j)->getFaction() == FACTION_PLAYER)
				{
					tally.liveSoldiers++;
				}
				else
				{
					tally.liveAliens++;
				}
			}
		}
	}

	return tally;
}

bool BattlescapeGame::convertInfected()
{
	bool retVal = false;
	std::vector<BattleUnit*> forTransform;
	for (BattleUnit* i : *_save->getUnits())
	{
		if (!i->isOutThresholdExceed() && i->getRespawn())
		{
			retVal = true;
			i->setRespawn(false);
			if (Options::battleNotifyDeath && i->getFaction() == FACTION_PLAYER)
			{
				Game *game = _parentState->getGame();
				game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_BEEN_KILLED", i->getGender()).arg(i->getName(game->getLanguage()))));
			}

			forTransform.push_back(i);
		}
	}

	for (BattleUnit* i : forTransform)
	{
		convertUnit(i);
	}
	return retVal;
}

/**
 * Sets the kneel reservation setting.
 * @param reserved Should we reserve an extra 4 TUs to kneel?
 */
void BattlescapeGame::setKneelReserved(bool reserved)
{
	_save->setKneelReserved(reserved);
}

/**
 * Gets the kneel reservation setting.
 * @return Kneel reservation setting.
 */
bool BattlescapeGame::getKneelReserved() const
{
	return _save->getKneelReserved();
}

/**
 * Checks if a unit has moved next to a proximity grenade.
 * Checks one tile around the unit in every direction.
 * For a large unit we check every tile it occupies.
 * @param unit Pointer to a unit.
 * @return 2 if a proximity grenade was triggered, 1 if light was changed.
 */
int BattlescapeGame::checkForProximityGrenades(BattleUnit *unit)
{
	if (_save->isPreview())
	{
		return 0;
	}

	// death trap?
	Tile* deathTrapTile = nullptr;
	for (int sx = 0; sx < unit->getArmor()->getSize(); sx++)
	{
		for (int sy = 0; sy < unit->getArmor()->getSize(); sy++)
		{
			Tile* t = _save->getTile(unit->getPosition() + Position(sx, sy, 0));
			if (!deathTrapTile && t && t->getFloorSpecialTileType() >= DEATH_TRAPS)
			{
				deathTrapTile = t;
			}
		}
	}
	if (deathTrapTile)
	{
		std::ostringstream ss;
		ss << "STR_DEATH_TRAP_" << deathTrapTile->getFloorSpecialTileType();
		auto deathTrapRule = getMod()->getItem(ss.str());
		if (deathTrapRule &&
			deathTrapRule->isTargetAllowed(unit->getOriginalFaction()) &&
			(deathTrapRule->getBattleType() == BT_PROXIMITYGRENADE || deathTrapRule->getBattleType() == BT_MELEE))
		{
			BattleItem* deathTrapItem = nullptr;
			for (auto item : *deathTrapTile->getInventory())
			{
				if (item->getRules() == deathTrapRule)
				{
					deathTrapItem = item;
					break;
				}
			}
			if (!deathTrapItem)
			{
				deathTrapItem = _save->createItemForTile(deathTrapRule, deathTrapTile);
			}
			if (deathTrapRule->getBattleType() == BT_PROXIMITYGRENADE)
			{
				deathTrapItem->setFuseTimer(0);
				Position p = deathTrapTile->getPosition().toVoxel() + Position(8, 8, deathTrapTile->getTerrainLevel());
				statePushNext(new ExplosionBState(this, p, BattleActionAttack::GetBeforeShoot(BA_TRIGGER_PROXY_GRENADE, nullptr, deathTrapItem)));
				return 2;
			}
			else if (deathTrapRule->getBattleType() == BT_MELEE)
			{
				Position p = deathTrapTile->getPosition().toVoxel() + Position(8, 8, 12);
				// EXPERIMENTAL: terrainMeleeTilePart = 4 (V_UNIT); no attacker
				statePushNext(new ExplosionBState(this, p, BattleActionAttack::GetBeforeShoot(BA_HIT, nullptr, deathTrapItem), nullptr, false, 0, 0, 4));
				return 2;
			}
		}
	}

	bool exploded = false;
	bool glow = false;
	int size = unit->getArmor()->getSize() + 1;
	for (int tx = -1; tx < size; tx++)
	{
		for (int ty = -1; ty < size; ty++)
		{
			Tile *t = _save->getTile(unit->getPosition() + Position(tx,ty,0));
			if (t)
			{
				std::vector<BattleItem*> forRemoval;
				for (BattleItem *item : *t->getInventory())
				{
					const RuleItem *ruleItem = item->getRules();
					bool g = item->getGlow();
					if (item->fuseProximityEvent())
					{
						if (ruleItem->getBattleType() == BT_GRENADE || ruleItem->getBattleType() == BT_PROXIMITYGRENADE)
						{
							Position p = t->getPosition().toVoxel() + Position(8, 8, t->getTerrainLevel());
							statePushNext(new ExplosionBState(this, p, BattleActionAttack::GetBeforeShoot(BA_TRIGGER_PROXY_GRENADE, nullptr, item)));
							exploded = true;
						}
						else
						{
							forRemoval.push_back(item);
							if (g)
							{
								glow = true;
							}
						}
					}
					else
					{
						if (g != item->getGlow())
						{
							glow = true;
						}
					}
				}
				for (BattleItem *item : forRemoval)
				{
					_save->removeItem(item);
				}
			}
		}
	}
	return exploded ? 2 : glow ? 1 : 0;
}

/**
 * Cleans up all the deleted states.
 */
void BattlescapeGame::cleanupDeleted()
{
	for (std::list<BattleState*>::iterator i = _deleted.begin(); i != _deleted.end(); ++i)
	{
		delete *i;
	}
	_deleted.clear();
}

/**
 * Gets the depth of the battlescape.
 * @return the depth of the battlescape.
 */
int BattlescapeGame::getDepth() const
{
	return _save->getDepth();
}

/**
 * Play sound on battlefield (with direction).
 */
void BattlescapeGame::playSound(int sound, const Position &pos)
{
	if (sound != Mod::NO_SOUND)
	{
		_parentState->getGame()->getMod()->getSoundByDepth(_save->getDepth(), sound)->play(-1, _parentState->getMap()->getSoundAngle(pos));
	}
}

/**
 * Play sound on battlefield.
 */
void BattlescapeGame::playSound(int sound)
{
	if (sound != Mod::NO_SOUND)
	{
		_parentState->getGame()->getMod()->getSoundByDepth(_save->getDepth(), sound)->play();
	}
}

/**
 * Play unit response sound on battlefield.
 */
void BattlescapeGame::playUnitResponseSound(BattleUnit *unit, int type)
{
	if (!getMod()->getEnableUnitResponseSounds())
		return;

	if (!Options::oxceEnableUnitResponseSounds)
		return;

	if (!unit)
		return;

	int chance = Mod::UNIT_RESPONSE_SOUNDS_FREQUENCY[type];
	if (chance < 100 && RNG::seedless(0, 99) >= chance)
	{
		return;
	}

	std::vector<int> sounds;
	if (type == 0)
		sounds = unit->getSelectUnitSounds();
	else if (type == 1)
		sounds = unit->getStartMovingSounds();
	else if (type == 2)
		sounds = unit->getSelectWeaponSounds();
	else if (type == 3)
		sounds = unit->getAnnoyedSounds();

	int sound = -1;
	if (!sounds.empty())
	{
		if (sounds.size() > 1)
			sound = sounds[RNG::seedless(0, sounds.size() - 1)];
		else
			sound = sounds.front();
	}

	if (sound != Mod::NO_SOUND)
	{
		if (!Mix_Playing(4))
		{
			// use fixed channel, so that we can check if the unit isn't already/still talking
			getMod()->getSoundByDepth(_save->getDepth(), sound)->play(4);
		}
	}
}


std::list<BattleState*> BattlescapeGame::getStates()
{
	return _states;
}

/**
 * Ends the turn if auto-end battle is enabled
 * and all mission objectives are completed.
 */
void BattlescapeGame::autoEndBattle()
{
	if (_save->isPreview())
	{
		return;
	}
	if (Options::battleAutoEnd)
	{
		if (_save->getVIPSurvivalPercentage() > 0 && _save->getVIPEscapeType() != ESCAPE_NONE)
		{
			return; // "escort the VIPs" missions don't end when all aliens are neutralized
		}
		bool end = false;
		bool askForConfirmation = false;
		if (_save->getObjectiveType() == MUST_DESTROY)
		{
			end = _save->allObjectivesDestroyed();
		}
		else
		{
			auto tally = tallyUnits();
			end = (tally.liveAliens == 0 || tally.liveSoldiers == 0);
			if (tally.liveAliens == 0)
			{
				if (tally.liveSentientAliens == 0)
				{
				_allEnemiesNeutralized = true; // remember that all aliens were neutralized (and the battle should end no matter what)
				askForConfirmation = true;
				}
			}
		}
		if (end)
		{
			_save->setSelectedUnit(0);
			cancelCurrentAction(true);
			requestEndTurn(askForConfirmation);
		}
	}
}

}
