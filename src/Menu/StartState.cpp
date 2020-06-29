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
#include "StartState.h"
#include "../version.h"
#include "../Engine/Logger.h"
#include "../Engine/Game.h"
#include "../Engine/Screen.h"
#include "../Engine/Action.h"
#include "../Engine/Surface.h"
#include "../Engine/Options.h"
#include "../Engine/Language.h"
#include "../Engine/Sound.h"
#include "../Engine/Music.h"
#include "../Engine/Font.h"
#include "../Engine/Timer.h"
#include "../Engine/CrossPlatform.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "MainMenuState.h"
#include "CutsceneState.h"
#include <SDL_mixer.h>
#include <SDL_thread.h>

namespace OpenXcom
{

LoadingPhase StartState::loading;
std::string StartState::error;

/**
 * Initializes all the elements in the Loading screen.
 * @param game Pointer to the core game.
 */
StartState::StartState() : _anim(0)
{
	//updateScale() uses newDisplayWidth/Height and needs to be set ahead of time
	Options::newDisplayWidth = Options::displayWidth;
	Options::newDisplayHeight = Options::displayHeight;
	Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, false);
	Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, false);
#ifdef __ANDROID__
	// Maybe try a higher resolution this time?
	Options::baseXResolution = std::min(Screen::ORIGINAL_WIDTH * 2, Options::displayWidth);
	Options::baseYResolution = std::min(Screen::ORIGINAL_HEIGHT * 2, Options::displayHeight);
#else
	Options::baseXResolution = Options::displayWidth;
	Options::baseYResolution = Options::displayHeight;
#endif
	_game->getScreen()->resetDisplay(false, true);

	// Create objects
	_thread = 0;
	loading = LOADING_STARTED;
	error = "";
	_oldMaster = Options::getActiveMaster();

	_font = new Font();
	_font->loadTerminal();
	_lang = new Language();

	_text = new Text(Options::baseXResolution, Options::baseYResolution, 0, 0);
	_cursor = new Text(_font->getWidth(), _font->getHeight(), 0, 0);
	_timer = new Timer(150);

	SDL_Color bnw[3];

	bnw[0].a = 255;
	bnw[1].a = 255;
	bnw[2].a = 255;
	bnw[0].r = 0;
	bnw[0].g = 0;
	bnw[0].b = 0;
	bnw[1].r = 255;
	bnw[1].g = 255;
	bnw[1].b = 255;
	bnw[2].r = 185;
	bnw[2].g = 185;
	bnw[2].b = 185;

	setStatePalette(bnw, 0, 3);

	add(_text);
	add(_cursor);

	// Set up objects
	_text->initText(_font, _font, _lang);
	_text->setColor(0);
	_text->setWordWrap(true);

	_cursor->initText(_font, _font, _lang);
	_cursor->setColor(0);
	_cursor->setText("_");

	_timer->onTimer((StateHandler)&StartState::animate);
	_timer->start();

	// Hide UI
	_game->getCursor()->setVisible(false);
	_game->getFpsCounter()->setVisible(false);

	if (Options::reload)
	{
		if (Options::oxceStartUpTextMode < 2)
		{
			addLine("Restarting...");
			addLine("");
		}
	}
	else
	{
		if (Options::oxceStartUpTextMode < 2)
		{
			addLine(CrossPlatform::getDosPath() + ">openxcom");
		}
	}
}

/**
 * Kill the thread in case the game is quit early.
 */
StartState::~StartState()
{
	if (_thread != 0)
	{
		int status;
		SDL_WaitThread(_thread, &status);
	}
	delete _font;
	delete _timer;
	delete _lang;
}

/**
 * Reset and reload data.
 */
void StartState::init()
{
	State::init();

	// Silence!
	Sound::stop();
	Music::stop();
	if (!Options::mute && Options::reload)
	{
		Mix_CloseAudio();
		_game->initAudio();
	}

	// Load the game data in a separate thread
	_thread = SDL_CreateThread(load, "load_data", (void*)_game);
	if (_thread == 0)
	{
		// If we can't create the thread, just load it as usual
		load((void*)_game);
	}
}

/**
 * If the loading fails, it shows an error, otherwise moves on to the game.
 */
void StartState::think()
{
	State::think();
	_timer->think(this, 0);

	switch (loading)
	{
	case LOADING_FAILED:
	{
		CrossPlatform::flashWindow(_game->getScreen()->getWindow());
		addLine("");
		addLine("ERROR: " + error);
		addLine("");
		addLine("More details here: " + CrossPlatform::getLogFileName());
		addLine("Make sure OpenXcom and any mods are installed correctly.");
		addLine("");
#ifdef __ANDROID__
		addLine("Android users: your data folder is located at:");
		addLine(Options::getDataFolder());
		addLine("(as reported by your system)");
		addLine("");
		CrossPlatform::findDirDialog();
#endif
		addLine("Press any key to continue.");
		loading = LOADING_DONE;
		break;
	}
	case LOADING_SUCCESSFUL:
		CrossPlatform::flashWindow(_game->getScreen()->getWindow());
		Log(LOG_INFO) << "OpenXcom started successfully!";
		_game->setState(new GoToMainMenuState());
		if (_oldMaster != Options::getActiveMaster() && Options::playIntro)
		{
			_game->pushState(new CutsceneState("intro"));
		}
		if (Options::reload)
		{
			Options::reload = false;
		}
		_game->getCursor()->setVisible(true);
		_game->getFpsCounter()->setVisible(Options::fpsCounter);
		break;
	default:
		break;
	}
}

/**
 * The game quits if the player presses any key when an error
 * message is on display.
 * @param action Pointer to an action.
 */
void StartState::handle(Action *action)
{
	State::handle(action);
	if (loading == LOADING_DONE)
	{
		if (action->getDetails()->type == SDL_KEYDOWN)
		{
			_game->quit();
		}
	}
}

/**
 * Blinks the cursor and spreads out terminal output.
 */
void StartState::animate()
{
	_cursor->setVisible(!_cursor->getVisible());
	_anim++;

	if (loading == LOADING_STARTED)
	{
		std::ostringstream ss;
		ss << "Loading OpenXcom " << OPENXCOM_VERSION_SHORT << OPENXCOM_VERSION_GIT << "...";
		if (Options::reload)
		{
			if (Options::oxceStartUpTextMode < 2)
			{
				if (_anim == 2)
					addLine(ss.str());
			}
		}
		else
		{
			switch (_anim)
			{
			case 1:
				if (Options::oxceStartUpTextMode < 1)
				{
					addLine("DOS/4GW Protected Mode Run-time  Version 1.9");
					addLine("Copyright (c) Rational Systems, Inc. 1990-1993");
				}
				break;
			case 6:
				if (Options::oxceStartUpTextMode < 2)
				{
					addLine("");
					addLine("OpenXcom initialisation");
				}
				break;
			case 7:
				if (Options::oxceStartUpTextMode < 1)
				{
					addLine("");
					if (Options::mute)
					{
						addLine("No Sound Detected");
					}
					else
					{
						addLine("SoundBlaster Sound Effects");
						if (Options::preferredMusic == MUSIC_MIDI)
							addLine("General MIDI Music");
						else
							addLine("SoundBlaster Music");
						addLine("Base Port 220  Irq 7  Dma 1");
					}
				}
				if (Options::oxceStartUpTextMode < 2)
				{
					addLine("");
				}
				break;
			case 9:
				if (Options::oxceStartUpTextMode < 2)
				{
					addLine(ss.str());
				}
				break;
			}
		}
	}
}

/**
 * Adds a line of text to the terminal and moves
 * the cursor appropriately.
 * @param str Text line to add.
 */
void StartState::addLine(const std::string &str)
{
	_output << "\n" << str;
	_text->setText(_output.str());
	int y = _text->getTextHeight() - _font->getHeight();
	int x = _text->getTextWidth(y / _font->getHeight());
	_cursor->setX(x);
	_cursor->setY(y);
}

/**
 * Loads game data and updates status accordingly.
 * @param game_ptr Pointer to the game.
 * @return Thread status, 0 = ok
 */
int StartState::load(void *game_ptr)
{
	Game *game = (Game*)game_ptr;
	try
	{
		Log(LOG_INFO) << "Loading data...";
		Options::updateMods();
		game->loadMods();
		Log(LOG_INFO) << "Data loaded successfully.";
		Log(LOG_INFO) << "Loading language...";
		game->loadLanguages();
		Log(LOG_INFO) << "Language loaded successfully.";
		loading = LOADING_SUCCESSFUL;
	}
	catch (std::exception &e)
	{
		error = e.what();
		Log(LOG_ERROR) << error;
		loading = LOADING_FAILED;
	}

	return 0;
}

}
