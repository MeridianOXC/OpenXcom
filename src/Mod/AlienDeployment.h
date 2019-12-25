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
#include "Mod.h"
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
	int lowQty, highQty, dQty, extraQty;
	int percentageOutsideUfo;
	std::vector<ItemSet> itemSets, extraRandomItems;
	DeploymentData() : alienRank(0), lowQty(0), highQty(0), dQty(0), extraQty(0), percentageOutsideUfo(0) { }
};
struct BriefingData
{
	int palette, textOffset;
	std::string title, desc, music, background, cutscene;
	bool showCraft, showTarget;
	BriefingData() : palette(0), textOffset(0), music("GMDEFEND"), background("BACK16.SCR"), showCraft(true), showTarget(true) { /*Empty by Design*/ };
};
enum ChronoTrigger { FORCE_LOSE, FORCE_ABORT, FORCE_WIN, FORCE_WIN_SURRENDER };
enum EscapeType { ESCAPE_NONE, ESCAPE_EXIT, ESCAPE_ENTRY, ESCAPE_EITHER };
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
	std::string _unlockedResearch, _missionBountyItem;
	int _bughuntMinTurn;
	std::vector<DeploymentData> _data;
	int _width, _length, _height, _civilians;
	int _civilianSpawnNodeRank;
	std::map<std::string, int> _civiliansByType;
	std::vector<std::string> _terrains, _music;
	int _shade, _minShade, _maxShade;
	std::string _nextStage, _race, _script;
	std::vector<std::string> _randomRaces;
	bool _finalDestination, _isAlienBase;
	std::string _winCutscene, _loseCutscene, _abortCutscene;
	std::string _alert, _alertBackground, _alertDescription;
	int _alertSound;
	BriefingData _briefingData;
	std::string _markerName, _objectivePopup, _objectiveCompleteText, _objectiveFailedText;
	WeightedOptions _genMission;
	int _markerIcon, _durationMin, _durationMax, _minDepth, _maxDepth, _genMissionFrequency, _genMissionLimit;
	int _objectiveType, _objectivesRequired, _objectiveCompleteScore, _objectiveFailedScore, _despawnPenalty, _abortPenalty, _points, _turnLimit, _cheatTurn;
	ChronoTrigger _chronoTrigger;
	bool _keepCraftAfterFailedMission;
	EscapeType _escapeType;
	std::string _baseSelfDestructCode;
	int _baseDetectionRange, _baseDetectionChance, _huntMissionMaxFrequency;
	std::vector<std::pair<size_t, WeightedOptions*> > _huntMissionDistribution;
	std::vector<std::pair<size_t, WeightedOptions*> > _alienBaseUpgrades;
public:
	/// Creates a blank Alien Deployment ruleset.
	AlienDeployment(const std::string &type);
	/// Cleans up the Alien Deployment ruleset.
	~AlienDeployment();
	/// Loads Alien Deployment data from YAML.
	void load(const YAML::Node& node, Mod *mod);
	/// Gets the Alien Deployment's type.
	std::string getType() const;
	/// Gets the custom UFO name to use for the dummy/blank 'addUFO' mapscript command.
	const std::string& getCustomUfoName() const { return _customUfo; }
	/// Gets the Alien Deployment's enviro effects.
	const std::string& getEnviroEffects() const;
	/// Gets the Alien Deployment's starting condition.
	const std::string& getStartingCondition() const;
	/// Gets the research topic to be unlocked after a successful mission.
	std::string getUnlockedResearch() const;
	/// Gets the item to be recovered/given after a successful mission.
	std::string getMissionBountyItem() const;
	/// Gets the bug hunt mode minimum turn requirement (default = 0 = not used).
	int getBughuntMinTurn() const;
	/// Gets a pointer to the data.
	const std::vector<DeploymentData>* getDeploymentData() const;
	/// Gets dimensions.
	void getDimensions(int *width, int *length, int *height) const;
	/// Gets civilians.
	int getCivilians() const;
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
	std::string getScript() const;
	/// Checks if this is the destination for the final mission (mars stage 1, t'leth stage 1).
	bool isFinalDestination() const;
	/// Gets the cutscene to play when this mission is won.
	std::string getWinCutscene() const;
	/// Gets the cutscene to play when this mission is lost.
	std::string getLoseCutscene() const;
	/// Gets the cutscene to play when this mission is aborted.
	std::string getAbortCutscene() const;
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
	bool getObjectiveCompleteInfo(std::string &text, int &score) const;
	/// Fills out the objective failed info.
	bool getObjectiveFailedInfo(std::string &text, int &score) const;
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

	std::string chooseGenMissionType() const;

	int getGenMissionFrequency() const;

	int getGenMissionLimit() const;

	bool keepCraftAfterFailedMission() const;
	EscapeType getEscapeType() const;

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

	/// Generates an alien base upgrade.
	std::string generateAlienBaseUpgrade(const size_t baseAgeInMonths) const;

};

}
