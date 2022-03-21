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

#include "UnitWalkBState.h"
#include "MeleeAttackBState.h"
#include "TileEngine.h"
#include "Pathfinding.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "Camera.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Engine/Sound.h"
#include "../Engine/Options.h"
#include "../Engine/Logger.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "UnitFallBState.h"

namespace OpenXcom
{

/**
 * Sets up an UnitWalkBState.
 * @param parent Pointer to the Battlescape.
 * @param action Pointer to an action.
 */
UnitWalkBState::UnitWalkBState(BattlescapeGame *parent, BattleAction action) : BattleState(parent, action), _unit(0), _pf(0), _terrain(0), _falling(false), _beforeFirstStep(false), _numUnitsSpotted(0), _preMovementCost(0)
{

}

/**
 * Deletes the UnitWalkBState.
 */
UnitWalkBState::~UnitWalkBState()
{

}

/**
 * Initializes the state.
 */
void UnitWalkBState::init()
{
	_unit = _action.actor;
	_numUnitsSpotted = _unit->getUnitsSpottedThisTurn().size();
	setNormalWalkSpeed();
	_pf = _parent->getPathfinding();
	_terrain = _parent->getTileEngine();
	_target = _action.target;
	if (Options::traceAI) { Log(LOG_INFO) << "Walking from: " << _unit->getPosition() << "," << " to " << _target;}
	int dir = _pf->getStartDirection();
	if (!_action.strafe && dir != -1 && dir != _unit->getDirection())
	{
		_beforeFirstStep = true;
	}
	_terrain->addMovingUnit(_unit);
}

/**
 * Deinitalize the state.
 */
void UnitWalkBState::deinit()
{
	_terrain->removeMovingUnit(_unit);
}

/**
 * Runs state functionality every cycle.
 */
void UnitWalkBState::think()
{
	if (!_unit->getArmor()->allowsMoving())
	{
		_pf->abortPath();
		_parent->popState();
		return;
	}

	bool unitSpotted = false;
	int size = _unit->getArmor()->getSize() - 1;
	bool onScreen = (_unit->getVisible() && _parent->getMap()->getCamera()->isOnScreen(_unit->getPosition(), true, size, false));
	if (_unit->isKneeled())
	{
		if (_parent->kneel(_unit))
		{
			return;
		}
		else
		{
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			_pf->abortPath();
			_parent->popState();
			return;
		}
	}


	if (_unit->isOut())
	{
		_pf->abortPath();
		_parent->popState();
		return;
	}

	if (_unit->getStatus() == STATUS_WALKING || _unit->getStatus() == STATUS_FLYING)
	{
		if ((_parent->getSave()->getTile(_unit->getDestination())->getUnit() == 0) || // next tile must be not occupied
			(_parent->getSave()->getTile(_unit->getDestination())->getUnit() == _unit))
		{
			bool onScreenBoundary = (_unit->getVisible() && _parent->getMap()->getCamera()->isOnScreen(_unit->getPosition(), true, size, true));
			_unit->keepWalking(_parent->getSave(), onScreenBoundary); // advances the phase
			playMovementSound();
			if (_parent->getSave()->isPreview())
			{
				_unit->resetTimeUnitsAndEnergy();
			}
		}
		else if (!_falling)
		{
			_unit->lookAt(_unit->getDestination(), (_unit->getTurretType() != -1));	// turn to undiscovered unit
			_pf->abortPath();
		}

		// unit moved from one tile to the other, update the tiles
		if (_unit->getPosition() != _unit->getLastPosition())
		{
			_falling = _unit->haveNoFloorBelow() && _unit->getPosition().z != 0 && _unit->getMovementType() != MT_FLY && _unit->getWalkingPhase() == 0;

			if (_falling)
			{
				for (int x = size; x >= 0; --x)
				{
					for (int y = size; y >= 0; --y)
					{
						Tile *otherTileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x,y,-1));
						if (otherTileBelow && otherTileBelow->getUnit())
						{
							_falling = false;
							_pf->dequeuePath();
							_parent->getSave()->addFallingUnit(_unit);
							_parent->statePushFront(new UnitFallBState(_parent));
							return;
						}
					}
				}
			}

			if (!_parent->getMap()->getCamera()->isOnScreen(_unit->getPosition(), true, size, false) && _unit->getFaction() != FACTION_PLAYER && _unit->getVisible())
				_parent->getMap()->getCamera()->centerOnPosition(_unit->getPosition());
			// if the unit changed level, camera changes level with
			_parent->getMap()->getCamera()->setViewLevel(_unit->getPosition().z);
		}

		// is the step finished?
		if (_unit->getStatus() == STATUS_STANDING)
		{
			// update the TU display
			_parent->getSave()->getBattleState()->updateSoldierInfo();
			// if the unit burns floor tiles, burn floor tiles as long as we're not falling
			if (!_falling && (_unit->getSpecialAbility() == SPECAB_BURNFLOOR || _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
			{
				_unit->getTile()->ignite(1);
				Position posHere = _unit->getPosition();
				Position voxelHere = posHere.toVoxel() + Position(8,8,-(_unit->getTile()->getTerrainLevel()));
				_parent->getTileEngine()->hit(BattleActionAttack{ BA_NONE, _unit, }, voxelHere, _unit->getBaseStats()->strength, _parent->getMod()->getDamageType(DT_IN), false);

				if (_unit->getStatus() != STATUS_STANDING) // ie: we burned a hole in the floor and fell through it
				{
					_pf->abortPath();
					return;
				}
			}

			if (_unit->getFaction() != FACTION_PLAYER)
			{
				_unit->setVisible(false);
			}

			auto change = _parent->checkForProximityGrenades(_unit);
			// move our personal lighting with us
			_terrain->calculateLighting(change ? LL_ITEMS : LL_UNITS, _unit->getPosition(), 2);
			_terrain->calculateFOV(_unit->getPosition(), 2, false); //update unit visibility for all units which can see last and current position.
			//tile visibility for this unit is handled later.
			unitSpotted = (!_action.ignoreSpottedEnemies && !_falling && !_action.desperate && _parent->getPanicHandled() && _numUnitsSpotted != _unit->getUnitsSpottedThisTurn().size());

			if (change > 1)
			{
				_parent->popState();
				return;
			}
			if (unitSpotted)
			{
				_pf->abortPath();
				_parent->popState();
				return;
			}
			// check for reaction fire
			if (!_falling)
			{
				if (_terrain->checkReactionFire(_unit, _action))
				{
					// unit got fired upon - stop walking
					_pf->abortPath();
					_parent->popState();
					return;
				}
			}
		}
		else if (onScreen)
		{
			// make sure the unit sprites are up to date
			if (_pf->getStrafeMove())
			{
				// This is where we fake out the strafe movement direction so the unit "moonwalks"
				int dirTemp = _unit->getDirection();
				_unit->setDirection(_unit->getFaceDirection());
				//TODO fix moonwalk
				_unit->setDirection(dirTemp);
			}
		}
	}

	// we are just standing around, shouldn't we be walking?
	if (_unit->getStatus() == STATUS_STANDING || _unit->getStatus() == STATUS_PANICKING || _unit->getStatus() == STATUS_BERSERK)
	{
		// check if we did spot new units
		if (unitSpotted && !_action.desperate && _unit->getCharging() == 0 && !_falling)
		{
			if (Options::traceAI) { Log(LOG_INFO) << "Uh-oh! Company!"; }
			_unit->setHiding(false); // clearly we're not hidden now
			postPathProcedures();
			return;
		}

		if (onScreen || _parent->getSave()->getDebugMode())
		{
			setNormalWalkSpeed();
		}
		else
		{
			_parent->setStateInterval(0);
		}
		int dir = _pf->getStartDirection();
		if (_falling)
		{
			dir = Pathfinding::DIR_DOWN;
		}

		if (dir != -1)
		{
			if (_pf->getStrafeMove())
			{
				_unit->setFaceDirection(_unit->getDirection());
			}

			_pf->setUnit(_unit); //TODO: remove as was done by `getTUCost`
			auto r = _pf->getTUCost(_unit->getPosition(), dir, _unit, 0, _action.getMoveType());

			auto tu = r.cost.time;
			int energy = r.cost.energy;
			auto destination = r.pos;

			if (tu == Pathfinding::INVALID_MOVE_COST)
			{
				_pf->abortPath();
				_parent->popState();
				return;
			}

			if (tu > _unit->getTimeUnits())
			{
				if (_parent->getPanicHandled())
				{
					_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
				}
				_pf->abortPath();
				_parent->popState();
				return;
			}

			if (energy > _unit->getEnergy())
			{
				if (_parent->getPanicHandled())
				{
					_action.result = "STR_NOT_ENOUGH_ENERGY";
				}
				_pf->abortPath();
				_parent->popState();
				return;
			}

			if (_parent->getPanicHandled() && !_falling && _parent->checkReservedTU(_unit, tu, energy) == false)
			{
				_pf->abortPath();
				return;
			}

			// we are looking in the wrong way, turn first (unless strafing)
			// we are not using the turn state, because turning during walking costs no tu
			if (dir != _unit->getDirection() && dir < Pathfinding::DIR_UP && !_pf->getStrafeMove())
			{
				_unit->lookAt(dir);
				return;
			}

			// now open doors (if any)
			if (dir < Pathfinding::DIR_UP)
			{
				int door = _terrain->unitOpensDoor(_unit, false, dir);
				if (door == 3)
				{
					return; // don't start walking yet, wait for the ufo door to open
				}
				if (door == 0)
				{
					_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::DOOR_OPEN)->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition())); // normal door
				}
				if (door == 1)
				{
					_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::SLIDING_DOOR_OPEN)->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition())); // ufo door
					return; // don't start walking yet, wait for the ufo door to open
				}
			}
			for (int x = size; x >= 0; --x)
			{
				for (int y = size; y >= 0; --y)
				{
					BattleUnit* unitInMyWay = _parent->getSave()->getTile(destination + Position(x,y,0))->getUnit();
					BattleUnit* unitBelowMyWay = 0;
					Tile* belowDest = _parent->getSave()->getTile(destination + Position(x,y,-1));
					if (belowDest)
					{
						unitBelowMyWay = belowDest->getUnit();
					}
					// can't walk into units in this tile, or on top of other units sticking their head into this tile
					if (!_falling &&
						((unitInMyWay && unitInMyWay != _unit)
						|| (belowDest && unitBelowMyWay && unitBelowMyWay != _unit &&
						(-belowDest->getTerrainLevel() + unitBelowMyWay->getFloatHeight() + unitBelowMyWay->getHeight())
						>= 28)))  // 4+ voxels poking into the tile above, we don't kick people in the head here at XCom.
					{
						_action.clearTU();
						_pf->abortPath();
						_parent->popState();
						return;
					}
				}
			}
			// now start moving
			dir = _pf->dequeuePath();
			if (_falling)
			{
				dir = Pathfinding::DIR_DOWN;
			}

			if (_unit->spendTimeUnits(tu))
			{
				if (_unit->spendEnergy(energy))
				{
					_unit->startWalking(dir, destination, _parent->getSave());
					_beforeFirstStep = false;
				}
			}
			// make sure the unit sprites are up to date
			if (onScreen)
			{
				if (_pf->getStrafeMove())
				{
					// This is where we fake out the strafe movement direction so the unit "moonwalks"
					int dirTemp = _unit->getDirection();
					_unit->setDirection(_unit->getFaceDirection());
					_unit->setDirection(dirTemp);
				}
			}
		}
		else
		{
			postPathProcedures();
			return;
		}
	}

	// turning during walking costs no tu
	if (_unit->getStatus() == STATUS_TURNING)
	{
		// except before the first step.
		if (_beforeFirstStep)
		{
			if (_unit->getArmor()->getTurnBeforeFirstStep())
			{
				_unit->spendTimeUnits(_unit->getTurnCost());
			}
			else
			{
				_preMovementCost++;
			}
		}

		_unit->turn();

		// calculateFOV is unreliable for setting the unitSpotted bool, as it can be called from various other places
		// in the code, ie: doors opening, and this messes up the result.
		_terrain->calculateFOV(_unit);
		unitSpotted = (!_action.ignoreSpottedEnemies && !_falling && !_action.desperate && _parent->getPanicHandled() && _numUnitsSpotted != _unit->getUnitsSpottedThisTurn().size());

		if (unitSpotted && !_action.desperate && !_unit->getCharging() && !_falling)
		{
			if (_beforeFirstStep)
			{
				_preMovementCost = _preMovementCost * _unit->getTurnCost();
				_unit->spendTimeUnits(_preMovementCost);
			}
			if (Options::traceAI) { Log(LOG_INFO) << "Egads! A turn reveals new units! I must pause!"; }
			_unit->setHiding(false); // not hidden, are we...
			_pf->abortPath();
			_unit->abortTurn(); //revert to a standing state.
			_parent->popState();
		}
	}
}

/**
 * Aborts unit walking.
 */
void UnitWalkBState::cancel()
{ //host
	if ((_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getSide() == FACTION_ALIEN_PLAYER) && _parent->getPanicHandled())
	_pf->abortPath();
}
// Client{ 
// Client	if (_parent->getSave()->getSide() == FACTION_ALIEN_PLAYER && _parent->getPanicHandled())
// Client		_pf->abortPath();
// Client}

/**
 * Handles some calculations when the path is finished.
 */
void UnitWalkBState::postPathProcedures()
{
	_action.clearTU();
	if (_unit->getFaction() != FACTION_PLAYER || _unit->getFaction() != FACTION_ALIEN_PLAYER) //Possible bug.Copypasted without thinknig
	{
		int dir = _action.finalFacing;
		if (_action.finalAction)
		{
			_unit->dontReselect();
		}
		if (_unit->getCharging() != 0)
		{
			dir = _parent->getTileEngine()->getDirectionTo(_unit->getPosition(), _unit->getCharging()->getPosition());
			if (_parent->getTileEngine()->validMeleeRange(_unit, _action.actor->getCharging(), dir))
			{
				BattleAction action;
				action.actor = _unit;
				action.target = _unit->getCharging()->getPosition();
				action.weapon = _unit->getUtilityWeapon(BT_MELEE);
				action.type = BA_HIT;
				action.targeting = true;
				action.updateTU();
				_unit->setCharging(0);
				_parent->statePushBack(new MeleeAttackBState(_parent, action));
			}
		}
		else if (_unit->isHiding())
		{
			dir = _unit->getDirection() + 4;
			_unit->setHiding(false);
			_unit->dontReselect();
		}
		if (dir != -1)
		{
			if (dir >= 8)
			{
				dir -= 8;
			}
			_unit->lookAt(dir);
			while (_unit->getStatus() == STATUS_TURNING)
			{
				_unit->turn();
				_parent->getTileEngine()->calculateFOV(_unit);
			}
		}
	}
	else if (!_parent->getPanicHandled())
	{
		//todo: set the unit to aggrostate and try to find cover?
		_unit->clearTimeUnits();
	}

	_terrain->calculateLighting(LL_UNITS, _unit->getPosition());
	_terrain->calculateFOV(_unit);
	if (!_falling)
		_parent->popState();
}

/**
 * Handles some calculations when the walking is finished.
 */
void UnitWalkBState::setNormalWalkSpeed()
{
	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::battleXcomSpeed);
	else if (_unit->getFaction() == FACTION_ALIEN_PLAYER)
		_parent->setStateInterval(Options::battleXcomSpeed);
	else
		_parent->setStateInterval(Options::battleAlienSpeed);
}


/**
 * Handles the stepping sounds.
 */
void UnitWalkBState::playMovementSound()
{
	int size = _unit->getArmor()->getSize() - 1;
	if ((!_unit->getVisible() && !_parent->getSave()->getDebugMode()) || !_parent->getMap()->getCamera()->isOnScreen(_unit->getPosition(), true, size, false)) return;

	Tile *tile = _unit->getTile();
	int sound = -1;
	int unitSound = _unit->getMoveSound();
	int tileSoundOffset = tile->getFootstepSound(_parent->getSave()->getBelowTile(tile));
	int tileSound = Mod::NO_SOUND;
	if (tileSoundOffset > -1)
	{
		// play footstep sound 1
		if (_unit->getWalkingPhase() == 3)
		{
			tileSound = Mod::WALK_OFFSET + (tileSoundOffset*2);
		}
		// play footstep sound 2
		if (_unit->getWalkingPhase() == 7)
		{
			tileSound = Mod::WALK_OFFSET + (tileSoundOffset*2) + 1;
		}
	}
	if (unitSound != Mod::NO_SOUND)
	{
		// if a sound is configured in the ruleset, play that one
		if (_unit->getWalkingPhase() == 0)
		{
			sound = unitSound;
		}
	}
	else
	{
		if (_unit->getStatus() == STATUS_WALKING)
		{
			if (tileSound > Mod::NO_SOUND) //TODO: it should be `!=` but its possbile that offset could get negative is based on mod data
			{
				sound = tileSound;
			}
		}
		else if (_unit->getMovementType() == MT_FLY)
		{
			// play default flying sound
			if (_unit->getWalkingPhase() == 1)
			{
				sound = Mod::FLYING_SOUND;
			}
		}
	}

	sound = ModScript::scriptFunc1<ModScript::SelectMoveSoundUnit>(
		_unit->getArmor(),
		sound,
		_unit, _unit->getWalkingPhase(), unitSound, tileSound, Mod::WALK_OFFSET, tileSoundOffset, Mod::FLYING_SOUND, _action.getMoveType()
	);
	if (sound >= 0)
	{
		_parent->getMod()->getSoundByDepth(_parent->getDepth(), sound)->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
	}
}

}
