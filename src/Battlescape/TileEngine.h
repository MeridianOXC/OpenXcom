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
#ifndef OPENXCOM_TILEENGINE_H
#define OPENXCOM_TILEENGINE_H

#include <vector>
#include "Position.h"
#include "BattlescapeGame.h"
#include "../Mod/RuleItem.h"
#include <SDL.h>

namespace OpenXcom
{

class SavedBattleGame;
class BattleUnit;
class BattleItem;
class Tile;
struct BattleAction;


/**
 * A utility class that modifies tile properties on a battlescape map. This includes lighting, destruction, smoke, fire, fog of war.
 * Note that this function does not handle any sounds or animations.
 */
class TileEngine
{
private:
	SavedBattleGame *_save;
	std::vector<Uint16> *_voxelData;
	static const int heightFromCenter[11];
	bool _personalLighting;
	const int _maxViewDistance;        // 20 tiles by default
	const int _maxViewDistanceSq;      // 20 * 20
	const int _maxVoxelViewDistance;   // maxViewDistance * 16
	const int _maxDarknessToSeeUnits;  // 9 by default
	Position _eventVisibilitySectorL, _eventVisibilitySectorR, _eventVisibilityObserverPos;

	/// Add light source.
	void addLight(const Position &center, int power, int layer);
	/// Calculate blockage amount.
	int blockage(Tile *tile, const int part, ItemDamageType type, int direction = -1, bool checkingFromOrigin = false);
	/// Get max view distance.
	inline int getMaxViewDistance() const        {return _maxViewDistance;}
	/// Get square of max view distance.
	inline int getMaxViewDistanceSq() const      {return _maxViewDistanceSq;}
	/// Get max view distance in voxel space.
	inline int getMaxVoxelViewDistance() const   {return _maxVoxelViewDistance;}
	/// Get threshold of darkness for LoS calculation.
	inline int getMaxDarknessToSeeUnits() const  {return _maxDarknessToSeeUnits;}

	void setupEventVisibilitySector(const Position &observerPos, const Position &eventPos, const int &eventRadius);
	inline bool inEventVisibilitySector(const Position &toCheck) const;

	/**
	 * Helper class storing reaction data
	 */
	struct ReactionScore
	{
		BattleUnit *unit;
		int attackType;
		double reactionScore;
		double reactionReduction;
	};

	/// Checks validity of a snap shot to this position.
	ReactionScore determineReactionType(BattleUnit *unit, BattleUnit *target);
	/// Creates a vector of units that can spot this unit.
	std::vector<ReactionScore> getSpottingUnits(BattleUnit* unit);
	/// Given a vector of spotters, and a unit, picks the spotter with the highest reaction score.
	ReactionScore *getReactor(std::vector<ReactionScore> &spotters, BattleUnit *unit);
	/// Tries to perform a reaction snap shot to this location.
	bool tryReaction(BattleUnit *unit, BattleUnit *target, int attackType);
public:
	/// Creates a new TileEngine class.
	TileEngine(SavedBattleGame *save, std::vector<Uint16> *voxelData, int maxViewDistance, int maxDarknessToSeeUnits);
	/// Cleans up the TileEngine.
	~TileEngine();
	/// Calculates sun shading of the whole map.
	void calculateSunShading();
	/// Calculates sun shading for the column of tiles at pos
	void calculateSunShading(const Position &pos);
	/// Calculates sun shading of a single tile.
	void calculateSunShading(Tile *tile);
	/// Calculates visible tiles within the field of view. Supply an eventPosition to do an update limited to a small slice of the view sector.
	void calculateTilesInFOV(BattleUnit *unit, const Position eventPos = { -1, -1, -1 }, const int eventRadius = 0);
	/// Calculates visible units within the field of view. Supply an eventPosition to do an update limited to a small slice of the view sector.
	bool calculateUnitsInFOV(BattleUnit* unit, const Position eventPos = { -1, -1, -1 }, const int eventRadius = 0);
	/// Calculates the field of view from a units view point.
	bool calculateFOV(BattleUnit *unit, bool doTileRecalc = true, bool doUnitRecalc = true);
	/// Calculates the field of view within range of a certain position.
	void calculateFOV(const Position &position, int eventRadius = -1, const bool updateTiles = true, const bool appendToTileVisibility = false);
	/// Checks reaction fire.
	bool checkReactionFire(BattleUnit *unit);
	/// Recalculates lighting of the battlescape for terrain.
	void calculateTerrainLighting();
	/// Recalculates lighting of the battlescape for units.
	void calculateUnitLighting();
	/// Handles tile hit.
	int hitTile(Tile *tile, int damage, const RuleDamageType* type);
	/// Handles experience training.
	bool awardExperience(BattleUnit *unit, BattleItem *weapon, BattleUnit *target, bool rangeAtack);
	/// Handles unit hit.
	bool hitUnit(BattleUnit *unit, BattleItem *clipOrWeapon, BattleUnit *target, const Position &relative, int damage, const RuleDamageType *type, bool rangeAtack = true);
	/// Handles bullet/weapon hits.
	BattleUnit *hit(const Position &center, int power, const RuleDamageType *type, BattleUnit *unit, BattleItem *clipOrWeapon, bool rangeAtack = true);
	/// Handles explosions.
	void explode(const Position &center, int power, const RuleDamageType *type, int maxRadius, BattleUnit *unit, BattleItem *clipOrWeapon, bool rangeAtack = true);
	/// Checks if a destroyed tile starts an explosion.
	Tile *checkForTerrainExplosions();
	/// Unit opens door?
	int unitOpensDoor(BattleUnit *unit, bool rClick = false, int dir = -1);
	/// Closes ufo doors.
	int closeUfoDoors();
	/// Calculates a line trajectory.
	int calculateLine(const Position& origin, const Position& target, bool storeTrajectory, std::vector<Position> *trajectory, BattleUnit *excludeUnit, bool doVoxelCheck = true, bool onlyVisible = false, BattleUnit *excludeAllBut = 0);
	/// Calculates a parabola trajectory.
	int calculateParabola(const Position& origin, const Position& target, bool storeTrajectory, std::vector<Position> *trajectory, BattleUnit *excludeUnit, double curvature, const Position delta);
	/// Gets the origin voxel of a unit's eyesight.
	Position getSightOriginVoxel(BattleUnit *currentUnit);
	/// Checks visibility of a unit on this tile.
	bool visible(BattleUnit *currentUnit, Tile *tile);
	/// Turn XCom soldier's personal lighting on or off.
	void togglePersonalLighting();
	/// Checks the distance between two positions.
	int distance(const Position &pos1, const Position &pos2) const;
	/// Checks the distance squared between two positions.
	int distanceSq(const Position &pos1, const Position &pos2, bool considerZ = true) const;
	/// Checks the horizontal blockage of a tile.
	int horizontalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type, bool skipObject = false);
	/// Checks the vertical blockage of a tile.
	int verticalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type, bool skipObject = false);
	/// Calculate success rate of psi attack.
	int psiAttackCalculate(BattleActionType type, BattleUnit *attacker, BattleUnit *victim, BattleItem *weapon);
	/// Attempts a panic or mind control action.
	bool psiAttack(BattleAction *action);
	/// Attempts a melee attack action.
	bool meleeAttack(BattleAction *action);
	/// Remove the medikit from the game if consumable and empty.
	void medikitRemoveIfEmpty(BattleAction *action);
	/// Try using medikit heal ability.
	void medikitHeal(BattleAction *action, BattleUnit *target, int bodyPart);
	/// Try using medikit stimulant ability.
	void medikitStimulant(BattleAction *action, BattleUnit *target);
	/// Try using medikit pain killer ability.
	void medikitPainKiller(BattleAction *action, BattleUnit *target);
	/// Applies gravity to anything that occupy this tile.
	Tile *applyGravity(Tile *t);
	/// Returns melee validity between two units.
	bool validMeleeRange(BattleUnit *attacker, BattleUnit *target, int dir);
	/// Returns validity of a melee attack from a given position.
	bool validMeleeRange(Position pos, int direction, BattleUnit *attacker, BattleUnit *target, Position *dest, bool preferEnemy = true);
	/// Gets the AI to look through a window.
	int faceWindow(const Position &position);
	/// Checks a unit's % exposure on a tile.
	int checkVoxelExposure(Position *originVoxel, Tile *tile, BattleUnit *excludeUnit, BattleUnit *excludeAllBut);
	/// Checks validity for targetting a unit.
	bool canTargetUnit(Position *originVoxel, Tile *tile, Position *scanVoxel, BattleUnit *excludeUnit, BattleUnit *potentialUnit = 0);
	/// Check validity for targetting a tile.
	bool canTargetTile(Position *originVoxel, Tile *tile, int part, Position *scanVoxel, BattleUnit *excludeUnit);
	/// Calculates the z voxel for shadows.
	int castedShade(const Position& voxel);
	/// Checks the visibility of a given voxel.
	bool isVoxelVisible(const Position& voxel);
	/// Checks what type of voxel occupies this space.
	int voxelCheck(const Position& voxel, BattleUnit *excludeUnit, bool excludeAllUnits = false, bool onlyVisible = false, BattleUnit *excludeAllBut = 0);
	/// Blows this tile up.
	bool detonate(Tile* tile, int power);
	/// Validates a throwing action.
	bool validateThrow(BattleAction &action, Position originVoxel, Position targetVoxel, double *curve = 0, int *voxelType = 0);
	/// Opens any doors this door is connected to.
	std::pair<int, Position> checkAdjacentDoors(Position pos, int part);
	/// Recalculates FOV of all units in-game.
	void recalculateFOV();
	/// Get direction to a certain point
	int getDirectionTo(const Position &origin, const Position &target) const;
	/// Get arc between two direction.
	int getArcDirection(int directionA, int directionB) const;
	/// determine the origin voxel of a given action.
	Position getOriginVoxel(BattleAction &action, Tile *tile);
	/// mark a region of the map as "dangerous" for a turn.
	void setDangerZone(Position pos, int radius, BattleUnit *unit);

};

}

#endif
