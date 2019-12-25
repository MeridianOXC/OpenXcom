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
#include <algorithm>

namespace OpenXcom
{

class Particle
{
private:
	float _xOffset, _yOffset, _density;
	Uint16 _voxelZ;
	Uint8 _color, _opacity, _size;
public:
	/// Create a particle.
	Particle(int voxelZ, float xOffset, float yOffset, float density, Uint8 color, Uint8 opacity);
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
	/// Get the size value.
	int getSize() const { return _size; }
	/// Get the color.
	Uint8 getColor() const { return _color; }
	/// Get the opacity.
	Uint8 getOpacity() const { return std::min((_opacity + 7) / 10, 3); }
	/// Get the horizontal shift.
	float getX() const { return _xOffset; }
	/// Get the vertical shift.
	float getY() const { return _yOffset; }
	/// Get voxel position of particle.
	int getVoxelZ() const { return _voxelZ; }
};

}
