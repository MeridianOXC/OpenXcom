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
#include "../Engine/Yaml.h"
#include "BattlescapeGame.h"
#include "Position.h"
#include "../Savegame/BattleUnit.h"
#include <vector>


namespace OpenXcom
{

class SavedBattleGame;
class BattleUnit;
struct BattleAction;
class BattlescapeState;
class Node;

enum AIMode { AI_PATROL, AI_AMBUSH, AI_COMBAT, AI_ESCAPE };
enum AIAttackWeight : int
{
	/// Base scale of attack weights
	AIW_SCALE = 100,
	AIW_IGNORED = 0,
};

/**
 * This class is used by the BattleUnit AI.
 */
class AIModule
{
private:
	SavedBattleGame *_save;
	BattleUnit *_unit;
	BattleUnit *_aggroTarget;
	int _knownEnemies, _visibleEnemies, _spottingEnemies;
	int _escapeTUs, _ambushTUs;
	bool _weaponPickedUp;
	bool _rifle, _melee, _blaster, _grenade;
	bool _traceAI, _didPsi;
	int _AIMode, _intelligence, _closestDist;
	Node *_fromNode, *_toNode;
	bool _foundBaseModuleToDestroy;
	std::vector<int> _reachable, _reachableWithAttack, _wasHitBy;
	BattleActionType _reserve;
	UnitFaction _targetFaction;

	BattleAction _escapeAction, _ambushAction, _attackAction, _patrolAction, _psiAction;

	bool selectPointNearTargetLeeroy(BattleUnit *target, bool canRun);
	int selectNearestTargetLeeroy(bool canRun);
	void meleeActionLeeroy(bool canRun);
	void dont_think(BattleAction *action);
public:
	bool medikit_think(BattleMediKitType healOrStim);
public:
	/// Creates a new AIModule linked to the game and a certain unit.
	AIModule(SavedBattleGame *save, BattleUnit *unit, Node *node);
	/// Cleans up the AIModule.
	~AIModule();
	/// Sets the target faction.
	void setTargetFaction(UnitFaction f);
	/// Resets the unsaved AI state.
	void reset();
	/// Loads the AI Module from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Saves the AI Module to YAML.
	void save(YAML::YamlNodeWriter writer) const;
	/// Runs Module functionality every AI cycle.
	void think(BattleAction *action);
	/// Sets the "unit was hit" flag true.
	void setWasHitBy(BattleUnit *attacker);
	/// Sets the "unit picked up a weapon" flag.
	void setWeaponPickedUp();
	/// Gets whether the unit was hit.
	bool getWasHitBy(int attacker) const;
	/// Set start node.
	void setStartNode(Node *node) { _fromNode = node; }
	/// setup a patrol objective.
	void setupPatrol();
	/// setup an ambush objective.
	void setupAmbush();
	/// setup a combat objective.
	void setupAttack();
	/// setup an escape objective.
	void setupEscape();
	/// count how many xcom/civilian units are known to this unit.
	int countKnownTargets() const;
	/// count how many known XCom units are able to see this unit.
	int getSpottingUnits(const Position& pos) const;
	/// Selects the nearest target we can see, and return the number of viable targets.
	int selectNearestTarget();
	/// Selects the closest known xcom unit for ambushing.
	bool selectClosestKnownEnemy();
	/// Selects a random known target.
	bool selectRandomTarget();
	/// Selects the nearest reachable point relative to a target.
	bool selectPointNearTarget(BattleUnit *target, int maxTUs);
	/// Selects a target from a list of units seen by spotter units for out-of-LOS actions
	bool selectSpottedUnitForSniper();
	/// Scores a firing mode action based on distance to target and accuracy.
	int scoreFiringMode(BattleAction *action, BattleUnit *target, bool checkLOF);
	/// re-evaluate our situation, and make a decision from our available options.
	void evaluateAIMode();
	/// Selects a suitable position from which to attack.
	bool findFirePoint();
	/// Decides if we should throw a grenade/launch a missile to this position.
	int explosiveEfficacy(Position targetPos, BattleUnit *attackingUnit, int radius, int diff, bool grenade = false) const;
	bool getNodeOfBestEfficacy(BattleAction *action, int radius);
	/// Attempts to take a melee attack/charge an enemy we can see.
	void meleeAction();
	/// Attempts to fire a waypoint projectile at an enemy we, or one of our teammates sees.
	void wayPointAction();
	/// Attempts to fire at an enemy spotted for us.
	bool sniperAction();
	/// Attempts to fire at an enemy we can see.
	void projectileAction();
	/// Chooses a firing mode for the AI based on expected number of hits per turn
	void extendedFireModeChoice(BattleActionCost& costAuto, BattleActionCost& costSnap, BattleActionCost& costAimed, BattleActionCost& costThrow, bool checkLOF = false);
	/// Attempts to throw a grenade at an enemy (or group of enemies) we can see.
	void grenadeAction();
	/// Performs a psionic attack.
	bool psiAction();
	/// Performs a melee attack action.
	void meleeAttack();

	/// How much given unit is worth as target of attack.
	AIAttackWeight getTargetAttackWeight(BattleUnit* target) const;
	/// Checks to make sure a target is valid, given the parameters
	bool validTarget(BattleUnit* target, bool assessDanger, bool includeCivs) const;

	/// Checks the alien's TU reservation setting.
	BattleActionType getReserveMode();
	/// Assuming we have both a ranged and a melee weapon, we have to select one.
	void selectMeleeOrRanged();
	/// Gets the current targetted unit.
	BattleUnit* getTarget();
	/// Frees up the destination node for another Unit to select
	void freePatrolTarget();
};

}
