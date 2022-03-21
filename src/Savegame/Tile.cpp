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
#include "Tile.h"
#include <algorithm>
#include "../Mod/MapData.h"
#include "../Mod/MapDataSet.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Surface.h"
#include "../Engine/RNG.h"
#include "../Engine/ScriptBind.h"
#include "BattleUnit.h"
#include "BattleItem.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "SerializationHelper.h"
#include "../Battlescape/Particle.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../fmath.h"
#include "SavedBattleGame.h"

namespace OpenXcom
{

/// How many bytes various fields use in a serialized tile. See header.
Tile::SerializationKey Tile::serializationKey =
{4, // index
 2, // _mapDataSetID, four of these
 2, // _mapDataID, four of these
 1, // _fire
 1, // _smoke
 1,	// one 8-bit bool field
 4 + 2*4 + 2*4 + 1 + 1 + 1 // total bytes to save one tile
};

/**
 * constructor
 * @param pos Position.
 */
Tile::Tile(Position pos, SavedBattleGame* save): _save(save), _pos(pos)
{
	for (int i = 0; i < O_MAX; ++i)
	{
		_objects[i] = 0;
		_mapData->ID[i] = -1;
		_mapData->SetID[i] = -1;
		_objectsCache[i].currentFrame = 0;
	}
	for (int layer = 0; layer < LL_MAX; layer++)
	{
		_light[layer] = 0;
	}
	for (int i = 0; i < O_MAX; ++i)
	{
		_objectsCache[i].discovered = 0;
	}
	_cache.isNoFloor = 1;
}

/**
 * destructor
 */
Tile::~Tile()
{
	_inventory.clear();
}

/**
 * Load the tile from a YAML node.
 * @param node YAML node.
 */
void Tile::load(const YAML::Node &node)
{
	//_position = node["position"].as<Position>(_position);
	for (int i = 0; i < 4; i++)
	{
		_mapData->ID[i] = node["mapDataID"][i].as<int>(_mapData->ID[i]);
		_mapData->SetID[i] = node["mapDataSetID"][i].as<int>(_mapData->SetID[i]);
	}
	_fire = node["fire"].as<int>(_fire);
	_smoke = node["smoke"].as<int>(_smoke);
	if (node["discovered"])
	{
		for (int i = 0; i < 3; i++)
		{
			auto realTilePart = (i == 2 ? 0 : i - 1); //convert old convention to new one
			_objectsCache[realTilePart].discovered = (Uint8)node["discovered"][i].as<bool>();
		}
	}
	if (node["openDoorWest"])
	{
		_objectsCache[1].currentFrame = 7;
	}
	if (node["openDoorNorth"])
	{
		_objectsCache[2].currentFrame = 7;
	}
	if (_fire || _smoke)
	{
		_animationOffset = RNG::seedless(0, 3);
	}
}

/**
 * Load the tile from binary.
 * @param buffer Pointer to buffer.
 * @param serKey Serialization key.
 */
void Tile::loadBinary(Uint8 *buffer, Tile::SerializationKey& serKey)
{
	_mapData->ID[0] = unserializeInt(&buffer, serKey._mapDataID);
	_mapData->ID[1] = unserializeInt(&buffer, serKey._mapDataID);
	_mapData->ID[2] = unserializeInt(&buffer, serKey._mapDataID);
	_mapData->ID[3] = unserializeInt(&buffer, serKey._mapDataID);
	_mapData->SetID[0] = unserializeInt(&buffer, serKey._mapDataSetID);
	_mapData->SetID[1] = unserializeInt(&buffer, serKey._mapDataSetID);
	_mapData->SetID[2] = unserializeInt(&buffer, serKey._mapDataSetID);
	_mapData->SetID[3] = unserializeInt(&buffer, serKey._mapDataSetID);

	_smoke = unserializeInt(&buffer, serKey._smoke);
	_fire = unserializeInt(&buffer, serKey._fire);

	Uint8 boolFields = unserializeInt(&buffer, serKey.boolFields);
	_objectsCache[O_WESTWALL].discovered = (boolFields & 1) ? 1 : 0;
	_objectsCache[O_NORTHWALL].discovered = (boolFields & 2) ? 1 : 0;
	_objectsCache[O_FLOOR].discovered = (boolFields & 4) ? 1 : 0;
	_objectsCache[O_WESTWALL].currentFrame = (boolFields & 8) ? 7 : 0;
	_objectsCache[O_NORTHWALL].currentFrame = (boolFields & 0x10) ? 7 : 0;
	if (_fire || _smoke)
	{
		_animationOffset = RNG::seedless(0, 3);
	}
}


/**
 * Saves the tile to a YAML node.
 * @return YAML node.
 */
YAML::Node Tile::save() const
{
	YAML::Node node;
	node["position"] = _pos;
	for (int i = 0; i < 4; i++)
	{
		node["mapDataID"].push_back(_mapData->ID[i]);
		node["mapDataSetID"].push_back(_mapData->SetID[i]);
	}
	if (_smoke)
		node["smoke"] = _smoke;
	if (_fire)
		node["fire"] = _fire;
	if (_objectsCache[O_FLOOR].discovered || _objectsCache[O_WESTWALL].discovered || _objectsCache[O_NORTHWALL].discovered)
	{
		throw Exception("Obsolete code");
//		for (int i = O_FLOOR; i <= O_NORTHWALL; i++)
//		{
//			node["discovered"].push_back(_objectsCache[i].discovered);
//		}
	}
	if (isUfoDoorOpen(O_WESTWALL))
	{
		node["openDoorWest"] = true;
	}
	if (isUfoDoorOpen(O_NORTHWALL))
	{
		node["openDoorNorth"] = true;
	}
	return node;
}

/**
 * Saves the tile to binary.
 * @param buffer pointer to buffer.
 */
void Tile::saveBinary(Uint8** buffer) const
{
	serializeInt(buffer, serializationKey._mapDataID, _mapData->ID[0]);
	serializeInt(buffer, serializationKey._mapDataID, _mapData->ID[1]);
	serializeInt(buffer, serializationKey._mapDataID, _mapData->ID[2]);
	serializeInt(buffer, serializationKey._mapDataID, _mapData->ID[3]);
	serializeInt(buffer, serializationKey._mapDataSetID, _mapData->SetID[0]);
	serializeInt(buffer, serializationKey._mapDataSetID, _mapData->SetID[1]);
	serializeInt(buffer, serializationKey._mapDataSetID, _mapData->SetID[2]);
	serializeInt(buffer, serializationKey._mapDataSetID, _mapData->SetID[3]);

	serializeInt(buffer, serializationKey._smoke, _smoke);
	serializeInt(buffer, serializationKey._fire, _fire);

	Uint8 boolFields = (_objectsCache[O_WESTWALL].discovered?1:0) + (_objectsCache[O_NORTHWALL].discovered?2:0) + (_objectsCache[O_FLOOR].discovered?4:0);
	boolFields |= isUfoDoorOpen(O_WESTWALL) ? 8 : 0; // west
	boolFields |= isUfoDoorOpen(O_NORTHWALL) ? 0x10 : 0; // north?
	serializeInt(buffer, serializationKey.boolFields, boolFields);
}

/**
 * Set the MapData references of part 0 to 3.
 * @param dat pointer to the data object
 * @param mapDataID
 * @param mapDataSetID
 * @param part Part of the tile to set data of
 */
void Tile::setMapData(MapData *dat, int mapDataID, int mapDataSetID, TilePart part)
{
	_objects[part] = dat;
	_mapData->ID[part] = mapDataID;
	_mapData->SetID[part] = mapDataSetID;
	_objectsCache[part].isDoor = dat ? dat->isDoor() : 0;
	_objectsCache[part].isUfoDoor = dat ? dat->isUFODoor() : 0;
	_objectsCache[part].offsetY = dat ? dat->getYOffset() : 0;
	_objectsCache[part].isBackTileObject = dat ? dat->isBackTileObject() : 0;
	if (part == O_FLOOR || part == O_OBJECT)
	{
		int level = 0;

		if (_objects[O_FLOOR])
		{
			level = _objects[O_FLOOR]->getTerrainLevel();
			_cache.isNoFloor = _objects[O_FLOOR]->isNoFloor();
		}
		else
		{
			_cache.isNoFloor = 1;
		}
		// whichever's higher, but not the sum.
		if (_objects[O_OBJECT])
		{
			level = std::min(_objects[O_OBJECT]->getTerrainLevel(), level);
			_cache.bigWall = _objects[O_OBJECT]->getBigWall() != 0;
		}
		else
		{
			_cache.bigWall = 0;
		}
		_cache.terrainLevel = level;
	}
	updateSprite(part);
}

/**
 * get the MapData references of part 0 to 3.
 * @param mapDataID
 * @param mapDataSetID
 * @param part is part of the tile to get data from
 * @return the object ID
 */
void Tile::getMapData(int *mapDataID, int *mapDataSetID, TilePart part) const
{
	*mapDataID = _mapData->ID[part];
	*mapDataSetID = _mapData->SetID[part];
}

/**
 * Gets whether this tile has no objects. Note that we can have a unit or smoke on this tile.
 * @return bool True if there is nothing but air on this tile.
 */
bool Tile::isVoid() const
{
	return _objects[0] == 0 && _objects[1] == 0 && _objects[2] == 0 && _objects[3] == 0 && _smoke == 0 && _inventory.empty();
}

/**
 * Gets the TU cost to walk over a certain part of the tile.
 * @param part The part number.
 * @param movementType The movement type.
 * @return TU cost.
 */
int Tile::getTUCost(int part, MovementType movementType) const
{
	if (_objects[part])
	{
		if (_objectsCache[part].isUfoDoor && _objectsCache[part].currentFrame > 1)
			return 0;
		if (part == O_OBJECT && _objects[part]->getBigWall() >= 4)
			return 0;
		return _objects[part]->getTUCost(movementType);
	}
	else
		return 0;
}

/**
 * Whether this tile has a floor or not. If no object defined as floor, it has no floor.
 * @param savedBattleGame Save to get tile below to check if it can work as floor.
 * @return bool
 */
bool Tile::hasNoFloor(const SavedBattleGame *savedBattleGame) const
{
	//There's no point in checking for "floor" below if we have floor in this tile already.
	if (_cache.isNoFloor)
	{
		if (_pos.z > 0 && savedBattleGame)
		{
			const Tile* tileBelow = savedBattleGame->getBelowTile(this);
			if (tileBelow != 0 && tileBelow->getTerrainLevel() == -24)
				return false;
		}
	}

	return _cache.isNoFloor;
}

/**
 * Gets the tile's footstep sound.
 * @param tileBelow
 * @return sound ID
 */
int Tile::getFootstepSound(Tile *tileBelow) const
{
	int sound = -1;

	if (_objects[O_FLOOR])
		sound = _objects[O_FLOOR]->getFootstepSound();
	if (_objects[O_OBJECT] && _objects[O_OBJECT]->getBigWall() <= 1 && _objects[O_OBJECT]->getFootstepSound() > -1)
		sound = _objects[O_OBJECT]->getFootstepSound();
	if (!_objects[O_FLOOR] && !_objects[O_OBJECT] && tileBelow != 0 && tileBelow->getTerrainLevel() == -24 && tileBelow->getMapData(O_OBJECT))
		sound = tileBelow->getMapData(O_OBJECT)->getFootstepSound();

	return sound;
}


/**
 * Open a door on this tile.
 * @param part
 * @param unit
 * @param reserve
 * @param rClick
 * @return a value: 0(normal door), 1(ufo door) or -1 if no door opened or 3 if ufo door(=animated) is still opening 4 if not enough TUs
 */
int Tile::openDoor(TilePart part, BattleUnit *unit, BattleActionType reserve, bool rClick)
{
	if (!_objects[part]) return -1;

	BattleActionCost cost;
	if (unit)
	{
		int tuCost = _objects[part]->getTUCost(unit->getMovementType());
		cost = BattleActionCost(reserve, unit, unit->getMainHandWeapon(false));
		cost.Time += tuCost;
		if (!rClick)
		{
			cost.Energy += tuCost / 2;
		}
	}

	if (_objectsCache[part].isDoor)
	{
		if (unit && unit->getArmor()->getSize() > 1) // don't allow double-wide units to open swinging doors due to engine limitations
			return -1;
		if (unit && cost.Time && !cost.haveTU())
			return 4;
		if (_unit && _unit != unit && _unit->getPosition() != getPosition())
			return -1;
		setMapData(_objects[part]->getDataset()->getObject(_objects[part]->getAltMCD()), _objects[part]->getAltMCD(), _mapData->SetID[part],
				   _objects[part]->getDataset()->getObject(_objects[part]->getAltMCD())->getObjectType());
		setMapData(0, -1, -1, part);
		return 0;
	}
	if (_objectsCache[part].isUfoDoor && _objectsCache[part].currentFrame == 0) // ufo door part 0 - door is closed
	{
		if (unit && cost.Time && !cost.haveTU())
			return 4;
		_objectsCache[part].currentFrame = 1; // start opening door
		updateSprite((TilePart)part);
		return 1;
	}
	if (_objectsCache[part].isUfoDoor && _objectsCache[part].currentFrame != 7) // ufo door != part 7 - door is still opening
	{
		return 3;
	}
	return -1;
}

int Tile::closeUfoDoor()
{
	int retval = 0;

	for (int part = O_FLOOR; part <= O_NORTHWALL; ++part)
	{
		if (isUfoDoorOpen((TilePart)part))
		{
			_objectsCache[part].currentFrame = 0;
			retval = 1;
			updateSprite((TilePart)part);
		}
	}

	return retval;
}

/**
 * Sets the tile's cache flag. - TODO: set this for each object separately?
 * @param flag true/false
 * @param part Tile part
 */
void Tile::setDiscovered(bool flag, TilePart part)
{
	if (_objectsCache[part].discovered != flag)
	{
		_objectsCache[part].discovered = flag;
		if (part == O_FLOOR && flag == true)
		{
			_objectsCache[O_WESTWALL].discovered = true;
			_objectsCache[O_NORTHWALL].discovered = true;
		}
	}
}

/**
 * Get the black fog of war state of this tile.
 * @param part Tile part
 * @return bool True = discovered the tile.
 */
bool Tile::isDiscovered(TilePart part) const
{
	return _objectsCache[part].discovered;
}


/**
 * Reset the light amount on the tile. This is done before a light level recalculation.
 * @param layer Light is separated in 3 layers: Ambient, Static and Dynamic.
 */
void Tile::resetLight(LightLayers layer)
{
	_light[layer] = 0;
}

/**
 * Reset multiple layers of light from defined one.
 * @param layer From with layer start reset.
 */
void Tile::resetLightMulti(LightLayers layer)
{
	for (int l = layer; l < LL_MAX; l++)
	{
		_light[l] = 0;
	}
}

/**
 * Add the light amount on the tile. Only add light if the current light is lower.
 * @param light Amount of light to add.
 * @param layer Light is separated in 3 layers: Ambient, Static and Dynamic.
 */
void Tile::addLight(int light, LightLayers layer)
{
	if (_light[layer] < light)
		_light[layer] = light;
}

/**
 * Get current light amount of the tile.
 * @param layer Light is separated in 3 layers: Ambient, Static and Dynamic.
 * @return Max light value of selected layer.
 */
int Tile::getLight(LightLayers layer) const
{
	return _light[layer];
}

int Tile::getLightMulti(LightLayers layer) const
{
	int light = 0;

	for (int l = layer; l >= 0; --l)
	{
		if (_light[l] > light)
			light = _light[l];
	}

	return light;
}


/**
 * Gets the tile's shade amount 0-15. It returns the brightest of all light layers.
 * Shade level is the inverse of light level. So a maximum amount of light (15) returns shade level 0.
 * @return shade
 */
int Tile::getShade() const
{
	int light = 0;

	for (int layer = 0; layer < LL_MAX; layer++)
	{
		if (_light[layer] > light)
			light = _light[layer];
	}

	return std::max(0, 15 - light);
}

/**
 * Destroy a part on this tile. We first remove the old object, then replace it with the destroyed one.
 * This is because the object type of the old and new one are not necessarily the same.
 * If the destroyed part is an explosive, set the tile's explosive value, which will trigger a chained explosion.
 * @param part the part to destroy.
 * @param type the objective type for this mission we are checking against.
 * @return bool Return true objective was destroyed.
 */
bool Tile::destroy(TilePart part, SpecialTileType type)
{
	bool _objective = false;
	if (_objects[part])
	{
		if (_objects[part]->isGravLift())
			return false;
		_objective = _objects[part]->getSpecialType() == type;
		MapData *originalPart = _objects[part];
		int originalMapDataSetID = _mapData->SetID[part];
		setMapData(0, -1, -1, part);
		if (originalPart->getDieMCD())
		{
			MapData *dead = originalPart->getDataset()->getObject(originalPart->getDieMCD());
			setMapData(dead, originalPart->getDieMCD(), originalMapDataSetID, dead->getObjectType());
		}
		if (originalPart->getExplosive())
		{
			setExplosive(originalPart->getExplosive(), originalPart->getExplosiveType());
		}
	}
	/* check if the floor on the lowest level is gone */
	if (part == O_FLOOR && getPosition().z == 0 && _objects[O_FLOOR] == 0)
	{
		/* replace with scorched earth */
		setMapData(MapDataSet::getScorchedEarthTile(), 1, 0, O_FLOOR);
	}
	return _objective;
}

/**
 * damage terrain - check against armor
 * @param part Part to check.
 * @param power Power of the damage.
 * @param type the objective type for this mission we are checking against.
 * @return bool Return true objective was destroyed
 */
bool Tile::damage(TilePart part, int power, SpecialTileType type)
{
	bool objective = false;
	if (power >= _objects[part]->getArmor())
		objective = destroy(part, type);
	return objective;
}

/**
 * Set a "virtual" explosive on this tile. We mark a tile this way to detonate it later.
 * We do it this way, because the same tile can be visited multiple times by an "explosion ray".
 * The explosive power on the tile is some kind of moving MAXIMUM of the explosive rays that passes it.
 * @param power Power of the damage.
 * @param damageType the damage type of the explosion (not the same as item damage types)
 * @param force Force damage.
 */
void Tile::setExplosive(int power, int damageType, bool force)
{
	if (force || _explosive < power)
	{
		_explosive = power;
		_explosiveType = damageType;
	}
}

/**
 * Get explosive on this tile.
 * @return explosive
 */
int Tile::getExplosive() const
{
	return _explosive;
}

/**
 * Get explosive on this tile.
 * @return explosive
 */
int Tile::getExplosiveType() const
{
	return _explosiveType;
}

/*
 * Flammability of a tile is the lowest flammability of it's objects.
 * @return Flammability : the lower the value, the higher the chance the tile/object catches fire.
 */
int Tile::getFlammability() const
{
	int flam = 255;

	for (int i=0; i<4; ++i)
		if (_objects[i] && (_objects[i]->getFlammable() < flam))
			flam = _objects[i]->getFlammable();

	return flam;
}

/*
 * Fuel of a tile is the highest fuel of it's objects.
 * @return how long to burn.
 */
int Tile::getFuel() const
{
	int fuel = 0;

	for (int i=0; i<4; ++i)
		if (_objects[i] && (_objects[i]->getFuel() > fuel))
			fuel = _objects[i]->getFuel();

	return fuel;
}


/*
 * Flammability of the particular part of the tile
 * @return Flammability : the lower the value, the higher the chance the tile/object catches fire.
 */
int Tile::getFlammability(TilePart part) const
{
	return _objects[part]->getFlammable();
}

/*
 * Fuel of particular part of the tile
 * @return how long to burn.
 */
int Tile::getFuel(TilePart part) const
{
	return _objects[part]->getFuel();
}

/*
 * Ignite starts fire on a tile, it will burn <fuel> rounds. Fuel of a tile is the highest fuel of its objects.
 * NOT the sum of the fuel of the objects!
 */
void Tile::ignite(int power)
{
	if (getFlammability() != 255)
	{
		power = power - (getFlammability() / 10) + 15;
		if (power < 0)
		{
			power = 0;
		}
		if (RNG::percent(power) && getFuel())
		{
			if (_fire == 0)
			{
				_smoke = 15 - Clamp(getFlammability() / 10, 1, 12);
				_overlaps = 1;
				_fire = getFuel() + 1;
				_animationOffset = RNG::generate(0,3);
			}
		}
	}
}

/**
 * Animate the tile. This means to advance the current frame for every object.
 * Ufo doors are a bit special, they animated only when triggered.
 * When ufo doors are on frame 0(closed) or frame 7(open) they are not animated further.
 */
void Tile::animate()
{
	int newframe;
	for (int i = O_FLOOR; i < O_MAX; ++i)
	{
		if (_objects[i])
		{
			if (_objectsCache[i].isUfoDoor && (_objectsCache[i].currentFrame == 0 || _objectsCache[i].currentFrame == 7)) // ufo door is static
			{
				continue;
			}
			newframe = _objectsCache[i].currentFrame + 1;
			if (_objectsCache[i].isUfoDoor && _objects[i]->getSpecialType() == START_POINT && newframe == 3)
			{
				newframe = 7;
			}
			if (newframe == 8)
			{
				newframe = 0;
			}
			_objectsCache[i].currentFrame = newframe;
		}
		updateSprite((TilePart)i);
	}
}

/**
 * Update cached value of sprite.
 */
void Tile::updateSprite(TilePart part)
{
	if (_objects[part])
	{
		_currentSurface[part] = _objects[part]->getDataset()->getSurfaceset()->getFrame(_objects[part]->getSprite(_objectsCache[part].currentFrame));
	}
	else
	{
		_currentSurface[part] = nullptr;
	}
}

/**
 * Get unit from this tile or from tile below if unit poke out.
 * @param saveBattleGame
 * @return BattleUnit.
 */
BattleUnit *Tile::getOverlappingUnit(const SavedBattleGame *saveBattleGame, TileUnitOverlapping range) const
{
	auto bu = getUnit();
	if (!bu && _pos.z > 0 && hasNoFloor(saveBattleGame) && _objects[O_OBJECT] == nullptr)
	{
		auto tileBelow = saveBattleGame->getBelowTile(this);
		bu = tileBelow->getUnit();
		if (bu && bu->getHeight() + bu->getFloatHeight() - tileBelow->getTerrainLevel() <= static_cast<int>(range))
		{
			bu = nullptr; // if the unit below has no voxels poking into the tile, don't select it.
		}
	}
	return bu;
}

/**
 * Set the amount of turns this tile is on fire. 0 = no fire.
 * @param fire : amount of turns this tile is on fire.
 */
void Tile::setFire(int fire)
{
	_fire = Clamp(fire, 0, 255);
	_animationOffset = RNG::generate(0,3);
}

/**
 * Get the amount of turns this tile is on fire. 0 = no fire.
 * @return fire : amount of turns this tile is on fire.
 */
int Tile::getFire() const
{
	return _fire;
}

/**
 * Set the amount of turns this tile is smoking. 0 = no smoke.
 * @param smoke : amount of turns this tile is smoking.
 */
void Tile::addSmoke(int smoke)
{
	if (_fire == 0)
	{
		if (_overlaps == 0)
		{
			_smoke = Clamp(_smoke + smoke, 1, 15);
		}
		else
		{
			_smoke += smoke;
		}
		_animationOffset = RNG::generate(0,3);
		addOverlap();
	}
}

/**
 * Set the amount of turns this tile is smoking. 0 = no smoke.
 * @param smoke : amount of turns this tile is smoking.
 */
void Tile::setSmoke(int smoke)
{
	_smoke = Clamp(smoke, 0, 255);
	_animationOffset = RNG::generate(0,3);
}


/**
 * Get the amount of turns this tile is smoking. 0 = no smoke.
 * @return smoke : amount of turns this tile is smoking.
 */
int Tile::getSmoke() const
{
	return _smoke;
}

/**
 * Get the number of frames the fire or smoke animation is off-sync.
 * To void fire and smoke animations of different tiles moving nice in sync - it looks fake.
 * @return offset
 */
int Tile::getAnimationOffset() const
{
	return _animationOffset;
}

/**
 * Add an item on the tile.
 * @param item
 * @param ground
 */
void Tile::addItem(BattleItem *item, RuleInventory *ground)
{
	item->setSlot(ground);
	_inventory.push_back(item);
	item->setTile(this);

	// Note: floorOb drawing optimisation
	if (item->getUnit() && _inventory.size() > 1)
	{
		std::swap(_inventory.front(), _inventory.back());
	}
}

/**
 * Remove an item from the tile.
 * @param item
 */
void Tile::removeItem(BattleItem *item)
{
	for (std::vector<BattleItem*>::iterator i = _inventory.begin(); i != _inventory.end(); ++i)
	{
		if ((*i) == item)
		{
			_inventory.erase(i);
			break;
		}
	}
	item->setTile(0);
}

/**
 * Get the topmost item sprite to draw on the battlescape.
 * @return item sprite ID in floorob, or -1 when no item
 */
BattleItem* Tile::getTopItem()
{
	// Note: floorOb drawing optimisation
	if (_inventory.size() > 100)
	{
		// this tile has a metric ton of junk, it doesn't matter what gets drawn, let's draw it quickly
		return _inventory.front();
	}

	int biggestWeight = -1;
	BattleItem* biggestItem = 0;
	for (std::vector<BattleItem*>::iterator i = _inventory.begin(); i != _inventory.end(); ++i)
	{
		// Note: floorOb drawing optimisation
		if ((*i)->getUnit())
		{
			// any unit has the highest priority (btw. this is still backwards-compatible with both xcom1/xcom2, where corpses are the heaviest items)
			return *i;
		}
		int temp = (*i)->getTotalWeight();
		if (temp > biggestWeight)
		{
			biggestWeight = temp;
			biggestItem = *i;
		}
	}
	return biggestItem;
}

/**
 * Apply environment damage to unit.
 * @param unit affected unit.
 * @param smoke amount of smoke.
 * @param fire amount of file.
 */
static inline void applyEnvi(BattleUnit* unit, int smoke, int fire, bool smokeDamage)
{
	if (unit)
	{
		if (fire)
		{
			//and avoid setting fire elementals on fire
			if (unit->getSpecialAbility() != SPECAB_BURNFLOOR && unit->getSpecialAbility() != SPECAB_BURN_AND_EXPLODE)
			{
				// _smoke becomes our damage value
				unit->setEnviFire(smoke);
			}
		}
		// no fire: must be smoke
		else if (smokeDamage)
		{
			// try to knock this guy out.
			unit->setEnviSmoke(smoke / 4 + 1);
		}
	}
}

/**
 * New turn preparations.
 * average out any smoke added by the number of overlaps.
 * apply fire/smoke damage to units as applicable.
 */
void Tile::prepareNewTurn(bool smokeDamage)
{
	// we've received new smoke in this turn, but we're not on fire, average out the smoke.
	if ( _overlaps != 0 && _smoke != 0 && _fire == 0)
	{
		_smoke = Clamp((_smoke / _overlaps) - 1, 0, 15);
	}
	// if we still have smoke/fire
	if (_smoke)
	{
		applyEnvi(_unit, _smoke, _fire, smokeDamage);
		for (std::vector<BattleItem*>::iterator i = _inventory.begin(); i != _inventory.end(); ++i)
		{
			applyEnvi((*i)->getUnit(), _smoke, _fire, smokeDamage);
		}
	}
	_overlaps = 0;
}

/**
 * Get the inventory on this tile.
 * @return pointer to a vector of battle items.
 */
std::vector<BattleItem *> *Tile::getInventory()
{
	return &_inventory;
}


/**
 * Set the marker color on this tile.
 * @param color
 */
void Tile::setMarkerColor(int color)
{
	_markerColor = color;
}

/**
 * Get the marker color on this tile.
 * @return color
 */
int Tile::getMarkerColor() const
{
	return _markerColor;
}

/**
 * Set the tile visible flag.
 * @param visibility
 */
void Tile::setVisible(int visibility)
{
	_visible += visibility;
}

/**
 * Get the tile visible flag.
 * @return visibility
 */
int Tile::getVisible() const
{
	return _visible;
}

/**
 * set the direction used for path previewing.
 * @param dir
 */
void Tile::setPreview(int dir)
{
	_preview = dir;
}

/**
 * retrieve the direction stored by the pathfinding.
 * @return preview
 */
int Tile::getPreview() const
{
	return _preview;
}

/**
 * set the number to be displayed for pathfinding preview.
 * @param tu
 */
void Tile::setTUMarker(int tu)
{
	_TUMarker = tu;
}

/**
 * get the number to be displayed for pathfinding preview.
 * @return marker
 */
int Tile::getTUMarker() const
{
	return _TUMarker;
}

/**
 * get the overlap value of this tile.
 * @return overlap
 */
int Tile::getOverlaps() const
{
	return _overlaps;
}

/**
 * increment the overlap value on this tile.
 */
void Tile::addOverlap()
{
	++_overlaps;
}

/**
 * set the danger flag on this tile.
 */
void Tile::setDangerous(bool danger)
{
	_cache.danger = danger;
}

/**
 * get the danger flag on this tile.
 * @return the danger flag for this tile.
 */
bool Tile::getDangerous() const
{
	return _cache.danger;
}

/**
 * sets the flag of an obstacle for single part.
 */
void Tile::setObstacle(int part)
{
	_obstacle |= (1 << part);
}

/**
 * resets obstacle flag for all parts of the tile.
 */
void Tile::resetObstacle(void)
{
	_obstacle = 0;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

/**
 * Get the X part of the tile coordinate of this tile.
 * @return X Position.
 */
void getPositionXScript(const Tile *t, int &ret)
{
	if (t)
	{
		ret = t->getPosition().x;
		return;
	}
	ret = 0;
}

/**
 * Get the Y part of the tile coordinate of this tile.
 * @return Y Position.
 */
void getPositionYScript(const Tile *t, int &ret)
{
	if (t)
	{
		ret = t->getPosition().y;
		return;
	}
	ret = 0;
}

/**
 * Get the Z part of the tile coordinate of this tile.
 * @return Z Position.
 */
void getPositionZScript(const Tile *t, int &ret)
{
	if (t)
	{
		ret = t->getPosition().z;
		return;
	}
	ret = 0;
}

void getDistanceTileScript(const Tile *t, int &ret, const Tile *other)
{
	ret = t && other ? Position::distance(t->getPosition(), other->getPosition()) : -1;
}

void getDistanceVoxelScript(const Tile *t, int &ret, const Tile *other)
{
	ret = t && other ? Position::distance(t->getPosition().toVoxel(), other->getPosition().toVoxel()) : -1;
}

void getFloorSpecialTileTypeScript(const Tile *t, int &ret)
{
	ret = t ? t->getFloorSpecialTileType() : TILE;
}

void getObjectSpecialTileTypeScript(const Tile *t, int &ret)
{
	ret = t ? t->getObjectSpecialTileType() : TILE;
}

void getUnitScript(const Tile *t, const BattleUnit*& ret)
{
	ret = t ? t->getUnit() : nullptr;
}

std::string debugDisplayScript(const Tile* t)
{
	if (t)
	{
		std::string s;
		s += Tile::ScriptName;
		s += "(x: ";
		s += std::to_string(t->getPosition().x);
		s += " y: ";
		s += std::to_string(t->getPosition().y);
		s += " z: ";
		s += std::to_string(t->getPosition().z);
		s += " isVoid: ";
		s += t->isVoid() ? "true" : "false";
		if (t->getUnit())
		{
			s += " unit: \"";
			s += t->getUnit()->getType();
			s += "\"";
		}
		s += ")";
		return s;
	}
	else
	{
		return "null";
	}
}



} //namespace

void Tile::ScriptRegister(ScriptParserBase* parser)
{
	Bind<Tile> t = { parser };

	t.add<&getPositionXScript>("getPosition.getX");
	t.add<&getPositionYScript>("getPosition.getY");
	t.add<&getPositionZScript>("getPosition.getZ");
	t.add<&Tile::getFire>("getFire");
	t.add<&Tile::getSmoke>("getSmoke");
	t.add<&Tile::getShade>("getShade");

	t.add<&getUnitScript>("getUnit");

	t.add<&getDistanceTileScript>("getDistanceTile");
	t.add<&getDistanceVoxelScript>("getDistanceVoxel");

	t.add<&getFloorSpecialTileTypeScript>("getFloorSpecialTileType");
	t.add<&getObjectSpecialTileTypeScript>("getObjectSpecialTileType");

	t.addDebugDisplay<&debugDisplayScript>();

	t.addCustomConst("STT_TILE", SpecialTileType::TILE);
	t.addCustomConst("STT_START_POINT", SpecialTileType::START_POINT);
	t.addCustomConst("STT_END_POINT", SpecialTileType::END_POINT);

}

} //namespace OpenXcom
