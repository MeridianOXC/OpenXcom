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
#include "Target.h"
#include <string>
#include "../Mod/AlienDeployment.h"
#include "../Engine/Yaml.h"

namespace OpenXcom
{

/**
 * Represents an alien base on the world.
 */
class AlienBase : public Target
{
private:
	std::string _pactCountry;
	std::string _race;
	bool _inBattlescape, _discovered;
	AlienDeployment *_deployment;
	int _startMonth;
	int _minutesSinceLastHuntMissionGeneration;
	int _genMissionCount;
public:
	/// Creates an alien base.
	AlienBase(AlienDeployment *deployment, int startMonth);
	/// Cleans up the alien base.
	~AlienBase();
	/// Loads the alien base from YAML.
	void load(const YAML::YamlNodeReader& reader) override;
	/// Saves the alien base to YAML.
	void save(YAML::YamlNodeWriter writer) const override;
	/// Gets the alien base's type.
	std::string getType() const override;
	/// Gets the alien base's marker sprite.
	int getMarker() const override;
	/// Gets the alien base's pact country.
	const std::string &getPactCountry() const;
	/// Sets the alien base's pact country.
	void setPactCountry(const std::string &pactCountry);
	/// Gets the alien base's amount of active hours.
	std::string getAlienRace() const;
	/// Sets the alien base's alien race.
	void setAlienRace(const std::string &race);
	/// Sets the alien base's battlescape status.
	void setInBattlescape(bool inbattle);
	/// Gets the alien base's battlescape status.
	bool isInBattlescape() const;
	/// Gets the alien base's discovered status.
	bool isDiscovered() const;
	/// Sets the alien base's discovered status.
	void setDiscovered(bool discovered);

	AlienDeployment *getDeployment() const;
	void setDeployment(AlienDeployment *deployment);

	/// Gets the month on which the base spawned.
	int getStartMonth() const { return _startMonth; }
	/// Sets the month on which the base spawned.
	void setStartMonth(int startMonth) { _startMonth = startMonth; }
	/// Gets the number of minutes passed since the last hunt mission was generated.
	int getMinutesSinceLastHuntMissionGeneration() const;
	/// Sets the number of minutes passed since the last hunt mission was generated.
	void setMinutesSinceLastHuntMissionGeneration(int newValue);
	/// Gets the number of genMissions generated so far by this base.
	int getGenMissionCount() const;
	/// Sets the number of genMissions generated so far by this base.
	void setGenMissionCount(int newValue);

};

}
