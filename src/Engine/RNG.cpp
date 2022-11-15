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
#include "RNG.h"
#include <time.h>
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL
#endif

namespace OpenXcom
{
namespace RNG
{

/*  Written in 2014 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is a good generator if you're short on memory, but otherwise we
   rather suggest to use a xorshift128+ (for maximum speed) or
   xorshift1024* (for speed and very long period) generator. */

static uint64_t nextImpl(uint64_t& state)
{
	state ^= state >> 12; // a
	state ^= state << 25; // b
	state ^= state >> 27; // c
	return state * 2685821657736338717ULL;
}




/**
 * Default constructor initializing the seed by time and this type address.
 */
RandomState::RandomState()
{
	_seedState = time(0) ^ (~(uint64_t)&_seedState);
}

/**
 * Constructor from predefined seed.
 */
RandomState::RandomState(uint64_t seed) : _seedState(seed)
{

}

/**
 * Returns the current seed in use by the generator.
 * @return Current seed.
 */
uint64_t RandomState::getSeed() const
{
	return _seedState;
}

/**
 * Get next random number.
 * @return Random number.
 */
uint64_t RandomState::next()
{
	return nextImpl(_seedState);
}

/**
 * Generates a random integer number, inclusive.
 */
int RandomState::generate(int min, int max)
{
	return (int)(next() % (max - min + 1) + min);
}



/**
 * State for game random number generator. Do not use during other variable static initialization because: https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use-members
 */
RandomState x;

/**
 * Separate state for some auxiliary random numbers that do not affect game state. Do not use during other variable static initialization because: https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use-members
 */
RandomState x_seedless;




/**
 * Returns the current seed in use by the generator.
 * @return Current seed.
 */
uint64_t getSeed()
{
	return x.getSeed();
}

/**
 * Changes the current seed in use by the generator.
 * @param n New seed.
 */
void setSeed(uint64_t n)
{
	x = RandomState(n);
}

/**
 * State
 */
RandomState& globalRandomState()
{
	return x;
}

/**
 * Generates a random integer number within a certain range.
 * @param min Minimum number, inclusive.
 * @param max Maximum number, inclusive.
 * @return Generated number.
 */
int generate(int min, int max)
{
	return x.generate(min, max);
}

/**
 * Generates a random decimal number within a certain range.
 * @param min Minimum number.
 * @param max Maximum number.
 * @return Generated number.
 */
double generate(double min, double max)
{
	double num = x.next();
	return (num / ((double)UINT64_MAX / (max - min)) + min);
}

/**
 * Generates a random integer number within a certain range.
 * Distinct from "generate" in that it doesn't touch the seed.
 * @param min Minimum number, inclusive.
 * @param max Maximum number, inclusive.
 * @return Generated number.
 */
int seedless(int min, int max)
{
	return x_seedless.generate(min, max);
}

/**
 * Generates a random percent chance of an event occurring,
 * and returns the result
 * @param value Value percentage (0-100%)
 * @return True if the chance succeeded.
 */
bool percent(int value)
{
	return x.percent(value);
}

}

}
