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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */
#include "Position.h"
#include "../Mod/RuleItem.h"
#include <SDL.h>
#include <string>
#include <list>
#include <vector>

namespace OpenXcom
{

class BattleUnit;
class SavedBattleGame;
class BattleItem;
class BattleState;
class BattlescapeState;
class Map;
class TileEngine;
class Pathfinding;
class Mod;
class InfoboxOKState;
class SoldierDiary;

enum BattleActionType : Uint8 { BA_NONE, BA_TURN, BA_WALK, BA_KNEEL, BA_PRIME, BA_UNPRIME, BA_THROW, BA_AUTOSHOT, BA_SNAPSHOT, BA_AIMEDSHOT, BA_HIT, BA_USE, BA_LAUNCH, BA_MINDCONTROL, BA_PANIC, BA_RETHINK, BA_EXECUTE, BA_CQB };
enum BattleActionMove { BAM_NORMAL = 0, BAM_RUN = 1, BAM_STRAFE = 2 };

struct BattleActionCost : RuleItemUseCost
{
	BattleActionType type;
	BattleUnit *actor;
	BattleItem *weapon, *origWeapon;

	/// Default constructor.
	BattleActionCost() : type(BA_NONE), actor(0), weapon(0), origWeapon(0) { }

	/// Constructor from unit.
	BattleActionCost(BattleUnit *unit) : type(BA_NONE), actor(0), weapon(0) { }

	/// Constructor with update.
	BattleActionCost(BattleActionType action, BattleUnit *unit, BattleItem *item) : type(action), actor(unit), weapon(item), origWeapon(0) { updateTU(); }

	/// Update value of TU based of actor, weapon and type.
	void updateTU();
	/// Set TU to zero.
	void clearTU();
	/// Test if actor have enough TU to perform weapon action.
	bool haveTU(std::string *message = 0);
	/// Spend TU when actor have enough TU.
	bool spendTU(std::string *message = 0);
};

struct BattleAction : BattleActionCost
{
	Position target;
	std::list<Position> waypoints;
	bool targeting;
	int value;
	std::string result;
	bool strafe, run, ignoreSpottedEnemies;
	int diff;
	int autoShotCounter;
	Position cameraPosition;
	bool desperate; // ignoring newly-spotted units
	int finalFacing;
	bool finalAction;
	int number; // first action of turn, second, etc.?

	/// Default constructor
	BattleAction() : target(-1, -1, -1), targeting(false), value(0), strafe(false), run(false), diff(0), autoShotCounter(0), cameraPosition(0, 0, -1), desperate(false), finalFacing(-1), finalAction(false), number(0) { }
};

struct BattleActionAttack
{
	BattleActionType type;
	BattleUnit *attacker;
	BattleItem *weapon_item;
	BattleItem *damage_item;

	/// Defulat constructor.
	BattleActionAttack(BattleActionType action = BA_NONE, BattleUnit *unit = nullptr) : type{ action }, attacker{ unit }, weapon_item{ nullptr }, damage_item{ nullptr }
	{

	}

	/// Constructor.
	BattleActionAttack(BattleActionType action, BattleUnit *unit, BattleItem *item, BattleItem *ammo);

	/// Constructor.
	BattleActionAttack(const BattleActionCost &action, BattleItem *ammo);
};

/**
 * Battlescape game - the core game engine of the battlescape game.
 */
class BattlescapeGame
{
private:
	SavedBattleGame *_save;
	BattlescapeState *_parentState;
	std::list<BattleState*> _states, _deleted;
	bool _playerPanicHandled;
	int _AIActionCounter;
	BattleAction _currentAction;
	bool _AISecondMove, _playedAggroSound;
	bool _endTurnRequested, _endTurnProcessed;
	bool _endConfirmationHandled;

	/// Ends the turn.
	void endTurn();
	/// Picks the first soldier that is panicking.
	bool handlePanickingPlayer();
	/// Common function for hanlding panicking units.
	bool handlePanickingUnit(BattleUnit *unit);
	/// Determines whether there are any actions pending for the given unit.
	bool noActionsPending(BattleUnit *bu);
	std::vector<InfoboxOKState*> _infoboxQueue;
	/// Shows the infoboxes in the queue (if any).
	void showInfoBoxQueue();
public:
	/// is debug mode enabled in the battlescape?
	static bool _debugPlay;

	/// Creates the BattlescapeGame state.
	BattlescapeGame(SavedBattleGame *save, BattlescapeState *parentState);
	/// Cleans up the BattlescapeGame state.
	~BattlescapeGame();
	/// Checks for units panicking or falling and so on.
	void think();
	/// Initializes the Battlescape game.
	void init();
	/// Determines whether a playable unit is selected.
	bool playableUnitSelected() const;
	/// Handles states timer.
	void handleState();
	/// Pushes a state to the front of the list.
	void statePushFront(BattleState *bs);
	/// Pushes a state to second on the list.
	void statePushNext(BattleState *bs);
	/// Pushes a state to the back of the list.
	void statePushBack(BattleState *bs);
	/// Handles the result of non target actions, like priming a grenade.
	void handleNonTargetAction();
	/// Removes current state.
	void popState();
	/// Sets state think interval.
	void setStateInterval(Uint32 interval);
	/// Checks for casualties in battle.
	void checkForCasualties(const RuleDamageType *damageType, BattleActionAttack attack, bool hiddenExplosion = false, bool terrainExplosion = false);
	/// Checks reserved tu and energy.
	bool checkReservedTU(BattleUnit *bu, int tu, int energy, bool justChecking = false);
	/// Handles unit AI.
	void handleAI(BattleUnit *unit);
	/// Drops an item and affects it with gravity.
	void dropItem(Position position, BattleItem *item, bool removeItem = false, bool updateLight = true);
	/// Converts a unit into a unit of another type.
	BattleUnit *convertUnit(BattleUnit *unit);
	/// Handles kneeling action.
	bool kneel(BattleUnit *bu);
	/// Cancels the current action.
	bool cancelCurrentAction(bool bForce = false);
	/// Gets a pointer to access action members directly.
	BattleAction *getCurrentAction();
	/// Determines whether there is an action currently going on.
	bool isBusy() const;
	/// Activates primary action (left click).
	void primaryAction(Position pos);
	/// Activates secondary action (right click).
	void secondaryAction(Position pos);
	/// Handler for the blaster launcher button.
	void launchAction();
	/// Handler for the psi button.
	void psiButtonAction();
	/// Handle psi attack.
	bool psiAttack(BattleAction *action);
	/// Moves a unit up or down.
	void moveUpDown(BattleUnit *unit, int dir);
	/// Requests the end of the turn (wait for explosions etc to really end the turn).
	void requestEndTurn(bool askForConfirmation);
	/// Sets the TU reserved type.
	void setTUReserved(BattleActionType tur);
	/// Sets the fire mode the selected unit prefers for reaction fire.
	void setReservedAction(BattleActionType type);
	/// Excludes/de-excludes a fire mode from reactions for the selected unit.
	void excludeAction(BattleActionType action, bool exclude = false);
	/// Sets up the cursor taking into account the action.
	void setupCursor();
	/// Gets the map.
	Map *getMap();
	/// Gets the save.
	SavedBattleGame *getSave();
	/// Gets the tilengine.
	TileEngine *getTileEngine();
	/// Gets the pathfinding.
	Pathfinding *getPathfinding();
	/// Gets the mod.
	Mod *getMod();
	/// Returns whether panic has been handled.
	bool getPanicHandled() const { return _playerPanicHandled; }
	/// Tries to find an item and pick it up if possible.
	void findItem(BattleAction *action);
	/// Checks through all the items on the ground and picks one.
	BattleItem *surveyItems(BattleAction *action);
	/// Evaluates if it's worthwhile to take this item.
	bool worthTaking(BattleItem* item, BattleAction *action);
	/// Picks the item up from the ground.
	int takeItemFromGround(BattleItem* item, BattleAction *action);
	/// Assigns the item to a slot (stolen from battlescapeGenerator::addItem()).
	bool takeItem(BattleItem* item, BattleAction *action);
	/// Returns the type of action that is reserved.
	BattleActionType getReservedAction();
	/// Tallies the living units, converting them if necessary.
	bool isSurrendering(BattleUnit* bu);
	void tallyUnits(int &liveAliens, int &liveSoldiers);
	bool convertInfected();
	/// Sets the kneel reservation setting.
	void setKneelReserved(bool reserved);
	/// Checks the kneel reservation setting.
	bool getKneelReserved() const;
	/// Checks for and triggers proximity grenades.
	bool checkForProximityGrenades(BattleUnit *unit);
	/// Cleans up all the deleted states.
	void cleanupDeleted();
	/// Get the depth of the saved game.
	int getDepth() const;
	/// Play sound on battlefield (with direction).
	void playSound(int sound, const Position &pos);
	/// Play sound on battlefield.
	void playSound(int sound);
	/// Sets up a mission complete notification.
	void missionComplete();
	std::list<BattleState*> getStates();
};

}
