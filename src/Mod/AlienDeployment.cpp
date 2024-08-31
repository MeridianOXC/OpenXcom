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

namespace YAML
{
	template<>
	struct convert<OpenXcom::ItemSet>
	{
		static Node encode(const OpenXcom::ItemSet& rhs)
		{
			Node node;
			node = rhs.items;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::ItemSet& rhs)
		{
			if (!node.IsSequence())
				return false;

			rhs.items = node.as< std::vector<std::string> >(rhs.items);
			return true;
		}
	};
	template<>
	struct convert<OpenXcom::DeploymentData>
	{
		static Node encode(const OpenXcom::DeploymentData& rhs)
		{
			Node node;
			node["alienRank"] = rhs.alienRank;
			node["customUnitType"] = rhs.customUnitType;
			node["lowQty"] = rhs.lowQty;
			node["medQty"] = rhs.medQty;
			node["highQty"] = rhs.highQty;
			node["dQty"] = rhs.dQty;
			node["extraQty"] = rhs.extraQty;
			node["percentageOutsideUfo"] = rhs.percentageOutsideUfo;
			node["itemSets"] = rhs.itemSets;
			node["extraRandomItems"] = rhs.extraRandomItems;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::DeploymentData& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.alienRank = node["alienRank"].as<int>(rhs.alienRank);
			rhs.customUnitType = node["customUnitType"].as<std::string>(rhs.customUnitType);
			rhs.lowQty = node["lowQty"].as<int>(rhs.lowQty);
			rhs.medQty = node["medQty"].as<int>(rhs.medQty);
			rhs.highQty = node["highQty"].as<int>(rhs.highQty);
			rhs.dQty = node["dQty"].as<int>(rhs.dQty);
			rhs.extraQty = node["extraQty"].as<int>(0); // give this a default, as it's not 100% needed, unlike the others.
			rhs.percentageOutsideUfo = node["percentageOutsideUfo"].as<int>(rhs.percentageOutsideUfo);
			rhs.itemSets = node["itemSets"].as< std::vector<OpenXcom::ItemSet> >(rhs.itemSets);
			rhs.extraRandomItems = node["extraRandomItems"].as< std::vector<OpenXcom::ItemSet> >(rhs.extraRandomItems);
			return true;
		}
	};
	template<>
	struct convert<OpenXcom::BriefingData>
	{
		static Node encode(const OpenXcom::BriefingData& rhs)
		{
			Node node;
			node["palette"] = rhs.palette;
			node["textOffset"] = rhs.textOffset;
			node["title"] = rhs.title;
			node["desc"] = rhs.desc;
			node["music"] = rhs.music;
			node["cutscene"] = rhs.cutscene;
			node["background"] = rhs.background;
			node["showCraft"] = rhs.showCraft;
			node["showTarget"] = rhs.showTarget;
			return node;
		}
		static bool decode(const Node& node, OpenXcom::BriefingData& rhs)
		{
			if (!node.IsMap())
				return false;
			rhs.palette = node["palette"].as<int>(rhs.palette);
			rhs.textOffset = node["textOffset"].as<int>(rhs.textOffset);
			rhs.title = node["title"].as<std::string>(rhs.title);
			rhs.desc = node["desc"].as<std::string>(rhs.desc);
			rhs.music = node["music"].as<std::string>(rhs.music);
			rhs.cutscene = node["cutscene"].as<std::string>(rhs.cutscene);
			rhs.background = node["background"].as<std::string>(rhs.background);
			rhs.showCraft = node["showCraft"].as<bool>(rhs.showCraft);
			rhs.showTarget = node["showTarget"].as<bool>(rhs.showTarget);
			return true;
		}
	};
	template<>
	struct convert<OpenXcom::ReinforcementsData>
	{
		static Node encode(const OpenXcom::ReinforcementsData& rhs)
		{
			Node node;
			node["type"] = rhs.type;
			node["briefing"] = rhs.briefing;
			node["minDifficulty"] = rhs.minDifficulty;
			node["maxDifficulty"] = rhs.maxDifficulty;
			node["objectiveDestroyed"] = rhs.objectiveDestroyed;
			node["turns"] = rhs.turns;
			node["minTurn"] = rhs.minTurn;
			node["maxTurn"] = rhs.maxTurn;
			node["executionOdds"] = rhs.executionOdds;
			node["maxRuns"] = rhs.maxRuns;
			node["useSpawnNodes"] = rhs.useSpawnNodes;
			node["mapBlockFilterType"] = (int)(rhs.mapBlockFilterType);
			node["spawnBlocks"] = rhs.spawnBlocks;
			node["spawnBlockGroups"] = rhs.spawnBlockGroups;
			node["spawnNodeRanks"] = rhs.spawnNodeRanks;
			node["spawnZLevels"] = rhs.spawnZLevels;
			node["randomizeZLevels"] = rhs.randomizeZLevels;
			node["minDistanceFromXcomUnits"] = rhs.minDistanceFromXcomUnits;
			node["maxDistanceFromBorders"] = rhs.maxDistanceFromBorders;
			node["forceSpawnNearFriend"] = rhs.forceSpawnNearFriend;
			node["data"] = rhs.data;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::ReinforcementsData& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.type = node["type"].as<std::string>(rhs.type);
			rhs.briefing = node["briefing"].as< OpenXcom::BriefingData >(rhs.briefing);
			rhs.minDifficulty = node["minDifficulty"].as<int>(rhs.minDifficulty);
			rhs.maxDifficulty = node["maxDifficulty"].as<int>(rhs.maxDifficulty);
			rhs.objectiveDestroyed = node["objectiveDestroyed"].as<bool>(rhs.objectiveDestroyed);
			rhs.turns = node["turns"].as< std::vector<int> >(rhs.turns);
			rhs.minTurn = node["minTurn"].as<int>(rhs.minTurn);
			rhs.maxTurn = node["maxTurn"].as<int>(rhs.maxTurn);
			rhs.executionOdds = node["executionOdds"].as<int>(rhs.executionOdds);
			rhs.maxRuns = node["maxRuns"].as<int>(rhs.maxRuns);
			rhs.useSpawnNodes = node["useSpawnNodes"].as<bool>(rhs.useSpawnNodes);
			rhs.mapBlockFilterType = (OpenXcom::MapBlockFilterType)(node["mapBlockFilterType"].as<int>(rhs.mapBlockFilterType));
			rhs.spawnBlocks = node["spawnBlocks"].as< std::vector<std::string> >(rhs.spawnBlocks);
			rhs.spawnBlockGroups = node["spawnBlockGroups"].as< std::vector<int> >(rhs.spawnBlockGroups);
			rhs.spawnNodeRanks = node["spawnNodeRanks"].as< std::vector<int> >(rhs.spawnNodeRanks);
			rhs.spawnZLevels = node["spawnZLevels"].as< std::vector<int> >(rhs.spawnZLevels);
			rhs.randomizeZLevels = node["randomizeZLevels"].as<bool>(rhs.randomizeZLevels);
			rhs.minDistanceFromXcomUnits = node["minDistanceFromXcomUnits"].as<int>(rhs.minDistanceFromXcomUnits);
			rhs.maxDistanceFromBorders = node["maxDistanceFromBorders"].as<int>(rhs.maxDistanceFromBorders);
			rhs.forceSpawnNearFriend = node["forceSpawnNearFriend"].as<bool>(rhs.forceSpawnNearFriend);
			rhs.data = node["data"].as< std::vector<OpenXcom::DeploymentData> >(rhs.data);
			return true;
		}
	};
}

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
void AlienDeployment::load(const YAML::Node &node, Mod *mod)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod);
	}

	_customUfo = node["customUfo"].as<std::string>(_customUfo);
	_enviroEffects = node["enviroEffects"].as<std::string>(_enviroEffects);
	_startingCondition = node["startingCondition"].as<std::string>(_startingCondition);
	_unlockedResearchOnSuccess = node["unlockedResearch"].as<std::string>(_unlockedResearchOnSuccess);
	_unlockedResearchOnFailure = node["unlockedResearchOnFailure"].as<std::string>(_unlockedResearchOnFailure);
	_unlockedResearchOnDespawn = node["unlockedResearchOnDespawn"].as<std::string>(_unlockedResearchOnDespawn);
	_counterSuccess = node["counterSuccess"].as<std::string>(_counterSuccess);
	_counterFailure = node["counterFailure"].as<std::string>(_counterFailure);
	_counterDespawn = node["counterDespawn"].as<std::string>(_counterDespawn);
	_counterAll = node["counterAll"].as<std::string>(_counterAll);
	_decreaseCounterSuccess = node["decreaseCounterSuccess"].as<std::string>(_decreaseCounterSuccess);
	_decreaseCounterFailure = node["decreaseCounterFailure"].as<std::string>(_decreaseCounterFailure);
	_decreaseCounterDespawn = node["decreaseCounterDespawn"].as<std::string>(_decreaseCounterDespawn);
	_decreaseCounterAll = node["decreaseCounterAll"].as<std::string>(_decreaseCounterAll);
	_missionBountyItem = node["missionBountyItem"].as<std::string>(_missionBountyItem);
	_missionBountyItemCount = node["missionBountyItemCount"].as<int>(_missionBountyItemCount);
	_bughuntMinTurn = node["bughuntMinTurn"].as<int>(_bughuntMinTurn);
	_forcePercentageOutsideUfo = node["forcePercentageOutsideUfo"].as<bool>(_forcePercentageOutsideUfo);
	_data = node["data"].as< std::vector<DeploymentData> >(_data);
	_reinforcements = node["reinforcements"].as< std::vector<ReinforcementsData> >(_reinforcements);
	_width = node["width"].as<int>(_width);
	_length = node["length"].as<int>(_length);
	_height = node["height"].as<int>(_height);
	_civilians = node["civilians"].as<int>(_civilians);
	_markCiviliansAsVIP = node["markCiviliansAsVIP"].as<bool>(_markCiviliansAsVIP);
	_civilianSpawnNodeRank = node["civilianSpawnNodeRank"].as<int>(_civilianSpawnNodeRank);
	mod->loadUnorderedNamesToInt(_type, _civiliansByType, node["civiliansByType"]);
	_terrains = node["terrains"].as<std::vector<std::string> >(_terrains);
	_shade = node["shade"].as<int>(_shade);
	_minShade = node["minShade"].as<int>(_minShade);
	_maxShade = node["maxShade"].as<int>(_maxShade);
	_nextStage = node["nextStage"].as<std::string>(_nextStage);
	_race = node["race"].as<std::string>(_race);
	_randomRaces = node["randomRace"].as<std::vector<std::string> >(_randomRaces);
	_finalDestination = node["finalDestination"].as<bool>(_finalDestination);
	_winCutscene = node["winCutscene"].as<std::string>(_winCutscene);
	_loseCutscene = node["loseCutscene"].as<std::string>(_loseCutscene);
	_abortCutscene = node["abortCutscene"].as<std::string>(_abortCutscene);
	_mapScript = node["script"].as<std::string>(_mapScript);
	_mapScripts = node["mapScripts"].as<std::vector<std::string> >(_mapScripts);
	_alert = node["alert"].as<std::string>(_alert);
	_alertBackground = node["alertBackground"].as<std::string>(_alertBackground);
	_alertDescription = node["alertDescription"].as<std::string>(_alertDescription);
	mod->loadSoundOffset(_type, _alertSound, node["alertSound"], "GEO.CAT");
	_briefingData = node["briefing"].as<BriefingData>(_briefingData);
	_markerName = node["markerName"].as<std::string>(_markerName);
	if (node["markerIcon"])
	{
		_markerIcon = mod->getOffset(node["markerIcon"].as<int>(_markerIcon), 8);
	}
	if (node["depth"])
	{
		_minDepth = node["depth"][0].as<int>(_minDepth);
		_maxDepth = node["depth"][1].as<int>(_maxDepth);
	}
	if (node["duration"])
	{
		_durationMin = node["duration"][0].as<int>(_durationMin);
		_durationMax = node["duration"][1].as<int>(_durationMax);
	}
	_music = node["music"].as< std::vector<std::string> >(_music);
	_objectiveType = node["objectiveType"].as<int>(_objectiveType);
	_objectivesRequired = node["objectivesRequired"].as<int>(_objectivesRequired);
	_objectivePopup = node["objectivePopup"].as<std::string>(_objectivePopup);

	if (node["objectiveComplete"])
	{
		_objectiveCompleteText = node["objectiveComplete"][0].as<std::string>(_objectiveCompleteText);
		_objectiveCompleteScore = node["objectiveComplete"][1].as<int>(_objectiveCompleteScore);
	}
	if (node["objectiveFailed"])
	{
		_objectiveFailedText = node["objectiveFailed"][0].as<std::string>(_objectiveFailedText);
		_objectiveFailedScore = node["objectiveFailed"][1].as<int>(_objectiveFailedScore);
	}
	_missionCompleteText = node["missionCompleteText"].as<std::string>(_missionCompleteText);
	_missionFailedText = node["missionFailedText"].as<std::string>(_missionFailedText);
	if (node["successEvents"])
	{
		_successEvents.load(node["successEvents"]);
	}
	if (node["despawnEvents"])
	{
		_despawnEvents.load(node["despawnEvents"]);
	}
	if (node["failureEvents"])
	{
		_failureEvents.load(node["failureEvents"]);
	}
	_despawnPenalty = node["despawnPenalty"].as<int>(_despawnPenalty);
	_abortPenalty = node["abortPenalty"].as<int>(_abortPenalty);
	_points = node["points"].as<int>(_points);
	_cheatTurn = node["cheatTurn"].as<int>(_cheatTurn);
	_turnLimit = node["turnLimit"].as<int>(_turnLimit);
	_chronoTrigger = ChronoTrigger(node["chronoTrigger"].as<int>(_chronoTrigger));
	_isAlienBase = node["alienBase"].as<bool>(_isAlienBase);
	_isHidden = node["isHidden"].as<bool>(_isHidden);
	_fakeUnderwaterSpawnChance = node["fakeUnderwaterSpawnChance"].as<int>(_fakeUnderwaterSpawnChance);
	_keepCraftAfterFailedMission = node["keepCraftAfterFailedMission"].as<bool>(_keepCraftAfterFailedMission);
	_allowObjectiveRecovery = node["allowObjectiveRecovery"].as<bool>(_allowObjectiveRecovery);
	_escapeType = EscapeType(node["escapeType"].as<int>(_escapeType));
	_vipSurvivalPercentage = node["vipSurvivalPercentage"].as<int>(_vipSurvivalPercentage);
	if (node["genMission"])
	{
		_genMission.load(node["genMission"]);
	}
	_genMissionFrequency = node["genMissionFreq"].as<int>(_genMissionFrequency);
	_genMissionLimit = node["genMissionLimit"].as<int>(_genMissionLimit);
	_genMissionRaceFromAlienBase = node["genMissionRaceFromAlienBase"].as<bool>(_genMissionRaceFromAlienBase);

	_baseSelfDestructCode = node["baseSelfDestructCode"].as<std::string>(_baseSelfDestructCode);
	_baseDetectionRange = node["baseDetectionRange"].as<int>(_baseDetectionRange);
	_baseDetectionChance = node["baseDetectionChance"].as<int>(_baseDetectionChance);
	_huntMissionMaxFrequency = node["huntMissionMaxFrequency"].as<int>(_huntMissionMaxFrequency);
	_huntMissionRaceFromAlienBase = node["huntMissionRaceFromAlienBase"].as<bool>(_huntMissionRaceFromAlienBase);
	if (const YAML::Node &weights = node["huntMissionWeights"])
	{
		for (YAML::const_iterator nn = weights.begin(); nn != weights.end(); ++nn)
		{
			WeightedOptions *nw = new WeightedOptions();
			nw->load(nn->second);
			_huntMissionDistribution.push_back(std::make_pair(nn->first.as<size_t>(0), nw));
		}
	}
	if (const YAML::Node &weights = node["alienBaseUpgrades"])
	{
		for (YAML::const_iterator nn = weights.begin(); nn != weights.end(); ++nn)
		{
			WeightedOptions *nw = new WeightedOptions();
			nw->load(nn->second);
			_alienBaseUpgrades.push_back(std::make_pair(nn->first.as<size_t>(0), nw));
		}
	}
	_resetAlienBaseAgeAfterUpgrade = node["resetAlienBaseAgeAfterUpgrade"].as<bool>(_resetAlienBaseAgeAfterUpgrade);
	_resetAlienBaseAge = node["resetAlienBaseAge"].as<bool>(_resetAlienBaseAge);
	_upgradeRace = node["upgradeRace"].as<std::string>(_upgradeRace);
	_noWeaponPile = node["noWeaponPile"].as<bool>(_noWeaponPile);
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

}
