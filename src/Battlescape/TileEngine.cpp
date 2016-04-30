/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#define _USE_MATH_DEFINES
#include <assert.h>
#include <cmath>
#include <climits>
#include <set>
#include "TileEngine.h"
#include <SDL.h>
#include "AlienBAIState.h"
#include "Map.h"
#include "Camera.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "ExplosionBState.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Engine/RNG.h"
#include "BattlescapeState.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/MapData.h"
#include "../Mod/Unit.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "Pathfinding.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "ProjectileFlyBState.h"
#include "MeleeAttackBState.h"
#include "../Engine/Logger.h"
#include "../fmath.h"

namespace OpenXcom
{

const int TileEngine::heightFromCenter[11] = {0,-2,+2,-4,+4,-6,+6,-8,+8,-12,+12};

/**
 * Sets up a TileEngine.
 * @param save Pointer to SavedBattleGame object.
 * @param voxelData List of voxel data.
 * @param maxViewDistance Max view distance in tiles.
 * @param maxDarknessToSeeUnits Threshold of darkness for LoS calculation.
 */
TileEngine::TileEngine(SavedBattleGame *save, std::vector<Uint16> *voxelData, int maxViewDistance, int maxDarknessToSeeUnits) :
	_save(save), _voxelData(voxelData), _personalLighting(true),
	_maxViewDistance(maxViewDistance), _maxViewDistanceSq(maxViewDistance * maxViewDistance),
	_maxVoxelViewDistance(maxViewDistance * 16), _maxDarknessToSeeUnits(maxDarknessToSeeUnits)
{
}

/**
 * Deletes the TileEngine.
 */
TileEngine::~TileEngine()
{

}

/**
  * Calculates sun shading for the whole terrain.
  */
void TileEngine::calculateSunShading()
{
	const int layer = 0; // Ambient lighting layer.

	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		_save->getTile(i)->resetLight(layer);
		calculateSunShading(_save->getTile(i));
	}
}

/**
* Calculates sun shading for all tiles in the column at the given X - Y - coordinates.
* @param pos The position to calculate sun shading for.Z - coordinate is ignored.
*/
void TileEngine::calculateSunShading(const Position &pos)
{
	Position toUpdate = pos;
	for (int z = _save->getMapSizeZ() - 1; z >= 0; z--)
	{
		toUpdate.z = z;
		_save->getTile(toUpdate)->resetLight(0); //reset ambient lighting
		calculateSunShading(_save->getTile(toUpdate));
	}
}

/**
  * Calculates sun shading for 1 tile. Sun comes from above and is blocked by floors or objects.
  * TODO: angle the shadow according to the time? - link to Options::globeSeasons (or whatever the realistic lighting one is)
  * @param tile The tile to calculate sun shading for.
  */
void TileEngine::calculateSunShading(Tile *tile)
{
	const int layer = 0; // Ambient lighting layer.

	int power = 15 - _save->getGlobalShade();

	// At night/dusk sun isn't dropping shades blocked by roofs
	if (_save->getGlobalShade() <= 4)
	{
		int block = 0;
		int x = tile->getPosition().x;
		int y = tile->getPosition().y;
		for (int z = _save->getMapSizeZ()-1; z > tile->getPosition().z ; z--)
		{
			block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, DT_NONE);
			block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, DT_NONE, Pathfinding::DIR_DOWN);
		}
		if (block>0)
		{
			power -= 2;
		}
	}
	tile->addLight(power, layer);
}

/**
  * Recalculates lighting for the terrain: objects,items,fire.
  */
void TileEngine::calculateTerrainLighting()
{
	const int layer = 1; // Static lighting layer.
	const int fireLightPower = 15; // amount of light a fire generates

	// reset all light to 0 first
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		_save->getTile(i)->resetLight(layer);
	}

	// add lighting of terrain
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		// only floors and objects can light up
		if (_save->getTile(i)->getMapData(O_FLOOR)
			&& _save->getTile(i)->getMapData(O_FLOOR)->getLightSource())
		{
			addLight(_save->getTile(i)->getPosition(), _save->getTile(i)->getMapData(O_FLOOR)->getLightSource(), layer);
		}
		if (_save->getTile(i)->getMapData(O_OBJECT)
			&& _save->getTile(i)->getMapData(O_OBJECT)->getLightSource())
		{
			addLight(_save->getTile(i)->getPosition(), _save->getTile(i)->getMapData(O_OBJECT)->getLightSource(), layer);
		}

		// fires
		if (_save->getTile(i)->getFire())
		{
			addLight(_save->getTile(i)->getPosition(), fireLightPower, layer);
		}

		for (std::vector<BattleItem*>::iterator it = _save->getTile(i)->getInventory()->begin(); it != _save->getTile(i)->getInventory()->end(); ++it)
		{
			if ((*it)->getGlow())
			{
				addLight(_save->getTile(i)->getPosition(), (*it)->getRules()->getPower(), layer);
			}
		}

	}

}

/**
  * Recalculates lighting for the units.
  */
void TileEngine::calculateUnitLighting()
{
	const int layer = 2; // Dynamic lighting layer.
	const int personalLightPower = 15; // amount of light a unit generates
	const int fireLightPower = 15; // amount of light a fire generates

	// reset all light to 0 first
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		_save->getTile(i)->resetLight(layer);
	}

	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		// add lighting of soldiers
		if (_personalLighting && (*i)->getFaction() == FACTION_PLAYER && !(*i)->isOut())
		{
			addLight((*i)->getPosition(), personalLightPower, layer);
		}
		// add lighting of units on fire
		if ((*i)->getFire())
		{
			addLight((*i)->getPosition(), fireLightPower, layer);
		}
	}
}

/**
 * Adds circular light pattern starting from center and losing power with distance travelled.
 * @param center Center.
 * @param power Power.
 * @param layer Light is separated in 3 layers: Ambient, Static and Dynamic.
 */
void TileEngine::addLight(const Position &center, int power, int layer)
{
	// only loop through the positive quadrant.
	for (int x = 0; x <= power; ++x)
	{
		for (int y = 0; y <= power; ++y)
		{
			for (int z = 0; z < _save->getMapSizeZ(); z++)
			{
				int distance = (int)Round(sqrt(float(x*x + y*y)));

				if (_save->getTile(Position(center.x + x,center.y + y, z)))
					_save->getTile(Position(center.x + x,center.y + y, z))->addLight(power - distance, layer);

				if (_save->getTile(Position(center.x - x,center.y - y, z)))
					_save->getTile(Position(center.x - x,center.y - y, z))->addLight(power - distance, layer);

				if (_save->getTile(Position(center.x - x,center.y + y, z)))
					_save->getTile(Position(center.x - x,center.y + y, z))->addLight(power - distance, layer);

				if (_save->getTile(Position(center.x + x,center.y - y, z)))
					_save->getTile(Position(center.x + x,center.y - y, z))->addLight(power - distance, layer);
			}
		}
	}
}

/**
* Setups the internal event visibility search space reduction system. This system defines a narrow circle sector around
* a given event as viewed from an external observer. This allows narrowing down which tiles/units may need to be updated for
* the observer based on the event affecting visibility at the event itself and beyond it in its direction.
* Imagines a circle around the event of eventRadius, calculates its tangents, and places points at the circle's tangent 
* intersections for later bounds checking.
* @param observerPos Position of the observer of this event.
* @param eventPos The centre of the event. Ie a moving unit's position, centre of explosion, a single destroyed tile, etc.
* @param eventRadius Radius big enough to fully envelop the event. Ie for a single tile change, set radius to 1.
*/
void TileEngine::setupEventVisibilitySector(const Position &observerPos, const Position &eventPos, const int &eventRadius)
{
	Position posDiff = eventPos - observerPos;
	float a = asinf(eventRadius / sqrtf(posDiff.x * posDiff.x + posDiff.y * posDiff.y));
	float b = atan2f(posDiff.y, posDiff.x);
	float t1 = b - a;
	float t2 = b + a;
	//Define the points where the lines tangent to the circle intersect it. Note: resulting positions are relative to observer, not in direct tile space.
	_eventVisibilitySectorL.x = roundf(eventPos.x + eventRadius * sinf(t1)) - observerPos.x;
	_eventVisibilitySectorL.y = roundf(eventPos.y - eventRadius * cosf(t1)) - observerPos.y;
	_eventVisibilitySectorR.x = roundf(eventPos.x - eventRadius * sinf(t2)) - observerPos.x;
	_eventVisibilitySectorR.y = roundf(eventPos.y + eventRadius * cosf(t2)) - observerPos.y;
	_eventVisibilityObserverPos = observerPos;
}

/**
* Checks whether toCheck is within a previously setup eventVisibilitySector. See setupEventVisibilitySector(...).
* May be used to rapidly reduce the searchspace when updating unit and tile visibility.
* @param toCheck The position to check.
* @return true if within the circle sector.
*/
inline bool TileEngine::inEventVisibilitySector(const Position &toCheck) const
{
	Position posDiff = toCheck - _eventVisibilityObserverPos;
	//Is toCheck within the arc as defined by the two tangent points?
	return (!(-_eventVisibilitySectorL.x * posDiff.y + _eventVisibilitySectorL.y * posDiff.x > 0) &&
		(-_eventVisibilitySectorR.x * posDiff.y + _eventVisibilitySectorR.y * posDiff.x > 0));
}

/**
* Updates line of sight of a single soldier in a narrow arc around a given event position.
* @param unit Unit to check line of sight of.
* @param eventPos The centre of the event which necessitated the FOV update. Used to optimize which tiles to update.
* @param eventRadius The radius of a circle able to fully encompass the event, in tiles. Hence: 1 for a singletile event.
* @return True when new aliens are spotted.
*/
bool TileEngine::calculateUnitsInFOV(BattleUnit* unit, const Position eventPos, const int eventRadius)
{
	size_t oldNumVisibleUnits = unit->getUnitsSpottedThisTurn().size();
	bool useTurretDirection = false;
	bool selfWithinEventRadius = false;
	if (Options::strafe && (unit->getTurretType() > -1)) {
		useTurretDirection = true;
	}

	if (unit->isOut())
		return false;

	Position posSelf = unit->getPosition();
	if (eventRadius == 0 || eventPos == Position(-1, -1, -1) || distanceSq(posSelf, eventPos) <= eventRadius * eventRadius)
	{
		//Asked to do a full check. Or the event is overlapping our tile. Better check everything.
		selfWithinEventRadius = true;
		unit->clearVisibleUnits();
	} 
	else 
	{
		//Use search space reduction by updating within a narrow circle sector covering the event and any 
		//units beyond it (they can now be hidden or revealed based on what occured at the event position)
		//So, with a circle at eventPos of radius eventRadius, define its tangent points as viewed from this unit:
		setupEventVisibilitySector(posSelf, eventPos, eventRadius);
	}

	//Loop through all units specified and figure out which ones we can actually see.
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		Position posOther = (*i)->getPosition();
		if (!(*i)->isOut() && (unit->getId() != (*i)->getId()))
		{
			int sizeOther = (*i)->getArmor()->getSize();
			for (int x = 0; x < sizeOther; ++x)
			{
				for (int y = 0; y < sizeOther; ++y)
				{
					Position posToCheck = posOther + Position(x, y, 0);
					//If we can now find any unit within the arc defined by the event tangent points, its visibility may have been affected by the event.
					if (selfWithinEventRadius || inEventVisibilitySector(posToCheck))
					{
						if (!unit->checkViewSector(posToCheck, useTurretDirection))
						{
							//Unit within arc, but not in view sector. If it just walked out we need to remove it.
							unit->removeFromVisibleUnits((*i));
						}
						else if (visible(unit, _save->getTile(posToCheck))) // (distance is checked here)
						{
							//Unit (or part thereof) visible to one or more eyes of this unit.
							if (unit->getFaction() == FACTION_PLAYER)
							{
								(*i)->setVisible(true);
							}
							if ((( (*i)->getFaction() == FACTION_HOSTILE && unit->getFaction() == FACTION_PLAYER )
								|| ( (*i)->getFaction() != FACTION_HOSTILE && unit->getFaction() == FACTION_HOSTILE ))
								&& !unit->hasVisibleUnit((*i)))
							{
								unit->addToVisibleUnits((*i));
								unit->addToVisibleTiles((*i)->getTile());

								if (unit->getFaction() == FACTION_HOSTILE && (*i)->getFaction() != FACTION_HOSTILE)
								{
									(*i)->setTurnsSinceSpotted(0);
								}
							}

							x = y = sizeOther; //If a unit's tile is visible there's no need to check the others: break the loops.
						}
						else
						{
							//Within arc, but not visible. Need to check to see if whatever happened at eventPos blocked a previously seen unit.
							unit->removeFromVisibleUnits((*i));
						}
					}
				}
			}
		}
	}
	// we only react when there are at least the same amount of visible units as before AND the checksum is different
	// this way we stop if there are the same amount of visible units, but a different unit is seen
	// or we stop if there are more visible units seen
	if (unit->getUnitsSpottedThisTurn().size() > oldNumVisibleUnits && !unit->getVisibleUnits()->empty())
	{
		return true;
	}
	return false;
}

/**
* Calculates line of sight of tiles for a player controlled soldier.
* If supplied with an event position differing from the soldier's position, it will only
* calculate tiles within a narrow arc.
* @param unit Unit to check line of sight of.
* @param eventPos The centre of the event which necessitated the FOV update. Used to optimize which tiles to update.
* @param eventRadius The radius of a circle able to fully encompass the event, in tiles. Hence: 1 for a singletile event.
*/
void TileEngine::calculateTilesInFOV(BattleUnit *unit, const Position eventPos, const int eventRadius)
{
	bool useTurretDirection = false;
	bool skipNarrowArcTest = false;
	int direction;
	if (Options::strafe && (unit->getTurretType() > -1)) {
		direction = unit->getTurretDirection();
		useTurretDirection = true;
	}
	else
	{
		direction = unit->getDirection();
	}
	if (unit->getFaction() != FACTION_PLAYER || (eventRadius == 1 && !unit->checkViewSector(eventPos, useTurretDirection)))
	{
		//The event wasn't meant for us and/or visible for us.
		return;
	}
	else if (unit->isOut())
	{
		unit->clearVisibleTiles();
		return;
	}
	Position posSelf = unit->getPosition();
	if (eventPos == Position(-1,-1,-1) || eventPos == unit->getPosition() || distanceSq(posSelf, eventPos) <= eventRadius * eventRadius)
	{
		//Asked to do a full check. Or unit within event. Should update all.
		unit->clearVisibleTiles();
		skipNarrowArcTest = true;
	}
	else {
		//Use search space reduction by updating within a narrow circle sector covering the event and any 
		//tiles beyond it (changes in terrain can reveal tiles beyond the event as well)
		//So, with a circle at eventPos of radius eventRadius, define its tangent points as viewed from this unit:
		setupEventVisibilitySector(posSelf, eventPos, eventRadius);
	}
	
	//Only recalculate bresenham lines to tiles that are at the event or further away.
	const int distanceSqrMin = skipNarrowArcTest ? 0 : std::max(distanceSq(posSelf, eventPos) - eventRadius*eventRadius, 0); 
	
	//Variables for finding the tiles to test based on the view direction.
	Position posTest;
	std::vector<Position> _trajectory;
	bool swap = (direction == 0 || direction == 4);
	const int signX[8] = { +1, +1, +1, +1, -1, -1, -1, -1 };
	const int signY[8] = { -1, -1, -1, +1, +1, +1, -1, -1 };
	int y1, y2;

	if ((unit->getHeight() + unit->getFloatHeight() + -_save->getTile(unit->getPosition())->getTerrainLevel()) >= 24 + 4)
	{
		Tile *tileAbove = _save->getTile(posSelf + Position(0, 0, 1));
		if (tileAbove && tileAbove->hasNoFloor(0))
		{
			++posSelf.z;
		}
	}
	//Test all tiles within view cone for visibility.
	for (int x = 0; x <= getMaxViewDistance(); ++x) //TODO: Possible improvement: find the intercept points of the arc at max view distance and choose a more intelligent sweep of values when an event arc is defined.
	{
		if (direction & 1)
		{
			y1 = 0;
			y2 = getMaxViewDistance();
		}
		else
		{
			y1 = -x;
			y2 = x;
		}
		for (int y = y1; y <= y2; ++y) //TODO: Possible improvement: find the intercept points of the arc at max view distance and choose a more intelligent sweep of values when an event arc is defined.
		{
			const int distanceSqr = x*x + y*y;
			if (distanceSqr <= getMaxViewDistanceSq() && distanceSqr >= distanceSqrMin)
			{
				posTest.x = posSelf.x + signX[direction] * (swap ? y : x);
				posTest.y = posSelf.y + signY[direction] * (swap ? x : y);
				//Only continue if the column of tiles at (x,y) is within the narrow arc of interest (if enabled)
				if (skipNarrowArcTest || inEventVisibilitySector(posTest))
				{
					for (int z = 0; z < _save->getMapSizeZ(); z++)
					{
						posTest.z = z;

						if (_save->getTile(posTest)) //inside map?
						{
							// this sets tiles to discovered if they are in LOS - tile visibility is not calculated in voxelspace but in tilespace
							// large units have "4 pair of eyes"
							int size = unit->getArmor()->getSize();
							for (int xo = 0; xo < size; xo++)
							{
								for (int yo = 0; yo < size; yo++)
								{
									Position poso = posSelf + Position(xo, yo, 0);
									_trajectory.clear();
									int tst = calculateLine(poso, posTest, true, &_trajectory, unit, false);
									if (tst > 127)
									{
										//Vision impacted something before reaching posTest. Throw away the impact point.
										_trajectory.pop_back();
									}
									//Reveal all tiles along line of vision. Note: needed due to width of bresenham stroke.
									for (std::vector<Position>::iterator i = _trajectory.begin(); i != _trajectory.end(); ++i)
									{
										Position posVisited = (*i);
										//Add tiles to the visible list only once. BUT we still need to calculate the whole trajectory as
										// this bresenham line's period might be different from the one that originally revealed the tile.
										if (!unit->hasVisibleTile(_save->getTile(posVisited)))
										{
											unit->addToVisibleTiles(_save->getTile(posVisited));
											_save->getTile(posVisited)->setVisible(+1);
											_save->getTile(posVisited)->setDiscovered(true, 2);

											// walls to the east or south of a visible tile, we see that too
											Tile* t = _save->getTile(Position(posVisited.x + 1, posVisited.y, posVisited.z));
											if (t) t->setDiscovered(true, 0);
											t = _save->getTile(Position(posVisited.x, posVisited.y + 1, posVisited.z));
											if (t) t->setDiscovered(true, 1);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
* Recalculates line of sight of a soldier.
* @param unit Unit to check line of sight of.
* @param doTileRecalc True (default) to recalculate the visible tiles for this unit.
* @param doUnitRecalc True (default) to recalculate the visible units for this unit.
* @return True when new aliens are spotted.
*/
bool TileEngine::calculateFOV(BattleUnit *unit, bool doTileRecalc, bool doUnitRecalc)
{
	//Force a full FOV recheck for this unit.
	if (doTileRecalc) calculateTilesInFOV(unit);
	return doUnitRecalc ? calculateUnitsInFOV(unit) : false;
}

/**
 * Gets the origin voxel of a unit's eyesight (from just one eye or something? Why is it x+7??
 * @param currentUnit The watcher.
 * @return Approximately an eyeball voxel.
 */
Position TileEngine::getSightOriginVoxel(BattleUnit *currentUnit)
{
	// determine the origin and target voxels for the raytrace
	Position originVoxel;
	originVoxel = Position((currentUnit->getPosition().x * 16) + 7, (currentUnit->getPosition().y * 16) + 8, currentUnit->getPosition().z*24);
	originVoxel.z += -_save->getTile(currentUnit->getPosition())->getTerrainLevel();
	originVoxel.z += currentUnit->getHeight() + currentUnit->getFloatHeight() - 1; //one voxel lower (eye level)
	Tile *tileAbove = _save->getTile(currentUnit->getPosition() + Position(0,0,1));
	if (currentUnit->getArmor()->getSize() > 1)
	{
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 1; //topmost voxel
	}
	if (originVoxel.z >= (currentUnit->getPosition().z + 1)*24 && (!tileAbove || !tileAbove->hasNoFloor(0)))
	{
		while (originVoxel.z >= (currentUnit->getPosition().z + 1)*24)
		{
			originVoxel.z--;
		}
	}

	return originVoxel;
}

/**
 * Checks for an opposing unit on this tile.
 * @param currentUnit The watcher.
 * @param tile The tile to check for
 * @return True if visible.
 */
bool TileEngine::visible(BattleUnit *currentUnit, Tile *tile)
{
	// if there is no tile or no unit, we can't see it
	if (!tile || !tile->getUnit())
	{
		return false;
	}

	// friendlies are always seen
	if (currentUnit->getFaction() == tile->getUnit()->getFaction()) return true;

	// if beyond global max. range, nobody can see anyone
	if (distanceSq(currentUnit->getPosition(), tile->getPosition(), false) > getMaxViewDistanceSq())
	{
		return false;
	}

	// during dark aliens can see 20 tiles, xcom can see 9 by default... unless overridden by armor
	if (tile->getExternalShade() > getMaxDarknessToSeeUnits())
	{
		if (getMaxViewDistanceSq() > currentUnit->getMaxViewDistanceAtDarkSq() &&
			distanceSq(currentUnit->getPosition(), tile->getPosition(), false) > currentUnit->getMaxViewDistanceAtDarkSq() &&
			tile->getUnit()->getFire() == 0)
		{
			return false;
		}
	}
	// during day (or if enough other light) both see 20 tiles ... unless overridden by armor
	else
	{
		if (getMaxViewDistanceSq() > currentUnit->getMaxViewDistanceAtDaySq() &&
			distanceSq(currentUnit->getPosition(), tile->getPosition(), false) > currentUnit->getMaxViewDistanceAtDaySq())
		{
			return false;
		}
	}

	Position originVoxel = getSightOriginVoxel(currentUnit);

	Position scanVoxel;
	std::vector<Position> _trajectory;
	bool unitSeen = canTargetUnit(&originVoxel, tile, &scanVoxel, currentUnit);

	if (unitSeen)
	{
		// now check if we really see it taking into account smoke tiles
		// initial smoke "density" of a smoke grenade is around 15 per tile
		// we do density/3 to get the decay of visibility
		// so in fresh smoke we should only have 4 tiles of visibility
		// this is traced in voxel space, with smoke affecting visibility every step of the way
		_trajectory.clear();
		calculateLine(originVoxel, scanVoxel, true, &_trajectory, currentUnit);
		int visibleDistance = _trajectory.size();
		int densityOfSmoke = 0;
		Position voxelToTile(16, 16, 24);
		Position trackTile(-1, -1, -1);
		Tile *t = 0;

		for (int i = 0; i < visibleDistance; i++)
		{
			_trajectory.at(i) /= voxelToTile;
			if (trackTile != _trajectory.at(i))
			{
				// 3  - coefficient of calculation (see above).
				// 20 - maximum view distance in vanilla Xcom.
				// Even if MaxViewDistance will be increased via ruleset, smoke will keep effect.
				if (visibleDistance > getMaxVoxelViewDistance() - densityOfSmoke * getMaxViewDistance()/(3 * 20))
				{
					return false;
				}
				trackTile = _trajectory.at(i);
				t = _save->getTile(trackTile);
			}
			if (t->getFire() == 0)
			{
				densityOfSmoke += t->getSmoke();
			}
		}
		unitSeen = visibleDistance <= getMaxVoxelViewDistance() - densityOfSmoke * getMaxViewDistance()/(3 * 20);
	}
	return unitSeen;
}

/**
 * Checks for how exposed unit is for another unit.
 * @param originVoxel Voxel of trace origin (eye or gun's barrel).
 * @param tile The tile to check for.
 * @param excludeUnit Is self (not to hit self).
 * @param excludeAllBut [Optional] is unit which is the only one to be considered for ray hits.
 * @return Degree of exposure (as percent).
 */
int TileEngine::checkVoxelExposure(Position *originVoxel, Tile *tile, BattleUnit *excludeUnit, BattleUnit *excludeAllBut)
{
	Position targetVoxel = Position((tile->getPosition().x * 16) + 7, (tile->getPosition().y * 16) + 8, tile->getPosition().z * 24);
	Position scanVoxel;
	std::vector<Position> _trajectory;
	BattleUnit *otherUnit = tile->getUnit();
	if (otherUnit == 0) return 0; //no unit in this tile, even if it elevated and appearing in it.
	if (otherUnit == excludeUnit) return 0; //skip self

	int targetMinHeight = targetVoxel.z - tile->getTerrainLevel();
	if (otherUnit)
		 targetMinHeight += otherUnit->getFloatHeight();

	// if there is an other unit on target tile, we assume we want to check against this unit's height
	int heightRange;

	int unitRadius = otherUnit->getLoftemps(); //width == loft in default loftemps set
	if (otherUnit->getArmor()->getSize() > 1)
	{
		unitRadius = 3;
	}

	// vector manipulation to make scan work in view-space
	Position relPos = targetVoxel - *originVoxel;
	float normal = unitRadius/sqrt((float)(relPos.x*relPos.x + relPos.y*relPos.y));
	int relX = floor(((float)relPos.y)*normal+0.5);
	int relY = floor(((float)-relPos.x)*normal+0.5);

	int sliceTargets[10]={0,0, relX,relY, -relX,-relY};

	if (!otherUnit->isOut())
	{
		heightRange = otherUnit->getHeight();
	}
	else
	{
		heightRange = 12;
	}

	int targetMaxHeight=targetMinHeight+heightRange;
	// scan ray from top to bottom  plus different parts of target cylinder
	int total=0;
	int visible=0;
	for (int i = heightRange; i >=0; i-=2)
	{
		++total;
		scanVoxel.z=targetMinHeight+i;
		for (int j = 0; j < 2; ++j)
		{
			scanVoxel.x=targetVoxel.x + sliceTargets[j*2];
			scanVoxel.y=targetVoxel.y + sliceTargets[j*2+1];
			_trajectory.clear();
			int test = calculateLine(*originVoxel, scanVoxel, false, &_trajectory, excludeUnit, true, false, excludeAllBut);
			if (test == V_UNIT)
			{
				//voxel of hit must be inside of scanned box
				if (_trajectory.at(0).x/16 == scanVoxel.x/16 &&
					_trajectory.at(0).y/16 == scanVoxel.y/16 &&
					_trajectory.at(0).z >= targetMinHeight &&
					_trajectory.at(0).z <= targetMaxHeight)
				{
					++visible;
				}
			}
		}
	}
	return (visible*100)/total;
}

/**
 * Checks for another unit available for targeting and what particular voxel.
 * @param originVoxel Voxel of trace origin (eye or gun's barrel).
 * @param tile The tile to check for.
 * @param scanVoxel is returned coordinate of hit.
 * @param excludeUnit is self (not to hit self).
 * @param potentialUnit is a hypothetical unit to draw a virtual line of fire for AI. if left blank, this function behaves normally.
 * @return True if the unit can be targetted.
 */
bool TileEngine::canTargetUnit(Position *originVoxel, Tile *tile, Position *scanVoxel, BattleUnit *excludeUnit, BattleUnit *potentialUnit)
{
	Position targetVoxel = Position((tile->getPosition().x * 16) + 7, (tile->getPosition().y * 16) + 8, tile->getPosition().z * 24);
	std::vector<Position> _trajectory;
	bool hypothetical = potentialUnit != 0;
	if (potentialUnit == 0)
	{
		potentialUnit = tile->getUnit();
		if (potentialUnit == 0) return false; //no unit in this tile, even if it elevated and appearing in it.
	}

	if (potentialUnit == excludeUnit) return false; //skip self

	int targetMinHeight = targetVoxel.z - tile->getTerrainLevel();
	targetMinHeight += potentialUnit->getFloatHeight();

	int targetMaxHeight = targetMinHeight;
	int targetCenterHeight;
	// if there is an other unit on target tile, we assume we want to check against this unit's height
	int heightRange;

	int unitRadius = potentialUnit->getLoftemps(); //width == loft in default loftemps set
	int targetSize = potentialUnit->getArmor()->getSize() - 1;
	int xOffset = potentialUnit->getPosition().x - tile->getPosition().x;
	int yOffset = potentialUnit->getPosition().y - tile->getPosition().y;
	if (targetSize > 0)
	{
		unitRadius = 3;
	}
	// vector manipulation to make scan work in view-space
	Position relPos = targetVoxel - *originVoxel;
	float normal = unitRadius/sqrt((float)(relPos.x*relPos.x + relPos.y*relPos.y));
	int relX = floor(((float)relPos.y)*normal+0.5);
	int relY = floor(((float)-relPos.x)*normal+0.5);

	int sliceTargets[10]={0,0, relX,relY, -relX,-relY, relY,-relX, -relY,relX};

	if (!potentialUnit->isOut())
	{
		heightRange = potentialUnit->getHeight();
	}
	else
	{
		heightRange = 12;
	}

	targetMaxHeight += heightRange;
	targetCenterHeight=(targetMaxHeight+targetMinHeight)/2;
	heightRange/=2;
	if (heightRange>10) heightRange=10;
	if (heightRange<=0) heightRange=0;

	// scan ray from top to bottom  plus different parts of target cylinder
	for (int i = 0; i <= heightRange; ++i)
	{
		scanVoxel->z=targetCenterHeight+heightFromCenter[i];
		for (int j = 0; j < 5; ++j)
		{
			if (i < (heightRange-1) && j>2) break; //skip unnecessary checks
			scanVoxel->x=targetVoxel.x + sliceTargets[j*2];
			scanVoxel->y=targetVoxel.y + sliceTargets[j*2+1];
			_trajectory.clear();
			int test = calculateLine(*originVoxel, *scanVoxel, false, &_trajectory, excludeUnit, true, false);
			if (test == V_UNIT)
			{
				for (int x = 0; x <= targetSize; ++x)
				{
					for (int y = 0; y <= targetSize; ++y)
					{
						//voxel of hit must be inside of scanned box
						if (_trajectory.at(0).x/16 == (scanVoxel->x/16) + x + xOffset &&
							_trajectory.at(0).y/16 == (scanVoxel->y/16) + y + yOffset &&
							_trajectory.at(0).z >= targetMinHeight &&
							_trajectory.at(0).z <= targetMaxHeight)
						{
							return true;
						}
					}
				}
			}
			else if (test == V_EMPTY && hypothetical && !_trajectory.empty())
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * Checks for a tile part available for targeting and what particular voxel.
 * @param originVoxel Voxel of trace origin (gun's barrel).
 * @param tile The tile to check for.
 * @param part Tile part to check for.
 * @param scanVoxel Is returned coordinate of hit.
 * @param excludeUnit Is self (not to hit self).
 * @return True if the tile can be targetted.
 */
bool TileEngine::canTargetTile(Position *originVoxel, Tile *tile, int part, Position *scanVoxel, BattleUnit *excludeUnit)
{
	static int sliceObjectSpiral[82] = {8,8, 8,6, 10,6, 10,8, 10,10, 8,10, 6,10, 6,8, 6,6, //first circle
		8,4, 10,4, 12,4, 12,6, 12,8, 12,10, 12,12, 10,12, 8,12, 6,12, 4,12, 4,10, 4,8, 4,6, 4,4, 6,4, //second circle
		8,1, 12,1, 15,1, 15,4, 15,8, 15,12, 15,15, 12,15, 8,15, 4,15, 1,15, 1,12, 1,8, 1,4, 1,1, 4,1}; //third circle
	static int westWallSpiral[14] = {0,7, 0,9, 0,6, 0,11, 0,4, 0,13, 0,2};
	static int northWallSpiral[14] = {7,0, 9,0, 6,0, 11,0, 4,0, 13,0, 2,0};

	Position targetVoxel = Position((tile->getPosition().x * 16), (tile->getPosition().y * 16), tile->getPosition().z * 24);
	std::vector<Position> _trajectory;

	int *spiralArray;
	int spiralCount;

	int minZ, maxZ;
	bool minZfound = false, maxZfound = false;

	if (part == O_OBJECT)
	{
		spiralArray = sliceObjectSpiral;
		spiralCount = 41;
	}
	else
	if (part == O_NORTHWALL)
	{
		spiralArray = northWallSpiral;
		spiralCount = 7;
	}
	else
	if (part == O_WESTWALL)
	{
		spiralArray = westWallSpiral;
		spiralCount = 7;
	}
	else if (part == O_FLOOR)
	{
		spiralArray = sliceObjectSpiral;
		spiralCount = 41;
		minZfound = true; minZ=0;
		maxZfound = true; maxZ=0;
	}
	else
	{
		return false;
	}

// find out height range

	if (!minZfound)
	{
		for (int j = 1; j < 12; ++j)
		{
			if (minZfound) break;
			for (int i = 0; i < spiralCount; ++i)
			{
				int tX = spiralArray[i*2];
				int tY = spiralArray[i*2+1];
				if (voxelCheck(Position(targetVoxel.x + tX, targetVoxel.y + tY, targetVoxel.z + j*2),0,true) == part) //bingo
				{
					if (!minZfound)
					{
						minZ = j*2;
						minZfound = true;
						break;
					}
				}
			}
		}
	}

	if (!minZfound) return false;//empty object!!!

	if (!maxZfound)
	{
		for (int j = 10; j >= 0; --j)
		{
			if (maxZfound) break;
			for (int i = 0; i < spiralCount; ++i)
			{
				int tX = spiralArray[i*2];
				int tY = spiralArray[i*2+1];
				if (voxelCheck(Position(targetVoxel.x + tX, targetVoxel.y + tY, targetVoxel.z + j*2),0,true) == part) //bingo
				{
					if (!maxZfound)
					{
						maxZ = j*2;
						maxZfound = true;
						break;
					}
				}
			}
		}
	}

	if (!maxZfound) return false;//it's impossible to get there

	if (minZ > maxZ) minZ = maxZ;
	int rangeZ = maxZ - minZ;
	int centerZ = (maxZ + minZ)/2;

	for (int j = 0; j <= rangeZ; ++j)
	{
		scanVoxel->z = targetVoxel.z + centerZ + heightFromCenter[j];
		for (int i = 0; i < spiralCount; ++i)
		{
			scanVoxel->x = targetVoxel.x + spiralArray[i*2];
			scanVoxel->y = targetVoxel.y + spiralArray[i*2+1];
			_trajectory.clear();
			int test = calculateLine(*originVoxel, *scanVoxel, false, &_trajectory, excludeUnit, true);
			if (test == part) //bingo
			{
				if (_trajectory.at(0).x/16 == scanVoxel->x/16 &&
					_trajectory.at(0).y/16 == scanVoxel->y/16 &&
					_trajectory.at(0).z/24 == scanVoxel->z/24)
				{
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * Calculates line of sight of a soldiers within range of the Position
 * (used when terrain has changed, which can reveal new parts of terrain or units).
 * @param position Position of the changed terrain.
 * @param eventRadius Radius of circle big enough to encompass the event.
 * @param updateTiles true to do an update of visible tiles.
 * @param appendToTileVisibility true to append only new tiles and skip previously seen ones.
 */
void TileEngine::calculateFOV(const Position &position, int eventRadius, const bool updateTiles, const bool appendToTileVisibility)
{
	int updateRadius;
	if (eventRadius == -1)
	{
		eventRadius = getMaxViewDistance();
		updateRadius = getMaxViewDistance();
	} 
	else {
		//Need to grab units which are out of range of the centre of the event, but can still see the edge of the effect.
		updateRadius = getMaxViewDistance() + (eventRadius > 0 ? eventRadius : 0);
	}
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		if (distanceSq(position, (*i)->getPosition(), false) < updateRadius) //could this unit have observed the event?
		{
			if (updateTiles)
			{
				if (!appendToTileVisibility)
				{
					(*i)->clearVisibleTiles();
				}
				calculateTilesInFOV((*i), position, eventRadius);
			}

			calculateUnitsInFOV((*i), position, eventRadius);

		}
	}
}

/**
 * Checks if a sniper from the opposing faction sees this unit. The unit with the highest reaction score will be compared with the current unit's reaction score.
 * If it's higher, a shot is fired when enough time units, a weapon and ammo are available.
 * @param unit The unit to check reaction fire upon.
 * @return True if reaction fire took place.
 */
bool TileEngine::checkReactionFire(BattleUnit *unit)
{
	// reaction fire only triggered when the actioning unit is of the currently playing side, and is still on the map (alive)
	if (unit->getFaction() != _save->getSide() || unit->getTile() == 0)
	{
		return false;
	}

	std::vector<ReactionScore> spotters = getSpottingUnits(unit);
	bool result = false;

	// not mind controlled, or controlled by the player
	if (unit->getFaction() == unit->getOriginalFaction()
		|| unit->getFaction() != FACTION_HOSTILE)
	{
		// get the first man up to bat.
		ReactionScore *reactor = getReactor(spotters, unit);
		// start iterating through the possible reactors until the current unit is the one with the highest score.
		while (reactor != 0)
		{
			if (!tryReaction(reactor->unit, unit, reactor->attackType))
			{
				for (std::vector<ReactionScore>::iterator i = spotters.begin(); i != spotters.end(); ++i)
				{
					if (&(*i) == reactor)
					{
						spotters.erase(i);
						reactor = 0;
						break;
					}
				}
				// can't make a reaction snapshot for whatever reason, boot this guy from the vector.
				// avoid setting result to true, but carry on, just cause one unit can't react doesn't mean the rest of the units in the vector (if any) can't
				reactor = getReactor(spotters, unit);
				continue;
			}
			// nice shot, kid. don't get cocky.
			result = true;
			reactor->reactionScore -= reactor->reactionReduction;
			reactor = getReactor(spotters, unit);
		}
	}
	return result;
}

/**
 * Creates a vector of units that can spot this unit.
 * @param unit The unit to check for spotters of.
 * @return A vector of units that can see this unit.
 */
std::vector<TileEngine::ReactionScore> TileEngine::getSpottingUnits(BattleUnit* unit)
{
	std::vector<TileEngine::ReactionScore> spotters;
	Tile *tile = unit->getTile();
	int threshold = unit->getReactionScore();
	for (std::vector<BattleUnit*>::const_iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
			// not dead/unconscious
		if (!(*i)->isOut() &&
			// not dying
			(*i)->getHealth() > 0 &&
			// not about to pass out
			(*i)->getStunlevel() < (*i)->getHealth() &&
			// have any chances for reacting
			(*i)->getReactionScore() >= threshold &&
			// not a friend
			(*i)->getFaction() != _save->getSide() &&
			// closer than 20 tiles
			distanceSq(unit->getPosition(), (*i)->getPosition(), false) <= getMaxViewDistanceSq())
		{
			Position originVoxel = _save->getTileEngine()->getSightOriginVoxel(*i);
			originVoxel.z -= 2;
			Position targetVoxel;
			AlienBAIState *aggro = dynamic_cast<AlienBAIState*>((*i)->getCurrentAIState());
			bool gotHit = (aggro != 0 && aggro->getWasHitBy(unit->getId()));
				// can actually see the target Tile, or we got hit
			if (((*i)->checkViewSector(unit->getPosition()) || gotHit) &&
				// can actually target the unit
				canTargetUnit(&originVoxel, tile, &targetVoxel, *i) &&
				// can actually see the unit
				visible(*i, tile))
			{
				if ((*i)->getFaction() == FACTION_PLAYER)
				{
					unit->setVisible(true);
				}
				(*i)->addToVisibleUnits(unit);
				// no reaction on civilian turn.
				if (_save->getSide() != FACTION_NEUTRAL)
				{
					ReactionScore rs = determineReactionType(*i, unit);
					if (rs.attackType != BA_NONE)
					{
						spotters.push_back(rs);
					}
				}
			}
		}
	}
	return spotters;
}

/**
 * Gets the unit with the highest reaction score from the spotter vector.
 * @param spotters The vector of spotting units.
 * @param unit The unit to check scores against.
 * @return The unit with the highest reactions.
 */
TileEngine::ReactionScore *TileEngine::getReactor(std::vector<TileEngine::ReactionScore> &spotters, BattleUnit *unit)
{
	ReactionScore *best = 0;
	for (std::vector<ReactionScore>::iterator i = spotters.begin(); i != spotters.end(); ++i)
	{
		if (!(*i).unit->isOut() && !(*i).unit->getRespawn() && (!best || (*i).reactionScore > best->reactionScore))
		{
			best = &(*i);
		}
	}
	if (best &&(unit->getReactionScore() <= best->reactionScore))
	{
		if (best->unit->getOriginalFaction() == FACTION_PLAYER)
		{
			best->unit->addReactionExp();
		}
	}
	else
	{
		best = 0;
	}
	return best;
}

/**
 * Checks the validity of a snap shot performed here.
 * @param unit The unit to check sight from.
 * @param target The unit to check sight TO.
 * @return True if the target is valid.
 */
TileEngine::ReactionScore TileEngine::determineReactionType(BattleUnit *unit, BattleUnit *target)
{
	ReactionScore reaction =
	{
		unit,
		BA_NONE,
		unit->getReactionScore(),
		0,
	};
	// prioritize melee
	BattleItem *meleeWeapon = unit->getUtilityWeapon(BT_MELEE);
	if (_save->canUseWeapon(meleeWeapon, unit, false) &&
		// has a melee weapon and is in melee range
		validMeleeRange(unit, target, unit->getDirection()) &&
		BattleActionCost(BA_HIT, unit, meleeWeapon).haveTU())
	{
		reaction.attackType = BA_HIT;
		reaction.reactionReduction = 1.0 * BattleActionCost(BA_HIT, unit, meleeWeapon).Time * unit->getBaseStats()->reactions / unit->getBaseStats()->tu;
		return reaction;
	}

	// has a weapon
	BattleItem *weapon = unit->getMainHandWeapon(unit->getFaction() != FACTION_PLAYER);
	if (_save->canUseWeapon(weapon, unit, false) &&
		(	// has a melee weapon and is in melee range
			(weapon->getRules()->getBattleType() == BT_MELEE &&
				validMeleeRange(unit, target, unit->getDirection()) &&
				BattleActionCost(BA_HIT, unit, weapon).haveTU()) ||
			// has a gun capable of snap shot with ammo
			(weapon->getRules()->getBattleType() != BT_MELEE &&
				weapon->getAmmoItem() &&
				BattleActionCost(BA_SNAPSHOT, unit, weapon).haveTU())))
	{
		reaction.attackType = BA_SNAPSHOT;
		reaction.reactionReduction = 1.0 * BattleActionCost(BA_SNAPSHOT, unit, weapon).Time * unit->getBaseStats()->reactions / unit->getBaseStats()->tu;
		return reaction;
	}

	return reaction;
}

/**
 * Attempts to perform a reaction snap shot.
 * @param unit The unit to check sight from.
 * @param target The unit to check sight TO.
 * @return True if the action should (theoretically) succeed.
 */
bool TileEngine::tryReaction(BattleUnit *unit, BattleUnit *target, int attackType)
{
	BattleAction action;
	action.cameraPosition = _save->getBattleState()->getMap()->getCamera()->getMapOffset();
	action.actor = unit;
	if (attackType == BA_HIT)
	{
		action.weapon = unit->getUtilityWeapon(BT_MELEE);
	}
	else
	{
		action.weapon = unit->getMainHandWeapon(unit->getFaction() != FACTION_PLAYER);
	}

	if (!_save->canUseWeapon(action.weapon, action.actor, false))
	{
		return false;
	}

	action.type = (BattleActionType)(attackType);
	action.target = target->getPosition();
	action.updateTU();

	if (action.weapon->getAmmoItem() && action.weapon->getAmmoItem()->getAmmoQuantity() && action.haveTU())
	{
		action.targeting = true;

		// hostile units will go into an "aggro" state when they react.
		if (unit->getFaction() == FACTION_HOSTILE)
		{
			AlienBAIState *aggro = dynamic_cast<AlienBAIState*>(unit->getCurrentAIState());
			if (aggro == 0)
			{
				// should not happen, but just in case...
				aggro = new AlienBAIState(_save, unit, 0);
				unit->setAIState(aggro);
			}

			int radius = action.weapon->getAmmoItem()->getRules()->getExplosionRadius(unit);
			if (radius > 0 &&
				aggro->explosiveEfficacy(action.target, unit, radius, -1) == 0)
			{
				action.targeting = false;
			}
		}

		if (action.targeting)
		{
			// start new hit log
			_save->hitLog.str(L"");
			_save->hitLog.clear();
			// log weapon?
			_save->hitLog << "Reaction fire...\n\n";

			if (action.type == BA_HIT)
			{
				_save->getBattleGame()->statePushBack(new MeleeAttackBState(_save->getBattleGame(), action));
			}
			else
			{
				_save->getBattleGame()->statePushBack(new ProjectileFlyBState(_save->getBattleGame(), action));
			}
			return true;
		}
	}
	return false;
}

/**
 * Handling of hitting tile.
 * @param tile targeted tile.
 * @param damage power of hit.
 * @param type damage type of hit.
 * @return whether a smoke (1) or fire (2) effect was produced
 */
int TileEngine::hitTile(Tile* tile, int damage, const RuleDamageType* type)
{
	if (damage >= type->SmokeThreshold)
	{
		// smoke from explosions always stay 6 to 14 turns - power of a smoke grenade is 60
		if (tile->getSmoke() < _save->getBattleGame()->getMod()->getTooMuchSmokeThreshold() && tile->getTerrainLevel() > -24)
		{
			tile->setFire(0);
			if (damage >= type->SmokeThreshold * 2)
				tile->setSmoke(RNG::generate(7, 15)); // for SmokeThreshold == 0
			else
				tile->setSmoke(RNG::generate(7, 15) * (damage - type->SmokeThreshold) / type->SmokeThreshold);
			return 1;
		}
	}
	else if (damage >= type->FireThreshold)
	{
		if (!tile->isVoid())
		{
			if (tile->getFire() == 0 && (tile->getMapData(O_FLOOR) || tile->getMapData(O_OBJECT)))
			{
				if (damage >= type->FireThreshold * 2)
					tile->setFire(tile->getFuel() + 1); // for FireThreshold == 0
				else
					tile->setFire(tile->getFuel() * (damage - type->FireThreshold) / type->FireThreshold + 1);
				tile->setSmoke(std::max(1, std::min(15 - (tile->getFlammability() / 10), 12)));
				return 2;
			}
		}
	}
	return 0;
}

/**
 * Handling of experience training.
 * @param unit hitter.
 * @param weapon weapon causing the damage.
 * @param target targeted unit.
 * @param rangeAtack is ranged attack or not?
 * @return Was experience awarded or not?
 */
bool TileEngine::awardExperience(BattleUnit *unit, BattleItem *weapon, BattleUnit *target, bool rangeAtack)
{
	if (!weapon)
	{
		return false;
	}

	if (!target)
	{
		return false;
	}
	else if (weapon->getRules()->getBattleType() != BT_MEDIKIT)
	{
		// only enemies count, not friends or neutrals
		if (target->getOriginalFaction() != FACTION_HOSTILE) return false;

		// mind-controlled enemies don't count though!
		if (target->getFaction() != FACTION_HOSTILE) return false;
	}

	if (weapon->getRules()->getExperienceTrainingMode() > ETM_DEFAULT)
	{
		// can train psi strength and psi skill only if psi skill is already > 0
		if (weapon->getRules()->getExperienceTrainingMode() >= ETM_PSI_STRENGTH && weapon->getRules()->getExperienceTrainingMode() <= ETM_PSI_STRENGTH_OR_SKILL_2X)
		{
			// cannot use "unit->getBaseStats()->psiSkill", because armor can give +psiSkill bonus
			if (unit->getGeoscapeSoldier() && unit->getGeoscapeSoldier()->getCurrentStats()->psiSkill <= 0)
				return false;
		}

		switch (weapon->getRules()->getExperienceTrainingMode())
		{
		case ETM_MELEE_100: unit->addMeleeExp(); break;
		case ETM_MELEE_50: if (RNG::percent(50)) { unit->addMeleeExp(); } break;
		case ETM_MELEE_33: if (RNG::percent(33)) { unit->addMeleeExp(); } break;
		case ETM_FIRING_100: unit->addFiringExp(); break;
		case ETM_FIRING_50: if (RNG::percent(50)) { unit->addFiringExp(); } break;
		case ETM_FIRING_33: if (RNG::percent(33)) { unit->addFiringExp(); } break;
		case ETM_THROWING_100: unit->addThrowingExp(); break;
		case ETM_THROWING_50: if (RNG::percent(50)) { unit->addThrowingExp(); } break;
		case ETM_THROWING_33: if (RNG::percent(33)) { unit->addThrowingExp(); } break;
		case ETM_FIRING_AND_THROWING: unit->addFiringExp(); unit->addThrowingExp(); break;
		case ETM_FIRING_OR_THROWING: if (RNG::percent(50)) { unit->addFiringExp(); } else { unit->addThrowingExp(); } break;
		case ETM_REACTIONS: unit->addReactionExp(); break;
		case ETM_REACTIONS_AND_MELEE: unit->addReactionExp(); unit->addMeleeExp(); break;
		case ETM_REACTIONS_AND_FIRING: unit->addReactionExp(); unit->addFiringExp(); break;
		case ETM_REACTIONS_AND_THROWING: unit->addReactionExp(); unit->addThrowingExp(); break;
		case ETM_REACTIONS_OR_MELEE: if (RNG::percent(50)) { unit->addReactionExp(); } else { unit->addMeleeExp(); } break;
		case ETM_REACTIONS_OR_FIRING: if (RNG::percent(50)) { unit->addReactionExp(); } else { unit->addFiringExp(); } break;
		case ETM_REACTIONS_OR_THROWING: if (RNG::percent(50)) { unit->addReactionExp(); } else { unit->addThrowingExp(); } break;
		case ETM_BRAVERY: unit->addBraveryExp(); break;
		case ETM_BRAVERY_2X: unit->addBraveryExp(); unit->addBraveryExp(); break;
		case ETM_BRAVERY_AND_REACTIONS: unit->addBraveryExp(); unit->addReactionExp(); break;
		case ETM_BRAVERY_OR_REACTIONS: if (RNG::percent(50)) { unit->addBraveryExp(); } else { unit->addReactionExp(); } break;
		case ETM_BRAVERY_OR_REACTIONS_2X: if (RNG::percent(50)) { unit->addBraveryExp(); unit->addBraveryExp(); } else { unit->addReactionExp(); unit->addReactionExp(); } break;
		case ETM_PSI_STRENGTH: unit->addPsiStrengthExp(); break;
		case ETM_PSI_STRENGTH_2X: unit->addPsiStrengthExp(); unit->addPsiStrengthExp(); break;
		case ETM_PSI_SKILL: unit->addPsiSkillExp(); break;
		case ETM_PSI_SKILL_2X: unit->addPsiSkillExp(); unit->addPsiSkillExp(); break;
		case ETM_PSI_STRENGTH_AND_SKILL: unit->addPsiStrengthExp(); unit->addPsiSkillExp(); break;
		case ETM_PSI_STRENGTH_AND_SKILL_2X: unit->addPsiStrengthExp(); unit->addPsiStrengthExp(); unit->addPsiSkillExp(); unit->addPsiSkillExp(); break;
		case ETM_PSI_STRENGTH_OR_SKILL: if (RNG::percent(50)) { unit->addPsiStrengthExp(); } else { unit->addPsiSkillExp(); } break;
		case ETM_PSI_STRENGTH_OR_SKILL_2X: if (RNG::percent(50)) { unit->addPsiStrengthExp(); unit->addPsiStrengthExp(); } else { unit->addPsiSkillExp(); unit->addPsiSkillExp(); } break;
		case ETM_NOTHING:
		default:
			return false;
		}

		return true;
	}

	// GRENADES AND PROXIES
	if (weapon->getRules()->getBattleType() == BT_GRENADE || weapon->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
	{
		unit->addThrowingExp(); // e.g. willie pete, acid grenade, stun grenade, HE grenade, smoke grenade, proxy grenade, ...
	}
	// MELEE
	else if (weapon->getRules()->getBattleType() == BT_MELEE)
	{
		unit->addMeleeExp(); // e.g. cattle prod, cutlass, rope, ...
	}
	// FIREARMS and other
	else
	{
		if (!rangeAtack)
		{
			unit->addMeleeExp(); // e.g. rifle/shotgun gun butt, ...
		}
		else if (weapon->getRules()->getArcingShot())
		{
			unit->addThrowingExp(); // e.g. flamethrower, javelins, combat bow, grenade launcher, molotov, black powder bomb, stick grenade, acid flask, apple, ...
		}
		else
		{
			int maxRange = weapon->getRules()->getMaxRange();
			if (maxRange > 10)
			{
				unit->addFiringExp(); // e.g. panzerfaust, harpoon gun, shotgun, assault rifle, rocket launcher, small launcher, heavy cannon, blaster launcher, ...
			}
			else if (maxRange > 1)
			{
				unit->addThrowingExp(); // e.g. fuso knives, zapper, ...
			}
			else if (maxRange == 1)
			{
				unit->addMeleeExp(); // e.g. hammer, chainsaw, fusion torch, ...
			}
			else
			{
				return false; // what is this? no training!
			}
		}
	}

	return true;
}

/**
 * Handling of hitting unit.
 * @param unit hitter.
 * @param clipOrWeapon clip or weapon causing the damage.
 * @param target targeted unit.
 * @param relative angle of hit.
 * @param damage power of hit.
 * @param type damage type of hit.
 * @return Did unit get hit?
 */
bool TileEngine::hitUnit(BattleUnit *unit, BattleItem *clipOrWeapon, BattleUnit *target, const Position &relative, int damage, const RuleDamageType *type, bool rangeAtack)
{
	if (!target || target->getHealth() <= 0)
	{
		return false;
	}

	const int wounds = target->getFatalWounds();
	const int stunLevelOrig = target->getStunlevel();
	const int adjustedDamage = target->damage(relative, damage, type);
	// lethal + stun
	const int totalDamage = adjustedDamage + (target->getStunlevel() - stunLevelOrig);

	// hit log
	const int damagePercent = (totalDamage * 100) / target->getBaseStats()->health;
	if (damagePercent <= 0)
	{
		_save->hitLog << "0 ";
	}
	else if (damagePercent <= 20)
	{
		_save->hitLog << "hit ";
	}
	else
	{
		_save->hitLog << "hit! ";
	}

	if (unit && target->getFaction() != FACTION_PLAYER)
	{
		// if it's going to bleed to death and it's not a player, give credit for the kill.
		if (wounds < target->getFatalWounds())
		{
			target->killedBy(unit->getFaction());
		}
	}

	// single place for firing/throwing/melee experience training
	if (unit && unit->getOriginalFaction() == FACTION_PLAYER)
	{
		if (clipOrWeapon && clipOrWeapon->getRules()->getBattleType() == BT_AMMO)
		{
			// send the weapon info, not the ammo info
			awardExperience(unit, unit->getItem(unit->getActiveHand()), target, rangeAtack);
		}
		else
		{
			awardExperience(unit, clipOrWeapon, target, rangeAtack);
		}
	}

	if (type->IgnoreNormalMoraleLose == false)
	{
		const int bravery = (110 - target->getBaseStats()->bravery) / 10;
		const int modifier = target->getFaction() == FACTION_PLAYER ? _save->getFactionMoraleModifier(true) : 100;
		const int morale_loss = 100 * (adjustedDamage * bravery / 10) / modifier;

		target->moraleChange(-morale_loss);
	}

	if ((target->getSpecialAbility() == SPECAB_EXPLODEONDEATH || target->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE) && !target->isOut() && (target->getHealth() == 0 || target->getStunlevel() >= target->getHealth()))
	{
		if (type->IgnoreSelfDestruct == false)
		{
			Position p = Position(target->getPosition().x * 16, target->getPosition().y * 16, target->getPosition().z * 24);
			_save->getBattleGame()->statePushNext(new ExplosionBState(_save->getBattleGame(), p, BA_NONE, 0, target, 0));
		}
	}

	if (adjustedDamage >= type->FireThreshold)
	{
		float resistance = target->getArmor()->getDamageModifier(type->ResistType);
		if (resistance > 0.0)
		{
			int burnTime = RNG::generate(0, int(5.0f * resistance));
			if (target->getFire() < burnTime)
			{
				target->setFire(burnTime); // catch fire and burn
			}
		}
	}

	// fire extinguisher
	if (target && target->getFire())
	{
		if (clipOrWeapon)
		{
			// melee weapon, bullet or even a grenade...
			if (clipOrWeapon->getRules()->isFireExtinguisher())
			{
				target->setFire(0);
			}
			// if the bullet is not a fire extinguisher, check the weapon itself
			else if (unit && clipOrWeapon->getRules()->getBattleType() == BT_AMMO)
			{
				BattleItem *weapon = unit->getItem(unit->getActiveHand());
				if (weapon && weapon->getRules()->isFireExtinguisher())
				{
					target->setFire(0);
				}
			}
		}
	}

	return true;
}

/**
 * Handles bullet/weapon hits.
 *
 * A bullet/weapon hits a voxel.
 * @param center Center of the explosion in voxelspace.
 * @param power Power of the explosion.
 * @param type The damage type of the explosion.
 * @param unit The unit that caused the explosion.
 * @param clipOrWeapon clip or weapon causing the damage.
 * @return The Unit that got hit.
 */
BattleUnit *TileEngine::hit(const Position &center, int power, const RuleDamageType *type, BattleUnit *unit, BattleItem *clipOrWeapon, bool rangeAtack)
{
	bool terrainChanged = false; //did the hit destroy a tile thereby changing line of sight?
	int effectGenerated = 0; //did the hit produce smoke (1), fire/light (2) or disabled a unit (3) ?
	Tile *tile = _save->getTile(center.toTile());
	if (!tile || power <= 0)
	{
		return 0;
	}

	BattleUnit *bu = tile->getUnit();
	const int part = voxelCheck(center, unit);
	const int damage = type->getRandomDamage(power);
	if (part >= V_FLOOR && part <= V_OBJECT)
	{
		bool nothing = true;
		if (part == V_FLOOR || part == V_OBJECT)
		{
			for (std::vector<BattleItem*>::iterator i = tile->getInventory()->begin(); i != tile->getInventory()->end(); ++i)
			{
				if (hitUnit(unit, clipOrWeapon, (*i)->getUnit(), Position(0,0,0), damage, type, rangeAtack))
				{
					if ((*i)->getGlow()) effectGenerated = 2; //Any glowing corpses?
					nothing = false;
					break;
				}
			}
		}
		if (nothing)
		{
			const int tileDmg = damage * type->ToTile;
			//Do we need to update the visibility of units due to smoke/fire?
			effectGenerated = hitTile(tile, damage, type);
			//If a tile was destroyed we may have revealed new areas for one or more observers
			if (tileDmg >= tile->getMapData(part)->getArmor()) terrainChanged = true;

			if (part == V_OBJECT && _save->getMissionType() == "STR_BASE_DEFENSE")
			{
				if (tileDmg >= tile->getMapData(O_OBJECT)->getArmor() && tile->getMapData(V_OBJECT)->isBaseModule())
				{
					_save->getModuleMap()[(center.x/16)/10][(center.y/16)/10].second--;
				}
			}
			if (tile->damage(part, tileDmg, _save->getObjectiveType()))
			{
				_save->addDestroyedObjective();
			}
		}
	}
	else if (part == V_UNIT)
	{
		int verticaloffset = 0;
		if (!bu)
		{
			// it's possible we have a unit below the actual tile, when he stands on a stairs and sticks his head out to the next tile
			Tile *below = _save->getTile(center.toTile() - Position(0, 0, 1));
			if (below)
			{
				BattleUnit *buBelow = below->getUnit();
				if (buBelow)
				{
					bu = buBelow;
					verticaloffset = 24;
				}
			}
		}
		if (bu && bu->getHealth() != 0)
		{
			const int sz = bu->getArmor()->getSize() * 8;
			const Position target = bu->getPosition().toVexel() + Position(sz,sz, bu->getFloatHeight() - tile->getTerrainLevel());
			const Position relative = (center - target) - Position(0,0,verticaloffset);

			hitUnit(unit, clipOrWeapon, bu, relative, damage, type, rangeAtack);
			if (bu->getFire())
			{
				effectGenerated = 2;
			}
		}
	}
	//Recalculate relevant item/unit locations and visibility depending on what happened during the hit
	if (terrainChanged || effectGenerated)
	{
		applyGravity(tile);
	}
	if (effectGenerated == 2) //fire, or lighting otherwise changed
	{
		calculateTerrainLighting(); // fires could have been started
	}
	if (terrainChanged) //part of tile destroyed
	{
		if (part == V_FLOOR && _save->getTile(center.toTile() - Position(0, 0, 1))) {
			calculateSunShading(center.toTile()); // roof destroyed, update sunlight in this tile column
		}
		calculateFOV(center.toTile(), 1, true, true); //append any new units or tiles revealed by the terrain change
	}
	else if (effectGenerated)
	{
		calculateFOV(center.toTile(), 1, false); //skip updating of tiles
	}
	//Note: If bu was knocked out this will have no effect on unit visibility quite yet, as it is not marked as out 
	//and will continue to block visibility at this point in time.

	return bu;
}

/**
 * Handles explosions.
 *
 * HE, smoke and fire explodes in a circular pattern on 1 level only. HE however damages floor tiles of the above level. Not the units on it.
 * HE destroys an object if its armor is lower than the explosive power, then it's HE blockage is applied for further propagation.
 * See http://www.ufopaedia.org/index.php?title=Explosions for more info.
 * @param center Center of the explosion in voxelspace.
 * @param power Power of the explosion.
 * @param type The damage type of the explosion.
 * @param maxRadius The maximum radius othe explosion.
 * @param unit The unit that caused the explosion.
 * @param clipOrWeapon The clip or weapon that caused the explosion.
 */
void TileEngine::explode(const Position &center, int power, const RuleDamageType *type, int maxRadius, BattleUnit *unit, BattleItem *clipOrWeapon, bool rangeAtack)
{
	double centerZ = center.z / 24 + 0.5;
	double centerX = center.x / 16 + 0.5;
	double centerY = center.y / 16 + 0.5;
	int power_;
	std::map<Tile*, int> tilesAffected;
	std::vector<BattleItem*> toRemove;
	std::pair<std::map<Tile*, int>::iterator, bool> ret;

	if (type->FireBlastCalc)
	{
		power /= 2;
	}

	int exHeight = std::max(0, std::min(3, Options::battleExplosionHeight));
	int vertdec = 1000; //default flat explosion

	switch (exHeight)
	{
	case 1:
		vertdec = 3.0f * type->RadiusReduction;
		break;
	case 2:
		vertdec = 1.0f * type->RadiusReduction;
		break;
	case 3:
		vertdec = 0.5f * type->RadiusReduction;
	}

	for (int fi = -90; fi <= 90; fi += 5)
	{
		// raytrace every 3 degrees makes sure we cover all tiles in a circle.
		for (int te = 0; te <= 360; te += 3)
		{
			double cos_te = cos(te * M_PI / 180.0);
			double sin_te = sin(te * M_PI / 180.0);
			double sin_fi = sin(fi * M_PI / 180.0);
			double cos_fi = cos(fi * M_PI / 180.0);

			Tile *origin = _save->getTile(Position(centerX, centerY, centerZ));
			Tile *dest = origin;
			double l = 0;
			int tileX, tileY, tileZ;
			power_ = power;
			while (power_ > 0 && l <= maxRadius)
			{
				if (power_ > 0)
				{
					ret = tilesAffected.insert(std::make_pair(dest, 0)); // check if we had this tile already affected

					const int tileDmg = power_ * type->ToTile;
					if (tileDmg > ret.first->second)
					{
						ret.first->second = tileDmg;
					}
					if (ret.second)
					{
						const int damage = type->getRandomDamage(power_);
						BattleUnit *bu = dest->getUnit();

						toRemove.clear();
						if (bu)
						{
							if (distance(dest->getPosition(), Position(centerX, centerY, centerZ)) < 2)
							{
								// ground zero effect is in effect
								hitUnit(unit, clipOrWeapon, bu, Position(0, 0, 0), damage, type, rangeAtack);
							}
							else
							{
								// directional damage relative to explosion position.
								// units above the explosion will be hit in the legs, units lateral to or below will be hit in the torso
								hitUnit(unit, clipOrWeapon, bu, Position(centerX, centerY, centerZ + 5) - dest->getPosition(), damage, type, rangeAtack);
							}

							// Affect all items and units in inventory
							const int itemDamage = bu->getOverKillDamage();
							if (itemDamage > 0)
							{
								for (std::vector<BattleItem*>::iterator it = bu->getInventory()->begin(); it != bu->getInventory()->end(); ++it)
								{
									if (!hitUnit(unit, clipOrWeapon, (*it)->getUnit(), Position(0, 0, 0), itemDamage, type, rangeAtack) && itemDamage * type->ToItem > (*it)->getRules()->getArmor())
									{
										toRemove.push_back(*it);
									}
								}
							}
						}
						// Affect all items and units on ground
						for (std::vector<BattleItem*>::iterator it = dest->getInventory()->begin(); it != dest->getInventory()->end(); ++it)
						{
							if (!hitUnit(unit, clipOrWeapon, (*it)->getUnit(), Position(0, 0, 0), damage, type) && damage * type->ToItem > (*it)->getRules()->getArmor())
							{
								toRemove.push_back(*it);
							}
						}
						for (std::vector<BattleItem*>::iterator it = toRemove.begin(); it != toRemove.end(); ++it)
						{
							_save->removeItem((*it));
						}

						hitTile(dest, damage, type);
					}
				}

				l += 1.0;

				tileX = int(floor(centerX + l * sin_te * cos_fi));
				tileY = int(floor(centerY + l * cos_te * cos_fi));
				tileZ = int(floor(centerZ + l * sin_fi));

				origin = dest;
				dest = _save->getTile(Position(tileX, tileY, tileZ));

				if (!dest) break; // out of map!

				// blockage by terrain is deducted from the explosion power
				power_ -= type->RadiusReduction; // explosive damage decreases by 10 per tile
				if (origin->getPosition().z != tileZ)
					power_ -= vertdec; //3d explosion factor

				if (type->FireBlastCalc)
				{
					int dir;
					Pathfinding::vectorToDirection(origin->getPosition() - dest->getPosition(), dir);
					if (dir != -1 && dir %2) power_ -= 0.5f * type->RadiusReduction; // diagonal movement costs an extra 50% for fire.
				}
				if (l>0.5) power_-= horizontalBlockage(origin, dest, type->ResistType, l<1.5) * 2;
				if (l>0.5) power_-= verticalBlockage(origin, dest, type->ResistType, l<1.5) * 2;
			}
		}
	}

	// now detonate the tiles affected by explosion
	if (type->ToTile > 0.0f)
	{
		for (std::map<Tile*, int>::iterator i = tilesAffected.begin(); i != tilesAffected.end(); ++i)
		{
			if (detonate(i->first, i->second))
			{
				_save->addDestroyedObjective();
			}
			applyGravity(i->first);
			Tile *j = _save->getTile(i->first->getPosition() + Position(0,0,1));
			if (j)
				applyGravity(j);
		}
	}
	//TODO: Can update shading and lighting on only tiles affected
	calculateSunShading(); // roofs could have been destroyed
	calculateTerrainLighting(); // fires could have been started
	calculateFOV(center / Position(16, 16, 24), maxRadius + 1, true, true);
}

/**
 * Applies the explosive power to the tile parts. This is where the actual destruction takes place.
 * Must affect 9 objects (6 box sides and the object inside plus 2 outer walls).
 * @param tile Tile affected.
 * @return True if the objective was destroyed.
 */
bool TileEngine::detonate(Tile* tile, int explosive)
{
	if (explosive == 0) return false; // no damage applied for this tile
	bool objective = false;
	Tile* tiles[9];
	static const int parts[9]={0,1,2,0,1,2,3,3,3}; //6th is the object of current
	Position pos = tile->getPosition();

	tiles[0] = _save->getTile(Position(pos.x, pos.y, pos.z+1)); //ceiling
	tiles[1] = _save->getTile(Position(pos.x+1, pos.y, pos.z)); //east wall
	tiles[2] = _save->getTile(Position(pos.x, pos.y+1, pos.z)); //south wall
	tiles[3] = tiles[4] = tiles[5] = tiles[6] = tile;
	tiles[7] = _save->getTile(Position(pos.x, pos.y-1, pos.z)); //north bigwall
	tiles[8] = _save->getTile(Position(pos.x-1, pos.y, pos.z)); //west bigwall

	int remainingPower, fireProof, fuel;
	bool destroyed, bigwalldestroyed = true, skipnorthwest = false;
	for (int i = 8; i >=0; --i)
	{
		if (!tiles[i] || !tiles[i]->getMapData(parts[i]))
			continue; //skip out of map and emptiness
		int bigwall = tiles[i]->getMapData(parts[i])->getBigWall();
		if (i > 6 && !( (bigwall==1) || (bigwall==8) || (i==8 && bigwall==6) || (i==7 && bigwall==7)))
			continue;
		if ((bigwall!=0)) skipnorthwest = true;
		if (!bigwalldestroyed && i<6) //when ground shouldn't be destroyed
			continue;
		if (skipnorthwest && (i == 2 || i == 1)) continue;
		remainingPower = explosive;
		destroyed = false;
		int volume = 0;
		int currentpart = parts[i], currentpart2, diemcd;
		fireProof = tiles[i]->getFlammability(currentpart);
		fuel = tiles[i]->getFuel(currentpart) + 1;
		// get the volume of the object by checking it's loftemps objects.
		for (int j = 0; j < 12; j++)
		{
			if (tiles[i]->getMapData(currentpart)->getLoftID(j) != 0)
				++volume;
		}
		if ( i == 6 &&
			(bigwall == 2 || bigwall == 3) && //diagonals
			tiles[i]->getMapData(currentpart)->getArmor() > remainingPower) //not enough to destroy
		{
			bigwalldestroyed = false;
		}
		// iterate through tile armor and destroy if can
		while (	tiles[i]->getMapData(currentpart) &&
				tiles[i]->getMapData(currentpart)->getArmor() <= remainingPower &&
				tiles[i]->getMapData(currentpart)->getArmor() != 255)
		{
			if ( i == 6 && (bigwall == 2 || bigwall == 3)) //diagonals for the current tile
			{
				bigwalldestroyed = true;
			}
			remainingPower -= tiles[i]->getMapData(currentpart)->getArmor();
			destroyed = true;
			if (_save->getMissionType() == "STR_BASE_DEFENSE" &&
				tiles[i]->getMapData(currentpart)->isBaseModule())
			{
				_save->getModuleMap()[tile->getPosition().x/10][tile->getPosition().y/10].second--;
			}
			//this trick is to follow transformed object parts (object can become a ground)
			diemcd = tiles[i]->getMapData(currentpart)->getDieMCD();
			if (diemcd!=0)
				currentpart2 = tiles[i]->getMapData(currentpart)->getDataset()->getObjects()->at(diemcd)->getObjectType();
			else
				currentpart2 = currentpart;
			if (tiles[i]->destroy(currentpart, _save->getObjectiveType()))
				objective = true;
			currentpart =  currentpart2;
			if (tiles[i]->getMapData(currentpart)) // take new values
			{
				fireProof = tiles[i]->getFlammability(currentpart);
				fuel = tiles[i]->getFuel(currentpart) + 1;
			}
		}
		// set tile on fire
		if (fireProof < remainingPower)
		{
			if (tiles[i]->getMapData(O_FLOOR) || tiles[i]->getMapData(O_OBJECT))
			{
				tiles[i]->setFire(fuel);
				tiles[i]->setSmoke(std::max(1, std::min(15 - (fireProof / 10), 12)));
			}
		}
		// add some smoke if tile was destroyed and not set on fire
		if (destroyed && !tiles[i]->getFire())
		{
			int smoke = RNG::generate(1, (volume / 2) + 3) + (volume / 2);
			if (smoke > tiles[i]->getSmoke())
			{
				tiles[i]->setSmoke(std::max(0, std::min(smoke, 15)));
			}
		}
	}
	return objective;
}

/**
 * Checks for chained explosions.
 *
 * Chained explosions are explosions which occur after an explosive map object is destroyed.
 * May be due a direct hit, other explosion or fire.
 * @return tile on which a explosion occurred
 */
Tile *TileEngine::checkForTerrainExplosions()
{
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		if (_save->getTile(i)->getExplosive())
		{
			return _save->getTile(i);
		}
	}
	return 0;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on a different level.
 * @param startTile The tile where the power starts.
 * @param endTile The adjacent tile where the power ends.
 * @param type The type of power/damage.
 * @return Amount of blockage of this power.
 */
int TileEngine::verticalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type, bool skipObject)
{
	int block = 0;

	// safety check
	if (startTile == 0 || endTile == 0) return 0;
	int direction = endTile->getPosition().z - startTile->getPosition().z;

	if (direction == 0 ) return 0;

	int x = startTile->getPosition().x;
	int y = startTile->getPosition().y;
	int z = startTile->getPosition().z;

	if (direction < 0) // down
	{
		block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, type);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, type, Pathfinding::DIR_DOWN);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, type);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, type, Pathfinding::DIR_DOWN);
		}
	}
	else if (direction > 0) // up
	{
		block += blockage(_save->getTile(Position(x, y, z+1)), O_FLOOR, type);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z+1)), O_OBJECT, type, Pathfinding::DIR_UP);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z+1;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, type);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, type, Pathfinding::DIR_UP);
		}
	}

	return block;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on the same level.
 * @param startTile The tile where the power starts.
 * @param endTile The adjacent tile where the power ends.
 * @param type The type of power/damage.
 * @return Amount of blockage.
 */
int TileEngine::horizontalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type, bool skipObject)
{
	static const Position oneTileNorth = Position(0, -1, 0);
	static const Position oneTileEast = Position(1, 0, 0);
	static const Position oneTileSouth = Position(0, 1, 0);
	static const Position oneTileWest = Position(-1, 0, 0);

	// safety check
	if (startTile == 0 || endTile == 0) return 0;
	if (startTile->getPosition().z != endTile->getPosition().z) return 0;
	Tile *tmpTile;

	int direction;
	Pathfinding::vectorToDirection(endTile->getPosition() - startTile->getPosition(), direction);
	if (direction == -1) return 0;
	int block = 0;

	switch(direction)
	{
	case 0:	// north
		block = blockage(startTile, O_NORTHWALL, type);
		break;
	case 1: // north east
		if (type == DT_NONE) //this is two-way diagonal visibility check, used in original game
		{
			block = blockage(startTile, O_NORTHWALL, type) + blockage(endTile, O_WESTWALL, type); //up+right
			tmpTile = _save->getTile(startTile->getPosition() + oneTileNorth);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNESW)
				block += blockage(tmpTile, O_OBJECT, type, 3);
			if (block == 0) break; //this way is opened
			block = blockage(_save->getTile(startTile->getPosition() + oneTileEast), O_NORTHWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), O_WESTWALL, type); //right+up
			tmpTile = _save->getTile(startTile->getPosition() + oneTileEast);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNESW)
				block += blockage(tmpTile, O_OBJECT, type, 7);
		}
		else
		{
			block = (blockage(startTile,O_NORTHWALL, type) + blockage(endTile,O_WESTWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileEast),O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileEast),O_NORTHWALL, type))/2;

			block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth),O_OBJECT, type, 4)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileEast),O_OBJECT, type, 6))/2;
		}
		break;
	case 2: // east
		block = blockage(endTile,O_WESTWALL, type);
		break;
	case 3: // south east
		if (type == DT_NONE)
		{
			block = blockage(_save->getTile(startTile->getPosition() + oneTileSouth), O_NORTHWALL, type)
				+ blockage(endTile, O_WESTWALL, type); //down+right
			tmpTile = _save->getTile(startTile->getPosition() + oneTileSouth);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNWSE)
				block += blockage(tmpTile, O_OBJECT, type, 1);
			if (block == 0) break; //this way is opened
			block = blockage(_save->getTile(startTile->getPosition() + oneTileEast), O_WESTWALL, type)
				+ blockage(endTile, O_NORTHWALL, type); //right+down
			tmpTile = _save->getTile(startTile->getPosition() + oneTileEast);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNWSE)
				block += blockage(tmpTile, O_OBJECT, type, 5);
		}
		else
		{
			block = (blockage(endTile,O_WESTWALL, type) + blockage(endTile,O_NORTHWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileEast),O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth),O_NORTHWALL, type))/2;
			block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth),O_OBJECT, type, 0)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileEast),O_OBJECT, type, 6))/2;
		}
		break;
	case 4: // south
		block = blockage(endTile,O_NORTHWALL, type);
		break;
	case 5: // south west
		if (type == DT_NONE)
		{
			block = blockage(_save->getTile(startTile->getPosition() + oneTileSouth), O_NORTHWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth), O_WESTWALL, type); //down+left
			tmpTile = _save->getTile(startTile->getPosition() + oneTileSouth);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNESW)
				block += blockage(tmpTile, O_OBJECT, type, 7);
			if (block == 0) break; //this way is opened
			block = blockage(startTile, O_WESTWALL, type) + blockage(endTile, O_NORTHWALL, type); //left+down
			tmpTile = _save->getTile(startTile->getPosition() + oneTileWest);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNESW)
				block += blockage(tmpTile, O_OBJECT, type, 3);
		}
		else
		{
			block = (blockage(endTile,O_NORTHWALL, type) + blockage(startTile,O_WESTWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileSouth),O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth),O_NORTHWALL, type))/2;
			block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth),O_OBJECT, type, 0)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileWest),O_OBJECT, type, 2))/2;
		}
		break;
	case 6: // west
		block = blockage(startTile,O_WESTWALL, type);
		break;
	case 7: // north west

		if (type == DT_NONE)
		{
			block = blockage(startTile, O_NORTHWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileNorth), O_WESTWALL, type); //up+left
			tmpTile = _save->getTile(startTile->getPosition() + oneTileNorth);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNWSE)
				block += blockage(tmpTile, O_OBJECT, type, 5);
			if (block == 0) break; //this way is opened
			block = blockage(startTile, O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileWest), O_NORTHWALL, type); //left+up
			tmpTile = _save->getTile(startTile->getPosition() + oneTileWest);
			if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNWSE)
				block += blockage(tmpTile, O_OBJECT, type, 1);
		}
		else
		{
			block = (blockage(startTile,O_WESTWALL, type) + blockage(startTile,O_NORTHWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileNorth),O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileWest),O_NORTHWALL, type))/2;
			block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth),O_OBJECT, type, 4)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileWest),O_OBJECT, type, 2))/2;
		}
		break;
	}

	if (!skipObject)
		block += blockage(startTile,O_OBJECT, type, direction);

	if (type != DT_NONE)
	{
		if (skipObject) return block;

		direction += 4;
		if (direction > 7)
			direction -= 8;
		if (endTile->isBigWall())
			block += blockage(endTile,O_OBJECT, type, direction, true);
	}
	else
	{
        if ( block <= 127 )
        {
            direction += 4;
            if (direction > 7)
                direction -= 8;
            if (blockage(endTile,O_OBJECT, type, direction, true) > 127){
                return -1; //hit bigwall, reveal bigwall tile
            }
        }
	}

	return block;
}

/**
 * Calculates the amount this certain wall or floor-part of the tile blocks.
 * @param startTile The tile where the power starts.
 * @param part The part of the tile the power needs to go through.
 * @param type The type of power/damage.
 * @param direction Direction the power travels.
 * @return Amount of blockage.
 */
int TileEngine::blockage(Tile *tile, const int part, ItemDamageType type, int direction, bool checkingFromOrigin)
{
	int blockage = 0;

	if (tile == 0) return 0; // probably outside the map here
	if (tile->getMapData(part))
	{
		bool check = true;
		int wall = -1;
		if (direction != -1)
		{
			wall = tile->getMapData(O_OBJECT)->getBigWall();

			if (type != DT_SMOKE &&
				checkingFromOrigin &&
				(wall == Pathfinding::BIGWALLNESW ||
				wall == Pathfinding::BIGWALLNWSE))
			{
				check = false;
			}
			switch (direction)
			{
			case 0: // north
				if (wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLEASTANDSOUTH)
				{
					check = false;
				}
				break;
			case 1: // north east
				if (wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLSOUTH)
				{
					check = false;
				}
				break;
			case 2: // east
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLWESTANDNORTH)
				{
					check = false;
				}
				break;
			case 3: // south east
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLWESTANDNORTH)
				{
					check = false;
				}
				break;
			case 4: // south
				if (wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLWESTANDNORTH)
				{
					check = false;
				}
				break;
			case 5: // south west
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLEAST)
				{
					check = false;
				}
				break;
			case 6: // west
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLEASTANDSOUTH)
				{
					check = false;
				}
				break;
			case 7: // north west
				if (wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLEASTANDSOUTH)
				{
					check = false;
				}
				break;
			case 8: // up
			case 9: // down
				if (wall != 0 && wall != Pathfinding::BLOCK)
				{
					check = false;
				}
				break;
			default:
				break;
			}
		}
		else if (part == O_FLOOR &&
					tile->getMapData(part)->getBlock(type) == 0)
		{
			if (type != DT_NONE)
			{
				blockage += tile->getMapData(part)->getArmor();
			}
			else if (!tile->getMapData(part)->isNoFloor())
			{
				return 256;
			}
		}

		if (check)
		{
			// -1 means we have a regular wall, and anything over 0 means we have a bigwall.
			if (type == DT_SMOKE && wall != 0 && !tile->isUfoDoorOpen(part))
			{
				return 256;
			}
			blockage += tile->getMapData(part)->getBlock(type);
		}
	}

	// open ufo doors are actually still closed behind the scenes
	// so a special trick is needed to see if they are open, if they are, they obviously don't block anything
	if (tile->isUfoDoorOpen(part))
		blockage = 0;

	return blockage;
}

/**
 * Opens a door (if any) by rightclick, or by walking through it. The unit has to face in the right direction.
 * @param unit Unit.
 * @param rClick Whether the player right clicked.
 * @param dir Direction.
 * @return -1 there is no door, you can walk through;
 *		  0 normal door opened, make a squeaky sound and you can walk through;
 *		  1 ufo door is starting to open, make a whoosh sound, don't walk through;
 *		  3 ufo door is still opening, don't walk through it yet. (have patience, futuristic technology...)
 *		  4 not enough TUs
 *		  5 would contravene fire reserve
 */
int TileEngine::unitOpensDoor(BattleUnit *unit, bool rClick, int dir)
{
	int door = -1;
	int TUCost = 0;
	int size = unit->getArmor()->getSize();
	int z = unit->getTile()->getTerrainLevel() < -12 ? 1 : 0; // if we're standing on stairs, check the tile above instead.
	int doorsOpened = 0;
	Position doorCentre;

	if (dir == -1)
	{
		dir = unit->getDirection();
	}
	Tile *tile;
	for (int x = 0; x < size && door == -1; x++)
	{
		for (int y = 0; y < size && door == -1; y++)
		{
			std::vector<std::pair<Position, int> > checkPositions;
			tile = _save->getTile(unit->getPosition() + Position(x,y,z));
			if (!tile) continue;

			switch (dir)
			{
			case 0: // north
				checkPositions.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL)); // origin
				if (x != 0)
				{
					checkPositions.push_back(std::make_pair(Position(0, -1, 0), O_WESTWALL)); // one tile north
				}
				break;
			case 1: // north east
				checkPositions.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL)); // origin
				checkPositions.push_back(std::make_pair(Position(1, -1, 0), O_WESTWALL)); // one tile north-east
				if (rClick)
				{
					checkPositions.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL)); // one tile east
					checkPositions.push_back(std::make_pair(Position(1, 0, 0), O_NORTHWALL)); // one tile east
				}
				break;
			case 2: // east
				checkPositions.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL)); // one tile east
				break;
			case 3: // south-east
				if (!y)
					checkPositions.push_back(std::make_pair(Position(1, 1, 0), O_WESTWALL)); // one tile south-east
				if (!x)
					checkPositions.push_back(std::make_pair(Position(1, 1, 0), O_NORTHWALL)); // one tile south-east
				if (rClick)
				{
					checkPositions.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL)); // one tile east
					checkPositions.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL)); // one tile south
				}
				break;
			case 4: // south
				checkPositions.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL)); // one tile south
				break;
			case 5: // south-west
				checkPositions.push_back(std::make_pair(Position(0, 0, 0), O_WESTWALL)); // origin
				checkPositions.push_back(std::make_pair(Position(-1, 1, 0), O_NORTHWALL)); // one tile south-west
				if (rClick)
				{
					checkPositions.push_back(std::make_pair(Position(0, 1, 0), O_WESTWALL)); // one tile south
					checkPositions.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL)); // one tile south
				}
				break;
			case 6: // west
				checkPositions.push_back(std::make_pair(Position(0, 0, 0), O_WESTWALL)); // origin
				if (y != 0)
				{
					checkPositions.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL)); // one tile west
				}
				break;
			case 7: // north-west
				checkPositions.push_back(std::make_pair(Position(0, 0, 0), O_WESTWALL)); // origin
				checkPositions.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL)); // origin
				if (x)
				{
					checkPositions.push_back(std::make_pair(Position(-1, -1, 0), O_WESTWALL)); // one tile north
				}
				if (y)
				{
					checkPositions.push_back(std::make_pair(Position(-1, -1, 0), O_NORTHWALL)); // one tile north
				}
				if (rClick)
				{
					checkPositions.push_back(std::make_pair(Position(0, -1, 0), O_WESTWALL)); // one tile north
					checkPositions.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL)); // one tile west
				}
				break;
			default:
				break;
			}

			int part = 0;
			for (std::vector<std::pair<Position, int> >::const_iterator i = checkPositions.begin(); i != checkPositions.end() && door == -1; ++i)
			{
				tile = _save->getTile(unit->getPosition() + Position(x,y,z) + i->first);
				if (tile)
				{
					door = tile->openDoor(i->second, unit, _save->getBattleGame()->getReservedAction());
					if (door != -1)
					{
						part = i->second;
						if (door == 0)
						{
							++doorsOpened;
							doorCentre = unit->getPosition() + Position(x, y, z) + i->first;
						}
						else if (door == 1)
						{
							std::pair<int, Position> adjacentDoors = checkAdjacentDoors(unit->getPosition() + Position(x,y,z) + i->first, i->second);
							doorsOpened += adjacentDoors.first + 1;
							doorCentre = adjacentDoors.second;
						}
					}
				}
			}
			if (door == 0 && rClick)
			{
				if (part == O_WESTWALL)
				{
					part = O_NORTHWALL;
				}
				else
				{
					part = O_WESTWALL;
				}
				TUCost = tile->getTUCost(part, unit->getMovementType());
			}
			else if (door == 1 || door == 4)
			{
				TUCost = tile->getTUCost(part, unit->getMovementType());
			}
		}
	}

	if (TUCost != 0)
	{
		if (_save->getBattleGame()->checkReservedTU(unit, TUCost, 0))
		{
			if (unit->spendTimeUnits(TUCost))
			{
				// Update FOV through the doorway.
				calculateFOV(doorCentre, doorsOpened, true, true);
			}
			else return 4;
		}
		else return 5;
	}

	return door;
}

/**
 * Opens any doors connected to this part at this position,
 * Keeps processing til it hits a non-ufo-door.
 * @param pos The starting position
 * @param part The part to open, defines which direction to check.
 * @return <The number of adjacent doors opened, Position of door centre>
 */
std::pair<int, Position> TileEngine::checkAdjacentDoors(Position pos, int part)
{
	Position offset;
	int adjacentDoorsOpened = 0;
	int doorOffset = 0;
	bool westSide = (part == 1);
	for (int i = 1;; ++i)
	{
		offset = westSide ? Position(0,i,0):Position(i,0,0);
		Tile *tile = _save->getTile(pos + offset);
		if (tile && tile->getMapData(part) && tile->getMapData(part)->isUFODoor())
		{
			int doorAdj = tile->openDoor(part);
			if (doorAdj == 1) //only expecting ufo doors
			{
				adjacentDoorsOpened++;
				doorOffset++;
			}
		}
		else break;
	}
	for (int i = -1;; --i)
	{
		offset = westSide ? Position(0,i,0):Position(i,0,0);
		Tile *tile = _save->getTile(pos + offset);
		if (tile && tile->getMapData(part) && tile->getMapData(part)->isUFODoor())
		{
			int doorAdj = tile->openDoor(part);
			if (doorAdj == 1)
			{
				adjacentDoorsOpened++;
				doorOffset--;
			}
		}
		else break;
	}
	doorOffset /= 2; //Find ~centre of door
	return {adjacentDoorsOpened, pos + (westSide ? Position(0, doorOffset, 0) : Position(doorOffset, 0, 0))};
}

/**
 * Closes ufo doors.
 * @return Whether doors are closed.
 */
int TileEngine::closeUfoDoors()
{
	int doorsclosed = 0;

	// prepare a list of tiles on fire/smoke & close any ufo doors
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		if (_save->getTile(i)->getUnit() && _save->getTile(i)->getUnit()->getArmor()->getSize() > 1)
		{
			BattleUnit *bu = _save->getTile(i)->getUnit();
			Tile *tile = _save->getTile(i);
			Tile *oneTileNorth = _save->getTile(tile->getPosition() + Position(0, -1, 0));
			Tile *oneTileWest = _save->getTile(tile->getPosition() + Position(-1, 0, 0));
			if ((tile->isUfoDoorOpen(O_NORTHWALL) && oneTileNorth && oneTileNorth->getUnit() && oneTileNorth->getUnit() == bu) ||
				(tile->isUfoDoorOpen(O_WESTWALL) && oneTileWest && oneTileWest->getUnit() && oneTileWest->getUnit() == bu))
			{
				continue;
			}
		}
		doorsclosed += _save->getTile(i)->closeUfoDoor();
	}

	return doorsclosed;
}

/**
 * Calculates a line trajectory, using bresenham algorithm in 3D.
 * @param origin Origin (voxel??).
 * @param target Target (also voxel??).
 * @param storeTrajectory True will store the whole trajectory - otherwise it just stores the last position.
 * @param trajectory A vector of positions in which the trajectory is stored.
 * @param excludeUnit Excludes this unit in the collision detection.
 * @param doVoxelCheck Check against voxel or tile blocking? (first one for units visibility and line of fire, second one for terrain visibility).
 * @param onlyVisible Skip invisible units? used in FPS view.
 * @param excludeAllBut [Optional] The only unit to be considered for ray hits.
 * @return the objectnumber(0-3) or unit(4) or out of map (5) or -1(hit nothing).
 */
int TileEngine::calculateLine(const Position& origin, const Position& target, bool storeTrajectory, std::vector<Position> *trajectory, BattleUnit *excludeUnit, bool doVoxelCheck, bool onlyVisible, BattleUnit *excludeAllBut)
{
	int x, x0, x1, delta_x, step_x;
	int y, y0, y1, delta_y, step_y;
	int z, z0, z1, delta_z, step_z;
	int swap_xy, swap_xz;
	int drift_xy, drift_xz;
	int cx, cy, cz;
	Position lastPoint(origin);
	int result;
	int steps = 0;

	//start and end points
	x0 = origin.x;	 x1 = target.x;
	y0 = origin.y;	 y1 = target.y;
	z0 = origin.z;	 z1 = target.z;

	//'steep' xy Line, make longest delta x plane
	swap_xy = abs(y1 - y0) > abs(x1 - x0);
	if (swap_xy)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	//do same for xz
	swap_xz = abs(z1 - z0) > abs(x1 - x0);
	if (swap_xz)
	{
		std::swap(x0, z0);
		std::swap(x1, z1);
	}

	//delta is Length in each plane
	delta_x = abs(x1 - x0);
	delta_y = abs(y1 - y0);
	delta_z = abs(z1 - z0);

	//drift controls when to step in 'shallow' planes
	//starting value keeps Line centred
	drift_xy  = (delta_x / 2);
	drift_xz  = (delta_x / 2);

	//direction of line
	step_x = 1;  if (x0 > x1) {  step_x = -1; }
	step_y = 1;  if (y0 > y1) {  step_y = -1; }
	step_z = 1;  if (z0 > z1) {  step_z = -1; }

	//starting point
	y = y0;
	z = z0;

	//step through longest delta (which we have swapped to x)
	for (x = x0; x != (x1+step_x); x += step_x)
	{
		//copy position
		cx = x;	cy = y;	cz = z;

		//unswap (in reverse)
		if (swap_xz) std::swap(cx, cz);
		if (swap_xy) std::swap(cx, cy);

		if (storeTrajectory && trajectory)
		{
			trajectory->push_back(Position(cx, cy, cz));
		}
		//passes through this point?
		if (doVoxelCheck)
		{
			result = voxelCheck(Position(cx, cy, cz), excludeUnit, false, onlyVisible, excludeAllBut);
			if (result != V_EMPTY)
			{
				if (trajectory)
				{ // store the position of impact
					trajectory->push_back(Position(cx, cy, cz));
				}
				return result;
			}
		}
		else
		{
			int temp_res = verticalBlockage(_save->getTile(lastPoint), _save->getTile(Position(cx, cy, cz)), DT_NONE);
			result = horizontalBlockage(_save->getTile(lastPoint), _save->getTile(Position(cx, cy, cz)), DT_NONE, steps<2);
			steps++;
			if (result == -1)
			{
				if (temp_res > 127)
				{
					result = 0;
				}
				else
				{
					return result; // We hit a big wall
				}
			}
			result += temp_res;
			if (result > 127)
			{
				return result;
			}

			lastPoint = Position(cx, cy, cz);
		}
		//update progress in other planes
		drift_xy = drift_xy - delta_y;
		drift_xz = drift_xz - delta_z;

		//step in y plane
		if (drift_xy < 0)
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;

			//check for xy diagonal intermediate voxel step
			if (doVoxelCheck)
			{
				cx = x;	cz = z; cy = y;
				if (swap_xz) std::swap(cx, cz);
				if (swap_xy) std::swap(cx, cy);
				result = voxelCheck(Position(cx, cy, cz), excludeUnit, false, onlyVisible, excludeAllBut);
				if (result != V_EMPTY)
				{
					if (trajectory != 0)
					{ // store the position of impact
						trajectory->push_back(Position(cx, cy, cz));
					}
					return result;
				}
			}
		}

		//same in z
		if (drift_xz < 0)
		{
			z = z + step_z;
			drift_xz = drift_xz + delta_x;

			//check for xz diagonal intermediate voxel step
			if (doVoxelCheck)
			{
				cx = x;	cz = z; cy = y;
				if (swap_xz) std::swap(cx, cz);
				if (swap_xy) std::swap(cx, cy);
				result = voxelCheck(Position(cx, cy, cz), excludeUnit, false, onlyVisible,  excludeAllBut);
				if (result != V_EMPTY)
				{
					if (trajectory != 0)
					{ // store the position of impact
						trajectory->push_back(Position(cx, cy, cz));
					}
					return result;
				}
			}
		}
	}

	return V_EMPTY;
}

/**
 * Calculates a parabola trajectory, used for throwing items.
 * @param origin Origin in voxelspace.
 * @param target Target in voxelspace.
 * @param storeTrajectory True will store the whole trajectory - otherwise it just stores the last position.
 * @param trajectory A vector of positions in which the trajectory is stored.
 * @param excludeUnit Makes sure the trajectory does not hit the shooter itself.
 * @param curvature How high the parabola goes: 1.0 is almost straight throw, 3.0 is a very high throw, to throw over a fence for example.
 * @param delta Is the deviation of the angles it should take into account, 0,0,0 is perfection.
 * @return The objectnumber(0-3) or unit(4) or out of map (5) or -1(hit nothing).
 */
int TileEngine::calculateParabola(const Position& origin, const Position& target, bool storeTrajectory, std::vector<Position> *trajectory, BattleUnit *excludeUnit, double curvature, const Position delta)
{
	double ro = sqrt((double)((target.x - origin.x) * (target.x - origin.x) + (target.y - origin.y) * (target.y - origin.y) + (target.z - origin.z) * (target.z - origin.z)));

	if (AreSame(ro, 0.0)) return V_EMPTY;//just in case

	double fi = acos((double)(target.z - origin.z) / ro);
	double te = atan2((double)(target.y - origin.y), (double)(target.x - origin.x));

	te += (delta.x / ro) / 2 * M_PI; //horizontal magic value
	fi += ((delta.z + delta.y) / ro) / 14 * M_PI * curvature; //another magic value (vertical), to make it in line with fire spread

	double zA = sqrt(ro)*curvature;
	double zK = 4.0 * zA / ro / ro;

	int x = origin.x;
	int y = origin.y;
	int z = origin.z;
	int i = 8;
	Position lastPosition = Position(x,y,z);
	while (z > 0)
	{
		x = (int)((double)origin.x + (double)i * cos(te) * sin(fi));
		y = (int)((double)origin.y + (double)i * sin(te) * sin(fi));
		z = (int)((double)origin.z + (double)i * cos(fi) - zK * ((double)i - ro / 2.0) * ((double)i - ro / 2.0) + zA);
		if (storeTrajectory && trajectory)
		{
			trajectory->push_back(Position(x, y, z));
		}
		//passes through this point?
		Position nextPosition = Position(x,y,z);
		int result = calculateLine(lastPosition, nextPosition, false, 0, excludeUnit);
		if (result != V_EMPTY)
		{
			if (lastPosition.z < nextPosition.z)
			{
				result = V_OUTOFBOUNDS;
			}
			if (!storeTrajectory && trajectory != 0)
			{ // store the position of impact
				trajectory->push_back(nextPosition);
			}
			return result;
		}
		lastPosition = Position(x,y,z);
		++i;
	}
	if (!storeTrajectory && trajectory != 0)
	{ // store the position of impact
		trajectory->push_back(Position(x, y, z));
	}
	return V_EMPTY;
}

/**
 * Calculates z "grounded" value for a particular voxel (used for projectile shadow).
 * @param voxel The voxel to trace down.
 * @return z coord of "ground".
 */
int TileEngine::castedShade(const Position& voxel)
{
	int zstart = voxel.z;
	Position tmpCoord = voxel / Position(16,16,24);
	Tile *t = _save->getTile(tmpCoord);
	while (t && t->isVoid() && !t->getUnit())
	{
		zstart = tmpCoord.z* 24;
		--tmpCoord.z;
		t = _save->getTile(tmpCoord);
	}

	Position tmpVoxel = voxel;
	int z;

	for (z = zstart; z>0; z--)
	{
		tmpVoxel.z = z;
		if (voxelCheck(tmpVoxel, 0) != V_EMPTY) break;
	}
    return z;
}

/**
 * Traces voxel visibility.
 * @param voxel Voxel coordinates.
 * @return True if visible.
 */

bool TileEngine::isVoxelVisible(const Position& voxel)
{
	int zstart = voxel.z+3; // slight Z adjust
	if ((zstart/24)!=(voxel.z/24))
		return true; // visible!
	Position tmpVoxel = voxel;
	int zend = (zstart/24)*24 +24;
	for (int z = zstart; z<zend; z++)
	{
		tmpVoxel.z=z;
		// only OBJECT can cause additional occlusion (because of any shape)
		if (voxelCheck(tmpVoxel, 0) == V_OBJECT) return false;
		++tmpVoxel.x;
		if (voxelCheck(tmpVoxel, 0) == V_OBJECT) return false;
		++tmpVoxel.y;
		if (voxelCheck(tmpVoxel, 0) == V_OBJECT) return false;
	}
    return true;
}

/**
 * Checks if we hit a voxel.
 * @param voxel The voxel to check.
 * @param excludeUnit Don't do checks on this unit.
 * @param excludeAllUnits Don't do checks on any unit.
 * @param onlyVisible Whether to consider only visible units.
 * @param excludeAllBut If set, the only unit to be considered for ray hits.
 * @return The objectnumber(0-3) or unit(4) or out of map (5) or -1 (hit nothing).
 */
int TileEngine::voxelCheck(const Position& voxel, BattleUnit *excludeUnit, bool excludeAllUnits, bool onlyVisible, BattleUnit *excludeAllBut)
{
	Tile *tile = _save->getTile(voxel / Position(16, 16, 24));
	// check if we are not out of the map
	if (tile == 0 || voxel.x < 0 || voxel.y < 0 || voxel.z < 0)
	{
		return V_OUTOFBOUNDS;
	}
	Tile *tileBelow = _save->getTile(tile->getPosition() + Position(0,0,-1));
	if (tile->isVoid() && tile->getUnit() == 0 && (!tileBelow || tileBelow->getUnit() == 0))
	{
		return V_EMPTY;
	}

	if ((voxel.z % 24 == 0 || voxel.z % 24 == 1) && tile->getMapData(O_FLOOR) && tile->getMapData(O_FLOOR)->isGravLift())
	{
		if ((tile->getPosition().z == 0) || (tileBelow && tileBelow->getMapData(O_FLOOR) && !tileBelow->getMapData(O_FLOOR)->isGravLift()))
			return V_FLOOR;
	}

	// first we check terrain voxel data, not to allow 2x2 units stick through walls
	for (int i=0; i< 4; ++i)
	{
		MapData *mp = tile->getMapData(i);
		if (tile->isUfoDoorOpen(i))
			continue;
		if (mp != 0)
		{
			int x = 15 - voxel.x%16;
			int y = voxel.y%16;
			int idx = (mp->getLoftID((voxel.z%24)/2)*16) + y;
			if (_voxelData->at(idx) & (1 << x))
			{
				return i;
			}
		}
	}

	if (!excludeAllUnits)
	{
		BattleUnit *unit = tile->getUnit();
		// sometimes there is unit on the tile below, but sticks up to this tile with his head,
		// in this case we couldn't have unit standing at current tile.
		if (unit == 0 && tile->hasNoFloor(0))
		{
			tile = _save->getTile(Position(voxel.x/16, voxel.y/16, (voxel.z/24)-1)); //below
			if (tile) unit = tile->getUnit();
		}

		if (unit != 0 && unit != excludeUnit && (!excludeAllBut || unit == excludeAllBut) && (!onlyVisible || unit->getVisible() ) )
		{
			Position tilepos;
			Position unitpos = unit->getPosition();
			int tz = unitpos.z*24 + unit->getFloatHeight()+(-tile->getTerrainLevel());//bottom
			if ((voxel.z > tz) && (voxel.z <= tz + unit->getHeight()) )
			{
				int x = voxel.x%16;
				int y = voxel.y%16;
				int part = 0;
				if (unit->getArmor()->getSize() > 1)
				{
					tilepos = tile->getPosition();
					part = tilepos.x - unitpos.x + (tilepos.y - unitpos.y)*2;
				}
				int idx = (unit->getLoftemps(part) * 16) + y;
				if (_voxelData->at(idx) & (1 << x))
				{
					return V_UNIT;
				}
			}
		}
	}
	return V_EMPTY;
}

/**
 * Toggles personal lighting on / off.
 */
void TileEngine::togglePersonalLighting()
{
	_personalLighting = !_personalLighting;
	calculateUnitLighting();
}

/**
 * Calculates the distance between 2 points. Rounded up to first INT.
 * @param pos1 Position of first square.
 * @param pos2 Position of second square.
 * @return Distance.
 */
int TileEngine::distance(const Position &pos1, const Position &pos2) const
{
	int x = pos1.x - pos2.x;
	int y = pos1.y - pos2.y;
	return (int)std::ceil(sqrt(float(x*x + y*y)));
}

/**
 * Calculates the distance squared between 2 points. No sqrt(), not floating point math, and sometimes it's all you need.
 * @param pos1 Position of first square.
 * @param pos2 Position of second square.
 * @param considerZ Whether to consider the z coordinate.
 * @return Distance.
 */
int TileEngine::distanceSq(const Position &pos1, const Position &pos2, bool considerZ) const
{
	int x = pos1.x - pos2.x;
	int y = pos1.y - pos2.y;
	int sq = x*x + y*y;
	if (considerZ)
	{
		int z = pos1.z - pos2.z;
		sq += z*z;
	}
	return sq;
}

/**
 * Calculate strength of psi attack based on range and victim.
 * @param type Type of attack.
 * @param attacker Unit attacking.
 * @param victim Attacked unit.
 * @param weapon Attack item.
 * @return Value greater than zero mean successful attack.
 */
int TileEngine::psiAttackCalculate(BattleActionType type, BattleUnit *attacker, BattleUnit *victim, BattleItem *weapon)
{
	if (!victim)
		return 0;

	float attackStrength = attacker->getPsiAccuracy(type, weapon);
	float defenseStrength = 30.0f + victim->getArmor()->getPsiDefence(victim);

	Position p = attacker->getPosition().toVexel() - victim->getPosition().toVexel();
	p *= p;
	attackStrength -= weapon->getRules()->getPsiAccuracyRangeReduction(sqrt(float(p.x + p.y + p.z)));
	attackStrength += RNG::generate(0,55);

	return attackStrength - defenseStrength;
}

/**
 * Attempts a panic or mind control action.
 * @param action Pointer to an action.
 * @return Whether it failed or succeeded.
 */
bool TileEngine::psiAttack(BattleAction *action)
{
	BattleUnit *victim = _save->getTile(action->target)->getUnit();
	if (!victim)
		return false;

	action->actor->addPsiSkillExp();
	if (Options::allowPsiStrengthImprovement) victim->addPsiStrengthExp();
	if (psiAttackCalculate(action->type, action->actor, victim, action->weapon) > 0)
	{
		action->actor->addPsiSkillExp();
		action->actor->addPsiSkillExp();
		if (action->type == BA_PANIC)
		{
			int moraleLoss = (110-victim->getBaseStats()->bravery);
			if (moraleLoss > 0)
				victim->moraleChange(-moraleLoss);
		}
		else if (action->type == BA_MINDCONTROL)
		{
			victim->convertToFaction(action->actor->getFaction());
			calculateFOV(victim->getPosition()); //happens fairly rarely, so do a full recalc for units in range to handle the potential unit visible cache issues.
			calculateUnitLighting();
			victim->recoverTimeUnits();
			victim->allowReselect();
			victim->abortTurn(); // resets unit status to STANDING
			// if all units from either faction are mind controlled - auto-end the mission.
			if (_save->getSide() == FACTION_PLAYER && Options::battleAutoEnd && Options::allowPsionicCapture)
			{
				int liveAliens = 0;
				int liveSoldiers = 0;
				_save->getBattleGame()->tallyUnits(liveAliens, liveSoldiers);
				if (liveAliens == 0 || liveSoldiers == 0)
				{
					_save->setSelectedUnit(0);
					_save->getBattleGame()->cancelCurrentAction(true);
					_save->getBattleGame()->requestEndTurn(true);
				}
			}
		}
		return true;
	}
	else
	{
		if (Options::allowPsiStrengthImprovement)
		{
			victim->addPsiStrengthExp();
		}
		return false;
	}
}

/**
 *  Attempts a melee attack action.
 * @param action Pointer to an action.
 * @return Whether it failed or succeeded.
 */
bool TileEngine::meleeAttack(BattleAction *action)
{
	BattleUnit *targetUnit = _save->getTile(action->target)->getUnit();
	if (!targetUnit && action->target.z > 0)
	{
		targetUnit = _save->getTile(action->target - Position(0, 0, 1))->getUnit();
	}

	int hitChance = action->actor->getFiringAccuracy(BA_HIT, action->weapon);
	if (targetUnit)
	{
		int arc = _save->getTileEngine()->getArcDirection(_save->getTileEngine()->getDirectionTo(targetUnit->getPositionVexels(), action->actor->getPositionVexels()), targetUnit->getDirection());
		float penalty = 1.0f - arc * targetUnit->getArmor()->getMeleeDodgeBackPenalty() / 4.0f;
		if (penalty > 0)
		{
			hitChance -= targetUnit->getArmor()->getMeleeDodge(targetUnit) * penalty;
		}
	}
	// hit log - new melee attack
	_save->hitLog << "=> ";
	if (!RNG::percent(hitChance))
	{
		return false;
	}
	return true;
}

/**
 * Remove the medikit from the game if consumable and empty.
 * @param action
 */
void TileEngine::medikitRemoveIfEmpty(BattleAction *action)
{
	// remove item if completely used up
	if (action->weapon->getRules()->isConsumable())
	{
		if (action->weapon->getPainKillerQuantity() == 0 && action->weapon->getStimulantQuantity() == 0 && action->weapon->getHealQuantity() == 0)
		{
			_save->removeItem(action->weapon);
		}
	}
}

/**
 * Try using medikit heal ability.
 * @param action
 * @param target
 * @param bodyPart
 */
void TileEngine::medikitHeal(BattleAction *action, BattleUnit *target, int bodyPart)
{
	RuleItem *rule = action->weapon->getRules();

	if (target->getFatalWound(bodyPart))
	{
		// award experience only if healed body part has a fatal wound (to prevent abuse)
		awardExperience(action->actor, action->weapon, target, false);
	}

	target->heal(bodyPart, rule->getWoundRecovery(), rule->getHealthRecovery());
	action->weapon->setHealQuantity(action->weapon->getHealQuantity() - 1);

	_save->getBattleGame()->playSound(action->weapon->getRules()->getHitSound());
}

/**
 * Try using medikit stimulant ability.
 * @param action
 * @param target
 */
void TileEngine::medikitStimulant(BattleAction *action, BattleUnit *target)
{
	RuleItem *rule = action->weapon->getRules();

	target->stimulant(rule->getEnergyRecovery(), rule->getStunRecovery());
	action->weapon->setStimulantQuantity(action->weapon->getStimulantQuantity() - 1);

	_save->getBattleGame()->playSound(action->weapon->getRules()->getHitSound());
}

/**
 * Try using medikit pain killer ability.
 * @param action
 * @param target
 */
void TileEngine::medikitPainKiller(BattleAction *action, BattleUnit *target)
{
	RuleItem *rule = action->weapon->getRules();

	target->painKillers(rule->getMoraleRecovery(), rule->getPainKillerRecovery());
	action->weapon->setPainKillerQuantity(action->weapon->getPainKillerQuantity() - 1);

	_save->getBattleGame()->playSound(action->weapon->getRules()->getHitSound());
}

/**
 * Applies gravity to a tile. Causes items and units to drop.
 * @param t Tile.
 * @return Tile where the items end up in eventually.
 */
Tile *TileEngine::applyGravity(Tile *t)
{
	if (!t || (t->getInventory()->empty() && !t->getUnit())) return t; // skip this if there are no items

	Position p = t->getPosition();
	Tile *rt = t;
	Tile *rtb;
	BattleUnit *occupant = t->getUnit();

	if (occupant)
	{
		Position unitpos = occupant->getPosition();
		while (unitpos.z >= 0)
		{
			bool canFall = true;
			for (int y = 0; y < occupant->getArmor()->getSize() && canFall; ++y)
			{
				for (int x = 0; x < occupant->getArmor()->getSize() && canFall; ++x)
				{
					rt = _save->getTile(Position(unitpos.x+x, unitpos.y+y, unitpos.z));
					rtb = _save->getTile(Position(unitpos.x+x, unitpos.y+y, unitpos.z-1)); //below
					if (!rt->hasNoFloor(rtb))
					{
						canFall = false;
					}
				}
			}
			if (!canFall)
				break;
			unitpos.z--;
		}
		if (unitpos != occupant->getPosition())
		{
			if (occupant->getHealth() > 0 && occupant->getStunlevel() < occupant->getHealth())
			{
				if (occupant->getMovementType() == MT_FLY)
				{
					// move to the position you're already in. this will unset the kneeling flag, set the floating flag, etc.
					occupant->startWalking(occupant->getDirection(), occupant->getPosition(), _save->getTile(occupant->getPosition() + Position(0,0,-1)), true);
					// and set our status to standing (rather than walking or flying) to avoid weirdness.
					occupant->abortTurn();
				}
				else
				{
					occupant->startWalking(Pathfinding::DIR_DOWN, occupant->getPosition() + Position(0,0,-1), _save->getTile(occupant->getPosition() + Position(0,0,-1)), true);
					_save->addFallingUnit(occupant);
				}
			}
			else if (occupant->isOut())
			{
				Position origin = occupant->getPosition();
				for (int y = occupant->getArmor()->getSize()-1; y >= 0; --y)
				{
					for (int x = occupant->getArmor()->getSize()-1; x >= 0; --x)
					{
						_save->getTile(origin + Position(x, y, 0))->setUnit(0);
					}
				}
				occupant->setPosition(unitpos);
			}
		}
	}
	rt = t;
	bool canFall = true;
	while (p.z >= 0 && canFall)
	{
		rt = _save->getTile(p);
		rtb = _save->getTile(Position(p.x, p.y, p.z-1)); //below
		if (!rt->hasNoFloor(rtb))
			canFall = false;
		p.z--;
	}

	for (std::vector<BattleItem*>::iterator it = t->getInventory()->begin(); it != t->getInventory()->end(); ++it)
	{
		if ((*it)->getUnit() && t->getPosition() == (*it)->getUnit()->getPosition())
		{
			(*it)->getUnit()->setPosition(rt->getPosition());
		}
		if (t != rt)
		{
			rt->addItem(*it, (*it)->getSlot());
		}
	}

	if (t != rt)
	{
		// clear tile
		t->getInventory()->clear();
	}

	return rt;
}

/**
 * Validates the melee range between two units.
 * @param attacker The attacking unit.
 * @param target The unit we want to attack.
 * @param dir Direction to check.
 * @return True when the range is valid.
 */
bool TileEngine::validMeleeRange(BattleUnit *attacker, BattleUnit *target, int dir)
{
	return validMeleeRange(attacker->getPosition(), dir, attacker, target, 0);
}

/**
 * Validates the melee range between a tile and a unit.
 * @param pos Position to check from.
 * @param direction Direction to check.
 * @param attacker The attacking unit.
 * @param target The unit we want to attack, 0 for any unit.
 * @param dest Destination position.
 * @return True when the range is valid.
 */
bool TileEngine::validMeleeRange(Position pos, int direction, BattleUnit *attacker, BattleUnit *target, Position *dest, bool preferEnemy)
{
	if (direction < 0 || direction > 7)
	{
		return false;
	}
	std::vector<BattleUnit*> potentialTargets;
	BattleUnit *chosenTarget = 0;
	Position p;
	int size = attacker->getArmor()->getSize() - 1;
	Pathfinding::directionToVector(direction, &p);
	for (int x = 0; x <= size; ++x)
	{
		for (int y = 0; y <= size; ++y)
		{
			Tile *origin (_save->getTile(Position(pos + Position(x, y, 0))));
			Tile *targetTile (_save->getTile(Position(pos + Position(x, y, 0) + p)));
			Tile *aboveTargetTile (_save->getTile(Position(pos + Position(x, y, 1) + p)));
			Tile *belowTargetTile (_save->getTile(Position(pos + Position(x, y, -1) + p)));

			if (targetTile && origin)
			{
				if (origin->getTerrainLevel() <= -16 && aboveTargetTile && !aboveTargetTile->hasNoFloor(targetTile))
				{
					targetTile = aboveTargetTile;
				}
				else if (belowTargetTile && targetTile->hasNoFloor(belowTargetTile) && !targetTile->getUnit() && belowTargetTile->getTerrainLevel() <= -16)
				{
					targetTile = belowTargetTile;
				}
				if (targetTile->getUnit())
				{
					if (target == 0 || targetTile->getUnit() == target)
					{
						Position originVoxel = Position(origin->getPosition() * Position(16,16,24))
							+ Position(8,8,attacker->getHeight() + attacker->getFloatHeight() - 4 -origin->getTerrainLevel());
						Position targetVoxel;
						if (canTargetUnit(&originVoxel, targetTile, &targetVoxel, attacker))
						{
							if (dest)
							{
								*dest = targetTile->getPosition();
							}
							if (target != 0)
							{
								return true;
							}
							else
							{
								potentialTargets.push_back(targetTile->getUnit());
							}
						}
					}
				}
			}
		}
	}

	for (std::vector<BattleUnit*>::const_iterator i = potentialTargets.begin(); i != potentialTargets.end(); ++i)
	{
		// if there's actually something THERE, we'll chalk this up as a success.
		if (!chosenTarget)
		{
			chosenTarget = *i;
		}
		// but if there's a target of a different faction, we'll prioritize them.
		else if ((preferEnemy && (*i)->getFaction() != attacker->getFaction())
		// or, if we're using a medikit, prioritize whichever friend is wounded the most.
		|| (!preferEnemy && (*i)->getFaction() == attacker->getFaction() &&
		(*i)->getFatalWounds() > chosenTarget->getFatalWounds()))
		{
			chosenTarget = *i;
		}
	}

	if (dest && chosenTarget)
	{
		*dest = chosenTarget->getPosition();
	}

	return chosenTarget != 0;
}

/**
 * Gets the AI to look through a window.
 * @param position Current position.
 * @return Direction or -1 when no window found.
 */
int TileEngine::faceWindow(const Position &position)
{
	static const Position oneTileEast = Position(1, 0, 0);
	static const Position oneTileSouth = Position(0, 1, 0);

	Tile *tile = _save->getTile(position);
	if (tile && tile->getMapData(O_NORTHWALL) && tile->getMapData(O_NORTHWALL)->getBlock(DT_NONE)==0) return 0;
	tile = _save->getTile(position + oneTileEast);
	if (tile && tile->getMapData(O_WESTWALL) && tile->getMapData(O_WESTWALL)->getBlock(DT_NONE)==0) return 2;
	tile = _save->getTile(position + oneTileSouth);
	if (tile && tile->getMapData(O_NORTHWALL) && tile->getMapData(O_NORTHWALL)->getBlock(DT_NONE)==0) return 4;
	tile = _save->getTile(position);
	if (tile && tile->getMapData(O_WESTWALL) && tile->getMapData(O_WESTWALL)->getBlock(DT_NONE)==0) return 6;

	return -1;
}

/**
 * Validates a throw action.
 * @param action The action to validate.
 * @param originVoxel The origin point of the action.
 * @param targetVoxel The target point of the action.
 * @param curve The curvature of the throw.
 * @param voxelType The type of voxel at which this parabola terminates.
 * @return Validity of action.
 */
bool TileEngine::validateThrow(BattleAction &action, Position originVoxel, Position targetVoxel, double *curve, int *voxelType)
{
	bool foundCurve = false;
	double curvature = 0.5;
	if (action.type == BA_THROW)
	{
		curvature = std::max(0.48, 1.73 / sqrt(sqrt((double)(action.actor->getBaseStats()->strength) / (double)(action.weapon->getRules()->getWeight()))) + (action.actor->isKneeled()? 0.1 : 0.0));
	}
	else
	{
		// arcing projectile weapons assume a fixed strength and weight.(70 and 10 respectively)
		// curvature should be approximately 1.06358350461 at this point.
		curvature = 1.73 / sqrt(sqrt(70.0 / 10.0)) + (action.actor->isKneeled()? 0.1 : 0.0);
	}

	Tile *targetTile = _save->getTile(action.target);
	// object blocking - can't throw here
	if ((action.type == BA_THROW
		&& targetTile
		&& targetTile->getMapData(O_OBJECT)
		&& targetTile->getMapData(O_OBJECT)->getTUCost(MT_WALK) == 255)
		|| ProjectileFlyBState::validThrowRange(&action, originVoxel, targetTile) == false)
	{
		return false;
	}

	// we try 8 different curvatures to try and reach our goal.
	int test = V_OUTOFBOUNDS;
	while (!foundCurve && curvature < 5.0)
	{
		std::vector<Position> trajectory;
		test = calculateParabola(originVoxel, targetVoxel, false, &trajectory, action.actor, curvature, Position(0,0,0));
		if (test != V_OUTOFBOUNDS && (trajectory.at(0) / Position(16, 16, 24)) == (targetVoxel / Position(16, 16, 24)))
		{
			if (voxelType)
			{
				*voxelType = test;
			}
			foundCurve = true;
		}
		else
		{
			curvature += 0.5;
		}
	}
	if (curvature >= 5.0)
	{
		return false;
	}
	if (curve)
	{
		*curve = curvature;
	}

	return true;
}

/**
 * Recalculates FOV of all units in-game.
 */
void TileEngine::recalculateFOV()
{
	for (std::vector<BattleUnit*>::iterator bu = _save->getUnits()->begin(); bu != _save->getUnits()->end(); ++bu)
	{
		if ((*bu)->getTile() != 0)
		{
			calculateFOV(*bu);
		}
	}
}

/**
 * Returns the direction from origin to target.
 * @param origin The origin point of the action.
 * @param target The target point of the action.
 * @return direction.
 */
int TileEngine::getDirectionTo(const Position &origin, const Position &target) const
{
	double ox = target.x - origin.x;
	double oy = target.y - origin.y;
	double angle = atan2(ox, -oy);
	// divide the pie in 4 angles each at 1/8th before each quarter
	double pie[4] = {(M_PI_4 * 4.0) - M_PI_4 / 2.0, (M_PI_4 * 3.0) - M_PI_4 / 2.0, (M_PI_4 * 2.0) - M_PI_4 / 2.0, (M_PI_4 * 1.0) - M_PI_4 / 2.0};
	int dir = 0;

	if (angle > pie[0] || angle < -pie[0])
	{
		dir = 4;
	}
	else if (angle > pie[1])
	{
		dir = 3;
	}
	else if (angle > pie[2])
	{
		dir = 2;
	}
	else if (angle > pie[3])
	{
		dir = 1;
	}
	else if (angle < -pie[1])
	{
		dir = 5;
	}
	else if (angle < -pie[2])
	{
		dir = 6;
	}
	else if (angle < -pie[3])
	{
		dir = 7;
	}
	else if (angle < pie[0])
	{
		dir = 0;
	}
	return dir;
}

/**
 * Calculate arc between two unit directions.
 * e.g. Arc of 7 and 3 is 4. Arc of 0 and 7 is 1.
 * @param directionA Value of 0 to 7
 * @param directionB Value of 0 to 7
 * @return Value in range of 0 - 4
 */
int TileEngine::getArcDirection(int directionA, int directionB) const
{
	return std::abs((((directionA - directionB) + 12) % 8) - 4);
}

/**
 * Gets the origin voxel of a certain action.
 * @param action Battle action.
 * @param tile Pointer to the action tile.
 * @return origin position.
 */
Position TileEngine::getOriginVoxel(BattleAction &action, Tile *tile)
{
	const int dirYshift[8] = {1, 1, 8, 15,15,15,8, 1};
	const int dirXshift[8] = {8, 14,15,15,8, 1, 1, 1};
	if (!tile)
	{
		tile = action.actor->getTile();
	}

	Position origin = tile->getPosition();
	Tile *tileAbove = _save->getTile(origin + Position(0,0,1));
	Position originVoxel = Position(origin.x*16, origin.y*16, origin.z*24);

	// take into account soldier height and terrain level if the projectile is launched from a soldier
	if (action.actor->getPosition() == origin || action.type != BA_LAUNCH)
	{
		// calculate offset of the starting point of the projectile
		originVoxel.z += -tile->getTerrainLevel();

		originVoxel.z += action.actor->getHeight() + action.actor->getFloatHeight();

		if (action.type == BA_THROW)
		{
			originVoxel.z -= 3;
		}
		else
		{
			originVoxel.z -= 4;
		}

		if (originVoxel.z >= (origin.z + 1)*24)
		{
			if (tileAbove && tileAbove->hasNoFloor(0))
			{
				origin.z++;
			}
			else
			{
				while (originVoxel.z >= (origin.z + 1)*24)
				{
					originVoxel.z--;
				}
				originVoxel.z -= 4;
			}
		}
		int direction = getDirectionTo(origin, action.target);
		originVoxel.x += dirXshift[direction]*action.actor->getArmor()->getSize();
		originVoxel.y += dirYshift[direction]*action.actor->getArmor()->getSize();
	}
	else
	{
		// don't take into account soldier height and terrain level if the projectile is not launched from a soldier(from a waypoint)
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 16;
	}
	return originVoxel;
}

/**
 * mark a region of the map as "dangerous" for a turn.
 * @param pos is the epicenter of the explosion.
 * @param radius how far to spread out.
 * @param unit the unit that is triggering this action.
 */
void TileEngine::setDangerZone(Position pos, int radius, BattleUnit *unit)
{
	Tile *tile = _save->getTile(pos);
	if (!tile)
	{
		return;
	}
	// set the epicenter as dangerous
	tile->setDangerous();
	Position originVoxel = (pos * Position(16,16,24)) + Position(8,8,12 + -tile->getTerrainLevel());
	Position targetVoxel;
	for (int x = -radius; x != radius; ++x)
	{
		for (int y = -radius; y != radius; ++y)
		{
			// we can skip the epicenter
			if (x != 0 || y != 0)
			{
				// make sure we're within the radius
				if ((x*x)+(y*y) <= (radius*radius))
				{
					tile = _save->getTile(pos + Position(x,y,0));
					if (tile)
					{
						targetVoxel = ((pos + Position(x,y,0)) * Position(16,16,24)) + Position(8,8,12 + -tile->getTerrainLevel());
						std::vector<Position> trajectory;
						// we'll trace a line here, ignoring all units, to check if the explosion will reach this point
						// granted this won't properly account for explosions tearing through walls, but then we can't really
						// know that kind of information before the fact, so let's have the AI assume that the wall (or tree)
						// is enough to protect them.
						if (calculateLine(originVoxel, targetVoxel, false, &trajectory, unit, true, false, unit) == V_EMPTY)
						{
							if (trajectory.size() && (trajectory.back() / Position(16,16,24)) == pos + Position(x,y,0))
							{
								tile->setDangerous();
							}
						}
					}
				}
			}
		}
	}
}

}
