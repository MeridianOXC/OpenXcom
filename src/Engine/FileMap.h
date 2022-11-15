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

#include <set>
#include <string>
#include <vector>
#include <istream>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <SDL_rwops.h>
#include "ModInfo.h"

namespace OpenXcom
{

/**
 * Maps canonical names to file paths and maintains the virtual file system
 * for resource files.
 */
namespace FileMap
{
	struct FileRecord {
		std::string fullpath; 	// includes zip file name if any

		void *zip; 				// borrowed reference/weakref. NOTNULL:
		size_t findex;       	// file index in the zipfile.

		FileRecord();

		/// Open file warped in RWops.
		SDL_RWops *getRWops() const;
		/// Read the whole file to memory and warp in RWops.
		SDL_RWops *getRWopsReadAll() const;

		std::unique_ptr<std::istream> getIStream() const;
		YAML::Node getYAML() const;
		std::vector<YAML::Node> getAllYAML() const;
	};

	/// For common operations on bunches of filenames
	typedef std::unordered_set<std::string> NameSet;

	/// Get FileRecord for the fullpath ..
	const FileRecord *at(const std::string &relativeFilePath);

	/// Gets SDL_RWops for the file data of a data file blah blah read above.
	SDL_RWops *getRWops(const std::string &relativeFilePath);

	/// Gets SDL_RWops for the file data of a data file blah blah read above. Reads the whole file to memory.
	SDL_RWops *getRWopsReadAll(const std::string &relativeFilePath);

	/// Gets an std::istream interface to the file data. Has to be deleted on the caller's end.
	std::unique_ptr<std::istream>getIStream(const std::string &relativeFilePath);

	/// Gets a 'vertical slice' through all the VFS layers for a given file name. (used for langs)
	/// Beware of NULLs for the layers that miss the relpath.
	const std::vector<const FileRecord *> getSlice(const std::string &relativeFilePath);

	/// Returns parsed YAML for a filename
	YAML::Node getYAML(const std::string &relativeFilePath);
	std::vector<YAML::Node> getAllYAML(const std::string &relativeFilePath);

	/// if we have the file
	bool fileExists(const std::string &relativeFilePath);

	/// Returns the set of files in a virtual folder.  The virtual folder contains files from all active mods
	/// that are in similarly-named subdirectories.
	/// level select the layer in the VFS cake. Useful value is 0 for the bottommost aka 'dataFolder/common'
	/// Does not include rulesets (*.rul).
	const NameSet &getVFolderContents(const std::string &relativePath);
	const NameSet &getVFolderContents(const std::string &relativePath, size_t level);

	/// Returns the subset of the given files that matches the given extension
	NameSet filterFiles(const std::vector<std::string> &files, const std::string &ext);
	NameSet filterFiles(const std::set<std::string>    &files, const std::string &ext);
	NameSet filterFiles(const NameSet &files, const std::string &ext);

	/// Returns the ruleset files found, grouped by mod, while mapping resources.  The highest-priority mod
	/// will be last in the returned vector.

	typedef std::vector<std::pair<std::string, std::vector<FileRecord>>> RSOrder;
	const RSOrder &getRulesets();

	/// absolutely clears FileMap state and maps common resources (dataDir/common)
	void clear(bool clearOnly, bool embeddedOnly);

	/// sets up VFS according to the mod sequence given (rescans common resources). does call clear().
	void setup(const std::vector<const ModInfo *>& active, bool embeddedOnly);

	/// lowercase it
	std::string canonicalize(const std::string& fname);

	/// scans a moddir for mods, (privately) maps them.
	void scanModDir(const std::string& dirname, const std::string& basename, bool protectedLocation);

	/// scans a .zip from the rwops for mods
	void scanModZipRW(SDL_RWops *rwops, const std::string& fullpath);

	/// removes mods that lack extResourses, depend on missing mods
	/// or participate in dependency loops
	void checkModsDependencies();

	/// returns a list of mods that are loadable.
	std::map<std::string, ModInfo> getModInfos();

	/// Get mod file based on mod info.
	const FileRecord* getModRuleFile(const ModInfo* modInfo, const std::string& relpath);

	/// Unzip a file from a .zip into memory.
	SDL_RWops *zipGetFileByName(const std::string& zipfile, const std::string& fullpath);

}

}
