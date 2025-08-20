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
#include "../Engine/Yaml.h"
#include "BattleUnit.h"
#include "SavedGame.h"

namespace OpenXcom
{

class Mod;
class RuleCommendations;
struct BattleUnitKills;

/**
 * Each entry will be its own commendation.
 */
class SoldierCommendations
{
private:
	std::string  _type, _noun;
	int  _decorationLevel;
	bool _isNew;
	RuleCommendations *_rule = nullptr;

public:
	/// Creates a new commendation and loads its contents from YAML.
	SoldierCommendations(const YAML::YamlNodeReader& reader, const Mod* mod);
	/// Creates a commendation of the specified type.
	SoldierCommendations(const std::string& commendationName, const std::string& noun, const Mod* mod);
	/// Cleans up the commendation.
	~SoldierCommendations();
	/// Loads the commendation information from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Saves the commendation information to YAML.
	void save(YAML::YamlNodeWriter writer) const;

	/// Get commendation rule config.
	RuleCommendations* getRule() { return _rule; }

	/// Get commendation name.
	const std::string &getType() const;
	/// Get commendation noun.
	const std::string &getNoun() const;
	/// Get the commendation's decoration level's name.
	std::string getDecorationLevelName(int skipCounter) const;
	/// Get the commendation's decoration description.
	std::string getDecorationDescription() const;
	/// Get the commendation's decoration level's int.
	int getDecorationLevelInt() const;
	/// Get the newness of the commendation.
	bool isNew() const;
	/// Set the commendation newness to false.
	void makeOld();
	/// Increment decoration level.
	// Sets _isNew to true.
	void addDecoration();
};

class SoldierDiary
{
private:
	std::vector<SoldierCommendations*> _commendations;
	std::vector<BattleUnitKills*> _killList;
	std::vector<int> _missionIdList;
	int _daysWoundedTotal, _totalShotByFriendlyCounter, _totalShotFriendlyCounter, _loneSurvivorTotal, _monthsService, _unconciousTotal, _shotAtCounterTotal,
		_hitCounterTotal, _ironManTotal, _longDistanceHitCounterTotal, _lowAccuracyHitCounterTotal, _shotsFiredCounterTotal, _shotsLandedCounterTotal,
		_shotAtCounter10in1Mission,	_hitCounter5in1Mission, _timesWoundedTotal, _KIA, _allAliensKilledTotal, _allAliensStunnedTotal,
		_woundsHealedTotal, _allUFOs, _allMissionTypes, _statGainTotal, _revivedUnitTotal, _wholeMedikitTotal, _braveryGainTotal, _bestOfRank, _MIA,
		_martyrKillsTotal, _postMortemKills, _slaveKillsTotal, _bestSoldier, _revivedSoldierTotal, _revivedHostileTotal, _revivedNeutralTotal;
	bool _globeTrotter;
public:
	/// Construct a diary.
	SoldierDiary();
	/// Cleans up a diary.
	~SoldierDiary();
	/// Load a diary.
	void load(const YAML::YamlNodeReader& reader, const Mod *mod);
	/// Save a diary.
	void save(YAML::YamlNodeWriter writer) const;
	/// Update the diary statistics.
	void updateDiary(BattleUnitStatistics*, std::vector<MissionStatistics*>*, Mod*);
	/// Get the list of kills, mapped by rank.
	std::map<std::string, int> getAlienRankTotal() const;
	/// Get the list of kills, mapped by race.
	std::map<std::string, int> getAlienRaceTotal() const;
	/// Get the list of kills, mapped by weapon used.
	std::map<std::string, int> getWeaponTotal() const;
	/// Get the list of kills, mapped by weapon ammo used.
	std::map<std::string, int> getWeaponAmmoTotal() const;
	/// Get the list of missions, mapped by region.
	std::map<std::string, int> getRegionTotal(std::vector<MissionStatistics*>*) const;
	/// Get the list of missions, mapped by country.
	std::map<std::string, int> getCountryTotal(std::vector<MissionStatistics*>*) const;
	/// Get the list of missions, mapped by type.
	std::map<std::string, int> getTypeTotal(std::vector<MissionStatistics*>*) const;
	/// Get the list of missions, mapped by UFO.
	std::map<std::string, int> getUFOTotal(std::vector<MissionStatistics*>*) const;
	/// Get the total number of kills.
	int getKillTotal() const;
	/// Get the total number of missions.
	int getMissionTotal() const;
	/// Get the total number of missions filtered by modder's criteria.
	int getMissionTotalFiltered(std::vector<MissionStatistics*>*, const RuleCommendations* rule) const;
	/// Get the total number of wins.
	int getWinTotal(std::vector<MissionStatistics*>*) const;
	/// Get the total number of stuns.
	int getStunTotal() const;
	/// Get the total number of psi panics.
	int getPanickTotal() const;
	/// Get the total number of psi mind controls.
	int getControlTotal() const;
	/// Get the total number of days wounded.
	int getDaysWoundedTotal() const;
	/// Get the solder's commendations.
	std::vector<SoldierCommendations*> *getSoldierCommendations();
	/// Checks whether the diary contains a given commendation.
	bool containsCommendation(const RuleCommendations* rule) const;
	/// Manage commendations, return true if a medal is awarded.
	bool manageCommendations(const Mod* mod, SavedGame* save, const Soldier* soldier);
	/// Increment the soldier's service time.
	void addMonthlyService();
	/// Get the total months in service.
	int getMonthsService() const;
	/// Get the mission id list.
	std::vector<int> &getMissionIdList();
	const std::vector<int>& getMissionIdList() const;
	/// Get the kill list.
	std::vector<BattleUnitKills*> &getKills();
	/// Award special commendation to the original 8 soldiers.
	void awardOriginalEightCommendation(const Mod* mod);
	/// Award posthumous best-of rank commendation.
	void awardBestOfRank(int score);
	/// Award posthumous best overall commendation.
	void awardBestOverall(int score);
	/// Award posthumous kills commendation.
	void awardPostMortemKill(int kills);
	/// Get the total number of shots fired.
	int getShotsFiredTotal() const;
	/// Get the total number of shots landed on target.
	int getShotsLandedTotal() const;
	/// Get the soldier's accuracy.
	int getAccuracy() const;
	/// Get the total number of trap kills.
	int getTrapKillTotal(const Mod* mod) const;
	/// Get the total number of reaction fire kills.
	int getReactionFireKillTotal(const Mod* mod) const;
	/// Get the total number of terror missions.
	int getTerrorMissionTotal(std::vector<MissionStatistics*>*) const;
	/// Get the total number of night missions.
	int getNightMissionTotal(std::vector<MissionStatistics*>*, const Mod* mod) const;
	/// Get the total number of night terror missions.
	int getNightTerrorMissionTotal(std::vector<MissionStatistics*>*, const Mod* mod) const;
	/// Get the total number of base defense missions.
	int getBaseDefenseMissionTotal(std::vector<MissionStatistics*>*) const;
	/// Get the total number of alien base assaults.
	int getAlienBaseAssaultTotal(std::vector<MissionStatistics*>*) const;
	/// Get the total number of important missions.
	int getImportantMissionTotal(std::vector<MissionStatistics*>*) const;
	/// Get the total score.
	int getScoreTotal(std::vector<MissionStatistics*>*) const;
	/// Get the Valiant Crux total.
	int getValiantCruxTotal(std::vector<MissionStatistics*>*) const;
	/// Get the loot value total.
	int getLootValueTotal(std::vector<MissionStatistics*>*) const;
};

}
