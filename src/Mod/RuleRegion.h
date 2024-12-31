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
#include "../fmath.h"
#include "../Savegame/WeightedOptions.h"
#include "RuleBaseFacilityFunctions.h"

namespace OpenXcom
{

/**
 * Defines a rectangle in polar coordinates.
 * It is used to define areas for a mission zone.
 */
struct MissionArea
{
	double lonMin, lonMax, latMin, latMax;
	int texture;
	std::string name;

	MissionArea() : lonMin(0.0), lonMax(360.0), latMin(-90.0), latMax(90.0), texture(0) { }

	bool operator== (const MissionArea& ma) const
	{
		return AreSame(lonMax, ma.lonMax) && AreSame(lonMin, ma.lonMin) && AreSame(latMax, ma.latMax) && AreSame(latMin, ma.latMin);
	}

	bool isPoint() const
	{
		return AreSame(lonMin, lonMax) && AreSame(latMin, latMax);
	}
};

/**
 * A zone (set of areas) on the globe.
 */
struct MissionZone
{
	std::vector<MissionArea> areas;

	void swap(MissionZone &other)
	{
		areas.swap(other.areas);
	}
};

class City;
class Mod;


/**
 * Represents a specific region of the world.
 * Contains constant info about a region like area
 * covered and base construction costs.
 */
class RuleRegion
{
private:
	std::string _type;
	int _cost;
	std::vector<double> _lonMin, _lonMax, _latMin, _latMax;
	std::vector<City*> _cities;
	/// Weighted list of the different mission types for this region.
	WeightedOptions _missionWeights;
	/// Weight of this region when selecting regions for alien missions.
	size_t _regionWeight;
	/// All the mission zones in this region.
	std::vector<MissionZone> _missionZones;
	/// Do missions in the region defined by this string instead.
	std::string _missionRegion;
	RuleBaseFacilityFunctions _provideBaseFunc = 0;
	RuleBaseFacilityFunctions _forbiddenBaseFunc = 0;
public:
	/// Creates a blank region ruleset.
	RuleRegion(const std::string &type);
	/// Cleans up the region ruleset.
	~RuleRegion();
	/// Loads the region from YAML.
	void load(const YAML::YamlNodeReader& reader, Mod* mod);
	/// Gets the region's type.
	const std::string& getType() const;
	/// Gets the region's base cost.
	int getBaseCost() const;
	/// Checks if a point is inside the region.
	bool insideRegion(double lon, double lat, bool ignoreTechnicalRegion = false) const;
	/// Gets the cities in this region.
	std::vector<City*> *getCities();
	/// Gets the weight of this region for mission selection.
	size_t getWeight() const;
	/// Gets the weighted list of missions for this region.
	const WeightedOptions &getAvailableMissions() const { return _missionWeights; }
	/// Gets the substitute mission region.
	const std::string &getMissionRegion() const { return _missionRegion; }
	/// Gets a random point inside a mission zone.
	std::pair<double, double> getRandomPoint(size_t zone, int area = -1) const;
	/// Gets the maximum longitude.
	const std::vector<double> &getLonMax() const { return _lonMax; }
	/// Gets the minimum longitude.
	const std::vector<double> &getLonMin() const { return _lonMin; }
	/// Gets the maximum latitude.
	const std::vector<double> &getLatMax() const { return _latMax; }
	/// Gets the minimum latitude.
	const std::vector<double> &getLatMin() const { return _latMin; }
	/// Gets a list of MissionZones.
	const std::vector<MissionZone> &getMissionZones() const;
	/// Gets the functions provided by the region.
	RuleBaseFacilityFunctions getProvidedBaseFunc() const { return _provideBaseFunc; }
	/// Gets the functions forbidden by the region.
	RuleBaseFacilityFunctions getForbiddenBaseFunc() const { return _forbiddenBaseFunc; }
};

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, MissionArea* val);
bool read(ryml::ConstNodeRef const& n, MissionZone* val);

}
