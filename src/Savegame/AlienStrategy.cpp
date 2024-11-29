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
#include <assert.h>
#include "AlienStrategy.h"
#include "WeightedOptions.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleRegion.h"
#include "../Engine/Logger.h"

namespace OpenXcom
{

/**
 * Create an AlienStrategy with no values.
 * Running a game like this will most likely crash.
 */
AlienStrategy::AlienStrategy()
{
	// Empty by design!
}

/**
 * Free all resources used by this AlienStrategy.
 */
AlienStrategy::~AlienStrategy()
{
	// Free allocated memory.
	for (auto& pair : _regionMissions)
	{
		delete pair.second;
	}
}

/**
 * Get starting values from the rules.
 * @param mod Pointer to the game mod.
 */
void AlienStrategy::init(const Mod *mod)
{
	for (const auto& regionName : mod->getRegionsList())
	{
		RuleRegion *region = mod->getRegion(regionName, true);
		_regionChances.set(regionName, region->getWeight());
		WeightedOptions *missions = new WeightedOptions(region->getAvailableMissions());
		_regionMissions.insert(std::make_pair(regionName, missions));
	}
}

/**
 * Loads the data from a YAML file.
 * @param node YAML node.
 */
void AlienStrategy::load(const YAML::YamlNodeReader& reader, const Mod* mod)
{
	// Free allocated memory.
	for (auto& pair : _regionMissions)
	{
		delete pair.second;
	}
	_regionMissions.clear();
	_regionChances.clear();
	_regionChances.load(reader["regions"]);

	for (const auto& possibleMissionReader : reader["possibleMissions"].children())
	{
		std::string region = possibleMissionReader["region"].readVal<std::string>();
		if (mod->getRegion(region, false))
		{
			const auto& missionOptionsReader = possibleMissionReader["missions"];
			WeightedOptions* options = new WeightedOptions();
			options->load(missionOptionsReader);
			_regionMissions.insert(std::make_pair(region, options));
		}
		else
		{
			Log(LOG_WARNING) << "Corrupted save: Alien strategy contains an invalid region: " << region << ", skipping...";
		}
	}
	reader.tryRead("missionLocations", _missionLocations);
	reader.tryRead("missionsRun", _missionRuns);
}

/**
 * Saves the alien data to a YAML file.
 * @return YAML node.
 */
void AlienStrategy::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	_regionChances.save(writer["regions"]);
	if (!_regionMissions.empty())
	{
		auto possibleMissionsWriter = writer["possibleMissions"];
		possibleMissionsWriter.setAsSeq();
		for (auto& pair : _regionMissions)
		{
			auto regionWriter = possibleMissionsWriter.write();
			regionWriter.setAsMap();
			regionWriter.write("region", pair.first);
			pair.second->save(regionWriter["missions"]);
		}
	}
	writer.write("missionLocations", _missionLocations);
	writer.write("missionsRun", _missionRuns);
}

/**
 * Choose one of the regions for a mission.
 * @param mod Pointer to the mod.
 * @return The region id.
 */
std::string AlienStrategy::chooseRandomRegion(const Mod *mod)
{
	std::string chosen = _regionChances.choose();
	if (chosen.empty())
	{
		// no more missions to choose from: refresh.
		// First, free allocated memory.
		for (auto& pair : _regionMissions)
		{
			delete pair.second;
		}
		_regionMissions.clear();
		// re-initialize the list
		init(mod);
		// now let's try that again.
		chosen = _regionChances.choose();
	}
	assert ("" != chosen);
	return chosen;
}

/**
 * Choose one missions available for @a region.
 * @param region The region id.
 * @return The mission id.
 */
std::string AlienStrategy::chooseRandomMission(const std::string &region) const
{
	auto found = _regionMissions.find(region);
	assert(found != _regionMissions.end());
	return found->second->choose();
}

/**
 * Remove @a mission from the list of possible missions for @a region.
 * @param region The region id.
 * @param mission The mission id.
 * @return If there are no more regions with missions available.
 */
bool AlienStrategy::removeMission(const std::string &region, const std::string &mission)
{
	auto found = _regionMissions.find(region);
	if (found != _regionMissions.end())
	{
		found->second->set(mission, 0);
		if (found->second->empty())
		{
			_regionMissions.erase(found);
			_regionChances.set(region, 0);
		}
	}
	return _regionMissions.empty();
}


/**
 * Checks the number of missions run labelled as "varName".
 * @return the number of missions run under the variable name.
 */
int AlienStrategy::getMissionsRun(const std::string &varName)
{
	if (_missionRuns.find(varName) != _missionRuns.end())
		return _missionRuns[varName];
	return 0;
}

/**
 * Increments the number of missions run labelled as "varName".
 * @param varName the variable name that we want to use to keep track of this.
 * @param increment the value to increment by.
 */
void AlienStrategy::addMissionRun(const std::string &varName, int increment)
{
	if (varName.empty())
		return;
	_missionRuns[varName] += increment;
}

/**
 * Adds a mission location to our storage array.
 * @param varName the name on the variable under which to store this info.
 * @param regionName the name of the region we're using.
 * @param zoneNumber the number of the zone within that region we're using.
 * @param maximum the maximum size of the list we want to maintain.
 */
void AlienStrategy::addMissionLocation(const std::string &varName, const std::string &regionName, int zoneNumber, int maximum)
{
	if (maximum <= 0) return;
	_missionLocations[varName].push_back(std::make_pair(regionName, zoneNumber));
	if (_missionLocations[varName].size() > (size_t)(maximum))
	{
		_missionLocations.erase(_missionLocations.begin());
	}
}

/**
 * Checks that a given mission location (city or whatever) isn't stored in our list of previously attacked locations.
 * @param varName the name of the variable that is storing our data.
 * @param regionName the name of the region we're looking for.
 * @param zoneNumber the number in the region that we want to check.
 * @return if the region is valid (meaning it is not in our table).
 */
bool AlienStrategy::validMissionLocation(const std::string &varName, const std::string &regionName, int zoneNumber)
{
	if (_missionLocations.find(varName) != _missionLocations.end())
	{
		for (const auto& pair : _missionLocations[varName])
		{
			if (pair.first == regionName && pair.second == zoneNumber)
				return false;
		}
	}
	return true;
}

/**
 * Checks that a given region appears in our strategy table.
 * @param region the region we want to check for validity.
 * @return if the region appears in the table or not.
 */
bool AlienStrategy::validMissionRegion(const std::string &region)
{
	auto search = _regionMissions.find(region);
	return (search != _regionMissions.end());
}

}
