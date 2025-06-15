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
#include "RuleCraftWeapon.h"
#include "Mod.h"
#include "../Engine/Logger.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of craft weapon.
 * @param type String defining the type.
 */
RuleCraftWeapon::RuleCraftWeapon(const std::string &type) :
	_type(type), _sprite(-1), _sound(-1), _damage(0), _shieldDamageModifier(100), _range(0), _accuracy(0),
	_reloadCautious(0), _reloadStandard(0), _reloadAggressive(0), _ammoMax(0),
	_rearmRate(1), _projectileSpeed(0), _weaponType(0), _projectileType(CWPT_CANNON_ROUND),
	_stats(), _underwaterOnly(false),
	_tractorBeamPower(0), _hidePediaInfo(false), _statisticalBulletSaving(false), _unifiedDamageFormula(false)
{
}

/**
 *
 */
RuleCraftWeapon::~RuleCraftWeapon()
{
}

/**
 * Loads the craft weapon from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the craft weapon.
 */
void RuleCraftWeapon::load(const YAML::YamlNodeReader& node, Mod *mod)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	reader.tryRead("tooltip", _tooltip);
	if (reader["stats"])
	{
		_stats.load(reader["stats"]);
	}
	if (reader["sprite"])
	{
		// used in
		// Surface set (baseOffset):
		//   BASEBITS.PCK (48)
		//   INTICON.PCK (5)
		//
		// Final index in surfaceset is `baseOffset + sprite + (sprite > 5 ? modOffset : 0)`
		_sprite = mod->getOffset(reader["sprite"].readVal(_sprite), 5);
	}
	mod->loadSoundOffset(_type, _sound, reader["sound"], "GEO.CAT");
	reader.tryRead("damage", _damage);
	reader.tryRead("unifiedDamageFormula", _unifiedDamageFormula);
	reader.tryRead("shieldDamageModifier", _shieldDamageModifier);
	reader.tryRead("range", _range);
	reader.tryRead("accuracy", _accuracy);
	reader.tryRead("reloadCautious", _reloadCautious);
	reader.tryRead("reloadStandard", _reloadStandard);
	reader.tryRead("reloadAggressive", _reloadAggressive);
	reader.tryRead("ammoMax", _ammoMax);
	reader.tryRead("rearmRate", _rearmRate);
	reader.tryRead("projectileType", _projectileType);
	reader.tryRead("projectileSpeed", _projectileSpeed);
	reader.tryRead("launcher", _launcherName);
	reader.tryRead("clip", _clipName);
	reader.tryRead("weaponType", _weaponType);
	reader.tryRead("underwaterOnly", _underwaterOnly);
	reader.tryRead("tractorBeamPower", _tractorBeamPower);
	reader.tryRead("hidePediaInfo", _hidePediaInfo);
	reader.tryRead("bulletSaving", _statisticalBulletSaving);
}


/**
 * Cross link with other rules.
 */
void RuleCraftWeapon::afterLoad(const Mod* mod)
{
	mod->linkRule(_launcher, _launcherName);
	mod->linkRule(_clip, _clipName);

	if (_unifiedDamageFormula && !_launcher && !_clip)
	{
		throw Exception("Unified damage formula requires `clip` or `launcher` to be defined.");
	}

	if (_projectileType < CWPT_LASER_BEAM && _damage > 0)
	{
		if (_projectileSpeed <= 0)
		{
			throw Exception("Missile-like craft weapons (with 'damage' > 0) must have a positive 'projectileSpeed'.");
		}
		else if (_projectileSpeed <= 4 && _range > 10)
		{
			Log(LOG_WARNING) << "Missile speed for " << _type << " is very low! Depending on craft approach speed, the missile may seem not moving, or even moving backwards. Speed: " << _projectileSpeed << "; range: " << _range;
		}
		else if (_projectileSpeed <= 5 && _range > 20)
		{
			Log(LOG_INFO) << "Missile speed for " << _type << " is quite low. Depending on craft approach speed, the missile may seem moving very slowly. Speed: " << _projectileSpeed << "; range: " << _range;
		}
	}
	if (_launcher == nullptr)
	{
		throw Exception("Launcher item is required for a craft weapon");
	}
	if (_ammoMax)
	{
		if (_rearmRate <= 0)
		{
			throw Exception("Attribute 'rearmRate' must be positive when 'ammoMax' is set");
		}
		if (_clip && _clip->getClipSize() > _rearmRate)
		{
			throw Exception("Attribute 'clipSize' of the clip item is too big for the given 'rearmRate'");
		}
	}
}


/**
 * Gets the language string that names this craft weapon.
 * Each craft weapon type has a unique name.
 * @return The craft weapon's name.
 */
const std::string& RuleCraftWeapon::getType() const
{
	return _type;
}

/**
 * Gets the ID of the sprite used to draw the craft weapon
 * in the Equip Craft and Interception screens.
 * @return The sprite ID.
 */
int RuleCraftWeapon::getSprite() const
{
	return _sprite;
}

/**
 * Gets the ID of the sound used when firing the weapon
 * in the Dogfight screen.
 * @return The sound ID.
 */
int RuleCraftWeapon::getSound() const
{
	return _sound;
}

/**
 * Gets the amount of damage this craft weapon
 * inflicts on enemy crafts.
 * @return The damage amount.
 */
int RuleCraftWeapon::getDamage() const
{
	return _damage;
}

/**
 * Gets the percent effectiveness of this craft weapon against shields
 * @return modifier to damage against shields
 */
int RuleCraftWeapon::getShieldDamageModifier() const
{
	return _shieldDamageModifier;
}

/**
 * Gets the maximum range of this craft weapon.
 * @return The range in km.
 */
int RuleCraftWeapon::getRange() const
{
	return _range;
}

/**
 * Gets the percentage chance of each shot of
 * this craft weapon hitting an enemy craft.
 * @return The accuracy as a percentage.
 */
int RuleCraftWeapon::getAccuracy() const
{
	return _accuracy;
}

/**
 * Gets the amount of time the craft weapon takes to
 * reload in cautious mode.
 * @return The time in game seconds.
 */
int RuleCraftWeapon::getCautiousReload() const
{
	return _reloadCautious;
}

/**
 * Gets the amount of time the craft weapon takes to
 * reload in standard mode.
 * @return The time in game seconds.
 */
int RuleCraftWeapon::getStandardReload() const
{
	return _reloadStandard;
}

/**
 * Gets the amount of time the craft weapon takes to
 * reload in aggressive mode.
 * @return The time in game seconds.
 */
int RuleCraftWeapon::getAggressiveReload() const
{
	return _reloadAggressive;
}

/**
 * Gets the maximum amount of ammo the craft weapon
 * can carry.
 * @return The amount of ammo.
 */
int RuleCraftWeapon::getAmmoMax() const
{
	return _ammoMax;
}

/**
 * Gets how much ammo is added to the craft weapon
 * while rearming (the amount of ammo in each clip item).
 * @return The amount of ammo.
 */
int RuleCraftWeapon::getRearmRate() const
{
	return _rearmRate;
}

/**
 * Gets the rule of the item used to
 * equip this craft weapon.
 * @return The item rule.
 */
const RuleItem* RuleCraftWeapon::getLauncherItem() const
{
	return _launcher;
}

/**
 * Gets the rule of the item used to
 * load this craft weapon with ammo.
 * @return The item rule.
 */
const RuleItem* RuleCraftWeapon::getClipItem() const
{
	return _clip;
}

/**
 * Gets the Projectile Type this weapon will fire
 * @return The projectile type.
 */
CraftWeaponProjectileType RuleCraftWeapon::getProjectileType() const
{
	return _projectileType;
}

/**
 * Gets the speed of the projectile fired by this weapon
 * @return The projectile speed.
 */
int RuleCraftWeapon::getProjectileSpeed() const
{
	return _projectileSpeed;
}

/**
 * Gets weapon type used by craft slots.
 * @return What type of slot is valid to equip this weapon.
 */
int RuleCraftWeapon::getWeaponType() const
{
	return _weaponType;
}

/**
 * Gets bonus stats given by this weapon.
 */
const RuleCraftStats& RuleCraftWeapon::getBonusStats() const
{
	return _stats;
}

/**
 * Can this item be used on land or is it underwater only?
 * @return if this is an underwater weapon or not.
 */
bool RuleCraftWeapon::isWaterOnly() const
{
	return _underwaterOnly;
}

/**
 * Get the craft weapon's tractor beam power
 * @return The tractor beam power.
 */
int RuleCraftWeapon::getTractorBeamPower() const
{
	return _tractorBeamPower;
}

}
