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

namespace OpenXcom
{

/**
 * Low overhead callback.
 *
 * Base template, not defined anywhere.
 */
template<typename T>
class FuncRef;

/**
 * Low overhead callback.
 */
template<typename Ret, typename... Args>
class FuncRef<Ret(Args...)>
{
public:
	using ApiFunc = Ret(*)(Args...);
	using InternalFunc = Ret(*)(Args..., void*);

	/**
	 * A constructor that takes a reference to some temporary callable object.
	 */
	template<typename F>
	FuncRef(F&& f) :FuncRef{ +[](Args... args, void* p){ return std::invoke(*static_cast<std::remove_reference_t<F>*>(p), std::forward<Args>(args)...); }, &f }
	{

	}

	/**
	 * Copy constructor.
	 * @param f
	 */
	FuncRef(const FuncRef& f) = default;

	/**
	 * Move constructor.
	 * @param f
	 */
	FuncRef(FuncRef&& f) = default;

	/**
	 * A constructor that takes a pointer to some callable object.
	 */
	template<typename P>
	constexpr FuncRef(P* f) : FuncRef{ +[](Args... args, void* p){ return std::invoke(*reinterpret_cast<P*>(p), std::forward<Args>(args)...); }, reinterpret_cast<void*>(f) }
	{
		static_assert(sizeof(f) == sizeof(_data), "This code works only if a void pointer has the same size as the given pointer (in special case function pointer)");
	}

	/**
	 * Invalid constructor from empty pointer.
	 */
	FuncRef(std::nullptr_t) = delete;

	/**
	 * Helper constructor for minimum overhead functions.
	 * @param func Internal function to call.
	 * @param data State that is passed to the internal function call.
	 */
	constexpr FuncRef(InternalFunc func, void* data) : _func{ func }, _data{ data }
	{

	}

	/**
	 * Call operator.
	 */
	constexpr Ret operator()(Args... args) const
	{
		return _func(std::forward<Args>(args)..., _data);
	}

	/**
	 * bool operator
	 * @return always true as it can't be empty.
	 */
	constexpr explicit operator bool()
	{
		return true;
	}

private:

	InternalFunc _func;
	void* _data;
};


} //namespace OpenXcom
