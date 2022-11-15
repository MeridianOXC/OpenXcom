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
#include <assert.h>
#include "Unit.h"
#include "RuleStatBonus.h"
#include "RuleSkill.h"
#include "../Engine/ScriptBind.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../fmath.h"

namespace OpenXcom
{

namespace
{

typedef float (*BonusStatFunc)(const BattleUnit*);

/**
 * Function returning static value.
 */
template<int I>
float stat0(const BattleUnit *unit)
{
	return I;
}
/**
 * Getter for one basic stat of unit.
 */
template<UnitStats::Ptr field>
float stat1(const BattleUnit *unit)
{
	const UnitStats *stat = unit->getBaseStats();
	return stat->*field;
}

/**
 * Getter for multiply of two basic stat of unit.
 */
template<UnitStats::Ptr fieldA, UnitStats::Ptr fieldB>
float stat2(const BattleUnit *unit)
{
	const UnitStats *stat = unit->getBaseStats();
	return (stat->*fieldA) * (stat->*fieldB);
}

float currentFatalWounds(const BattleUnit *unit)
{
	return unit->getFatalWounds();
}

float currentRank(const BattleUnit *unit)
{
	return unit->getRankInt();
}

float currentTimeUnits(const BattleUnit *unit)
{
	return unit->getTimeUnits();
}

float currentHealth(const BattleUnit *unit)
{
	return unit->getHealth();
}

float currentMana(const BattleUnit* unit)
{
	return unit->getMana();
}

float currentEnergy(const BattleUnit *unit)
{
	return unit->getEnergy();
}

float currentMorale(const BattleUnit *unit)
{
	return unit->getMorale();
}

float currentStun(const BattleUnit *unit)
{
	return unit->getStunlevel();
}

float normalizedHelper(int val, int max)
{
	return max > 0 ? 1.0f * val / max : 0.0f;
}

float normalizedTimeUnits(const BattleUnit *unit)
{
	return normalizedHelper(unit->getTimeUnits(), unit->getBaseStats()->tu);
}

float normalizedHealth(const BattleUnit *unit)
{
	return normalizedHelper(unit->getHealth(), unit->getBaseStats()->health);
}

float normalizedMana(const BattleUnit* unit)
{
	return normalizedHelper(unit->getMana(), unit->getBaseStats()->mana);
}

float normalizedEnergy(const BattleUnit *unit)
{
	return normalizedHelper(unit->getEnergy(), unit->getBaseStats()->stamina);
}

float normalizedMorale(const BattleUnit *unit)
{
	return normalizedHelper(unit->getMorale(), 100);
}

float normalizedStun(const BattleUnit *unit)
{
	return normalizedHelper(unit->getStunlevel(), unit->getHealth());
}

float basicEnergyRegeneration(const BattleUnit *unit)
{
	Soldier *solder = unit->getGeoscapeSoldier();
	if (solder != 0)
	{
		return solder->getInitStats()->tu / 3;
	}
	else
	{
		return unit->getUnitRules()->getEnergyRecovery();
	}
}

constexpr size_t statDataFuncSize = 4;
constexpr size_t statMultiper = 1000;
constexpr const char* statNamePostfix = "BonusStats";

template<BonusStatFunc Func>
struct getBonusStatsScript
{
	static RetEnum func(const BattleUnit *bu, int &ret, int pow1, int pow2, int pow3, int pow4)
	{
		if (bu)
		{
			const float stat = Func(bu);
			float bonus = 0;
			bonus += pow4; bonus *= stat;
			bonus += pow3; bonus *= stat;
			bonus += pow2; bonus *= stat;
			bonus += pow1; bonus *= stat;
			ret += bonus / statMultiper;
		}
		return RetContinue;
	}
};

/**
 * Data describing same functions but with different exponent.
 */
using BonusStatDataFunc = void (*)(Bind<BattleUnit>& b, const std::string& name);
/**
 * Data describing basic stat getter.
 */
struct BonusStatData
{
	std::string name;
	BonusStatDataFunc func;
};

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<BonusStatFunc Func>
BonusStatDataFunc create()
{
	return [](Bind<BattleUnit>& b, const std::string& name)
	{
		b.addFunc<getBonusStatsScript<Func>>(name + statNamePostfix, "add stat '" + name + "' transformed by polynomial (const arguments are coefficients), final result of polynomial is divided by " + std::to_string(statMultiper));
	};
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<int Val>
BonusStatDataFunc create0()
{
	return create<&stat0<Val> >();
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<UnitStats::Ptr fieldA>
BonusStatDataFunc create1()
{
	return create<&stat1<fieldA> >();
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<UnitStats::Ptr fieldA, UnitStats::Ptr fieldB>
BonusStatDataFunc create2()
{
	return create<&stat2<fieldA, fieldB> >();
}

/**
 * List of all possible getters of basic stats.
 */
BonusStatData statDataMap[] =
{
	{ "flatOne", create0<1>() },
	{ "flatHundred", create0<100>() },
	{ "strength", create1<&UnitStats::strength>() },
	{ "psi", create2<&UnitStats::psiSkill, &UnitStats::psiStrength>() },
	{ "psiSkill", create1<&UnitStats::psiSkill>() },
	{ "psiStrength", create1<&UnitStats::psiStrength>() },
	{ "throwing", create1<&UnitStats::throwing>() },
	{ "bravery", create1<&UnitStats::bravery>() },
	{ "firing", create1<&UnitStats::firing>() },
	{ "health", create1<&UnitStats::health>() },
	{ "mana", create1<&UnitStats::mana>() },
	{ "tu", create1<&UnitStats::tu>() },
	{ "reactions", create1<&UnitStats::reactions>() },
	{ "stamina", create1<&UnitStats::stamina>() },
	{ "melee", create1<&UnitStats::melee>() },
	{ "strengthMelee", create2<&UnitStats::strength, &UnitStats::melee>() },
	{ "strengthThrowing", create2<&UnitStats::strength, &UnitStats::throwing>() },
	{ "firingReactions", create2<&UnitStats::firing, &UnitStats::reactions>() },

	{ "rank", create<&currentRank>() },
	{ "fatalWounds", create<&currentFatalWounds>() },

	{ "healthCurrent", create<&currentHealth>() },
	{ "manaCurrent", create<&currentMana>() },
	{ "tuCurrent", create<&currentTimeUnits>() },
	{ "energyCurrent", create<&currentEnergy>() },
	{ "moraleCurrent", create<&currentMorale>() },
	{ "stunCurrent", create<&currentStun>() },

	{ "healthNormalized", create<&normalizedHealth>() },
	{ "manaNormalized", create<&normalizedMana>() },
	{ "tuNormalized", create<&normalizedTimeUnits>() },
	{ "energyNormalized", create<&normalizedEnergy>() },
	{ "moraleNormalized", create<&normalizedMorale>() },
	{ "stunNormalized", create<&normalizedStun>() },

	{ "energyRegen", create<&basicEnergyRegeneration>() },
};

} //namespace

/**
 * Default constructor.
 */
RuleStatBonus::RuleStatBonus()
{

}
/**
 * Loads the item from a YAML file.
 * @param node YAML node.
 */
void RuleStatBonus::load(const std::string& parentName, const YAML::Node& node, const ModScript::BonusStatsCommon& parser)
{
	if (node)
	{
		if (const YAML::Node& stats = node[parser.getPropertyNodeName()])
		{
			_bonusOrig.clear();
			if (stats.IsMap())
			{
				for (const auto& stat : statDataMap)
				{
					if (const YAML::Node &dd = stats[stat.name])
					{
						std::vector<float> vec;
						if (dd.IsScalar())
						{
							float val = dd.as<float>();
							vec.push_back(val);
						}
						else
						{
							for (size_t j = 0; j < statDataFuncSize; ++j)
							{
								if (j < dd.size())
								{
									float val = dd[j].as<float>();
									vec.push_back(val);
								}
							}
						}
						_bonusOrig.push_back(std::make_pair(stat.name, std::move(vec)));
					}
				}
				_refresh = true;
			}
			else if (stats.IsScalar())
			{
				_container.load(parentName, stats.as<std::string>(), parser);
				_refresh = false;
			}
			// let's remember that this was modified by a modder (i.e. is not a default value)
			_modded = true;
		}
	}

	//convert bonus vector to script
	if (_refresh)
	{
		auto script = std::string{ };
		script.reserve(1024);

		if (!_bonusOrig.empty())
		{
			//scale up for rounding
			script += "mul bonus 1000;\n";

			for (const auto& p : _bonusOrig)
			{
				script += "unit.";
				script += p.first;
				script += statNamePostfix;
				script += " bonus";
				for (size_t j = 0; j < statDataFuncSize; ++j)
				{
					if (j < p.second.size())
					{
						script += " ";
						script += std::to_string((int)(p.second[j] * statMultiper * 1000));
					}
					else
					{
						script += " 0";
					}
				}
				script += ";\n";
			}

			//rounding to the nearest
			script += "if ge bonus 0; add bonus 500; else; sub bonus 500; end;\n";
			script += "div bonus 1000;\n";
		}
		script += "return bonus;";
		_container.load(parentName, script, parser);
		_refresh = false;
	}
}

/**
 * Set new values of bonus vector
 * @param bonuses
 */
void RuleStatBonus::setValues(std::vector<RuleStatBonusDataOrig>&& bonuses)
{
	_bonusOrig = std::move(bonuses);
	_refresh = true;
}
/**
 * Set default bonus for firing accuracy.
 */
void RuleStatBonus::setFiring()
{
	setValues(
		{
			{ "firing", { 1.0f } },
		}
	);
}

/**
 * Set default bonus for melee accuracy.
 */
void RuleStatBonus::setMelee()
{
	setValues(
		{
			{ "melee", { 1.0f } },
		}
	);
}

/**
 * Set default bonus for throwing accuracy.
 */
void RuleStatBonus::setThrowing()
{
	setValues(
		{
			{ "throwing", { 1.0f } },
		}
	);
}

/**
 * Set default bonus for close quarters combat
 */
void RuleStatBonus::setCloseQuarters()
{
	setValues(
		{
			{ "melee", { 0.5f } },
			{ "reactions", { 0.5f } },
		}
	);
}

/**
 * Set default bonus for psi attack accuracy.
 */
void RuleStatBonus::setPsiAttack()
{
	setValues(
		{
			{ "psi", { 0.02f } },
		}
	);
}

/**
 * Set default bonus for psi defense.
 */
void RuleStatBonus::setPsiDefense()
{
	setValues(
		{
			{ "psiStrength", { 1.0f } },
			{ "psiSkill", { 0.2f } },
		}
	);
}

/**
 * Set flat bonus equal 100% hit chance.
 */
void RuleStatBonus::setFlatHundred()
{
	setValues(
		{
			{ "flatHundred", { 1.0f } },
		}
	);
}

/**
 * Set default bonus for melee power.
 */
void RuleStatBonus::setStrength()
{
	setValues(
		{
			{ "strength", { 1.0f } },
		}
	);
}

/**
 *  Set default for TU recovery.
 */
void RuleStatBonus::setTimeRecovery()
{
	setValues(
		{
			{ "tu", { 1.0f } },
		}
	);
}

/**
 * Set default for Energy recovery.
 */
void RuleStatBonus::setEnergyRecovery()
{
	setValues(
		{
			{ "energyRegen", { 1.0f } },
		}
	);
}

/**
 *  Set default for Stun recovery.
 */
void RuleStatBonus::setStunRecovery()
{
	setValues(
		{
			{ "flatOne", { 1.0f } },
		}
	);
}

/**
 * Calculate bonus based on attack unit and weapons.
 */
int RuleStatBonus::getBonus(BattleActionAttack::ReadOnly attack, int externalBonuses) const
{
	assert(!_refresh && "RuleStatBonus not loaded correctly");

	ModScript::BonusStatsCommon::Output arg{ externalBonuses };
	ModScript::BonusStatsCommon::Worker work{ attack.attacker, externalBonuses, attack.weapon_item, attack.damage_item, attack.type, attack.skill_rules };
	work.execute(_container, arg);

	return arg.getFirst();
}

/**
 * Calculate bonus based on unit stats.
 */
int RuleStatBonus::getBonus(const BattleUnit* unit, int externalBonuses) const
{
	assert(!_refresh && "RuleStatBonus not loaded correctly");

	ModScript::BonusStatsCommon::Output arg{ externalBonuses };
	ModScript::BonusStatsCommon::Worker work{ unit, externalBonuses, nullptr, nullptr, BA_NONE, nullptr };
	work.execute(_container, arg);

	return arg.getFirst();
}

////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

ModScript::BonusStatsBaseParser::BonusStatsBaseParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"bonus",
	"unit", "external_bonuses", "weapon", "ammo", "battle_action", "skill" }
{
	Bind<BattleUnit> bu = { this };

	for (const auto& stat : statDataMap)
	{
		stat.func(bu, stat.name);
	}
}

} //namespace OpenXcom
