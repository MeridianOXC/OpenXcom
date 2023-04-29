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
#include "../Engine/RNG.h"
#include "Particle.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Creates a particle.
 * @param xOffset the horizontal offset for this particle (relative to the tile in screen space)
 * @param yOffset the vertical offset for this particle (relative to the tile in screen space)
 * @param density the density of the particle dictates the speed at which it moves upwards, and is inversely proportionate to its size.
 * @param color the color set to use from the transparency LUTs
 * @param opacity another reference for the LUT, this one is divided by 5 for the actual offset to use.
 */
Particle::Particle(Position voxelPos, Uint8 density, Uint8 color, Uint8 opacity) : _density(density), _color(color), _opacity(opacity), _size(0)
{
	_layerZ = (voxelPos.z / Position::TileZ) << 1;

	_subVoxelPos = voxelPos.clipVoxel() * SubVoxelAccuracy;

	// approximation of old `int offset = RNG::seedless(0, 4) - 2;`
	const int offset = SubVoxelAccuracy * 3 / 2;
	_subVoxelPos.x += RNG::seedless(-offset, +offset);
	_subVoxelPos.y += RNG::seedless(-offset, +offset);
	_subVoxelPos.z += RNG::seedless(-offset, +offset);

	//size is initialized at 0
	if (density < 100)
	{
		_size = 3;
	}
	else if (density < 125)
	{
		_size = 2;
	}
	else if (density < 150)
	{
		_size = 1;
	}
}

/**
 * Animates the particle.
 * @return if we are done animating this particle yet.
 */
bool Particle::animate()
{
	_subVoxelPos.z += (320-_density);
	_opacity--;

	// approximation of old `_xOffset += (RNG::seedless(0,1)*2 -1)* (0.25 + (float)RNG::seedless(0,9)/30);`
	const int drift = SubVoxelAccuracy / 2;
	_subVoxelPos.x += RNG::seedless(-drift, drift);
	_subVoxelPos.y += RNG::seedless(-drift, drift);
	_subVoxelPos.z += RNG::seedless(-drift, drift);

	if ( _opacity == 0 )
	{
		return false;
	}
	return true;
}

/**
 * Update relative screen position of particle.
 * @return Offset to next tile if particle cross tile boundaries.
 */
Position Particle::updateScreenPosition()
{
	static constexpr Position one = Position(1, 1, 1);
	static constexpr Position scale = one.toVoxel() * SubVoxelAccuracy;

	// this convert postion to -1, 0 or +1 depending if _subVoxelPos is outside current tile
	Position tileOffset = ((_subVoxelPos + scale) /  scale - one);

	// keep values inside one tile
	if (tileOffset.x)
	{
		_subVoxelPos.x -= tileOffset.x * Position::TileXY * SubVoxelAccuracy;
	}
	if (tileOffset.y)
	{
		_subVoxelPos.y -= tileOffset.y * Position::TileXY * SubVoxelAccuracy;
	}
	if (tileOffset.z)
	{
		_subVoxelPos.z -= tileOffset.z * Position::TileZ * SubVoxelAccuracy;
		_layerZ += tileOffset.z * LayerAccuracy;
	}

	// voxel closer to front of screen are consider on higher layer
	_layerZ &= 0xFE; //cut smallest bit
	_layerZ |= (_subVoxelPos.x + _subVoxelPos.y > Position::TileXY*SubVoxelAccuracy);

	Position v = _subVoxelPos / SubVoxelAccuracy;
	_screenData.x = v.x - v.y;
	_screenData.y = (v.x / 2) + (v.y / 2) - v.z - getTileZ() * Position::TileZ;
	_screenData.z = std::min((_opacity + 7) / 10, Mod::TransparenciesOpacityLevels - 1);

	return tileOffset;
}

}
