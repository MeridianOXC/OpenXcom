#pragma once
/*
 * Copyright 2010-2020 OpenXcom Developers.
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
#include <vector>
#include "RuleItem.h"
#include "RuleSoldierBonus.h"
#include "ModScript.h"

namespace OpenXcom
{

enum BattleActionType : Uint8;

class RuleSkill
{
private:
	std::string _type;
	BattleActionType _targetMode;
	BattleType _compatibleBattleType;
	bool _isPsiRequired;
	bool _checkHandsOnly;
	bool _checkHandsOnly2;
	RuleItemUseCost _cost;
	RuleItemUseCost _flat;
	std::vector<std::string> _compatibleWeaponNames;
	std::vector<std::string> _requiredBonusNames;
	std::vector<const RuleItem*> _compatibleWeapons;
	std::vector<const RuleSoldierBonus*> _requiredBonuses;

	ScriptValues<RuleSkill> _scriptValues;
	ModScript::SkillScripts::Container _skillScripts;

public:
	/// Creates a blank soldier skill ruleset.
	RuleSkill(const std::string& type);
	/// Cleans up the soldier skill ruleset.
	~RuleSkill() = default;
	/// Loads the soldier skill data from YAML.
	void load(const YAML::Node& node, Mod *mod, const ModScript& parsers);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);

	/// Gets the skill's type.
	const std::string& getType() const { return _type; }
	/// Gets the targeting mode this skill will be using.
	BattleActionType getTargetMode() const { return _targetMode; }
	/// Gets the BattleType for potentially compatible items.
	BattleType getCompatibleBattleType() const { return _compatibleBattleType; }
	/// Is Psi Skill required for this skill to work?
	bool isPsiRequired() const { return _isPsiRequired; }
	/// Should the check for compatible items only consider the hands (or also the inventory and specialweapon)?
	bool checkHandsOnly() const { return _checkHandsOnly; }
	/// Should the check for compatible items (by battle type) only consider the hands (or also the inventory and specialweapon)?
	bool checkHandsOnly2() const { return _checkHandsOnly2; }
	/// Gets the use cost for this skill.
	const RuleItemUseCost& getCost() const { return _cost; }
	/// Gets the flat vs. percentage cost flags for the use cost of this skill.
	const RuleItemUseCost& getFlat() const { return _flat; }
	/// Gets the list of weapons which are compatible with this skill.
	const std::vector<const RuleItem*>& getCompatibleWeapons() const { return _compatibleWeapons; }
	/// Gets the list of required soldier bonuses for this skill.
	const std::vector<const RuleSoldierBonus*>& getRequiredBonuses() const { return _requiredBonuses; }

	/// Name of class used in script.
	static constexpr const char* ScriptName = "RuleSkill";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Gets script.
	template<typename Script>
	const typename Script::Container& getScript() const { return _skillScripts.get<Script>(); }
	/// Get all script values.
	const ScriptValues<RuleSkill>& getScriptValuesRaw() const { return _scriptValues; }
};

}
