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
#include <list>
#include <vector>
#include <memory>
#include "../Engine/Surface.h"
#include "../Battlescape/Position.h"
#include "../Mod/MapData.h"

#include <SDL_types.h> // for Uint8

namespace OpenXcom
{

class MapData;
class BattleUnit;
class BattleItem;
class RuleInventory;
class Particle;
class SavedBattleGame;
class ScriptParserBase;

enum LightLayers : Uint8 { LL_AMBIENT, LL_FIRE, LL_ITEMS, LL_UNITS, LL_MAX };

enum TileUnitOverlapping : int
{
	/// Any unit overlapping tile will be returned
	TUO_NORMAL = 24,
	/// Only units that overlap by 2 or more voxel will be returned
	TUO_IGNORE_SMALL = 26,
	/// Take unit from lower even if it not overlap
	TUO_ALWAYS = 0,
};

/**
 * Basic element of which a battle map is build.
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 */
class Tile
{
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "Tile";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	static struct SerializationKey
	{
		// how many bytes to store for each variable or each member of array of the same name
		Uint8 index; // for indexing the actual tile array
		Uint8 _mapDataSetID;
		Uint8 _mapDataID;
		Uint8 _smoke;
		Uint8 _fire;
		Uint8 boolFields;
		Uint32 totalBytes; // per structure, including any data not mentioned here and accounting for all array members!
	} serializationKey;

	static const int NOT_CALCULATED = -1;

	/**
	 * Cache of ID for tile parts used to save and load.
	 */
	struct TileMapDataCache
	{
		int ID[O_MAX];
		int SetID[O_MAX];
	};
	/**
	 * Cached data that belongs to each tile object
	 */
	struct TileObjectCache
	{
		Sint8 offsetY;
		Uint8 currentFrame:4;
		Uint8 discovered:1;
		Uint8 isUfoDoor:1;
		Uint8 isDoor:1;
		Uint8 isBackTileObject:1;
	};
	/**
	 * Cached data that belongs to whole tile
	 */
	struct TileCache
	{
		Sint8 terrainLevel = 0;
		Uint8 isNoFloor:1;
		Uint8 bigWall:1;
		Uint8 danger:1;
	};

protected:
	SavedBattleGame* _save;
	MapData *_objects[O_MAX];
	BattleUnit *_unit = nullptr;
	std::vector<BattleItem *> _inventory;
	std::unique_ptr<TileMapDataCache> _mapData = std::make_unique<TileMapDataCache>();
	SurfaceRaw<const Uint8> _currentSurface[O_MAX] = { };
	TileObjectCache _objectsCache[O_MAX] = { };
	TileCache _cache = { };
	Position _pos;
	Uint8 _light[LL_MAX];
	Uint8 _fire = 0;
	Uint8 _smoke = 0;
	Uint8 _markerColor = 0;
	Uint8 _animationOffset = 0;
	Uint8 _obstacle = 0;
	Uint8 _explosiveType = 0;
	Sint16 _explosive = 0;
	Sint16 _visible = 0;
	Sint16 _TUMarker = -1;
	Sint8 _preview = -1;
	Uint8 _overlaps = 0;


public:
	/// Creates a tile.
	Tile(Position pos, SavedBattleGame* save);
	/// Copy constructor.
	Tile(Tile&&) = default;
	/// Cleans up a tile.
	~Tile();
	/// Load the tile from yaml
	void load(const YAML::Node &node);
	/// Load the tile from binary buffer in memory
	void loadBinary(Uint8 *buffer, Tile::SerializationKey& serializationKey);
	/// Saves the tile to yaml
	YAML::Node save() const;
	/// Saves the tile to binary
	void saveBinary(Uint8** buffer) const;

	/**
	 * Get the MapData pointer of a part of the tile.
	 * @param part TilePart whose data is needed.
	 * @return pointer to mapdata
	 */
	MapData *getMapData(TilePart part) const
	{
		return _objects[part];
	}

	/**
	 * Get special tile type of floor part.
	 * @return Type of Tile.
	 */
	SpecialTileType getFloorSpecialTileType() const
	{
		return _objects[O_FLOOR] ? _objects[O_FLOOR]->getSpecialType() : TILE;
	}

	/**
	 * Get special tile type of object part.
	 * @return Type of Tile.
	 */
	SpecialTileType getObjectSpecialTileType() const
	{
		return _objects[O_OBJECT] ? _objects[O_OBJECT]->getSpecialType() : TILE;
	}

	/// Get saved battle game that tile belongs.
	const SavedBattleGame* getSavedGame() const { return _save; }
	/// Get saved battle game that tile belongs.
	SavedBattleGame* getSavedGame() { return _save; }


	/// Sets the pointer to the mapdata for a specific part of the tile
	void setMapData(MapData *dat, int mapDataID, int mapDataSetID, TilePart part);
	/// Gets the IDs to the mapdata for a specific part of the tile
	void getMapData(int *mapDataID, int *mapDataSetID, TilePart part) const;
	/// Gets whether this tile has no objects
	bool isVoid() const;
	/// Get the TU cost to walk over a certain part of the tile.
	int getTUCost(int part, MovementType movementType) const;
	/// Checks if this tile has a floor.
	bool hasNoFloor(const SavedBattleGame *savedBattleGame = nullptr) const;

	/**
	 * Whether this tile has a big wall.
	 * @return bool
	 */
	bool isBigWall() const
	{
		return _cache.bigWall;
	}

	/**
	 * Gets the height of the terrain (dirt/stairs/etc.) on this tile.
	 * @return the height in voxels (more negative values are higher, e.g. -8 = lower stairs, -16 = higher stairs)
	 */
	int getTerrainLevel() const
	{
		return _cache.terrainLevel;
	}

	/**
	 * Gets the tile's position.
	 * @return position
	 */
	Position getPosition() const
	{
		return _pos;
	}

	/// Gets the floor object footstep sound.
	int getFootstepSound(Tile *tileBelow) const;
	/// Open a door, returns the ID, 0(normal), 1(ufo) or -1 if no door opened.
	int openDoor(TilePart part, BattleUnit *unit = 0, BattleActionType reserve = BA_NONE, bool rClick = false);

	/**
	 * Check if the ufo door is open or opening. Used for visibility/light blocking checks.
	 * This function assumes that there never are 2 doors on 1 tile or a door and another wall on 1 tile.
	 * @param part Tile part to look for door
	 * @return bool
	 */
	bool isUfoDoorOpen(TilePart tp) const
	{
		return (_objectsCache[tp].isUfoDoor && _objectsCache[tp].currentFrame);
	}

	/**
	 * Check if part is ufo door.
	 * @param tp Part to check
	 * @return True if part is ufo door.
	 */
	bool isUfoDoor(TilePart tp) const
	{
		return _objectsCache[tp].isUfoDoor;
	}

	/**
	 * Check if part is door.
	 * @param tp Part to check
	 * @return True if part is door.
	 */
	bool isDoor(TilePart tp) const
	{
		return _objectsCache[tp].isDoor;
	}

	/**
	 * Check if an object should be drawn behind or in front of a unit.
	 * @param tp Part to check
	 * @return True if its back object.
	 */
	bool isBackTileObject(TilePart tp) const
	{
		return _objectsCache[tp].isBackTileObject;
	}

	/**
	 * Gets surface Y offset.
	 * @param tp Part for offset.
	 * @return Offset value.
	 */
	int getYOffset(TilePart tp) const
	{
		return _objectsCache[tp].offsetY;
	}

	/// Close ufo door.
	int closeUfoDoor();
	/// Sets the black fog of war status of this tile.
	void setDiscovered(bool flag, TilePart part);
	/// Gets the black fog of war status of this tile.
	bool isDiscovered(TilePart part) const;
	/// Reset light to zero for this tile.
	void resetLight(LightLayers layer);
	/// Reset light to zero for this tile and multiple layers.
	void resetLightMulti(LightLayers layer);
	/// Add light to this tile.
	void addLight(int light, LightLayers layer);
	/// Get max light to this tile.
	int getLight(LightLayers layer) const;
	/// Get max light to this tile and multiple layers.
	int getLightMulti(LightLayers layer) const;
	/// Get the shade amount.
	int getShade() const;
	/// Destroy a tile part.
	bool destroy(TilePart part, SpecialTileType type);
	/// Damage a tile part.
	bool damage(TilePart part, int power, SpecialTileType type);
	/// Set a "virtual" explosive on this tile, to detonate later.
	void setExplosive(int power, int damageType, bool force = false);
	/// Get explosive power of this tile.
	int getExplosive() const;
	/// Get explosive power of this tile.
	int getExplosiveType() const;
	/// Animated the tile parts.
	void animate();
	/// Update cached value of sprite.
	void updateSprite(TilePart part);
	/// Get object sprites.
	SurfaceRaw<const Uint8> getSprite(TilePart part) const
	{
		return _currentSurface[part];
	}

	/**
	 * Set a unit on this tile.
	 */
	void setUnit(BattleUnit *unit)
	{
		_unit = unit;
	}

	/**
	 * Get the (alive) unit on this tile.
	 * @return BattleUnit.
	 */
	BattleUnit *getUnit() const
	{
		return _unit;
	}

	/// Get unit from this tile or from tile below.
	BattleUnit *getOverlappingUnit(const SavedBattleGame *saveBattleGame, TileUnitOverlapping range = TUO_NORMAL) const;
	/// Set fire, does not increment overlaps.
	void setFire(int fire);
	/// Get fire.
	int getFire() const;
	/// Add smoke, increments overlap.
	void addSmoke(int smoke);
	/// Set smoke, does not increment overlaps.
	void setSmoke(int smoke);
	/// Get smoke.
	int getSmoke() const;
	/// Get flammability.
	int getFlammability() const;
	/// Get turns to burn
	int getFuel() const;
	/// Get flammability of part.
	int getFlammability(TilePart part) const;
	/// Get turns to burn of part
	int getFuel(TilePart part) const;
	/// attempt to set the tile on fire, sets overlaps to one if successful.
	void ignite(int power);
	/// Get fire and smoke animation offset.
	int getAnimationOffset() const;
	/// Add item
	void addItem(BattleItem *item, RuleInventory *ground);
	/// Remove item
	void removeItem(BattleItem *item);
	/// Get top-most item
	BattleItem* getTopItem();
	/// New turn preparations.
	void prepareNewTurn(bool smokeDamage);
	/// Get inventory on this tile.
	std::vector<BattleItem *> *getInventory();
	/// Set the tile marker color.
	void setMarkerColor(int color);
	/// Get the tile marker color.
	int getMarkerColor() const;
	/// Set the tile visible flag.
	void setVisible(int visibility);
	/// Get the tile visible flag.
	int getVisible() const;
	/// set the direction (used for path previewing)
	void setPreview(int dir);
	/// retrieve the direction stored by the pathfinding.
	int getPreview() const;
	/// set the number to be displayed for pathfinding preview.
	void setTUMarker(int tu);
	/// get the number to be displayed for pathfinding preview.
	int getTUMarker() const;
	/// how many times has this tile been overlapped with smoke/fire (runtime only)
	int getOverlaps() const;
	/// increment the overlap value on this tile.
	void addOverlap();
	/// set the danger flag on this tile (so the AI will avoid it).
	void setDangerous(bool danger);
	/// check the danger flag on this tile.
	bool getDangerous() const;

	/// sets single obstacle flag.
	void setObstacle(int part);
	/// gets single obstacle flag.
	bool getObstacle(int part) const
	{
		return _obstacle & (1 << part);
	}
	/// does the tile have obstacle flag set for at least one part?
	bool isObstacle(void) const
	{
		return _obstacle != 0;
	}
	/// reset obstacle flags
	void resetObstacle(void);
};

}
