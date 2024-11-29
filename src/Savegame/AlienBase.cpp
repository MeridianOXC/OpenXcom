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
#include "AlienBase.h"
#include "../Engine/Language.h"

namespace OpenXcom
{

/**
 * Initializes an alien base
 */
AlienBase::AlienBase(AlienDeployment *deployment, int startMonth) : Target(), _inBattlescape(false), _discovered(false), _deployment(deployment), _startMonth(startMonth), _genMissionCount(0)
{
	// allow spawning hunt missions immediately after the base is created, i.e. no initial delay
	_minutesSinceLastHuntMissionGeneration = _deployment->getHuntMissionMaxFrequency();
}

/**
 *
 */
AlienBase::~AlienBase()
{
}

/**
 * Loads the alien base from a YAML file.
 * @param node YAML node.
 */
void AlienBase::load(const YAML::YamlNodeReader& reader)
{
	Target::load(reader);
	reader.tryRead("pactCountry", _pactCountry);
	reader.tryRead("race", _race);
	reader.tryRead("inBattlescape", _inBattlescape);
	reader.tryRead("discovered", _discovered);
	reader.tryRead("startMonth", _startMonth);
	reader.tryRead("minutesSinceLastHuntMissionGeneration", _minutesSinceLastHuntMissionGeneration);
	reader.tryRead("genMissionCount", _genMissionCount);
}

/**
 * Saves the alien base to a YAML file.
 * @return YAML node.
 */
void AlienBase::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	Target::save(writer);

	writer.write("pactCountry", _pactCountry);
	writer.write("race", _race);
	if (_inBattlescape)
		writer.write("inBattlescape", _inBattlescape);
	if (_discovered)
		writer.write("discovered", _discovered);
	writer.write("deployment", _deployment->getType());
	writer.write("startMonth", _startMonth);
	writer.write("minutesSinceLastHuntMissionGeneration", _minutesSinceLastHuntMissionGeneration);
	writer.write("genMissionCount", _genMissionCount);
}

/**
 * Returns the alien base's unique type used for
 * savegame purposes.
 * @return ID.
 */
std::string AlienBase::getType() const
{
	return _deployment->getMarkerName();
}

/**
 * Returns the globe marker for the alien base.
 * @return Marker sprite, -1 if none.
 */
int AlienBase::getMarker() const
{
	if (!_discovered)
		return -1;
	return _deployment->getMarkerIcon();
}

/**
 * Returns the country this base has a pact with.
 * @return Country ID.
 */
const std::string &AlienBase::getPactCountry() const
{
	return _pactCountry;
}

/**
 * Changes the country that has a pact with this alien base.
 * @param pactCountry Country ID.
 */
void AlienBase::setPactCountry(const std::string &pactCountry)
{
	_pactCountry = pactCountry;
}

/**
 * Returns the alien race currently residing in the alien base.
 * @return Alien race.
 */
std::string AlienBase::getAlienRace() const
{
	return _race;
}

/**
 * Changes the alien race currently residing in the alien base.
 * @param race Alien race.
 */
void AlienBase::setAlienRace(const std::string &race)
{
	_race = race;
}

/**
 * Gets the alien base's battlescape status.
 * @return Is the base on the battlescape?
 */
bool AlienBase::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Sets the alien base's battlescape status.
 * @param inbattle True if it's in battle, False otherwise.
 */
void AlienBase::setInBattlescape(bool inbattle)
{
	_inBattlescape = inbattle;
}

/**
 * Gets the alien base's geoscape status.
 * @return Has the base been discovered?
 */
bool AlienBase::isDiscovered() const
{
	return _discovered;
}

/**
 * Sets the alien base's geoscape status.
 * @param discovered Has the base been discovered?
 */
void AlienBase::setDiscovered(bool discovered)
{
	_discovered = discovered;
}

AlienDeployment *AlienBase::getDeployment() const
{
	return _deployment;
}

void AlienBase::setDeployment(AlienDeployment *deployment)
{
	_deployment = deployment;

	// allow spawning hunt missions immediately after the base is upgraded, i.e. no initial delay
	_minutesSinceLastHuntMissionGeneration = _deployment->getHuntMissionMaxFrequency();
}

/**
 * Gets the number of minutes passed since the last hunt mission was generated.
 * @return Number of minutes.
 */
int AlienBase::getMinutesSinceLastHuntMissionGeneration() const
{
	return _minutesSinceLastHuntMissionGeneration;
}

/**
 * Sets the number of minutes passed since the last hunt mission was generated.
 * @param newValue Number of minutes.
 */
void AlienBase::setMinutesSinceLastHuntMissionGeneration(int newValue)
{
	_minutesSinceLastHuntMissionGeneration = newValue;
}

/**
 * Gets the number of genMissions generated so far by this base.
 * @return Number of missions.
 */
int AlienBase::getGenMissionCount() const
{
	return _genMissionCount;
}

/**
 * Sets the number of genMissions generated so far by this base.
 * @param newValue Number of missions.
 */
void AlienBase::setGenMissionCount(int newValue)
{
	_genMissionCount = newValue;
}

}
