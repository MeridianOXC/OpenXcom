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
#include <yaml-cpp/yaml.h>
#include "Tile.h"
#include "BattleUnit.h"
#include "../Mod/AlienDeployment.h"
#include "HitLog.h"

namespace OpenXcom
{

class Tile;
class SavedGame;
class MapDataSet;
class Node;
class BattlescapeState;
class Position;
class Pathfinding;
class TileEngine;
class RuleEnviroEffects;
class BattleItem;
class Mod;
class State;
class ItemContainer;
class RuleItem;

/**
 * The battlescape data that gets written to disk when the game is saved.
 * A saved game holds all the variable info in a game like mapdata,
 * soldiers, items, etc.
 */
class SavedBattleGame
{
public:
	/// Name of class used in script.
	static constexpr const char *ScriptName = "BattleGame";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

private:
	BattlescapeState *_battleState;
	Mod *_rule;
	int _mapsize_x, _mapsize_y, _mapsize_z;
	std::vector<MapDataSet*> _mapDataSets;
	std::vector<Tile> _tiles;
	BattleUnit *_selectedUnit, *_lastSelectedUnit;
	std::vector<Node*> _nodes;
	std::vector<BattleUnit*> _units;
	std::vector<BattleItem*> _items, _deleted;
	Pathfinding *_pathfinding;
	TileEngine *_tileEngine;
	std::string _missionType, _strTarget, _strCraftOrBase, _alienCustomDeploy, _alienCustomMission;
	const RuleEnviroEffects *_enviroEffects;
	bool _ecEnabledFriendly, _ecEnabledHostile, _ecEnabledNeutral;
	int _globalShade;
	UnitFaction _side;
	int _turn, _bughuntMinTurn;
	int _animFrame;
	bool _debugMode, _bughuntMode;
	bool _aborted;
	bool _baseCraftInventory = false;
	int _itemId;
	int _objectiveType, _objectivesDestroyed, _objectivesNeeded;
	std::vector<BattleUnit*> _exposedUnits;
	std::list<BattleUnit*> _fallingUnits;
	bool _unitsFalling, _cheating;
	std::vector<Position> _tileSearch, _storageSpace;
	BattleActionType _tuReserved;
	bool _kneelReserved;
	std::vector< std::vector<std::pair<int, int> > > _baseModules;
	ItemContainer *_baseItems;
	int _depth, _ambience;
	double _ambientVolume;
	std::vector<int> _ambienceRandom;
	int _minAmbienceRandomDelay, _maxAmbienceRandomDelay, _currentAmbienceDelay;
	std::vector<BattleItem*> _recoverGuaranteed, _recoverConditional;
	std::string _music;
	int _turnLimit, _cheatTurn;
	ChronoTrigger _chronoTrigger;
	bool _beforeGame;
	std::string _hiddenMovementBackground;
	HitLog *_hitLog;
	ScriptValues<SavedBattleGame> _scriptValues;
	/// Selects a soldier.
	BattleUnit *selectPlayerUnit(int dir, bool checkReselect = false, bool setReselect = false, bool checkInventory = false);

public:
	/// Creates a new battle save, based on the current generic save.
	SavedBattleGame(Mod *rule, Language *lang);
	/// Cleans up the saved game.
	~SavedBattleGame();
	/// Loads a saved battle game from YAML.
	void load(const YAML::Node& node, Mod *mod, SavedGame* savedGame);
	/// Saves a saved battle game to YAML.
	YAML::Node save() const;
	/// Sets the dimensions of the map and initializes it.
	void initMap(int mapsize_x, int mapsize_y, int mapsize_z, bool resetTerrain = true);
	/// Initialises the pathfinding and tile engine.
	void initUtilities(Mod *mod, bool craftInventory = false);
	/// Gets if this is craft pre-equip phase in base view.
	bool isBaseCraftInventory();
	/// Gets the game's mapdata files.
	std::vector<MapDataSet*> *getMapDataSets();
	/// Sets the mission type.
	void setMissionType(const std::string &missionType);
	/// Gets the mission type.
	const std::string &getMissionType() const;
	/// Sets the mission target.
	void setMissionTarget(const std::string& missionTarget) { _strTarget = missionTarget; }
	/// Gets the mission target.
	const std::string& getMissionTarget() const { return _strTarget; }
	/// Sets the mission craft/base.
	void setMissionCraftOrBase(const std::string& missionCraftOrBase) { _strCraftOrBase = missionCraftOrBase; }
	/// Gets the mission craft/base.
	const std::string& getMissionCraftOrBase() const { return _strCraftOrBase; }
	/// Gets the base's items BEFORE the mission.
	ItemContainer *getBaseStorageItems();
	/// Applies the enviro effects.
	void applyEnviroEffects(const RuleEnviroEffects* enviroEffects);
	/// Gets the enviro effects.
	const RuleEnviroEffects* getEnviroEffects() const;
	/// Are environmental conditions (for a given faction) enabled?
	bool getEnvironmentalConditionsEnabled(UnitFaction faction) const;
	/// Sets the custom alien data.
	void setAlienCustom(const std::string &deploy, const std::string &mission);
	/// Gets the custom alien deploy.
	const std::string &getAlienCustomDeploy() const;
	/// Gets the custom mission definition.
	const std::string &getAlienCustomMission() const;
	/// Sets the global shade.
	void setGlobalShade(int shade);
	/// Gets the global shade.
	int getGlobalShade() const;
	/// Gets a pointer to the list of nodes.
	std::vector<Node*> *getNodes();
	/// Gets a pointer to the list of items.
	std::vector<BattleItem*> *getItems();
	/// Gets a pointer to the list of units.
	std::vector<BattleUnit*> *getUnits();
	/// Gets terrain size x.
	int getMapSizeX() const;
	/// Gets terrain size y.
	int getMapSizeY() const;
	/// Gets terrain size z.
	int getMapSizeZ() const;
	/// Gets terrain x*y*z
	int getMapSizeXYZ() const;

	/**
	 * Converts coordinates into a unique index.
	 * getTile() calls this every time, so should be inlined along with it.
	 * @param pos The position to convert.
	 * @return A unique index.
	 */
	inline int getTileIndex(Position pos) const
	{
		return pos.z * _mapsize_y * _mapsize_x + pos.y * _mapsize_x + pos.x;
	}

	/// Converts a tile index to its coordinates.
	Position getTileCoords(int index) const;

	/**
	 * Gets the Tile at a given position on the map.
	 * This method is called over 50mil+ times per turn so it seems useful
	 * to inline it.
	 * @param pos Map position.
	 * @return Pointer to the tile at that position.
	 */
	inline Tile *getTile(Position pos)
	{
		if (pos.x < 0 || pos.y < 0 || pos.z < 0
			|| pos.x >= _mapsize_x || pos.y >= _mapsize_y || pos.z >= _mapsize_z)
			return 0;

		return &_tiles[getTileIndex(pos)];
	}

	/*
	 * Gets a pointer to the tiles, a tile is the smallest component of battlescape.
	 * @param pos Index position, less than `getMapSizeXYZ()`.
	 * @return Pointer to the tile at that index.
	 */
	Tile* getTile(int i)
	{
		return &_tiles[i];
	}

	/**
	 * Get tile that is below current one (const version).
	 * @param tile
	 * @return Tile below
	 */
	inline const Tile *getBelowTile(const Tile* tile) const
	{
		if (!tile || tile->getPosition().z == 0)
		{
			return nullptr;
		}
		// difference of pointers between layers is equal `_mapsize_y * _mapsize_x`
		// when we subtract this value from valid tile we get valid tile from lower layer.
		return tile - _mapsize_y * _mapsize_x;
	}

	/**
	 * Get tile that is below current one.
	 * @param tile
	 * @return Tile below
	 */
	inline Tile *getBelowTile(Tile* tile)
	{
		if (!tile || tile->getPosition().z == 0)
		{
			return nullptr;
		}
		// difference of pointers between layers is equal `_mapsize_y * _mapsize_x`
		// when we subtract this value from valid tile we get valid tile from lower layer.
		return tile - _mapsize_y * _mapsize_x;
	}

	/**
	 * Get tile that is over current one (const version).
	 * @param tile
	 * @return Tile over
	 */
	inline const Tile *getAboveTile(const Tile* tile) const
	{
		if (!tile || tile->getPosition().z == _mapsize_z - 1)
		{
			return nullptr;
		}
		// difference of pointers between layers is equal `_mapsize_y * _mapsize_x`
		// when we add this value from valid tile we get valid tile from upper layer.
		return tile + _mapsize_y * _mapsize_x;
	}

	/**
	 * Get tile that is over current one.
	 * @param tile
	 * @return Tile over
	 */
	inline Tile *getAboveTile(Tile* tile)
	{
		if (!tile || tile->getPosition().z == _mapsize_z - 1)
		{
			return nullptr;
		}
		// difference of pointers between layers is equal `_mapsize_y * _mapsize_x`
		// when we add this value from valid tile we get valid tile from upper layer.
		return tile + _mapsize_y * _mapsize_x;
	}

	/// Gets the currently selected unit.
	BattleUnit *getSelectedUnit() const;
	/// Sets the currently selected unit.
	void setSelectedUnit(BattleUnit *unit);
	/// Selects the previous soldier.
	BattleUnit *selectPreviousPlayerUnit(bool checkReselect = false, bool setReselect = false, bool checkInventory = false);
	/// Selects the next soldier.
	BattleUnit *selectNextPlayerUnit(bool checkReselect = false, bool setReselect = false, bool checkInventory = false);
	/// Selects the unit with position on map.
	BattleUnit *selectUnit(Position pos);
	/// Gets the pathfinding object.
	Pathfinding *getPathfinding() const;
	/// Gets a pointer to the tile engine.
	TileEngine *getTileEngine() const;
	/// Gets the playing side.
	UnitFaction getSide() const;
	/// Can unit use that weapon?
	bool canUseWeapon(const BattleItem *weapon, const BattleUnit *unit, bool isBerserking) const;
	/// Gets the turn number.
	int getTurn() const;
	/// Sets the bug hunt turn number.
	void setBughuntMinTurn(int bughuntMinTurn);
	/// Gets the bug hunt turn number.
	int getBughuntMinTurn() const;
	/// Start first turn of battle.
	void startFirstTurn();
	/// Ends the turn.
	void endTurn();
	/// Gets animation frame.
	int getAnimFrame() const;
	/// Increase animation frame.
	void nextAnimFrame();
	/// Sets debug mode.
	void setDebugMode();
	/// Gets debug mode.
	bool getDebugMode() const;
	/// Sets bug hunt mode.
	void setBughuntMode(bool bughuntMode);
	/// Gets bug hunt mode.
	bool getBughuntMode() const;
	/// Load map resources.
	void loadMapResources(Mod *mod);
	/// Resets tiles units are standing on
	void resetUnitTiles();
	/// Add item to delete list.
	void deleteList(BattleItem *item);
	/// Removes an item from the game.
	void removeItem(BattleItem *item);
	/// Add buildIn weapon form list to unit.
	void addFixedItems(BattleUnit *unit, const std::vector<std::string> &fixed);
	/// Init new created unit.
	void initUnit(BattleUnit *unit, size_t itemLevel = 0);
	/// Init new created item.
	void initItem(BattleItem *item);
	/// Create new item for unit.
	BattleItem *createItemForUnit(const RuleItem *rule, BattleUnit *unit, bool fixedWeapon = false);
	/// Create new item for unit.
	BattleItem *createItemForUnit(const std::string& type, BattleUnit *unit, bool fixedWeapon = false);
	/// Create new built-in item for unit.
	BattleItem *createItemForUnitBuildin(RuleItem *rule, BattleUnit *unit);
	/// Create new item for tile.
	BattleItem *createItemForTile(RuleItem *rule, Tile *tile);
	/// Create new item for tile.
	BattleItem *createItemForTile(const std::string& type, Tile *tile);
	/// Sets whether the mission was aborted.
	void setAborted(bool flag);
	/// Checks if the mission was aborted.
	bool isAborted() const;
	/// Sets how many objectives need to be destroyed.
	void setObjectiveCount(int counter);
	/// increments the objective counter.
	void addDestroyedObjective();
	/// Checks if all the objectives are destroyed.
	bool allObjectivesDestroyed() const;
	/// Gets the current item ID.
	int *getCurrentItemId();
	/// Gets a spawn node.
	Node *getSpawnNode(int nodeRank, BattleUnit *unit);
	/// Gets a patrol node.
	Node *getPatrolNode(bool scout, BattleUnit *unit, Node *fromNode);
	/// Carries out new turn preparations.
	void prepareNewTurn();
	/// Revives unconscious units (health check).
	void reviveUnconsciousUnits(bool noTU = false);
	/// Removes the body item that corresponds to the unit.
	void removeUnconsciousBodyItem(BattleUnit *bu);
	/// Sets or tries to set a unit of a certain size on a certain position of the map.
	bool setUnitPosition(BattleUnit *bu, Position position, bool testOnly = false);
	/// Adds this unit to the vector of falling units.
	bool addFallingUnit(BattleUnit* unit);
	/// Gets the vector of falling units.
	std::list<BattleUnit*> *getFallingUnits();
	/// Toggles the switch that says "there are units falling, start the fall state".
	void setUnitsFalling(bool fall);
	/// Checks the status of the switch that says "there are units falling".
	bool getUnitsFalling() const;
	/// Gets a pointer to the BattlescapeState.
	BattlescapeState *getBattleState();
	/// Gets a pointer to the BattlescapeGame.
	BattlescapeGame *getBattleGame();
	/// Sets the pointer to the BattlescapeState.
	void setBattleState(BattlescapeState *bs);
	/// Gets the highest ranked, living XCom unit.
	BattleUnit* getHighestRankedXCom();
	/// Gets the morale modifier for the unit passed to this function.
	int getUnitMoraleModifier(BattleUnit* unit);
	/// Gets the morale loss modifier (by unit type) of the killed unit.
	int getMoraleLossModifierWhenKilled(BattleUnit* unit);
	/// Gets the morale modifier for Aliens based on they number or XCom based on the highest ranked soldier.
	int getFactionMoraleModifier(bool player);
	/// Checks whether a particular faction has eyes on *unit (whether any unit on that faction sees *unit).
	bool eyesOnTarget(UnitFaction faction, BattleUnit* unit);
	/// Attempts to place a unit on or near entryPoint.
	bool placeUnitNearPosition(BattleUnit *unit, const Position& entryPoint, bool largeFriend);
	/// Resets the turn counter.
	void resetTurnCounter();
	/// Resets the visibility of all tiles on the map.
	void resetTiles();
	/// get an 11x11 grid of positions (-10 to +10) to check.
	const std::vector<Position> &getTileSearch() const;
	/// check if the AI has engaged cheat mode.
	bool isCheating() const;
	/// get the reserved fire mode.
	BattleActionType getTUReserved() const;
	/// set the reserved fire mode.
	void setTUReserved(BattleActionType reserved);
	/// get whether we are reserving TUs to kneel.
	bool getKneelReserved() const;
	/// set whether we are reserving TUs to kneel.
	void setKneelReserved(bool reserved);
	/// give me access to the storage tiles vector.
	std::vector<Position> &getStorageSpace();
	/// move all the leftover items to random locations in the storage tiles vector.
	void randomizeItemLocations(Tile *t);
	/// get a reference to the baseModules map.
	std::vector< std::vector<std::pair<int, int> > > &getModuleMap();
	/// calculate the number of map modules remaining
	void calculateModuleMap();
	/// a shortcut to the geoscape save.
	SavedGame *getGeoscapeSave() const;
	/// get the depth of the battlescape game.
	int getDepth() const;
	/// set the depth of the battlescape game.
	void setDepth(int depth);
	/// uses the depth variable to set a palette.
	void setPaletteByDepth(State *state);
	/// sets the ambient sound effect;
	void setAmbientSound(int sound);
	/// gets the ambient sound effect;
	int getAmbientSound() const;
	/// Gets/sets the random ambient sound effects.
	const std::vector<int> &getAmbienceRandom() const { return _ambienceRandom; };
	void setAmbienceRandom(const std::vector<int> &ambienceRandom) { _ambienceRandom = ambienceRandom; }
	/// Gets/sets the minimum delay for the random ambient sound effect.
	int getMinAmbienceRandomDelay() const { return _minAmbienceRandomDelay; }
	void setMinAmbienceRandomDelay(int minAmbienceRandomDelay) { _minAmbienceRandomDelay = minAmbienceRandomDelay; }
	/// Gets/sets the maximum delay for the random ambient sound effect.
	int getMaxAmbienceRandomDelay() const { return _maxAmbienceRandomDelay; }
	void setMaxAmbienceRandomDelay(int maxAmbienceRandomDelay) { _maxAmbienceRandomDelay = maxAmbienceRandomDelay; }
	/// Gets/sets the current delay for the random ambient sound effect.
	int getCurrentAmbienceDelay() const { return _currentAmbienceDelay; }
	void setCurrentAmbienceDelay(int currentAmbienceDelay) { _currentAmbienceDelay = currentAmbienceDelay; }
	void decreaseCurrentAmbienceDelay() { _currentAmbienceDelay--; }
	/// Reset the current random ambient sound delay.
	void resetCurrentAmbienceDelay();
	/// Play a random ambient sound.
	void playRandomAmbientSound();
	// gets ruleset.
	const Mod *getMod() const;
	/// gets the list of items we're guaranteed.
	std::vector<BattleItem*> *getGuaranteedRecoveredItems();
	/// gets the list of items we MIGHT get.
	std::vector<BattleItem*> *getConditionalRecoveredItems();
	/// Get the name of the music track.
	const std::string &getMusic() const;
	/// Set the name of the music track.
	void setMusic(const std::string &track);
	/// Sets the objective type for this mission.
	void setObjectiveType(int type);
	/// Gets the objective type of this mission.
	SpecialTileType getObjectiveType() const;
	/// sets the ambient sound effect;
	void setAmbientVolume(double volume);
	/// gets the ambient sound effect;
	double getAmbientVolume() const;
	/// Gets the turn limit for this mission.
	int getTurnLimit() const;
	/// Gets the action that triggers when the timer runs out.
	ChronoTrigger getChronoTrigger() const;
	/// Sets the turn limit for this mission.
	void setTurnLimit(int limit);
	/// Sets the action that triggers when the timer runs out.
	void setChronoTrigger(ChronoTrigger trigger);
	/// Sets the turn to start the aliens cheating.
	void setCheatTurn(int turn);
	/// Check whether the battle has actually commenced or not.
	bool isBeforeGame() const;
	/// Randomly chooses hidden movement background.
	void setRandomHiddenMovementBackground(const Mod *mod);
	/// Gets the hidden movement background ID.
	std::string getHiddenMovementBackground() const;
	/// Appends a given entry to the hit log. Works only during the player's turn.
	void appendToHitLog(HitLogEntryType type, UnitFaction faction);
	void appendToHitLog(HitLogEntryType type, UnitFaction faction, const std::string &text);
	/// Gets the hit log.
	const HitLog *getHitLog() const;
	/// Reset all the unit hit state flags.
	void resetUnitHitStates();
};

}
