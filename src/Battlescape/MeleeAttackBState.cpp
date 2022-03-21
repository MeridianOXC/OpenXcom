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
#include "MeleeAttackBState.h"
#include "ExplosionBState.h"
#include "BattlescapeGame.h"
#include "BattlescapeState.h"
#include "TileEngine.h"
#include "Map.h"
#include "Camera.h"
#include "AIModule.h"
#include "../Savegame/Tile.h"
#include "../Engine/RNG.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Engine/Exception.h"
#include "../Engine/Sound.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Sets up a MeleeAttackBState.
 */
MeleeAttackBState::MeleeAttackBState(BattlescapeGame *parent, BattleAction action) : BattleState(parent, action), _unit(0), _target(0), _weapon(0), _ammo(0), _hitNumber(0), _initialized(false), _reaction(false)
{
}

/**
 * Deletes the MeleeAttackBState.
 */
MeleeAttackBState::~MeleeAttackBState()
{
}

/**
 * Initializes the sequence.
 * does a lot of validity checking.
 */
void MeleeAttackBState::init()
{
	if (_initialized) return;
	_initialized = true;

	int terrainMeleeTilePart = _action.terrainMeleeTilePart;
	_action.terrainMeleeTilePart = 0; // reset!

	_weapon = _action.weapon;
	if (!_weapon) // can't hit without weapon
	{
		_parent->popState();
		return;
	}

	_unit = _action.actor;

	bool reactionShoot = _unit->getFaction() != _parent->getSave()->getSide();
	_ammo = _action.weapon->getAmmoForAction(BA_HIT, reactionShoot ? nullptr : &_action.result);
	if (!_ammo)
	{
		_parent->popState();
		return;
	}

	if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		_parent->popState();
		return;
	}

	if (_unit->isOut() || _unit->isOutThresholdExceed())
	{
		// something went wrong - we can't shoot when dead or unconscious, or if we're about to fall over.
		_parent->popState();
		return;
	}

	// reaction fire
	if (reactionShoot)
	{
		// no ammo or target is dead: give the time units back and cancel the shot.
		auto target = _parent->getSave()->getTile(_action.target)->getUnit();
		if (!target || target->isOut() || target->isOutThresholdExceed() || target != _parent->getSave()->getSelectedUnit())
		{
			_parent->popState();
			return;
		}
		_unit->lookAt(_action.target, _unit->getTurretType() != -1);
		while (_unit->getStatus() == STATUS_TURNING)
		{
			_unit->turn(_unit->getTurretType() != -1);
		}
	}

	//spend TU
	if (!_action.spendTU(&_action.result))
	{
		_parent->popState();
		return;
	}

	// terrain melee
	if (terrainMeleeTilePart > 0)
	{
		_voxel = _action.target.toVoxel() + Position(8, 8, 12);
		performMeleeAttack(terrainMeleeTilePart);
		return;
	}

	AIModule *ai = _unit->getAIModule();

	if (_unit->getFaction() == _parent->getSave()->getSide() &&
		_unit->getFaction() != FACTION_PLAYER && _unit->getFaction() != FACTION_ALIEN_PLAYER &&
		_parent->_debugPlay == false &&
		ai && ai->getTarget())
	{
		_target = ai->getTarget();
	}
	else
	{
		_target = _parent->getSave()->getTile(_action.target)->getUnit();
	}

	if (!_target)
	{
		throw Exception("This is a known (but tricky) bug... still fixing it, sorry. In the meantime, try save scumming option or kill all aliens in debug mode to finish the mission.");
	}

	int height = _target->getFloatHeight() + (_target->getHeight() / 2) - _parent->getSave()->getTile(_action.target)->getTerrainLevel();
	_voxel = _action.target.toVoxel() + Position(8, 8, height);

	if (!_parent->getSave()->getTile(_voxel.toTile()))
	{
		throw Exception("Melee attack animation overflow: target voxel is outside of the map boundaries.");
	}

	if (_unit->getFaction() == FACTION_HOSTILE)
	{
		_hitNumber = _weapon->getRules()->getAIMeleeHitCount() - 1;
	}

	performMeleeAttack();
}

/**
 * Performs all the overall functions of the state, this code runs AFTER the explosion state pops.
 */
void MeleeAttackBState::think()
{
	_parent->getSave()->getBattleState()->clearMouseScrollingState();
	if (_reaction && !_parent->getSave()->getUnitsFalling())
	{
		_reaction = false;
		if (_parent->getTileEngine()->checkReactionFire(_unit, _action))
		{
			return;
		}
	}

	// if the unit burns floor tiles, burn floor tiles
	if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR || _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
	{
		_parent->getSave()->getTile(_action.target)->ignite(15);
	}
	if (_hitNumber > 0 &&
		// not performing a reaction attack
		_unit->getFaction() == _parent->getSave()->getSide() &&
		// whose target is still alive or at least conscious
		_target && !_target->isOutThresholdExceed() &&
		// and we still have ammo to make the attack
		_weapon->getAmmoForAction(BA_HIT) &&
		// spend the TUs immediately
		_action.spendTU())
	{
		--_hitNumber;
		performMeleeAttack();
	}
	else
	{
		if (_action.cameraPosition.z != -1)
		{
			_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
			_parent->getMap()->invalidate();
		}

		if (_unit->getFaction() == _parent->getSave()->getSide()) // not a reaction attack
		{
			_parent->getCurrentAction()->type = BA_NONE; // do this to restore cursor
		}

		if (_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getSide() == FACTION_ALIEN_PLAYER || _parent->getSave()->getDebugMode())
		{
			_parent->setupCursor();
		}
		_parent->convertInfected();
		_parent->popState();
	}
}

/**
 * Sets up a melee attack, inserts an explosion into the map and make noises.
 */
void MeleeAttackBState::performMeleeAttack(int terrainMeleeTilePart)
{
	// set the soldier in an aiming position
	_unit->aim(true);

	// use up ammo if applicable
	_action.weapon->spendAmmoForAction(BA_HIT, _parent->getSave());
	_parent->getMap()->setCursorType(CT_NONE);

	// offset the damage voxel ever so slightly so that the target knows which side the attack came from
	Position difference = _unit->getPosition() - _action.target;
	// large units may cause it to offset too much, so we'll clamp the values.
	difference.x = Clamp<Sint16>(difference.x, -1, 1);
	difference.y = Clamp<Sint16>(difference.y, -1, 1);

	Position damagePosition = _voxel + difference;


	// make an explosion action
	_parent->statePushFront(new ExplosionBState(_parent, damagePosition, BattleActionAttack::GetAferShoot(_action, _ammo), 0, true, 0, 0, terrainMeleeTilePart));


	_reaction = true;
}

}
