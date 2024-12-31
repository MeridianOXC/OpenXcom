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
#include "../Engine/Yaml.h"
#include <map>
#include <string>
#include <sstream>
#include "GameTime.h"
#include "../Engine/Language.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Container for mission statistics.
 */
struct MissionStatistics
{
	// Variables
	int id;
	std::string markerName;
	int markerId;
	GameTime time;
	std::string region, country, type, ufo;
	bool success;
	std::string rating;
	int score;
	std::string alienRace;
	int daylight;
	std::map<int, int> injuryList;
	bool valiantCrux;
	int lootValue;

	/// Load
	void load(const YAML::YamlNodeReader& reader)
	{
		reader.tryRead("id", id);
		reader.tryRead("markerName", markerName);
		reader.tryRead("markerId", markerId);
		time.load(reader["time"]);
		reader.tryRead("region", region);
		reader.tryRead("country", country);
		reader.tryRead("type", type);
		reader.tryRead("ufo", ufo);
		reader.tryRead("success", success);
		reader.tryRead("score", score);
		reader.tryRead("rating", rating);
		reader.tryRead("alienRace", alienRace);
		reader.tryRead("daylight", daylight);
		reader.tryRead("injuryList", injuryList);
		reader.tryRead("valiantCrux", valiantCrux);
		reader.tryRead("lootValue", lootValue);
	}

	/// Save
	void save(YAML::YamlNodeWriter writer) const
	{
		writer.setAsMap();
		writer.write("id", id);
		if (!markerName.empty())
		{
			writer.write("markerName", markerName);
			writer.write("markerId", markerId);
		}
		time.save(writer["time"]);
		writer.write("region", region);
		writer.write("country", country);
		writer.write("type", type);
		writer.write("ufo", ufo);
		writer.write("success", success);
		writer.write("score", score);
		writer.write("rating", rating);
		writer.write("alienRace", alienRace);
		writer.write("daylight", daylight);
		if (!injuryList.empty())
			writer.write("injuryList", injuryList);
		if (valiantCrux) writer.write("valiantCrux", valiantCrux);
		if (lootValue) writer.write("lootValue", lootValue);
	}

	std::string getMissionName(Language *lang) const
	{
		if (!markerName.empty())
		{
			return lang->getString(markerName).arg(markerId);
		}
		else
		{
			return lang->getString(type);
		}
	}

	std::string getRatingString(Language *lang) const
	{
		std::ostringstream ss;
		if (success)
		{
			ss << lang->getString("STR_VICTORY");
		}
		else
		{
			ss << lang->getString("STR_DEFEAT");
		}
		ss << " - " << lang->getString(rating);
		return ss.str();
	}

	std::string getLocationString() const
	{
		if (country == "STR_UNKNOWN")
		{
			return region;
		}
		else
		{
			return country;
		}
	}

	bool isDarkness(const Mod* mod) const
	{
		return daylight > mod->getMaxDarknessToSeeUnits();
	}

	std::string getDaylightString(const Mod* mod) const
	{
		if (isDarkness(mod))
		{
			return "STR_NIGHT";
		}
		else
		{
			return "STR_DAY";
		}
	}

	bool isAlienBase() const
	{
		if (type.find("STR_ALIEN_BASE") != std::string::npos || type.find("STR_ALIEN_COLONY") != std::string::npos)
		{
			return true;
		}
		return false;
	}

	bool isBaseDefense() const
	{
		if (type == "STR_BASE_DEFENSE")
		{
			return true;
		}
		return false;
	}

	bool isUfoMission() const
	{
		if(ufo != "NO_UFO")
		{
			return true;
		}
		return false;
	}

	MissionStatistics(const YAML::YamlNodeReader& reader) : time(0, 0, 0, 0, 0, 0, 0) { load(reader); }
	MissionStatistics() : id(0), markerId(0), time(0, 0, 0, 0, 0, 0, 0), region("STR_REGION_UNKNOWN"), country("STR_UNKNOWN"), ufo("NO_UFO"), success(false), score(0), alienRace("STR_UNKNOWN"), daylight(0), valiantCrux(false), lootValue(0) { }
	~MissionStatistics() { }
};

}
