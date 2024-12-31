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
#include "RuleGlobe.h"
#include <SDL_endian.h>
#include "../Engine/Exception.h"
#include "Polygon.h"
#include "Polyline.h"
#include "Texture.h"
#include "../Engine/Palette.h"
#include "../Geoscape/Globe.h"
#include "../Engine/FileMap.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for globe contents.
 */
RuleGlobe::RuleGlobe()
{
}

/**
 *
 */
RuleGlobe::~RuleGlobe()
{
	for (auto* polygon : _polygons)
	{
		delete polygon;
	}
	for (auto* polyline : _polylines)
	{
		delete polyline;
	}
	for (auto& pair : _textures)
	{
		delete pair.second;
	}
}

/**
 * Loads the globe from a YAML file.
 * @param node YAML node.
 */
void RuleGlobe::load(const YAML::YamlNodeReader& reader)
{
	if (const auto& data = reader["data"])
	{
		for (auto* polygon : _polygons)
		{
			delete polygon;
		}
		_polygons.clear();
		loadDat(data.readVal<std::string>());
	}
	if (const auto& polygonsReader = reader["polygons"])
	{
		for (auto* polygon : _polygons)
		{
			delete polygon;
		}
		_polygons.clear();
		for (const auto& polygonReader : polygonsReader.children())
		{
			Polygon *polygon = new Polygon(3);
			polygon->load(polygonReader);
			_polygons.push_back(polygon);
		}
	}
	if (const auto& polylinesReader = reader["polylines"])
	{
		for (auto* polyline : _polylines)
		{
			delete polyline;
		}
		_polylines.clear();
		for (const auto& polylineReader : polylinesReader.children())
		{
			Polyline *polyline = new Polyline(3);
			polyline->load(polylineReader);
			_polylines.push_back(polyline);
		}
	}
	for (const auto& textureReader : reader["textures"].children())
	{
		if (textureReader["id"])
		{
			int id = textureReader["id"].readVal<int>();
			auto j = _textures.find(id);
			Texture *texture;
			if (j != _textures.end())
			{
				texture = j->second;
			}
			else
			{
				texture = new Texture(id);
				_textures[id] = texture;
			}
			texture->load(textureReader);
		}
		else if (textureReader["delete"])
		{
			int id = textureReader["delete"].readVal<int>();
			auto j = _textures.find(id);
			if (j != _textures.end())
			{
				_textures.erase(j);
			}
		}
	}

	reader.tryRead("countryColor", Globe::COUNTRY_LABEL_COLOR);
	reader.tryRead("cityColor", Globe::CITY_LABEL_COLOR);
	reader.tryRead("baseColor", Globe::BASE_LABEL_COLOR);
	reader.tryRead("lineColor", Globe::LINE_COLOR);
	if (reader["oceanPalette"])
	{
		Globe::OCEAN_COLOR = Palette::blockOffset(reader["oceanPalette"].readVal(Globe::OCEAN_COLOR));
	}
	reader.tryRead("oceanShading", Globe::OCEAN_SHADING);
}

/**
 * Returns the list of polygons in the globe.
 * @return Pointer to the list of polygons.
 */
std::list<Polygon*> *RuleGlobe::getPolygons()
{
	return &_polygons;
}

/**
 * Returns the list of polylines in the globe.
 * @return Pointer to the list of polylines.
 */
std::list<Polyline*> *RuleGlobe::getPolylines()
{
	return &_polylines;
}

/**
 * Loads a series of map polar coordinates in X-Com format,
 * converts them and stores them in a set of polygons.
 * @param filename Filename of the DAT file.
 * @sa http://www.ufopaedia.org/index.php?title=WORLD.DAT
 */
void RuleGlobe::loadDat(const std::string &filename)
{
	auto mapFile = FileMap::getIStream(filename);
	short value[10];

	while (mapFile->read((char*)&value, sizeof(value)))
	{
		Polygon* poly;
		int points;

		for (int i = 0; i < 10; ++i)
		{
			value[i] = SDL_SwapLE16(value[i]);
		}

		if (value[6] != -1)
		{
			points = 4;
		}
		else
		{
			points = 3;
		}
		poly = new Polygon(points);

		for (int i = 0, j = 0; i < points; ++i)
		{
			// Correct X-Com degrees and convert to radians
			double lonRad = Xcom2Rad(value[j++]);
			double latRad = Xcom2Rad(value[j++]);

			poly->setLongitude(i, lonRad);
			poly->setLatitude(i, latRad);
		}
		poly->setTexture(value[8]);

		_polygons.push_back(poly);
	}

	if (!mapFile->eof())
	{
		throw Exception("Invalid globe map");
	}
}

/**
 * Returns the rules for the specified texture.
 * @param id Texture ID.
 * @return Rules for the texture.
 */
Texture *RuleGlobe::getTexture(int id) const
{
	auto i = _textures.find(id);
	if (_textures.end() != i) return i->second; else return 0;
}

/**
 * Returns a list of all globe terrains associated with this deployment.
 * @param deployment Deployment name.
 * @return List of terrains.
 */
std::vector<std::string> RuleGlobe::getTerrains(const std::string &deployment) const
{
	std::vector<std::string> terrains;
	for (auto& pair : _textures)
	{
		if ((deployment.empty() && pair.second->getDeployments().empty()) || pair.second->getDeployments().find(deployment) != pair.second->getDeployments().end())
		{
			for (const auto& tc : *pair.second->getTerrain())
			{
				terrains.push_back(tc.name);
			}
		}
	}
	return terrains;
}

}
