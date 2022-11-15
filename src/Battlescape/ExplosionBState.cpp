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

#include "ExplosionBState.h"
#include "BattlescapeState.h"
#include "Explosion.h"
#include "TileEngine.h"
#include "Map.h"
#include "Camera.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Engine/RNG.h"

namespace OpenXcom
{

/**
 * Sets up an ExplosionBState.
 * @param parent Pointer to the BattleScape.
 * @param center Center position in voxelspace.
 * @param item Item involved in the explosion (eg grenade).
 * @param unit Unit involved in the explosion (eg unit throwing the grenade).
 * @param tile Tile the explosion is on.
 * @param lowerWeapon Whether the unit causing this explosion should now lower their weapon.
 * @param range Distance between weapon and target.
 * @param explosionCounter Counter for chain terrain explosions.
 * @param terrainMeleeTilePart Tile part for terrain melee.
 */
ExplosionBState::ExplosionBState(BattlescapeGame *parent, Position center, BattleActionAttack attack, Tile *tile, bool lowerWeapon, int range, int explosionCounter, int terrainMeleeTilePart) : BattleState(parent),
	_explosionCounter(explosionCounter), _terrainMeleeTilePart(terrainMeleeTilePart), _attack(attack), _center(center), _damageType(), _tile(tile), _targetPsiOrHit(nullptr),
	_power(0), _radius(6), _range(range), _areaOfEffect(false), _lowerWeapon(lowerWeapon), _hit(false), _psi(false)
{

}

/**
 * Deletes the ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{

}

/**
 * Set new value to reference if new value is not equal -1.
 * @param oldValue old value to change.
 * @param newValue new value to set, but only if is not equal -1.
 */
void ExplosionBState::optValue(int& oldValue, int newValue) const
{
	if (newValue != -1)
	{
		oldValue = newValue;
	}
}

/**
 * Initializes the explosion.
 * The animation and sound starts here.
 * If the animation is finished, the actual effect takes place.
 */
void ExplosionBState::init()
{
	BattleType type = BT_NONE;
	BattleActionType action = _attack.type;
	const RuleItem* itemRule = 0;
	bool miss = false;
	if (_attack.damage_item)
	{
		itemRule = _attack.damage_item->getRules();
		type = itemRule->getBattleType();

		_power = 0;
		_hit = action == BA_HIT;
		_psi = type == BT_PSIAMP && action != BA_USE && !_hit;
		if (_hit && type != BT_MELEE)
		{
			_power += itemRule->getMeleeBonus(_attack);

			_radius = 0;
			_damageType = itemRule->getMeleeType();
		}
		else
		{
			_power += itemRule->getPowerBonus(_attack);
			_power -= itemRule->getPowerRangeReduction(_range);

			_radius = itemRule->getExplosionRadius(_attack);
			_damageType = itemRule->getDamageType();
		}

		if (type == BT_PSIAMP || _hit)
		{
			Position targetPos = _center.toTile();
			_targetPsiOrHit = _parent->getSave()->getTile(targetPos)->getOverlappingUnit(_parent->getSave());
		}

		//testing if we hit target
		if (type == BT_PSIAMP && !_hit)
		{
			if (action != BA_USE)
			{
				_power = 0;
			}
			if (!_parent->getTileEngine()->psiAttack(_attack, _targetPsiOrHit))
			{
				_power = 0;
				miss = true;
			}
			else
			{
				_parent->psiAttackMessage(_attack, _targetPsiOrHit);
			}
		}
		else if (type == BT_MELEE || _hit)
		{
			if (!_parent->getTileEngine()->meleeAttack(_attack, _targetPsiOrHit, _terrainMeleeTilePart))
			{
				_power = 0;
				miss = true;
			}
		}
		else if (type == BT_FIREARM)
		{
			if (_power <= 0)
			{
				miss = true;
			}
		}

		_areaOfEffect = type != BT_MELEE && _radius != 0 &&
						(type != BT_PSIAMP || action == BA_USE) &&
						!_hit && !miss;
	}
	else if (_tile)
	{
		ItemDamageType DT;
		switch (_tile->getExplosiveType())
		{
		case 0:
			DT = DT_HE;
			break;
		case 5:
			DT = DT_IN;
			break;
		case 6:
			DT = DT_STUN;
			break;
		default:
			DT = DT_SMOKE;
			break;
		}
		_power = _tile->getExplosive();
		_tile->setExplosive(0, 0, true);
		_damageType = _parent->getMod()->getDamageType(DT);
		_radius = _power /10;
		_areaOfEffect = true;
	}
	else if (_attack.type == BA_SELF_DESTRUCT)
	{
		_areaOfEffect = true;
		if (_attack.attacker)
		{
			itemRule = _attack.attacker->getArmor()->getCorpseGeoscape(); //TODO: not getCorpseBattlescape ones?
			_power = itemRule->getPowerBonus(_attack);
			_damageType = itemRule->getDamageType();
			_radius = itemRule->getExplosionRadius(_attack);
			if (!RNG::percent(itemRule->getSpecialChance()))
			{
				_power = 0;
			}
		}
		else
		{
			_power = 0;
		}
	}
	else
	{
		_power = 120;
		_damageType = _parent->getMod()->getDamageType(DT_HE);
		_areaOfEffect = true;
	}


	bool range = !(_hit || (_attack.weapon_item && _attack.weapon_item->getRules()->getBattleType() == BT_PSIAMP));

	if (_areaOfEffect)
	{
		if (_power > 0)
		{
			_parent->getSave()->getTileEngine()->explode(_attack, _center, _power, _damageType, _radius, range);

			int powerForAnimation = _power;
			if (itemRule && itemRule->getPowerForAnimation() > 0)
			{
				powerForAnimation = itemRule->getPowerForAnimation();
			}

			int frame = Mod::EXPLOSION_OFFSET;
			int frameCount = -1;
			int sound = powerForAnimation <= 80 ? Mod::SMALL_EXPLOSION : Mod::LARGE_EXPLOSION;

			if (itemRule)
			{
				frame = itemRule->getHitAnimation();
				frameCount = itemRule->getHitAnimationFrames();
				optValue(sound, itemRule->getExplosionHitSound());
			}
			if (_parent->getDepth() > 0)
			{
				frame -= (frameCount > 0 ? frameCount : Explosion::EXPLODE_FRAMES);
			}
			int frameDelay = 0;
			int counter = std::max(1, (powerForAnimation / 5) / 5);
			_parent->getMap()->setBlastFlash(true);
			int lowerLimit = std::max(1, powerForAnimation / 5);
			for (int i = 0; i < lowerLimit; i++)
			{
				int X = RNG::generate(-powerForAnimation / 2, powerForAnimation / 2);
				int Y = RNG::generate(-powerForAnimation / 2, powerForAnimation / 2);
				Position p = _center;
				p.x += X; p.y += Y;
				Explosion *explosion = new Explosion(p, frame, frameDelay, true, false, frameCount);
				// add the explosion on the map
				_parent->getMap()->getExplosions()->push_back(explosion);
				if (i > 0 && i % counter == 0)
				{
					frameDelay++;
				}
			}
			int explosionSpeed = BattlescapeState::DEFAULT_ANIM_SPEED/2;
			if (itemRule)
			{
				explosionSpeed -= (10 * itemRule->getExplosionSpeed());
			}
			if (_explosionCounter > 6)
			{
				explosionSpeed = 1; // maximum animation speed for long chain terrain explosions
			}
			_parent->setStateInterval(std::max(1, explosionSpeed));
			// explosion sound
			_parent->playSound(sound);
			if (_parent->getMap()->getFollowProjectile() || _explosionCounter > 0)
			{
				_parent->getMap()->getCamera()->centerOnPosition(_center.toTile(), false);
			}
		}
		else
		{
			_parent->popState();
		}
	}
	else
	// create a bullet hit
	{
		_parent->getSave()->getTileEngine()->hit(_attack, _center, _power, _damageType, range, _terrainMeleeTilePart);

		_parent->setStateInterval(std::max(1, ((BattlescapeState::DEFAULT_ANIM_SPEED/2) - (10 * itemRule->getExplosionSpeed()))));
		int anim = -1;
		int animFrames = -1;
		int sound = -1;

		const RuleItem *weaponRule = _attack.weapon_item->getRules();
		const RuleItem *damageRule = _attack.weapon_item != _attack.damage_item ? itemRule : nullptr;

		if (_hit || _psi)
		{
			anim = weaponRule->getMeleeAnimation();
			animFrames = weaponRule->getMeleeAnimationFrames();
			if (_psi)
			{
				// psi attack sound is based weapon hit sound
				sound = weaponRule->getHitSound();

				optValue(anim, weaponRule->getPsiAnimation());
				optValue(animFrames, weaponRule->getPsiAnimationFrames());
				optValue(sound, weaponRule->getPsiSound());
			}
			else
			{
				sound = weaponRule->getMeleeSound();
				if (damageRule)
				{
					optValue(anim, damageRule->getMeleeAnimation());
					optValue(animFrames, damageRule->getMeleeAnimationFrames());
					optValue(sound, damageRule->getMeleeSound());
				}
			}
		}
		else
		{
			anim = itemRule->getHitAnimation();
			animFrames = itemRule->getHitAnimationFrames();
			sound = itemRule->getHitSound();
		}

		if (miss)
		{
			if (_hit || _psi)
			{
				optValue(anim, weaponRule->getMeleeMissAnimation());
				optValue(animFrames, weaponRule->getMeleeMissAnimationFrames());
				if (_psi)
				{
					// psi attack sound is based weapon hit sound
					optValue(sound, weaponRule->getHitMissSound());

					optValue(anim, weaponRule->getPsiMissAnimation());
					optValue(animFrames, weaponRule->getPsiMissAnimationFrames());
					optValue(sound, weaponRule->getPsiMissSound());
				}
				else
				{
					optValue(sound, weaponRule->getMeleeMissSound());
					if (damageRule)
					{
						optValue(anim, damageRule->getMeleeMissAnimation());
						optValue(animFrames, damageRule->getMeleeMissAnimationFrames());
						optValue(sound, damageRule->getMeleeMissSound());
					}
				}
			}
			else
			{
				optValue(anim, itemRule->getHitMissAnimation());
				optValue(animFrames, itemRule->getHitMissAnimationFrames());
				optValue(sound, itemRule->getHitMissSound());
			}
		}

		if (anim != -1)
		{
			Explosion *explosion = new Explosion(_center, anim, 0, false, (_hit || _psi), animFrames); // Don't burn the tile
			_parent->getMap()->getExplosions()->push_back(explosion);
		}
		if (_parent->getMap()->getFollowProjectile())
		{
			_parent->getMap()->getCamera()->setViewLevel(_center.z / 24);
		}

		if (_targetPsiOrHit && _parent->getSave()->getSide() == FACTION_HOSTILE && _targetPsiOrHit->getFaction() == FACTION_PLAYER)
		{
			_parent->getMap()->getCamera()->centerOnPosition(_center.toTile(), false);
		}
		// bullet hit sound
		_parent->playSound(sound, _center.toTile());
	}

	if (_attack.type == BA_SELF_DESTRUCT)
	{
		if (_attack.attacker)
		{
			_attack.attacker->setAlreadyExploded(false);
		}
	}
}

/**
 * Animates explosion sprites. If their animation is finished remove them from the list.
 * If the list is empty, this state is finished and the actual calculations take place.
 */
void ExplosionBState::think()
{
	if (!_parent->getMap()->getBlastFlash())
	{
		if (_parent->getMap()->getExplosions()->empty())
			explode();

		for (std::list<Explosion*>::iterator i = _parent->getMap()->getExplosions()->begin(); i != _parent->getMap()->getExplosions()->end();)
		{
			if (!(*i)->animate())
			{
				delete (*i);
				i = _parent->getMap()->getExplosions()->erase(i);
				if (_parent->getMap()->getExplosions()->empty())
				{
					explode();
					return;
				}
			}
			else
			{
				++i;
			}
		}
	}
}

/**
 * Explosions cannot be cancelled.
 */
void ExplosionBState::cancel()
{
}

/**
 * Calculates the effects of the explosion.
 */
void ExplosionBState::explode()
{
	bool terrainExplosion = false;
	SavedBattleGame *save = _parent->getSave();
	// last minute adjustment: determine if we actually
	if (_hit)
	{
		if (_attack.attacker && !_attack.attacker->isOut())
		{
			_attack.attacker->aim(false);
		}

		if (_power <= 0)
		{
			_parent->popState();
			return;
		}

		int sound = _attack.weapon_item->getRules()->getMeleeHitSound();
		if (_attack.weapon_item != _attack.damage_item)
		{
			// melee weapon with ammo
			optValue(sound, _attack.damage_item->getRules()->getMeleeHitSound());
		}
		_parent->playSound(sound, _center.toTile());
	}

	if (_tile)
	{
		terrainExplosion = true;
	}
	if (!_tile && !_attack.damage_item)
	{
		terrainExplosion = true;
	}

	// now check for new casualties
	_parent->checkForCasualties(_attack.damage_item ? _damageType : nullptr, _attack, false, terrainExplosion);
	// revive units if damage could give hp or reduce stun
	_parent->getSave()->reviveUnconsciousUnits(true);
	// if any unit get infected turn it to zombie
	_parent->convertInfected();

	// if this explosion was caused by a unit shooting, now it's the time to put the gun down
	if (_attack.attacker && !_attack.attacker->isOut() && _lowerWeapon)
	{
		_attack.attacker->aim(false);
	}

	if (_attack.damage_item && (_attack.damage_item->getRules()->getBattleType() == BT_GRENADE || _attack.damage_item->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
	{
		_parent->getSave()->removeItem(_attack.damage_item);
	}

	_parent->popState();

	// check for terrain explosions
	Tile *t = save->getTileEngine()->checkForTerrainExplosions();
	if (t)
	{
		Position p = t->getPosition().toVoxel();
		p += Position(8,8,0);
		_parent->statePushFront(new ExplosionBState(_parent, p, BattleActionAttack{ BA_NONE, _attack.attacker, }, t, false, 0, _explosionCounter + 1));
	}

	// Spawn a unit if the item does that
	if (_attack.damage_item && !_attack.damage_item->getRules()->getSpawnUnit().empty())
	{
		_parent->spawnNewUnit(_attack, _center.toTile());
	}
}

}
