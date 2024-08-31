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
#include "../Mod/AlienDeployment.h"
#include "../Mod/RuleCraft.h"

namespace OpenXcom
{

class Tile;
class SavedGame;
class MapDataSet;
class Node;
class BattlescapeState;
class BattlescapeGame;
class Position;
class Pathfinding;
class TileEngine;
class RuleStartingCondition;
class RuleEnviroEffects;
class BattleItem;
class BattleUnit;
class Mod;
class State;
class ItemContainer;
class Craft;
class RuleItem;
class HitLog;
enum HitLogEntryType : int;
struct BattlescapeTally;

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
	/// Register useful function used by graphic scripts.
	static void ScriptRegisterUnitAnimations(ScriptParserBase* parser);

private:
	bool _isPreview;
	SDL_Rect _craftPos;
	int _craftZ;
	Craft* _craftForPreview;
	std::vector<Position> _craftTiles;
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
	std::string _lastUsedMapScript;
	int _alienItemLevel = 0;
	std::string _reinforcementsDeployment, _reinforcementsRace;
	int _reinforcementsItemLevel;
	std::map<std::string, int> _reinforcementsMemory;
	std::vector< std::vector<int> > _reinforcementsBlocks;
	std::vector< std::vector<std::string> > _flattenedMapTerrainNames;
	std::vector< std::vector<std::string> > _flattenedMapBlockNames;
	const RuleStartingCondition *_startingCondition;
	const RuleEnviroEffects *_enviroEffects;
	bool _ecEnabledFriendly, _ecEnabledHostile, _ecEnabledNeutral;
	int _globalShade;
	UnitFaction _side;
	int _turn, _bughuntMinTurn;
	int _animFrame;
	bool _nameDisplay;
	bool _debugMode, _bughuntMode;
	bool _aborted;
	bool _baseCraftInventory = false;
	int _itemId;
	EscapeType _vipEscapeType;
	int _vipSurvivalPercentage, _vipsSaved, _vipsLost, _vipsWaitingOutside, _vipsSavedScore, _vipsLostScore, _vipsWaitingOutsideScore;
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
	bool _togglePersonalLight, _toggleNightVision;
	int _toggleBrightness;
	bool _togglePersonalLightTemp = false, _toggleNightVisionTemp = false;
	int _toggleBrightnessTemp = 0, _toggleNightVisionColorTemp = 0;
	std::string _hiddenMovementBackground;
	HitLog *_hitLog;
	ScriptValues<SavedBattleGame> _scriptValues;
	/// Selects a soldier.
	BattleUnit *selectPlayerUnit(int dir, bool checkReselect = false, bool setReselect = false, bool checkInventory = false);
	/// Run newTurnUnit and newTurnItem scripts
	void newTurnUpdateScripts();
public:
	/// Creates a new battle save, based on the current generic save.
	SavedBattleGame(Mod *rule, Language *lang, bool isPreview = false);
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
	/// Sets the starting conditions.
	void setStartingCondition(const RuleStartingCondition* sc) { _startingCondition = sc; }
	/// Gets the starting conditions.
	const RuleStartingCondition* getStartingCondition() const { return _startingCondition; }
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
	/// Sets the last used map script.
	void setLastUsedMapScript(const std::string& lastUsedMapScript) { _lastUsedMapScript = lastUsedMapScript; }
	/// Gets the last used map script.
	const std::string& getLastUsedMapScript() const { return _lastUsedMapScript; }

	/// Sets the alien item level.
	void setAlienItemLevel(int itemLevel) { _alienItemLevel = itemLevel; }
	/// Gets the alien item level.
	int getAlienItemLevel() const { return _alienItemLevel; }

	/// Sets the alien deployment to use for reinforcements.
	void setReinforcementsDeployment(const std::string &reinforcementsDeployment) { _reinforcementsDeployment = reinforcementsDeployment; }
	/// Gets the alien deployment to use for reinforcements.
	const std::string &getReinforcementsDeployment() const { return _reinforcementsDeployment; }
	/// Sets the alien race to use for reinforcements.
	void setReinforcementsRace(const std::string &reinforcementsRace) { _reinforcementsRace = reinforcementsRace; }
	/// Gets the alien race to use for reinforcements.
	const std::string &getReinforcementsRace() const { return _reinforcementsRace; }
	/// Sets the alien item level to use for reinforcements.
	void setReinforcementsItemLevel(int reinforcementsItemLevel) { _reinforcementsItemLevel = reinforcementsItemLevel; }
	/// Gets the alien item level to use for reinforcements.
	int getReinforcementsItemLevel() const { return _reinforcementsItemLevel; }
	/// Gets the memory used for reinforcements.
	std::map<std::string, int> &getReinforcementsMemory() { return _reinforcementsMemory; }
	/// Gets the map blocks used for reinforcements.
	std::vector< std::vector<int> > &getReinforcementsBlocks() { return _reinforcementsBlocks; }

	/// Gets the flattened map block terrain names (or craft/ufo names) generated by the BattlescapeGenerator.
	std::vector< std::vector<std::string> > &getFlattenedMapTerrainNames() { return _flattenedMapTerrainNames; }
	/// Gets the flattened map block names generated by the BattlescapeGenerator.
	std::vector< std::vector<std::string> > &getFlattenedMapBlockNames() { return _flattenedMapBlockNames; }

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
	int getMapSizeX() const { return _mapsize_x; }
	/// Gets terrain size y.
	int getMapSizeY() const { return _mapsize_y; }
	/// Gets terrain size z.
	int getMapSizeZ() const { return _mapsize_z; }
	/// Gets terrain x*y*z
	int getMapSizeXYZ() const { return _mapsize_x * _mapsize_y * _mapsize_z; }

	/// Is this just a craft or base deployment preview?
	bool isPreview() const { return _isPreview; }
	/// Sets craft position.
	void setCraftPos(SDL_Rect craftPos) { _craftPos = craftPos; }
	/// Sets craft elevation.
	void setCraftZ(int craftZ) { _craftZ = craftZ; }
	/// Sets craft for preview.
	void setCraftForPreview(Craft* craftForPreview) { _craftForPreview = craftForPreview; }
	/// Gets craft for preview.
	const Craft* getCraftForPreview() const { return _craftForPreview; }
	/// Pre-calculate all valid tiles for later use in map drawing.
	void calculateCraftTiles();
	/// Gets craft tiles.
	const std::vector<Position>& getCraftTiles() const { return _craftTiles; }

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

	/**
	 * Gets the Tile at a given position on the map.
	 * This method is called over 50mil+ times per turn so it seems useful
	 * to inline it.
	 * @param pos Map position.
	 * @return Pointer to the tile at that position.
	 */
	inline const Tile *getTile(Position pos) const
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
	/// Clear state that given unit is selected.
	void clearUnitSelection(BattleUnit *unit);
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
	bool canUseWeapon(const BattleItem *weapon, const BattleUnit *unit, bool isBerserking, BattleActionType actionType, std::string* message = nullptr) const;
	/// Gets the turn number.
	int getTurn() const;
	/// Sets the bug hunt turn number.
	void setBughuntMinTurn(int bughuntMinTurn);
	/// Gets the bug hunt turn number.
	int getBughuntMinTurn() const;
	/// Start first turn of battle.
	void startFirstTurn();
	/// Check count of units in different state for craft deployment preview.
	BattlescapeTally tallyUnitsForPreview();
	/// Saves the custom craft deployment.
	void saveCustomCraftDeployment();
	/// Saves the custom RuleCraft deployment. Invalidates corresponding custom craft deployments.
	void saveDummyCraftDeployment();
	/// Does the given craft type have a custom deployment?
	bool hasCustomDeployment(const RuleCraft* rule) const;
	/// Gets a custom deployment for the given craft type.
	const RuleCraftDeployment& getCustomDeployment(const RuleCraft* rule) const;
	/// Ends the turn.
	void endTurn();
	/// Gets animation frame.
	int getAnimFrame() const;
	/// Increase animation frame.
	void nextAnimFrame();
	/// Sets debug mode.
	void setDebugMode();
	void revealMap();
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

	/// Add buildIn weapon from list to unit.
	void addFixedItems(BattleUnit *unit, const std::vector<const RuleItem*> &fixed);
	/// Init new created unit.
	void initUnit(BattleUnit *unit, size_t itemLevel = 0);
	/// Init new created item.
	void initItem(BattleItem *item, BattleUnit *unit = nullptr);
	/// Create new item for unit.
	BattleItem *createItemForUnit(const RuleItem *rule, BattleUnit *unit, bool fixedWeapon = false);
	/// Create new special built-in item for unit.
	BattleItem *createItemForUnitSpecialBuiltin(const RuleItem *rule, BattleUnit *unit);
	/// Create new item for tile.
	BattleItem *createItemForTile(const RuleItem *rule, Tile *tile, BattleUnit *corpseFor = nullptr);
	/// Create new item for tile.
	BattleItem *createItemForTile(const std::string& type, Tile *tile);
	/// Create new temporary item.
	BattleItem *createTempItem(const RuleItem *rule);
	/// Create new temporary unit.
	BattleUnit *createTempUnit(const Unit *rules, UnitFaction faction, int nextUnitId = -1);
	/// Converts a unit into a unit of another type.
	BattleUnit *convertUnit(BattleUnit *unit);

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
	/// Gets a pointer to the BattlescapeState.
	const BattlescapeState *getBattleState() const;
	/// Gets a pointer to the BattlescapeGame.
	BattlescapeGame *getBattleGame();
	/// Gets a pointer to the BattlescapeGame.
	const BattlescapeGame *getBattleGame() const;
	/// Is BattlescapeState busy?
	bool isBattlescapeStateBusy() const;
	/// Sets the pointer to the BattlescapeState.
	void setBattleState(BattlescapeState *bs);

	/// Is CTRL pressed?
	bool isCtrlPressed(bool considerTouchButtons = false) const;
	/// Is ALT pressed?
	bool isAltPressed(bool considerTouchButtons = false) const;
	/// Is SHIFT pressed?
	bool isShiftPressed(bool considerTouchButtons = false) const;

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
	/// check if the names or the callsigns of the soldiers should be displayed
	bool isNameDisplay() const;
	/// set displaying the name (true) or the callsign (false)
	void setNameDisplay(bool displayName);
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

	/// Sets the VIP escape type.
	void setVIPEscapeType(EscapeType vipEscapeType);
	/// Gets the VIP escape type.
	EscapeType getVIPEscapeType() const;
	/// Sets the percentage of VIPs that must survive in order to accomplish the mission.
	void setVIPSurvivalPercentage(int vipSurvivalPercentage);
	/// Gets the percentage of VIPs that must survive in order to accomplish the mission.
	int getVIPSurvivalPercentage() const;
	/// Increase the saved VIPs counter.
	void addSavedVIP(int score);
	/// Gets the saved VIPs counter.
	int getSavedVIPs() const;
	/// Gets the saved VIPs total score.
	int getSavedVIPsScore() const;
	/// Increase the lost VIPs counter.
	void addLostVIP(int score);
	/// Gets the lost VIPs counter.
	int getLostVIPs() const;
	/// Gets the lost VIPs total score.
	int getLostVIPsScore() const;
	/// Increase the waiting outside VIPs counter.
	void addWaitingOutsideVIP(int score);
	/// Corrects the VIP stats based on the final mission outcome.
	void correctVIPStats(bool success, bool retreated);

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

	/// gets personal light toggle
	bool getTogglePersonalLight() const { return _togglePersonalLight; }
	/// sets personal light toggle
	void setTogglePersonalLight(bool togglePersonalLight) { _togglePersonalLight = togglePersonalLight; }
	/// gets night vision toggle
	bool getToggleNightVision() const { return _toggleNightVision; }
	/// sets night vision toggle
	void setToggleNightVision(bool toggleNightVision) { _toggleNightVision = toggleNightVision; }
	/// gets brightness toggle
	int getToggleBrightness() const { return _toggleBrightness; }
	/// sets brightness toggle
	void setToggleBrightness(int toggleBrightness) { _toggleBrightness = toggleBrightness; }

	/// sets final value for personal light toggle (for scripts)
	void setTogglePersonalLightTemp(bool togglePersonalLight) { _togglePersonalLightTemp = togglePersonalLight; }
	/// sets final value for night vision toggle (for scripts)
	void setToggleNightVisionTemp(bool toggleNightVision) { _toggleNightVisionTemp = toggleNightVision; }
	/// sets final value for night vision color toggle (for scripts)
	void setToggleNightVisionColorTemp(int toggleNightColorVision) { _toggleNightVisionColorTemp = toggleNightColorVision; }
	/// sets final value for brightness toggle (for scripts)
	void setToggleBrightnessTemp(int toggleBrightness) { _toggleBrightnessTemp = toggleBrightness; }

	/// Randomly chooses hidden movement background.
	void setRandomHiddenMovementBackground(const Mod *mod);
	/// Gets the hidden movement background ID.
	const std::string& getHiddenMovementBackground() const;
	/// Appends a given entry to the hit log. Works only during the player's turn.
	void appendToHitLog(HitLogEntryType type, UnitFaction faction);
	void appendToHitLog(HitLogEntryType type, UnitFaction faction, const std::string &text);
	/// Gets the hit log.
	const HitLog *getHitLog() const;
	/// Reset all the unit hit state flags.
	void resetUnitHitStates();
};

}
