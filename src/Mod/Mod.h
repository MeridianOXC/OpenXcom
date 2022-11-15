#pragma once
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
#include <map>
#include <vector>
#include <string>
#include <bitset>
#include <SDL.h>
#include <yaml-cpp/yaml.h>
#include "../Engine/Options.h"
#include "../Engine/FileMap.h"
#include "../Engine/Collections.h"
#include "../Savegame/GameTime.h"
#include "RuleDamageType.h"
#include "RuleAlienMission.h"
#include "RuleBaseFacilityFunctions.h"
#include "RuleItem.h"

namespace OpenXcom
{

class Surface;
class SurfaceSet;
class Font;
class Palette;
class Music;
class SoundSet;
class Sound;
class CatFile;
class GMCatFile;
class Music;
class Palette;
class SavedGame;
class Soldier;
class RuleCountry;
class RuleRegion;
class RuleBaseFacility;
class RuleCraft;
class RuleCraftWeapon;
class RuleItemCategory;
class RuleItem;
struct RuleDamageType;
class RuleUfo;
class RuleTerrain;
class MapDataSet;
class RuleSkill;
class RuleSoldier;
class Unit;
class Armor;
class ArticleDefinition;
class RuleInventory;
class RuleResearch;
class RuleManufacture;
class RuleManufactureShortcut;
class RuleSoldierBonus;
class RuleSoldierTransformation;
class AlienRace;
class RuleEnviroEffects;
class RuleStartingCondition;
class AlienDeployment;
class UfoTrajectory;
class RuleAlienMission;
class Base;
class MCDPatch;
class ExtraSprites;
class ExtraSounds;
class CustomPalettes;
class ExtraStrings;
class RuleCommendations;
class StatString;
class RuleInterface;
class RuleGlobe;
class RuleConverter;
class SoundDefinition;
class MapScript;
class ModInfo;
class RuleVideo;
class RuleMusic;
class RuleArcScript;
class RuleEventScript;
class RuleEvent;
class RuleMissionScript;
class ModScript;
class ModScriptGlobal;
class ScriptParserBase;
class ScriptGlobal;
struct StatAdjustment;

enum GameDifficulty : int;

/**
 * Mod data used when loading resources
 */
struct ModData
{
	/// Mod name
	std::string name;
	/// Optional info about mod
	const ModInfo* info;
	/// Offset that mod use is common sets
	size_t offset;
	/// Maximum size allowed by mod in common sets
	size_t size;
};

/**
 * Helper exception representing the final message with all the required context for the end user to fix the errors in rulesets
 */
struct LoadRuleException : Exception
{
	LoadRuleException(const std::string& parent, const YAML::Node &node, const std::string& message) : Exception{ "Error for '" + parent + "': " + message + " at line " + std::to_string(node.Mark().line)}
	{

	}

	LoadRuleException(const std::string& parent, const std::string& message) : Exception{ "Error for '" + parent + "': " + message}
	{

	}
};

/**
 * Contains all the game-specific static data that never changes
 * throughout the game, like rulesets and resources.
 */
class Mod
{
private:
	Music *_muteMusic;
	Sound *_muteSound;
	std::string _playingMusic, _currentMusicTrack;

	std::map<std::string, Palette*> _palettes;
	std::map<std::string, Font*> _fonts;
	std::map<std::string, Surface*> _surfaces;
	std::map<std::string, SurfaceSet*> _sets;
	std::map<std::string, SoundSet*> _sounds;
	std::map<std::string, Music*> _musics;
	std::vector<Uint16> _voxelData;
	std::vector<std::vector<Uint8> > _transparencyLUTs;

	std::map<std::string, RuleCountry*> _countries, _extraGlobeLabels;
	std::map<std::string, RuleRegion*> _regions;
	std::map<std::string, RuleBaseFacility*> _facilities;
	std::map<std::string, RuleCraft*> _crafts;
	std::map<std::string, RuleCraftWeapon*> _craftWeapons;
	std::map<std::string, RuleItemCategory*> _itemCategories;
	std::map<std::string, RuleItem*> _items;
	std::map<std::string, RuleUfo*> _ufos;
	std::map<std::string, RuleTerrain*> _terrains;
	std::map<std::string, MapDataSet*> _mapDataSets;
	std::map<std::string, RuleSkill*> _skills;
	std::map<std::string, RuleSoldier*> _soldiers;
	std::map<std::string, Unit*> _units;
	std::map<std::string, AlienRace*> _alienRaces;
	std::map<std::string, RuleEnviroEffects*> _enviroEffects;
	std::map<std::string, RuleStartingCondition*> _startingConditions;
	std::map<std::string, AlienDeployment*> _alienDeployments;
	std::map<std::string, Armor*> _armors;
	std::map<std::string, ArticleDefinition*> _ufopaediaArticles;
	std::map<std::string, RuleInventory*> _invs;
	bool _inventoryOverlapsPaperdoll;
	std::map<std::string, RuleResearch *> _research;
	std::map<std::string, RuleManufacture *> _manufacture;
	std::map<std::string, RuleManufactureShortcut *> _manufactureShortcut;
	std::map<std::string, RuleSoldierBonus *> _soldierBonus;
	std::map<std::string, RuleSoldierTransformation *> _soldierTransformation;
	std::map<std::string, UfoTrajectory *> _ufoTrajectories;
	std::map<std::string, RuleAlienMission *> _alienMissions;
	std::map<std::string, RuleInterface *> _interfaces;
	std::map<std::string, SoundDefinition *> _soundDefs;
	std::map<std::string, RuleVideo *>_videos;
	std::map<std::string, MCDPatch *> _MCDPatches;
	std::map<std::string, std::vector<MapScript *> > _mapScripts;
	std::map<std::string, RuleCommendations *> _commendations;
	std::map<std::string, RuleArcScript*> _arcScripts;
	std::map<std::string, RuleEventScript*> _eventScripts;
	std::map<std::string, RuleEvent*> _events;
	std::map<std::string, RuleMissionScript*> _missionScripts;
	std::map<std::string, std::vector<ExtraSprites *> > _extraSprites;
	std::map<std::string, CustomPalettes *> _customPalettes;
	std::vector<std::pair<std::string, ExtraSounds *> > _extraSounds;
	std::map<std::string, ExtraStrings *> _extraStrings;
	std::vector<StatString*> _statStrings;
	std::vector<RuleDamageType*> _damageTypes;
	std::map<std::string, RuleMusic *> _musicDefs;

	RuleGlobe *_globe;
	RuleConverter *_converter;
	ModScriptGlobal *_scriptGlobal;

	int _maxViewDistance, _maxDarknessToSeeUnits;
	int _maxStaticLightDistance, _maxDynamicLightDistance, _enhancedLighting;
	int _costHireEngineer, _costHireScientist;
	int _costEngineer, _costScientist, _timePersonnel, _hireByCountryOdds, _hireByRegionOdds, _initialFunding;
	int _aiUseDelayBlaster, _aiUseDelayFirearm, _aiUseDelayGrenade, _aiUseDelayMelee, _aiUseDelayPsionic;
	int _aiFireChoiceIntelCoeff, _aiFireChoiceAggroCoeff;
	bool _aiExtendedFireModeChoice, _aiRespectMaxRange, _aiDestroyBaseFacilities;
	bool _aiPickUpWeaponsMoreActively, _aiPickUpWeaponsMoreActivelyCiv;
	int _maxLookVariant, _tooMuchSmokeThreshold, _customTrainingFactor, _minReactionAccuracy;
	int _chanceToStopRetaliation;
	bool _lessAliensDuringBaseDefense;
	bool _allowCountriesToCancelAlienPact, _buildInfiltrationBaseCloseToTheCountry;
	bool _allowAlienBasesOnWrongTextures;
	int _kneelBonusGlobal, _oneHandedPenaltyGlobal;
	int _enableCloseQuartersCombat, _closeQuartersAccuracyGlobal, _closeQuartersTuCostGlobal, _closeQuartersEnergyCostGlobal, _closeQuartersSneakUpGlobal;
	int _noLOSAccuracyPenaltyGlobal;
	int _surrenderMode;
	int _bughuntMinTurn, _bughuntMaxEnemies, _bughuntRank, _bughuntLowMorale, _bughuntTimeUnitsLeft;

	int _manaMissingWoundThreshold = 200;
	int _healthMissingWoundThreshold = 100;
	bool _manaEnabled, _manaBattleUI, _manaTrainingPrimary, _manaTrainingSecondary, _manaReplenishAfterMission;
	bool _healthReplenishAfterMission = true;
	std::string _manaUnlockResearch;

	std::string _loseMoney, _loseRating, _loseDefeat;
	int _ufoGlancingHitThreshold, _ufoBeamWidthParameter;
	int _ufoTractorBeamSizeModifiers[5];
	int _escortRange, _drawEnemyRadarCircles;
	bool _escortsJoinFightAgainstHK, _hunterKillerFastRetarget;
	int _crewEmergencyEvacuationSurvivalChance, _pilotsEmergencyEvacuationSurvivalChance;
	int _soldiersPerSergeant, _soldiersPerCaptain, _soldiersPerColonel, _soldiersPerCommander;
	int _pilotAccuracyZeroPoint, _pilotAccuracyRange, _pilotReactionsZeroPoint, _pilotReactionsRange;
	int _pilotBraveryThresholds[3];
	int _performanceBonusFactor;
	bool _enableNewResearchSorting;
	int _displayCustomCategories;
	bool _shareAmmoCategories, _showDogfightDistanceInKm, _showFullNameInAlienInventory;
	int _alienInventoryOffsetX, _alienInventoryOffsetBigUnit;
	bool _hidePediaInfoButton;
	int _extraNerdyPediaInfoType;
	bool _giveScoreAlsoForResearchedArtifacts, _statisticalBulletConservation, _stunningImprovesMorale;
	int _tuRecoveryWakeUpNewTurn;
	int _shortRadarRange;
	int _buildTimeReductionScaling;
	int _defeatScore, _defeatFunds;
	bool _difficultyDemigod;
	std::pair<std::string, int> _alienFuel;
	std::string _fontName, _finalResearch, _psiUnlockResearch, _fakeUnderwaterBaseUnlockResearch, _newBaseUnlockResearch;
	std::string _hireScientistsUnlockResearch, _hireEngineersUnlockResearch;
	RuleBaseFacilityFunctions _hireScientistsRequiresBaseFunc, _hireEngineersRequiresBaseFunc;

	std::string _destroyedFacility;
	YAML::Node _startingBaseDefault, _startingBaseBeginner, _startingBaseExperienced, _startingBaseVeteran, _startingBaseGenius, _startingBaseSuperhuman;
	Collections::NamesToIndex _baseFunctionNames;

	GameTime _startingTime;
	int _startingDifficulty;
	int _baseDefenseMapFromLocation;
	std::map<int, std::string> _missionRatings, _monthlyRatings;
	std::map<std::string, std::string> _fixedUserOptions, _recommendedUserOptions;
	std::vector<std::string> _hiddenMovementBackgrounds;
	std::vector<std::string> _baseNamesFirst, _baseNamesMiddle, _baseNamesLast;
	std::vector<std::string> _operationNamesFirst, _operationNamesLast;
	bool _disableUnderwaterSounds;
	bool _enableUnitResponseSounds;
	std::map<std::string, std::vector<int> > _selectUnitSound, _startMovingSound, _selectWeaponSound, _annoyedSound;
	std::vector<int> _selectBaseSound, _startDogfightSound;
	std::vector<int> _flagByKills;
	int _pediaReplaceCraftFuelWithRangeType;
	std::vector<StatAdjustment> _statAdjustment;

	// overrides for DIFFICULTY_COEFFICIENT[]
	std::vector<int> _monthlyRatingThresholds;
	std::vector<int> _ufoFiringRateCoefficients;
	std::vector<int> _ufoEscapeCountdownCoefficients;
	std::vector<int> _retaliationTriggerOdds;
	std::vector<int> _retaliationBaseRegionOdds;
	std::vector<int> _aliensFacingCraftOdds;

	std::map<std::string, int> _ufopaediaSections;
	std::vector<std::string> _countriesIndex, _extraGlobeLabelsIndex, _regionsIndex, _facilitiesIndex, _craftsIndex, _craftWeaponsIndex, _itemCategoriesIndex, _itemsIndex, _invsIndex, _ufosIndex;
	std::vector<std::string> _aliensIndex, _enviroEffectsIndex, _startingConditionsIndex, _deploymentsIndex, _armorsIndex, _ufopaediaIndex, _ufopaediaCatIndex, _researchIndex, _manufactureIndex;
	std::vector<std::string> _skillsIndex, _soldiersIndex, _soldierTransformationIndex, _soldierBonusIndex;
	std::vector<std::string> _alienMissionsIndex, _terrainIndex, _customPalettesIndex, _arcScriptIndex, _eventScriptIndex, _eventIndex, _missionScriptIndex;
	std::vector<std::vector<int> > _alienItemLevels;
	std::vector<SDL_Color> _transparencies;
	int _facilityListOrder, _craftListOrder, _itemCategoryListOrder, _itemListOrder, _researchListOrder,  _manufactureListOrder;
	int _soldierBonusListOrder, _transformationListOrder, _ufopaediaListOrder, _invListOrder, _soldierListOrder;
	std::vector<ModData> _modData;
	ModData* _modCurrent;
	const SDL_Color *_statePalette;

	std::vector<std::string> _psiRequirements; // it's a cache for psiStrengthEval
	std::vector<const Armor*> _armorsForSoldiersCache;
	std::vector<const RuleItem*> _armorStorageItemsCache;
	std::vector<const RuleItem*> _craftWeaponStorageItemsCache;

	size_t _surfaceOffsetBigobs = 0;
	size_t _surfaceOffsetFloorob = 0;
	size_t _surfaceOffsetHandob = 0;
	size_t _surfaceOffsetSmoke = 0;
	size_t _surfaceOffsetHit = 0;
	size_t _surfaceOffsetBasebits = 0;
	size_t _soundOffsetBattle = 0;
	size_t _soundOffsetGeo = 0;

	/// Loads a ruleset from a YAML file that have basic resources configuration.
	void loadResourceConfigFile(const FileMap::FileRecord &filerec);
	void loadConstants(const YAML::Node &node);
	/// Loads a ruleset from a YAML file.
	void loadFile(const FileMap::FileRecord &filerec, ModScript &parsers);
	/// Loads a ruleset element.
	template <typename T>
	T *loadRule(const YAML::Node &node, std::map<std::string, T*> *map, std::vector<std::string> *index = 0, const std::string &key = "type") const;
	/// Gets a ruleset element.
	template <typename T>
	T *getRule(const std::string &id, const std::string &name, const std::map<std::string, T*> &map, bool error) const;
	/// Gets a random music. This is private to prevent access, use playMusic(name, true) instead.
	Music *getRandomMusic(const std::string &name) const;
	/// Gets a particular sound set. This is private to prevent access, use getSound(name, id) instead.
	SoundSet *getSoundSet(const std::string &name, bool error = true) const;
	/// Loads battlescape specific resources.
	void loadBattlescapeResources();
	/// Loads a specified music file.
	Music* loadMusic(MusicFormat fmt, RuleMusic* rule, CatFile* adlibcat, CatFile* aintrocat, GMCatFile* gmcat) const;
	/// Creates a transparency lookup table for a given palette.
	void createTransparencyLUT(Palette *pal);
	/// Loads a specified mod content.
	void loadMod(const std::vector<FileMap::FileRecord> &rulesetFiles, ModScript &parsers);
	/// Loads resources from vanilla.
	void loadVanillaResources();
	/// Loads resources from extra rulesets.
	void loadExtraResources();
	/// Loads surfaces on demand.
	void lazyLoadSurface(const std::string &name);
	/// Loads an external sprite.
	void loadExtraSprite(ExtraSprites *spritePack);
	/// Applies mods to vanilla resources.
	void modResources();
	/// Sorts all our lists according to their weight.
	void sortLists();
public:
	static int DOOR_OPEN;
	static int SLIDING_DOOR_OPEN;
	static int SLIDING_DOOR_CLOSE;
	static int SMALL_EXPLOSION;
	static int LARGE_EXPLOSION;
	static int EXPLOSION_OFFSET;
	static int SMOKE_OFFSET;
	static int UNDERWATER_SMOKE_OFFSET;

	/// Empty surface.
	constexpr static int NO_SURFACE = -1;
	/// Empty sound.
	constexpr static int NO_SOUND = -1;
	/// Special value for default string different to empty one.
	static const std::string STR_NULL;

	static int ITEM_DROP;
	static int ITEM_THROW;
	static int ITEM_RELOAD;
	static int WALK_OFFSET;
	static int FLYING_SOUND;
	static int BUTTON_PRESS;
	static int WINDOW_POPUP[3];
	static int UFO_FIRE;
	static int UFO_HIT;
	static int UFO_CRASH;
	static int UFO_EXPLODE;
	static int INTERCEPTOR_HIT;
	static int INTERCEPTOR_EXPLODE;

	static int GEOSCAPE_CURSOR;
	static int BASESCAPE_CURSOR;
	static int BATTLESCAPE_CURSOR;
	static int UFOPAEDIA_CURSOR;
	static int GRAPHS_CURSOR;
	static int DAMAGE_RANGE;
	static int EXPLOSIVE_DAMAGE_RANGE;
	static int FIRE_DAMAGE_RANGE[2];
	static std::string DEBRIEF_MUSIC_GOOD;
	static std::string DEBRIEF_MUSIC_BAD;
	static int DIFFICULTY_COEFFICIENT[5];
	static int SELL_PRICE_COEFFICIENT[5];
	static int DIFFICULTY_BASED_RETAL_DELAY[5];
	static int UNIT_RESPONSE_SOUNDS_FREQUENCY[4];
	static bool EXTENDED_ITEM_RELOAD_COST;
	static bool EXTENDED_RUNNING_COST;
	static bool EXTENDED_HWP_LOAD_ORDER;
	static int EXTENDED_MELEE_REACTIONS;
	static int EXTENDED_TERRAIN_MELEE;
	static int EXTENDED_UNDERWATER_THROW_FACTOR;


	/// Return `true` when given string is empty or pseudo null value.
	static bool isEmptyRuleName(const std::string& s)
	{
		return s.empty() || s == Mod::STR_NULL;
	}
	// reset all the statics in all classes to default values
	static void resetGlobalStatics();

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleMod";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Creates a blank mod.
	Mod();
	/// Cleans up the mod.
	~Mod();

	/// For internal use only
	const std::map<std::string, int> &getUfopaediaSections() const { return _ufopaediaSections; }

	/// Gets a particular font.
	Font *getFont(const std::string &name, bool error = true) const;
	/// Gets a particular surface.
	Surface *getSurface(const std::string &name, bool error = true);
	/// Gets a particular surface set.
	SurfaceSet *getSurfaceSet(const std::string &name, bool error = true);
	/// Gets a particular music.
	Music *getMusic(const std::string &name, bool error = true) const;
	/// Gets the available music tracks.
	const std::map<std::string, Music*> &getMusicTrackList() const;
	const std::string& getCurrentMusicTrack() const { return _currentMusicTrack; }
	void setCurrentMusicTrack(const std::string& currentMusicTrack) { _currentMusicTrack = currentMusicTrack; }
	/// Plays a particular music.
	void playMusic(const std::string &name, int id = 0);
	/// Gets a particular sound.
	Sound *getSound(const std::string &set, int sound) const;
	/// Gets all palettes.
	const std::map<std::string, Palette*> &getPalettes() const { return _palettes; }
	/// Gets a particular palette.
	Palette *getPalette(const std::string &name, bool error = true) const;
	/// Gets list of voxel data.
	const std::vector<Uint16> *getVoxelData() const;
	/// Returns a specific sound from either the land or underwater sound set.
	Sound *getSoundByDepth(unsigned int depth, unsigned int sound) const;
	/// Gets list of LUT data.
	const std::vector<std::vector<Uint8> > *getLUTs() const;

	/// Check for error that we can ignore by user request.
	bool checkForSoftError(bool check, const std::string &parent, const YAML::Node &node, const std::string &error, SeverityLevel level = LOG_WARNING) const
	{
		if (check)
		{
			auto ex = LoadRuleException(parent, node, error);
			if (Options::oxceModValidationLevel < level)
			{
				Log(level) << "Supressed " << ex.what();
				return true;
			}
			else
			{
				throw ex;
			}
		}
		return false;
	}

	/// Check for error that we can ignore by user request.
	bool checkForSoftError(bool check, const std::string &parent, const std::string &error, SeverityLevel level = LOG_WARNING) const
	{
		if (check)
		{
			auto ex = LoadRuleException(parent, error);
			if (Options::oxceModValidationLevel < level)
			{
				Log(level) << "Supressed " << ex.what();
				return true;
			}
			else
			{
				throw ex;
			}
		}
		return false;
	}

	/// Verify if value have defined surface in given set.
	void verifySpriteOffset(const std::string &parent, const int& sprite, const std::string &set) const;
	/// Verify if value have defined surface in given set.
	void verifySpriteOffset(const std::string &parent, const std::vector<int>& sprites, const std::string &set) const;
	/// Verify if value have defined sound in given set.
	void verifySoundOffset(const std::string &parent, const int& sound, const std::string &set) const;
	/// Verify if value have defined sound in given set.
	void verifySoundOffset(const std::string &parent, const std::vector<int>& sounds, const std::string &set) const;


	/// Gets the mod offset.
	int getModOffset() const;
	/// Get offset and index for sound set or sprite set.
	void loadOffsetNode(const std::string &parent, int& offset, const YAML::Node &node, int shared, const std::string &set, size_t multiplier, size_t sizeScale = 1) const;
	/// Gets the mod offset for a certain sprite.
	void loadSpriteOffset(const std::string &parent, int& sprite, const YAML::Node &node, const std::string &set, size_t multiplier = 1) const;
	/// Gets the mod offset array for a certain sprite.
	void loadSpriteOffset(const std::string &parent, std::vector<int>& sprites, const YAML::Node &node, const std::string &set) const;
	/// Gets the mod offset for a certain sound.
	void loadSoundOffset(const std::string &parent, int& sound, const YAML::Node &node, const std::string &set) const;
	/// Gets the mod offset array for a certain sound.
	void loadSoundOffset(const std::string &parent, std::vector<int>& sounds, const YAML::Node &node, const std::string &set) const;
	/// Gets the mod offset array for a certain transparency index.
	void loadTransparencyOffset(const std::string &parent, int& index, const YAML::Node &node) const;
	/// Gets the mod offset for a generic value.
	int getOffset(int id, int max) const;

	/// Gets base functions from string array in yaml.
	void loadBaseFunction(const std::string &parent, RuleBaseFacilityFunctions& f, const YAML::Node &node);
	/// Get names of function names in given bitset.
	std::vector<std::string> getBaseFunctionNames(RuleBaseFacilityFunctions f) const;

	/// Loads a list of ints.
	void loadInts(const std::string &parent, std::vector<int>& ints, const YAML::Node &node) const;
	/// Loads a list of ints where order of items does not matter.
	void loadUnorderedInts(const std::string &parent, std::vector<int>& ints, const YAML::Node &node) const;

	/// Loads a name.
	void loadName(const std::string &parent, std::string& names, const YAML::Node &node) const;
	/// Loads a name.
	void loadNameNull(const std::string &parent, std::string& names, const YAML::Node &node) const;
	/// Loads a list of names.
	void loadNames(const std::string &parent, std::vector<std::string>& names, const YAML::Node &node) const;
	/// Loads a list of names where order of items does not matter.
	void loadUnorderedNames(const std::string &parent, std::vector<std::string>& names, const YAML::Node &node) const;

	/// Loads a map from names to names.
	void loadNamesToNames(const std::string &parent, std::vector<std::pair<std::string, std::vector<std::string>>>& names, const YAML::Node &node) const;
	/// Loads a map from names to names.
	void loadUnorderedNamesToNames(const std::string &parent, std::map<std::string, std::string>& names, const YAML::Node &node) const;
	/// Loads a map from names to ints.
	void loadUnorderedNamesToInt(const std::string &parent, std::map<std::string, int>& names, const YAML::Node &node) const;
	/// Loads a map from names to vector of ints.
	void loadUnorderedNamesToInts(const std::string &parent, std::map<std::string, std::vector<int>>& names, const YAML::Node &node) const;
	/// Loads a map from names to names to int.
	void loadUnorderedNamesToNamesToInt(const std::string &parent, std::map<std::string, std::map<std::string, int>>& names, const YAML::Node &node) const;

	/// Loads data for kill criteria from Commendations.
	void loadKillCriteria(const std::string &parent, std::vector<std::vector<std::pair<int, std::vector<std::string> > > >& names, const YAML::Node &node) const;


	/// Convert names to correct rule objects
	template<typename T>
	void linkRule(const T*& rule, std::string& name) const
	{
		if constexpr (std::is_same_v<T, RuleItem>)
		{
			rule = getItem(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleSoldier>)
		{
			rule = getSoldier(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleSoldierBonus>)
		{
			rule = getSoldierBonus(name, true);
		}
		else if constexpr (std::is_same_v<T, Armor>)
		{
			rule = getArmor(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleResearch>)
		{
			rule = getResearch(name, true);
		}
		else if constexpr (std::is_same_v<T, Unit>)
		{
			rule = getUnit(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleSkill>)
		{
			rule = getSkill(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleCraft>)
		{
			rule = getCraft(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleUfo>)
		{
			rule = getUfo(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleBaseFacility>)
		{
			rule = getBaseFacility(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleInventory>)
		{
			rule = getInventory(name, true);
		}
		else if constexpr (std::is_same_v<T, RuleEvent>)
		{
			rule = getEvent(name, true);
		}
		else
		{
			static_assert(sizeof(T) == 0, "Unsupported type to link");
		}
		name = {};
	}
	/// Convert names to correct rule objects
	template<typename T>
	void linkRule(std::vector<T>& rule, std::vector<std::string>& names) const
	{
		rule.reserve(names.size());
		for (auto& n : names)
		{
			linkRule(rule.emplace_back(), n);
		}
		names = {};
	}
	/// Convert names to correct rule objects
	template<typename T>
	void linkRule(std::vector<T>& rule, std::vector<std::vector<std::string>>& names) const
	{
		rule.reserve(names.size());
		for (auto& n : names)
		{
			linkRule(rule.emplace_back(), n);
		}
		names = {};
	}


	/// Loads a list of mods.
	void loadAll();
	/// Generates the starting saved game.
	SavedGame *newSave(GameDifficulty diff) const;
	/// Gets the ruleset for a country type.
	RuleCountry *getCountry(const std::string &id, bool error = false) const;
	/// Gets the available countries.
	const std::vector<std::string> &getCountriesList() const;
	/// Gets the ruleset for an extra globe label type.
	RuleCountry *getExtraGlobeLabel(const std::string &id, bool error = false) const;
	/// Gets the available extra globe labels.
	const std::vector<std::string> &getExtraGlobeLabelsList() const;
	/// Gets the ruleset for a region type.
	RuleRegion *getRegion(const std::string &id, bool error = false) const;
	/// Gets the available regions.
	const std::vector<std::string> &getRegionsList() const;
	/// Gets the ruleset for a facility type.
	RuleBaseFacility *getBaseFacility(const std::string &id, bool error = false) const;
	/// Gets the available facilities.
	const std::vector<std::string> &getBaseFacilitiesList() const;
	/// Gets the ruleset for a craft type.
	RuleCraft *getCraft(const std::string &id, bool error = false) const;
	/// Gets the available crafts.
	const std::vector<std::string> &getCraftsList() const;

	/// Gets the ruleset for a craft weapon type.
	RuleCraftWeapon *getCraftWeapon(const std::string &id, bool error = false) const;
	/// Gets the available craft weapons.
	const std::vector<std::string> &getCraftWeaponsList() const;
	/// Is given item a launcher or ammo for craft weapon.
	bool isCraftWeaponStorageItem(const RuleItem* item) const;

	/// Gets the ruleset for an item category type.
	RuleItemCategory *getItemCategory(const std::string &id, bool error = false) const;
	/// Gets the available item categories.
	const std::vector<std::string> &getItemCategoriesList() const;
	/// Gets the ruleset for an item type.
	RuleItem *getItem(const std::string &id, bool error = false) const;
	/// Gets the available items.
	const std::vector<std::string> &getItemsList() const;
	/// Gets the ruleset for a UFO type.
	RuleUfo *getUfo(const std::string &id, bool error = false) const;
	/// Gets the available UFOs.
	const std::vector<std::string> &getUfosList() const;
	/// Gets terrains for battlescape games.
	RuleTerrain *getTerrain(const std::string &name, bool error = false) const;
	/// Gets the available terrains.
	const std::vector<std::string> &getTerrainList() const;
	/// Gets mapdatafile for battlescape games.
	MapDataSet *getMapDataSet(const std::string &name);
	/// Gets skill rules.
	RuleSkill *getSkill(const std::string &name, bool error = false) const;
	/// Gets soldier unit rules.
	RuleSoldier *getSoldier(const std::string &name, bool error = false) const;
	/// Gets the available soldiers.
	const std::vector<std::string> &getSoldiersList() const;
	/// Gets commendation rules.
	RuleCommendations *getCommendation(const std::string &id, bool error = false) const;
	/// Gets the available commendations.
	const std::map<std::string, RuleCommendations *> &getCommendationsList() const;
	/// Gets generated unit rules.
	Unit *getUnit(const std::string &name, bool error = false) const;
	/// Gets alien race rules.
	AlienRace *getAlienRace(const std::string &name, bool error = false) const;
	/// Gets the available alien races.
	const std::vector<std::string> &getAlienRacesList() const;
	/// Gets an enviro effects definition.
	RuleEnviroEffects* getEnviroEffects(const std::string& name) const;
	/// Gets the available enviro effects.
	const std::vector<std::string>& getEnviroEffectsList() const;
	/// Gets a starting condition.
	RuleStartingCondition* getStartingCondition(const std::string& name) const;
	/// Gets the available starting conditions.
	const std::vector<std::string>& getStartingConditionsList() const;
	/// Gets deployment rules.
	AlienDeployment *getDeployment(const std::string &name, bool error = false) const;
	/// Gets the available alien deployments.
	const std::vector<std::string> &getDeploymentsList() const;

	/// Gets armor rules.
	Armor *getArmor(const std::string &name, bool error = false) const;
	/// Gets the all armors.
	const std::vector<std::string> &getArmorsList() const;
	/// Gets the available armors for soldiers.
	const std::vector<const Armor*> &getArmorsForSoldiers() const;
	/// Check if item is used for armor storage.
	bool isArmorStorageItem(const RuleItem* item) const;

	/// Gets Ufopaedia article definition.
	ArticleDefinition *getUfopaediaArticle(const std::string &name, bool error = false) const;
	/// Gets the available articles.
	const std::vector<std::string> &getUfopaediaList() const;
	/// Gets the available article categories.
	const std::vector<std::string> &getUfopaediaCategoryList() const;

	/// Gets the inventory list.
	std::map<std::string, RuleInventory*> *getInventories();
	/// Gets the ruleset for a specific inventory.
	RuleInventory *getInventory(const std::string &id, bool error = false) const;
	/// Gets the ruleset for right hand inventory slot.
	RuleInventory *getInventoryRightHand() const { return getInventory("STR_RIGHT_HAND", true); }
	/// Gets the ruleset for left hand inventory slot.
	RuleInventory *getInventoryLeftHand() const { return getInventory("STR_LEFT_HAND", true); }
	/// Gets the ruleset for backpack inventory slot.
	RuleInventory *getInventoryBackpack() const { return getInventory("STR_BACK_PACK", true); }
	/// Gets the ruleset for belt inventory slot.
	RuleInventory *getInventoryBelt() const { return getInventory("STR_BELT", true); }
	/// Gets the ruleset for ground inventory slot.
	RuleInventory *getInventoryGround() const { return getInventory("STR_GROUND", true); }

	/// Gets whether or not the inventory slots overlap with the paperdoll button
	bool getInventoryOverlapsPaperdoll() const { return _inventoryOverlapsPaperdoll; }
	/// Gets max view distance in BattleScape.
	int getMaxViewDistance() const { return _maxViewDistance; }
	/// Gets threshold of darkness for LoS calculation.
	int getMaxDarknessToSeeUnits() const { return _maxDarknessToSeeUnits; }
	/// Gets max static (tiles & fire) light distance in BattleScape.
	int getMaxStaticLightDistance() const { return _maxStaticLightDistance; }
	/// Gets max dynamic (items & units) light distance in BattleScape.
	int getMaxDynamicLightDistance() const { return _maxDynamicLightDistance; }
	/// Get flags for enhanced lighting, 0x1 - tiles and fire, 0x2 - items, 0x4 - units.
	int getEnhancedLighting() const { return _enhancedLighting; }
	/// Get basic damage type
	const RuleDamageType *getDamageType(ItemDamageType type) const;

	/// Gets the cost of hiring an engineer.
	int getHireEngineerCost() const;
	/// Gets the research topic required for hiring new engineers.
	const std::string &getHireEngineersUnlockResearch() const { return _hireEngineersUnlockResearch; }
	/// Gets the base functions required for hiring new engineers.
	RuleBaseFacilityFunctions getHireEngineersRequiresBaseFunc() const { return _hireEngineersRequiresBaseFunc; }
	/// Gets the cost of hiring a scientist.
	int getHireScientistCost() const;
	/// Gets the research topic required for hiring new scientists.
	const std::string &getHireScientistsUnlockResearch() const { return _hireScientistsUnlockResearch; }
	/// Gets the base functions topic required for hiring new scientists.
	RuleBaseFacilityFunctions getHireScientistsRequiresBaseFunc() const { return _hireScientistsRequiresBaseFunc; }
	/// Gets the monthly cost of an engineer.
	int getEngineerCost() const;
	/// Gets the monthly cost of a scientist.
	int getScientistCost() const;
	/// Gets the transfer time of personnel.
	int getPersonnelTime() const;
	/// Gets the odds of hiring soldiers by country.
	int getHireByCountryOdds() const { return _hireByCountryOdds; }
	/// Gets the odds of hiring soldiers by region.
	int getHireByRegionOdds() const { return _hireByRegionOdds; }

	/// Gets first turn when AI can use Blaster launcher.
	int getAIUseDelayBlaster() const  {return _aiUseDelayBlaster;}
	/// Gets first turn when AI can use firearms.
	int getAIUseDelayFirearm() const  {return _aiUseDelayFirearm;}
	/// Gets first turn when AI can use grenades.
	int getAIUseDelayGrenade() const  {return _aiUseDelayGrenade;}
	/// Gets first turn when AI can use martial arts.
	int getAIUseDelayMelee() const {return _aiUseDelayMelee;}
	/// Gets first turn when AI can use psionic abilities.
	int getAIUseDelayPsionic() const  {return _aiUseDelayPsionic;}
	/// Gets how much AI intelligence should be used to determine firing mode for sniping.
	int getAIFireChoiceIntelCoeff() const {return _aiFireChoiceIntelCoeff;}
	/// Gets how much AI aggression should be used to determine firing mode for sniping.
	int getAIFireChoiceAggroCoeff() const {return _aiFireChoiceAggroCoeff;}
	/// Gets whether or not to use extended firing mode scoring for determining which attack the AI should use
	bool getAIExtendedFireModeChoice() const {return _aiExtendedFireModeChoice;}
	/// Gets whether or not the AI should try to shoot beyond a weapon's max range, true = don't shoot if you can't
	bool getAIRespectMaxRange() const {return _aiRespectMaxRange;}
	/// Gets whether or not the AI should be allowed to continue destroying base facilities after first encountering XCom
	bool getAIDestroyBaseFacilities() const { return _aiDestroyBaseFacilities; }
	/// Gets whether or not the alien AI should pick up weapons more actively.
	bool getAIPickUpWeaponsMoreActively() const { return _aiPickUpWeaponsMoreActively; }
	/// Gets whether or not the civilian AI should pick up weapons more actively.
	bool getAIPickUpWeaponsMoreActivelyCiv() const { return _aiPickUpWeaponsMoreActivelyCiv; }
	/// Gets maximum supported lookVariant (0-15)
	int getMaxLookVariant() const  {return abs(_maxLookVariant) % 16;}
	/// Gets the threshold for too much smoke (vanilla default = 10).
	int getTooMuchSmokeThreshold() const  {return _tooMuchSmokeThreshold;}
	/// Gets the custom physical training factor in percent (default = 100).
	int getCustomTrainingFactor() const { return _customTrainingFactor; }
	/// Gets the minimum firing accuracy for reaction fire (default = 0).
	int getMinReactionAccuracy() const { return _minReactionAccuracy; }
	/// Gets the chance to stop retaliation after unsuccessful xcom base attack (default = 0).
	int getChanceToStopRetaliation() const { return _chanceToStopRetaliation; }
	/// Should a damaged UFO deploy less aliens during the base defense?
	bool getLessAliensDuringBaseDefense() const { return _lessAliensDuringBaseDefense; }
	/// Will countries join the good side again after the infiltrator base is destroyed?
	bool getAllowCountriesToCancelAlienPact() const { return _allowCountriesToCancelAlienPact; }
	/// Should alien infiltration bases be built close to the infiltrated country?
	bool getBuildInfiltrationBaseCloseToTheCountry() const { return _buildInfiltrationBaseCloseToTheCountry; }
	/// Should alien bases be allowed (in worst case) on invalid globe textures or not?
	bool getAllowAlienBasesOnWrongTextures() const { return _allowAlienBasesOnWrongTextures; }
	/// Gets the global kneel bonus (default = 115).
	int getKneelBonusGlobal() const { return _kneelBonusGlobal; }
	/// Gets the global one-handed penalty (default = 80).
	int getOneHandedPenaltyGlobal() const { return _oneHandedPenaltyGlobal; }
	/// Gets whether close quarters combat is enabled (default = 0 is off).
	int getEnableCloseQuartersCombat() const { return _enableCloseQuartersCombat; }
	/// Gets the default close quarters combat accuracy (default = 100).
	int getCloseQuartersAccuracyGlobal() const { return _closeQuartersAccuracyGlobal; }
	/// Gets the default close quarters combat TU cost (default = 12).
	int getCloseQuartersTuCostGlobal() const { return _closeQuartersTuCostGlobal; }
	/// Gets the default close quarters combat energy cost (default = 8).
	int getCloseQuartersEnergyCostGlobal() const { return _closeQuartersEnergyCostGlobal; }
	/// Gets the percentage for successfully avoiding CQC when sneaking up on the enemy (default = 0% = turned off).
	int getCloseQuartersSneakUpGlobal() const { return _closeQuartersSneakUpGlobal; }
	/// Gets the default accuracy penalty for having no LOS to the target (default = 0 is no penalty)
	int getNoLOSAccuracyPenaltyGlobal() const { return _noLOSAccuracyPenaltyGlobal; }
	/// Gets the surrender mode (default = 0).
	int getSurrenderMode() const { return _surrenderMode; }
	/// Gets the bug hunt mode minimum turn requirement (default = 20).
	int getBughuntMinTurn() const { return _bughuntMinTurn; }
	/// Gets the bug hunt mode maximum remaining enemies requirement (default = 2).
	int getBughuntMaxEnemies() const { return _bughuntMaxEnemies; }
	/// Gets the bug hunt mode "VIP rank" parameter (default = 0).
	int getBughuntRank() const { return _bughuntRank; }
	/// Gets the bug hunt mode low morale threshold parameter (default = 40).
	int getBughuntLowMorale() const { return _bughuntLowMorale; }
	/// Gets the bug hunt mode time units % parameter (default = 60).
	int getBughuntTimeUnitsLeft() const { return _bughuntTimeUnitsLeft; }

	/// Is the mana feature enabled (default false)?
	bool isManaFeatureEnabled() const { return _manaEnabled; }
	/// Is the mana bar enabled for the Battlescape UI (default false)?
	bool isManaBarEnabled() const { return _manaBattleUI; }
	/// Is the mana trained as a primary skill (e.g. like firing accuracy)?
	bool isManaTrainingPrimary() const { return _manaTrainingPrimary; }
	/// Is the mana trained as a secondary skill (e.g. like strength)?
	bool isManaTrainingSecondary() const { return _manaTrainingSecondary; }
	/// Gets the mana unlock research topic (default empty)?
	const std::string &getManaUnlockResearch() const { return _manaUnlockResearch; }

	/// How much missing mana will act as "fatal wounds" and prevent the soldier from going into battle.
	int getManaWoundThreshold() const { return _manaMissingWoundThreshold; }
	/// Should a soldier's mana be fully replenished after a mission?
	bool getReplenishManaAfterMission() const { return _manaReplenishAfterMission; }

	/// How much missing health will act as "fatal wounds" and prevent the soldier from going into battle.
	int getHealthWoundThreshold() const { return _healthMissingWoundThreshold; }
	/// Should a soldier's health be fully replenished after a mission?
	bool getReplenishHealthAfterMission() const { return _healthReplenishAfterMission; }

	/// Gets the cutscene ID that should be played when the player loses due to poor economy management.
	const std::string &getLoseMoneyCutscene() const { return _loseMoney; }
	/// Gets the cutscene ID that should be played when the player loses due to poor rating.
	const std::string &getLoseRatingCutscene() const { return _loseRating; }
	/// Gets the cutscene ID that should be played when the player loses the last base.
	const std::string &getLoseDefeatCutscene() const { return _loseDefeat; }

	/// Gets the research topic required for building XCOM bases on fakeUnderwater globe textures.
	const std::string &getFakeUnderwaterBaseUnlockResearch() const { return _fakeUnderwaterBaseUnlockResearch; }
	/// Gets the research topic required for building XCOM bases.
	const std::string &getNewBaseUnlockResearch() const { return _newBaseUnlockResearch; }

	/// Gets the threshold for defining a glancing hit on a ufo during interception
	int getUfoGlancingHitThreshold() const { return _ufoGlancingHitThreshold; }
	/// Gets the parameter for drawing the width of a ufo's beam weapon based on power
	int getUfoBeamWidthParameter() const { return _ufoBeamWidthParameter; }
	/// Gets the modifier to a tractor beam's power based on a ufo's size
	int getUfoTractorBeamSizeModifier(int ufoSize) const { return _ufoTractorBeamSizeModifiers[ufoSize]; }
	/// Gets the escort range
	double getEscortRange() const;
	/// Gets the setting for drawing alien base/UFO radar circles.
	int getDrawEnemyRadarCircles() const { return _drawEnemyRadarCircles; }
	/// Should escorts join the fight against HK (automatically)? Or is only fighting one-on-one allowed?
	bool getEscortsJoinFightAgainstHK() const { return _escortsJoinFightAgainstHK; }
	/// Should hunter-killers be able to retarget every 5 seconds on slow game timers (5Sec / 1Min)?
	bool getHunterKillerFastRetarget() const { return _hunterKillerFastRetarget; }
	/// Gets the crew emergency evacuation survival chance
	int getCrewEmergencyEvacuationSurvivalChance() const { return _crewEmergencyEvacuationSurvivalChance; }
	/// Gets the pilots emergency evacuation survival chance
	int getPilotsEmergencyEvacuationSurvivalChance() const { return _pilotsEmergencyEvacuationSurvivalChance; }
	/// Gets how many soldiers are needed for one sergeant promotion
	int getSoldiersPerSergeant() const { return _soldiersPerSergeant; }
	/// Gets how many soldiers are needed for one captain promotion
	int getSoldiersPerCaptain() const { return _soldiersPerCaptain; }
	/// Gets how many soldiers are needed for one colonel promotion
	int getSoldiersPerColonel() const { return _soldiersPerColonel; }
	/// Gets how many soldiers are needed for one commander promotion
	int getSoldiersPerCommander() const { return _soldiersPerCommander; }
	/// Gets the firing accuracy needed for no bonus/penalty
	int getPilotAccuracyZeroPoint() const { return _pilotAccuracyZeroPoint; }
	/// Gets the firing accuracy impact (as percentage of distance to zero point) on pilot's aim in dogfight
	int getPilotAccuracyRange() const { return _pilotAccuracyRange; }
	/// Gets the reactions needed for no bonus/penalty
	int getPilotReactionsZeroPoint() const { return _pilotReactionsZeroPoint; }
	/// Gets the reactions impact (as percentage of distance to zero point) on pilot's dodge ability in dogfight
	int getPilotReactionsRange() const { return _pilotReactionsRange; }
	/// Gets the pilot's bravery needed for very bold approach speed
	int getPilotBraveryThresholdVeryBold() const { return _pilotBraveryThresholds[0]; }
	/// Gets the pilot's bravery needed for bold approach speed
	int getPilotBraveryThresholdBold() const { return _pilotBraveryThresholds[1]; }
	/// Gets the pilot's bravery needed for normal approach speed
	int getPilotBraveryThresholdNormal() const { return _pilotBraveryThresholds[2]; }
	/// Gets a performance bonus factor
	int getPerformanceBonusFactor() const { return _performanceBonusFactor; }
	/// Should the player have the option to sort the 'New Research' list?
	bool getEnableNewResearchSorting() const { return _enableNewResearchSorting; }
	/// Should custom categories be used in Buy/Sell/Transfer GUIs? 0=no, 1=yes, custom only, 2=both vanilla and custom.
	int getDisplayCustomCategories() const { return _displayCustomCategories; }
	/// Should weapons "inherit" categories of their ammo?
	bool getShareAmmoCategories() const { return _shareAmmoCategories; }
	/// Should distance in dogfight GUI be shown in kilometers?
	bool getShowDogfightDistanceInKm() const { return _showDogfightDistanceInKm; }
	/// Should alien inventory show full name (e.g. Sectoid Leader) or just the race (e.g. Sectoid)?
	bool getShowFullNameInAlienInventory() const { return _showFullNameInAlienInventory; }
	/// Gets the offset for alien inventory paperdoll and hand slots
	int getAlienInventoryOffsetX() const { return _alienInventoryOffsetX; }
	/// Gets the extra offset for alien inventory hand slots for 2x2 units
	int getAlienInventoryOffsetBigUnit() const { return _alienInventoryOffsetBigUnit; }
	/// Show the INFO button (where applicable) or not?
	bool getShowPediaInfoButton() const { return !_hidePediaInfoButton; }
	/// Display extra item info (accuracy modifier and power bonus) in the main pedia article?
	int getExtraNerdyPediaInfoType() const { return _extraNerdyPediaInfoType; }
	/// In debriefing, give score also for already researched alien artifacts?
	bool getGiveScoreAlsoForResearchedArtifacts() const { return _giveScoreAlsoForResearchedArtifacts; }
	/// When recovering ammo, should partially spent clip have a chance to recover as full?
	bool getStatisticalBulletConservation() const { return _statisticalBulletConservation; }
	/// Does stunning an enemy improve unit and squad morale?
	bool getStunningImprovesMorale() const { return _stunningImprovesMorale; }
	/// Gets how much TU (in percent) should be given to a unit waking up from stun at the beginning of a new turn.
	int getTURecoveryWakeUpNewTurn() const { return _tuRecoveryWakeUpNewTurn; }
	/// Gets whether or not to load base defense terrain from globe texture
	int getBaseDefenseMapFromLocation() const { return _baseDefenseMapFromLocation; }
	/// Gets the ruleset for a specific research project.
	RuleResearch *getResearch(const std::string &id, bool error = false) const;
	/// Gets the ruleset for a specific research project.
	std::vector<const RuleResearch*> getResearch(const std::vector<std::string> &id) const;
	/// Gets the ruleset for a specific research project.
	const std::map<std::string, RuleResearch *> &getResearchMap() const;
	/// Gets the list of all research projects.
	const std::vector<std::string> &getResearchList() const;
	/// Gets the ruleset for a specific manufacture project.
	RuleManufacture *getManufacture (const std::string &id, bool error = false) const;
	/// Gets the list of all manufacture projects.
	const std::vector<std::string> &getManufactureList() const;
	/// Gets the ruleset for a specific soldier bonus type.
	RuleSoldierBonus *getSoldierBonus(const std::string &id, bool error = false) const;
	/// Gets the list of all soldier bonus types.
	const std::vector<std::string> &getSoldierBonusList() const;
	/// Gets the ruleset for a specific soldier transformation project.
	RuleSoldierTransformation *getSoldierTransformation(const std::string &id, bool error = false) const;
	/// Gets the list of all soldier transformation projects.
	const std::vector<std::string> &getSoldierTransformationList() const;
	/// Gets facilities for custom bases.
	std::vector<RuleBaseFacility*> getCustomBaseFacilities(GameDifficulty diff) const;
	/// Gets a specific UfoTrajectory.
	const UfoTrajectory *getUfoTrajectory(const std::string &id, bool error = false) const;
	/// Gets the ruleset for a specific alien mission.
	const RuleAlienMission *getAlienMission(const std::string &id, bool error = false) const;
	/// Gets the ruleset for a random alien mission.
	const RuleAlienMission *getRandomMission(MissionObjective objective, size_t monthsPassed) const;
	/// Gets the list of all alien missions.
	const std::vector<std::string> &getAlienMissionList() const;
	/// Gets the alien item level table.
	const std::vector<std::vector<int> > &getAlienItemLevels() const;
	/// Gets the player starting base.
	const YAML::Node &getDefaultStartingBase() const;
	const YAML::Node &getStartingBase(GameDifficulty diff) const;
	/// Gets the game starting time.
	const GameTime &getStartingTime() const;
	/// Gets the game starting difficulty.
	int getStartingDifficulty() const { return _startingDifficulty; }
	/// Gets an MCDPatch.
	MCDPatch *getMCDPatch(const std::string &id) const;
	/// Gets the list of external Sprites.
	const std::map<std::string, std::vector<ExtraSprites *> > &getExtraSprites() const;
	/// Gets the list of custom palettes.
	const std::vector<std::string> &getCustomPalettes() const;
	/// Gets the list of external Sounds.
	const std::vector<std::pair<std::string, ExtraSounds *> > &getExtraSounds() const;
	/// Gets the list of external Strings.
	const std::map<std::string, ExtraStrings *> &getExtraStrings() const;
	/// Gets the list of StatStrings.
	const std::vector<StatString *> &getStatStrings() const;
	/// Gets the research-requirements for Psi-Lab (it's a cache for psiStrengthEval)
	const std::vector<std::string> &getPsiRequirements() const;
	/// Returns the sorted list of inventories.
	const std::vector<std::string> &getInvsList() const;
	/// Generates a new soldier.
	Soldier *genSoldier(SavedGame *save, RuleSoldier* ruleSoldier, int nationality) const;
	/// Gets the item to be used as fuel for ships.
	std::string getAlienFuelName() const;
	/// Gets the amount of alien fuel to recover
	int getAlienFuelQuantity() const;
	/// Gets the font name.
	std::string getFontName() const;
	/// Gets the maximum radar range still considered as short.
	int getShortRadarRange() const;
	/// Gets the custom scaling (defined by the modder) applied on the facility upgrade/build time reduction calculated by the game.
	int getBuildTimeReductionScaling() const { return _buildTimeReductionScaling; }
	/// Gets what type of information should be shown in craft articles for the fuel capacity/range
	int getPediaReplaceCraftFuelWithRangeType() const;
	/// Gets information on an interface element.
	RuleInterface *getInterface(const std::string &id, bool error = true) const;
	/// Gets the ruleset for the globe.
	RuleGlobe *getGlobe() const;
	/// Gets the ruleset for the converter.
	RuleConverter *getConverter() const;
	/// Gets the list of selective files for insertion into our cat files.
	const std::map<std::string, SoundDefinition *> *getSoundDefinitions() const;
	/// Gets the list of transparency colors,
	const std::vector<SDL_Color> *getTransparencies() const;
	const std::vector<MapScript*> *getMapScript(const std::string& id) const;
	const std::map<std::string, std::vector<MapScript*> > &getMapScriptsRaw() const { return _mapScripts; }
	/// Gets a video for intro/outro etc.
	RuleVideo *getVideo(const std::string &id, bool error = false) const;
	const std::map<std::string, RuleMusic *> *getMusic() const;
	const std::vector<std::string>* getArcScriptList() const;
	RuleArcScript* getArcScript(const std::string& name, bool error = false) const;
	const std::vector<std::string>* getEventScriptList() const;
	RuleEventScript* getEventScript(const std::string& name, bool error = false) const;
	const std::vector<std::string>* getEventList() const;
	RuleEvent* getEvent(const std::string& name, bool error = false) const;
	const std::vector<std::string> *getMissionScriptList() const;
	RuleMissionScript *getMissionScript(const std::string &name, bool error = false) const;
	/// Get global script data.
	ScriptGlobal *getScriptGlobal() const;
	RuleResearch *getFinalResearch() const;
	RuleBaseFacility *getDestroyedFacility() const;
	const std::map<int, std::string> *getMissionRatings() const;
	const std::map<int, std::string> *getMonthlyRatings() const;
	const std::map<std::string, std::string> &getFixedUserOptions() const { return _fixedUserOptions; }
	const std::map<std::string, std::string> &getRecommendedUserOptions() const { return _recommendedUserOptions; }
	const std::vector<std::string> &getHiddenMovementBackgrounds() const;
	const std::vector<std::string> &getBaseNamesFirst() const { return _baseNamesFirst; }
	const std::vector<std::string> &getBaseNamesMiddle() const { return _baseNamesMiddle; }
	const std::vector<std::string> &getBaseNamesLast() const { return _baseNamesLast; }
	const std::vector<std::string> &getOperationNamesFirst() const { return _operationNamesFirst; }
	const std::vector<std::string> &getOperationNamesLast() const { return _operationNamesLast; }
	bool getEnableUnitResponseSounds() const { return _enableUnitResponseSounds; }
	const std::map<std::string, std::vector<int> > &getSelectUnitSounds() const { return _selectUnitSound; }
	const std::map<std::string, std::vector<int> > &getStartMovingSounds() const { return _startMovingSound; }
	const std::map<std::string, std::vector<int> > &getSelectWeaponSounds() const { return _selectWeaponSound; }
	const std::map<std::string, std::vector<int> > &getAnnoyedSounds() const { return _annoyedSound; }
	const std::vector<int> &getSelectBaseSounds() const { return _selectBaseSound; }
	const std::vector<int> &getStartDogfightSounds() const { return _startDogfightSound; }
	const std::vector<int> &getFlagByKills() const;
	StatAdjustment *getStatAdjustment(int difficulty);
	int getDefeatScore() const;
	int getDefeatFunds() const;
	bool isDemigod() const;
	const std::vector<int>& getMonthlyRatingThresholds() { return _monthlyRatingThresholds; }
	const std::vector<int>& getUfoFiringRateCoefficients() { return _ufoFiringRateCoefficients; }
	const std::vector<int>& getUfoEscapeCountdownCoefficients() { return _ufoEscapeCountdownCoefficients; }
	const std::vector<int>& getRetaliationTriggerOdds() { return _retaliationTriggerOdds; }
	const std::vector<int>& getRetaliationBaseRegionOdds() { return _retaliationBaseRegionOdds; }
	const std::vector<int>& getAliensFacingCraftOdds() { return _aliensFacingCraftOdds; }

};

}
