#pragma once
/*
 * Copyright 2010-2023 OpenXcom Developers.
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
#include <yaml-cpp/yaml.h>
#include <SDL_stdinc.h>

namespace OpenXcom
{

// Default value that is interpreted as null.
constexpr int defIntNullable = -1;

// Default value that is interpreted as null.
constexpr Sint8 defBoolNullable = -1;

/// Load bool from yaml.
inline void loadBool(bool& value, const YAML::Node& node)
{
	if (node)
	{
		value = node.as<bool>();
	}
}

/// Load bool as int from yaml.
inline void loadBoolNullable(int& value, const YAML::Node& node)
{
	if (node)
	{
		if (node.IsNull())
		{
			value = defBoolNullable;
		}
		else
		{
			value = node.as<bool>();
		}
	}
}

/// Load bool as int from yaml.
inline void loadBoolNullable(Sint8& value, const YAML::Node& node)
{
	if (node)
	{
		if (node.IsNull())
		{
			value = defBoolNullable;
		}
		else
		{
			value = node.as<bool>();
		}
	}
}

/// Load int from yaml.
inline void loadInt(int& value, const YAML::Node& node)
{
	if (node)
	{
		value = node.as<int>();
	}
}

/// Load int from yaml.
inline void loadIntNullable(int& value, const YAML::Node& node)
{
	if (node)
	{
		if (node.IsNull())
		{
			value = defIntNullable;
		}
		else
		{
			value = node.as<int>();
		}
	}
}

inline bool useBoolNullable(int value, bool def)
{
	return value == defBoolNullable ? def : value;
}

inline int useIntNullable(int value, int def)
{
	return value == defIntNullable ? def : value;
}


} //namespace OpenXcom
