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
#include "AlienDeployment.h"
#include "MapScript.h"
#include "../Engine/RNG.h"
#include "../Mod/Mod.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain
 * type of deployment data.
 * @param type String defining the type.
 */
AlienDeployment::AlienDeployment(const std::string &type) :
	_type(type), _missionBountyItemCount(1), _bughuntMinTurn(0), _forcePercentageOutsideUfo(false),
	_width(0), _length(0), _height(0), _civilians(0), _markCiviliansAsVIP(false), _civilianSpawnNodeRank(0),
	_shade(-1), _minShade(-1), _maxShade(-1), _finalDestination(false), _isAlienBase(false), _isHidden(false), _fakeUnderwaterSpawnChance(0),
	_alert("STR_ALIENS_TERRORISE"), _alertBackground("BACK03.SCR"), _alertDescription(""), _alertSound(-1),
	_markerName("STR_TERROR_SITE"), _markerIcon(-1), _durationMin(0), _durationMax(0), _minDepth(0), _maxDepth(0),
	_genMissionFrequency(0), _genMissionLimit(1000), _genMissionRaceFromAlienBase(true),
	_objectiveType(-1), _objectivesRequired(0), _objectiveCompleteScore(0), _objectiveFailedScore(0), _despawnPenalty(0), _abortPenalty(0), _points(0),
	_turnLimit(0), _cheatTurn(20), _chronoTrigger(FORCE_LOSE), _keepCraftAfterFailedMission(false), _allowObjectiveRecovery(false), _escapeType(ESCAPE_NONE), _vipSurvivalPercentage(0),
	_baseDetectionRange(0), _baseDetectionChance(100), _huntMissionMaxFrequency(60), _huntMissionRaceFromAlienBase(true),
	_resetAlienBaseAgeAfterUpgrade(false), _resetAlienBaseAge(false), _noWeaponPile(false)
{
}

/**
 *
 */
AlienDeployment::~AlienDeployment()
{
	for (auto& pair : _huntMissionDistribution)
	{
		delete pair.second;
	}
	for (auto& pair : _alienBaseUpgrades)
	{
		delete pair.second;
	}
}

/**
 * Loads the Deployment from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the deployment.
 */
void AlienDeployment::load(const YAML::YamlNodeReader& node, Mod *mod)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	reader.tryRead("customUfo", _customUfo);
	reader.tryRead("enviroEffects", _enviroEffects);
	reader.tryRead("startingCondition", _startingCondition);
	reader.tryRead("unlockedResearch", _unlockedResearchOnSuccess);
	reader.tryRead("unlockedResearchOnFailure", _unlockedResearchOnFailure);
	reader.tryRead("unlockedResearchOnDespawn", _unlockedResearchOnDespawn);
	reader.tryRead("counterSuccess", _counterSuccess);
	reader.tryRead("counterFailure", _counterFailure);
	reader.tryRead("counterDespawn", _counterDespawn);
	reader.tryRead("counterAll", _counterAll);
	reader.tryRead("decreaseCounterSuccess", _decreaseCounterSuccess);
	reader.tryRead("decreaseCounterFailure", _decreaseCounterFailure);
	reader.tryRead("decreaseCounterDespawn", _decreaseCounterDespawn);
	reader.tryRead("decreaseCounterAll", _decreaseCounterAll);
	reader.tryRead("missionBountyItem", _missionBountyItem);
	reader.tryRead("missionBountyItemCount", _missionBountyItemCount);
	reader.tryRead("bughuntMinTurn", _bughuntMinTurn);
	reader.tryRead("forcePercentageOutsideUfo", _forcePercentageOutsideUfo);
	reader.tryRead("data", _data);
	reader.tryRead("reinforcements", _reinforcements);
	reader.tryRead("width", _width);
	reader.tryRead("length", _length);
	reader.tryRead("height", _height);
	reader.tryRead("civilians", _civilians);
	reader.tryRead("markCiviliansAsVIP", _markCiviliansAsVIP);
	reader.tryRead("civilianSpawnNodeRank", _civilianSpawnNodeRank);
	mod->loadUnorderedNamesToInt(_type, _civiliansByType, reader["civiliansByType"]);
	reader.tryRead("terrains", _terrains);
	reader.tryRead("shade", _shade);
	reader.tryRead("minShade", _minShade);
	reader.tryRead("maxShade", _maxShade);
	reader.tryRead("nextStage", _nextStage);
	reader.tryRead("race", _race);
	reader.tryRead("randomRace", _randomRaces);
	reader.tryRead("finalDestination", _finalDestination);
	reader.tryRead("winCutscene", _winCutscene);
	reader.tryRead("loseCutscene", _loseCutscene);
	reader.tryRead("abortCutscene", _abortCutscene);
	reader.tryRead("script", _mapScript);
	reader.tryRead("mapScripts", _mapScripts);
	reader.tryRead("alert", _alert);
	reader.tryRead("alertBackground", _alertBackground);
	reader.tryRead("alertDescription", _alertDescription);
	mod->loadSoundOffset(_type, _alertSound, reader["alertSound"], "GEO.CAT");
	reader.tryRead("briefing", _briefingData);
	reader.tryRead("markerName", _markerName);
	if (reader["markerIcon"])
	{
		_markerIcon = mod->getOffset(reader["markerIcon"].readVal<int>(), 8);
	}
	if (reader["depth"])
	{
		_minDepth = reader["depth"][0].readVal(_minDepth);
		_maxDepth = reader["depth"][1].readVal(_maxDepth);
	}
	if (reader["duration"])
	{
		_durationMin = reader["duration"][0].readVal(_durationMin);
		_durationMax = reader["duration"][1].readVal(_durationMax);
	}
	reader.tryRead("music", _music);
	reader.tryRead("objectiveType", _objectiveType);
	reader.tryRead("objectivesRequired", _objectivesRequired);
	reader.tryRead("objectivePopup", _objectivePopup);

	if (reader["objectiveComplete"])
	{
		_objectiveCompleteText = reader["objectiveComplete"][0].readVal(_objectiveCompleteText);
		_objectiveCompleteScore = reader["objectiveComplete"][1].readVal(_objectiveCompleteScore);
	}
	if (reader["objectiveFailed"])
	{
		_objectiveFailedText = reader["objectiveFailed"][0].readVal(_objectiveFailedText);
		_objectiveFailedScore = reader["objectiveFailed"][1].readVal(_objectiveFailedScore);
	}
	reader.tryRead("missionCompleteText", _missionCompleteText);
	reader.tryRead("missionFailedText", _missionFailedText);
	if (reader["successEvents"])
	{
		_successEvents.load(reader["successEvents"]);
	}
	if (reader["despawnEvents"])
	{
		_despawnEvents.load(reader["despawnEvents"]);
	}
	if (reader["failureEvents"])
	{
		_failureEvents.load(reader["failureEvents"]);
	}
	reader.tryRead("despawnPenalty", _despawnPenalty);
	reader.tryRead("abortPenalty", _abortPenalty);
	reader.tryRead("points", _points);
	reader.tryRead("cheatTurn", _cheatTurn);
	reader.tryRead("turnLimit", _turnLimit);
	reader.tryRead("chronoTrigger", _chronoTrigger);
	reader.tryRead("alienBase", _isAlienBase);
	reader.tryRead("isHidden", _isHidden);
	reader.tryRead("fakeUnderwaterSpawnChance", _fakeUnderwaterSpawnChance);
	reader.tryRead("keepCraftAfterFailedMission", _keepCraftAfterFailedMission);
	reader.tryRead("allowObjectiveRecovery", _allowObjectiveRecovery);
	reader.tryRead("escapeType", _escapeType);
	reader.tryRead("vipSurvivalPercentage", _vipSurvivalPercentage);
	if (reader["genMission"])
	{
		_genMission.load(reader["genMission"]);
	}
	reader.tryRead("genMissionFreq", _genMissionFrequency);
	reader.tryRead("genMissionLimit", _genMissionLimit);
	reader.tryRead("genMissionRaceFromAlienBase", _genMissionRaceFromAlienBase);

	reader.tryRead("baseSelfDestructCode", _baseSelfDestructCode);
	reader.tryRead("baseDetectionRange", _baseDetectionRange);
	reader.tryRead("baseDetectionChance", _baseDetectionChance);
	reader.tryRead("huntMissionMaxFrequency", _huntMissionMaxFrequency);
	reader.tryRead("huntMissionRaceFromAlienBase", _huntMissionRaceFromAlienBase);
	for (const auto& weights : reader["huntMissionWeights"].children())
	{
		WeightedOptions *nw = new WeightedOptions();
		nw->load(weights);
		_huntMissionDistribution.push_back(std::make_pair(weights.readKey<size_t>(0), nw));
	}
	for (const auto& weights : reader["alienBaseUpgrades"].children())
	{
		WeightedOptions *nw = new WeightedOptions();
		nw->load(weights);
		_alienBaseUpgrades.push_back(std::make_pair(weights.readKey<size_t>(0), nw));
	}
	reader.tryRead("resetAlienBaseAgeAfterUpgrade", _resetAlienBaseAgeAfterUpgrade);
	reader.tryRead("resetAlienBaseAge", _resetAlienBaseAge);
	reader.tryRead("upgradeRace", _upgradeRace);
	reader.tryRead("noWeaponPile", _noWeaponPile);
}

/**
 * Returns the language string that names
 * this deployment. Each deployment type has a unique name.
 * @return Deployment name.
 */
const std::string& AlienDeployment::getType() const
{
	return _type;
}

/**
 * Returns the enviro effects name for this mission.
 * @return String ID for the enviro effects.
 */
const std::string& AlienDeployment::getEnviroEffects() const
{
	return _enviroEffects;
}

/**
 * Returns the starting condition name for this mission.
 * @return String ID for starting condition.
 */
const std::string& AlienDeployment::getStartingCondition() const
{
	return _startingCondition;
}

/**
* Returns the item to be recovered/given after a successful mission.
* @return String ID for the item.
*/
std::string AlienDeployment::getMissionBountyItem() const
{
	return _missionBountyItem;
}

/**
* Gets the bug hunt mode minimum turn requirement (default = 0 = not used).
* @return Bug hunt min turn number.
*/
int AlienDeployment::getBughuntMinTurn() const
{
	return _bughuntMinTurn;
}

/**
 * Gets a pointer to the data.
 * @return Pointer to the data.
 */
const std::vector<DeploymentData>* AlienDeployment::getDeploymentData() const
{
	return &_data;
}

/**
 * Gets the highest used alien rank.
 * @return Highest used alien rank.
 */
int AlienDeployment::getMaxAlienRank() const
{
	int max = 0;
	for (auto& dd : _data)
	{
		if (dd.alienRank > max)
			max = dd.alienRank;
	}
	return max;
}

/**
 * Gets a pointer to the reinforcements data.
 * @return Pointer to the reinforcements data.
 */
const std::vector<ReinforcementsData>* AlienDeployment::getReinforcementsData() const
{
	return &_reinforcements;
}

/**
 * Gets dimensions.
 * @param width Width.
 * @param length Length.
 * @param height Height.
 */
void AlienDeployment::getDimensions(int *width, int *length, int *height) const
{
	*width = _width;
	*length = _length;
	*height = _height;
}

/**
 * Gets the number of civilians.
 * @return The number of civilians.
 */
int AlienDeployment::getCivilians() const
{
	return _civilians;
}

/**
 * Gets the number of civilians per type.
 * @return The number of civilians per type.
 */
const std::map<std::string, int> &AlienDeployment::getCiviliansByType() const
{
	return _civiliansByType;
}

/**
 * Gets the terrain for battlescape generation.
 * @return The terrain.
 */
std::vector<std::string> AlienDeployment::getTerrains() const
{
	return _terrains;
}

/**
 * Gets the shade level for battlescape generation.
 * @return The shade level.
 */
int AlienDeployment::getShade() const
{
	return _shade;
}

/**
* Gets the min shade level for battlescape generation.
* @return The min shade level.
*/
int AlienDeployment::getMinShade() const
{
	return _minShade;
}

/**
* Gets the max shade level for battlescape generation.
* @return The max shade level.
*/
int AlienDeployment::getMaxShade() const
{
	return _maxShade;
}

/**
 * Gets the next stage of the mission.
 * @return The next stage of the mission.
 */
std::string AlienDeployment::getNextStage() const
{
	return _nextStage;
}

/**
 * Gets the race to use on the next stage of the mission.
 * @return The race for the next stage of the mission.
 */
std::string AlienDeployment::getRace() const
{
	if (!_randomRaces.empty())
	{
		return _randomRaces[RNG::generate(0, _randomRaces.size() - 1)];
	}
	return _race;
}

/**
 * Gets the script to use to generate a mission of this type.
 * @return The script to use to generate a mission of this type.
 */
const std::string& AlienDeployment::getRandomMapScript() const
{
	if (!_mapScripts.empty())
	{
		size_t pick = RNG::generate(0, _mapScripts.size() - 1);
		return _mapScripts[pick];
	}
	return _mapScript;
}

/**
 * Does any map script use globe terrain?
 * @return 1 = yes, 0 = no, -1 = no map script found.
 */
int AlienDeployment::hasTextureBasedScript(const Mod* mod) const
{
	int ret = -1;
	// iterate _mapScripts
	for (const std::string& script : _mapScripts)
	{
		auto* vec = mod->getMapScript(script);
		if (vec)
		{
			ret = 0;
			// iterate map script commands
			for (auto* ms : *vec)
			{
				// iterate terrains
				for (const std::string& terrain : ms->getRandomAlternateTerrain())
				{
					if (terrain == "globeTerrain" || terrain == "baseTerrain")
					{
						return 1;
					}
				}
				// iterate vertical levels
				for (auto& vlevel : ms->getVerticalLevels())
				{
					if (vlevel.levelTerrain == "globeTerrain" || vlevel.levelTerrain == "baseTerrain")
					{
						return 1;
					}
				}
			}
		}
	}
	// check also _mapScript
	{
		auto* vec = mod->getMapScript(_mapScript);
		if (vec)
		{
			ret = 0;
			// iterate map script commands
			for (auto* ms : *vec)
			{
				// iterate terrains
				for (const std::string& terrain : ms->getRandomAlternateTerrain())
				{
					if (terrain == "globeTerrain" || terrain == "baseTerrain")
					{
						return 1;
					}
				}
				// iterate vertical levels
				for (auto& vlevel : ms->getVerticalLevels())
				{
					if (vlevel.levelTerrain == "globeTerrain" || vlevel.levelTerrain == "baseTerrain")
					{
						return 1;
					}
				}
			}
		}

	}
	return ret;
}

/**
 * Gets if winning this mission completes the game.
 * @return if winning this mission completes the game.
 */
bool AlienDeployment::isFinalDestination() const
{
	return _finalDestination;
}

/**
 * Gets the cutscene to play when the mission is won.
 * @return the cutscene to play when the mission is won.
 */
std::string AlienDeployment::getWinCutscene() const
{
	return _winCutscene;
}

/**
 * Gets the cutscene to play when the mission is lost.
 * @return the cutscene to play when the mission is lost.
 */
std::string AlienDeployment::getLoseCutscene() const
{
	return _loseCutscene;
}

/**
* Gets the cutscene to play when the mission is aborted.
* @return the cutscene to play when the mission is aborted.
*/
std::string AlienDeployment::getAbortCutscene() const
{
	return _abortCutscene;
}

/**
 * Gets the alert message displayed when this mission spawns.
 * @return String ID for the message.
 */
std::string AlienDeployment::getAlertMessage() const
{
	return _alert;
}

/**
* Gets the alert background displayed when this mission spawns.
* @return Sprite ID for the background.
*/
std::string AlienDeployment::getAlertBackground() const
{
	return _alertBackground;
}

/**
* Gets the alert description (displayed when clicking on [Info] button in TargetInfo).
* @return String ID for the description.
*/
std::string AlienDeployment::getAlertDescription() const
{
	return _alertDescription;
}

/**
* Gets the alert sound (played when mission detected screen pops up).
* @return ID for the sound.
*/
int AlienDeployment::getAlertSound() const
{
	return _alertSound;
}

/**
 * Gets the briefing data for this mission type.
 * @return data for the briefing window to use.
 */
BriefingData AlienDeployment::getBriefingData() const
{
	return _briefingData;
}

/**
 * Returns the globe marker name for this mission.
 * @return String ID for marker name.
 */
std::string AlienDeployment::getMarkerName() const
{
	return _markerName;
}

/**
 * Returns the globe marker icon for this mission.
 * @return Marker sprite, -1 if none.
 */
int AlienDeployment::getMarkerIcon() const
{
	return _markerIcon;
}

/**
 * Returns the minimum duration for this mission type.
 * @return Duration in hours.
 */
int AlienDeployment::getDurationMin() const
{
	return _durationMin;
}

/**
 * Returns the maximum duration for this mission type.
 * @return Duration in hours.
 */
int AlienDeployment::getDurationMax() const
{
	return _durationMax;
}

/**
 * Gets The list of musics this deployment has to choose from.
 * @return The list of track names.
 */
const std::vector<std::string> &AlienDeployment::getMusic() const
{
	return _music;
}

/**
 * Gets The minimum depth for this deployment.
 * @return The minimum depth.
 */
int AlienDeployment::getMinDepth() const
{
	return _minDepth;
}

/**
 * Gets The maximum depth for this deployment.
 * @return The maximum depth.
 */
int AlienDeployment::getMaxDepth() const
{
	return _maxDepth;
}

/**
 * Gets the target type for this mission (ie: alien control consoles and synomium devices).
 * @return the target type for this mission.
 */
int AlienDeployment::getObjectiveType() const
{
	return _objectiveType;
}

/**
 * Gets the number of objectives required by this mission.
 * @return the number of objectives required.
 */
int AlienDeployment::getObjectivesRequired() const
{
	return _objectivesRequired;
}

/**
 * Gets the string name for the popup to splash when the objective conditions are met.
 * @return the string to pop up.
 */
const std::string &AlienDeployment::getObjectivePopup() const
{
	return _objectivePopup;
}

/**
 * Fills out the variables associated with mission success, and returns if those variables actually contain anything.
 * @param &text a reference to the text we wish to alter.
 * @param &score a reference to the score we wish to alter.
 * @param &missionText a reference to the custom mission text we wish to alter.
 * @return if there is anything worthwhile processing.
 */
bool AlienDeployment::getObjectiveCompleteInfo(std::string &text, int &score, std::string &missionText) const
{
	text = _objectiveCompleteText;
	score = _objectiveCompleteScore;
	missionText = _missionCompleteText;
	return !text.empty();
}

/**
 * Fills out the variables associated with mission failure, and returns if those variables actually contain anything.
 * @param &text a reference to the text we wish to alter.
 * @param &score a reference to the score we wish to alter.
 * @param &missionText a reference to the custom mission text we wish to alter.
 * @return if there is anything worthwhile processing.
 */
bool AlienDeployment::getObjectiveFailedInfo(std::string &text, int &score, std::string &missionText) const
{
	text = _objectiveFailedText;
	score = _objectiveFailedScore;
	missionText = _missionFailedText;
	return !text.empty();
}

/**
 * Gets the score penalty XCom receives for letting this mission despawn.
 * @return the score for letting this site despawn.
 */
int AlienDeployment::getDespawnPenalty() const
{
	return _despawnPenalty;
}

/**
 * Gets the score penalty against XCom for this site existing.
 * This penalty is applied half-hourly for sites and daily for bases.
 * @return the number of points the aliens get per half hour.
 */
int AlienDeployment::getPoints() const
{
	return _points;
}

/**
 * Gets the maximum number of turns we have before this mission ends.
 * @return the turn limit.
 */
int AlienDeployment::getTurnLimit() const
{
	return _turnLimit;
}

/**
 * Gets the action type to perform when the timer expires.
 * @return the action type to perform.
 */
ChronoTrigger AlienDeployment::getChronoTrigger() const
{
	return _chronoTrigger;
}

/**
 * Gets the turn at which the players become exposed to the AI.
 * @return the turn to start cheating.
 */
int AlienDeployment::getCheatTurn() const
{
	return _cheatTurn;
}

bool AlienDeployment::isAlienBase() const
{
	return _isAlienBase;
}

std::string AlienDeployment::chooseGenMissionType() const
{
	return _genMission.choose();
}

int AlienDeployment::getGenMissionFrequency() const
{
	return _genMissionFrequency;
}

int AlienDeployment::getGenMissionLimit() const
{
	return _genMissionLimit;
}

bool AlienDeployment::isGenMissionRaceFromAlienBase() const
{
	return _genMissionRaceFromAlienBase;
}

bool AlienDeployment::keepCraftAfterFailedMission() const
{
	return _keepCraftAfterFailedMission;
}

bool AlienDeployment::allowObjectiveRecovery() const
{
	return _allowObjectiveRecovery;
}

EscapeType AlienDeployment::getEscapeType() const
{
	return _escapeType;
}

/**
 * Chooses one of the available missions.
 * @param monthsPassed The number of months that have passed in the game world.
 * @return The string id of the hunt mission.
 */
std::string AlienDeployment::generateHuntMission(const size_t monthsPassed) const
{
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rw;
	rw = _huntMissionDistribution.rbegin();
	while (monthsPassed < rw->first)
		++rw;
	return rw->second->choose();
}

/**
 * Returns the Alien Base self destruct code.
 * @return String ID of the corresponding research topic.
 */
const std::string& AlienDeployment::getBaseSelfDestructCode() const
{
	return _baseSelfDestructCode;
}

/**
 * Gets the detection range of an alien base.
 * @return Detection range.
 */
double AlienDeployment::getBaseDetectionRange() const
{
	return _baseDetectionRange;
}

/**
 * Gets the chance of an alien base to detect a player's craft (once every 10 minutes).
 * @return Chance in percent.
 */
int AlienDeployment::getBaseDetectionChance() const
{
	return _baseDetectionChance;
}

/**
 * Gets the maximum frequency of hunt missions generated by an alien base.
 * @return The frequency (in minutes).
 */
int AlienDeployment::getHuntMissionMaxFrequency() const
{
	return _huntMissionMaxFrequency;
}

/**
 * Should the hunt missions inherit the race from the alien base, or take it from their own 'raceWeights'?
 * @return True, if the race is taken from the alien base (vanilla behavior).
 */
bool AlienDeployment::isHuntMissionRaceFromAlienBase() const
{
	return _huntMissionRaceFromAlienBase;
}

/**
 * Chooses one of the available deployments.
 * @param baseAgeInMonths The number of months that have passed in the game world since the alien base spawned.
 * @return The string id of the deployment.
 */
std::string AlienDeployment::generateAlienBaseUpgrade(const size_t baseAgeInMonths) const
{
	if (_alienBaseUpgrades.empty())
		return "";

	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rw;
	rw = _alienBaseUpgrades.rbegin();
	while (baseAgeInMonths < rw->first)
		++rw;
	return rw->second->choose();
}

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, ItemSet* val)
{
	YAML::YamlNodeReader reader(nullptr, n);
	reader.tryReadVal(val->items);
	return true;
}

bool read(ryml::ConstNodeRef const& n, DeploymentData* val)
{
	YAML::YamlNodeReader reader(nullptr, n);
	reader.tryRead("alienRank", val->alienRank);
	reader.tryRead("customUnitType", val->customUnitType);
	reader.tryRead("lowQty", val->lowQty);
	reader.tryRead("medQty", val->medQty);
	reader.tryRead("highQty", val->highQty);
	reader.tryRead("dQty", val->dQty);
	reader.readNode("extraQty", val->extraQty, 0);
	reader.tryRead("percentageOutsideUfo", val->percentageOutsideUfo);
	reader.tryRead("itemSets", val->itemSets);
	reader.tryRead("extraRandomItems", val->extraRandomItems);
	return true;
}

bool read(ryml::ConstNodeRef const& n, BriefingData* val)
{
	YAML::YamlNodeReader reader(nullptr, n);
	reader.tryRead("palette", val->palette);
	reader.tryRead("textOffset", val->textOffset);
	reader.tryRead("title", val->title);
	reader.tryRead("desc", val->desc);
	reader.tryRead("music", val->music);
	reader.tryRead("cutscene", val->cutscene);
	reader.tryRead("background", val->background);
	reader.tryRead("showCraft", val->showCraft);
	reader.tryRead("showTarget", val->showTarget);
	return true;
}

bool read(ryml::ConstNodeRef const& n, ReinforcementsData* val)
{
	YAML::YamlNodeReader reader(nullptr, n);
	reader.tryRead("type", val->type);
	reader.tryRead("briefing", val->briefing);
	reader.tryRead("minDifficulty", val->minDifficulty);
	reader.tryRead("maxDifficulty", val->maxDifficulty);
	reader.tryRead("objectiveDestroyed", val->objectiveDestroyed);
	reader.tryRead("turns", val->turns);
	reader.tryRead("minTurn", val->minTurn);
	reader.tryRead("maxTurn", val->maxTurn);
	reader.tryRead("executionOdds", val->executionOdds);
	reader.tryRead("maxRuns", val->maxRuns);
	reader.tryRead("useSpawnNodes", val->useSpawnNodes);
	reader.tryRead("mapBlockFilterType", val->mapBlockFilterType);
	reader.tryRead("spawnBlocks", val->spawnBlocks);
	reader.tryRead("spawnBlockGroups", val->spawnBlockGroups);
	reader.tryRead("spawnNodeRanks", val->spawnNodeRanks);
	reader.tryRead("spawnZLevels", val->spawnZLevels);
	reader.tryRead("randomizeZLevels", val->randomizeZLevels);
	reader.tryRead("minDistanceFromXcomUnits", val->minDistanceFromXcomUnits);
	reader.tryRead("maxDistanceFromBorders", val->maxDistanceFromBorders);
	reader.tryRead("forceSpawnNearFriend", val->forceSpawnNearFriend);
	reader.tryRead("data", val->data);
	return true;
}

}
