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

#include "RuleCommendations.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank set of commendation data.
 */
RuleCommendations::RuleCommendations(const std::string& type) : _type(type), _criteria(), _killCriteria(), _description(""), _sprite(), _soldierBonusTypes()
{
}

/**
 * Cleans up the commendation.
 */
RuleCommendations::~RuleCommendations()
{
}

/**
 * Loads the commendations from YAML.
 * @param node YAML node.
 */
void RuleCommendations::load(const YAML::Node &node, const Mod* mod)
{
	_description = node["description"].as<std::string>(_description);
	mod->loadUnorderedNamesToInts(_type, _criteria, node["criteria"]);
	_sprite = node["sprite"].as<int>(_sprite);
	mod->loadKillCriteria(_type, _killCriteria, node["killCriteria"]);
	mod->loadNames(_type, _soldierBonusTypesNames, node["soldierBonusTypes"]);
	mod->loadNames(_type, _missionMarkerNames, node["missionMarkerFilter"]);
	mod->loadNames(_type, _missionTypeNames, node["missionTypeFilter"]);
}

/**
 * Cross link with other rules.
 */
void RuleCommendations::afterLoad(const Mod* mod)
{
	mod->linkRule(_soldierBonusTypes, _soldierBonusTypesNames);
}

/**
 * Get the commendation's description.
 * @return string Commendation description.
 */
const std::string& RuleCommendations::getDescription() const
{
	return _description;
}

/**
 * Get the commendation's award criteria.
 * @return map<string, int> Commendation criteria.
 */
const std::map<std::string, std::vector<int> > *RuleCommendations::getCriteria() const
{
	return &_criteria;
}

/**
 * Get the commendation's award kill criteria.
 * @return vector<string> Commendation kill criteria.
 */
const std::vector<std::vector<std::pair<int, std::vector<std::string> > > > *RuleCommendations::getKillCriteria() const
{
	return &_killCriteria;
}

/**
 * Get the commendation's sprite.
 * @return int Sprite number.
 */
int RuleCommendations::getSprite() const
{
	return _sprite;
}

/**
 * Gets the soldier bonus type corresponding to the commendation's decoration level.
 * @return Soldier bonus type.
 */
const RuleSoldierBonus *RuleCommendations::getSoldierBonus(int decorationLevel) const
{
	if (!_soldierBonusTypes.empty())
	{
		int lastIndex = (int)(_soldierBonusTypes.size()) - 1;
		int index = decorationLevel > lastIndex ? lastIndex : decorationLevel;
		return _soldierBonusTypes.at(index);
	}
	return nullptr;
}

/**
 * Gets the commendation's mission marker filter.
 * @return vector<string> Mission marker types.
 */
const std::vector<std::string>& RuleCommendations::getMissionMarkerNames() const
{
	return _missionMarkerNames;
}

/**
 * Gets the commendation's mission type filter.
 * @return vector<string> Mission types.
 */
const std::vector<std::string>& RuleCommendations::getMissionTypeNames() const
{
	return _missionTypeNames;
}

}
