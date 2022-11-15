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
#include "UnitFallBState.h"
#include <algorithm>
#include "TileEngine.h"
#include "Pathfinding.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Engine/Options.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Sets up an UnitFallBState.
 * @param parent Pointer to the Battlescape.
 */
UnitFallBState::UnitFallBState(BattlescapeGame *parent) : BattleState(parent), _terrain(0)
{

}

/**
 * Deletes the UnitWalkBState.
 */
UnitFallBState::~UnitFallBState()
{

}

/**
 * Initializes the state.
 */
void UnitFallBState::init()
{
	_terrain = _parent->getTileEngine();
	if (_parent->getSave()->getSide() == FACTION_PLAYER)
		_parent->setStateInterval(Options::battleXcomSpeed);
	else
		_parent->setStateInterval(Options::battleAlienSpeed);

}

/**
 * Runs state functionality every cycle.
 * Progresses the fall, updates the battlescape, ...
 */
void UnitFallBState::think()
{
	for (std::list<BattleUnit*>::iterator unit = _parent->getSave()->getFallingUnits()->begin(); unit != _parent->getSave()->getFallingUnits()->end();)
	{
		// I ain't got time to panic
		if ((*unit)->getStatus() == STATUS_TURNING || (*unit)->getStatus() == STATUS_PANICKING || (*unit)->getStatus() == STATUS_BERSERK)
		{
			(*unit)->abortTurn();
		}
		bool falling = true;
		int size = (*unit)->getArmor()->getSize() - 1;
		if ((*unit)->isOutThresholdExceed())
		{
			unit = _parent->getSave()->getFallingUnits()->erase(unit);
			continue;
		}

		if ((*unit)->getStatus() == STATUS_WALKING || (*unit)->getStatus() == STATUS_FLYING)
		{
			(*unit)->keepWalking(_parent->getSave(), true); // advances the phase

			++unit;
			continue;
		}

		falling = (*unit)->haveNoFloorBelow()
			&& (*unit)->getPosition().z != 0
			&& (*unit)->getMovementType() != MT_FLY
			&& (*unit)->getWalkingPhase() == 0;

		if (falling)
		{
			// Tile(s) unit is falling into.
			for (int x = size; x >= 0; --x)
			{
				for (int y = size; y >= 0; --y)
				{
					Tile *tileTarget = _parent->getSave()->getTile((*unit)->getPosition() + Position(x,y,-1));
					tilesToFallInto.push_back(tileTarget);
				}
			}
			std::list<BattleUnit*> *fallingUnits = _parent->getSave()->getFallingUnits();
			// Check each tile for units that need moving out of the way.
			for (std::vector<Tile*>::iterator i = tilesToFallInto.begin(); i < tilesToFallInto.end(); ++i)
			{
				BattleUnit *unitBelow = (*i)->getUnit();
				if (unitBelow
					&& !(std::find(fallingUnits->begin(), fallingUnits->end(), unitBelow) != fallingUnits->end())  // ignore falling units (including self)
					&& !(std::find(unitsToMove.begin(), unitsToMove.end(), unitBelow) != unitsToMove.end()))       // ignore already added units
				{
					unitsToMove.push_back(unitBelow);
				}
			}
		}

		// we are just standing around, we are done falling.
		if ((*unit)->getStatus() == STATUS_STANDING)
		{
			if (falling)
			{
				Position destination = (*unit)->getPosition() + Position(0,0,-1);
				(*unit)->startWalking(Pathfinding::DIR_DOWN, destination, _parent->getSave());
				++unit;
			}
			else
			{
				// if the unit burns floor tiles, burn floor tiles
				if ((*unit)->getSpecialAbility() == SPECAB_BURNFLOOR || (*unit)->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
				{
					(*unit)->getTile()->ignite(1);
					Position groundVoxel = ((*unit)->getPosition().toVoxel()) + Position(8,8,-((*unit)->getTile()->getTerrainLevel()));
					_parent->getTileEngine()->hit(BattleActionAttack{ BA_NONE, (*unit), }, groundVoxel, (*unit)->getBaseStats()->strength, _parent->getMod()->getDamageType(DT_IN), false);

					if ((*unit)->getStatus() != STATUS_STANDING) // ie: we burned a hole in the floor and fell through it
					{
						_parent->getPathfinding()->abortPath();
					}
				}
				// move our personal lighting with us
				auto change = _parent->checkForProximityGrenades(*unit);
				_terrain->calculateLighting(change ? LL_ITEMS : LL_UNITS, (*unit)->getPosition(), 2);
				_terrain->calculateFOV((*unit)->getPosition(), 2, false); //update everyone else to see this unit, as well as all this unit's visible units.
				_terrain->calculateFOV((*unit), true, false); //update tiles
				if ((*unit)->getStatus() == STATUS_STANDING)
				{
					BattleAction fall;
					fall.type = BA_WALK;
					fall.actor = *unit;
					if (_parent->getTileEngine()->checkReactionFire(*unit, fall))
						_parent->getPathfinding()->abortPath();
					unit = _parent->getSave()->getFallingUnits()->erase(unit);
				}
			}
		}
		else
		{
			++unit;
		}
	}

	// Find somewhere to move the unit(s) In danger of being squashed.
	if (!unitsToMove.empty())
	{
		std::vector<Tile*> escapeTiles;
		for (auto ub = unitsToMove.begin(); ub < unitsToMove.end(); )
		{
			BattleUnit *unitBelow = (*ub);
			bool escapeFound = false;

			// We need to move all sections of the unit out of the way.
			std::vector<Position> bodySections;
			for (int x = unitBelow->getArmor()->getSize() - 1; x >= 0; --x)
			{
				for (int y = unitBelow->getArmor()->getSize() - 1; y >= 0; --y)
				{
					Position bs = unitBelow->getPosition() + Position(x, y, 0);
					bodySections.push_back(bs);
				}
			}

			// Check in each compass direction.
			for (int dir = 0; dir < Pathfinding::DIR_UP && !escapeFound; dir++)
			{
				Position offset;
				Pathfinding::directionToVector(dir, &offset);

				for (std::vector<Position>::iterator bs = bodySections.begin(); bs < bodySections.end(); )
				{
					Position originalPosition = (*bs);
					Position endPosition = originalPosition + offset;
					Tile *t = _parent->getSave()->getTile(endPosition);

					bool aboutToBeOccupiedFromAbove = t && std::find(tilesToFallInto.begin(), tilesToFallInto.end(), t) != tilesToFallInto.end();
					bool alreadyTaken = t && std::find(escapeTiles.begin(), escapeTiles.end(), t) != escapeTiles.end();
					bool alreadyOccupied = t && t->getUnit() && (t->getUnit() != unitBelow);
					_parent->getSave()->getPathfinding()->setUnit(*ub); //TODO: remove as was done by `getTUCost`
					auto r = _parent->getSave()->getPathfinding()->getTUCost(originalPosition, dir, *ub, 0, BAM_NORMAL);
					bool movementBlocked = r.cost.time == Pathfinding::INVALID_MOVE_COST;
					endPosition = r.pos;
					bool hasFloor = t && !t->hasNoFloor(_parent->getSave());
					bool unitCanFly = unitBelow->getMovementType() == MT_FLY;

					bool canMoveToTile = t && !alreadyOccupied && !alreadyTaken && !aboutToBeOccupiedFromAbove && !movementBlocked && (hasFloor || unitCanFly);
					if (canMoveToTile)
					{
						// Check next section of the unit.
						++bs;
					}
					else
					{
						// Try next direction.
						break;
					}

					// If all sections of the fallen onto unit can be moved, then we move it.
					if (bs == bodySections.end())
					{
						if (_parent->getSave()->addFallingUnit(unitBelow))
						{
							escapeFound = true;
							// Now ensure no other unit escapes to here too.
							for (int x = unitBelow->getArmor()->getSize() - 1; x >= 0; --x)
							{
								for (int y = unitBelow->getArmor()->getSize() - 1; y >= 0; --y)
								{
									Tile *et = _parent->getSave()->getTile(t->getPosition() + Position(x,y,0));
									escapeTiles.push_back(et);
								}
							}

							unitBelow->startWalking(dir, unitBelow->getPosition() + offset, _parent->getSave());
							ub = unitsToMove.erase(ub);
						}
					}
				}
			}
			if (!escapeFound)
			{
				// STOMP THAT GOOMBAH!
				unitBelow->knockOut(_parent);
				ub = unitsToMove.erase(ub);
			}
		}
		_parent->checkForCasualties(nullptr, BattleActionAttack{ BA_NONE, nullptr });
	}

	if (_parent->getSave()->getFallingUnits()->empty())
	{
		tilesToFallInto.clear();
		unitsToMove.clear();
		_parent->popState();
		return;
	}
}

}
