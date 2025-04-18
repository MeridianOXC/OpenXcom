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
#include "MovingTarget.h"
#include "../fmath.h"
#include "SerializationHelper.h"
#include "../Engine/Options.h"

namespace OpenXcom
{

/**
 * Initializes a moving target with blank coordinates.
 */
MovingTarget::MovingTarget() : Target(), _dest(0), _speedLon(0.0), _speedLat(0.0), _speedRadian(0.0), _meetPointLon(0.0), _meetPointLat(0.0), _speed(0), _meetCalculated(false)
{
}

/**
 * Make sure to cleanup the target's destination followers.
 */
MovingTarget::~MovingTarget()
{
	setDestination(0);
}

/**
 * Loads the moving target from a YAML file.
 * @param node YAML node.
 */
void MovingTarget::load(const YAML::YamlNodeReader& reader)
{
	Target::load(reader);
	reader.tryRead("speedLon", _speedLon);
	reader.tryRead("speedLat", _speedLat);
	reader.tryRead("speedRadian", _speedRadian);
	reader.tryRead("speed", _speed);
}

/**
 * Saves the moving target to a YAML file.
 * @return YAML node.
 */
void MovingTarget::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	Target::save(writer);
	if (_dest)
		_dest->saveId(writer["dest"]);
	writer.write("speedLon", serializeDouble(_speedLon));
	writer.write("speedLat", serializeDouble(_speedLat));
	writer.write("speedRadian", serializeDouble(_speedRadian));
	writer.write("speed", _speed);
}

/**
 * Returns the destination the moving target is heading to.
 * @return Pointer to destination.
 */
Target *MovingTarget::getDestination() const
{
	return _dest;
}

/**
 * Changes the destination the moving target is heading to.
 * @param dest Pointer to destination.
 */
void MovingTarget::setDestination(Target *dest)
{
	_meetCalculated = false;
	// Remove moving target from old destination's followers
	if (_dest != 0)
	{
		for (auto iter = _dest->getFollowers()->begin(); iter != _dest->getFollowers()->end(); ++iter)
		{
			if ((*iter) == this)
			{
				_dest->getFollowers()->erase(iter);
				break;
			}
		}
	}
	_dest = dest;
	// Add moving target to new destination's followers
	if (_dest != 0)
	{
		_dest->getFollowers()->push_back(this);
	}
	// Recalculate meeting point for any followers
	for (auto* mt : *getFollowers())
	{
		mt->resetMeetPoint();
	}
	calculateSpeed();
}

/**
 * Returns the speed of the moving target.
 * @return Speed in knots.
 */
int MovingTarget::getSpeed() const
{
	return _speed;
}

/**
 * Returns the radial speed of the moving target.
 * @return Speed in 1 / 5 sec.
 */
double MovingTarget::getSpeedRadian() const
{
	return _speedRadian;
}

/**
 * Converts a speed in degrees to a speed in radians.
 * Each nautical mile is 1/60th of a degree.
 * Each hour contains 720 5-seconds.
 * @param speed Speed in degrees.
 * @return Speed in radians.
 */
double MovingTarget::calculateRadianSpeed(int speed)
{
	return Nautical(speed) / 720.0;
}

/**
 * Changes the speed of the moving target
 * and converts it from standard knots (nautical miles per hour)
 * into radians per 5 in-game seconds.
 * @param speed Speed in knots.
 */
void MovingTarget::setSpeed(int speed)
{
	_speed = speed;
	_speedRadian = calculateRadianSpeed(_speed);
	// Recalculate meeting point for any followers
	for (auto* mt : *getFollowers())
	{
		mt->resetMeetPoint();
	}
	calculateSpeed();
}

/**
 * Calculates the speed vector based on the
 * great circle distance to destination and
 * current raw speed.
 */
void MovingTarget::calculateSpeed()
{
	calculateMeetPoint();
	if (_dest != 0)
	{
		double dLon, dLat, length;
		dLon = sin(_meetPointLon - _lon) * cos(_meetPointLat);
		dLat = cos(_lat) * sin(_meetPointLat) - sin(_lat) * cos(_meetPointLat) * cos(_meetPointLon - _lon);
		length = sqrt(dLon * dLon + dLat * dLat);
		_speedLat = dLat / length * _speedRadian;
		_speedLon = dLon / length * _speedRadian / cos(_lat + _speedLat);

		// Check for invalid speeds when a division by zero occurs due to near-zero values
		if (!(_speedLon == _speedLon) || !(_speedLat == _speedLat))
		{
			_speedLon = 0;
			_speedLat = 0;
		}
	}
	else
	{
		_speedLon = 0;
		_speedLat = 0;
	}
}

/**
 * Checks if the moving target has reached its destination.
 * @return True if it has, False otherwise.
 */
bool MovingTarget::reachedDestination() const
{
	if (_dest == 0)
	{
		return false;
	}
	return ( AreSame(_dest->getLongitude(), _lon) && AreSame(_dest->getLatitude(), _lat) );
}

/**
 * Executes a movement cycle for the moving target.
 */
void MovingTarget::move()
{
	calculateSpeed();
	if (_dest != 0)
	{
		if (getDistance(_meetPointLon, _meetPointLat) > _speedRadian)
		{
			setLongitude(_lon + _speedLon);
			setLatitude(_lat + _speedLat);
		}
		else
		{
			if (getDistance(_dest) > _speedRadian)
			{
				setLongitude(_meetPointLon);
				setLatitude(_meetPointLat);
			}
			else
			{
				setLongitude(_dest->getLongitude());
				setLatitude(_dest->getLatitude());
			}
			resetMeetPoint();
		}
	}
}

/**
 * Calculate meeting point with the target.
 */
void MovingTarget::calculateMeetPoint()
{
#if 0
	if (!Options::meetingPoint) _meetCalculated = false;
	if (_meetCalculated) return;
#endif
	_meetCalculated = false;

	// Initialize
	if (_dest != 0)
	{
		_meetPointLat = _dest->getLatitude();
		_meetPointLon = _dest->getLongitude();
	}
	else
	{
		_meetPointLat = _lat;
		_meetPointLon = _lon;
	}

	// ***IMPORTANT*** this functionality has been disabled until further notice, most probably forever
#if 0

	if (!_dest || !Options::meetingPoint || reachedDestination()) return;

	MovingTarget *t = dynamic_cast<MovingTarget*>(_dest);
	if (!t || !t->getDestination()) return;

	// Speed ratio
	if (AreSame(t->getSpeedRadian(), 0.0)) return;
	const double speedRatio = _speedRadian/ t->getSpeedRadian();

	// The direction pseudovector
	double	nx = cos(t->getLatitude())*sin(t->getLongitude())*sin(t->getDestination()->getLatitude()) -
					sin(t->getLatitude())*cos(t->getDestination()->getLatitude())*sin(t->getDestination()->getLongitude()),
			ny = sin(t->getLatitude())*cos(t->getDestination()->getLatitude())*cos(t->getDestination()->getLongitude()) -
					cos(t->getLatitude())*cos(t->getLongitude())*sin(t->getDestination()->getLatitude()),
			nz = cos(t->getLatitude())*cos(t->getDestination()->getLatitude())*sin(t->getDestination()->getLongitude() - t->getLongitude());
	// Normalize and multiplex with radian speed
	double	nk = _speedRadian/sqrt(nx*nx+ny*ny+nz*nz);
	nx *= nk;
	ny *= nk;
	nz *= nk;

	// Finding the meeting point. Don't search further than halfway across the
	// globe (distance from interceptor's current point >= 1), as that may
	// cause the interceptor to go the wrong way later.
	for (double path = 0, distance = 1;
		path < M_PI && distance - path*speedRatio > 0 && path*speedRatio < 1;
		path += _speedRadian)
	{
		_meetPointLat += nx*sin(_meetPointLon) - ny*cos(_meetPointLon);
		if (std::abs(_meetPointLat) < M_PI_2) _meetPointLon += nz - (nx*cos(_meetPointLon) + ny*sin(_meetPointLon))*tan(_meetPointLat); else _meetPointLon += M_PI;

		distance = acos(cos(_lat) * cos(_meetPointLat) * cos(_meetPointLon - _lon) + sin(_lat) * sin(_meetPointLat));
	}

	// Correction overflowing angles
	double lonSign = Sign(_meetPointLon);
	double latSign = Sign(_meetPointLat);
	while (std::abs(_meetPointLon) > M_PI) _meetPointLon -= lonSign * 2 * M_PI;
	while (std::abs(_meetPointLat) > M_PI) _meetPointLat -= latSign * 2 * M_PI;
	if (std::abs(_meetPointLat) > M_PI_2) { _meetPointLat = latSign * std::abs(2 * M_PI - std::abs(_meetPointLat)); _meetPointLon -= lonSign * M_PI; }

	_meetCalculated = true;
#endif
}

/**
 * Returns the latitude of the meeting point.
 * @return Angle in rad.
 */
double MovingTarget::getMeetLatitude() const
{
	return _meetPointLat;
}

/**
 * Returns the longitude of the meeting point.
 * @return Angle in rad.
 */
double MovingTarget::getMeetLongitude() const
{
	return _meetPointLon;
}

/**
 * Forces the meeting point to be recalculated in the event
 * that the target has changed direction.
 */
void MovingTarget::resetMeetPoint()
{
	_meetCalculated = false;
}

bool MovingTarget::isMeetCalculated() const
{
	return _meetCalculated;
}

}
