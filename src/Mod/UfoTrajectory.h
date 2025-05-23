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
#include <vector>
#include <string>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

/**
 * Information for points on a UFO trajectory.
 */
struct TrajectoryWaypoint
{
	/// The mission zone.
	size_t zone;
	/// The altitude to reach.
	size_t altitude;
	/// The speed percentage ([0..100])
	size_t speed;
};

/**
 * Holds information about a specific trajectory.
 * Ufo trajectories are a sequence of mission zones, altitudes and speed percentages.
 */
class UfoTrajectory
{
public:
	static const std::string RETALIATION_ASSAULT_RUN;

	UfoTrajectory(const std::string &id);
	/**
	 * Gets the trajectory's ID.
	 * @return The trajectory's ID.
	 */
	const std::string &getID() const { return _id; }

	/// Loads trajectory data from YAML.
	void load(const YAML::YamlNodeReader& reader);

	/**
	 * Gets the number of waypoints in this trajectory.
	 * @return The number of waypoints.
	 */
	size_t getWaypointCount() const { return _waypoints.size(); }

	/**
	 * Gets the zone index at a waypoint.
	 * @param wp The waypoint.
	 * @return The zone index.
	 */
	size_t getZone(size_t wp) const { return _waypoints[wp].zone; }

	/// Gets the altitude at a waypoint.
	std::string getAltitude(size_t wp) const;

	/**
	 * Applies the speed percentage at a waypoint to the base speed.
	 * @param wp The waypoint.
	 * @param baseSpeed The base speed.
	 * @return The new speed after applying the percentage.
	 */
	int applySpeedPercentage(size_t wp, int baseSpeed) const { return baseSpeed * _waypoints[wp].speed / 100; }

	/**
	 * Gets the number of seconds UFOs should spend on the ground.
	 * @return The number of seconds.
	 */
	size_t groundTimer() const { return _groundTimer; }
private:
	std::string _id;
	size_t _groundTimer;
	std::vector<TrajectoryWaypoint> _waypoints;
};

// helper overloads for (de)serialization
bool read(ryml::ConstNodeRef const& n, TrajectoryWaypoint* val);

}
