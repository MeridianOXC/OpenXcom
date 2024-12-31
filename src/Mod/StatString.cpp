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
#include "StatString.h"
#include "Unit.h"
#include <vector>
#include "../Engine/Language.h"
#include "../Engine/Unicode.h"

namespace OpenXcom
{

/**
 * Creates a blank StatString.
 */
StatString::StatString()
{
}

/**
 * Cleans up the extra StatString.
 */
StatString::~StatString()
{
}

/**
 * Loads the StatString from a YAML file.
 * @param node YAML node.
 */
void StatString::load(const YAML::YamlNodeReader& reader)
{
	std::string conditionNames[] = {"psiStrength", "psiSkill", "bravery", "strength", "firing", "reactions", "stamina", "tu", "health", "throwing", "melee", "psiTraining", "manaPool"};
	reader.tryRead("string", _stringToBeAddedIfAllConditionsAreMet);
	for (size_t i = 0; i < std::size(conditionNames); i++)
	{
		if (const auto& conditionReader = reader[ryml::to_csubstr(conditionNames[i])])
		{
			_conditions.push_back(getCondition(conditionNames[i], conditionReader));
		}
	}
}

/**
 * Generates a condition from YAML.
 * @param conditionName Stat name of the condition.
 * @param node YAML node.
 * @return New StatStringCondition.
 */
StatStringCondition *StatString::getCondition(const std::string &conditionName, const YAML::YamlNodeReader& conditionReader)
{
	// These are the defaults from xcomutil
	int minValue = 0, maxValue = 255;
	if (conditionReader[0] && !conditionReader[0].hasNullVal())
		conditionReader[0].tryReadVal(minValue);
	if (conditionReader[1] && !conditionReader[1].hasNullVal())
		conditionReader[1].tryReadVal(maxValue);
	StatStringCondition *thisCondition = new StatStringCondition(conditionName, minValue, maxValue);
	return thisCondition;
}

/**
 * Returns the conditions associated with this StatString.
 * @return List of StatStringConditions.
 */
const std::vector<StatStringCondition*> &StatString::getConditions() const
{
	return _conditions;
}

/**
 * Returns the string to add to a name for this StatString.
 * @return StatString... string.
 */
std::string StatString::getString() const
{
	return _stringToBeAddedIfAllConditionsAreMet;
}

/**
 * Calculates the list of StatStrings that apply to certain unit stats.
 * @param currentStats Unit stats.
 * @param statStrings List of statString rules.
 * @param psiStrengthEval Are psi stats available?
 * @return Resulting string of all valid StatStrings.
 */
std::string StatString::calcStatString(UnitStats &currentStats, const std::vector<StatString *> &statStrings, bool psiStrengthEval, bool inTraining)
{
	std::string result;
	std::map<std::string, int> currentStatsMap = getCurrentStats(currentStats);
	if (inTraining)
	{
		currentStatsMap["psiTraining"] = 1;
	}
	for (auto* statStringDef : statStrings)
	{
		bool conditionsMet = true;
		for (auto* statStringCondition : statStringDef->getConditions())
		{
			if (!conditionsMet) break; // loop finished
			auto name = currentStatsMap.find(statStringCondition->getConditionName());
			if (name != currentStatsMap.end())
			{
				conditionsMet = conditionsMet && statStringCondition->isMet(name->second, currentStats.psiSkill > 0 || psiStrengthEval);
			}
			else
			{
				// if name == currentStatsMap.end() we've searched for a stat that doesn't exist.
				// this means psi training. if there's no "psiTraining" stat in the statsMap,
				// this soldier isn't in training, so we won't append his name with the psiTraining tag.
				// presumably conditionsMet was originally initialized as false, but for whatever reason that was changed, hence this.
				conditionsMet = false;
			}
		}
		if (conditionsMet)
		{
			std::string wstring = statStringDef->getString();
			result += wstring;
			if (Unicode::codePointLengthUTF8(wstring) > 1)
			{
				break;
			}
		}
	}
	return result;
}

/**
 * Get a map associating stat names to unit stats.
 * @param currentStats Unit stats to use.
 * @return Map of unit stats.
 */
std::map<std::string, int> StatString::getCurrentStats(UnitStats &currentStats)
{
	std::map<std::string, int> currentStatsMap;
	currentStatsMap["psiStrength"] = currentStats.psiStrength;
	currentStatsMap["psiSkill"] = currentStats.psiSkill;
	currentStatsMap["bravery"] = currentStats.bravery;
	currentStatsMap["strength"] = currentStats.strength;
	currentStatsMap["firing"] = currentStats.firing;
	currentStatsMap["reactions"] = currentStats.reactions;
	currentStatsMap["stamina"] = currentStats.stamina;
	currentStatsMap["tu"] = currentStats.tu;
	currentStatsMap["health"] = currentStats.health;
	currentStatsMap["throwing"] = currentStats.throwing;
	currentStatsMap["melee"] = currentStats.melee;
	currentStatsMap["manaPool"] = currentStats.mana;
	return currentStatsMap;
}


}
