/*
 * Copyright 2010-2020 OpenXcom Developers.
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
#include "RuleManufactureShortcut.h"

namespace OpenXcom
{

/**
 * Creates a new RuleManufactureShortcut.
 * @param name The unique manufacture name.
 */
RuleManufactureShortcut::RuleManufactureShortcut(const std::string &name) : _name(name), _breakDownRequires(false), _breakDownRequiresBaseFunc(true)
{
	// empty
}

/**
 * Loads the RuleManufactureShortcut from a YAML file.
 * @param node YAML node.
 */
void RuleManufactureShortcut::load(const YAML::YamlNodeReader& reader)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("startFrom", _startFrom);
	reader.tryRead("breakDownItems", _breakDownItems);
	reader.tryRead("breakDownRequires", _breakDownRequires);
	reader.tryRead("breakDownRequiresBaseFunc", _breakDownRequiresBaseFunc);
}

}
