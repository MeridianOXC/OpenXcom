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
#include <yaml-cpp/yaml.h>
#include "../Mod/RuleBaseFacility.h"

namespace OpenXcom
{

class RuleBaseFacility;
class Base;
class Mod;
class Craft;

/**
 * Represents a base facility placed in a base.
 * Contains variable info about a facility like
 * position and build time.
 * @sa RuleBaseFacility
 */
class BaseFacility
{
private:
	const RuleBaseFacility *_rules;
	Base *_base;
	int _x, _y, _buildTime;
	bool _disabled;
	Craft *_craftForDrawing;	// craft, used for drawing facility
	bool _hadPreviousFacility;
public:
	/// Creates a base facility of the specified type.
	BaseFacility(const RuleBaseFacility *rules, Base *base);
	/// Cleans up the base facility.
	~BaseFacility();
	/// Loads the base facility from YAML.
	void load(const YAML::Node& node);
	/// Saves the base facility to YAML.
	YAML::Node save() const;
	/// Gets the facility's ruleset.
	const RuleBaseFacility *getRules() const;
	/// Gets the facility's X position.
	int getX() const;
	/// Sets the facility's X position.
	void setX(int x);
	/// Gets the facility's Y position.
	int getY() const;
	/// Sets the facility's Y position.
	void setY(int y);
	/// Get placement of facility in base.
	BaseAreaSubset getPlacement() const;
	/// Gets the facility's construction time.
	int getBuildTime() const;
	/// Gets the facility's adjusted construction time (i.e. NOT considering upgrades/downgrades).
	int getAdjustedBuildTime() const;
	/// Sets the facility's construction time.
	void setBuildTime(int time);
	/// Builds up the facility.
	void build();
	/// Checks if the facility is currently in use.
	bool inUse() const;
	/// Checks if the facility is disabled.
	bool getDisabled() const;
	/// Sets the facility's disabled flag.
	void setDisabled(bool disabled);
	/// Gets craft, used for drawing facility.
	Craft *getCraftForDrawing() const;
	/// Sets craft, used for drawing facility.
	void setCraftForDrawing(Craft *craft);
	/// Gets whether this facility was placed over another or was placed by removing another
	bool getIfHadPreviousFacility() const;
	/// Sets whether this facility was placed over another or was placed by removing another
	void setIfHadPreviousFacility(bool hadPreviousFacility);
	/// Is the facility fully built or being upgraded/downgraded?
	bool isBuiltOrHadPreviousFacility() const;
};

}
