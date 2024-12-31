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
#include "SoldierDeath.h"
#include "BattleUnitStatistics.h"

namespace OpenXcom
{

/**
 * Initializes a death event.
 */
SoldierDeath::SoldierDeath(GameTime time, BattleUnitKills *cause) : _time(time), _cause(cause)
{
}

/**
 * Cleans up a death event.
 */
SoldierDeath::~SoldierDeath()
{
	delete _cause;
}

/**
 * Loads the death from a YAML file.
 * @param node YAML node.
 */
void SoldierDeath::load(const YAML::YamlNodeReader& reader)
{
	_time.load(reader["time"]);
	if (const auto& kill = reader["cause"])
		_cause = new BattleUnitKills(kill);
}

/**
 * Saves the death to a YAML file.
 * @returns YAML node.
 */
void SoldierDeath::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	_time.save(writer["time"]);
	if (_cause)
		_cause->save(writer["cause"]);
}

/**
 * Returns the time of death of this soldier.
 * @return Pointer to the time.
 */
const GameTime *SoldierDeath::getTime() const
{
	return &_time;
}

/**
* Returns the time of death of this soldier.
* @return Pointer to the time.
*/
const BattleUnitKills *SoldierDeath::getCause() const
{
	return _cause;
}

}
