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
#include "Mod.h"
#include "ModScript.h"
#include <algorithm>
#include <functional>
#include <sstream>
#include <climits>
#include <cassert>
#include "../version.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/FileMap.h"
#include "../Engine/Palette.h"
#include "../Engine/Font.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Music.h"
#include "../Engine/GMCat.h"
#include "../Engine/SoundSet.h"
#include "../Engine/Sound.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "MapDataSet.h"
#include "RuleMusic.h"
#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"
#include "../Engine/Exception.h"
#include "../Engine/Logger.h"
#include "../Engine/ScriptBind.h"
#include "../Engine/Collections.h"
#include "SoundDefinition.h"
#include "ExtraSprites.h"
#include "CustomPalettes.h"
#include "ExtraSounds.h"
#include "../Engine/AdlibMusic.h"
#include "../Engine/CatFile.h"
#include "../fmath.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"
#include "../Battlescape/Pathfinding.h"
#include "RuleCountry.h"
#include "RuleRegion.h"
#include "RuleBaseFacility.h"
#include "RuleCraft.h"
#include "RuleCraftWeapon.h"
#include "RuleItemCategory.h"
#include "RuleItem.h"
#include "RuleWeaponSet.h"
#include "RuleUfo.h"
#include "RuleTerrain.h"
#include "MapScript.h"
#include "RuleSoldier.h"
#include "RuleSkill.h"
#include "RuleCommendations.h"
#include "AlienRace.h"
#include "RuleEnviroEffects.h"
#include "RuleStartingCondition.h"
#include "AlienDeployment.h"
#include "Armor.h"
#include "ArticleDefinition.h"
#include "RuleInventory.h"
#include "RuleResearch.h"
#include "RuleManufacture.h"
#include "RuleManufactureShortcut.h"
#include "ExtraStrings.h"
#include "RuleInterface.h"
#include "RuleArcScript.h"
#include "RuleEventScript.h"
#include "RuleEvent.h"
#include "RuleMissionScript.h"
#include "../Geoscape/Globe.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Region.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Transfer.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Savegame/AlienStrategy.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/SoldierDiary.h"
#include "UfoTrajectory.h"
#include "RuleAlienMission.h"
#include "MCDPatch.h"
#include "StatString.h"
#include "RuleGlobe.h"
#include "RuleVideo.h"
#include "RuleConverter.h"
#include "RuleSoldierTransformation.h"
#include "RuleSoldierBonus.h"

#define ARRAYLEN(x) (std::size(x))

namespace OpenXcom
{

namespace
{

struct OxceVersionDate
{
	int year = 0;
	int month = 0;
	int day = 0;

	OxceVersionDate(const std::string& data)
	{
		auto correct = false;
		// check if look like format " (v2023-10-21)"
		size_t offset = data.find(" (v");
		if (offset != std::string::npos && data.size() >= offset + 14 && data[offset + 2] == 'v' && data[offset + 7] == '-' && data[offset + 10] == '-' && data[offset + 13] == ')')
		{
			correct = (std::sscanf(data.data() + offset, " (v%4d-%2d-%2d)", &year, &month, &day) == 3);
		}

		if (!correct)
		{
			year = 0;
			month = 0;
			day = 0;
		}
	}

	explicit operator bool() const
	{
		return year && month && day;
	}
};


} //namespace


int Mod::DOOR_OPEN;
int Mod::SLIDING_DOOR_OPEN;
int Mod::SLIDING_DOOR_CLOSE;
int Mod::SMALL_EXPLOSION;
int Mod::LARGE_EXPLOSION;
int Mod::EXPLOSION_OFFSET;
int Mod::SMOKE_OFFSET;
int Mod::UNDERWATER_SMOKE_OFFSET;
int Mod::ITEM_DROP;
int Mod::ITEM_THROW;
int Mod::ITEM_RELOAD;
int Mod::WALK_OFFSET;
int Mod::FLYING_SOUND;
int Mod::BUTTON_PRESS;
int Mod::WINDOW_POPUP[3];
int Mod::UFO_FIRE;
int Mod::UFO_HIT;
int Mod::UFO_CRASH;
int Mod::UFO_EXPLODE;
int Mod::INTERCEPTOR_HIT;
int Mod::INTERCEPTOR_EXPLODE;
int Mod::GEOSCAPE_CURSOR;
int Mod::BASESCAPE_CURSOR;
int Mod::BATTLESCAPE_CURSOR;
int Mod::UFOPAEDIA_CURSOR;
int Mod::GRAPHS_CURSOR;
int Mod::DAMAGE_RANGE;
int Mod::EXPLOSIVE_DAMAGE_RANGE;
int Mod::FIRE_DAMAGE_RANGE[2];
std::string Mod::DEBRIEF_MUSIC_GOOD;
std::string Mod::DEBRIEF_MUSIC_BAD;
int Mod::DIFFICULTY_COEFFICIENT[5];
int Mod::SELL_PRICE_COEFFICIENT[5];
int Mod::BUY_PRICE_COEFFICIENT[5];
int Mod::DIFFICULTY_BASED_RETAL_DELAY[5];
int Mod::UNIT_RESPONSE_SOUNDS_FREQUENCY[4];
int Mod::PEDIA_FACILITY_RENDER_PARAMETERS[4];
bool Mod::EXTENDED_ITEM_RELOAD_COST;
bool Mod::EXTENDED_INVENTORY_SLOT_SORTING;
bool Mod::EXTENDED_RUNNING_COST;
int Mod::EXTENDED_MOVEMENT_COST_ROUNDING;
bool Mod::EXTENDED_HWP_LOAD_ORDER;
int Mod::EXTENDED_MELEE_REACTIONS;
int Mod::EXTENDED_TERRAIN_MELEE;
int Mod::EXTENDED_UNDERWATER_THROW_FACTOR;
bool Mod::EXTENDED_EXPERIENCE_AWARD_SYSTEM;

extern std::string OXCE_CURRENCY_SYMBOL;

constexpr size_t MaxDifficultyLevels = 5;


/// Special value for default string different to empty one.
const std::string Mod::STR_NULL = { '\0' };
/// Predefined name for first loaded mod that have all original data
const std::string ModNameMaster = "master";
/// Predefined name for current mod that is loading rulesets.
const std::string ModNameCurrent = "current";

/// Reduction of size allocated for transparency LUTs.
const size_t ModTransparencySizeReduction = 100;

void Mod::resetGlobalStatics()
{
	DOOR_OPEN = 3;
	SLIDING_DOOR_OPEN = 20;
	SLIDING_DOOR_CLOSE = 21;
	SMALL_EXPLOSION = 2;
	LARGE_EXPLOSION = 5;
	EXPLOSION_OFFSET = 0;
	SMOKE_OFFSET = 8;
	UNDERWATER_SMOKE_OFFSET = 0;
	ITEM_DROP = 38;
	ITEM_THROW = 39;
	ITEM_RELOAD = 17;
	WALK_OFFSET = 22;
	FLYING_SOUND = 15;
	BUTTON_PRESS = 0;
	WINDOW_POPUP[0] = 1;
	WINDOW_POPUP[1] = 2;
	WINDOW_POPUP[2] = 3;
	UFO_FIRE = 8;
	UFO_HIT = 12;
	UFO_CRASH = 10;
	UFO_EXPLODE = 11;
	INTERCEPTOR_HIT = 10;
	INTERCEPTOR_EXPLODE = 13;
	GEOSCAPE_CURSOR = 252;
	BASESCAPE_CURSOR = 252;
	BATTLESCAPE_CURSOR = 144;
	UFOPAEDIA_CURSOR = 252;
	GRAPHS_CURSOR = 252;
	DAMAGE_RANGE = 100;
	EXPLOSIVE_DAMAGE_RANGE = 50;
	FIRE_DAMAGE_RANGE[0] = 5;
	FIRE_DAMAGE_RANGE[1] = 10;
	DEBRIEF_MUSIC_GOOD = "GMMARS";
	DEBRIEF_MUSIC_BAD = "GMMARS";

	Globe::OCEAN_COLOR = Palette::blockOffset(12);
	Globe::OCEAN_SHADING = true;
	Globe::COUNTRY_LABEL_COLOR = 239;
	Globe::LINE_COLOR = 162;
	Globe::CITY_LABEL_COLOR = 138;
	Globe::BASE_LABEL_COLOR = 133;

	TextButton::soundPress = 0;

	Window::soundPopup[0] = 0;
	Window::soundPopup[1] = 0;
	Window::soundPopup[2] = 0;

	Pathfinding::red = 3;
	Pathfinding::yellow = 10;
	Pathfinding::green = 4;

	DIFFICULTY_COEFFICIENT[0] = 0;
	DIFFICULTY_COEFFICIENT[1] = 1;
	DIFFICULTY_COEFFICIENT[2] = 2;
	DIFFICULTY_COEFFICIENT[3] = 3;
	DIFFICULTY_COEFFICIENT[4] = 4;

	SELL_PRICE_COEFFICIENT[0] = 100;
	SELL_PRICE_COEFFICIENT[1] = 100;
	SELL_PRICE_COEFFICIENT[2] = 100;
	SELL_PRICE_COEFFICIENT[3] = 100;
	SELL_PRICE_COEFFICIENT[4] = 100;

	BUY_PRICE_COEFFICIENT[0] = 100;
	BUY_PRICE_COEFFICIENT[1] = 100;
	BUY_PRICE_COEFFICIENT[2] = 100;
	BUY_PRICE_COEFFICIENT[3] = 100;
	BUY_PRICE_COEFFICIENT[4] = 100;

	DIFFICULTY_BASED_RETAL_DELAY[0] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[1] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[2] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[3] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[4] = 0;

	UNIT_RESPONSE_SOUNDS_FREQUENCY[0] = 100; // select unit
	UNIT_RESPONSE_SOUNDS_FREQUENCY[1] = 100; // start moving
	UNIT_RESPONSE_SOUNDS_FREQUENCY[2] = 100; // select weapon
	UNIT_RESPONSE_SOUNDS_FREQUENCY[3] = 20;  // annoyed

	PEDIA_FACILITY_RENDER_PARAMETERS[0] = 2; // pedia facility max width
	PEDIA_FACILITY_RENDER_PARAMETERS[1] = 2; // pedia facility max height
	PEDIA_FACILITY_RENDER_PARAMETERS[2] = 0; // pedia facility X offset
	PEDIA_FACILITY_RENDER_PARAMETERS[3] = 0; // pedia facility Y offset

	EXTENDED_ITEM_RELOAD_COST = false;
	EXTENDED_INVENTORY_SLOT_SORTING = false;
	EXTENDED_RUNNING_COST = false;
	EXTENDED_MOVEMENT_COST_ROUNDING = 0;
	EXTENDED_HWP_LOAD_ORDER = false;
	EXTENDED_MELEE_REACTIONS = 0;
	EXTENDED_TERRAIN_MELEE = 0;
	EXTENDED_UNDERWATER_THROW_FACTOR = 0;
	EXTENDED_EXPERIENCE_AWARD_SYSTEM = true; // FIXME: change default to false in OXCE v8.0+ ?

	OXCE_CURRENCY_SYMBOL = "$";
}

/**
 * Detail
 */
class ModScriptGlobal : public ScriptGlobal
{
	size_t _modCurr = 0;
	std::vector<std::pair<std::string, int>> _modNames;
	ScriptValues<Mod> _scriptValues;

	void loadRuleList(int &value, const YAML::YamlNodeReader& reader) const
	{
		if (reader)
		{
			auto name = reader.readVal<std::string>();
			if (name == ModNameMaster)
			{
				value = 0;
			}
			else if (name == ModNameCurrent)
			{
				value = _modCurr;
			}
			else
			{
				for (const auto& p : _modNames)
				{
					if (name == p.first)
					{
						value = p.second;
						return;
					}
				}
				value = -1;
			}
		}
	}
	void saveRuleList(const int &value, YAML::YamlNodeWriter& writer) const
	{
		for (const auto& p : _modNames)
		{
			if (value == p.second)
			{
				writer.setValue(p.first);
				return;
			}
		}
	}

public:
	/// Initialize shared globals like types.
	void initParserGlobals(ScriptParserBase* parser) override
	{
		parser->registerPointerType<Mod>();
		parser->registerPointerType<SavedGame>();
		parser->registerPointerType<SavedBattleGame>();
	}

	/// Prepare for loading data.
	void beginLoad() override
	{
		ScriptGlobal::beginLoad();

		addTagValueType<ModScriptGlobal, &ModScriptGlobal::loadRuleList, &ModScriptGlobal::saveRuleList>("RuleList");
		addConst("RuleList." + ModNameMaster, (int)0);
		addConst("RuleList." + ModNameCurrent, (int)0);

		auto v = OxceVersionDate(OPENXCOM_VERSION_GIT);
		addConst("SCRIPT_VERSION_DATE", (int)(v.year * 10000 + v.month * 100 + v.day));
	}
	/// Finishing loading data.
	void endLoad() override
	{
		ScriptGlobal::endLoad();
	}

	/// Add mod name and id.
	void addMod(const std::string& s, int i)
	{
		auto name = "RuleList." + s;
		addConst(name, (int)i);
		_modNames.push_back(std::make_pair(s, i));
	}
	/// Set current mod id.
	void setMod(int i)
	{
		updateConst("RuleList." + ModNameCurrent, (int)i);
		_modCurr = i;
	}

	/// Get script values
	ScriptValues<Mod>& getScriptValues() { return _scriptValues; }
};

/**
 * Creates an empty mod.
 */
Mod::Mod() :
	_inventoryOverlapsPaperdoll(false),
	_maxViewDistance(20), _maxDarknessToSeeUnits(9), _maxStaticLightDistance(16), _maxDynamicLightDistance(24), _enhancedLighting(0),
	_costHireEngineer(0), _costHireScientist(0),
	_costEngineer(0), _costScientist(0), _timePersonnel(0), _hireByCountryOdds(0), _hireByRegionOdds(0), _initialFunding(0),
	_aiUseDelayBlaster(3), _aiUseDelayFirearm(0), _aiUseDelayGrenade(3), _aiUseDelayProxy(999), _aiUseDelayMelee(0), _aiUseDelayPsionic(0),
	_aiFireChoiceIntelCoeff(5), _aiFireChoiceAggroCoeff(5), _aiExtendedFireModeChoice(false), _aiRespectMaxRange(false), _aiDestroyBaseFacilities(false),
	_aiPickUpWeaponsMoreActively(false), _aiPickUpWeaponsMoreActivelyCiv(false),
	_maxLookVariant(0), _tooMuchSmokeThreshold(10), _customTrainingFactor(100), _minReactionAccuracy(0), _chanceToStopRetaliation(0), _lessAliensDuringBaseDefense(false),
	_allowCountriesToCancelAlienPact(false), _buildInfiltrationBaseCloseToTheCountry(false), _infiltrateRandomCountryInTheRegion(false), _allowAlienBasesOnWrongTextures(true),
	_kneelBonusGlobal(115), _oneHandedPenaltyGlobal(80),
	_enableCloseQuartersCombat(0), _closeQuartersAccuracyGlobal(100), _closeQuartersTuCostGlobal(12), _closeQuartersEnergyCostGlobal(8), _closeQuartersSneakUpGlobal(0),
	_noLOSAccuracyPenaltyGlobal(-1),
	_surrenderMode(0),
	_bughuntMinTurn(999), _bughuntMaxEnemies(2), _bughuntRank(0), _bughuntLowMorale(40), _bughuntTimeUnitsLeft(60),
	_manaEnabled(false), _manaBattleUI(false), _manaTrainingPrimary(false), _manaTrainingSecondary(false), _manaReplenishAfterMission(true),
	_loseMoney("loseGame"), _loseRating("loseGame"), _loseDefeat("loseGame"),
	_ufoGlancingHitThreshold(0), _ufoBeamWidthParameter(1000),
	_escortRange(20), _drawEnemyRadarCircles(1), _escortsJoinFightAgainstHK(true), _hunterKillerFastRetarget(true),
	_crewEmergencyEvacuationSurvivalChance(100), _pilotsEmergencyEvacuationSurvivalChance(100),
	_soldiersPerRank({-1, -1, 5, 11, 23, 30}),
	_pilotAccuracyZeroPoint(55), _pilotAccuracyRange(40), _pilotReactionsZeroPoint(55), _pilotReactionsRange(60),
	_performanceBonusFactor(0.0), _enableNewResearchSorting(false), _displayCustomCategories(0), _shareAmmoCategories(false), _showDogfightDistanceInKm(false), _showFullNameInAlienInventory(false),
	_alienInventoryOffsetX(80), _alienInventoryOffsetBigUnit(32),
	_hidePediaInfoButton(false), _extraNerdyPediaInfoType(0),
	_giveScoreAlsoForResearchedArtifacts(false), _statisticalBulletConservation(false), _stunningImprovesMorale(false),
	_tuRecoveryWakeUpNewTurn(100), _shortRadarRange(0), _buildTimeReductionScaling(100),
	_defeatScore(0), _defeatFunds(0), _difficultyDemigod(false), _startingTime(6, 1, 1, 1999, 12, 0, 0), _startingDifficulty(0),
	_baseDefenseMapFromLocation(0), _disableUnderwaterSounds(false), _enableUnitResponseSounds(false), _pediaReplaceCraftFuelWithRangeType(-1),
	_facilityListOrder(0), _craftListOrder(0), _itemCategoryListOrder(0), _itemListOrder(0), _armorListOrder(0), _alienRaceListOrder(0),
	_researchListOrder(0),  _manufactureListOrder(0), _soldierBonusListOrder(0), _transformationListOrder(0), _ufopaediaListOrder(0), _invListOrder(0), _soldierListOrder(0),
	_modCurrent(0), _statePalette(0),
	_startingBaseBeginner(""), _startingBaseDefault(""), _startingBaseExperienced(""), _startingBaseVeteran(""), _startingBaseGenius(""), _startingBaseSuperhuman("")
{
	_muteMusic = new Music();
	_muteSound = new Sound();
	_globe = new RuleGlobe();
	_scriptGlobal = new ModScriptGlobal();

	//load base damage types
	RuleDamageType *dmg;
	_damageTypes.resize(DAMAGE_TYPES);

	dmg = new RuleDamageType();
	dmg->ResistType = DT_NONE;
	dmg->RandomType = DRT_NONE;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_AP;
	dmg->IgnoreOverKill = true;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_ACID;
	dmg->IgnoreOverKill = true;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_LASER;
	dmg->IgnoreOverKill = true;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_PLASMA;
	dmg->IgnoreOverKill = true;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_MELEE;
	dmg->IgnoreOverKill = true;
	dmg->IgnoreSelfDestruct = true;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_STUN;
	dmg->FixRadius = -1;
	dmg->IgnoreOverKill = true;
	dmg->IgnoreSelfDestruct = true;
	dmg->IgnorePainImmunity = true;
	dmg->RadiusEffectiveness = 0.05f;
	dmg->ToHealth = 0.0f;
	dmg->ToArmor = 0.0f;
	dmg->ToWound = 0.0f;
	dmg->ToItem = 0.0f;
	dmg->ToTile = 0.0f;
	dmg->ToStun = 1.0f;
	dmg->RandomStun = false;
	dmg->TileDamageMethod = 2;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_HE;
	dmg->RandomType = DRT_EXPLOSION;
	dmg->FixRadius = -1;
	dmg->IgnoreOverKill = true;
	dmg->IgnoreSelfDestruct = true;
	dmg->RadiusEffectiveness = 0.05f;
	dmg->ToItem = 1.0f;
	dmg->TileDamageMethod = 2;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_SMOKE;
	dmg->RandomType = DRT_NONE;
	dmg->FixRadius = -1;
	dmg->IgnoreOverKill = true;
	dmg->IgnoreDirection = true;
	dmg->ArmorEffectiveness = 0.0f;
	dmg->RadiusEffectiveness = 0.05f;
	dmg->SmokeThreshold = 0;
	dmg->ToHealth = 0.0f;
	dmg->ToArmor = 0.0f;
	dmg->ToWound = 0.0f;
	dmg->ToItem = 0.0f;
	dmg->ToTile = 0.0f;
	dmg->ToStun = 1.0f;
	dmg->TileDamageMethod = 2;
	_damageTypes[dmg->ResistType] = dmg;

	dmg = new RuleDamageType();
	dmg->ResistType = DT_IN;
	dmg->RandomType = DRT_FIRE;
	dmg->FixRadius = -1;
	dmg->FireBlastCalc = true;
	dmg->IgnoreOverKill = true;
	dmg->IgnoreDirection = true;
	dmg->IgnoreSelfDestruct = true;
	dmg->ArmorEffectiveness = 0.0f;
	dmg->RadiusEffectiveness = 0.03f;
	dmg->FireThreshold = 0;
	dmg->ToHealth = 1.0f;
	dmg->ToArmor = 0.0f;
	dmg->ToWound = 0.0f;
	dmg->ToItem = 0.0f;
	dmg->ToTile = 0.0f;
	dmg->ToStun = 0.0f;
	dmg->TileDamageMethod = 2;
	_damageTypes[dmg->ResistType] = dmg;

	for (int itd = DT_10; itd < DAMAGE_TYPES; ++itd)
	{
		dmg = new RuleDamageType();
		dmg->ResistType = static_cast<ItemDamageType>(itd);
		dmg->IgnoreOverKill = true;
		_damageTypes[dmg->ResistType] = dmg;
	}

	_converter = new RuleConverter();
	_statAdjustment.resize(MaxDifficultyLevels);
	_statAdjustment[0].aimMultiplier = 0.5;
	_statAdjustment[0].armorMultiplier = 0.5;
	_statAdjustment[0].armorMultiplierAbs = 0;
	_statAdjustment[0].growthMultiplier = 0;
	for (size_t i = 1; i != MaxDifficultyLevels; ++i)
	{
		_statAdjustment[i].aimMultiplier = 1.0;
		_statAdjustment[i].armorMultiplier = 1.0;
		_statAdjustment[i].armorMultiplierAbs = 0;
		_statAdjustment[i].growthMultiplier = (int)i;
	}

	// Setting default value for array
	_ufoTractorBeamSizeModifiers[0] = 400;
	_ufoTractorBeamSizeModifiers[1] = 200;
	_ufoTractorBeamSizeModifiers[2] = 100;
	_ufoTractorBeamSizeModifiers[3] = 50;
	_ufoTractorBeamSizeModifiers[4] = 25;

	_pilotBraveryThresholds[0] = 90;
	_pilotBraveryThresholds[1] = 80;
	_pilotBraveryThresholds[2] = 30;
}

/**
 * Deletes all the mod data from memory.
 */
Mod::~Mod()
{
	delete _muteMusic;
	delete _muteSound;
	delete _globe;
	delete _converter;
	delete _scriptGlobal;
	for (auto& pair : _fonts)
	{
		delete pair.second;
	}
	for (auto& pair : _surfaces)
	{
		delete pair.second;
	}
	for (auto& pair : _sets)
	{
		delete pair.second;
	}
	for (auto& pair : _palettes)
	{
		delete pair.second;
	}
	for (auto& pair : _musics)
	{
		delete pair.second;
	}
	for (auto& pair : _sounds)
	{
		delete pair.second;
	}
	for (auto* rdt : _damageTypes)
	{
		delete rdt;
	}
	for (auto& pair : _countries)
	{
		delete pair.second;
	}
	for (auto& pair : _extraGlobeLabels)
	{
		delete pair.second;
	}
	for (auto& pair : _regions)
	{
		delete pair.second;
	}
	for (auto& pair : _facilities)
	{
		delete pair.second;
	}
	for (auto& pair : _crafts)
	{
		delete pair.second;
	}
	for (auto& pair : _craftWeapons)
	{
		delete pair.second;
	}
	for (auto& pair : _itemCategories)
	{
		delete pair.second;
	}
	for (auto& pair : _items)
	{
		delete pair.second;
	}
	for (auto& pair : _weaponSets)
	{
		delete pair.second;
	}
	for (auto& pair : _ufos)
	{
		delete pair.second;
	}
	for (auto& pair : _terrains)
	{
		delete pair.second;
	}
	for (auto& pair : _mapDataSets)
	{
		delete pair.second;
	}
	for (auto& pair : _soldiers)
	{
		delete pair.second;
	}
	for (auto& pair : _skills)
	{
		delete pair.second;
	}
	for (auto& pair : _units)
	{
		delete pair.second;
	}
	for (auto& pair : _alienRaces)
	{
		delete pair.second;
	}
	for (auto& pair : _enviroEffects)
	{
		delete pair.second;
	}
	for (auto& pair : _startingConditions)
	{
		delete pair.second;
	}
	for (auto& pair : _alienDeployments)
	{
		delete pair.second;
	}
	for (auto& pair : _armors)
	{
		delete pair.second;
	}
	for (auto& pair : _ufopaediaArticles)
	{
		delete pair.second;
	}
	for (auto& pair : _invs)
	{
		delete pair.second;
	}
	for (auto& pair : _research)
	{
		delete pair.second;
	}
	for (auto& pair : _manufacture)
	{
		delete pair.second;
	}
	for (auto& pair : _manufactureShortcut)
	{
		delete pair.second;
	}
	for (auto& pair : _soldierBonus)
	{
		delete pair.second;
	}
	for (auto& pair : _soldierTransformation)
	{
		delete pair.second;
	}
	for (auto& pair : _ufoTrajectories)
	{
		delete pair.second;
	}
	for (auto& pair : _alienMissions)
	{
		delete pair.second;
	}
	for (auto& pair : _MCDPatches)
	{
		delete pair.second;
	}
	for (auto& pair : _extraSprites)
	{
		for (auto* extraSprites : pair.second)
		{
			delete extraSprites;
		}
	}
	for (auto& pair : _customPalettes)
	{
		delete pair.second;
	}
	for (auto& pair : _extraSounds)
	{
		delete pair.second;
	}
	for (auto& pair : _extraStrings)
	{
		delete pair.second;
	}
	for (auto& pair : _interfaces)
	{
		delete pair.second;
	}
	for (auto& pair : _mapScripts)
	{
		for (auto* mapScript : pair.second)
		{
			delete mapScript;
		}
	}
	for (auto& pair : _videos)
	{
		delete pair.second;
	}
	for (auto& pair : _musicDefs)
	{
		delete pair.second;
	}
	for (auto& pair : _arcScripts)
	{
		delete pair.second;
	}
	for (auto& pair : _eventScripts)
	{
		delete pair.second;
	}
	for (auto& pair : _events)
	{
		delete pair.second;
	}
	for (auto& pair : _missionScripts)
	{
		delete pair.second;
	}
	for (auto& pair : _soundDefs)
	{
		delete pair.second;
	}
	for (auto* statString : _statStrings)
	{
		delete statString;
	}
	for (auto& pair : _commendations)
	{
		delete pair.second;
	}
}

/**
 * Gets a specific rule element by ID.
 * @param id String ID of the rule element.
 * @param name Human-readable name of the rule type.
 * @param map Map associated to the rule type.
 * @param error Throw an error if not found.
 * @return Pointer to the rule element, or NULL if not found.
 */
template <typename T>
T *Mod::getRule(const std::string &id, const std::string &name, const std::map<std::string, T*> &map, bool error) const
{
	if (isEmptyRuleName(id))
	{
		return 0;
	}
	typename std::map<std::string, T*>::const_iterator i = map.find(id);
	if (i != map.end() && i->second != 0)
	{
		return i->second;
	}
	else
	{
		if (error)
		{
			throw Exception(name + " " + id + " not found");
		}
		return 0;
	}
}

/**
 * Returns a specific font from the mod.
 * @param name Name of the font.
 * @return Pointer to the font.
 */
Font *Mod::getFont(const std::string &name, bool error) const
{
	return getRule(name, "Font", _fonts, error);
}

/**
 * Loads any extra sprites associated to a surface when
 * it's first requested.
 * @param name Surface name.
 */
void Mod::lazyLoadSurface(const std::string &name)
{
	if (Options::lazyLoadResources)
	{
		auto i = _extraSprites.find(name);
		if (i != _extraSprites.end())
		{
			for (auto* extraSprites : i->second)
			{
				loadExtraSprite(extraSprites);
			}
		}
	}
}

/**
 * Returns a specific surface from the mod.
 * @param name Name of the surface.
 * @return Pointer to the surface.
 */
Surface *Mod::getSurface(const std::string &name, bool error)
{
	lazyLoadSurface(name);
	return getRule(name, "Sprite", _surfaces, error);
}

/**
 * Returns a specific surface set from the mod.
 * @param name Name of the surface set.
 * @return Pointer to the surface set.
 */
SurfaceSet *Mod::getSurfaceSet(const std::string &name, bool error)
{
	lazyLoadSurface(name);
	return getRule(name, "Sprite Set", _sets, error);
}

/**
 * Returns a specific music from the mod.
 * @param name Name of the music.
 * @return Pointer to the music.
 */
Music *Mod::getMusic(const std::string &name, bool error) const
{
	if (Options::mute)
	{
		return _muteMusic;
	}
	else
	{
		return getRule(name, "Music", _musics, error);
	}
}

/**
 * Returns the list of all music tracks
 * provided by the mod.
 * @return List of music tracks.
 */
const std::map<std::string, Music*> &Mod::getMusicTrackList() const
{
	return _musics;
}

/**
 * Returns a random music from the mod.
 * @param name Name of the music to pick from.
 * @return Pointer to the music.
 */
Music *Mod::getRandomMusic(const std::string &name) const
{
	if (Options::mute)
	{
		return _muteMusic;
	}
	else
	{
		std::vector<Music*> music;
		for (auto& pair : _musics)
		{
			if (pair.first.find(name) != std::string::npos)
			{
				music.push_back(pair.second);
			}
		}
		if (music.empty())
		{
			return _muteMusic;
		}
		else
		{
			return music[RNG::seedless(0, music.size() - 1)];
		}
	}
}

/**
 * Plays the specified track if it's not already playing.
 * @param name Name of the music.
 * @param id Id of the music, 0 for random.
 */
void Mod::playMusic(const std::string &name, int id)
{
	if (!Options::mute && _playingMusic != name)
	{
		int loop = -1;
		// hacks
		if (!Options::musicAlwaysLoop && (name == "GMSTORY" || name == "GMWIN" || name == "GMLOSE"))
		{
			loop = 0;
		}

		Music *music = 0;
		if (id == 0)
		{
			music = getRandomMusic(name);
		}
		else
		{
			std::ostringstream ss;
			ss << name << id;
			music = getMusic(ss.str());
		}
		music->play(loop);
		if (music != _muteMusic)
		{
			_playingMusic = name;
			for (auto& item : _musics)
			{
				if (item.second == music)
				{
					setCurrentMusicTrack(item.first);
					break;
				}
			}
		}
		Log(LOG_VERBOSE)<<"Mod::playMusic('" << name << "'): playing " << _playingMusic;
	}
}

/**
 * Returns a specific sound set from the mod.
 * @param name Name of the sound set.
 * @return Pointer to the sound set.
 */
SoundSet *Mod::getSoundSet(const std::string &name, bool error) const
{
	return getRule(name, "Sound Set", _sounds, error);
}

/**
 * Returns a specific sound from the mod.
 * @param set Name of the sound set.
 * @param sound ID of the sound.
 * @return Pointer to the sound.
 */
Sound *Mod::getSound(const std::string &set, int sound) const
{
	if (Options::mute)
	{
		return _muteSound;
	}
	else
	{
		SoundSet *ss = getSoundSet(set, false);
		if (ss != 0)
		{
			Sound *s = ss->getSound(sound);
			if (s == 0)
			{
				Log(LOG_ERROR) << "Sound " << sound << " in " << set << " not found";
				return _muteSound;
			}
			return s;
		}
		else
		{
			Log(LOG_ERROR) << "SoundSet " << set << " not found";
			return _muteSound;
		}
	}
}

/**
 * Returns a specific palette from the mod.
 * @param name Name of the palette.
 * @return Pointer to the palette.
 */
Palette *Mod::getPalette(const std::string &name, bool error) const
{
	return getRule(name, "Palette", _palettes, error);
}

/**
 * Returns the list of voxeldata in the mod.
 * @return Pointer to the list of voxeldata.
 */
const std::vector<Uint16> *Mod::getVoxelData() const
{
	return &_voxelData;
}

/**
 * Returns a specific sound from either the land or underwater sound set.
 * @param depth the depth of the battlescape.
 * @param sound ID of the sound.
 * @return Pointer to the sound.
 */
Sound *Mod::getSoundByDepth(unsigned int depth, unsigned int sound) const
{
	if (depth == 0 || _disableUnderwaterSounds)
		return getSound("BATTLE.CAT", sound);
	else
		return getSound("BATTLE2.CAT", sound);
}

/**
 * Returns the list of color LUTs in the mod.
 * @return Pointer to the list of LUTs.
 */
const std::vector<std::vector<Uint8> > *Mod::getLUTs() const
{
	return &_transparencyLUTs;
}


/**
 * Check for obsolete error based on year.
 * @param year Year when given function stop be available.
 * @return True if code still should run.
 */
bool Mod::checkForObsoleteErrorByYear(const std::string &parent, const YAML::YamlNodeReader& reader, const std::string &error, int year) const
{
	SeverityLevel level = LOG_INFO;
	bool r = true;

	const static OxceVersionDate currYear = { OPENXCOM_VERSION_GIT };
	if (currYear)
	{
		if (currYear.year < year)
		{
			level = LOG_INFO;
		}
		else if (currYear.year == year)
		{
			level = LOG_WARNING;
		}
		else // after obsolete year functionality is disabled
		{
			level = LOG_ERROR;
			r = false;
		}
	}
	checkForSoftError(true, parent, reader, "Obsolete (to removed after year " + std::to_string(year) + ") operation " + error, level);

	return r;
}

/**
 * Verify if value have defined surface in given set.
 */
void Mod::verifySpriteOffset(const std::string &parent, const int& sprite, const std::string &set) const
{
	if (Options::lazyLoadResources)
	{
		// we can't check if index is correct when set is loaded
		return;
	}

	auto* s = getRule(set, "Sprite Set", _sets, true);

	if (s->getTotalFrames() == 0)
	{
		// HACK: some sprites should be shared between different sets (for example 'Projectiles' and 'UnderwaterProjectiles'),
		// but in some cases one set is not used (for example if the weapon is 'underwaterOnly: true').
		// If there are no surfaces at all, this means this index is not used.
		// In some corner cases it will not work correcty, for example if someone does not add any surface to the set at all.
		return;
	}

	checkForSoftError(
		sprite != Mod::NO_SURFACE && s->getFrame(sprite) == nullptr,
		parent,
		"Wrong index " + std::to_string(sprite) + " for surface set " + set + " (please note that the index in the ruleset is smaller, by several thousands)",
		LOG_ERROR
	);
}

/**
 * Verify if value have defined surface in given set.
 */
void Mod::verifySpriteOffset(const std::string &parent, const std::vector<int>& sprites, const std::string &set) const
{
	if (Options::lazyLoadResources)
	{
		// we can't check if index is correct when set is loaded
		return;
	}

	auto* s = getRule(set, "Sprite Set", _sets, true);

	if (s->getTotalFrames() == 0)
	{
		// HACK: some sprites should be shared between different sets (for example 'Projectiles' and 'UnderwaterProjectiles'),
		// but in some cases one set is not used (for example if the weapon is 'underwaterOnly: true').
		// If there are no surfaces at all, this means this index is not used.
		// In some corner cases it will not work correcty, for example if someone does not add any surface to the set at all.
		return;
	}

	for (int sprite : sprites)
	{
		checkForSoftError(
			sprite != Mod::NO_SURFACE && s->getFrame(sprite) == nullptr,
			parent,
			"Wrong index " + std::to_string(sprite) + " for surface set " + set + " (please note that the index in the ruleset is smaller, by several thousands)",
			LOG_ERROR
		);
	}
}

/**
 * Verify if value have defined sound in given set.
 */
void Mod::verifySoundOffset(const std::string &parent, const int& sound, const std::string &set) const
{
	if (Options::mute)
	{
		// when mute is set not sound data is loaded and we can't check for correct data
		return;
	}

	auto* s = getSoundSet(set);

	checkForSoftError(
		sound != Mod::NO_SOUND && s->getSound(sound) == nullptr,
		parent,
		"Wrong index " + std::to_string(sound) + " for sound set " + set + " (please note that the index in the ruleset is smaller, by several thousands)",
		LOG_ERROR
	);
}

/**
 * Verify if value have defined sound in given set.
 */
void Mod::verifySoundOffset(const std::string &parent, const std::vector<int>& sounds, const std::string &set) const
{
	if (Options::mute)
	{
		// when mute is set not sound data is loaded and we can't check for correct data
		return;
	}

	auto* s = getSoundSet(set);

	for (int sound : sounds)
	{
		checkForSoftError(
			sound != Mod::NO_SOUND && s->getSound(sound) == nullptr,
			parent,
			"Wrong index " + std::to_string(sound) + " for sound set " + set + " (please note that the index in the ruleset is smaller, by several thousands)",
			LOG_ERROR
		);
	}
}


/**
 * Returns the current mod-based offset for resources.
 * @return Mod offset.
 */
int Mod::getModOffset() const
{
	return _modCurrent->offset;
}



namespace
{

const std::string InfoTag = "!info";
const std::string AddTag = "!add";
const std::string RemoveTag = "!remove";

bool isListHelper(const YAML::YamlNodeReader& reader)
{
	return reader.isSeq() && (!reader.hasValTag() || reader.hasValTag(ryml::TAG_SEQ) || reader.hasValTag(InfoTag));
}

bool isListAddTagHelper(const YAML::YamlNodeReader& reader)
{
	return reader.isSeq() && reader.hasValTag(AddTag);
}

bool isListRemoveTagHelper(const YAML::YamlNodeReader& reader)
{
	return reader.isSeq() && reader.hasValTag(RemoveTag);
}

bool isMapHelper(const YAML::YamlNodeReader& reader)
{
	return reader.isMap() && (!reader.hasValTag() || reader.hasValTag(ryml::TAG_SEQ) || reader.hasValTag(InfoTag));
}

bool isMapAddTagHelper(const YAML::YamlNodeReader& reader)
{
	return reader.isMap() && reader.hasValTag(AddTag);
}

void throwOnBadListHelper(const std::string &parent, const YAML::YamlNodeReader& reader)
{
	std::ostringstream err;
	if (reader.isSeq())
	{
		// it is a sequence, but it could not be loaded... this means the tag is not supported
		err << "unsupported node tag '" << reader.getValTag() << "'";
	}
	else
	{
		err << "wrong node type, expected a list";
	}
	throw LoadRuleException(parent, reader, err.str());
}

void throwOnBadMapHelper(const std::string &parent, const YAML::YamlNodeReader& reader)
{
	std::ostringstream err;
	if (reader.isMap())
	{
		// it is a map, but it could not be loaded... this means the tag is not supported
		err << "unsupported node tag '" << reader.getValTag() << "'";
	}
	else
	{
		err << "wrong node type, expected a map";
	}
	throw LoadRuleException(parent, reader, err.str());
}

template<typename... T>
void showInfo(const std::string &parent, const YAML::YamlNodeReader& reader, T... names)
{
	if (reader.hasValTag(InfoTag))
	{
		Logger info;
		info.get() << "Options available for " << parent << " at line " << reader.getLocationInFile().line << " are: ";
		((info.get() << " " << names), ...);
	}
}



/**
 * Tag dispatch struct representing normal load logic.
 */
struct LoadFuncStandard
{
	auto funcTagForNew() -> LoadFuncStandard { return { }; }
};

/**
 * Tag dispatch struct representing special function that allows adding and removing elements.
 */
struct LoadFuncEditable
{
	auto funcTagForNew() -> LoadFuncStandard { return { }; }
};

/**
 * Tag dispatch struct representing can have null value.
 */
struct LoadFuncNullable
{
	auto funcTagForNew() -> LoadFuncNullable { return { }; }
};



/**
 * Terminal function loading integer.
 */
void loadHelper(const std::string &parent, int& v, const YAML::YamlNodeReader& reader)
{
	v = reader.readVal<int>();
}

/**
 * Terminal function loading string.
 * Function can't load empty string.
 */
void loadHelper(const std::string &parent, std::string& v, const YAML::YamlNodeReader& reader)
{
	v = reader.readVal<std::string>();
	if (Mod::isEmptyRuleName(v))
	{
		throw LoadRuleException(parent, reader, "Invalid value for name");
	}
}

/**
 * Function loading string.
 * If node do not exists then it do not change value.
 * Function can't load empty string.
 */
void loadHelper(const std::string &parent, std::string& v, const YAML::YamlNodeReader& reader, LoadFuncStandard)
{
	if (reader)
	{
		loadHelper(parent, v, reader);
	}
}

/**
 * Function loading string with option for pseudo null value.
 * If node do not exists then it do not change value.
 */
void loadHelper(const std::string &parent, std::string& v, const YAML::YamlNodeReader& reader, LoadFuncNullable)
{
	if (reader)
	{
		if (reader.hasNullVal())
		{
			v = Mod::STR_NULL;
		}
		else
		{
			v = reader.readVal<std::string>();
			if (v == Mod::STR_NULL)
			{
				throw LoadRuleException(parent, reader, "Invalid value for name ");
			}
		}
	}
}

template<typename T, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::vector<T>& v, const YAML::YamlNodeReader& reader, LoadFuncStandard, LoadFuncTag... rest)
{
	if (reader)
	{
		showInfo(parent, reader);

		if (isListHelper(reader))
		{
			v.clear();
			v.reserve(reader.childrenCount());
			for (const auto& n : reader.children())
			{
				loadHelper(parent, v.emplace_back(), n, rest.funcTagForNew()...);
			}
		}
		else
		{
			throwOnBadListHelper(parent, reader);
		}
	}
}

template<typename T, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::vector<T>& v, const YAML::YamlNodeReader& reader, LoadFuncEditable, LoadFuncTag... rest)
{
	if (reader)
	{
		showInfo(parent, reader, AddTag, RemoveTag);

		if (isListHelper(reader))
		{
			v.clear();
			v.reserve(reader.childrenCount());
			for (const auto& n : reader.children())
			{
				loadHelper(parent, v.emplace_back(), n, rest.funcTagForNew()...);
			}
		}
		else if (isListAddTagHelper(reader))
		{
			v.reserve(v.size() + reader.childrenCount());
			for (const auto& n : reader.children())
			{
				loadHelper(parent, v.emplace_back(), n, rest...);
			}
		}
		else if (isListRemoveTagHelper(reader))
		{
			const auto begin = v.begin();
			auto end = v.end();
			for (const auto& n : reader.children())
			{
				end = std::remove(begin, end, n.readVal<T>());
			}
			v.erase(end, v.end());
		}
		else
		{
			throwOnBadListHelper(parent, reader);
		}
	}
}

template<typename K, typename V, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::map<K, V>& v, const YAML::YamlNodeReader& reader, LoadFuncStandard, LoadFuncTag... rest)
{
	if (reader)
	{
		showInfo(parent, reader);

		if (isMapHelper(reader))
		{
			v.clear();
			for (const auto& n : reader.children())
			{
				auto key = n.readKey<K>();

				loadHelper(parent, v[key], n, rest.funcTagForNew()...);
			}
		}
		else
		{
			throwOnBadMapHelper(parent, reader);
		}
	}
}

template<typename K, typename V, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::map<K, V>& v, const YAML::YamlNodeReader& reader, LoadFuncEditable, LoadFuncTag... rest)
{
	if (reader)
	{
		showInfo(parent, reader, AddTag, RemoveTag);

		if (isMapHelper(reader))
		{
			v.clear();
			for (const auto& n : reader.children())
			{
				auto key = n.readKey<K>();

				loadHelper(parent, v[key], n, rest.funcTagForNew()...);
			}
		}
		else if (isMapAddTagHelper(reader))
		{
			for (const auto& n : reader.children())
			{
				auto key = n.readKey<K>();

				loadHelper(parent, v[key], n, rest...);
			}
		}
		else if (isListRemoveTagHelper(reader)) // we use a list here as we only need the keys
		{
			for (const auto& n : reader.children())
			{
				v.erase(n.readVal<K>());
			}
		}
		else
		{
			throwOnBadMapHelper(parent, reader);
		}
	}
}

/**
 * Fixed order map, rely on fact that yaml-cpp try preserve map order from loaded file
 */
template<typename K, typename V, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::vector<std::pair<K, V>>& v, const YAML::YamlNodeReader& reader, LoadFuncEditable, LoadFuncTag... rest)
{
	if (reader)
	{
		showInfo(parent, reader, AddTag, RemoveTag);

		auto pushBack = [&](const K& k) -> V&
		{
			return v.emplace_back(std::pair<K, V>{ k, V{} }).second;
		};

		auto findOrPushBack = [&](const K& k) -> V&
		{
			for (auto& p : v)
			{
				if (p.first == k)
				{
					return p.second;
				}
			}
			return pushBack(k);
		};

		if (isMapHelper(reader))
		{
			v.clear();
			for (const auto& n : reader.children())
			{
				auto key = n.readKey<K>();

				loadHelper(parent, pushBack(key), n, rest.funcTagForNew()...);
			}
		}
		else if (isMapAddTagHelper(reader))
		{
			for (const auto& n : reader.children())
			{
				auto key = n.readKey<K>();

				loadHelper(parent, findOrPushBack(key), n, rest...);
			}
		}
		else if (isListRemoveTagHelper(reader)) // we use a list here as we only need the keys
		{
			for (const auto& n : reader.children())
			{
				auto key = n.readVal<K>();
				Collections::removeIf(v, [&](auto& p){ return p.first == key; });
			}
		}
		else
		{
			throwOnBadMapHelper(parent, reader);
		}
	}
}



const std::string YamlRuleNodeDelete = "delete";
const std::string YamlRuleNodeNew = "new";
const std::string YamlRuleNodeOverride = "override";
const std::string YamlRuleNodeUpdate = "update";
const std::string YamlRuleNodeIgnore = "ignore";


void loadRuleInfoHelper(const YAML::YamlNodeReader& reader, const char* nodeName, const char* type)
{
	if (reader.hasValTag(InfoTag))
	{
		Logger info;
		info.get() << "Main node names available for '" << nodeName << ":' at line " << reader.getLocationInFile().line << " are: ";
		info.get() << " '" << YamlRuleNodeDelete << ":',";
		info.get() << " '" << YamlRuleNodeNew << ":',";
		info.get() << " '" << YamlRuleNodeOverride << ":',";
		info.get() << " '" << YamlRuleNodeUpdate << ":',";
		info.get() << " '" << YamlRuleNodeIgnore << ":',";
		info.get() << " '" << type << ":'";
	}
}

} // namespace

/**
 * Get offset and index for sound set or sprite set.
 * @param parent Name of parent node, used for better error message
 * @param offset Member to load new value.
 * @param node Node with data
 * @param shared Max offset limit that is shared for every mod
 * @param multiplier Value used by `projectile` surface set to convert projectile offset to index offset in surface.
 * @param sizeScale Value used by transparency colors, reduce total number of available space for offset.
 */
void Mod::loadOffsetNode(const std::string &parent, int& offset, const YAML::YamlNodeReader& reader, int shared, const std::string &set, size_t multiplier, size_t sizeScale) const
{
	assert(_modCurrent);
	const ModData* curr = _modCurrent;
	if (reader.hasVal())
	{
		offset = reader.readVal<int>();
	}
	else if (isMapHelper(reader))
	{
		offset = reader["index"].readVal<int>();
		std::string mod = reader["mod"].readVal<std::string>();
		if (mod == ModNameMaster)
		{
			curr = &_modData.at(0);
		}
		else if (mod == ModNameCurrent)
		{
			//nothing
		}
		else
		{
			const ModData* n = 0;
			for (size_t i = 0; i < _modData.size(); ++i)
			{
				const ModData& d = _modData[i];
				if (d.name == mod)
				{
					n = &d;
					break;
				}
			}

			if (n)
			{
				curr = n;
			}
			else
			{
				std::ostringstream err;
				err << "unknown mod '" << mod << "' used";
				throw LoadRuleException(parent, reader, err.str());
			}
		}
	}
	else
	{
		throw LoadRuleException(parent, reader, "unsupported yaml node");
	}

	static_assert(Mod::NO_SOUND == -1, "NO_SOUND need to equal -1");
	static_assert(Mod::NO_SURFACE == -1, "NO_SURFACE need to equal -1");

	if (offset < -1)
	{
		std::ostringstream err;
		err << "offset '" << offset << "' has incorrect value in set '" << set << "'";
		throw LoadRuleException(parent, reader, err.str());
	}
	else if (offset == -1)
	{
		//ok
	}
	else
	{
		int f = offset;
		f *= multiplier;
		if ((size_t)f > curr->size / sizeScale)
		{
			std::ostringstream err;
			err << "offset '" << offset << "' exceeds mod size limit " << (curr->size / multiplier / sizeScale) << " in set '" << set << "'";
			throw LoadRuleException(parent, reader, err.str());
		}
		if (f >= shared)
			f += curr->offset / sizeScale;
		offset = f;
	}
}

/**
 * Returns the appropriate mod-based offset for a sprite.
 * If the ID is bigger than the surfaceset contents, the mod offset is applied.
 * @param parent Name of parent node, used for better error message
 * @param sprite Member to load new sprite ID index.
 * @param node Node with data
 * @param set Name of the surfaceset to lookup.
 * @param multiplier Value used by `projectile` surface set to convert projectile offset to index offset in surface.
 */
void Mod::loadSpriteOffset(const std::string &parent, int& sprite, const YAML::YamlNodeReader& reader, const std::string &set, size_t multiplier) const
{
	if (reader)
	{
		loadOffsetNode(parent, sprite, reader, getRule(set, "Sprite Set", _sets, true)->getMaxSharedFrames(), set, multiplier);
	}
}

/**
 * Gets the mod offset array for a certain sprite.
 * @param parent Name of parent node, used for better error message
 * @param sprites Member to load new array of sprite ID index.
 * @param node Node with data
 * @param set Name of the surfaceset to lookup.
 */
void Mod::loadSpriteOffset(const std::string &parent, std::vector<int>& sprites, const YAML::YamlNodeReader& reader, const std::string &set) const
{
	if (reader)
	{
		int maxShared = getRule(set, "Sprite Set", _sets, true)->getMaxSharedFrames();
		sprites.clear();
		if (isListHelper(reader))
		{
			for (const auto& spriteReader : reader.children())
			{
				sprites.push_back(Mod::NO_SURFACE);
				loadOffsetNode(parent, sprites.back(), spriteReader, maxShared, set, 1);
				if (checkForSoftError(sprites.back() == Mod::NO_SURFACE, parent, spriteReader, "incorrect value in sprite list"))
				{
					sprites.pop_back();
				}
			}
		}
		else
		{
			sprites.push_back(Mod::NO_SURFACE);
			loadOffsetNode(parent, sprites.back(), reader, maxShared, set, 1);
		}
	}
}

/**
 * Returns the appropriate mod-based offset for a sound.
 * If the ID is bigger than the soundset contents, the mod offset is applied.
 * @param parent Name of parent node, used for better error message
 * @param sound Member to load new sound ID index.
 * @param node Node with data
 * @param set Name of the soundset to lookup.
 */
void Mod::loadSoundOffset(const std::string &parent, int& sound, const YAML::YamlNodeReader& reader, const std::string &set) const
{
	if (reader)
	{
		loadOffsetNode(parent, sound, reader, getSoundSet(set)->getMaxSharedSounds(), set, 1);
	}
}

/**
 * Gets the mod offset array for a certain sound.
 * @param parent Name of parent node, used for better error message
 * @param sounds Member to load new list of sound ID indexes.
 * @param node Node with data
 * @param set Name of the soundset to lookup.
 */
void Mod::loadSoundOffset(const std::string &parent, std::vector<int>& sounds, const YAML::YamlNodeReader& reader, const std::string &set) const
{
	if (reader)
	{
		int maxShared = getSoundSet(set)->getMaxSharedSounds();
		sounds.clear();
		if (isListHelper(reader))
		{
			for (const auto& soundReader : reader.children())
			{
				sounds.push_back(Mod::NO_SOUND);
				loadOffsetNode(parent, sounds.back(), soundReader, maxShared, set, 1);
				if (checkForSoftError(sounds.back() == Mod::NO_SOUND, parent, soundReader, "incorrect value in sound list"))
				{
					sounds.pop_back();
				}
			}
		}
		else
		{
			sounds.push_back(Mod::NO_SOUND);
			loadOffsetNode(parent, sounds.back(), reader, maxShared, set, 1);
		}
	}
}

/**
 * Gets the mod offset array for a certain transparency index.
 * @param parent Name of parent node, used for better error message.
 * @param index Member to load new transparency index.
 * @param node Node with data.
 */
void Mod::loadTransparencyOffset(const std::string &parent, int& index, const YAML::YamlNodeReader& reader) const
{
	if (reader)
	{
		loadOffsetNode(parent, index, reader, 0, "TransparencyLUTs", 1, ModTransparencySizeReduction);
	}
}

/**
 * Returns the appropriate mod-based offset for a generic ID.
 * If the ID is bigger than the max, the mod offset is applied.
 * @param id Numeric ID.
 * @param max Maximum vanilla value.
 */
int Mod::getOffset(int id, int max) const
{
	assert(_modCurrent);
	if (id > max)
		return id + _modCurrent->offset;
	else
		return id;
}

/**
 * Load base functions to bit set.
 */
void Mod::loadBaseFunction(const std::string& parent, RuleBaseFacilityFunctions& f, const YAML::YamlNodeReader& reader)
{
	if (reader)
	{
		try
		{
			if (isListHelper(reader))
			{
				f.reset();
				for (const auto& n : reader.children())
				{
					f.set(_baseFunctionNames.addName(n.readVal<std::string>(), f.size()));
				}
			}
			else if (isListAddTagHelper(reader))
			{
				for (const auto& n : reader.children())
				{
					f.set(_baseFunctionNames.addName(n.readVal<std::string>(), f.size()));
				}
			}
			else if (isListRemoveTagHelper(reader))
			{
				for (const auto& n : reader.children())
				{
					f.set(_baseFunctionNames.addName(n.readVal<std::string>(), f.size()), false);
				}
			}
			else
			{
				throwOnBadListHelper(parent, reader);
			}
		}
		catch(LoadRuleException& ex)
		{
			//context is already included in exception, no need add more
			throw;
		}
		catch(Exception& ex)
		{
			throw LoadRuleException(parent, reader, ex.what());
		}
	}
}

/**
 * Get names of function names in given bitset.
 */
std::vector<std::string> Mod::getBaseFunctionNames(RuleBaseFacilityFunctions f) const
{
	std::vector<std::string> vec;
	vec.reserve(f.count());
	for (size_t i = 0; i < f.size(); ++i)
	{
		if (f.test(i))
		{
			vec.push_back(_baseFunctionNames.getName(i));
		}
	}
	return vec;
}

/**
 * Loads a list of ints.
 * Another mod can only override the whole list, no partial edits allowed.
 */
void Mod::loadInts(const std::string &parent, std::vector<int>& ints, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, ints, reader, LoadFuncStandard{});
}

/**
 * Loads a list of ints where order of items does not matter.
 * Another mod can remove or add new values without altering the whole list.
 */
void Mod::loadUnorderedInts(const std::string &parent, std::vector<int>& ints, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, ints, reader, LoadFuncEditable{});
}


/**
 * Loads a name.
 */
void Mod::loadName(const std::string &parent, std::string& name, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, name, reader, LoadFuncStandard{});
}

/**
 * Loads a name. Have option of loading null `~` as special string value.
 */
void Mod::loadNameNull(const std::string &parent, std::string& name, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, name, reader, LoadFuncNullable{});
}

/**
 * Loads a list of names.
 * Another mod can only override the whole list, no partial edits allowed.
 */
void Mod::loadNames(const std::string &parent, std::vector<std::string>& names, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, names, reader, LoadFuncStandard{});
}

/**
 * Loads a list of names where order of items does not matter.
 * Another mod can remove or add new values without altering the whole list.
 */
void Mod::loadUnorderedNames(const std::string &parent, std::vector<std::string>& names, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, names, reader, LoadFuncEditable{});
}



/**
 * Loads a map from names to names.
 */
void Mod::loadNamesToNames(const std::string &parent, std::vector<std::pair<std::string, std::vector<std::string>>>& names, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, names, reader, LoadFuncEditable{}, LoadFuncEditable{});
}

/**
 * Loads a map from names to names.
 */
void Mod::loadUnorderedNamesToNames(const std::string &parent, std::map<std::string, std::string>& names, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, names, reader, LoadFuncEditable{});
}

/**
 * Loads a map from names to ints.
 */
void Mod::loadUnorderedNamesToInt(const std::string &parent, std::map<std::string, int>& names, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, names, reader, LoadFuncEditable{});
}

/**
 * Loads a map from names to vector of ints.
 */
void Mod::loadUnorderedNamesToInts(const std::string &parent, std::map<std::string, std::vector<int>>& names, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, names, reader, LoadFuncEditable{}, LoadFuncStandard{});
}

/**
 * Loads a map from names to names to int.
 */
void Mod::loadUnorderedNamesToNamesToInt(const std::string &parent, std::map<std::string, std::map<std::string, int>>& names, const YAML::YamlNodeReader& reader) const
{
	loadHelper(parent, names, reader, LoadFuncEditable{}, LoadFuncEditable{});
}

/**
 * Loads data for kill criteria from Commendations.
 */
void Mod::loadKillCriteria(const std::string &parent, std::vector<std::vector<std::pair<int, std::vector<std::string> > > >& v, const YAML::YamlNodeReader& reader) const
{
	//TODO: very specific use case, not all levels fully supported
	if (reader)
	{
		auto loadInner = [&](std::vector<std::pair<int, std::vector<std::string>>>& vv, const YAML::YamlNodeReader& n)
		{
			showInfo(parent, n);

			if (isListHelper(n))
			{
				vv = n.readVal<std::vector<std::pair<int, std::vector<std::string>>>>();
			}
			else
			{
				throwOnBadListHelper(parent, n);
			}
		};

		showInfo(parent, reader, AddTag);

		if (isListHelper(reader))
		{
			v.clear();
			v.reserve(reader.childrenCount());
			for (const auto& n : reader.children())
			{
				loadInner(v.emplace_back(), n);
			}
		}
		else if (isListAddTagHelper(reader))
		{
			v.reserve(v.size() + reader.childrenCount());
			for (const auto& n : reader.children())
			{
				loadInner(v.emplace_back(), n);
			}
		}
		else
		{
			throwOnBadListHelper(parent, reader);
		}
	}
}




template<typename T>
static void afterLoadHelper(const char* name, Mod* mod, std::map<std::string, T*>& list, void (T::* func)(const Mod*))
{
	std::ostringstream errorStream;
	int errorLimit = 30;
	int errorCount = 0;

	errorStream << "During linking rulesets of " << name << ":\n";
	for (auto& rule : list)
	{
		try
		{
			(rule.second->* func)(mod);
		}
		catch (LoadRuleException &e)
		{
			++errorCount;
			errorStream << e.what() << "\n";
			if (errorCount == errorLimit)
			{
				break;
			}
		}
		catch (Exception &e)
		{
			++errorCount;
			errorStream << "Error processing '" << rule.first << "' in " << name << ": " << e.what() << "\n";
			if (errorCount == errorLimit)
			{
				break;
			}
		}
	}
	if (errorCount)
	{
		throw Exception(errorStream.str());
	}
}

/**
 * Helper function used to disable invalid mod and throw exception to quit game
 * @param modId Mod id
 * @param error Error message
 */
static void throwModOnErrorHelper(const std::string& modId, const std::string& error)
{
	std::ostringstream errorStream;

	errorStream << "failed to load '"
		<< Options::getModInfos().at(modId).getName()
		<< "'";

	if (!Options::debug)
	{
		Log(LOG_WARNING) << "disabling mod with invalid ruleset: " << modId;
		auto it = std::find(Options::mods.begin(), Options::mods.end(), std::pair<std::string, bool>(modId, true));
		if (it == Options::mods.end())
		{
			Log(LOG_ERROR) << "cannot find broken mod in mods list: " << modId;
			Log(LOG_ERROR) << "clearing mods list";
			Options::mods.clear();
		}
		else
		{
			it->second = false;
		}
		Options::save();

		errorStream << "; mod disabled";
	}
	errorStream << std::endl << error;

	throw Exception(errorStream.str());
}

/**
 * Loads a list of mods specified in the options.
 * List of <modId, rulesetFiles> pairs is fetched from the FileMap / VFS
 * being set up in options updateMods
 */
void Mod::loadAll()
{
	ModScript parser{ _scriptGlobal, this };
	const auto& mods = FileMap::getRulesets();

	Log(LOG_INFO) << "Loading begins...";
	if (Options::oxceModValidationLevel < LOG_ERROR)
	{
		Log(LOG_ERROR) << "Validation of mod data disabled, game can crash when run";
	}
	else if (Options::oxceModValidationLevel < LOG_WARNING)
	{
		Log(LOG_WARNING) << "Validation of mod data reduced, game can behave incorrectly";
	}
	_scriptGlobal->beginLoad();
	_modData.clear();
	_modData.resize(mods.size());

	std::set<std::string> usedModNames;
	usedModNames.insert(ModNameMaster);
	usedModNames.insert(ModNameCurrent);


	// calculated offsets and other things for all mods
	size_t offset = 0;
	for (size_t i = 0; mods.size() > i; ++i)
	{
		const std::string& modId = mods[i].first;
		if (usedModNames.insert(modId).second == false)
		{
			throwModOnErrorHelper(modId, "this mod name is already used");
		}
		_scriptGlobal->addMod(mods[i].first, 1000 * (int)offset);
		const ModInfo *modInfo = &Options::getModInfos().at(modId);
		size_t size = modInfo->getReservedSpace();
		_modData[i].name = modId;
		_modData[i].offset = 1000 * offset;
		_modData[i].info = modInfo;
		_modData[i].size = 1000 * size;
		offset += size;
	}

	Log(LOG_INFO) << "Pre-loading rulesets...";
	// load rulesets that can affect loading vanilla resources
	for (size_t i = 0; _modData.size() > i; ++i)
	{
		_modCurrent = &_modData.at(i);
		//if (_modCurrent->info->isMaster())
		{
			auto* file = FileMap::getModRuleFile(_modCurrent->info, _modCurrent->info->getResourceConfigFile());
			if (file)
			{
				loadResourceConfigFile(*file);
			}
		}
	}

	Log(LOG_INFO) << "Loading vanilla resources...";
	// vanilla resources load
	_modCurrent = &_modData.at(0);
	loadVanillaResources();
	_surfaceOffsetBasebits = _sets["BASEBITS.PCK"]->getMaxSharedFrames();
	_surfaceOffsetBigobs = _sets["BIGOBS.PCK"]->getMaxSharedFrames();
	_surfaceOffsetFloorob = _sets["FLOOROB.PCK"]->getMaxSharedFrames();
	_surfaceOffsetHandob = _sets["HANDOB.PCK"]->getMaxSharedFrames();
	_surfaceOffsetHit = _sets["HIT.PCK"]->getMaxSharedFrames();
	_surfaceOffsetSmoke = _sets["SMOKE.PCK"]->getMaxSharedFrames();

	_soundOffsetBattle = _sounds["BATTLE.CAT"]->getMaxSharedSounds();
	_soundOffsetGeo = _sounds["GEO.CAT"]->getMaxSharedSounds();

	Log(LOG_INFO) << "Loading rulesets...";
	// load rest rulesets
	for (size_t i = 0; mods.size() > i; ++i)
	{
		try
		{
			_modCurrent = &_modData.at(i);
			_scriptGlobal->setMod((int)_modCurrent->offset);
			loadMod(mods[i].second, parser);
		}
		catch (Exception &e)
		{
			const std::string &modId = mods[i].first;
			throwModOnErrorHelper(modId, e.what());
		}
	}
	Log(LOG_INFO) << "Loading rulesets done.";

	//back master
	_modCurrent = &_modData.at(0);
	_scriptGlobal->endLoad();

	// post-processing item categories
	std::map<std::string, std::string> replacementRules;
	for (const auto& pair : _itemCategories)
	{
		if (!pair.second->getReplaceBy().empty())
		{
			replacementRules[pair.first] = pair.second->getReplaceBy();
		}
	}
	for (auto& pair : _items)
	{
		pair.second->updateCategories(&replacementRules);
	}

	// find out if paperdoll overlaps with inventory slots
	int x1 = RuleInventory::PAPERDOLL_X;
	int y1 = RuleInventory::PAPERDOLL_Y;
	int w1 = RuleInventory::PAPERDOLL_W;
	int h1 = RuleInventory::PAPERDOLL_H;
	for (const auto& invCategory : _invs)
	{
		for (const auto& invSlot : *invCategory.second->getSlots())
		{
			int x2 = invCategory.second->getX() + (invSlot.x * RuleInventory::SLOT_W);
			int y2 = invCategory.second->getY() + (invSlot.y * RuleInventory::SLOT_H);
			int w2 = RuleInventory::SLOT_W;
			int h2 = RuleInventory::SLOT_H;
			if (x1 + w1 < x2 || x2 + w2 < x1 || y1 + h1 < y2 || y2 + h2 < y1)
			{
				// intersection is empty
			}
			else
			{
				_inventoryOverlapsPaperdoll = true;
			}
		}
	}

	loadExtraResources();


	Log(LOG_INFO) << "After load.";
	// cross link rule objects

	afterLoadHelper("research", this, _research, &RuleResearch::afterLoad);
	afterLoadHelper("items", this, _items, &RuleItem::afterLoad);
	afterLoadHelper("weaponSets", this, _weaponSets, &RuleWeaponSet::afterLoad);
	afterLoadHelper("manufacture", this, _manufacture, &RuleManufacture::afterLoad);
	afterLoadHelper("armors", this, _armors, &Armor::afterLoad);
	afterLoadHelper("units", this, _units, &Unit::afterLoad);
	afterLoadHelper("soldiers", this, _soldiers, &RuleSoldier::afterLoad);
	afterLoadHelper("facilities", this, _facilities, &RuleBaseFacility::afterLoad);
	afterLoadHelper("startingConditions", this, _startingConditions, &RuleStartingCondition::afterLoad);
	afterLoadHelper("enviroEffects", this, _enviroEffects, &RuleEnviroEffects::afterLoad);
	afterLoadHelper("commendations", this, _commendations, &RuleCommendations::afterLoad);
	afterLoadHelper("skills", this, _skills, &RuleSkill::afterLoad);
	afterLoadHelper("craftWeapons", this, _craftWeapons, &RuleCraftWeapon::afterLoad);
	afterLoadHelper("countries", this, _countries, &RuleCountry::afterLoad);
	afterLoadHelper("crafts", this, _crafts, &RuleCraft::afterLoad);

	for (auto& a : _armors)
	{
		if (a.second->hasInfiniteSupply())
		{
			_armorsForSoldiersCache.push_back(a.second);
		}
		else if (a.second->getStoreItem())
		{
			_armorsForSoldiersCache.push_back(a.second);
			_armorStorageItemsCache.push_back(a.second->getStoreItem());
		}
	}
	//_armorsForSoldiersCache sorted in sortList()
	Collections::sortVector(_armorStorageItemsCache);
	Collections::sortVectorMakeUnique(_armorStorageItemsCache);


	for (auto& c : _craftWeapons)
	{
		const RuleItem* item = nullptr;

		item = c.second->getLauncherItem();
		if (item)
		{
			_craftWeaponStorageItemsCache.push_back(item);
		}

		item = c.second->getClipItem();
		if (item)
		{
			_craftWeaponStorageItemsCache.push_back(item);
		}
	}
	Collections::sortVector(_craftWeaponStorageItemsCache);
	Collections::sortVectorMakeUnique(_craftWeaponStorageItemsCache);


	for (auto& r : _research)
	{
		if (r.second->unlockFinalMission())
		{
			if (_finalResearch != nullptr)
			{
				checkForSoftError(true, "mod", "Both '" + _finalResearch->getName() + "' and '" + r.second->getName() + "' research are marked as 'unlockFinalMission: true'", LOG_INFO);

				// to make old mods semi-compatible with new code we decide that last updated rule will be consider final research. This could make false-positive as last update could not touch this flag.
				if (getModLastUpdatingRule(r.second)->offset < getModLastUpdatingRule(_finalResearch)->offset)
				{
					continue;
				}
			}
			_finalResearch = r.second;
		}
	}


	// check unique listOrder
	{
		std::vector<int> tmp;
		tmp.reserve(_soldierBonus.size());
		for (auto& i : _soldierBonus)
		{
			tmp.push_back(i.second->getListOrder());
		}
		std::sort(tmp.begin(), tmp.end());
		auto it = std::unique(tmp.begin(), tmp.end());
		bool wasUnique = (it == tmp.end());
		if (!wasUnique)
		{
			throw Exception("List order for soldier bonus types must be unique!");
		}
	}

	// auto-create alternative manufacture rules
	for (auto& shortcutPair : _manufactureShortcut)
	{
		// 1. check if the new project has a unique name
		auto& typeNew = shortcutPair.first;
		auto it = _manufacture.find(typeNew);
		if (it != _manufacture.end())
		{
			throw Exception("Manufacture project '" + typeNew + "' already exists! Choose a different name for this alternative project.");
		}

		// 2. copy an existing manufacture project
		const RuleManufacture* ruleStartFrom = getManufacture(shortcutPair.second->getStartFrom(), true);
		RuleManufacture* ruleNew = new RuleManufacture(*ruleStartFrom);
		_manufacture[typeNew] = ruleNew;
		_manufactureIndex.push_back(typeNew);

		// 3. change the name and break down the sub-projects into simpler components
		if (ruleNew != 0)
		{
			ruleNew->breakDown(this, shortcutPair.second);
		}
	}

	// recommended user options
	if (!_recommendedUserOptions.empty() && !Options::oxceRecommendedOptionsWereSet)
	{
		_recommendedUserOptions.erase("maximizeInfoScreens"); // FIXME: make proper categorisations in the next release
		_recommendedUserOptions.erase("oxceModValidationLevel");

		for (auto& optionInfo : Options::getOptionInfo())
		{
			if (optionInfo.type() != OPTION_KEY && !optionInfo.category().empty())
			{
				optionInfo.load(_recommendedUserOptions, false);
			}
		}

		Options::oxceRecommendedOptionsWereSet = true;
		Options::save();
	}

	// fixed user options
	if (!_fixedUserOptions.empty())
	{
		_fixedUserOptions.erase("oxceLinks");
		_fixedUserOptions.erase("oxceUpdateCheck");
		_fixedUserOptions.erase("maximizeInfoScreens"); // FIXME: make proper categorisations in the next release
		_fixedUserOptions.erase("oxceModValidationLevel");
		_fixedUserOptions.erase("oxceAutoNightVisionThreshold");
		_fixedUserOptions.erase("oxceAlternateCraftEquipmentManagement");

		for (auto& optionInfo : Options::getOptionInfo())
		{
			if (optionInfo.type() != OPTION_KEY && !optionInfo.category().empty())
			{
				optionInfo.load(_fixedUserOptions, false);
			}
		}
		Options::save();
	}

	// additional validation of options not visible in the GUI
	{
		if (Options::oxceMaxEquipmentLayoutTemplates < 10 ||
			Options::oxceMaxEquipmentLayoutTemplates > SavedGame::MAX_EQUIPMENT_LAYOUT_TEMPLATES ||
			Options::oxceMaxEquipmentLayoutTemplates % 10 != 0)
		{
			Options::oxceMaxEquipmentLayoutTemplates = 20;
		}
	}

	Log(LOG_INFO) << "Loading ended.";

	sortLists();
	modResources();
}

/**
 * Loads a list of rulesets from YAML files for the mod at the specified index. The first
 * mod loaded should be the master at index 0, then 1, and so on.
 * @param rulesetFiles List of rulesets to load.
 * @param parsers Object with all available parsers.
 */
void Mod::loadMod(const std::vector<FileMap::FileRecord> &rulesetFiles, ModScript &parsers)
{
	for (const auto& filerec : rulesetFiles)
	{
		Log(LOG_VERBOSE) << "- " << filerec.fullpath;
		try
		{
			_scriptGlobal->fileLoad(filerec.fullpath);
			loadFile(filerec, parsers);
		}
		catch (Exception &e)
		{
			throw Exception(filerec.fullpath + ": " + std::string(e.what()));
		}
		catch (YAML::Exception &e)
		{
			throw Exception(filerec.fullpath + ": " + std::string(e.what()));
		}
	}

	// these need to be validated, otherwise we're gonna get into some serious trouble down the line.
	// it may seem like a somewhat arbitrary limitation, but there is a good reason behind it.
	// i'd need to know what results are going to be before they are formulated, and there's a hierarchical structure to
	// the order in which variables are determined for a mission, and the order is DIFFERENT for regular missions vs
	// missions that spawn a mission site. where normally we pick a region, then a mission based on the weights for that region.
	// a terror-type mission picks a mission type FIRST, then a region based on the criteria defined by the mission.
	// there is no way i can conceive of to reconcile this difference to allow mixing and matching,
	// short of knowing the results of calls to the RNG before they're determined.
	// the best solution i can come up with is to disallow it, as there are other ways to achieve what this would amount to anyway,
	// and they don't require time travel. - Warboy
	for (auto& pair : _missionScripts)
	{
		RuleMissionScript *rule = pair.second;
		std::set<std::string> missions = rule->getAllMissionTypes();
		if (!missions.empty())
		{
			auto j = missions.begin();
			if (!getAlienMission(*j))
			{
				throw Exception("Error with MissionScript: " + pair.first + ": alien mission type: " + *j + " not defined, do not incite the judgement of Amaunator.");
			}
			bool isSiteType = getAlienMission(*j)->getObjective() == OBJECTIVE_SITE;
			rule->setSiteType(isSiteType);
			for (;j != missions.end(); ++j)
			{
				if (getAlienMission(*j) && (getAlienMission(*j)->getObjective() == OBJECTIVE_SITE) != isSiteType)
				{
					throw Exception("Error with MissionScript: " + pair.first + ": cannot mix terror/non-terror missions in a single command, so sayeth the wise Alaundo.");
				}
			}
		}
	}

	// instead of passing a pointer to the region load function and moving the alienMission loading before region loading
	// and sanitizing there, i'll sanitize here, i'm sure this sanitation will grow, and will need to be refactored into
	// its own function at some point, but for now, i'll put it here next to the missionScript sanitation, because it seems
	// the logical place for it, given that this sanitation is required as a result of moving all terror mission handling
	// into missionScripting behaviour. apologies to all the modders that will be getting errors and need to adjust their
	// rulesets, but this will save you weird errors down the line.
	for (auto& pair : _regions)
	{
		// bleh, make copies, const correctness kinda screwed me here.
		WeightedOptions weights = pair.second->getAvailableMissions();
		std::vector<std::string> names = weights.getNames();
		for (const auto& name : names)
		{
			if (!getAlienMission(name))
			{
				throw Exception("Error with MissionWeights: Region: " + pair.first + ": alien mission type: " + name + " not defined, do not incite the judgement of Amaunator.");
			}
			if (getAlienMission(name)->getObjective() == OBJECTIVE_SITE)
			{
				throw Exception("Error with MissionWeights: Region: " + pair.first + " has " + name + " listed. Terror mission can only be invoked via missionScript, so sayeth the Spider Queen.");
			}
		}
	}
}

/**
 * Loads a ruleset from a YAML file that have basic resources configuration.
 * @param filename YAML filename.
 */
void Mod::loadResourceConfigFile(const FileMap::FileRecord &filerec)
{
	YAML::YamlRootNodeReader reader = filerec.getYAML();

	for (const auto& soundDefReader : reader["soundDefs"].children())
	{
		SoundDefinition *rule = loadRule(soundDefReader, &_soundDefs);
		if (rule != 0)
		{
			rule->load(soundDefReader);
		}
	}

	if (const auto& luts = reader["transparencyLUTs"])
	{
		const size_t start = _modCurrent->offset / ModTransparencySizeReduction;
		const size_t limit =  _modCurrent->size / ModTransparencySizeReduction;
		size_t curr = 0;

		_transparencies.resize(start + limit);
		for (const auto& lut : luts.children())
		{
			const auto& colors = lut["colors"];
			if (colors.isSeq())
			{
				for (const auto& colorReader : colors.children())
				{
					if (curr == limit)
					{
						throw Exception("transparencyLUTs mod limit reach");
					}

					auto loadByteValue = [&](const YAML::YamlNodeReader& n)
					{
						int v = n.readVal<int>(-1);
						checkForSoftError(v < 0 || v > 255, "transparencyLUTs", n, "value outside allowed range");
						return Clamp(v, 0, 255);
					};

					if (colorReader[0].hasVal())
					{
						SDL_Color color;
						color.r = loadByteValue(colorReader[0]);
						color.g = loadByteValue(colorReader[1]);
						color.b = loadByteValue(colorReader[2]);
						color.unused = colorReader[3] ? loadByteValue(colorReader[3]): 2;

						for (int opacity = 0; opacity < TransparenciesOpacityLevels; ++opacity)
						{
							// pseudo interpolation of palette color with tint
							// for small values `op` its should behave same as original TFTD
							// but for bigger values it make result closer to tint color
							const int op = Clamp((opacity+1) * color.unused, 0, 64);
							const float co = 1.0f - Sqr(op / 64.0f); // 1.0 -> 0.0
							const float to = op * 1.0f; // 0.0 -> 64.0

							SDL_Color taint;
							taint.r = Clamp((int)(color.r * to), 0, 255);
							taint.g = Clamp((int)(color.g * to), 0, 255);
							taint.b = Clamp((int)(color.b * to), 0, 255);
							taint.unused = 255 * co;
							_transparencies[start + curr][opacity] = taint;
						};
					}
					else
					{
						for (int opacity = 0; opacity < TransparenciesOpacityLevels; ++opacity)
						{
							const auto& n = colorReader[opacity];

							SDL_Color taint;
							taint.r = loadByteValue(n[0]);
							taint.g = loadByteValue(n[1]);
							taint.b = loadByteValue(n[2]);
							taint.unused = 255 - loadByteValue(n[3]);
							_transparencies[start + curr][opacity] = taint;
						};
						std::reverse(std::begin(_transparencies[start + curr]), std::end(_transparencies[start + curr]));
					}
					curr++;
				}
			}
			else
			{
				throw Exception("unknown transparencyLUTs node type");
			}
		}
	}
}

/**
 * Loads "constants" node.
 */
void Mod::loadConstants(const YAML::YamlNodeReader &reader)
{
	loadSoundOffset("constants", DOOR_OPEN, reader["doorSound"], "BATTLE.CAT");
	loadSoundOffset("constants", SLIDING_DOOR_OPEN, reader["slidingDoorSound"], "BATTLE.CAT");
	loadSoundOffset("constants", SLIDING_DOOR_CLOSE, reader["slidingDoorClose"], "BATTLE.CAT");
	loadSoundOffset("constants", SMALL_EXPLOSION, reader["smallExplosion"], "BATTLE.CAT");
	loadSoundOffset("constants", LARGE_EXPLOSION, reader["largeExplosion"], "BATTLE.CAT");

	loadSpriteOffset("constants", EXPLOSION_OFFSET, reader["explosionOffset"], "X1.PCK");
	loadSpriteOffset("constants", SMOKE_OFFSET, reader["smokeOffset"], "SMOKE.PCK");
	loadSpriteOffset("constants", UNDERWATER_SMOKE_OFFSET, reader["underwaterSmokeOffset"], "SMOKE.PCK");

	loadSoundOffset("constants", ITEM_DROP, reader["itemDrop"], "BATTLE.CAT");
	loadSoundOffset("constants", ITEM_THROW, reader["itemThrow"], "BATTLE.CAT");
	loadSoundOffset("constants", ITEM_RELOAD, reader["itemReload"], "BATTLE.CAT");
	loadSoundOffset("constants", WALK_OFFSET, reader["walkOffset"], "BATTLE.CAT");
	loadSoundOffset("constants", FLYING_SOUND, reader["flyingSound"], "BATTLE.CAT");

	loadSoundOffset("constants", BUTTON_PRESS, reader["buttonPress"], "GEO.CAT");
	if (const auto& arrayReader = reader["windowPopup"])
		for (size_t j = 0; j < std::size(WINDOW_POPUP); j++)
			loadSoundOffset("constants", WINDOW_POPUP[j], arrayReader[j], "GEO.CAT");

	loadSoundOffset("constants", UFO_FIRE, reader["ufoFire"], "GEO.CAT");
	loadSoundOffset("constants", UFO_HIT, reader["ufoHit"], "GEO.CAT");
	loadSoundOffset("constants", UFO_CRASH, reader["ufoCrash"], "GEO.CAT");
	loadSoundOffset("constants", UFO_EXPLODE, reader["ufoExplode"], "GEO.CAT");
	loadSoundOffset("constants", INTERCEPTOR_HIT, reader["interceptorHit"], "GEO.CAT");
	loadSoundOffset("constants", INTERCEPTOR_EXPLODE, reader["interceptorExplode"], "GEO.CAT");
	reader.tryRead("geoscapeCursor", GEOSCAPE_CURSOR);
	reader.tryRead("basescapeCursor", BASESCAPE_CURSOR);
	reader.tryRead("battlescapeCursor", BATTLESCAPE_CURSOR);
	reader.tryRead("ufopaediaCursor", UFOPAEDIA_CURSOR);
	reader.tryRead("graphsCursor", GRAPHS_CURSOR);
	reader.tryRead("damageRange", DAMAGE_RANGE);
	reader.tryRead("explosiveDamageRange", EXPLOSIVE_DAMAGE_RANGE);
	if (const auto& arrayReader = reader["fireDamageRange"])
		for (size_t j = 0; j < std::size(FIRE_DAMAGE_RANGE); j++)
			arrayReader[j].tryReadVal(FIRE_DAMAGE_RANGE[j]);
	reader.tryRead("goodDebriefingMusic", DEBRIEF_MUSIC_GOOD);
	reader.tryRead("badDebriefingMusic", DEBRIEF_MUSIC_BAD);
	if (const auto& arrayReader = reader["extendedPediaFacilityParams"])
		for (size_t j = 0; j < std::size(PEDIA_FACILITY_RENDER_PARAMETERS); j++)
			arrayReader[j].tryReadVal(PEDIA_FACILITY_RENDER_PARAMETERS[j]);
	reader.tryRead("extendedItemReloadCost", EXTENDED_ITEM_RELOAD_COST);
	reader.tryRead("extendedInventorySlotSorting", EXTENDED_INVENTORY_SLOT_SORTING);
	reader.tryRead("extendedRunningCost", EXTENDED_RUNNING_COST);
	reader.tryRead("extendedMovementCostRounding", EXTENDED_MOVEMENT_COST_ROUNDING);
	reader.tryRead("extendedHwpLoadOrder", EXTENDED_HWP_LOAD_ORDER);
	reader.tryRead("extendedMeleeReactions", EXTENDED_MELEE_REACTIONS);
	reader.tryRead("extendedTerrainMelee", EXTENDED_TERRAIN_MELEE);
	reader.tryRead("extendedUnderwaterThrowFactor", EXTENDED_UNDERWATER_THROW_FACTOR);
	reader.tryRead("extendedExperienceAwardSystem", EXTENDED_EXPERIENCE_AWARD_SYSTEM);

	reader.tryRead("extendedCurrencySymbol", OXCE_CURRENCY_SYMBOL);
}

/**
 * Loads a ruleset's contents from a YAML file.
 * Rules that match pre-existing rules overwrite them.
 * @param filename YAML filename.
 * @param parsers Object with all available parsers.
 */
void Mod::loadFile(const FileMap::FileRecord &filerec, ModScript &parsers)
{
	YAML::YamlRootNodeReader r = filerec.getYAML();
	YAML::YamlNodeReader reader = r.useIndex();

	auto loadDocInfoHelper = [&](const char* nodeName)
	{
		if (reader.hasValTag(InfoTag))
		{
			Logger info;
			info.get() << "Available rule '" << nodeName << ":'";
		}
		return reader[nodeName];
	};

	if (const auto& extended = loadDocInfoHelper("extended"))
	{
		if (const auto& t = extended["tagsFile"])
		{
			auto filePath = t.readVal<std::string>();
			auto file = FileMap::getModRuleFile(_modCurrent->info, filePath);

			if (false == checkForSoftError(file == nullptr, "extended", t, "Unknown file name for 'tagsFile': '" + filePath + "'", LOG_ERROR))
			{
				//copy only tags and load them in current file.
				const YAML::YamlRootNodeReader& tempReader = file->getYAML();
				std::map<std::string, std::map<std::string, std::string> > tempTags;
				tempReader["extended"].tryRead("tags", tempTags);
				YAML::YamlRootNodeWriter tempExtended;
				tempExtended.setAsMap();
				tempExtended.write("tags", tempTags);
				_scriptGlobal->load(tempExtended.toReader());
			}
		}

		_scriptGlobal->load(extended);
		_scriptGlobal->getScriptValues().load(extended, parsers.getShared(), "globals");
	}

	auto iterateRules = [&](const char* nodeName, const char* type)
	{
		const auto& node = loadDocInfoHelper(nodeName);

		loadRuleInfoHelper(node, nodeName, type);

		return node.children();
	};

	auto iterateRulesSpecific = [&](const char* nodeName)
	{
		const auto& node = loadDocInfoHelper(nodeName);

		return node.children();
	};



	for (const auto& ruleReader : iterateRules("countries", "type"))
	{
		RuleCountry* rule = loadRule(ruleReader, &_countries, &_countriesIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, parsers, this);
		}
	}
	for (const auto& ruleReader : iterateRules("extraGlobeLabels", "type"))
	{
		RuleCountry *rule = loadRule(ruleReader, &_extraGlobeLabels, &_extraGlobeLabelsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, parsers, this);
		}
	}
	for (const auto& ruleReader : iterateRules("regions", "type"))
	{
		RuleRegion *rule = loadRule(ruleReader, &_regions, &_regionsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("facilities", "type"))
	{
		RuleBaseFacility *rule = loadRule(ruleReader, &_facilities, &_facilitiesIndex, "type", RuleListOrderedFactory<RuleBaseFacility>{ _facilityListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("crafts", "type"))
	{
		RuleCraft *rule = loadRule(ruleReader, &_crafts, &_craftsIndex, "type", RuleListOrderedFactory<RuleCraft>{ _craftListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("craftWeapons", "type"))
	{
		RuleCraftWeapon *rule = loadRule(ruleReader, &_craftWeapons, &_craftWeaponsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("itemCategories", "type"))
	{
		RuleItemCategory *rule = loadRule(ruleReader, &_itemCategories, &_itemCategoriesIndex, "type", RuleListOrderedFactory<RuleItemCategory>{ _itemCategoryListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("items", "type"))
	{
		RuleItem *rule = loadRule(ruleReader, &_items, &_itemsIndex, "type", RuleListOrderedFactory<RuleItem>{ _itemListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("weaponSets", "type"))
	{
		RuleWeaponSet* rule = loadRule(ruleReader, &_weaponSets);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("ufos", "type"))
	{
		RuleUfo *rule = loadRule(ruleReader, &_ufos, &_ufosIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("invs", "id"))
	{
		RuleInventory *rule = loadRule(ruleReader, &_invs, &_invsIndex, "id", RuleListOrderedFactory<RuleInventory>{ _invListOrder, 10 });
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRules("terrains", "name"))
	{
		RuleTerrain *rule = loadRule(ruleReader, &_terrains, &_terrainIndex, "name");
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}

	for (const auto& ruleReader : iterateRules("armors", "type"))
	{
		Armor *rule = loadRule(ruleReader, &_armors, &_armorsIndex, "type", RuleListOrderedFactory<Armor>{ _armorListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("skills", "type"))
	{
		RuleSkill *rule = loadRule(ruleReader, &_skills, &_skillsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("soldiers", "type"))
	{
		RuleSoldier *rule = loadRule(ruleReader, &_soldiers, &_soldiersIndex, "type", RuleListOrderedFactory<RuleSoldier>{ _soldierListOrder, 1 });
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("units", "type"))
	{
		Unit *rule = loadRule(ruleReader, &_units);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("alienRaces", "id"))
	{
		AlienRace *rule = loadRule(ruleReader, &_alienRaces, &_aliensIndex, "id", RuleListOrderedFactory<AlienRace>{ _alienRaceListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("enviroEffects", "type"))
	{
		RuleEnviroEffects* rule = loadRule(ruleReader, &_enviroEffects, &_enviroEffectsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("startingConditions", "type"))
	{
		RuleStartingCondition *rule = loadRule(ruleReader, &_startingConditions, &_startingConditionsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("alienDeployments", "type"))
	{
		AlienDeployment *rule = loadRule(ruleReader, &_alienDeployments, &_deploymentsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("research", "name"))
	{
		RuleResearch *rule = loadRule(ruleReader, &_research, &_researchIndex, "name", RuleListOrderedFactory<RuleResearch>{ _researchListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("manufacture", "name"))
	{
		RuleManufacture *rule = loadRule(ruleReader, &_manufacture, &_manufactureIndex, "name", RuleListOrderedFactory<RuleManufacture>{ _manufactureListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("manufactureShortcut", "name"))
	{
		RuleManufactureShortcut *rule = loadRule(ruleReader, &_manufactureShortcut, 0, "name");
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRules("soldierBonuses", "name"))
	{
		RuleSoldierBonus *rule = loadRule(ruleReader, &_soldierBonus, &_soldierBonusIndex, "name", RuleListOrderedFactory<RuleSoldierBonus>{ _soldierBonusListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this, parsers);
		}
	}
	for (const auto& ruleReader : iterateRules("soldierTransformation", "name"))
	{
		RuleSoldierTransformation *rule = loadRule(ruleReader, &_soldierTransformation, &_soldierTransformationIndex, "name", RuleListOrderedFactory<RuleSoldierTransformation>{ _transformationListOrder, 100 });
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}
	for (const auto& ruleReader : iterateRules("commendations", "type"))
	{
		RuleCommendations *rule = loadRule(ruleReader, &_commendations);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}



	for (const auto& ruleReader : iterateRules("ufoTrajectories", "id"))
	{
		UfoTrajectory *rule = loadRule(ruleReader, &_ufoTrajectories, 0, "id");
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRules("alienMissions", "type"))
	{
		RuleAlienMission *rule = loadRule(ruleReader, &_alienMissions, &_alienMissionsIndex);
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRules("arcScripts", "type"))
	{
		RuleArcScript* rule = loadRule(ruleReader, &_arcScripts, &_arcScriptIndex, "type");
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRules("eventScripts", "type"))
	{
		RuleEventScript* rule = loadRule(ruleReader, &_eventScripts, &_eventScriptIndex, "type");
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRules("events", "name"))
	{
		RuleEvent* rule = loadRule(ruleReader, &_events, &_eventIndex, "name");
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRules("missionScripts", "type"))
	{
		RuleMissionScript *rule = loadRule(ruleReader, &_missionScripts, &_missionScriptIndex, "type");
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}



	for (const auto& ruleReader : iterateRulesSpecific("mapScripts"))
	{
		std::string type = ruleReader["type"].readVal<std::string>();
		if (ruleReader["delete"])
		{
			type = ruleReader["delete"].readVal<std::string>(type);
		}
		if (_mapScripts.find(type) != _mapScripts.end())
		{
			Collections::deleteAll(_mapScripts[type]);
		}
		for (const auto& commandsReader : ruleReader["commands"].children())
		{
			MapScript *mapScript = new MapScript();
			mapScript->load(commandsReader);
			_mapScripts[type].push_back(mapScript);
		}
	}



	for (const auto& ruleReader : iterateRulesSpecific("ufopaedia"))
	{
		if (ruleReader["id"])
		{
			std::string id = ruleReader["id"].readVal<std::string>();
			ArticleDefinition *rule;
			if (_ufopaediaArticles.find(id) != _ufopaediaArticles.end())
			{
				rule = _ufopaediaArticles[id];
			}
			else
			{
				if (!ruleReader["type_id"]) { // otherwise it throws and I wasted hours
					Log(LOG_ERROR) << "ufopaedia item misses type_id attribute.";
					continue;
				}
				UfopaediaTypeId type = ruleReader["type_id"].readVal<UfopaediaTypeId>();
				switch (type)
				{
				case UFOPAEDIA_TYPE_CRAFT: rule = new ArticleDefinitionCraft(); break;
				case UFOPAEDIA_TYPE_CRAFT_WEAPON: rule = new ArticleDefinitionCraftWeapon(); break;
				case UFOPAEDIA_TYPE_VEHICLE: rule = new ArticleDefinitionVehicle(); break;
				case UFOPAEDIA_TYPE_ITEM: rule = new ArticleDefinitionItem(); break;
				case UFOPAEDIA_TYPE_ARMOR: rule = new ArticleDefinitionArmor(); break;
				case UFOPAEDIA_TYPE_BASE_FACILITY: rule = new ArticleDefinitionBaseFacility(); break;
				case UFOPAEDIA_TYPE_TEXTIMAGE: rule = new ArticleDefinitionTextImage(); break;
				case UFOPAEDIA_TYPE_TEXT: rule = new ArticleDefinitionText(); break;
				case UFOPAEDIA_TYPE_UFO: rule = new ArticleDefinitionUfo(); break;
				case UFOPAEDIA_TYPE_TFTD:
				case UFOPAEDIA_TYPE_TFTD_CRAFT:
				case UFOPAEDIA_TYPE_TFTD_CRAFT_WEAPON:
				case UFOPAEDIA_TYPE_TFTD_VEHICLE:
				case UFOPAEDIA_TYPE_TFTD_ITEM:
				case UFOPAEDIA_TYPE_TFTD_ARMOR:
				case UFOPAEDIA_TYPE_TFTD_BASE_FACILITY:
				case UFOPAEDIA_TYPE_TFTD_USO:
					rule = new ArticleDefinitionTFTD();
					break;
				default: rule = 0; break;
				}
				_ufopaediaArticles[id] = rule;
				_ufopaediaIndex.push_back(id);
			}
			_ufopaediaListOrder += 100;
			rule->load(ruleReader, _ufopaediaListOrder);
		}
		else if (ruleReader["delete"])
		{
			std::string type = ruleReader["delete"].readVal<std::string>();
			auto j = _ufopaediaArticles.find(type);
			if (j != _ufopaediaArticles.end())
			{
				_ufopaediaArticles.erase(j);
			}
			auto idx = std::find(_ufopaediaIndex.begin(), _ufopaediaIndex.end(), type);
			if (idx != _ufopaediaIndex.end())
			{
				_ufopaediaIndex.erase(idx);
			}
		}
	}



	auto loadStartingBase = [&](const char* startingBaseType, YAML::YamlString& destRef)
	{
		// Bases can't be copied, so for savegame purposes we store the node instead
		const auto& base = loadDocInfoHelper(startingBaseType);
		if (base)
		{
			if (isMapHelper(base))
			{
				destRef = base.emitDescendants();
			}
			else
			{
				throw LoadRuleException(startingBaseType, base, "expected normal map node");
			}
		}
	};
	loadStartingBase("startingBase", _startingBaseDefault);
	loadStartingBase("startingBaseBeginner", _startingBaseBeginner);
	loadStartingBase("startingBaseExperienced", _startingBaseExperienced);
	loadStartingBase("startingBaseVeteran", _startingBaseVeteran);
	loadStartingBase("startingBaseGenius", _startingBaseGenius);
	loadStartingBase("startingBaseSuperhuman", _startingBaseSuperhuman);

	if (reader["startingTime"])
	{
		_startingTime.load(reader["startingTime"]);
	}
	reader.tryRead("startingDifficulty", _startingDifficulty);
	reader.tryRead("maxViewDistance", _maxViewDistance);
	reader.tryRead("maxDarknessToSeeUnits", _maxDarknessToSeeUnits);
	reader.tryRead("costHireEngineer", _costHireEngineer);
	reader.tryRead("costHireScientist", _costHireScientist);
	reader.tryRead("costEngineer", _costEngineer);
	reader.tryRead("costScientist", _costScientist);
	reader.tryRead("timePersonnel", _timePersonnel);
	reader.tryRead("hireByCountryOdds", _hireByCountryOdds);
	reader.tryRead("hireByRegionOdds", _hireByRegionOdds);
	reader.tryRead("initialFunding", _initialFunding);
	reader.tryRead("alienFuel", _alienFuel);
	reader.tryRead("fontName", _fontName);
	reader.tryRead("psiUnlockResearch", _psiUnlockResearch);
	reader.tryRead("fakeUnderwaterBaseUnlockResearch", _fakeUnderwaterBaseUnlockResearch);
	reader.tryRead("newBaseUnlockResearch", _newBaseUnlockResearch);
	reader.tryRead("hireScientistsUnlockResearch", _hireScientistsUnlockResearch);
	reader.tryRead("hireEngineersUnlockResearch", _hireEngineersUnlockResearch);
	loadBaseFunction("mod", _hireScientistsRequiresBaseFunc, reader["hireScientistsRequiresBaseFunc"]);
	loadBaseFunction("mod", _hireEngineersRequiresBaseFunc, reader["hireEngineersRequiresBaseFunc"]);
	reader.tryRead("destroyedFacility", _destroyedFacility);

	reader.tryRead("turnAIUseGrenade", _aiUseDelayGrenade);
	reader.tryRead("turnAIUseBlaster", _aiUseDelayBlaster);
	if (const auto& nodeAI = loadDocInfoHelper("ai"))
	{
		nodeAI.tryRead("useDelayBlaster", _aiUseDelayBlaster);
		nodeAI.tryRead("useDelayFirearm", _aiUseDelayFirearm);
		nodeAI.tryRead("useDelayGrenade", _aiUseDelayGrenade);
		nodeAI.tryRead("aiUseDelayProxy", _aiUseDelayProxy);
		nodeAI.tryRead("useDelayMelee", _aiUseDelayMelee);
		nodeAI.tryRead("useDelayPsionic", _aiUseDelayPsionic);

		nodeAI.tryRead("fireChoiceIntelCoeff", _aiFireChoiceIntelCoeff);
		nodeAI.tryRead("fireChoiceAggroCoeff", _aiFireChoiceAggroCoeff);
		nodeAI.tryRead("extendedFireModeChoice", _aiExtendedFireModeChoice);
		nodeAI.tryRead("respectMaxRange", _aiRespectMaxRange);
		nodeAI.tryRead("destroyBaseFacilities", _aiDestroyBaseFacilities);
		nodeAI.tryRead("pickUpWeaponsMoreActively", _aiPickUpWeaponsMoreActively);
		nodeAI.tryRead("pickUpWeaponsMoreActivelyCiv", _aiPickUpWeaponsMoreActivelyCiv);
	}
	reader.tryRead("maxLookVariant", _maxLookVariant);
	reader.tryRead("tooMuchSmokeThreshold", _tooMuchSmokeThreshold);
	reader.tryRead("customTrainingFactor", _customTrainingFactor);
	reader.tryRead("minReactionAccuracy", _minReactionAccuracy);
	reader.tryRead("chanceToStopRetaliation", _chanceToStopRetaliation);
	reader.tryRead("lessAliensDuringBaseDefense", _lessAliensDuringBaseDefense);
	reader.tryRead("allowCountriesToCancelAlienPact", _allowCountriesToCancelAlienPact);
	reader.tryRead("buildInfiltrationBaseCloseToTheCountry", _buildInfiltrationBaseCloseToTheCountry);
	reader.tryRead("infiltrateRandomCountryInTheRegion", _infiltrateRandomCountryInTheRegion);
	reader.tryRead("allowAlienBasesOnWrongTextures", _allowAlienBasesOnWrongTextures);
	reader.tryRead("kneelBonusGlobal", _kneelBonusGlobal);
	reader.tryRead("oneHandedPenaltyGlobal", _oneHandedPenaltyGlobal);
	reader.tryRead("enableCloseQuartersCombat", _enableCloseQuartersCombat);
	reader.tryRead("closeQuartersAccuracyGlobal", _closeQuartersAccuracyGlobal);
	reader.tryRead("closeQuartersTuCostGlobal", _closeQuartersTuCostGlobal);
	reader.tryRead("closeQuartersEnergyCostGlobal", _closeQuartersEnergyCostGlobal);
	reader.tryRead("closeQuartersSneakUpGlobal", _closeQuartersSneakUpGlobal);
	reader.tryRead("noLOSAccuracyPenaltyGlobal", _noLOSAccuracyPenaltyGlobal);
	reader.tryRead("surrenderMode", _surrenderMode);
	reader.tryRead("bughuntMinTurn", _bughuntMinTurn);
	reader.tryRead("bughuntMaxEnemies", _bughuntMaxEnemies);
	reader.tryRead("bughuntRank", _bughuntRank);
	reader.tryRead("bughuntLowMorale", _bughuntLowMorale);
	reader.tryRead("bughuntTimeUnitsLeft", _bughuntTimeUnitsLeft);


	if (const auto& nodeMana = loadDocInfoHelper("mana"))
	{
		nodeMana.tryRead("enabled", _manaEnabled);
		nodeMana.tryRead("battleUI", _manaBattleUI);
		nodeMana.tryRead("unlockResearch", _manaUnlockResearch);
		nodeMana.tryRead("trainingPrimary", _manaTrainingPrimary);
		nodeMana.tryRead("trainingSecondary", _manaTrainingSecondary);

		nodeMana.tryRead("woundThreshold", _manaMissingWoundThreshold);
		nodeMana.tryRead("replenishAfterMission", _manaReplenishAfterMission);
	}
	if (const auto& nodeHealth = loadDocInfoHelper("health"))
	{
		nodeHealth.tryRead("woundThreshold", _healthMissingWoundThreshold);
		nodeHealth.tryRead("replenishAfterMission", _healthReplenishAfterMission);
	}


	if (const auto& nodeGameOver = loadDocInfoHelper("gameOver"))
	{
		nodeGameOver.tryRead("loseMoney", _loseMoney);
		nodeGameOver.tryRead("loseRating", _loseRating);
		nodeGameOver.tryRead("loseDefeat", _loseDefeat);
	}
	reader.tryRead("ufoGlancingHitThreshold", _ufoGlancingHitThreshold);
	reader.tryRead("ufoBeamWidthParameter", _ufoBeamWidthParameter);
	if (const auto& arrayReader = reader["ufoTractorBeamSizeModifiers"])
	{
		for (size_t j = 0; j < std::size(_ufoTractorBeamSizeModifiers); j++)
			arrayReader[j].tryReadVal(_ufoTractorBeamSizeModifiers[j]);
	}
	reader.tryRead("escortRange", _escortRange);
	reader.tryRead("drawEnemyRadarCircles", _drawEnemyRadarCircles);
	reader.tryRead("escortsJoinFightAgainstHK", _escortsJoinFightAgainstHK);
	reader.tryRead("hunterKillerFastRetarget", _hunterKillerFastRetarget);
	reader.tryRead("crewEmergencyEvacuationSurvivalChance", _crewEmergencyEvacuationSurvivalChance);
	reader.tryRead("pilotsEmergencyEvacuationSurvivalChance", _pilotsEmergencyEvacuationSurvivalChance);
	reader.tryRead("soldiersPerSergeant", _soldiersPerRank[RANK_SERGEANT]);
	reader.tryRead("soldiersPerCaptain", _soldiersPerRank[RANK_CAPTAIN]);
	reader.tryRead("soldiersPerColonel", _soldiersPerRank[RANK_COLONEL]);
	reader.tryRead("soldiersPerCommander", _soldiersPerRank[RANK_COMMANDER]);
	reader.tryRead("pilotAccuracyZeroPoint", _pilotAccuracyZeroPoint);
	reader.tryRead("pilotAccuracyRange", _pilotAccuracyRange);
	reader.tryRead("pilotReactionsZeroPoint", _pilotReactionsZeroPoint);
	reader.tryRead("pilotReactionsRange", _pilotReactionsRange);
	if (const auto& arrayReader = reader["pilotBraveryThresholds"])
	{
		for (size_t j = 0; j < std::size(_pilotBraveryThresholds); j++)
			arrayReader[j].tryReadVal(_pilotBraveryThresholds[j]);
	}
	reader.tryRead("performanceBonusFactor", _performanceBonusFactor);
	reader.tryRead("enableNewResearchSorting", _enableNewResearchSorting);
	reader.tryRead("displayCustomCategories", _displayCustomCategories);
	reader.tryRead("shareAmmoCategories", _shareAmmoCategories);
	reader.tryRead("showDogfightDistanceInKm", _showDogfightDistanceInKm);
	reader.tryRead("showFullNameInAlienInventory", _showFullNameInAlienInventory);
	reader.tryRead("alienInventoryOffsetX", _alienInventoryOffsetX);
	reader.tryRead("alienInventoryOffsetBigUnit", _alienInventoryOffsetBigUnit);
	reader.tryRead("hidePediaInfoButton", _hidePediaInfoButton);
	reader.tryRead("extraNerdyPediaInfoType", _extraNerdyPediaInfoType);
	reader.tryRead("giveScoreAlsoForResearchedArtifacts", _giveScoreAlsoForResearchedArtifacts);
	reader.tryRead("statisticalBulletConservation", _statisticalBulletConservation);
	reader.tryRead("stunningImprovesMorale", _stunningImprovesMorale);
	reader.tryRead("tuRecoveryWakeUpNewTurn", _tuRecoveryWakeUpNewTurn);
	reader.tryRead("shortRadarRange", _shortRadarRange);
	reader.tryRead("buildTimeReductionScaling", _buildTimeReductionScaling);
	reader.tryRead("baseDefenseMapFromLocation", _baseDefenseMapFromLocation);
	reader.tryRead("pediaReplaceCraftFuelWithRangeType", _pediaReplaceCraftFuelWithRangeType);
	reader.tryRead("missionRatings", _missionRatings);
	reader.tryRead("monthlyRatings", _monthlyRatings);
	loadUnorderedNamesToNames("mod", _fixedUserOptions, reader["fixedUserOptions"]);
	loadUnorderedNamesToNames("mod", _recommendedUserOptions, reader["recommendedUserOptions"]);
	loadUnorderedNames("mod", _hiddenMovementBackgrounds, reader["hiddenMovementBackgrounds"]);
	loadUnorderedNames("mod", _baseNamesFirst, reader["baseNamesFirst"]);
	loadUnorderedNames("mod", _baseNamesMiddle, reader["baseNamesMiddle"]);
	loadUnorderedNames("mod", _baseNamesLast, reader["baseNamesLast"]);
	loadUnorderedNames("mod", _operationNamesFirst, reader["operationNamesFirst"]);
	loadUnorderedNames("mod", _operationNamesLast, reader["operationNamesLast"]);
	reader.tryRead("disableUnderwaterSounds", _disableUnderwaterSounds);
	reader.tryRead("enableUnitResponseSounds", _enableUnitResponseSounds);
	for (const auto& unitResponseSound : iterateRulesSpecific("unitResponseSounds"))
	{
		std::string type = unitResponseSound["name"].readVal<std::string>();
		if (unitResponseSound["selectUnitSound"])
			loadSoundOffset(type, _selectUnitSound[type], unitResponseSound["selectUnitSound"], "BATTLE.CAT");
		if (unitResponseSound["startMovingSound"])
			loadSoundOffset(type, _startMovingSound[type], unitResponseSound["startMovingSound"], "BATTLE.CAT");
		if (unitResponseSound["selectWeaponSound"])
			loadSoundOffset(type, _selectWeaponSound[type], unitResponseSound["selectWeaponSound"], "BATTLE.CAT");
		if (unitResponseSound["annoyedSound"])
			loadSoundOffset(type, _annoyedSound[type], unitResponseSound["annoyedSound"], "BATTLE.CAT");
	}
	loadSoundOffset("global", _selectBaseSound, reader["selectBaseSound"], "BATTLE.CAT");
	loadSoundOffset("global", _startDogfightSound, reader["startDogfightSound"], "BATTLE.CAT");
	reader.tryRead("flagByKills", _flagByKills);
	reader.tryRead("defeatScore", _defeatScore);
	reader.tryRead("defeatFunds", _defeatFunds);
	reader.tryRead("difficultyDemigod", _difficultyDemigod);

	if (const auto& difficultyCoefficientOverrides = loadDocInfoHelper("difficultyCoefficientOverrides"))
	{
		difficultyCoefficientOverrides.tryRead("monthlyRatingThresholds", _monthlyRatingThresholds);
		difficultyCoefficientOverrides.tryRead("ufoFiringRateCoefficients", _ufoFiringRateCoefficients);
		difficultyCoefficientOverrides.tryRead("ufoEscapeCountdownCoefficients", _ufoEscapeCountdownCoefficients);
		difficultyCoefficientOverrides.tryRead("retaliationTriggerOdds", _retaliationTriggerOdds);
		difficultyCoefficientOverrides.tryRead("retaliationBaseRegionOdds", _retaliationBaseRegionOdds);
		difficultyCoefficientOverrides.tryRead("aliensFacingCraftOdds", _aliensFacingCraftOdds);
	}

	if (const auto& arrayReader = reader["difficultyCoefficient"])
	{
		for (size_t j = 0; j < std::size(DIFFICULTY_COEFFICIENT); j++)
		{
			arrayReader[j].tryReadVal(DIFFICULTY_COEFFICIENT[j]);
			_statAdjustment[j].growthMultiplier = DIFFICULTY_COEFFICIENT[j];
		}
	}
	if (const auto& arrayReader = reader["sellPriceCoefficient"])
	{
		for (size_t j = 0; j < std::size(SELL_PRICE_COEFFICIENT); j++)
			arrayReader[j].tryReadVal(SELL_PRICE_COEFFICIENT[j]);
	}
	if (const auto& arrayReader = reader["buyPriceCoefficient"])
	{
		for (size_t j = 0; j < std::size(BUY_PRICE_COEFFICIENT); j++)
			arrayReader[j].tryReadVal(BUY_PRICE_COEFFICIENT[j]);
	}
	if (const auto& arrayReader = reader["difficultyBasedRetaliationDelay"])
	{
		for (size_t j = 0; j < std::size(DIFFICULTY_BASED_RETAL_DELAY); j++)
			arrayReader[j].tryReadVal(DIFFICULTY_BASED_RETAL_DELAY[j]);
	}
	if (const auto& arrayReader = reader["unitResponseSoundsFrequency"])
	{
		for (size_t j = 0; j < std::size(UNIT_RESPONSE_SOUNDS_FREQUENCY); j++)
			arrayReader[j].tryReadVal(UNIT_RESPONSE_SOUNDS_FREQUENCY[j]);
	}

	reader.tryRead("alienItemLevels", _alienItemLevels);

	for (const auto& patchReader : reader["MCDPatches"].children()) // this should not be used by mods
	{
		std::string type = patchReader["type"].readVal<std::string>();
		if (_MCDPatches.find(type) != _MCDPatches.end())
		{
			_MCDPatches[type]->load(patchReader);
		}
		else
		{
			MCDPatch *patch = new MCDPatch();
			patch->load(patchReader);
			_MCDPatches[type] = patch;
		}
	}
	for (const auto& ruleReader : iterateRulesSpecific("extraSprites"))
	{
		if (ruleReader["type"] || ruleReader["typeSingle"])
		{
			std::string type;
			ruleReader["type"].tryReadVal<std::string>(type);
			if (type.empty())
			{
				type = ruleReader["typeSingle"].readVal<std::string>();
			}
			ExtraSprites *extraSprites = new ExtraSprites();
			const ModData* data = _modCurrent;
			// doesn't support modIndex
			if (type == "TEXTURE.DAT")
				data = &_modData.at(0);
			extraSprites->load(ruleReader, data);
			_extraSprites[type].push_back(extraSprites);
		}
		else if (ruleReader["delete"])
		{
			std::string type = ruleReader["delete"].readVal<std::string>();
			auto j = _extraSprites.find(type);
			if (j != _extraSprites.end())
			{
				_extraSprites.erase(j);
			}
		}
	}
	for (const auto& ruleReader : iterateRulesSpecific("customPalettes"))
	{
		CustomPalettes* rule = loadRule(ruleReader, &_customPalettes, &_customPalettesIndex);
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRulesSpecific("extraSounds"))
	{
		std::string type = ruleReader["type"].readVal<std::string>();
		ExtraSounds *extraSounds = new ExtraSounds();
		extraSounds->load(ruleReader, _modCurrent);
		_extraSounds.push_back(std::make_pair(type, extraSounds));
	}
	for (const auto& ruleReader : iterateRulesSpecific("extraStrings"))
	{
		std::string type = ruleReader["type"].readVal<std::string>();
		if (_extraStrings.find(type) != _extraStrings.end())
		{
			_extraStrings[type]->load(ruleReader);
		}
		else
		{
			ExtraStrings *extraStrings = new ExtraStrings();
			extraStrings->load(ruleReader);
			_extraStrings[type] = extraStrings;
		}
	}

	for (const auto& ruleReader : iterateRulesSpecific("statStrings"))
	{
		StatString *statString = new StatString();
		statString->load(ruleReader);
		_statStrings.push_back(statString);
	}

	for (const auto& ruleReader : iterateRulesSpecific("interfaces"))
	{
		RuleInterface *rule = loadRule(ruleReader, &_interfaces);
		if (rule != 0)
		{
			rule->load(ruleReader, this);
		}
	}

	for (const auto& ruleReader : iterateRulesSpecific("cutscenes"))
	{
		RuleVideo *rule = loadRule(ruleReader, &_videos);
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}
	for (const auto& ruleReader : iterateRulesSpecific("musics"))
	{
		RuleMusic *rule = loadRule(ruleReader, &_musicDefs);
		if (rule != 0)
		{
			rule->load(ruleReader);
		}
	}

	if (reader["globe"])
	{
		_globe->load(reader["globe"]);
	}
	if (reader["converter"])
	{
		_converter->load(reader["converter"]);
	}
	if (const auto& constants = reader["constants"])
	{
		//backward compatibility version
		if (constants.isSeq())
		{
			for (const auto& constant : constants.children())
			{
				loadConstants(constant);
			}
		}
		else
		{
			loadConstants(constants.useIndex());
		}
	}

	// refresh _psiRequirements for psiStrengthEval
	for (const auto& facType : _facilitiesIndex)
	{
		RuleBaseFacility *rule = getBaseFacility(facType);
		if (rule->getPsiLaboratories() > 0)
		{
			_psiRequirements = rule->getRequirements();
			break;
		}
	}
	// override the default (used when you want to separate screening and training)
	if (!_psiUnlockResearch.empty())
	{
		_psiRequirements.clear();
		_psiRequirements.push_back(_psiUnlockResearch);
	}

	if (const auto& arrayReader = reader["aimAndArmorMultipliers"])
	{
		for (size_t j = 0; j < MaxDifficultyLevels; j++)
		{
			arrayReader[j].tryReadVal(_statAdjustment[j].aimMultiplier);
			arrayReader[j].tryReadVal(_statAdjustment[j].armorMultiplier);
		}
	}
	if (const auto& arrayReader = reader["aimMultipliers"])
	{
		for (size_t j = 0; j < MaxDifficultyLevels; j++)
			arrayReader[j].tryReadVal(_statAdjustment[j].aimMultiplier);
	}
	if (const auto& arrayReader = reader["armorMultipliers"])
	{
		for (size_t j = 0; j < MaxDifficultyLevels; j++)
			arrayReader[j].tryReadVal(_statAdjustment[j].armorMultiplier);
	}
	if (const auto& arrayReader = reader["armorMultipliersAbs"])
	{
		for (size_t j = 0; j < MaxDifficultyLevels; j++)
			arrayReader[j].tryReadVal(_statAdjustment[j].armorMultiplierAbs);
	}
	if (const auto& arrayReader = reader["statGrowthMultipliersAbs"])
	{
		for (size_t j = 0; j < MaxDifficultyLevels; j++)
			arrayReader[j].tryReadVal(_statAdjustment[j].statGrowthAbs);
	}
	if (const auto& arrayReader = reader["statGrowthMultipliers"])
	{
		arrayReader.tryReadVal(_statAdjustment[0].statGrowth);
		for (size_t j = 1; j < MaxDifficultyLevels; j++)
			_statAdjustment[j].statGrowth = _statAdjustment[0].statGrowth;
	}
	if (const auto& lighting = loadDocInfoHelper("lighting"))
	{
		lighting.tryRead("maxStatic", _maxStaticLightDistance);
		lighting.tryRead("maxDynamic", _maxDynamicLightDistance);
		lighting.tryRead("enhanced", _enhancedLighting);
	}
}

/**
 * Helper function protecting from circular references in node definition.
 * @param node Node to test
 * @param name Name of original node.
 * @param limit Current depth.
 */
static void refNodeTestDeepth(const YAML::YamlNodeReader& reader, const std::string &name, int limit)
{
	if (limit > 64)
	{
		throw Exception("Nest limit of refNode reach in " + name);
	}
	if (const auto& nested = reader["refNode"])
	{
		if (!nested.isMap())
		{
			std::stringstream ss;
			ss << "Invalid refNode at nest level of ";
			ss << limit;
			ss << " in ";
			ss << name;
			throw Exception(ss.str());
		}
		refNodeTestDeepth(nested, name, limit + 1);
	}
}

/**
 * Loads a rule element, adding/removing from vectors as necessary.
 * @param node YAML node.
 * @param map Map associated to the rule type.
 * @param index Index vector for the rule type.
 * @param key Rule key name.
 * @return Pointer to new rule if one was created, or NULL if one was removed.
 */
template <typename T, typename F>
T *Mod::loadRule(const YAML::YamlNodeReader& reader, std::map<std::string, T*> *map, std::vector<std::string> *index, const std::string &key, F&& factory)
{
	T *rule = 0;

	auto getDescriptionNode = [&](const YAML::YamlNodeReader& r)
	{
		return ryml::formatrs<std::string>("'{}' at line {}", r.readKey<ryml::csubstr>(), r.getLocationInFile().line);
	};
	auto getNameFromNode = [&](const YAML::YamlNodeReader& r)
	{
		std::string name = r.readVal<std::string>();
		if (isEmptyRuleName(name))
		{
			throw Exception(ryml::formatrs<std::string>("Invalid value for {}", getDescriptionNode(r)));
		}
		return name;
	};
	auto addTracking = [&](std::unordered_map<const void*, const ModData*>& track, const auto* t)
	{
		track[static_cast<const void*>(t)] = _modCurrent;
	};
	auto removeTracking = [&]( std::unordered_map<const void*, const ModData*>& track, const auto* t)
	{
		track.erase(static_cast<const void*>(t));
	};

	const auto& defaultNode = reader[ryml::to_csubstr(key)];
	const auto& deleteNode = reader[ryml::to_csubstr(YamlRuleNodeDelete)];
	const auto& newNode = reader[ryml::to_csubstr(YamlRuleNodeNew)];
	const auto& overrideNode = reader[ryml::to_csubstr(YamlRuleNodeOverride)];
	const auto& updateNode = reader[ryml::to_csubstr(YamlRuleNodeUpdate)];
	const auto& ignoreNode = reader[ryml::to_csubstr(YamlRuleNodeIgnore)];

	// check for duplicates
	if ((!!defaultNode + !!deleteNode + !!newNode + !!overrideNode + !!updateNode + !!ignoreNode) > 1)
	{
		throw Exception(ryml::formatrs<std::string>("Rule at line {} can only have one of: {} / new / update / override / delete / ignore", reader.getLocationInFile().line, key));
	}

	if (defaultNode)
	{
		std::string type = getNameFromNode(defaultNode);


		auto i = map->find(type);
		if (i != map->end())
		{
			rule = i->second;
		}
		else
		{
			rule = factory(type);
			addTracking(_ruleCreationTracking, rule);
			(*map)[type] = rule;
			if (index != 0)
			{
				index->push_back(type);
			}
		}

		// protection from self referencing refNode node
		refNodeTestDeepth(reader, type, 0);
		addTracking(_ruleLastUpdateTracking, rule);
	}
	else if (deleteNode)
	{
		std::string type = getNameFromNode(deleteNode);

		auto i = map->find(type);
		if (i != map->end())
		{
			removeTracking(_ruleCreationTracking, i->second);
			removeTracking(_ruleLastUpdateTracking, i->second);
			delete i->second;
			map->erase(i);
		}
		if (index != 0)
		{
			auto idx = std::find(index->begin(), index->end(), type);
			if (idx != index->end())
			{
				index->erase(idx);
			}
		}
	}
	else if (newNode)
	{
		std::string type = getNameFromNode(newNode);

		auto i = map->find(type);
		if (i != map->end())
		{
			checkForSoftError(true, type, "Rule named '" + type  + "' already used for " + getDescriptionNode(newNode), LOG_ERROR);
		}
		else
		{
			rule = factory(type);
			addTracking(_ruleCreationTracking, rule);
			(*map)[type] = rule;
			if (index != 0)
			{
				index->push_back(type);
			}

			// protection from self referencing refNode node
			refNodeTestDeepth(reader, type, 0);
			addTracking(_ruleLastUpdateTracking, rule);
		}
	}
	else if (overrideNode)
	{
		std::string type = getNameFromNode(overrideNode);

		auto i = map->find(type);
		if (i != map->end())
		{
			rule = i->second;

			// protection from self referencing refNode node
			refNodeTestDeepth(reader, type, 0);
			addTracking(_ruleLastUpdateTracking, rule);
		}
		else
		{
			checkForSoftError(true, type, "Rule named '" + type  + "' does not exist for " + getDescriptionNode(overrideNode), LOG_ERROR);
		}
	}
	else if (updateNode)
	{
		std::string type = getNameFromNode(updateNode);

		auto i = map->find(type);
		if (i != map->end())
		{
			rule = i->second;

			// protection from self referencing refNode node
			refNodeTestDeepth(reader, type, 0);
			addTracking(_ruleLastUpdateTracking, rule);
		}
		else
		{
			Log(LOG_INFO) << "Rule named '" << type  << "' does not exist for " << getDescriptionNode(updateNode);
		}
	}
	else if (ignoreNode)
	{
		// nothing to see there...
	}
	else
	{
		checkForObsoleteErrorByYear("Mod", reader, "Missing main node", 2025);
	}

	return rule;
}

/**
 * Generates a brand new saved game with starting data.
 * @return A new saved game.
 */
SavedGame *Mod::newSave(GameDifficulty diff) const
{
	SavedGame *save = new SavedGame();
	save->setDifficulty(diff);

	// Add countries
	for (const auto& countryName : _countriesIndex)
	{
		RuleCountry *countryRule = getCountry(countryName);
		if (!countryRule->getLonMin().empty())
			save->getCountries()->push_back(new Country(countryRule));
	}
	// Adjust funding to total $6M
	int missing = ((_initialFunding - save->getCountryFunding()/1000) / (int)save->getCountries()->size()) * 1000;
	for (auto* country : *save->getCountries())
	{
		int funding = country->getFunding().back() + missing;
		if (funding < 0)
		{
			funding = country->getFunding().back();
		}
		country->setFunding(funding);
	}
	save->setFunds(save->getCountryFunding());

	// Add regions
	for (const auto& regionName : _regionsIndex)
	{
		RuleRegion *regionRule = getRegion(regionName);
		if (!regionRule->getLonMin().empty())
			save->getRegions()->push_back(new Region(regionRule));
	}

	// Set up starting base
	const YAML::YamlRootNodeReader startingBaseByDiff(getStartingBase(diff), "(starting base template)");
	Base *base = new Base(this);
	base->load(startingBaseByDiff, save, true);
	if (const auto& globalTemplates = startingBaseByDiff["globalTemplates"])
	{
		save->loadTemplates(globalTemplates, this);
	}
	if (const auto& ufopediaRuleStatus = startingBaseByDiff["ufopediaRuleStatus"])
	{
		save->loadUfopediaRuleStatus(ufopediaRuleStatus);
	}
	save->getBases()->push_back(base);

	// Correct IDs
	for (auto* craft : *base->getCrafts())
	{
		save->getId(craft->getRules()->getType());
	}

	// Remove craft weapons if needed
	for (auto* craft : *base->getCrafts())
	{
		if (craft->getMaxUnitsRaw() < 0 || craft->getMaxVehiclesAndLargeSoldiersRaw() < 0)
		{
			size_t weaponIndex = 0;
			for (auto* current : *craft->getWeapons())
			{
				base->getStorageItems()->addItem(current->getRules()->getLauncherItem());
				base->getStorageItems()->addItem(current->getRules()->getClipItem(), current->getClipsLoaded());
				craft->addCraftStats(-current->getRules()->getBonusStats());
				craft->setShield(craft->getShield());
				delete current;
				craft->getWeapons()->at(weaponIndex) = 0;
				weaponIndex++;
			}
		}
	}

	// Determine starting soldier types
	std::vector<std::string> soldierTypes = _soldiersIndex; // copy!
	for (auto iter = soldierTypes.begin(); iter != soldierTypes.end();)
	{
		if (getSoldier(*iter)->getRequirements().empty())
		{
			++iter;
		}
		else
		{
			iter = soldierTypes.erase(iter);
		}
	}

	const auto& randomSoldiersReader = startingBaseByDiff["randomSoldiers"];
	std::vector<std::string> randomTypes;
	if (randomSoldiersReader)
	{
		// Starting soldiers specified by type
		if (randomSoldiersReader.isMap())
		{
			std::map<std::string, int> randomSoldiers;
			randomSoldiersReader.tryReadVal(randomSoldiers);
			for (const auto& pair : randomSoldiers)
			{
				for (int s = 0; s < pair.second; ++s)
				{
					randomTypes.push_back(pair.first);
				}
			}
		}
		// Starting soldiers specified by amount
		else if (randomSoldiersReader.hasVal())
		{
			int randomSoldiers = randomSoldiersReader.readVal(0);
			if (randomSoldiers > 0 && soldierTypes.empty())
			{
				Log(LOG_ERROR) << "Cannot generate soldiers for the starting base. There are no available soldier types. Maybe all of them are locked by research?";
			}
			else
			{
				for (int s = 0; s < randomSoldiers; ++s)
				{
					randomTypes.push_back(soldierTypes[RNG::generate(0, soldierTypes.size() - 1)]);
				}
			}
		}
		// Generate soldiers
		for (size_t i = 0; i < randomTypes.size(); ++i)
		{
			RuleSoldier* ruleSoldier = getSoldier(randomTypes[i], true);
			int nationality = save->selectSoldierNationalityByLocation(this, ruleSoldier, nullptr); // -1 (unfortunately the first base is not placed yet)
			Soldier *soldier = genSoldier(save, ruleSoldier, nationality);
			base->getSoldiers()->push_back(soldier);
			// Award soldier a special 'original eight' commendation
			if (_commendations.find("STR_MEDAL_ORIGINAL8_NAME") != _commendations.end())
			{
				SoldierDiary *diary = soldier->getDiary();
				diary->awardOriginalEightCommendation(this);
				for (auto* comm : *diary->getSoldierCommendations())
				{
					comm->makeOld();
				}
			}
		}
		// Assign pilots to craft (interceptors first, transport last) and non-pilots to transports only
		for (auto* soldier : *base->getSoldiers())
		{
			if (soldier->getArmor()->getSize() > 1)
			{
				// "Large soldiers" just stay in the base
			}
			else if (soldier->getRules()->getAllowPiloting())
			{
				Craft *found = 0;
				for (auto* craft : *base->getCrafts())
				{
					CraftPlacementErrors err = craft->validateAddingSoldier(craft->getSpaceAvailable(), soldier);
					if (!found && craft->getRules()->getAllowLanding() && err == CPE_None)
					{
						// Remember transporter as fall-back, but search further for interceptors
						found = craft;
					}
					if (!craft->getRules()->getAllowLanding() && err == CPE_None && craft->getSpaceUsed() < craft->getRules()->getPilots())
					{
						// Fill interceptors with minimum amount of pilots necessary
						found = craft;
					}
				}
				soldier->setCraft(found);
			}
			else
			{
				Craft *found = 0;
				for (auto* craft : *base->getCrafts())
				{
					CraftPlacementErrors err = craft->validateAddingSoldier(craft->getSpaceAvailable(), soldier);
					if (craft->getRules()->getAllowLanding() && err == CPE_None)
					{
						// First available transporter will do
						found = craft;
						break;
					}
				}
				soldier->setCraft(found);
			}
		}
	}

	// Setup alien strategy
	save->getAlienStrategy().init(this);
	save->setTime(_startingTime);

	return save;
}

/**
 * Returns the rules for the specified country.
 * @param id Country type.
 * @return Rules for the country.
 */
RuleCountry *Mod::getCountry(const std::string &id, bool error) const
{
	return getRule(id, "Country", _countries, error);
}

/**
 * Returns the list of all countries
 * provided by the mod.
 * @return List of countries.
 */
const std::vector<std::string> &Mod::getCountriesList() const
{
	return _countriesIndex;
}

/**
 * Returns the rules for the specified extra globe label.
 * @param id Extra globe label type.
 * @return Rules for the extra globe label.
 */
RuleCountry *Mod::getExtraGlobeLabel(const std::string &id, bool error) const
{
	return getRule(id, "Extra Globe Label", _extraGlobeLabels, error);
}

/**
 * Returns the list of all extra globe labels
 * provided by the mod.
 * @return List of extra globe labels.
 */
const std::vector<std::string> &Mod::getExtraGlobeLabelsList() const
{
	return _extraGlobeLabelsIndex;
}

/**
 * Returns the rules for the specified region.
 * @param id Region type.
 * @return Rules for the region.
 */
RuleRegion *Mod::getRegion(const std::string &id, bool error) const
{
	return getRule(id, "Region", _regions, error);
}

/**
 * Returns the list of all regions
 * provided by the mod.
 * @return List of regions.
 */
const std::vector<std::string> &Mod::getRegionsList() const
{
	return _regionsIndex;
}

/**
 * Returns the rules for the specified base facility.
 * @param id Facility type.
 * @return Rules for the facility.
 */
RuleBaseFacility *Mod::getBaseFacility(const std::string &id, bool error) const
{
	return getRule(id, "Facility", _facilities, error);
}

/**
 * Returns the list of all base facilities
 * provided by the mod.
 * @return List of base facilities.
 */
const std::vector<std::string> &Mod::getBaseFacilitiesList() const
{
	return _facilitiesIndex;
}

/**
 * Returns the rules for the specified craft.
 * @param id Craft type.
 * @return Rules for the craft.
 */
RuleCraft *Mod::getCraft(const std::string &id, bool error) const
{
	return getRule(id, "Craft", _crafts, error);
}

/**
 * Returns the list of all crafts
 * provided by the mod.
 * @return List of crafts.
 */
const std::vector<std::string> &Mod::getCraftsList() const
{
	return _craftsIndex;
}

/**
 * Returns the rules for the specified craft weapon.
 * @param id Craft weapon type.
 * @return Rules for the craft weapon.
 */
RuleCraftWeapon *Mod::getCraftWeapon(const std::string &id, bool error) const
{
	return getRule(id, "Craft Weapon", _craftWeapons, error);
}

/**
 * Returns the list of all craft weapons
 * provided by the mod.
 * @return List of craft weapons.
 */
const std::vector<std::string> &Mod::getCraftWeaponsList() const
{
	return _craftWeaponsIndex;
}
/**
 * Is given item a launcher or ammo for craft weapon.
 */
bool Mod::isCraftWeaponStorageItem(const RuleItem* item) const
{
	return Collections::sortVectorHave(_craftWeaponStorageItemsCache, item);
}

/**
* Returns the rules for the specified item category.
* @param id Item category type.
* @return Rules for the item category, or 0 when the item category is not found.
*/
RuleItemCategory *Mod::getItemCategory(const std::string &id, bool error) const
{
	auto i = _itemCategories.find(id);
	if (_itemCategories.end() != i) return i->second; else return 0;
}

/**
* Returns the list of all item categories
* provided by the mod.
* @return List of item categories.
*/
const std::vector<std::string> &Mod::getItemCategoriesList() const
{
	return _itemCategoriesIndex;
}

/**
 * Returns the rules for the specified item.
 * @param id Item type.
 * @return Rules for the item, or 0 when the item is not found.
 */
RuleItem *Mod::getItem(const std::string &id, bool error) const
{
	if (id == Armor::NONE)
	{
		return 0;
	}
	return getRule(id, "Item", _items, error);
}

/**
 * Returns the list of all items
 * provided by the mod.
 * @return List of items.
 */
const std::vector<std::string> &Mod::getItemsList() const
{
	return _itemsIndex;
}

/**
 * Returns the rules for the specified weapon set.
 * @param type Weapon set type.
 * @return Rules for the weapon set.
 */
RuleWeaponSet* Mod::getWeaponSet(const std::string& type, bool error) const
{
	return getRule(type, "WeaponSet", _weaponSets, error);
}

/**
 * Returns the rules for the specified UFO.
 * @param id UFO type.
 * @return Rules for the UFO.
 */
RuleUfo *Mod::getUfo(const std::string &id, bool error) const
{
	return getRule(id, "UFO", _ufos, error);
}

/**
 * Returns the list of all ufos
 * provided by the mod.
 * @return List of ufos.
 */
const std::vector<std::string> &Mod::getUfosList() const
{
	return _ufosIndex;
}

/**
 * Returns the rules for the specified terrain.
 * @param name Terrain name.
 * @return Rules for the terrain.
 */
RuleTerrain *Mod::getTerrain(const std::string &name, bool error) const
{
	return getRule(name, "Terrain", _terrains, error);
}

/**
 * Returns the list of all terrains
 * provided by the mod.
 * @return List of terrains.
 */
const std::vector<std::string> &Mod::getTerrainList() const
{
	return _terrainIndex;
}

/**
 * Returns the info about a specific map data file.
 * @param name Datafile name.
 * @return Rules for the datafile.
 */
MapDataSet *Mod::getMapDataSet(const std::string &name)
{
	auto map = _mapDataSets.find(name);
	if (map == _mapDataSets.end())
	{
		MapDataSet *set = new MapDataSet(name);
		_mapDataSets[name] = set;
		return set;
	}
	else
	{
		return map->second;
	}
}

/**
 * Returns the rules for the specified skill.
 * @param name Skill type.
 * @return Rules for the skill.
 */
RuleSkill *Mod::getSkill(const std::string &name, bool error) const
{
	return getRule(name, "Skill", _skills, error);
}

/**
 * Returns the info about a specific unit.
 * @param name Unit name.
 * @return Rules for the units.
 */
RuleSoldier *Mod::getSoldier(const std::string &name, bool error) const
{
	return getRule(name, "Soldier", _soldiers, error);
}

/**
 * Returns the list of all soldiers
 * provided by the mod.
 * @return List of soldiers.
 */
const std::vector<std::string> &Mod::getSoldiersList() const
{
	return _soldiersIndex;
}

/**
 * Returns the rules for the specified commendation.
 * @param id Commendation type.
 * @return Rules for the commendation.
 */
RuleCommendations *Mod::getCommendation(const std::string &id, bool error) const
{
	return getRule(id, "Commendation", _commendations, error);
}

/**
 * Gets the list of commendations provided by the mod.
 * @return The list of commendations.
 */
const std::map<std::string, RuleCommendations *> &Mod::getCommendationsList() const
{
	return _commendations;
}

/**
 * Returns the info about a specific unit.
 * @param name Unit name.
 * @return Rules for the units.
 */
Unit *Mod::getUnit(const std::string &name, bool error) const
{
	return getRule(name, "Unit", _units, error);
}

/**
 * Returns the info about a specific alien race.
 * @param name Race name.
 * @return Rules for the race.
 */
AlienRace *Mod::getAlienRace(const std::string &name, bool error) const
{
	return getRule(name, "Alien Race", _alienRaces, error);
}

/**
 * Returns the list of all alien races.
 * provided by the mod.
 * @return List of alien races.
 */
const std::vector<std::string> &Mod::getAlienRacesList() const
{
	return _aliensIndex;
}

/**
 * Returns specific EnviroEffects.
 * @param name EnviroEffects name.
 * @return Rules for the EnviroEffects.
 */
RuleEnviroEffects* Mod::getEnviroEffects(const std::string& name) const
{
	auto i = _enviroEffects.find(name);
	if (_enviroEffects.end() != i) return i->second; else return 0;
}

/**
 * Returns the list of all EnviroEffects
 * provided by the mod.
 * @return List of EnviroEffects.
 */
const std::vector<std::string>& Mod::getEnviroEffectsList() const
{
	return _enviroEffectsIndex;
}

/**
 * Returns the info about a specific starting condition.
 * @param name Starting condition name.
 * @return Rules for the starting condition.
 */
RuleStartingCondition* Mod::getStartingCondition(const std::string& name) const
{
	auto i = _startingConditions.find(name);
	if (_startingConditions.end() != i) return i->second; else return 0;
}

/**
 * Returns the list of all starting conditions
 * provided by the mod.
 * @return List of starting conditions.
 */
const std::vector<std::string>& Mod::getStartingConditionsList() const
{
	return _startingConditionsIndex;
}

/**
 * Returns the info about a specific deployment.
 * @param name Deployment name.
 * @return Rules for the deployment.
 */
AlienDeployment *Mod::getDeployment(const std::string &name, bool error) const
{
	return getRule(name, "Alien Deployment", _alienDeployments, error);
}

/**
 * Returns the list of all alien deployments
 * provided by the mod.
 * @return List of alien deployments.
 */
const std::vector<std::string> &Mod::getDeploymentsList() const
{
	return _deploymentsIndex;
}


/**
 * Returns the info about a specific armor.
 * @param name Armor name.
 * @return Rules for the armor.
 */
Armor *Mod::getArmor(const std::string &name, bool error) const
{
	return getRule(name, "Armor", _armors, error);
}

/**
 * Returns the list of all armors
 * provided by the mod.
 * @return List of armors.
 */
const std::vector<std::string> &Mod::getArmorsList() const
{
	return _armorsIndex;
}

/**
 * Gets the available armors for soldiers.
 */
const std::vector<const Armor*> &Mod::getArmorsForSoldiers() const
{
	return _armorsForSoldiersCache;
}

/**
 * Check if item is used for armor storage.
 */
bool Mod::isArmorStorageItem(const RuleItem* item) const
{
	return Collections::sortVectorHave(_armorStorageItemsCache, item);
}


/**
 * Returns the hiring cost of an individual engineer.
 * @return Cost.
 */
int Mod::getHireEngineerCost() const
{
	return _costHireEngineer != 0 ? _costHireEngineer : _costEngineer * 2;
}

/**
* Returns the hiring cost of an individual scientist.
* @return Cost.
*/
int Mod::getHireScientistCost() const
{
	return _costHireScientist != 0 ? _costHireScientist: _costScientist * 2;
}

/**
 * Returns the monthly cost of an individual engineer.
 * @return Cost.
 */
int Mod::getEngineerCost() const
{
	return _costEngineer;
}

/**
 * Returns the monthly cost of an individual scientist.
 * @return Cost.
 */
int Mod::getScientistCost() const
{
	return _costScientist;
}

/**
 * Returns the time it takes to transfer personnel
 * between bases.
 * @return Time in hours.
 */
int Mod::getPersonnelTime() const
{
	return _timePersonnel;
}

/**
 * Gets maximum supported lookVariant.
 * @return value in range from 0 to 63
 */
int Mod::getMaxLookVariant() const
{
	return abs(_maxLookVariant) % RuleSoldier::LookVariantMax;
}

/**
 * Gets the escort range.
 * @return Escort range.
 */
double Mod::getEscortRange() const
{
	return _escortRange;
}

/**
 * Returns the article definition for a given name.
 * @param name Article name.
 * @return Article definition.
 */
ArticleDefinition *Mod::getUfopaediaArticle(const std::string &name, bool error) const
{
	return getRule(name, "UFOpaedia Article", _ufopaediaArticles, error);
}

/**
 * Returns the list of all articles
 * provided by the mod.
 * @return List of articles.
 */
const std::vector<std::string> &Mod::getUfopaediaList() const
{
	return _ufopaediaIndex;
}

/**
* Returns the list of all article categories
* provided by the mod.
* @return List of categories.
*/
const std::vector<std::string> &Mod::getUfopaediaCategoryList() const
{
	return _ufopaediaCatIndex;
}

/**
 * Returns the list of inventories.
 * @return Pointer to inventory list.
 */
std::map<std::string, RuleInventory*> *Mod::getInventories()
{
	return &_invs;
}

/**
 * Returns the rules for a specific inventory.
 * @param id Inventory type.
 * @return Inventory ruleset.
 */
RuleInventory *Mod::getInventory(const std::string &id, bool error) const
{
	return getRule(id, "Inventory", _invs, error);
}

/**
 * Returns basic damage type.
 * @param type damage type.
 * @return basic damage ruleset.
 */
const RuleDamageType *Mod::getDamageType(ItemDamageType type) const
{
	return _damageTypes.at(type);
}

/**
 * Returns the list of inventories.
 * @return The list of inventories.
 */
const std::vector<std::string> &Mod::getInvsList() const
{
	return _invsIndex;
}

/**
 * Returns the rules for the specified research project.
 * @param id Research project type.
 * @return Rules for the research project.
 */
RuleResearch *Mod::getResearch(const std::string &id, bool error) const
{
	return getRule(id, "Research", _research, error);
}

/**
 * Gets the ruleset list for from research list.
 */
std::vector<const RuleResearch*> Mod::getResearch(const std::vector<std::string> &id) const
{
	std::vector<const RuleResearch*> dest;
	dest.reserve(id.size());
	for (auto& name : id)
	{
		auto* rule = getResearch(name, false);
		if (rule)
		{
			dest.push_back(rule);
		}
		else
		{
			throw Exception("Unknown research '" + name + "'");
		}
	}
	return dest;
}

/**
 * Gets the ruleset for a specific research project.
 */
const std::map<std::string, RuleResearch *> &Mod::getResearchMap() const
{
	return _research;
}

/**
 * Returns the list of research projects.
 * @return The list of research projects.
 */
const std::vector<std::string> &Mod::getResearchList() const
{
	return _researchIndex;
}

/**
 * Returns the rules for the specified manufacture project.
 * @param id Manufacture project type.
 * @return Rules for the manufacture project.
 */
RuleManufacture *Mod::getManufacture (const std::string &id, bool error) const
{
	return getRule(id, "Manufacture", _manufacture, error);
}

/**
 * Returns the list of manufacture projects.
 * @return The list of manufacture projects.
 */
const std::vector<std::string> &Mod::getManufactureList() const
{
	return _manufactureIndex;
}

/**
 * Returns the rules for the specified soldier bonus type.
 * @param id Soldier bonus type.
 * @return Rules for the soldier bonus type.
 */
RuleSoldierBonus *Mod::getSoldierBonus(const std::string &id, bool error) const
{
	return getRule(id, "SoldierBonus", _soldierBonus, error);
}

/**
 * Returns the list of soldier bonus types.
 * @return The list of soldier bonus types.
 */
const std::vector<std::string> &Mod::getSoldierBonusList() const
{
	return _soldierBonusIndex;
}

/**
 * Returns the rules for the specified soldier transformation project.
 * @param id Soldier transformation project type.
 * @return Rules for the soldier transformation project.
 */
RuleSoldierTransformation *Mod::getSoldierTransformation (const std::string &id, bool error) const
{
	return getRule(id, "SoldierTransformation", _soldierTransformation, error);
}

/**
 * Returns the list of soldier transformation projects.
 * @return The list of soldier transformation projects.
 */
const std::vector<std::string> &Mod::getSoldierTransformationList() const
{
	return _soldierTransformationIndex;
}

/**
 * Generates and returns a list of facilities for custom bases.
 * The list contains all the facilities that are listed in the 'startingBase'
 * part of the ruleset.
 * @return The list of facilities for custom bases.
 */
std::vector<RuleBaseFacility*> Mod::getCustomBaseFacilities(GameDifficulty diff) const
{
	std::vector<RuleBaseFacility*> placeList;

	const YAML::YamlRootNodeReader startingBaseByDiff(getStartingBase(diff), "(starting base template)");
	for (const auto& facilityReader : startingBaseByDiff["facilities"].children())
	{
		std::string type = facilityReader["type"].readVal<std::string>();
		RuleBaseFacility *facility = getBaseFacility(type, true);
		if (!facility->isLift())
		{
			placeList.push_back(facility);
		}
	}
	return placeList;
}

/**
 * Returns the data for the specified ufo trajectory.
 * @param id Ufo trajectory id.
 * @return A pointer to the data for the specified ufo trajectory.
 */
const UfoTrajectory *Mod::getUfoTrajectory(const std::string &id, bool error) const
{
	return getRule(id, "Trajectory", _ufoTrajectories, error);
}

/**
 * Returns the rules for the specified alien mission.
 * @param id Alien mission type.
 * @return Rules for the alien mission.
 */
const RuleAlienMission *Mod::getAlienMission(const std::string &id, bool error) const
{
	return getRule(id, "Alien Mission", _alienMissions, error);
}

/**
 * Returns the rules for a random alien mission based on a specific objective.
 * @param objective Alien mission objective.
 * @return Rules for the alien mission.
 */
const RuleAlienMission *Mod::getRandomMission(MissionObjective objective, size_t monthsPassed) const
{
	int totalWeight = 0;
	std::map<int, RuleAlienMission*> possibilities;
	for (auto& pair : _alienMissions)
	{
		if (pair.second->getObjective() == objective && pair.second->getWeight(monthsPassed) > 0)
		{
			totalWeight += pair.second->getWeight(monthsPassed);
			possibilities[totalWeight] = pair.second;
		}
	}
	if (totalWeight > 0)
	{
		int pick = RNG::generate(1, totalWeight);
		for (auto& pair : possibilities)
		{
			if (pick <= pair.first)
			{
				return pair.second;
			}
		}
	}
	return 0;
}

/**
 * Returns the list of alien mission types.
 * @return The list of alien mission types.
 */
const std::vector<std::string> &Mod::getAlienMissionList() const
{
	return _alienMissionsIndex;
}

/**
 * Gets the alien item level table.
 * @return A deep array containing the alien item levels.
 */
const std::vector<std::vector<int> > &Mod::getAlienItemLevels() const
{
	return _alienItemLevels;
}

/**
 * Gets the default starting base.
 * @return The starting base definition.
 */
const YAML::YamlString& Mod::getDefaultStartingBase() const
{
	return _startingBaseDefault;
}

/**
 * Gets the custom starting base (by game difficulty).
 * @return The starting base definition.
 */
const YAML::YamlString& Mod::getStartingBase(GameDifficulty diff) const
{
	if (diff == DIFF_BEGINNER && _startingBaseBeginner.yaml != "")
	{
		return _startingBaseBeginner;
	}
	else if (diff == DIFF_EXPERIENCED && _startingBaseExperienced.yaml != "")
	{
		return _startingBaseExperienced;
	}
	else if (diff == DIFF_VETERAN && _startingBaseVeteran.yaml != "")
	{
		return _startingBaseVeteran;
	}
	else if (diff == DIFF_GENIUS && _startingBaseGenius.yaml != "")
	{
		return _startingBaseGenius;
	}
	else if (diff == DIFF_SUPERHUMAN && _startingBaseSuperhuman.yaml != "")
	{
		return _startingBaseSuperhuman;
	}

	return _startingBaseDefault;
}

/**
 * Gets the defined starting time.
 * @return The time the game starts in.
 */
const GameTime &Mod::getStartingTime() const
{
	return _startingTime;
}

/**
 * Gets an MCDPatch.
 * @param id The ID of the MCDPatch we want.
 * @return The MCDPatch based on ID, or 0 if none defined.
 */
MCDPatch *Mod::getMCDPatch(const std::string &id) const
{
	auto i = _MCDPatches.find(id);
	if (_MCDPatches.end() != i) return i->second; else return 0;
}

/**
 * Gets the list of external sprites.
 * @return The list of external sprites.
 */
const std::map<std::string, std::vector<ExtraSprites *> > &Mod::getExtraSprites() const
{
	return _extraSprites;
}

/**
 * Gets the list of custom palettes.
 * @return The list of custom palettes.
 */
const std::vector<std::string> &Mod::getCustomPalettes() const
{
	return _customPalettesIndex;
}

/**
 * Gets the list of external sounds.
 * @return The list of external sounds.
 */
const std::vector<std::pair<std::string, ExtraSounds *> > &Mod::getExtraSounds() const
{
	return _extraSounds;
}

/**
 * Gets the list of external strings.
 * @return The list of external strings.
 */
const std::map<std::string, ExtraStrings *> &Mod::getExtraStrings() const
{
	return _extraStrings;
}

/**
 * Gets the list of StatStrings.
 * @return The list of StatStrings.
 */
const std::vector<StatString *> &Mod::getStatStrings() const
{
	return _statStrings;
}

/**
 * Compares rules based on their list orders.
 */
template <typename T>
struct compareRule
{
	Mod *_mod;

	compareRule(Mod *mod) : _mod(mod)
	{
	}

	bool operator()(const std::pair<const std::string, T*>* r1, const std::pair<const std::string, T*>* r2) const
	{
		return (r1->second->getListOrder() < r2->second->getListOrder());
	}
};

/**
 * Craft weapons use the list order of their launcher item.
 */
template <>
struct compareRule<RuleCraftWeapon>
{
	Mod *_mod;

	compareRule(Mod *mod) : _mod(mod)
	{
	}

	bool operator()(const std::pair<const std::string, RuleCraftWeapon*>* r1, const std::pair<const std::string, RuleCraftWeapon*>* r2) const
	{
		return (r1->second->getLauncherItem()->getListOrder() < r2->second->getLauncherItem()->getListOrder());
	}
};

/**
 * Armor uses the list order of their store item.
 * Itemless armor comes before all else.
 */
template <>
struct compareRule<Armor>
{
	Mod *_mod;

	compareRule(Mod *mod) : _mod(mod)
	{
	}

	bool operator()(const std::pair<const std::string, Armor*>* r1, const std::pair<const std::string, Armor*>* r2) const
	{
		return operator()(r1->second, r2->second);
	}

	bool operator()(const Armor* armor1, const Armor* armor2) const
	{
		const RuleItem *rule1 = armor1->getStoreItem();
		const RuleItem *rule2 = armor2->getStoreItem();
		if (!rule1 && !rule2)
			return (armor1->getListOrder() < armor2->getListOrder()); // tiebreaker
		else if (!rule1)
			return true;
		else if (!rule2)
			return false;
		else
			return (rule1->getListOrder() < rule2->getListOrder() ||
				   (rule1->getListOrder() == rule2->getListOrder() && armor1->getListOrder() < armor2->getListOrder()));
	}
};

/**
 * Ufopaedia articles use section and list order.
 */
template <>
struct compareRule<ArticleDefinition>
{
	Mod *_mod;
	const std::map<std::string, int> &_sections;

	compareRule(Mod *mod) : _mod(mod), _sections(mod->getUfopaediaSections())
	{
	}

	bool operator()(const std::pair<const std::string, ArticleDefinition*>* r1, const std::pair<const std::string, ArticleDefinition*>* r2) const
	{
		if (r1->second->section == r2->second->section)
			return (r1->second->getListOrder() < r2->second->getListOrder());
		else
			return (_sections.at(r1->second->section) < _sections.at(r2->second->section));
	}
};

/**
 * Ufopaedia sections use article list order.
 */
struct compareSection
{
	Mod *_mod;
	const std::map<std::string, int> &_sections;

	compareSection(Mod *mod) : _mod(mod), _sections(mod->getUfopaediaSections())
	{
	}

	bool operator()(const std::string &r1, const std::string &r2) const
	{
		return _sections.at(r1) < _sections.at(r2);
	}
};

/**
* Sort helper to avoid having to do loop-ups inside comparator function
*/
template <typename RuleType, typename ComparatorFunc>
void sortIndex(std::vector<std::string>& index, std::map<std::string, RuleType*>& map, ComparatorFunc comparator)
{
	std::vector<const std::pair<const std::string, RuleType*>*> tempVector;
	tempVector.reserve(index.size());
	for (const auto& pair : map)
		tempVector.push_back(&pair);
	std::sort(tempVector.begin(), tempVector.end(), comparator);
	for (int i = 0; i < index.size(); ++i)
		index[i].assign(tempVector[i]->first);
}

/**
 * Sorts all our lists according to their weight.
 */
void Mod::sortLists()
{
	for (auto& rulePair : _ufopaediaArticles)
	{
		auto* rule = rulePair.second;
		if (rule->section != UFOPAEDIA_NOT_AVAILABLE)
		{
			if (_ufopaediaSections.find(rule->section) == _ufopaediaSections.end())
			{
				_ufopaediaSections[rule->section] = rule->getListOrder();
				_ufopaediaCatIndex.push_back(rule->section);
			}
			else
			{
				_ufopaediaSections[rule->section] = std::min(_ufopaediaSections[rule->section], rule->getListOrder());
			}
		}
	}
	sortIndex(_itemCategoriesIndex, _itemCategories, compareRule<RuleItemCategory>(this));
	sortIndex(_itemsIndex, _items, compareRule<RuleItem>(this));
	sortIndex(_craftsIndex, _crafts, compareRule<RuleCraft>(this));
	sortIndex(_facilitiesIndex, _facilities, compareRule<RuleBaseFacility>(this));
	sortIndex(_researchIndex, _research, compareRule<RuleResearch>(this));
	sortIndex(_manufactureIndex, _manufacture, compareRule<RuleManufacture>(this));
	sortIndex(_soldierTransformationIndex, _soldierTransformation, compareRule<RuleSoldierTransformation>(this));
	sortIndex(_invsIndex, _invs, compareRule<RuleInventory>(this));
	// special cases
	sortIndex(_craftWeaponsIndex, _craftWeapons, compareRule<RuleCraftWeapon>(this));
	sortIndex(_armorsIndex, _armors, compareRule<Armor>(this));
	std::sort(_armorsForSoldiersCache.begin(), _armorsForSoldiersCache.end(), compareRule<Armor>(this));
	_ufopaediaSections[UFOPAEDIA_NOT_AVAILABLE] = 0;
	sortIndex(_ufopaediaIndex, _ufopaediaArticles, compareRule<ArticleDefinition>(this));
	std::sort(_ufopaediaCatIndex.begin(), _ufopaediaCatIndex.end(), compareSection(this));
	sortIndex(_soldiersIndex, _soldiers, compareRule<RuleSoldier>(this));
	sortIndex(_aliensIndex, _alienRaces, compareRule<AlienRace>(this));
}

/**
 * Gets the research-requirements for Psi-Lab (it's a cache for psiStrengthEval)
 */
const std::vector<std::string> &Mod::getPsiRequirements() const
{
	return _psiRequirements;
}

/**
 * Creates a new randomly-generated soldier.
 * @param save Saved game the soldier belongs to.
 * @param type The soldier type to generate.
 * @return Newly generated soldier.
 */
Soldier *Mod::genSoldier(SavedGame *save, const RuleSoldier* ruleSoldier, int nationality) const
{
	Soldier *soldier = 0;
	int newId = save->getId("STR_SOLDIER");

	// Check for duplicates
	// Original X-COM gives up after 10 tries so might as well do the same here
	bool duplicate = true;
	for (int tries = 0; tries < 10 && duplicate; ++tries)
	{
		delete soldier;
		soldier = new Soldier(const_cast<RuleSoldier*>(ruleSoldier), ruleSoldier->getDefaultArmor(), nationality, newId);
		duplicate = false;
		for (auto* xbase : *save->getBases())
		{
			if (duplicate) break; // loop finished
			for (auto* xsoldier : *xbase->getSoldiers())
			{
				if (duplicate) break; // loop finished
				if (xsoldier->getName() == soldier->getName())
				{
					duplicate = true;
				}
			}
			for (auto* transfer : *xbase->getTransfers())
			{
				if (duplicate) break; // loop finished
				if (transfer->getType() == TRANSFER_SOLDIER && transfer->getSoldier()->getName() == soldier->getName())
				{
					duplicate = true;
				}
			}
		}
	}

	// calculate new statString
	soldier->calcStatString(getStatStrings(), (Options::psiStrengthEval && save->isResearched(getPsiRequirements())));

	return soldier;
}

/**
 * Gets the name of the item to be used as alien fuel.
 * @return the name of the fuel.
 */
std::string Mod::getAlienFuelName() const
{
	return _alienFuel.first;
}

/**
 * Gets the amount of alien fuel to recover.
 * @return the amount to recover.
 */
int Mod::getAlienFuelQuantity() const
{
	return _alienFuel.second;
}

/**
 * Gets name of font collection.
 * @return the name of YAML-file with font data
 */
std::string Mod::getFontName() const
{
	return _fontName;
}

/**
 * Returns the maximum radar range still considered as short.
 * @return The short radar range threshold.
 */
 int Mod::getShortRadarRange() const
 {
	if (_shortRadarRange > 0)
		return _shortRadarRange;

	int minRadarRange = 0;

	{
		for (auto& facType : _facilitiesIndex)
		{
			RuleBaseFacility *f = getBaseFacility(facType);
			if (f == 0) continue;

			int radarRange = f->getRadarRange();
			if (radarRange > 0 && (minRadarRange == 0 || minRadarRange > radarRange))
			{
				minRadarRange = radarRange;
			}
		}
	}

	return minRadarRange;
 }

/**
 * Returns what should be displayed in craft pedia articles for fuel capacity/range
 * @return 0 = Max theoretical range, 1 = Min and max theoretical max range, 2 = average of the two
 * Otherwise (default), just show the fuel capacity
 */
int Mod::getPediaReplaceCraftFuelWithRangeType() const
{
	return _pediaReplaceCraftFuelWithRangeType;
}

/**
 * Gets information on an interface.
 * @param id the interface we want info on.
 * @return the interface.
 */
RuleInterface *Mod::getInterface(const std::string &id, bool error) const
{
	return getRule(id, "Interface", _interfaces, error);
}

/**
 * Gets the rules for the Geoscape globe.
 * @return Pointer to globe rules.
 */
RuleGlobe *Mod::getGlobe() const
{
	return _globe;
}

/**
* Gets the rules for the Save Converter.
* @return Pointer to converter rules.
*/
RuleConverter *Mod::getConverter() const
{
	return _converter;
}

const std::map<std::string, SoundDefinition *> *Mod::getSoundDefinitions() const
{
	return &_soundDefs;
}

const std::vector<MapScript*> *Mod::getMapScript(const std::string& id) const
{
	auto i = _mapScripts.find(id);
	if (_mapScripts.end() != i)
	{
		return &i->second;
	}
	else
	{
		return 0;
	}
}

/**
 * Returns the data for the specified video cutscene.
 * @param id Video id.
 * @return A pointer to the data for the specified video.
 */
RuleVideo *Mod::getVideo(const std::string &id, bool error) const
{
	return getRule(id, "Video", _videos, error);
}

const std::map<std::string, RuleMusic *> *Mod::getMusic() const
{
	return &_musicDefs;
}

const std::vector<std::string>* Mod::getArcScriptList() const
{
	return &_arcScriptIndex;
}

RuleArcScript* Mod::getArcScript(const std::string& name, bool error) const
{
	return getRule(name, "Arc Script", _arcScripts, error);
}

const std::vector<std::string>* Mod::getEventScriptList() const
{
	return &_eventScriptIndex;
}

RuleEventScript* Mod::getEventScript(const std::string& name, bool error) const
{
	return getRule(name, "Event Script", _eventScripts, error);
}

const std::vector<std::string>* Mod::getEventList() const
{
	return &_eventIndex;
}

RuleEvent* Mod::getEvent(const std::string& name, bool error) const
{
	return getRule(name, "Event", _events, error);
}

const std::vector<std::string> *Mod::getMissionScriptList() const
{
	return &_missionScriptIndex;
}

RuleMissionScript *Mod::getMissionScript(const std::string &name, bool error) const
{
	return getRule(name, "Mission Script", _missionScripts, error);
}
/// Get global script data.
ScriptGlobal *Mod::getScriptGlobal() const
{
	return _scriptGlobal;
}

RuleResearch *Mod::getFinalResearch() const
{
	return _finalResearch;
}

RuleBaseFacility *Mod::getDestroyedFacility() const
{
	if (isEmptyRuleName(_destroyedFacility))
		return 0;

	auto* temp = getBaseFacility(_destroyedFacility, true);
	if (!temp->isSmall())
	{
		throw Exception("Destroyed base facility definition must have size: 1");
	}
	return temp;
}

const std::map<int, std::string> *Mod::getMissionRatings() const
{
	return &_missionRatings;
}
const std::map<int, std::string> *Mod::getMonthlyRatings() const
{
	return &_monthlyRatings;
}

const std::vector<std::string> &Mod::getHiddenMovementBackgrounds() const
{
	return _hiddenMovementBackgrounds;
}

const std::vector<int> &Mod::getFlagByKills() const
{
	return _flagByKills;
}

namespace
{
	const Uint8 ShadeMax = 15;
	/**
	* Recolor class used in UFO
	*/
	struct HairXCOM1
	{
		static const Uint8 Hair = 9 << 4;
		static const Uint8 Face = 6 << 4;
		static inline void func(Uint8& src, const Uint8& cutoff)
		{
			if (src > cutoff && src <= Face + ShadeMax)
			{
				src = Hair + (src & ShadeMax) - 6; //make hair color like male in xcom_0.pck
			}
		}
	};

	/**
	* Recolor class used in TFTD
	*/
	struct HairXCOM2
	{
		static const Uint8 ManHairColor = 4 << 4;
		static const Uint8 WomanHairColor = 1 << 4;
		static inline void func(Uint8& src)
		{
			if (src >= WomanHairColor && src <= WomanHairColor + ShadeMax)
			{
				src = ManHairColor + (src & ShadeMax);
			}
		}
	};

	/**
	* Recolor class used in TFTD
	*/
	struct FaceXCOM2
	{
		static const Uint8 FaceColor = 10 << 4;
		static const Uint8 PinkColor = 14 << 4;
		static inline void func(Uint8& src)
		{
			if (src >= FaceColor && src <= FaceColor + ShadeMax)
			{
				src = PinkColor + (src & ShadeMax);
			}
		}
	};

	/**
	* Recolor class used in TFTD
	*/
	struct BodyXCOM2
	{
		static const Uint8 IonArmorColor = 8 << 4;
		static inline void func(Uint8& src)
		{
			if (src == 153)
			{
				src = IonArmorColor + 12;
			}
			else if (src == 151)
			{
				src = IonArmorColor + 10;
			}
			else if (src == 148)
			{
				src = IonArmorColor + 4;
			}
			else if (src == 147)
			{
				src = IonArmorColor + 2;
			}
			else if (src >= HairXCOM2::WomanHairColor && src <= HairXCOM2::WomanHairColor + ShadeMax)
			{
				src = IonArmorColor + (src & ShadeMax);
			}
		}
	};
	/**
	* Recolor class used in TFTD
	*/
	struct FallXCOM2
	{
		static const Uint8 RoguePixel = 151;
		static inline void func(Uint8& src)
		{
			if (src == RoguePixel)
			{
				src = FaceXCOM2::PinkColor + (src & ShadeMax) + 2;
			}
			else if (src >= BodyXCOM2::IonArmorColor && src <= BodyXCOM2::IonArmorColor + ShadeMax)
			{
				src = FaceXCOM2::PinkColor + (src & ShadeMax);
			}
		}
	};
}

/**
 * Loads the vanilla resources required by the game.
 */
void Mod::loadVanillaResources()
{
	// Create Geoscape surface
	_sets["GlobeMarkers"] = new SurfaceSet(3, 3);
	// dummy resources, that need to be defined in order for mod loading to work correctly
	_sets["CustomArmorPreviews"] = new SurfaceSet(12, 20);
	_sets["CustomItemPreviews"] = new SurfaceSet(12, 20);
	_sets["TinyRanks"] = new SurfaceSet(7, 7);
	_sets["Touch"] = new SurfaceSet(32, 24);

	// Load palettes
	const char *pal[] = { "PAL_GEOSCAPE", "PAL_BASESCAPE", "PAL_GRAPHS", "PAL_UFOPAEDIA", "PAL_BATTLEPEDIA" };
	for (size_t i = 0; i < ARRAYLEN(pal); ++i)
	{
		std::string s = "GEODATA/PALETTES.DAT";
		_palettes[pal[i]] = new Palette();
		_palettes[pal[i]]->loadDat(s, 256, Palette::palOffset(i));
	}
	{
		std::string s1 = "GEODATA/BACKPALS.DAT";
		std::string s2 = "BACKPALS.DAT";
		_palettes[s2] = new Palette();
		_palettes[s2]->loadDat(s1, 128);
	}

	// Correct Battlescape palette
	{
		std::string s1 = "GEODATA/PALETTES.DAT";
		std::string s2 = "PAL_BATTLESCAPE";
		_palettes[s2] = new Palette();
		_palettes[s2]->loadDat(s1, 256, Palette::palOffset(4));

		// Last 16 colors are a greyish gradient
		SDL_Color gradient[] = { { 140, 152, 148, 255 },
		{ 132, 136, 140, 255 },
		{ 116, 124, 132, 255 },
		{ 108, 116, 124, 255 },
		{ 92, 104, 108, 255 },
		{ 84, 92, 100, 255 },
		{ 76, 80, 92, 255 },
		{ 56, 68, 84, 255 },
		{ 48, 56, 68, 255 },
		{ 40, 48, 56, 255 },
		{ 32, 36, 48, 255 },
		{ 24, 28, 32, 255 },
		{ 16, 20, 24, 255 },
		{ 8, 12, 16, 255 },
		{ 3, 4, 8, 255 },
		{ 3, 3, 6, 255 } };
		for (size_t i = 0; i < ARRAYLEN(gradient); ++i)
		{
			SDL_Color *color = _palettes[s2]->getColors(Palette::backPos + 16 + i);
			*color = gradient[i];
		}
		//_palettes[s2]->savePalMod("../../../customPalettes.rul", "PAL_BATTLESCAPE_CUSTOM", "PAL_BATTLESCAPE");
	}

	// Load surfaces
	{
		std::string s1 = "GEODATA/INTERWIN.DAT";
		std::string s2 = "INTERWIN.DAT";
		_surfaces[s2] = new Surface(160, 600);
		_surfaces[s2]->loadScr(s1);
	}

	const auto& geographFiles = FileMap::getVFolderContents("GEOGRAPH");
	auto scrs = FileMap::filterFiles(geographFiles, "SCR");
	for (const auto& name : scrs)
	{
		std::string fname = name;
		std::transform(name.begin(), name.end(), fname.begin(), toupper);
		_surfaces[fname] = new Surface(320, 200);
		_surfaces[fname]->loadScr("GEOGRAPH/" + fname);
	}
	auto bdys = FileMap::filterFiles(geographFiles, "BDY");
	for (const auto& name : bdys)
	{
		std::string fname = name;
		std::transform(name.begin(), name.end(), fname.begin(), toupper);
		_surfaces[fname] = new Surface(320, 200);
		_surfaces[fname]->loadBdy("GEOGRAPH/" + fname);
	}

	auto spks = FileMap::filterFiles(geographFiles, "SPK");
	for (const auto& name : spks)
	{
		std::string fname = name;
		std::transform(name.begin(), name.end(), fname.begin(), toupper);
		_surfaces[fname] = new Surface(320, 200);
		_surfaces[fname]->loadSpk("GEOGRAPH/" + fname);
	}

	// Load surface sets
	std::string sets[] = { "BASEBITS.PCK",
		"INTICON.PCK",
		"TEXTURE.DAT" };

	for (size_t i = 0; i < ARRAYLEN(sets); ++i)
	{
		std::ostringstream s;
		s << "GEOGRAPH/" << sets[i];

		std::string ext = sets[i].substr(sets[i].find_last_of('.') + 1, sets[i].length());
		if (ext == "PCK")
		{
			std::string tab = CrossPlatform::noExt(sets[i]) + ".TAB";
			std::ostringstream s2;
			s2 << "GEOGRAPH/" << tab;
			_sets[sets[i]] = new SurfaceSet(32, 40);
			_sets[sets[i]]->loadPck(s.str(), s2.str());
		}
		else
		{
			_sets[sets[i]] = new SurfaceSet(32, 32);
			_sets[sets[i]]->loadDat(s.str());
		}
	}
	{
		std::string s1 = "GEODATA/SCANG.DAT";
		std::string s2 = "SCANG.DAT";
		_sets[s2] = new SurfaceSet(4, 4);
		_sets[s2]->loadDat(s1);
	}

	// construct sound sets
	_sounds["GEO.CAT"] = new SoundSet();
	_sounds["BATTLE.CAT"] = new SoundSet();
	_sounds["BATTLE2.CAT"] = new SoundSet();
	_sounds["SAMPLE3.CAT"] = new SoundSet();
	_sounds["INTRO.CAT"] = new SoundSet();

	if (!Options::mute) // TBD: ain't it wrong? can Options::mute be reset without a reload?
	{
		// Load sounds
		const auto& contents = FileMap::getVFolderContents("SOUND");
		auto soundFiles = FileMap::filterFiles(contents, "CAT");
		if (_soundDefs.empty())
		{
			std::string catsId[] = { "GEO.CAT", "BATTLE.CAT" };
			std::string catsDos[] = { "SOUND2.CAT", "SOUND1.CAT" };
			std::string catsWin[] = { "SAMPLE.CAT", "SAMPLE2.CAT" };

			// Try the preferred format first, otherwise use the default priority
			std::string *cats[] = { 0, catsWin, catsDos };
			if (Options::preferredSound == SOUND_14)
				cats[0] = catsWin;
			else if (Options::preferredSound == SOUND_10)
				cats[0] = catsDos;

			Options::currentSound = SOUND_AUTO;
			for (size_t i = 0; i < ARRAYLEN(catsId); ++i)
			{
				SoundSet *sound = _sounds[catsId[i]];
				for (size_t j = 0; j < ARRAYLEN(cats); ++j)
				{
					bool wav = true;
					if (cats[j] == 0)
						continue;
					else if (cats[j] == catsDos)
						wav = false;
					std::string fname = "SOUND/" + cats[j][i];
					if (FileMap::fileExists(fname))
					{
						Log(LOG_VERBOSE) << catsId[i] << ": loading sound "<<fname;
						CatFile catfile(fname);
						sound->loadCat(catfile);
						Options::currentSound = (wav) ? SOUND_14 : SOUND_10;
						break;
					} else {
						Log(LOG_VERBOSE) << catsId[i] << ": sound file not found: "<<fname;
					}
				}
				if (sound->getTotalSounds() == 0)
				{
					Log(LOG_ERROR) << catsId[i] << " not found: " << catsWin[i] + " or " + catsDos[i] + " required";
				}
			}
		}
		else
		{
			// we're here if and only if this is the first mod loading
			// and it got soundDefs in the ruleset, which basically means it's xcom2.
			for (auto& i : _soundDefs)
			{
				if (_sounds.find(i.first) == _sounds.end())
				{
					_sounds[i.first] = new SoundSet();
					Log(LOG_VERBOSE) << "TFTD: adding soundset" << i.first;
				}
				std::string fname = "SOUND/" + i.second->getCATFile();
				if (FileMap::fileExists(fname))
				{
					CatFile catfile(fname);
					for (int j : i.second->getSoundList())
					{
						_sounds[i.first]->loadCatByIndex(catfile, j, true);
						Log(LOG_VERBOSE) << "TFTD: adding sound " << j << " to " << i.first;
					}
				}
				else
				{
					Log(LOG_ERROR) << "TFTD sound file not found:" << fname;
				}
			}
		}

		auto file = soundFiles.find("intro.cat");
		if (file != soundFiles.end())
		{
			auto catfile = CatFile("SOUND/INTRO.CAT");
			_sounds["INTRO.CAT"]->loadCat(catfile);
		}

		file = soundFiles.find("sample3.cat");
		if (file != soundFiles.end())
		{
			auto catfile = CatFile("SOUND/SAMPLE3.CAT");
			_sounds["SAMPLE3.CAT"]->loadCat(catfile);
		}
	}

	loadBattlescapeResources(); // TODO load this at battlescape start, unload at battlescape end?


	//update number of shared indexes in surface sets and sound sets
	{
		std::string surfaceNames[] =
		{
			"BIGOBS.PCK",
			"FLOOROB.PCK",
			"HANDOB.PCK",
			"SMOKE.PCK",
			"HIT.PCK",
			"BASEBITS.PCK",
			"INTICON.PCK",
			"CustomArmorPreviews",
			"CustomItemPreviews",
		};

		for (size_t i = 0; i < ARRAYLEN(surfaceNames); ++i)
		{
			SurfaceSet* s = _sets[surfaceNames[i]];
			if (s)
			{
				s->setMaxSharedFrames((int)s->getTotalFrames());
			}
			else
			{
				Log(LOG_ERROR) << "Surface set " << surfaceNames[i] << " not found.";
				throw Exception("Surface set " + surfaceNames[i] + " not found.");
			}
		}
		//special case for surface set that is loaded later
		{
			SurfaceSet* s = _sets["Projectiles"];
			s->setMaxSharedFrames(385);
		}
		{
			SurfaceSet* s = _sets["UnderwaterProjectiles"];
			s->setMaxSharedFrames(385);
		}
		{
			SurfaceSet* s = _sets["GlobeMarkers"];
			s->setMaxSharedFrames(9);
		}
		//HACK: because of value "hitAnimation" from item that is used as offset in "X1.PCK", this set need have same number of shared frames as "SMOKE.PCK".
		{
			SurfaceSet* s = _sets["X1.PCK"];
			s->setMaxSharedFrames((int)_sets["SMOKE.PCK"]->getMaxSharedFrames());
		}
		{
			SurfaceSet* s = _sets["TinyRanks"];
			s->setMaxSharedFrames(6);
		}
		{
			SurfaceSet* s = _sets["Touch"];
			s->setMaxSharedFrames(10);
		}
	}
	{
		std::string soundNames[] =
		{
			"BATTLE.CAT",
			"GEO.CAT",
		};

		for (size_t i = 0; i < ARRAYLEN(soundNames); ++i)
		{
			SoundSet* s = _sounds[soundNames[i]];
			s->setMaxSharedSounds((int)s->getTotalSounds());
		}
		//HACK: case for underwater surface, it should share same offsets as "BATTLE.CAT"
		{
			SoundSet* s = _sounds["BATTLE2.CAT"];
			s->setMaxSharedSounds((int)_sounds["BATTLE.CAT"]->getTotalSounds());
		}
	}
}

/**
 * Loads the resources required by the Battlescape.
 */
void Mod::loadBattlescapeResources()
{
	// Load Battlescape ICONS
	_sets["SPICONS.DAT"] = new SurfaceSet(32, 24);
	_sets["SPICONS.DAT"]->loadDat("UFOGRAPH/SPICONS.DAT");
	_sets["CURSOR.PCK"] = new SurfaceSet(32, 40);
	_sets["CURSOR.PCK"]->loadPck("UFOGRAPH/CURSOR.PCK", "UFOGRAPH/CURSOR.TAB");
	_sets["SMOKE.PCK"] = new SurfaceSet(32, 40);
	_sets["SMOKE.PCK"]->loadPck("UFOGRAPH/SMOKE.PCK", "UFOGRAPH/SMOKE.TAB");
	_sets["HIT.PCK"] = new SurfaceSet(32, 40);
	_sets["HIT.PCK"]->loadPck("UFOGRAPH/HIT.PCK", "UFOGRAPH/HIT.TAB");
	_sets["X1.PCK"] = new SurfaceSet(128, 64);
	_sets["X1.PCK"]->loadPck("UFOGRAPH/X1.PCK", "UFOGRAPH/X1.TAB");
	_sets["MEDIBITS.DAT"] = new SurfaceSet(52, 58);
	_sets["MEDIBITS.DAT"]->loadDat("UFOGRAPH/MEDIBITS.DAT");
	_sets["DETBLOB.DAT"] = new SurfaceSet(16, 16);
	_sets["DETBLOB.DAT"]->loadDat("UFOGRAPH/DETBLOB.DAT");
	_sets["Projectiles"] = new SurfaceSet(3, 3);
	_sets["UnderwaterProjectiles"] = new SurfaceSet(3, 3);

	// Load Battlescape Terrain (only blanks are loaded, others are loaded just in time)
	_sets["BLANKS.PCK"] = new SurfaceSet(32, 40);
	_sets["BLANKS.PCK"]->loadPck("TERRAIN/BLANKS.PCK", "TERRAIN/BLANKS.TAB");

	// Load Battlescape units
	const auto& unitsContents = FileMap::getVFolderContents("UNITS");
	auto usets = FileMap::filterFiles(unitsContents, "PCK");
	for (const auto& name : usets)
	{
		std::string fname = name;
		std::transform(name.begin(), name.end(), fname.begin(), toupper);
		if (fname != "BIGOBS.PCK")
			_sets[fname] = new SurfaceSet(32, 40);
		else
			_sets[fname] = new SurfaceSet(32, 48);
		_sets[fname]->loadPck("UNITS/" + name, "UNITS/" + CrossPlatform::noExt(name) + ".TAB");
	}
	// incomplete chryssalid set: 1.0 data: stop loading.
	if (_sets.find("CHRYS.PCK") != _sets.end() && !_sets["CHRYS.PCK"]->getFrame(225))
	{
		Log(LOG_FATAL) << "Version 1.0 data detected";
		throw Exception("Invalid CHRYS.PCK, please patch your X-COM data to the latest version");
	}
	// TFTD uses the loftemps dat from the terrain folder, but still has enemy unknown's version in the geodata folder, which is short by 2 entries.
	const auto& terrainContents = FileMap::getVFolderContents("TERRAIN");
	if (terrainContents.find("loftemps.dat") != terrainContents.end())
	{
		MapDataSet::loadLOFTEMPS("TERRAIN/LOFTEMPS.DAT", &_voxelData);
	}
	else
	{
		MapDataSet::loadLOFTEMPS("GEODATA/LOFTEMPS.DAT", &_voxelData);
	}

	std::string scrs[] = { "TAC00.SCR" };

	for (size_t i = 0; i < ARRAYLEN(scrs); ++i)
	{
		_surfaces[scrs[i]] = new Surface(320, 200);
		_surfaces[scrs[i]]->loadScr("UFOGRAPH/" + scrs[i]);
	}

	// lower case so we can find them in the contents map
	std::string lbms[] = { "d0.lbm",
		"d1.lbm",
		"d2.lbm",
		"d3.lbm" };
	std::string pals[] = { "PAL_BATTLESCAPE",
		"PAL_BATTLESCAPE_1",
		"PAL_BATTLESCAPE_2",
		"PAL_BATTLESCAPE_3" };

	SDL_Color backPal[] = { { 0, 5, 4, 255 },
	{ 0, 10, 34, 255 },
	{ 2, 9, 24, 255 },
	{ 2, 0, 24, 255 } };

	const auto& ufographContents = FileMap::getVFolderContents("UFOGRAPH");
	for (size_t i = 0; i < ARRAYLEN(lbms); ++i)
	{
		if (ufographContents.find(lbms[i]) == ufographContents.end())
		{
			continue;
		}

		if (!i)
		{
			delete _palettes["PAL_BATTLESCAPE"];
		}
		// TODO: if we need only the palette, say so.
		Surface *tempSurface = new Surface(1, 1);
		tempSurface->loadImage("UFOGRAPH/" + lbms[i]);
		_palettes[pals[i]] = new Palette();
		SDL_Color *colors = tempSurface->getPalette();
		colors[255] = backPal[i];
		_palettes[pals[i]]->setColors(colors, 256);
		createTransparencyLUT(_palettes[pals[i]]);
		delete tempSurface;
	}

	std::string spks[] = { "TAC01.SCR",
		"DETBORD.PCK",
		"DETBORD2.PCK",
		"ICONS.PCK",
		"MEDIBORD.PCK",
		"SCANBORD.PCK",
		"UNIBORD.PCK" };

	for (size_t i = 0; i < ARRAYLEN(spks); ++i)
	{
		std::string fname = spks[i];
		std::transform(fname.begin(), fname.end(), fname.begin(), tolower);
		if (ufographContents.find(fname) == ufographContents.end())
		{
			continue;
		}

		_surfaces[spks[i]] = new Surface(320, 200);
		_surfaces[spks[i]]->loadSpk("UFOGRAPH/" + spks[i]);
	}

	auto bdys = FileMap::filterFiles(ufographContents, "BDY");
	for (const auto& name : bdys)
	{
		std::string idxName = name;
		std::transform(name.begin(), name.end(), idxName.begin(), toupper);
		idxName = idxName.substr(0, idxName.length() - 3);
		if (idxName.substr(0, 3) == "MAN")
		{
			idxName = idxName + "SPK";
		}
		else if (idxName == "TAC01.")
		{
			idxName = idxName + "SCR";
		}
		else
		{
			idxName = idxName + "PCK";
		}
		_surfaces[idxName] = new Surface(320, 200);
		_surfaces[idxName]->loadBdy("UFOGRAPH/" + name);
	}

	// Load Battlescape inventory
	auto invs = FileMap::filterFiles(ufographContents, "SPK");
	for (const auto& name : invs)
	{
		std::string fname = name;
		std::transform(name.begin(), name.end(), fname.begin(), toupper);
		_surfaces[fname] = new Surface(320, 200);
		_surfaces[fname]->loadSpk("UFOGRAPH/" + fname);
	}

	//"fix" of color index in original solders sprites
	if (Options::battleHairBleach)
	{
		std::string name;

		//personal armor
		name = "XCOM_1.PCK";
		if (_sets.find(name) != _sets.end())
		{
			SurfaceSet *xcom_1 = _sets[name];

			for (int i = 0; i < 8; ++i)
			{
				//chest frame
				Surface *surf = xcom_1->getFrame(4 * 8 + i);
				ShaderMove<Uint8> head = ShaderMove<Uint8>(surf);
				GraphSubset dim = head.getBaseDomain();
				surf->lock();
				dim.beg_y = 6;
				dim.end_y = 9;
				head.setDomain(dim);
				ShaderDraw<HairXCOM1>(head, ShaderScalar<Uint8>(HairXCOM1::Face + 5));
				dim.beg_y = 9;
				dim.end_y = 10;
				head.setDomain(dim);
				ShaderDraw<HairXCOM1>(head, ShaderScalar<Uint8>(HairXCOM1::Face + 6));
				surf->unlock();
			}

			for (int i = 0; i < 3; ++i)
			{
				//fall frame
				Surface *surf = xcom_1->getFrame(264 + i);
				ShaderMove<Uint8> head = ShaderMove<Uint8>(surf);
				GraphSubset dim = head.getBaseDomain();
				dim.beg_y = 0;
				dim.end_y = 24;
				dim.beg_x = 11;
				dim.end_x = 20;
				head.setDomain(dim);
				surf->lock();
				ShaderDraw<HairXCOM1>(head, ShaderScalar<Uint8>(HairXCOM1::Face + 6));
				surf->unlock();
			}
		}

		//all TFTD armors
		name = "TDXCOM_?.PCK";
		for (int j = 0; j < 3; ++j)
		{
			name[7] = '0' + j;
			if (_sets.find(name) != _sets.end())
			{
				SurfaceSet *xcom_2 = _sets[name];
				for (int i = 0; i < 16; ++i)
				{
					//chest frame without helm
					Surface *surf = xcom_2->getFrame(262 + i);
					surf->lock();
					if (i < 8)
					{
						//female chest frame
						ShaderMove<Uint8> head = ShaderMove<Uint8>(surf);
						GraphSubset dim = head.getBaseDomain();
						dim.beg_y = 6;
						dim.end_y = 18;
						head.setDomain(dim);
						ShaderDraw<HairXCOM2>(head);

						if (j == 2)
						{
							//fix some pixels in ION armor that was overwrite by previous function
							if (i == 0)
							{
								surf->setPixel(18, 14, 16);
							}
							else if (i == 3)
							{
								surf->setPixel(19, 12, 20);
							}
							else if (i == 6)
							{
								surf->setPixel(13, 14, 16);
							}
						}
					}

					//we change face to pink, to prevent mixup with ION armor backpack that have same color group.
					ShaderDraw<FaceXCOM2>(ShaderMove<Uint8>(surf));
					surf->unlock();
				}

				for (int i = 0; i < 2; ++i)
				{
					//fall frame (first and second)
					Surface *surf = xcom_2->getFrame(256 + i);
					surf->lock();

					ShaderMove<Uint8> head = ShaderMove<Uint8>(surf);
					GraphSubset dim = head.getBaseDomain();
					dim.beg_y = 0;
					if (j == 3)
					{
						dim.end_y = 11 + 5 * i;
					}
					else
					{
						dim.end_y = 17;
					}
					head.setDomain(dim);
					ShaderDraw<FallXCOM2>(head);

					//we change face to pink, to prevent mixup with ION armor backpack that have same color group.
					ShaderDraw<FaceXCOM2>(ShaderMove<Uint8>(surf));
					surf->unlock();
				}

				//Palette fix for ION armor
				if (j == 2)
				{
					int size = xcom_2->getTotalFrames();
					for (int i = 0; i < size; ++i)
					{
						Surface *surf = xcom_2->getFrame(i);
						surf->lock();
						ShaderDraw<BodyXCOM2>(ShaderMove<Uint8>(surf));
						surf->unlock();
					}
				}
			}
		}
	}
}

/**
 * Loads the extra resources defined in rulesets.
 */
void Mod::loadExtraResources()
{
	// Load fonts
	YAML::YamlRootNodeReader reader = FileMap::getYAML("Language/" + _fontName);
	Log(LOG_INFO) << "Loading fonts... " << _fontName;
	for (const auto& fontReader : reader["fonts"].children())
	{
		std::string id = fontReader["id"].readVal<std::string>();
		Font *font = new Font();
		font->load(fontReader);
		_fonts[id] = font;
	}

#ifndef __NO_MUSIC
	// Load musics
	if (!Options::mute)
	{
		const auto& soundFiles = FileMap::getVFolderContents("SOUND");

		// Check which music version is available
		CatFile *adlibcat = 0, *aintrocat = 0;
		GMCatFile *gmcat = 0;

		for (const auto& name : soundFiles)
		{
			if (0 == name.compare("adlib.cat"))
			{
				adlibcat = new CatFile("SOUND/" + name);
			}
			else if (0 == name.compare("aintro.cat"))
			{
				aintrocat = new CatFile("SOUND/" + name);
			}
			else if (0 == name.compare("gm.cat"))
			{
				gmcat = new GMCatFile("SOUND/" + name);
			}
		}

		// Try the preferred format first, otherwise use the default priority
		MusicFormat priority[] = { Options::preferredMusic, MUSIC_FLAC, MUSIC_OGG, MUSIC_MP3, MUSIC_MOD, MUSIC_WAV, MUSIC_ADLIB, MUSIC_GM, MUSIC_MIDI };
		for (auto& pair : _musicDefs)
		{
			Music *music = 0;
			for (size_t j = 0; j < ARRAYLEN(priority) && music == 0; ++j)
			{
				music = loadMusic(priority[j], pair.second, adlibcat, aintrocat, gmcat);
			}
			if (music)
			{
				_musics[pair.first] = music;
			}

		}

		delete gmcat;
		delete adlibcat;
		delete aintrocat;
	}
#endif

	Log(LOG_INFO) << "Lazy loading: " << Options::lazyLoadResources;
	if (!Options::lazyLoadResources)
	{
		Log(LOG_INFO) << "Loading extra resources from ruleset...";
		for (auto& pair : _extraSprites)
		{
			for (auto* extraSprites : pair.second)
			{
				loadExtraSprite(extraSprites);
			}
		}
	}

	if (!Options::mute)
	{
		for (const auto& pair : _extraSounds)
		{
			const auto& setName = pair.first;
			ExtraSounds *soundPack = pair.second;
			SoundSet *set = 0;

			auto search = _sounds.find(setName);
			if (search != _sounds.end())
			{
				set = search->second;
			}
			_sounds[setName] = soundPack->loadSoundSet(set);
		}
	}

	Log(LOG_INFO) << "Loading custom palettes from ruleset...";
	for (const auto& pair : _customPalettes)
	{
		CustomPalettes *palDef = pair.second;
		const auto& palTargetName = palDef->getTarget();
		if (_palettes.find(palTargetName) == _palettes.end())
		{
			Log(LOG_INFO) << "Creating a new palette: " << palTargetName;
			_palettes[palTargetName] = new Palette();
			_palettes[palTargetName]->initBlack();
		}
		else
		{
			Log(LOG_VERBOSE) << "Replacing items in target palette: " << palTargetName;
		}

		Palette *target = _palettes[palTargetName];
		const auto& fileName = palDef->getFile();
		if (fileName.empty())
		{
			for (const auto& def : *palDef->getPalette())
			{
				target->setColor(def.first, def.second.x, def.second.y, def.second.z);
			}
		}
		else
		{
			// Load from JASC file
			auto palFile = FileMap::getIStream(fileName);
			std::string line;
			std::getline(*palFile, line); // header
			std::getline(*palFile, line); // file format
			std::getline(*palFile, line); // number of colors
			int r = 0, g = 0, b = 0;
			for (int j = 0; j < 256; ++j)
			{
				std::getline(*palFile, line); // j-th color index
				std::stringstream ss(line);
				ss >> r;
				ss >> g;
				ss >> b;
				target->setColor(j, r, g, b);
			}
		}
	}

	bool backup_logged = false;
	for (auto& pal : _palettes)
	{
		if (pal.first.find("PAL_") == 0)
		{
			if (!backup_logged) { Log(LOG_INFO) << "Making palette backups..."; backup_logged = true; }
			Log(LOG_VERBOSE) << "Creating a backup for palette: " << pal.first;
			std::string newName = "BACKUP_" + pal.first;
			_palettes[newName] = new Palette();
			_palettes[newName]->initBlack();
			_palettes[newName]->copyFrom(pal.second);
		}
	}

	// Support for UFO-based mods and hybrid mods
	if (_transparencyLUTs.empty() && !_transparencies.empty())
	{
		if (_palettes["PAL_BATTLESCAPE"])
		{
			Log(LOG_INFO) << "Creating transparency LUTs for PAL_BATTLESCAPE...";
			createTransparencyLUT(_palettes["PAL_BATTLESCAPE"]);
		}
		if (_palettes["PAL_BATTLESCAPE_1"] &&
			_palettes["PAL_BATTLESCAPE_2"] &&
			_palettes["PAL_BATTLESCAPE_3"])
		{
			Log(LOG_INFO) << "Creating transparency LUTs for hybrid custom palettes...";
			createTransparencyLUT(_palettes["PAL_BATTLESCAPE_1"]);
			createTransparencyLUT(_palettes["PAL_BATTLESCAPE_2"]);
			createTransparencyLUT(_palettes["PAL_BATTLESCAPE_3"]);
		}
	}

	TextButton::soundPress = getSound("GEO.CAT", Mod::BUTTON_PRESS);
	Window::soundPopup[0] = getSound("GEO.CAT", Mod::WINDOW_POPUP[0]);
	Window::soundPopup[1] = getSound("GEO.CAT", Mod::WINDOW_POPUP[1]);
	Window::soundPopup[2] = getSound("GEO.CAT", Mod::WINDOW_POPUP[2]);
}

void Mod::loadExtraSprite(ExtraSprites *spritePack)
{
	if (spritePack->isLoaded())
		return;

	if (spritePack->getSingleImage())
	{
		Surface *surface = 0;
		auto i = _surfaces.find(spritePack->getType());
		if (i != _surfaces.end())
		{
			surface = i->second;
		}

		_surfaces[spritePack->getType()] = spritePack->loadSurface(surface);
		if (_statePalette)
		{
			if (spritePack->getType().find("_CPAL") == std::string::npos)
			{
				_surfaces[spritePack->getType()]->setPalette(_statePalette);
			}
		}
	}
	else
	{
		SurfaceSet *set = 0;
		auto i = _sets.find(spritePack->getType());
		if (i != _sets.end())
		{
			set = i->second;
		}

		_sets[spritePack->getType()] = spritePack->loadSurfaceSet(set);
		if (_statePalette)
		{
			if (spritePack->getType().find("_CPAL") == std::string::npos)
			{
				_sets[spritePack->getType()]->setPalette(_statePalette);
			}
		}
	}
}

/**
 * Applies necessary modifications to vanilla resources.
 */
void Mod::modResources()
{
	// we're gonna need these
	getSurface("GEOBORD.SCR");
	getSurface("ALTGEOBORD.SCR", false);
	getSurface("BACK07.SCR");
	getSurface("ALTBACK07.SCR", false);
	getSurface("BACK06.SCR");
	getSurface("UNIBORD.PCK");
	getSurfaceSet("HANDOB.PCK");
	getSurfaceSet("FLOOROB.PCK");
	getSurfaceSet("BIGOBS.PCK");

	// embiggen the geoscape background by mirroring the contents
	// modders can provide their own backgrounds via ALTGEOBORD.SCR
	if (_surfaces.find("ALTGEOBORD.SCR") == _surfaces.end())
	{
		int newWidth = 320 - 64, newHeight = 200;
		Surface *newGeo = new Surface(newWidth * 3, newHeight * 3);
		Surface *oldGeo = _surfaces["GEOBORD.SCR"];
		for (int x = 0; x < newWidth; ++x)
		{
			for (int y = 0; y < newHeight; ++y)
			{
				newGeo->setPixel(newWidth + x, newHeight + y, oldGeo->getPixel(x, y));
				newGeo->setPixel(newWidth - x - 1, newHeight + y, oldGeo->getPixel(x, y));
				newGeo->setPixel(newWidth * 3 - x - 1, newHeight + y, oldGeo->getPixel(x, y));

				newGeo->setPixel(newWidth + x, newHeight - y - 1, oldGeo->getPixel(x, y));
				newGeo->setPixel(newWidth - x - 1, newHeight - y - 1, oldGeo->getPixel(x, y));
				newGeo->setPixel(newWidth * 3 - x - 1, newHeight - y - 1, oldGeo->getPixel(x, y));

				newGeo->setPixel(newWidth + x, newHeight * 3 - y - 1, oldGeo->getPixel(x, y));
				newGeo->setPixel(newWidth - x - 1, newHeight * 3 - y - 1, oldGeo->getPixel(x, y));
				newGeo->setPixel(newWidth * 3 - x - 1, newHeight * 3 - y - 1, oldGeo->getPixel(x, y));
			}
		}
		_surfaces["ALTGEOBORD.SCR"] = newGeo;
	}

	// here we create an "alternate" background surface for the base info screen.
	if (_surfaces.find("ALTBACK07.SCR") == _surfaces.end())
	{
		_surfaces["ALTBACK07.SCR"] = new Surface(320, 200);
		_surfaces["ALTBACK07.SCR"]->loadScr("GEOGRAPH/BACK07.SCR");
		for (int y = 172; y >= 152; --y)
			for (int x = 5; x <= 314; ++x)
				_surfaces["ALTBACK07.SCR"]->setPixel(x, y + 4, _surfaces["ALTBACK07.SCR"]->getPixel(x, y));
		for (int y = 147; y >= 134; --y)
			for (int x = 5; x <= 314; ++x)
				_surfaces["ALTBACK07.SCR"]->setPixel(x, y + 9, _surfaces["ALTBACK07.SCR"]->getPixel(x, y));
		for (int y = 132; y >= 109; --y)
			for (int x = 5; x <= 314; ++x)
				_surfaces["ALTBACK07.SCR"]->setPixel(x, y + 10, _surfaces["ALTBACK07.SCR"]->getPixel(x, y));
	}

	// we create extra rows on the soldier stat screens by shrinking them all down one pixel/two pixels.
	int rowHeight = _manaEnabled ? 10 : 11;
	bool moveOnePixelUp = _manaEnabled ? false : true;

	// first, let's do the base info screen
	// erase the old lines, copying from a +2 offset to account for the dithering
	for (int y = 91; y < 199; y += 12)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixel(x, y, _surfaces["BACK06.SCR"]->getPixel(x, y + 2));
	// drawn new lines, use the bottom row of pixels as a basis
	for (int y = 89; y < 199; y += rowHeight)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixel(x, y, _surfaces["BACK06.SCR"]->getPixel(x, 199));
	// finally, move the top of the graph up by one pixel, offset for the last iteration again due to dithering.
	if (moveOnePixelUp)
	{
		for (int y = 72; y < 80; ++y)
			for (int x = 0; x < 320; ++x)
			{
				_surfaces["BACK06.SCR"]->setPixel(x, y, _surfaces["BACK06.SCR"]->getPixel(x, y + (y == 79 ? 2 : 1)));
			}
	}

	// now, let's adjust the battlescape info screen.
	int startHere = _manaEnabled ? 191 : 190;
	int stopHere = _manaEnabled ? 28 : 37;
	bool moveDown = _manaEnabled ? false : true;

	// erase the old lines, no need to worry about dithering on this one.
	for (int y = 39; y < 199; y += 10)
		for (int x = 0; x < 169; ++x)
			_surfaces["UNIBORD.PCK"]->setPixel(x, y, _surfaces["UNIBORD.PCK"]->getPixel(x, 30));
	// drawn new lines, use the bottom row of pixels as a basis
	for (int y = startHere; y > stopHere; y -= 9)
		for (int x = 0; x < 169; ++x)
			_surfaces["UNIBORD.PCK"]->setPixel(x, y, _surfaces["UNIBORD.PCK"]->getPixel(x, 199));
	// move the top of the graph down by eight pixels to erase the row we don't need (we actually created ~1.8 extra rows earlier)
	if (moveDown)
	{
		for (int y = 37; y > 29; --y)
			for (int x = 0; x < 320; ++x)
			{
				_surfaces["UNIBORD.PCK"]->setPixel(x, y, _surfaces["UNIBORD.PCK"]->getPixel(x, y - 8));
				_surfaces["UNIBORD.PCK"]->setPixel(x, y - 8, 0);
			}
	}
	else
	{
		// remove bottom line of the (entire) last row
		for (int x = 0; x < 320; ++x)
			_surfaces["UNIBORD.PCK"]->setPixel(x, 199, _surfaces["UNIBORD.PCK"]->getPixel(x, 30));
	}
}

/**
 * Loads the specified music file format.
 * @param fmt Format of the music.
 * @param rule Parameters of the music.
 * @param adlibcat Pointer to ADLIB.CAT if available.
 * @param aintrocat Pointer to AINTRO.CAT if available.
 * @param gmcat Pointer to GM.CAT if available.
 * @return Pointer to the music file, or NULL if it couldn't be loaded.
 */
Music* Mod::loadMusic(MusicFormat fmt, RuleMusic* rule, CatFile* adlibcat, CatFile* aintrocat, GMCatFile* gmcat) const
{
	/* MUSIC_AUTO, MUSIC_FLAC, MUSIC_OGG, MUSIC_MP3, MUSIC_MOD, MUSIC_WAV, MUSIC_ADLIB, MUSIC_GM, MUSIC_MIDI */
	static const std::string exts[] = { "", ".flac", ".ogg", ".mp3", ".mod", ".wav", "", "", ".mid" };
	Music *music = 0;
	const auto& soundContents = FileMap::getVFolderContents("SOUND");
	size_t track = rule->getCatPos();
	try
	{
		// Try Adlib music
		if (fmt == MUSIC_ADLIB)
		{
			if (adlibcat && Options::audioBitDepth == 16)
			{
				if (track < adlibcat->size())
				{
					music = new AdlibMusic(rule->getNormalization());
					music->load(adlibcat->getRWops(track));
				}
				// separate intro music
				else if (aintrocat)
				{
					track -= adlibcat->size();
					if (track < aintrocat->size())
					{
						music = new AdlibMusic(rule->getNormalization());
						music->load(aintrocat->getRWops(track));
					}
					else
					{
						delete music;
						music = 0;
					}
				}
			}
		}
		// Try MIDI music (from GM.CAT)
		else if (fmt == MUSIC_GM)
		{
			// DOS MIDI
			if (gmcat && track < gmcat->size())
			{
				music = gmcat->loadMIDI(track);
			}
		}
		// Try digital tracks
		else
		{
			std::string fname = rule->getName() + exts[fmt];
			std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);

			if (soundContents.find(fname) != soundContents.end())
			{
				music = new Music();
				music->load("SOUND/" + fname);
			}
		}
	}
	catch (Exception &e)
	{
		Log(LOG_INFO) << e.what();
		if (music) delete music;
		music = 0;
	}
	return music;
}

/**
 * Preamble:
 * this is the most horrible function i've ever written, and it makes me sad.
 * this is, however, a necessary evil, in order to save massive amounts of time in the draw function.
 * when used with the default TFTD mod, this function loops 4,194,304 times
 * (4 palettes, 4 tints, 4 levels of opacity, 256 colors, 256 comparisons per)
 * each additional tint in the rulesets will result in over a million iterations more.
 * @param pal the palette to base the lookup table on.
 */
void Mod::createTransparencyLUT(Palette *pal)
{
	const SDL_Color* palColors = pal->getColors(0);
	std::vector<Uint8> lookUpTable;
	// start with the color sets
	lookUpTable.reserve(_transparencies.size() * TransparenciesPaletteColors * TransparenciesOpacityLevels);
	for (const auto& tintLevels : _transparencies)
	{
		// then the opacity levels, using the alpha channel as the step
		for (const SDL_Color& tint : tintLevels)
		{
			// then the palette itself
			for (int currentColor = 0; currentColor < TransparenciesPaletteColors; ++currentColor)
			{
				SDL_Color desiredColor;

				desiredColor.r = std::min(255, (palColors[currentColor].r * tint.unused / 255) + tint.r);
				desiredColor.g = std::min(255, (palColors[currentColor].g * tint.unused / 255) + tint.g);
				desiredColor.b = std::min(255, (palColors[currentColor].b * tint.unused / 255) + tint.b);

				Uint8 closest = currentColor;
				int lowestDifference = INT_MAX;
				// if opacity is zero then we stay with current color, transparent color will stay same too
				if (tint.unused != 0 && currentColor != 0)
				{
					// now compare each color in the palette to find the closest match to our desired one
					for (int comparator = 1; comparator < TransparenciesPaletteColors; ++comparator)
					{
						int currentDifference = Sqr(desiredColor.r - palColors[comparator].r) +
							Sqr(desiredColor.g - palColors[comparator].g) +
							Sqr(desiredColor.b - palColors[comparator].b);

						if (currentDifference < lowestDifference)
						{
							closest = comparator;
							lowestDifference = currentDifference;
						}
					}
				}
				lookUpTable.push_back(closest);
			}
		}
	}
	_transparencyLUTs.push_back(std::move(lookUpTable));
}

StatAdjustment *Mod::getStatAdjustment(int difficulty)
{
	if ((size_t)difficulty >= MaxDifficultyLevels)
	{
		return &_statAdjustment[MaxDifficultyLevels - 1];
	}
	return &_statAdjustment[difficulty];
}

/**
 * Returns the minimum amount of score the player can have,
 * otherwise they are defeated. Changes based on difficulty.
 * @return Score.
 */
int Mod::getDefeatScore() const
{
	return _defeatScore;
}

/**
 * Returns the minimum amount of funds the player can have,
 * otherwise they are defeated.
 * @return Funds.
 */
int Mod::getDefeatFunds() const
{
	return _defeatFunds;
}

/**
 * Enables non-vanilla difficulty features.
 * Dehumanize yourself and face the Warboy.
 * @return Is the player screwed?
*/
bool Mod::isDemigod() const
{
	return _difficultyDemigod;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

template<size_t Mod::*f>
void offset(const Mod *m, int &base, int modId)
{
	int baseMax = (m->*f);
	if (base >= baseMax)
	{
		base += modId;
	}
}

void getSmokeReduction(const Mod *m, int &smoke)
{
	// initial smoke "density" of a smoke grenade is around 15 per tile
	// we do density/3 to get the decay of visibility
	// so in fresh smoke we should only have 4 tiles of visibility
	// this is traced in voxel space, with smoke affecting visibility every step of the way

	// 3  - coefficient of calculation (see above).
	// 20 - maximum view distance in vanilla Xcom.
	// Even if MaxViewDistance will be increased via ruleset, smoke will keep effect.
	smoke = smoke * m->getMaxViewDistance() / (3 * 20);
}

void getUnitScript(const Mod* mod, const Unit* &unit, const std::string &name)
{
	if (mod)
	{
		unit = mod->getUnit(name);
	}
	else
	{
		unit = nullptr;
	}
}
void getArmorScript(const Mod* mod, const Armor* &armor, const std::string &name)
{
	if (mod)
	{
		armor = mod->getArmor(name);
	}
	else
	{
		armor = nullptr;
	}
}
void getItemScript(const Mod* mod, const RuleItem* &item, const std::string &name)
{
	if (mod)
	{
		item = mod->getItem(name);
	}
	else
	{
		item = nullptr;
	}
}
void getSkillScript(const Mod* mod, const RuleSkill* &skill, const std::string &name)
{
	if (mod)
	{
		skill = mod->getSkill(name);
	}
	else
	{
		skill = nullptr;
	}
}
void getRuleResearch(const Mod* mod, const RuleResearch*& rule, const std::string& name)
{
	if (mod)
	{
		rule = mod->getResearch(name);
	}
	else
	{
		rule = nullptr;
	}
}
void getSoldierScript(const Mod* mod, const RuleSoldier* &soldier, const std::string &name)
{
	if (mod)
	{
		soldier = mod->getSoldier(name);
	}
	else
	{
		soldier = nullptr;
	}
}
void getInventoryScript(const Mod* mod, const RuleInventory* &inv, const std::string &name)
{
	if (mod)
	{
		inv = mod->getInventory(name);
	}
	else
	{
		inv = nullptr;
	}
}

} // namespace

/**
 * Register all useful function used by script.
 */
void Mod::ScriptRegister(ScriptParserBase *parser)
{
	parser->registerPointerType<Unit>();
	parser->registerPointerType<RuleItem>();
	parser->registerPointerType<Armor>();
	parser->registerPointerType<RuleSkill>();
	parser->registerPointerType<RuleResearch>();
	parser->registerPointerType<RuleSoldier>();
	parser->registerPointerType<RuleInventory>();

	Bind<Mod> mod = { parser };

	mod.add<&offset<&Mod::_soundOffsetBattle>>("getSoundOffsetBattle", "convert mod sound index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&offset<&Mod::_soundOffsetGeo>>("getSoundOffsetGeo", "convert mod sound index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&offset<&Mod::_surfaceOffsetBasebits>>("getSpriteOffsetBasebits", "convert mod surface index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&offset<&Mod::_surfaceOffsetBigobs>>("getSpriteOffsetBigobs", "convert mod surface index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&offset<&Mod::_surfaceOffsetFloorob>>("getSpriteOffsetFloorob", "convert mod surface index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&offset<&Mod::_surfaceOffsetHandob>>("getSpriteOffsetHandob", "convert mod surface index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&offset<&Mod::_surfaceOffsetHit>>("getSpriteOffsetHit", "convert mod surface index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&offset<&Mod::_surfaceOffsetSmoke>>("getSpriteOffsetSmoke", "convert mod surface index in first argument to runtime index in given set, second argument is mod id");
	mod.add<&Mod::getMaxDarknessToSeeUnits>("getMaxDarknessToSeeUnits");
	mod.add<&Mod::getMaxViewDistance>("getMaxViewDistance");
	mod.add<&getSmokeReduction>("getSmokeReduction");

	mod.add<&getUnitScript>("getRuleUnit");
	mod.add<&getItemScript>("getRuleItem");
	mod.add<&getArmorScript>("getRuleArmor");
	mod.add<&getSkillScript>("getRuleSkill");
	mod.add<&getRuleResearch>("getRuleResearch");
	mod.add<&getSoldierScript>("getRuleSoldier");
	mod.add<&getInventoryScript>("getRuleInventory");
	mod.add<&Mod::getInventoryRightHand>("getRuleInventoryRightHand");
	mod.add<&Mod::getInventoryLeftHand>("getRuleInventoryLeftHand");
	mod.add<&Mod::getInventoryBackpack>("getRuleInventoryBackpack");
	mod.add<&Mod::getInventoryBelt>("getRuleInventoryBelt");
	mod.add<&Mod::getInventoryGround>("getRuleInventoryGround");

	mod.addScriptValue<&Mod::_scriptGlobal, &ModScriptGlobal::getScriptValues>();
}


#ifdef OXCE_AUTO_TEST

static auto dummyParseDate = ([]
{
	assert(OxceVersionDate(OPENXCOM_VERSION_GIT));
	assert(OxceVersionDate(" (v1976-04-23)"));
	assert(OxceVersionDate(" (v9999-99-99)")); //accept impossible dates
	assert(OxceVersionDate(" (v   6-04-23)"));
	assert(OxceVersionDate(" (v   1- 1- 1)"));

	assert(!OxceVersionDate(" (v21976-04-23)"));
	assert(!OxceVersionDate(" (v1976-034-22)"));
	assert(!OxceVersionDate(" (v1976-04-232)"));
	assert(!OxceVersionDate(" (v1976-b4-23)"));

	assert(!OxceVersionDate(""));
	assert(!OxceVersionDate(" (v"));
	assert(!OxceVersionDate(" (v)"));
	assert(!OxceVersionDate(" (v 1976-04-23)"));
	assert(!OxceVersionDate(" (v1976- 04-23)"));
	assert(!OxceVersionDate(" (v1976-04- 23)"));
	assert(!OxceVersionDate(" (v1976-04-23 )"));
	assert(!OxceVersionDate(" (v    -  -  )"));
	assert(!OxceVersionDate(" (v   0- 0- 0)"));
	assert(!OxceVersionDate(" (v 1 1- 1- 1)"));

	{
		OxceVersionDate d("   (v1976-04-23)");
		assert(d && d.year == 1976 && d.month == 04 && d.day == 23);
	}

	{
		OxceVersionDate d("   (v1976-04-22)    ");
		assert(d && d.year == 1976 && d.month == 04 && d.day == 22);
	}

	{
		OxceVersionDate d(" aaads  (v1976-04-22)  sdafdfsfsd  ");
		assert(d && d.year == 1976 && d.month == 04 && d.day == 22);
	}

	return 0;
})();
#endif

}
