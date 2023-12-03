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
#include <string>
#include <yaml-cpp/yaml.h>
#include "../Mod/Unit.h"
#include "../Mod/StatString.h"
#include "../Engine/Script.h"

namespace OpenXcom
{

enum SoldierRank : char { RANK_ROOKIE, RANK_SQUADDIE, RANK_SERGEANT, RANK_CAPTAIN, RANK_COLONEL, RANK_COMMANDER};
enum SoldierGender : char { GENDER_MALE, GENDER_FEMALE };
enum SoldierLook : char { LOOK_BLONDE, LOOK_BROWNHAIR, LOOK_ORIENTAL, LOOK_AFRICAN };

class Craft;
class SoldierNamePool;
class Mod;
class RuleSoldier;
class Armor;
class Language;
class EquipmentLayoutItem;
class SoldierDeath;
class SoldierDiary;
class SavedGame;
class RuleSoldierTransformation;
class RuleSoldierBonus;
class Base;
struct BaseSumDailyRecovery;

/**
 * Represents a soldier hired by the player.
 * Soldiers have a wide variety of stats that affect
 * their performance during battles.
 */
class Soldier
{
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "GeoscapeSoldier";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

private:
	std::string _name;
	std::string _callsign;
	int _id, _nationality, _improvement, _psiStrImprovement;
	RuleSoldier *_rules;
	UnitStats _initialStats, _currentStats, _tmpStatsWithSoldierBonuses, _tmpStatsWithAllBonuses;
	UnitStats _dailyDogfightExperienceCache;
	SoldierRank _rank;
	Craft *_craft;
	SoldierGender _gender;
	SoldierLook _look;
	int _lookVariant;
	int _missions, _kills, _stuns;
	int _healthMissing = 0; // amount of health missing until full health recovery, this is less serious than wound recovery.
	int _manaMissing = 0;   // amount of mana missing until full mana recovery
	float _recovery = 0.0;  // amount of hospital attention soldier needs... used to calculate recovery time
	bool _recentlyPromoted, _psiTraining, _training, _returnToTrainingWhenHealed;
	Armor *_armor;
	Armor *_replacedArmor;
	Armor *_transformedArmor;
	std::vector<EquipmentLayoutItem*> _equipmentLayout;           // last used equipment layout, managed by the game
	std::vector<EquipmentLayoutItem*> _personalEquipmentLayout;   // personal  equipment layout, managed by the player
	const Armor* _personalEquipmentArmor;
	SoldierDeath *_death;
	SoldierDiary *_diary;
	std::string _statString;
	bool _corpseRecovered;
	std::map<std::string, int> _previousTransformations, _transformationBonuses;
	std::vector<const RuleSoldierBonus*> _bonusCache;
	ScriptValues<Soldier> _scriptValues;
public:
	/// Creates a new soldier.
	Soldier(RuleSoldier *rules, Armor *armor, int nationality, int id = 0);
	/// Cleans up the soldier.
	~Soldier();
	/// Loads the soldier from YAML.
	void load(const YAML::Node& node, const Mod *mod, SavedGame *save, const ScriptGlobal *shared, bool soldierTemplate = false);
	/// Saves the soldier to YAML.
	YAML::Node save(const ScriptGlobal *shared) const;
	/// Get the soldier corresponding YAML.
	YAML::Node asYaml() const;
	/// Gets the soldier's name.
	std::string getName(bool statstring = false, unsigned int maxLength = 20) const;
	/// Sets the soldier's name.
	void setName(const std::string &name);
	/// Generates a new name based on nationality.
	void genName();
	/// Gets the soldier's callsign.
	std::string getCallsign(unsigned int maxLength = 20) const;
	/// Sets the soldier's callsign.
	void setCallsign(const std::string &callsign);
	/// Check for callsign assignment.
	bool hasCallsign() const;
	/// Gets the soldier's nationality.
	int getNationality() const;
	/// Sets the soldier's nationality.
	void setNationality(int nationality);
	/// Gets the soldier's craft.
	Craft *getCraft() const;
	/// Sets the soldier's craft.
	void setCraft(Craft *craft, bool resetCustomDeployment = false);
	/// Sets the soldier's craft and automatically moves the equipment (if enabled).
	void setCraftAndMoveEquipment(Craft* craft, Base* base, bool isNewBattle, bool resetCustomDeployment = false);
	/// Gets the soldier's craft string.
	std::string getCraftString(Language *lang, const BaseSumDailyRecovery& recovery) const;
	/// Gets a string version of the soldier's rank.
	std::string getRankString() const;
	/// Gets a sprite version of the soldier's rank. Used for BASEBITS.PCK.
	int getRankSprite() const;
	/// Gets a sprite version of the soldier's rank. Used for SMOKE.PCK.
	int getRankSpriteBattlescape() const;
	/// Gets a sprite version of the soldier's rank. Used for TinyRanks.
	int getRankSpriteTiny() const;
	/// Gets the soldier's rank.
	SoldierRank getRank() const;
	/// Increase the soldier's military rank.
	void promoteRank();
	/// Promotes/demotes a soldier to a specific rank.
	void setRank(const SoldierRank newRank);
	/// Gets the soldier's missions.
	int getMissions() const;
	/// Gets the soldier's kills.
	int getKills() const;
	/// Gets the soldier's stuns.
	int getStuns() const;
	/// Gets the soldier's gender.
	SoldierGender getGender() const;
	/// Sets the soldier's gender.
	void setGender(SoldierGender gender);
	/// Gets the soldier's look.
	SoldierLook getLook() const;
	/// Sets the soldier's look.
	void setLook(SoldierLook look);
	/// Gets the soldier's look sub type.
	int getLookVariant() const;
	/// Sets the soldier's look sub type.
	void setLookVariant(int lookVariant);
	/// Gets soldier rules.
	RuleSoldier *getRules() const;
	/// Gets the soldier's unique ID.
	int getId() const;
	/// Add a mission to the counter.
	void addMissionCount();
	/// Add a kill to the counter.
	void addKillCount(int count);
	/// Add a stun to the counter.
	void addStunCount(int count);
	/// Get pointer to initial stats.
	const UnitStats* getInitStats() const;
	/// Get pointer to current stats.
	UnitStats *getCurrentStatsEditable();
	const UnitStats* getCurrentStats() const;
	/// Set initial and current stats.
	void setBothStats(UnitStats *stats);
	/// Get whether the unit was recently promoted.
	bool isPromoted();
	/// Gets the soldier armor.
	Armor *getArmor() const;
	/// Sets the soldier armor.
	void setArmor(Armor *armor, bool resetCustomDeployment = false);
	/// Gets the armor layers (sprite names).
	const std::vector<std::string>& getArmorLayers(Armor *customArmor = nullptr) const;
	/// Gets the soldier's original armor (before replacement).
	Armor *getReplacedArmor() const;
	/// Backs up the soldier's original armor (before replacement).
	void setReplacedArmor(Armor *armor);
	/// Gets the soldier's original armor (before transformation).
	Armor *getTransformedArmor() const;
	/// Backs up the soldier's original armor (before transformation).
	void setTransformedArmor(Armor *armor);


	/// Is the soldier wounded or not?.
	bool isWounded() const;
	/// Is the soldier wounded or not?.
	bool hasFullHealth() const;
	/// Is the soldier capable of defending a base?.
	bool canDefendBase() const;

	/// Gets the amount of missing mana.
	int getManaMissing() const;
	/// Sets the amount of missing mana.
	void setManaMissing(int manaMissing);
	/// Gets the soldier's mana recovery time.
	int getManaRecovery(int manaRecoveryPerDay) const;

	/// Gets the amount of missing health.
	int getHealthMissing() const;
	/// Sets the amount of missing health.
	void setHealthMissing(int healthMissing);
	/// Gets the soldier's health recovery time.
	int getHealthRecovery(int healthRecoveryPerDay) const;

	/// Gets the soldier's wound recovery time.
	int getWoundRecoveryInt() const;
	/// Sets the soldier's wound recovery time.
	void setWoundRecovery(int recovery);
	/// Gets ther soldier's wound recovery.
	int getWoundRecovery(float absBonus, float relBonus) const;

	/// Heals wound recoveries.
	void healWound(float absBonus, float relBonus);
	/// Replenishes mana.
	void replenishMana(int manaRecoveryPerDay);
	/// Replenishes health.
	void replenishHealth(int healthRecoveryPerDay);

	/// Daily stat replenish and healing of the soldier based on the facilities available in the base.
	void replenishStats(const BaseSumDailyRecovery& recovery);
	/// Gets number of days until the soldier is ready for action again.
	int getNeededRecoveryTime(const BaseSumDailyRecovery& recovery) const;


	/// Gets the soldier's equipment-layout.
	std::vector<EquipmentLayoutItem*> *getEquipmentLayout();
	std::vector<EquipmentLayoutItem*> *getPersonalEquipmentLayout() { return &_personalEquipmentLayout; }
	/// Gets the soldier's personal equipment armor.
	const Armor* getPersonalEquipmentArmor() const { return _personalEquipmentArmor; }
	/// Sets the soldier's personal equipment armor.
	void setPersonalEquipmentArmor(const Armor* armor) { _personalEquipmentArmor = armor; }

	/// Trains a soldier's psychic stats
	void trainPsi();
	/// Trains a soldier's psionic abilities (anytimePsiTraining option).
	void trainPsi1Day();
	/// Is the soldier already fully psi-trained?
	bool isFullyPsiTrained();
	/// Returns whether the unit is in psi training or not
	bool isInPsiTraining() const;
	/// set the psi training status
	void setPsiTraining(bool psi);
	/// returns this soldier's psionic skill improvement score for this month.
	int getImprovement() const;
	/// returns this soldier's psionic strength improvement score for this month.
	int getPsiStrImprovement() const;
	/// Gets the soldier death info.
	SoldierDeath *getDeath() const;
	/// Kills the soldier.
	void die(SoldierDeath *death);
	/// Clears the equipment layout.
	void clearEquipmentLayout();
	/// Gets the soldier's diary.
	SoldierDiary *getDiary();
	const SoldierDiary* getDiary() const;
	/// Resets the soldier's diary.
	void resetDiary();
	/// Calculate statString.
	void calcStatString(const std::vector<StatString *> &statStrings, bool psiStrengthEval);
	/// Trains a soldier's physical stats
	void trainPhys(int customTrainingFactor);
	/// Is the soldier already fully trained?
	bool isFullyTrained() const;
	/// Returns whether the unit is in training or not
	bool isInTraining() const;
	/// set the training status
	void setTraining(bool training);
	/// Should the soldier return to martial training automatically when fully healed?
	bool getReturnToTrainingWhenHealed() const;
	/// Sets whether the soldier should return to martial training automatically when fully healed.
	void setReturnToTrainingWhenHealed(bool returnToTrainingWhenHealed);
	/// Sets whether the soldier's body was recovered from a battle
	void setCorpseRecovered(bool corpseRecovered);
	/// Gets the previous transformations performed on this soldier
	std::map<std::string, int> &getPreviousTransformations();
	/// Returns whether the unit is eligible for a certain transformation
	bool isEligibleForTransformation(const RuleSoldierTransformation *transformationRule) const;
	/// Performs a transformation on this soldier
	void transform(const Mod *mod, RuleSoldierTransformation *transformationRule, Soldier *sourceSoldier, Base *base);
	/// Calculates how this project changes the soldier's stats
	UnitStats calculateStatChanges(const Mod *mod, RuleSoldierTransformation *transformationRule, Soldier *sourceSoldier, int mode, const RuleSoldier *sourceSoldierType);
	/// Gets all the soldier bonuses
	const std::vector<const RuleSoldierBonus*> *getBonuses(const Mod *mod);
	/// Get pointer to current stats with soldier bonuses, but without armor bonuses.
	const UnitStats *getStatsWithSoldierBonusesOnly() const;
	/// Get pointer to current stats with armor and soldier bonuses.
	const UnitStats *getStatsWithAllBonuses() const;
	/// Pre-calculates soldier stats with various bonuses.
	bool prepareStatsWithBonuses(const Mod *mod);
	/// Gets a pointer to the daily dogfight experience cache.
	UnitStats* getDailyDogfightExperienceCache();
	/// Resets the daily dogfight experience cache.
	void resetDailyDogfightExperienceCache();

private:
	std::string generateCallsign(const std::vector<SoldierNamePool*> &names);
	/// Automatically move equipment between the craft and the base when assigning/deassigning/reassigning soldiers.
	void autoMoveEquipment(Craft* craft, Base* base, int toBase);

};

}
