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
#include <sstream>
#include <string>
#include "CrossPlatform.h"

namespace OpenXcom
{

/**
 * Defines the various severity levels of
 * information logged by the game.
 */
enum SeverityLevel
{
	LOG_FATAL,		/**< Something horrible has happened and the game is going to die! */
	LOG_ERROR,		/**< Something bad happened but we can still move on. */
	LOG_WARNING,	/**< Something weird happened, nothing special but it's good to know. */
	LOG_INFO,		/**< Useful information for users/developers to help debug and figure stuff out. */
	LOG_DEBUG,		/**< Purely test stuff to help developers implement, not really relevant to users. */
	LOG_VERBOSE,	/**< Extra details that even developers won't really need 90% of the time. */

	LOG_UNCENSORED  /**< Makes sure everything makes it into XComLog buffer until there's a logfile set up */
};

/**
 * A basic logging and debugging class, prints output to stdout/files.
 * @note Wasn't really satisfied with any of the libraries around
 * so I rolled my own. Based on http://www.drdobbs.com/cpp/logging-in-c/201804215
 */
class Logger
{
public:
	Logger() : _level(LOG_INFO) { };
	virtual ~Logger() { CrossPlatform::Log(_level, os); };
	std::ostringstream& get(SeverityLevel level = LOG_INFO) { _level = level; return os; };

	static SeverityLevel& reportingLevel() {
		static SeverityLevel reportingLevel = LOG_UNCENSORED;
		return reportingLevel;
	};
	static const std::string& toString(int level) {
		static const std::string buffer[] = { "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "VERB", "ALL" };
		return buffer[level];
	};
private:
	Logger(const Logger&);
	SeverityLevel _level;
	std::ostringstream os;
};

#define XComLog(level) if (level > Logger::reportingLevel()) { } else Logger().get(level)

}
