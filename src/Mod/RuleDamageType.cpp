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
#include <cmath>
#include "RuleDamageType.h"
#include "../Engine/RNG.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Default constructor
 */
RuleDamageType::RuleDamageType() :
	FixRadius(0), RandomType(DRT_STANDARD), ResistType(DT_NONE), FireBlastCalc(false),
	IgnoreDirection(false), IgnoreSelfDestruct(false), IgnorePainImmunity(false), IgnoreNormalMoraleLose(false), IgnoreOverKill(false),
	ArmorEffectiveness(1.0f), RadiusEffectiveness(0.0f), RadiusReduction(10.0f),
	FireThreshold(1000), SmokeThreshold(1000),
	ToHealth(1.0f), ToMana(0.0f), ToArmor(0.1f), ToArmorPre(0.0f), ToWound(1.0f), ToItem(0.0f), ToTile(0.5f), ToStun(0.25f), ToEnergy(0.0f), ToTime(0.0f), ToMorale(0.0f),
	RandomHealth(false), RandomMana(false), RandomArmor(false), RandomArmorPre(false), RandomWound(true), RandomItem(false), RandomTile(false), RandomStun(true), RandomEnergy(false), RandomTime(false), RandomMorale(false),
	TileDamageMethod(1)
{

}


/**
 * Function converting power to damage.
 * @param power Input power.
 * @return Random damage based on power.
 */
int RuleDamageType::getRandomDamage(int power) const
{
	return getRandomDamage(power, +[](int min, int max){ return RNG::generate(min, max); });
}

/**
 * Function converting power to damage.
 * @param power Input power.
 * @param mode Calc mode (0 = dmg, 1 = min dmg, 2 = max dmg)
 * @return Random damage based on power.
 */
int RuleDamageType::getRandomDamage(int power, int mode) const
{
	if (mode == 0)
	{
		return getRandomDamage(power);
	}
	else if (mode == 1)
	{
		return getRandomDamage(power, +[](int min, int max){ return min; });
	}
	else
	{
		return getRandomDamage(power, +[](int min, int max){ return max; });
	}
}

/**
 * Function converting power to damage.
 * @param power Input power.
 * @param randFunc Function to get random value
 * @return Random damage based on power.
 */
int RuleDamageType::getRandomDamage(int power, FuncRef<int(int, int)> randFunc) const
{
	ItemDamageRandomType randType = RandomType;
	if (randType == DRT_UFO_WITH_TWO_DICE)
	{
		int firstThrow = randFunc(0, power);
		int secondThrow = randFunc(0, power);

		return firstThrow + secondThrow;
	}
	else if (randType == DRT_EASY)
	{
		int min = power / 2; // 50%
		int max = power * 2; // 200%

		return randFunc(min, max);
	}

	const bool def = randType == DRT_DEFAULT;
	if (def)
	{
		switch (ResistType)
		{
		case DT_NONE: randType = DRT_NONE; break;
		case DT_IN: randType = DRT_FIRE; break;
		case DT_HE: randType = DRT_EXPLOSION; break;
		case DT_SMOKE: randType = DRT_NONE; break;
		default: randType = DRT_STANDARD; break;
		}
	}

	int dmgRng = 0;
	switch (randType)
	{
	case DRT_UFO: dmgRng = 100; break;
	case DRT_TFTD: dmgRng = 50; break;
	case DRT_STANDARD: dmgRng = Mod::DAMAGE_RANGE; break;
	case DRT_EXPLOSION: dmgRng = Mod::EXPLOSIVE_DAMAGE_RANGE; break;
	case DRT_FLAT: dmgRng = 0; break;
	case DRT_FIRE:
		return randFunc(Mod::FIRE_DAMAGE_RANGE[0], Mod::FIRE_DAMAGE_RANGE[1]);

	case DRT_NONE: return 0;
	default: return 0;
	}

	int min = power * (100 - dmgRng) / 100;
	int max = power * (100 + dmgRng) / 100;

	return randFunc(min, max);
}

/**
 * Calculate random value of damage for tile attack.
 * @param power Raw power of attack
 * @param damage Random damage done to units
 * @return Random damage for tile.
 */
int RuleDamageType::getRandomDamageForTile(int power, int damage) const
{
	return TileDamageMethod == 1 ? RNG::generate(power / 2, 3 * power / 2) : damage;
}

/**
 * Is this damage single target.
 * @return True if it can only hit one target.
 */
bool RuleDamageType::isDirect() const
{
	return FixRadius == 0;
}

/**
 * Load rule from YAML.
 * @param node Node with data.
 */
void RuleDamageType::load(const YAML::YamlNodeReader& node)
{
	const auto& reader = node.useIndex();
	reader.tryRead("FixRadius", FixRadius);
	reader.tryRead("RandomType", RandomType);
	reader.tryRead("ResistType", ResistType);
	reader.tryRead("FireBlastCalc", FireBlastCalc);
	reader.tryRead("IgnoreDirection", IgnoreDirection);
	reader.tryRead("IgnoreSelfDestruct", IgnoreSelfDestruct);
	reader.tryRead("IgnorePainImmunity", IgnorePainImmunity);
	reader.tryRead("IgnoreNormalMoraleLose", IgnoreNormalMoraleLose);
	reader.tryRead("IgnoreOverKill", IgnoreOverKill);
	reader.tryRead("ArmorEffectiveness", ArmorEffectiveness);
	reader.tryRead("RadiusEffectiveness", RadiusEffectiveness);
	reader.tryRead("RadiusReduction", RadiusReduction);

	reader.tryRead("FireThreshold", FireThreshold);
	reader.tryRead("SmokeThreshold", SmokeThreshold);

	reader.tryRead("ToHealth", ToHealth);
	reader.tryRead("ToMana", ToMana);
	reader.tryRead("ToArmor", ToArmor);
	reader.tryRead("ToArmorPre", ToArmorPre);
	reader.tryRead("ToWound", ToWound);
	reader.tryRead("ToItem", ToItem);
	reader.tryRead("ToTile", ToTile);
	reader.tryRead("ToStun", ToStun);
	reader.tryRead("ToEnergy", ToEnergy);
	reader.tryRead("ToTime", ToTime);
	reader.tryRead("ToMorale", ToMorale);

	reader.tryRead("RandomHealth", RandomHealth);
	reader.tryRead("RandomMana", RandomMana);
	reader.tryRead("RandomArmor", RandomArmor);
	reader.tryRead("RandomArmorPre", RandomArmorPre);
	reader.tryRead("RandomWound", RandomWound);
	reader.tryRead("RandomItem", RandomItem);
	reader.tryRead("RandomTile", RandomTile);
	reader.tryRead("RandomStun", RandomStun);
	reader.tryRead("RandomEnergy", RandomEnergy);
	reader.tryRead("RandomTime", RandomTime);
	reader.tryRead("RandomMorale", RandomMorale);

	reader.tryRead("TileDamageMethod", TileDamageMethod);
}

namespace
{
/**
 * Helper function for calculating damage from damage.
 * @param random
 * @param multiplier
 * @param damage
 * @return Damage to something.
 */
int getDamageHelper(bool random, float multipler, int damage)
{
	if (damage > 0)
	{
		if (random)
		{
			return (int)std::round(RNG::generate(0, damage) * multipler);
		}
		else
		{
			return (int)std::round(damage * multipler);
		}
	}
	return 0;
}

}

/**
 * Get final damage value to health based on damage.
 */
int RuleDamageType::getHealthFinalDamage(int damage) const
{
	return getDamageHelper(RandomHealth, ToHealth, damage);
}

/**
 * Get final damage value to mana based on damage.
 */
int RuleDamageType::getManaFinalDamage(int damage) const
{
	return getDamageHelper(RandomMana, ToMana, damage);
}

/**
 * Get final damage value to armor based on damage.
 */
int RuleDamageType::getArmorFinalDamage(int damage) const
{
	return getDamageHelper(RandomArmor, ToArmor, damage);
}

/**
 * Get final damage value to armor based on damage before armor reduction.
 */
int RuleDamageType::getArmorPreFinalDamage(int damage) const
{
	return getDamageHelper(RandomArmorPre, ToArmorPre, damage);
}

/**
 * Get numbers of wound based on damage.
 */
int RuleDamageType::getWoundFinalDamage(int damage) const
{
	if (damage > 0)
	{
		if (RandomWound)
		{
			if (RNG::generate(0, 10) < int(damage * ToWound))
			{
				return RNG::generate(1,3);
			}
		}
		else
		{
			return (int)std::round(damage * ToWound);
		}
	}
	return 0;
}

/**
 * Get final damage value to item based on damage.
 */
int RuleDamageType::getItemFinalDamage(int damage) const
{
	return getDamageHelper(RandomItem, ToItem, damage);
}

/**
 * Get final damage value to tile based on damage.
 */
int RuleDamageType::getTileFinalDamage(int damage) const
{
	return getDamageHelper(RandomTile, ToTile, damage);
}

/**
 * Get stun level change based on damage.
 */
int RuleDamageType::getStunFinalDamage(int damage) const
{
	return getDamageHelper(RandomStun, ToStun, damage);
}

/**
 * Get energy change based on damage.
 */
int RuleDamageType::getEnergyFinalDamage(int damage) const
{
	return getDamageHelper(RandomEnergy, ToEnergy, damage);
}

/**
 * Get time units change based on damage.
 */
int RuleDamageType::getTimeFinalDamage(int damage) const
{
	return getDamageHelper(RandomTime, ToTime, damage);
}

/**
 * Get morale change based on damage.
 */
int RuleDamageType::getMoraleFinalDamage(int damage) const
{
	return getDamageHelper(RandomMorale, ToMorale, damage);
}

} //namespace OpenXcom
