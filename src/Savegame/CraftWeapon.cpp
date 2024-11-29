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
#include <cmath>
#include <algorithm>
#include <cmath>
#include "CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "CraftWeaponProjectile.h"
#include "../Engine/RNG.h"

namespace OpenXcom
{

/**
 * Initializes a craft weapon of the specified type.
 * @param rules Pointer to ruleset.
 * @param ammo Initial ammo.
 */
CraftWeapon::CraftWeapon(RuleCraftWeapon *rules, int ammo) : _rules(rules), _ammo(ammo), _rearming(false), _disabled(false)
{
}

/**
 *
 */
CraftWeapon::~CraftWeapon()
{
}

/**
 * Loads the craft weapon from a YAML file.
 * @param node YAML node.
 */
void CraftWeapon::load(const YAML::YamlNodeReader& reader)
{
	reader.tryRead("ammo", _ammo);
	reader.tryRead("rearming", _rearming);
	reader.tryRead("disabled", _disabled);
}

/**
 * Saves the base to a YAML file.
 * @return YAML node.
 */
void CraftWeapon::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();

	writer.write("type", _rules->getType());
	writer.write("ammo", _ammo);
	if (_rearming)
		writer.write("rearming", _rearming);
	if (_disabled)
		writer.write("disabled", _disabled);
}

/**
 * Returns the ruleset for the craft weapon's type.
 * @return Pointer to ruleset.
 */
RuleCraftWeapon *CraftWeapon::getRules() const
{
	return _rules;
}

/**
 * Returns the ammo contained in this craft weapon.
 * @return Weapon ammo.
 */
int CraftWeapon::getAmmo() const
{
	return _ammo;
}

/**
 * Changes the ammo contained in this craft weapon.
 * @param ammo Weapon ammo.
 * @return If the weapon ran out of ammo.
 */
bool CraftWeapon::setAmmo(int ammo)
{
	_ammo = ammo;
	if (_ammo < 0)
	{
		_ammo = 0;
		return false;
	}
	if (_ammo > _rules->getAmmoMax())
	{
		_ammo = _rules->getAmmoMax();
	}
	return true;
}

/**
 * Returns whether this craft weapon needs rearming.
 * @return Rearming status.
 */
bool CraftWeapon::isRearming() const
{
	if (_disabled)
		return false;

	return _rearming;
}

/**
 * Changes whether this craft weapon needs rearming
 * (for example, in case there's no more ammo).
 * @param rearming Rearming status.
 */
void CraftWeapon::setRearming(bool rearming)
{
	_rearming = rearming;
}

/**
 * Returns whether this craft weapon is disabled.
 * @return Disabled status.
 */
bool CraftWeapon::isDisabled() const
{
	return _disabled;
}

/**
 * Sets whether this craft weapon is disabled or not.
 * @param disabled Disabled status.
 */
void CraftWeapon::setDisabled(bool disabled)
{
	_disabled = disabled;
}

/**
 * Rearms this craft weapon's ammo.
 * @param available number of clips available.
 * @param clipSize number of rounds in said clips.
 * @return number of clips used.
 */
int CraftWeapon::rearm(const int available, const int clipSize)
{
	int ammoUsed = _rules->getRearmRate();
	int clipsSaved = 0;

	if (clipSize > 0)
	{	// +(clipSize - 1) correction for rounding up
		int needed = std::min(_rules->getRearmRate(), _rules->getAmmoMax() - _ammo + clipSize - 1) / clipSize;
		ammoUsed = ((needed > available)? available : needed) * clipSize;

		// statistical bullet saving
		if (clipSize > 1 && _rules->useStatisticalBulletSaving())
		{
			int overusedAmmo = _ammo + ammoUsed - _rules->getAmmoMax();
			if (overusedAmmo > 0)
			{
				if (RNG::generate(0, clipSize - 1) < overusedAmmo)
				{
					clipsSaved = 1;
				}
			}
		}
	}

	setAmmo(_ammo + ammoUsed);

	_rearming = _ammo < _rules->getAmmoMax();

	return (clipSize <= 0)? 0 : (ammoUsed / clipSize) - clipsSaved;
}

/*
 * Fires a projectile from crafts weapon.
 * @return Pointer to the new projectile.
 */
CraftWeaponProjectile* CraftWeapon::fire() const
{
	CraftWeaponProjectile *p = new CraftWeaponProjectile();
	p->setType(this->getRules()->getProjectileType());
	p->setSpeed(this->getRules()->getProjectileSpeed());
	p->setAccuracy(this->getRules()->getAccuracy());
	p->setDamage(this->getRules()->getDamage());
	p->setRange(this->getRules()->getRange());
	p->setShieldDamageModifier(this->getRules()->getShieldDamageModifier());
	return p;
}

/*
 * get how many clips are loaded into this weapon.
 * @return number of clips loaded.
 */
int CraftWeapon::getClipsLoaded() const
{
	int retVal = (int)floor((double)_ammo / _rules->getRearmRate());
	auto* clip = _rules->getClipItem();

	if (clip && clip->getClipSize() > 0)
	{
		retVal = (int)floor((double)_ammo / clip->getClipSize());
	}

	return retVal;
}

}
