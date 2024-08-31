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
#include <yaml-cpp/yaml.h>
#include "../Savegame/WeightedOptions.h"

namespace OpenXcom
{

class RuleTerrain;
class Mod;

struct ItemSet
{
	std::vector<std::string> items;
};

struct DeploymentData
{
	int alienRank;
	std::string customUnitType;
	int lowQty, medQty, highQty, dQty, extraQty;
	int percentageOutsideUfo;
	std::vector<ItemSet> itemSets, extraRandomItems;
	DeploymentData() : alienRank(0), lowQty(0), medQty(0), highQty(0), dQty(0), extraQty(0), percentageOutsideUfo(0) { }
};
struct BriefingData
{
	int palette, textOffset;
	std::string title, desc, music, background, cutscene;
	bool showCraft, showTarget;
	BriefingData() : palette(0), textOffset(0), music("GMDEFEND"), background("BACK16.SCR"), showCraft(true), showTarget(true) { /*Empty by Design*/ };
};
enum MapBlockFilterType : int { MFT_NONE, MFT_BY_MAPSCRIPT, MFT_BY_REINFORCEMENTS, MFT_BY_BOTH_UNION, MFT_BY_BOTH_INTERSECTION };
struct ReinforcementsData
{
	std::string type;
	BriefingData briefing;
	int minDifficulty = 0;
	int maxDifficulty = 4;
	bool objectiveDestroyed = false;
	std::vector<int> turns;
	int minTurn = 0;
	int maxTurn = -1;
	int executionOdds = 100;
	int maxRuns = -1;
	bool useSpawnNodes = true;
	MapBlockFilterType mapBlockFilterType = MFT_BY_BOTH_UNION;
	std::vector<std::string> spawnBlocks;
	std::vector<int> spawnBlockGroups;
	std::vector<int> spawnNodeRanks;
	std::vector<int> spawnZLevels;
	bool randomizeZLevels = true;
	int minDistanceFromXcomUnits = 0;
	int maxDistanceFromBorders = 0;
	bool forceSpawnNearFriend = true;
	std::vector<DeploymentData> data;
};
enum ChronoTrigger { FORCE_LOSE, FORCE_ABORT, FORCE_WIN, FORCE_WIN_SURRENDER };
enum EscapeType : int { ESCAPE_NONE, ESCAPE_EXIT, ESCAPE_ENTRY, ESCAPE_EITHER };
/**
 * Represents a specific type of Alien Deployment.
 * Contains constant info about a Alien Deployment like
 * the number of aliens for each alien type and what items they carry
 * (itemset depends on alien technology advancement level 0, 1 or 2).
 * - deployment type can be a craft's name, but also alien base or cydonia.
 * - alienRank is used to check which nodeRanks can be used to deploy this unit
 *   + to match to a specific unit (=race/rank combination) that should be deployed.
 * @sa Node
 */
class AlienDeployment
{
private:
	std::string _type;
	std::string _customUfo;
	std::string _enviroEffects, _startingCondition;
	std::string _unlockedResearchOnSuccess, _unlockedResearchOnFailure, _unlockedResearchOnDespawn;
	std::string _counterSuccess, _counterFailure, _counterDespawn, _counterAll;
	std::string _decreaseCounterSuccess, _decreaseCounterFailure, _decreaseCounterDespawn, _decreaseCounterAll;
	std::string _missionBountyItem;
	int _missionBountyItemCount;
	int _bughuntMinTurn;
	bool _forcePercentageOutsideUfo;
	std::vector<DeploymentData> _data;
	std::vector<ReinforcementsData> _reinforcements;
	int _width, _length, _height, _civilians;
	bool _markCiviliansAsVIP;
	int _civilianSpawnNodeRank;
	std::map<std::string, int> _civiliansByType;
	std::vector<std::string> _terrains, _music;
	int _shade, _minShade, _maxShade;
	std::string _nextStage, _race, _mapScript;
	std::vector<std::string> _mapScripts;
	std::vector<std::string> _randomRaces;
	bool _finalDestination, _isAlienBase, _isHidden;
	int _fakeUnderwaterSpawnChance;
	std::string _winCutscene, _loseCutscene, _abortCutscene;
	std::string _alert, _alertBackground, _alertDescription;
	int _alertSound;
	BriefingData _briefingData;
	std::string _markerName, _objectivePopup, _objectiveCompleteText, _objectiveFailedText;
	std::string _missionCompleteText, _missionFailedText;
	WeightedOptions _genMission, _successEvents, _failureEvents, _despawnEvents;
	int _markerIcon, _durationMin, _durationMax, _minDepth, _maxDepth, _genMissionFrequency, _genMissionLimit;
	bool _genMissionRaceFromAlienBase;
	int _objectiveType, _objectivesRequired, _objectiveCompleteScore, _objectiveFailedScore, _despawnPenalty, _abortPenalty, _points, _turnLimit, _cheatTurn;
	ChronoTrigger _chronoTrigger;
	bool _keepCraftAfterFailedMission, _allowObjectiveRecovery;
	EscapeType _escapeType;
	int _vipSurvivalPercentage;
	std::string _baseSelfDestructCode;
	int _baseDetectionRange, _baseDetectionChance, _huntMissionMaxFrequency;
	bool _huntMissionRaceFromAlienBase;
	std::vector<std::pair<size_t, WeightedOptions*> > _huntMissionDistribution;
	std::vector<std::pair<size_t, WeightedOptions*> > _alienBaseUpgrades;
	bool _resetAlienBaseAgeAfterUpgrade, _resetAlienBaseAge;
	std::string _upgradeRace;
	bool _noWeaponPile;
public:
	/// Creates a blank Alien Deployment ruleset.
	AlienDeployment(const std::string &type);
	/// Cleans up the Alien Deployment ruleset.
	~AlienDeployment();
	/// Loads Alien Deployment data from YAML.
	void load(const YAML::Node& node, Mod *mod);
	/// Gets the Alien Deployment's type.
	const std::string& getType() const;
	/// Gets the custom UFO name to use for the dummy/blank 'addUFO' mapscript command.
	const std::string& getCustomUfoName() const { return _customUfo; }
	/// Gets the Alien Deployment's enviro effects.
	const std::string& getEnviroEffects() const;
	/// Gets the Alien Deployment's starting condition.
	const std::string& getStartingCondition() const;
	/// Gets the research topic to be unlocked after a successful mission.
	const std::string& getUnlockedResearchOnSuccess() const { return _unlockedResearchOnSuccess; }
	/// Gets the research topic to be unlocked after a failed mission.
	const std::string& getUnlockedResearchOnFailure() const { return _unlockedResearchOnFailure; }
	/// Gets the research topic to be unlocked after a despawned mission site.
	const std::string& getUnlockedResearchOnDespawn() const { return _unlockedResearchOnDespawn; }
	/// Gets the name of a custom counter variable to increase on mission success.
	const std::string& getCounterSuccess() const { return _counterSuccess; }
	/// Gets the name of a custom counter variable to increase on mission failure (incl. mission despawn).
	const std::string& getCounterFailure() const { return _counterFailure; }
	/// Gets the name of a custom counter variable to increase on mission despawn.
	const std::string& getCounterDespawn() const { return _counterDespawn; }
	/// Gets the name of a custom counter variable to increase on any mission result (success, failure and despawn).
	const std::string& getCounterAll() const { return _counterAll; }
	/// Gets the name of a custom counter variable to decrease on mission success.
	const std::string& getDecreaseCounterSuccess() const { return _decreaseCounterSuccess; }
	/// Gets the name of a custom counter variable to decrease on mission failure (incl. mission despawn).
	const std::string& getDecreaseCounterFailure() const { return _decreaseCounterFailure; }
	/// Gets the name of a custom counter variable to decrease on mission despawn.
	const std::string& getDecreaseCounterDespawn() const { return _decreaseCounterDespawn; }
	/// Gets the name of a custom counter variable to decrease on any mission result (success, failure and despawn).
	const std::string& getDecreaseCounterAll() const { return _decreaseCounterAll; }
	/// Gets the item to be recovered/given after a successful mission.
	std::string getMissionBountyItem() const;
	/// Gets the number of items to be recovered/given after a successful mission.
	int getMissionBountyItemCount() const { return _missionBountyItemCount; }
	/// Gets the bug hunt mode minimum turn requirement (default = 0 = not used).
	int getBughuntMinTurn() const;
	/// Should `percentageOutsideUfo` be forced for all kinds of missions? (instead of just UFO crash/landing sites)
	bool getForcePercentageOutsideUfo() const { return _forcePercentageOutsideUfo; }
	/// Gets a pointer to the data.
	const std::vector<DeploymentData>* getDeploymentData() const;
	/// Gets the highest used alien rank.
	int getMaxAlienRank() const;
	/// Gets a pointer to the reinforcements data.
	const std::vector<ReinforcementsData>* getReinforcementsData() const;
	/// Gets dimensions.
	void getDimensions(int *width, int *length, int *height) const;
	/// Gets civilians.
	int getCivilians() const;
	/// Gets the civilian spawn node rank.
	bool getMarkCiviliansAsVIP() const { return _markCiviliansAsVIP; }
	/// Gets the civilian spawn node rank.
	int getCivilianSpawnNodeRank() const { return _civilianSpawnNodeRank; }
	/// Gets civilians by type.
	const std::map<std::string, int> &getCiviliansByType() const;
	/// Gets the terrain for battlescape generation.
	std::vector<std::string> getTerrains() const;
	/// Gets the shade level for battlescape generation.
	int getShade() const;
	/// Gets the min shade level for battlescape generation.
	int getMinShade() const;
	/// Gets the max shade level for battlescape generation.
	int getMaxShade() const;
	/// Gets the next stage of the mission.
	std::string getNextStage() const;
	/// Gets the race to use in the next stage.
	std::string getRace() const;
	/// Gets the script to use for this deployment.
	const std::string& getRandomMapScript() const;
	int hasTextureBasedScript(const Mod* mod) const;
	/// Checks if this is the destination for the final mission (mars stage 1, t'leth stage 1).
	bool isFinalDestination() const;
	/// Gets the cutscene to play when this mission is won.
	std::string getWinCutscene() const;
	/// Gets the cutscene to play when this mission is lost.
	std::string getLoseCutscene() const;
	/// Gets the cutscene to play when this mission is aborted.
	std::string getAbortCutscene() const;
	/// Gets geoscape event rule name to spawn after success mission.
	std::string chooseSuccessEvent() const { return _successEvents.choose(); };
	/// Gets geoscape event rule name to despawn after success mission.
	std::string chooseDespawnEvent() const { return _despawnEvents.choose(); };
	/// Gets geoscape event rule name to spawn after failure mission.
	std::string chooseFailureEvent() const { return _failureEvents.choose(); };
	/// Gets the alert message for this mission type.
	std::string getAlertMessage() const;
	/// Gets the alert background for this mission type.
	std::string getAlertBackground() const;
	/// Gets the alert description for this mission type.
	std::string getAlertDescription() const;
	/// Gets the alert sound for this mission type.
	int getAlertSound() const;
	/// Gets the briefing data for this mission type.
	BriefingData getBriefingData() const;
	/// Gets the marker name for this mission.
	std::string getMarkerName() const;
	/// Gets the marker icon for this mission.
	int getMarkerIcon() const;
	/// Gets the minimum duration for this mission.
	int getDurationMin() const;
	/// Gets the maximum duration for this mission.
	int getDurationMax() const;
	/// Gets the list of music to pick from.
	const std::vector<std::string> &getMusic() const;
	/// Gets the minimum depth.
	int getMinDepth() const;
	/// Gets the maximum depth.
	int getMaxDepth() const;
	/// Gets the target type for this mission.
	int getObjectiveType() const;
	/// Gets a fixed number of objectives requires (if any).
	int getObjectivesRequired() const;
	/// Gets the string to pop up when the mission objectives are complete.
	const std::string &getObjectivePopup() const;
	/// Fills out the objective complete info.
	bool getObjectiveCompleteInfo(std::string &text, int &score, std::string &missionText) const;
	/// Fills out the objective failed info.
	bool getObjectiveFailedInfo(std::string &text, int &score, std::string &missionText) const;
	/// Gets the score penalty XCom receives for ignoring this site.
	int getDespawnPenalty() const;
	/// Gets the score penalty XCom receives for aborting this mission.
	int getAbortPenalty() const { return _abortPenalty; }
	/// Gets the (half hourly) score penalty XCom receives for this site existing.
	int getPoints() const;
	/// Gets the turn limit for this deployment.
	int getTurnLimit() const;
	/// Gets the action that triggers when the timer runs out.
	ChronoTrigger getChronoTrigger() const;
	/// Gets which turn the aliens start cheating on.
	int getCheatTurn() const;
	/// Gets whether or not this is an alien base (purely for new battle mode)
	bool isAlienBase() const;
	/// Gets whether or not this mission should be hidden (purely for new battle mode)
	bool isHidden() const { return _isHidden; }
	/// Gets the chance for deciding to spawn an alien base on fakeUnderwater globe texture.
	int getFakeUnderwaterSpawnChance() const { return _fakeUnderwaterSpawnChance; }

	std::string chooseGenMissionType() const;
	int getGenMissionFrequency() const;
	int getGenMissionLimit() const;
	bool isGenMissionRaceFromAlienBase() const;

	bool keepCraftAfterFailedMission() const;
	bool allowObjectiveRecovery() const;
	EscapeType getEscapeType() const;
	/// Gets the percentage of VIP units that must survive in order to accomplish the mission.
	int getVIPSurvivalPercentage() const { return _vipSurvivalPercentage; }

	/// Generates a hunt mission based on the given month.
	std::string generateHuntMission(const size_t monthsPassed) const;
	/// Gets the Alien Base self destruct code.
	const std::string& getBaseSelfDestructCode() const;
	/// Gets the detection range of an alien base.
	double getBaseDetectionRange() const;
	/// Gets the chance of an alien base to detect a player's craft (once every 10 minutes).
	int getBaseDetectionChance() const;
	/// Gets the maximum frequency of hunt missions generated by an alien base.
	int getHuntMissionMaxFrequency() const;
	/// Should the hunt missions inherit the race from the alien base, or take it from their own 'raceWeights'?
	bool isHuntMissionRaceFromAlienBase() const;

	/// Generates an alien base upgrade.
	std::string generateAlienBaseUpgrade(const size_t baseAgeInMonths) const;
	/// Should the age of an alien base be reset after an upgrade (from this type)?
	bool resetAlienBaseAgeAfterUpgrade() const { return _resetAlienBaseAgeAfterUpgrade; }
	/// Should the age of an alien base be reset after an upgrade (into this type)?
	bool resetAlienBaseAge() const { return _resetAlienBaseAge; }
	/// Gets the new race for an alien base after an upgrade (into this type).
	const std::string& getUpgradeRace() const { return _upgradeRace; }

	/// Should items on the "weapon pile" be hidden from the player?
	bool getNoWeaponPile() const { return _noWeaponPile; }
};

}
