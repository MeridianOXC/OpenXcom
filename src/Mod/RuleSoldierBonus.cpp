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
#include "RuleSoldierBonus.h"
#include "Mod.h"
#include "../Engine/ScriptBind.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

RuleSoldierBonus::RuleSoldierBonus(const std::string &name, int listOrder) : _name(name), _frontArmor(0), _sideArmor(0), _leftArmorDiff(0), _rearArmor(0), _underArmor(0), _listOrder(listOrder)
{
}

/**
 * Loads the soldier bonus definition from YAML.
 * @param node YAML node.
 */
void RuleSoldierBonus::load(const YAML::Node &node, Mod* mod, const ModScript &parsers)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod, parsers);
	}

	_visibilityAtDark = node["visibilityAtDark"].as<int>(_visibilityAtDark);
	_visibilityAtDay = node["visibilityAtDay"].as<int>(_visibilityAtDay);
	_psiVision = node["psiVision"].as<int>(_psiVision);
	_heatVision = node["heatVision"].as<int>(_heatVision);

	_frontArmor = node["frontArmor"].as<int>(_frontArmor);
	_sideArmor = node["sideArmor"].as<int>(_sideArmor);
	_leftArmorDiff = node["leftArmorDiff"].as<int>(_leftArmorDiff);
	_rearArmor = node["rearArmor"].as<int>(_rearArmor);
	_underArmor = node["underArmor"].as<int>(_underArmor);
	_stats.merge(node["stats"].as<UnitStats>(_stats));
	const YAML::Node &rec = node["recovery"];
	{
		_timeRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::TimeSoldierRecoveryStatBonus>());
		_energyRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::EnergySoldierRecoveryStatBonus>());
		_moraleRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::MoraleSoldierRecoveryStatBonus>());
		_healthRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::HealthSoldierRecoveryStatBonus>());
		_manaRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::ManaSoldierRecoveryStatBonus>());
		_stunRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::StunSoldierRecoveryStatBonus>());
	}

	_soldierBonusScripts.load(_name, node, parsers.soldierBonusScripts);
	_scriptValues.load(node, parsers.getShared());

	_listOrder = node["listOrder"].as<int>(_listOrder);
}

/**
 *  Gets the bonus TU recovery.
 */
int RuleSoldierBonus::getTimeRecovery(const BattleUnit *unit) const
{
	return _timeRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Energy recovery.
 */
int RuleSoldierBonus::getEnergyRecovery(const BattleUnit *unit) const
{
	return _energyRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Morale recovery.
 */
int RuleSoldierBonus::getMoraleRecovery(const BattleUnit *unit) const
{
	return _moraleRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Health recovery.
 */
int RuleSoldierBonus::getHealthRecovery(const BattleUnit *unit) const
{
	return _healthRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Mana recovery.
 */
int RuleSoldierBonus::getManaRecovery(const BattleUnit *unit) const
{
	return _manaRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Stun recovery.
 */
int RuleSoldierBonus::getStunRegeneration(const BattleUnit *unit) const
{
	return _stunRecovery.getBonus(unit);
}

////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

std::string debugDisplayScript(const RuleSoldierBonus* ri)
{
	if (ri)
	{
		std::string s;
		s += RuleSoldierBonus::ScriptName;
		s += "(name: \"";
		s += ri->getName();
		s += "\")";
		return s;
	}
	else
	{
		return "null";
	}
}

}

/**
 * Register RuleSoldierBonus in script parser.
 * @param parser Script parser.
 */
void RuleSoldierBonus::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<Mod>();

	Bind<RuleSoldierBonus> rsb = { parser };

	UnitStats::addGetStatsScript<&RuleSoldierBonus::_stats>(rsb, "Stats.");
	rsb.addScriptValue<BindBase::OnlyGet, &RuleSoldierBonus::_scriptValues>();
	rsb.addDebugDisplay<&debugDisplayScript>();
}


ModScript::ApplySoldierBonusesParser::ApplySoldierBonusesParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "unit", "save_game", "soldier_bonus", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

}
