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
#include <yaml-cpp/yaml.h>
#include "../Savegame/WeightedOptions.h"

enum AlienRank{AR_HUMAN = -1, AR_COMMANDER, AR_LEADER, AR_ENGINEER, AR_MEDIC, AR_NAVIGATOR, AR_SOLDIER, AR_TERRORIST, AR_TERRORIST2};

namespace OpenXcom
{

class Mod;

/**
 * Represents a specific race "family", or a "main race" if you wish.
 * Here is defined which ranks it contains and also which accompanying terror units.
 */
class AlienRace
{
private:
	std::string _id;
	std::string _baseCustomDeploy;
	std::string _baseCustomMission;
	std::vector<std::pair<size_t, WeightedOptions*> > _retaliationMissionDistribution;
	std::vector<std::string> _members;
	std::vector< std::vector<std::string> > _membersRandom;
	int _retaliationAggression;
	int _listOrder;
public:
	/// Creates a blank alien race ruleset.
	AlienRace(const std::string &id, int listOrder);
	/// Cleans up the alien race ruleset.
	~AlienRace();
	/// Loads alien race data from YAML.
	void load(const YAML::Node& node, const Mod* mod);
	/// Gets the alien race's id.
	const std::string &getId() const;
	/// Gets the alien base weapon deploy.
	const std::string &getBaseCustomDeploy() const;
	/// Gets the alien base mission.
	const std::string &getBaseCustomMission() const;
	/// Gets a certain member of this alien race family.
	const std::string &getMember(int id) const;
	/// Gets the total number of members of this alien race family.
	int getMembers() const;
	/// Gets how aggressive alien are to spawn retaliation mission after destroying ufo.
	int getRetaliationAggression() const;
	/// Returns a list of retaliation missions based on the given month.
	WeightedOptions* retaliationMissionWeights(const size_t monthsPassed) const;
	/// Get the list weight for this alien race.
	int getListOrder() const { return _listOrder; }
};

}
