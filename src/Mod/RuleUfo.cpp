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
#include "RuleUfo.h"
#include "RuleTerrain.h"
#include "Mod.h"
#include "../Engine/ScriptBind.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain
 * type of UFO.
 * @param type String defining the type.
 */
RuleUfo::RuleUfo(const std::string &type) :
	_type(type), _size("STR_VERY_SMALL"),
	_radius(-1), _visibility(0), _blobSize(-1),
	_sprite(-1), _marker(-1), _markerLand(-1), _markerCrash(-1),
	_power(0), _range(0), _score(0), _reload(0), _breakOffTime(0), _missionScore(1),
	_hunterKillerPercentage(0), _huntMode(0), _huntSpeed(100), _huntBehavior(2), _softlockThreshold(100),
	_missilePower(0), _unmanned(false), _instaHyper(false),
	_splashdownSurvivalChance(100), _fakeWaterLandingChance(0),
	_fireSound(-1), _alertSound(-1), _huntAlertSound(-1), _hitSound(-1),
	_battlescapeTerrainData(0), _stats(), _statsRaceBonus()
{
	_stats.sightRange = 268;
	_stats.radarRange = 672; // same default as in RuleCraft (used by hunter-killers)
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
void RuleUfo::load(const YAML::YamlNodeReader& node, Mod *mod, const ModScript &parsers)
{
	const auto& reader = node.useIndex();
	if (const auto& parent = reader["refNode"])
	{
		load(parent, mod, parsers);
	}

	reader.tryRead("size", _size);
	// sigh
	if (_size == "STR_MEDIUM")
	{
		_size = "STR_MEDIUM_UC";
	}
	reader.tryRead("radius", _radius);
	reader.tryRead("visibility", _visibility);
	reader.tryRead("blobSize", _blobSize);
	if (_blobSize > 7)
	{
		_blobSize = 7; // maximum possible
	}
	reader.tryRead("sprite", _sprite);
	if (reader["marker"])
	{
		_marker = mod->getOffset(reader["marker"].readVal(_marker), 8);
	}
	if (reader["markerLand"])
	{
		_markerLand = mod->getOffset(reader["markerLand"].readVal(_markerLand), 8);
	}
	if (reader["markerCrash"])
	{
		_markerCrash = mod->getOffset(reader["markerCrash"].readVal(_markerCrash), 8);
	}
	reader.tryRead("power", _power);
	reader.tryRead("range", _range);
	reader.tryRead("score", _score);
	reader.tryRead("reload", _reload);
	reader.tryRead("breakOffTime", _breakOffTime);
	reader.tryRead("missionScore", _missionScore);
	reader.tryRead("hunterKillerPercentage", _hunterKillerPercentage);
	reader.tryRead("huntMode", _huntMode);
	reader.tryRead("huntSpeed", _huntSpeed);
	reader.tryRead("huntBehavior", _huntBehavior);
	reader.tryRead("softlockThreshold", _softlockThreshold);
	reader.tryRead("missilePower", _missilePower);
	reader.tryRead("unmanned", _unmanned);
	reader.tryRead("instaHyper", _instaHyper);
	reader.tryRead("splashdownSurvivalChance", _splashdownSurvivalChance);
	reader.tryRead("fakeWaterLandingChance", _fakeWaterLandingChance);

	_stats.load(reader);

	if (const auto& terrain = reader["battlescapeTerrainData"])
	{
		if (_battlescapeTerrainData)
			delete _battlescapeTerrainData;
		RuleTerrain *rule = new RuleTerrain(terrain["name"].readVal<std::string>());
		rule->load(terrain, mod);
		_battlescapeTerrainData = rule;
	}
	reader.tryRead("modSprite", _modSprite);
	reader.tryRead("hitImage", _hitImage);
	for (const auto& raceBonus : reader["raceBonus"].children())
	{
		_statsRaceBonus[raceBonus.readKey<std::string>()].load(raceBonus);
	}

	mod->loadSoundOffset(_type, _fireSound, reader["fireSound"], "GEO.CAT");
	mod->loadSoundOffset(_type, _alertSound, reader["alertSound"], "GEO.CAT");
	mod->loadSoundOffset(_type, _huntAlertSound, reader["huntAlertSound"], "GEO.CAT");
	mod->loadSoundOffset(_type, _hitSound, reader["hitSound"], "GEO.CAT");

	_ufoScripts.load(_type, reader, parsers.ufoScripts);
	_scriptValues.load(reader, parsers.getShared());
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
	if (_radius > -1)
	{
		return _radius;
	}

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
 * Gets the default visibility of this type of UFO,
 * i.e. not considering the altitude.
 * @return The default visibility.
 */
int RuleUfo::getDefaultVisibility() const
{
	if (_visibility != 0)
	{
		return _visibility;
	}

	// vanilla = 15*(3-ufosize);
	if (_size == "STR_VERY_SMALL")
		return -30;
	else if (_size == "STR_SMALL")
		return -15;
	else if (_size == "STR_MEDIUM_UC")
		return 0;
	else if (_size == "STR_LARGE")
		return 15;
	else if (_size == "STR_VERY_LARGE")
		return 30;

	return 0;
}

/**
 * Gets the blob size of this type of UFO
 * on the dogfighting window.
 * @return The blob size.
 */
int RuleUfo::getBlobSize() const
{
	if (_blobSize > -1 && _blobSize < 8)
	{
		return _blobSize;
	}

	if (_size == "STR_VERY_SMALL")
	{
		return 0;
	}
	else if (_size == "STR_SMALL")
	{
		return 1;
	}
	else if (_size == "STR_MEDIUM_UC")
	{
		return 2;
	}
	else if (_size == "STR_LARGE")
	{
		return 3;
	}
	else if (_size == "STR_VERY_LARGE")
	{
		return 4;
	}

	return 4;
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
 * Returns the globe marker for the UFO while in flight.
 * @return Marker sprite, -1 if none.
 */
int RuleUfo::getMarker() const
{
	return _marker;
}

/**
 * Returns the globe marker for the UFO while landed.
 * @return Marker sprite, -1 if none.
 */
int RuleUfo::getLandMarker() const
{
	return _markerLand;
}

/**
 * Returns the globe marker for the UFO when crashed.
 * @return Marker sprite, -1 if none.
 */
int RuleUfo::getCrashMarker() const
{
	return _markerCrash;
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
 * Gets the UFO's fire sound.
 * @return The fire sound ID.
 */
int RuleUfo::getFireSound() const
{
	return _fireSound;
}

/**
 * Gets the UFO's alert sound (UFO detected alert).
 * @return The alert sound ID.
 */
int RuleUfo::getAlertSound() const
{
	return _alertSound;
}

/**
 * Gets the UFO's alert sound (UFO on intercept course alert).
 * @return The alert sound ID.
 */
int RuleUfo::getHuntAlertSound() const
{
	return _huntAlertSound;
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
	auto i = _statsRaceBonus.find(s);
	if (i != _statsRaceBonus.end())
		return i->second;
	else
		return _statsRaceBonus.find("")->second;
}

const std::map<std::string, RuleUfoStats> &RuleUfo::getRaceBonusRaw() const
{
	return _statsRaceBonus;
}

/**
 * Gets the amount of points awarded every 30 minutes
 * while the UFO is on a mission (doubled when landed).
 * @return Score.
 */
int RuleUfo::getMissionScore() const
{
	return _missionScore;
}

/**
 * Gets the UFO's chance to become a hunter-killer.
 * @return Percentage to become a hunter-killer.
 */
int RuleUfo::getHunterKillerPercentage() const
{
	return _hunterKillerPercentage;
}

/**
 * Gets the UFO's hunting preferences.
 * @return Hunt mode ID.
 */
int RuleUfo::getHuntMode() const
{
	return _huntMode;
}

/**
 * Gets the UFO's hunting speed (in percent of maximum speed).
 * @return Percentage of maximum speed.
 */
int RuleUfo::getHuntSpeed() const
{
	return _huntSpeed;
}

/**
 * Gets the UFO's hunting behavior (normal, kamikaze or random).
 * @return Hunt behavior ID.
 */
int RuleUfo::getHuntBehavior() const
{
	return _huntBehavior;
}

////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

std::string debugDisplayScript(const RuleUfo* ru)
{
	if (ru)
	{
		std::string s;
		s += RuleUfo::ScriptName;
		s += "(name: \"";
		s += ru->getType();
		s += "\")";
		return s;
	}
	else
	{
		return "null";
	}
}

} // namespace

/**
 * Register RuleUfo in script parser.
 * @param parser Script parser.
 */
void RuleUfo::ScriptRegister(ScriptParserBase* parser)
{
	Bind<RuleUfo> ar = { parser };

	ar.add<&RuleUfo::getRadius>("getRadius");
	ar.add<&RuleUfo::getWeaponRange>("getWeaponRange");
	ar.add<&RuleUfo::getWeaponPower>("getWeaponPower");
	ar.add<&RuleUfo::getWeaponReload>("getWeaponReload");

	RuleUfoStats::addGetStatsScript<&RuleUfo::_stats>(ar, "");

	ar.addScriptValue<BindBase::OnlyGet, &RuleUfo::_scriptValues>();
	ar.addDebugDisplay<&debugDisplayScript>();
}

}
