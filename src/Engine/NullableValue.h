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

#include <SDL_stdinc.h>
#include <limits>


namespace OpenXcom
{


/**
 * Simpler version of std::optional, that sacrifice one possible value for null
 */
template<typename T>
class NullableValue
{
	T value;

public:
	/// Default constructor.
	NullableValue() : value{ std::numeric_limits<T>::min() }
	{

	}
	/// Constructor.
	NullableValue(T t) : value{ t }
	{

	}

	/// Current value is null.
	bool isNull() const
	{
		return value == std::numeric_limits<T>::min();
	}
	/// Have value.
	bool isValue() const
	{
		return !isNull();
	}


	/// Set new value.
	void setValue(T t)
	{
		value = t;
	}
	/// Set null.
	void setNull()
	{
		value = std::numeric_limits<T>::min();
	}

	/// Get value of default.
	T getValueOrDefualt() const
	{
		return isNull() ? T{ } : value;
	}

	/// Get value or fallback value.
	T getValueOr(T fallback) const
	{
		return isNull() ? fallback : value;
	}
};

/**
 * Simpler version of std::optional, optimized for `bool`
 */
template<>
class NullableValue<bool>
{
	Sint8 value;

public:
	/// Default constructor.
	NullableValue() : value{ -1 }
	{

	}
	/// Constructor.
	NullableValue(bool t) : value{ t }
	{

	}

	/// Current value is null.
	bool isNull() const
	{
		return value == -1;
	}
	/// Have value.
	bool isValue() const
	{
		return !isNull();
	}


	/// Set new value.
	void setValue(bool t)
	{
		value = t;
	}
	/// Set null.
	void setNull()
	{
		value = -1;
	}

	/// Get value of default.
	bool getValueOrDefualt() const
	{
		return isNull() ? false : value;
	}
};

template<typename T, typename... Rest>
T coalesceNullValues(NullableValue<T> t, Rest... rest)
{
	if constexpr (sizeof...(rest) > 0)
	{
		if (t.isNull())
		{
			return coalesceNullValues(rest...);
		}
	}

	return t.getValueOrDefualt();
}



} //namespace OpenXcom
