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
#include "Craft.h"
#include "Ufo.h"
#include "SerializationHelper.h"
#include "../fmath.h"
#include "../Engine/Language.h"

namespace OpenXcom
{

/**
 * Initializes a target with blank coordinates.
 */
Target::Target() : _lon(0.0), _lat(0.0), _id(0)
{
}

/**
 * Make sure no crafts are chasing this target.
 */
Target::~Target()
{
	for (auto* follower : getCraftFollowers())
	{
		follower->returnToBase();
	}
	for (auto* ufoFollower : getUfoFollowers())
	{
		ufoFollower->resetOriginalDestination(false);
	}
}

/**
 * Loads the target from a YAML file.
 * @param node YAML node.
 */
void Target::load(const YAML::YamlNodeReader& reader)
{
	reader.tryRead("lon", _lon);
	reader.tryRead("lat", _lat);
	reader.tryRead("id", _id);
	reader.tryRead("name", _name);
}

/**
 * Saves the target to a YAML file.
 * @returns YAML node.
 */
void Target::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	writer.write("lon", _lon);
	writer.write("lat", _lat);
	if (_id)
		writer.write("id", _id);
	if (!_name.empty())
		writer.write("name", _name);
}

/**
 * Saves the target's unique identifiers to a YAML file.
 * @return YAML node.
 */
void Target::saveId(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	writer.write("lon", _lon);
	writer.write("lat", _lat);
	writer.write("type", getType());
	writer.write("id", _id);
}

/**
 * Returns the longitude coordinate of the target.
 * @return Longitude in radian.
 */
double Target::getLongitude() const
{
	return _lon;
}

/**
 * Changes the longitude coordinate of the target.
 * @param lon Longitude in radian.
 */
void Target::setLongitude(double lon)
{
	_lon = lon;

	// Keep between 0 and 2xPI
	while (_lon < 0)
		_lon += 2 * M_PI;
	while (_lon >= 2 * M_PI)
		_lon -= 2 * M_PI;
}

/**
 * Returns the latitude coordinate of the target.
 * @return Latitude in radian.
 */
double Target::getLatitude() const
{
	return _lat;
}

/**
 * Changes the latitude coordinate of the target.
 * @param lat Latitude in radian.
 */
void Target::setLatitude(double lat)
{
	_lat = lat;
	// If you travel past a pole, continue on the other side of the globe.
	if (_lat < -M_PI/2)
	{
		_lat = -M_PI - _lat;
		setLongitude(_lon + M_PI);
	}
	else if (_lat > M_PI/2)
	{
		_lat = M_PI - _lat;
		setLongitude(_lon - M_PI);
	}
}

/**
 * Returns the target's unique ID.
 * @return Unique ID, 0 if none.
 */
int Target::getId() const
{
	return _id;
}

/**
 * Changes the target's unique ID.
 * @param id Unique ID.
 */
void Target::setId(int id)
{
	_id = id;
}

/**
 * Returns the target's user-readable name.
 * If there's no custom name, the language default is used.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::string Target::getName(Language *lang) const
{
	if (_name.empty())
		return getDefaultName(lang);
	return _name;
}

/**
 * Changes the target's custom name.
 * @param newName New custom name. If set to blank, the language default is used.
 */
void Target::setName(const std::string &newName)
{
	_name = newName;
}

/**
 * Returns the target's unique default name.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::string Target::getDefaultName(Language *lang) const
{
	return lang->getString(getMarkerName()).arg(_id);
}

/**
 * Returns the name on the globe for the target.
 * @return String ID.
 */
std::string Target::getMarkerName() const
{
	return getType() + "_";
}

/**
 * Returns the marker ID on the globe for the target.
 * @return Marker ID.
 */
int Target::getMarkerId() const
{
	return _id;
}

/**
 * Returns the list of targets currently
 * following this target.
 * @return Pointer to list of targets.
 */
std::vector<MovingTarget*> *Target::getFollowers()
{
	return &_followers;
}

/**
 * Returns the list of crafts currently
 * following this target.
 * @return List of crafts.
 */
std::vector<Craft*> Target::getCraftFollowers() const
{
	std::vector<Craft*> crafts;
	for (auto* mt : _followers)
	{
		Craft *craft = dynamic_cast<Craft*>(mt);
		if (craft)
		{
			crafts.push_back(craft);
		}
	}
	return crafts;
}

/**
 * Returns the list of UFOs currently
 * following this target.
 * @return List of UFOs.
 */
std::vector<Ufo*> Target::getUfoFollowers() const
{
	std::vector<Ufo*> ufos;
	for (auto* mt : _followers)
	{
		Ufo *ufo = dynamic_cast<Ufo*>(mt);
		if (ufo)
		{
			ufos.push_back(ufo);
		}
	}
	return ufos;
}

/**
 * Returns the great circle distance to another
 * target on the globe.
 * @param lon Longitude.
 * @param lat Latitude.
 * @returns Distance in radian.
 */
double Target::getDistance(double lon, double lat) const
{
	if (AreSame(lon, _lon) && AreSame(lat, _lat))
		return 0.0;
	return acos(cos(_lat) * cos(lat) * cos(lon - _lon) + sin(_lat) * sin(lat));
}

}
