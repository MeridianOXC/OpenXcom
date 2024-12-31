#pragma once
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
#include "../Engine/Yaml.h"
#include "../Battlescape/Position.h"

namespace OpenXcom
{

/**
 * For adding custom palettes to the game.
 */
class CustomPalettes
{
private:
	std::string _type, _target, _file;
	std::map<int, Position> _palette;
public:
	/// Creates a blank custom palette.
	CustomPalettes(const std::string &type);
	/// Clean up.
	virtual ~CustomPalettes();
	/// Loads the data from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the palette.
	std::map<int, Position> *getPalette();
	/// Gets the type.
	const std::string &getType() const;
	/// Gets the target.
	const std::string &getTarget() const;
	/// Gets the source file in JASC palette format (256 colors).
	const std::string &getFile() const;

};

}
