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
#include "SoldierDiary.h"
#include <algorithm>
#include "../Mod/RuleCommendations.h"
#include "../Mod/Mod.h"
#include "BattleUnitStatistics.h"
#include "MissionStatistics.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Initializes a new blank diary.
 */
SoldierDiary::SoldierDiary() : _daysWoundedTotal(0), _totalShotByFriendlyCounter(0), _totalShotFriendlyCounter(0), _loneSurvivorTotal(0),
	_monthsService(0), _unconciousTotal(0),	_shotAtCounterTotal(0), _hitCounterTotal(0), _ironManTotal(0), _longDistanceHitCounterTotal(0),
	_lowAccuracyHitCounterTotal(0), _shotsFiredCounterTotal(0), _shotsLandedCounterTotal(0), _shotAtCounter10in1Mission(0), _hitCounter5in1Mission(0),
	_timesWoundedTotal(0), _KIA(0), _allAliensKilledTotal(0), _allAliensStunnedTotal(0), _woundsHealedTotal(0), _allUFOs(0), _allMissionTypes(0),
	_statGainTotal(0), _revivedUnitTotal(0), _wholeMedikitTotal(0), _braveryGainTotal(0), _bestOfRank(0),
	_MIA(0), _martyrKillsTotal(0), _postMortemKills(0), _slaveKillsTotal(0), _bestSoldier(false),
	_revivedSoldierTotal(0), _revivedHostileTotal(0), _revivedNeutralTotal(0), _globeTrotter(false)
{
}

/**
 *
 */
SoldierDiary::~SoldierDiary()
{
	for (auto* comm : _commendations)
	{
		delete comm;
	}
	for (auto* buk : _killList)
	{
		delete buk;
	}
}

/**
 * Loads the diary from a YAML file.
 * @param node YAML node.
 */
void SoldierDiary::load(const YAML::Node& node, const Mod *mod)
{
	if (const YAML::Node &commendations = node["commendations"])
	{
		for (YAML::const_iterator i = commendations.begin(); i != commendations.end(); ++i)
		{
			SoldierCommendations *sc = new SoldierCommendations(*i, mod);
			if (sc->getRule())
			{
				_commendations.push_back(sc);
			}
			else
			{
				// obsolete commendation, ignore it... otherwise it would cause a crash later
				delete sc;
			}
		}
	}
	if (const YAML::Node &killList = node["killList"])
	{
		for (YAML::const_iterator i = killList.begin(); i != killList.end(); ++i)
			_killList.push_back(new BattleUnitKills(*i));
	}
	_missionIdList = node["missionIdList"].as<std::vector<int> >(_missionIdList);
	_daysWoundedTotal = node["daysWoundedTotal"].as<int>(_daysWoundedTotal);
	_totalShotByFriendlyCounter = node["totalShotByFriendlyCounter"].as<int>(_totalShotByFriendlyCounter);
	_totalShotFriendlyCounter = node["totalShotFriendlyCounter"].as<int>(_totalShotFriendlyCounter);
	_loneSurvivorTotal = node["loneSurvivorTotal"].as<int>(_loneSurvivorTotal);
	_monthsService = node["monthsService"].as<int>(_monthsService);
	_unconciousTotal = node["unconciousTotal"].as<int>(_unconciousTotal);
	_shotAtCounterTotal = node["shotAtCounterTotal"].as<int>(_shotAtCounterTotal);
	_hitCounterTotal = node["hitCounterTotal"].as<int>(_hitCounterTotal);
	_ironManTotal = node["ironManTotal"].as<int>(_ironManTotal);
	_longDistanceHitCounterTotal = node["longDistanceHitCounterTotal"].as<int>(_longDistanceHitCounterTotal);
	_lowAccuracyHitCounterTotal = node["lowAccuracyHitCounterTotal"].as<int>(_lowAccuracyHitCounterTotal);
	_shotsFiredCounterTotal = node["shotsFiredCounterTotal"].as<int>(_shotsFiredCounterTotal);
	_shotsLandedCounterTotal = node["shotsLandedCounterTotal"].as<int>(_shotsLandedCounterTotal);
	_shotAtCounter10in1Mission = node["shotAtCounter10in1Mission"].as<int>(_shotAtCounter10in1Mission);
	_hitCounter5in1Mission = node["hitCounter5in1Mission"].as<int>(_hitCounter5in1Mission);
	_timesWoundedTotal = node["timesWoundedTotal"].as<int>(_timesWoundedTotal);
	_KIA = node["killedInAction"].as<int>(_KIA);
	_allAliensKilledTotal = node["allAliensKilledTotal"].as<int>(_allAliensKilledTotal);
	_allAliensStunnedTotal = node["allAliensStunnedTotal"].as<int>(_allAliensStunnedTotal);
	_woundsHealedTotal = node["woundsHealedTotal"].as<int>(_woundsHealedTotal);
	_allUFOs = node["allUFOs"].as<int>(_allUFOs);
	_allMissionTypes = node["allMissionTypes"].as<int>(_allMissionTypes);
	_statGainTotal = node["statGainTotal"].as<int>(_statGainTotal);
	_revivedUnitTotal = node["revivedUnitTotal"].as<int>(_revivedUnitTotal);
	_revivedSoldierTotal = node["revivedSoldierTotal"].as<int>(_revivedSoldierTotal);
	_revivedHostileTotal = node["revivedHostileTotal"].as<int>(_revivedHostileTotal);
	_revivedNeutralTotal = node["revivedNeutralTotal"].as<int>(_revivedNeutralTotal);
	_wholeMedikitTotal = node["wholeMedikitTotal"].as<int>(_wholeMedikitTotal);
	_braveryGainTotal = node["braveryGainTotal"].as<int>(_braveryGainTotal);
	_bestOfRank = node["bestOfRank"].as<int>(_bestOfRank);
	_MIA = node["missingInAction"].as<int>(_MIA);
	_bestSoldier = node["bestSoldier"].as<bool>(_bestSoldier);
	_martyrKillsTotal = node["martyrKillsTotal"].as<int>(_martyrKillsTotal);
	_postMortemKills = node["postMortemKills"].as<int>(_postMortemKills);
	_globeTrotter = node["globeTrotter"].as<bool>(_globeTrotter);
	_slaveKillsTotal = node["slaveKillsTotal"].as<int>(_slaveKillsTotal);
}

/**
 * Saves the diary to a YAML file.
 * @return YAML node.
 */
YAML::Node SoldierDiary::save() const
{
	YAML::Node node;
	for (const auto* comm : _commendations)
	{
		node["commendations"].push_back(comm->save());
	}
	for (const auto* buk : _killList)
	{
		node["killList"].push_back(buk->save());
	}
	if (!_missionIdList.empty()) { YAML::Node t; t = _missionIdList; t.SetStyle(YAML::EmitterStyle::Flow); node["missionIdList"] = t; }
	if (_daysWoundedTotal) node["daysWoundedTotal"] = _daysWoundedTotal;
	if (_totalShotByFriendlyCounter) node["totalShotByFriendlyCounter"] = _totalShotByFriendlyCounter;
	if (_totalShotFriendlyCounter) node["totalShotFriendlyCounter"] = _totalShotFriendlyCounter;
	if (_loneSurvivorTotal) node["loneSurvivorTotal"] = _loneSurvivorTotal;
	if (_monthsService) node["monthsService"] = _monthsService;
	if (_unconciousTotal) node["unconciousTotal"] = _unconciousTotal;
	if (_shotAtCounterTotal) node["shotAtCounterTotal"] = _shotAtCounterTotal;
	if (_hitCounterTotal) node["hitCounterTotal"] = _hitCounterTotal;
	if (_ironManTotal) node["ironManTotal"] = _ironManTotal;
	if (_longDistanceHitCounterTotal) node["longDistanceHitCounterTotal"] = _longDistanceHitCounterTotal;
	if (_lowAccuracyHitCounterTotal) node["lowAccuracyHitCounterTotal"] = _lowAccuracyHitCounterTotal;
	if (_shotsFiredCounterTotal) node["shotsFiredCounterTotal"] = _shotsFiredCounterTotal;
	if (_shotsLandedCounterTotal) node["shotsLandedCounterTotal"] = _shotsLandedCounterTotal;
	if (_shotAtCounter10in1Mission) node["shotAtCounter10in1Mission"] = _shotAtCounter10in1Mission;
	if (_hitCounter5in1Mission) node["hitCounter5in1Mission"] = _hitCounter5in1Mission;
	if (_timesWoundedTotal) node["timesWoundedTotal"] = _timesWoundedTotal;
	if (_KIA) node["killedInAction"] = _KIA;
	if (_allAliensKilledTotal) node["allAliensKilledTotal"] = _allAliensKilledTotal;
	if (_allAliensStunnedTotal) node["allAliensStunnedTotal"] = _allAliensStunnedTotal;
	if (_woundsHealedTotal) node["woundsHealedTotal"] = _woundsHealedTotal;
	if (_allUFOs) node["allUFOs"] = _allUFOs;
	if (_allMissionTypes) node["allMissionTypes"] = _allMissionTypes;
	if (_statGainTotal) node["statGainTotal"] =_statGainTotal;
	if (_revivedUnitTotal) node["revivedUnitTotal"] = _revivedUnitTotal;
	if (_revivedSoldierTotal) node["revivedSoldierTotal"] = _revivedSoldierTotal;
	if (_revivedHostileTotal) node["revivedHostileTotal"] = _revivedHostileTotal;
	if (_revivedNeutralTotal) node["revivedNeutralTotal"] = _revivedNeutralTotal;
	if (_wholeMedikitTotal) node["wholeMedikitTotal"] = _wholeMedikitTotal;
	if (_braveryGainTotal) node["braveryGainTotal"] = _braveryGainTotal;
	if (_bestOfRank) node["bestOfRank"] = _bestOfRank;
	if (_MIA) node["missingInAction"] = _MIA;
	if (_bestSoldier) node["bestSoldier"] = _bestSoldier;
	if (_martyrKillsTotal) node["martyrKillsTotal"] = _martyrKillsTotal;
	if (_postMortemKills) node["postMortemKills"] = _postMortemKills;
	if (_globeTrotter) node["globeTrotter"] = _globeTrotter;
	if (_slaveKillsTotal) node["slaveKillsTotal"] = _slaveKillsTotal;
	return node;
}

/**
 * Update soldier diary statistics.
 * @param unitStatistics BattleUnitStatistics to get stats from.
 * @param missionStatistics MissionStatistics to get stats from.
 */
void SoldierDiary::updateDiary(BattleUnitStatistics *unitStatistics, std::vector<MissionStatistics*> *allMissionStatistics, Mod *rules)
{
	if (allMissionStatistics->empty()) return;
	auto* missionStatistics = allMissionStatistics->back();
	_missionIdList.push_back(missionStatistics->id);
	auto& unitKills = unitStatistics->kills;
	for (auto* buk : unitKills)
	{
		buk->makeTurnUnique();
		_killList.push_back(buk);
	}
	unitKills.clear();
	if (missionStatistics->success)
	{
		if (unitStatistics->loneSurvivor)
			_loneSurvivorTotal++;
		if (unitStatistics->ironMan)
			_ironManTotal++;
		if (unitStatistics->nikeCross)
			_allAliensKilledTotal++;
		if (unitStatistics->mercyCross)
			_allAliensStunnedTotal++;
	}
	_daysWoundedTotal += unitStatistics->daysWounded;
	if (unitStatistics->daysWounded)
		_timesWoundedTotal++;

	if (unitStatistics->wasUnconcious)
		_unconciousTotal++;
	_shotAtCounterTotal += unitStatistics->shotAtCounter;
	_shotAtCounter10in1Mission += (unitStatistics->shotAtCounter)/10;
	_hitCounterTotal += unitStatistics->hitCounter;
	_hitCounter5in1Mission += (unitStatistics->hitCounter)/5;
	_totalShotByFriendlyCounter += unitStatistics->shotByFriendlyCounter;
	_totalShotFriendlyCounter += unitStatistics->shotFriendlyCounter;
	_longDistanceHitCounterTotal += unitStatistics->longDistanceHitCounter;
	_lowAccuracyHitCounterTotal += unitStatistics->lowAccuracyHitCounter;
	_shotsFiredCounterTotal += unitStatistics->shotsFiredCounter;
	_shotsLandedCounterTotal += unitStatistics->shotsLandedCounter;
	if (unitStatistics->KIA)
		_KIA++;
	if (unitStatistics->MIA)
		_MIA++;
	_woundsHealedTotal += unitStatistics->woundsHealed;
	if (getUFOTotal(allMissionStatistics).size() >= rules->getUfosList().size())
		_allUFOs = 1;
	if ((getUFOTotal(allMissionStatistics).size() + getTypeTotal(allMissionStatistics).size()) == (rules->getUfosList().size() + rules->getDeploymentsList().size() - 2))
		_allMissionTypes = 1;
	if (getCountryTotal(allMissionStatistics).size() == 1 + rules->getCountriesList().size())
		_globeTrotter = true;
	_martyrKillsTotal += unitStatistics->martyr;
	_slaveKillsTotal += unitStatistics->slaveKills;

	// Stat change long hand calculation
	_statGainTotal = 0; // Reset.
	_statGainTotal += unitStatistics->delta.tu;
	_statGainTotal += unitStatistics->delta.stamina;
	_statGainTotal += unitStatistics->delta.health;
	_statGainTotal += unitStatistics->delta.bravery / 10; // Normalize
	_statGainTotal += unitStatistics->delta.reactions;
	_statGainTotal += unitStatistics->delta.firing;
	_statGainTotal += unitStatistics->delta.throwing;
	_statGainTotal += unitStatistics->delta.strength;
	_statGainTotal += unitStatistics->delta.mana;
	_statGainTotal += unitStatistics->delta.psiStrength;
	_statGainTotal += unitStatistics->delta.melee;
	_statGainTotal += unitStatistics->delta.psiSkill;

	_braveryGainTotal = unitStatistics->delta.bravery;
	_revivedUnitTotal += (unitStatistics->revivedSoldier + unitStatistics->revivedHostile + unitStatistics->revivedNeutral);
	_revivedSoldierTotal += unitStatistics->revivedSoldier;
	_revivedNeutralTotal += unitStatistics->revivedNeutral;
	_revivedHostileTotal += unitStatistics->revivedHostile;
	_wholeMedikitTotal += std::min( std::min(unitStatistics->woundsHealed, unitStatistics->appliedStimulant), unitStatistics->appliedPainKill);
}

/**
 * Get soldier commendations.
 * @return SoldierCommendations list of soldier's commendations.
 */
std::vector<SoldierCommendations*> *SoldierDiary::getSoldierCommendations()
{
	return &_commendations;
}

/**
 * Manage the soldier's commendations.
 * Award new ones, if deserved.
 * @return bool Has a commendation been awarded?
 */
bool SoldierDiary::manageCommendations(Mod *mod, std::vector<MissionStatistics*> *missionStatistics)
{
	const int BATTLE_TYPES = 13;
	const std::string battleTypeArray[BATTLE_TYPES] = { "BT_NONE", "BT_FIREARM", "BT_AMMO", "BT_MELEE", "BT_GRENADE",
		"BT_PROXIMITYGRENADE", "BT_MEDIKIT", "BT_SCANNER", "BT_MINDPROBE", "BT_PSIAMP", "BT_FLARE", "BT_CORPSE", "BT_END" };
	const int DAMAGE_TYPES = 21;
	const std::string damageTypeArray[DAMAGE_TYPES] = { "DT_NONE", "DT_AP", "DT_IN", "DT_HE", "DT_LASER", "DT_PLASMA",
		"DT_STUN", "DT_MELEE", "DT_ACID", "DT_SMOKE",
		"DT_10", "DT_11", "DT_12", "DT_13", "DT_14", "DT_15", "DT_16", "DT_17", "DT_18", "DT_19", "DT_END" };

	const auto& commendationsList = mod->getCommendationsList();
	bool ret = false;                                   // This value is returned if at least one commendation was given.
	std::map<std::string, int> nextCommendationLevel;   // Noun, threshold.
	std::vector<std::string> modularCommendations;      // Noun.
	bool awardCommendationBool = false;                 // This value determines if a commendation will be given.
	// Loop over all possible commendations
	for (auto iter = commendationsList.begin(); iter != commendationsList.end(); )
	{
		const auto& commType = (*iter).first;
		const RuleCommendations* commRule = (*iter).second;

		awardCommendationBool = true;
		nextCommendationLevel.clear();
		nextCommendationLevel["noNoun"] = 0;
		modularCommendations.clear();
		// Loop over all the soldier's commendations, see if we already have the commendation.
		// If so, get the level and noun.
		for (const auto* sc : _commendations)
		{
			if (commType == sc->getType())
			{
				nextCommendationLevel[sc->getNoun()] = sc->getDecorationLevelInt() + 1;
			}
		}
		// Go through each possible criteria. Assume the medal is awarded, set to false if not.
		// As soon as we find a medal criteria that we FAIL TO achieve, then we are not awarded a medal.
		for (const auto& critPair : *commRule->getCriteria())
		{
			const auto& critName = critPair.first;
			const auto& critDef = critPair.second;
			const size_t nextLevel = nextCommendationLevel["noNoun"];
			const int nextLevelThreshold = (nextLevel >= critDef.size()) ? 0 /* just a dummy */ : critDef.at(nextLevel);

			// Skip this medal if we have reached its max award level.
			if (nextLevel >= critDef.size())
			{
				awardCommendationBool = false;
				break;
			}
			// These criteria have no nouns, so only the nextCommendationLevel["noNoun"] will ever be used.
			else if
			(
				nextCommendationLevel.count("noNoun") == 1 &&
				(
					(critName == "totalKills" && getKillTotal() < nextLevelThreshold) ||
					(critName == "totalMissions" && getMissionTotalFiltered(missionStatistics, commRule) < nextLevelThreshold) ||
					(critName == "totalWins" && getWinTotal(missionStatistics) < nextLevelThreshold) ||
					(critName == "totalScore" && getScoreTotal(missionStatistics) < nextLevelThreshold) ||
					(critName == "totalStuns" && getStunTotal() < nextLevelThreshold) ||
					(critName == "totalDaysWounded" && _daysWoundedTotal < nextLevelThreshold) ||
					(critName == "totalBaseDefenseMissions" && getBaseDefenseMissionTotal(missionStatistics) < nextLevelThreshold) ||
					(critName == "totalTerrorMissions" && getTerrorMissionTotal(missionStatistics) < nextLevelThreshold) ||
					(critName == "totalNightMissions" && getNightMissionTotal(missionStatistics, mod) < nextLevelThreshold) ||
					(critName == "totalNightTerrorMissions" && getNightTerrorMissionTotal(missionStatistics, mod) < nextLevelThreshold) ||
					(critName == "totalMonthlyService" && _monthsService < nextLevelThreshold) ||
					(critName == "totalFellUnconcious" && _unconciousTotal < nextLevelThreshold) ||
					(critName == "totalShotAt10Times" && _shotAtCounter10in1Mission < nextLevelThreshold) ||
					(critName == "totalHit5Times" && _hitCounter5in1Mission < nextLevelThreshold) ||
					(critName == "totalFriendlyFired" && (_totalShotByFriendlyCounter < nextLevelThreshold || _KIA || _MIA)) ||
					(critName == "total_lone_survivor" && _loneSurvivorTotal < nextLevelThreshold) ||
					(critName == "totalIronMan" && _ironManTotal < nextLevelThreshold) ||
					(critName == "totalImportantMissions" && getImportantMissionTotal(missionStatistics) < nextLevelThreshold) ||
					(critName == "totalLongDistanceHits" && _longDistanceHitCounterTotal < nextLevelThreshold) ||
					(critName == "totalLowAccuracyHits" && _lowAccuracyHitCounterTotal < nextLevelThreshold) ||
					(critName == "totalReactionFire" && getReactionFireKillTotal(mod) < nextLevelThreshold) ||
					(critName == "totalTimesWounded" && _timesWoundedTotal < nextLevelThreshold) ||
					(critName == "totalDaysWounded" && _daysWoundedTotal < nextLevelThreshold) ||
					(critName == "totalValientCrux" && getValiantCruxTotal(missionStatistics) < nextLevelThreshold) ||
					(critName == "isDead" && _KIA < nextLevelThreshold) ||
					(critName == "totalTrapKills" && getTrapKillTotal(mod) < nextLevelThreshold) ||
					(critName == "totalAlienBaseAssaults" && getAlienBaseAssaultTotal(missionStatistics) < nextLevelThreshold) ||
					(critName == "totalAllAliensKilled" && _allAliensKilledTotal < nextLevelThreshold) ||
					(critName == "totalAllAliensStunned" && _allAliensStunnedTotal < nextLevelThreshold) ||
					(critName == "totalWoundsHealed" && _woundsHealedTotal < nextLevelThreshold) ||
					(critName == "totalAllUFOs" && _allUFOs < nextLevelThreshold) ||
					(critName == "totalAllMissionTypes" && _allMissionTypes < nextLevelThreshold) ||
					(critName == "totalStatGain" && _statGainTotal < nextLevelThreshold) ||
					(critName == "totalRevives" && _revivedUnitTotal < nextLevelThreshold) ||
					(critName == "totalSoldierRevives" && _revivedSoldierTotal < nextLevelThreshold) ||
					(critName == "totalHostileRevives" && _revivedHostileTotal < nextLevelThreshold) ||
					(critName == "totalNeutralRevives" && _revivedNeutralTotal < nextLevelThreshold) ||
					(critName == "totalWholeMedikit" && _wholeMedikitTotal < nextLevelThreshold) ||
					(critName == "totalBraveryGain" && _braveryGainTotal < nextLevelThreshold) ||
					(critName == "bestOfRank" && _bestOfRank < nextLevelThreshold) ||
					(critName == "bestSoldier" && _bestSoldier < nextLevelThreshold) ||
					(critName == "isMIA" && _MIA < nextLevelThreshold) ||
					(critName == "totalMartyrKills" && _martyrKillsTotal < nextLevelThreshold) ||
					(critName == "totalPostMortemKills" && _postMortemKills < nextLevelThreshold) ||
					(critName == "globeTrotter" && (int)_globeTrotter < nextLevelThreshold) ||
					(critName == "totalSlaveKills" && _slaveKillsTotal < nextLevelThreshold)
				)
			)
			{
				awardCommendationBool = false;
				break;
			}
			// Medals with the following criteria are unique because they need a noun.
			// And because they loop over a map<> (this allows for maximum moddability).
			else if (critName == "totalKillsWithAWeapon" || critName == "totalMissionsInARegion" || critName == "totalKillsByRace" || critName == "totalKillsByRank")
			{
				std::map<std::string, int> tempTotal;
				if (critName == "totalKillsWithAWeapon")
					tempTotal = getWeaponTotal();
				else if (critName == "totalMissionsInARegion")
					tempTotal = getRegionTotal(missionStatistics);
				else if (critName == "totalKillsByRace")
					tempTotal = getAlienRaceTotal();
				else if (critName == "totalKillsByRank")
					tempTotal = getAlienRankTotal();
				// Loop over the temporary map.
				// Match nouns and decoration levels.
				for (const auto& pair : tempTotal)
				{
					int criteria = -1;
					const auto& noun = pair.first;
					// If there is no matching noun, get the first award criteria.
					if (nextCommendationLevel.count(noun) == 0)
						criteria = critDef.front();
					// Otherwise, get the criteria that reflects the soldier's commendation level.
					else if ((unsigned int)nextCommendationLevel[noun] != critDef.size())
						criteria = critDef.at(nextCommendationLevel[noun]);

					// If a criteria was set AND the stat's count exceeds the criteria.
					if (criteria != -1 && pair.second >= criteria)
					{
						modularCommendations.push_back(noun);
					}
				}
				// If it is still empty, we did not get a commendation.
				if (modularCommendations.empty())
				{
					awardCommendationBool = false;
					break;
				}
			}
			// Medals that are based on _how_ a kill was achieved are found here.
			else if (critName == "killsWithCriteriaCareer" || critName == "killsWithCriteriaMission" || critName == "killsWithCriteriaTurn")
			{
				// Fetch the kill criteria list.
				if (!commRule->getKillCriteria())
					break;
				const auto* _killCriteriaList = commRule->getKillCriteria();

				int totalKillGroups = 0; // holds the total number of kill groups which satisfy one of the OR criteria blocks
				bool enoughForNextCommendation = false;

				// Loop over the OR vectors.
				// if OR criteria are not disjunctive (e.g. "kill 1 enemy" or "kill 1 enemy"), each one will be counted and added to totals - avoid that if you want good statistics
				for (auto orCriteria = _killCriteriaList->begin(); orCriteria != _killCriteriaList->end(); ++orCriteria)
				{
					// prepare counters
					std::vector<int> referenceBlockCounters;
					referenceBlockCounters.assign((*orCriteria).size(), 0);
					int referenceTotalCounters = 0;
					for (auto andCriteria2 = orCriteria->begin(); andCriteria2 != orCriteria->end(); ++andCriteria2)
					{
						int index = andCriteria2 - orCriteria->begin();
						referenceBlockCounters[index] = (*andCriteria2).first;
						referenceTotalCounters += (*andCriteria2).first;
					}
					std::vector<int> currentBlockCounters;
					if (critName == "killsWithCriteriaCareer") {
						currentBlockCounters = referenceBlockCounters;
					}
					int currentTotalCounters = referenceTotalCounters;
					int lastTimeSpan = -1;
					bool skipThisTimeSpan = false;
					// Loop over the KILLS, seeking to fulfill all criteria from entire AND block within the specified time span (career/mission/turn)
					for (const auto* singleKill : _killList)
					{
						int thisTimeSpan = -1;
						if (critName == "killsWithCriteriaMission")
						{
							thisTimeSpan = singleKill->mission;
						}
						else if (critName == "killsWithCriteriaTurn")
						{
							thisTimeSpan = singleKill->turn;
						}
						if (thisTimeSpan != lastTimeSpan)
						{
							// next time span, reset counters
							lastTimeSpan = thisTimeSpan;
							skipThisTimeSpan = false;
							currentBlockCounters = referenceBlockCounters;
							currentTotalCounters = referenceTotalCounters;
						}
						// same time span, we're skipping the rest of it if we already fulfilled criteria
						else if (skipThisTimeSpan)
						{
							continue;
						}

						bool andCriteriaMet = false;

						// Loop over the AND vectors.
						for (auto andCriteria = orCriteria->begin(); andCriteria != orCriteria->end(); ++andCriteria)
						{
							bool foundMatch = true;

							// Loop over the DETAILs of one AND vector.
							for (auto detailIt = andCriteria->second.begin(); detailIt != andCriteria->second.end(); ++detailIt)
							{
								const auto& detail = (*detailIt);
								// Look if for match for this criteria.
								// If we find a match, continue to the next criteria. (We must match all criteria in the list.)
								// If we don't find a match, set foundMatch = false; then break.

								if (singleKill->rank == detail ||
									singleKill->race == detail ||
									singleKill->weapon == detail ||
									singleKill->weaponAmmo == detail ||
									singleKill->getUnitStatusString() == detail ||
									singleKill->getUnitFactionString() == detail ||
									singleKill->getUnitSideString() == detail ||
									singleKill->getUnitBodyPartString() == detail)
								{
									// Found match
									continue;
								}

								// check the weapon's battle type and damage type
								RuleItem *weapon = mod->getItem(singleKill->weapon);
								if (weapon != 0)
								{
									int battleType = weapon->getBattleType();

									if (battleType >=0 && battleType < BATTLE_TYPES && battleTypeArray[battleType] == detail)
									{
										// the detail matched the weapon's battle type
										continue;
									}


									RuleItem *weaponAmmo = mod->getItem(singleKill->weaponAmmo);
									int damageType = -1;

									if (weaponAmmo != 0)
									{
										damageType = weaponAmmo->getDamageType()->ResistType;
									}
									else if (singleKill->weaponAmmo == "__GUNBUTT")
									{
										// If weaponAmmo == "__GUNBUTT", that means the gun's secondary melee attack was used.
										damageType = weapon->getMeleeType()->ResistType;
									}
									// If we were unable to determine the damage type, leave it as -1.

									if (damageType >= 0 && damageType < DAMAGE_TYPES && damageTypeArray[damageType] == detail)
									{
										// the detail matched the damage type
										continue;
									}
								}

								// That's all we can check. We didn't find a match
								foundMatch = false;
								break;
							} /// End of DETAIL loop.

							if (foundMatch)
							{
								int index = andCriteria - orCriteria->begin();
								// some current block counters might go into negatives, this is used to tally career kills correctly
								// currentTotalCounters will always ensure we're counting in proper batches
								if (currentBlockCounters[index]-- > 0 && --currentTotalCounters <= 0)
								{
									// we just counted all counters in a block to zero, this certainly means that the entire block criteria is fulfilled
									andCriteriaMet = true;
									break;
								}
							}
						} /// End of AND loop.

						if (andCriteriaMet)
						{
							// early exit if we got enough, no reason to continue iterations
							if (++totalKillGroups >= critDef.at(nextCommendationLevel["noNoun"]))
							{
								enoughForNextCommendation = true;
								break;
							}

							// "killsWithCriteriaTurn" and "killsWithCriteriaMission" are "peak achivements", they are counted once per their respective time span if criteria are fulfilled
							// so if we got them, we're skipping the rest of this time span to avoid counting more than once
							// e.g. 20 kills in a mission will not be counted as "10 kills in a mission" criteria twice
							// "killsWithCriteriaCareer" are totals, so they are never skipped this way
							if (critName == "killsWithCriteriaTurn" || critName == "killsWithCriteriaMission")
							{
								skipThisTimeSpan = true;
							}
							// for career kills we'll ADD reference counters to the current values and recalculate current total
							// this is used to count instances of full criteria blocks, e.g. if rules state that a career commendation must be awarded for 2 kills of alien leaders
							// and 1 kill of  alien commander, then we must ensure there's 2 leader kills + 1 commander kill for each instance of criteria fulfilled
							else if (critName == "killsWithCriteriaCareer")
							{
								currentTotalCounters = 0;
								for (std::size_t i2 = 0; i2 < currentBlockCounters.size(); i2++)
								{
									currentBlockCounters[i2] += referenceBlockCounters[i2];
									currentTotalCounters += std::max(currentBlockCounters[i2], 0);
								}
							}
						}
					} /// End of KILLs loop.

					if (enoughForNextCommendation)
						break; // stop iterating here too, we've got enough

				} /// End of OR loop.

				if (!enoughForNextCommendation)
					awardCommendationBool = false;

			}
		}
		if (awardCommendationBool)
		{
			// If we do not have modular medals, but are awarded a different medal,
			// its noun will be "noNoun".
			if (modularCommendations.empty())
			{
				modularCommendations.push_back("noNoun");
			}
			for (const auto& noun : modularCommendations)
			{
				bool newCommendation = true;
				for (auto* soldierComm : _commendations)
				{
					if (soldierComm->getType() == commType && soldierComm->getNoun() == noun)
					{
						soldierComm->addDecoration();
						newCommendation = false;
						break;
					}
				}
				if (newCommendation)
				{
					_commendations.push_back(new SoldierCommendations(commType, noun, mod));
				}
			}
			ret = true;
		}
		else
		{
			++iter;
		}
	}
	return ret;
}

/**
 * Get vector of mission ids.
 * @return Vector of mission ids.
 */
std::vector<int> &SoldierDiary::getMissionIdList()
{
	return _missionIdList;
}
const std::vector<int>& SoldierDiary::getMissionIdList() const
{
	return _missionIdList;
}

/**
 * Get vector of kills.
 * @return vector of BattleUnitKills
 */
std::vector<BattleUnitKills*> &SoldierDiary::getKills()
{
	return _killList;
}

/**
 * Get list of kills sorted by rank
 * @return
 */
std::map<std::string, int> SoldierDiary::getAlienRankTotal() const
{
	std::map<std::string, int> list;
	for (const auto* buk : _killList)
	{
		list[buk->rank]++;
	}
	return list;
}

/**
 *
 */
std::map<std::string, int> SoldierDiary::getAlienRaceTotal() const
{
	std::map<std::string, int> list;
	for (const auto* buk : _killList)
	{
		list[buk->race]++;
	}
	return list;
}

/**
 *
 */
std::map<std::string, int> SoldierDiary::getWeaponTotal() const
{
	std::map<std::string, int> list;
	for (const auto* buk : _killList)
	{
		if (buk->faction == FACTION_HOSTILE)
			list[buk->weapon]++;
	}
	return list;
}

/**
 *
 */
std::map<std::string, int> SoldierDiary::getWeaponAmmoTotal() const
{
	std::map<std::string, int> list;
	for (const auto* buk : _killList)
	{
		if (buk->faction == FACTION_HOSTILE)
			list[buk->weaponAmmo]++;
	}
	return list;
}

/**
 *  Get a map of the amount of missions done in each region.
 *  @param MissionStatistics
 */
std::map<std::string, int> SoldierDiary::getRegionTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	std::map<std::string, int> regionTotal;

	for (const auto* ms : *missionStatistics)
	{
		for (int missionId : _missionIdList)
		{
			if (missionId == ms->id)
			{
				regionTotal[ms->region]++;
			}
		}
	}

	return regionTotal;
}

/**
 *  Get a map of the amount of missions done in each country.
 *  @param MissionStatistics
 */
std::map<std::string, int> SoldierDiary::getCountryTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	std::map<std::string, int> countryTotal;

	for (const auto* ms : *missionStatistics)
	{
		for (int missionId : _missionIdList)
		{
			if (missionId == ms->id)
			{
				countryTotal[ms->country]++;
			}
		}
	}

	return countryTotal;
}

/**
 *  Get a map of the amount of missions done in each type.
 *  @param MissionStatistics
 */
std::map<std::string, int> SoldierDiary::getTypeTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	std::map<std::string, int> typeTotal;

	for (const auto* ms : *missionStatistics)
	{
		for (int missionId : _missionIdList)
		{
			if (missionId == ms->id)
			{
				typeTotal[ms->type]++;
			}
		}
	}

	return typeTotal;
}

/**
 *  Get a map of the amount of missions done in each UFO.
 *  @param MissionStatistics
 */
std::map<std::string, int> SoldierDiary::getUFOTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	std::map<std::string, int> ufoTotal;

	for (const auto* ms : *missionStatistics)
	{
		for (int missionId : _missionIdList)
		{
			if (missionId == ms->id)
			{
				ufoTotal[ms->ufo]++;
			}
		}
	}

	return ufoTotal;
}

/**
 *
 */
int SoldierDiary::getKillTotal() const
{
	int killTotal = 0;

	for (const auto* buk : _killList)
	{
		if (buk->status == STATUS_DEAD && buk->faction == FACTION_HOSTILE)
		{
			killTotal++;
		}
	}

	return killTotal;
}

/**
 *
 */
int SoldierDiary::getMissionTotal() const
{
	return _missionIdList.size();
}

/**
 *
 */
int SoldierDiary::getMissionTotalFiltered(std::vector<MissionStatistics*>* missionStatistics, const RuleCommendations* rule) const
{
	if (!rule->getMissionTypeNames().empty())
	{
		int total = 0;
		for (const auto* ms : *missionStatistics)
		{
			if (ms->success)
			{
				if (std::find(_missionIdList.begin(), _missionIdList.end(), ms->id) != _missionIdList.end())
				{
					if (std::find(rule->getMissionTypeNames().begin(), rule->getMissionTypeNames().end(), ms->type) != rule->getMissionTypeNames().end())
					{
						++total;
					}
				}
			}
		}
		return total;
	}
	else if (!rule->getMissionMarkerNames().empty())
	{
		int total = 0;
		for (const auto* ms : *missionStatistics)
		{
			if (ms->success)
			{
				if (std::find(_missionIdList.begin(), _missionIdList.end(), ms->id) != _missionIdList.end())
				{
					if (std::find(rule->getMissionMarkerNames().begin(), rule->getMissionMarkerNames().end(), ms->markerName) != rule->getMissionMarkerNames().end())
					{
						++total;
					}
				}
			}
		}
		return total;
	}

	return (int)_missionIdList.size();
}

/**
 *  Get the total if wins.
 *  @param Mission Statistics
 */
int SoldierDiary::getWinTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	int winTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int missionId : _missionIdList)
		{
			if (missionId == ms->id)
			{
				if (ms->success)
				{
					winTotal++;
				}
			}
		}
	}

	return winTotal;
}

/**
 *
 */
int SoldierDiary::getStunTotal() const
{
	int stunTotal = 0;

	for (const auto* buk : _killList)
	{
		if (buk->status == STATUS_UNCONSCIOUS && buk->faction == FACTION_HOSTILE)
		{
			stunTotal++;
		}
	}

	return stunTotal;
}

/**
 *
 */
int SoldierDiary::getPanickTotal() const
{
	int panickTotal = 0;

	for (const auto* buk : _killList)
	{
		if (buk->status == STATUS_PANICKING && buk->faction == FACTION_HOSTILE)
		{
			panickTotal++;
		}
	}

	return panickTotal;
}

/**
 *
 */
int SoldierDiary::getControlTotal() const
{
	int controlTotal = 0;

	for (const auto* buk : _killList)
	{
		if (buk->status == STATUS_TURNING && buk->faction == FACTION_HOSTILE)
		{
			controlTotal++;
		}
	}

	return controlTotal;
}

/**
 *
 */
int SoldierDiary::getDaysWoundedTotal() const
{
	return _daysWoundedTotal;
}

/**
 * Increment soldier's service time one month
 */
void SoldierDiary::addMonthlyService()
{
	_monthsService++;
}

/**
 * Returns the total months this soldier has been in service.
 */
int SoldierDiary::getMonthsService() const
{
	return _monthsService;
}

/**
 * Award special commendation to the original 8 soldiers.
 */
void SoldierDiary::awardOriginalEightCommendation(const Mod* mod)
{
	// TODO: Unhardcode this
	_commendations.push_back(new SoldierCommendations("STR_MEDAL_ORIGINAL8_NAME", "NoNoun", mod));
}

/**
 * Award posthumous best-of commendation.
 */
void SoldierDiary::awardBestOfRank(int score)
{
	_bestOfRank = score;
}

/**
 * Award posthumous best-of commendation.
 */
void SoldierDiary::awardBestOverall(int score)
{
	_bestSoldier = score;
}

/**
 * Award posthumous kills commendation.
 */
void SoldierDiary::awardPostMortemKill(int kills)
{
	_postMortemKills = kills;
}

/**
 *
 */
int SoldierDiary::getShotsFiredTotal() const
{
	return _shotsFiredCounterTotal;
}

/**
 *
 */
int SoldierDiary::getShotsLandedTotal() const
{
	return _shotsLandedCounterTotal;
}

/**
 *  Get the soldier's accuracy.
 */
int SoldierDiary::getAccuracy() const
{
	if (_shotsFiredCounterTotal != 0)
		return 100 * _shotsLandedCounterTotal / _shotsFiredCounterTotal;
	return 0;
}

/**
 *  Get trap kills total.
 */
int SoldierDiary::getTrapKillTotal(Mod *mod) const
{
	int trapKillTotal = 0;

	for (const auto* buk : _killList)
	{
		RuleItem *item = mod->getItem(buk->weapon);
		if (buk->hostileTurn() && (item == 0 || item->getBattleType() == BT_GRENADE || item->getBattleType() == BT_PROXIMITYGRENADE))
		{
			trapKillTotal++;
		}
	}

	return trapKillTotal;
}

/**
 *  Get reaction kill total.
 */
 int SoldierDiary::getReactionFireKillTotal(Mod *mod) const
 {
	int reactionFireKillTotal = 0;

	for (const auto* buk : _killList)
	{
		RuleItem *item = mod->getItem(buk->weapon);
		if (buk->hostileTurn() && item != 0 && item->getBattleType() != BT_GRENADE && item->getBattleType() != BT_PROXIMITYGRENADE)
		{
			reactionFireKillTotal++;
		}
	}

	return reactionFireKillTotal;
 }

/**
 *  Get the total of terror missions.
 *  @param Mission Statistics
 */
int SoldierDiary::getTerrorMissionTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	/// Not a UFO, not the base, not the alien base or colony
	int terrorMissionTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				if (ms->success && !ms->isBaseDefense() && !ms->isUfoMission() && !ms->isAlienBase())
				{
					terrorMissionTotal++;
				}
			}
		}
	}

	return terrorMissionTotal;
}

/**
 *  Get the total of night missions.
 *  @param Mission Statistics
 */
int SoldierDiary::getNightMissionTotal(std::vector<MissionStatistics*> *missionStatistics, const Mod* mod) const
{
	int nightMissionTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				if (ms->success && ms->isDarkness(mod) && !ms->isBaseDefense() && !ms->isAlienBase())
				{
					nightMissionTotal++;
				}
			}
		}
	}

	return nightMissionTotal;
}

/**
 *  Get the total of night terror missions.
 *  @param Mission Statistics
 */
int SoldierDiary::getNightTerrorMissionTotal(std::vector<MissionStatistics*> *missionStatistics, const Mod* mod) const
{
	int nightTerrorMissionTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				if (ms->success && ms->isDarkness(mod) && !ms->isBaseDefense() && !ms->isUfoMission() && !ms->isAlienBase())
				{
					nightTerrorMissionTotal++;
				}
			}
		}
	}

	return nightTerrorMissionTotal;
}

/**
 *  Get the total of base defense missions.
 *  @param Mission Statistics
 */
int SoldierDiary::getBaseDefenseMissionTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	int baseDefenseMissionTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				if (ms->success && ms->isBaseDefense())
				{
					baseDefenseMissionTotal++;
				}
			}
		}
	}

	return baseDefenseMissionTotal;
}

/**
 *  Get the total of alien base assaults.
 *  @param Mission Statistics
 */
int SoldierDiary::getAlienBaseAssaultTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	int alienBaseAssaultTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				if (ms->success && ms->isAlienBase())
				{
					alienBaseAssaultTotal++;
				}
			}
		}
	}

	return alienBaseAssaultTotal;
}

/**
 *  Get the total of important missions.
 *  @param Mission Statistics
 */
int SoldierDiary::getImportantMissionTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	int importantMissionTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				if (ms->success && ms->type != "STR_UFO_CRASH_RECOVERY")
				{
					importantMissionTotal++;
				}
			}
		}
	}

	return importantMissionTotal;
}

/**
 *  Get the total score.
 *  @param Mission Statistics
 */
int SoldierDiary::getScoreTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	int scoreTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				scoreTotal += ms->score;
			}
		}
	}

	return scoreTotal;
}

/**
 *  Get the Valiant Crux total.
 *  @param Mission Statistics
 */
int SoldierDiary::getValiantCruxTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	int valiantCruxTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id && ms->valiantCrux)
			{
				valiantCruxTotal++;
			}
		}
	}

	return valiantCruxTotal;
}

/**
 *  Get the loot value total.
 *  @param Mission Statistics
 */
int SoldierDiary::getLootValueTotal(std::vector<MissionStatistics*> *missionStatistics) const
{
	int lootValueTotal = 0;

	for (const auto* ms : *missionStatistics)
	{
		for (int misionId : _missionIdList)
		{
			if (misionId == ms->id)
			{
				lootValueTotal += ms->lootValue;
			}
		}
	}

	return lootValueTotal;
}

/**
 * Initializes a new commendation entry from YAML.
 * @param node YAML node.
 */
SoldierCommendations::SoldierCommendations(const YAML::Node &node, const Mod* mod)
{
	load(node);
	_rule = mod->getCommendation(_type, false); //TODO: during load we can load obsolete value, some else need cleanup
}

/**
 * Initializes a soldier commendation.
 */
SoldierCommendations::SoldierCommendations(const std::string& commendationName, const std::string& noun, const Mod* mod) :
	_type(commendationName), _noun(noun), _decorationLevel(0), _isNew(true)
{
	_rule = mod->getCommendation(_type, true); //if there is no commendation rule then someone messed up
}

/**
 *
 */
SoldierCommendations::~SoldierCommendations()
{
}

/**
 * Loads the commendation from a YAML file.
 * @param node YAML node.
 */
void SoldierCommendations::load(const YAML::Node &node)
{
	_type = node["commendationName"].as<std::string>(_type);
	_noun = node["noun"].as<std::string>("noNoun");
	_decorationLevel = node["decorationLevel"].as<int>(_decorationLevel);
	_isNew = node["isNew"].as<bool>(false);
}

/**
 * Saves the commendation to a YAML file.
 * @return YAML node.
 */
YAML::Node SoldierCommendations::save() const
{
	YAML::Node node;
	node.SetStyle(YAML::EmitterStyle::Flow);
	node["commendationName"] = _type;
	if (_noun != "noNoun") node["noun"] = _noun;
	node["decorationLevel"] = _decorationLevel;
	return node;
}

/**
 * Get the soldier's commendation's name.
 * @return string Commendation name.
 */
const std::string& SoldierCommendations::getType() const
{
	return _type;
}

/**
 * Get the soldier's commendation's noun.
 * @return string Commendation noun
 */
const std::string& SoldierCommendations::getNoun() const
{
	return _noun;
}

/**
 * Get the soldier commendation level's name.
 * @return string Commendation level.
 */
std::string SoldierCommendations::getDecorationLevelName(int skipCounter) const
{
	std::ostringstream ss;
	ss << "STR_AWARD_" << _decorationLevel - skipCounter;
	return ss.str();
}

/**
 * Get the soldier commendation level's description.
 * @return string Commendation level description.
 */
std::string SoldierCommendations::getDecorationDescription() const
{
	std::ostringstream ss;
	ss << "STR_AWARD_DECOR_" << _decorationLevel;
	return ss.str();
}

/**
 * Get the soldier commendation level's int.
 * @return int Commendation level.
 */
int SoldierCommendations::getDecorationLevelInt() const
{
	return _decorationLevel;
}

/**
 * Get newness of commendation.
 * @return bool Is the commendation new?
 */
bool SoldierCommendations::isNew() const
{
	return _isNew;
}

/**
 * Set the newness of the commendation to old.
 */
void SoldierCommendations::makeOld()
{
	_isNew = false;
}

/**
 * Add a level of decoration to the commendation.
 * Sets isNew to true.
 */
void SoldierCommendations::addDecoration()
{
	_decorationLevel++;
	_isNew = true;
}

}
