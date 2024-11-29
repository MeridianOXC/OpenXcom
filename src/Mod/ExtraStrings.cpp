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

#include "ExtraStrings.h"

namespace OpenXcom
{

/**
 * Creates a blank set of extra strings data.
 */
ExtraStrings::ExtraStrings()
{
}

/**
 * Cleans up the extra strings set.
 */
ExtraStrings::~ExtraStrings()
{
}

/**
 * Loads the extra strings set from YAML.
 * @param node YAML node.
 */
void ExtraStrings::load(const YAML::YamlNodeReader& reader)
{
	for (const auto& string : reader["strings"].children())
	{
		// Regular strings
		std::string key = string.readKey<std::string>();
		if (string.hasVal())
		{
			_strings[key] = string.readVal<std::string>();
		}
		// Strings with plurality
		else if (string.isMap())
		{
			for (const auto& plurality : string.children())
			{
				std::string s = key + "_" + plurality.readKey<std::string>();
				_strings[s] = plurality.readVal<std::string>();
			}
		}
	}
}

/**
 * Gets the list of strings defined my this mod.
 * @return The list of strings.
 */
std::map<std::string, std::string> *ExtraStrings::getStrings()
{
	return &_strings;
}

}
