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

#include "../version.h"
#include "ModInfo.h"
#include "CrossPlatform.h"
#include "Exception.h"
#include "Logger.h"
#include "../Engine/Yaml.h"
#include <sstream>
#include <assert.h>

namespace OpenXcom
{

namespace
{

ModInfoVersion normalizeModVersion(const std::string& parent, const std::string& ver)
{
	const int prefix_num_min = 0;
	const int prefix_num_max = 10;
	const int prefix_text = 11;

	const int was_nothing = 0;
	const int was_num = 1;
	const int was_text = 2;
	const int was_dot = 3;


	int state = was_nothing;
	size_t last_prefix_num = 0;

	ModInfoNormalizedVersion normalized;

	for (std::string::const_iterator it = ver.begin(); it != ver.end(); ++it)
	{
		char curr = *it;
		if (curr >= 'a' && curr <= 'z')
		{
			curr += 'A' - 'a'; // make case-insensitive
		}

		if (curr >= 'A' && curr <= 'Z')
		{
			switch (state)
			{
				case was_nothing:
				case was_dot:
				case was_num:
				{
					state = was_text;
					normalized.push_back(prefix_text);
					normalized.push_back(curr);
					continue;
				}
				case was_text:
				{
					normalized.push_back(curr);
					continue;
				}
			}
		}
		else if (curr >= '0' && curr <= '9')
		{
			switch (state)
			{
				case was_nothing:
				case was_dot:
				case was_text:
				{
					state = was_num;
					last_prefix_num = normalized.size();
					normalized.push_back(prefix_num_min);
					if (curr > '0')
					{
						++normalized[last_prefix_num];
						normalized.push_back(curr);
					}
					continue;
				}
				case was_num:
				{
					int temp = normalized[last_prefix_num];
					if (temp == prefix_num_max)
					{
						Log(LOG_WARNING) << "Error in version number in mod '" << parent << "': unsupported number length";
						return std::make_pair(ver, ModInfoNormalizedVersion());
					}
					if (temp != prefix_num_min || curr > '0')
					{
						++normalized[last_prefix_num];
						normalized.push_back(curr);
					}
					continue;
				}
			}
		}
		else if (curr == '.')
		{
			if (was_dot == state)
			{
				Log(LOG_WARNING) << "Error in version number in mod '" << parent << "': duplicated dots";
				return std::make_pair(ver, ModInfoNormalizedVersion());
			}
			state = was_dot;
			continue;
		}
		else
		{
			Log(LOG_WARNING) << "Error in version number in mod '" << parent << "': unexpected symbol";
			return std::make_pair(ver, ModInfoNormalizedVersion());
		}
		assert(false && "invalid move version parser state");
	}

	// version could be ends with chain of "0.0.0.0.0", trim to last not zero
	std::string::size_type last = normalized.find_last_not_of(ModInfoNormalizedVersion::value_type(0));
	if (last != std::string::npos)
	{
		normalized.erase(last + 1);
	}
	else if (normalized.size())
	{
		// version is only "0" leave first "0" and remove rest
		normalized.erase(1);
	}

	return std::make_pair(ver, normalized);
}

bool compareVersions(const ModInfoVersion& provided, const ModInfoVersion& required)
{
	return provided.first == required.first || provided.second >= required.second;
}

const ModInfoVersion defaultModVersion = normalizeModVersion("def", "1.0");


} //namespace

ModInfo::ModInfo(const std::string &path) :
	 _path(path), _name(CrossPlatform::baseFilename(path)),
	_desc("No description."), _author("unknown author"),
	_id(_name), _master("xcom1"),
	_versionDisplay("1.0"),
	_version(defaultModVersion),
	_isMaster(false), _reservedSpace(1),
	_engineOk(false)
{
	// empty
}

namespace
{

struct EngineData
{
	std::string name;
	int version[4];
};

/**
 * List of engines that current version support.
 */
const EngineData supportedEngines[] = {
	{ OPENXCOM_VERSION_ENGINE, { OPENXCOM_VERSION_NUMBER }},
	{ "", { 0, 0, 0, 0 } }, // assume that every engine support mods from base game, remove if its not true.
};

template<int I>
bool findCompatibleEngine(const EngineData (&l)[I], const std::string& e, const std::array<int, 4>& version)
{
	for (auto& d : supportedEngines)
	{
		if (d.name == e)
		{
			return !CrossPlatform::isHigherThanCurrentVersion(version, d.version);
		}
	}
	return false;
}

} //namespace

void ModInfo::load(const YAML::YamlNodeReader& reader)
{
	reader.tryRead("id", _id);
	reader.tryRead("name", _name);
	reader.tryRead("description", _desc);
	if (const auto& ver = reader["version"])
	{
		_version  = normalizeModVersion(_id, ver.readVal<std::string>());
		_versionDisplay = _version.first;
	}
	reader.tryRead("versionDisplay", _versionDisplay);
	reader.tryRead("author", _author);
	reader.tryRead("isMaster", _isMaster);
	reader.tryRead("reservedSpace", _reservedSpace);

	if (reader.tryRead("requiredExtendedVersion", _requiredExtendedVersion))
		_requiredExtendedEngine = "Extended"; // for backward compatibility
	reader.tryRead("requiredExtendedEngine", _requiredExtendedEngine);

	_engineOk = findCompatibleEngine(supportedEngines, _requiredExtendedEngine, CrossPlatform::parseVersion(_requiredExtendedVersion));

	if (_reservedSpace < 1)
	{
		_reservedSpace = 1;
	}
	else if (_reservedSpace > 100)
	{
		_reservedSpace = 100;
	}

	if (_isMaster)
	{
		// default a master's master to none.  masters can still have
		// masters, but they must be explicitly declared.
		_master = "";
	}
	reader.tryRead("resourceConfig", _resourceConfigFile);

	reader.tryRead("master", _master);
	if (_master == "*")
	{
		_master = "";
	}

	if (_isMaster && _master.empty())
	{
		// only top-level masters can load external resource dirs
		reader.tryRead("loadResources", _externalResourceDirs);
	}

	if (const auto& req = reader["requiredMasterModVersion"])
	{
		if (_master.empty())
		{
			Log(LOG_WARNING) << "Mod '" << _id << "' without master can't have 'requiredMasterModVersion'.";
		}
		else
		{
			_requiredMasterModVersion = normalizeModVersion(_id, req.readVal<std::string>());
		}
	}
}

const std::string &ModInfo::getPath()                    const { return _path;                    }
const std::string &ModInfo::getName()                    const { return _name;                    }
const std::string &ModInfo::getDescription()             const { return _desc;                    }
const std::string &ModInfo::getVersion()                 const { return _version.first;           }
const std::string &ModInfo::getVersionDisplay()          const { return _versionDisplay;          }
const std::string &ModInfo::getAuthor()                  const { return _author;                  }
const std::string &ModInfo::getId()                      const { return _id;                      }
const std::string &ModInfo::getMaster()                  const { return _master;                  }
const std::string &ModInfo::getRequiredMasterVersion()   const { return _requiredMasterModVersion.first; }
bool               ModInfo::isMaster()                   const { return _isMaster;                }
bool               ModInfo::isEngineOk()                 const { return _engineOk;                }
const std::string &ModInfo::getRequiredExtendedEngine()  const { return _requiredExtendedEngine;  }
const std::string &ModInfo::getRequiredExtendedVersion() const { return _requiredExtendedVersion; }
const std::string &ModInfo::getResourceConfigFile()      const { return _resourceConfigFile;      }
int                ModInfo::getReservedSpace()           const { return _reservedSpace;           }

/**
 * Is parent mod is in required version?
 */
bool ModInfo::isParentMasterOk(const ModInfo* parentMod) const
{
	return _requiredMasterModVersion.first == "" || compareVersions(parentMod->_version, _requiredMasterModVersion);
}

/**
 * Checks if a given mod can be activated.
 * It must either be:
 * - a Master mod
 * - a standalone mod (no master)
 * - depend on the current Master mod
 * @param curMaster Id of the active master mod.
 * @return True if it's activable, false otherwise.
*/
bool ModInfo::canActivate(const std::string &curMaster) const
{
	return (isMaster() || getMaster().empty() || getMaster() == curMaster);
}


const std::vector<std::string> &ModInfo::getExternalResourceDirs() const { return _externalResourceDirs; }



#ifndef NDEBUG

static auto dummy = ([]
{
	auto create = [](int i, int j, int k, int l)
	{
		return std::array<int, 4>{{i, j, k, l}};
	};

	assert(findCompatibleEngine(supportedEngines, "Extended", create(OPENXCOM_VERSION_NUMBER)));
	assert(findCompatibleEngine(supportedEngines, "Extended", create(1, 0, 0, 0)));
	assert(findCompatibleEngine(supportedEngines, "", create(0, 0, 0, 0)));
	assert(!findCompatibleEngine(supportedEngines, "Extended", create(OPENXCOM_VERSION_NUMBER + 1)));
	assert(!findCompatibleEngine(supportedEngines, "XYZ", create(OPENXCOM_VERSION_NUMBER)));
	assert(!findCompatibleEngine(supportedEngines, "XYZ", create(0, 0, 0, 0)));


	auto check = [](const std::string& a, const std::string& b)
	{
		ModInfoVersion aa = normalizeModVersion("x", a);
		ModInfoVersion bb = normalizeModVersion("x", b);

		assert(aa.second != ModInfoNormalizedVersion());
		assert(bb.second != ModInfoNormalizedVersion());

		return compareVersions(aa, bb);
	};

	assert(normalizeModVersion("x", "") == ModInfoVersion());

	assert(check("A", "1"));
	assert(!check("1", "A"));

	assert(check("A0", "A.0"));
	assert(check("A.0", "A0"));

	assert(check("A1", "A.0"));
	assert(!check("A.0", "A1"));

	assert(check("A0.0", "A.0"));

	assert(check("B", "A.0"));
	assert(!check("A.0", "B"));

	assert(check("BA", "B"));

	assert(check("A11", "A2"));

	assert(!check("0000", "0001"));
	assert(check("0001", "0000"));
	assert(check("0001", "0000000"));
	assert(check("1", "0000000"));

	assert(check("1", "0000001"));
	assert(check("0001", "0000001"));
	assert(check("0001", "1"));

	assert(check("A1", "A0000001"));
	assert(check("A0001", "A0000001"));
	assert(check("A0001", "A1"));

	assert(check("10001", "0000"));
	assert(!check("0000", "10001"));

	assert(check("1.0", "1"));
	assert(check("1", "1.0"));
	assert(check("1", "1.0.0.0"));
	assert(check("1", "1.0.000.0"));


	assert(!check("1", "1.0.000.0.1"));

	return 0;
})();
#endif

}
