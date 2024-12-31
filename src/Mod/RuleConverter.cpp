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
#include "RuleConverter.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for converter data.
 */
RuleConverter::RuleConverter()
{
}

/**
 *
 */
RuleConverter::~RuleConverter()
{
}

/**
 * Loads the converter data from a YAML file.
 * @param node YAML node.
 */
void RuleConverter::load(const YAML::YamlNodeReader& node)
{
	const auto reader = node.useIndex();
	reader.tryRead("offsets", _offsets);
	reader.tryRead("markers", _markers);
	reader.tryRead("countries", _countries);
	reader.tryRead("regions", _regions);
	reader.tryRead("facilities", _facilities);
	reader.tryRead("items", _items);
	reader.tryRead("crews", _crews);
	reader.tryRead("crafts", _crafts);
	reader.tryRead("ufos", _ufos);
	reader.tryRead("craftWeapons", _craftWeapons);
	reader.tryRead("missions", _missions);
	reader.tryRead("armor", _armor);
	reader.tryRead("alienRaces", _alienRaces);
	reader.tryRead("alienRanks", _alienRanks);
	reader.tryRead("research", _research);
	reader.tryRead("manufacture", _manufacture);
	reader.tryRead("ufopaedia", _ufopaedia);
}

}
