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
#include "OptionInfo.h"
#include "Exception.h"
#include <sstream>

namespace OpenXcom
{

/**
 * Creates info for a boolean option.
 * @param owner Owner ID.
 * @param id String ID used in serializing.
 * @param option Pointer to the option.
 * @param def Default option value.
 * @param desc Language ID for the option description (if any).
 * @param cat Language ID for the option category (if any).
 */
OptionInfo::OptionInfo(OptionOwner owner, const std::string &id, bool *option, bool def, const std::string &desc, const std::string &cat) :
	_id(id), _desc(desc), _cat(cat), _type(OPTION_BOOL), _owner(owner)
{
	_ref.b = option;
	_def.b = def;
}

/**
 * Creates info for an integer option.
 * @param owner Owner ID.
 * @param id String ID used in serializing.
 * @param option Pointer to the option.
 * @param def Default option value.
 * @param desc Language ID for the option description (if any).
 * @param cat Language ID for the option category (if any).
 */
OptionInfo::OptionInfo(OptionOwner owner, const std::string &id, int *option, int def, const std::string &desc, const std::string &cat) :
	_id(id), _desc(desc), _cat(cat), _type(OPTION_INT), _owner(owner)
{
	_ref.i = option;
	_def.i = def;
}

/**
 * Creates info for a keyboard shortcut option.
 * @param owner Owner ID.
 * @param id String ID used in serializing.
 * @param option Pointer to the option.
 * @param def Default option value.
 * @param desc Language ID for the option description (if any).
 * @param cat Language ID for the option category (if any).
 */
OptionInfo::OptionInfo(OptionOwner owner, const std::string &id, SDLKey *option, SDLKey def, const std::string &desc, const std::string &cat) :
	_id(id), _desc(desc), _cat(cat), _type(OPTION_KEY), _owner(owner)
{
	_ref.k = option;
	_def.k = def;
}

/**
 * Creates info for a string option.
 * @param owner Owner ID.
 * @param id String ID used in serializing.
 * @param option Pointer to the option.
 * @param def Default option value.
 * @param desc Language ID for the option description (if any).
 * @param cat Language ID for the option category (if any).
 */
OptionInfo::OptionInfo(OptionOwner owner, const std::string &id, std::string *option, const char *def, const std::string &desc, const std::string &cat) :
	_id(id), _desc(desc), _cat(cat), _type(OPTION_STRING), _owner(owner)
{
	_ref.s = option;
	_def.s = def;
}

/**
 * Loads an option value from the corresponding YAML.
 * @param node Options YAML node.
 */
void OptionInfo::load(const YAML::YamlNodeReader& reader) const
{
	switch (_type)
	{
	case OPTION_BOOL:
		*(_ref.b) = reader[ryml::to_csubstr(_id)].readVal(_def.b);
		break;
	case OPTION_INT:
		*(_ref.i) = reader[ryml::to_csubstr(_id)].readVal(_def.i);
		break;
	case OPTION_KEY:
		*(_ref.k) = (SDLKey)reader[ryml::to_csubstr(_id)].readVal((int)_def.k);
		if (*(_ref.k) == SDLK_LSHIFT || *(_ref.k) == SDLK_LALT || *(_ref.k) == SDLK_LCTRL ||
			*(_ref.k) == SDLK_RSHIFT || *(_ref.k) == SDLK_RALT || *(_ref.k) == SDLK_RCTRL)
		{
			*(_ref.k) = SDLK_UNKNOWN;
		}
		break;
	case OPTION_STRING:
		*(_ref.s) = reader[ryml::to_csubstr(_id)].readVal<std::string>(_def.s);
		break;
	}
}

/**
 * Loads an option value from the corresponding map
 * (eg. for command-line options).
 * @param map Options map.
 */
void OptionInfo::load(const std::map<std::string, std::string> &map, bool makeLowercase) const
{
	std::string id = _id;
	if (makeLowercase)
	{
		std::transform(id.begin(), id.end(), id.begin(), ::tolower);
	}
	auto it = map.find(id);
	if (it != map.end())
	{
		std::string value = it->second;
		std::stringstream ss;
		bool b;
		int i;
		switch (_type)
		{
		case OPTION_BOOL:
			ss << std::boolalpha << value;
			ss >> std::boolalpha >> b;
			*(_ref.b) = b;
			break;
		case OPTION_INT:
			ss << std::dec << value;
			ss >> std::dec >> i;
			*(_ref.i) = i;
			break;
		case OPTION_KEY:
			ss << std::dec << value;
			ss >> std::dec >> i;
			*(_ref.k) = (SDLKey)i;
			break;
		case OPTION_STRING:
			*(_ref.s) = value;
			break;
		}
	}
}

/**
 * Saves an option value to the corresponding YAML.
 * @param node Options YAML node.
 */
void OptionInfo::save(YAML::YamlNodeWriter writer) const
{
	switch (_type)
	{
	case OPTION_BOOL:
		writer.write(writer.saveString(_id), *(_ref.b));
		break;
	case OPTION_INT:
		writer.write(writer.saveString(_id), *(_ref.i));
		break;
	case OPTION_KEY:
		writer.write(writer.saveString(_id), (int)*(_ref.k));
		break;
	case OPTION_STRING:
		writer.write(writer.saveString(_id), *(_ref.s));
		break;
	}
}

/**
 * Resets an option back to its default value.
 */
void OptionInfo::reset() const
{
	switch (_type)
	{
	case OPTION_BOOL:
		*(_ref.b) = _def.b;
		break;
	case OPTION_INT:
		*(_ref.i) = _def.i;
		break;
	case OPTION_KEY:
		*(_ref.k) = _def.k;
		break;
	case OPTION_STRING:
		*(_ref.s) = _def.s;
		break;
	}
}

/**
 * Returns the pointer to the boolean option,
 * or throws an exception if it's not a boolean.
 * @return Pointer to the option.
 */
bool *OptionInfo::asBool() const
{
	if (_type != OPTION_BOOL)
	{
		throw Exception(_id + " is not a boolean!");
	}
	return _ref.b;
}

/**
 * Returns the pointer to the integer option,
 * or throws an exception if it's not a integer.
 * @return Pointer to the option.
 */
int *OptionInfo::asInt() const
{
	if (_type != OPTION_INT)
	{
		throw Exception(_id + " is not an integer!");
	}
	return _ref.i;
}

/**
 * Returns the pointer to the key option,
 * or throws an exception if it's not a key.
 * @return Pointer to the option.
 */
SDLKey *OptionInfo::asKey() const
{
	if (_type != OPTION_KEY)
	{
		throw Exception(_id + " is not a key!");
	}
	return _ref.k;
}

/**
 * Returns the pointer to the string option,
 * or throws an exception if it's not a string.
 * @return Pointer to the option.
 */
std::string *OptionInfo::asString() const
{
	if (_type != OPTION_STRING)
	{
		throw Exception(_id + " is not a string!");
	}
	return _ref.s;
}

}
