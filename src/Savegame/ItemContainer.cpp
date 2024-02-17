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
#include "ItemContainer.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"

namespace OpenXcom
{

/**
 * Initializes an item container with no contents.
 */
ItemContainer::ItemContainer()
{
}

/**
 *
 */
ItemContainer::~ItemContainer()
{
}

/**
 * Loads the item container from a YAML file.
 * @param node YAML node.
 */
void ItemContainer::load(const YAML::Node &node, const Mod* mod)
{
	if (node && node.IsMap())
	{
		_qty.clear();
		for (const std::pair<YAML::Node, YAML::Node>& pair : node)
		{
			auto name = pair.first.as<std::string>();
			const auto* type = mod->getItem(name);
			if (type)
			{
				_qty[type] = pair.second.as<int>();
			}
			else
			{
				Log(LOG_ERROR) << "Failed to load item " << name;
			}
		}
	}
}

/**
 * Saves the item container to a YAML file.
 * @return YAML node.
 */
YAML::Node ItemContainer::save() const
{
	YAML::Node node(YAML::NodeType::Map);
	std::vector<std::pair<std::string, int>> sortedItems;

	for (auto& pair : _qty)
	{
		sortedItems.push_back(std::make_pair(pair.first->getType(), pair.second));
	}

	// enforce order of positions in node
	std::sort(sortedItems.begin(), sortedItems.end(), [](auto& a, auto& b){ return a < b; });
	for (auto& pair : sortedItems)
	{
		node[pair.first] = pair.second;
	}

	return node;
}

/**
 * Adds an item amount to the container.
 * @param id Item ID.
 * @param qty Item quantity.
 */
void ItemContainer::addItem(const RuleItem* item, int qty)
{
	if (item)
	{
		_qty[item] += qty;
	}
}

/**
 * Removes an item amount from the container.
 * @param id Item ID.
 * @param qty Item quantity.
 */
void ItemContainer::removeItem(const std::string &id, int qty)
{
	if (Mod::isEmptyRuleName(id))
	{
		return;
	}
	auto it = std::find_if(_qty.begin(), _qty.end(), [&](auto& pair) { return pair.first->getType() == id; });
	if (it == _qty.end())
	{
		return;
	}

	if (qty < it->second)
	{
		it->second -= qty;
	}
	else
	{
		_qty.erase(it);
	}
}

/**
 * Removes an item amount from the container.
 * @param id Item ID.
 * @param qty Item quantity.
 */
void ItemContainer::removeItem(const RuleItem* item, int qty)
{
	if (item)
	{
		auto it = _qty.find(item);
		if (it == _qty.end())
		{
			return;
		}

		if (qty < it->second)
		{
			it->second -= qty;
		}
		else
		{
			_qty.erase(it);
		}
	}
}

/**
 * Returns the quantity of an item in the container.
 * @param id Item ID.
 * @return Item quantity.
 */
int ItemContainer::getItem(const std::string &id) const
{
	if (Mod::isEmptyRuleName(id))
	{
		return 0;
	}

	auto it = std::find_if(_qty.begin(), _qty.end(), [&](auto& pair) { return pair.first->getType() == id; });
	if (it == _qty.end())
	{
		return 0;
	}
	else
	{
		return it->second;
	}
}

/**
 * Returns the quantity of an item in the container.
 * @param id Item ID.
 * @return Item quantity.
 */
int ItemContainer::getItem(const RuleItem* item) const
{
	if (item)
	{
		auto it = _qty.find(item);
		if (it == _qty.end())
		{
			return 0;
		}
		else
		{
			return it->second;
		}
	}
	else
	{
		return 0;
	}
}

/**
 * Returns the total quantity of the items in the container.
 * @return Total item quantity.
 */
int ItemContainer::getTotalQuantity() const
{
	int total = 0;
	for (const auto& pair : _qty)
	{
		total += pair.second;
	}
	return total;
}

/**
 * Returns the total size of the items in the container.
 * @param mod Pointer to mod.
 * @return Total item size.
 */
double ItemContainer::getTotalSize(const Mod *mod) const
{
	double total = 0;
	for (const auto& pair : _qty)
	{
		total += pair.first->getSize() * pair.second;
	}
	return total;
}

/**
 * Returns all the items currently contained within.
 * @return List of contents.
 */
const std::map<const RuleItem*, int> *ItemContainer::getContents() const
{
	return &_qty;
}

}
