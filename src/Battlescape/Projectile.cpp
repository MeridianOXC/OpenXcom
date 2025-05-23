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
#include "Projectile.h"
#include "TileEngine.h"
#include "Map.h"
#include "Camera.h"
#include "Particle.h"
#include "Pathfinding.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/MapData.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Sets up a UnitSprite with the specified size and position.
 * @param mod Pointer to mod.
 * @param save Pointer to battle savegame.
 * @param action An action.
 * @param origin Position the projectile originates from.
 * @param targetVoxel Position the projectile is targeting.
 * @param ammo the ammo that produced this projectile, where applicable.
 */
Projectile::Projectile(Mod *mod, SavedBattleGame *save, BattleAction action, Position origin, Position targetVoxel, BattleItem *ammo) : _mod(mod), _save(save), _action(action), _ammo(ammo), _origin(origin), _targetVoxel(targetVoxel), _position(0), _distance(0.0f), _bulletSprite(-1), _reversed(false), _vaporColor(-1), _vaporDensity(-1), _vaporProbability(5)
{
	// this is the number of pixels the sprite will move between frames
	_speed = Options::battleFireSpeed;
	if (_action.weapon)
	{
		if (_action.type != BA_THROW)
		{
			assert(_ammo && "missing ammo for Projectile");

			// try to get all the required info from the ammo
			_bulletSprite = _ammo->getRules()->getBulletSprite();
			_vaporColor = _ammo->getRules()->getVaporColor(_save->getDepth());
			_vaporDensity = _ammo->getRules()->getVaporDensity(_save->getDepth());
			_vaporProbability = _ammo->getRules()->getVaporProbability(_save->getDepth());
			_speed = std::max(1, _speed + _ammo->getRules()->getBulletSpeed());

			// the ammo didn't contain the info we wanted, see what the weapon has on offer.
			if (_bulletSprite == Mod::NO_SURFACE)
			{
				_bulletSprite = _action.weapon->getRules()->getBulletSprite();
			}
			if (_vaporColor == -1)
			{
				_vaporColor = _action.weapon->getRules()->getVaporColor(_save->getDepth());
			}
			if (_vaporDensity == -1)
			{
				_vaporDensity = _action.weapon->getRules()->getVaporDensity(_save->getDepth());
			}
			if (_vaporProbability == 5)
			{
				_vaporProbability = _action.weapon->getRules()->getVaporProbability(_save->getDepth());
			}
			if (!ammo || (ammo != _action.weapon || ammo->getRules()->getBulletSpeed() == 0))
			{
				_speed = std::max(1, _speed + _action.weapon->getRules()->getBulletSpeed());
			}
		}
	}
	if ((targetVoxel.x - origin.x) + (targetVoxel.y - origin.y) >= 0)
	{
		_reversed = true;
	}
}

/**
 * Deletes the Projectile.
 */
Projectile::~Projectile()
{

}

/**
 * Calculates the trajectory for a straight path.
 * @param accuracy The unit's accuracy.
 * @return The objectnumber(0-3) or unit(4) or out of map (5) or -1 (no line of fire).
 */

int Projectile::calculateTrajectory(double accuracy)
{
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, _save->getTile(_origin));
	return calculateTrajectory(accuracy, originVoxel);
}

int Projectile::calculateTrajectory(double accuracy, const Position& originVoxel, bool excludeUnit)
{
	Tile *targetTile = _save->getTile(_action.target);
	BattleUnit *bu = _action.actor;

	_distance = 0.0f;
	int test;
	if (excludeUnit)
	{
		test = _save->getTileEngine()->calculateLineVoxel(originVoxel, _targetVoxel, false, &_trajectory, bu);
	}
	else
	{
		test = _save->getTileEngine()->calculateLineVoxel(originVoxel, _targetVoxel, false, &_trajectory, nullptr);
	}

	if (test != V_EMPTY &&
		!_trajectory.empty() &&
		_action.actor->getFaction() == FACTION_PLAYER &&
		_action.autoShotCounter == 1 &&
		(!_save->isCtrlPressed(true) || !Options::forceFire) &&
		_save->getBattleGame()->getPanicHandled() &&
		_action.type != BA_LAUNCH &&
		!_action.sprayTargeting)
	{
		Position hitPos = _trajectory.at(0).toTile();
		if (test == V_UNIT && _save->getTile(hitPos) && _save->getTile(hitPos)->getUnit() == 0) //no unit? must be lower
		{
			hitPos = Position(hitPos.x, hitPos.y, hitPos.z-1);
		}

		if (hitPos != _action.target && _action.result.empty())
		{
			if (test == V_NORTHWALL)
			{
				if (hitPos.y - 1 != _action.target.y)
				{
					_trajectory.clear();
					return V_EMPTY;
				}
			}
			else if (test == V_WESTWALL)
			{
				if (hitPos.x - 1 != _action.target.x)
				{
					_trajectory.clear();
					return V_EMPTY;
				}
			}
			else if (test == V_UNIT)
			{
				BattleUnit *hitUnit = _save->getTile(hitPos)->getUnit();
				BattleUnit *targetUnit = targetTile->getUnit(); // Note: hitPos could be 1 tile lower and hitUnit could be on both tiles; change in OXC?
				if (hitUnit != targetUnit)
				{
					_trajectory.clear();
					return V_EMPTY;
				}
			}
			else
			{
				_trajectory.clear();
				return V_EMPTY;
			}
		}
	}

	_trajectory.clear();

	bool extendLine = true;
	// even guided missiles drift, but how much is based on
	// the shooter's faction, rather than accuracy.
	if (_action.type == BA_LAUNCH)
	{
		if (_action.actor->getFaction() == FACTION_PLAYER)
		{
			accuracy = 0.60;
		}
		else
		{
			accuracy = 0.55;
		}
		extendLine = _action.waypoints.size() <= 1;
	}

	// apply some accuracy modifiers.
	// This will results in a new target voxel
	applyAccuracy(originVoxel, &_targetVoxel, accuracy, false, extendLine);

	// finally do a line calculation and store this trajectory.
	return _save->getTileEngine()->calculateLineVoxel(originVoxel, _targetVoxel, true, &_trajectory, bu);
}

/**
 * Calculates the trajectory for a curved path.
 * @param accuracy The unit's accuracy.
 * @return True when a trajectory is possible.
 */
int Projectile::calculateThrow(double accuracy)
{
	Tile *targetTile = _save->getTile(_action.target);

	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, 0);
	Position targetVoxel;
	std::vector<Position> targets;
	double curvature;
	targetVoxel = _action.target.toVoxel() + Position(8,8, (1 + -targetTile->getTerrainLevel()));
	targets.clear();
	bool forced = false;

	if (_action.type == BA_THROW)
	{
		targets.push_back(targetVoxel);
	}
	else
	{
		BattleUnit *tu = targetTile->getOverlappingUnit(_save);
		if (Options::forceFire && _save->isCtrlPressed(true) && _save->getSide() == FACTION_PLAYER)
		{
			targets.push_back(_action.target.toVoxel() + Position(0, 0, 12));
			forced = true;
		}
		else if (tu && ((_action.actor->getFaction() != FACTION_PLAYER) ||
			tu->getVisible()))
		{ //unit
			targetVoxel.z += tu->getFloatHeight(); //ground level is the base
			targets.push_back(targetVoxel + Position(0, 0, tu->getHeight()/2 + 1));
			targets.push_back(targetVoxel + Position(0, 0, 2));
			targets.push_back(targetVoxel + Position(0, 0, tu->getHeight() - 1));
		}
		else if (targetTile->getMapData(O_OBJECT) != 0)
		{
			targetVoxel = _action.target.toVoxel() + Position(8,8,0);
			targets.push_back(targetVoxel + Position(0, 0, 13));
			targets.push_back(targetVoxel + Position(0, 0, 8));
			targets.push_back(targetVoxel + Position(0, 0, 23));
			targets.push_back(targetVoxel + Position(0, 0, 2));
		}
		else if (targetTile->getMapData(O_NORTHWALL) != 0)
		{
			targetVoxel = _action.target.toVoxel() + Position(8,0,0);
			targets.push_back(targetVoxel + Position(0, 0, 13));
			targets.push_back(targetVoxel + Position(0, 0, 8));
			targets.push_back(targetVoxel + Position(0, 0, 20));
			targets.push_back(targetVoxel + Position(0, 0, 3));
		}
		else if (targetTile->getMapData(O_WESTWALL) != 0)
 		{
			targetVoxel = _action.target.toVoxel() + Position(0,8,0);
			targets.push_back(targetVoxel + Position(0, 0, 13));
			targets.push_back(targetVoxel + Position(0, 0, 8));
			targets.push_back(targetVoxel + Position(0, 0, 20));
			targets.push_back(targetVoxel + Position(0, 0, 2));
		}
		else if (targetTile->getMapData(O_FLOOR) != 0)
		{
			targets.push_back(targetVoxel);
		}
	}

	_distance = 0.0f;
	int test = V_OUTOFBOUNDS;
	for (const auto& pos : targets)
	{
		targetVoxel = pos;
		if (_save->getTileEngine()->validateThrow(_action, originVoxel, targetVoxel, _save->getDepth(), &curvature, &test, forced))
		{
			break;
		}
	}
	if (!forced && test == V_OUTOFBOUNDS) return test; //no line of fire

	test = V_OUTOFBOUNDS;
	int tries = 0;
	// finally do a line calculation and store this trajectory, make sure it's valid.
	while (test == V_OUTOFBOUNDS && tries < 100)
	{
		++tries;
		Position deltas = targetVoxel;
		// apply some accuracy modifiers
		_trajectory.clear();
		if (_action.type == BA_THROW)
		{
			applyAccuracy(originVoxel, &deltas, accuracy, true, false); //calling for best flavor
			deltas -= targetVoxel;
		}
		else
		{
			applyAccuracy(originVoxel, &targetVoxel, accuracy, true, false); //arcing shot deviation
			deltas = Position(0,0,0);
		}


		test = _save->getTileEngine()->calculateParabolaVoxel(originVoxel, targetVoxel, true, &_trajectory, _action.actor, curvature, deltas);
		if (forced) return O_OBJECT; //fake hit
		Position endPoint = getPositionFromEnd(_trajectory, ItemDropVoxelOffset).toTile();
		Tile *endTile = _save->getTile(endPoint);
		// check if the item would land on a tile with a blocking object
		if (_action.type == BA_THROW
			&& endTile
			&& endTile->getMapData(O_OBJECT)
			&& endTile->getMapData(O_OBJECT)->getTUCost(MT_WALK) == Pathfinding::INVALID_MOVE_COST
			&& !(endTile->isBigWall() && (endTile->getMapData(O_OBJECT)->getBigWall()<1 || endTile->getMapData(O_OBJECT)->getBigWall()>3)))
		{
			test = V_OUTOFBOUNDS;
		}
	}
	return test;
}

/**
 * Calculates the new target in voxel space, based on the given accuracy modifier.
 * @param origin Start position of the trajectory in voxels.
 * @param target Endpoint of the trajectory in voxels.
 * @param accuracy Accuracy modifier.
 * @param keepRange Whether range affects accuracy.
 * @param extendLine should this line get extended to maximum distance?
 */
void Projectile::applyAccuracy(Position origin, Position *target, double accuracy, bool keepRange, bool extendLine)
{
	int xdiff = origin.x - target->x;
	int ydiff = origin.y - target->y;
	int zdiff = origin.z - target->z;
	double realDistance = sqrt((double)(xdiff*xdiff)+(double)(ydiff*ydiff)+(double)(zdiff*zdiff));
	// maxRange is the maximum range a projectile shall ever travel in voxel space
	double maxRange = keepRange?realDistance:16*1000; // 1000 tiles
	maxRange = _action.type == BA_HIT?46:maxRange; // up to 2 tiles diagonally (as in the case of reaper v reaper)

	if (_action.type != BA_HIT)
	{
		int upperLimit, lowerLimit;
		int dropoff = _action.weapon->getRules()->calculateLimits(upperLimit, lowerLimit, _save->getDepth(), _action.type);

		double distance = realDistance / 16; // distance in tiles, but still fractional
		double accuracyLoss = 0.0;
		if (distance > upperLimit)
		{
			accuracyLoss = (dropoff * (distance - upperLimit)) / 100;
		}
		else if (distance < lowerLimit)
		{
			accuracyLoss = (dropoff * (lowerLimit - distance)) / 100;
		}
		accuracy = std::max(0.0, accuracy - accuracyLoss);
	}

	int xDist = abs(origin.x - target->x);
	int yDist = abs(origin.y - target->y);
	int zDist = abs(origin.z - target->z);
	int xyShift, zShift;

	if (Options::oxceUniformShootingSpread) // Uniform shooting spread
	{
		if (xDist <= yDist)
			xyShift = xDist / 4 + yDist;
		else
			xyShift = xDist + yDist / 4;

		xyShift *= 0.839; // Constant to match average xyShift to vanilla
	}
	else
	{
		if (xDist / 2 <= yDist)				//yes, we need to add some x/y non-uniformity
			xyShift = xDist / 4 + yDist;	//and don't ask why, please. it's The Commandment
		else
			xyShift = (xDist + yDist) / 2;	//that's uniform part of spreading
	}

	if (xyShift <= zDist)				//slight z deviation
		zShift = xyShift / 2 + zDist;
	else
		zShift = xyShift + zDist / 2;

	// Apply penalty for having no LOS to target
	int noLOSAccuracyPenalty = _action.weapon->getRules()->getNoLOSAccuracyPenalty(_mod);
	if (noLOSAccuracyPenalty != -1)
	{
		Tile *t = _save->getTile(target->toTile());
		if (t)
		{
			bool hasLOS = false;
			BattleUnit *bu = _action.actor;
			BattleUnit *targetUnit = t->getUnit(); // we can call TileEngine::visible() only if the target unit is on the same tile

			if (targetUnit)
			{
				hasLOS = _save->getTileEngine()->visible(bu, t);
			}
			else
			{
				hasLOS = _save->getTileEngine()->isTileInLOS(&_action, t, false);
			}

			if (!hasLOS)
			{
				accuracy = accuracy * noLOSAccuracyPenalty / 100;
			}
		}
	}

	int deviation = RNG::generate(0, 100) - (accuracy * 100);

	if (deviation >= 0)
		deviation += 50;				// add extra spread to "miss" cloud
	else
		deviation += 10;				//accuracy of 109 or greater will become 1 (tightest spread)

	deviation = std::max(1, zShift * deviation / 200);	//range ratio

	if (Options::oxceUniformShootingSpread)
	{
		// First, new target point is rolled as usual. Then, if it lies outside of outer circle (in square's corner)
		// it's rerolled inside inner circle

		const double OVERALL_SPREAD_COEFF = 1.0; // Overall spread diameter change compared to vanilla
		const double INNER_SPREAD_COEFF = 0.85; // Inner spread circle diameter compared to outer

		double targetDist2D = sqrt(xDist * xDist + yDist * yDist);
		bool resultShifted = false;
		int dX, dY;

		for (int i = 0; i < 10; ++i) // Break from this cycle when proper target is found
		{
			dX = RNG::generate(0, deviation) - deviation / 2;
			dY = RNG::generate(0, deviation) - deviation / 2;

			double exprX = target->x + dX - origin.x;
			double exprY = target->y + dY - origin.y;
			double deviateDist2D = sqrt(exprX * exprX + exprY * exprY); // Distance from origin to deviation point

			if (resultShifted &&                  // point is on inner ring and should be a "miss"
				deviateDist2D > targetDist2D-2 &&
				deviateDist2D < targetDist2D+2)
				break;

			if (!resultShifted) // This is the FIRST roll!
			{
				int radiusSq = dX*dX + dY*dY;
				int deviateRadius = OVERALL_SPREAD_COEFF * deviation / 2;
				int deviateRadiusSq = deviateRadius * deviateRadius;
				if (radiusSq <= deviateRadiusSq) break;  // If we inside of outer circle - we're done!

				resultShifted = true;
				deviation *= INNER_SPREAD_COEFF; // Next attempts will be closer to target
			}
		}
		target->x += dX;
		target->y += dY;
	}

	else // Classic shooting spread
	{
		target->x += RNG::generate(0, deviation) - deviation / 2;
		target->y += RNG::generate(0, deviation) - deviation / 2;
	}

	target->z += RNG::generate(0, deviation / 2) / 2 - deviation / 8;

	if (extendLine)
	{
		double rotation, tilt;
		rotation = atan2(double(target->y - origin.y), double(target->x - origin.x)) * 180 / M_PI;
		tilt = atan2(double(target->z - origin.z),
			sqrt(double(target->x - origin.x)*double(target->x - origin.x)+double(target->y - origin.y)*double(target->y - origin.y))) * 180 / M_PI;
		// calculate new target
		// this new target can be very far out of the map, but we don't care about that right now
		double cos_fi = cos(Deg2Rad(tilt));
		double sin_fi = sin(Deg2Rad(tilt));
		double cos_te = cos(Deg2Rad(rotation));
		double sin_te = sin(Deg2Rad(rotation));
		target->x = (int)(origin.x + maxRange * cos_te * cos_fi);
		target->y = (int)(origin.y + maxRange * sin_te * cos_fi);
		target->z = (int)(origin.z + maxRange * sin_fi);
	}
}

/**
 * Moves further in the trajectory.
 * @return false if the trajectory is finished - no new position exists in the trajectory.
 */
bool Projectile::move()
{
	if (_position == 0)
	{
		_distanceMax = 0;
		for (std::size_t i = 0; i < _trajectory.size(); ++i)
		{
			_distanceMax += TileEngine::trajectoryStepSize(_trajectory, i);
		}
	}

	for (int i = 0; i < _speed; ++i)
	{
		_position++;
		if (_position == _trajectory.size())
		{
			_position--;
			return false;
		}

		_distance += TileEngine::trajectoryStepSize(_trajectory, _position);

		if (_vaporColor != -1 && _ammo && _action.type != BA_THROW)
		{
			addVaporCloud();
		}
	}
	return true;
}

/**
 * Get Position at offset from start from trajectory vector.
 * @param trajectory Vector that have trajectory.
 * @param pos Offset counted from begining of trajectory.
 * @return Position in voxel space.
 */
Position Projectile::getPositionFromStart(const std::vector<Position>& trajectory, int pos)
{
	if (pos >= 0 && pos < (int)trajectory.size())
		return trajectory.at(pos);
	else if (pos < 0)
		return trajectory.at(0);
	else
		return trajectory.at(trajectory.size() - 1);
}

/**
 * Get Position at offset from start from trajectory vector.
 * @param trajectory Vector that have trajectory.
 * @param pos Offset counted from ending of trajectory.
 * @return Position in voxel space.
 */
Position Projectile::getPositionFromEnd(const std::vector<Position>& trajectory, int pos)
{
	return getPositionFromStart(trajectory, trajectory.size() + pos - 1);
}

/**
 * Gets the current position in voxel space.
 * @param offset Offset.
 * @return Position in voxel space.
 */
Position Projectile::getPosition(int offset) const
{
	return getPositionFromStart(_trajectory, (int)_position + offset);
}

/**
 * Gets a particle reference from the projectile surfaces.
 * @param i Index.
 * @return Particle id.
 */
int Projectile::getParticle(int i) const
{
	if (_bulletSprite != Mod::NO_SURFACE)
		return _bulletSprite + i;
	else
		return Mod::NO_SURFACE;
}

/**
 * Gets the project tile item.
 * Returns 0 when there is no item thrown.
 * @return Pointer to BattleItem.
 */
BattleItem *Projectile::getItem() const
{
	if (_action.type == BA_THROW)
		return _action.weapon;
	else
		return 0;
}

/**
 * Skips to the end of the trajectory.
 */
void Projectile::skipTrajectory()
{
	while (move());
}

/**
 * Gets the Position of origin for the projectile
 * @return origin as a tile position.
 */
Position Projectile::getOrigin() const
{
	// instead of using the actor's position, we'll use the voxel origin translated to a tile position
	// this is a workaround for large units.
	return _trajectory.front().toTile();
}

/**
 * Gets the INTENDED target for this projectile
 * it is important to note that we do not use the final position of the projectile here,
 * but rather the targetted tile
 * @return target as a tile position.
 */
Position Projectile::getTarget() const
{
	return _action.target;
}

/**
 * Gets distances that projectile have traveled until now.
 * @return Returns traveled distance.
 */
float Projectile::getDistance() const
{
	return _distance;
}

/**
 * Is this projectile drawn back to front or front to back?
 * @return return if this is to be drawn in reverse order.
 */
bool Projectile::isReversed() const
{
	return _reversed;
}

/**
 * adds a cloud of vapor at the projectile's current position.
 */
void Projectile::addVaporCloud()
{
	RNG::RandomState rng = RNG::globalRandomState().subSequence();
	if (rng.percent(_vaporProbability) == false)
	{
		return;
	}

	Position subvoxelForwardDirection;
	Position subvoxelRightDirection;
	Position subvoxelUpDirection;

	auto voxelPos = getPosition();
	auto subvoxelPosFrom = getPosition(-4) * Particle::SubVoxelAccuracy;
	auto subvoxelPosTo = getPosition(+4) * Particle::SubVoxelAccuracy;
	auto subvoxelVector = subvoxelPosTo - subvoxelPosFrom;

	if (subvoxelVector == Position())
	{
		// strange trajectory, use fixed directions
		subvoxelForwardDirection.x = Particle::SubVoxelAccuracy;
		subvoxelRightDirection.y = Particle::SubVoxelAccuracy;
		subvoxelUpDirection.z = Particle::SubVoxelAccuracy;
	}
	else if (std::abs(subvoxelVector.x) < 2 &&std::abs(subvoxelVector.y) < 2)
	{
		// straight up trajectory
		subvoxelForwardDirection.z = Particle::SubVoxelAccuracy;
		subvoxelRightDirection.y = Particle::SubVoxelAccuracy;
		subvoxelUpDirection.x = - Particle::SubVoxelAccuracy;
	}
	else
	{
		// normalize vectors
		subvoxelForwardDirection = VectNormalize(subvoxelVector, Particle::SubVoxelAccuracy);

		subvoxelUpDirection.z = Particle::SubVoxelAccuracy;

		subvoxelRightDirection = VectNormalize(VectCrossProduct(subvoxelUpDirection, subvoxelForwardDirection, Particle::SubVoxelAccuracy), Particle::SubVoxelAccuracy);

		subvoxelUpDirection = VectCrossProduct(subvoxelForwardDirection, subvoxelRightDirection, Particle::SubVoxelAccuracy);
	}

	ModScript::VaporParticleAmmo::Worker worker {
		_action.weapon,
		_ammo,
		_vaporDensity,
		(int)(_distance * Particle::SubVoxelAccuracy),
		(int)(_distanceMax * Particle::SubVoxelAccuracy),
		subvoxelForwardDirection,
		subvoxelRightDirection,
		subvoxelUpDirection,
		&rng
	};

	auto tilePos = voxelPos.toTile();
	for (int i = 0; i != _vaporDensity; ++i)
	{
		ModScript::VaporParticleAmmo::Output arg = {
			_vaporColor, // "vapor_color",
			Position{ }, // "subvoxel_offset",
			Position{ }, // "subvoxel_velocity",
			Position{ }, // "subvoxel_acceleration",
			Particle::SubVoxelAccuracy / 2, // "subvoxel_drift",
			rng.generate(48, 224), // "particle_density",
			rng.generate(32, 44), // "particle_lifetime",
			i, // "particle_number",
		};

		worker.execute(_ammo->getRules()->getScript<ModScript::VaporParticleAmmo>(), arg);
		worker.execute(_action.weapon->getRules()->getScript<ModScript::VaporParticleWeapon>(), arg);

		auto varporColor = std::get<0>(arg.data);
		auto subVoxelOffset = std::get<1>(arg.data);
		auto subVoxelVelocity = std::get<2>(arg.data);
		auto subVoxelAcceleration = std::get<3>(arg.data);
		auto drift = std::get<4>(arg.data);
		auto density = std::get<5>(arg.data);
		auto particleLifetime = std::get<6>(arg.data);

		Uint8 size = 0;
		//size is initialized at 0
		if (density < 100)
		{
			size = 3;
		}
		else if (density < 125)
		{
			size = 2;
		}
		else if (density < 150)
		{
			size = 1;
		}

		if (varporColor >= 0)
		{
			Particle particle = Particle(voxelPos, subVoxelOffset, subVoxelVelocity, subVoxelAcceleration, drift, varporColor, particleLifetime, size);
			Position tileOffset = particle.updateScreenPosition();
			_save->getBattleGame()->getMap()->addVaporParticle(tilePos + tileOffset, particle);
		}
	}
}

}
