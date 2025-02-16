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
#include "UfoTrajectory.h"
#include "../Savegame/Ufo.h"

namespace OpenXcom
{

const std::string UfoTrajectory::RETALIATION_ASSAULT_RUN = "__RETALIATION_ASSAULT_RUN";

UfoTrajectory::UfoTrajectory(const std::string &id) : _id(id), _groundTimer(5)
{
}

/**
 * Overwrites trajectory data with the data stored in @a node.
 * Only the fields contained in the node will be overwritten.
 * @param node The node containing the new values.
 */
void UfoTrajectory::load(const YAML::YamlNodeReader& reader)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("groundTimer", _groundTimer);
	reader.tryRead("waypoints", _waypoints);
}

/**
 * Gets the altitude at a waypoint.
 * @param wp The waypoint.
 * @return The altitude.
 */
std::string UfoTrajectory::getAltitude(size_t wp) const
{
	return Ufo::ALTITUDE_STRING[_waypoints[wp].altitude];
}

// helper overloads for (de)serialization
bool read(ryml::ConstNodeRef const& n, TrajectoryWaypoint* val)
{
	YAML::YamlNodeReader reader(n);
	val->zone = reader[0].readVal<int>();
	val->altitude = reader[1].readVal<int>();
	val->speed = reader[2].readVal<int>();
	return true;
}

}
