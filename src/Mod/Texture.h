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
#include "../fmath.h"
#include <string>
#include <vector>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

struct TerrainCriteria
{
	std::string name;
	int weight;
	double lonMin, lonMax, latMin, latMax;
	TerrainCriteria() : weight(1), lonMin(0.0), lonMax(360.0), latMin(-90.0), latMax(90.0) {};
};

class Target;

/**
 * Represents the relations between a Geoscape texture
 * and the corresponding Battlescape mission attributes.
 */
class Texture
{
private:
	int _id;
	bool _isOcean;
	bool _fakeUnderwater;
	std::string _startingCondition;
	std::map<std::string, int> _deployments;
	std::vector<TerrainCriteria> _terrain;
	std::vector<TerrainCriteria> _baseTerrain;
public:
	/// Creates a new texture with mission data.
	Texture(int id);
	/// Cleans up the texture.
	~Texture();
	/// Loads the texture from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the list of terrain criteria.
	std::vector<TerrainCriteria> *getTerrain();
	/// Gets a random texture terrain for a given target.
	std::string getRandomTerrain(Target *target) const;
	/// Gets the list of terrain criteria for base defenses.
	std::vector<TerrainCriteria> *getBaseTerrain();
	/// Gets a random texture terrain for base defenses for a given target.
	std::string getRandomBaseTerrain(Target *target) const;
	/// Gets the alien deployment for this texture.
	const std::map<std::string, int> &getDeployments() const;
	/// Gets a random deployment.
	std::string getRandomDeployment() const;
	/// Is the texture a cosmetic-only ocean texture?
	bool isCosmeticOcean() const { return _isOcean; }
	/// Is the texture a fake underwater texture?
	bool isFakeUnderwater() const { return _fakeUnderwater; }
	/// Gets the Texture's starting condition.
	const std::string &getStartingCondition() const { return _startingCondition; }
};

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, TerrainCriteria* val);

}
