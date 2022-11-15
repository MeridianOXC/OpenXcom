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
#include <assert.h>
#include <sstream>
#include "BattlescapeGenerator.h"
#include "TileEngine.h"
#include "Inventory.h"
#include "AIModule.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Node.h"
#include "../Savegame/Vehicle.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Engine/Game.h"
#include "../Engine/FileMap.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Exception.h"
#include "../Engine/Logger.h"
#include "../Mod/MapBlock.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/RuleUfo.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/Mod.h"
#include "../Mod/MapData.h"
#include "../Mod/Armor.h"
#include "../Mod/Unit.h"
#include "../Mod/AlienRace.h"
#include "../Mod/RuleEnviroEffects.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleStartingCondition.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/Texture.h"
#include "Pathfinding.h"

namespace OpenXcom
{

/**
 * Sets up a BattlescapeGenerator.
 * @param game pointer to Game object.
 */
BattlescapeGenerator::BattlescapeGenerator(Game *game) :
	_game(game), _save(game->getSavedGame()->getSavedBattle()), _mod(_game->getMod()),
	_craft(0), _craftRules(0), _ufo(0), _base(0), _mission(0), _alienBase(0), _terrain(0), _baseTerrain(0), _globeTerrain(0), _alternateTerrain(0),
	_mapsize_x(0), _mapsize_y(0), _mapsize_z(0), _missionTexture(0), _globeTexture(0), _worldShade(0),
	_unitSequence(0), _craftInventoryTile(0), _alienCustomDeploy(0), _alienCustomMission(0), _alienItemLevel(0), _ufoDamagePercentage(0),
	_baseInventory(false), _generateFuel(true), _craftDeployed(false), _ufoDeployed(false), _craftZ(0), _craftPos(), _markAsReinforcementsBlock(0), _blocksToDo(0), _dummy(0)
{
	_allowAutoLoadout = !Options::disableAutoEquip;
	if (_game->getSavedGame()->getDisableSoldierEquipment())
	{
		_allowAutoLoadout = false;
	}
	_inventorySlotGround = _game->getMod()->getInventoryGround();
}

/**
 * Deletes the BattlescapeGenerator.
 */
BattlescapeGenerator::~BattlescapeGenerator()
{

}

/**
 * Sets up all our various arrays and whatnot according to the size of the map.
 */
void BattlescapeGenerator::init(bool resetTerrain)
{
	_blocks.clear();
	_landingzone.clear();
	_segments.clear();
	_drillMap.clear();
	_verticalLevels.clear();
	_loadedTerrains.clear();
	_verticalLevelSegments.clear();

	_markAsReinforcementsBlock = 0;
	_save->getReinforcementsBlocks().clear();
	_save->getReinforcementsBlocks().resize((_mapsize_x / 10), std::vector<int>((_mapsize_y / 10), 0));

	_save->getFlattenedMapTerrainNames().clear();
	_save->getFlattenedMapTerrainNames().resize((_mapsize_x / 10), std::vector<std::string>((_mapsize_y / 10), ""));
	_save->getFlattenedMapBlockNames().clear();
	_save->getFlattenedMapBlockNames().resize((_mapsize_x / 10), std::vector<std::string>((_mapsize_y / 10), ""));

	_blocks.resize((_mapsize_x / 10), std::vector<MapBlock*>((_mapsize_y / 10)));
	_landingzone.resize((_mapsize_x / 10), std::vector<bool>((_mapsize_y / 10),false));
	_segments.resize((_mapsize_x / 10), std::vector<int>((_mapsize_y / 10),0));
	_drillMap.resize((_mapsize_x / 10), std::vector<int>((_mapsize_y / 10),MD_NONE));

	_blocksToDo = (_mapsize_x / 10) * (_mapsize_y / 10);
	// creates the tile objects
	_save->initMap(_mapsize_x, _mapsize_y, _mapsize_z, resetTerrain);
	_save->initUtilities(_mod);
}

/**
 * Sets the XCom craft involved in the battle.
 * @param craft Pointer to XCom craft.
 */
void BattlescapeGenerator::setCraft(Craft *craft)
{
	_craft = craft;
	_craftRules = _craft->getRules();
	_craft->setInBattlescape(true);
}

/**
 * Sets the ufo involved in the battle.
 * @param ufo Pointer to UFO.
 */
void BattlescapeGenerator::setUfo(Ufo *ufo)
{
	_ufo = ufo;
	_ufo->setInBattlescape(true);
}

/**
 * Sets the world texture where a ufo crashed. This is used to determine the terrain.
 * @param missionTexture Texture id of the polygon on the globe (for UFOs). For mission sites, alien bases, etc. can be something else.
 * @param globeTexture Texture id of the polygon on the globe.
 */
void BattlescapeGenerator::setWorldTexture(Texture *missionTexture, Texture *globeTexture)
{
	_missionTexture = missionTexture;
	_globeTexture = globeTexture;
}

/**
 * Sets the world shade where a ufo crashed. This is used to determine the battlescape light level.
 * @param shade Shade of the polygon on the globe.
 */
void BattlescapeGenerator::setWorldShade(int shade)
{
	if (shade > 15) shade = 15;
	if (shade < 0) shade = 0;
	_worldShade = shade;
}

/**
 * Sets the alien race on the mission. This is used to determine the various alien types to spawn.
 * @param alienRace Alien (main) race.
 */
void BattlescapeGenerator::setAlienRace(const std::string &alienRace)
{
	_alienRace = alienRace;
}

/**
 * Sets the alien item level. This is used to determine how advanced the equipment of the aliens will be.
 * note: this only applies to "New Battle" type games. we intentionally don't alter the month for those,
 * because we're using monthsPassed -1 for new battle in other sections of code.
 * - this value should be from 0 to the size of the itemLevel array in the ruleset (default 9).
 * - at a certain number of months higher item levels appear more and more and lower ones will gradually disappear
 * @param alienItemLevel AlienItemLevel.
 */
void BattlescapeGenerator::setAlienItemlevel(int alienItemLevel)
{
	_alienItemLevel = alienItemLevel;
}

/**
 * Sets the UFO damage percentage. Used to spawn less aliens during base defense.
 * @param ufoDamagePercentage Damage percentage.
 */
void BattlescapeGenerator::setUfoDamagePercentage(int ufoDamagePercentage)
{
	_ufoDamagePercentage = ufoDamagePercentage;
}

/**
 * Set new weapon deploy for aliens that overrides weapon deploy data from mission type/ufo.
 * @param alienCustomDeploy
 */
void BattlescapeGenerator::setAlienCustomDeploy(const AlienDeployment* alienCustomDeploy, const AlienDeployment* alienCustomMission)
{
	_alienCustomDeploy = alienCustomDeploy;
	_alienCustomMission = alienCustomMission;
}

/**
 * Sets the XCom base involved in the battle.
 * @param base Pointer to XCom base.
 */
void BattlescapeGenerator::setBase(Base *base)
{
	_base = base;
	_base->setInBattlescape(true);
}

/**
 * Sets the mission site involved in the battle.
 * @param mission Pointer to mission site.
 */
void BattlescapeGenerator::setMissionSite(MissionSite *mission)
{
	_mission = mission;
	_mission->setInBattlescape(true);
}


/**
 * Switches an existing battlescape savegame to a new stage.
 */
void BattlescapeGenerator::nextStage()
{
	// check if the unit is available in the next stage
	auto isUnitStillActive = [](const BattleUnit* u)
	{
		return !u->isOut() || u->getStatus() == STATUS_UNCONSCIOUS;
	};
	// check if the unit is on the exit tile
	auto isInExit = [s = _save](const BattleUnit* u)
	{
		Tile *tmpTile = s->getTile(u->getPosition());
		return u->isInExitArea(END_POINT) || u->liesInExitArea(tmpTile, END_POINT);
	};

	// preventively drop all units from soldier's inventory (makes handling easier)
	// 1. no alien/civilian living, dead or unconscious is allowed to transition
	// 2. no dead xcom unit is allowed to transition
	// 3. only living or unconscious xcom units can transition
	for (std::vector<BattleUnit*>::iterator unit = _save->getUnits()->begin(); unit != _save->getUnits()->end(); ++unit)
	{
		if ((*unit)->getOriginalFaction() == FACTION_PLAYER && !(*unit)->isOut())
		{
			std::vector<BattleItem*> unitsToDrop;
			for (std::vector<BattleItem*>::iterator item = (*unit)->getInventory()->begin(); item != (*unit)->getInventory()->end(); ++item)
			{
				if ((*item)->getUnit())
				{
					unitsToDrop.push_back(*item);
				}
			}
			for (std::vector<BattleItem*>::iterator corpseItem = unitsToDrop.begin(); corpseItem != unitsToDrop.end(); ++corpseItem)
			{
				_save->getTileEngine()->itemDrop((*unit)->getTile(), (*corpseItem), false);
			}
		}

		// scripts (or some bugs in the game) could make aliens or soldiers that have "unresolved" stun or death state.
		if (!(*unit)->isOut() && (*unit)->isOutThresholdExceed())
		{
			(*unit)->instaFalling();
			if ((*unit)->getTile())
			{
				_save->getTileEngine()->itemDropInventory((*unit)->getTile(), (*unit));
			}

			//spawn corpse/body for unit to recover
			for (int i = (*unit)->getArmor()->getTotalSize() - 1; i >= 0; --i)
			{
				auto corpse = _save->createItemForTile((*unit)->getArmor()->getCorpseBattlescape()[i], nullptr);
				corpse->setUnit((*unit));
				_save->getTileEngine()->itemDrop((*unit)->getTile(), corpse, false);
			}
		}
	}

	int aliensAlive = 0;
	// send all enemy units, or those not in endpoint area (if aborted) to time out
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		(*i)->clearVisibleUnits();
		(*i)->clearVisibleTiles();

		if ((*i)->getStatus() != STATUS_DEAD                              // if they're not dead
			&& (((*i)->getOriginalFaction() == FACTION_PLAYER               // and they're a soldier
			&& _save->isAborted()											  // and you aborted
			&& !isInExit((*i)))                                                     // and they're not on the exit
			|| (*i)->getOriginalFaction() != FACTION_PLAYER))               // or they're not a soldier
		{
			if ((*i)->getOriginalFaction() == FACTION_HOSTILE && !(*i)->isOut())
			{
				if ((*i)->getOriginalFaction() == (*i)->getFaction())
				{
					aliensAlive++;
				}
				else if ((*i)->getTile())
				{
					_save->getTileEngine()->itemDropInventory((*i)->getTile(), (*i));
				}
			}
			(*i)->goToTimeOut();
			if ((*i)->getAIModule())
			{
				(*i)->setAIModule(0);
			}
		}
		(*i)->setFire(0);
		(*i)->setTile(nullptr, _save);
		(*i)->setPosition(TileEngine::invalid, false);
	}

	// send banned units to timeout
	if (_save->getStartingCondition() && !_save->getStartingCondition()->getForbiddenArmorsInNextStage().empty())
	{
		BattleUnit* firstUnitToTimeout = nullptr;
		bool everybodyInTimeout = true;
		for (auto* i : *_save->getUnits())
		{
			if (i->getOriginalFaction() == FACTION_PLAYER && i->getStatus() != STATUS_DEAD && i->getStatus() != STATUS_IGNORE_ME)
			{
				if (i->isBannedInNextStage())
				{
					if (firstUnitToTimeout)
					{
						i->goToTimeOut();
						i->setAIModule(0);
					}
					else
					{
						firstUnitToTimeout = i;
					}
				}
				else
				{
					everybodyInTimeout = false;
				}
			}
		}
		// should all remaining xcom units go to timeout, make one of them unconscious instead (to prevent a crash)
		if (firstUnitToTimeout)
		{
			if (everybodyInTimeout)
			{
				firstUnitToTimeout->healStun(-firstUnitToTimeout->getHealth() * 3);
				//firstUnitToTimeout->instaFalling(); // don't "resolve" the unit status yet (to prevent a crash)
				// drop inventory
				// spawn corpse
				// FIXME: this is basically just an ugly hack, maybe it can be done better? e.g. fail the mission already before calling nextStage() ?
			}
			else
			{
				firstUnitToTimeout->goToTimeOut();
				firstUnitToTimeout->setAIModule(0);
			}
		}
	}

	// remove all items not belonging to our soldiers from the map.
	// sort items into two categories:
	// the ones that we are guaranteed to be able to take home, barring complete failure (ie: stuff on the ship)
	// and the ones that are scattered about on the ground, that will be recovered ONLY on success.
	// this does not include items in your soldier's hands.
	std::vector<BattleItem*> *takeHomeGuaranteed = _save->getGuaranteedRecoveredItems();
	std::vector<BattleItem*> *takeHomeConditional = _save->getConditionalRecoveredItems();
	std::vector<BattleItem*> takeToNextStage, carryToNextStage, removeFromGame, dummyForSpecialWeaponsRemovedElsewhere;

	bool autowin = false;
	if (_save->getChronoTrigger() >= FORCE_WIN && _save->getTurn() > _save->getTurnLimit())
	{
		autowin = true;
	}


	// this will be `getItems` for next stage, allocate same size plus small buffer for new items
	carryToNextStage.reserve(_save->getItems()->size() + 16);

	_save->resetTurnCounter();

	for (std::vector<BattleItem*>::iterator i = _save->getItems()->begin(); i != _save->getItems()->end(); ++i)
	{
		// first off: don't process ammo loaded into weapons. at least not at this level. ammo will be handled simultaneously.
		if (!(*i)->isAmmo())
		{
			std::vector<BattleItem*> *toContainer = &removeFromGame;
			// if it's recoverable, and it's not owned by someone
			if ((((*i)->getUnit() && (*i)->getUnit()->getGeoscapeSoldier()) || (*i)->getRules()->isRecoverable()) && !(*i)->getOwner())
			{
				// first off: don't count primed grenades on the floor
				if ((*i)->getFuseTimer() == -1)
				{
					// protocol 1: all defenders dead, recover all items.
					if (aliensAlive == 0 || autowin)
					{
						// any corpses or unconscious units get put in the skyranger, as well as any unresearched items
						if (((*i)->getUnit() &&
							((*i)->getUnit()->getOriginalFaction() != FACTION_PLAYER ||
							(*i)->getUnit()->getStatus() == STATUS_DEAD))
							|| !_game->getSavedGame()->isResearched((*i)->getRules()->getRequirements()))
						{
							toContainer = takeHomeGuaranteed;
						}
						// otherwise it comes with us to stage two
						else
						{
							toContainer = &takeToNextStage;
						}
					}
					// protocol 2: some of the aliens survived, meaning we ran to the exit zone.
					// recover stuff depending on where it was at the end of the mission.
					else
					{
						Tile *tile = (*i)->getTile();
						if (tile)
						{
							// on a tile at least, so i'll give you the benefit of the doubt on this and give it a conditional recovery at this point
							toContainer = takeHomeConditional;
							if (tile->getMapData(O_FLOOR))
							{
								// in the skyranger? it goes home.
								if (tile->getFloorSpecialTileType() == START_POINT)
								{
									toContainer = takeHomeGuaranteed;
								}
								// on the exit grid? it goes to stage two.
								else if (tile->getFloorSpecialTileType() == END_POINT)
								{
									// apply similar logic (for units) as in protocol 1
									if ((*i)->getUnit() &&
										((*i)->getUnit()->getOriginalFaction() != FACTION_PLAYER ||
										(*i)->getUnit()->getStatus() == STATUS_DEAD))
									{
										toContainer = takeHomeConditional;
									}
									else
									{
										toContainer = &takeToNextStage;
									}
								}
							}
						}
					}
				}
			}

			if ((*i)->isSpecialWeapon())
			{
				if (isUnitStillActive((*i)->getOwner()))
				{
					// the owner of the weapon is still in the game
					toContainer = &carryToNextStage;
				}
				else
				{
					// the item will be deleted (moved to the delete list) by the `removeSpecialWeapons` function, skip all operations in this function
					toContainer = &dummyForSpecialWeaponsRemovedElsewhere;
				}
			}
			else
			{
				if ((*i)->isOwnerIgnored())
				{
					// the unit was set to "timeout" in a previous stage or in this stage
					// in both cases we propagate this item to the next stage even if it will not be accessible
					// "timeout" aliens should have their inventory purged already
					toContainer = &carryToNextStage;
				}
				else if ((*i)->getOwner() && (*i)->getOwner()->getFaction() == FACTION_PLAYER)
				{
					// if a soldier is already holding it, let's let him keep it
					toContainer = &carryToNextStage;
					// Note: if a soldier is recovered at the end, his items will be recovered with him (regardless of whether that soldier was in timeout at the end or not)
				}
			}

			// at this point, we know what happens with the item, so let's apply it to any ammo as well.
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				BattleItem *ammo = (*i)->getAmmoForSlot(slot);
				if (ammo && ammo != *i)
				{
					// break any tile links, because all the tiles are about to disappear.
					ammo->setTile(0);
					toContainer->push_back(ammo);
				}
			}
			// and now the actual item itself.
			(*i)->setTile(0);
			toContainer->push_back(*i);
		}
	}

	// remove special weapons
	for (auto* unit : *_save->getUnits())
	{
		if (!isUnitStillActive(unit))
		{
			unit->removeSpecialWeapons(_save);
		}
	}

	// anything in the "removeFromGame" vector will now be discarded - they're all dead to us now.
	for (std::vector<BattleItem*>::iterator i = removeFromGame.begin(); i != removeFromGame.end();++i)
	{
		// fixed weapons, or anything that's otherwise "equipped" will need to be de-equipped
		// from their owners to make sure we don't have any null pointers to worry about later
		(*i)->moveToOwner(nullptr);
		delete *i;
	}

	// rebuild it with only the items we want to keep active in battle for the next stage
	// here we add all the items that our soldiers are carrying, and we'll add the items on the
	// inventory tile after we've generated our map. everything else will either be in one of the
	// recovery arrays, or deleted from existence at this point.
	std::swap(*_save->getItems(), carryToNextStage);

	_alienCustomDeploy = _game->getMod()->getDeployment(_save->getAlienCustomDeploy());
	_alienCustomMission = _game->getMod()->getDeployment(_save->getAlienCustomMission());

	if (_alienCustomDeploy) _alienCustomDeploy = _game->getMod()->getDeployment(_alienCustomDeploy->getNextStage());
	if (_alienCustomMission) _alienCustomMission = _game->getMod()->getDeployment(_alienCustomMission->getNextStage());

	_save->setAlienCustom(_alienCustomDeploy ? _alienCustomDeploy->getType() : "", _alienCustomMission ? _alienCustomMission->getType() : "");

	const AlienDeployment *ruleDeploy = _alienCustomMission ? _alienCustomMission : _game->getMod()->getDeployment(_save->getMissionType(), true);
	_save->setTurnLimit(ruleDeploy->getTurnLimit());
	_save->setChronoTrigger(ruleDeploy->getChronoTrigger());
	_save->setCheatTurn(ruleDeploy->getCheatTurn());
	ruleDeploy->getDimensions(&_mapsize_x, &_mapsize_y, &_mapsize_z);
	size_t pick = RNG::generate(0, ruleDeploy->getTerrains().size() -1);
	_terrain = _game->getMod()->getTerrain(ruleDeploy->getTerrains().at(pick), true);
	setDepth(ruleDeploy, true);
	_worldShade = ruleDeploy->getShade();

	RuleEnviroEffects* enviro = _game->getMod()->getEnviroEffects(_terrain->getEnviroEffects());
	RuleEnviroEffects* temp = _game->getMod()->getEnviroEffects(ruleDeploy->getEnviroEffects());
	if (temp != 0)
	{
		enviro = temp;
	}
	_save->applyEnviroEffects(enviro);

	// enviro effects - armor transformation (no armor replacement! that is done only before the 1st stage)
	if (enviro != 0)
	{
		for (std::vector<BattleUnit*>::iterator j = _save->getUnits()->begin(); j != _save->getUnits()->end(); ++j)
		{
			if ((*j)->getOriginalFaction() == FACTION_PLAYER && (*j)->getGeoscapeSoldier())
			{
				auto transformedArmor = enviro->getArmorTransformation((*j)->getArmor());
				if (transformedArmor)
				{
					// remember the original armor (i.e. only if there were no transformations in earlier stage(s)!)
					if (!(*j)->getGeoscapeSoldier()->getTransformedArmor())
					{
						(*j)->getGeoscapeSoldier()->setTransformedArmor(_game->getMod()->getArmor((*j)->getArmor()->getType()));
					}
					// change soldier's armor (needed for inventory view!)
					(*j)->getGeoscapeSoldier()->setArmor(transformedArmor);
					// change battleunit's armor
					(*j)->updateArmorFromSoldier(_game->getMod(), (*j)->getGeoscapeSoldier(), transformedArmor, _save->getDepth(), true, _save->getStartingCondition());
					// remove old special built-in weapons and replace them with new fresh special built-in weapons
					// TODO? if this was a limited-use weapon, it will have full ammo again!
					(*j)->removeSpecialWeapons(_save);
					(*j)->setSpecialWeapon(_save, false);
				}
			}
			else if ((*j)->getOriginalFaction() == FACTION_PLAYER)
			{
				// HWPs
				auto transformedArmor = enviro->getArmorTransformation((*j)->getArmor());
				if (transformedArmor)
				{
					// change battleunit's armor
					(*j)->updateArmorFromNonSoldier(_game->getMod(), transformedArmor, _save->getDepth(), _save->getStartingCondition());
				}
			}
		}
	}

	auto& terrainMapScript = _terrain->getRandomMapScript();
	const std::vector<MapScript*> *script = _game->getMod()->getMapScript(terrainMapScript);
	_save->setLastUsedMapScript(terrainMapScript);

	auto& deployMapScript = ruleDeploy->getRandomMapScript();
	if (_game->getMod()->getMapScript(deployMapScript))
	{
		script = _game->getMod()->getMapScript(deployMapScript);
		_save->setLastUsedMapScript(deployMapScript);
	}
	else if (!deployMapScript.empty())
	{
		throw Exception("Map generator encountered an error: " + deployMapScript + " script not found.");
	}
	if (script == 0)
	{
		throw Exception("Map generator encountered an error: " + terrainMapScript + " script not found.");
	}

	// cleanup before map old map is destroyed
	for (auto unit : *_save->getUnits())
	{
		unit->clearVisibleTiles();
		unit->clearVisibleUnits();
	}

	generateMap(script, ruleDeploy->getCustomUfoName(), nullptr);

	setupObjectives(ruleDeploy);

	int highestSoldierID = 0;
	bool selectedFirstSoldier = false;
	int soldiersTotal = 0;
	int soldiersPlaced = 0;
	for (std::vector<BattleUnit*>::iterator j = _save->getUnits()->begin(); j != _save->getUnits()->end(); ++j)
	{
		if ((*j)->getOriginalFaction() == FACTION_PLAYER)
		{
			if (!(*j)->isOut())
			{
				++soldiersTotal;
				(*j)->resetTurnsSinceStunned();
				(*j)->setTurnsSinceSpotted(255);
				(*j)->setTurnsLeftSpottedForSnipers(0);
				if (!selectedFirstSoldier && (*j)->getGeoscapeSoldier())
				{
					_save->setSelectedUnit(*j);
					selectedFirstSoldier = true;
				}
				Node* node = _save->getSpawnNode(NR_XCOM, (*j));
				if (node || placeUnitNearFriend(*j))
				{
					++soldiersPlaced;
					if (node)
					{
						_save->setUnitPosition((*j), node->getPosition());
					}

					if (!_craftInventoryTile)
					{
						_craftInventoryTile = (*j)->getTile();
					}

					(*j)->setInventoryTile(_craftInventoryTile);
					(*j)->setVisible(false);
					if ((*j)->getId() > highestSoldierID)
					{
						highestSoldierID = (*j)->getId();
					}
					//reset TUs, regain energy, etc. but don't take damage or go berserk
					(*j)->prepareNewTurn(false);
				}
			}
		}
	}
	if (soldiersPlaced == 0)
	{
		throw Exception("Map generator encountered an error: no xcom units could be placed on the map.");
	}
	else if (soldiersPlaced < soldiersTotal)
	{
		std::ostringstream oss;
		oss << "Map generator encountered an error: not all xcom units could be placed on the map. Placed: " << soldiersPlaced << " of " << soldiersTotal << ".";
		throw Exception(oss.str());
	}

	if (_save->getSelectedUnit() == 0 || _save->getSelectedUnit()->isOut() || _save->getSelectedUnit()->getFaction() != FACTION_PLAYER)
	{
		_save->selectNextPlayerUnit();
	}
	RuleInventory *ground = _inventorySlotGround;

	for (std::vector<BattleItem*>::iterator i = takeToNextStage.begin(); i != takeToNextStage.end(); ++i)
	{
		_save->getItems()->push_back(*i);
		if ((*i)->getSlot() == ground)
		{
			_craftInventoryTile->addItem(*i, ground);
			if ((*i)->getUnit())
			{
				(*i)->getUnit()->setPosition(_craftInventoryTile->getPosition());
			}
		}
	}

	_unitSequence = _save->getUnits()->back()->getId() + 1;

	// Let's figure out what race we're up against.
	_alienRace = ruleDeploy->getRace();

	for (std::vector<MissionSite*>::iterator i = _game->getSavedGame()->getMissionSites()->begin();
		_alienRace.empty() && i != _game->getSavedGame()->getMissionSites()->end(); ++i)
	{
		if ((*i)->isInBattlescape())
		{
			_alienRace = (*i)->getAlienRace();
		}
	}

	for (std::vector<AlienBase*>::iterator i = _game->getSavedGame()->getAlienBases()->begin();
		_alienRace.empty() && i != _game->getSavedGame()->getAlienBases()->end(); ++i)
	{
		if ((*i)->isInBattlescape())
		{
			_alienRace = (*i)->getAlienRace();
		}
	}

	int civilianSpawnNodeRank = ruleDeploy->getCivilianSpawnNodeRank();
	bool markCiviliansAsVIP = ruleDeploy->getMarkCiviliansAsVIP();

	// Special case: deploy civilians before aliens
	if (civilianSpawnNodeRank > 0)
	{
		deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, ruleDeploy->getCivilians());
		for (std::map<std::string, int>::const_iterator i = ruleDeploy->getCiviliansByType().begin(); i != ruleDeploy->getCiviliansByType().end(); ++i)
		{
			deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, i->second, true, i->first);
		}
	}

	size_t unitCount = _save->getUnits()->size();

	deployAliens(_alienCustomDeploy ? _alienCustomDeploy : ruleDeploy);

	if (unitCount == _save->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}

	// Normal case: deploy civilians after aliens
	if (civilianSpawnNodeRank == 0)
	{
		deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, ruleDeploy->getCivilians());
		for (std::map<std::string, int>::const_iterator i = ruleDeploy->getCiviliansByType().begin(); i != ruleDeploy->getCiviliansByType().end(); ++i)
		{
			deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, i->second, true, i->first);
		}
	}

	_save->setAborted(false);
	setMusic(ruleDeploy, true);
	_save->setGlobalShade(_worldShade);
	_save->getTileEngine()->calculateLighting(LL_AMBIENT, TileEngine::invalid, 0, true);
}

/**
 * Starts the generator; it fills up the battlescape savegame with data.
 */
void BattlescapeGenerator::run()
{
	bool isPreview = _save->isPreview();

	_save->setAlienCustom(_alienCustomDeploy ? _alienCustomDeploy->getType() : "", _alienCustomMission ? _alienCustomMission->getType() : "");

	// Note: this considers also fake underwater UFO deployment (via _alienCustomMission)
	const AlienDeployment *ruleDeploy = _alienCustomMission ? _alienCustomMission : _game->getMod()->getDeployment(_ufo?_ufo->getRules()->getType():_save->getMissionType(), true);

	_save->setTurnLimit(ruleDeploy->getTurnLimit());
	_save->setChronoTrigger(ruleDeploy->getChronoTrigger());
	_save->setCheatTurn(ruleDeploy->getCheatTurn());
	ruleDeploy->getDimensions(&_mapsize_x, &_mapsize_y, &_mapsize_z);

	_unitSequence = BattleUnit::MAX_SOLDIER_ID; // geoscape soldier IDs should stay below this number

	if (_terrain == 0)
	{
		if (_missionTexture == 0 || _missionTexture->getTerrain()->empty() || !ruleDeploy->getTerrains().empty())
		{
			if (!ruleDeploy->getTerrains().empty())
			{
				size_t pick = RNG::generate(0, ruleDeploy->getTerrains().size() - 1);
				_terrain = _game->getMod()->getTerrain(ruleDeploy->getTerrains().at(pick), true);
			}
			else // trouble: no texture and no deployment terrain, most likely scenario is a UFO landing on water: use the first available terrain
			{
				Log(LOG_WARNING) << "Trouble: no texture and no deployment terrain, most likely scenario is a UFO landing on water: using the first available terrain...";
				_terrain = _game->getMod()->getTerrain(_game->getMod()->getTerrainList().front(), true);
			}
		}
		else
		{
			Target *target = _ufo;
			if (_mission) target = _mission;
			_terrain = _game->getMod()->getTerrain(_missionTexture->getRandomTerrain(target), true);
		}
	}

	if (_terrain == 0)
	{
		throw Exception("Map generator encountered an error: No valid terrain found.");
	}

	setDepth(ruleDeploy, false);

	if (ruleDeploy->getShade() != -1)
	{
		_worldShade = ruleDeploy->getShade();
	}
	else if (ruleDeploy->getMinShade() != -1 && _worldShade < ruleDeploy->getMinShade())
	{
		_worldShade = ruleDeploy->getMinShade();
	}
	else if (ruleDeploy->getMaxShade() != -1 && _worldShade > ruleDeploy->getMaxShade())
	{
		_worldShade = ruleDeploy->getMaxShade();
	}

	auto& terrainMapScript = _terrain->getRandomMapScript();
	const std::vector<MapScript*> *script = _game->getMod()->getMapScript(terrainMapScript);
	_save->setLastUsedMapScript(terrainMapScript);

	auto& deployMapScript = ruleDeploy->getRandomMapScript();
	if (_game->getMod()->getMapScript(deployMapScript))
	{
		script = _game->getMod()->getMapScript(deployMapScript);
		_save->setLastUsedMapScript(deployMapScript);
	}
	else if (!deployMapScript.empty())
	{
		throw Exception("Map generator encountered an error: " + deployMapScript + " script not found.");
	}
	if (script == 0)
	{
		throw Exception("Map generator encountered an error: " + terrainMapScript + " script not found.");
	}

	if (isPreview && _craftRules)
	{
		// resize the map if needed
		for (auto* dims : *_craftRules->getBattlescapeTerrainData()->getMapBlocks())
		{
			if (dims->getSizeX() > _mapsize_x)
			{
				_mapsize_x = dims->getSizeX();
			}
			if (dims->getSizeY() > _mapsize_y)
			{
				_mapsize_y = dims->getSizeY();
			}
		}
	}

	RuleStartingCondition* startingCondition = _game->getMod()->getStartingCondition(ruleDeploy->getStartingCondition());
	if (!startingCondition && _missionTexture)
	{
		startingCondition = _game->getMod()->getStartingCondition(_missionTexture->getStartingCondition());
	}
	_save->setStartingCondition(startingCondition);

	generateMap(script, ruleDeploy->getCustomUfoName(), startingCondition);

	if (isPreview && ruleDeploy->isHidden())
	{
		_save->revealMap();
	}

	setupObjectives(ruleDeploy);

	RuleEnviroEffects *enviro = _game->getMod()->getEnviroEffects(ruleDeploy->getEnviroEffects());
	if (!enviro && _terrain)
	{
		enviro = _game->getMod()->getEnviroEffects(_terrain->getEnviroEffects());
	}
	deployXCOM(isPreview ? nullptr : startingCondition, isPreview ? nullptr : enviro);

	int civilianSpawnNodeRank = ruleDeploy->getCivilianSpawnNodeRank();
	bool markCiviliansAsVIP = ruleDeploy->getMarkCiviliansAsVIP();

	// Special case: deploy civilians before aliens
	if (!isPreview && civilianSpawnNodeRank > 0)
	{
		deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, ruleDeploy->getCivilians());
		for (std::map<std::string, int>::const_iterator i = ruleDeploy->getCiviliansByType().begin(); i != ruleDeploy->getCiviliansByType().end(); ++i)
		{
			deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, i->second, true, i->first);
		}
	}

	size_t unitCount = _save->getUnits()->size();

	if (!isPreview)
	{
		deployAliens(_alienCustomDeploy ? _alienCustomDeploy : ruleDeploy);
	}

	if (!isPreview && unitCount == _save->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}

	// Normal case: deploy civilians after aliens
	if (!isPreview && civilianSpawnNodeRank == 0)
	{
		deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, ruleDeploy->getCivilians());
		for (std::map<std::string, int>::const_iterator i = ruleDeploy->getCiviliansByType().begin(); i != ruleDeploy->getCiviliansByType().end(); ++i)
		{
			deployCivilians(markCiviliansAsVIP, civilianSpawnNodeRank, i->second, true, i->first);
		}
	}

	if (!isPreview && _generateFuel)
	{
		fuelPowerSources();
	}

	if (!isPreview && _ufo && _ufo->getStatus() == Ufo::CRASHED)
	{
		explodePowerSources();
	}

	setMusic(ruleDeploy, false);
	// set shade (alien bases are a little darker, sites depend on world shade)
	_save->setGlobalShade(_worldShade);

	_save->getTileEngine()->calculateLighting(LL_AMBIENT, TileEngine::invalid, 0, true);
}

/**
 * Deploys all the X-COM units and equipment based on the Geoscape base / craft.
 */
void BattlescapeGenerator::deployXCOM(const RuleStartingCondition* startingCondition, const RuleEnviroEffects* enviro)
{
	bool isPreview = _save->isPreview();

	_save->applyEnviroEffects(enviro);

	RuleInventory *ground = _inventorySlotGround;

	if (_craft != 0)
	{
		_base = _craft->getBase();
		_craft->resetTemporaryCustomVehicleDeploymentFlags();
	}

	// we will need this during debriefing to show a list of recovered items
	// Note: saved info is required only because of base defense missions, other missions could work without a save too
	// IMPORTANT: the number of vehicles and their ammo has been messed up by Base::setupDefenses() already :( and will need to be handled separately later
	if (!isPreview && _base != 0)
	{
		ItemContainer *rememberMe = _save->getBaseStorageItems();
		for (std::map<std::string, int>::iterator i = _base->getStorageItems()->getContents()->begin(); i != _base->getStorageItems()->getContents()->end(); ++i)
		{
			rememberMe->addItem(i->first, i->second);
		}
	}

	// add vehicles that are in the craft - a vehicle is actually an item, which you will never see as it is converted to a unit
	// however the item itself becomes the weapon it "holds".
	if (!_baseInventory)
	{
		if (_craft != 0)
		{
			for (std::vector<Vehicle*>::iterator i = _craft->getVehicles()->begin(); i != _craft->getVehicles()->end(); ++i)
			{
				RuleItem *item = (*i)->getRules();
				if (startingCondition != 0 && !startingCondition->isVehiclePermitted(item->getType()))
				{
					// send disabled vehicles back to base
					_base->getStorageItems()->addItem(item, 1);
					// ammo too, if necessary
					if (item->getVehicleClipAmmo())
					{
						// Calculate how much ammo needs to be added to the base.
						_base->getStorageItems()->addItem(item->getVehicleClipAmmo(), item->getVehicleClipsLoaded());
					}
				}
				else if (item->getVehicleUnit()->getArmor()->getSize() > 1 || Mod::EXTENDED_HWP_LOAD_ORDER == false)
				{
					// 2x2 HWPs first
					BattleUnit *unit = addXCOMVehicle(*i);
					if (unit && !_save->getSelectedUnit())
						_save->setSelectedUnit(unit);
				}
			}
		}
		else if (_base != 0)
		{
			// add vehicles that are in the base inventory
			for (std::vector<Vehicle*>::iterator i = _base->getVehicles()->begin(); i != _base->getVehicles()->end(); ++i)
			{
				BattleUnit *unit = addXCOMVehicle(*i);
				if (unit && !_save->getSelectedUnit())
					_save->setSelectedUnit(unit);
			}
			// we only add vehicles from the craft in new battle mode,
			// otherwise the base's vehicle vector will already contain these
			// due to the geoscape calling base->setupDefenses()
			if (_game->getSavedGame()->getMonthsPassed() == -1)
			{
				for (std::vector<Craft*>::iterator i = _base->getCrafts()->begin(); i != _base->getCrafts()->end(); ++i)
				{
					for (std::vector<Vehicle*>::iterator j = (*i)->getVehicles()->begin(); j != (*i)->getVehicles()->end(); ++j)
					{
						BattleUnit *unit = addXCOMVehicle(*j);
						if (unit && !_save->getSelectedUnit())
							_save->setSelectedUnit(unit);
					}
				}
			}
		}
	}

	// enviro effects and starting conditions - armor transformation and replacement
	if (startingCondition != 0 || enviro != 0)
	{
		for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
		{
			if ((_craft != 0 && (*i)->getCraft() == _craft) ||
				(_craft == 0 && ((*i)->hasFullHealth() || (*i)->canDefendBase()) && ((*i)->getCraft() == 0 || (*i)->getCraft()->getStatus() != "STR_OUT")))
			{
				Armor* transformedArmor = nullptr;
				if (enviro)
					transformedArmor = enviro->getArmorTransformation((*i)->getArmor());
				if (transformedArmor)
				{
					(*i)->setTransformedArmor((*i)->getArmor());
					(*i)->setArmor(transformedArmor);
				}
				else
				{
					Armor* replacedArmor = nullptr;
					if (startingCondition)
					{
						std::string replacedArmorType = startingCondition->getArmorReplacement((*i)->getRules()->getType(), (*i)->getArmor()->getType());
						replacedArmor = _game->getMod()->getArmor(replacedArmorType, true);
						if (replacedArmor && replacedArmor->getSize() > (*i)->getArmor()->getSize())
						{
							// cannot switch into a bigger armor size!
							replacedArmor = nullptr;
						}
					}
					if (replacedArmor)
					{
						(*i)->setReplacedArmor((*i)->getArmor());
						(*i)->setArmor(replacedArmor);
					}
				}
			}
		}
	}

	// add soldiers that are in the craft or base (2x2 only)
	{
		for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
		{
			if ((*i)->getArmor()->getSize() == 1)
			{
				continue;
			}
			if ((_craft != 0 && (*i)->getCraft() == _craft) ||
				(_craft == 0 && ((*i)->hasFullHealth() || (*i)->canDefendBase()) && ((*i)->getCraft() == 0 || (*i)->getCraft()->getStatus() != "STR_OUT")))
			{
				// clear the soldier's equipment layout, we want to start fresh
				if (_game->getSavedGame()->getDisableSoldierEquipment())
				{
					(*i)->clearEquipmentLayout();
				}
				BattleUnit *unit = addXCOMUnit(new BattleUnit(_game->getMod() , *i, _save->getDepth(), _save->getStartingCondition()));
				if (unit && !_save->getSelectedUnit())
					_save->setSelectedUnit(unit);
			}
		}
	}

	// add remaining 1x1 craft vehicles
	if (!_baseInventory && Mod::EXTENDED_HWP_LOAD_ORDER)
	{
		if (_craft != 0)
		{
			for (std::vector<Vehicle *>::iterator i = _craft->getVehicles()->begin(); i != _craft->getVehicles()->end(); ++i)
			{
				RuleItem *item = (*i)->getRules();
				if (startingCondition != 0 && !startingCondition->isVehiclePermitted(item->getType()))
				{
					// skip, already done earlier
				}
				else if (item->getVehicleUnit()->getArmor()->getSize() == 1)
				{
					// 1x1 HWPs last
					BattleUnit *unit = addXCOMVehicle(*i);
					if (unit && !_save->getSelectedUnit())
						_save->setSelectedUnit(unit);
				}
			}
		}
	}

	// add soldiers that are in the craft or base (1x1 only)
	{
		for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
		{
			if ((*i)->getArmor()->getSize() > 1)
			{
				continue;
			}
			if ((_craft != 0 && (*i)->getCraft() == _craft) ||
				(_craft == 0 && ((*i)->hasFullHealth() || (*i)->canDefendBase()) && ((*i)->getCraft() == 0 || (*i)->getCraft()->getStatus() != "STR_OUT")))
			{
				// clear the soldier's equipment layout, we want to start fresh
				if (_game->getSavedGame()->getDisableSoldierEquipment())
				{
					(*i)->clearEquipmentLayout();
				}
				BattleUnit *unit = addXCOMUnit(new BattleUnit(_game->getMod(), *i, _save->getDepth(), _save->getStartingCondition()));
				if (unit && !_save->getSelectedUnit())
					_save->setSelectedUnit(unit);
			}
		}
	}

	// job's done
	_game->getSavedGame()->setDisableSoldierEquipment(false);

	// Corner case: the base has some soldiers, but nowhere to spawn them (e.g. after a previous base defense destroyed everything but access lift)
	if (_save->getUnits()->empty() && _save->getMissionType() == "STR_BASE_DEFENSE")
	{
		// let's force-spawn a dummy unit and auto-fail the battle
		std::string firstRace = _game->getMod()->getAlienRacesList().front();
		AlienRace* raceRule = _game->getMod()->getAlienRace(firstRace);
		std::string firstUnit = raceRule->getMember(0);
		Unit* unitRule = _game->getMod()->getUnit(firstUnit);
		BattleUnit* unit = _save->createTempUnit(unitRule, FACTION_PLAYER, _unitSequence++);
		unit->setSummonedPlayerUnit(true); // auto-fail battle
		_save->setSelectedUnit(unit);

		Tile* firstTile = _save->getTile(0);
		_save->setUnitPosition(unit, firstTile->getPosition());
		_save->getUnits()->push_back(unit);
		_craftInventoryTile = firstTile;
	}

	if (_save->getUnits()->empty())
	{
		throw Exception("Map generator encountered an error: no xcom units could be placed on the map.");
	}

	// maybe we should assign all units to the first tile of the skyranger before the inventory pre-equip and then reassign them to their correct tile afterwards?
	// fix: make them invisible, they are made visible afterwards.
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER)
		{
			(*i)->setInventoryTile(_craftInventoryTile);
			(*i)->setVisible(false);
		}
	}

	if (isPreview)
	{
		// alright, that's enough
		return;
	}

	if (_craft != 0)
	{
		// add items that are in the craft
		for (std::map<std::string, int>::iterator i = _craft->getItems()->getContents()->begin(); i != _craft->getItems()->getContents()->end(); ++i)
		{
			if (startingCondition != 0 && !startingCondition->isItemPermitted(i->first, _game->getMod(), _craft))
			{
				// send disabled items back to base
				_base->getStorageItems()->addItem(i->first, i->second);
			}
			else
			{
				for (int count = 0; count < i->second; count++)
				{
					_save->createItemForTile(i->first, _craftInventoryTile);
				}
			}
		}
	}
	else
	{
		// only use the items in the craft in new battle mode.
		if (_game->getSavedGame()->getMonthsPassed() != -1)
		{
			// add items that are in the base
			for (std::map<std::string, int>::iterator i = _base->getStorageItems()->getContents()->begin(); i != _base->getStorageItems()->getContents()->end();)
			{
				RuleItem *rule = _game->getMod()->getItem(i->first, true);
				if (
					// is item allowed in base defense?
					rule->canBeEquippedBeforeBaseDefense() &&
					// only put items in the battlescape that make sense (when the item got a sprite, it's probably ok)
					rule->isInventoryItem() &&
					// if item can be equip to craft then you should be able to use it in base defense
					// in some cases we forbid some items from craft but still allow them in base defense if normally they were available.
					(rule->isUsefulBattlescapeItem() || rule->canBeEquippedToCraftInventory()) &&
					// we know how to use this item
					_game->getSavedGame()->isResearched(rule->getRequirements()))
				{
					for (int count = 0; count < i->second; count++)
					{
						_save->createItemForTile(i->first, _craftInventoryTile);
					}
					std::map<std::string, int>::iterator tmp = i;
					++i;
					if (!_baseInventory)
					{
						_base->getStorageItems()->removeItem(tmp->first, tmp->second);
					}
				}
				else
				{
					++i;
				}
			}
		}
		// add items from crafts in base
		for (std::vector<Craft*>::iterator c = _base->getCrafts()->begin(); c != _base->getCrafts()->end(); ++c)
		{
			if ((*c)->getStatus() == "STR_OUT")
				continue;
			for (std::map<std::string, int>::iterator i = (*c)->getItems()->getContents()->begin(); i != (*c)->getItems()->getContents()->end(); ++i)
			{
				for (int count = 0; count < i->second; count++)
				{
					_save->createItemForTile(i->first, _craftInventoryTile);
				}
			}
		}
	}

	std::vector<BattleItem*> tempItemList = *_craftInventoryTile->getInventory();

	// equip soldiers based on equipment-layout
	for (BattleItem* i : tempItemList)
	{
		// set all the items on this tile as belonging to the XCOM faction.
		i->setXCOMProperty(true);
		// don't let the soldiers take extra ammo yet
		if (i->getRules()->getBattleType() == BT_AMMO)
			continue;
		placeItemByLayout(i, tempItemList);
	}

	// load fixed weapons based on equipment layout
	reloadFixedWeaponsByLayout();

	// refresh list
	tempItemList = *_craftInventoryTile->getInventory();

	// load weapons before loadouts take extra clips.
	loadWeapons(tempItemList);

	// refresh list
	tempItemList = *_craftInventoryTile->getInventory();

	for (BattleItem* i : tempItemList)
	{
		// we only need to distribute extra ammo at this point.
		if (i->getRules()->getBattleType() != BT_AMMO)
			continue;
		placeItemByLayout(i, tempItemList);
	}

	// refresh list
	tempItemList = *_craftInventoryTile->getInventory();

	// auto-equip soldiers (only soldiers without layout) and clean up moved items
	autoEquip(*_save->getUnits(), _game->getMod(), &tempItemList, ground, _worldShade, _allowAutoLoadout, false);
}

void BattlescapeGenerator::autoEquip(std::vector<BattleUnit*> units, Mod *mod, std::vector<BattleItem*> *craftInv,
		RuleInventory *groundRuleInv, int worldShade, bool allowAutoLoadout, bool overrideEquipmentLayout)
{
	for (int pass = 0; pass < 4; ++pass)
	{
		for (std::vector<BattleItem*>::iterator j = craftInv->begin(); j != craftInv->end();)
		{
			if ((*j)->getRules()->getInventoryHeight() == 0 || (*j)->getRules()->getInventoryWidth() == 0)
			{
				// don't autoequip hidden items, whatever they are
				++j;
				continue;
			}
			if ((*j)->getSlot() == groundRuleInv)
			{
				bool add = false;

				switch (pass)
				{
				// priority 1: rifles.
				case 0:
					add = (*j)->getRules()->isRifle();
					break;
				// priority 2: pistols (assuming no rifles were found).
				case 1:
					add = (*j)->getRules()->isPistol();
					break;
				// priority 3: ammunition.
				case 2:
					add = (*j)->getRules()->getBattleType() == BT_AMMO;
					break;
				// priority 4: leftovers.
				case 3:
					add = !(*j)->getRules()->isPistol() &&
							!(*j)->getRules()->isRifle() &&
							((*j)->getRules()->getBattleType() != BT_FLARE || worldShade > mod->getMaxDarknessToSeeUnits());
					break;
				default:
					break;
				}

				if (add)
				{
					for (std::vector<BattleUnit*>::iterator i = units.begin(); i != units.end(); ++i)
					{
						if (!(*i)->hasInventory() || !(*i)->getGeoscapeSoldier() || (!overrideEquipmentLayout && !(*i)->getGeoscapeSoldier()->getEquipmentLayout()->empty()))
						{
							continue;
						}
						// let's not be greedy, we'll only take a second extra clip
						// if everyone else has had a chance to take a first.
						bool allowSecondClip = (pass == 3);
						if ((*i)->addItem(*j, mod, allowSecondClip, allowAutoLoadout))
						{
							j = craftInv->erase(j);
							add = false;
							break;
						}
					}
					if (!add)
					{
						continue;
					}
				}
			}
			++j;
		}
	}
}

/**
 * Adds an XCom vehicle to the game.
 * Sets the correct turret depending on the ammo type.
 * @param v Pointer to the Vehicle.
 * @return Pointer to the spawned unit.
 */
BattleUnit *BattlescapeGenerator::addXCOMVehicle(Vehicle *v)
{
	const Unit *rule = v->getRules()->getVehicleUnit();
	BattleUnit *unit = addXCOMUnit(_save->createTempUnit(rule, FACTION_PLAYER, _unitSequence++));
	if (unit)
	{
		if (!_save->isPreview())
		{
			_save->createItemForUnit(v->getRules(), unit, true);
			if (v->getRules()->getVehicleClipAmmo())
			{
				BattleItem *ammoItem = _save->createItemForUnit(v->getRules()->getVehicleClipAmmo(), unit);
				if (ammoItem)
				{
					ammoItem->setAmmoQuantity(v->getAmmo());
				}
			}
		}
		unit->setTurretType(v->getRules()->getTurretType());
	}
	return unit;
}

/**
 * Adds a soldier to the game and places him on a free spawnpoint.
 * Spawnpoints are either tiles in case of an XCom craft that landed.
 * Or they are map nodes in case there's no craft.
 * @param soldier Pointer to the Soldier.
 * @return Pointer to the spawned unit.
 */
BattleUnit *BattlescapeGenerator::addXCOMUnit(BattleUnit *unit)
{
	if (_save->isPreview() && !_save->getCraftForPreview())
	{
		// base preview, one standard unit is enough
		if (_save->getUnits()->size() > 0 && !_save->getUnits()->back()->isSummonedPlayerUnit())
		{
			delete unit;
			return nullptr;
		}
	}

	if (_baseInventory)
	{
		if (unit->hasInventory())
		{
			_save->getUnits()->push_back(unit);
			_save->initUnit(unit);
			return unit;
		}
	}
	else
	{
		if (_craft == 0 || !_craftDeployed)
		{
			Node* node = _save->getSpawnNode(NR_XCOM, unit);
			if (node)
			{
				_save->setUnitPosition(unit, node->getPosition());
				_craftInventoryTile = _save->getTile(node->getPosition());
				unit->setDirection(RNG::generate(0, 7));
				_save->getUnits()->push_back(unit);
				_save->initUnit(unit);
				return unit;
			}
			else if (_save->getMissionType() != "STR_BASE_DEFENSE")
			{
				if (placeUnitNearFriend(unit))
				{
					_craftInventoryTile = _save->getTile(unit->getPosition());
					unit->setDirection(RNG::generate(0, 7));
					_save->getUnits()->push_back(unit);
					_save->initUnit(unit);
					return unit;
				}
			}
		}
		else if (_craft && _craft->hasCustomDeployment() && _craft->getRules() == _craftRules)
		{
			setCustomCraftInventoryTile();

			bool canPlace = false;
			Position pos;
			int dir = 0;
			if (unit->getGeoscapeSoldier())
			{
				auto& depl = _craft->getCustomSoldierDeployment().at(unit->getGeoscapeSoldier()->getId()); // crashes if not found
				pos = depl.first + Position(_craftPos.x * 10, _craftPos.y * 10, _craftZ);
				dir = depl.second;
				canPlace = true;
			}
			else
			{
				auto& deplVec = _craft->getCustomVehicleDeployment();
				for (auto& depl : deplVec)
				{
					if (depl.type == unit->getType() && !depl.used)
					{
						pos = depl.pos + Position(_craftPos.x * 10, _craftPos.y * 10, _craftZ);
						dir = depl.dir;
						canPlace = true;
						depl.used = true; // mark as used, i.e. don't try the same for the next vehicle unit
						break;
					}
				}
			}
			if (!canPlace)
			{
				throw Exception("Unit generator encountered an error: custom craft deployment failed.");
			}
			else
			{
				for (int x = 0; x < unit->getArmor()->getSize(); ++x)
				{
					for (int y = 0; y < unit->getArmor()->getSize(); ++y)
					{
						canPlace = (canPlace && canPlaceXCOMUnit(_save->getTile(pos + Position(x, y, 0))));
					}
				}
			}
			if (canPlace)
			{
				if (_save->setUnitPosition(unit, pos))
				{
					_save->getUnits()->push_back(unit);
					_save->initUnit(unit);
					unit->setDirection(dir);
					return unit;
				}
			}
		}
		else if (_craft && _save->hasCustomDeployment(_craftRules))
		{
			setCustomCraftInventoryTile();

			auto& deploy = _save->getCustomDeployment(_craftRules);
			for (RuleCraftDeployment::const_iterator i = deploy.begin(); i != deploy.end(); ++i)
			{
				Position pos = Position((*i)[0] + (_craftPos.x * 10), (*i)[1] + (_craftPos.y * 10), (*i)[2] + _craftZ);
				int dir = (*i)[3];
				bool canPlace = true;
				for (int x = 0; x < unit->getArmor()->getSize(); ++x)
				{
					for (int y = 0; y < unit->getArmor()->getSize(); ++y)
					{
						canPlace = (canPlace && canPlaceXCOMUnit(_save->getTile(pos + Position(x, y, 0))));
					}
				}
				if (canPlace)
				{
					if (_save->setUnitPosition(unit, pos))
					{
						_save->getUnits()->push_back(unit);
						_save->initUnit(unit);
						unit->setDirection(dir);
						return unit;
					}
				}
			}
		}
		else
		{
			if (_craft)
			{
				setCustomCraftInventoryTile();
			}

			for (int i = 0; i < _mapsize_x * _mapsize_y * _mapsize_z; ++i)
			{
				if (canPlaceXCOMUnit(_save->getTile(i)))
				{
					if (_save->setUnitPosition(unit, _save->getTile(i)->getPosition()))
					{
						_save->getUnits()->push_back(unit);
						_save->initUnit(unit);
						return unit;
					}
				}
			}
		}
	}
	delete unit;
	return 0;
}

/**
 * Tries to set a custom craft inventory tile.
 */
void BattlescapeGenerator::setCustomCraftInventoryTile()
{
	if (_craftInventoryTile == 0)
	{
		// Craft inventory tile position defined in the ruleset
		const std::vector<int> coords = _craftRules->getCraftInventoryTile();
		if (coords.size() >= 3)
		{
			Position craftInventoryTilePosition = Position(coords[0] + (_craftPos.x * 10), coords[1] + (_craftPos.y * 10), coords[2] + _craftZ);
			canPlaceXCOMUnit(_save->getTile(craftInventoryTilePosition));
		}
	}
}

/**
 * Checks if a soldier/tank can be placed on a given tile.
 * @param tile the given tile.
 * @return whether the unit can be placed here.
 */
bool BattlescapeGenerator::canPlaceXCOMUnit(Tile *tile)
{
	// to spawn an xcom soldier, there has to be a tile, with a floor, with the starting point attribute and no object in the way
	if (tile &&
		tile->getFloorSpecialTileType() == START_POINT &&
		!tile->getMapData(O_OBJECT) &&
		tile->getMapData(O_FLOOR) && // for clarity this is checked again, first time was in `getFloorSpecialTileType`
		tile->getMapData(O_FLOOR)->getTUCost(MT_WALK) != Pathfinding::INVALID_MOVE_COST)
	{
		if (_craftInventoryTile == 0)
			_craftInventoryTile = tile;

		return true;
	}
	return false;
}

/**
 * Deploys the aliens, according to the alien deployment rules.
 * @param race Pointer to the alien race.
 * @param deployment Pointer to the deployment rules.
 */
void BattlescapeGenerator::deployAliens(const AlienDeployment *deployment)
{
	// race defined by deployment if there is one.
	auto tmpRace = deployment->getRace();
	if (!tmpRace.empty() && _game->getSavedGame()->getMonthsPassed() > -1)
	{
		_alienRace = tmpRace;
	}

	if (_save->getDepth() > 0 && _alienRace.find("_UNDERWATER") == std::string::npos)
	{
		_alienRace = _alienRace + "_UNDERWATER";
	}

	AlienRace *race = _game->getMod()->getAlienRace(_alienRace);
	if (race == 0)
	{
		throw Exception("Map generator encountered an error: Unknown race: " + _alienRace + " defined in deployment: " + deployment->getType());
	}

	int month;
	if (_game->getSavedGame()->getMonthsPassed() != -1)
	{
		month =
		((size_t) _game->getSavedGame()->getMonthsPassed()) > _game->getMod()->getAlienItemLevels().size() - 1 ?  // if
		_game->getMod()->getAlienItemLevels().size() - 1 : // then
		_game->getSavedGame()->getMonthsPassed() ;  // else
	}
	else
	{
		month = _alienItemLevel;
	}

	// save for later use
	_save->setReinforcementsDeployment(deployment->getType());
	_save->setReinforcementsRace(race->getId());
	_save->setReinforcementsItemLevel(month);
	// and reset memory at each stage
	_save->getReinforcementsMemory().clear();

	for (std::vector<DeploymentData>::const_iterator d = deployment->getDeploymentData()->begin(); d != deployment->getDeploymentData()->end(); ++d)
	{
		int quantity;

		switch (_game->getSavedGame()->getDifficulty())
		{
		case DIFF_BEGINNER:
			quantity = (*d).lowQty;
			break;
		case DIFF_EXPERIENCED:
			quantity = (*d).medQty > 0 ? (*d).lowQty + (((*d).medQty - (*d).lowQty) / 2) : (*d).lowQty;
			break;
		case DIFF_VETERAN:
			quantity = (*d).medQty > 0 ? (*d).medQty : (*d).lowQty + (((*d).highQty - (*d).lowQty) / 2);
			break;
		case DIFF_GENIUS:
			quantity = (*d).medQty > 0 ? (*d).medQty + (((*d).highQty - (*d).medQty) / 2) : (*d).lowQty + (((*d).highQty - (*d).lowQty) / 2);
			break;
		case DIFF_SUPERHUMAN:
		default:
			quantity = (*d).highQty;
		}

		quantity += RNG::generate(0, (*d).dQty);
		quantity += RNG::generate(0, (*d).extraQty);

		for (int i = 0; i < quantity; ++i)
		{
			// if the UFO was damaged, each alien (after the first) has a chance to not spawn (during base defense)
			if (_ufoDamagePercentage > 0)
			{
				if (i > 0 && RNG::percent(_ufoDamagePercentage))
				{
					continue;
				}
			}

			std::string alienName = (*d).customUnitType.empty() ? race->getMember((*d).alienRank) : (*d).customUnitType;

			bool outside = RNG::generate(0,99) < (*d).percentageOutsideUfo;
			if (_ufo == 0)
				outside = false;
			Unit *rule = _game->getMod()->getUnit(alienName, true);
			BattleUnit *unit = addAlien(rule, (*d).alienRank, outside);
			size_t itemLevel = (size_t)(_game->getMod()->getAlienItemLevels().at(month).at(RNG::generate(0,9)));
			if (unit)
			{
				_save->initUnit(unit, itemLevel);
				if (!rule->isLivingWeapon())
				{
					if ((*d).itemSets.empty())
					{
						throw Exception("Unit generator encountered an error: item set not defined");
					}
					if (itemLevel >= (*d).itemSets.size())
					{
						itemLevel = (*d).itemSets.size() - 1;
					}
					for (std::vector<std::string>::const_iterator it = (*d).itemSets.at(itemLevel).items.begin(); it != (*d).itemSets.at(itemLevel).items.end(); ++it)
					{
						RuleItem *ruleItem = _game->getMod()->getItem(*it);
						if (ruleItem)
						{
							_save->createItemForUnit(ruleItem, unit);
						}
					}
					for (auto& iset : (*d).extraRandomItems)
					{
						if (iset.items.empty())
							continue;
						auto pick = RNG::generate(0, iset.items.size() - 1);
						RuleItem *ruleItem = _game->getMod()->getItem(iset.items[pick]);
						if (ruleItem)
						{
							_save->createItemForUnit(ruleItem, unit);
						}
					}
				}
			}
		}
	}
}



/**
 * Adds an alien to the game and places him on a free spawnpoint.
 * @param rules Pointer to the Unit which holds info about the alien .
 * @param alienRank The rank of the alien, used for spawn point search.
 * @param outside Whether the alien should spawn outside or inside the UFO.
 * @return Pointer to the created unit.
 */
BattleUnit *BattlescapeGenerator::addAlien(Unit *rules, int alienRank, bool outside)
{
	BattleUnit *unit = _save->createTempUnit(rules, FACTION_HOSTILE, _unitSequence++);
	Node *node = 0;

	// safety to avoid index out of bounds errors
	if (alienRank > 7)
		alienRank = 7;
	/* following data is the order in which certain alien ranks spawn on certain node ranks */
	/* note that they all can fall back to rank 0 nodes - which is scout (outside ufo) */

	for (int i = 0; i < 7 && node == 0; ++i)
	{
		if (outside)
			node = _save->getSpawnNode(0, unit); // when alien is instructed to spawn outside, we only look for node 0 spawnpoints
		else
			node = _save->getSpawnNode(Node::nodeRank[alienRank][i], unit);
	}

	int aliensFacingCraftOdds = 20 * _game->getSavedGame()->getDifficultyCoefficient();
	{
		int diff = _game->getSavedGame()->getDifficulty();
		auto& custom = _game->getMod()->getAliensFacingCraftOdds();
		if (custom.size() > (size_t)diff)
		{
			aliensFacingCraftOdds = custom[diff];
		}
	}

	if (node && _save->setUnitPosition(unit, node->getPosition()))
	{
		unit->getAIModule()->setStartNode(node);
		unit->setRankInt(alienRank);
		int dir = _save->getTileEngine()->faceWindow(node->getPosition());
		Position craft = _game->getSavedGame()->getSavedBattle()->getUnits()->at(0)->getPosition();
		if (Position::distance2d(node->getPosition(), craft) <= 20 && RNG::percent(aliensFacingCraftOdds))
			dir = unit->directionTo(craft);
		if (dir != -1)
			unit->setDirection(dir);
		else
			unit->setDirection(RNG::generate(0,7));

		// we only add a unit if it has a node to spawn on.
		// (stops them spawning at 0,0,0)
		_save->getUnits()->push_back(unit);
	}
	else
	{
		// DEMIGOD DIFFICULTY: screw the player: spawn as many aliens as possible.
		if (_game->getMod()->isDemigod() && placeUnitNearFriend(unit))
		{
			unit->setRankInt(alienRank);
			int dir = _save->getTileEngine()->faceWindow(unit->getPosition());
			Position craft = _game->getSavedGame()->getSavedBattle()->getUnits()->at(0)->getPosition();
			if (Position::distance2d(unit->getPosition(), craft) <= 20 && RNG::percent(aliensFacingCraftOdds))
				dir = unit->directionTo(craft);
			if (dir != -1)
				unit->setDirection(dir);
			else
				unit->setDirection(RNG::generate(0,7));

			_save->getUnits()->push_back(unit);
		}
		else
		{
			delete unit;
			unit = 0;
		}
	}

	return unit;
}

/**
 * Adds a civilian to the game and places him on a free spawnpoint.
 * @param rules Pointer to the Unit which holds info about the civilian.
 * @return Pointer to the created unit.
 */
BattleUnit *BattlescapeGenerator::addCivilian(Unit *rules, int nodeRank)
{
	BattleUnit *unit = _save->createTempUnit(rules, FACTION_NEUTRAL, _unitSequence++);
	Node *node = _save->getSpawnNode(nodeRank, unit);

	if (node)
	{
		_save->setUnitPosition(unit, node->getPosition());
		unit->getAIModule()->setStartNode(node);
		unit->setDirection(RNG::generate(0,7));

		// we only add a unit if it has a node to spawn on.
		// (stops them spawning at 0,0,0)
		_save->getUnits()->push_back(unit);
	}
	else if (placeUnitNearFriend(unit))
	{
		unit->setDirection(RNG::generate(0,7));
		_save->getUnits()->push_back(unit);
	}
	else
	{
		delete unit;
		unit = 0;
	}
	return unit;
}

/**
 * Places an item on an XCom soldier based on equipment layout.
 * @param item Pointer to the Item.
 * @return Pointer to the Item.
 */
bool BattlescapeGenerator::placeItemByLayout(BattleItem *item, const std::vector<BattleItem*> &itemList)
{
	if (item->getSlot() == _inventorySlotGround)
	{
		auto& itemType = item->getRules()->getType();

		// find the first soldier with a matching layout-slot
		for (auto unit : *_save->getUnits())
		{
			// skip the vehicles, we need only X-Com soldiers WITH equipment-layout
			if (!unit->getGeoscapeSoldier() || unit->getGeoscapeSoldier()->getEquipmentLayout()->empty())
			{
				continue;
			}

			// find the first matching layout-slot which is not already occupied
			for (auto layoutItem : *unit->getGeoscapeSoldier()->getEquipmentLayout())
			{
				// fixed items will be handled elsewhere
				if (layoutItem->isFixed()) continue;

				if (itemType != layoutItem->getItemType()) continue;

				auto inventorySlot = _game->getMod()->getInventory(layoutItem->getSlot(), true);

				// we need to check all "slot boxes" for overlap (not just top left)
				bool overlaps = false;
				for (int x = 0; x < item->getRules()->getInventoryWidth(); ++x)
				{
					for (int y = 0; y < item->getRules()->getInventoryHeight(); ++y)
					{
						if (!overlaps && unit->getItem(inventorySlot, x + layoutItem->getSlotX(), y + layoutItem->getSlotY()))
						{
							overlaps = true;
						}
					}
				}
				if (overlaps) continue;

				auto toLoad = 0;
				for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
				{
					if (layoutItem->getAmmoItemForSlot(slot) != "NONE")
					{
						++toLoad;
					}
				}

				if (toLoad)
				{
					// maybe we find the layout-ammo on the ground to load it with
					for (auto ammo : itemList)
					{
						if (ammo->getSlot() == _inventorySlotGround)
						{
							auto& ammoType = ammo->getRules()->getType();
							for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
							{
								if (ammoType == layoutItem->getAmmoItemForSlot(slot))
								{
									if (item->setAmmoPreMission(ammo))
									{
										--toLoad;
									}
									// even if item was not loaded other slots can't use it either
									break;
								}
							}
							if (!toLoad)
							{
								break;
							}
						}
					}
				}

				// only place the weapon onto the soldier when it's loaded with its layout-ammo (if any)
				if (!toLoad || item->haveAnyAmmo())
				{
					item->moveToOwner(unit);
					item->setSlot(inventorySlot);
					item->setSlotX(layoutItem->getSlotX());
					item->setSlotY(layoutItem->getSlotY());
					if (Options::includePrimeStateInSavedLayout && item->getRules()->getFuseTimerType() != BFT_NONE)
					{
						item->setFuseTimer(layoutItem->getFuseTimer());
					}
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * Reloads fixed weapons on XCom soldiers based on equipment layout.
 */
void BattlescapeGenerator::reloadFixedWeaponsByLayout()
{
	// go through all soldiers
	for (auto unit : *_save->getUnits())
	{
		// skip the vehicles, we need only X-Com soldiers WITH equipment-layout
		if (!unit->getGeoscapeSoldier() || unit->getGeoscapeSoldier()->getEquipmentLayout()->empty())
		{
			continue;
		}

		// find fixed weapons in the layout
		for (auto layoutItem : *unit->getGeoscapeSoldier()->getEquipmentLayout())
		{
			if (layoutItem->isFixed() == false) continue;

			// find matching fixed weapon in the inventory
			BattleItem* fixedItem = nullptr;
			for (auto item : *unit->getInventory())
			{
				if (item->getSlot()->getId() == layoutItem->getSlot() &&
					item->getSlotX() == layoutItem->getSlotX() &&
					item->getSlotY() == layoutItem->getSlotY() &&
					item->getRules()->getType() == layoutItem->getItemType())
				{
					fixedItem = item;
					break;
				}
			}
			if (!fixedItem) continue;

			auto toLoad = 0;
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				if (layoutItem->getAmmoItemForSlot(slot) != "NONE")
				{
					++toLoad;
				}
			}

			if (toLoad)
			{
				// maybe we find the layout-ammo on the ground to load it with
				for (auto ammo : *_craftInventoryTile->getInventory())
				{
					if (ammo->getSlot() == _inventorySlotGround)
					{
						auto& ammoType = ammo->getRules()->getType();
						for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
						{
							if (ammoType == layoutItem->getAmmoItemForSlot(slot))
							{
								if (fixedItem->setAmmoPreMission(ammo))
								{
									--toLoad;
								}
								// even if item was not loaded other slots can't use it either
								break;
							}
						}
						if (!toLoad)
						{
							break;
						}
					}
				}
			}
		}
	}
}

/**
 * Loads an XCom format MAP file into the tiles of the battlegame.
 * @param mapblock Pointer to MapBlock.
 * @param xoff Mapblock offset in X direction.
 * @param yoff Mapblock offset in Y direction.
 * @param save Pointer to the current SavedBattleGame.
 * @param terrain Pointer to the Terrain rule.
 * @param discovered Whether or not this mapblock is discovered (eg. landing site of the XCom plane).
 * @return int Height of the loaded mapblock (this is needed for spawnpoint calculation...)
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 * @note Y-axis is in reverse order.
 */
int BattlescapeGenerator::loadMAP(MapBlock *mapblock, int xoff, int yoff, int zoff, RuleTerrain *terrain, int mapDataSetOffset, bool discovered, bool craft, int ufoIndex)
{
	int sizex, sizey, sizez;
	int x = xoff, y = yoff, z = zoff;
	char size[3];
	unsigned char value[4];
	std::string filename = "MAPS/" + mapblock->getName() + ".MAP";
	unsigned int terrainObjectID;

	// Load file
	auto mapFile = FileMap::getIStream(filename);

	mapFile->read((char*)&size, sizeof(size));
	sizey = (int)size[0];
	sizex = (int)size[1];
	sizez = (int)size[2];

	mapblock->setSizeZ(sizez);

	std::ostringstream ss;
	if (sizez > _save->getMapSizeZ())
	{
		ss << "Height of map " + filename + " too big for this mission, block is " << sizez << ", expected: " << _save->getMapSizeZ();
		throw Exception(ss.str());
	}

	if (sizex != mapblock->getSizeX() ||
		sizey != mapblock->getSizeY())
	{
		ss << "Map block is not of the size specified " + filename + " is " << sizex << "x" << sizey << " , expected: " << mapblock->getSizeX() << "x" << mapblock->getSizeY();
		throw Exception(ss.str());
	}

	z += sizez - 1;

	for (int i = _mapsize_z-1; i >0; i--)
	{
		// check if there is already a layer - if so, we have to move Z up
		MapData *floor = _save->getTile(Position(x, y, i))->getMapData(O_FLOOR);
		if (floor != 0 && zoff == 0)
		{
			z += i;
			if (craft && _craftZ == 0)
			{
				_craftZ = i;
			}
			if (ufoIndex >= 0 && _ufoZ[ufoIndex] == 0)
			{
				_ufoZ[ufoIndex] = i;
			}
			break;
		}
	}

	if (z > (_save->getMapSizeZ()-1))
	{
		if (_save->getMissionType() == "STR_BASE_DEFENSE")
		{
			// we'll already have gone through _base->isOverlappingOrOverflowing() by the time we hit this, possibly multiple times
			// let's just throw an exception and tell them to check the log, it'll have all the detail they'll need.
			throw Exception("Something is wrong with your base, check your log file for additional information.");
		}
		throw Exception("Something is wrong in your map definitions, craft/ufo map is too tall?");
	}

	while (mapFile->read((char*)&value, sizeof(value)))
	{
		for (int part = O_FLOOR; part < O_MAX; ++part)
		{
			terrainObjectID = ((unsigned char)value[part]);
			if (terrainObjectID>0)
			{
				int mapDataSetID = mapDataSetOffset;
				unsigned int mapDataID = terrainObjectID;
				MapData *md = terrain->getMapData(&mapDataID, &mapDataSetID);
				if (mapDataSetOffset > 0) // ie: ufo or craft.
				{
					_save->getTile(Position(x, y, z))->setMapData(0, -1, -1, O_OBJECT);
				}
				TilePart tp = (TilePart) part;
				_save->getTile(Position(x, y, z))->setMapData(md, mapDataID, mapDataSetID, tp);
			}
		}

		_save->getTile(Position(x, y, z))->setDiscovered((discovered || mapblock->isFloorRevealed(z)), O_FLOOR);

		x++;

		if (x == (sizex + xoff))
		{
			x = xoff;
			y++;
		}
		if (y == (sizey + yoff))
		{
			y = yoff;
			z--;
		}
	}

	if (!mapFile->eof())
	{
		throw Exception("Invalid MAP file: " + filename);
	}

	// Add the craft offset to the positions of the items if we're loading a craft map
	// But don't do so if loading a verticalLevel, since the z offset of the craft is handled by that code
	if (craft && zoff == 0)
	{
		zoff += _craftZ;
	}
	// Add the ufo offset to the positions of the items if we're loading a ufo map
	// But don't do so if loading a verticalLevel, since the z offset of the ufo is handled by that code
	if (ufoIndex >= 0 && zoff == 0)
	{
		zoff += _ufoZ[ufoIndex];
	}

	if (_generateFuel)
	{
		// if one of the mapBlocks has an items array defined, don't deploy fuel algorithmically
		_generateFuel = mapblock->getItems()->empty();
	}
	std::map<std::string, std::pair<int, int> >::const_iterator primeEnd = mapblock->getItemsFuseTimers()->end();
	for (std::map<std::string, std::vector<Position> >::const_iterator i = mapblock->getItems()->begin(); i != mapblock->getItems()->end(); ++i)
	{
		std::map<std::string, std::pair<int, int> >::const_iterator prime = mapblock->getItemsFuseTimers()->find((*i).first);
		RuleItem *rule = _game->getMod()->getItem((*i).first, true);
		if (rule->getBattleType() == BT_CORPSE)
		{
			throw Exception("Placing corpse items (battleType: 11) on the map is not allowed. Item: " + rule->getType() + ", map block: " + mapblock->getName());
		}
		for (std::vector<Position>::const_iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
		{
			if ((*j).x >= mapblock->getSizeX() || (*j).y >= mapblock->getSizeY() || (*j).z >= mapblock->getSizeZ())
			{
				ss << "Item " << rule->getType() << " is outside of map block " << mapblock->getName() << ", position: [";
				ss << (*j).x << "," << (*j).y << "," << (*j).z << "], block size: [";
				ss << mapblock->getSizeX() << "," << mapblock->getSizeY() << "," << mapblock->getSizeZ() << "]";
				throw Exception(ss.str());
			}
			BattleItem *item = _save->createItemForTile(rule, _save->getTile((*j) + Position(xoff, yoff, zoff)));
			if (prime != primeEnd)
			{
				item->setFuseTimer(RNG::generate(prime->second.first, prime->second.second));
			}
		}
	}
	// randomized items
	for (std::vector<RandomizedItems>::const_iterator i = mapblock->getRandomizedItems()->begin(); i != mapblock->getRandomizedItems()->end(); ++i)
	{
		if (i->itemList.size() < 1)
		{
			continue; // skip empty definition
		}
		else
		{
			// mixed = false
			int index = RNG::generate(0, i->itemList.size() - 1);
			RuleItem *rule = _game->getMod()->getItem(i->itemList[index], true);

			for (int j = 0; j < i->amount; ++j)
			{
				if (i->mixed)
				{
					// mixed = true
					index = RNG::generate(0, i->itemList.size() - 1);
					rule = _game->getMod()->getItem(i->itemList[index], true);
				}

				if (rule->getBattleType() == BT_CORPSE)
				{
					throw Exception("Placing corpse items (battleType: 11) on the map is not allowed. Item: " + rule->getType() + ", map block: " + mapblock->getName());
				}
				if (i->position.x >= mapblock->getSizeX() || i->position.y >= mapblock->getSizeY() || i->position.z >= mapblock->getSizeZ())
				{
					ss << "Random item " << rule->getType() << " is outside of map block " << mapblock->getName() << ", position: [";
					ss << i->position.x << "," << i->position.y << "," << i->position.z << "], block size: [";
					ss << mapblock->getSizeX() << "," << mapblock->getSizeY() << "," << mapblock->getSizeZ() << "]";
					throw Exception(ss.str());
				}
				_save->createItemForTile(rule, _save->getTile(i->position + Position(xoff, yoff, zoff)));
			}
		}
	}

	return sizez;
}

/**
 * Loads an XCom format RMP file into the spawnpoints of the battlegame.
 * @param mapblock Pointer to MapBlock.
 * @param xoff Mapblock offset in X direction.
 * @param yoff Mapblock offset in Y direction.
 * @param zoff Mapblock vertical offset.
 * @param segment Mapblock segment.
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
void BattlescapeGenerator::loadRMP(MapBlock *mapblock, int xoff, int yoff, int zoff, int segment)
{
	unsigned char value[24];
	std::string filename = "ROUTES/" + mapblock->getName() +".RMP";
	// Load file
	auto mapFile = FileMap::getIStream(filename);

	size_t nodeOffset = _save->getNodes()->size();
	std::vector<int> badNodes;
	int nodesAdded = 0;
	while (mapFile->read((char*)&value, sizeof(value)))
	{
		int pos_x = value[1];
		int pos_y = value[0];
		int pos_z = value[2];
		Node *node;
		if (pos_x >= 0 && pos_x < mapblock->getSizeX() &&
			pos_y >= 0 && pos_y < mapblock->getSizeY() &&
			pos_z >= 0 && pos_z < mapblock->getSizeZ())
		{
			Position pos = Position(xoff + pos_x, yoff + pos_y, mapblock->getSizeZ() - 1 - pos_z + zoff);
			int type     = value[19];
			int rank     = value[20];
			int flags    = value[21];
			int reserved = value[22];
			int priority = value[23];
			node = new Node(_save->getNodes()->size(), pos, segment, type, rank, flags, reserved, priority);
			for (int j = 0; j < 5; ++j)
			{
				int connectID = value[4 + j * 3];
				// don't touch special values
				if (connectID <= 250)
				{
					connectID += nodeOffset;
				}
				// 255/-1 = unused, 254/-2 = north, 253/-3 = east, 252/-4 = south, 251/-5 = west
				else
				{
					connectID -= 256;
				}
				node->getNodeLinks()->push_back(connectID);
			}
		}
		else
		{
			// since we use getNodes()->at(n) a lot, we have to push a dummy node into the vector to keep the connections sane,
			// or else convert the node vector to a map, either way is as much work, so i'm sticking with vector for faster operation.
			// this is because the "built in" nodeLinks reference each other by number, and it's gonna be implementational hell to try
			// to adjust those numbers retroactively, post-culling. far better to simply mark these culled nodes as dummies, and discount their use
			// that way, all the connections will still line up properly in the array.
			node = new Node();
			node->setDummy(true);
			Log(LOG_INFO) << "Bad node in RMP file: " << filename << " Node #" << nodesAdded << " is outside map boundaries at X:" << pos_x << " Y:" << pos_y << " Z:" << pos_z << ". Culling Node.";
			badNodes.push_back(nodesAdded);
		}
		_save->getNodes()->push_back(node);
		nodesAdded++;
	}

	for (std::vector<int>::iterator i = badNodes.begin(); i != badNodes.end(); ++i)
	{
		int nodeCounter = nodesAdded;
		for (std::vector<Node*>::reverse_iterator j = _save->getNodes()->rbegin(); j != _save->getNodes()->rend() && nodeCounter > 0; ++j)
		{
			if (!(*j)->isDummy())
			{
				for(std::vector<int>::iterator k = (*j)->getNodeLinks()->begin(); k != (*j)->getNodeLinks()->end(); ++k)
				{
					if (*k - nodeOffset == (unsigned)*i)
					{
						Log(LOG_INFO) << "RMP file: " << filename << " Node #" << nodeCounter - 1 << " is linked to Node #" << *i << ", which was culled. Terminating Link.";
						*k = -1;
					}
				}
			}
			nodeCounter--;
		}
	}

	if (!mapFile->eof())
	{
		throw Exception("Invalid RMP file: " + filename);
	}
}

/**
 * Checks if a new terrain is being requested by a command and loads it if necessary
 * @param terrain Pointer to the terrain.
 * @return The MCD offset where the requested terrain begins
 */
int BattlescapeGenerator::loadExtraTerrain(RuleTerrain *terrain)
{
	int mapDataSetIDOffset;

	std::map<RuleTerrain*, int>::iterator it = _loadedTerrains.find(terrain);
	if (it != _loadedTerrains.end())
	{
		// Found the terrain in the already-loaded list, get the offset
		mapDataSetIDOffset = it->second;
	}
	else
	{
		// Terrain not loaded yet, go ahead and load it
		mapDataSetIDOffset = _save->getMapDataSets()->size(); // new terrain's offset starts at the end of the already-loaded list

		for (const auto& i : *terrain->getMapDataSets())
		{
			i->loadData(_game->getMod()->getMCDPatch(i->getName()));
			_save->getMapDataSets()->push_back(i);
		}

		_loadedTerrains[terrain] = mapDataSetIDOffset;
	}

	return mapDataSetIDOffset;
}

/**
 * Fill power sources with an alien fuel object.
 */
void BattlescapeGenerator::fuelPowerSources()
{
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		if (_save->getTile(i)->getObjectSpecialTileType() == UFO_POWER_SOURCE)
		{
			_save->createItemForTile(_game->getMod()->getAlienFuelName(), _save->getTile(i));
		}
	}
}


/**
 * When a UFO crashes, there is a 75% chance for each power source to explode.
 */
void BattlescapeGenerator::explodePowerSources()
{
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		if (_save->getTile(i)->getObjectSpecialTileType() == UFO_POWER_SOURCE && RNG::percent(75))
		{
			Position pos;
			pos.x = _save->getTile(i)->getPosition().x*16;
			pos.y = _save->getTile(i)->getPosition().y*16;
			pos.z = (_save->getTile(i)->getPosition().z*24) +12;
			_save->getTileEngine()->explode({ }, pos, 180+RNG::generate(0,70), _save->getMod()->getDamageType(DT_HE), 10);
		}
	}
	Tile *t = _save->getTileEngine()->checkForTerrainExplosions();
	while (t)
	{
		int power = t->getExplosive();
		t->setExplosive(0, 0, true);
		Position p = t->getPosition().toVoxel() + Position(8,8,0);
		_save->getTileEngine()->explode({ }, p, power, _game->getMod()->getDamageType(DT_HE), power / 10);
		t = _save->getTileEngine()->checkForTerrainExplosions();
	}
}

/**
 * Spawns civilians on a terror mission.
 * @param max Maximum number of civilians to spawn.
 */
void BattlescapeGenerator::deployCivilians(bool markAsVIP, int nodeRank, int max, bool roundUp, const std::string &civilianType)
{
	if (max)
	{
		// inevitably someone will point out that ufopaedia says 0-16 civilians.
		// to that person: this is only partially true;
		// 0 civilians would only be a possibility if there were already 80 units,
		// or no spawn nodes for civilians, but it would always try to spawn at least 8.
		int number = RNG::generate(roundUp ? (max+1)/2 : max/2, max);

		if (number > 0)
		{
			int month;
			if (_game->getSavedGame()->getMonthsPassed() != -1)
			{
				month =
				((size_t) _game->getSavedGame()->getMonthsPassed()) > _game->getMod()->getAlienItemLevels().size() - 1 ?  // if
				_game->getMod()->getAlienItemLevels().size() - 1 : // then
				_game->getSavedGame()->getMonthsPassed() ;  // else
			}
			else
			{
				month = _alienItemLevel;
			}
			for (int i = 0; i < number; ++i)
			{
				Unit* rule = 0;
				if (civilianType.size() > 0)
				{
					rule = _game->getMod()->getUnit(civilianType, true);
				}
				else
				{
					size_t pick = RNG::generate(0, _terrain->getCivilianTypes().size() - 1);
					rule = _game->getMod()->getUnit(_terrain->getCivilianTypes().at(pick), true);
				}
				BattleUnit* civ = addCivilian(rule, nodeRank);
				if (civ)
				{
					if (markAsVIP) civ->markAsVIP();
					size_t itemLevel = (size_t)(_game->getMod()->getAlienItemLevels().at(month).at(RNG::generate(0,9)));
					// Built in weapons: civilians may have levelled item lists with randomized distribution
					// following the same basic rules as the alien item levels.
					_save->initUnit(civ, itemLevel);
				}
			}
		}
	}
}

/**
 * Sets the alien base involved in the battle.
 * @param base Pointer to alien base.
 */
void BattlescapeGenerator::setAlienBase(AlienBase *base)
{
	_alienBase = base;
	_alienBase->setInBattlescape(true);
}

/**
 * Places a unit near a friendly unit.
 * @param unit Pointer to the unit in question.
 * @return If we successfully placed the unit.
 */
bool BattlescapeGenerator::placeUnitNearFriend(BattleUnit *unit)
{
	if (_save->getUnits()->empty())
	{
		return false;
	}
	for (int i = 0; i != 10; ++i)
	{
		Position entryPoint = TileEngine::invalid;
		int tries = 100;
		bool largeUnit = false;
		while (entryPoint == TileEngine::invalid && tries)
		{
			BattleUnit* k = _save->getUnits()->at(RNG::generate(0, _save->getUnits()->size()-1));
			if (k->getFaction() == unit->getFaction() && k->getPosition() != TileEngine::invalid && k->getArmor()->getSize() >= unit->getArmor()->getSize())
			{
				entryPoint = k->getPosition();
				largeUnit = k->isBigUnit();
			}
			--tries;
		}
		if (tries && _save->placeUnitNearPosition(unit, entryPoint, largeUnit))
		{
			return true;
		}
	}
	return false;
}


/**
 * Creates a mini-battle-save for managing inventory from the Geoscape.
 * Kids, don't try this at home!
 * @param craft Pointer to craft to manage.
 */
void BattlescapeGenerator::runInventory(Craft *craft)
{
	// we need to fake a map for soldier placement
	_baseInventory = true;
	_mapsize_x = 2;
	_mapsize_y = 2;
	_mapsize_z = 1;
	_save->initMap(_mapsize_x, _mapsize_y, _mapsize_z);
	_save->initUtilities(_mod, true);
	MapDataSet *set = new MapDataSet("dummy");
	MapData *data = new MapData(set);
	_craftInventoryTile = _save->getTile(0);

	// ok now generate the battle items for inventory
	if (craft != 0) setCraft(craft);
	deployXCOM(nullptr, nullptr);
	delete data;
	delete set;
}

/**
 * Loads all XCom weaponry before anything else is distributed.
 */
void BattlescapeGenerator::loadWeapons(const std::vector<BattleItem*> &itemList)
{
	// let's try to load this weapon, whether we equip it or not.
	for (BattleItem* i : itemList)
	{
		if (i->isWeaponWithAmmo() &&
			!i->haveAllAmmo() &&
			!i->getRules()->isFixed())
		{
			for (BattleItem* j : itemList)
			{
				if (j->getSlot() == _inventorySlotGround && i->setAmmoPreMission(j))
				{
					if (i->haveAllAmmo())
					{
						break;
					}
				}
			}
		}
	}
}

/**
 * Generates a map (set of tiles) for a new battlescape game.
 * @param script the script to use to build the map.
 * @param customUfoName custom UFO name to use for the dummy/blank 'addUFO' mapscript command.
 */
void BattlescapeGenerator::generateMap(const std::vector<MapScript*> *script, const std::string &customUfoName, const RuleStartingCondition* startingCondition)
{
	// reset ambient sound
	_save->setAmbientSound(Mod::NO_SOUND);
	_save->setAmbienceRandom({});

	// set our ambient sound
	if (_terrain->getAmbience() != Mod::NO_SOUND)
	{
		_save->setAmbientSound(_terrain->getAmbience());
	}
	else
	{
		_save->setAmbienceRandom(_terrain->getAmbienceRandom());
		_save->setMinAmbienceRandomDelay(_terrain->getMinAmbienceRandomDelay());
		_save->setMaxAmbienceRandomDelay(_terrain->getMaxAmbienceRandomDelay());
		_save->resetCurrentAmbienceDelay();
	}
	_save->setAmbientVolume(_terrain->getAmbientVolume());

	// set up our map generation vars
	_dummy = new MapBlock("dummy");

	init(true);

	MapBlock* craftMap = 0;
	std::vector<MapBlock*> ufoMaps;

	int mapDataSetIDOffset = 0;
	int craftDataSetIDOffset = 0;

	// create an array to track command success/failure
	std::map<int, bool> conditionals;

	// Load in the default terrain data
	for (std::vector<MapDataSet*>::iterator i = _terrain->getMapDataSets()->begin(); i != _terrain->getMapDataSets()->end(); ++i)
	{
		(*i)->loadData(_game->getMod()->getMCDPatch((*i)->getName()));
		_save->getMapDataSets()->push_back(*i);
		mapDataSetIDOffset++;
	}

	_loadedTerrains[_terrain] = 0;

	RuleTerrain* ufoTerrain = 0;
	std::string consolidatedUfoType;
	// lets generate the map now and store it inside the tile objects

	// determine globe terrain from globe texture
	_globeTerrain = _terrain;
	if (_globeTexture && _craft)
	{
		// TODO (cosmetic): multiple attempts (e.g. several attacks on the same alien base) may generate different terrains from the same globe texture
		auto tmpTerrain  = _game->getMod()->getTerrain(_globeTexture->getRandomTerrain(_craft), false);
		if (tmpTerrain)
		{
			_globeTerrain = tmpTerrain;
		}
	}

	// this mission type is "hard-coded" in terms of map layout
	uint64_t seed = RNG::getSeed();
	_baseTerrain = _terrain;
	if (_save->getMissionType() == "STR_BASE_DEFENSE")
	{
		if (_mod->getBaseDefenseMapFromLocation() == 1)
		{
			Target *target = _base;

			// Set RNG to make base generation dependent on base position
			double baseLon = target->getLongitude();
			double baseLat = target->getLatitude();
			if (baseLon == 0)
			{
				baseLon = 1.0;
			}
			if (baseLat == 0)
			{
				baseLat = 1.0;
			}
			uint64_t baseSeed = baseLon * baseLat * 1e6;
			RNG::setSeed(baseSeed);

			_baseTerrain = _game->getMod()->getTerrain(_missionTexture->getRandomBaseTerrain(target), true);
			generateBaseMap();
		}
		else
		{
			generateBaseMap();
		}
	}

	//process script
	for (std::vector<MapScript*>::const_iterator i = script->begin(); i != script->end(); ++i)
	{
		MapScript *command = *i;

		if (command->getLabel() > 0 && conditionals.find(command->getLabel()) != conditionals.end())
		{
			throw Exception("Map generator encountered an error: multiple commands are sharing the same label.");
		}
		bool &success = conditionals[command->getLabel()] = false;


		// if this command runs conditionally on the failures or successes of previous commands
		if (!command->getConditionals()->empty())
		{
			bool execute = true;
			// compare the corresponding entries in the success/failure vector
			for (std::vector<int>::const_iterator condition = command->getConditionals()->begin(); condition != command->getConditionals()->end(); ++condition)
			{
				// positive numbers indicate conditional on success, negative means conditional on failure
				// ie: [1, -2] means this command only runs if command 1 succeeded and command 2 failed.
				if (conditionals.find(std::abs(*condition)) != conditionals.end())
				{
					if ((*condition > 0 && !conditionals[*condition]) || (*condition < 0 && conditionals[std::abs(*condition)]))
					{
						execute = false;
						break;
					}
				}
				else
				{
					throw Exception("Map generator encountered an error: conditional command expected a label that did not exist before this command.");
				}
			}
			if (!execute)
			{
				continue;
			}
		}

		// if there's a chance a command won't execute by design, take that into account here.
		if (RNG::percent(command->getChancesOfExecution()))
		{
			// initialize the block selection arrays
			command->init();

			// each command can be attempted multiple times, as randomization within the rects may occur
			for (int j = 0; j < command->getExecutions(); ++j)
			{
				_ufoDeployed = false; // reset EACH time
				_alternateTerrain = nullptr; // reset

				int x, y;
				MapBlock *block = 0;

				// Check if the command has vertical levels
				_verticalLevels.clear();
				bool doLevels = populateVerticalLevels(command);
				std::vector<VerticalLevel>::iterator currentLevel = _verticalLevels.begin();
				// Special case among verticalLevels, we want to add the blocks using the "line" level, not the ground or the first level
				if (doLevels && command->getType() == MSC_ADDLINE)
				{
					for ( ; currentLevel != _verticalLevels.end(); ++currentLevel)
					{
						if (currentLevel->levelType == VLT_LINE)
						{
							break;
						}
					}

					// How did you pass the populateVerticalLevels validation???
					if (currentLevel == _verticalLevels.end())
					{
						throw Exception("Map generator encountered an error: An addLine command with verticalLevels doesn't contain a level for the line.");
					}
				}

				// Variable for holding positions of blocks added for use in loading vertical levels
				_placedBlockRects.clear();

				RuleTerrain *terrain = _terrain;
				if (!command->getRandomAlternateTerrain().empty())
				{
					size_t pick = RNG::generate(0, command->getRandomAlternateTerrain().size() - 1);
					terrain = pickTerrain(command->getRandomAlternateTerrain().at(pick));
				}
				_alternateTerrain = terrain; // backup for later use

				if (doLevels)
				{
					// initNextLevel() << returns next block for use
					command->initVerticalLevel(*currentLevel);
					terrain = currentLevel->levelTerrain == "" ? terrain : pickTerrain(currentLevel->levelTerrain);
				}

				_markAsReinforcementsBlock = command->markAsReinforcementsBlock() ? 1 : 0;

				switch (command->getType())
				{
				case MSC_ADDBLOCK:
					block = command->getNextBlock(terrain);
					// select an X and Y position from within the rects, using an even distribution
					if (block && selectPosition(command->getRects(), x, y, block->getSizeX(), block->getSizeY()))
					{
						bool blockAdded = addBlock(x, y, block, terrain);
						success = blockAdded || success;

						if (blockAdded && doLevels)
						{
							SDL_Rect blockRect;
							blockRect.x = x;
							blockRect.y = y;
							blockRect.w = block->getSizeX();
							blockRect.h = block->getSizeY();
							_placedBlockRects.push_back(blockRect);

							loadVerticalLevels(command);
						}
					}

					break;
				case MSC_ADDLINE:
					success = addLine((MapDirection)(command->getDirection()), command->getRects(), terrain, command->getVerticalGroup(), command->getHorizontalGroup(), command->getCrossingGroup());

					if (doLevels && success)
					{
						loadVerticalLevels(command, true);
					}

					break;
				case MSC_ADDCRAFT:
					if (_craft)
					{
						const RuleCraft *craftRulesOverride = _save->getMod()->getCraft(command->getCraftName());
						if (startingCondition)
						{
							craftRulesOverride = startingCondition->getCraftReplacement(_craftRules, craftRulesOverride);
						}
						if (craftRulesOverride != 0)
						{
							_craftRules = craftRulesOverride;
						}

						craftMap = _craftRules->getBattlescapeTerrainData()->getRandomMapBlock(999, 999, 0, false);
						bool craftPlaced = addCraft(craftMap, command, _craftPos, terrain);
						// Note: if label != 0, we assume that this may actually fail and will be handled by subsequent command(s)
						if (!craftPlaced && command->getLabel() == 0)
						{
							throw Exception("Map generator encountered an error: Craft (MapBlock: "
								+ craftMap->getName()
								+ ") could not be placed on the map. You may try turning on 'save scumming' to work around this issue.");
						}
						if (craftPlaced)
						{
							// by default addCraft adds blocks from group 1.
							// this can be overwritten in the command by defining specific groups or blocks
							// or this behaviour can be suppressed by leaving group 1 empty
							// this is intentional to allow for TFTD's cruise liners/etc
							// in this situation, you can end up with ANYTHING under your craft, so be careful
							for (x = _craftPos.x; x < _craftPos.x + _craftPos.w; ++x)
							{
								for (y = _craftPos.y; y < _craftPos.y + _craftPos.h; ++y)
								{
									if (_blocks[x][y])
									{
										bool visible = (_save->getMissionType() == "STR_BASE_DEFENSE");

										int terrainMapDataSetIDOffset = loadExtraTerrain(terrain);
										loadMAP(_blocks[x][y], x * 10, y * 10, 0, terrain, terrainMapDataSetIDOffset, visible);

										if (doLevels)
										{
											SDL_Rect blockRect;
											blockRect.x = x;
											blockRect.y = y;
											blockRect.w = _blocks[x][y]->getSizeX();
											blockRect.h = _blocks[x][y]->getSizeY();
											_placedBlockRects.push_back(blockRect);
										}
									}
								}
							}

							if (doLevels)
							{
								loadVerticalLevels(command, true, craftMap);
							}

							_craftDeployed = true;
							success = true;
						}
					}
					break;
				case MSC_ADDUFO:
					// as above, note that the craft and the ufo will never be allowed to overlap.
					// significant difference here is that we accept a UFOName string here to choose the UFO map
					// and we store the UFO positions in a vector, which we iterate later when actually loading the
					// map and route data. this makes it possible to add multiple UFOs to a single map
					// IMPORTANTLY: all the UFOs must use _exactly_ the same MCD set.
					// this is fine for most UFOs but it does mean small scouts can't be combined with larger ones
					// unless some major alterations are done to the MCD sets and maps themselves beforehand
					// this is because serializing all the MCDs is an implementational nightmare from my perspective,
					// and modders can take care of all that manually on their end.
					if (_game->getMod()->getUfo(command->getUFOName()))
					{
						ufoTerrain = _game->getMod()->getUfo(command->getUFOName())->getBattlescapeTerrainData();
						consolidatedUfoType = command->getUFOName();
					}
					else if (_ufo)
					{
						ufoTerrain = _ufo->getRules()->getBattlescapeTerrainData();
						consolidatedUfoType = _ufo->getRules()->getType();
					}
					else if (!customUfoName.empty())
					{
						auto customUfoRule = _game->getMod()->getUfo(customUfoName, true); // crash if it doesn't exist, let the modder know what's going on
						ufoTerrain = customUfoRule->getBattlescapeTerrainData();
						consolidatedUfoType = customUfoName;
					}

					if (ufoTerrain)
					{
						MapBlock *ufoMap = ufoTerrain->getRandomMapBlock(999, 999, 0, false);
						SDL_Rect ufoPosTemp;
						bool ufoPlaced = addCraft(ufoMap, command, ufoPosTemp, terrain);
						// Note: if label != 0, we assume that this may actually fail and will be handled by subsequent command(s)
						if (!ufoPlaced && command->getLabel() == 0)
						{
							if (command->canBeSkipped())
							{
								Log(LOG_INFO) << "Map generator encountered a recoverable problem: UFO (MapBlock: " << ufoMap->getName() << ") could not be placed on the map. Command skipped.";
								break;
							}
							else
							{
								throw Exception("Map generator encountered an error: UFO (MapBlock: "
									+ ufoMap->getName()
									+ ") could not be placed on the map. You may try turning on 'save scumming' to work around this issue.");
							}
						}
						if (ufoPlaced)
						{
							_ufoPos.push_back(ufoPosTemp);
							ufoMaps.push_back(ufoMap);
							for (x = ufoPosTemp.x; x < ufoPosTemp.x + ufoPosTemp.w; ++x)
							{
								for (y = ufoPosTemp.y; y < ufoPosTemp.y + ufoPosTemp.h; ++y)
								{
									if (_blocks[x][y])
									{

										bool visible = (_save->getMissionType() == "STR_BASE_DEFENSE");

										int terrainMapDataSetIDOffset = loadExtraTerrain(terrain);
										loadMAP(_blocks[x][y], x * 10, y * 10, 0, terrain, terrainMapDataSetIDOffset, visible);

										if (doLevels)
										{
											SDL_Rect blockRect;
											blockRect.x = x;
											blockRect.y = y;
											blockRect.w = _blocks[x][y]->getSizeX();
											blockRect.h = _blocks[x][y]->getSizeY();
											_placedBlockRects.push_back(blockRect);
										}
									}
								}
							}

							_ufoZ.push_back(0);

							if (doLevels)
							{
								loadVerticalLevels(command, true, ufoMap);
							}

							_ufoDeployed = true;
							success = true;
						}
					}
					break;
				case MSC_DIGTUNNEL:
					drillModules(command->getTunnelData(), command->getRects(), command->getDirection(), terrain);
					success = true; // this command is fail-proof
					break;
				case MSC_FILLAREA:
					block = command->getNextBlock(terrain);
					while (block)
					{
						if (selectPosition(command->getRects(), x, y, block->getSizeX(), block->getSizeY()))
						{
							// fill area will succeed if even one block is added
							success = addBlock(x, y, block, terrain) || success;

							// Add to a list of rects for fill area to be able to do verticalLevels
							SDL_Rect blockRect;
							blockRect.x = x;
							blockRect.y = y;
							blockRect.w = block->getSizeX();
							blockRect.h = block->getSizeY();
							_placedBlockRects.push_back(blockRect);
						}
						else
						{
							break;
						}
						block = command->getNextBlock(terrain);
					}

					// Now load all the vertical levels
					if(doLevels && _placedBlockRects.size() != 0)
					{
						loadVerticalLevels(command, true);
					}

					break;
				case MSC_CHECKBLOCK:
					for (std::vector<SDL_Rect*>::const_iterator k = command->getRects()->begin(); k != command->getRects()->end() && !success; ++k)
					{
						for (x = (*k)->x; x != (*k)->x + (*k)->w && x != _mapsize_x / 10 && !success; ++x)
						{
							for (y = (*k)->y; y != (*k)->y + (*k)->h && y != _mapsize_y / 10 && !success; ++y)
							{
								if (!command->getGroups()->empty())
								{
									for (std::vector<int>::const_iterator z = command->getGroups()->begin(); z != command->getGroups()->end() && !success; ++z)
									{
										success = _blocks[x][y] && _blocks[x][y]->isInGroup((*z));
									}
								}
								else if (!command->getBlocks()->empty())
								{
									for (std::vector<int>::const_iterator z = command->getBlocks()->begin(); z != command->getBlocks()->end() && !success; ++z)
									{
										if ((size_t)(*z) < terrain->getMapBlocks()->size())
										{
											success = (_blocks[x][y] == terrain->getMapBlocks()->at(*z));
										}
									}
								}
								else
								{
									// wildcard, we don't care what block it is, we just wanna know if there's a block here
									success = (_blocks[x][y] != 0);
								}
							}
						}
					}
					break;
				case MSC_REMOVE:
					success = removeBlocks(command);
					break;
				case MSC_RESIZE:
					if (_save->getMissionType() == "STR_BASE_DEFENSE")
					{
						throw Exception("Map Generator encountered an error: Base defense map cannot be resized.");
					}
					if (_blocksToDo < (_mapsize_x / 10) * (_mapsize_y / 10))
					{
						throw Exception("Map Generator encountered an error: One does not simply resize the map after adding blocks.");
					}

					if (command->getSizeX() > 0 && command->getSizeX() != _mapsize_x / 10)
					{
						_mapsize_x = command->getSizeX() * 10;
					}
					if (command->getSizeY() > 0 && command->getSizeY() != _mapsize_y / 10)
					{
						_mapsize_y = command->getSizeY() * 10;
					}
					if (command->getSizeZ() > 0 && command->getSizeZ() != _mapsize_z)
					{
						_mapsize_z = command->getSizeZ();
					}
					init(false);
					break;
				default:
					break;
				}
			}
		}
	}

	if (_blocksToDo)
	{
		throw Exception("Map failed to fully generate.");
	}

	// Put the map data set ID offset to the end of the terrains in the save since we may have loaded more than the default
	mapDataSetIDOffset = _save->getMapDataSets()->size();

	loadNodes();

	if (!ufoMaps.empty() && ufoTerrain)
	{
		for (std::vector<MapDataSet*>::iterator i = ufoTerrain->getMapDataSets()->begin(); i != ufoTerrain->getMapDataSets()->end(); ++i)
		{
			(*i)->loadData(_game->getMod()->getMCDPatch((*i)->getName()));
			_save->getMapDataSets()->push_back(*i);
			craftDataSetIDOffset++;
		}

		for (size_t i = 0; i < ufoMaps.size(); ++i)
		{
			loadMAP(ufoMaps[i], _ufoPos[i].x * 10, _ufoPos[i].y * 10, _ufoZ[i], ufoTerrain, mapDataSetIDOffset, false, false, i);
			loadRMP(ufoMaps[i], _ufoPos[i].x * 10, _ufoPos[i].y * 10, _ufoZ[i], Node::UFOSEGMENT);
			for (int j = 0; j < ufoMaps[i]->getSizeX() / 10; ++j)
			{
				for (int k = 0; k < ufoMaps[i]->getSizeY() / 10; k++)
				{
					_save->getFlattenedMapTerrainNames()[_ufoPos[i].x + j][_ufoPos[i].y + k] = consolidatedUfoType; // UFO name, not terrain name
					_save->getFlattenedMapBlockNames()[_ufoPos[i].x + j][_ufoPos[i].y + k] = ufoMaps[i]->getName();
					_segments[_ufoPos[i].x + j][_ufoPos[i].y + k] = Node::UFOSEGMENT;
				}
			}
		}
	}

	if (craftMap)
	{
		_craftRules->getBattlescapeTerrainData()->refreshMapDataSets(_craft->getSkinIndex(), _game->getMod()); // change skin if needed
		for (std::vector<MapDataSet*>::iterator i = _craftRules->getBattlescapeTerrainData()->getMapDataSets()->begin(); i != _craftRules->getBattlescapeTerrainData()->getMapDataSets()->end(); ++i)
		{
			(*i)->loadData(_game->getMod()->getMCDPatch((*i)->getName()));
			_save->getMapDataSets()->push_back(*i);
		}
		loadMAP(craftMap, _craftPos.x * 10, _craftPos.y * 10, _craftZ, _craftRules->getBattlescapeTerrainData(), mapDataSetIDOffset + craftDataSetIDOffset, _craftRules->isMapVisible(), true);
		loadRMP(craftMap, _craftPos.x * 10, _craftPos.y * 10, _craftZ, Node::CRAFTSEGMENT);
		for (int i = 0; i < craftMap->getSizeX() / 10; ++i)
		{
			for (int j = 0; j < craftMap->getSizeY() / 10; j++)
			{
				_save->getFlattenedMapTerrainNames()[_craftPos.x + i][_craftPos.y + j] = _craftRules->getType(); // craft name, not terrain name
				_save->getFlattenedMapBlockNames()[_craftPos.x + i][_craftPos.y + j] = craftMap->getName();
				_segments[_craftPos.x + i][_craftPos.y + j] = Node::CRAFTSEGMENT;
			}
		}
		for (int i = (_craftPos.x * 10) - 1; i <= (_craftPos.x * 10) + craftMap->getSizeX(); ++i)
		{
			for (int j = (_craftPos.y * 10) - 1; j <= (_craftPos.y * 10) + craftMap->getSizeY(); j++)
			{
				for (int k = _mapsize_z - 1; k >= _craftZ; --k)
				{
					if (_save->getTile(Position(i, j, k)))
					{
						_save->getTile(Position(i, j, k))->setDiscovered(_craftRules->isMapVisible(), O_FLOOR);
					}
				}
			}
		}
	}

	delete _dummy;

	// special hacks to fill in empty floors on level 0
	for (int x = 0; x < _mapsize_x; ++x)
	{
		for (int y = 0; y < _mapsize_y; ++y)
		{
			if (_save->getTile(Position(x, y, 0))->getMapData(O_FLOOR) == 0)
			{
				_save->getTile(Position(x, y, 0))->setMapData(MapDataSet::getScorchedEarthTile(), 1, 0, O_FLOOR);
			}
		}
	}

	attachNodeLinks();

	if (_save->getMissionType() == "STR_BASE_DEFENSE" && _mod->getBaseDefenseMapFromLocation() == 1)
	{
		RNG::setSeed(seed);
	}
}

/**
 * Generates a map based on the base's layout.
 * this doesn't drill or fill with dirt, the script must do that.
 */
void BattlescapeGenerator::generateBaseMap()
{
	// add modules based on the base's layout
	for (std::vector<BaseFacility*>::const_iterator i = _base->getFacilities()->begin(); i != _base->getFacilities()->end(); ++i)
	{
		if ((*i)->isBuiltOrHadPreviousFacility())
		{
			int num = 0;
			int xLimit = (*i)->getX() + (*i)->getRules()->getSize() -1;
			int yLimit = (*i)->getY() + (*i)->getRules()->getSize() -1;

			// Do we use the normal method for placing items on the ground or an explicit definition?
			bool storageCheckerboard = ((*i)->getRules()->getStorageTiles().size() == 0);

			// If the facility has a verticalLevels ruleset definition, load it as if it's an addBlock command
			if ((*i)->getRules()->getVerticalLevels().size() != 0)
			{
				// Create a mapscript command to handle picking blocks for the vertical level
				MapScript command;

				// Get the vertical levels from the facility ruleset and create a list according to map size
				_verticalLevels.clear();
				_verticalLevels = (*i)->getRules()->getVerticalLevels();
				command.setVerticalLevels(_verticalLevels, (*i)->getRules()->getSize());
				populateVerticalLevels(&command);

				std::vector<VerticalLevel>::iterator currentLevel = _verticalLevels.begin();
				command.initVerticalLevel(*currentLevel);
				RuleTerrain *terrain;
				terrain = currentLevel->levelTerrain == "" ? _terrain : pickTerrain(currentLevel->levelTerrain);
				_placedBlockRects.clear();

				MapBlock *block = command.getNextBlock(terrain);
				if (block)
				{
					int x = (*i)->getX();
					int y = (*i)->getY();
					addBlock(x, y, block, terrain);

					SDL_Rect blockRect;
					blockRect.x = x;
					blockRect.y = y;
					blockRect.w = block->getSizeX();
					blockRect.h = block->getSizeY();
					_placedBlockRects.push_back(blockRect);

					_alternateTerrain = nullptr; // not used in this case
					loadVerticalLevels(&command);
				}
				else
				{
					throw Exception("Map generator encountered an error: facility " + (*i)->getRules()->getType() + " has no block on first verticalLevel.");
				}
			}

			for (int y = (*i)->getY(); y <= yLimit; ++y)
			{
				for (int x = (*i)->getX(); x <= xLimit; ++x)
				{
					// lots of crazy stuff here, which is for the hangars (or other large base facilities one may create)
					// TODO: clean this mess up, make the mapNames a vector in the base module defs
					// also figure out how to do the terrain sets on a per-block basis.

					// Only use the mapName if we didn't load the map by verticalLevels
					if ((*i)->getRules()->getVerticalLevels().size() == 0)
					{
						std::string mapname = (*i)->getRules()->getMapName();
						std::ostringstream newname;
						newname << mapname.substr(0, mapname.size()-2); // strip of last 2 digits
						int mapnum = atoi(mapname.substr(mapname.size()-2, 2).c_str()); // get number
						mapnum += num;
						if (mapnum < 10) newname << 0;
						newname << mapnum;
						auto block = _terrain->getMapBlock(newname.str());
						if (!block)
						{
							throw Exception("Map generator encountered an error: map block "
								+ newname.str()
								+ " is not defined in terrain "
								+ _terrain->getName()
								+ ".");
						}
						addBlock(x, y, block, _terrain);
					}
					_drillMap[x][y] = MD_NONE;
					num++;
					if ((*i)->getRules()->getStorage() > 0 && storageCheckerboard)
					{
						int groundLevel;
						for (groundLevel = _mapsize_z - 1; groundLevel >= 0; --groundLevel)
						{
							if (!_save->getTile(Position(x * 10, y * 10, groundLevel))->hasNoFloor(0))
								break;
						}
						// general stores - there is where the items are put
						for (int k = x * 10; k != (x + 1) * 10; ++k)
						{
							for (int l = y * 10; l != (y + 1) * 10; ++l)
							{
								// we only want every other tile, giving us a "checkerboard" pattern
								if ((k + l) % 2 == 0)
								{
									Tile *t = _save->getTile(Position(k, l, groundLevel));
									Tile *tEast = _save->getTile(Position(k + 1, l, groundLevel));
									Tile *tSouth = _save->getTile(Position(k, l + 1, groundLevel));
									if (t && t->getMapData(O_FLOOR) && !t->getMapData(O_OBJECT) &&
										tEast && !tEast->getMapData(O_WESTWALL) &&
										tSouth && !tSouth->getMapData(O_NORTHWALL))
									{
										_save->getStorageSpace().push_back(Position(k, l, groundLevel));
									}
								}
							}
						}
						// let's put the inventory tile on the lower floor, just to be safe.
						if (!_craftInventoryTile)
						{
							_craftInventoryTile = _save->getTile(Position((x * 10) + 5, (y * 10) + 5, groundLevel - 1));
						}
					}
				}
			}

			// Extended handling for placing storage tiles by ruleset definition
			if ((*i)->getRules()->getStorage() > 0 && !storageCheckerboard)
			{
				int x = (*i)->getX();
				int y = (*i)->getY();
				bool validPos = false;
				for (std::vector<Position>::const_iterator j = (*i)->getRules()->getStorageTiles().begin(); j != (*i)->getRules()->getStorageTiles().end(); ++j)
				{
					if (*j == TileEngine::invalid)
					{
						validPos = true;
						break;
					}

					if (j->x < 0 || j->x / 10 > (*i)->getRules()->getSize()
						|| j->y < 0 || j->y / 10 > (*i)->getRules()->getSize()
						|| j->z < 0 || j->z > _mapsize_z)
					{
						Log(LOG_ERROR) << "Tile position " << (*j) << " is outside the facility " << (*i)->getRules()->getType() << ", skipping placing items there.";
						continue;
					}

					Position tilePos = Position((x * 10) + j->x, (y * 10) + j->y, j->z);
					if (!_save->getTile(tilePos))
					{
						Log(LOG_ERROR) << "Tile position " << tilePos << ", from the facility " << (*i)->getRules()->getType() << ", is outside the map; skipping placing items there.";
						continue;
					}

					validPos = true;
					_save->getStorageSpace().push_back(tilePos);

					if (!_craftInventoryTile) // just to be safe, make sure we have a craft inventory tile
					{
						_craftInventoryTile = _save->getTile(tilePos);
					}
				}

				// Crash gracefully with some information before we spawn a map where no items could be placed.
				if (!validPos)
				{
					throw Exception("Could not place items on given tiles in storage facility " + (*i)->getRules()->getType());
				}
			}

			for (int x = (*i)->getX(); x <= xLimit; ++x)
			{
				_drillMap[x][yLimit] = MD_VERTICAL;
			}
			for (int y = (*i)->getY(); y <= yLimit; ++y)
			{
				_drillMap[xLimit][y] = MD_HORIZONTAL;
			}
			_drillMap[xLimit][yLimit] = MD_BOTH;
		}
	}
	_save->calculateModuleMap();

}

/**
 * Populates the verticalLevels vector
 * @param command the mapscript command from which to get the size
 * @return whether or not the command has levels
 */
bool BattlescapeGenerator::populateVerticalLevels(MapScript *command)
{
	_verticalLevels.clear();

	if (command->getVerticalLevels().size() == 0)
	{
		return false;
	}

	// First check for ground level
	bool levelFound = false;
	for (auto &i : command->getVerticalLevels())
	{
		if (i.levelType == VLT_GROUND)
		{
			if (!levelFound)
			{
				_verticalLevels.push_back(i);
				// For consistency with the standard addCraft/addUFO commands, set default groups to 1 (LZ) if not defined
				if ((command->getType() == MSC_ADDCRAFT || command->getType() == MSC_ADDUFO) && i.levelGroups.size() == 0)
				{
					_verticalLevels.back().levelGroups.push_back(1);
				}
				levelFound = true;
			}
			else
			{
				Log(LOG_WARNING) << "Map generator encountered an error: a 'ground' verticalLevel can only be used once per command.";
			}
		}
	}

	// Next, find the "filler" levels
	levelFound = false;
	for (auto &i : command->getVerticalLevels())
	{
		switch (i.levelType)
		{
			case VLT_CRAFT:
				if ((command->getType() == MSC_ADDCRAFT || command->getType() == MSC_ADDUFO) && !levelFound)
				{
					_verticalLevels.push_back(i);
					_verticalLevels.back().maxRepeats = 1;
					levelFound = true;
				}
				else if (command->getType() == MSC_ADDCRAFT || command->getType() == MSC_ADDUFO)
				{
					Log(LOG_WARNING) << "Map generator encountered an error: a 'craft' verticalLevel can only be used once per command.";
				}
				else
				{
					Log(LOG_WARNING) << "Map generator encountered an error: a 'craft' verticalLevel is only valid for addCraft and addUFO commands, skipping level.";
				}

				break;

			case VLT_LINE:
				if (command->getType() == MSC_ADDLINE && !levelFound)
				{
					_verticalLevels.push_back(i);
					levelFound = true;
				}
				else if (command->getType() == MSC_ADDLINE)
				{
					Log(LOG_WARNING) << "Map generator encountered an error: a 'line' verticalLevel can only be used once per command.";
				}
				else
				{
					Log(LOG_WARNING) << "Map generator encountered an error: a 'line' verticalLevel is only valid for addLine commands, skipping level.";
				}

				break;

			case VLT_MIDDLE:
			case VLT_EMPTY:
			case VLT_DECORATION:
				_verticalLevels.push_back(i);

				break;

			default :

				break;
		}
	}

	// Finally, add the "ceiling" level
	levelFound = false;
	for (auto &i : command->getVerticalLevels())
	{
		if (i.levelType == VLT_CEILING)
		{
			if (!levelFound)
			{
				_verticalLevels.push_back(i);
				levelFound = true;
			}
			else
			{
				Log(LOG_WARNING) << "Map generator encountered an error: a 'ceiling' verticalLevel can only be used once per command.";
			}
		}
	}

	return true;
}

/**
 * Gets a terrain from a terrain name for a command or vertical level, validates if there is no terrain name
 * @param terrainName the name of the terrain to look for.
 * @return pointer to the terrain
 */
RuleTerrain* BattlescapeGenerator::pickTerrain(std::string terrainName)
{
	RuleTerrain *terrain;

	if (terrainName == "baseTerrain")
	{
		terrain = _baseTerrain;
	}
	else if (terrainName == "globeTerrain")
	{
		terrain = _globeTerrain;
	}
	else if (terrainName != "")
	{
		//get the terrain according to the string name
		terrain = _game->getMod()->getTerrain(terrainName);
		if (!terrain)
		{
			// make sure we get a terrain, and put an error in the log, continuing with generation
			Log(LOG_ERROR) << "Map generator could not find alternate terrain " << terrainName << ", proceeding with terrain from alienDeployments or Geoscape texture.";
			terrain = _terrain;
		}
	}
	else
	{
		// no alternate terrain in command or verticalLevel, default to deployment terrain
		terrain = _terrain;
	}

	return terrain;
}

/**
 * Loads maps from verticalLevels within a mapscript command
 * @param command the mapScript command.
 * @param terrain pointer to the terrain for the the mapScript command
 */
void BattlescapeGenerator::loadVerticalLevels(MapScript *command, bool repopulateLevels, MapBlock *craftMap)
{
	for (std::vector<SDL_Rect>::iterator i = _placedBlockRects.begin(); i != _placedBlockRects.end(); ++i)
	{
		// If we're using a command that adds multiple blocks per single execution, we need to make sure the _verticalLevels vector gets repopulated after each iteration.
		if (repopulateLevels)
		{
			populateVerticalLevels(command);
		}

		int x = i->x;
		int y = i->y;
		MapBlock *block = _blocks[x][y];

		std::vector<VerticalLevel>::iterator currentLevel = _verticalLevels.begin();
		RuleTerrain *terrain = _terrain;
		if (_alternateTerrain)
		{
			terrain = _alternateTerrain; // alternate terrain defined on the map script command
		}

		int zOffset = 0;
		int zLevelsLeft = command->getSizeZ();
		if (zLevelsLeft < 1)
		{
			zLevelsLeft = _mapsize_z;
		}
		else if (zLevelsLeft > _mapsize_z)
		{
			Log(LOG_WARNING) << "Battlescape Generator has encountered an error: a mapscript command has height " << command->getSizeZ() << " while the map is only " << _mapsize_z << " tiles tall.";
			Log(LOG_WARNING) << "Reducing command size to map height";

			zLevelsLeft = _mapsize_z;
		}

		// Special handling for addline command: we need to check if the line is on ground level. If not, we need to re-load the z = 0 level and load the line later
		MapBlock *lineBlock = 0;
		if (command->getType() == MSC_ADDLINE && currentLevel->levelType != VLT_LINE)
		{
			lineBlock = _blocks[x][y];

			clearModule(x*10, y*10, 10, 10);
			++_blocksToDo; // this will be re-added with addBlock
			command->initVerticalLevel(*currentLevel);
			RuleTerrain *levelTerrain = pickTerrain(currentLevel->levelTerrain);
			block = command->getNextBlock(levelTerrain);

			if (block)
			{
				addBlock(x, y, block, levelTerrain);
			}
			else
			{
				Log(LOG_WARNING) << "Battlescape Generator has encountered an error: an addLine mapscript command has vertical levels but a mapblock for the ground level could not be loaded.";
			}
		}

		// For the first level, "ground" or "line" should only done once, so remove it. Otherwise, just advance the iterator
		int z = currentLevel->levelSizeZ == -1 ? block->getSizeZ() : currentLevel->levelSizeZ;
		zOffset += z;
		zLevelsLeft -= z;
		if (currentLevel->levelType == VLT_GROUND || currentLevel->levelType == VLT_LINE)
		{
			currentLevel = _verticalLevels.erase(currentLevel);
		}
		else
		{
			++currentLevel;
		}

		// Reserve the space for the ceiling level
		MapBlock *ceilingBlock = 0;
		RuleTerrain *ceilingTerrain = terrain;
		if (_verticalLevels.back().levelType == VLT_CEILING)
		{
			command->initVerticalLevel(_verticalLevels.back());
			ceilingTerrain = _verticalLevels.back().levelTerrain == "" ? terrain : pickTerrain(_verticalLevels.back().levelTerrain);
			ceilingBlock = command->getNextBlock(ceilingTerrain);
			if (ceilingBlock)
			{
				z = _verticalLevels.back().levelSizeZ == -1 ? ceilingBlock->getSizeZ() : _verticalLevels.back().levelSizeZ;
				if (z > zLevelsLeft)
				{
					ceilingBlock = 0;
				}
				else
				{
					zLevelsLeft -= z;
					_verticalLevels.pop_back();
				}
			}
		}

		// Now load the rest of the levels
		int tries = 0;
		int maxTries = 20;

		// If a addLine command for some reason can't fit in the line itself, note this in the log for the modder to fix
		bool lineAdded = false;

		// Loop over the "filling" levels to build up the maps placed at this location
		while (_verticalLevels.size() > 0 && zLevelsLeft > 0 && tries <= maxTries)
		{
			// Since our goal is filling the space between the bottom and the max height, we repeat the list of levels until we're done
			if (currentLevel == _verticalLevels.end())
				currentLevel = _verticalLevels.begin();

			// Determine what we're doing with the current level
			RuleTerrain *levelTerrain = terrain;
			block = 0;
			z = 0;

			switch (currentLevel->levelType)
			{
				case VLT_CRAFT:
					// Set the craft height offset if necessary
					if (command->getType() == MSC_ADDCRAFT && zOffset > _craftZ)
					{
						_craftZ = zOffset;
					}
					else if (command->getType() == MSC_ADDUFO && zOffset > _ufoZ.back())
					{
						_ufoZ.back() = zOffset;
					}

					z = currentLevel->levelSizeZ == -1 ? craftMap->getSizeZ() : currentLevel->levelSizeZ;

					if (z > zLevelsLeft)
					{
						throw Exception("Map Generator has encountered an error: craft or ufo map"
							+ craftMap->getName()
							+ "could not be placed because either the map is too tall or the verticalLevels place it too high.");
					}

					break;

				case VLT_EMPTY:
					// We're just bumping the zOffset upwards
					z = currentLevel->levelSizeZ == -1 ? 1 : currentLevel->levelSizeZ;

					break;

				default:
					// All other level types: we're loading a map block
					if (currentLevel->levelTerrain != "")
					{
						levelTerrain = pickTerrain(currentLevel->levelTerrain);
					}

					if (command->getType() == MSC_ADDLINE && currentLevel->levelType == VLT_LINE && lineBlock)
					{
						// We're re-loading a block for the addLine command
						block = lineBlock;
						lineAdded = true;
					}
					else
					{
						// We're loading a new block
						command->initVerticalLevel(*currentLevel);
						block = command->getNextBlock(levelTerrain);
					}

					if(block)
					{
						z = currentLevel->levelSizeZ == -1 ? block->getSizeZ() : currentLevel->levelSizeZ;
					}

					break;
			}

			// Apply the current level if it fits before we reach the max height, otherwise try again with the current level
			if (z <= zLevelsLeft)
			{
				// Load a map if that's what we're doing
				if (block)
				{
					bool visible = (_save->getMissionType() == "STR_BASE_DEFENSE");

					int terrainMapDataSetIDOffset = loadExtraTerrain(levelTerrain);
					loadMAP(block, x * 10, y * 10, zOffset, levelTerrain, terrainMapDataSetIDOffset, visible);
					_verticalLevelSegments.push_back(std::make_pair(block, Position(x, y, zOffset)));
				}

				zLevelsLeft -= z;
				zOffset += z;

				// Move to the next level in the list and remove the current one if we've used it up while looping
				--currentLevel->maxRepeats;
				if (currentLevel->maxRepeats == 0)
				{
					currentLevel = _verticalLevels.erase(currentLevel);
				}
				else
				{
					++currentLevel;
				}
			}
			else
			{
				++tries;
			}
		}

		if (command->getType() == MSC_ADDLINE && !lineAdded)
		{
			Log(LOG_WARNING) << "Battlescape Generator has encountered an error: a addLine mapscript command with vertical levels did not load the line itself. The modder may want to reduce the number of levels before the line.";
		}

		// Now we can finally load the ceiling level
		if (ceilingBlock)
		{
			bool visible = (_save->getMissionType() == "STR_BASE_DEFENSE");

			int terrainMapDataSetIDOffset = loadExtraTerrain(ceilingTerrain);
			loadMAP(ceilingBlock, x * 10, y * 10, zOffset, ceilingTerrain, terrainMapDataSetIDOffset, visible);
			_verticalLevelSegments.push_back(std::make_pair(ceilingBlock, Position(x, y, zOffset)));
		}
	}
}

/**
 * Clears a module from the map.
 * @param x the x offset.
 * @param y the y offset.
 * @param sizeX how far along the x axis to clear.
 * @param sizeY how far along the y axis to clear.
 */
void BattlescapeGenerator::clearModule(int x, int y, int sizeX, int sizeY)
{
	for (int z = 0; z != _mapsize_z; ++z)
	{
		for (int dx = x; dx != x + sizeX; ++dx)
		{
			for (int dy = y; dy != y + sizeY; ++dy)
			{
				Tile *tile = _save->getTile(Position(dx,dy,z));
				for (int i = O_FLOOR; i < O_MAX; i++)
					tile->setMapData(0, -1, -1, (TilePart)i);
			}
		}
	}
}

/**
 * Loads all the nodes from the map modules.
 */
void BattlescapeGenerator::loadNodes()
{
	int segment = 0;

	// Loading nodes for ground-level maps
	for (int itY = 0; itY < (_mapsize_y / 10); itY++)
	{
		for (int itX = 0; itX < (_mapsize_x / 10); itX++)
		{
			_segments[itX][itY] = segment;
			if (_blocks[itX][itY] != 0 && _blocks[itX][itY] != _dummy)
			{
				if (!(_blocks[itX][itY]->isInGroup(MT_LANDINGZONE) && _landingzone[itX][itY] && _craftZ == 0))
				{
					loadRMP(_blocks[itX][itY], itX * 10, itY * 10, 0, segment++);
				}
				}
			}
		}

	// Loading nodes for verticalLevels maps
	for (std::vector<std::pair<MapBlock*, Position> >::iterator i = _verticalLevelSegments.begin(); i != _verticalLevelSegments.end(); ++i)
	{
		loadRMP(i->first, i->second.x * 10, i->second.y * 10, i->second.z, segment++);
	}
}

/**
 * Attaches all the nodes together in an intricate web of lies.
 */
void BattlescapeGenerator::attachNodeLinks()
{
	// Since standard vanilla maps are added first, the cutoff between nodes in
	// _segments and those loaded in verticalLevels is the largest value of _segements
	int lastSegmentsIndex = 0;
	for (int itY = 0; itY < (_mapsize_y / 10); itY++)
	{
		for (int itX = 0; itX < (_mapsize_x / 10); itX++)
		{
			if(_segments[itX][itY] > lastSegmentsIndex)
			{
				lastSegmentsIndex = _segments[itX][itY];
			}
		}
	}

	// First pass is original code, connects all ground-level maps
	for (std::vector<Node*>::iterator i = _save->getNodes()->begin(); i != _save->getNodes()->end(); ++i)
	{
		if ((*i)->isDummy())
		{
			continue;
		}

		// Did we load this node from a verticalLevel?
		// If so, it's time to go to the next loop!
		if ((*i)->getSegment() > lastSegmentsIndex)
		{
			break;
		}

		Node *node = (*i);
		int segmentX = node->getPosition().x / 10;
		int segmentY = node->getPosition().y / 10;
		int neighbourSegments[4];
		int neighbourDirections[4] = { -2, -3, -4, -5 };
		int neighbourDirectionsInverted[4] = { -4, -5, -2, -3 };

		if (segmentX == (_mapsize_x / 10)-1)
			neighbourSegments[0] = -1;
		else
			neighbourSegments[0] = _segments[segmentX+1][segmentY];
		if (segmentY == (_mapsize_y / 10)-1)
			neighbourSegments[1] = -1;
		else
			neighbourSegments[1] = _segments[segmentX][segmentY+1];
		if (segmentX == 0)
			neighbourSegments[2] = -1;
		else
			neighbourSegments[2] = _segments[segmentX-1][segmentY];
		if (segmentY == 0)
			neighbourSegments[3] = -1;
		else
			neighbourSegments[3] = _segments[segmentX][segmentY-1];

		for (std::vector<int>::iterator j = node->getNodeLinks()->begin(); j != node->getNodeLinks()->end(); ++j )
		{
			for (int n = 0; n < 4; n++)
			{
				if (*j == neighbourDirections[n])
				{
					for (std::vector<Node*>::iterator k = _save->getNodes()->begin(); k != _save->getNodes()->end(); ++k)
					{
						if ((*k)->isDummy())
						{
							continue;
						}
						if ((*k)->getSegment() == neighbourSegments[n])
						{
							for (std::vector<int>::iterator l = (*k)->getNodeLinks()->begin(); l != (*k)->getNodeLinks()->end(); ++l )
							{
								if (*l == neighbourDirectionsInverted[n])
								{
									*l = node->getID();
									*j = (*k)->getID();
								}
							}
						}
					}
				}
			}
		}
	}

	// Second pass to obfuscate even further by adding an additional layer of connections
	// Goes through _segmentsVertical to connect nodes from verticalLevels that would
	// be missed in the original code
	for (std::vector<Node*>::iterator i = _save->getNodes()->begin(); i != _save->getNodes()->end(); ++i)
	{
		// All the nodes before lastSegmentsIndex were already connected
		// verticalLevels start for segment values greater than lastSegmentsIndex
		if ((*i)->isDummy() || (*i)->getSegment() > lastSegmentsIndex)
		{
			continue;
		}

		Node *node = (*i);
		int nodeX = node->getPosition().x / 10;
		int nodeY = node->getPosition().y / 10;
		int nodeZ = node->getPosition().z;
		std::map<int, std::vector<int> > neighbourDirections;

		// Make a list of directions in which to look for neighbouring nodes
		if (nodeX != 0)
		{
			std::vector<int> tempVector;
			tempVector.push_back(-1);
			tempVector.push_back(0);
			tempVector.push_back(0);
			neighbourDirections[-5] = tempVector;
		}
		if (nodeX != (_mapsize_x / 10)-1)
		{
			std::vector<int> tempVector;
			tempVector.push_back(1);
			tempVector.push_back(0);
			tempVector.push_back(0);
			neighbourDirections[-3] = tempVector;
		}
		if (nodeY != 0)
		{
			std::vector<int> tempVector;
			tempVector.push_back(0);
			tempVector.push_back(-1);
			tempVector.push_back(0);
			neighbourDirections[-2] = tempVector;
		}
		if (nodeY != (_mapsize_y / 10)-1)
		{
			std::vector<int> tempVector;
			tempVector.push_back(0);
			tempVector.push_back(1);
			tempVector.push_back(0);
			neighbourDirections[-4] = tempVector;
		}
		if (nodeZ != 0)
		{
			std::vector<int> tempVector;
			tempVector.push_back(0);
			tempVector.push_back(0);
			tempVector.push_back(-1);
			neighbourDirections[-6] = tempVector;
		}
		if (nodeZ != _mapsize_z)
		{
			std::vector<int> tempVector;
			tempVector.push_back(0);
			tempVector.push_back(0);
			tempVector.push_back(1);
			neighbourDirections[-1] = tempVector;
		}

		std::map<int, int> neighbourDirectionsInverted;
		neighbourDirectionsInverted[-2] = -4;
		neighbourDirectionsInverted[-3] = -5;
		neighbourDirectionsInverted[-4] = -2;
		neighbourDirectionsInverted[-5] = -3;

		for (std::map<int, std::vector<int> >::iterator j = neighbourDirections.begin(); j != neighbourDirections.end(); j++)
		{
			std::vector<int>::iterator linkDirection;
			linkDirection = std::find(node->getNodeLinks()->begin(), node->getNodeLinks()->end(), (*j).first);
			if (linkDirection != node->getNodeLinks()->end() || (*j).first == -1 || (*j).first == -6)
			{
				for (std::vector<Node*>::iterator k = _save->getNodes()->begin(); k != _save->getNodes()->end(); ++k)
				{
					if ((*k)->isDummy())
					{
						continue;
					}

					std::vector<int> currentDirection = (*j).second;
					int linkX = (*k)->getPosition().x / 10 - currentDirection[0];
					int linkY = (*k)->getPosition().y / 10 - currentDirection[1];
					int linkZ = (*k)->getPosition().z - currentDirection[2];

					if (linkX == nodeX && linkY == nodeY && linkZ == nodeZ)
					{
						for (std::vector<int>::iterator l = (*k)->getNodeLinks()->begin(); l != (*k)->getNodeLinks()->end(); ++l )
						{
							std::map<int, int>::iterator invertedDirection = neighbourDirectionsInverted.find((*l));
							if (invertedDirection != neighbourDirectionsInverted.end() && !((*j).first == -1 || (*j).first == -6) && (*invertedDirection).second == *linkDirection)
							{
								*l = node->getID();
								*linkDirection = (*k)->getID();
							}
						}

						if ((*j).first == -1 || (*j).first == -6)
						{
							// Create a vertical link between nodes only if the nodes are within an x+y distance of 3 and the link isn't already there
							int xDistance = abs(node->getPosition().x - (*k)->getPosition().x);
							int yDistance = abs(node->getPosition().y - (*k)->getPosition().y);
							int xyDistance = xDistance + yDistance;
							std::vector<int>::iterator l;
							l = std::find((*k)->getNodeLinks()->begin(), (*k)->getNodeLinks()->end(), node->getID());
							if (xyDistance <= 3 && l == (*k)->getNodeLinks()->end())
							{
								(*k)->getNodeLinks()->push_back(node->getID());
								(*i)->getNodeLinks()->push_back((*k)->getID());
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Selects a position for a map block.
 * @param rects the positions to select from, none meaning the whole map.
 * @param X the x position for the block gets stored in this variable.
 * @param Y the y position for the block gets stored in this variable.
 * @param sizeX the x size of the block we want to add.
 * @param sizeY the y size of the block we want to add.
 * @return if a valid position was selected or not.
 */
bool BattlescapeGenerator::selectPosition(const std::vector<SDL_Rect *> *rects, int &X, int &Y, int sizeX, int sizeY)
{
	std::vector<SDL_Rect*> available;
	std::vector<std::pair<int, int> > valid;
	SDL_Rect wholeMap;
	wholeMap.x = 0;
	wholeMap.y = 0;
	wholeMap.w = (_mapsize_x / 10);
	wholeMap.h = (_mapsize_y / 10);
	sizeX = (sizeX / 10);
	sizeY = (sizeY / 10);
	if (rects->empty())
	{
		available.push_back(&wholeMap);
	}
	else
	{
		available = *rects;
	}
	for (std::vector<SDL_Rect*>::const_iterator i = available.begin(); i != available.end(); ++i)
	{
		if (sizeX > (*i)->w || sizeY > (*i)->h)
		{
			continue;
		}
		for (int x = (*i)->x; x + sizeX <= (*i)->x + (*i)->w && x + sizeX <= wholeMap.w; ++x)
		{
			for (int y = (*i)->y; y + sizeY <= (*i)->y + (*i)->h && y + sizeY <= wholeMap.h; ++y)
			{
				if (std::find(valid.begin(), valid.end(), std::make_pair(x,y)) == valid.end())
				{
					bool add = true;
					for (int xCheck = x; xCheck != x + sizeX; ++xCheck)
					{
						for (int yCheck = y; yCheck != y + sizeY; ++yCheck)
						{
							if (_blocks[xCheck][yCheck])
							{
								add = false;
							}
						}
					}
					if (add)
					{
						valid.push_back(std::make_pair(x,y));
					}
				}
			}
		}
	}
	if (valid.empty())
	{
		return false;
	}
	std::pair<int, int> selection = valid.at(RNG::generate(0, valid.size() - 1));
	X = selection.first;
	Y = selection.second;
	return true;
}

/**
 * Adds a craft (or UFO) to the map, and tries to add a landing zone type block underneath it.
 * @param craftMap the map for the craft in question.
 * @param command the script command to pull info from.
 * @param craftPos the position of the craft is stored here.
 * @return if the craft was placed or not.
 */
bool BattlescapeGenerator::addCraft(MapBlock *craftMap, MapScript *command, SDL_Rect &craftPos, RuleTerrain *terrain)
{
	craftPos.w = craftMap->getSizeX();
	craftPos.h = craftMap->getSizeY();
	bool placed = false;
	int x, y;

	placed = selectPosition(command->getRects(), x, y, craftPos.w, craftPos.h);
	// if ok, allocate it
	if (placed)
	{
		if (_verticalLevels.size() != 0)
		{
			command->initVerticalLevel(_verticalLevels.front());
		}

		craftPos.x = x;
		craftPos.y = y;
		craftPos.w /= 10;
		craftPos.h /= 10;
		for (x = 0; x < craftPos.w; ++x)
		{
			for (y = 0; y < craftPos.h; ++y)
			{
				_landingzone[craftPos.x + x][craftPos.y + y] = true;
				MapBlock *block = command->getNextBlock(terrain);
				if (block && !_blocks[craftPos.x + x][craftPos.y + y])
				{
					_save->getReinforcementsBlocks()[craftPos.x + x][craftPos.y + y] = _markAsReinforcementsBlock;
					_save->getFlattenedMapTerrainNames()[craftPos.x + x][craftPos.y + y] = terrain->getName();
					_save->getFlattenedMapBlockNames()[craftPos.x + x][craftPos.y + y] = block->getName();
					_blocks[craftPos.x + x][craftPos.y + y] = block;
					_blocksToDo--;
				}
			}
		}
	}

	return placed;
}

/**
 * draws a line along the map either horizontally, vertically or both.
 * @param direction the direction to draw the line
 * @param rects the positions to allow the line to be drawn in.
 * @return if the blocks were added or not.
 */
bool BattlescapeGenerator::addLine(MapDirection direction, const std::vector<SDL_Rect*> *rects, RuleTerrain *terrain, int verticalGroup, int horizontalGroup, int crossingGroup)
{
	if (direction == MD_BOTH)
	{
		if (addLine(MD_VERTICAL, rects, terrain, verticalGroup, horizontalGroup, crossingGroup))
		{
			addLine(MD_HORIZONTAL, rects, terrain, verticalGroup, horizontalGroup, crossingGroup);
			return true;
		}
		return false;
	}

	int tries = 0;
	bool placed = false;

	int roadX, roadY = 0;
	int *iteratorValue = &roadX;
	int comparator = verticalGroup;
	int typeToAdd = horizontalGroup;
	int limit = _mapsize_x / 10;
	if (direction == MD_VERTICAL)
	{
		iteratorValue = &roadY;
		comparator = horizontalGroup;
		typeToAdd = verticalGroup;
		limit = _mapsize_y / 10;
	}
	while (!placed)
	{
		placed = selectPosition(rects, roadX, roadY, 10, 10);
		for (*iteratorValue = 0; *iteratorValue < limit; *iteratorValue += 1)
		{
			if (placed && _blocks[roadX][roadY] != 0 && _blocks[roadX][roadY]->isInGroup(comparator) == false)
			{
				placed = false;
				break;
			}
		}
		if (tries++ > 20)
		{
			return false;
		}
	}
	*iteratorValue = 0;
	while (*iteratorValue < limit)
	{
		if (_blocks[roadX][roadY] == 0)
		{
			MapBlock* randomMapBlock1 = terrain->getRandomMapBlock(10, 10, typeToAdd);
			if (!randomMapBlock1)
			{
				std::ostringstream ss1;
				ss1 << "Map script command `addLine` failed (1). Terrain " + terrain->getName() + ", block group " << typeToAdd << ", script " << _save->getLastUsedMapScript();
				throw Exception(ss1.str());
			}
			addBlock(roadX, roadY, randomMapBlock1, terrain);

			SDL_Rect blockRect;
			blockRect.x = roadX;
			blockRect.y = roadY;
			blockRect.w = 1;
			blockRect.h = 1;

			// Only add unique positions for loading vertical levels
			auto it = std::find_if(_placedBlockRects.begin(), _placedBlockRects.end(), [&](const SDL_Rect& rect) { return rect.x == roadX && rect.y == roadY; });
			if (it == _placedBlockRects.end())
			{
				_placedBlockRects.push_back(blockRect);
			}
		}
		else if (_blocks[roadX][roadY]->isInGroup(comparator))
		{
			MapBlock* randomMapBlock2 = terrain->getRandomMapBlock(10, 10, crossingGroup);
			if (!randomMapBlock2)
			{
				std::ostringstream ss2;
				ss2 << "Map script command `addLine` failed (2). Terrain " + terrain->getName() + ", block group " << crossingGroup << ", script " << _save->getLastUsedMapScript();
				throw Exception(ss2.str());
			}
			_blocks[roadX][roadY] = randomMapBlock2;
			clearModule(roadX * 10, roadY * 10, 10, 10);
			int terrainMapDataSetIDOffset = loadExtraTerrain(terrain);
			loadMAP(_blocks[roadX][roadY], roadX * 10, roadY * 10, 0, terrain, terrainMapDataSetIDOffset);

			SDL_Rect blockRect;
			blockRect.x = roadX;
			blockRect.y = roadY;
			blockRect.w = 1;
			blockRect.h = 1;

			// Only add unique positions for loading vertical levels
			auto it = std::find_if(_placedBlockRects.begin(), _placedBlockRects.end(), [&](const SDL_Rect& rect) { return rect.x == roadX && rect.y == roadY; });
			if (it == _placedBlockRects.end())
			{
				_placedBlockRects.push_back(blockRect);
			}
		}
		*iteratorValue += 1;
	}
	return true;
}

/**
 * Adds a single block to the map.
 * @param x the x position to add the block
 * @param y the y position to add the block
 * @param z the z offset for loading the map
 * @param block the block to add.
 * @param terrain pointer to the terrain for the block.
 * @return if the block was added or not.
 */
bool BattlescapeGenerator::addBlock(int x, int y, MapBlock *block, RuleTerrain* terrain)
{
	int xSize = (block->getSizeX() - 1) / 10;
	int ySize = (block->getSizeY() - 1) / 10;

	for (int xd = 0; xd <= xSize; ++xd)
	{
		for (int yd = 0; yd != ySize; ++yd)
		{
			if (_blocks[x + xd][y + yd])
				return false;
		}
	}

	for (int xd = 0; xd <= xSize; ++xd)
	{
		for (int yd = 0; yd <= ySize; ++yd)
		{
			_save->getReinforcementsBlocks()[x + xd][y + yd] = _markAsReinforcementsBlock;
			_save->getFlattenedMapTerrainNames()[x + xd][y + yd] = terrain->getName();
			_save->getFlattenedMapBlockNames()[x + xd][y + yd] = block->getName();
			_blocks[x + xd][y + yd] = _dummy;
			_blocksToDo--;
		}
	}

	// mark the south edge of the block for drilling
	for (int xd = 0; xd <= xSize; ++xd)
	{
		_drillMap[x+xd][y + ySize] = MD_VERTICAL;
	}
	// then the east edge
	for (int yd = 0; yd <= ySize; ++yd)
	{
		_drillMap[x + xSize][y+yd] = MD_HORIZONTAL;
	}
	// then the far corner gets marked for both
	// this also marks 1x1 modules
	_drillMap[x + xSize][y+ySize] = MD_BOTH;

	_blocks[x][y] = block;
	bool visible = (_save->getMissionType() == "STR_BASE_DEFENSE"); // yes, i'm hard coding these, big whoop, wanna fight about it?

	int terrainMapDataSetIDOffset = loadExtraTerrain(terrain);
	loadMAP(_blocks[x][y], x * 10, y * 10, 0, terrain, terrainMapDataSetIDOffset, visible);

	return true;
}

/**
 * Drills a tunnel between existing map modules.
 * note that this drills all modules currently on the map,
 * so it should take place BEFORE the dirt is added in base defenses.
 * @param data the wall replacements and level to dig on.
 * @param rects the length/width of the tunnels themselves.
 * @param dir the direction to drill.
 * @param terrain which terrain to use for the wall replacements.
 */
void BattlescapeGenerator::drillModules(TunnelData* data, const std::vector<SDL_Rect *> *rects, MapDirection dir, RuleTerrain *terrain)
{
	// Make sure the terrain we're going to use for the replacements is loaded
	loadExtraTerrain(terrain);

	const MCDReplacement *wWall = data->getMCDReplacement("westWall");
	const MCDReplacement *nWall = data->getMCDReplacement("northWall");
	const MCDReplacement *corner = data->getMCDReplacement("corner");
	const MCDReplacement *floor = data->getMCDReplacement("floor");
	SDL_Rect rect;
	rect.x = rect.y = rect.w = rect.h = 3;
	if (!rects->empty())
	{
		rect = *rects->front();
	}

	for (int i = 0; i < (_mapsize_x / 10); ++i)
	{
		for (int j = 0; j < (_mapsize_y / 10); ++j)
		{
			if (_blocks[i][j] == 0)
				continue;

			MapData *md;

			if (dir != MD_VERTICAL)
			{
				// drill east
				if (i < (_mapsize_x / 10)-1 && (_drillMap[i][j] == MD_HORIZONTAL || _drillMap[i][j] == MD_BOTH) && _blocks[i+1][j] != 0)
				{
					Tile *tile;
					// remove stuff
					for (int k = rect.y; k != rect.y + rect.h; ++k)
					{
						tile = _save->getTile(Position((i*10)+9, (j*10)+k, data->level));
						if (tile)
						{
							tile->setMapData(0, -1, -1, O_WESTWALL);
							tile->setMapData(0, -1, -1, O_OBJECT);
							if (floor)
							{
								md = terrain->getMapDataSets()->at(floor->set)->getObject(floor->entry);
								tile->setMapData(md, floor->entry, floor->set, O_FLOOR);
							}

							tile = _save->getTile(Position((i+1)*10, (j*10)+k, data->level));
							tile->setMapData(0, -1, -1, O_WESTWALL);
							MapData* obj = tile->getMapData(O_OBJECT);
							if (obj && obj->getTUCost(MT_WALK) == 0)
							{
								tile->setMapData(0, -1, -1, O_OBJECT);
							}
						}
					}

					if (nWall)
					{
						md = terrain->getMapDataSets()->at(nWall->set)->getObject(nWall->entry);
						tile = _save->getTile(Position((i*10)+9, (j*10)+rect.y, data->level));
						tile->setMapData(md, nWall->entry, nWall->set, O_NORTHWALL);
						tile = _save->getTile(Position((i*10)+9, (j*10)+rect.y+rect.h, data->level));
						tile->setMapData(md, nWall->entry, nWall->set, O_NORTHWALL);
					}

					if (corner)
					{
						md = terrain->getMapDataSets()->at(corner->set)->getObject(corner->entry);
						tile = _save->getTile(Position((i+1)*10, (j*10)+rect.y, data->level));
						if (tile->getMapData(O_NORTHWALL) == 0)
							tile->setMapData(md, corner->entry, corner->set, O_NORTHWALL);
					}
				}
			}

			if (dir != MD_HORIZONTAL)
			{
				// drill south
				if (j < (_mapsize_y / 10)-1 && (_drillMap[i][j] == MD_VERTICAL || _drillMap[i][j] == MD_BOTH) && _blocks[i][j+1] != 0)
				{
					// remove stuff
					for (int k = rect.x; k != rect.x + rect.w; ++k)
					{
						Tile * tile = _save->getTile(Position((i*10)+k, (j*10)+9, data->level));
						if (tile)
						{
							tile->setMapData(0, -1, -1, O_NORTHWALL);
							tile->setMapData(0, -1, -1, O_OBJECT);
							if (floor)
							{
								md = terrain->getMapDataSets()->at(floor->set)->getObject(floor->entry);
								tile->setMapData(md, floor->entry, floor->set, O_FLOOR);
							}

							tile = _save->getTile(Position((i*10)+k, (j+1)*10, data->level));
							tile->setMapData(0, -1, -1, O_NORTHWALL);
							MapData* obj = tile->getMapData(O_OBJECT);
							if (obj && obj->getTUCost(MT_WALK) == 0)
							{
								tile->setMapData(0, -1, -1, O_OBJECT);
							}
						}
					}

					if (wWall)
					{
						md = terrain->getMapDataSets()->at(wWall->set)->getObject(wWall->entry);
						Tile *tile = _save->getTile(Position((i*10)+rect.x, (j*10)+9, data->level));
						tile->setMapData(md, wWall->entry, wWall->set, O_WESTWALL);
						tile = _save->getTile(Position((i*10)+rect.x+rect.w, (j*10)+9, data->level));
						tile->setMapData(md, wWall->entry, wWall->set, O_WESTWALL);
					}

					if (corner)
					{
						md = terrain->getMapDataSets()->at(corner->set)->getObject(corner->entry);
						Tile *tile = _save->getTile(Position((i*10)+rect.x, (j+1)*10, data->level));
						if (tile->getMapData(O_WESTWALL) == 0)
							tile->setMapData(md, corner->entry, corner->set, O_WESTWALL);
					}
				}
			}
		}
	}
}

/**
 * Removes all blocks within a given set of rects, as defined in the command.
 * @param command contains all the info we need.
 * @return success of the removal.
 * @feel shame for having written this.
 */
bool BattlescapeGenerator::removeBlocks(MapScript *command)
{
	std::vector<std::pair<int, int> > deleted;
	bool success = false;

	for (std::vector<SDL_Rect*>::const_iterator k = command->getRects()->begin(); k != command->getRects()->end(); ++k)
	{
		for (int x = (*k)->x; x != (*k)->x + (*k)->w && x != _mapsize_x / 10; ++x)
		{
			for (int y = (*k)->y; y != (*k)->y + (*k)->h && y != _mapsize_y / 10; ++y)
			{
				if (_blocks[x][y] != 0 && _blocks[x][y] != _dummy)
				{
					std::pair<int, int> pos(x, y);
					if (!command->getGroups()->empty())
					{
						for (std::vector<int>::const_iterator z = command->getGroups()->begin(); z != command->getGroups()->end(); ++z)
						{
							if (_blocks[x][y]->isInGroup((*z)))
							{
								// the deleted vector should only contain unique entries
								if (std::find(deleted.begin(), deleted.end(), pos) == deleted.end())
								{
									deleted.push_back(pos);
								}
							}
						}
					}
					else if (!command->getBlocks()->empty())
					{
						for (std::vector<int>::const_iterator z = command->getBlocks()->begin(); z != command->getBlocks()->end(); ++z)
						{
							if ((size_t)(*z) < _terrain->getMapBlocks()->size())
							{
								// the deleted vector should only contain unique entries
								if (std::find(deleted.begin(), deleted.end(), pos) == deleted.end())
								{
									deleted.push_back(pos);
								}
							}
						}
					}
					else
					{
						// the deleted vector should only contain unique entries
						if (std::find(deleted.begin(), deleted.end(), pos) == deleted.end())
						{
							deleted.push_back(pos);
						}
					}
				}
			}
		}
	}
	for (std::vector<std::pair<int, int> >::const_iterator z = deleted.begin(); z != deleted.end(); ++z)
	{
		int x = (*z).first;
		int y = (*z).second;
		clearModule(x * 10, y * 10, _blocks[x][y]->getSizeX(), _blocks[x][y]->getSizeY());

		int delx = (_blocks[x][y]->getSizeX() / 10);
		int dely = (_blocks[x][y]->getSizeY() / 10);

		for (int dx = x; dx != x + delx; ++dx)
		{
			for (int dy = y; dy != y + dely; ++dy)
			{
				_save->getReinforcementsBlocks()[dx][dy] = 0;
				_save->getFlattenedMapTerrainNames()[dx][dy] = "";
				_save->getFlattenedMapBlockNames()[dx][dy] = "";
				_blocks[dx][dy] = 0;
				_blocksToDo++;

				// Make sure vertical levels segment data is removed too
				std::vector<std::pair<MapBlock*, Position> >::iterator it = _verticalLevelSegments.begin();
				while (it != _verticalLevelSegments.end())
				{
					if (it->second.x == dx && it->second.y == dy)
					{
						it = _verticalLevelSegments.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
		}
		// this command succeeds if even one block is removed.
		success = true;
	}
	return success;
}

/**
 * Sets the terrain to be used in battle generation.
 * @param terrain Pointer to the terrain rules.
 */
void BattlescapeGenerator::setTerrain(RuleTerrain *terrain)
{
	_terrain = terrain;
}


/**
 * Sets up the objectives for the map.
 * @param ruleDeploy the deployment data we're gleaning data from.
 */
void BattlescapeGenerator::setupObjectives(const AlienDeployment *ruleDeploy)
{
	// reset bug hunt mode (necessary for multi-stage missions)
	_save->setBughuntMode(false);
	// set global min turn
	_save->setBughuntMinTurn(_game->getMod()->getBughuntMinTurn());
	// set min turn override per deployment (if defined)
	if (ruleDeploy->getBughuntMinTurn() > 0)
	{
		_save->setBughuntMinTurn(ruleDeploy->getBughuntMinTurn());
	}

	// used for "escort the VIPs" and "protect the VIPs" missions
	_save->setVIPSurvivalPercentage(ruleDeploy->getVIPSurvivalPercentage());
	_save->setVIPEscapeType(ruleDeploy->getEscapeType());

	int targetType = ruleDeploy->getObjectiveType();

	if (targetType > -1)
	{
		int objectives = ruleDeploy->getObjectivesRequired();
		int actualCount = 0;

		for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
		{
			for (int j = O_FLOOR; j < O_MAX; ++j)
			{
				TilePart tp = (TilePart)j;
				if (_save->getTile(i)->getMapData(tp) && _save->getTile(i)->getMapData(tp)->getSpecialType() == targetType)
				{
					actualCount++;
				}
			}
		}

		if (actualCount > 0)
		{
			_save->setObjectiveType(targetType);

			if (actualCount < objectives || objectives == 0)
			{
				_save->setObjectiveCount(actualCount);
			}
			else
			{
				_save->setObjectiveCount(objectives);
			}
		}
	}
}

/**
* Sets the depth based on the terrain or the provided AlienDeployment rule.
* @param ruleDeploy the deployment data we're gleaning data from.
* @param nextStage whether the mission is progressing to the next stage.
*/
void BattlescapeGenerator::setDepth(const AlienDeployment* ruleDeploy, bool nextStage)
{
	if (_save->getDepth() > 0 && !nextStage)
	{
		// new battle menu will have set the depth already
		return;
	}

	if (ruleDeploy->getMaxDepth() > 0)
	{
		_save->setDepth(RNG::generate(ruleDeploy->getMinDepth(), ruleDeploy->getMaxDepth()));
	}
	else if (_terrain->getMaxDepth() > 0 || nextStage)
	{
		_save->setDepth(RNG::generate(_terrain->getMinDepth(), _terrain->getMaxDepth()));
	}
}

/**
* Sets the background music based on the terrain or the provided AlienDeployment rule.
* @param ruleDeploy the deployment data we're gleaning data from.
* @param nextStage whether the mission is progressing to the next stage.
*/
void BattlescapeGenerator::setMusic(const AlienDeployment* ruleDeploy, bool nextStage)
{
	if (!ruleDeploy->getMusic().empty())
	{
		_save->setMusic(ruleDeploy->getMusic().at(RNG::generate(0, ruleDeploy->getMusic().size() - 1)));
	}
	else if (!_terrain->getMusic().empty())
	{
		_save->setMusic(_terrain->getMusic().at(RNG::generate(0, _terrain->getMusic().size() - 1)));
	}
	else if (nextStage)
	{
		_save->setMusic("");
	}
}

}
