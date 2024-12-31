/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#include "CustomPalettes.h"

namespace OpenXcom
{

/**
 * Creates a blank custom palette.
 */
CustomPalettes::CustomPalettes(const std::string &type) : _type(type)
{
}

/**
 * Clean up.
 */
CustomPalettes::~CustomPalettes()
{
}

/**
 * Loads the custom palette from YAML.
 * @param node YAML node.
 */
void CustomPalettes::load(const YAML::YamlNodeReader& reader)
{
	reader.tryRead("target", _target);
	reader.tryRead("file", _file);
	reader.tryRead("palette", _palette);
}

/**
 * Returns the custom palette definition.
 * @return The list of RGB values.
 */
std::map<int, Position> *CustomPalettes::getPalette()
{
	return &_palette;
}

/**
 * Gets the name of the palette.
 * @return The palette type/name.
 */
const std::string &CustomPalettes::getType() const
{
	return _type;
}

/**
 * Gets the name of the target palette to be modded.
 * @return The target palette type/name.
 */
const std::string &CustomPalettes::getTarget() const
{
	return _target;
}

/**
 * Gets the source file in JASC palette format (256 colors).
 * @return The relative file path and name.
 */
const std::string &CustomPalettes::getFile() const
{
	return _file;
}

}
