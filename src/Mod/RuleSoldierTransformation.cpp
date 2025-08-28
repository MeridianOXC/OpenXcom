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
#include "RuleSoldierTransformation.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Constructor for a soldier transformation project (necromancy, cloning, ascending!)
 * @param name The unique project name id
 */
RuleSoldierTransformation::RuleSoldierTransformation(const std::string &name, int listOrder) :
	_name(name),
	_keepSoldierArmor(false), _createsClone(false), _needsCorpseRecovered(true),
	_allowsDeadSoldiers(false), _allowsLiveSoldiers(false), _allowsWoundedSoldiers(false),
	_listOrder(listOrder), _cost(0), _transferTime(0), _recoveryTime(0), _minRank(0), _includeBonusesForMinStats(false), _includeBonusesForMaxStats(false),
	_showMinMax(false), _lowerBoundAtMinStats(true), _upperBoundAtMaxStats(false), _upperBoundAtStatCaps(false), _upperBoundType(0),
	_reset(false), _resetRank(false)
{
	_requiredMaxStats = UnitStats::scalar(9999); // set all default max stats to 9999
}

/**
 * Loads the transformation project from a YAML file.
 * @param node YAML node.
 * @param listOrder The list weight for this transformation project.
 */
void RuleSoldierTransformation::load(const YAML::YamlNodeReader& node, Mod* mod)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	reader.tryRead("listOrder", _listOrder);

	mod->loadUnorderedNames(_name, _requires, reader["requires"]);
	mod->loadBaseFunction(_name, _requiresBaseFunc, reader["requiresBaseFunc"]);
	reader.tryRead("producedItem", _producedItem);
	reader.tryRead("producedSoldierType", _producedSoldierType);
	reader.tryRead("producedSoldierArmor", _producedSoldierArmor);
	reader.tryRead("keepSoldierArmor", _keepSoldierArmor);
	reader.tryRead("createsClone", _createsClone);
	reader.tryRead("needsCorpseRecovered", _needsCorpseRecovered);
	reader.tryRead("allowsDeadSoldiers", _allowsDeadSoldiers);
	reader.tryRead("allowsLiveSoldiers", _allowsLiveSoldiers);
	reader.tryRead("allowsWoundedSoldiers", _allowsWoundedSoldiers);
	mod->loadUnorderedNames(_name, _allowedSoldierTypes, reader["allowedSoldierTypes"]);
	mod->loadUnorderedNames(_name, _requiredPreviousTransformations, reader["requiredPreviousTransformations"]);
	mod->loadUnorderedNames(_name, _forbiddenPreviousTransformations, reader["forbiddenPreviousTransformations"]);
	reader.tryRead("includeBonusesForMinStats", _includeBonusesForMinStats);
	reader.tryRead("includeBonusesForMaxStats", _includeBonusesForMaxStats);
	reader.tryRead("requiredMinStats", _requiredMinStats);
	if (reader["requiredMaxStats"])
	{
		UnitStats tmp = reader["requiredMaxStats"].readVal(_requiredMaxStats);
		_requiredMaxStats.merge(tmp);
	}
	mod->loadUnorderedNamesToInt(_name, _requiredItems, reader["requiredItems"]);
	mod->loadUnorderedNamesToInt(_name, _requiredCommendations, reader["requiredCommendations"]);
	reader.tryRead("cost", _cost);
	reader.tryRead("transferTime", _transferTime);
	reader.tryRead("recoveryTime", _recoveryTime);
	reader.tryRead("minRank", _minRank);
	reader.tryRead("flatOverallStatChange", _flatOverallStatChange);
	reader.tryRead("percentOverallStatChange", _percentOverallStatChange);
	reader.tryRead("percentGainedStatChange", _percentGainedStatChange);
	reader.tryRead("flatMin", _flatMin);
	reader.tryRead("flatMax", _flatMax);
	reader.tryRead("percentMin", _percentMin);
	reader.tryRead("percentMax", _percentMax);
	reader.tryRead("percentGainedMin", _percentGainedMin);
	reader.tryRead("percentGainedMax", _percentGainedMax);
	reader.tryRead("showMinMax", _showMinMax);
	reader.tryRead("rerollStats", _rerollStats);
	reader.tryRead("lowerBoundAtMinStats", _lowerBoundAtMinStats);
	reader.tryRead("upperBoundAtMaxStats", _upperBoundAtMaxStats);
	reader.tryRead("upperBoundAtStatCaps", _upperBoundAtStatCaps);
	reader.tryRead("upperBoundType", _upperBoundType);

	mod->loadUnorderedNames(_name, _removeTransformations, reader["removeTransformations"]);
	reader.tryRead("reset", _reset);
	reader.tryRead("resetRank", _resetRank);
	reader.tryRead("soldierBonusType", _soldierBonusType);
}

/**
 * Gets the unique name id of the project
 * @return The name of the project
 */
const std::string &RuleSoldierTransformation::getName() const
{
	return _name;
}

/**
 * Gets the list weight of the project
 * @return The list order
 */
int RuleSoldierTransformation::getListOrder() const
{
	return _listOrder;
}

/**
 * Gets the list of research this project requires
 * @return The list of required research
 */
const std::vector<std::string > &RuleSoldierTransformation::getRequiredResearch() const
{
	return _requires;
}

/**
 * Gets the type of soldier produced by this project
 * @return The soldier type
 */
const std::string &RuleSoldierTransformation::getProducedSoldierType() const
{
	return _producedSoldierType;
}

/**
 * Gets the armor that the produced soldier should be wearing
 * @return The armor type
 */
const std::string &RuleSoldierTransformation::getProducedSoldierArmor() const
{
	return _producedSoldierArmor;
}

/**
 * Gets whether or not the project should have the soldier keep their current armor
 * @return Keep the current armor?
 */
bool RuleSoldierTransformation::isKeepingSoldierArmor() const
{
	return _keepSoldierArmor;
}

/**
 * Gets whether or not the project should produce a clone (new id) of the input soldier
 * @return Create a clone?
 */
bool RuleSoldierTransformation::isCreatingClone() const
{
	return _createsClone;
}

/**
 * Gets whether or not the project needs the body of the soldier to have been recovered
 * @return Needs the body?
 */
bool RuleSoldierTransformation::needsCorpseRecovered() const
{
	return _needsCorpseRecovered;
}

/**
 * Gets whether or not the project allows input of dead soldiers
 * @return Allow dead soldiers?
 */
bool RuleSoldierTransformation::isAllowingDeadSoldiers() const
{
	return _allowsDeadSoldiers;
}

/**
 * Gets whether or not the project allows input of alive soldiers
 * @return Allow live soldiers?
 */
bool RuleSoldierTransformation::isAllowingAliveSoldiers() const
{
	return _allowsLiveSoldiers;
}

/**
 * Gets whether or not the project allows input of wounded soldiers
 * @return Allow wounded soldiers?
 */
bool RuleSoldierTransformation::isAllowingWoundedSoldiers() const
{
	return _allowsWoundedSoldiers;
}

/**
 * Gets the list of soldier types eligible for this project
 * @return The list of allowed soldier types
 */
const std::vector<std::string > &RuleSoldierTransformation::getAllowedSoldierTypes() const
{
	return _allowedSoldierTypes;
}

/**
 * Gets the list of previous soldier transformations a soldier needs for this project
 * @return The list of required previous transformations.
 */
const std::vector<std::string > &RuleSoldierTransformation::getRequiredPreviousTransformations() const
{
	return _requiredPreviousTransformations;
}

/**
 * Gets the list of previous soldier transformations that make a soldier ineligible for this project
 * @return The list of forbidden previous
 */
const std::vector<std::string > &RuleSoldierTransformation::getForbiddenPreviousTransformations() const
{
	return _forbiddenPreviousTransformations;
}

/**
 * Gets the minimum stats a soldier needs to be eligible for this project
 * @return The stat requirements
 */
const UnitStats &RuleSoldierTransformation::getRequiredMinStats() const
{
	return _requiredMinStats;
}

/**
 * Gets the maximum stats a soldier can have to be eligible for this project
 * @return The stat requirements
 */
const UnitStats &RuleSoldierTransformation::getRequiredMaxStats() const
{
	return _requiredMaxStats;
}

/**
 * Gets the list of items necessary to complete this project
 * @return The list of required items
 */
const std::map<std::string, int> &RuleSoldierTransformation::getRequiredItems() const
{
	return _requiredItems;
}

/**
 * Gets the list of commendations necessary to complete this project
 * @return The list of required commendations
 */
const std::map<std::string, int> &RuleSoldierTransformation::getRequiredCommendations() const
{
	return _requiredCommendations;
}

/**
 * Gets the cash cost of the project
 * @return The cost
 */
int RuleSoldierTransformation::getCost() const
{
	return _cost;
}

/**
 * Gets how long the transformed soldier should be in transit to the base after completion
 * @return The transfer time in hours
 */
int RuleSoldierTransformation::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets how long the transformed soldier should take to recover after completion
 * @return The recovery time in days
 */
int RuleSoldierTransformation::getRecoveryTime() const
{
	return _recoveryTime;
}

/**
 * Gets the minimum rank a soldier needs to be eligible for this project
 * @return The rank requirement
 */
int RuleSoldierTransformation::getMinRank() const
{
	return _minRank;
}

/**
 * Gets the flat change to a soldier's overall stats when undergoing this project
 * @return The flat overall stat changes
 */
const UnitStats &RuleSoldierTransformation::getFlatOverallStatChange() const
{
	return _flatOverallStatChange;
}

/**
 * Gets the percent change to a soldier's overall stats when undergoing this project
 * @return The percent overall stat changes
 */
const UnitStats &RuleSoldierTransformation::getPercentOverallStatChange() const
{
	return _percentOverallStatChange;
}

/**
 * Gets the percent change to a soldier's gained stats when undergoing this project
 * @return The percent gained stat changes
 */
const UnitStats &RuleSoldierTransformation::getPercentGainedStatChange() const
{
	return _percentGainedStatChange;
}

/**
 * Gets whether or not this project should bound stat penalties at the produced RuleSoldier's minStats
 * @return Lower bound at minStats?
 */
bool RuleSoldierTransformation::hasLowerBoundAtMinStats() const
{
	return _lowerBoundAtMinStats;
}

/**
 * Gets whether or not this project should cap stats at the produced RuleSoldier's maxStats
 * @return Upper bound at maxStats?
 */
bool RuleSoldierTransformation::hasUpperBoundAtMaxStats() const
{
	return _upperBoundAtMaxStats;
}

/**
 * Gets whether or not this project should cap stats at the produced RuleSoldier's statCaps
 * @return Upper bound at statCaps?
 */
bool RuleSoldierTransformation::hasUpperBoundAtStatCaps() const
{
	return _upperBoundAtStatCaps;
}

/**
 * Gets whether to use soft upper bound limit or not.
 * @return Soft upper bound or hard upper bound?
 */
bool RuleSoldierTransformation::isSoftLimit(bool isSameSoldierType) const
{
	if (_upperBoundType == 0)
	{
		return isSameSoldierType; // 0 = dynamic
	}
	else if (_upperBoundType == 1)
	{
		return true; // 1 = soft limit
	}
	return false; // 2+ = hard limit
}

/**
 * Gets whether or not this project should reset info about all previous transformations and all previously assigned soldier bonuses
 * @return Reset previous transformation info and soldier bonuses?
 */
bool RuleSoldierTransformation::getReset() const
{
	return _reset;
}

/**
 * Gets whether or not this project should reset the rank of the destination soldier to rookie
 * @return Reset the rank to rookie?
 */
bool RuleSoldierTransformation::getResetRank() const
{
	return _resetRank;
}

/**
 * Gets the type of soldier bonus assigned by this project
 * @return The soldier bonus type
 */
const std::string &RuleSoldierTransformation::getSoldierBonusType() const
{
	return _soldierBonusType;
}

}
