#pragma once
/*
 * Copyright 2010-2025 OpenXcom Developers.
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
#include "GameTime.h"
#include <string>
#include "../Mod/Mod.h"
#include "../Mod/RuleResearch.h"
#include "../Mod/RuleEvent.h"
#include "../Mod/AlienDeployment.h"

namespace OpenXcom
{

enum DiscoverySourceType : Uint8 { BASE = 0, FREE_FROM = 1, FREE_AFTER = 2, MISSION = 3, EVENT = 4 };

struct DiscoverySource
{
	DiscoverySourceType type;
	std::string name; // Not guaranteed to be valid
	const RuleResearch* research;
	const RuleEvent* event;
	const AlienDeployment* mission;

	std::string_view getTypeString()
	{
		switch (type)
		{
		case DiscoverySourceType::BASE:
			return "STR_DISCOVERY_SOURCE_BASE";
		case DiscoverySourceType::FREE_FROM:
			return "STR_DISCOVERY_SOURCE_FREE_FROM";
		case DiscoverySourceType::FREE_AFTER:
			return "STR_DISCOVERY_SOURCE_FREE_AFTER";
		case DiscoverySourceType::MISSION:
			return "STR_DISCOVERY_SOURCE_MISSION";
		case DiscoverySourceType::EVENT:
			return "STR_DISCOVERY_SOURCE_EVENT";
		default:
			throw Exception("Invalid DiscoverySourceType!");
		}
	}
};

/**
 * Element of the research diary
 */
struct ResearchDiaryEntry
{
	const RuleResearch* research;
	Uint32 year;
	Uint32 month;
	Uint32 day;
	DiscoverySource source;

	ResearchDiaryEntry(const RuleResearch* r) : year(), month(), day()
	{
		research = r;
	}

	void setDate(GameTime* time)
	{
		year = time->getYear();
		month = time->getMonth();
		day = time->getDay();
	}

	void load(const YAML::YamlNodeReader& reader, Mod* mod)
	{
		if (!research)
		{
			std::string name;
			reader.tryRead("name", name);
			research = mod->getResearch(name);
		}
		const YAML::YamlNodeReader& dateReader = reader["date"];
		year = dateReader[0].readVal<Uint32>();
		month = dateReader[1].readVal<Uint32>();
		day = dateReader[2].readVal<Uint32>();
		reader.tryRead("sourceType", source.type);
		reader.tryRead("sourceName", source.name);
		switch (source.type)
		{
		case DiscoverySourceType::FREE_FROM:
		case DiscoverySourceType::FREE_AFTER:
			source.research = mod->getResearch(source.name, false);
			break;
		case DiscoverySourceType::MISSION:
			source.mission = mod->getDeployment(source.name, false);
			break;
		case DiscoverySourceType::EVENT:
			source.event = mod->getEvent(source.name, false);
			break;
		default:
			break;
		}
	}

	void save(YAML::YamlNodeWriter writer) const
	{
		writer.setAsMap();
		writer.setFlowStyle();
		YAML::YamlNodeWriter dateWriter = writer["date"];
		dateWriter.setAsSeq();
		dateWriter.setFlowStyle();
		dateWriter.write(year);
		dateWriter.write(month);
		dateWriter.write(day);
		writer.write("name", research->getName());
		writer.write("sourceType", source.type);
		writer.write("sourceName", source.name);
	}
};

}
