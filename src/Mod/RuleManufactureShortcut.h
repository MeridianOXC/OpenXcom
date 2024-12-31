#pragma once
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
#include <string>
#include <vector>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

/**
 * Represents the information needed to create a new manufacture rule based on some existing manufacture rule.
 */
class RuleManufactureShortcut
{
private:
	std::string _name, _startFrom;
	std::vector<std::string> _breakDownItems;
	bool _breakDownRequires, _breakDownRequiresBaseFunc;
public:
	/// Creates a new RuleManufactureShortcut.
	RuleManufactureShortcut(const std::string &name);
	/// Deletes a RuleManufactureShortcut.
	~RuleManufactureShortcut() = default;
	/// Loads the RuleManufactureShortcut from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the name of a new manufacture rule to be created.
	const std::string& getName() const { return _name; }
	/// Gets the name of a manufacture rule to be used as a starting point.
	const std::string& getStartFrom() const { return _startFrom; }
	/// Gets the names of items that should be broken down into simpler components.
	const std::vector<std::string>& getBreakDownItems() const { return _breakDownItems; }
	/// Gets whether or not the required research should also be broken down.
	bool getBreakDownRequires() const { return _breakDownRequires; }
	/// Gets whether or not the required base services should also be broken down.
	bool getBreakDownRequiresBaseFunc() const { return _breakDownRequiresBaseFunc; }
};

}
