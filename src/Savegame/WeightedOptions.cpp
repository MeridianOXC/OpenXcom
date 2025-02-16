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
#include "WeightedOptions.h"
#include "../Engine/RNG.h"

namespace OpenXcom
{

/**
 * Select a random choice from among the contents.
 * This MUST be called on non-empty objects.
 * Each time this is called, the returned value can be different.
 * @return The key of the selected choice.
 */
std::string WeightedOptions::choose() const
{
	if (_totalWeight == 0)
	{
		return "";
	}
	size_t var = RNG::generate(1, _totalWeight);
	auto ii = _choices.begin();
	for (; ii != _choices.end(); ++ii)
	{
		if (var <= ii->second)
			break;
		var -= ii->second;
	}
	// We always have a valid iterator here.
	return ii->first;
}

/**
 * Set an option's weight.
 * If @a weight is set to 0, the option is removed from the list of choices.
 * If @a id already exists, the new weight replaces the old one, otherwise
 * @a id is added to the list of choices, with @a weight as the weight.
 * @param id The option name.
 * @param weight The option's new weight.
 */
void WeightedOptions::set(const std::string &id, size_t weight)
{
	auto option = _choices.find(id);
	if (option != _choices.end())
	{
		_totalWeight -= option->second;
		if (0 != weight)
		{
			option->second = weight;
			_totalWeight += weight;
		}
		else
		{
			_choices.erase(option);
		}
	}
	else if (0 != weight)
	{
		_choices.insert(std::make_pair(id, weight));
		_totalWeight += weight;
	}
}

/**
 * Read all the weighted options from a node reader and add them to the WeightedOptions.
 * The weight option list is not replaced, only values in @a nd will be added /
 * changed.
 * @param nd The YAML node (containing a map) with the new values.
 */
void WeightedOptions::load(const YAML::YamlNodeReader& reader)
{
	for (const auto& option : reader.children())
		set(option.readKey<std::string>(), option.readVal<size_t>());
}

/**
 * Send the WeightedOption contents to a YAML::Emitter.
 * @return YAML node.
 */
void WeightedOptions::save(YAML::YamlNodeWriter writer) const
{
	if (_choices.empty()) // for compatibility reasons, we output null instead of empty map
	{
		writer.setValueNull();
		return;
	}
	writer.setAsMap();
	for (const auto& pair : _choices)
		writer.write(writer.saveString(pair.first), pair.second);
}

/**
 * Get the list of strings associated with these weights.
 * @return the list of strings in these weights.
 */
std::vector<std::string> WeightedOptions::getNames()
{
	std::vector<std::string> names;
	for (const auto& pair : _choices)
	{
		names.push_back(pair.first);
	}
	return names;
}

}
