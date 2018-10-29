#pragma once
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
#include <vector>
#include "Position.h"
#include "PathfindingNode.h"
#include "../Mod/MapData.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

class Tile;
class BattleUnit;
struct BattleActionCost;

/**
 * A utility class that calculates the shortest path between two points on the battlescape map.
 */
class Pathfinding
{
public:
	/// symbolic name for directions on all 3 adjacent planes Horizontal, Up, Down and the from North
	/// towards East trhrough South till NorthWest
	enum directions {
		DIR_HN, DIR_HNE, DIR_HE, DIR_HSE, DIR_HS, DIR_HSW, DIR_HW, DIR_HNW,
		DIR_UP, DIR_UN, DIR_UNE, DIR_UE, DIR_USE, DIR_US, DIR_USW, DIR_UW, DIR_UNW,
		DIR_DOWN, DIR_DN, DIR_DNE, DIR_DE, DIR_DSE, DIR_DS, DIR_DSW, DIR_DW, DIR_DNW, DIR_TOTAL
	};
	/// "steps" to be added to cycle cardinal directions
	static const int DIR_CARD_STEP = 2;
	/// steps to perform a full horizontal turn
	static const int DIR_FULL_HTURN = 4;
	/// adjacent move impossible cost
	static const int FORBID_MOVE = 255;
private:
	constexpr static int dir_max = DIR_TOTAL;
	constexpr static int dir_x[dir_max] = { 0, 1, 1, 1, 0, -1, -1, -1, 0, 0, 1, 1, 1, 0, -1, -1, -1, 0, 0, 1, 1, 1, 0, -1, -1, -1 };
	constexpr static int dir_y[dir_max] = { -1, -1, 0, 1, 1, 1, 0, -1, 0, -1, -1, 0, 1, 1, 1, 0, -1, 0, -1, -1, 0, 1, 1, 1, 0, -1 };
	constexpr static int dir_z[dir_max] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

	SavedBattleGame *_save;
	std::vector<PathfindingNode> _nodes;
	int _size;
	BattleUnit *_unit;
	bool _pathPreviewed;
	bool _strafeMove;
	int _totalTUCost;
	bool _modifierUsed;
	MovementType _movementType;
	/// Gets the node at certain position.
	PathfindingNode *getNode(Position pos);
	/// Determines whether a tile blocks a certain movementType.
	bool isBlocked(Tile *tile, int part, BattleUnit *missileTarget, int bigWallExclusion = -1) const;
	/// Tries to find a straight line path between two positions.
	bool bresenhamPath(Position origin, Position target, BattleUnit *missileTarget, bool sneak = false, int maxTUCost = 1000);
	/// Tries to find a path between two positions.
	bool aStarPath(Position origin, Position target, BattleUnit *missileTarget, bool sneak = false, int maxTUCost = 1000);
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(Tile *destinationTile) const;
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(Tile *destinationTile, int size) const;
	/// get the tile pointed by the direction from origin
	Tile *getTile(const Position& origin, int direction) const
	{
		Position endPosition;
		directionToVector(direction, &endPosition);
		endPosition += origin;
		return _save->getTile(endPosition);
	}
	/// get the next direction from the table above, checking if 3dFlight is enabled
	static int nextDirection(int direction)
	{
		if (!Options::threeDFlight && direction == DIR_UP)
			return DIR_DOWN;
		else
			return direction + 1;
	}
	std::vector<int> _path;
public:
	/// Determines whether the unit is going up a stairs.
	bool isOnStairs(Position startPosition, Position endPosition) const;
	/// Determines whether or not movement between starttile and endtile is possible in the direction.
	bool isBlocked(Tile *startTile, Tile *endTile, const int direction, BattleUnit *missileTarget);
	enum bigWallTypes{ BLOCK = 1, BIGWALLNESW, BIGWALLNWSE, BIGWALLWEST, BIGWALLNORTH, BIGWALLEAST, BIGWALLSOUTH, BIGWALLEASTANDSOUTH, BIGWALLWESTANDNORTH};
	static const int O_BIGWALL = -1;
	static int red;
	static int green;
	static int yellow;
	/// Creates a new Pathfinding class.
	Pathfinding(SavedBattleGame *save);
	/// Cleans up the Pathfinding.
	~Pathfinding();
	/// Calculates the shortest path.
	void calculate(BattleUnit *unit, Position endPosition, BattleUnit *missileTarget = 0, int maxTUCost = 1000);

	/**
	 * Converts direction to a vector. Direction starts north = 0 and goes clockwise.
	 * @param direction Source direction.
	 * @param vector Pointer to a position (which acts as a vector).
	 */
	static void directionToVector(int direction, Position *vector)
	{
		vector->x = dir_x[direction];
		vector->y = dir_y[direction];
		vector->z = dir_z[direction];
	}

	/**
	 * Converts direction to a vector. Direction starts north = 0 and goes clockwise.
	 * @param vector Pointer to a position (which acts as a vector).
	 * @param dir Resulting direction.
	 */
	static void vectorToDirection(const Position &vector, int &dir)
	{
		dir = -1;
		for (int i = 0; i < 8; ++i)
		{
			if (dir_x[i] == vector.x && dir_y[i] == vector.y)
			{
				dir = i;
				return;
			}
		}
	}
	/// Converts direction to a vector (horizontal component only).
	static void directionToVectorH(int direction, Position *vector)
	{
		vector->x = dir_x[direction];
		vector->y = dir_y[direction];
		vector->z = 0;
	}
	/// Converts a 3d direction into it's horizontal direction projection
	static int horizontalDirection(int direction)
	{
		if (direction < 0)
			return -1;
		else if (direction < DIR_UP)
			return direction;
		else if (direction < DIR_DOWN)
			return direction - DIR_UN;
		else if (direction < DIR_TOTAL)
			return direction - DIR_DN;
		else
			return -1;
	}
	/// handles turning
	static void turnRight(int &direction)
	{
		switch (direction)
		{
		case DIR_HNW:
			direction = DIR_HN;
			return;
		case DIR_UNW:
			direction = DIR_UN;
			return;
		case DIR_DNW:
			direction = DIR_DN;
			return;
		case DIR_UP:
		case DIR_DOWN:
			return;
		default:
			++direction;
		}
	}
	static void turnLeft(int &direction)
	{
		switch (direction)
		{
		case DIR_HN:
			direction = DIR_HNW;
			return;
		case DIR_UN:
			direction = DIR_UNW;
			return;
		case DIR_DN:
			direction = DIR_DNW;
			return;
		case DIR_UP:
		case DIR_DOWN:
			return;
		default:
			--direction;
		}
	}
	static void turn(int &direction, int toDirection)
	{
		int dirfrom = horizontalDirection(direction), dirto = horizontalDirection(toDirection);
		if (dirfrom == -1 || dirto == -1)
			return;
		if (dirto - dirfrom <= -4)
			turnRight(direction);
		else if (dirto < dirfrom)
			turnLeft(direction);
		else if (dirto == dirfrom)
			return;
		else if (dirto - dirfrom <= 4)
			turnRight(direction);
		else
			turnLeft(direction);
	}
	/// Checks whether a path is ready and gives the first direction.
	int getStartDirection() const;
	/// Dequeues a direction.
	int dequeuePath();
	/// Gets the TU cost to move from 1 tile to the other.
	int getTUCost(Position startPosition, int direction, Position *endPosition, BattleUnit *unit, BattleUnit *target, bool missile);
	/// Aborts the current path.
	void abortPath();
	/// Gets the strafe move setting.
	bool getStrafeMove() const;
	/// Checks, for a vertical or combo move, if the movement is valid (returns MAX_COST overflowing if impossible move).
	int costUpDown(BattleUnit *bu, const Position& startPosition, int direction, bool missile = false) const;
	/// Previews the path.
	bool previewPath(bool bRemove = false);
	/// Removes the path preview.
	bool removePreview();
	/// Sets _unit in order to abuse low-level pathfinding functions from outside the class.
	void setUnit(BattleUnit *unit);
	/// Gets all reachable tiles, based on cost.
	std::vector<int> findReachable(BattleUnit *unit, const BattleActionCost &cost);
	/// Gets _totalTUCost; finds out whether we can hike somewhere in this turn or not.
	int getTotalTUCost() const { return _totalTUCost; }
	/// Gets the path preview setting.
	bool isPathPreviewed() const;
	/// Gets the modifier setting.
	bool isModifierUsed() const;
	/// Gets a reference to the path.
	const std::vector<int> &getPath() const;
	/// Makes a copy to the path.
	std::vector<int> copyPath() const;
};

}
