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

namespace OpenXcom
{

class SavedBattleGame;
class Tile;
class BattleUnit;
struct BattleActionCost;

enum BattleActionMove : char;


/**
 * A utility class that calculates the shortest path between two points on the battlescape map.
 */
class Pathfinding
{
private:
	constexpr static int dir_max = 10;
	constexpr static int dir_x[dir_max] = {  0, +1, +1, +1,  0, -1, -1, -1,  0,  0};
	constexpr static int dir_y[dir_max] = { -1, -1,  0, +1, +1, +1,  0, -1,  0,  0};
	constexpr static int dir_z[dir_max] = {  0,  0,  0,  0,  0,  0,  0,  0, +1, -1};

	SavedBattleGame *_save;
	std::vector<PathfindingNode> _nodes;
	int _size;
	BattleUnit *_unit;
	bool _pathPreviewed;
	bool _strafeMove;
	bool _ctrlUsed = false;
	bool _altUsed = false;
	PathfindingCost _totalTUCost;

	/// Gets the node at certain position.
	PathfindingNode *getNode(Position pos);

	/// Gets movement type of unit or movement of missile.
	MovementType getMovementType(const BattleUnit *unit, const BattleUnit *missileTarget, BattleActionMove bam) const;
	/// Determines whether a tile blocks a certain movementType.
	bool isBlocked(const BattleUnit *unit, const Tile *tile, const int part, BattleActionMove bam, const BattleUnit *missileTarget, int bigWallExclusion = -1) const;
	/// Determines whether or not movement between start tile and end tile is possible in the direction.
	bool isBlockedDirection(const BattleUnit *unit, const Tile *startTile, const int direction, BattleActionMove bam, const BattleUnit *missileTarget) const;
	/// Tries to find a straight line path between two positions.
	bool bresenhamPath(Position origin, Position target, BattleActionMove bam, const BattleUnit *missileTarget, bool sneak = false, int maxTUCost = 1000);
	/// Tries to find a path between two positions.
	bool aStarPath(Position origin, Position target, BattleActionMove bam, const BattleUnit *missileTarget, bool sneak = false, int maxTUCost = 1000);
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(const Tile *destinationTile) const;
	/// Determines whether a unit can fall down from this tile.
	bool canFallDown(const Tile *destinationTile, int size) const;
	std::vector<int> _path;
public:
	/// Determines whether the unit is going up a stairs.
	bool isOnStairs(Position startPosition, Position endPosition) const;
	/// Determines whether or not movement between start tile and end tile is possible in the direction.
	bool isBlockedDirection(const BattleUnit *unit, Tile *startTile, const int direction) const;

	/// Default move cost for tile that have floor with 0 cost.
	static constexpr int DEFAULT_MOVE_COST = 4;
	/// Default move cost for changing level for fly or gravlift.
	static constexpr int DEFAULT_MOVE_FLY_COST = 8;
	/// How much time units one move can have.
	static constexpr int MAX_MOVE_COST = 100;
	/// Fake cost representing invalid move.
	static constexpr int INVALID_MOVE_COST = 255;
	/// Fire penalty used in path search.
	static constexpr int FIRE_PREVIEW_MOVE_COST = 32;

	static const int DIR_UP = 8;
	static const int DIR_DOWN = 9;
	enum bigWallTypes
	{
		/**
		     /###\
		 W  /#####\  N
		   /#######\
		  |#########|
		   \#######/
		 S  \#####/  E
		     \###/
		 */
		BLOCK = 1,

		/**
		     /   \
		 W  /     \  N
		   /#######\
		  |#########|
		   \#######/
		 S  \     /  E
		     \   /
		 */
		BIGWALLNESW = 2,

		/**
		     /###\
		 W  / ### \  N
		   /  ###  \
		  |   ###   |
		   \  ###  /
		 S  \ ### /  E
		     \###/
		 */
		BIGWALLNWSE = 3,

		/**
		     /## \
		 W  /##   \  N
		   /##     \
		  |##       |
		   \       /
		 S  \     /  E
		     \   /
		 */
		BIGWALLWEST = 4,

		/**
		     / ##\
		 W  /   ##\  N
		   /     ##\
		  |       ##|
		   \       /
		 S  \     /  E
		     \   /
		 */
		BIGWALLNORTH = 5,

		/**
		     /   \
		 W  /     \  N
		   /       \
		  |       ##|
		   \     ##/
		 S  \   ##/  E
		     \ ##/
		 */
		BIGWALLEAST = 6,

		/**
		     /   \
		 W  /     \  N
		   /       \
		  |##       |
		   \##     /
		 S  \##   /  E
		     \## /
		 */
		BIGWALLSOUTH = 7,

		/**
		     /   \
		 W  /     \  N
		   /       \
		  |##     ##|
		   \##   ##/
		 S  \#####/  E
		     \###/
		 */
		BIGWALLEASTANDSOUTH = 8,

		/**
		     /###\
		 W  /#####\  N
		   /##   ##\
		  |##     ##|
		   \       /
		 S  \     /  E
		     \   /
		 */
		BIGWALLWESTANDNORTH = 9,
	};
	static const int O_BIGWALL = -1;
	static int red;
	static int green;
	static int yellow;

	/// Creates a new Pathfinding class.
	Pathfinding(SavedBattleGame *save);
	/// Cleans up the Pathfinding.
	~Pathfinding();
	/// Calculates the shortest path.
	void calculate(BattleUnit *unit, Position endPosition, BattleActionMove bam, const BattleUnit *missileTarget = 0, int maxTUCost = 1000);

	/**
	 * Converts direction to a vector. Direction starts north = 0 and goes clockwise.
	 * @param direction Source direction.
	 * @param vector Pointer to a position (which acts as a vector).
	 */
	constexpr static void directionToVector(int direction, Position *vector)
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
	constexpr static void vectorToDirection(Position vector, int &dir)
	{
		dir = vectorToDirection(vector);
	}

	/**
	 * Converts direction to a vector. Direction starts north = 0 and goes clockwise.
	 * @param vector Pointer to a position (which acts as a vector).
	 * @return dir Resulting direction.
	 */
	constexpr static int vectorToDirection(Position vector)
	{
		for (int i = 0; i < 8; ++i)
		{
			if (dir_x[i] == vector.x && dir_y[i] == vector.y)
			{
				return i;
			}
		}
		return -1;
	}

	/// Checks whether a path is ready and gives the first direction.
	int getStartDirection() const;
	/// Dequeues a direction.
	int dequeuePath();
	/// Gets the TU cost to move from 1 tile to the other.
	PathfindingStep getTUCost(Position startPosition, int direction, const BattleUnit *unit, const BattleUnit *missileTarget, BattleActionMove bam) const;
	/// Aborts the current path.
	void abortPath();
	/// Gets the strafe move setting.
	bool getStrafeMove() const;
	/// Checks, for the up/down button, if the movement is valid.
	bool validateUpDown(const BattleUnit *bu, const Position& startPosition, const int direction, bool missile = false) const;

	/// Previews the path.
	bool previewPath(bool bRemove = false);
	/// Removes the path preview.
	bool removePreview();
	/// Refresh the path preview.
	void refreshPath();

	/// Sets _unit in order to abuse low-level pathfinding functions from outside the class.
	void setUnit(BattleUnit *unit);
	/// Gets all reachable tiles, based on cost.
	std::vector<int> findReachable(const BattleUnit *unit, const BattleActionCost &cost);
	/// Gets _totalTUCost; finds out whether we can hike somewhere in this turn or not.
	int getTotalTUCost() const { return _totalTUCost.time; }
	/// Gets the path preview setting.
	bool isPathPreviewed() const;
	/// Gets the modifier setting.
	bool isModifierCtrlUsed() const { return _ctrlUsed; }
	/// Gets the modifier setting.
	bool isModifierAltUsed() const { return _altUsed; }
	/// Gets a reference to the path.
	const std::vector<int> &getPath() const;
	/// Makes a copy to the path.
	std::vector<int> copyPath() const;
};

}
