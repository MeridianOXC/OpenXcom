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
#include <SDL_types.h>
#include "Position.h"

namespace OpenXcom
{

class Particle
{
public:
	constexpr static int SubVoxelAccuracy = 256;
	constexpr static int LayerAccuracy = 2;

private:
	Position _subVoxelPos;
	Position _screenData;
	Position _subVoxelVelocity;
	Position _subVoxelAcceleration;
	Uint8 _drift;
	Uint8 _layerZ;
	Uint8 _color, _opacity, _size;
public:
	/// Create a particle.
	Particle(Position voxelPos, Position subVoxelOffset, Position subVoxelVel, Position subVoxelAcc, Uint8 drift, Uint8 color, Uint8 opacity, Uint8 size);
	/// Default copy constructor
	Particle(const Particle&) = default;
	/// Default move constructor
	Particle(Particle&&) = default;
	/// Copy assignment.
	Particle& operator=(const Particle&) = default;
	/// Move assignment.
	Particle& operator=(Particle&&) = default;
	/// Destroy a particle.
	~Particle() = default;
	/// Animate a particle.
	bool animate();
	/// Update screen data.
	Position updateScreenPosition();
	/// Get the size value.
	int getSize() const { return _size; }
	/// Get the color.
	Uint8 getColor() const { return _color; }
	/// Get the opacity.
	Uint8 getOpacity() const { return _screenData.z; }
	/// Gets screen offset X relative to tile.
	int getOffsetX() const { return _screenData.x; }
	/// Gets screen offset Y relative to tile.
	int getOffsetY() const { return _screenData.y; }
	/// Get layer position of particle.
	int getLayerZ() const { return _layerZ; }
	/// Get tile position of particle.
	int getTileZ() const { return _layerZ / LayerAccuracy; }
};

}
