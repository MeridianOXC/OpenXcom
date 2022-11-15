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
#include "Surface.h"
#include "GraphSubset.h"

namespace OpenXcom
{

namespace helper
{

/***
 * Calculate pointer offset using bytes
 */
template<typename Ptr>
Ptr* pointerByteOffset(Ptr* base, int offset)
{
	return reinterpret_cast<Ptr*>(reinterpret_cast<unsigned char*>(base) + offset);
}

/***
 * Calculate pointer offset using bytes
 */
template<typename Ptr>
const Ptr* pointerByteOffset(const Ptr* base, int offset)
{
	return reinterpret_cast<const Ptr*>(reinterpret_cast<const unsigned char*>(base) + offset);
}

/**
 * This is scalar argument to `ShaderDraw`.
 * when used in `ShaderDraw` return value of `t` to `ColorFunc::func` for every pixel
 */
template<typename T>
class Scalar
{
public:
	T& ref;
	inline Scalar(T& t) : ref(t)
	{

	}
};


/**
 * This is surface argument to `ShaderDraw`.
 * every pixel of this surface will have type `Pixel`.
 * Modify pixels of this surface, that will modifying original data.
 */
template<typename Pixel>
class ShaderBase
{
public:
	typedef Pixel* PixelPtr;
	typedef Pixel& PixelRef;

protected:
	const PixelPtr _orgin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;

public:
	/// copy constructor
	inline ShaderBase(const ShaderBase& s):
		_orgin(s.ptr()),
		_range_base(s._range_base),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())
	{

	}

	/// copy constructor
	template<typename = std::enable_if<std::is_const<Pixel>::value, void>>
	inline ShaderBase(const ShaderBase<typename std::remove_const<Pixel>::type>& s):
		_orgin(s.ptr()),
		_range_base(s.getBaseDomain()),
		_range_domain(s.getDomain()),
		_pitch(s.pitch())
	{

	}

	/**
	 * create surface using raw surface `s` as data source.
	 * surface will have same dimensions as `s`.
	 * Attention: after use of this constructor you change size of surface `s`
	 * then `_orgin` will be invalid and use of this object will cause memory exception.
     * @param s Raw Surface
     */
	inline ShaderBase(SurfaceRaw<Pixel> s):
		_orgin(s.getBuffer()),
		_range_base(s.getWidth(), s.getHeight()),
		_range_domain(s.getWidth(), s.getHeight()),
		_pitch(s.getPitch())
	{

	}

	/// Get pointer to beginning of surface
	inline PixelPtr ptr() const
	{
		return _orgin;
	}

	/// Get real distance between lines in bytes
	inline int pitch() const
	{
		return _pitch;
	}

	inline void setDomain(const GraphSubset& g)
	{
		_range_domain = GraphSubset::intersection(g, _range_base);
	}
	inline const GraphSubset& getDomain() const
	{
		return _range_domain;
	}
	inline const GraphSubset& getBaseDomain() const
	{
		return _range_base;
	}

	inline const GraphSubset& getImage() const
	{
		return _range_domain;
	}
};


/// helper class for handling implementation differences in different surfaces types
/// Used in function `ShaderDraw`.
template<typename SurfaceType>
struct controler
{
	//NOT IMPLEMENTED ANYWHERE!
	//you need create your own specification or use different type, no default version

	/**
	 * function used only when `SurfaceType` can be used as destination surface
	 * if that type should not be used as `dest` don't implement this.
	 * @return start drawing range
	 */
	inline const GraphSubset& get_range() = delete;
	/**
	 * function used only when `SurfaceType` is used as source surface.
	 * function reduce drawing range.
	 * @param g modify drawing range
	 */
	inline void mod_range(GraphSubset& g) = delete;
	/**
	 * set final drawing range.
	 * @param g drawing range
	 */
	inline void set_range(const GraphSubset& g) = delete;

	inline void mod_y(int& begin, int& end) = delete;
	inline void set_y(const int& begin, const int& end) = delete;
	inline void inc_y() = delete;


	inline void mod_x(int& begin, int& end) = delete;
	inline void set_x(const int& begin, const int& end) = delete;
	inline void inc_x() = delete;

	inline int& get_ref() = delete;
};

/// implementation for scalars types aka `int`, `double`, `float`
template<typename T>
struct controler<Scalar<T> >
{
	T& ref;

	inline controler(const Scalar<T>& s) : ref(s.ref)
	{

	}

	//cant use this function
	//inline GraphSubset get_range()

	inline void mod_range(GraphSubset&)
	{
		//nothing
	}
	inline void set_range(const GraphSubset&)
	{
		//nothing
	}

	inline void mod_y(int&, int&)
	{
		//nothing
	}
	inline void set_y(const int&, const int&)
	{
		//nothing
	}
	inline void inc_y()
	{
		//nothing
	}


	inline void mod_x(int&, int&)
	{
		//nothing
	}
	inline void set_x(const int&, const int&)
	{
		//nothing
	}
	inline void inc_x()
	{
		//nothing
	}

	inline T& get_ref()
	{
		return ref;
	}
};

template<typename PixelPtr, typename PixelRef>
struct controler_base
{

	const PixelPtr data;
	PixelPtr ptr_pos_y;
	PixelPtr ptr_pos_x;
	GraphSubset range;
	int start_x;
	int start_y;

	const std::pair<int, int> step;


	controler_base(PixelPtr base, const GraphSubset& d, const GraphSubset& r, const std::pair<int, int>& s) :
		data(pointerByteOffset(base, d.beg_x*s.first + d.beg_y*s.second)),
		ptr_pos_y(0), ptr_pos_x(0),
		range(r),
		start_x(), start_y(),
		step(s)
	{

	}


	inline const GraphSubset& get_range()
	{
		return range;
	}

	inline void mod_range(GraphSubset& r)
	{
		r = GraphSubset::intersection(range, r);
	}

	inline void set_range(const GraphSubset& r)
	{
		start_x = r.beg_x - range.beg_x;
		start_y = r.beg_y - range.beg_y;
		range = r;
	}

	inline void mod_y(int&, int&)
	{
		ptr_pos_y = pointerByteOffset(data, step.first * start_x + step.second * start_y);
	}
	inline void set_y(const int& begin, const int&)
	{
		ptr_pos_y = pointerByteOffset(ptr_pos_y, step.second * begin);
	}
	inline void inc_y()
	{
		ptr_pos_y = pointerByteOffset(ptr_pos_y, step.second);
	}


	inline void mod_x(int&, int&)
	{
		ptr_pos_x = ptr_pos_y;
	}
	inline void set_x(const int& begin, const int&)
	{
		ptr_pos_x = pointerByteOffset(ptr_pos_x, step.first * begin);
	}
	inline void inc_x()
	{
		ptr_pos_x = pointerByteOffset(ptr_pos_x, step.first);
	}

	inline PixelRef get_ref()
	{
		return *ptr_pos_x;
	}
};



template<typename Pixel>
struct controler<ShaderBase<Pixel> > : public controler_base<typename ShaderBase<Pixel>::PixelPtr, typename ShaderBase<Pixel>::PixelRef>
{
	typedef typename ShaderBase<Pixel>::PixelPtr PixelPtr;
	typedef typename ShaderBase<Pixel>::PixelRef PixelRef;

	typedef controler_base<PixelPtr, PixelRef> base_type;

	controler(const ShaderBase<Pixel>& f) : base_type(f.ptr(), f.getDomain(), f.getImage(), std::make_pair(sizeof(Pixel), f.pitch()))
	{

	}

};

}//namespace helper

}//namespace OpenXcom

