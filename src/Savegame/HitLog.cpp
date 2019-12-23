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
#include "HitLog.h"
#include "../Engine/Language.h"
#include <regex>

namespace OpenXcom
{

HitLog::HitLog(Language *lang) : _lastEventType(HITLOG_EMPTY), _lastFaction(FACTION_PLAYER)
{
	// cache
	_newTurn = lang->getString("STR_HIT_LOG_NEW_TURN");
	_reactionFire = lang->getString("STR_HIT_LOG_REACTION_FIRE");
	_newShot = lang->getString("STR_HIT_LOG_NEW_BULLET");
	_noDamage = lang->getString("STR_HIT_LOG_NO_DAMAGE");
	_smallDamage = lang->getString("STR_HIT_LOG_SMALL_DAMAGE");
	_bigDamage = lang->getString("STR_HIT_LOG_BIG_DAMAGE");
}

/**
 * Clears the hit log. And updates the turn diary.
 */
void HitLog::clearHitLog(bool resetTurnDiary, bool ignoreLastEntry)
{
	if (resetTurnDiary)
	{
		_turnDiary.clear();
	}
	else if (!ignoreLastEntry)
	{
		_turnDiary.push_back(getHitLogText(true));
	}

	_ss.str(std::string());
	_ss.clear();
}

/**
 * Appends a given entry to the hit log.
 * @param type Type of hit log entry.
 * @param faction Faction of the actor.
 */
void HitLog::appendToHitLog(HitLogEntryType type, UnitFaction faction)
{
	switch (type)
	{
	case HITLOG_NEW_TURN:
		clearHitLog(true);
		_ss << _newTurn;
		break;
	case HITLOG_REACTION_FIRE:
		if (_lastEventType != HITLOG_REACTION_FIRE) // don't produce duplicates
		{
			clearHitLog(false);
			_ss << _reactionFire;
			_ss << "\n\n";
		}
		break;
	case HITLOG_NEW_SHOT:
		if (_lastFaction != faction && faction == FACTION_PLAYER)
		{
			// player continues shooting (without selecting a weapon) after enemy reaction fire
			clearHitLog(false);
			_ss << _lastPlayerWeapon; // this is why we needed to remember it :)
			_ss << "\n\n";
		}
		_ss << _newShot;
		break;
	case HITLOG_NO_DAMAGE:
		_ss << _noDamage;
		break;
	case HITLOG_SMALL_DAMAGE:
		_ss << _smallDamage;
		break;
	case HITLOG_BIG_DAMAGE:
		_ss << _bigDamage;
		break;
	default:
		break;
	}

	_lastEventType = type;
	_lastFaction = faction;
}

/**
 * Appends a given entry to the hit log.
 * @param type Type of hit log entry.
 * @param faction Faction of the actor.
 * @param text Text to append.
 */
void HitLog::appendToHitLog(HitLogEntryType type, UnitFaction faction, const std::string &text)
{
	switch (type)
	{
	case HITLOG_NEW_TURN_WITH_MESSAGE:
		clearHitLog(true);
		_ss << text;
		break;
	case HITLOG_PLAYER_FIRING:
		clearHitLog(false, _lastEventType == HITLOG_PLAYER_FIRING); // don't produce duplicates and irrelevant diary entries
		_lastPlayerWeapon = text; // remember this, we may need it again if we are interrupted by reaction fire
		_ss << _lastPlayerWeapon;
		_ss << "\n\n";
		break;
	default:
		break;
	}

	_lastEventType = type;
	_lastFaction = faction;
}

/**
 * Gets the hit log text.
 * @param convert Convert line breaks to spaces?
 * @return hit log text.
 */
std::string HitLog::getHitLogText(bool convert) const
{
	if (convert)
	{
		return std::regex_replace(_ss.str(), std::regex("\n"), " ");
	}
	return _ss.str();
}

}
