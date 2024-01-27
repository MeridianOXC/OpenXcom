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
#include <algorithm>
#include <cfloat>
#define _USE_MATH_DEFINES
#include <cmath>
#include <assert.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616
#endif

// Float operations

inline bool AreSame(float l, float r)
{
	return std::fabs(l-r) <= FLT_EPSILON * std::max(1.0f, std::max(std::fabs(l), std::fabs(r)));
}

inline bool AreSame(double l, double r)
{
	return std::fabs(l-r) <= DBL_EPSILON * std::max(1.0, std::max(std::fabs(l), std::fabs(r)));
}

inline float Round(float x)
{
	return x < 0.0f ? std::ceil(x - 0.5f) : std::floor(x + 0.5f);
}

inline double Round(double x)
{
	return x < 0.0 ? std::ceil(x - 0.5) : std::floor(x + 0.5);
}

// Number operations

template <class _Tx>
inline _Tx Sqr(const _Tx& x)
{
	return x * x;
}

template <class _Tx>
inline _Tx Sign(const _Tx& x)
{
	return (_Tx(0) < x) - (x < _Tx(0));
}

template <class _Tx>
inline _Tx Clamp(const _Tx& x, const _Tx& min, const _Tx& max)
{
	return std::min(std::max(x, min), max);
}

template <class _Tx, class _Ts = int>
inline _Tx VectCrossProduct(const _Tx& a, const _Tx& b, _Ts scale = 1)
{
	_Tx temp
	{
		(a.y * b.z - a.z * b.y) / scale,
		(a.z * b.x - a.x * b.z) / scale,
		(a.x * b.y - a.y * b.x) / scale,
	};
	return temp;
}

template <class _Tx, class _Ts = int>
inline _Ts VectDotProduct(const _Tx& a, const _Tx& b, _Ts scale = 1)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z) / scale;
}

template <class _Tx, class _Ts = int>
inline _Tx VectNormalize(const _Tx& a, _Ts scale = 1)
{
	_Ts dot = VectDotProduct(a, a, scale);
	if (dot > 0)
	{
		dot = std::sqrt(dot * scale);
		_Tx temp
		{
			a.x * scale / dot,
			a.y * scale / dot,
			a.z * scale / dot,
		};
		return temp;
	}
	return _Tx{ };
}

/**
 * Interpolate between two variables.
 * @param a Left side value
 * @param b Rigth side value
 * @param step Value from range 0 to `stepMax` (including)
 * @param stepMax Maximum value for `step`.
 * @return if `step` equal 0 or less then it returns `a`, if is equal `stepMax` or bigger it will return `b`. Any other value will interpolate between `a` and `b`.
 */
template <class TValue, class TStep>
inline TValue Interpolate(const TValue& a, const TValue& b, TStep step, TStep stepMax)
{
	assert((stepMax > TStep(0)) && "max should be bigger than zero");
	step = Clamp(step, TStep(0), stepMax);
	return  (a * (stepMax - step) + b * (step)) / stepMax;
}

// Degree operations

inline double Deg2Rad(double deg)
{
	return deg * M_PI / 180.0;
}

inline double Rad2Deg(double rad)
{
	return rad / M_PI * 180.0;
}

inline double Xcom2Rad(int deg)
{
	return deg * 0.125 * M_PI / 180.0;
}

inline double Nautical(double x)
{
	return x * (1 / 60.0) * (M_PI / 180.0);
}

/**
 * Inversion of Nautical distance, use same value as in rulesets definitions
 */
inline int XcomDistance(double nautical)
{
	return nautical * 60.0 * (180.0 / M_PI);
}
