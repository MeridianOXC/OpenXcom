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
#include <string>
#include <map>
#include <vector>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

/**
 * String with normalized version number,
 * as is not printable it use `signed char` to
 * not confuse it with normal string.
 */
typedef std::basic_string<signed char> ModInfoNormalizedVersion;

/**
 * Version number with normalized value.
 */
typedef std::pair<std::string, ModInfoNormalizedVersion> ModInfoVersion;

/**
 * Represents mod metadata
 */
class ModInfo
{
private:
	const std::string _path;
	std::string _name, _desc, _author, _url, _id, _master;
	std::string _versionDisplay;
	ModInfoVersion _version;
	bool _isMaster;
	int _reservedSpace;
	bool _engineOk;
	std::string _requiredExtendedEngine;
	std::string _requiredExtendedVersion;

	ModInfoVersion _requiredMasterModVersion;

	std::string _resourceConfigFile;
	std::vector<std::string> _externalResourceDirs;
public:
	/// Creates default metadata for a mod at the specified path.
	ModInfo(const std::string &path);
	/// Loads the metadata from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the path where this mod resides on disk.
	const std::string &getPath() const;
	/// Gets the name of this mod.
	const std::string &getName() const;
	/// Gets the description for this mod.
	const std::string &getDescription() const;
	/// Gets the version of this mod.
	const std::string &getVersion() const;
	/// Gets the version for display of this mod.
	const std::string &getVersionDisplay() const;
	/// Gets the author of this mod.
	const std::string &getAuthor() const;
	/// Gets the id for this mod.
	const std::string &getId() const;

	/// Gets the master this mod can load under.  If it can load under any
	/// master (or if this mod is a master itself FIXME: this is wrong), the return value is empty.
	const std::string &getMaster() const;
	/// Gets required version of master mod.
	const std::string &getRequiredMasterVersion() const;

	/// Gets whether this mod is a master (i.e. a vanilla game/total conversion)
	bool isMaster() const;
	/// Is parent mod is in required version?
	bool isParentMasterOk(const ModInfo* parentMod) const;

	/// Gets whether this mod can be activated.
	bool canActivate(const std::string &curMaster) const;
	/// Gets size of mod, bigger mod reserve more values in common collections/surfacesets.
	int getReservedSpace() const;

	/// Is mod compatible with current OXCE engine aka "fork", check too whether the current OXCE version is equal to (or higher than) the required OXCE version.
	bool isEngineOk() const;
	/// Gets the OXCE engine (like "FtA" or "Extended") required by this mod.
	const std::string &getRequiredExtendedEngine() const;
	/// Gets the OXCE version required by this mod.
	const std::string &getRequiredExtendedVersion() const;

	/// Gets ruleset file where are defined based resources like required original game data.
	const std::string &getResourceConfigFile() const;
	/// Gets the list of external resource dirs to load for this mod.
	const std::vector<std::string> &getExternalResourceDirs() const;
};

}
