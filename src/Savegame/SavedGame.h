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
#include <set>
#include <string>
#include <time.h>
#include <stdint.h>
#include "GameTime.h"
#include "../Mod/RuleAlienMission.h"
#include "../Mod/RuleEvent.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/RuleCraft.h"
#include "../Engine/Script.h"

namespace OpenXcom
{

class Mod;
class GameTime;
class Country;
class Base;
class Region;
class Ufo;
class Waypoint;
class SavedBattleGame;
class TextList;
class Language;
class RuleResearch;
class ResearchProject;
class Soldier;
class RuleManufacture;
class RuleItem;
class ArticleDefinition;
class MissionSite;
class AlienBase;
class AlienStrategy;
class AlienMission;
class GeoscapeEvent;
class Target;
class Soldier;
class Craft;
class EquipmentLayoutItem;
class ItemContainer;
class RuleSoldierTransformation;
class AlienRace;
struct MissionStatistics;
struct BattleUnitKills;

/**
 * Enumerator containing all the possible game difficulties.
 */
enum GameDifficulty : int { DIFF_BEGINNER = 0, DIFF_EXPERIENCED, DIFF_VETERAN, DIFF_GENIUS, DIFF_SUPERHUMAN };

/**
 * Enumerator for the various save types.
 */
enum SaveType { SAVE_DEFAULT, SAVE_QUICK, SAVE_AUTO_GEOSCAPE, SAVE_AUTO_BATTLESCAPE, SAVE_IRONMAN, SAVE_IRONMAN_END };

/**
 * Enumerator for the current game ending.
 */
enum GameEnding { END_NONE, END_WIN, END_LOSE };

/**
 * Container for savegame info displayed on listings.
 */
struct SaveInfo
{
	std::string fileName;
	std::string displayName;
	time_t timestamp;
	std::string isoDate, isoTime;
	std::string details;
	std::vector<std::string> mods;
	bool reserved;
};

/**
 * The game data that gets written to disk when the game is saved.
 * A saved game holds all the variable info in a game like funds,
 * game time, current bases and contents, world activities, score, etc.
 */
class SavedGame
{
public:
	Country *debugCountry = nullptr;
	Region *debugRegion = nullptr;
	int debugType = 0;
	size_t debugZone = 0;
	size_t debugArea = 0;

	/// Name of class used in script.
	static constexpr const char *ScriptName = "GeoscapeGame";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);


	static const int MAX_EQUIPMENT_LAYOUT_TEMPLATES = 50;
	static const int MAX_CRAFT_LOADOUT_TEMPLATES = 10;

private:
	std::string _name;
	GameDifficulty _difficulty;
	GameEnding _end;
	bool _ironman;
	GameTime *_time;
	std::vector<std::string> _userNotes;
	std::vector<std::string> _geoscapeDebugLog;
	std::vector<int> _researchScores;
	std::vector<int64_t> _funds, _maintenance, _incomes, _expenditures;
	double _globeLon, _globeLat;
	int _globeZoom;
	std::map<std::string, int> _ids;
	std::vector<Country*> _countries;
	std::vector<Region*> _regions;
	std::vector<Base*> _bases;
	std::vector<Ufo*> _ufos;
	std::vector<Waypoint*> _waypoints;
	std::vector<MissionSite*> _missionSites;
	std::vector<AlienBase*> _alienBases;
	AlienStrategy *_alienStrategy;
	SavedBattleGame *_battleGame;
	std::vector<const RuleResearch*> _discovered;
	std::map<std::string, int> _generatedEvents;
	std::map<std::string, int> _ufopediaRuleStatus;
	std::map<std::string, int> _manufactureRuleStatus;
	std::map<std::string, int> _researchRuleStatus;
	std::map<std::string, int> _monthlyPurchaseLimitLog;
	std::map<std::string, bool> _hiddenPurchaseItemsMap;
	std::map<std::string, RuleCraftDeployment> _customRuleCraftDeployments;
	Base* _previewBase;
	std::vector<AlienMission*> _activeMissions;
	std::vector<GeoscapeEvent*> _geoscapeEvents;
	bool _debug, _warned;
	bool _togglePersonalLight, _toggleNightVision;
	int _toggleBrightness;
	int _monthsPassed;
	int _daysPassed;
	int _vehiclesLost;
	std::string _graphRegionToggles;
	std::string _graphCountryToggles;
	std::string _graphFinanceToggles;
	std::vector<const RuleResearch*> _poppedResearch;
	std::vector<Soldier*> _deadSoldiers;
	size_t _selectedBase;
	std::string _lastselectedArmor; //contains the last selected armor
	std::string _globalEquipmentLayoutName[MAX_EQUIPMENT_LAYOUT_TEMPLATES];
	std::string _globalEquipmentLayoutArmor[MAX_EQUIPMENT_LAYOUT_TEMPLATES];
	std::vector<EquipmentLayoutItem*> _globalEquipmentLayout[MAX_EQUIPMENT_LAYOUT_TEMPLATES];
	std::string _globalCraftLoadoutName[MAX_CRAFT_LOADOUT_TEMPLATES];
	ItemContainer *_globalCraftLoadout[MAX_CRAFT_LOADOUT_TEMPLATES];
	std::vector<MissionStatistics*> _missionStatistics;
	std::set<int> _ignoredUfos;
	std::set<const RuleItem *> _autosales;
	bool _disableSoldierEquipment;
	bool _alienContainmentChecked;
	ScriptValues<SavedGame> _scriptValues;

	static SaveInfo getSaveInfo(const std::string &file, Language *lang);
public:
	static const std::string AUTOSAVE_GEOSCAPE, AUTOSAVE_BATTLESCAPE, QUICKSAVE;
	/// Creates a new saved game.
	SavedGame();
	/// Cleans up the saved game.
	~SavedGame();
	/// Sanitizes a mod name in a save.
	static std::string sanitizeModName(const std::string &name);
	/// Gets list of saves in the user directory.
	static std::vector<SaveInfo> getList(Language *lang, bool autoquick);
	/// Loads a saved game from YAML.
	void load(const std::string &filename, Mod *mod, Language *lang);
	void loadTemplates(const YAML::Node& doc, const Mod* mod);
	void loadUfopediaRuleStatus(const YAML::Node& node);
	/// Saves a saved game to YAML.
	void save(const std::string &filename, Mod *mod) const;
	/// Gets the game name.
	std::string getName() const;
	/// Sets the game name.
	void setName(const std::string &name);
	/// Gets the game difficulty.
	GameDifficulty getDifficulty() const;
	/// Sets the game difficulty.
	void setDifficulty(GameDifficulty difficulty);
	/// Gets the game difficulty coefficient.
	int getDifficultyCoefficient() const;
	/// Gets the sell price coefficient.
	int getSellPriceCoefficient() const;
	/// Gets the buy price coefficient.
	int getBuyPriceCoefficient() const;
	/// Gets the game ending.
	GameEnding getEnding() const;
	/// Sets the game ending.
	void setEnding(GameEnding end);
	/// Gets if the game is in ironman mode.
	bool isIronman() const;
	/// Sets if the game is in ironman mode.
	void setIronman(bool ironman);
	/// Gets the current funds.
	int64_t getFunds() const;
	/// Gets the list of funds from previous months.
	std::vector<int64_t> &getFundsList();
	/// Sets new funds.
	void setFunds(int64_t funds);
	/// Gets the current globe longitude.
	double getGlobeLongitude() const;
	/// Sets the new globe longitude.
	void setGlobeLongitude(double lon);
	/// Gets the current globe latitude.
	double getGlobeLatitude() const;
	/// Sets the new globe latitude.
	void setGlobeLatitude(double lat);
	/// Gets the current globe zoom.
	int getGlobeZoom() const;
	/// Sets the new globe zoom.
	void setGlobeZoom(int zoom);
	/// Handles monthly funding.
	void monthlyFunding();
	/// Gets the current game time.
	GameTime *getTime() const;
	/// Sets the current game time.
	void setTime(const GameTime& time);
	/// Gets the current ID for an object.
	int getId(const std::string &name);
	/// Gets the last ID for an object.
	int getLastId(const std::string& name);
	/// Increase a custom counter.
	void increaseCustomCounter(const std::string& name);
	/// Decrease a custom counter.
	void decreaseCustomCounter(const std::string& name);
	/// Resets the list of object IDs.
	const std::map<std::string, int> &getAllIds() const;
	/// Resets the list of object IDs.
	void setAllIds(const std::map<std::string, int> &ids);
	/// Gets the list of countries.
	std::vector<Country*> *getCountries();
	/// Gets the total country funding.
	int getCountryFunding() const;
	/// Gets the list of regions.
	std::vector<Region*> *getRegions();
	/// Gets the list of bases.
	std::vector<Base*> *getBases();
	/// Gets the list of bases.
	const std::vector<Base*> *getBases() const;
	/// Gets the total base maintenance.
	int getBaseMaintenance() const;
	/// Gets the list of UFOs.
	std::vector<Ufo*> *getUfos();
	/// Gets the list of UFOs.
	const std::vector<Ufo*> *getUfos() const;
	/// Gets the list of waypoints.
	std::vector<Waypoint*> *getWaypoints();
	/// Gets the list of mission sites.
	std::vector<MissionSite*> *getMissionSites();
	/// Gets the current battle game.
	SavedBattleGame *getSavedBattle();
	/// Sets the current battle game.
	void setBattleGame(SavedBattleGame *battleGame);
	/// Sets the status of a ufopedia rule
	void setUfopediaRuleStatus(const std::string &ufopediaRule, int newStatus);
	/// Sets the status of a manufacture rule
	void setManufactureRuleStatus(const std::string &manufactureRule, int newStatus);
	/// Sets the status of a research rule
	void setResearchRuleStatus(const std::string &researchRule, int newStatus);
	/// Sets the item as hidden or unhidden
	void setHiddenPurchaseItemsStatus(const std::string &itemName, bool hidden);
	/// Selects a "getOneFree" topic for the given research rule.
	const RuleResearch* selectGetOneFree(const RuleResearch* research);
	/// Remove a research from the "already discovered" list
	void removeDiscoveredResearch(const RuleResearch *research);
	/// Add a finished ResearchProject
	void addFinishedResearchSimple(const RuleResearch *research);
	/// Add a finished ResearchProject
	void addFinishedResearch(const RuleResearch *research, const Mod *mod, Base *base, bool score = true);
	/// Get the list of already discovered research projects
	const std::vector<const RuleResearch*> & getDiscoveredResearch() const;
	/// Get the list of ResearchProject which can be researched in a Base
	void getAvailableResearchProjects(std::vector<RuleResearch*> & projects, const Mod *mod, Base *base, bool considerDebugMode = false) const;
	/// Get the list of newly available research projects once a research has been completed.
	void getNewlyAvailableResearchProjects(std::vector<RuleResearch*> & before, std::vector<RuleResearch*> & after, std::vector<RuleResearch*> & diff) const;
	/// Get the list of Productions which can be manufactured in a Base
	void getAvailableProductions(std::vector<RuleManufacture*> & productions, const Mod *mod, Base *base, ManufacturingFilterType filter = MANU_FILTER_DEFAULT) const;
	/// Get the list of newly available manufacture projects once a research has been completed.
	void getDependableManufacture(std::vector<RuleManufacture*> & dependables, const RuleResearch *research, const Mod *mod, Base *base) const;
	/// Get the list of Soldier Transformations that can occur at a base
	void getAvailableTransformations(std::vector<RuleSoldierTransformation*> & transformations, const Mod *mod, Base *base) const;
	/// Get the list of newly available items to purchase once a research has been completed.
	void getDependablePurchase(std::vector<RuleItem*> & dependables, const RuleResearch *research, const Mod *mod) const;
	/// Get the list of newly available craft to purchase/rent once a research has been completed.
	void getDependableCraft(std::vector<RuleCraft*> & dependables, const RuleResearch *research, const Mod *mod) const;
	/// Get the list of newly available facilities to build once a research has been completed.
	void getDependableFacilities(std::vector<RuleBaseFacility*> & dependables, const RuleResearch *research, const Mod *mod) const;
	/// Gets the status of a ufopedia rule.
	int getUfopediaRuleStatus(const std::string &ufopediaRule);
	/// Gets the log of monthly purchase limits
	std::map<std::string, int>& getMonthlyPurchaseLimitLog() { return _monthlyPurchaseLimitLog; }
	/// Gets the list of hidden items
	const std::map<std::string, bool> &getHiddenPurchaseItems();
	/// Gets the list of player-defined save-specific RuleCraft '_deployment' overrides
	std::map<std::string, RuleCraftDeployment>& getCustomRuleCraftDeployments() { return _customRuleCraftDeployments; }
	/// Gets the preview base.
	Base* getPreviewBase() { return _previewBase; }
	/// Sets the preview base.
	void setPreviewBase(Base* previewBase) { _previewBase = previewBase; }
	/// Gets the status of a manufacture rule.
	int getManufactureRuleStatus(const std::string &manufactureRule);
	/// Gets all the research rule status info.
	const std::map<std::string, int> &getResearchRuleStatusRaw() const { return _researchRuleStatus; }
	/// Gets the status of a research rule.
	int getResearchRuleStatus(const std::string &researchRule) const;
	/// Is the research permanently disabled?
	bool isResearchRuleStatusDisabled(const std::string &researchRule) const;
	/// Gets if a research still has undiscovered non-disabled "getOneFree".
	bool hasUndiscoveredGetOneFree(const RuleResearch * r, bool checkOnlyAvailableTopics) const;
	/// Gets if a research still has undiscovered non-disabled "protected unlocks".
	bool hasUndiscoveredProtectedUnlock(const RuleResearch * r) const;
	/// Gets if a certain research has been completed.
	bool isResearched(const std::string &research, bool considerDebugMode = true) const;
	/// Gets if a certain research has been completed.
	bool isResearched(const RuleResearch *research, bool considerDebugMode = true) const;
	/// Gets if a certain list of research topics has been completed.
	bool isResearched(const std::vector<std::string> &research, bool considerDebugMode = true) const;
	/// Gets if a certain list of research topics has been completed.
	bool isResearched(const std::vector<const RuleResearch *> &research, bool considerDebugMode = true, bool skipDisabled = false) const;
	/// Gets if a certain item has been obtained.
	bool isItemObtained(const std::string &itemType, const Mod* mod) const;
	/// Gets if a certain facility has been built.
	bool isFacilityBuilt(const std::string &facilityType) const;
	/// Gets if a certain soldier type has been hired.
	bool isSoldierTypeHired(const std::string& soldierType) const;
	/// Gets the soldier matching this ID.
	Soldier *getSoldier(int id) const;
	/// Handles the higher promotions.
	bool handlePromotions(std::vector<Soldier*> &participants, const Mod *mod);
	/// Checks how many soldiers of a rank exist and which one has the highest score.
	Soldier *inspectSoldiers(std::vector<Soldier*> &soldiers, std::vector<Soldier*> &participants, int rank);
	/// Gets the (approximate) number of idle days since the soldier's last mission.
	int getSoldierIdleDays(const Soldier *soldier);
	///  Returns the list of alien bases.
	std::vector<AlienBase*> *getAlienBases();
	/// Sets debug mode.
	void setDebugMode();
	/// Gets debug mode.
	bool getDebugMode() const;
	/// return a list of maintenance costs
	std::vector<int64_t> &getMaintenances();
	/// sets the research score for the month
	void addResearchScore(int score);
	/// gets the list of research scores
	std::vector<int> &getResearchScores();
	/// gets the list of incomes.
	std::vector<int64_t> &getIncomes();
	/// gets the list of expenditures.
	std::vector<int64_t> &getExpenditures();
	/// gets whether or not the player has been warned
	bool getWarned() const;
	/// sets whether or not the player has been warned
	void setWarned(bool warned);

	/// gets personal light toggle
	bool getTogglePersonalLight() const { return _togglePersonalLight; }
	/// sets personal light toggle
	void setTogglePersonalLight(bool togglePersonalLight) { _togglePersonalLight = togglePersonalLight; }
	/// gets night vision toggle
	bool getToggleNightVision() const { return _toggleNightVision; }
	/// sets night vision toggle
	void setToggleNightVision(bool toggleNightVision) { _toggleNightVision = toggleNightVision; }
	/// gets brightness toggle
	int getToggleBrightness() const { return _toggleBrightness; }
	/// sets brightness toggle
	void setToggleBrightness(int toggleBrightness) { _toggleBrightness = toggleBrightness; }

	/// Full access to the alien strategy data.
	AlienStrategy &getAlienStrategy() { return *_alienStrategy; }
	/// Read-only access to the alien strategy data.
	const AlienStrategy &getAlienStrategy() const { return *_alienStrategy; }
	/// Full access to the current alien missions.
	std::vector<AlienMission*> &getAlienMissions() { return _activeMissions; }
	/// Read-only access to the current alien missions.
	const std::vector<AlienMission*> &getAlienMissions() const { return _activeMissions; }
	/// Finds a mission by region and objective.
	AlienMission *findAlienMission(const std::string &region, MissionObjective objective, AlienRace* race = nullptr) const;
	/// Full access to the current geoscape events.
	std::vector<GeoscapeEvent*> &getGeoscapeEvents() { return _geoscapeEvents; }
	/// Read-only access to the current geoscape events.
	const std::vector<GeoscapeEvent*> &getGeoscapeEvents() const { return _geoscapeEvents; }
	/// Locate a region containing a position.
	Region *locateRegion(double lon, double lat) const;
	/// Locate a region containing a Target.
	Region *locateRegion(const Target &target) const;
	/// Locate a country containing a position.
	Country* locateCountry(double lon, double lat) const;
	/// Locate a country containing a Target.
	Country* locateCountry(const Target& target) const;
	/// Select a soldier nationality based on mod rules and location on the globe.
	int selectSoldierNationalityByLocation(const Mod* mod, const RuleSoldier* rule, const Target* target) const;
	/// Return the month counter.
	int getMonthsPassed() const;
	/// Return the day counter.
	int getDaysPassed() const { return _daysPassed; }
	void increaseDaysPassed() { _daysPassed++; }
	/// Return the vehicles lost counter.
	int getVehiclesLost() const { return _vehiclesLost; }
	void increaseVehiclesLost() { _vehiclesLost++; }
	/// Return the GraphRegionToggles.
	const std::string &getGraphRegionToggles() const;
	/// Return the GraphCountryToggles.
	const std::string &getGraphCountryToggles() const;
	/// Return the GraphFinanceToggles.
	const std::string &getGraphFinanceToggles() const;
	/// Sets the GraphRegionToggles.
	void setGraphRegionToggles(const std::string &value);
	/// Sets the GraphCountryToggles.
	void setGraphCountryToggles(const std::string &value);
	/// Sets the GraphFinanceToggles.
	void setGraphFinanceToggles(const std::string &value);
	/// Increment the month counter.
	void addMonth();
	/// add a research to the "popped up" array
	void addPoppedResearch(const RuleResearch* research);
	/// check if a research is on the "popped up" array
	bool wasResearchPopped(const RuleResearch* research);
	/// remove a research from the "popped up" array
	void removePoppedResearch(const RuleResearch* research);
	/// remembers that this event has been generated
	void addGeneratedEvent(const RuleEvent* event);
	/// checks if an event has been generated previously
	bool wasEventGenerated(const std::string& eventName);
	/// Gets the list of dead soldiers.
	std::vector<Soldier*> *getDeadSoldiers();
	/// Gets a list of all active soldiers.
	std::vector<Soldier*> getAllActiveSoldiers() const;
	/// Gets the last selected player base.
	Base *getSelectedBase();
	/// Set the last selected player base.
	void setSelectedBase(size_t base);
	/// Evaluate the score of a soldier based on all of his stats, missions and kills.
	int getSoldierScore(Soldier *soldier);
	/// Sets the last selected armor
	void setLastSelectedArmor(const std::string &value);
	/// Gets the last selected armor
	std::string getLastSelectedArmor() const;
	/// Gets the name of a global equipment layout at specified index.
	const std::string &getGlobalEquipmentLayoutName(int index) const;
	/// Sets the name of a global equipment layout at specified index.
	void setGlobalEquipmentLayoutName(int index, const std::string &name);
	/// Gets the armor type of a global equipment layout at specified index.
	const std::string& getGlobalEquipmentLayoutArmor(int index) const;
	/// Sets the armor type of a global equipment layout at specified index.
	void setGlobalEquipmentLayoutArmor(int index, const std::string& armorType);
	/// Gets the global equipment layout at specified index.
	std::vector<EquipmentLayoutItem*> *getGlobalEquipmentLayout(int index);
	/// Gets the name of a global craft loadout at specified index.
	const std::string &getGlobalCraftLoadoutName(int index) const;
	/// Sets the name of a global craft loadout at specified index.
	void setGlobalCraftLoadoutName(int index, const std::string &name);
	/// Gets the global craft loadout at specified index.
	ItemContainer *getGlobalCraftLoadout(int index);
	/// Gets the list of missions statistics
	std::vector<MissionStatistics*> *getMissionStatistics();
	/// Adds a UFO to the ignore list.
	void addUfoToIgnoreList(int ufoId);
	/// Checks if a UFO is on the ignore list.
	bool isUfoOnIgnoreList(int ufoId);
	/// Handles a soldier's death.
	std::vector<Soldier*>::iterator killSoldier(bool resetArmor, Soldier *soldier, BattleUnitKills *cause = 0);
	/// enables/disables autosell for an item type
	void setAutosell(const RuleItem *itype, const bool enabled);
	/// get autosell state for an item type
	bool getAutosell(const RuleItem *) const;
	/// Removes all soldiers from a given craft.
	void removeAllSoldiersFromXcomCraft(Craft *craft);
	/// Stop hunting the given xcom craft.
	void stopHuntingXcomCraft(Craft *target);
	/// Stop hunting all xcom craft from a given xcom base.
	void stopHuntingXcomCrafts(Base *base);
	/// Should all xcom soldiers have completely empty starting inventory when doing base equipment?
	bool getDisableSoldierEquipment() const;
	/// Sets the corresponding flag.
	void setDisableSoldierEquipment(bool disableSoldierEquipment);
	/// Is alien containment check finished?
	bool getAlienContainmentChecked() const { return _alienContainmentChecked; }
	/// Sets the corresponding flag.
	void setAlienContainmentChecked(bool alienContainmentChecked) { _alienContainmentChecked = alienContainmentChecked; }
	/// Is the mana feature already unlocked?
	bool isManaUnlocked(Mod *mod) const;
	/// Gets the current score based on research score and xcom/alien activity in regions.
	int getCurrentScore(int monthsPassed) const;
	/// Clear links for the given alien base. Use this before deleting the alien base.
	void clearLinksForAlienBase(AlienBase* alienBase, const Mod* mod);
	/// Delete the given retaliation mission.
	void deleteRetaliationMission(AlienMission* am, Base* base);
	/// Spawn a Geoscape event from the event rules.
	bool spawnEvent(const RuleEvent* eventRules);
	/// Checks if an instant Geoscape event can be spawned.
	bool canSpawnInstantEvent(const RuleEvent* eventRules);
	/// Handles research unlocked by successful/failed missions and despawned mission sites.
	bool handleResearchUnlockedByMissions(const RuleResearch* research, const Mod* mod);
	/// Handles research side effects for primary research sources.
	void handlePrimaryResearchSideEffects(const std::vector<const RuleResearch*> &topicsToCheck, const Mod* mod, Base* base);
	/// Gets the list of user notes.
	std::vector<std::string>& getUserNotes() { return _userNotes; }
	/// Gets the list of geoscape debug log entries.
	std::vector<std::string>& getGeoscapeDebugLog() { return _geoscapeDebugLog; }
};

}
