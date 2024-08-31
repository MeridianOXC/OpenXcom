#pragma once
/*
 * Copyright 2010-2018 OpenXcom Developers.
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
#include <map>
#include <yaml-cpp/yaml.h>
#include "Unit.h"
#include "RuleBaseFacilityFunctions.h"

namespace OpenXcom
{

class Mod;

/**
 * Ruleset data structure for the information to transform a soldier.
 */
class RuleSoldierTransformation
{
private:
	std::string _name;
	std::vector<std::string > _requires, _requiredPreviousTransformations, _forbiddenPreviousTransformations;
	RuleBaseFacilityFunctions _requiresBaseFunc;
	std::string _producedItem;
	std::string _producedSoldierType, _producedSoldierArmor;
	bool _keepSoldierArmor, _createsClone, _needsCorpseRecovered, _allowsDeadSoldiers, _allowsLiveSoldiers, _allowsWoundedSoldiers;
	std::vector<std::string > _allowedSoldierTypes;
	std::map<std::string, int> _requiredItems;
	std::map<std::string, int> _requiredCommendations;
	int _listOrder, _cost, _transferTime, _recoveryTime;
	int _minRank;
	bool _includeBonusesForMinStats, _includeBonusesForMaxStats;
	UnitStats _requiredMinStats, _requiredMaxStats, _flatOverallStatChange, _percentOverallStatChange, _percentGainedStatChange;
	UnitStats _flatMin, _flatMax, _percentMin, _percentMax, _percentGainedMin, _percentGainedMax;
	bool _showMinMax;
	UnitStats _rerollStats;
	bool _lowerBoundAtMinStats, _upperBoundAtMaxStats, _upperBoundAtStatCaps;
	int _upperBoundType;
	bool _reset;
	bool _resetRank;
	std::string _soldierBonusType;

public:
	/// Default constructor
	RuleSoldierTransformation(const std::string &name, int listOrder);
	/// Loads the project data from YAML
	void load(const YAML::Node& node, Mod* mod);
	/// Gets the unique name id of the project
	const std::string &getName() const;
	/// Gets the list weight of the project
	int getListOrder() const;
	/// Gets the list of research this project requires
	const std::vector<std::string > &getRequiredResearch() const;
	/// Gets the list of required base functions for this project
	RuleBaseFacilityFunctions getRequiredBaseFuncs() const { return _requiresBaseFunc; }
	/// Gets the type of item produced by this project (the soldier stops existing completely and is fully replaced by the item)
	const std::string &getProducedItem() const { return _producedItem; }
	/// Gets the type of soldier produced by this project
	const std::string &getProducedSoldierType() const;
	/// Gets the armor that the produced soldier should be wearing
	const std::string &getProducedSoldierArmor() const;
	/// Gets whether or not the soldier should keep their armor after the project
	bool isKeepingSoldierArmor() const;
	/// Gets whether or not the project should produce a clone (new id) of the input soldier
	bool isCreatingClone() const;
	/// Gets whether or not the project needs the body of the soldier to have been recovered
	bool needsCorpseRecovered() const;
	/// Gets whether or not the project allows input of dead soldiers
	bool isAllowingDeadSoldiers() const;
	/// Gets whether or not the project allows input of alive soldiers
	bool isAllowingAliveSoldiers() const;
	/// Gets whether or not the project allows input of wounded soldiers
	bool isAllowingWoundedSoldiers() const;
	/// Gets the list of soldier types eligible for this project
	const std::vector<std::string > &getAllowedSoldierTypes() const;
	/// Gets the list of previous soldier transformations a soldier needs for this project
	const std::vector<std::string > &getRequiredPreviousTransformations() const;
	/// Gets the list of previous soldier transformations that make a soldier ineligible for this project
	const std::vector<std::string > &getForbiddenPreviousTransformations() const;
	/// Gets whether or not to include soldier bonuses when checking required min stats.
	bool getIncludeBonusesForMinStats() const { return _includeBonusesForMinStats; }
	/// Gets whether or not to include soldier bonuses when checking required max stats.
	bool getIncludeBonusesForMaxStats() const { return _includeBonusesForMaxStats; }
	/// Gets the minimum stats a soldier needs to be eligible for this project
	const UnitStats &getRequiredMinStats() const;
	/// Gets the maximum stats a soldier can have to be eligible for this project
	const UnitStats &getRequiredMaxStats() const;
	/// Gets the list of items necessary to complete this project
	const std::map<std::string, int> &getRequiredItems() const;
	/// Gets the list of commendations necessary to complete this project
	const std::map<std::string, int> &getRequiredCommendations() const;
	/// Gets the cash cost of the project
	int getCost() const;
	/// Gets how long the transformed soldier should be in transit to the base after completion
	int getTransferTime() const;
	/// Gets how long the transformed soldier should take to recover after completion
	int getRecoveryTime() const;
	/// Gets the minimum rank a soldier needs to be eligible for this project
	int getMinRank() const;
	/// Gets the flat change to a soldier's overall stats when undergoing this project
	const UnitStats &getFlatOverallStatChange() const;
	/// Gets the percent change to a soldier's overall stats when undergoing this project
	const UnitStats &getPercentOverallStatChange() const;
	/// Gets the percent change to a soldier's gained stats when undergoing this project
	const UnitStats &getPercentGainedStatChange() const;

	/// Gets the min flat change to a soldier's overall stats when undergoing this project
	const UnitStats &getFlatMin() const { return _flatMin; }
	/// Gets the max flat change to a soldier's overall stats when undergoing this project
	const UnitStats &getFlatMax() const { return _flatMax; }
	/// Gets the min percent change to a soldier's overall stats when undergoing this project
	const UnitStats &getPercentMin() const { return _percentMin; }
	/// Gets the max percent change to a soldier's overall stats when undergoing this project
	const UnitStats &getPercentMax() const { return _percentMax; }
	/// Gets the min percent change to a soldier's gained stats when undergoing this project
	const UnitStats &getPercentGainedMin() const { return _percentGainedMin; }
	/// Gets the max percent change to a soldier's gained stats when undergoing this project
	const UnitStats &getPercentGainedMax() const { return _percentGainedMax; }
	/// Gets whether to display min/max changes in two lines; or just one line with randomized changes replaced with a question mark
	bool getShowMinMax() const { return _showMinMax; }

	/// Gets information about which soldier stats should be re-rolled when undergoing this project
	const UnitStats &getRerollStats() const { return _rerollStats; }

	/// Gets whether or not this project should bound stat penalties at the produced RuleSoldier's minStats
	bool hasLowerBoundAtMinStats() const;
	/// Gets whether or not this project should cap stats at the produced RuleSoldier's maxStats
	bool hasUpperBoundAtMaxStats() const;
	/// Gets whether or not this project should cap stats at the produced RuleSoldier's statCaps
	bool hasUpperBoundAtStatCaps() const;
	/// Gets whether to use soft upper bound limit or not.
	bool isSoftLimit(bool isSameSoldierType) const;

	/// Gets whether or not this project should reset info about all previous transformations and all previously assigned soldier bonuses
	bool getReset() const;
	/// Gets whether or not this project should reset the rank of the destination soldier to rookie
	bool getResetRank() const;
	/// Gets the type of soldier bonus assigned by this project
	const std::string &getSoldierBonusType() const;
};

}
