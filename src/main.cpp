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
#include <exception>
#include <cassert>
#include "version.h"
#include "Engine/Exception.h"
#include "Engine/Logger.h"
#include "Engine/CrossPlatform.h"
#include "Engine/Game.h"
#include "Engine/Options.h"
#include "Engine/FileMap.h"
#include "Menu/StartState.h"

/** @mainpage
 * @author OpenXcom Developers
 *
 * OpenXcom is an open-source clone of the original X-Com
 * written entirely in C++ and SDL. This documentation contains info
 * on every class contained in the source code and its public methods.
 * The code itself also contains in-line comments for more complicated
 * code blocks. Hopefully all of this will make the code a lot more
 * readable for you in case you which to learn or make use of it in
 * your own projects, though note that all the source code is licensed
 * under the GNU General Public License. Enjoy!
 */

using namespace OpenXcom;

// Crash handling routines
#ifdef _MSC_VER

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

LONG WINAPI crashLogger(PEXCEPTION_POINTERS exception)
{
	CrossPlatform::crashDump(exception, "");
	return EXCEPTION_CONTINUE_SEARCH;
}
#else
#include <signal.h>
void signalLogger(int sig)
{
	CrossPlatform::crashDump(&sig, "");
	exit(EXIT_FAILURE);
}

#endif

void exceptionLogger()
{
	static bool logged = false;
	std::string error;
	try
	{
		if (!logged)
		{
			logged = true;
			throw;
		}
	}
	catch (const std::exception &e)
	{
		error = e.what();
	}
	catch (...)
	{
		error = "Unknown exception";
	}
	CrossPlatform::crashDump(0, error);
	exit(EXIT_FAILURE);
}

Game *game = 0;

// If you can't tell what the main() is for you should have your
// programming license revoked...
int main(int argc, char *argv[])
{
#ifndef DUMP_CORE
#ifdef _MSC_VER
	// Uncomment to check memory leaks in VS
	//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

	SetUnhandledExceptionFilter(crashLogger);
#ifdef __MINGW32__
	// MinGW can use SJLJ or Dwarf exceptions, because of this SEH can't catch it.
	std::set_terminate(exceptionLogger);
#endif
	// Uncomment to debug crash handler
	// AddVectoredContinueHandler(1, crashLogger);
#else
	signal(SIGSEGV, signalLogger);
	std::set_terminate(exceptionLogger);
#endif
#endif
	YAML::setGlobalErrorHandler();
	CrossPlatform::getErrorDialog();
	CrossPlatform::processArgs(argc, argv);
	if (!Options::init())
		return EXIT_SUCCESS;
	std::ostringstream title;
	title << "OpenXcom " << OPENXCOM_VERSION_SHORT << OPENXCOM_VERSION_GIT;
	Options::baseXResolution = Options::displayWidth;
	Options::baseYResolution = Options::displayHeight;

	game = new Game(title.str());
	State::setGamePtr(game);
	game->setState(new StartState);
	game->run();

	bool startUpdate = game->getUpdateFlag();

	// Comment those two for faster exit.
	delete game;
	FileMap::clear(true, false); // make valgrind happy

	if (startUpdate)
	{
		CrossPlatform::startUpdateProcess();
	}

	return EXIT_SUCCESS;
}

namespace OpenXcom
{
	Exception::Exception(const std::string &msg) : runtime_error(msg) {
#ifdef DUMP_CORE
		__builtin_trap();
#endif
	}
}

#ifdef __MORPHOS__
const char Version[] = "$VER: OpenXCom " OPENXCOM_VERSION_SHORT " (" __AMIGADATE__  ")";
#endif



#ifndef NDEBUG

#include "Engine/Collections.h"
#include "fmath.h"

struct BadMove
{
	int i;

	BadMove(int b)
	{
		i = b;
	}
	BadMove(BadMove&& b)
	{
		i = b.i;
		b.i = {};
	}
	BadMove(const BadMove& b)
	{
		i = b.i;
	}

	BadMove& operator=(BadMove&& b)
	{
		i = {}; //this can reset other `b` too if we do not check for `this == &b`
		i = b.i;
		b.i = {}; //same there
		return *this;
	}

	bool operator==(const BadMove& b) const
	{
		return i == b.i;
	}
};

static auto dummy = ([]
{
	{
		std::vector<int> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](int& i) { return i < 3; });
		assert((v == std::vector<int>{ 3, 4 }));
	}
	{
		std::vector<int> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](int& i) { return i > 2; });
		assert((v == std::vector<int>{ 1, 2 }));
	}
	{
		std::vector<int> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](int& i) { return i < 2 || i == 4; });
		assert((v == std::vector<int>{ 2, 3 }));
	}
	{
		std::vector<int> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](int& i) { if (i < 2 || i == 4) { return true; } else { i += 10; return false; } });
		assert((v == std::vector<int>{ 12, 13 }));
	}
	{
		std::vector<int> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](int& i) { return false; });
		assert((v == std::vector<int>{ 1, 2, 3, 4 }));
	}
	{
		std::vector<int> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](int& i) { return true; });
		assert((v == std::vector<int>{ }));
	}
	{
		std::vector<int> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](int& i) { i += 10; return false; });
		assert((v == std::vector<int>{ 11, 12, 13, 14 }));
	}

	{
		std::vector<BadMove> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](BadMove& i) { return i.i < 3; });
		assert((v == std::vector<BadMove>{ 3, 4 }));
	}
	{
		std::vector<BadMove> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](BadMove& i) { return i.i > 2; });
		assert((v == std::vector<BadMove>{ 1, 2 }));
	}
	{
		std::vector<BadMove> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](BadMove& i) { if (i.i < 2 || i.i == 4) { return true; } else { i.i += 10; return false; } });
		assert((v == std::vector<BadMove>{ 12, 13 }));
	}
	{
		std::vector<BadMove> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](BadMove& i) { return false; });
		assert((v == std::vector<BadMove>{ 1, 2, 3, 4 }));
	}
	{
		std::vector<BadMove> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](BadMove& i) { i.i += 10; return false; });
		assert((v == std::vector<BadMove>{ 11, 12, 13, 14 }));
	}
	{
		std::vector<BadMove> v = { 1, 2, 3, 4 };
		Collections::removeIf(v, [](BadMove& i) { return true; });
		assert((v == std::vector<BadMove>{ }));
	}

	return 0;
})();

struct DummyVectDouble
{
	double x, y, z;

	bool operator==(const DummyVectDouble& a) const { return AreSame(x, a.x) && AreSame(y, a.y) && AreSame(z, a.z); };
};

static auto dummyMath = ([]
{
	const DummyVectDouble x { 1, 0, 0 };
	const DummyVectDouble y { 0, 1, 0 };
	const DummyVectDouble z { 0, 0, 1 };

	const DummyVectDouble x256 { 256, 0, 0 };
	const DummyVectDouble y256 { 0, 256, 0 };
	const DummyVectDouble z256 { 0, 0, 256 };

	assert(x == VectNormalize(x));
	assert(y == VectNormalize(y));
	assert(z == VectNormalize(z));
	assert(x == VectNormalize(x256));
	assert(y == VectNormalize(y256));
	assert(z == VectNormalize(z256));
	assert(x256 == VectNormalize(x256, 256));
	assert(y256 == VectNormalize(y256, 256));
	assert(z256 == VectNormalize(z256, 256));

	assert(z == VectCrossProduct(x, y));
	assert(x == VectCrossProduct(y, z));
	assert(y == VectCrossProduct(z, x));

	assert(z256 == VectCrossProduct(x256, y));
	assert(x256 == VectCrossProduct(y256, z));
	assert(y256 == VectCrossProduct(z256, x));

	assert(z256 == VectCrossProduct(x256, y256, 256));
	assert(x256 == VectCrossProduct(y256, z256, 256));
	assert(y256 == VectCrossProduct(z256, x256, 256));

	return 0;
})();

#endif
