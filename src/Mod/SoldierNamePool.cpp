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
#include "SoldierNamePool.h"
#include <sstream>
#include "../Savegame/Soldier.h"
#include "../Engine/RNG.h"
#include "../Engine/Language.h"
#include "../Engine/FileMap.h"

namespace OpenXcom
{

/**
 * Initializes a new pool with blank lists of names.
 */
SoldierNamePool::SoldierNamePool() : _totalWeight(0), _femaleFrequency(-1), _globalWeight(100)
{
}

/**
 *
 */
SoldierNamePool::~SoldierNamePool()
{
}

/**
 * Loads the pool from a YAML file.
 * @param filename YAML file.
 */
void SoldierNamePool::load(const std::string &filename)
{
	YAML::YamlRootNodeReader reader = FileMap::getYAML(filename);

	for (const auto& nameReader : reader["maleFirst"].children())
	{
		std::string name = nameReader.readVal<std::string>();
		_maleFirst.push_back(name);
	}
	for (const auto& nameReader : reader["femaleFirst"].children())
	{
		std::string name = nameReader.readVal<std::string>();
		_femaleFirst.push_back(name);
	}
	for (const auto& nameReader : reader["maleLast"].children())
	{
		std::string name = nameReader.readVal<std::string>();
		_maleLast.push_back(name);
	}
	for (const auto& nameReader : reader["femaleLast"].children())
	{
		std::string name = nameReader.readVal<std::string>();
		_femaleLast.push_back(name);
	}
	for (const auto& nameReader : reader["maleCallsign"].children())
	{
		std::string name = nameReader.readVal<std::string>();
		_maleCallsign.push_back(name);
	}
	for (const auto& nameReader : reader["femaleCallsign"].children())
	{
		std::string name = nameReader.readVal<std::string>();
		_femaleCallsign.push_back(name);
	}
	if (_femaleCallsign.empty())
	{
		_femaleCallsign = _maleCallsign;
	}
	if (_femaleFirst.empty())
	{
		_femaleFirst = _maleFirst;
	}
	if (_femaleLast.empty())
	{
		_femaleLast = _maleLast;
	}
	reader.tryRead("lookWeights", _lookWeights);
	_totalWeight = 0;
	for (int lw : _lookWeights)
	{
		_totalWeight += lw;
	}
	reader.tryRead("femaleFrequency", _femaleFrequency);
	reader.tryRead("globalWeight", _globalWeight);
	if (_globalWeight <= 0)
	{
		// can't let the modders break this completely
		_globalWeight = 100;
	}

	reader.tryRead("country", _country);
	reader.tryRead("region", _region);
}

/**
 * Returns a new random name (first + last) from the
 * lists of names contained within.
 * @param gender Returned gender of the name.
 * @return The soldier's name.
 */
std::string SoldierNamePool::genName(SoldierGender *gender, int femaleFrequency) const
{
	std::ostringstream name;
	bool female;
	if (_femaleFrequency > -1)
	{
		female = RNG::percent(_femaleFrequency);
	}
	else
	{
		female = RNG::percent(femaleFrequency);
	}

	if (!female)
	{
		*gender = GENDER_MALE;
		size_t first = RNG::generate(0, _maleFirst.size() - 1);
		name << _maleFirst[first];
		if (!_maleLast.empty())
		{
			size_t last = RNG::generate(0, _maleLast.size() - 1);
			name << " " << _maleLast[last];
		}
	}
	else
	{
		*gender = GENDER_FEMALE;
		size_t first = RNG::generate(0, _femaleFirst.size() - 1);
		name << _femaleFirst[first];
		if (!_femaleLast.empty())
		{
			size_t last = RNG::generate(0, _femaleLast.size() - 1);
			name << " " << _femaleLast[last];
		}
	}
	return name.str();
}

/**
 * Returns a new random callsign from the
 * lists of names contained within.
 * @param gender Gender of the callsign.
 * @return The soldier's callsign.
 */
std::string SoldierNamePool::genCallsign(const SoldierGender gender) const
{
	std::string callsign;
	if (!_femaleCallsign.empty())
	{
		if (gender == GENDER_MALE)
		{
			size_t first = RNG::generate(0, _maleCallsign.size() - 1);
			callsign = _maleCallsign[first];
		}
		else
		{
			size_t first = RNG::generate(0, _femaleCallsign.size() - 1);
			callsign = _femaleCallsign[first];
		}
	}
	return callsign;
}

/**
 * Generates an int representing the index of the soldier's look, when passed the maximum index value.
 * @param numLooks The maximum index.
 * @return The index of the soldier's look.
 */
size_t SoldierNamePool::genLook(size_t numLooks)
{
	int look = 0;
	const int minimumChance = 2;	// minimum chance of a look being selected if it isn't enumerated. This ensures that looks MUST be zeroed to not appear.

	while (_lookWeights.size() < numLooks)
	{
		_lookWeights.push_back(minimumChance);
		_totalWeight += minimumChance;
	}
	while (_lookWeights.size() > numLooks)
	{
		_totalWeight -= _lookWeights.back();
		_lookWeights.pop_back();
	}

	int random = RNG::generate(0, _totalWeight);
	for (int lw : _lookWeights)
	{
		if (random <= lw)
		{
			return look;
		}
		random -= lw;
		++look;
	}

	return RNG::generate(0, numLooks - 1);
}

}
