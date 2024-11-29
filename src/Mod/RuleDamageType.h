#pragma once
/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "../Engine/Yaml.h"
#include "../Engine/Functions.h"

namespace OpenXcom
{

enum ItemDamageType { DT_NONE, DT_AP, DT_IN, DT_HE, DT_LASER, DT_PLASMA, DT_STUN, DT_MELEE, DT_ACID, DT_SMOKE, DT_10, DT_11, DT_12, DT_13, DT_14, DT_15, DT_16, DT_17, DT_18, DT_19, DAMAGE_TYPES };
enum ItemDamageRandomType
{
	DRT_DEFAULT = 0,
	DRT_UFO = 1,
	DRT_TFTD = 2,
	DRT_FLAT = 3,
	DRT_FIRE = 4,
	DRT_NONE = 5,
	DRT_UFO_WITH_TWO_DICE = 6,
	DRT_EASY = 7,
	DRT_STANDARD = 8,
	DRT_EXPLOSION = 9,
};

/**
 * Represents a specific damage type.
 * @sa Item
 * @sa RuleItem
 */
struct RuleDamageType
{
	/// Set fixed arena of effect radius.
	int FixRadius;
	/// Set what random range use when calculating damage.
	ItemDamageRandomType RandomType;
	/// Set what resistance on armor is used.
	ItemDamageType ResistType;
	/// Use fire calculation of radius and power.
	bool FireBlastCalc;
	/// Damage type ignore direction of attack.
	bool IgnoreDirection;
	/// Damage type do not cause self destruct explosion.
	bool IgnoreSelfDestruct;
	/// Damage type can do stun damage to big units.
	bool IgnorePainImmunity;
	/// Damage type doesn't cause morale loss from health damage.
	bool IgnoreNormalMoraleLose;
	/// Damage type do not move health to negative values.
	bool IgnoreOverKill;
	/// How much of armor is ignored.
	float ArmorEffectiveness;
	/// Conversion from power to max explosion radius.
	float RadiusEffectiveness;
	/// Reduction of explosion power per tile.
	float RadiusReduction;
	/// Threshold of fire creation
	int FireThreshold;
	/// Threshold of smoke creation
	int SmokeThreshold;

	/// Conversion from power to unit damage.
	float ToHealth;
	/// Conversion from power to mana loss.
	float ToMana;
	/// Conversion from power to unit armor damage.
	float ToArmor;
	/// Conversion from power to unit armor damage before applying armor protection.
	float ToArmorPre;
	/// Conversion from power to wound chance.
	float ToWound;
	/// Conversion from power to item damage.
	float ToItem;
	/// Conversion from power to tile damage.
	float ToTile;
	/// Conversion from power to stun level.
	float ToStun;
	/// Conversion from power to energy lose.
	float ToEnergy;
	/// Conversion from power to time units lose.
	float ToTime;
	/// Conversion from power to morale lose.
	float ToMorale;

	/// Damage type use random conversion to health damage.
	bool RandomHealth;
	/// Damage type use random conversion mana loss.
	bool RandomMana;
	///Damage type use random conversion armor damage.
	bool RandomArmor;
	/// Damage type use random conversion armor pre damage.
	bool RandomArmorPre;
	/// Damage type use random chance for wound number or linear.
	bool RandomWound;
	/// Damage type use random conversion item damage.
	bool RandomItem;
	/// Damage type use random conversion tile damage.
	bool RandomTile;
	/// Damage type use random conversion stun level.
	bool RandomStun;
	/// Damage type use random conversion energy lose.
	bool RandomEnergy;
	/// Damage type use random conversion time units lose.
	bool RandomTime;
	/// Damage type use random conversion morale lose.
	bool RandomMorale;

	/// Which tile damage method should be used?
	/// A. For AOE (area-of-effect) weapons:
	///   TileDamageMethod==1: (50%-150% * power * ToTile)
	///   TileDamageMethod==2: ((power x RandomTile) * ToTile), which defaults to vanilla 50% damage for HE and 0% for stun, smoke and incendiary
	/// B. For non-AOE (projectile) weapons:
	///   TileDamageMethod==1: (50%-150% * power * ToTile), which defaults to vanilla 25%-75% damage for AP/laser/plasma/etc.
	///   TileDamageMethod==2: ((damage x RandomTile) * ToTile)
	int TileDamageMethod;

	/// Default constructor.
	RuleDamageType();
	/// Calculate random value of damage.
	int getRandomDamage(int power) const;
	/// Calculate random value of damage.
	int getRandomDamage(int power, int mode) const;
	/// Calculate random value of damage using given random func.
	int getRandomDamage(int power, FuncRef<int(int, int)> randFunc) const;
	/// Calculate random value of damage for tile attack.
	int getRandomDamageForTile(int power, int damage) const;
	/// Do this damage type affect only one target
	bool isDirect() const;
	/// Loads item data from YAML.
	void load(const YAML::YamlNodeReader& reader);

	/// Get final damage value to health based on damage.
	int getHealthFinalDamage(int damage) const;
	/// Get final damage value to mana based on damage.
	int getManaFinalDamage(int damage) const;
	/// Get final damage value to armor based on damage.
	int getArmorFinalDamage(int damage) const;
	/// Get final damage value to armor based on damage before armor reduction.
	int getArmorPreFinalDamage(int damage) const;
	/// Get numbers of wound based on damage.
	int getWoundFinalDamage(int damage) const;
	/// Get final damage value to item based on damage.
	int getItemFinalDamage(int damage) const;
	/// Get final damage value to tile based on damage.
	int getTileFinalDamage(int damage) const;
	/// Get stun level change based on damage.
	int getStunFinalDamage(int damage) const;
	/// Get energy change based on damage.
	int getEnergyFinalDamage(int damage) const;
	/// Get time units change based on damage.
	int getTimeFinalDamage(int damage) const;
	/// Get morale change based on damage.
	int getMoraleFinalDamage(int damage) const;
};

} //namespace OpenXcom
