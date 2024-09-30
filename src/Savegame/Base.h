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
#include "Target.h"
#include <string>
#include <vector>
#include <map>
#include <yaml-cpp/yaml.h>
#include "../Mod/RuleBaseFacilityFunctions.h"

#ifndef BASEFACILITIESITERATOR
#define BASEFACILITIESITERATOR std::vector<BaseFacility*>::iterator
#endif

namespace OpenXcom
{

class RuleCraft;
class Soldier;
class Craft;
class ItemContainer;
class Transfer;
class Language;
class Mod;
class SavedGame;
class RuleBaseFacility;
class BaseFacility;
class ResearchProject;
class Production;
class Vehicle;
class Ufo;
class AlienMission;

enum UfoDetection : int;
enum BasePlacementErrors : int
{
	/// 0: ok
	BPE_None = 0,
	/// 1: not connected to lift or on top of another facility (standard OXC behavior)
	BPE_NotConnected = 1,
	/// 2: trying to upgrade over existing facility, but it's in use
	BPE_Used = 2,
	/// 3: trying to upgrade over existing facility, but it's already being upgraded
	BPE_Upgrading = 3,
	/// 4: trying to upgrade over existing facility, but size/placement mismatch
	BPE_UpgradeSizeMismatch = 4,
	/// 5: trying to upgrade over existing facility, but ruleset of new facility requires a specific existing facility
	BPE_UpgradeRequireSpecific = 5,
	/// 6: trying to upgrade over existing facility, but ruleset disallows it
	BPE_UpgradeDisallowed = 6,
	/// 7: trying to upgrade over existing facility, but all buildings next to it are under construction and build queue is off
	BPE_Queue = 7,
	/// 8: trying to build a facility, which is forbidden by other existing facilities in the base
	BPE_ForbiddenByOther = 8,
	/// 9: trying to build a facility, which would forbid (i.e. would be in conflict with) other existing facilities in the base
	BPE_ForbiddenByThis = 9,
	/// 10: not enough storage space
	BPE_Used_Stores = 10,
	/// 11: not enough living quarters
	BPE_Used_Quarters = 11,
	/// 12: not enough laboratories
	BPE_Used_Laboratories = 12,
	/// 13: not enough workshops
	BPE_Used_Workshops = 13,
	/// 14: not enough hangars
	BPE_Used_Hangars = 14,
	/// 15: not enough psi labs
	BPE_Used_PsiLabs = 15,
	/// 16: not enough gyms
	BPE_Used_Gyms = 16,
	/// 17: not enough alien containment
	BPE_Used_AlienContainment = 17,
};

struct BaseSumDailyRecovery
{
	/// Amount of mana recovery or loss provided by the base.
	int ManaRecovery = 0;
	/// Amount of health recovery provided by the base.
	int HealthRecovery = 0;
	/// Sum of the amount of additional wounds healed in this base due to sick bay facilities (as percentage of max HP per soldier).
	float SickBayRelativeBonus = 0.0f;
	/// Sum of the amount of additional wounds healed in this base due to sick bay facilities (in absolute number).
	float SickBayAbsoluteBonus = 0.0f;
};

/**
 * Represents a player base on the globe.
 * Bases can contain facilities, personnel, crafts and equipment.
 */
class Base : public Target
{
private:
	static const int BASE_SIZE = 6;
	const Mod *_mod;
	std::vector<BaseFacility*> _facilities;
	std::vector<Soldier*> _soldiers;
	std::vector<Craft*> _crafts;
	std::vector<Transfer*> _transfers;
	ItemContainer *_items;
	int _scientists, _engineers;
	std::vector<ResearchProject *> _research;
	std::vector<Production *> _productions;
	bool _inBattlescape;
	bool _retaliationTarget;
	AlienMission* _retaliationMission;
	bool _fakeUnderwater;
	std::vector<Vehicle*> _vehicles;
	std::vector<Vehicle*> _vehiclesFromBase;
	std::vector<BaseFacility*> _defenses;
	std::map<const RuleBaseFacility*, int> _destroyedFacilitiesCache;
	RuleBaseFacilityFunctions _provideBaseFunc = 0;
	RuleBaseFacilityFunctions _forbiddenBaseFunc = 0;

	using Target::load;
public:
	/// Creates a new base.
	Base(const Mod *mod);
	/// Cleans up the base.
	~Base();
	/// Loads the base from YAML.
	void load(const YAML::Node& node, SavedGame *save, bool newGame, bool newBattleGame = false);
	/// Finishes loading the base (more specifically all craft in the base) from YAML.
	void finishLoading(const YAML::Node& node, SavedGame *save);
	void calculateServices(SavedGame* save);
	/// Tests whether the base facilities are within the base boundaries and not overlapping.
	bool isOverlappingOrOverflowing();
	/// Saves the base to YAML.
	YAML::Node save() const override;
	/// Gets the base's type.
	std::string getType() const override;
	/// Gets the base's name.
	std::string getName(Language *lang = 0) const override;
	/// Gets the base's marker sprite.
	int getMarker() const override;
	/// Gets the base's facilities.
	std::vector<BaseFacility*> *getFacilities();
	/// Gets the base's soldiers.
	std::vector<Soldier*> *getSoldiers();
	/// Pre-calculates soldier stats with various bonuses.
	void prepareSoldierStatsWithBonuses();
	/// Gets the base's crafts.
	std::vector<Craft*> *getCrafts() {	return &_crafts; }
	/// Gets the base's crafts.
	const std::vector<Craft*> *getCrafts() const { return &_crafts; }
	/// Gets the base's transfers.
	std::vector<Transfer*> *getTransfers() { return &_transfers; }
	/// Gets the base's transfers.
	const std::vector<Transfer*> *getTransfers() const { return &_transfers; }
	/// Gets the base's items.
	ItemContainer *getStorageItems() { return _items; }
	/// Gets the base's items.
	const ItemContainer *getStorageItems() const { return _items; }
	/// Gets the base's scientists.
	int getScientists() const;
	/// Sets the base's scientists.
	void setScientists(int scientists);
	/// Gets the base's engineers.
	int getEngineers() const;
	/// Sets the base's engineers.
	void setEngineers(int engineers);
	/// Checks if a target is detected by the base's radar.
	UfoDetection detect(const Ufo *target, const SavedGame *save, bool alreadyTracked) const;
	/// Gets the base's available soldiers.
	int getAvailableSoldiers(bool checkCombatReadiness = false, bool includeWounded = false) const;
	/// Gets the base's total soldiers.
	int getTotalSoldiers() const;
	/// Gets the base's available scientists.
	int getAvailableScientists() const;
	/// Gets the base's total scientists.
	int getTotalScientists() const;
	/// Gets the base's available engineers.
	int getAvailableEngineers() const;
	/// Gets the base's total engineers.
	int getTotalEngineers() const;
	/// Gets the base's total number and cost of other staff & inventory.
	int getTotalOtherStaffAndInventoryCost(int& staffCount, int& inventoryCount) const;
	/// Gets the base's used living quarters.
	int getUsedQuarters() const;
	/// Gets the base's available living quarters.
	int getAvailableQuarters() const;
	/// Gets the base's used storage space.
	double getUsedStores(bool excludeNormalItems = false) const;
	/// Checks if the base's stores are overfull.
	bool storesOverfull(double offset = 0.0) const;
	/// Checks if the base's stores are so full that even craft equipment and incoming transfers can't fit.
	bool storesOverfullCritical() const;
	/// Gets the base's available storage space.
	int getAvailableStores() const;
	/// Gets the base's used laboratory space.
	int getUsedLaboratories() const;
	/// Gets the base's available laboratory space.
	int getAvailableLaboratories() const;
	/// Gets the base's used workshop space.
	int getUsedWorkshops() const;
	/// Gets the base's available workshop space.
	int getAvailableWorkshops() const;
	/// Gets the base's used hangars.
	int getUsedHangars() const;
	/// Gets the base's available hangars.
	int getAvailableHangars() const;
	/// Get the number of available space lab (not used by a ResearchProject)
	int getFreeLaboratories() const;
	/// Get the number of available space lab (not used by a Production)
	int getFreeWorkshops() const;

	int getAllocatedScientists() const;

	int getAllocatedEngineers() const;
	/// Gets the base's defense value.
	int getDefenseValue() const;
	/// Gets the base's short range detection.
	int getShortRangeDetection() const;
	/// Gets the base's long range detection.
	int getLongRangeDetection() const;
	/// Gets the base's crafts of a certain type.
	int getCraftCount(const RuleCraft *craft) const;
	/// Gets the base's crafts of a certain type.
	int getCraftCountForProduction(const RuleCraft *craft) const;
	/// Gets the base's craft maintenance.
	int getCraftMaintenance() const;
	/// Gets the total count and total salary of soldiers of a certain type stored in the base.
	std::pair<int, int> getSoldierCountAndSalary(const std::string &soldier) const;
	/// Gets the base's personnel maintenance.
	int getPersonnelMaintenance() const;
	/// Gets the base's facility maintenance.
	int getFacilityMaintenance() const;
	/// Gets the base's total monthly maintenance.
	int getMonthlyMaintenace() const;
	/// Get the list of base's ResearchProject
	const std::vector<ResearchProject *> & getResearch() const;
	/// Add a new ResearchProject to the Base
	void addResearch(ResearchProject *);
	/// Remove a ResearchProject from the Base
	void removeResearch(ResearchProject *);
	/// Add a new Production to Base
	void addProduction (Production * p);
	/// Remove a Base Production's
	void removeProduction (Production * p);
	/// Get the list of Base Production's
	const std::vector<Production *> & getProductions() const;
	/// Gets the base's used psi lab space.
	int getUsedPsiLabs() const;
	/// Gets the base's total available psi lab space.
	int getAvailablePsiLabs() const;
	/// Gets the base's total free psi lab space.
	int getFreePsiLabs() const;
	/// Gets the base's used training space.
	int getUsedTraining() const;
	/// Gets the base's total available training space.
	int getAvailableTraining() const;
	/// Gets the base's total free training space.
	int getFreeTrainingSpace() const;
	/// Gets the amount of free Containment space.
	int getFreeContainment(int prisonType) const;
	/// Gets the total amount of Containment space.
	int getAvailableContainment(int prisonType) const;
	/// Gets the total amount of used Containment space.
	int getUsedContainment(int prisonType, bool onlyExternal = false) const;
	/// Sets the craft's battlescape status.
	void setInBattlescape(bool inbattle);
	/// Gets if the craft is in battlescape.
	bool isInBattlescape() const;
	/// Mark this base for alien retaliation.
	void setRetaliationTarget(bool mark = true);
	/// Gets the retaliation status of this base.
	bool getRetaliationTarget() const;
	/// Sets the corresponding alien retaliation mission.
	void setRetaliationMission(AlienMission* retaliationMission) { _retaliationMission = retaliationMission; }
	/// Gets the corresponding alien retaliation mission.
	AlienMission* getRetaliationMission() const { return _retaliationMission; }
	/// Mark/unmark this base as a fake underwater base.
	void setFakeUnderwater(bool fakeUnderwater) { _fakeUnderwater = fakeUnderwater; }
	/// Is this a fake underwater base?
	bool isFakeUnderwater() const { return _fakeUnderwater; }
	/// Get the detection chance for this base.
	size_t getDetectionChance() const;
	/// Gets how many Grav Shields the base has
	int getGravShields() const;
	/// Setup base defenses.
	void setupDefenses(AlienMission* am);
	/// Get a list of Defensive Facilities
	std::vector<BaseFacility*> *getDefenses();
	/// Gets the base's vehicles.
	std::vector<Vehicle*> *getVehicles();
	/// Gets the list of recently destroyed base facilities.
	std::map<const RuleBaseFacility*, int> *getDestroyedFacilitiesCache() { return &_destroyedFacilitiesCache; }
	/// Damage and/or destroy facilities after a missile impact.
	void damageFacilities(Ufo *ufo);
	/// Damage a given facility.
	int damageFacility(BaseFacility *toBeDamaged);
	/// Destroys all disconnected facilities in the base.
	void destroyDisconnectedFacilities();
	/// Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
	std::list<BASEFACILITIESITERATOR> getDisconnectedFacilities(BaseFacility *remove);
	/// destroy a facility and deal with the side effects.
	void destroyFacility(BASEFACILITIESITERATOR facility);
	void cleanupPrisons(int prisonType);
	/// Cleans up the defenses vector and optionally reclaims the tanks and their ammo.
	void cleanupDefenses(bool reclaimItems);

	/// Check if any facilities in a given area are used.
	BasePlacementErrors isAreaInUse(BaseAreaSubset area, const RuleBaseFacility* replacement = nullptr) const;
	/// Gets available base functionality.
	RuleBaseFacilityFunctions getProvidedBaseFunc(BaseAreaSubset skip) const;
	RuleBaseFacilityFunctions getInherentProvidedBaseFunc() const { return _provideBaseFunc; }
	/// Gets used base functionality.
	RuleBaseFacilityFunctions getRequireBaseFunc(BaseAreaSubset skip) const;
	/// Gets forbidden base functionality.
	RuleBaseFacilityFunctions getForbiddenBaseFunc(BaseAreaSubset skip) const;
	RuleBaseFacilityFunctions getInherentForbiddenBaseFunc() const { return _forbiddenBaseFunc; }
	/// Gets future base functionality.
	RuleBaseFacilityFunctions getFutureBaseFunc(BaseAreaSubset skip) const;
	RuleBaseFacilityFunctions getInherentFutureBaseFunc() const { return _provideBaseFunc; }
	/// Checks if it is possible to build another facility of a given type.
	bool isMaxAllowedLimitReached(RuleBaseFacility *rule) const;

	/// Gets the summary of all recovery rates provided by the base.
	BaseSumDailyRecovery getSumRecoveryPerDay() const;
	/// Removes a craft from the base.
	std::vector<Craft*>::iterator removeCraft(Craft *craft, bool unload);
};

}
