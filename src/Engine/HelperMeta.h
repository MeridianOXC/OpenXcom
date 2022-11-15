#pragma once
/*
 * Copyright 2010-2015 OpenXcom Developers.
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

namespace OpenXcom
{

namespace helper
{

/**
 * Tag class used for function tag dispatch based on type.
 */
template<typename T>
struct TypeTag
{
	using type = T;
};

/**
 * Tag class used for function tag dispatch based on int value.
 */
template<int I>
struct PosTag
{
	static constexpr int value = I;
};

/**
 * Tag class with list of integers.
 */
template<int... I>
struct ListTag
{
	static constexpr int size = sizeof...(I);
};

/**
 * Implementation used to create ListTag.
 */
template<typename PT>
struct ImplMakeListTagAdd;

/**
 * Implementation used to create ListTag.
 */
template<int... I>
struct ImplMakeListTagAdd<ListTag<I...>>
{
	using type = ListTag<I..., sizeof...(I)>;
};

/**
 * Implementation used to create ListTag.
 */
template<int I>
struct ImplMakeListTag
{
	using type = typename ImplMakeListTagAdd<typename ImplMakeListTag<I - 1>::type>::type;
};

/**
 * Implementation used to create ListTag.
 */
template<>
struct ImplMakeListTag<0>
{
	using type = ListTag<>;
};

/**
 * Create ListTag with template parameters starting from `0` to `I - 1`.
 */
template<int I>
using MakeListTag = typename ImplMakeListTag<I>::type;

/**
 * Remove all cv like const or volatile
 */
template<typename T>
using Decay = typename std::decay<T>::type;

/**
 * Helper used to delay static assertion error test.
 */
template<typename>
struct StaticError
{
	static constexpr bool value = false;
};


} //namespace helper

} //namespace OpenXcom
