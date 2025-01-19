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
#include <vector>
#include "../Engine/Yaml.h"
#include "RuleBaseFacilityFunctions.h"
#include "ModScript.h"

namespace OpenXcom
{

class Mod;

/**
 * Represents one research project.
 * Dependency is the list of RuleResearchs which must be discovered before a RuleResearch became available.
 * Unlocks are used to immediately unlock a RuleResearch (even if not all the dependencies have been researched).
 *
 * Fake ResearchProjects: A RuleResearch is fake one, if its cost is 0. They are used to to create check points in the dependency tree.
 *
 * For example, if we have a Research E which needs either A & B or C & D, we create two fake research projects:
 *  - F which need A & B
 *  - G which need C & D
 * both F and G can unlock E.
 */
class RuleResearch
{
 private:
	std::string _name, _lookup, _cutscene, _spawnedItem, _spawnedEvent;
	int _spawnedItemCount;
	std::vector<std::string> _spawnedItemList;
	std::vector<std::string> _decreaseCounter, _increaseCounter;
	int _cost, _points;
	std::vector<std::string> _dependenciesName, _unlocksName, _disablesName, _reenablesName, _getOneFreeName, _requiresName;
	RuleBaseFacilityFunctions _requiresBaseFunc;
	std::vector<const RuleResearch*> _dependencies, _unlocks, _disables, _reenables, _getOneFree, _requires;
	bool _sequentialGetOneFree;
	std::vector<std::pair<std::string, std::vector<std::string> > > _getOneFreeProtectedName;
	std::vector<std::pair<const RuleResearch*, std::vector<const RuleResearch*> > > _getOneFreeProtected;
	std::string _neededItemName;
	const RuleItem* _neededItem = nullptr;
	bool _needItem, _destroyItem, _unlockFinalMission;
	bool _repeatable;
	int _listOrder;

	ScriptValues<RuleResearch> _scriptValues;
public:
	/// Name of class used in script.
	static constexpr const char* ScriptName = "RuleResearch";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	static const int RESEARCH_STATUS_NEW = 0;
	static const int RESEARCH_STATUS_NORMAL = 1;
	static const int RESEARCH_STATUS_DISABLED = 2;
	static const int RESEARCH_STATUS_HIDDEN = 3;
	RuleResearch(const std::string &name, int listOrder);

	/// Loads the research from YAML.
	void load(const YAML::YamlNodeReader& reader, Mod* mod, const ModScript& parsers);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);

	/// Gets time needed to discover this ResearchProject.
	int getCost() const;
	/// Gets the research name.
	const std::string &getName() const;
	/// Gets the research dependencies.
	const std::vector<const RuleResearch*> &getDependencies() const;
	/// Checks if this ResearchProject gives free topics in sequential order (or random order).
	bool sequentialGetOneFree() const;
	/// Gets the Item needed for this research.
	const RuleItem* getNeededItem() const { return _neededItem; }
	/// Checks if this ResearchProject needs a corresponding Item to be researched.
	bool needItem() const;
	/// Checks if this ResearchProject consumes the corresponding Item when research completes.
	bool destroyItem() const;
	/// Check if this ResearchProject is unlocking final mission, it can be only one!
	bool unlockFinalMission() const { return _unlockFinalMission; }
	/// Check if this ResearchProject is repeatable, i.e. is never marked as discovered.
	bool isRepeatable() const { return _repeatable; }
	/// Gets the list of ResearchProjects unlocked by this research.
	const std::vector<const RuleResearch*> &getUnlocked() const;
	/// Gets the list of ResearchProjects disabled by this research.
	const std::vector<const RuleResearch*> &getDisabled() const;
	/// Gets the list of ResearchProjects reenabled by this research.
	const std::vector<const RuleResearch*> &getReenabled() const;
	/// Gets the points earned for discovering this ResearchProject.
	int getPoints() const;
	/// Gets the list of ResearchProjects granted at random for free by this research.
	const std::vector<const RuleResearch*> &getGetOneFree() const;
	/// Gets the list(s) of ResearchProjects granted at random for free by this research (if a defined prerequisite is met).
	const std::vector<std::pair<const RuleResearch*, std::vector<const RuleResearch*> > > &getGetOneFreeProtected() const;
	/// Gets what to look up in the ufopedia.
	const std::string &getLookup() const;
	/// Gets the requirements for this ResearchProject.
	const std::vector<const RuleResearch*> &getRequirements() const;
	/// Gets the base requirements for this ResearchProject.
	RuleBaseFacilityFunctions getRequireBaseFunc() const { return _requiresBaseFunc; }
	/// Gets the list weight for this research item.
	int getListOrder() const;
	/// Gets the cutscene to play when this item is researched
	const std::string & getCutscene() const;
	/// Gets the item to spawn in the base stores when this topic is researched.
	const std::string & getSpawnedItem() const;
	/// Gets the number of items to spawn in the base stores when this topic is researched.
	int getSpawnedItemCount() const { return _spawnedItemCount; }
	/// Gets the list of items to spawn in the base stores when this topic is researched.
	const std::vector<std::string>& getSpawnedItemList() const { return _spawnedItemList; }
	/// Gets the geoscape event to spawn when this topic is researched.
	const std::string& getSpawnedEvent() const { return _spawnedEvent; }
	/// Gets the name of custom counter variables to decrease when this topic is researched.
	const std::vector<std::string>& getDecreaseCounter() const { return _decreaseCounter; }
	/// Gets the name of custom counter variables to increase when this topic is researched.
	const std::vector<std::string>& getIncreaseCounter() const { return _increaseCounter; }
};

}
