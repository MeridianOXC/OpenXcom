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
#include <string>
#include "../Engine/Yaml.h"
#include "RuleCraft.h"
#include "../Savegame/CraftWeaponProjectile.h"

namespace OpenXcom
{

class Mod;
class RuleItem;

enum CraftWeaponCategory { CWC_WEAPON, CWC_TRACTOR_BEAM, CWC_EQUIPMENT };

/**
 * Represents a specific type of craft weapon.
 * Contains constant info about a craft weapon like
 * damage, range, accuracy, items used, etc.
 * @sa CraftWeapon
 */
class RuleCraftWeapon
{
private:
	std::string _type;
	std::string _tooltip;
	int _sprite, _sound, _damage, _shieldDamageModifier, _range, _accuracy, _reloadCautious, _reloadStandard, _reloadAggressive, _ammoMax, _rearmRate, _projectileSpeed, _weaponType;
	CraftWeaponProjectileType _projectileType;
	std::string _launcherName, _clipName;
	const RuleItem* _launcher;
	const RuleItem* _clip;
	RuleCraftStats _stats;
	bool _underwaterOnly;
	int _tractorBeamPower;
	bool _hidePediaInfo;
	bool _statisticalBulletSaving;
	bool _unifiedDamageFormula;
public:
	/// Creates a blank craft weapon ruleset.
	RuleCraftWeapon(const std::string &type);
	/// Cleans up the craft weapon ruleset.
	~RuleCraftWeapon();
	/// Loads craft weapon data from YAML.
	void load(const YAML::YamlNodeReader& reader, Mod *mod);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);

	/// Gets the craft weapon's type.
	const std::string& getType() const;
	/// Gets the craft weapon's tooltip.
	const std::string& getTooltip() const { return _tooltip; }
	/// Gets the craft weapon's sprite.
	int getSprite() const;
	/// Gets the craft weapon's sound.
	int getSound() const;
	/// Gets the craft weapon's damage.
	int getDamage() const;
	/// Should the weapon's stats be displayed in Ufopedia or not?
	bool getHidePediaInfo() const { return _hidePediaInfo; }
	/// Should the statistical bullet saving be used or not?
	bool useStatisticalBulletSaving() const { return _statisticalBulletSaving; }
	/// Should unified or vanilla formula be used?
	bool unifiedDamageFormula() const { return _unifiedDamageFormula; }
	/// Gets the craft weapon's effectiveness against shields.
	int getShieldDamageModifier() const;
	/// Gets the craft weapon's range.
	int getRange() const;
	/// Gets the craft weapon's accuracy.
	int getAccuracy() const;
	/// Gets the craft weapon's cautious reload time.
	int getCautiousReload() const;
	/// Gets the craft weapon's standard reload time.
	int getStandardReload() const;
	/// Gets the craft weapon's aggressive reload time.
	int getAggressiveReload() const;
	/// Gets the craft weapon's maximum ammo.
	int getAmmoMax() const;
	/// Gets the craft weapon's rearm rate.
	int getRearmRate() const;
	/// Gets the craft weapon's launcher item.
	const RuleItem* getLauncherItem() const;
	/// Gets the craft weapon's clip item.
	const RuleItem* getClipItem() const;
	/// Gets the craft weapon's projectile's type.
	CraftWeaponProjectileType getProjectileType() const;
	/// Gets the craft weapon's projectile speed.
	int getProjectileSpeed() const;
	/// Gets weapon type used by craft slots.
	int getWeaponType() const;
	/// Gets bonus stats given by this weapon.
	const RuleCraftStats& getBonusStats() const;
	/// Is this item restricted to use underwater?
	bool isWaterOnly() const;
	/// Get the craft weapon's tractor beam power
	int getTractorBeamPower() const;
};

}
