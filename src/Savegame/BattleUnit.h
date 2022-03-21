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
#include <string>
#include <unordered_set>
#include "../Battlescape/Position.h"
#include "../Mod/RuleItem.h"
#include "Soldier.h"
#include "BattleItem.h"

namespace OpenXcom
{

class Tile;
class BattleItem;
class Armor;
class Unit;
class BattlescapeGame;
struct BattleAction;
struct BattleActionCost;
struct RuleItemUseCost;
class SavedBattleGame;
class Node;
class Surface;
class RuleInventory;
class RuleEnviroEffects;
class Soldier;
class SavedGame;
class Language;
class AIModule;
template<typename, typename...> class ScriptContainer;
template<typename, typename...> class ScriptParser;
class ScriptWorkerBlit;
struct BattleUnitStatistics;
struct StatAdjustment;

enum SpecialTileType : int;
enum MovementType : int;


enum ForcedTorso : Uint8 { TORSO_USE_GENDER, TORSO_ALWAYS_MALE, TORSO_ALWAYS_FEMALE };
enum UnitSide : Uint8 { SIDE_FRONT, SIDE_LEFT, SIDE_RIGHT, SIDE_REAR, SIDE_UNDER, SIDE_MAX };
enum UnitStatus {STATUS_STANDING, STATUS_WALKING, STATUS_FLYING, STATUS_TURNING, STATUS_AIMING, STATUS_COLLAPSING, STATUS_DEAD, STATUS_UNCONSCIOUS, STATUS_PANICKING, STATUS_BERSERK, STATUS_IGNORE_ME};
enum UnitFaction : int {FACTION_PLAYER, FACTION_HOSTILE, FACTION_NEUTRAL, FACTION_ALIEN_PLAYER};
enum UnitBodyPart : int {BODYPART_HEAD, BODYPART_TORSO, BODYPART_RIGHTARM, BODYPART_LEFTARM, BODYPART_RIGHTLEG, BODYPART_LEFTLEG, BODYPART_MAX};
enum UnitBodyPartEx {BODYPART_LEGS = BODYPART_MAX, BODYPART_COLLAPSING, BODYPART_ITEM_RIGHTHAND, BODYPART_ITEM_LEFTHAND, BODYPART_ITEM_FLOOR, BODYPART_ITEM_INVENTORY, BODYPART_LARGE_TORSO, BODYPART_LARGE_PROPULSION = BODYPART_LARGE_TORSO + 4, BODYPART_LARGE_TURRET = BODYPART_LARGE_PROPULSION + 4};

/**
 * Placeholder class for future functionality.
 */
class BattleUnitVisibility
{
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "BattleUnitVisibility";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);
};

/**
 * Represents a moving unit in the battlescape, player controlled or AI controlled
 * it holds info about it's position, items carrying, stats, etc
 */
class BattleUnit
{
private:
	static const int SPEC_WEAPON_MAX = 3;

	UnitFaction _faction, _originalFaction;
	UnitFaction _killedBy;
	//UnitFaction _spawnUnitFaction = FACTION_HOSTILE; // Affects units spawned from items it seems, JOPER
	UnitFaction _spawnUnitFaction = FACTION_ALIEN_PLAYER;
	int _id;
	Position _pos;
	Tile *_tile;
	Position _lastPos;
	int _direction, _toDirection;
	int _directionTurret, _toDirectionTurret;
	int _verticalDirection;
	Position _destination;
	UnitStatus _status;
	bool _wantsToSurrender, _isSurrendering;
	int _walkPhase, _fallPhase;
	std::vector<BattleUnit *> _visibleUnits, _unitsSpottedThisTurn;
	std::vector<Tile *> _visibleTiles;
	std::unordered_set<Tile *> _visibleTilesLookup;
	int _tu, _energy, _health, _morale, _stunlevel, _mana;
	bool _kneeled, _floating, _dontReselect;
	bool _haveNoFloorBelow = false;
	int _currentArmor[SIDE_MAX], _maxArmor[SIDE_MAX];
	int _fatalWounds[BODYPART_MAX];
	int _fire;
	std::vector<BattleItem*> _inventory;
	BattleItem* _specWeapon[SPEC_WEAPON_MAX];
	AIModule *_currentAIState;
	bool _visible;
	UnitStats _exp, _expTmp;
	int _motionPoints;
	int _scannedTurn;
	int _kills;
	int _faceDirection; // used only during strafing moves
	std::vector<int> _meleeAttackedBy;
	bool _hitByFire, _hitByAnything, _alreadyExploded;
	int _fireMaxHit;
	int _smokeMaxHit;
	int _moraleRestored;
	BattleUnit *_charging;
	int _turnsSinceSpotted, _turnsLeftSpottedForSnipers, _turnsSinceStunned = 255;
	const Unit *_spawnUnit = nullptr;
	std::string _activeHand;
	std::string _preferredHandForReactions;
	BattleUnitStatistics* _statistics;
	int _murdererId;	// used to credit the murderer with the kills that this unit got by blowing up on death
	int _mindControllerID;	// used to credit the mind controller with the kills of the mind controllee
	UnitSide _fatalShotSide;
	UnitBodyPart _fatalShotBodyPart;
	std::string _murdererWeapon, _murdererWeaponAmmo;

	// static data
	std::string _type;
	std::string _rank;
	std::string _race;
	std::string _name;
	UnitStats _stats;
	int _standHeight, _kneelHeight, _floatHeight;
	int _lastReloadSound;
	std::vector<int> _deathSound, _aggroSound;
	std::vector<int> _selectUnitSound, _startMovingSound, _selectWeaponSound, _annoyedSound;
	int _value, _moveSound;
	int _intelligence, _aggression;
	int _maxViewDistanceAtDark, _maxViewDistanceAtDay;
	int _maxViewDistanceAtDarkSquared;
	SpecialAbility _specab;
	Armor *_armor;
	SoldierGender _gender;
	Soldier *_geoscapeSoldier;
	std::vector<int> _loftempsSet;
	Unit *_unitRules;
	int _rankInt;
	int _turretType;
	int _breathFrame;
	bool _breathing;
	bool _hidingForTurn, _floorAbove, _respawn, _alreadyRespawned;
	bool _isLeeroyJenkins;	// always charges enemy, never retreats.
	bool _summonedPlayerUnit, _resummonedFakeCivilian;
	bool _pickUpWeaponsMoreActively;
	bool _disableIndicators;
	MovementType _movementType;
	std::vector<std::pair<Uint8, Uint8> > _recolor;
	bool _capturable;
	bool _vip;
	ScriptValues<BattleUnit> _scriptValues;

	/// Calculate stat improvement.
	int improveStat(int exp) const;
	/// Helper function initializing recolor vector.
	void setRecolor(int basicLook, int utileLook, int rankLook);
	/// Helper function preparing Time Units recovery at beginning of turn.
	void prepareTimeUnits(int tu);
	/// Helper function preparing Energy recovery at beginning of turn.
	void prepareEnergy(int energy);
	/// Helper function preparing Health recovery at beginning of turn.
	void prepareHealth(int helath);
	/// Helper function preparing Mana recovery at beginning of turn.
	void prepareMana(int mana);
	/// Helper function preparing Stun recovery at beginning of turn.
	void prepareStun(int strun);
	/// Helper function preparing Morale recovery at beginning of turn.
	void prepareMorale(int morale);
	/// Helper function preparing unit sounds.
	void prepareUnitSounds();
	/// Helper function preparing unit response sounds.
	void prepareUnitResponseSounds(const Mod *mod);
	/// Applies percentual and/or flat adjustments to the use costs.
	void applyPercentages(RuleItemUseCost &cost, const RuleItemUseCost &flat) const;
public:
	static const int MAX_SOLDIER_ID = 1000000;
	static const int BUBBLES_FIRST_FRAME = 3;
	static const int BUBBLES_LAST_FRAME = BUBBLES_FIRST_FRAME + 15;

	/// Name of class used in script.
	static constexpr const char *ScriptName = "BattleUnit";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);
	/// Init all required data in script using object data.
	static void ScriptFill(ScriptWorkerBlit* w, const BattleUnit* item, const SavedBattleGame* save, int body_part, int anim_frame, int shade, int burn);

	/// Creates a BattleUnit from solder.
	BattleUnit(const Mod *mod, Soldier *soldier, int depth);
	/// Creates a BattleUnit from unit.
	BattleUnit(const Mod *mod, Unit *unit, UnitFaction faction, int id, const RuleEnviroEffects* enviro, Armor *armor, StatAdjustment *adjustment, int depth);
	/// Updates BattleUnit's armor and related attributes (after a change/transformation of armor).
	void updateArmorFromSoldier(const Mod *mod, Soldier *soldier, Armor *ruleArmor, int depth, bool inBattlescape);
	/// Updates BattleUnit's armor and related attributes (after a change/transformation of armor).
	void updateArmorFromNonSoldier(const Mod* mod, Armor* newArmor, int depth);
	/// Cleans up the BattleUnit.
	~BattleUnit();
	/// Loads the unit from YAML.
	void load(const YAML::Node &node, const Mod *mod, const ScriptGlobal *shared);
	/// Saves the unit to YAML.
	YAML::Node save(const ScriptGlobal *shared) const;
	/// Gets the BattleUnit's ID.
	int getId() const;
	/// Calculates the distance squared between the unit and a given position.
	int distance3dToPositionSq(const Position& pos) const;
	/// Calculates the distance squared between the unit and a given other unit.
	int distance3dToUnitSq(BattleUnit* otherUnit) const;
	/// Sets the unit's position
	void setPosition(Position pos, bool updateLastPos = true);
	/// Gets the unit's position.
	Position getPosition() const;
	/// Gets the unit's position.
	Position getLastPosition() const;
	/// Gets the unit's position of center in voxels.
	Position getPositionVexels() const;
	/// Sets the unit's direction 0-7.
	void setDirection(int direction);
	/// Sets the unit's face direction (only used by strafing moves)
	void setFaceDirection(int direction);
	/// Gets the unit's direction.
	int getDirection() const;
	/// Gets the unit's face direction (only used by strafing moves)
	int getFaceDirection() const;
	/// Gets the unit's turret direction.
	int getTurretDirection() const;
	/// Gets the unit's turret To direction.
	int getTurretToDirection() const;
	/// Gets the unit's vertical direction.
	int getVerticalDirection() const;
	/// Gets the unit's status.
	UnitStatus getStatus() const;
	/// Does the unit want to surrender?
	bool wantsToSurrender() const;
	/// Is the unit surrendering this turn?
	bool isSurrendering() const;
	/// Mark the unit as surrendering this turn.
	void setSurrendering(bool isSurrendering);
	/// Start the walkingPhase
	void startWalking(int direction, Position destination, SavedBattleGame *savedBattleGame);
	/// Increase the walkingPhase
	void keepWalking(SavedBattleGame *savedBattleGame, bool fullWalkCycle);
	/// Gets the walking phase for animation and sound
	int getWalkingPhase() const;
	/// Gets the walking phase for diagonal walking
	int getDiagonalWalkingPhase() const;
	/// Gets the unit's destination when walking
	Position getDestination() const;
	/// Look at a certain point.
	void lookAt(Position point, bool turret = false);
	/// Look at a certain direction.
	void lookAt(int direction, bool force = false);
	/// Turn to the destination direction.
	void turn(bool turret = false);
	/// Abort turning.
	void abortTurn();
	/// Gets the soldier's gender.
	SoldierGender getGender() const;
	/// Gets the unit's faction.
	UnitFaction getFaction() const;
	/// Gets unit sprite recolors values.
	const std::vector<std::pair<Uint8, Uint8> > &getRecolor() const;
	/// Kneel down.
	void kneel(bool kneeled);
	/// Is kneeled?
	bool isKneeled() const;
	/// Is floating?
	bool isFloating() const;
	/// Have unit floor below?
	bool haveNoFloorBelow() const { return _haveNoFloorBelow; }

	/// Aim.
	void aim(bool aiming);
	/// Get direction to a certain point
	int directionTo(Position point) const;
	/// Gets the unit's time units.
	int getTimeUnits() const;
	/// Gets the unit's stamina.
	int getEnergy() const;
	/// Gets the unit's health.
	int getHealth() const;
	/// Gets the unit's mana.
	int getMana() const;
	/// Gets the unit's bravery.
	int getMorale() const;
	/// Get overkill damage to unit.
	int getOverKillDamage() const;
	/// Do damage to the unit.
	int damage(Position relative, int power, const RuleDamageType *type, SavedBattleGame *save, BattleActionAttack attack, UnitSide sideOverride = SIDE_MAX, UnitBodyPart bodypartOverride = BODYPART_MAX);
	/// Heal stun level of the unit.
	void healStun(int power);
	/// Gets the unit's stun level.
	int getStunlevel() const;
	/// Is the unit losing HP (due to negative health regeneration)?
	bool hasNegativeHealthRegen() const;
	/// Knocks the unit out instantly.
	void knockOut(BattlescapeGame *battle);

	/// Start falling sequence.
	void startFalling();
	/// Increment the falling sequence.
	void keepFalling();
	/// Set final falling state.
	void instaFalling();
	/// Get falling sequence.
	int getFallingPhase() const;

	/// The unit is out - either dead or unconscious.
	bool isOut() const;
	/// Check if unit stats will cause knock out.
	bool isOutThresholdExceed() const;
	/// Unit is removed from game.
	bool isIgnored() const;

	/// Get the number of time units a certain action takes.
	RuleItemUseCost getActionTUs(BattleActionType actionType, const BattleItem *item) const;
	/// Get the number of time units a certain action takes.
	RuleItemUseCost getActionTUs(BattleActionType actionType, const RuleItem *item) const;
	/// Get the number of time units a certain skill action takes.
	RuleItemUseCost getActionTUs(BattleActionType actionType, const RuleSkill *skillRules) const;
	/// Spend time units if it can.
	bool spendTimeUnits(int tu);
	/// Spend energy if it can.
	bool spendEnergy(int energy);
	/// Force to spend resources cost without checking.
	void spendCost(const RuleItemUseCost& cost);
	/// Clear time units.
	void clearTimeUnits();
	/// Reset time units and energy.
	void resetTimeUnitsAndEnergy();
	/// Add unit to visible units.
	bool addToVisibleUnits(BattleUnit *unit);
	/// Remove a unit from the list of visible units.
	bool removeFromVisibleUnits(BattleUnit *unit);
	/// Is the given unit among this unit's visible units?
	bool hasVisibleUnit(BattleUnit *unit);
	/// Get the list of visible units.
	std::vector<BattleUnit*> *getVisibleUnits();
	/// Clear visible units.
	void clearVisibleUnits();
	/// Add unit to visible tiles.
	bool addToVisibleTiles(Tile *tile);
	/// Has this unit marked this tile as within its view?
	bool hasVisibleTile(Tile *tile) const
	{
		return _visibleTilesLookup.find(tile) != _visibleTilesLookup.end(); //find?
	}
	/// Get the list of visible tiles.
	const std::vector<Tile*> *getVisibleTiles();
	/// Clear visible tiles.
	void clearVisibleTiles();
	/// Calculate psi attack accuracy.
	static int getPsiAccuracy(BattleActionAttack::ReadOnly attack);
	/// Calculate firing accuracy.
	static int getFiringAccuracy(BattleActionAttack::ReadOnly attack, Mod *mod);
	/// Calculate accuracy modifier.
	int getAccuracyModifier(const BattleItem *item = 0) const;
	/// Get the current reaction score.
	double getReactionScore() const;
	/// Prepare for a new turn.
	void prepareNewTurn(bool fullProcess = true);
	/// Calculate change in unit stats.
	void updateUnitStats(bool tuAndEnergy, bool rest);
	/// Morale change
	void moraleChange(int change);
	/// Calculate value of morale change based on bravery.
	int reduceByBravery(int moraleChange) const;
	/// Calculate power reduction by resistances.
	int reduceByResistance(int power, ItemDamageType resistType) const;
	/// Don't reselect this unit
	void dontReselect();
	/// Reselect this unit
	void allowReselect();
	/// Check whether reselecting this unit is allowed.
	bool reselectAllowed() const;
	/// Set fire.
	void setFire(int fire);
	/// Get fire.
	int getFire() const;

	/// Get the list of items in the inventory.
	std::vector<BattleItem*> *getInventory();
	/// Fit item into inventory slot.
	bool fitItemToInventory(RuleInventory *slot, BattleItem *item);
	/// Add item to unit.
	bool addItem(BattleItem *item, const Mod *mod, bool allowSecondClip = false, bool allowAutoLoadout = false, bool allowUnloadedWeapons = false);

	/// Let AI do their thing.
	void think(BattleAction *action);
	/// Get AI Module.
	AIModule *getAIModule() const;
	/// Set AI Module.
	void setAIModule(AIModule *ai);
	/// Set whether this unit is visible
	void setVisible(bool flag);
	/// Get whether this unit is visible
	bool getVisible() const;

	/// Check if unit can fall down.
	void updateTileFloorState(SavedBattleGame *saveBattleGame);
	/// Sets the unit's tile it's standing on
	void setTile(Tile *tile, SavedBattleGame *saveBattleGame = 0);
	/// Set only unit tile without any additional logic.
	void setInventoryTile(Tile *tile);
	/// Gets the unit's tile.
	Tile *getTile() const;

	/// Gets the item in the specified slot.
	BattleItem *getItem(RuleInventory *slot, int x = 0, int y = 0) const;
	/// Gets the item in the main hand.
	BattleItem *getMainHandWeapon(bool quickest = true) const;
	/// Gets a grenade from the belt, if any.
	BattleItem *getGrenadeFromBelt() const;
	/// Gets the item from right hand.
	BattleItem *getRightHandWeapon() const;
	/// Gets the item from left hand.
	BattleItem *getLeftHandWeapon() const;
	/// Set the right hand as main active hand.
	void setActiveRightHand();
	/// Set the left hand as main active hand.
	void setActiveLeftHand();
	/// Choose what weapon was last use by unit.
	const BattleItem *getActiveHand(const BattleItem *left, const BattleItem *right) const;
	/// Reloads a weapon if needed.
	bool reloadAmmo();

	/// Toggle the right hand as main hand for reactions.
	void toggleRightHandForReactions();
	/// Toggle the left hand as main hand for reactions.
	void toggleLeftHandForReactions();
	/// Is right hand preferred for reactions?
	bool isRightHandPreferredForReactions() const;
	/// Is left hand preferred for reactions?
	bool isLeftHandPreferredForReactions() const;
	/// Get preferred weapon for reactions, if applicable.
	BattleItem *getWeaponForReactions(bool meleeOnly) const;

	/// Check if this unit is in the exit area
	bool isInExitArea(SpecialTileType stt) const;
	bool liesInExitArea(Tile *tile, SpecialTileType stt) const;
	/// Gets the unit height taking into account kneeling/standing.
	int getHeight() const;
	/// Gets the unit floating elevation.
	int getFloatHeight() const;
	/// Adds one to the bravery exp counter.
	void addBraveryExp();
	/// Adds one to the reaction exp counter.
	void addReactionExp();
	/// Adds one to the firing exp counter.
	void addFiringExp();
	/// Adds one to the throwing exp counter.
	void addThrowingExp();
	/// Adds one to the psi skill exp counter.
	void addPsiSkillExp();
	/// Adds one to the psi strength exp counter.
	void addPsiStrengthExp();
	/// Adds to the mana exp counter.
	void addManaExp(int weaponStat);
	/// Adds one to the melee exp counter.
	void addMeleeExp();
	/// Did the unit gain any experience yet?
	bool hasGainedAnyExperience();
	/// Updates the stats of a Geoscape soldier.
	void updateGeoscapeStats(Soldier *soldier) const;
	/// Check if unit eligible for squaddie promotion.
	bool postMissionProcedures(const Mod *mod, SavedGame *geoscape, SavedBattleGame *battle, StatAdjustment &statsDiff);
	/// Get the sprite index for the minimap
	int getMiniMapSpriteIndex() const;
	/// Set the turret type. -1 is no turret.
	void setTurretType(int turretType);
	/// Get the turret type. -1 is no turret.
	int getTurretType() const;

	/// Set armor value.
	void setArmor(int armor, UnitSide side);
	/// Get armor value.
	int getArmor(UnitSide side) const;
	/// Get max armor value.
	int getMaxArmor(UnitSide side) const;
	/// Set fatal wound amount of a body part
	void setFatalWound(int wound, UnitBodyPart part);
	/// Get total number of fatal wounds.
	int getFatalWounds() const;
	/// Get fatal wound amount of a body part
	int getFatalWound(UnitBodyPart part) const;

	/// Heal one fatal wound
	void heal(UnitBodyPart part, int woundAmount, int healthAmount);
	/// Give pain killers to this unit
	void painKillers(int moraleAmount, float painKillersStrength);
	/// Give stimulant to this unit
	void stimulant (int energy, int stun, int mana);
	/// Get motion points for the motion scanner.
	int getMotionPoints() const;
	/// Get turn when unit was scanned by the motion scanner.
	int getScannedTurn() const { return _scannedTurn; }
	/// Set turn when unit was scanned by the motion scanner.
	void setScannedTurn(int turn) { _scannedTurn = turn; }
	/// Gets the unit's armor.
	const Armor *getArmor() const;
	/// Sets the unit's name.
	void setName(const std::string &name);
	/// Gets the unit's name.
	std::string getName(Language *lang, bool debugAppendId = false) const;
	/// Gets the unit's stats.
	UnitStats *getBaseStats();
	/// Gets the unit's stats.
	const UnitStats *getBaseStats() const;
	/// Get the unit's stand height.
	int getStandHeight() const;
	/// Get the unit's kneel height.
	int getKneelHeight() const;
	/// Get the unit's loft ID.
	int getLoftemps(int entry = 0) const;
	/// Get the unit's value.
	int getValue() const;
	/// Get the reload sound (of the last reloaded weapon).
	int getReloadSound() const { return _lastReloadSound; }
	/// Get the unit's death sounds.
	const std::vector<int> &getDeathSounds() const;
	/// Gets the unit's "select unit" sounds.
	const std::vector<int> &getSelectUnitSounds() const { return _selectUnitSound; }
	/// Gets the unit's "start moving" sounds.
	const std::vector<int> &getStartMovingSounds() const { return _startMovingSound; }
	/// Gets the unit's "select weapon" sounds.
	const std::vector<int> &getSelectWeaponSounds() const { return _selectWeaponSound; }
	/// Gets the unit's "annoyed" sounds.
	const std::vector<int> &getAnnoyedSounds() const { return _annoyedSound; }
	/// Get the unit's move sound.
	int getMoveSound() const;
	/// Get whether the unit is affected by fatal wounds.
	bool isWoundable() const;
	/// Get whether the unit is affected by fear.
	bool isFearable() const;
	/// Get the unit's intelligence.
	int getIntelligence() const;
	/// Get the unit's aggression.
	int getAggression() const;
	/// Helper method.
	int getMaxViewDistance(int baseVisibility, int nerf, int buff) const;
	/// Get maximum view distance at dark.
	int getMaxViewDistanceAtDark(const Armor *otherUnitArmor) const;
	int getMaxViewDistanceAtDarkSquared() const;
	/// Get maximum view distance at day.
	int getMaxViewDistanceAtDay(const Armor *otherUnitArmor) const;
	/// Get the units's special ability.
	int getSpecialAbility() const;

	/// Gets the unit's spawn unit.
	const Unit *getSpawnUnit() const;
	/// Sets the unit's spawn unit.
	void setSpawnUnit(const Unit *spawnUnit);
	/// Gets the faction of spawned unit.
	UnitFaction getSpawnUnitFaction() const { return _spawnUnitFaction; }
	/// Set the faction of spawned unit.
	void setSpawnUnitFaction(UnitFaction f) { _spawnUnitFaction = f; }
	/// Set the units's respawn flag.
	void setRespawn(bool respawn);
	/// Get the units's respawn flag.
	bool getRespawn() const;
	/// Set the units's alreadyRespawned flag.
	void setAlreadyRespawned(bool alreadyRespawned);
	/// Get the units's alreadyRespawned flag.
	bool getAlreadyRespawned() const;
	/// Remove all spawn unit info.
	void clearSpawnUnit();

	/// Get the units's rank string.
	const std::string& getRankString() const;
	/// Get the geoscape-soldier object.
	Soldier *getGeoscapeSoldier() const;
	/// Add a kill to the counter.
	void addKillCount();
	/// Get unit type.
	const std::string& getType() const;
	/// Convert's unit to a faction
	void convertToFaction(UnitFaction f);
	/// Set health to 0
	void kill();
	/// Set health to 0 and set status dead
	void instaKill();

	/// Gets whether the unit has any aggro sounds.
	bool hasAggroSound() const;
	/// Gets a unit's random aggro sound.
	int getRandomAggroSound() const;
	/// Sets the unit's time units.
	void setTimeUnits(int tu);
	/// Get the faction that killed this unit.
	UnitFaction killedBy() const;
	/// Set the faction that killed this unit.
	void killedBy(UnitFaction f);
	/// Set the units we are charging towards.
	void setCharging(BattleUnit *chargeTarget);
	/// Get the units we are charging towards.
	BattleUnit *getCharging();
	/// Get the carried weight in strength units.
	int getCarriedWeight(BattleItem *draggingItem = 0) const;

	/// Set how many turns this unit will be exposed for.
	void setTurnsSinceSpotted (int turns);
	/// Set how many turns this unit will be exposed for.
	int getTurnsSinceSpotted() const;
	/// Set how many turns left snipers know about this target.
	void setTurnsLeftSpottedForSnipers (int turns);
	/// Get how many turns left snipers know about this target.
	int  getTurnsLeftSpottedForSnipers() const;
	/// Reset how many turns passed since stunned last time.
	void resetTurnsSinceStunned() { _turnsSinceStunned = 255; }
	/// Increase how many turns passed since stunned last time.
	void incTurnsSinceStunned() { _turnsSinceStunned = std::min(255, _turnsSinceStunned + 1); }
	/// Return how many turns passed since stunned last time.
	int getTurnsSinceStunned() const { return _turnsSinceStunned; }

	/// Get this unit's original faction
	UnitFaction getOriginalFaction() const;
	/// Get alien/HWP unit.
	Unit *getUnitRules() const { return _unitRules; }
	Position lastCover;
	/// get the vector of units we've seen this turn.
	std::vector<BattleUnit *> &getUnitsSpottedThisTurn();
	/// get the vector of units we've seen this turn.
	const std::vector<BattleUnit *> &getUnitsSpottedThisTurn() const;
	/// set the rank integer
	void setRankInt(int rank);
	/// get the rank integer
	int getRankInt() const;
	/// derive a rank integer based on rank string (for xcom soldiers ONLY)
	void deriveRank();
	/// this function checks if a tile is visible, using maths.
	bool checkViewSector(Position pos, bool useTurretDirection = false) const;
	/// adjust this unit's stats according to difficulty.
	void adjustStats(const StatAdjustment &adjustment);
	/// did this unit already take fire damage this turn? (used to avoid damaging large units multiple times.)
	bool tookFireDamage() const;
	/// switch the state of the fire damage tracker.
	void toggleFireDamage();
	/// Is this unit selectable?
	bool isSelectable(UnitFaction faction, bool checkReselect, bool checkInventory) const;
	/// Does this unit have an inventory?
	bool hasInventory() const;

	/// Is this unit breathing and if so what frame?
	int getBreathExhaleFrame() const;
	/// Count frames to next start of breath animation.
	int getBreathInhaleFrame() const;
	/// Start breathing and/or update the breathing frame.
	void breathe();

	/// Set the flag for "floor above me" meaning stop rendering bubbles.
	void setFloorAbove(bool floor);
	/// Get the flag for "floor above me".
	bool getFloorAbove() const;
	/// Get any utility weapon we may be carrying, or a built in one.
	BattleItem *getUtilityWeapon(BattleType type);
	/// Set fire damage from environment.
	void setEnviFire(int damage);
	/// Set smoke damage from environment.
	void setEnviSmoke(int damage);
	/// Calculate smoke and fire damage from environment.
	void calculateEnviDamage(Mod *mod, SavedBattleGame *save);
	/// Use this function to check the unit's movement type.
	MovementType getMovementType() const;
	/// Gets the turn cost.
	int getTurnCost() const;
	/// Gets cost of standing up from kneeling.
	int getKneelUpCost() const { return 8; }
	/// Gets cost of kneel down.
	int getKneelDownCost() const { return 4; }
	/// Gets cost of current transiton form kneeling to standing or reverse.
	int getKneelChangeCost() const { return isKneeled() ? getKneelUpCost() : getKneelDownCost(); }

	/// Create special weapon for unit.
	void setSpecialWeapon(SavedBattleGame *save, bool updateFromSave);
	/// Add/assign a special weapon loaded from a save.
	void addLoadedSpecialWeapon(BattleItem* item);
	/// Remove all special weapons.
	void removeSpecialWeapons(SavedBattleGame *save);
	/// Get special weapon by battle type.
	BattleItem *getSpecialWeapon(BattleType type) const;
	/// Get special weapon by name.
	BattleItem *getSpecialWeapon(const RuleItem *weaponRule) const;
	/// Gets special weapon that uses an icon, if any.
	BattleItem *getSpecialIconWeapon(BattleType &type) const;

	/// Checks if this unit is in hiding for a turn.
	bool isHiding() const {return _hidingForTurn; };
	/// Sets this unit is in hiding for a turn (or not).
	void setHiding(bool hiding) { _hidingForTurn = hiding; };
	/// Puts the unit in the corner to think about what he's done.
	void goToTimeOut();
	/// Recovers the unit's time units and energy.
	void recoverTimeUnits();
	/// Get the unit's mission statistics.
	BattleUnitStatistics* getStatistics();
	/// Set the unit murderer's id.
	void setMurdererId(int id);
	/// Get the unit murderer's id.
	int getMurdererId() const;
	/// Set information on the unit's fatal shot.
	void setFatalShotInfo(UnitSide side, UnitBodyPart bodypart);
	/// Get information on the unit's fatal shot's side.
	UnitSide getFatalShotSide() const;
	/// Get information on the unit's fatal shot's body part.
	UnitBodyPart getFatalShotBodyPart() const;
	/// Get the unit murderer's weapon.
	std::string getMurdererWeapon() const;
	/// Set the unit murderer's weapon.
	void setMurdererWeapon(const std::string& weapon);
	/// Get the unit murderer's weapon's ammo.
	std::string getMurdererWeaponAmmo() const;
	/// Set the unit murderer's weapon's ammo.
	void setMurdererWeaponAmmo(const std::string& weaponAmmo);
	/// Set the unit mind controller's id.
	void setMindControllerId(int id);
	/// Get the unit mind controller's id.
	int getMindControllerId() const;
	/// Get the unit leeroyJenkins flag
	bool isLeeroyJenkins() const { return _isLeeroyJenkins; };
	/// Gets the spotter score. This is the number of turns sniper AI units can use spotting info from this unit.
	int getSpotterDuration() const;
	/// Is this unit capable of shooting beyond max. visual range?
	bool isSniper() const;
	/// Remembers the unit's XP (used for shotguns).
	void rememberXP();
	/// Artificially alter a unit's XP (used for shotguns).
	void nerfXP();
	/// Was this unit just hit?
	bool getHitState();
	/// reset the unit hit state.
	void resetHitState();
	/// Was this unit melee attacked by a given attacker this turn (both hit and miss count)?
	bool wasMeleeAttackedBy(int attackerId) const;
	/// Set the "melee attacked by" flag.
	void setMeleeAttackedBy(int attackerId);
	/// Did this unit explode already?
	bool hasAlreadyExploded() const { return _alreadyExploded; }
	/// Set the already exploded flag.
	void setAlreadyExploded(bool alreadyExploded) { _alreadyExploded = alreadyExploded; }
	/// Gets whether this unit can be captured alive (applies to aliens).
	bool getCapturable() const;
	/// free up the patrol node target, to allow others to use it.
	void freePatrolTarget();
	/// Marks this unit as summoned by an item and therefore won't count for recovery or total player units left.
	void setSummonedPlayerUnit(bool summonedPlayerUnit);
	/// Was this unit summoned by an item?
	bool isSummonedPlayerUnit() const;
	/// Should this unit (player, alien or civilian) be ignored for various things related to soldier diaries and commendations?
	bool isCosmetic() const;
	/// Should this AI unit (alien or civilian) be ignored by other AI units?
	bool isIgnoredByAI() const;
	/// Marks this unit as resummoned fake civilian and therefore won't count for civilian scoring in the Debriefing.
	void markAsResummonedFakeCivilian() { _resummonedFakeCivilian = true; _status = STATUS_IGNORE_ME; }
	/// Is this unit a resummoned fake civilian?
	bool isResummonedFakeCivilian() const { return _resummonedFakeCivilian; }
	/// Marks this unit as VIP.
	void markAsVIP() { _vip = true; }
	/// Is this a VIP unit?
	bool isVIP() const { return _vip; }
	/// Is the unit eagerly picking up weapons?
	bool getPickUpWeaponsMoreActively() const { return _pickUpWeaponsMoreActively; }
	/// Show indicators for this unit or not?
	bool indicatorsAreEnabled() const { return !_disableIndicators; }
	/// Disable showing indicators for this unit.
	void disableIndicators();
};

} //namespace OpenXcom

