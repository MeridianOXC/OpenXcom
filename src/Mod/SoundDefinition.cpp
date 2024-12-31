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
#include "SoundDefinition.h"

namespace OpenXcom
{

SoundDefinition::SoundDefinition(const std::string &type) : _type(type)
{
}

SoundDefinition::~SoundDefinition()
{
}

void SoundDefinition::load(const YAML::YamlNodeReader& reader)
{
	for (const auto& soundRange : reader["soundRanges"].children())
	{
		std::pair<int, int> range = soundRange.readVal(std::make_pair<int, int>(0, 0));
		for (int j = range.first; j <= range.second; ++j)
		{
			_soundList.push_back(j);
		}
	}
	for (const auto& sound : reader["sounds"].children())
	{
		_soundList.push_back(sound.readVal(-1));
	}
	reader.tryRead("file", _catFile);
}

const std::vector<int> &SoundDefinition::getSoundList() const
{
	return _soundList;
}

std::string SoundDefinition::getCATFile() const
{
	return _catFile;
}

}
