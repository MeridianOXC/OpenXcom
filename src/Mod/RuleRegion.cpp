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
#include "RuleRegion.h"
#include "City.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of region.
 * @param type String defining the type.
 */
RuleRegion::RuleRegion(const std::string &type): _type(type), _cost(0), _regionWeight(0)
{
}

/**
 * Deletes the cities from memory.
 */
RuleRegion::~RuleRegion()
{
	for (auto* city : _cities)
	{
		delete city;
	}
}

/**
 * Loads the region type from a YAML file.
 * @param node YAML node.
 */
void RuleRegion::load(const YAML::Node &node)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent);
	}

	_cost = node["cost"].as<int>(_cost);

	if (node["deleteOldAreas"].as<bool>(false))
	{
		_lonMin.clear();
		_lonMax.clear();
		_latMin.clear();
		_latMax.clear();
	}
	if (auto& areaNode = node["areas"])
	{
		for (const auto& area : areaNode.as<std::vector<std::array<double, 4>>>())
		{
			_lonMin.push_back(Deg2Rad(area[0]));
			_lonMax.push_back(Deg2Rad(area[1]));
			_latMin.push_back(Deg2Rad(area[2]));
			_latMax.push_back(Deg2Rad(area[3]));

			if (_latMin.back() > _latMax.back())
				std::swap(_latMin.back(), _latMax.back());
		}
	}

	_missionZones = node["missionZones"].as< std::vector<MissionZone> >(_missionZones);
	{
		int zn = 0;
		for (auto& z : _missionZones)
		{
			if (z.areas.size() < 1)
			{
				Log(LOG_WARNING) << "Empty zone, region: " << _type << ", zone: " << zn;
				continue;
			}
			int an = 0;
			bool firstAreaType = z.areas.at(0).isPoint();
			for (auto& a : z.areas)
			{
				if (a.isPoint() != firstAreaType)
				{
					Log(LOG_WARNING) << "Mixed area types (point vs non-point), region: " << _type << ", zone: " << zn << ", area: " << an;
				}
				if (a.lonMin > a.lonMax)
				{
					Log(LOG_ERROR) << "Crossing the prime meridian in mission zones requires a different syntax, region: " << _type << ", zone: " << zn << ", area: " << an << ", lonMin: " << Rad2Deg(a.lonMin) << ", lonMax: " << Rad2Deg(a.lonMax);
					Log(LOG_INFO) << "  Wrong example: [350,   8, 20, 30]";
					Log(LOG_INFO) << "Correct example: [350, 368, 20, 30]";
				}
				++an;
			}
			++zn;
		}
	}
	if (const YAML::Node &weights = node["missionWeights"])
	{
		_missionWeights.load(weights);
	}
	_regionWeight = node["regionWeight"].as<size_t>(_regionWeight);
	_missionRegion = node["missionRegion"].as<std::string>(_missionRegion);
}

/**
 * Gets the language string that names
 * this region. Each region type
 * has a unique name.
 * @return The region type.
 */
const std::string& RuleRegion::getType() const
{
	return _type;
}

/**
 * Gets the cost of building a base inside this region.
 * @return The construction cost.
 */
int RuleRegion::getBaseCost() const
{
	return _cost;
}

/**
 * Checks if a point is inside this region.
 * @param lon Longitude in radians.
 * @param lat Latitude in radians.
 * @param ignoreTechnicalRegion If true, empty technical regions (i.e. regions with no areas, just having mission zones) will return true.
 * @return True if it's inside, false if it's outside.
 */
bool RuleRegion::insideRegion(double lon, double lat, bool ignoreTechnicalRegion) const
{
	if (ignoreTechnicalRegion && _lonMin.empty())
		return true;

	for (size_t i = 0; i < _lonMin.size(); ++i)
	{
		bool inLon, inLat;

		if (_lonMin[i] <= _lonMax[i])
			inLon = (lon >= _lonMin[i] && lon < _lonMax[i]);
		else
			inLon = ((lon >= _lonMin[i] && lon < M_PI*2.0) || (lon >= 0 && lon < _lonMax[i]));

		if (lat > 0) // make that both poles could be in some regions, this means `M_PI == _latMax[i]` or `-M_PI == _latMin[i]`
			inLat = (lat > _latMin[i] && lat <= _latMax[i]);
		else
			inLat = (lat >= _latMin[i] && lat < _latMax[i]);

		if (inLon && inLat)
			return true;
	}
	return false;
}

/**
 * Gets the list of cities contained in this region.
 * @return Pointer to a list.
 */
std::vector<City*> *RuleRegion::getCities()
{
	// Build a cached list of all mission zones that are cities
	// Saves us from constantly searching for them
	if (_cities.empty())
	{
		for (const auto& mz : _missionZones)
		{
			for (const auto& ma : mz.areas)
			{
				if (ma.isPoint() && !ma.name.empty())
				{
					_cities.push_back(new City(ma.name, ma.lonMin, ma.latMin));
				}
			}
		}
	}
	return &_cities;
}

/**
 * Gets the weight of this region for mission selection.
 * This is only used when creating a new game, since these weights change in the course of the game.
 * @return The initial weight of this region.
 */
size_t RuleRegion::getWeight() const
{
	return _regionWeight;
}

/**
 * Gets a list of all the missionZones in the region.
 * @return A list of missionZones.
 */
const std::vector<MissionZone> &RuleRegion::getMissionZones() const
{
	return _missionZones;
}

/**
 * Gets a random point that is guaranteed to be inside the given zone.
 * @param zone The target zone.
 * @return A pair of longitude and latitude.
 */
std::pair<double, double> RuleRegion::getRandomPoint(size_t zone, int area) const
{
	if (zone < _missionZones.size())
	{
		size_t a = area != -1 ? area : RNG::generate(0, _missionZones[zone].areas.size() - 1);
		double lonMin = _missionZones[zone].areas[a].lonMin;
		double lonMax = _missionZones[zone].areas[a].lonMax;
		double latMin = _missionZones[zone].areas[a].latMin;
		double latMax = _missionZones[zone].areas[a].latMax;
		if (lonMin > lonMax)
		{
			lonMin = _missionZones[zone].areas[a].lonMax;
			lonMax = _missionZones[zone].areas[a].lonMin;
		}
		if (latMin > latMax)
		{
			latMin = _missionZones[zone].areas[a].latMax;
			latMax = _missionZones[zone].areas[a].latMin;
		}
		double lon = RNG::generate(lonMin, lonMax);
		double lat = RNG::generate(latMin, latMax);
		return std::make_pair(lon, lat);
	}
	assert(0 && "Invalid zone number");
	return std::make_pair(0.0, 0.0);
}

}
