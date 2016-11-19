/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "RuleUfo.h"
#include "RuleTerrain.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain
 * type of UFO.
 * @param type String defining the type.
 */
RuleUfo::RuleUfo(const std::string &type) :
	_type(type), _size("STR_VERY_SMALL"), _sprite(-1), _marker(-1),
	_power(0), _range(0), _score(0), _reload(0), _breakOffTime(0),
	_fireSound(-1),
	_battlescapeTerrainData(0), _stats(), _statsRaceBonus()
{
	_stats.sightRange = 268;
	_statsRaceBonus[""] = RuleUfoStats();
}

/**
 *
 */
RuleUfo::~RuleUfo()
{
	delete _battlescapeTerrainData;
}

/**
 * Loads the UFO from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the UFO.
 */
void RuleUfo::load(const YAML::Node &node, Mod *mod)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod);
	}
	_type = node["type"].as<std::string>(_type);
	_size = node["size"].as<std::string>(_size);
	_sprite = node["sprite"].as<int>(_sprite);
	_marker = node["marker"].as<int>(_marker);
	_power = node["power"].as<int>(_power);
	_range = node["range"].as<int>(_range);
	_score = node["score"].as<int>(_score);
	_reload = node["reload"].as<int>(_reload);
	_breakOffTime = node["breakOffTime"].as<int>(_breakOffTime);

	_stats.load(node);

	if (const YAML::Node &terrain = node["battlescapeTerrainData"])
	{
		if (_battlescapeTerrainData)
			delete _battlescapeTerrainData;
		RuleTerrain *rule = new RuleTerrain(terrain["name"].as<std::string>());
		rule->load(terrain, mod);
		_battlescapeTerrainData = rule;
	}
	_modSprite = node["modSprite"].as<std::string>(_modSprite);
	if (const YAML::Node &raceBonus = node["raceBonus"])
	{
		for (YAML::const_iterator i = raceBonus.begin(); i != raceBonus.end(); ++i)
		{
			_statsRaceBonus[i->first.as<std::string>()].load(i->second);
		}
	}

	if (node["fireSound"])
	{
		_fireSound = mod->getSoundOffset(node["fireSound"].as<int>(_fireSound), "GEO.CAT");
	}
}

/**
 * Gets the language string that names
 * this UFO. Each UFO type has a unique name.
 * @return The Ufo's name.
 */
const std::string &RuleUfo::getType() const
{
	return _type;
}

/**
 * Gets the size of this type of UFO.
 * @return The Ufo's size.
 */
const std::string &RuleUfo::getSize() const
{
	return _size;
}

/**
 * Gets the radius of this type of UFO
 * on the dogfighting window.
 * @return The radius in pixels.
 */
int RuleUfo::getRadius() const
{
	if (_size == "STR_VERY_SMALL")
	{
		return 2;
	}
	else if (_size == "STR_SMALL")
	{
		return 3;
	}
	else if (_size == "STR_MEDIUM_UC")
	{
		return 4;
	}
	else if (_size == "STR_LARGE")
	{
		return 5;
	}
	else if (_size == "STR_VERY_LARGE")
	{
		return 6;
	}
	return 0;
}

/**
 * Gets the ID of the sprite used to draw the UFO
 * in the Dogfight window.
 * @return The sprite ID.
 */
int RuleUfo::getSprite() const
{
	return _sprite;
}

/**
 * Returns the globe marker for the UFO type.
 * @return Marker sprite, -1 if none.
 */
int RuleUfo::getMarker() const
{
	return _marker;
}

/**
 * Gets the maximum shield capacity for the UFO
 * @return The shield capacity.
 */
int RuleUfo::getShieldCapacity() const
{
	return _stats.shieldCapacity;
}

/**
 * Gets the amount of UFO shields recharged
 * per interception game second.
 * @return The shield recharge.
 */
int RuleUfo::getShieldRecharge() const
{
	return _stats.shieldRecharge;
}

/**
 * Gets the percent of damage that passes through
 * the UFO shield when the shield is downed.
 * @return The shield bleed through percentage.
 */
int RuleUfo::getShieldBleedThrough() const
{
	return _stats.shieldBleedThrough;
}

/**
 * Gets the maximum damage done by the
 * UFO's weapons per shot.
 * @return The weapon power.
 */
int RuleUfo::getWeaponPower() const
{
	return _power;
}

/**
 * Gets the maximum range for the
 * UFO's weapons.
 * @return The weapon range.
 */
int RuleUfo::getWeaponRange() const
{
	return _range;
}

/**
 * Gets the amount of points the player
 * gets for shooting down the UFO.
 * @return The score.
 */
int RuleUfo::getScore() const
{
	return _score;
}

/**
 * Gets the terrain data needed to draw the UFO in the battlescape.
 * @return The RuleTerrain.
 */
RuleTerrain *RuleUfo::getBattlescapeTerrainData() const
{
	return _battlescapeTerrainData;
}

/**
 * Gets the weapon reload for UFO ships.
 * @return The UFO weapon reload time.
 */
int RuleUfo::getWeaponReload() const
{
	return _reload;
}

/**
 * Gets the UFO's break off time.
 * @return The UFO's break off time in game seconds.
 */
int RuleUfo::getBreakOffTime() const
{
	return _breakOffTime;
}

/**
 * For user-defined UFOs, use a surface for the "preview" image.
 * @return The name of the surface that represents this UFO.
 */
const std::string &RuleUfo::getModSprite() const
{
	return _modSprite;
}

/**
 * Gets basic statistic of UFO.
 * @return Basic stats of UFO.
 */
const RuleUfoStats& RuleUfo::getStats() const
{
	return _stats;
}


/**
 * Gets bonus statistic of UFO based on race.
 * @param s Race name.
 * @return Bonus stats.
 */
const RuleUfoStats& RuleUfo::getRaceBonus(const std::string& s) const
{
	std::map<std::string, RuleUfoStats>::const_iterator i = _statsRaceBonus.find(s);
	if (i != _statsRaceBonus.end())
		return i->second;
	else
		return _statsRaceBonus.find("")->second;
}

/**
 * Gets the ufo's fire sound.
 * @return The fire sound id.
 */
int RuleUfo::getFireSound() const
{
	return _fireSound;
}

}
