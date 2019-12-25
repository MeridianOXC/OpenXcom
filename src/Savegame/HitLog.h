#pragma once
/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include <string>
#include <sstream>
#include <vector>
#include "BattleUnit.h"

namespace OpenXcom
{

enum HitLogEntryType {
	HITLOG_EMPTY,
	HITLOG_NEW_TURN,
	HITLOG_NEW_TURN_WITH_MESSAGE,
	HITLOG_PLAYER_FIRING,
	HITLOG_REACTION_FIRE,
	HITLOG_NEW_SHOT,
	HITLOG_NO_DAMAGE,
	HITLOG_SMALL_DAMAGE,
	HITLOG_BIG_DAMAGE
};

class Language;

class HitLog
{
private:
	std::ostringstream _ss;
	std::vector<std::string> _turnDiary;

	std::string _newTurn, _reactionFire, _newShot, _noDamage, _smallDamage, _bigDamage;

	HitLogEntryType _lastEventType;
	UnitFaction _lastFaction;
	std::string _lastPlayerWeapon;

	/// Clears the hit log. And updates the turn diary.
	void clearHitLog(bool resetTurnDiary, bool ignoreLastEntry = false);

public:
	/// Creates a new hit log.
	HitLog(Language *lang);
	/// Appends the given text to the hit log.
	void appendToHitLog(HitLogEntryType type, UnitFaction faction);
	void appendToHitLog(HitLogEntryType type, UnitFaction faction, const std::string &text);
	/// Gets the hit log text.
	std::string getHitLogText(bool convert = false) const;
	/// Gets the turn diary.
	const std::vector<std::string> &getTurnDiary() const { return _turnDiary; }
};

}
