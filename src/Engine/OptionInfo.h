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
#include <string>
#include <map>
#include <SDL.h>

namespace OpenXcom
{

enum OptionType { OPTION_BOOL, OPTION_INT, OPTION_STRING, OPTION_KEY };
enum OptionOwner { OPTION_OXC, OPTION_OXCE, OPTION_OTHER, OPTION_OWNER_MAX };

/**
 * Helper class that ties metadata to particular options to help in serializing
 * and stuff. The option variable must already exist, this info just points to it.
 * Does some special shenanigans to be able to be tied to different variable types.
 */
class OptionInfo
{
private:
	std::string _id, _desc, _cat;
	OptionType _type;
	OptionOwner _owner;
	union { bool *b; int *i; std::string *s; SDLKey *k; } _ref;
	union { bool b; int i; const char *s; SDLKey k; } _def; // can't put strings in unions
public:
	/// Creates a bool option.
	OptionInfo(OptionOwner owner, const std::string &id, bool *option, bool def, const std::string &desc = "", const std::string &cat = "");
	/// Creates a int option.
	OptionInfo(OptionOwner owner, const std::string &id, int *option, int def, const std::string &desc = "", const std::string &cat = "");
	/// Creates a key option.
	OptionInfo(OptionOwner owner, const std::string &id, SDLKey *option, SDLKey def, const std::string &desc = "", const std::string &cat = "");
	/// Creates a string option.
	OptionInfo(OptionOwner owner, const std::string &id, std::string *option, const char *def, const std::string &desc = "", const std::string &cat = "");
	/// Gets a bool option pointer.
	bool *asBool() const;
	/// Gets an int option pointer.
	int *asInt() const;
	/// Gets a string option pointer.
	std::string *asString() const;
	/// Gets a key option pointer.
	SDLKey *asKey() const;
	/// Loads the option from YAML.
	void load(const YAML::YamlNodeReader& reader) const;
	/// Loads the option from a map.
	void load(const std::map<std::string, std::string> &map, bool makeLowercase) const;
	/// Saves the option to YAML.
	void save(YAML::YamlNodeWriter writer) const;
	/// Resets the option to default.
	void reset() const;
	/// Gets the option ID.
	const std::string& id() const { return _id; }
	/// Gets the option type.
	OptionType type() const { return _type; }
	/// Gets the option owner.
	OptionOwner owner() const { return _owner; }
	/// Gets the option description.
	const std::string& description() const { return _desc; }
	/// Gets the option category.
	const std::string& category() const { return _cat; }
};

}
