#pragma once
/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include <yaml-cpp/yaml.h>
#include "Unit.h"
#include "RuleStatBonus.h"
#include "ModScript.h"

namespace OpenXcom
{

class BattleUnit;

/**
 * Represents an assignable extra bonus to soldier's stats, regen and night vision.
 * Bonus is awarded either via SoldierCommendations or via SoldierTransformations.
 */
class RuleSoldierBonus
{
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleSoldierBonus";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

private:
	std::string _name;

	int _visibilityAtDark = 0;
	int _visibilityAtDay = 0;
	int _psiVision = 0;
	int _heatVision = 0;

	int _frontArmor, _sideArmor, _leftArmorDiff, _rearArmor, _underArmor;
	UnitStats _stats;
	int _listOrder;
	RuleStatBonus _timeRecovery, _energyRecovery, _moraleRecovery, _healthRecovery, _stunRecovery, _manaRecovery;
	ScriptValues<RuleSoldierBonus> _scriptValues;
	ModScript::SoldierBonusScripts::Container _soldierBonusScripts;

public:
	/// Creates a blank RuleSoldierBonus.
	RuleSoldierBonus(const std::string &name, int listOrder);
	/// Cleans up the soldier bonus ruleset.
	~RuleSoldierBonus() = default;
	/// Loads the soldier bonus definition from YAML.
	void load(const YAML::Node &node, Mod* mod, const ModScript &parsers);
	/// Gets the soldier bonus unique name/type.
	const std::string &getName() const { return _name; }

	/// Gets the bonus to night vision (in tiles).
	int getVisibilityAtDark() const { return _visibilityAtDark; }
	/// Gets the bonus to day vision (in tiles).
	int getVisibilityAtDay() const { return _visibilityAtDay; }
	/// Gets the bonus to psi vision (in tiles).
	int getPsiVision() const { return _psiVision; }
	/// Gets the bonus to heat vision (in tiles).
	int getHeatVision() const { return _heatVision; }

	/// Gets the bonus to front armor.
	int getFrontArmor() const { return _frontArmor; }
	/// Gets the bonus to left side armor.
	int getLeftSideArmor() const { return _sideArmor + _leftArmorDiff; }
	/// Gets the bonus to right side armor.
	int getRightSideArmor() const { return _sideArmor; }
	/// Gets the bonus to rear armor.
	int getRearArmor() const { return _rearArmor; }
	/// Gets the bonus to under armor.
	int getUnderArmor() const { return _underArmor; }
	/// Gets the bonus stats.
	const UnitStats *getStats() const { return &_stats; }
	/// Gets the list order for display purposes.
	int getListOrder() const { return _listOrder; }

	/// Gets the bonus TU recovery.
	int getTimeRecovery(const BattleUnit *unit) const;
	const RuleStatBonus *getTimeRecoveryRaw() const { return &_timeRecovery; }
	/// Gets the bonus Energy recovery.
	int getEnergyRecovery(const BattleUnit *unit) const;
	const RuleStatBonus *getEnergyRecoveryRaw() const { return &_energyRecovery; }
	/// Gets the bonus Morale recovery.
	int getMoraleRecovery(const BattleUnit *unit) const;
	const RuleStatBonus *getMoraleRecoveryRaw() const { return &_moraleRecovery; }
	/// Gets the bonus Health recovery.
	int getHealthRecovery(const BattleUnit *unit) const;
	const RuleStatBonus *getHealthRecoveryRaw() const { return &_healthRecovery; }
	/// Gets the bonus Mana recovery.
	int getManaRecovery(const BattleUnit *unit) const;
	const RuleStatBonus *getManaRecoveryRaw() const { return &_manaRecovery; }
	/// Gets the bonus Stun recovery.
	int getStunRegeneration(const BattleUnit *unit) const;
	const RuleStatBonus *getStunRegenerationRaw() const { return &_stunRecovery; }
	/// Gets script.
	template<typename Script>
	const typename Script::Container &getScript() const { return _soldierBonusScripts.get<Script>(); }
	/// Get all script values.
	const ScriptValues<RuleSoldierBonus> &getScriptValuesRaw() const { return _scriptValues; }
};

}
