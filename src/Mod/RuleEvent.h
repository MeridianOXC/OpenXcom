#pragma once
/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include <map>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "../Savegame/WeightedOptions.h"

namespace OpenXcom
{

/**
 * Represents a custom Geoscape event.
 * Events are spawned using Event Script ruleset.
 */
class RuleEvent
{
private:
	std::string _name, _description, _background, _music, _cutscene;
	std::vector<std::string> _regionList;
	bool _alignBottom;
	bool _city;
	int _points, _funds;
	std::string _spawnedCraftType;
	int _spawnedPersons;
	std::string _spawnedPersonType, _spawnedPersonName;
	YAML::Node _spawnedSoldier;
	std::map<std::string, int> _everyMultiItemList;
	std::vector<std::string> _everyItemList, _randomItemList;
	std::vector<std::map<std::string, int> > _randomMultiItemList;
	WeightedOptions _weightedItemList;
	std::vector<std::string> _researchList;
	std::string _interruptResearch;
	int _timer, _timerRandom;
	bool _invert;
public:
	/// Creates a blank RuleEvent.
	RuleEvent(const std::string &name);
	/// Cleans up the event ruleset.
	~RuleEvent() = default;
	/// Loads the event definition from YAML.
	void load(const YAML::Node &node);
	/// Gets the event's name.
	const std::string &getName() const { return _name; }
	/// Gets the event's description.
	const std::string &getDescription() const { return _description; }
	/// Should the description be bottom aligned?
	bool alignBottom() const { return _alignBottom; }
	/// Gets the event's background sprite name.
	const std::string &getBackground() const { return _background; }
	/// Gets the event's music.
	const std::string &getMusic() const { return _music; }
	/// Gets the cutscene to play when the event dialog is closed.
	const std::string &getCutscene() const { return _cutscene; }
	/// Gets a list of regions where this event can occur.
	const std::vector<std::string> &getRegionList() const { return _regionList; }
	/// Is this event city specific?
	bool isCitySpecific() const { return _city; }
	/// Gets the amount of score points awarded when this event pops up.
	int getPoints() const { return _points; }
	/// Gets the amount of funds awarded when this event pops up.
	int getFunds() const { return _funds; }

	/// Gets the spawned craft type.
	const std::string& getSpawnedCraftType() const { return _spawnedCraftType; }

	/// Gets the number of spawned persons.
	int getSpawnedPersons() const { return _spawnedPersons; }
	/// Gets the spawned person type.
	const std::string& getSpawnedPersonType() const { return _spawnedPersonType; }
	/// Gets the custom name of the spawned person.
	const std::string& getSpawnedPersonName() const { return _spawnedPersonName; }
	/// Gets the spawned soldier template.
	const YAML::Node& getSpawnedSoldierTemplate() const { return _spawnedSoldier; }

	/// Gets a list of items; they are all transferred to HQ stores when this event pops up.
	const std::map<std::string, int> &getEveryMultiItemList() const { return _everyMultiItemList; }
	/// Gets a list of items; they are all transferred to HQ stores when this event pops up.
	const std::vector<std::string> &getEveryItemList() const { return _everyItemList; }
	/// Gets a list of items; one of them is randomly selected and transferred to HQ stores when this event pops up.
	const std::vector<std::string> &getRandomItemList() const { return _randomItemList; }
	/// Gets a list of lists of items; one of them is randomly selected and transferred to HQ stores when this event pops up.
	const std::vector<std::map<std::string, int> > &getRandomMultiItemList() const { return _randomMultiItemList; }
	/// Gets a list of items; one of them is randomly selected (considering weights) and transferred to HQ stores when this event pops up.
	const WeightedOptions &getWeightedItemList() const { return _weightedItemList; }
	/// Gets a list of research projects; one of them will be randomly discovered when this event pops up.
	const std::vector<std::string> &getResearchList() const { return _researchList; }
	/// Gets the research project that will interrupt/terminate an already generated (but not yet popped up) event.
	const std::string &getInterruptResearch() const { return _interruptResearch; }
	/// Gets the timer of delay for this event, for it occurring after being spawned with eventScripts ruleset.
	int getTimer() const { return _timer; }
	/// Gets value for calculation of random part of delay for this event.
	int getTimerRandom() const { return _timerRandom; }
	/// Should the event remove items instead of adding them?
	bool getInvert() const { return _invert; }
};

}
