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
#include <algorithm>
#include "ProjectileFlyBState.h"
#include "ExplosionBState.h"
#include "Projectile.h"
#include "TileEngine.h"
#include "Map.h"
#include "Pathfinding.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/HitLog.h"
#include "../Mod/Mod.h"
#include "../Engine/Sound.h"
#include "../Engine/RNG.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleItem.h"
#include "../Engine/Options.h"
#include "AIModule.h"
#include "Camera.h"
#include "Explosion.h"
#include "BattlescapeState.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Sets up an ProjectileFlyBState.
 */
ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame *parent, BattleAction action, Position origin, int range) : BattleState(parent, action), _unit(0), _ammo(0), _origin(origin), _originVoxel(-1,-1,-1), _projectileImpact(0), _range(range), _initialized(false), _targetFloor(false)
{
}

ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame *parent, BattleAction action) : BattleState(parent, action), _unit(0), _ammo(0), _origin(action.actor->getPosition()), _originVoxel(-1,-1,-1), _projectileImpact(0), _range(0), _initialized(false), _targetFloor(false)
{
}

/**
 * Deletes the ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{
}

/**
 * Initializes the sequence:
 * - checks if the shot is valid,
 * - calculates the base accuracy.
 */
void ProjectileFlyBState::init()
{
	if (_initialized) return;
	_initialized = true;

	BattleItem *weapon = _action.weapon;

	if (!weapon) // can't shoot without weapon
	{
		_parent->popState();
		return;
	}

	if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		_parent->popState();
		return;
	}

	//test TU only on first lunch waypoint or normal shoot
	if (_range == 0 && !_action.haveTU(&_action.result))
	{
		_parent->popState();
		return;
	}

	_unit = _action.actor;

	bool reactionShoot = _unit->getFaction() != _parent->getSave()->getSide();
	if (_action.type != BA_THROW)
	{
		_ammo = _action.weapon->getAmmoForAction(_action.type, reactionShoot ? nullptr : &_action.result);
		if (!_ammo)
		{
			_parent->popState();
			return;
		}
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
		auto target = _parent->getSave()->getTile(_action.target)->getUnit();
		// target is dead: cancel the shot.
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

	Tile *endTile = _parent->getSave()->getTile(_action.target);
	int distanceSq = _action.actor->distance3dToPositionSq(_action.target);
	bool isPlayer = (_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getSide() == FACTION_ALIEN_PLAYER );
	if (isPlayer) _parent->getMap()->resetObstacles();
	switch (_action.type)
	{
	case BA_SNAPSHOT:
	case BA_AIMEDSHOT:
	case BA_AUTOSHOT:
	case BA_LAUNCH:
		if (weapon->getRules()->isOutOfRange(distanceSq))
		{
			// out of range
			_action.result = "STR_OUT_OF_RANGE";
			_parent->popState();
			return;
		}
		break;
	case BA_THROW:
		if (!validThrowRange(&_action, _parent->getTileEngine()->getOriginVoxel(_action, 0), _parent->getSave()->getTile(_action.target), _parent->getSave()->getDepth()))
		{
			// out of range
			_action.result = "STR_OUT_OF_RANGE";
			_parent->popState();
			return;
		}
		if (endTile &&
			endTile->getTerrainLevel() == -24 &&
			endTile->getPosition().z + 1 < _parent->getSave()->getMapSizeZ())
		{
			_action.target.z += 1;
		}
		break;
	default:
		_parent->popState();
		return;
	}

	// Check for close quarters combat
	if (_parent->getMod()->getEnableCloseQuartersCombat() && _action.type != BA_THROW && _action.type != BA_LAUNCH && _unit->getTurretType() == -1 && !_unit->getArmor()->getIgnoresMeleeThreat())
	{
		// Start by finding 'targets' for the check
		std::vector<BattleUnit*> closeQuartersTargetList;
		int surroundingTilePositions [8][2] = {
			{0, -1}, // north (-y direction)
			{1, -1}, // northeast
			{1, 0}, // east (+ x direction)
			{1, 1}, // southeast
			{0, 1}, // south (+y direction)
			{-1, 1}, // southwest
			{-1, 0}, // west (-x direction)
			{-1, -1}}; // northwest
		for (int dir = 0; dir < 8; dir++)
		{
			Position tileToCheck = _origin;
			tileToCheck.x += surroundingTilePositions[dir][0];
			tileToCheck.y += surroundingTilePositions[dir][1];

			if (_parent->getSave()->getTile(tileToCheck)) // Make sure the tile is in bounds
			{
				BattleUnit* closeQuartersTarget = _parent->getSave()->selectUnit(tileToCheck);
				// Variable for LOS check
				int checkDirection = _parent->getTileEngine()->getDirectionTo(tileToCheck, _unit->getPosition());
				if (closeQuartersTarget && _unit->getFaction() != closeQuartersTarget->getFaction() // Unit must exist and not be same faction
					&& closeQuartersTarget->getArmor()->getCreatesMeleeThreat() // Unit must be valid defender, 2x2 default false here
					&& closeQuartersTarget->getTimeUnits() >= _parent->getMod()->getCloseQuartersTuCostGlobal() // Unit must have enough TUs
					&& closeQuartersTarget->getEnergy() >= _parent->getMod()->getCloseQuartersEnergyCostGlobal() // Unit must have enough Energy
					&& _parent->getTileEngine()->validMeleeRange(closeQuartersTarget, _unit, checkDirection) // Unit must be able to see the unit attempting to fire
					&& !(_unit->getFaction() == FACTION_PLAYER && closeQuartersTarget->getFaction() == FACTION_NEUTRAL) // Civilians don't inhibit player
					&& !(_unit->getFaction() == FACTION_NEUTRAL && closeQuartersTarget->getFaction() == FACTION_PLAYER)) // Player doesn't inhibit civilians
				{
					if (RNG::percent(_parent->getMod()->getCloseQuartersSneakUpGlobal()))
					{
						if (_unit->getFaction() == FACTION_HOSTILE || _unit->getFaction() == FACTION_ALIEN_PLAYER) // alien attacker (including mind-controlled xcom)
						{
							if (!closeQuartersTarget->hasVisibleUnit(_unit))
							{
								continue; // the xcom/civilian victim *DOES NOT SEE* the attacker and cannot defend itself
							}
						}
						else // xcom/civilian attacker (including mind-controlled aliens)
						{
							if (_unit->getTurnsSinceSpotted() > 1)
							{
								continue; // the aliens (as a collective) *ARE NOT AWARE* of the attacker and cannot defend themselves
							}
						}
					}
					closeQuartersTargetList.push_back(closeQuartersTarget);
				}
			}
		}

		if (!closeQuartersTargetList.empty())
		{
			int closeQuartersFailedResults[6] = {
				0,   // Fire straight down
				0,   // Fire straight up
				6,   // Fire left 90 degrees
				7,   // Fire left 45 degrees
				1,   // Fire right 45 degrees
				2 }; // Fire right 90 degrees

			for (std::vector<BattleUnit*>::iterator bu = closeQuartersTargetList.begin(); bu != closeQuartersTargetList.end(); ++bu)
			{
				BattleActionAttack attack;
				attack.type = BA_CQB;
				attack.attacker = _action.actor;
				attack.weapon_item = _action.weapon;
				attack.damage_item = _action.weapon;

				// Roll for the check
				if (!_parent->getTileEngine()->meleeAttack(attack, (*bu)))
				{
					// Failed the check, roll again to see result
					if (_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getSide() == FACTION_ALIEN_PLAYER) // Only show message during player's turn or ALIEN PLAYER turn
					{
						_action.result = "STR_FAILED_CQB_CHECK";
					}
					int rng = RNG::generate(0, 5);
					Position closeQuartersFailedNewTarget = _unit->getPosition();
					if (rng == 1)
					{
						closeQuartersFailedNewTarget.z += 1;
					}
					else if (rng > 1)
					{
						int newFacing = (_unit->getDirection() + closeQuartersFailedResults[rng]) % 8;
						closeQuartersFailedNewTarget.x += surroundingTilePositions[newFacing][0];
						closeQuartersFailedNewTarget.y += surroundingTilePositions[newFacing][1];
					}

					// Make sure the new target is in bounds
					if (!_parent->getSave()->getTile(closeQuartersFailedNewTarget))
					{
						// Default to firing at our feet
						closeQuartersFailedNewTarget = _unit->getPosition();
					}

					// Turn to look at new target
					_action.target = closeQuartersFailedNewTarget;
					_unit->lookAt(_action.target, _unit->getTurretType() != -1);
					while (_unit->getStatus() == STATUS_TURNING)
					{
						_unit->turn(_unit->getTurretType() != -1);
					}

					// We're done, spend TUs and Energy; and don't check remaining CQB candidates anymore
					(*bu)->spendTimeUnits(_parent->getMod()->getCloseQuartersTuCostGlobal());
					(*bu)->spendEnergy(_parent->getMod()->getCloseQuartersEnergyCostGlobal());
					break;
				}
			}
		}
	}

	bool forceEnableObstacles = false;
	if (_action.type == BA_LAUNCH || (Options::forceFire && _parent->getSave()->isCtrlPressed(true) && isPlayer) || !_parent->getPanicHandled())
	{
		// target nothing, targets the middle of the tile
		_targetVoxel = _action.target.toVoxel() + TileEngine::voxelTileCenter;
		if (_action.type == BA_LAUNCH)
		{
			if (_targetFloor)
			{
				// launched missiles with two waypoints placed on the same tile: target the floor.
				_targetVoxel.z -= 10;
			}
			else
			{
				// launched missiles go slightly higher than the middle.
				_targetVoxel.z += 4;
			}
		}
	}
	else if (!_action.weapon->getArcingShot(_action.type))
	{
		// determine the target voxel.
		// aim at the center of the unit, the object, the walls or the floor (in that priority)
		// if there is no LOF to the center, try elsewhere (more outward).
		// Store this target voxel.
		Tile *targetTile = _parent->getSave()->getTile(_action.target);
		Position originVoxel = _parent->getTileEngine()->getOriginVoxel(_action, _parent->getSave()->getTile(_origin));
		if (targetTile->getUnit() &&
				((_unit->getFaction() != FACTION_PLAYER && _unit->getFaction() != FACTION_ALIEN_PLAYER) || // POSSIBLE BUG GRANADE LAUNCHER ARCING SHOT
			targetTile->getUnit()->getVisible()))
		{
			if (_origin == _action.target || targetTile->getUnit() == _unit)
			{
				// don't shoot at yourself but shoot at the floor
				_targetVoxel = _action.target.toVoxel() + Position(8, 8, 0);
			}
			else
			{
				bool foundLoF = _parent->getTileEngine()->canTargetUnit(&originVoxel, targetTile, &_targetVoxel, _unit, isPlayer);

				if (!foundLoF && Options::oxceEnableOffCentreShooting)
				{
					// If we can't target from the standard shooting position, try a bit left and right from the centre.
					for (auto& rel_pos : { BattleActionOrigin::LEFT, BattleActionOrigin::RIGHT })
					{
						_action.relativeOrigin = rel_pos;
						originVoxel = _parent->getTileEngine()->getOriginVoxel(_action, _parent->getSave()->getTile(_origin));
						foundLoF = _parent->getTileEngine()->canTargetUnit(&originVoxel, targetTile, &_targetVoxel, _unit, isPlayer);
						if (foundLoF)
						{
							break;
						}
					}
				}

				if (!foundLoF)
				{
					// Failed to find LOF
					_action.relativeOrigin = BattleActionOrigin::CENTRE; // reset to the normal origin

					_targetVoxel = TileEngine::invalid.toVoxel(); // out of bounds, even after voxel to tile calculation.
					if (isPlayer)
					{
						forceEnableObstacles = true;
					}
				}
			}
		}
		else if (targetTile->getMapData(O_OBJECT) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_OBJECT, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(8, 8, 10);
			}
		}
		else if (targetTile->getMapData(O_NORTHWALL) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_NORTHWALL, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(8, 0, 9);
			}
		}
		else if (targetTile->getMapData(O_WESTWALL) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_WESTWALL, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(0, 8, 9);
			}
		}
		else if (targetTile->getMapData(O_FLOOR) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_FLOOR, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(8, 8, 2);
			}
		}
		else
		{
			// dummy attempt (only to highlight obstacles)
			_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, MapData::O_DUMMY, &_targetVoxel, _unit, isPlayer);

			// target nothing, targets the middle of the tile
			_targetVoxel = _action.target.toVoxel() + TileEngine::voxelTileCenter;
		}
	}

	if (createNewProjectile())
	{
		auto conf = weapon->getActionConf(_action.type);
		if (_parent->getMap()->isAltPressed() || (conf && !conf->followProjectiles))
		{
			// temporarily turn off camera following projectiles to prevent annoying flashing effects (e.g. on minigun-like weapons)
			_parent->getMap()->setFollowProjectile(false);
		}
		if (_range == 0) _action.spendTU();
		_parent->getMap()->setCursorType(CT_NONE);
		_parent->getMap()->getCamera()->stopMouseScrolling();
		_parent->getMap()->disableObstacles();
	}
	else if (isPlayer && (_targetVoxel.z >= 0 || forceEnableObstacles))
	{
		_parent->getMap()->enableObstacles();
	}
}

/**
 * Tries to create a projectile sprite and add it to the map,
 * calculating its trajectory.
 * @return True, if the projectile was successfully created.
 */
bool ProjectileFlyBState::createNewProjectile()
{
	++_action.autoShotCounter;

	// Special handling for "spray" auto attack, get target positions from the action's waypoints, starting from the back
	if (_action.sprayTargeting)
	{
		// Since we're just spraying, target the middle of the tile
		_targetVoxel = _action.waypoints.back();
		Position targetPosition = _targetVoxel.toTile();

		// The waypoint targeting is possibly out of range of the gun, so move the voxel to the max range of the gun if it is
		int distanceSq = _action.actor->distance3dToPositionSq(targetPosition);
		if (_action.weapon->getRules()->isOutOfRange(distanceSq))
		{
			Position actorPosition = _action.actor->getPosition();
			int maxRange = _action.weapon->getRules()->getMaxRange();
			int distance = (int)std::ceil(sqrt(float(distanceSq)));
			_targetVoxel = (actorPosition + (targetPosition - actorPosition) * maxRange / distance).toVoxel() + TileEngine::voxelTileCenter;
			targetPosition = _targetVoxel.toTile();
		}

		// Turn at the end (to a potentially modified target position)
		_unit->lookAt(targetPosition, _unit->getTurretType() != -1);
		while (_unit->getStatus() == STATUS_TURNING)
		{
			_unit->turn(_unit->getTurretType() != -1);
		}

		_action.waypoints.pop_back();
	}

	// create a new projectile
	Projectile *projectile = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, _targetVoxel, _ammo);

	// add the projectile on the map
	_parent->getMap()->setProjectile(projectile);

	// set the speed of the state think cycle to 16 ms (roughly one think cycle per frame)
	_parent->setStateInterval(1000/60);

	// let it calculate a trajectory
	_projectileImpact = V_EMPTY;

	double accuracyDivider = 100.0;
	// berserking units are half as accurate
	if (!_parent->getPanicHandled())
	{
		accuracyDivider = 200.0;
	}

	auto attack = BattleActionAttack::GetAferShoot(_action, _ammo);
	if (_action.type == BA_THROW)
	{
		_projectileImpact = projectile->calculateThrow(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider);
		const RuleItem *ruleItem = _action.weapon->getRules();
		if (_projectileImpact == V_FLOOR || _projectileImpact == V_UNIT || _projectileImpact == V_OBJECT)
		{
			if (_unit->getFaction() != FACTION_PLAYER && _unit->getFaction() != FACTION_ALIEN_PLAYER && ruleItem->getBattleType() == BT_GRENADE)
			{
				_action.weapon->setFuseTimer(ruleItem->getFuseTimerDefault());
			}
			_action.weapon->moveToOwner(nullptr);
			if (_action.weapon->getGlow())
			{
				_parent->getTileEngine()->calculateLighting(LL_UNITS, _unit->getPosition());
				_parent->getTileEngine()->calculateFOV(_unit->getPosition(), _action.weapon->getGlowRange(), false);
			}
			_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::ITEM_THROW)->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
		}
		else
		{
			// unable to throw here
			delete projectile;
			_parent->getMap()->setProjectile(0);
			_action.result = "STR_UNABLE_TO_THROW_HERE";
			_action.clearTU();
			_parent->popState();
			return false;
		}
	}
	else if (_action.weapon->getArcingShot(_action.type)) // special code for the "spit" trajectory
	{
		_projectileImpact = projectile->calculateThrow(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider);
		if (_projectileImpact != V_EMPTY && _projectileImpact != V_OUTOFBOUNDS)
		{
			// set the soldier in an aiming position
			_unit->aim(true);
			// and we have a lift-off
			if (_ammo->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _ammo->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
			}
			else if (_action.weapon->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _action.weapon->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
			}
			if (_action.type != BA_LAUNCH)
			{
				_action.weapon->spendAmmoForAction(_action.type, _parent->getSave());
			}
		}
		else
		{
			// no line of fire
			delete projectile;
			_parent->getMap()->setProjectile(0);
			if (_parent->getPanicHandled())
			{
				_action.result = "STR_NO_TRAJECTORY";
			}
			_unit->abortTurn();
			_parent->popState();
			return false;
		}
	}
	else
	{
		if (_originVoxel != TileEngine::invalid)
		{
			_projectileImpact = projectile->calculateTrajectory(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider, _originVoxel, false);
		}
		else
		{
			_projectileImpact = projectile->calculateTrajectory(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider);
		}
		if (_targetVoxel != TileEngine::invalid.toVoxel() && (_projectileImpact != V_EMPTY || _action.type == BA_LAUNCH))
		{
			// set the soldier in an aiming position
			_unit->aim(true);
			// and we have a lift-off
			if (_ammo->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _ammo->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(projectile->getOrigin()));
			}
			else if (_action.weapon->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _action.weapon->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(projectile->getOrigin()));
			}
			if (_action.type != BA_LAUNCH)
			{
				_action.weapon->spendAmmoForAction(_action.type, _parent->getSave());
			}
		}
		else
		{
			// no line of fire
			delete projectile;
			_parent->getMap()->setProjectile(0);
			if (_parent->getPanicHandled())
			{
				_action.result = "STR_NO_LINE_OF_FIRE";
			}
			_unit->abortTurn();
			_parent->popState();
			return false;
		}
	}

	if (_action.type != BA_THROW && _action.type != BA_LAUNCH)
		_unit->getStatistics()->shotsFiredCounter++;

	// hit log - new bullet
	if (_action.actor)
	{
		_parent->getSave()->appendToHitLog(HITLOG_NEW_SHOT, _action.actor->getFaction());
	}

	return true;
}

/**
 * Animates the projectile (moves to the next point in its trajectory).
 * If the animation is finished the projectile sprite is removed from the map,
 * and this state is finished.
 */
void ProjectileFlyBState::think()
{
	_parent->getSave()->getBattleState()->clearMouseScrollingState();
	/* TODO refactoring : store the projectile in this state, instead of getting it from the map each time? */
	if (_parent->getMap()->getProjectile() == 0)
	{
		bool hasFloor = _action.actor->haveNoFloorBelow() == false;
		bool unitCanFly = _action.actor->getMovementType() == MT_FLY;

		if (_action.weapon->haveNextShotsForAction(_action.type, _action.autoShotCounter)
			&& !_action.actor->isOut()
			&& _ammo->getAmmoQuantity() != 0
			&& (hasFloor || unitCanFly))
		{
			bool success = createNewProjectile();
			if (_action.cameraPosition.z != -1)
			{
				_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
				_parent->getMap()->invalidate();
			}
			if (!success)
			{
				_parent->getMap()->setFollowProjectile(true); // turn back on when done shooting
			}
		}
		else
		{
			_parent->getMap()->setFollowProjectile(true); // turn back on when done shooting
			if (_action.cameraPosition.z != -1 && _action.waypoints.size() <= 1)
			{
				_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
				_parent->getMap()->invalidate();
			}
			if (!_parent->getSave()->getUnitsFalling() && _parent->getPanicHandled())
			{
				_parent->getTileEngine()->checkReactionFire(_unit, _action);
			}
			if (!_unit->isOut())
			{
				_unit->abortTurn();
			}
			if (_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getSide() == FACTION_ALIEN_PLAYER || _parent->getSave()->getDebugMode())
			{
				_parent->setupCursor();
			}
			_parent->convertInfected();
			_parent->popState();
		}
	}
	else
	{
		auto attack = BattleActionAttack::GetAferShoot(_action, _ammo);
		if (_action.type != BA_THROW && _ammo && _ammo->getRules()->getShotgunPellets() != 0)
		{
			// shotgun pellets move to their terminal location instantly as fast as possible
			_parent->getMap()->getProjectile()->skipTrajectory();
		}
		if (!_parent->getMap()->getProjectile()->move())
		{
			// impact !
			if (_action.type == BA_THROW)
			{
				_parent->getMap()->resetCameraSmoothing();
				Position pos = _parent->getMap()->getProjectile()->getPosition(Projectile::ItemDropVoxelOffset).toTile();
				if (pos.y > _parent->getSave()->getMapSizeY())
				{
					pos.y--;
				}
				if (pos.x > _parent->getSave()->getMapSizeX())
				{
					pos.x--;
				}

				_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::ITEM_DROP)->play(-1, _parent->getMap()->getSoundAngle(pos));
				const RuleItem *ruleItem = _action.weapon->getRules();
				if (_action.weapon->fuseThrowEvent())
				{
					if (ruleItem->getBattleType() == BT_GRENADE || ruleItem->getBattleType() == BT_PROXIMITYGRENADE)
					{
						// it's a hot grenade to explode immediately
						_parent->statePushFront(new ExplosionBState(_parent, _parent->getMap()->getProjectile()->getPosition(Projectile::ItemDropVoxelOffset), attack));
					}
					else
					{
						_parent->getSave()->removeItem(_action.weapon);
					}
				}
				else
				{
					_parent->dropItem(pos, _action.weapon);
					if (_unit->getFaction() != FACTION_PLAYER && _unit->getFaction() != FACTION_ALIEN_PLAYER && ruleItem->getBattleType() == BT_GRENADE)
					{
						_parent->getTileEngine()->setDangerZone(pos, ruleItem->getExplosionRadius(attack), _action.actor);
					}
				}
			}
			else if (_action.type == BA_LAUNCH && _action.waypoints.size() > 1 && _projectileImpact == V_EMPTY)
			{
				_origin = _action.waypoints.front();
				_action.waypoints.pop_front();
				_action.target = _action.waypoints.front();
				// launch the next projectile in the waypoint cascade
				ProjectileFlyBState *nextWaypoint = new ProjectileFlyBState(_parent, _action, _origin, _range + _parent->getMap()->getProjectile()->getDistance());
				nextWaypoint->setOriginVoxel(_parent->getMap()->getProjectile()->getPosition(-1));
				if (_origin == _action.target)
				{
					nextWaypoint->targetFloor();
				}
				_parent->statePushNext(nextWaypoint);
			}
			else
			{
				auto tmpUnit = _parent->getSave()->getTile(_action.target)->getUnit();
				if (tmpUnit && tmpUnit != _unit)
				{
					tmpUnit->getStatistics()->shotAtCounter++; // Only counts for guns, not throws or launches
				}

				_parent->getMap()->resetCameraSmoothing();
				if (_action.type == BA_LAUNCH)
				{
					_action.weapon->spendAmmoForAction(_action.type, _parent->getSave());
				}

				if (_projectileImpact != V_OUTOFBOUNDS)
				{
					bool shotgun = _ammo && _ammo->getRules()->getShotgunPellets() != 0 && _ammo->getRules()->getDamageType()->isDirect();
					int offset = 0;
					// explosions impact not inside the voxel but two steps back (projectiles generally move 2 voxels at a time)
					if (_ammo && _ammo->getRules()->getExplosionRadius(attack) != 0 && _projectileImpact != V_UNIT)
					{
						offset = -2;
					}

					_parent->statePushFront(new ExplosionBState(
						_parent, _parent->getMap()->getProjectile()->getPosition(offset),
						attack, 0,
						_action.weapon->haveNextShotsForAction(_action.type, _action.autoShotCounter) || !_action.weapon->getAmmoForAction(_action.type),
						shotgun ? 0 : _range + _parent->getMap()->getProjectile()->getDistance()
					));

					if (_projectileImpact == V_UNIT)
					{
						projectileHitUnit(_parent->getMap()->getProjectile()->getPosition(offset));
					}

					// remember unit's original XP values, used for nerfing below
					_unit->rememberXP();

					// special shotgun behaviour: trace extra projectile paths, and add bullet hits at their termination points.
					if (shotgun)
					{
						int behaviorType = _ammo->getRules()->getShotgunBehaviorType();
						int spread = _ammo->getRules()->getShotgunSpread();
						int choke = _action.weapon->getRules()->getShotgunChoke();
						Position firstPelletImpact = _parent->getMap()->getProjectile()->getPosition(-2);
						Position originalTarget = _targetVoxel;

						int i = 1;
						while (i != _ammo->getRules()->getShotgunPellets())
						{
							if (behaviorType == 1)
							{
								// use impact location to determine spread (instead of originally targeted voxel), as long as it's not the same as the origin
								if (firstPelletImpact != _parent->getSave()->getTileEngine()->getOriginVoxel(_action, _parent->getSave()->getTile(_origin)))
								{
									_targetVoxel = firstPelletImpact;
								}
								else
								{
									_targetVoxel = originalTarget;
								}
							}


							Projectile *proj = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, _targetVoxel, _ammo);

							// let it trace to the point where it hits
							int secondaryImpact = V_EMPTY;
							if (behaviorType == 1)
							{
								// pellet spread based on spread and choke values
								secondaryImpact = proj->calculateTrajectory(std::max(0.0, (1.0 - spread / 100.0) * choke / 100.0));

							}
							else
							{
								// pellet spread based on spread and firing accuracy with diminishing formula
								// identical with vanilla formula when spread = 100 (default)
								secondaryImpact = proj->calculateTrajectory(std::max(0.0, (BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / 100.0) - i * 5.0 * spread / 100.0));
							}

							if (secondaryImpact != V_EMPTY)
							{
								// as above: skip the shot to the end of it's path
								proj->skipTrajectory();
								// insert an explosion and hit
								if (secondaryImpact != V_OUTOFBOUNDS)
								{
									if (secondaryImpact == V_UNIT)
									{
										projectileHitUnit(proj->getPosition(offset));
									}
									Explosion *explosion = new Explosion(proj->getPosition(offset), _ammo->getRules()->getHitAnimation(), 0, false, false, _ammo->getRules()->getHitAnimationFrames());
									int power = _ammo->getRules()->getPowerBonus(attack) - _ammo->getRules()->getPowerRangeReduction(proj->getDistance());
									_parent->getMap()->getExplosions()->push_back(explosion);
									_parent->getSave()->getTileEngine()->hit(attack, proj->getPosition(offset), power, _ammo->getRules()->getDamageType());

									//do not work yet
//									if (_ammo->getRules()->getExplosionRadius(_unit) != 0)
//									{
//										_parent->getTileEngine()->explode({ _action, _ammo }, proj->getPosition(offset), _ammo->getRules()->getPower(), _ammo->getRules()->getDamageType(), _ammo->getRules()->getExplosionRadius(), _unit);
//									}
								}
							}
							++i;
							delete proj;
						}

						// reset back for the next shot in the (potential) autoshot sequence
						_targetVoxel = originalTarget;
					}

					// nerf unit's XP values (gained via extra shotgun bullets)
					_unit->nerfXP();
				}
				else if (!_action.weapon->haveNextShotsForAction(_action.type, _action.autoShotCounter) || !_action.weapon->getAmmoForAction(_action.type))
				{
					_unit->aim(false);
				}
			}

			delete _parent->getMap()->getProjectile();
			_parent->getMap()->setProjectile(0);
		}
	}
}

/**
 * Flying projectiles cannot be cancelled,
 * but they can be "skipped".
 */
void ProjectileFlyBState::cancel()
{
	if (_parent->getMap()->getProjectile())
	{
		_parent->getMap()->getProjectile()->skipTrajectory();
		Position p = _parent->getMap()->getProjectile()->getPosition().toTile();
		if (!_parent->getMap()->getCamera()->isOnScreen(p, false, 0, false))
			_parent->getMap()->getCamera()->centerOnPosition(p);
	}
	if (_parent->areAllEnemiesNeutralized())
	{
		// stop autoshots when battle auto-ends
		_action.autoShotCounter = 1000;

		// Rationale: if there are any fatally wounded soldiers
		// the game still allows the player to resume playing the current turn (and heal them)
		// but we don't want to resume auto-shooting (it just looks silly)
	}
}

/**
 * Validates the throwing range.
 * @param action Pointer to throw action.
 * @param origin Position to throw from.
 * @param target Tile to throw to.
 * @param depth Battlescape depth.
 * @return True when the range is valid.
 */
bool ProjectileFlyBState::validThrowRange(BattleAction *action, Position origin, Tile *target, int depth)
{
	// note that all coordinates and thus also distances below are in number of tiles (not in voxels).
	if (action->type != BA_THROW)
	{
		return true;
	}
	int xdiff = action->target.x - action->actor->getPosition().x;
	int ydiff = action->target.y - action->actor->getPosition().y;
	int realDistanceSq = (xdiff * xdiff) + (ydiff * ydiff);

	if (depth > 0)
	{
		if (action->weapon->getRules()->getUnderwaterThrowRange() > 0)
		{
			return realDistanceSq <= action->weapon->getRules()->getUnderwaterThrowRangeSq();
		}
	}
	else
	{
		if (action->weapon->getRules()->getThrowRange() > 0)
		{
			return realDistanceSq <= action->weapon->getRules()->getThrowRangeSq();
		}
	}

	double realDistance = sqrt((double)realDistanceSq);

	int offset = 2;
	int zd = (origin.z)-((action->target.z * 24 + offset) - target->getTerrainLevel());
	int weight = action->weapon->getTotalWeight();
	double maxDistance = (getMaxThrowDistance(weight, action->actor->getBaseStats()->strength, zd) + 8) / 16.0;

	if (depth > 0 && Mod::EXTENDED_UNDERWATER_THROW_FACTOR > 0)
	{
		maxDistance = maxDistance * (double)Mod::EXTENDED_UNDERWATER_THROW_FACTOR / 100.0;
	}

	return realDistance <= maxDistance;
}

/**
 * Validates the throwing range.
 * @param weight the weight of the object.
 * @param strength the strength of the thrower.
 * @param level the difference in height between the thrower and the target.
 * @return the maximum throwing range.
 */
int ProjectileFlyBState::getMaxThrowDistance(int weight, int strength, int level)
{
	double curZ = level + 0.5;
	double dz = 1.0;
	int dist = 0;
	while (dist < 4000) //just in case
	{
		dist += 8;
		if (dz<-1)
			curZ -= 8;
		else
			curZ += dz * 8;

		if (curZ < 0 && dz < 0) //roll back
		{
			dz = std::max(dz, -1.0);
			if (std::abs(dz)>1e-10) //rollback horizontal
				dist -= curZ / dz;
			break;
		}
		dz -= (double)(50 * weight / strength)/100;
		if (dz <= -2.0) //become falling
			break;
	}
	return dist;
}

/**
 * Set the origin voxel, used for the blaster launcher.
 * @param pos the origin voxel.
 */
void ProjectileFlyBState::setOriginVoxel(const Position& pos)
{
	_originVoxel = pos;
}

/**
 * Set the boolean flag to angle a blaster bomb towards the floor.
 */
void ProjectileFlyBState::targetFloor()
{
	_targetFloor = true;
}

void ProjectileFlyBState::projectileHitUnit(Position pos)
{
	BattleUnit *victim = _parent->getSave()->getTile(pos.toTile())->getOverlappingUnit(_parent->getSave());
	BattleUnit *targetVictim = _parent->getSave()->getTile(_action.target)->getUnit(); // Who we were aiming at (not necessarily who we hit)
	if (victim && !victim->isOut())
	{
		victim->getStatistics()->hitCounter++;
		if (_unit->getOriginalFaction() == FACTION_PLAYER && victim->getOriginalFaction() == FACTION_PLAYER)
		{
			victim->getStatistics()->shotByFriendlyCounter++;
			_unit->getStatistics()->shotFriendlyCounter++;
		}
		if (victim == targetVictim) // Hit our target
		{
			_unit->getStatistics()->shotsLandedCounter++;
			if (Position::distance2d(_action.actor->getPosition(), victim->getPosition()) > 30)
			{
				_unit->getStatistics()->longDistanceHitCounter++;
			}
			if (BattleUnit::getFiringAccuracy(BattleActionAttack::GetAferShoot(_action, _ammo), _parent->getMod()) < Position::distance2d(_action.actor->getPosition(), victim->getPosition()))
			{
				_unit->getStatistics()->lowAccuracyHitCounter++;
			}
		}
		if (victim->getFaction() == FACTION_HOSTILE || victim->getFaction() == FACTION_ALIEN_PLAYER)
		{
			AIModule *ai = victim->getAIModule();
			if (ai != 0)
			{
				ai->setWasHitBy(_unit);
				_unit->setTurnsSinceSpotted(0);
				_unit->setTurnsLeftSpottedForSnipers(std::max(victim->getSpotterDuration(), _unit->getTurnsLeftSpottedForSnipers()));
			}
		}
	}
}

}
