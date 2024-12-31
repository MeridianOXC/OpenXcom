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
#include <vector>
#include <string>
#include "../Engine/Yaml.h"
#include "ModScript.h"

namespace OpenXcom
{

namespace helper { struct BattleActionAttackReadOnlyImpl; }

class BattleUnit;
class BattleItem;
typedef std::pair<float (*)(const BattleUnit*), float> RuleStatBonusData;
typedef std::pair<std::string, std::vector<float> > RuleStatBonusDataOrig;
/**
 * Helper class used for storing unit stat bonuses.
 */
class RuleStatBonus
{
	ModScript::BonusStatsCommon::Container _container;
	std::vector<RuleStatBonusDataOrig> _bonusOrig;
	bool _modded = false;
	bool _refresh = true;

	void setValues(std::vector<RuleStatBonusDataOrig>&& bonuses);

public:
	/// Default constructor.
	RuleStatBonus();
	/// Loads item data from YAML.
	void load(const std::string& parentName, const YAML::YamlNodeReader& reader, const ModScript::BonusStatsCommon& parser);
	/// Set default firing bonus.
	void setFiring();
	/// Set default melee bonus.
	void setMelee();
	/// Set default throwing bonus.
	void setThrowing();
	/// Set default close quarters combat bonus.
	void setCloseQuarters();
	/// Set default psi attack bonus.
	void setPsiAttack();
	/// Set default psi defense bonus.
	void setPsiDefense();
	/// Set default strength bonus.
	void setStrength();
	/// Set flat 100 bonus.
	void setFlatHundred();
	/// Set default for TU recovery.
	void setTimeRecovery();
	/// Set default for Energy recovery.
	void setEnergyRecovery();
	/// Set default for Stun recovery.
	void setStunRecovery();
	/// Get bonus based on attack unit and weapons.
	int getBonus(helper::BattleActionAttackReadOnlyImpl unit, int externalBonuses = 0) const;
	/// Get bonus based on unit stats.
	int getBonus(const BattleUnit* unit, int externalBonuses = 0) const;
	/// Used for "Stats for Nerds".
	const std::vector<RuleStatBonusDataOrig> *getBonusRaw() const { return &_bonusOrig; }
	bool isModded() const { return _modded; }
	void setModded(bool modded) { _modded = modded; }
};

}
