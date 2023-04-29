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
#include "BattleState.h"
#include "BattlescapeGame.h"
#include "Position.h"

namespace OpenXcom
{

class BattleUnit;
class Pathfinding;
class TileEngine;

/**
 * State for walking units.
 */
class UnitWalkBState : public BattleState
{
private:
	Position _target;
	BattleUnit *_unit;
	Pathfinding *_pf;
	TileEngine *_terrain;

	/// Unit will fall down always.
	bool _falling = false;
	/// Allow to move over some tiles that normally should fall down.
	bool _fallingWhenStopped = false;

	bool _beforeFirstStep;
	/// Handles some calculations when the path is finished.
	void postPathProcedures();
	/// Handles some calculations when the walking is finished.
	void setNormalWalkSpeed();
	/// Handles the stepping sounds.
	void playMovementSound();
	std::size_t _numUnitsSpotted;
	int _preMovementCost;
public:
	/// Creates a new UnitWalkBState class.
	UnitWalkBState(BattlescapeGame *parent, BattleAction _action);
	/// Cleans up the UnitWalkBState.
	~UnitWalkBState();
	/// Initializes the state.
	void init() override;
	/// Deinitializes the state.
	void deinit() override;
	/// Handles a cancels request.
	void cancel() override;
	/// Runs state functionality every cycle.
	void think() override;
};

}
