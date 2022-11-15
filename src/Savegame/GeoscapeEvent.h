#pragma once
/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

class RuleEvent;

/**
 * Represents a custom Geoscape event, generated in the background and waiting to pop up.
 */
class GeoscapeEvent
{
private:
	const RuleEvent &_rule;
	size_t _spawnCountdown;
	bool _over;
public:
	/// Creates a blank GeoscapeEvent.
	GeoscapeEvent(const RuleEvent &rule);
	/// Cleans up the event info.
	~GeoscapeEvent();
	/// Loads the event from YAML.
	void load(const YAML::Node &node);
	/// Saves the event to YAML.
	YAML::Node save() const;
	/// Gets the event's ruleset.
	const RuleEvent &getRules() const { return _rule; };
	/// Gets the minutes until the event pops up.
	size_t getSpawnCountdown() const { return _spawnCountdown; }
	/// Sets the minutes until the event pops up.
	void setSpawnCountdown(size_t minutes);

	/// Is this event over?
	bool isOver() const { return _over; }
	/// Handle event spawning schedule.
	void think();
};

}
