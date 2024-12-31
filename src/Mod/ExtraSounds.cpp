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

#include <algorithm>
#include "ExtraSounds.h"
#include "../Engine/SoundSet.h"
#include "../Engine/Sound.h"
#include "../Engine/FileMap.h"
#include "../Engine/Logger.h"
#include "../Engine/Exception.h"
#include "../Engine/Unicode.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank set of extra sound data.
 */
ExtraSounds::ExtraSounds() : _current(0)
{
}

/**
 * Cleans up the extra sound set.
 */
ExtraSounds::~ExtraSounds()
{
}

/**
 * Loads the extra sound set from YAML.
 * @param node YAML node.
 * @param modIndex The internal index of the associated mod.
 */
void ExtraSounds::load(const YAML::YamlNodeReader& reader, const ModData* current)
{
	reader.tryRead("type", _type);
	reader.tryRead("files", _sounds);
	_current = current;
}

/**
 * Gets the filename that this sound set represents.
 * @return The sound name.
 */
const std::string& ExtraSounds::getType() const
{
	return _type;
}

/**
 * Gets the list of sounds defined by this mod.
 * @return The list of sounds defined my this mod.
 */
std::map<int, std::string> *ExtraSounds::getSounds()
{
	return &_sounds;
}

/**
 * Loads the external sounds into a new or existing soundset.
 * @param set Existing soundset.
 * @return New soundset.
 */
SoundSet *ExtraSounds::loadSoundSet(SoundSet *set) const
{
	if (set == 0)
	{
		Log(LOG_WARNING) << "Creating new sound set: " << _type << ", this will likely have no in-game use.";
		set = new SoundSet();
	}
	else
	{
		Log(LOG_VERBOSE) << "Adding/Replacing items in sound set: " << _type;
	}
	for (const auto& pair : _sounds)
	{
		int startSound = pair.first;
		const auto& fileName = pair.second;
		if (fileName[fileName.length() - 1] == '/')
		{
			Log(LOG_VERBOSE) << "Loading sound set from folder: " << fileName << " starting at index: " << startSound;
			int offset = startSound;
			std::vector<std::string> contents;
			for (const auto& f: FileMap::getVFolderContents(fileName)) { contents.push_back(f); }
			std::sort(contents.begin(), contents.end(), Unicode::naturalCompare);
			for (const auto& name : contents)
			{
				try
				{
					loadSound(set, offset, fileName + name);
					offset++;
				}
				catch (Exception &e)
				{
					Log(LOG_WARNING) << e.what();
				}
			}
		}
		else
		{
			loadSound(set, startSound, fileName);
		}
	}
	return set;
}

void ExtraSounds::loadSound(SoundSet *set, int index, const std::string &fileName) const
{
	int indexWithOffset = index;
	if (indexWithOffset >= set->getMaxSharedSounds())
	{
		if ((size_t)indexWithOffset >= _current->size)
		{
			std::ostringstream err;
			err << "ExtraSounds '" << _type << "' sound '" << indexWithOffset << "' exceeds mod '"<< _current->name <<"' size limit " << _current->size;
			throw Exception(err.str());
		}
		indexWithOffset += _current->offset;
	}

	Sound *sound = set->getSound(indexWithOffset);
	if (sound)
	{
		Log(LOG_VERBOSE) << "Replacing sound: " << index << ", using index: " << indexWithOffset;
	}
	else
	{
		Log(LOG_VERBOSE) << "Adding sound: " << index << ", using index: " << indexWithOffset;
		sound = set->addSound(indexWithOffset);
	}
	sound->load(fileName);
}

}
