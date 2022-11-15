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
#include "BaseFacility.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Engine/GraphSubset.h"
#include "Base.h"

namespace OpenXcom
{

/**
 * Initializes a base facility of the specified type.
 * @param rules Pointer to ruleset.
 * @param base Pointer to base of origin.
 */
BaseFacility::BaseFacility(const RuleBaseFacility *rules, Base *base) : _rules(rules), _base(base), _x(-1), _y(-1), _buildTime(0), _disabled(false), _craftForDrawing(0), _hadPreviousFacility(false)
{
}

/**
 *
 */
BaseFacility::~BaseFacility()
{
}

/**
 * Loads the base facility from a YAML file.
 * @param node YAML node.
 */
void BaseFacility::load(const YAML::Node &node)
{
	_x = node["x"].as<int>(_x);
	_y = node["y"].as<int>(_y);
	_buildTime = node["buildTime"].as<int>(_buildTime);
	_disabled = node["disabled"].as<bool>(_disabled);
	_hadPreviousFacility = node["hadPreviousFacility"].as<bool>(_hadPreviousFacility);
}

/**
 * Saves the base facility to a YAML file.
 * @return YAML node.
 */
YAML::Node BaseFacility::save() const
{
	YAML::Node node;
	node["type"] = _rules->getType();
	node["x"] = _x;
	node["y"] = _y;
	if (_buildTime != 0)
		node["buildTime"] = _buildTime;
	if (_disabled)
		node["disabled"] = _disabled;
	if (_hadPreviousFacility)
		node["hadPreviousFacility"] = _hadPreviousFacility;
	return node;
}

/**
 * Returns the ruleset for the base facility's type.
 * @return Pointer to ruleset.
 */
const RuleBaseFacility *BaseFacility::getRules() const
{
	return _rules;
}

/**
 * Returns the base facility's X position on the
 * base grid that it's placed on.
 * @return X position in grid squares.
 */
int BaseFacility::getX() const
{
	return _x;
}

/**
 * Changes the base facility's X position on the
 * base grid that it's placed on.
 * @param x X position in grid squares.
 */
void BaseFacility::setX(int x)
{
	_x = x;
}

/**
 * Returns the base facility's Y position on the
 * base grid that it's placed on.
 * @return Y position in grid squares.
 */
int BaseFacility::getY() const
{
	return _y;
}

/**
 * Changes the base facility's Y position on the
 * base grid that it's placed on.
 * @param y Y position in grid squares.
 */
void BaseFacility::setY(int y)
{
	_y = y;
}

/**
 * Get placement of facility in base.
 */
BaseAreaSubset BaseFacility::getPlacement() const
{
	auto size = _rules->getSize();
	return BaseAreaSubset(size, size).offset(_x, _y);
}

/**
 * Returns the base facility's remaining time
 * until it's finished building (0 = complete).
 * @return Time left in days.
 */
int BaseFacility::getBuildTime() const
{
	return _buildTime;
}

/**
 * Returns the base facility's remaining time
 * until it's finished building (0 = complete).
 * Facility upgrades and downgrades are ignored in this calculation.
 * @return Time left in days.
 */
int BaseFacility::getAdjustedBuildTime() const
{
	if (_hadPreviousFacility)
		return 0;

	return _buildTime;
}

/**
 * Changes the base facility's remaining time
 * until it's finished building.
 * @param time Time left in days.
 */
void BaseFacility::setBuildTime(int time)
{
	_buildTime = time;
}

/**
 * Handles the facility building every day.
 */
void BaseFacility::build()
{
	_buildTime--;
	if (_buildTime == 0)
		_hadPreviousFacility = false;
}

/**
 * Returns if this facility is currently being
 * used by its base.
 * @return True if it's under use, False otherwise.
 */
bool BaseFacility::inUse() const
{
	if (_buildTime > 0)
	{
		return false;
	}

	return _base->isAreaInUse(getPlacement()) != BPE_None;
}

/**
 * Checks if the facility is disabled.
 * @return True if facility is disabled, False otherwise.
 */
bool BaseFacility::getDisabled() const
{
	return _disabled;
}

/**
 * Sets the facility's disabled flag.
 * @param disabled flag to set.
 */
void BaseFacility::setDisabled(bool disabled)
{
	_disabled = disabled;
}

/**
 * Gets craft, used for drawing facility.
 * @return craft
 */
Craft *BaseFacility::getCraftForDrawing() const
{
	return _craftForDrawing;
}

/**
 * Sets craft, used for drawing facility.
 * @param craft for drawing hangar.
 */
void BaseFacility::setCraftForDrawing(Craft *craft)
{
	_craftForDrawing = craft;
}

/**
 * Gets whether this facility was placed over another or was placed by removing another
 * @return true if placed over or by removing another facility
 */
bool BaseFacility::getIfHadPreviousFacility() const
{
	return _hadPreviousFacility;
}

/**
 * Sets whether this facility was placed over another or was placed by removing another
 * @param was there another facility just here?
 */
void BaseFacility::setIfHadPreviousFacility(bool hadPreviousFacility)
{
	_hadPreviousFacility = hadPreviousFacility;
}

/**
 * Is the facility fully built or being upgraded/downgraded?
 * Used for determining if this facility should count for base connectivity
 * @return True, if fully built or being upgraded/downgraded.
 */
bool BaseFacility::isBuiltOrHadPreviousFacility() const
{
	return _buildTime == 0 || _hadPreviousFacility;
}

}
