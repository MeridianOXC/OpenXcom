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
#include <sstream>
#include <climits>
#include <cassert>
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
int Mod::DIFFICULTY_BASED_RETAL_DELAY[5];
int Mod::UNIT_RESPONSE_SOUNDS_FREQUENCY[4];
bool Mod::EXTENDED_ITEM_RELOAD_COST;
bool Mod::EXTENDED_RUNNING_COST;
bool Mod::EXTENDED_HWP_LOAD_ORDER;
int Mod::EXTENDED_MELEE_REACTIONS;
int Mod::EXTENDED_TERRAIN_MELEE;
int Mod::EXTENDED_UNDERWATER_THROW_FACTOR;

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

	DIFFICULTY_BASED_RETAL_DELAY[0] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[1] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[2] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[3] = 0;
	DIFFICULTY_BASED_RETAL_DELAY[4] = 0;

	UNIT_RESPONSE_SOUNDS_FREQUENCY[0] = 100; // select unit
	UNIT_RESPONSE_SOUNDS_FREQUENCY[1] = 100; // start moving
	UNIT_RESPONSE_SOUNDS_FREQUENCY[2] = 100; // select weapon
	UNIT_RESPONSE_SOUNDS_FREQUENCY[3] = 20;  // annoyed

	EXTENDED_ITEM_RELOAD_COST = false;
	EXTENDED_RUNNING_COST = false;
	EXTENDED_HWP_LOAD_ORDER = false;
	EXTENDED_MELEE_REACTIONS = 0;
	EXTENDED_TERRAIN_MELEE = 0;
	EXTENDED_UNDERWATER_THROW_FACTOR = 0;
}

/**
 * Detail
 */
class ModScriptGlobal : public ScriptGlobal
{
	size_t _modCurr = 0;
	std::vector<std::pair<std::string, int>> _modNames;
	ScriptValues<Mod> _scriptValues;

	void loadRuleList(int &value, const YAML::Node &node) const
	{
		if (node)
		{
			auto name = node.as<std::string>();
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
	void saveRuleList(const int &value, YAML::Node &node) const
	{
		for (const auto& p : _modNames)
		{
			if (value == p.second)
			{
				node = p.first;
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
	_aiUseDelayBlaster(3), _aiUseDelayFirearm(0), _aiUseDelayGrenade(3), _aiUseDelayMelee(0), _aiUseDelayPsionic(0),
	_aiFireChoiceIntelCoeff(5), _aiFireChoiceAggroCoeff(5), _aiExtendedFireModeChoice(false), _aiRespectMaxRange(false), _aiDestroyBaseFacilities(false),
	_aiPickUpWeaponsMoreActively(false), _aiPickUpWeaponsMoreActivelyCiv(false),
	_maxLookVariant(0), _tooMuchSmokeThreshold(10), _customTrainingFactor(100), _minReactionAccuracy(0), _chanceToStopRetaliation(0), _lessAliensDuringBaseDefense(false),
	_allowCountriesToCancelAlienPact(false), _buildInfiltrationBaseCloseToTheCountry(false), _allowAlienBasesOnWrongTextures(true),
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
	_soldiersPerSergeant(5), _soldiersPerCaptain(11), _soldiersPerColonel(23), _soldiersPerCommander(30),
	_pilotAccuracyZeroPoint(55), _pilotAccuracyRange(40), _pilotReactionsZeroPoint(55), _pilotReactionsRange(60),
	_performanceBonusFactor(0), _enableNewResearchSorting(false), _displayCustomCategories(0), _shareAmmoCategories(false), _showDogfightDistanceInKm(false), _showFullNameInAlienInventory(false),
	_alienInventoryOffsetX(80), _alienInventoryOffsetBigUnit(32),
	_hidePediaInfoButton(false), _extraNerdyPediaInfoType(0),
	_giveScoreAlsoForResearchedArtifacts(false), _statisticalBulletConservation(false), _stunningImprovesMorale(false),
	_tuRecoveryWakeUpNewTurn(100), _shortRadarRange(0), _buildTimeReductionScaling(100),
	_defeatScore(0), _defeatFunds(0), _difficultyDemigod(false), _startingTime(6, 1, 1, 1999, 12, 0, 0), _startingDifficulty(0),
	_baseDefenseMapFromLocation(0), _disableUnderwaterSounds(false), _enableUnitResponseSounds(false), _pediaReplaceCraftFuelWithRangeType(-1),
	_facilityListOrder(0), _craftListOrder(0), _itemCategoryListOrder(0), _itemListOrder(0),
	_researchListOrder(0),  _manufactureListOrder(0), _soldierBonusListOrder(0), _transformationListOrder(0), _ufopaediaListOrder(0), _invListOrder(0), _soldierListOrder(0),
	_modCurrent(0), _statePalette(0)
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
	_statAdjustment[0].growthMultiplier = 0;
	for (size_t i = 1; i != MaxDifficultyLevels; ++i)
	{
		_statAdjustment[i].aimMultiplier = 1.0;
		_statAdjustment[i].armorMultiplier = 1.0;
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
	for (std::map<std::string, Font*>::iterator i = _fonts.begin(); i != _fonts.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, SurfaceSet*>::iterator i = _sets.begin(); i != _sets.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, Palette*>::iterator i = _palettes.begin(); i != _palettes.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, Music*>::iterator i = _musics.begin(); i != _musics.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, SoundSet*>::iterator i = _sounds.begin(); i != _sounds.end(); ++i)
	{
		delete i->second;
	}
	for (std::vector<RuleDamageType*>::iterator i = _damageTypes.begin(); i != _damageTypes.end(); ++i)
	{
		delete *i;
	}
	for (std::map<std::string, RuleCountry*>::iterator i = _countries.begin(); i != _countries.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleCountry*>::iterator i = _extraGlobeLabels.begin(); i != _extraGlobeLabels.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleRegion*>::iterator i = _regions.begin(); i != _regions.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleBaseFacility*>::iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleCraft*>::iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleCraftWeapon*>::iterator i = _craftWeapons.begin(); i != _craftWeapons.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleItemCategory*>::iterator i = _itemCategories.begin(); i != _itemCategories.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleItem*>::iterator i = _items.begin(); i != _items.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleUfo*>::iterator i = _ufos.begin(); i != _ufos.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleTerrain*>::iterator i = _terrains.begin(); i != _terrains.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, MapDataSet*>::iterator i = _mapDataSets.begin(); i != _mapDataSets.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleSoldier*>::iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleSkill*>::iterator i = _skills.begin(); i != _skills.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, Unit*>::iterator i = _units.begin(); i != _units.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, AlienRace*>::iterator i = _alienRaces.begin(); i != _alienRaces.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleEnviroEffects*>::iterator i = _enviroEffects.begin(); i != _enviroEffects.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleStartingCondition*>::iterator i = _startingConditions.begin(); i != _startingConditions.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, AlienDeployment*>::iterator i = _alienDeployments.begin(); i != _alienDeployments.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, Armor*>::iterator i = _armors.begin(); i != _armors.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, ArticleDefinition*>::iterator i = _ufopaediaArticles.begin(); i != _ufopaediaArticles.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleInventory*>::iterator i = _invs.begin(); i != _invs.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleResearch *>::const_iterator i = _research.begin(); i != _research.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleManufacture *>::const_iterator i = _manufacture.begin(); i != _manufacture.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleManufactureShortcut *>::const_iterator i = _manufactureShortcut.begin(); i != _manufactureShortcut.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleSoldierBonus *>::const_iterator i = _soldierBonus.begin(); i != _soldierBonus.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleSoldierTransformation *>::const_iterator i = _soldierTransformation.begin(); i != _soldierTransformation.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, UfoTrajectory *>::const_iterator i = _ufoTrajectories.begin(); i != _ufoTrajectories.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleAlienMission *>::const_iterator i = _alienMissions.begin(); i != _alienMissions.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, MCDPatch *>::const_iterator i = _MCDPatches.begin(); i != _MCDPatches.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, std::vector<ExtraSprites*> >::iterator i = _extraSprites.begin(); i != _extraSprites.end(); ++i)
	{
		for (std::vector<ExtraSprites*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			delete *j;
		}
	}
	for (std::map<std::string, CustomPalettes *>::const_iterator i = _customPalettes.begin(); i != _customPalettes.end(); ++i)
	{
		delete i->second;
	}
	for (std::vector< std::pair<std::string, ExtraSounds *> >::const_iterator i = _extraSounds.begin(); i != _extraSounds.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, ExtraStrings *>::const_iterator i = _extraStrings.begin(); i != _extraStrings.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleInterface *>::const_iterator i = _interfaces.begin(); i != _interfaces.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, std::vector<MapScript*> >::iterator i = _mapScripts.begin(); i != _mapScripts.end(); ++i)
	{
		for (std::vector<MapScript*>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
		{
			delete *j;
		}
	}
	for (std::map<std::string, RuleVideo *>::const_iterator i = _videos.begin(); i != _videos.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleMusic *>::const_iterator i = _musicDefs.begin(); i != _musicDefs.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleArcScript*>::const_iterator i = _arcScripts.begin(); i != _arcScripts.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleEventScript*>::const_iterator i = _eventScripts.begin(); i != _eventScripts.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleEvent*>::const_iterator i = _events.begin(); i != _events.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, RuleMissionScript*>::const_iterator i = _missionScripts.begin(); i != _missionScripts.end(); ++i)
	{
		delete i->second;
	}
	for (std::map<std::string, SoundDefinition*>::const_iterator i = _soundDefs.begin(); i != _soundDefs.end(); ++i)
	{
		delete i->second;
	}
	for (std::vector<StatString*>::const_iterator i = _statStrings.begin(); i != _statStrings.end(); ++i)
	{
		delete (*i);
	}
	for (std::map<std::string, RuleCommendations *>::const_iterator i = _commendations.begin(); i != _commendations.end(); ++i)
	{
		delete i->second;
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
		std::map<std::string, std::vector<ExtraSprites *> >::const_iterator i = _extraSprites.find(name);
		if (i != _extraSprites.end())
		{
			for (std::vector<ExtraSprites*>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				loadExtraSprite(*j);
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
		for (std::map<std::string, Music*>::const_iterator i = _musics.begin(); i != _musics.end(); ++i)
		{
			if (i->first.find(name) != std::string::npos)
			{
				music.push_back(i->second);
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

	checkForSoftError(sprite != Mod::NO_SURFACE && s->getFrame(sprite) == nullptr, parent, "Wrong index " + std::to_string(sprite) + " for surface set " + set, LOG_ERROR);
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

	for (auto sprite : sprites)
	{
		checkForSoftError(sprite != Mod::NO_SURFACE && s->getFrame(sprite) == nullptr, parent, "Wrong index " + std::to_string(sprite) + " for surface set " + set, LOG_ERROR);
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

	checkForSoftError(sound != Mod::NO_SOUND && s->getSound(sound) == nullptr, parent, "Wrong index " + std::to_string(sound) + " for sound set " + set, LOG_ERROR);
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

	for (auto sound : sounds)
	{
		checkForSoftError(sound != Mod::NO_SOUND && s->getSound(sound) == nullptr, parent, "Wrong index " + std::to_string(sound) + " for sound set " + set, LOG_ERROR);
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

const std::string YamlTagSeqShort = "!!seq";
const std::string YamlTagSeq = "tag:yaml.org,2002:seq";
const std::string YamlTagMapShort = "!!map";
const std::string YamlTagMap = "tag:yaml.org,2002:map";
const std::string YamlTagNonSpecific = "?";

const std::string InfoTag = "!info";
const std::string AddTag = "!add";
const std::string RemoveTag = "!remove";

bool isListHelper(const YAML::Node &node)
{
	return node.IsSequence() == true && (node.Tag() == YamlTagSeq || node.Tag() == YamlTagNonSpecific || node.Tag() == InfoTag);
}

bool isListAddTagHelper(const YAML::Node &node)
{
	return node.IsSequence() == true && node.Tag() == AddTag;
}

bool isListRemoveTagHelper(const YAML::Node &node)
{
	return node.IsSequence() == true && node.Tag() == RemoveTag;
}

bool isMapHelper(const YAML::Node &node)
{
	return node.IsMap() == true && (node.Tag() == YamlTagMap || node.Tag() == YamlTagNonSpecific || node.Tag() == InfoTag);
}

bool isMapAddTagHelper(const YAML::Node &node)
{
	return node.IsMap() == true && node.Tag() == AddTag;
}

void throwOnBadListHelper(const std::string &parent, const YAML::Node &node)
{
	std::ostringstream err;
	if (node.IsSequence())
	{
		// it is a sequence, but it could not be loaded... this means the tag is not supported
		err << "unsupported node tag '" << node.Tag() << "'";
	}
	else
	{
		err << "wrong node type, expected a list";
	}
	throw LoadRuleException(parent, node, err.str());
}

void throwOnBadMapHelper(const std::string &parent, const YAML::Node &node)
{
	std::ostringstream err;
	if (node.IsMap())
	{
		// it is a map, but it could not be loaded... this means the tag is not supported
		err << "unsupported node tag '" << node.Tag() << "'";
	}
	else
	{
		err << "wrong node type, expected a map";
	}
	throw LoadRuleException(parent, node, err.str());
}

template<typename... T>
void showInfo(const std::string &parent, const YAML::Node &node, T... names)
{
	if (node.Tag() == InfoTag)
	{
		Logger info;
		info.get() << "Options available for " << parent << " at line " << node.Mark().line << " are: ";
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
void loadHelper(const std::string &parent, int& v, const YAML::Node &node)
{
	v = node.as<int>();
}

/**
 * Terminal function loading string.
 * Function can't load empty string.
 */
void loadHelper(const std::string &parent, std::string& v, const YAML::Node &node)
{
	v = node.as<std::string>();
	if (Mod::isEmptyRuleName(v))
	{
		throw LoadRuleException(parent, node, "Invalid value for name");
	}
}

/**
 * Function loading string.
 * If node do not exists then it do not change value.
 * Function can't load empty string.
 */
void loadHelper(const std::string &parent, std::string& v, const YAML::Node &node, LoadFuncStandard)
{
	if (node)
	{
		loadHelper(parent, v, node);
	}
}

/**
 * Function loading string with option for pseudo null value.
 * If node do not exists then it do not change value.
 */
void loadHelper(const std::string &parent, std::string& v, const YAML::Node &node, LoadFuncNullable)
{
	if (node)
	{
		if (node.IsNull())
		{
			v = Mod::STR_NULL;
		}
		else
		{
			v = node.as<std::string>();
			if (v == Mod::STR_NULL)
			{
				throw LoadRuleException(parent, node, "Invalid value for name ");
			}
		}
	}
}

template<typename T, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::vector<T>& v, const YAML::Node &node, LoadFuncStandard, LoadFuncTag... rest)
{
	if (node)
	{
		showInfo(parent, node, YamlTagSeqShort);

		if (isListHelper(node))
		{
			v.clear();
			v.reserve(node.size());
			for (const YAML::Node& n : node)
			{
				loadHelper(parent, v.emplace_back(), n, rest.funcTagForNew()...);
			}
		}
		else
		{
			throwOnBadListHelper(parent, node);
		}
	}
}

template<typename T, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::vector<T>& v, const YAML::Node &node, LoadFuncEditable, LoadFuncTag... rest)
{
	if (node)
	{
		showInfo(parent, node, YamlTagSeqShort, AddTag, RemoveTag);

		if (isListHelper(node))
		{
			v.clear();
			v.reserve(node.size());
			for (const YAML::Node& n : node)
			{
				loadHelper(parent, v.emplace_back(), n, rest.funcTagForNew()...);
			}
		}
		else if (isListAddTagHelper(node))
		{
			v.reserve(v.size() + node.size());
			for (const YAML::Node& n : node)
			{
				loadHelper(parent, v.emplace_back(), n, rest...);
			}
		}
		else if (isListRemoveTagHelper(node))
		{
			const auto begin = v.begin();
			auto end = v.end();
			for (const YAML::Node& n : node)
			{
				end = std::remove(begin, end, n.as<T>());
			}
			v.erase(end, v.end());
		}
		else
		{
			throwOnBadListHelper(parent, node);
		}
	}
}

template<typename K, typename V, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::map<K, V>& v, const YAML::Node &node, LoadFuncStandard, LoadFuncTag... rest)
{
	if (node)
	{
		showInfo(parent, node, YamlTagMapShort);

		if (isMapHelper(node))
		{
			v.clear();
			for (const std::pair<YAML::Node, YAML::Node>& n : node)
			{
				auto key = n.first.as<K>();

				loadHelper(parent, v[key], n.second, rest.funcTagForNew()...);
			}
		}
		else
		{
			throwOnBadMapHelper(parent, node);
		}
	}
}

template<typename K, typename V, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::map<K, V>& v, const YAML::Node &node, LoadFuncEditable, LoadFuncTag... rest)
{
	if (node)
	{
		showInfo(parent, node, YamlTagMapShort, AddTag, RemoveTag);

		if (isMapHelper(node))
		{
			v.clear();
			for (const std::pair<YAML::Node, YAML::Node>& n : node)
			{
				auto key = n.first.as<K>();

				loadHelper(parent, v[key], n.second, rest.funcTagForNew()...);
			}
		}
		else if (isMapAddTagHelper(node))
		{
			for (const std::pair<YAML::Node, YAML::Node>& n : node)
			{
				auto key = n.first.as<K>();

				loadHelper(parent, v[key], n.second, rest...);
			}
		}
		else if (isListRemoveTagHelper(node)) // we use a list here as we only need the keys
		{
			for (const YAML::Node& n : node)
			{
				v.erase(n.as<K>());
			}
		}
		else
		{
			throwOnBadMapHelper(parent, node);
		}
	}
}

/**
 * Fixed order map, rely on fact that yaml-cpp try preserve map order from loaded file
 */
template<typename K, typename V, typename... LoadFuncTag>
void loadHelper(const std::string &parent, std::vector<std::pair<K, V>>& v, const YAML::Node &node, LoadFuncEditable, LoadFuncTag... rest)
{
	if (node)
	{
		showInfo(parent, node, YamlTagMapShort, AddTag, RemoveTag);

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

		if (isMapHelper(node))
		{
			v.clear();
			for (const std::pair<YAML::Node, YAML::Node>& n : node)
			{
				auto key = n.first.as<K>();

				loadHelper(parent, pushBack(key), n.second, rest.funcTagForNew()...);
			}
		}
		else if (isMapAddTagHelper(node))
		{
			for (const std::pair<YAML::Node, YAML::Node>& n : node)
			{
				auto key = n.first.as<K>();

				loadHelper(parent, findOrPushBack(key), n.second, rest...);
			}
		}
		else if (isListRemoveTagHelper(node)) // we use a list here as we only need the keys
		{
			for (const YAML::Node& n : node)
			{
				auto key = n.as<K>();
				Collections::removeIf(v, [&](auto& p){ return p.first == key; });
			}
		}
		else
		{
			throwOnBadMapHelper(parent, node);
		}
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
void Mod::loadOffsetNode(const std::string &parent, int& offset, const YAML::Node &node, int shared, const std::string &set, size_t multiplier, size_t sizeScale) const
{
	assert(_modCurrent);
	const ModData* curr = _modCurrent;
	if (node.IsScalar())
	{
		offset = node.as<int>();
	}
	else if (isMapHelper(node))
	{
		offset = node["index"].as<int>();
		std::string mod = node["mod"].as<std::string>();
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
				throw LoadRuleException(parent, node, err.str());
			}
		}
	}
	else
	{
		throw LoadRuleException(parent, node, "unsupported yaml node");
	}

	static_assert(Mod::NO_SOUND == -1, "NO_SOUND need to equal -1");
	static_assert(Mod::NO_SURFACE == -1, "NO_SURFACE need to equal -1");

	if (offset < -1)
	{
		std::ostringstream err;
		err << "offset '" << offset << "' has incorrect value in set '" << set << "'";
		throw LoadRuleException(parent, node, err.str());
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
			throw LoadRuleException(parent, node, err.str());
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
void Mod::loadSpriteOffset(const std::string &parent, int& sprite, const YAML::Node &node, const std::string &set, size_t multiplier) const
{
	if (node)
	{
		loadOffsetNode(parent, sprite, node, getRule(set, "Sprite Set", _sets, true)->getMaxSharedFrames(), set, multiplier);
	}
}

/**
 * Gets the mod offset array for a certain sprite.
 * @param parent Name of parent node, used for better error message
 * @param sprites Member to load new array of sprite ID index.
 * @param node Node with data
 * @param set Name of the surfaceset to lookup.
 */
void Mod::loadSpriteOffset(const std::string &parent, std::vector<int>& sprites, const YAML::Node &node, const std::string &set) const
{
	if (node)
	{
		int maxShared = getRule(set, "Sprite Set", _sets, true)->getMaxSharedFrames();
		sprites.clear();
		if (isListHelper(node))
		{
			for (YAML::const_iterator i = node.begin(); i != node.end(); ++i)
			{
				sprites.push_back(Mod::NO_SURFACE);
				loadOffsetNode(parent, sprites.back(), *i, maxShared, set, 1);
				if (checkForSoftError(sprites.back() == Mod::NO_SURFACE, parent, *i, "incorrect value in sprite list"))
				{
					sprites.pop_back();
				}
			}
		}
		else
		{
			sprites.push_back(Mod::NO_SURFACE);
			loadOffsetNode(parent, sprites.back(), node, maxShared, set, 1);
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
void Mod::loadSoundOffset(const std::string &parent, int& sound, const YAML::Node &node, const std::string &set) const
{
	if (node)
	{
		loadOffsetNode(parent, sound, node, getSoundSet(set)->getMaxSharedSounds(), set, 1);
	}
}

/**
 * Gets the mod offset array for a certain sound.
 * @param parent Name of parent node, used for better error message
 * @param sounds Member to load new list of sound ID indexes.
 * @param node Node with data
 * @param set Name of the soundset to lookup.
 */
void Mod::loadSoundOffset(const std::string &parent, std::vector<int>& sounds, const YAML::Node &node, const std::string &set) const
{
	if (node)
	{
		int maxShared = getSoundSet(set)->getMaxSharedSounds();
		sounds.clear();
		if (isListHelper(node))
		{
			for (YAML::const_iterator i = node.begin(); i != node.end(); ++i)
			{
				sounds.push_back(Mod::NO_SOUND);
				loadOffsetNode(parent, sounds.back(), *i, maxShared, set, 1);
				if (checkForSoftError(sounds.back() == Mod::NO_SOUND, parent, *i, "incorrect value in sound list"))
				{
					sounds.pop_back();
				}
			}
		}
		else
		{
			sounds.push_back(Mod::NO_SOUND);
			loadOffsetNode(parent, sounds.back(), node, maxShared, set, 1);
		}
	}
}

/**
 * Gets the mod offset array for a certain transparency index.
 * @param parent Name of parent node, used for better error message.
 * @param index Member to load new transparency index.
 * @param node Node with data.
 */
void Mod::loadTransparencyOffset(const std::string &parent, int& index, const YAML::Node &node) const
{
	if (node)
	{
		loadOffsetNode(parent, index, node, 0, "TransparencyLUTs", 1, ModTransparencySizeReduction);
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
void Mod::loadBaseFunction(const std::string& parent, RuleBaseFacilityFunctions& f, const YAML::Node& node)
{
	if (node)
	{
		try
		{
			if (isListHelper(node))
			{
				f.reset();
				for (const YAML::Node& n : node)
				{
					f.set(_baseFunctionNames.addName(n.as<std::string>(), f.size()));
				}
			}
			else if (isListAddTagHelper(node))
			{
				for (const YAML::Node& n : node)
				{
					f.set(_baseFunctionNames.addName(n.as<std::string>(), f.size()));
				}
			}
			else if (isListRemoveTagHelper(node))
			{
				for (const YAML::Node& n : node)
				{
					f.set(_baseFunctionNames.addName(n.as<std::string>(), f.size()), false);
				}
			}
			else
			{
				throwOnBadListHelper(parent, node);
			}
		}
		catch(LoadRuleException& ex)
		{
			//context is already included in exception, no need add more
			throw;
		}
		catch(Exception& ex)
		{
			throw LoadRuleException(parent, node, ex.what());
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
void Mod::loadInts(const std::string &parent, std::vector<int>& ints, const YAML::Node &node) const
{
	loadHelper(parent, ints, node, LoadFuncStandard{});
}

/**
 * Loads a list of ints where order of items does not matter.
 * Another mod can remove or add new values without altering the whole list.
 */
void Mod::loadUnorderedInts(const std::string &parent, std::vector<int>& ints, const YAML::Node &node) const
{
	loadHelper(parent, ints, node, LoadFuncEditable{});
}


/**
 * Loads a name.
 */
void Mod::loadName(const std::string &parent, std::string& name, const YAML::Node &node) const
{
	loadHelper(parent, name, node, LoadFuncStandard{});
}

/**
 * Loads a name. Have option of loading null `~` as special string value.
 */
void Mod::loadNameNull(const std::string &parent, std::string& name, const YAML::Node &node) const
{
	loadHelper(parent, name, node, LoadFuncNullable{});
}

/**
 * Loads a list of names.
 * Another mod can only override the whole list, no partial edits allowed.
 */
void Mod::loadNames(const std::string &parent, std::vector<std::string>& names, const YAML::Node &node) const
{
	loadHelper(parent, names, node, LoadFuncStandard{});
}

/**
 * Loads a list of names where order of items does not matter.
 * Another mod can remove or add new values without altering the whole list.
 */
void Mod::loadUnorderedNames(const std::string &parent, std::vector<std::string>& names, const YAML::Node &node) const
{
	loadHelper(parent, names, node, LoadFuncEditable{});
}



/**
 * Loads a map from names to names.
 */
void Mod::loadNamesToNames(const std::string &parent, std::vector<std::pair<std::string, std::vector<std::string>>>& names, const YAML::Node &node) const
{
	loadHelper(parent, names, node, LoadFuncEditable{}, LoadFuncEditable{});
}

/**
 * Loads a map from names to names.
 */
void Mod::loadUnorderedNamesToNames(const std::string &parent, std::map<std::string, std::string>& names, const YAML::Node &node) const
{
	loadHelper(parent, names, node, LoadFuncEditable{});
}

/**
 * Loads a map from names to ints.
 */
void Mod::loadUnorderedNamesToInt(const std::string &parent, std::map<std::string, int>& names, const YAML::Node &node) const
{
	loadHelper(parent, names, node, LoadFuncEditable{});
}

/**
 * Loads a map from names to vector of ints.
 */
void Mod::loadUnorderedNamesToInts(const std::string &parent, std::map<std::string, std::vector<int>>& names, const YAML::Node &node) const
{
	loadHelper(parent, names, node, LoadFuncEditable{}, LoadFuncStandard{});
}

/**
 * Loads a map from names to names to int.
 */
void Mod::loadUnorderedNamesToNamesToInt(const std::string &parent, std::map<std::string, std::map<std::string, int>>& names, const YAML::Node &node) const
{
	loadHelper(parent, names, node, LoadFuncEditable{}, LoadFuncEditable{});
}

/**
 * Loads data for kill criteria from Commendations.
 */
void Mod::loadKillCriteria(const std::string &parent, std::vector<std::vector<std::pair<int, std::vector<std::string> > > >& v, const YAML::Node &node) const
{
	//TODO: very specific use case, not all levels fully supported
	if (node)
	{
		auto loadInner = [&](std::vector<std::pair<int, std::vector<std::string>>>& vv, const YAML::Node &n)
		{
			showInfo(parent, n, YamlTagSeqShort);

			if (isListHelper(n))
			{
				vv = n.as<std::vector<std::pair<int, std::vector<std::string>>>>();
			}
			else
			{
				throwOnBadListHelper(parent, n);
			}
		};

		showInfo(parent, node, YamlTagSeqShort, AddTag);

		if (isListHelper(node))
		{
			v.clear();
			v.reserve(node.size());
			for (const YAML::Node& n : node)
			{
				loadInner(v.emplace_back(), n);
			}
		}
		else if (isListAddTagHelper(node))
		{
			v.reserve(v.size() + node.size());
			for (const YAML::Node& n : node)
			{
				loadInner(v.emplace_back(), n);
			}
		}
		else
		{
			throwOnBadListHelper(parent, node);
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
		std::vector<std::pair<std::string, bool> >::iterator it =
			std::find(Options::mods.begin(), Options::mods.end(),
				std::pair<std::string, bool>(modId, true));
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
	auto mods = FileMap::getRulesets();

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
			auto file = FileMap::getModRuleFile(_modCurrent->info, _modCurrent->info->getResourceConfigFile());
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
	for (auto i = _itemCategories.begin(); i != _itemCategories.end(); ++i)
	{
		if (!i->second->getReplaceBy().empty())
		{
			replacementRules[i->first] = i->second->getReplaceBy();
		}
	}
	for (auto j = _items.begin(); j != _items.end(); ++j)
	{
		j->second->updateCategories(&replacementRules);
	}

	// find out if paperdoll overlaps with inventory slots
	int x1 = RuleInventory::PAPERDOLL_X;
	int y1 = RuleInventory::PAPERDOLL_Y;
	int w1 = RuleInventory::PAPERDOLL_W;
	int h1 = RuleInventory::PAPERDOLL_H;
	for (auto invCategory : _invs)
	{
		for (auto invSlot : *invCategory.second->getSlots())
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


	// check unique listOrder
	{
		std::vector<int> tmp;
		tmp.reserve(_soldierBonus.size());
		for (auto i : _soldierBonus)
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
	for (auto shortcutPair : _manufactureShortcut)
	{
		// 1. check if the new project has a unique name
		auto typeNew = shortcutPair.first;
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

		const std::vector<OptionInfo> &options = Options::getOptionInfo();
		for (std::vector<OptionInfo>::const_iterator i = options.begin(); i != options.end(); ++i)
		{
			if (i->type() != OPTION_KEY && !i->category().empty())
			{
				i->load(_recommendedUserOptions, false);
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

		const std::vector<OptionInfo> &options = Options::getOptionInfo();
		for (std::vector<OptionInfo>::const_iterator i = options.begin(); i != options.end(); ++i)
		{
			if (i->type() != OPTION_KEY && !i->category().empty())
			{
				i->load(_fixedUserOptions, false);
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
	for (auto i = rulesetFiles.begin(); i != rulesetFiles.end(); ++i)
	{
		Log(LOG_VERBOSE) << "- " << i->fullpath;
		try
		{
			loadFile(*i, parsers);
		}
		catch (Exception &e)
		{
			throw Exception(i->fullpath + ": " + std::string(e.what()));
		}
		catch (YAML::Exception &e)
		{
			throw Exception(i->fullpath + ": " + std::string(e.what()));
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
	for (std::map<std::string, RuleMissionScript*>::iterator i = _missionScripts.begin(); i != _missionScripts.end(); ++i)
	{
		RuleMissionScript *rule = (*i).second;
		std::set<std::string> missions = rule->getAllMissionTypes();
		if (!missions.empty())
		{
			std::set<std::string>::const_iterator j = missions.begin();
			if (!getAlienMission(*j))
			{
				throw Exception("Error with MissionScript: " + (*i).first + ": alien mission type: " + *j + " not defined, do not incite the judgement of Amaunator.");
			}
			bool isSiteType = getAlienMission(*j)->getObjective() == OBJECTIVE_SITE;
			rule->setSiteType(isSiteType);
			for (;j != missions.end(); ++j)
			{
				if (getAlienMission(*j) && (getAlienMission(*j)->getObjective() == OBJECTIVE_SITE) != isSiteType)
				{
					throw Exception("Error with MissionScript: " + (*i).first + ": cannot mix terror/non-terror missions in a single command, so sayeth the wise Alaundo.");
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
	for (std::map<std::string, RuleRegion*>::iterator i = _regions.begin(); i != _regions.end(); ++i)
	{
		// bleh, make copies, const correctness kinda screwed me here.
		WeightedOptions weights = (*i).second->getAvailableMissions();
		std::vector<std::string> names = weights.getNames();
		for (std::vector<std::string>::iterator j = names.begin(); j != names.end(); ++j)
		{
			if (!getAlienMission(*j))
			{
				throw Exception("Error with MissionWeights: Region: " + (*i).first + ": alien mission type: " + *j + " not defined, do not incite the judgement of Amaunator.");
			}
			if (getAlienMission(*j)->getObjective() == OBJECTIVE_SITE)
			{
				throw Exception("Error with MissionWeights: Region: " + (*i).first + " has " + *j + " listed. Terror mission can only be invoked via missionScript, so sayeth the Spider Queen.");
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
	YAML::Node doc = filerec.getYAML();

	for (YAML::const_iterator i = doc["soundDefs"].begin(); i != doc["soundDefs"].end(); ++i)
	{
		SoundDefinition *rule = loadRule(*i, &_soundDefs);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}

	if (const YAML::Node& luts = doc["transparencyLUTs"])
	{
		const size_t start = _modCurrent->offset / ModTransparencySizeReduction;
		const size_t limit =  _modCurrent->size / ModTransparencySizeReduction;
		size_t curr = 0;

		_transparencies.resize(start + limit);
		for (YAML::const_iterator i = luts.begin(); i != luts.end(); ++i)
		{
			const YAML::Node& c = (*i)["colors"];
			if (c.IsSequence())
			{
				for (YAML::const_iterator j = c.begin(); j != c.end(); ++j)
				{
					if (curr == limit)
					{
						throw Exception("transparencyLUTs mod limit reach");
					}
					SDL_Color color;
					color.r = (*j)[0].as<int>(0);
					color.g = (*j)[1].as<int>(0);
					color.b = (*j)[2].as<int>(0);
					color.unused = (*j)[3].as<int>(2);
					// technically it's a breaking change as it always overwrites from offset `start + 0` but no two mods could work correctly before this change.
					_transparencies[start + curr++] = color;
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
void Mod::loadConstants(const YAML::Node &node)
{
	loadSoundOffset("constants", DOOR_OPEN, node["doorSound"], "BATTLE.CAT");
	loadSoundOffset("constants", SLIDING_DOOR_OPEN, node["slidingDoorSound"], "BATTLE.CAT");
	loadSoundOffset("constants", SLIDING_DOOR_CLOSE, node["slidingDoorClose"], "BATTLE.CAT");
	loadSoundOffset("constants", SMALL_EXPLOSION, node["smallExplosion"], "BATTLE.CAT");
	loadSoundOffset("constants", LARGE_EXPLOSION, node["largeExplosion"], "BATTLE.CAT");

	loadSpriteOffset("constants", EXPLOSION_OFFSET, node["explosionOffset"], "X1.PCK");
	loadSpriteOffset("constants", SMOKE_OFFSET, node["smokeOffset"], "SMOKE.PCK");
	loadSpriteOffset("constants", UNDERWATER_SMOKE_OFFSET, node["underwaterSmokeOffset"], "SMOKE.PCK");

	loadSoundOffset("constants", ITEM_DROP, node["itemDrop"], "BATTLE.CAT");
	loadSoundOffset("constants", ITEM_THROW, node["itemThrow"], "BATTLE.CAT");
	loadSoundOffset("constants", ITEM_RELOAD, node["itemReload"], "BATTLE.CAT");
	loadSoundOffset("constants", WALK_OFFSET, node["walkOffset"], "BATTLE.CAT");
	loadSoundOffset("constants", FLYING_SOUND, node["flyingSound"], "BATTLE.CAT");

	loadSoundOffset("constants", BUTTON_PRESS, node["buttonPress"], "GEO.CAT");
	if (node["windowPopup"])
	{
		int k = 0;
		for (YAML::const_iterator j = node["windowPopup"].begin(); j != node["windowPopup"].end() && k < 3; ++j, ++k)
		{
			loadSoundOffset("constants", WINDOW_POPUP[k], (*j), "GEO.CAT");
		}
	}
	loadSoundOffset("constants", UFO_FIRE, node["ufoFire"], "GEO.CAT");
	loadSoundOffset("constants", UFO_HIT, node["ufoHit"], "GEO.CAT");
	loadSoundOffset("constants", UFO_CRASH, node["ufoCrash"], "GEO.CAT");
	loadSoundOffset("constants", UFO_EXPLODE, node["ufoExplode"], "GEO.CAT");
	loadSoundOffset("constants", INTERCEPTOR_HIT, node["interceptorHit"], "GEO.CAT");
	loadSoundOffset("constants", INTERCEPTOR_EXPLODE, node["interceptorExplode"], "GEO.CAT");
	GEOSCAPE_CURSOR = node["geoscapeCursor"].as<int>(GEOSCAPE_CURSOR);
	BASESCAPE_CURSOR = node["basescapeCursor"].as<int>(BASESCAPE_CURSOR);
	BATTLESCAPE_CURSOR = node["battlescapeCursor"].as<int>(BATTLESCAPE_CURSOR);
	UFOPAEDIA_CURSOR = node["ufopaediaCursor"].as<int>(UFOPAEDIA_CURSOR);
	GRAPHS_CURSOR = node["graphsCursor"].as<int>(GRAPHS_CURSOR);
	DAMAGE_RANGE = node["damageRange"].as<int>(DAMAGE_RANGE);
	EXPLOSIVE_DAMAGE_RANGE = node["explosiveDamageRange"].as<int>(EXPLOSIVE_DAMAGE_RANGE);
	size_t num = 0;
	for (YAML::const_iterator j = node["fireDamageRange"].begin(); j != node["fireDamageRange"].end() && num < 2; ++j)
	{
		FIRE_DAMAGE_RANGE[num] = (*j).as<int>(FIRE_DAMAGE_RANGE[num]);
		++num;
	}
	DEBRIEF_MUSIC_GOOD = node["goodDebriefingMusic"].as<std::string>(DEBRIEF_MUSIC_GOOD);
	DEBRIEF_MUSIC_BAD = node["badDebriefingMusic"].as<std::string>(DEBRIEF_MUSIC_BAD);
	EXTENDED_ITEM_RELOAD_COST = node["extendedItemReloadCost"].as<bool>(EXTENDED_ITEM_RELOAD_COST);
	EXTENDED_RUNNING_COST = node["extendedRunningCost"].as<bool>(EXTENDED_RUNNING_COST);
	EXTENDED_HWP_LOAD_ORDER = node["extendedHwpLoadOrder"].as<bool>(EXTENDED_HWP_LOAD_ORDER);
	EXTENDED_MELEE_REACTIONS = node["extendedMeleeReactions"].as<int>(EXTENDED_MELEE_REACTIONS);
	EXTENDED_TERRAIN_MELEE = node["extendedTerrainMelee"].as<int>(EXTENDED_TERRAIN_MELEE);
	EXTENDED_UNDERWATER_THROW_FACTOR = node["extendedUnderwaterThrowFactor"].as<int>(EXTENDED_UNDERWATER_THROW_FACTOR);
}

/**
 * Loads a ruleset's contents from a YAML file.
 * Rules that match pre-existing rules overwrite them.
 * @param filename YAML filename.
 * @param parsers Object with all available parsers.
 */
void Mod::loadFile(const FileMap::FileRecord &filerec, ModScript &parsers)
{
	auto doc = filerec.getYAML();

	if (const YAML::Node &extended = doc["extended"])
	{
		_scriptGlobal->load(extended);
		_scriptGlobal->getScriptValues().load(extended, parsers.getShared(), "globals");
	}
	for (YAML::const_iterator i = doc["countries"].begin(); i != doc["countries"].end(); ++i)
	{
		RuleCountry *rule = loadRule(*i, &_countries, &_countriesIndex);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["extraGlobeLabels"].begin(); i != doc["extraGlobeLabels"].end(); ++i)
	{
		RuleCountry *rule = loadRule(*i, &_extraGlobeLabels, &_extraGlobeLabelsIndex);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["regions"].begin(); i != doc["regions"].end(); ++i)
	{
		RuleRegion *rule = loadRule(*i, &_regions, &_regionsIndex);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["facilities"].begin(); i != doc["facilities"].end(); ++i)
	{
		RuleBaseFacility *rule = loadRule(*i, &_facilities, &_facilitiesIndex);
		if (rule != 0)
		{
			_facilityListOrder += 100;
			rule->load(*i, this, _facilityListOrder);
		}
	}
	for (YAML::const_iterator i = doc["crafts"].begin(); i != doc["crafts"].end(); ++i)
	{
		RuleCraft *rule = loadRule(*i, &_crafts, &_craftsIndex);
		if (rule != 0)
		{
			_craftListOrder += 100;
			rule->load(*i, this, _craftListOrder, parsers);
		}
	}
	for (YAML::const_iterator i = doc["craftWeapons"].begin(); i != doc["craftWeapons"].end(); ++i)
	{
		RuleCraftWeapon *rule = loadRule(*i, &_craftWeapons, &_craftWeaponsIndex);
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}
	for (YAML::const_iterator i = doc["itemCategories"].begin(); i != doc["itemCategories"].end(); ++i)
	{
		RuleItemCategory *rule = loadRule(*i, &_itemCategories, &_itemCategoriesIndex);
		if (rule != 0)
		{
			_itemCategoryListOrder += 100;
			rule->load(*i, this, _itemCategoryListOrder);
		}
	}
	for (YAML::const_iterator i = doc["items"].begin(); i != doc["items"].end(); ++i)
	{
		RuleItem *rule = loadRule(*i, &_items, &_itemsIndex);
		if (rule != 0)
		{
			_itemListOrder += 100;
			rule->load(*i, this, _itemListOrder, parsers);
		}
	}
	for (YAML::const_iterator i = doc["ufos"].begin(); i != doc["ufos"].end(); ++i)
	{
		RuleUfo *rule = loadRule(*i, &_ufos, &_ufosIndex);
		if (rule != 0)
		{
			rule->load(*i, parsers, this);
		}
	}
	for (YAML::const_iterator i = doc["invs"].begin(); i != doc["invs"].end(); ++i)
	{
		RuleInventory *rule = loadRule(*i, &_invs, &_invsIndex, "id");
		if (rule != 0)
		{
			_invListOrder += 10;
			rule->load(*i, _invListOrder);
		}
	}
	for (YAML::const_iterator i = doc["terrains"].begin(); i != doc["terrains"].end(); ++i)
	{
		RuleTerrain *rule = loadRule(*i, &_terrains, &_terrainIndex, "name");
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}

	for (YAML::const_iterator i = doc["armors"].begin(); i != doc["armors"].end(); ++i)
	{
		Armor *rule = loadRule(*i, &_armors, &_armorsIndex);
		if (rule != 0)
		{
			rule->load(*i, parsers, this);
		}
	}
	for (YAML::const_iterator i = doc["skills"].begin(); i != doc["skills"].end(); ++i)
	{
		RuleSkill *rule = loadRule(*i, &_skills, &_skillsIndex);
		if (rule != 0)
		{
			rule->load(*i, this, parsers);
		}
	}
	for (YAML::const_iterator i = doc["soldiers"].begin(); i != doc["soldiers"].end(); ++i)
	{
		RuleSoldier *rule = loadRule(*i, &_soldiers, &_soldiersIndex);
		if (rule != 0)
		{
			_soldierListOrder += 1;
			rule->load(*i, this, _soldierListOrder, parsers);
		}
	}
	for (YAML::const_iterator i = doc["units"].begin(); i != doc["units"].end(); ++i)
	{
		Unit *rule = loadRule(*i, &_units);
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}
	for (YAML::const_iterator i = doc["alienRaces"].begin(); i != doc["alienRaces"].end(); ++i)
	{
		AlienRace *rule = loadRule(*i, &_alienRaces, &_aliensIndex, "id");
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["enviroEffects"].begin(); i != doc["enviroEffects"].end(); ++i)
	{
		RuleEnviroEffects* rule = loadRule(*i, &_enviroEffects, &_enviroEffectsIndex);
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}
	for (YAML::const_iterator i = doc["startingConditions"].begin(); i != doc["startingConditions"].end(); ++i)
	{
		RuleStartingCondition *rule = loadRule(*i, &_startingConditions, &_startingConditionsIndex);
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}
	for (YAML::const_iterator i = doc["alienDeployments"].begin(); i != doc["alienDeployments"].end(); ++i)
	{
		AlienDeployment *rule = loadRule(*i, &_alienDeployments, &_deploymentsIndex);
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}
	for (YAML::const_iterator i = doc["research"].begin(); i != doc["research"].end(); ++i)
	{
		RuleResearch *rule = loadRule(*i, &_research, &_researchIndex, "name");
		if (rule != 0)
		{
			_researchListOrder += 100;
			rule->load(*i, this, parsers, _researchListOrder);
			if ((*i)["unlockFinalMission"].as<bool>(false))
			{
				_finalResearch = (*i)["name"].as<std::string>(_finalResearch);
			}
		}
	}
	for (YAML::const_iterator i = doc["manufacture"].begin(); i != doc["manufacture"].end(); ++i)
	{
		RuleManufacture *rule = loadRule(*i, &_manufacture, &_manufactureIndex, "name");
		if (rule != 0)
		{
			_manufactureListOrder += 100;
			rule->load(*i, this, _manufactureListOrder);
		}
	}
	for (YAML::const_iterator i = doc["manufactureShortcut"].begin(); i != doc["manufactureShortcut"].end(); ++i)
	{
		RuleManufactureShortcut *rule = loadRule(*i, &_manufactureShortcut, 0, "name");
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["soldierBonuses"].begin(); i != doc["soldierBonuses"].end(); ++i)
	{
		RuleSoldierBonus *rule = loadRule(*i, &_soldierBonus, &_soldierBonusIndex, "name");
		if (rule != 0)
		{
			_soldierBonusListOrder += 100;
			rule->load(*i, parsers, _soldierBonusListOrder);
		}
	}
	for (YAML::const_iterator i = doc["soldierTransformation"].begin(); i != doc["soldierTransformation"].end(); ++i)
	{
		RuleSoldierTransformation *rule = loadRule(*i, &_soldierTransformation, &_soldierTransformationIndex, "name");
		if (rule != 0)
		{
			_transformationListOrder += 100;
			rule->load(*i, this, _transformationListOrder);
		}
	}
	for (YAML::const_iterator i = doc["ufopaedia"].begin(); i != doc["ufopaedia"].end(); ++i)
	{
		if ((*i)["id"])
		{
			std::string id = (*i)["id"].as<std::string>();
			ArticleDefinition *rule;
			if (_ufopaediaArticles.find(id) != _ufopaediaArticles.end())
			{
				rule = _ufopaediaArticles[id];
			}
			else
			{
				if (!(*i)["type_id"].IsDefined()) { // otherwise it throws and I wasted hours
					Log(LOG_ERROR) << "ufopaedia item misses type_id attribute.";
					continue;
				}
				UfopaediaTypeId type = (UfopaediaTypeId)(*i)["type_id"].as<int>();
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
			rule->load(*i, _ufopaediaListOrder);
		}
		else if ((*i)["delete"])
		{
			std::string type = (*i)["delete"].as<std::string>();
			std::map<std::string, ArticleDefinition*>::iterator j = _ufopaediaArticles.find(type);
			if (j != _ufopaediaArticles.end())
			{
				_ufopaediaArticles.erase(j);
			}
			std::vector<std::string>::iterator idx = std::find(_ufopaediaIndex.begin(), _ufopaediaIndex.end(), type);
			if (idx != _ufopaediaIndex.end())
			{
				_ufopaediaIndex.erase(idx);
			}
		}
	}
	auto loadStartingBase = [](YAML::Node &docRef, const std::string &startingBaseType, YAML::Node &destRef)
	{
		// Bases can't be copied, so for savegame purposes we store the node instead
		YAML::Node base = docRef[startingBaseType];
		if (base)
		{
			if (isMapHelper(base))
			{
				for (YAML::const_iterator i = base.begin(); i != base.end(); ++i)
				{
					destRef[i->first.as<std::string>()] = YAML::Node(i->second);
				}
			}
			else
			{
				throw LoadRuleException(startingBaseType, base, "expected normal map node");
			}
		}
	};
	loadStartingBase(doc, "startingBase", _startingBaseDefault);
	loadStartingBase(doc, "startingBaseBeginner", _startingBaseBeginner);
	loadStartingBase(doc, "startingBaseExperienced", _startingBaseExperienced);
	loadStartingBase(doc, "startingBaseVeteran", _startingBaseVeteran);
	loadStartingBase(doc, "startingBaseGenius", _startingBaseGenius);
	loadStartingBase(doc, "startingBaseSuperhuman", _startingBaseSuperhuman);

	if (doc["startingTime"])
	{
		_startingTime.load(doc["startingTime"]);
	}
	_startingDifficulty = doc["startingDifficulty"].as<int>(_startingDifficulty);
	_maxViewDistance = doc["maxViewDistance"].as<int>(_maxViewDistance);
	_maxDarknessToSeeUnits = doc["maxDarknessToSeeUnits"].as<int>(_maxDarknessToSeeUnits);
	_costHireEngineer = doc["costHireEngineer"].as<int>(_costHireEngineer);
	_costHireScientist = doc["costHireScientist"].as<int>(_costHireScientist);
	_costEngineer = doc["costEngineer"].as<int>(_costEngineer);
	_costScientist = doc["costScientist"].as<int>(_costScientist);
	_timePersonnel = doc["timePersonnel"].as<int>(_timePersonnel);
	_hireByCountryOdds = doc["hireByCountryOdds"].as<int>(_hireByCountryOdds);
	_hireByRegionOdds = doc["hireByRegionOdds"].as<int>(_hireByRegionOdds);
	_initialFunding = doc["initialFunding"].as<int>(_initialFunding);
	_alienFuel = doc["alienFuel"].as<std::pair<std::string, int> >(_alienFuel);
	_fontName = doc["fontName"].as<std::string>(_fontName);
	_psiUnlockResearch = doc["psiUnlockResearch"].as<std::string>(_psiUnlockResearch);
	_fakeUnderwaterBaseUnlockResearch = doc["fakeUnderwaterBaseUnlockResearch"].as<std::string>(_fakeUnderwaterBaseUnlockResearch);
	_newBaseUnlockResearch = doc["newBaseUnlockResearch"].as<std::string>(_newBaseUnlockResearch);
	_hireScientistsUnlockResearch = doc["hireScientistsUnlockResearch"].as<std::string>(_hireScientistsUnlockResearch);
	_hireEngineersUnlockResearch = doc["hireEngineersUnlockResearch"].as<std::string>(_hireEngineersUnlockResearch);
	loadBaseFunction("mod", _hireScientistsRequiresBaseFunc, doc["hireScientistsRequiresBaseFunc"]);
	loadBaseFunction("mod", _hireEngineersRequiresBaseFunc, doc["hireEngineersRequiresBaseFunc"]);
	_destroyedFacility = doc["destroyedFacility"].as<std::string>(_destroyedFacility);

	_aiUseDelayGrenade = doc["turnAIUseGrenade"].as<int>(_aiUseDelayGrenade);
	_aiUseDelayBlaster = doc["turnAIUseBlaster"].as<int>(_aiUseDelayBlaster);
	if (const YAML::Node &nodeAI = doc["ai"])
	{
		_aiUseDelayBlaster = nodeAI["useDelayBlaster"].as<int>(_aiUseDelayBlaster);
		_aiUseDelayFirearm = nodeAI["useDelayFirearm"].as<int>(_aiUseDelayFirearm);
		_aiUseDelayGrenade = nodeAI["useDelayGrenade"].as<int>(_aiUseDelayGrenade);
		_aiUseDelayMelee   = nodeAI["useDelayMelee"].as<int>(_aiUseDelayMelee);
		_aiUseDelayPsionic = nodeAI["useDelayPsionic"].as<int>(_aiUseDelayPsionic);

		_aiFireChoiceIntelCoeff = nodeAI["fireChoiceIntelCoeff"].as<int>(_aiFireChoiceIntelCoeff);
		_aiFireChoiceAggroCoeff = nodeAI["fireChoiceAggroCoeff"].as<int>(_aiFireChoiceAggroCoeff);
		_aiExtendedFireModeChoice = nodeAI["extendedFireModeChoice"].as<bool>(_aiExtendedFireModeChoice);
		_aiRespectMaxRange = nodeAI["respectMaxRange"].as<bool>(_aiRespectMaxRange);
		_aiDestroyBaseFacilities = nodeAI["destroyBaseFacilities"].as<bool>(_aiDestroyBaseFacilities);
		_aiPickUpWeaponsMoreActively = nodeAI["pickUpWeaponsMoreActively"].as<bool>(_aiPickUpWeaponsMoreActively);
		_aiPickUpWeaponsMoreActivelyCiv = nodeAI["pickUpWeaponsMoreActivelyCiv"].as<bool>(_aiPickUpWeaponsMoreActivelyCiv);
	}
	_maxLookVariant = doc["maxLookVariant"].as<int>(_maxLookVariant);
	_tooMuchSmokeThreshold = doc["tooMuchSmokeThreshold"].as<int>(_tooMuchSmokeThreshold);
	_customTrainingFactor = doc["customTrainingFactor"].as<int>(_customTrainingFactor);
	_minReactionAccuracy = doc["minReactionAccuracy"].as<int>(_minReactionAccuracy);
	_chanceToStopRetaliation = doc["chanceToStopRetaliation"].as<int>(_chanceToStopRetaliation);
	_lessAliensDuringBaseDefense = doc["lessAliensDuringBaseDefense"].as<bool>(_lessAliensDuringBaseDefense);
	_allowCountriesToCancelAlienPact = doc["allowCountriesToCancelAlienPact"].as<bool>(_allowCountriesToCancelAlienPact);
	_buildInfiltrationBaseCloseToTheCountry = doc["buildInfiltrationBaseCloseToTheCountry"].as<bool>(_buildInfiltrationBaseCloseToTheCountry);
	_allowAlienBasesOnWrongTextures = doc["allowAlienBasesOnWrongTextures"].as<bool>(_allowAlienBasesOnWrongTextures);
	_kneelBonusGlobal = doc["kneelBonusGlobal"].as<int>(_kneelBonusGlobal);
	_oneHandedPenaltyGlobal = doc["oneHandedPenaltyGlobal"].as<int>(_oneHandedPenaltyGlobal);
	_enableCloseQuartersCombat = doc["enableCloseQuartersCombat"].as<int>(_enableCloseQuartersCombat);
	_closeQuartersAccuracyGlobal = doc["closeQuartersAccuracyGlobal"].as<int>(_closeQuartersAccuracyGlobal);
	_closeQuartersTuCostGlobal = doc["closeQuartersTuCostGlobal"].as<int>(_closeQuartersTuCostGlobal);
	_closeQuartersEnergyCostGlobal = doc["closeQuartersEnergyCostGlobal"].as<int>(_closeQuartersEnergyCostGlobal);
	_closeQuartersSneakUpGlobal = doc["closeQuartersSneakUpGlobal"].as<int>(_closeQuartersSneakUpGlobal);
	_noLOSAccuracyPenaltyGlobal = doc["noLOSAccuracyPenaltyGlobal"].as<int>(_noLOSAccuracyPenaltyGlobal);
	_surrenderMode = doc["surrenderMode"].as<int>(_surrenderMode);
	_bughuntMinTurn = doc["bughuntMinTurn"].as<int>(_bughuntMinTurn);
	_bughuntMaxEnemies = doc["bughuntMaxEnemies"].as<int>(_bughuntMaxEnemies);
	_bughuntRank = doc["bughuntRank"].as<int>(_bughuntRank);
	_bughuntLowMorale = doc["bughuntLowMorale"].as<int>(_bughuntLowMorale);
	_bughuntTimeUnitsLeft = doc["bughuntTimeUnitsLeft"].as<int>(_bughuntTimeUnitsLeft);


	if (const YAML::Node &nodeMana = doc["mana"])
	{
		_manaEnabled = nodeMana["enabled"].as<bool>(_manaEnabled);
		_manaBattleUI = nodeMana["battleUI"].as<bool>(_manaBattleUI);
		_manaUnlockResearch = nodeMana["unlockResearch"].as<std::string>(_manaUnlockResearch);
		_manaTrainingPrimary = nodeMana["trainingPrimary"].as<bool>(_manaTrainingPrimary);
		_manaTrainingSecondary = nodeMana["trainingSecondary"].as<bool>(_manaTrainingSecondary);

		_manaMissingWoundThreshold = nodeMana["woundThreshold"].as<int>(_manaMissingWoundThreshold);
		_manaReplenishAfterMission = nodeMana["replenishAfterMission"].as<bool>(_manaReplenishAfterMission);
	}
	if (const YAML::Node &nodeHealth = doc["health"])
	{
		_healthMissingWoundThreshold = nodeHealth["woundThreshold"].as<int>(_healthMissingWoundThreshold);
		_healthReplenishAfterMission = nodeHealth["replenishAfterMission"].as<bool>(_healthReplenishAfterMission);
	}


	if (const YAML::Node &nodeGameOver = doc["gameOver"])
	{
		_loseMoney = nodeGameOver["loseMoney"].as<std::string>(_loseMoney);
		_loseRating = nodeGameOver["loseRating"].as<std::string>(_loseRating);
		_loseDefeat = nodeGameOver["loseDefeat"].as<std::string>(_loseDefeat);
	}
	_ufoGlancingHitThreshold = doc["ufoGlancingHitThreshold"].as<int>(_ufoGlancingHitThreshold);
	_ufoBeamWidthParameter = doc["ufoBeamWidthParameter"].as<int>(_ufoBeamWidthParameter);
	if (doc["ufoTractorBeamSizeModifiers"])
	{
		int index = 0;
		for (YAML::const_iterator i = doc["ufoTractorBeamSizeModifiers"].begin(); i != doc["ufoTractorBeamSizeModifiers"].end() && index < 5; ++i)
		{
			_ufoTractorBeamSizeModifiers[index] = (*i).as<int>(_ufoTractorBeamSizeModifiers[index]);
			index++;
		}
	}
	_escortRange = doc["escortRange"].as<int>(_escortRange);
	_drawEnemyRadarCircles = doc["drawEnemyRadarCircles"].as<int>(_drawEnemyRadarCircles);
	_escortsJoinFightAgainstHK = doc["escortsJoinFightAgainstHK"].as<bool>(_escortsJoinFightAgainstHK);
	_hunterKillerFastRetarget = doc["hunterKillerFastRetarget"].as<bool>(_hunterKillerFastRetarget);
	_crewEmergencyEvacuationSurvivalChance = doc["crewEmergencyEvacuationSurvivalChance"].as<int>(_crewEmergencyEvacuationSurvivalChance);
	_pilotsEmergencyEvacuationSurvivalChance = doc["pilotsEmergencyEvacuationSurvivalChance"].as<int>(_pilotsEmergencyEvacuationSurvivalChance);
	_soldiersPerSergeant = doc["soldiersPerSergeant"].as<int>(_soldiersPerSergeant);
	_soldiersPerCaptain = doc["soldiersPerCaptain"].as<int>(_soldiersPerCaptain);
	_soldiersPerColonel = doc["soldiersPerColonel"].as<int>(_soldiersPerColonel);
	_soldiersPerCommander = doc["soldiersPerCommander"].as<int>(_soldiersPerCommander);
	_pilotAccuracyZeroPoint = doc["pilotAccuracyZeroPoint"].as<int>(_pilotAccuracyZeroPoint);
	_pilotAccuracyRange = doc["pilotAccuracyRange"].as<int>(_pilotAccuracyRange);
	_pilotReactionsZeroPoint = doc["pilotReactionsZeroPoint"].as<int>(_pilotReactionsZeroPoint);
	_pilotReactionsRange = doc["pilotReactionsRange"].as<int>(_pilotReactionsRange);
	if (doc["pilotBraveryThresholds"])
	{
		int index = 0;
		for (YAML::const_iterator i = doc["pilotBraveryThresholds"].begin(); i != doc["pilotBraveryThresholds"].end() && index < 3; ++i)
		{
			_pilotBraveryThresholds[index] = (*i).as<int>(_pilotBraveryThresholds[index]);
			index++;
		}
	}
	_performanceBonusFactor = doc["performanceBonusFactor"].as<int>(_performanceBonusFactor);
	_enableNewResearchSorting = doc["enableNewResearchSorting"].as<bool>(_enableNewResearchSorting);
	_displayCustomCategories = doc["displayCustomCategories"].as<int>(_displayCustomCategories);
	_shareAmmoCategories = doc["shareAmmoCategories"].as<bool>(_shareAmmoCategories);
	_showDogfightDistanceInKm = doc["showDogfightDistanceInKm"].as<bool>(_showDogfightDistanceInKm);
	_showFullNameInAlienInventory = doc["showFullNameInAlienInventory"].as<bool>(_showFullNameInAlienInventory);
	_alienInventoryOffsetX = doc["alienInventoryOffsetX"].as<int>(_alienInventoryOffsetX);
	_alienInventoryOffsetBigUnit = doc["alienInventoryOffsetBigUnit"].as<int>(_alienInventoryOffsetBigUnit);
	_hidePediaInfoButton = doc["hidePediaInfoButton"].as<bool>(_hidePediaInfoButton);
	_extraNerdyPediaInfoType = doc["extraNerdyPediaInfoType"].as<int>(_extraNerdyPediaInfoType);
	_giveScoreAlsoForResearchedArtifacts = doc["giveScoreAlsoForResearchedArtifacts"].as<bool>(_giveScoreAlsoForResearchedArtifacts);
	_statisticalBulletConservation = doc["statisticalBulletConservation"].as<bool>(_statisticalBulletConservation);
	_stunningImprovesMorale = doc["stunningImprovesMorale"].as<bool>(_stunningImprovesMorale);
	_tuRecoveryWakeUpNewTurn = doc["tuRecoveryWakeUpNewTurn"].as<int>(_tuRecoveryWakeUpNewTurn);
	_shortRadarRange = doc["shortRadarRange"].as<int>(_shortRadarRange);
	_buildTimeReductionScaling = doc["buildTimeReductionScaling"].as<int>(_buildTimeReductionScaling);
	_baseDefenseMapFromLocation = doc["baseDefenseMapFromLocation"].as<int>(_baseDefenseMapFromLocation);
	_pediaReplaceCraftFuelWithRangeType = doc["pediaReplaceCraftFuelWithRangeType"].as<int>(_pediaReplaceCraftFuelWithRangeType);
	_missionRatings = doc["missionRatings"].as<std::map<int, std::string> >(_missionRatings);
	_monthlyRatings = doc["monthlyRatings"].as<std::map<int, std::string> >(_monthlyRatings);
	loadUnorderedNamesToNames("mod", _fixedUserOptions, doc["fixedUserOptions"]);
	loadUnorderedNamesToNames("mod", _recommendedUserOptions, doc["recommendedUserOptions"]);
	loadUnorderedNames("mod", _hiddenMovementBackgrounds, doc["hiddenMovementBackgrounds"]);
	loadUnorderedNames("mod", _baseNamesFirst, doc["baseNamesFirst"]);
	loadUnorderedNames("mod", _baseNamesMiddle, doc["baseNamesMiddle"]);
	loadUnorderedNames("mod", _baseNamesLast, doc["baseNamesLast"]);
	loadUnorderedNames("mod", _operationNamesFirst, doc["operationNamesFirst"]);
	loadUnorderedNames("mod", _operationNamesLast, doc["operationNamesLast"]);
	_disableUnderwaterSounds = doc["disableUnderwaterSounds"].as<bool>(_disableUnderwaterSounds);
	_enableUnitResponseSounds = doc["enableUnitResponseSounds"].as<bool>(_enableUnitResponseSounds);
	for (YAML::const_iterator i = doc["unitResponseSounds"].begin(); i != doc["unitResponseSounds"].end(); ++i)
	{
		std::string type = (*i)["name"].as<std::string>();
		if ((*i)["selectUnitSound"])
			loadSoundOffset(type, _selectUnitSound[type], (*i)["selectUnitSound"], "BATTLE.CAT");
		if ((*i)["startMovingSound"])
			loadSoundOffset(type, _startMovingSound[type], (*i)["startMovingSound"], "BATTLE.CAT");
		if ((*i)["selectWeaponSound"])
			loadSoundOffset(type, _selectWeaponSound[type], (*i)["selectWeaponSound"], "BATTLE.CAT");
		if ((*i)["annoyedSound"])
			loadSoundOffset(type, _annoyedSound[type], (*i)["annoyedSound"], "BATTLE.CAT");
	}
	loadSoundOffset("global", _selectBaseSound, doc["selectBaseSound"], "BATTLE.CAT");
	loadSoundOffset("global", _startDogfightSound, doc["startDogfightSound"], "BATTLE.CAT");
	_flagByKills = doc["flagByKills"].as<std::vector<int> >(_flagByKills);

	_defeatScore = doc["defeatScore"].as<int>(_defeatScore);
	_defeatFunds = doc["defeatFunds"].as<int>(_defeatFunds);
	_difficultyDemigod = doc["difficultyDemigod"].as<bool>(_difficultyDemigod);

	if (const YAML::Node& difficultyCoefficientOverrides = doc["difficultyCoefficientOverrides"])
	{
		_monthlyRatingThresholds = difficultyCoefficientOverrides["monthlyRatingThresholds"].as< std::vector<int> >(_monthlyRatingThresholds);
		_ufoFiringRateCoefficients = difficultyCoefficientOverrides["ufoFiringRateCoefficients"].as< std::vector<int> >(_ufoFiringRateCoefficients);
		_ufoEscapeCountdownCoefficients = difficultyCoefficientOverrides["ufoEscapeCountdownCoefficients"].as< std::vector<int> >(_ufoEscapeCountdownCoefficients);
		_retaliationTriggerOdds = difficultyCoefficientOverrides["retaliationTriggerOdds"].as< std::vector<int> >(_retaliationTriggerOdds);
		_retaliationBaseRegionOdds = difficultyCoefficientOverrides["retaliationBaseRegionOdds"].as< std::vector<int> >(_retaliationBaseRegionOdds);
		_aliensFacingCraftOdds = difficultyCoefficientOverrides["aliensFacingCraftOdds"].as< std::vector<int> >(_aliensFacingCraftOdds);
	}

	if (doc["difficultyCoefficient"])
	{
		size_t num = 0;
		for (YAML::const_iterator i = doc["difficultyCoefficient"].begin(); i != doc["difficultyCoefficient"].end() && num < MaxDifficultyLevels; ++i)
		{
			DIFFICULTY_COEFFICIENT[num] = (*i).as<int>(DIFFICULTY_COEFFICIENT[num]);
			_statAdjustment[num].growthMultiplier = DIFFICULTY_COEFFICIENT[num];
			++num;
		}
	}
	if (doc["sellPriceCoefficient"])
	{
		size_t num = 0;
		for (YAML::const_iterator i = doc["sellPriceCoefficient"].begin(); i != doc["sellPriceCoefficient"].end() && num < MaxDifficultyLevels; ++i)
		{
			SELL_PRICE_COEFFICIENT[num] = (*i).as<int>(SELL_PRICE_COEFFICIENT[num]);
			++num;
		}
	}
	if (doc["difficultyBasedRetaliationDelay"])
	{
		size_t num = 0;
		for (YAML::const_iterator i = doc["difficultyBasedRetaliationDelay"].begin(); i != doc["difficultyBasedRetaliationDelay"].end() && num < MaxDifficultyLevels; ++i)
		{
			DIFFICULTY_BASED_RETAL_DELAY[num] = (*i).as<int>(DIFFICULTY_BASED_RETAL_DELAY[num]);
			++num;
		}
	}
	if (doc["unitResponseSoundsFrequency"])
	{
		size_t num = 0;
		for (YAML::const_iterator i = doc["unitResponseSoundsFrequency"].begin(); i != doc["unitResponseSoundsFrequency"].end() && num < 4; ++i)
		{
			UNIT_RESPONSE_SOUNDS_FREQUENCY[num] = (*i).as<int>(UNIT_RESPONSE_SOUNDS_FREQUENCY[num]);
			++num;
		}
	}
	for (YAML::const_iterator i = doc["ufoTrajectories"].begin(); i != doc["ufoTrajectories"].end(); ++i)
	{
		UfoTrajectory *rule = loadRule(*i, &_ufoTrajectories, 0, "id");
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["alienMissions"].begin(); i != doc["alienMissions"].end(); ++i)
	{
		RuleAlienMission *rule = loadRule(*i, &_alienMissions, &_alienMissionsIndex);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	_alienItemLevels = doc["alienItemLevels"].as< std::vector< std::vector<int> > >(_alienItemLevels);
	for (YAML::const_iterator i = doc["MCDPatches"].begin(); i != doc["MCDPatches"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (_MCDPatches.find(type) != _MCDPatches.end())
		{
			_MCDPatches[type]->load(*i);
		}
		else
		{
			MCDPatch *patch = new MCDPatch();
			patch->load(*i);
			_MCDPatches[type] = patch;
		}
	}
	for (YAML::const_iterator i = doc["extraSprites"].begin(); i != doc["extraSprites"].end(); ++i)
	{
		if ((*i)["type"] || (*i)["typeSingle"])
		{
			std::string type;
			type = (*i)["type"].as<std::string>(type);
			if (type.empty())
			{
				type = (*i)["typeSingle"].as<std::string>();
			}
			ExtraSprites *extraSprites = new ExtraSprites();
			const ModData* data = _modCurrent;
			// doesn't support modIndex
			if (type == "TEXTURE.DAT")
				data = &_modData.at(0);
			extraSprites->load(*i, data);
			_extraSprites[type].push_back(extraSprites);
		}
		else if ((*i)["delete"])
		{
			std::string type = (*i)["delete"].as<std::string>();
			std::map<std::string, std::vector<ExtraSprites*> >::iterator j = _extraSprites.find(type);
			if (j != _extraSprites.end())
			{
				_extraSprites.erase(j);
			}
		}
	}
	for (YAML::const_iterator i = doc["customPalettes"].begin(); i != doc["customPalettes"].end(); ++i)
	{
		CustomPalettes *rule = loadRule(*i, &_customPalettes, &_customPalettesIndex);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["extraSounds"].begin(); i != doc["extraSounds"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		ExtraSounds *extraSounds = new ExtraSounds();
		extraSounds->load(*i, _modCurrent);
		_extraSounds.push_back(std::make_pair(type, extraSounds));
	}
	for (YAML::const_iterator i = doc["extraStrings"].begin(); i != doc["extraStrings"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (_extraStrings.find(type) != _extraStrings.end())
		{
			_extraStrings[type]->load(*i);
		}
		else
		{
			ExtraStrings *extraStrings = new ExtraStrings();
			extraStrings->load(*i);
			_extraStrings[type] = extraStrings;
		}
	}

	for (YAML::const_iterator i = doc["statStrings"].begin(); i != doc["statStrings"].end(); ++i)
	{
		StatString *statString = new StatString();
		statString->load(*i);
		_statStrings.push_back(statString);
	}

	for (YAML::const_iterator i = doc["interfaces"].begin(); i != doc["interfaces"].end(); ++i)
	{
		RuleInterface *rule = loadRule(*i, &_interfaces);
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}
	if (doc["globe"])
	{
		_globe->load(doc["globe"]);
	}
	if (doc["converter"])
	{
		_converter->load(doc["converter"]);
	}
	if (const YAML::Node& constants = doc["constants"])
	{
		//backward compatibility version
		if (constants.IsSequence())
		{
			for (YAML::const_iterator i = constants.begin(); i != constants.end(); ++i)
			{
				loadConstants((*i));
			}
		}
		else
		{
			loadConstants(constants);
		}
	}
	for (YAML::const_iterator i = doc["mapScripts"].begin(); i != doc["mapScripts"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if ((*i)["delete"])
		{
			type = (*i)["delete"].as<std::string>(type);
		}
		if (_mapScripts.find(type) != _mapScripts.end())
		{
			Collections::deleteAll(_mapScripts[type]);
		}
		for (YAML::const_iterator j = (*i)["commands"].begin(); j != (*i)["commands"].end(); ++j)
		{
			MapScript *mapScript = new MapScript();
			mapScript->load(*j);
			_mapScripts[type].push_back(mapScript);
		}
	}
	for (YAML::const_iterator i = doc["arcScripts"].begin(); i != doc["arcScripts"].end(); ++i)
	{
		RuleArcScript* rule = loadRule(*i, &_arcScripts, &_arcScriptIndex, "type");
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["eventScripts"].begin(); i != doc["eventScripts"].end(); ++i)
	{
		RuleEventScript* rule = loadRule(*i, &_eventScripts, &_eventScriptIndex, "type");
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["events"].begin(); i != doc["events"].end(); ++i)
	{
		RuleEvent* rule = loadRule(*i, &_events, &_eventIndex, "name");
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["missionScripts"].begin(); i != doc["missionScripts"].end(); ++i)
	{
		RuleMissionScript *rule = loadRule(*i, &_missionScripts, &_missionScriptIndex, "type");
		if (rule != 0)
		{
			rule->load(*i);
		}
	}

	// refresh _psiRequirements for psiStrengthEval
	for (std::vector<std::string>::const_iterator i = _facilitiesIndex.begin(); i != _facilitiesIndex.end(); ++i)
	{
		RuleBaseFacility *rule = getBaseFacility(*i);
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

	for (YAML::const_iterator i = doc["cutscenes"].begin(); i != doc["cutscenes"].end(); ++i)
	{
		RuleVideo *rule = loadRule(*i, &_videos);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["musics"].begin(); i != doc["musics"].end(); ++i)
	{
		RuleMusic *rule = loadRule(*i, &_musicDefs);
		if (rule != 0)
		{
			rule->load(*i);
		}
	}
	for (YAML::const_iterator i = doc["commendations"].begin(); i != doc["commendations"].end(); ++i)
	{
		RuleCommendations *rule = loadRule(*i, &_commendations);
		if (rule != 0)
		{
			rule->load(*i, this);
		}
	}
	size_t count = 0;
	for (YAML::const_iterator i = doc["aimAndArmorMultipliers"].begin(); i != doc["aimAndArmorMultipliers"].end() && count < MaxDifficultyLevels; ++i)
	{
		_statAdjustment[count].aimMultiplier = (*i).as<double>(_statAdjustment[count].aimMultiplier);
		_statAdjustment[count].armorMultiplier = (*i).as<double>(_statAdjustment[count].armorMultiplier);
		++count;
	}
	count = 0;
	for (YAML::const_iterator i = doc["aimMultipliers"].begin(); i != doc["aimMultipliers"].end() && count < MaxDifficultyLevels; ++i)
	{
		_statAdjustment[count].aimMultiplier = (*i).as<double>(_statAdjustment[count].aimMultiplier);
		++count;
	}
	count = 0;
	for (YAML::const_iterator i = doc["armorMultipliers"].begin(); i != doc["armorMultipliers"].end() && count < MaxDifficultyLevels; ++i)
	{
		_statAdjustment[count].armorMultiplier = (*i).as<double>(_statAdjustment[count].armorMultiplier);
		++count;
	}
	if (doc["statGrowthMultipliers"])
	{
		_statAdjustment[0].statGrowth = doc["statGrowthMultipliers"].as<UnitStats>(_statAdjustment[0].statGrowth);
		for (size_t i = 1; i != MaxDifficultyLevels; ++i)
		{
			_statAdjustment[i].statGrowth = _statAdjustment[0].statGrowth;
		}
	}
	if (const YAML::Node &lighting = doc["lighting"])
	{
		_maxStaticLightDistance = lighting["maxStatic"].as<int>(_maxStaticLightDistance);
		_maxDynamicLightDistance = lighting["maxDynamic"].as<int>(_maxDynamicLightDistance);
		_enhancedLighting = lighting["enhanced"].as<int>(_enhancedLighting);
	}
}

/**
 * Helper function protecting from circular references in node definition.
 * @param node Node to test
 * @param name Name of original node.
 * @param limit Current depth.
 */
static void refNodeTestDeepth(const YAML::Node &node, const std::string &name, int limit)
{
	if (limit > 64)
	{
		throw Exception("Nest limit of refNode reach in " + name);
	}
	if (const YAML::Node &nested = node["refNode"])
	{
		if (!nested.IsMap())
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
template <typename T>
T *Mod::loadRule(const YAML::Node &node, std::map<std::string, T*> *map, std::vector<std::string> *index, const std::string &key) const
{
	T *rule = 0;
	if (node[key])
	{
		std::string type = node[key].as<std::string>();

		if (isEmptyRuleName(type))
		{
			throw Exception("Invalid value for '" + key + "' at line " + std::to_string(node[key].Mark().line));
		}

		typename std::map<std::string, T*>::const_iterator i = map->find(type);
		if (i != map->end())
		{
			rule = i->second;
		}
		else
		{
			rule = new T(type);
			(*map)[type] = rule;
			if (index != 0)
			{
				index->push_back(type);
			}
		}

		// protection from self referencing refNode node
		refNodeTestDeepth(node, type, 0);
	}
	else if (node["delete"])
	{
		std::string type = node["delete"].as<std::string>();

		if (isEmptyRuleName(type))
		{
			throw Exception("Invalid value for 'delete' at line " +  std::to_string(node["delete"].Mark().line));
		}

		typename std::map<std::string, T*>::iterator i = map->find(type);
		if (i != map->end())
		{
			map->erase(i);
		}
		if (index != 0)
		{
			std::vector<std::string>::iterator idx = std::find(index->begin(), index->end(), type);
			if (idx != index->end())
			{
				index->erase(idx);
			}
		}
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
	for (std::vector<std::string>::const_iterator i = _countriesIndex.begin(); i != _countriesIndex.end(); ++i)
	{
		RuleCountry *country = getCountry(*i);
		if (!country->getLonMin().empty())
			save->getCountries()->push_back(new Country(country));
	}
	// Adjust funding to total $6M
	int missing = ((_initialFunding - save->getCountryFunding()/1000) / (int)save->getCountries()->size()) * 1000;
	for (std::vector<Country*>::iterator i = save->getCountries()->begin(); i != save->getCountries()->end(); ++i)
	{
		int funding = (*i)->getFunding().back() + missing;
		if (funding < 0)
		{
			funding = (*i)->getFunding().back();
		}
		(*i)->setFunding(funding);
	}
	save->setFunds(save->getCountryFunding());

	// Add regions
	for (std::vector<std::string>::const_iterator i = _regionsIndex.begin(); i != _regionsIndex.end(); ++i)
	{
		RuleRegion *region = getRegion(*i);
		if (!region->getLonMin().empty())
			save->getRegions()->push_back(new Region(region));
	}

	// Set up starting base
	const YAML::Node &startingBaseByDiff = getStartingBase(diff);
	Base *base = new Base(this);
	base->load(startingBaseByDiff, save, true);
	save->getBases()->push_back(base);

	// Correct IDs
	for (std::vector<Craft*>::const_iterator i = base->getCrafts()->begin(); i != base->getCrafts()->end(); ++i)
	{
		save->getId((*i)->getRules()->getType());
	}

	// Determine starting soldier types
	std::vector<std::string> soldierTypes = _soldiersIndex;
	for (std::vector<std::string>::iterator i = soldierTypes.begin(); i != soldierTypes.end();)
	{
		if (getSoldier(*i)->getRequirements().empty())
		{
			++i;
		}
		else
		{
			i = soldierTypes.erase(i);
		}
	}

	const YAML::Node &node = startingBaseByDiff["randomSoldiers"];
	std::vector<std::string> randomTypes;
	if (node)
	{
		// Starting soldiers specified by type
		if (node.IsMap())
		{
			std::map<std::string, int> randomSoldiers = node.as< std::map<std::string, int> >(std::map<std::string, int>());
			for (std::map<std::string, int>::iterator i = randomSoldiers.begin(); i != randomSoldiers.end(); ++i)
			{
				for (int s = 0; s < i->second; ++s)
				{
					randomTypes.push_back(i->first);
				}
			}
		}
		// Starting soldiers specified by amount
		else if (node.IsScalar())
		{
			int randomSoldiers = node.as<int>(0);
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
				for (std::vector<SoldierCommendations*>::iterator comm = diary->getSoldierCommendations()->begin(); comm != diary->getSoldierCommendations()->end(); ++comm)
				{
					(*comm)->makeOld();
				}
			}
		}
		// Assign pilots to craft (interceptors first, transport last) and non-pilots to transports only
		for (auto& soldier : *base->getSoldiers())
		{
			if (soldier->getArmor()->getSize() > 1)
			{
				// "Large soldiers" just stay in the base
			}
			else if (soldier->getRules()->getAllowPiloting())
			{
				Craft *found = 0;
				for (auto& craft : *base->getCrafts())
				{
					if (!found && craft->getRules()->getAllowLanding() && craft->getSpaceUsed() < craft->getRules()->getMaxUnits())
					{
						// Remember transporter as fall-back, but search further for interceptors
						found = craft;
					}
					if (!craft->getRules()->getAllowLanding() && craft->getSpaceUsed() < craft->getRules()->getPilots())
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
				for (auto& craft : *base->getCrafts())
				{
					if (craft->getRules()->getAllowLanding() && craft->getSpaceUsed() < craft->getRules()->getMaxUnits())
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
	std::map<std::string, RuleItemCategory*>::const_iterator i = _itemCategories.find(id);
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
	std::map<std::string, MapDataSet*>::iterator map = _mapDataSets.find(name);
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
	std::map<std::string, RuleEnviroEffects*>::const_iterator i = _enviroEffects.find(name);
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
	std::map<std::string, RuleStartingCondition*>::const_iterator i = _startingConditions.find(name);
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
	for (auto& n : id)
	{
		auto r = getResearch(n, false);
		if (r)
		{
			dest.push_back(r);
		}
		else
		{
			throw Exception("Unknown research '" + n + "'");
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

	const YAML::Node &startingBaseByDiff = getStartingBase(diff);
	for (YAML::const_iterator i = startingBaseByDiff["facilities"].begin(); i != startingBaseByDiff["facilities"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
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
	for (std::map<std::string, RuleAlienMission *>::const_iterator i = _alienMissions.begin(); i != _alienMissions.end(); ++i)
	{
		if (i->second->getObjective() == objective && i->second->getWeight(monthsPassed) > 0)
		{
			totalWeight += i->second->getWeight(monthsPassed);
			possibilities[totalWeight] = i->second;
		}
	}
	if (totalWeight > 0)
	{
		int pick = RNG::generate(1, totalWeight);
		for (std::map<int, RuleAlienMission*>::const_iterator i = possibilities.begin(); i != possibilities.end(); ++i)
		{
			if (pick <= i->first)
			{
				return i->second;
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
const YAML::Node &Mod::getDefaultStartingBase() const
{
	return _startingBaseDefault;
}

/**
 * Gets the custom starting base (by game difficulty).
 * @return The starting base definition.
 */
const YAML::Node &Mod::getStartingBase(GameDifficulty diff) const
{
	if (diff == DIFF_BEGINNER && _startingBaseBeginner && !_startingBaseBeginner.IsNull())
	{
		return _startingBaseBeginner;
	}
	else if (diff == DIFF_EXPERIENCED && _startingBaseExperienced && !_startingBaseExperienced.IsNull())
	{
		return _startingBaseExperienced;
	}
	else if (diff == DIFF_VETERAN && _startingBaseVeteran && !_startingBaseVeteran.IsNull())
	{
		return _startingBaseVeteran;
	}
	else if (diff == DIFF_GENIUS && _startingBaseGenius && !_startingBaseGenius.IsNull())
	{
		return _startingBaseGenius;
	}
	else if (diff == DIFF_SUPERHUMAN && _startingBaseSuperhuman && !_startingBaseSuperhuman.IsNull())
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
	std::map<std::string, MCDPatch*>::const_iterator i = _MCDPatches.find(id);
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
	typedef const std::string& first_argument_type;
	typedef const std::string& second_argument_type;
	typedef bool result_type;

	Mod *_mod;
	typedef T*(Mod::*RuleLookup)(const std::string &id, bool error) const;
	RuleLookup _lookup;

	compareRule(Mod *mod, RuleLookup lookup) : _mod(mod), _lookup(lookup)
	{
	}

	bool operator()(const std::string &r1, const std::string &r2) const
	{
		T *rule1 = (_mod->*_lookup)(r1, true);
		T *rule2 = (_mod->*_lookup)(r2, true);
		return (rule1->getListOrder() < rule2->getListOrder());
	}
};

/**
 * Craft weapons use the list order of their launcher item.
 */
template <>
struct compareRule<RuleCraftWeapon>
{
	typedef const std::string& first_argument_type;
	typedef const std::string& second_argument_type;
	typedef bool result_type;

	Mod *_mod;

	compareRule(Mod *mod) : _mod(mod)
	{
	}

	bool operator()(const std::string &r1, const std::string &r2) const
	{
		auto *rule1 = _mod->getCraftWeapon(r1)->getLauncherItem();
		auto *rule2 = _mod->getCraftWeapon(r2)->getLauncherItem();
		return (rule1->getListOrder() < rule2->getListOrder());
	}
};

/**
 * Armor uses the list order of their store item.
 * Itemless armor comes before all else.
 */
template <>
struct compareRule<Armor>
{
	typedef const std::string& first_argument_type;
	typedef const std::string& second_argument_type;
	typedef bool result_type;

	Mod *_mod;

	compareRule(Mod *mod) : _mod(mod)
	{
	}

	bool operator()(const std::string &r1, const std::string &r2) const
	{
		Armor* armor1 = _mod->getArmor(r1);
		Armor* armor2 = _mod->getArmor(r2);
		return operator()(armor1, armor2);
	}

	bool operator()(const Armor* armor1, const Armor* armor2) const
	{
		const RuleItem *rule1 = armor1->getStoreItem();
		const RuleItem *rule2 = armor2->getStoreItem();
		if (!rule1 && !rule2)
			return (armor1 < armor2); // tiebreaker, don't care about order, pointers are as good as any
		else if (!rule1)
			return true;
		else if (!rule2)
			return false;
		else
			return (rule1->getListOrder() < rule2->getListOrder());
	}
};

/**
 * Ufopaedia articles use section and list order.
 */
template <>
struct compareRule<ArticleDefinition>
{
	typedef const std::string& first_argument_type;
	typedef const std::string& second_argument_type;
	typedef bool result_type;

	Mod *_mod;
	const std::map<std::string, int> &_sections;

	compareRule(Mod *mod) : _mod(mod), _sections(mod->getUfopaediaSections())
	{
	}

	bool operator()(const std::string &r1, const std::string &r2) const
	{
		ArticleDefinition *rule1 = _mod->getUfopaediaArticle(r1);
		ArticleDefinition *rule2 = _mod->getUfopaediaArticle(r2);
		if (rule1->section == rule2->section)
			return (rule1->getListOrder() < rule2->getListOrder());
		else
			return (_sections.at(rule1->section) < _sections.at(rule2->section));
	}
};

/**
 * Ufopaedia sections use article list order.
 */
struct compareSection
{
	typedef const std::string& first_argument_type;
	typedef const std::string& second_argument_type;
	typedef bool result_type;

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
 * Sorts all our lists according to their weight.
 */
void Mod::sortLists()
{
	for (auto rulePair : _ufopaediaArticles)
	{
		auto rule = rulePair.second;
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

	std::sort(_itemCategoriesIndex.begin(), _itemCategoriesIndex.end(), compareRule<RuleItemCategory>(this, (compareRule<RuleItemCategory>::RuleLookup)&Mod::getItemCategory));
	std::sort(_itemsIndex.begin(), _itemsIndex.end(), compareRule<RuleItem>(this, (compareRule<RuleItem>::RuleLookup)&Mod::getItem));
	std::sort(_craftsIndex.begin(), _craftsIndex.end(), compareRule<RuleCraft>(this, (compareRule<RuleCraft>::RuleLookup)&Mod::getCraft));
	std::sort(_facilitiesIndex.begin(), _facilitiesIndex.end(), compareRule<RuleBaseFacility>(this, (compareRule<RuleBaseFacility>::RuleLookup)&Mod::getBaseFacility));
	std::sort(_researchIndex.begin(), _researchIndex.end(), compareRule<RuleResearch>(this, (compareRule<RuleResearch>::RuleLookup)&Mod::getResearch));
	std::sort(_manufactureIndex.begin(), _manufactureIndex.end(), compareRule<RuleManufacture>(this, (compareRule<RuleManufacture>::RuleLookup)&Mod::getManufacture));
	std::sort(_soldierTransformationIndex.begin(), _soldierTransformationIndex.end(), compareRule<RuleSoldierTransformation>(this,  (compareRule<RuleSoldierTransformation>::RuleLookup)&Mod::getSoldierTransformation));
	std::sort(_invsIndex.begin(), _invsIndex.end(), compareRule<RuleInventory>(this, (compareRule<RuleInventory>::RuleLookup)&Mod::getInventory));
	// special cases
	std::sort(_craftWeaponsIndex.begin(), _craftWeaponsIndex.end(), compareRule<RuleCraftWeapon>(this));
	std::sort(_armorsIndex.begin(), _armorsIndex.end(), compareRule<Armor>(this));
	std::sort(_armorsForSoldiersCache.begin(), _armorsForSoldiersCache.end(), compareRule<Armor>(this));
	_ufopaediaSections[UFOPAEDIA_NOT_AVAILABLE] = 0;
	std::sort(_ufopaediaIndex.begin(), _ufopaediaIndex.end(), compareRule<ArticleDefinition>(this));
	std::sort(_ufopaediaCatIndex.begin(), _ufopaediaCatIndex.end(), compareSection(this));
	std::sort(_soldiersIndex.begin(), _soldiersIndex.end(), compareRule<RuleSoldier>(this, (compareRule<RuleSoldier>::RuleLookup) & Mod::getSoldier));
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
Soldier *Mod::genSoldier(SavedGame *save, RuleSoldier* ruleSoldier, int nationality) const
{
	Soldier *soldier = 0;
	int newId = save->getId("STR_SOLDIER");

	// Check for duplicates
	// Original X-COM gives up after 10 tries so might as well do the same here
	bool duplicate = true;
	for (int tries = 0; tries < 10 && duplicate; ++tries)
	{
		delete soldier;
		soldier = new Soldier(ruleSoldier, ruleSoldier->getDefaultArmor(), nationality, newId);
		duplicate = false;
		for (std::vector<Base*>::iterator i = save->getBases()->begin(); i != save->getBases()->end() && !duplicate; ++i)
		{
			for (std::vector<Soldier*>::iterator j = (*i)->getSoldiers()->begin(); j != (*i)->getSoldiers()->end() && !duplicate; ++j)
			{
				if ((*j)->getName() == soldier->getName())
				{
					duplicate = true;
				}
			}
			for (std::vector<Transfer*>::iterator k = (*i)->getTransfers()->begin(); k != (*i)->getTransfers()->end() && !duplicate; ++k)
			{
				if ((*k)->getType() == TRANSFER_SOLDIER && (*k)->getSoldier()->getName() == soldier->getName())
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
		for (std::vector<std::string>::const_iterator i = _facilitiesIndex.begin(); i != _facilitiesIndex.end(); ++i)
		{
			RuleBaseFacility *f = getBaseFacility(*i);
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

const std::vector<SDL_Color> *Mod::getTransparencies() const
{
	return &_transparencies;
}

const std::vector<MapScript*> *Mod::getMapScript(const std::string& id) const
{
	std::map<std::string, std::vector<MapScript*> >::const_iterator i = _mapScripts.find(id);
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
	return getResearch(_finalResearch, true);
}

RuleBaseFacility *Mod::getDestroyedFacility() const
{
	if (isEmptyRuleName(_destroyedFacility))
		return 0;

	auto temp = getBaseFacility(_destroyedFacility, true);
	if (temp->getSize() != 1)
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

	auto geographFiles = FileMap::getVFolderContents("GEOGRAPH");
	auto scrs = FileMap::filterFiles(geographFiles, "SCR");
	for (auto i = scrs.begin(); i != scrs.end(); ++i)
	{
		std::string fname = *i;
		std::transform(i->begin(), i->end(), fname.begin(), toupper);
		_surfaces[fname] = new Surface(320, 200);
		_surfaces[fname]->loadScr("GEOGRAPH/" + fname);
	}
	auto bdys = FileMap::filterFiles(geographFiles, "BDY");
	for (auto i = bdys.begin(); i != bdys.end(); ++i)
	{
		std::string fname = *i;
		std::transform(i->begin(), i->end(), fname.begin(), toupper);
		_surfaces[fname] = new Surface(320, 200);
		_surfaces[fname]->loadBdy("GEOGRAPH/" + fname);
	}

	auto spks = FileMap::filterFiles(geographFiles, "SPK");
	for (auto i = spks.begin(); i != spks.end(); ++i)
	{
		std::string fname = *i;
		std::transform(i->begin(), i->end(), fname.begin(), toupper);
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
		auto contents = FileMap::getVFolderContents("SOUND");
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
			for (auto i : _soundDefs)
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
					for (auto j : i.second->getSoundList())
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
	auto unitsContents = FileMap::getVFolderContents("UNITS");
	auto usets = FileMap::filterFiles(unitsContents, "PCK");
	for (auto i = usets.begin(); i != usets.end(); ++i)
	{
		std::string fname = *i;
		std::transform(i->begin(), i->end(), fname.begin(), toupper);
		if (fname != "BIGOBS.PCK")
			_sets[fname] = new SurfaceSet(32, 40);
		else
			_sets[fname] = new SurfaceSet(32, 48);
		_sets[fname]->loadPck("UNITS/" + *i, "UNITS/" + CrossPlatform::noExt(*i) + ".TAB");
	}
	// incomplete chryssalid set: 1.0 data: stop loading.
	if (_sets.find("CHRYS.PCK") != _sets.end() && !_sets["CHRYS.PCK"]->getFrame(225))
	{
		Log(LOG_FATAL) << "Version 1.0 data detected";
		throw Exception("Invalid CHRYS.PCK, please patch your X-COM data to the latest version");
	}
	// TFTD uses the loftemps dat from the terrain folder, but still has enemy unknown's version in the geodata folder, which is short by 2 entries.
	auto terrainContents = FileMap::getVFolderContents("TERRAIN");
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

	auto ufographContents = FileMap::getVFolderContents("UFOGRAPH");
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
	for (auto i = bdys.begin(); i != bdys.end(); ++i)
	{
		std::string idxName = *i;
		std::transform(i->begin(), i->end(), idxName.begin(), toupper);
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
		_surfaces[idxName]->loadBdy("UFOGRAPH/" + *i);
	}

	// Load Battlescape inventory
	auto invs = FileMap::filterFiles(ufographContents, "SPK");
	for (auto i = invs.begin(); i != invs.end(); ++i)
	{
		std::string fname = *i;
		std::transform(i->begin(), i->end(), fname.begin(), toupper);
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
	YAML::Node doc = FileMap::getYAML("Language/" + _fontName);
	Log(LOG_INFO) << "Loading fonts... " << _fontName;
	for (YAML::const_iterator i = doc["fonts"].begin(); i != doc["fonts"].end(); ++i)
	{
		std::string id = (*i)["id"].as<std::string>();
		Font *font = new Font();
		font->load(*i);
		_fonts[id] = font;
	}

#ifndef __NO_MUSIC
	// Load musics
	if (!Options::mute)
	{
		auto soundFiles = FileMap::getVFolderContents("SOUND");

		// Check which music version is available
		CatFile *adlibcat = 0, *aintrocat = 0;
		GMCatFile *gmcat = 0;

		for (auto i = soundFiles.begin(); i != soundFiles.end(); ++i)
		{
			if (0 == i->compare("adlib.cat"))
			{
				adlibcat = new CatFile("SOUND/" + *i);
			}
			else if (0 == i->compare("aintro.cat"))
			{
				aintrocat = new CatFile("SOUND/" + *i);
			}
			else if (0 == i->compare("gm.cat"))
			{
				gmcat = new GMCatFile("SOUND/" + *i);
			}
		}

		// Try the preferred format first, otherwise use the default priority
		MusicFormat priority[] = { Options::preferredMusic, MUSIC_FLAC, MUSIC_OGG, MUSIC_MP3, MUSIC_MOD, MUSIC_WAV, MUSIC_ADLIB, MUSIC_GM, MUSIC_MIDI };
		for (std::map<std::string, RuleMusic *>::const_iterator i = _musicDefs.begin(); i != _musicDefs.end(); ++i)
		{
			Music *music = 0;
			for (size_t j = 0; j < ARRAYLEN(priority) && music == 0; ++j)
			{
				music = loadMusic(priority[j], (*i).second, adlibcat, aintrocat, gmcat);
			}
			if (music)
			{
				_musics[(*i).first] = music;
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
		for (std::map<std::string, std::vector<ExtraSprites *> >::const_iterator i = _extraSprites.begin(); i != _extraSprites.end(); ++i)
		{
			for (std::vector<ExtraSprites*>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				loadExtraSprite(*j);
			}
		}
	}

	if (!Options::mute)
	{
		for (std::vector< std::pair<std::string, ExtraSounds *> >::const_iterator i = _extraSounds.begin(); i != _extraSounds.end(); ++i)
		{
			std::string setName = i->first;
			ExtraSounds *soundPack = i->second;
			SoundSet *set = 0;

			std::map<std::string, SoundSet*>::iterator j = _sounds.find(setName);
			if (j != _sounds.end())
			{
				set = j->second;
			}
			_sounds[setName] = soundPack->loadSoundSet(set);
		}
	}

	Log(LOG_INFO) << "Loading custom palettes from ruleset...";
	for (std::map<std::string, CustomPalettes *>::const_iterator i = _customPalettes.begin(); i != _customPalettes.end(); ++i)
	{
		CustomPalettes *palDef = i->second;
		std::string palTargetName = palDef->getTarget();
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
		std::string fileName = palDef->getFile();
		if (fileName.empty())
		{
			for (std::map<int, Position>::iterator j = palDef->getPalette()->begin(); j != palDef->getPalette()->end(); ++j)
			{
				target->setColor(j->first, j->second.x, j->second.y, j->second.z);
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
	for (auto pal : _palettes)
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
		std::map<std::string, Surface*>::iterator i = _surfaces.find(spritePack->getType());
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
		std::map<std::string, SurfaceSet*>::iterator i = _sets.find(spritePack->getType());
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
	auto soundContents = FileMap::getVFolderContents("SOUND");
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
	const int opacityMax = 4;
	const SDL_Color* palColors = pal->getColors(0);
	std::vector<Uint8> lookUpTable;
	// start with the color sets
	lookUpTable.reserve(_transparencies.size() * 256 * opacityMax);
	for (std::vector<SDL_Color>::const_iterator tint = _transparencies.begin(); tint != _transparencies.end(); ++tint)
	{
		// then the opacity levels, using the alpha channel as the step
		for (int opacity = 1; opacity <= opacityMax; ++opacity)
		{
			// pseudo interpolation of palette color with tint
			// for small values `op` its should behave same as original TFTD
			// but for bigger values it make result closer to tint color
			const int op = Clamp(opacity * tint->unused, 0, 64);
			const float co = 1.0f - Sqr(op / 64.0f); // 1.0 -> 0.0
			const float to = op * 1.0f; // 0.0 -> 64.0

			// then the palette itself
			for (int currentColor = 0; currentColor < 256; ++currentColor)
			{
				SDL_Color desiredColor;

				desiredColor.r = std::min(255, (int)Round((palColors[currentColor].r * co) + (tint->r * to)));
				desiredColor.g = std::min(255, (int)Round((palColors[currentColor].g * co) + (tint->g * to)));
				desiredColor.b = std::min(255, (int)Round((palColors[currentColor].b * co) + (tint->b * to)));

				Uint8 closest = currentColor;
				int lowestDifference = INT_MAX;
				// if opacity is zero then we stay with current color, transparet color will stay same too
				if (op != 0 && currentColor != 0)
				{
					// now compare each color in the palette to find the closest match to our desired one
					for (int comparator = 1; comparator < 256; ++comparator)
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
	_transparencyLUTs.push_back(lookUpTable);
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

}
