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
void RuleSoldierBonus::load(const YAML::YamlNodeReader& node, Mod* mod, const ModScript &parsers)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod, parsers);
	}

	reader.tryRead("visibilityAtDark", _visibilityAtDark);
	reader.tryRead("visibilityAtDay", _visibilityAtDay);
	reader.tryRead("psiVision", _psiVision);
	reader.tryRead("heatVision", _visibilityThroughSmoke);
	reader.tryRead("visibilityThroughFire", _visibilityThroughFire);

	reader.tryRead("frontArmor", _frontArmor);
	reader.tryRead("sideArmor", _sideArmor);
	reader.tryRead("leftArmorDiff", _leftArmorDiff);
	reader.tryRead("rearArmor", _rearArmor);
	reader.tryRead("underArmor", _underArmor);
	_stats.merge(reader["stats"].readVal(_stats));
	const auto& rec = reader["recovery"];
	{
		_timeRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::TimeSoldierRecoveryStatBonus>());
		_energyRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::EnergySoldierRecoveryStatBonus>());
		_moraleRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::MoraleSoldierRecoveryStatBonus>());
		_healthRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::HealthSoldierRecoveryStatBonus>());
		_manaRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::ManaSoldierRecoveryStatBonus>());
		_stunRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::StunSoldierRecoveryStatBonus>());
	}

	_soldierBonusScripts.load(_name, reader, parsers.soldierBonusScripts);
	_scriptValues.load(reader, parsers.getShared());

	reader.tryRead("listOrder", _listOrder);
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
