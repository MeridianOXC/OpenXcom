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
#include "RuleItemCategory.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain item category.
 * @param type String defining the item category.
 */
RuleItemCategory::RuleItemCategory(const std::string &type, int listOrder) : _type(type), _hidden(false), _listOrder(listOrder)
{
}

/**
 *
 */
RuleItemCategory::~RuleItemCategory()
{
}

/**
 * Loads the item category from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the item.
 * @param listOrder The list weight for this item.
 */
void RuleItemCategory::load(const YAML::YamlNodeReader& reader, Mod *mod)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod);
	}

	reader.tryRead("replaceBy", _replaceBy);
	reader.tryRead("hidden", _hidden);

	reader.tryRead("listOrder", _listOrder);
	mod->loadUnorderedNames(_type, _invOrder, reader["invOrder"]);
}

/**
 * Gets the item category type. Each category has a unique type.
 * @return The category's type.
 */
const std::string &RuleItemCategory::getType() const
{
	return _type;
}

/**
* Gets the item category type, which should be used instead of this one.
* @return The replacement category's type.
*/
const std::string &RuleItemCategory::getReplaceBy() const
{
	return _replaceBy;
}

/**
* Indicates whether the category is hidden or visible.
* @return True if hidden, false if visible.
*/
bool RuleItemCategory::isHidden() const
{
	return _hidden;
}

/**
 * Gets the list weight for this item category.
 * @return The list weight.
 */
int RuleItemCategory::getListOrder() const
{
	return _listOrder;
}

}
