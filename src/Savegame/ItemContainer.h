#pragma once
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
#include <string>
#include <map>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

class Mod;
class RuleItem;

/**
 * Represents the items contained by a certain entity,
 * like base stores, craft equipment, etc.
 * Handles all necessary item management tasks.
 */
class ItemContainer
{
private:
	std::map<const RuleItem*, int> _qty;
public:
	/// Creates an empty item container.
	ItemContainer();
	/// Cleans up the item container.
	~ItemContainer();
	/// Loads the item container from YAML.
	void load(const YAML::YamlNodeReader& reader, const Mod* mod);
	/// Saves the item container to YAML.
	void save(YAML::YamlNodeWriter writer) const;
	/// Adds an item to the container.
	void addItem(const std::string &id, int qty = 1) = delete;
	/// Adds an item to the container.
	void addItem(const RuleItem* item, int qty = 1);
	/// Removes an item from the container.
	void removeItem(const std::string &id, int qty = 1);
	/// Removes an item from the container.
	void removeItem(const RuleItem* item, int qty = 1);
	/// Gets an item in the container.
	int getItem(const std::string &id) const;
	/// Gets an item in the container.
	int getItem(const RuleItem* item) const;
	/// Gets the total quantity of items in the container.
	int getTotalQuantity() const;
	/// Gets the total size of items in the container.
	double getTotalSize() const;
	/// Check if have any item
	bool empty() const { return _qty.empty(); }
	/// Clear all content.
	void clear() { _qty.clear(); }
	/// Gets all the items in the container.
	const std::map<const RuleItem*, int> *getContents() const;
};

}
