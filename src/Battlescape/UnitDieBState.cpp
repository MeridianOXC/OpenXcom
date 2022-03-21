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

#include "UnitDieBState.h"
#include "TileEngine.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "../Engine/Game.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Mod/Mod.h"
#include "../Engine/Sound.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"
#include "../Engine/Language.h"
#include "../Mod/Armor.h"
#include "InfoboxOKState.h"
#include "InfoboxState.h"
#include "../Savegame/Node.h"

namespace OpenXcom
{

/**
 * Sets up an UnitDieBState.
 * @param parent Pointer to the Battlescape.
 * @param unit Dying unit.
 * @param damageType Type of damage that caused the death.
 * @param noSound Whether to disable the death sound.
 */
UnitDieBState::UnitDieBState(BattlescapeGame *parent, BattleUnit *unit, const RuleDamageType* damageType, bool noSound) : BattleState(parent),
	_unit(unit), _damageType(damageType), _noSound(noSound), _extraFrame(0), _overKill(unit->getOverKillDamage())
{
	// don't show the "fall to death" animation when a unit is blasted with explosives or he is already unconscious
	if (!_damageType->isDirect() || _unit->getStatus() == STATUS_UNCONSCIOUS)
	{

		/********************************************************
		Proclamation from Lord Xenu:

		any unit that is going to skip its death pirouette
		MUST have its direction set to 3 first.

		Failure to comply is treason, and treason is punishable
		by death. (after being correctly oriented)

		********************************************************/
		_unit->setDirection(3);


		_unit->instaFalling();
		if (_parent->getSave()->isBeforeGame())
		{
			convertUnitToCorpse();
			_extraFrame = 3; // shortcut to popState()
		}
	}
	else
	{
		if (_unit->getFaction() == FACTION_PLAYER)
		{
			_parent->getMap()->setUnitDying(true);
		}
		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);
		if (_unit->getDirection() != 3)
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED / 3);
		}
	}

	_unit->clearVisibleTiles();
	_unit->clearVisibleUnits();
	_unit->freePatrolTarget();

	if (!_parent->getSave()->isBeforeGame() && (_unit->getFaction() == FACTION_HOSTILE || _unit->getFaction() == FACTION_ALIEN_PLAYER))
	{
		std::vector<Node *> *nodes = _parent->getSave()->getNodes();
		if (!nodes) return; // this better not happen.

		for (std::vector<Node*>::iterator  n = nodes->begin(); n != nodes->end(); ++n)
		{
			if (!(*n)->isDummy() && Position::distanceSq((*n)->getPosition(), _unit->getPosition()) < 4)
			{
				(*n)->setType((*n)->getType() | Node::TYPE_DANGEROUS);
			}
		}
	}
}

/**
 * Deletes the UnitDieBState.
 */
UnitDieBState::~UnitDieBState()
{

}

void UnitDieBState::init()
{
#if 0
	// check for presence of battlestate to ensure that we're not pre-battle
	// check for the unit's tile to make sure we're not trying to kill a dead guy
	if (_parent->getSave()->getBattleState() && !_unit->getTile())
	{
		_parent->popState();
	}
#endif
}

/**
 * Runs state functionality every cycle.
 * Progresses the death, displays any messages, checks if the mission is over, ...
 */
void UnitDieBState::think()
{
	if (_extraFrame == 3)
	{
		_parent->popState();
		return;
	}
	if (_unit->getDirection() != 3 && _damageType->isDirect())
	{
		int dir = _unit->getDirection() + 1;
		if (dir == 8)
		{
			dir = 0;
		}
		_unit->lookAt(dir);
		_unit->turn();
		if (dir == 3)
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);
		}
	}
	else if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		_unit->keepFalling();
	}
	else if (!_unit->isOut())
	{
		_unit->startFalling();

		if (!_noSound)
		{
			playDeathSound();
		}
		if (_unit->getRespawn())
		{
			while (_unit->getStatus() == STATUS_COLLAPSING)
			{
				_unit->keepFalling();
			}
		}
	}
	if (_extraFrame == 2)
	{
		_parent->getMap()->setUnitDying(false);
		_parent->getTileEngine()->calculateLighting(LL_ITEMS, _unit->getPosition(), _unit->getArmor()->getSize());
		_parent->getTileEngine()->calculateFOV(_unit->getPosition(), _unit->getArmor()->getSize(), false); //Update FOV for anyone that can see me
		_parent->popState();
		if (_unit->getOriginalFaction() == FACTION_PLAYER)
		{
			Game *game = _parent->getSave()->getBattleState()->getGame();
			if (_unit->getStatus() == STATUS_DEAD)
			{
				if (_damageType->ResistType == DT_NONE && !_unit->getSpawnUnit())
				{
					game->pushState(new InfoboxOKState(game->getLanguage()->getString("STR_HAS_DIED_FROM_A_FATAL_WOUND", _unit->getGender()).arg(_unit->getName(game->getLanguage()))));
				}
				else if (Options::battleNotifyDeath && _unit->getGeoscapeSoldier() != 0)
				{
					game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_BEEN_KILLED", _unit->getGender()).arg(_unit->getName(game->getLanguage()))));
				}
			}
			else if (_unit->indicatorsAreEnabled())
			{
				game->pushState(new InfoboxOKState(game->getLanguage()->getString("STR_HAS_BECOME_UNCONSCIOUS", _unit->getGender()).arg(_unit->getName(game->getLanguage()))));
			}
		}
		// if all units from either faction are killed - auto-end the mission.
		if (_parent->getSave()->getSide() == FACTION_PLAYER)
		{
			_parent->autoEndBattle();
		}
	}
	else if (_extraFrame == 1)
	{
		_extraFrame++;
	}
	else if (_unit->isOut())
	{
		_extraFrame = 1;
		if (!_noSound && !_damageType->isDirect() && _unit->getStatus() != STATUS_UNCONSCIOUS)
		{
			playDeathSound();
		}
		if (_unit->getStatus() == STATUS_UNCONSCIOUS && !_unit->getCapturable())
		{
			_unit->instaKill();
		}
		if (_unit->getTurnsSinceSpotted() < 255)
		{
			_unit->setTurnsSinceSpotted(255);
		}
		if (_unit->getTurnsLeftSpottedForSnipers() != 0)
		{
			_unit->setTurnsLeftSpottedForSnipers(0);
		}
		if (_unit->getSpawnUnit() && !_overKill)
		{
			if (!_unit->getAlreadyRespawned())
			{
				// converts the dead zombie to a chryssalid
				_parent->convertUnit(_unit);
			}
		}
		else
		{
			convertUnitToCorpse();
		}
		if (_unit == _parent->getSave()->getSelectedUnit())
		{
			_parent->getSave()->setSelectedUnit(0);
		}
	}

}

/**
 * Unit falling cannot be cancelled.
 */
void UnitDieBState::cancel()
{
}

/**
 * Converts unit to a corpse (item).
 */
void UnitDieBState::convertUnitToCorpse()
{
	Position lastPosition = _unit->getPosition();
	int size = _unit->getArmor()->getSize();
	bool dropItems = (_unit->hasInventory() &&
		(!Options::weaponSelfDestruction ||
		(_unit->getOriginalFaction() != FACTION_HOSTILE ||_unit->getOriginalFaction() != FACTION_ALIEN_PLAYER || _unit->getStatus() == STATUS_UNCONSCIOUS)));
	if (!_noSound)
	{
		_parent->getSave()->getBattleState()->resetUiButton();
	}
	// remove the unconscious body item corresponding to this unit, and if it was being carried, keep track of what slot it was in
	if (lastPosition != TileEngine::invalid)
	{
		_parent->getSave()->removeUnconsciousBodyItem(_unit);
	}

	// move inventory from unit to the ground
	if (dropItems && _unit->getTile())
	{
		_parent->getTileEngine()->itemDropInventory(_unit->getTile(), _unit);
	}

	// remove unit-tile link
	_unit->setTile(nullptr, _parent->getSave());

	if (lastPosition == TileEngine::invalid) // we're being carried
	{
		if (_overKill)
		{
			_parent->getSave()->removeUnconsciousBodyItem(_unit);
		}
		else
		{
			// replace the unconscious body item with a corpse in the carrying unit's inventory
			for (std::vector<BattleItem*>::iterator it = _parent->getSave()->getItems()->begin(); it != _parent->getSave()->getItems()->end(); )
			{
				if ((*it)->getUnit() == _unit)
				{
					auto corpseRules = _unit->getArmor()->getCorpseBattlescape()[0]; // we're in an inventory, so we must be a 1x1 unit
					(*it)->convertToCorpse(corpseRules);
					break;
				}
				++it;
			}
		}
	}
	else
	{
		if (!_overKill)
		{
			int i = size * size - 1;
			for (int y = size - 1; y >= 0; --y)
			{
				for (int x = size - 1; x >= 0; --x)
				{
					BattleItem *corpse = _parent->getSave()->createItemForTile(_unit->getArmor()->getCorpseBattlescape()[i], nullptr);
					corpse->setUnit(_unit);
					_parent->dropItem(lastPosition + Position(x,y,0), corpse, false);
					--i;
				}
			}
		}
		else
		{
			_parent->getSave()->getTileEngine()->applyGravity(_parent->getSave()->getTile(lastPosition));
		}
	}
}

/**
 * Plays the death sound.
 */
void UnitDieBState::playDeathSound()
{
	const std::vector<int> &sounds = _unit->getDeathSounds();
	if (!sounds.empty())
	{
		int i = sounds[RNG::generate(0, sounds.size() - 1)];
		if (i >= 0)
		{
			_parent->getMod()->getSoundByDepth(_parent->getDepth(), i)->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
		}
	}
}

}
