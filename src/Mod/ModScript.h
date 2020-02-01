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
#include "../Engine/Script.h"

namespace OpenXcom
{

class Surface;
class SurfaceSet;
class Soldier;
class RuleCountry;
class RuleRegion;
class RuleBaseFacility;
class RuleCraft;
class RuleCraftWeapon;
class RuleItem;
struct RuleDamageType;
class RuleTerrain;
class MapDataSet;
class RuleSoldier;
class RuleSoldierBonus;
class Unit;
class Armor;
class RuleInventory;
class RuleResearch;
class RuleManufacture;
class AlienRace;
class RuleAlienMission;
class Base;
class MapScript;
class RuleVideo;

class Mod;
class BattleUnit;
class BattleUnitVisibility;
class BattleItem;
struct StatAdjustment;

class Ufo;
class RuleUfo;

class SavedBattleGame;
class SavedGame;

class ModScript
{
	friend class Mod;

	ScriptGlobal* _shared;
	Mod* _mod;

	ModScript(ScriptGlobal* shared, Mod* mod) : _shared{ shared }, _mod{ mod }
	{

	}

	using Output = ScriptOutputArgs<int&, int>;

	////////////////////////////////////////////////////////////
	//					unit script
	////////////////////////////////////////////////////////////

	struct RecolorUnitParser : ScriptParserEvents<Output, const BattleUnit*, int, int, int, int>
	{
		RecolorUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct SelectUnitParser : ScriptParserEvents<Output, const BattleUnit*, int, int, int>
	{
		SelectUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct SelectMoveSoundUnitParser : ScriptParserEvents<ScriptOutputArgs<int&>, const BattleUnit*, int, int, int, int, int, int, int>
	{
		SelectMoveSoundUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	struct ReactionUnitParser : ScriptParserEvents<Output, const BattleUnit*, const BattleUnit*, const BattleItem*, int, const BattleUnit*, int>
	{
		ReactionUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	struct VisibilityUnitParser : ScriptParserEvents<ScriptOutputArgs<int&, int, ScriptTag<BattleUnitVisibility>&>, const BattleUnit*, const BattleUnit*, int, int, int, int>
	{
		VisibilityUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct HitUnitParser : ScriptParserEvents<ScriptOutputArgs<int&, int&, int&>, BattleUnit*, BattleItem*, BattleItem*, BattleUnit*, SavedBattleGame*, int, int, int>
	{
		HitUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct TryPsiAttackUnitParser : ScriptParserEvents<ScriptOutputArgs<int&>, const BattleItem*, const BattleUnit*, const BattleUnit*, int, int, int>
	{
		TryPsiAttackUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct DamageUnitParser : ScriptParserEvents<ScriptOutputArgs<int&, int&, int&, int&, int&, int&, int&, int&, int&>, BattleUnit*, BattleItem*, BattleItem*, BattleUnit*, SavedBattleGame*, int, int, int, int, int, int>
	{
		DamageUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct HealUnitParser : ScriptParserEvents<ScriptOutputArgs<int&, int&, int&, int&, int&, int&, int&, int&, int&>, BattleUnit*, BattleItem*, SavedBattleGame*, BattleUnit*, int>
	{
		HealUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct CreateUnitParser : ScriptParserEvents<ScriptOutputArgs<>, BattleUnit*, SavedBattleGame*, int>
	{
		CreateUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct NewTurnUnitParser : ScriptParserEvents<ScriptOutputArgs<>, BattleUnit*, SavedBattleGame*, int, int>
	{
		NewTurnUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct ReturnFromMissionUnitParser : ScriptParserEvents<ScriptOutputArgs<int&, int>, BattleUnit*, SavedBattleGame*, Soldier*, const StatAdjustment*, const StatAdjustment*>
	{
		ReturnFromMissionUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	struct AwardExperienceParser : ScriptParserEvents<Output, const BattleUnit*, const BattleUnit*, const BattleItem*, int>
	{
		AwardExperienceParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	////////////////////////////////////////////////////////////
	//					item script
	////////////////////////////////////////////////////////////

	struct CreateItemParser : ScriptParserEvents<ScriptOutputArgs<>, BattleItem*, SavedBattleGame*, int>
	{
		CreateItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct NewTurnItemParser : ScriptParserEvents<ScriptOutputArgs<>, BattleItem*, SavedBattleGame*, int, int>
	{
		NewTurnItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct RecolorItemParser : ScriptParserEvents<Output, const BattleItem*, int, int, int>
	{
		RecolorItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct SelectItemParser : ScriptParserEvents<Output, const BattleItem*, int, int, int>
	{
		SelectItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	////////////////////////////////////////////////////////////
	//					bonus stat script
	////////////////////////////////////////////////////////////

	struct BonusStatsBaseParser : ScriptParserEvents<ScriptOutputArgs<int&>, const BattleUnit*, int>
	{
		BonusStatsBaseParser(ScriptGlobal* shared, const std::string& name, Mod* mod);

		/// Get name of YAML node where config is stored.
		const std::string& getPropertyNodeName() const { return _propertyNodeName; }

	protected:

		/// Name of YAML node where config is stored.
		std::string _propertyNodeName;
	};

	struct BonusStatsParser : BonusStatsBaseParser
	{
		BonusStatsParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : BonusStatsBaseParser{ shared, name + "BonusStats", mod }
		{
			_propertyNodeName = name;
		}
	};

	struct BonusStatsRecoveryParser : BonusStatsBaseParser
	{
		BonusStatsRecoveryParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : BonusStatsBaseParser{ shared, name + "RecoveryBonusStats", mod }
		{
			_propertyNodeName = name;
		}
	};

	struct BonusSoldierStatsRecoveryParser : BonusStatsBaseParser
	{
		BonusSoldierStatsRecoveryParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : BonusStatsBaseParser{ shared, name + "SoldierRecoveryBonusStats", mod }
		{
			_propertyNodeName = name;
		}
	};

	////////////////////////////////////////////////////////////
	//					ufo script
	////////////////////////////////////////////////////////////

	struct DetectUfoFromBaseParser : ScriptParserEvents<ScriptOutputArgs<int&, int&>, const Ufo*, int, int, int, int, int, int>
	{
		DetectUfoFromBaseParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	struct DetectUfoFromCraftParser : ScriptParserEvents<ScriptOutputArgs<int&, int&>, const Ufo*, int, int, int, int>
	{
		DetectUfoFromCraftParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	////////////////////////////////////////////////////////////
	//				soldier bonus stat script
	////////////////////////////////////////////////////////////

	struct ApplySoldierBonusesParser : ScriptParserEvents<ScriptOutputArgs<>, BattleUnit*, SavedBattleGame*, const RuleSoldierBonus*>
	{
		ApplySoldierBonusesParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

public:
	/// Get shared state.
	const ScriptGlobal* getShared() const
	{
		return _shared;
	}

	using ReactionCommon = ReactionUnitParser;
	using BonusStatsCommon = BonusStatsBaseParser;

	////////////////////////////////////////////////////////////
	//					unit script
	////////////////////////////////////////////////////////////

	using RecolorUnitSprite = MACRO_NAMED_SCRIPT("recolorUnitSprite", RecolorUnitParser);
	using SelectUnitSprite = MACRO_NAMED_SCRIPT("selectUnitSprite", SelectUnitParser);
	using SelectMoveSoundUnit = MACRO_NAMED_SCRIPT("selectMoveSoundUnit", SelectMoveSoundUnitParser);

	using ReactionUnitAction = MACRO_NAMED_SCRIPT("reactionUnitAction", ReactionUnitParser);
	using ReactionUnitReaction = MACRO_NAMED_SCRIPT("reactionUnitReaction", ReactionUnitParser);

	using TryPsiAttackUnit = MACRO_NAMED_SCRIPT("tryPsiAttackUnit", TryPsiAttackUnitParser);
	using HitUnit = MACRO_NAMED_SCRIPT("hitUnit", HitUnitParser);
	using DamageUnit = MACRO_NAMED_SCRIPT("damageUnit", DamageUnitParser);
	using HealUnit = MACRO_NAMED_SCRIPT("healUnit", HealUnitParser);
	using CreateUnit = MACRO_NAMED_SCRIPT("createUnit", CreateUnitParser);
	using NewTurnUnit = MACRO_NAMED_SCRIPT("newTurnUnit", NewTurnUnitParser);
	using ReturnFromMissionUnit = MACRO_NAMED_SCRIPT("returnFromMissionUnit", ReturnFromMissionUnitParser);

	using AwardExperience = MACRO_NAMED_SCRIPT("awardExperience", AwardExperienceParser);

	using VisibilityUnit = MACRO_NAMED_SCRIPT("visibilityUnit", VisibilityUnitParser);

	////////////////////////////////////////////////////////////
	//					item script
	////////////////////////////////////////////////////////////

	using RecolorItemSprite = MACRO_NAMED_SCRIPT("recolorItemSprite", RecolorItemParser);
	using SelectItemSprite = MACRO_NAMED_SCRIPT("selectItemSprite", SelectItemParser);

	using ReactionWeaponAction = MACRO_NAMED_SCRIPT("reactionWeaponAction", ReactionUnitParser);

	using CreateItem = MACRO_NAMED_SCRIPT("createItem", CreateItemParser);
	using NewTurnItem = MACRO_NAMED_SCRIPT("newTurnItem", NewTurnItemParser);

	////////////////////////////////////////////////////////////
	//					bonus stat script
	////////////////////////////////////////////////////////////

	using PsiDefenceStatBonus = MACRO_NAMED_SCRIPT("psiDefence", BonusStatsParser);
	using MeleeDodgeStatBonus = MACRO_NAMED_SCRIPT("meleeDodge", BonusStatsParser);

	using TimeRecoveryStatBonus = MACRO_NAMED_SCRIPT("time", BonusStatsRecoveryParser);
	using EnergyRecoveryStatBonus = MACRO_NAMED_SCRIPT("energy", BonusStatsRecoveryParser);
	using MoraleRecoveryStatBonus = MACRO_NAMED_SCRIPT("morale", BonusStatsRecoveryParser);
	using HealthRecoveryStatBonus = MACRO_NAMED_SCRIPT("health", BonusStatsRecoveryParser);
	using ManaRecoveryStatBonus = MACRO_NAMED_SCRIPT("mana", BonusStatsRecoveryParser);
	using StunRecoveryStatBonus = MACRO_NAMED_SCRIPT("stun", BonusStatsRecoveryParser);

	using TimeSoldierRecoveryStatBonus = MACRO_NAMED_SCRIPT("time", BonusSoldierStatsRecoveryParser);
	using EnergySoldierRecoveryStatBonus = MACRO_NAMED_SCRIPT("energy", BonusSoldierStatsRecoveryParser);
	using MoraleSoldierRecoveryStatBonus = MACRO_NAMED_SCRIPT("morale", BonusSoldierStatsRecoveryParser);
	using HealthSoldierRecoveryStatBonus = MACRO_NAMED_SCRIPT("health", BonusSoldierStatsRecoveryParser);
	using ManaSoldierRecoveryStatBonus = MACRO_NAMED_SCRIPT("mana", BonusSoldierStatsRecoveryParser);
	using StunSoldierRecoveryStatBonus = MACRO_NAMED_SCRIPT("stun", BonusSoldierStatsRecoveryParser);

	using DamageBonusStatBonus = MACRO_NAMED_SCRIPT("damageBonus", BonusStatsParser);
	using MeleeBonusStatBonus = MACRO_NAMED_SCRIPT("meleeBonus", BonusStatsParser);
	using AccuracyMultiplierStatBonus = MACRO_NAMED_SCRIPT("accuracyMultiplier", BonusStatsParser);
	using MeleeMultiplierStatBonus = MACRO_NAMED_SCRIPT("meleeMultiplier", BonusStatsParser);
	using ThrowMultiplierStatBonus = MACRO_NAMED_SCRIPT("throwMultiplier", BonusStatsParser);
	using CloseQuarterMultiplierStatBonus = MACRO_NAMED_SCRIPT("closeQuartersMultiplier", BonusStatsParser);

	////////////////////////////////////////////////////////////
	//					ufo script
	////////////////////////////////////////////////////////////

	using DetectUfoFromBase = MACRO_NAMED_SCRIPT("detectUfoFromBase", DetectUfoFromBaseParser);
	using DetectUfoFromCraft = MACRO_NAMED_SCRIPT("detectUfoFromCraft", DetectUfoFromCraftParser);

	////////////////////////////////////////////////////////////
	//					soldier bonus stat script
	////////////////////////////////////////////////////////////

	using ApplySoldierBonuses = MACRO_NAMED_SCRIPT("applySoldierBonuses", ApplySoldierBonusesParser);

	////////////////////////////////////////////////////////////
	//					groups
	////////////////////////////////////////////////////////////

	using BattleUnitScripts = ScriptGroup<Mod,
		RecolorUnitSprite,
		SelectUnitSprite,
		SelectMoveSoundUnit,

		ReactionUnitAction,
		ReactionUnitReaction,

		TryPsiAttackUnit,
		HitUnit,
		DamageUnit,
		HealUnit,
		CreateUnit,
		NewTurnUnit,
		ReturnFromMissionUnit,

		AwardExperience,

		VisibilityUnit
	>;

	using BattleItemScripts = ScriptGroup<Mod,
		RecolorItemSprite,
		SelectItemSprite,

		ReactionWeaponAction,

		CreateItem,
		NewTurnItem
	>;

	using BonusStatsScripts = ScriptGroup<Mod,
		PsiDefenceStatBonus,
		MeleeDodgeStatBonus,

		TimeRecoveryStatBonus,
		EnergyRecoveryStatBonus,
		MoraleRecoveryStatBonus,
		HealthRecoveryStatBonus,
		ManaRecoveryStatBonus,
		StunRecoveryStatBonus,

		TimeSoldierRecoveryStatBonus,
		EnergySoldierRecoveryStatBonus,
		MoraleSoldierRecoveryStatBonus,
		HealthSoldierRecoveryStatBonus,
		ManaSoldierRecoveryStatBonus,
		StunSoldierRecoveryStatBonus,

		DamageBonusStatBonus,
		MeleeBonusStatBonus,
		AccuracyMultiplierStatBonus,
		MeleeMultiplierStatBonus,
		ThrowMultiplierStatBonus,
		CloseQuarterMultiplierStatBonus
	>;

	using UfoScripts = ScriptGroup<Mod,
		DetectUfoFromBase,
		DetectUfoFromCraft
	>;

	using SoldierBonusScripts = ScriptGroup<Mod,
		ApplySoldierBonuses
	>;

	////////////////////////////////////////////////////////////
	//					members
	////////////////////////////////////////////////////////////
	BattleUnitScripts battleUnitScripts = { _shared, _mod, };
	BattleItemScripts battleItemScripts = { _shared, _mod, };
	BonusStatsScripts bonusStatsScripts = { _shared, _mod, };
	UfoScripts ufoScripts = { _shared, _mod, };
	SoldierBonusScripts soldierBonusScripts = { _shared, _mod, };


	////////////////////////////////////////////////////////////
	//					helper functions
	////////////////////////////////////////////////////////////

	/**
	 * Script helper that call script that do not return any values.
	 * @param t Obect that hold script data.
	 * @param args List of additionl parameters.
	 * @return void
	 */
	template<typename ScriptType, typename T, typename... Args>
	static auto scriptCallback(T* t, Args... args) -> std::enable_if_t<std::is_same<typename ScriptType::Output, ScriptOutputArgs<>>::value, void>
	{
		typename ScriptType::Output arg{};
		typename ScriptType::Worker work{ std::forward<Args>(args)... };

		work.execute(t->template getScript<ScriptType>(), arg);
	}

	/**
	 * Script helper that call script that return one value and take one parmeters using `ScriptType::Output`
	 * @param t Obect that hold script data.
	 * @param first First parameter of `ScriptType::Output`.
	 * @param args List of additionl parameters.
	 * @return final value of first parameter.
	 */
	template<typename ScriptType, typename T, typename... Args>
	static auto scriptFunc1(T* t, int first, Args... args) -> std::enable_if_t<std::is_same<typename ScriptType::Output, ScriptOutputArgs<int&>>::value, int>
	{
		typename ScriptType::Output arg{ first };
		typename ScriptType::Worker work{ std::forward<Args>(args)... };

		work.execute(t->template getScript<ScriptType>(), arg);

		return arg.getFirst();
	}

	/**
	 * Script helper that call script that return one value and take two parmeters using `ScriptType::Output`
	 * @param t Obect that hold script data.
	 * @param first First parameter of `ScriptType::Output`.
	 * @param second Second parameter of `ScriptType::Output`.
	 * @param args List of additionl parameters.
	 * @return final value of first parameter.
	 */
	template<typename ScriptType, typename T, typename... Args>
	static auto scriptFunc2(T* t, int first, int second, Args... args) -> std::enable_if_t<std::is_same<typename ScriptType::Output, Output>::value, int>
	{
		typename ScriptType::Output arg{ first, second };
		typename ScriptType::Worker work{ std::forward<Args>(args)... };

		work.execute(t->template getScript<ScriptType>(), arg);

		return arg.getFirst();
	}
};

}
