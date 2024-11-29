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
#include "AlienRace.h"
#include "../Engine/Exception.h"
#include "../Engine/RNG.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank alien race.
 * @param id String defining the id.
 */
AlienRace::AlienRace(const std::string &id, int listOrder) : _id(id), _retaliationAggression(0), _listOrder(listOrder)
{
}

AlienRace::~AlienRace()
{
	for (auto& pair : _retaliationMissionDistribution)
	{
		delete pair.second;
	}
}

/**
 * Loads the alien race from a YAML file.
 * @param node YAML node.
 */
void AlienRace::load(const YAML::YamlNodeReader& reader, const Mod* mod)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	reader.tryRead("baseCustomDeploy", _baseCustomDeploy);
	reader.tryRead("baseCustomMission", _baseCustomMission);
	reader.tryRead("members", _members);
	reader.tryRead("membersRandom", _membersRandom);
	reader.tryRead("retaliationAggression", _retaliationAggression);

	for (const auto& weights : reader["retaliationMissionWeights"].children())
	{
		WeightedOptions* nw = new WeightedOptions();
		nw->load(weights);
		_retaliationMissionDistribution.push_back(std::make_pair(weights.readKey<size_t>(0), nw));
	}
	reader.tryRead("listOrder", _listOrder);
}

/**
 * Returns the language string that names
 * this alien race. Each race has a unique name.
 * @return Race name.
 */
const std::string &AlienRace::getId() const
{
	return _id;
}

/**
 * Returns optional weapon deploy for aliens in they base.
 * @return Alien deployment id.
 */
const std::string &AlienRace::getBaseCustomDeploy() const
{
	return _baseCustomDeploy;
}

/**
 * Returns custom alien base deploy.
 * @return Alien deployment id.
 */
const std::string &AlienRace::getBaseCustomMission() const
{
	return _baseCustomMission;
}
/**
 * Gets a certain member of this alien race family.
 * @param id The member's id.
 * @return The member's name.
 */
const std::string &AlienRace::getMember(int id) const
{
	if (!_membersRandom.empty())
	{
		if ((size_t)id >= _membersRandom.size())
		{
			throw Exception("Race " + _id + " does not have a random member at position/rank " + std::to_string(id));
		}
		int rng = RNG::generate(0, _membersRandom[id].size() - 1);
		return _membersRandom[id][rng];
	}

	if ((size_t)id >= _members.size())
	{
		throw Exception("Race " + _id + " does not have a member at position/rank " + std::to_string(id));
	}
	return _members[id];
}

/**
 * Gets the total number of members of this alien race family.
 * @return The number of members.
 */
int AlienRace::getMembers() const
{
	if (!_membersRandom.empty())
	{
		return _membersRandom.size();
	}

	return _members.size();
}

/**
 * Gets how aggressive alien are
 * @return Mission ID or empty string.
 */
int AlienRace::getRetaliationAggression() const
{
	return _retaliationAggression;
}

/**
 * Returns a list of retaliation missions based on the given month.
 * @param monthsPassed The number of months that have passed in the game world.
 * @return The list of missions. Can be NULL.
 */
WeightedOptions* AlienRace::retaliationMissionWeights(const size_t monthsPassed) const
{
	if (_retaliationMissionDistribution.empty())
		return nullptr;

	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rw;
	rw = _retaliationMissionDistribution.rbegin();
	while (monthsPassed < rw->first)
		++rw;
	return rw->second;
}

}
