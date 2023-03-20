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
#include <string_view>
#include "StatString.h"
#include "Unit.h"
#include "../Engine/Unicode.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{

typedef UnitStats::Type UnitStat;

constexpr static auto absoluteConditionData = std::array<std::pair<std::string_view, UnitStat UnitStats::*>, 12>{{
	{"psiStrength", &UnitStats::psiStrength},
	{"psiSkill",    &UnitStats::psiSkill},
	{"bravery",     &UnitStats::bravery},
	{"strength",    &UnitStats::strength},
	{"firing",      &UnitStats::firing},
	{"reactions",   &UnitStats::reactions},
	{"stamina",     &UnitStats::stamina},
	{"tu",          &UnitStats::tu},
	{"health",      &UnitStats::health},
	{"throwing",    &UnitStats::throwing},
	{"melee",       &UnitStats::melee},
	{"manaPool",    &UnitStats::mana},
}};

constexpr static auto percentConditionData = std::array<std::pair<std::string_view, UnitStats::Type UnitStats::*>, 12>{{
	{"percentPsiStrength", &UnitStats::psiStrength},
	{"percentPsiSkill",    &UnitStats::psiSkill},
	{"percentBravery",     &UnitStats::bravery},
	{"percentStrength",    &UnitStats::strength},
	{"percentFiring",      &UnitStats::firing},
	{"percentReactions",   &UnitStats::reactions},
	{"percentStamina",     &UnitStats::stamina},
	{"percentTu",          &UnitStats::tu},
	{"percentHealth",      &UnitStats::health},
	{"percentThrowing",    &UnitStats::throwing},
	{"percentMelee",       &UnitStats::melee},
	{"percentManaPool",    &UnitStats::mana},
}};

constexpr static auto trainingConditionData = std::array<std::pair<std::string_view, TrainingType>, 2>{{
	{"psiTraining", TrainingType::PSI_TRAINING},
	{"physTraining", TrainingType::PHYS_TRAINING},
}};

/**
 * Loads the StatString from a YAML file.
 * @param node YAML node.
 */
void StatString::load(const YAML::Node &node)
{
	_conditionString = node["string"].as<std::string>(std::string());
	_globalRule = node["globalRule"].as<bool>(STATSTRING_GLOBALRULE_DEFAULT);

	for (const auto &datum : absoluteConditionData)
	{
		if (auto subnode = node[std::string(datum.first)])
		{
			int minValue = STATSTRING_CONDITION_ABSOLUTE_DEFAULT_MIN, maxValue = STATSTRING_CONDITION_ABSOLUTE_DEFAULT_MAX;
			if (subnode[0])
			{
				minValue = subnode[0].as<int>(minValue);
			}
			if (subnode[1])
			{
				maxValue = subnode[1].as<int>(maxValue);
			}
			auto condition = std::make_unique<StatStringConditionAbsolulte>(datum.second, minValue, maxValue);
			_conditions.emplace_back(std::move(condition));
		}
	};

	for (const auto &datum : percentConditionData)
	{
		if (auto subnode = node[std::string(datum.first)])
		{
			float minValue = STATSTRING_CONDITION_PERCENT_DEFAULT_MIN, maxValue = STATSTRING_CONDITION_PERCENT_DEFAULT_MAX;
			if (subnode[0])
			{
				minValue = subnode[0].as<float>(minValue);
			}
			if (subnode[1])
			{
				maxValue = subnode[1].as<float>(maxValue);
			}
			auto condition = std::make_unique<StatStringConditionPercent>(datum.second, minValue, maxValue);
			_conditions.emplace_back(std::move(condition));
		}
	};

	for (const auto &datum : trainingConditionData)
	{
		if (auto subnode = node[std::string(datum.first)])
		{
			bool inTraining = subnode.as<bool>(STAT_CONDITION_TRAINING_INTRAINING_DEFAULT);
			auto condition = std::make_unique<StatStringConditionTraining>(datum.second, inTraining);
			_conditions.emplace_back(std::move(condition));
		}
	}
}

/**
 * @brief Calculates a combined StatString based a vector of rules.
 * @param statStringsToCalc The rules to evaluate
 * @param showPsiStats Should Psi Rules be evaluted?
 * @return The calculated string that should apply to the name.
*/
std::string StatString::calcStatString(Soldier *soldier, const std::vector<StatString *> &statStringsToCalc, bool showPsiStats)
{
	std::string result;

	for (auto &statString : statStringsToCalc)
	{
		const auto &conditions = statString->getConditions();

		// Condition test. Never apply if it's a psi condition and we aren't showing them. Otherwise, test the condition.
		const auto conditionTest = [&](auto &condition) -> bool
		{
			return (condition->isPsiCondition() && !showPsiStats) ? false : condition->isMet(*soldier);
		};

		bool conditionsMet = !conditions.empty() && std::all_of(conditions.begin(), conditions.end(), conditionTest);

		if (conditionsMet)
		{
			// if the applied string is longer than 1, we stop here.
			std::string wstring = statString->getString();
			result += wstring;
			if (Unicode::codePointLengthUTF8(wstring) > 1)
			{
				break;
			}
		}
	}
	return result;
}
}
