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
#include "Game.h"
#include "../resource.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <SDL_mixer.h>
#include "State.h"
#include "Screen.h"
#include "Sound.h"
#include "Music.h"
#include "Language.h"
#include "Logger.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Mod/Mod.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "Action.h"
#include "Exception.h"
#include "Options.h"
#include "CrossPlatform.h"
#include "FileMap.h"
#include "Unicode.h"
#include "../Menu/TestState.h"
#include <algorithm>
#include "../fallthrough.h"

namespace OpenXcom
{

const double Game::VOLUME_GRADIENT = 10.0;

/**
 * Starts up SDL with all the subsystems and SDL_mixer for audio processing,
 * creates the display screen and sets up the cursor.
 * @param title Title of the game window.
 */
Game::Game(const std::string &title) : _screen(0), _cursor(0), _lang(0), _save(0), _mod(0), _quit(false), _init(false), _update(false),  _mouseActive(true)
{
	Options::reload = false;
	Options::mute = false;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Log(LOG_ERROR) << SDL_GetError();
		Log(LOG_WARNING) << "No video detected, quit.";
		throw Exception(SDL_GetError());
	}
	Log(LOG_INFO) << "SDL initialized successfully.";

	// Initialize SDL_mixer
	initAudio();

	// trap the mouse inside the window
	SDL_WM_GrabInput(Options::captureMouse);

	// Set the window icon
	CrossPlatform::setWindowIcon(IDI_ICON1, "openxcom.png");

	// Set the window caption
	SDL_WM_SetCaption(title.c_str(), 0);

	// Set up unicode
	SDL_EnableUNICODE(1);
	Unicode::getUtf8Locale();

	// Create display
	_screen = new Screen();

	// Create cursor
	_cursor = new Cursor(9, 13);

	// Create invisible hardware cursor to workaround bug with absolute positioning pointing devices
	SDL_ShowCursor(SDL_ENABLE);
	Uint8 cursor = 0;
	SDL_SetCursor(SDL_CreateCursor(&cursor, &cursor, 1,1,0,0));

	// Create fps counter
	_fpsCounter = new FpsCounter(15, 5, 0, 0);

	// Create blank language
	_lang = new Language();
}

/**
 * Deletes the display screen, cursor, states and shuts down all the SDL subsystems.
 */
Game::~Game()
{
	Sound::stop();
	Music::stop();

	for (std::list<State*>::iterator i = _states.begin(); i != _states.end(); ++i)
	{
		delete *i;
	}

	SDL_FreeCursor(SDL_GetCursor());

	delete _cursor;
	delete _lang;
	delete _save;
	delete _mod;
	delete _screen;
	delete _fpsCounter;

	Mix_CloseAudio();

	SDL_Quit();
}

/**
 * The state machine takes care of passing all the events from SDL to the
 * active state, running any code within and blitting all the states and
 * cursor to the screen. This is run indefinitely until the game quits.
 */
void Game::run()
{
	enum ApplicationState { RUNNING = 0, SLOWED = 1, PAUSED = 2 } runningState = RUNNING;
	static const ApplicationState kbFocusRun[4] = { RUNNING, RUNNING, SLOWED, PAUSED };
	static const ApplicationState stateRun[4] = { SLOWED, PAUSED, PAUSED, PAUSED };
	// this will avoid processing SDL's resize event on startup, workaround for the heap allocation error it causes.
	bool startupEvent = Options::allowResize;
	while (!_quit)
	{
		Uint32 timeFrameStarted = SDL_GetTicks();
		// Clean up states
		while (!_deleted.empty())
		{
			delete _deleted.back();
			_deleted.pop_back();
		}

		// Initialize active state
		if (!_init)
		{
			_init = true;
			_states.back()->init();

			// Unpress buttons
			_states.back()->resetAll();

			// Refresh mouse position
			SDL_Event ev;
			int x, y;
			SDL_GetMouseState(&x, &y);
			ev.type = SDL_MOUSEMOTION;
			ev.motion.x = x;
			ev.motion.y = y;
			Action action = Action(&ev, _screen->getXScale(), _screen->getYScale(), _screen->getCursorTopBlackBand(), _screen->getCursorLeftBlackBand());
			_states.back()->handle(&action);
		}

		// Process events
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (CrossPlatform::isQuitShortcut(event))
				event.type = SDL_QUIT;
			switch (event.type)
			{
				case SDL_QUIT:
					quit();
					break;
				case SDL_ACTIVEEVENT:
					// An event other than SDL_APPMOUSEFOCUS change happened.
					if (reinterpret_cast<SDL_ActiveEvent*>(&event)->state & ~SDL_APPMOUSEFOCUS)
					{
						Uint8 currentState = SDL_GetAppState();
						// Game is minimized
						if (!(currentState & SDL_APPACTIVE))
						{
							runningState = stateRun[Options::pauseMode];
							if (Options::backgroundMute)
							{
								setVolume(0, 0, 0);
							}
						}
						// Game is not minimized but has no keyboard focus.
						else if (!(currentState & SDL_APPINPUTFOCUS))
						{
							runningState = kbFocusRun[Options::pauseMode];
							if (Options::backgroundMute)
							{
								setVolume(0, 0, 0);
							}
						}
						// Game has keyboard focus.
						else
						{
							runningState = RUNNING;
							if (Options::backgroundMute)
							{
								setVolume(Options::soundVolume, Options::musicVolume, Options::uiVolume);
							}
						}
					}
					break;
				case SDL_VIDEORESIZE:
					if (Options::allowResize)
					{
						if (!startupEvent)
						{
							Options::newDisplayWidth = Options::displayWidth = std::max(Screen::ORIGINAL_WIDTH, event.resize.w);
							Options::newDisplayHeight = Options::displayHeight = std::max(Screen::ORIGINAL_HEIGHT, event.resize.h);
							int dX = 0, dY = 0;
							Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, false);
							Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, false);
							for (std::list<State*>::iterator i = _states.begin(); i != _states.end(); ++i)
							{
								(*i)->resize(dX, dY);
							}
							_screen->resetDisplay();
						}
						else
						{
							startupEvent = false;
						}
					}
					break;
				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					// Skip mouse events if they're disabled
					if (!_mouseActive) continue;
					// re-gain focus on mouse-over or keypress.
					runningState = RUNNING;
					// Go on, feed the event to others
					FALLTHROUGH;
				default:
					Action action = Action(&event, _screen->getXScale(), _screen->getYScale(), _screen->getCursorTopBlackBand(), _screen->getCursorLeftBlackBand());
					_screen->handle(&action);
					_cursor->handle(&action);
					_fpsCounter->handle(&action);
					if (action.getDetails()->type == SDL_KEYDOWN)
					{
						// "ctrl-g" grab input
						if (action.getDetails()->key.keysym.sym == SDLK_g && (SDL_GetModState() & KMOD_CTRL) != 0)
						{
							Options::captureMouse = (SDL_GrabMode)(!Options::captureMouse);
							SDL_WM_GrabInput(Options::captureMouse);
						}
						else if (Options::debug)
						{
							if (action.getDetails()->key.keysym.sym == SDLK_t && (SDL_GetModState() & KMOD_CTRL) != 0)
							{
								pushState(new TestState);
							}
							// "ctrl-u" debug UI
							else if (action.getDetails()->key.keysym.sym == SDLK_u && (SDL_GetModState() & KMOD_CTRL) != 0)
							{
								Options::debugUi = !Options::debugUi;
								_states.back()->redrawText();
							}
						}
					}
					_states.back()->handle(&action);
					break;
			}
			if (!_init)
			{
				// States stack was changed, break the loop so new state
				// can be initialized before processing new events
				break;
			}
		}

		// Process rendering
		if (runningState != PAUSED)
		{
			// Process logic
			_states.back()->think();
			_fpsCounter->think();
			if (_init)
			{
				_fpsCounter->addFrame();
				_screen->clear();
				std::list<State*>::iterator i = _states.end();
				do
				{
					--i;
				}
				while (i != _states.begin() && !(*i)->isScreen());

				for (; i != _states.end(); ++i)
				{
					(*i)->blit();
				}
				_fpsCounter->blit(_screen->getSurface());
				_cursor->blit(_screen->getSurface());
				_screen->flip();
			}
		}

		// Calculate how long we are to sleep
		Uint32 idleTime = 0;
		if (Options::FPS > 0 && !(Options::useOpenGL && Options::vSyncForOpenGL))
		{
			// Uint32 milliseconds do wrap around in about 49.7 days
			Uint32 timeFrameEnded = SDL_GetTicks();
			Uint32 elapsedFrameTime =  timeFrameEnded > timeFrameStarted ? timeFrameEnded - timeFrameStarted : 0;
			Uint32 nominalFPS = SDL_GetAppState() & SDL_APPINPUTFOCUS ? Options::FPS : Options::FPSInactive;
			Uint32 nominalFrameTime = Options::FPS > 0 ? 1000.0f / nominalFPS : 1;
			idleTime = elapsedFrameTime > nominalFrameTime ? 0 : nominalFrameTime - elapsedFrameTime;
			idleTime = idleTime > 100 ? 100 : idleTime;
		}

		// Save on CPU
		switch (runningState)
		{
			case RUNNING:
				SDL_Delay(idleTime); 	// Save CPU from going 100%
				break;
			case SLOWED:
			case PAUSED:
				SDL_Delay(100); 		// More slowing down.
				break;
		}
	}

	Options::save();
}

/**
 * Stops the state machine and the game is shut down.
 */
void Game::quit()
{
	// Always save ironman
	if (_save != 0 && _save->isIronman() && !_save->getName().empty())
	{
		std::string filename = CrossPlatform::sanitizeFilename(_save->getName()) + ".sav";
		_save->save(filename, _mod);
	}
	_quit = true;
}

/**
 * Changes the audio volume of the music and
 * sound effect channels.
 * @param sound Sound volume, from 0 to MIX_MAX_VOLUME.
 * @param music Music volume, from 0 to MIX_MAX_VOLUME.
 * @param ui UI volume, from 0 to MIX_MAX_VOLUME.
 */
void Game::setVolume(int sound, int music, int ui)
{
	if (!Options::mute)
	{
		if (sound >= 0)
		{
			sound = volumeExponent(sound) * (double)SDL_MIX_MAXVOLUME;
			Mix_Volume(-1, sound);
			if (_save && _save->getSavedBattle())
			{
				Mix_Volume(3, sound * _save->getSavedBattle()->getAmbientVolume());
			}
			else
			{
				// channel 3: reserved for ambient sound effect.
				Mix_Volume(3, sound / 2);
			}
			// channel 4: reserved for unit responses
			Mix_Volume(4, sound);
		}
		if (music >= 0)
		{
			music = volumeExponent(music) * (double)SDL_MIX_MAXVOLUME;
			Mix_VolumeMusic(music);
		}
		if (ui >= 0)
		{
			ui = volumeExponent(ui) * (double)SDL_MIX_MAXVOLUME;
			Mix_Volume(1, ui);
			Mix_Volume(2, ui);
		}
	}
}

double Game::volumeExponent(int volume)
{
	return (exp(log(Game::VOLUME_GRADIENT + 1.0) * volume / (double)SDL_MIX_MAXVOLUME) -1.0 ) / Game::VOLUME_GRADIENT;
}

/**
 * Returns the display screen used by the game.
 * @return Pointer to the screen.
 */
Screen *Game::getScreen() const
{
	return _screen;
}

/**
 * Returns the mouse cursor used by the game.
 * @return Pointer to the cursor.
 */
Cursor *Game::getCursor() const
{
	return _cursor;
}

/**
 * Returns the FpsCounter used by the game.
 * @return Pointer to the FpsCounter.
 */
FpsCounter *Game::getFpsCounter() const
{
	return _fpsCounter;
}

/**
 * Pops all the states currently in stack and pushes in the new state.
 * A shortcut for cleaning up all the old states when they're not necessary
 * like in one-way transitions.
 * @param state Pointer to the new state.
 */
void Game::setState(State *state)
{
	while (!_states.empty())
	{
		popState();
	}
	pushState(state);
	_init = false;
}

/**
 * Pushes a new state into the top of the stack and initializes it.
 * The new state will be used once the next game cycle starts.
 * @param state Pointer to the new state.
 */
void Game::pushState(State *state)
{
	_states.push_back(state);
	_init = false;
}

/**
 * Pops the last state from the top of the stack. Since states
 * can't actually be deleted mid-cycle, it's moved into a separate queue
 * which is cleared at the start of every cycle, so the transition
 * is seamless.
 */
void Game::popState()
{
	_deleted.push_back(_states.back());
	_states.pop_back();
	_init = false;
}

/**
 * Returns the language currently in use by the game.
 * @return Pointer to the language.
 */
Language *Game::getLanguage() const
{
	return _lang;
}

/**
 * Returns the saved game currently in use by the game.
 * @return Pointer to the saved game.
 */
SavedGame *Game::getSavedGame() const
{
	return _save;
}

/**
 * Sets a new saved game for the game to use.
 * @param save Pointer to the saved game.
 */
void Game::setSavedGame(SavedGame *save)
{
	delete _save;
	_save = save;
}

/**
 * Returns the mod currently in use by the game.
 * @return Pointer to the mod.
 */
Mod *Game::getMod() const
{
	return _mod;
}

/**
 * Loads the mods specified in the game options.
 */
void Game::loadMods()
{
	Mod::resetGlobalStatics();
	delete _mod;
	_mod = new Mod();
	_mod->loadAll();
}

/**
 * Sets whether the mouse is activated.
 * If it is, mouse events are processed, otherwise
 * they are ignored and the cursor is hidden.
 * @param active Is mouse activated?
 */
void Game::setMouseActive(bool active)
{
	_mouseActive = active;
	_cursor->setVisible(active);
}

/**
 * Returns whether current state is *state
 * @param state The state to test against the stack state
 * @return Is state the current state?
 */
bool Game::isState(State *state) const
{
	return !_states.empty() && _states.back() == state;
}

/**
 * Checks if the game is currently quitting.
 * @return whether the game is shutting down or not.
 */
bool Game::isQuitting() const
{
	return _quit;
}

/**
 * Loads the most appropriate languages
 * given current system and game options.
 */
void Game::loadLanguages()
{
	const std::string defaultLang = "en-US";
	std::string currentLang = defaultLang;

	delete _lang;
	_lang = new Language();

	// No language set, detect based on system
	if (Options::language.empty())
	{
		std::string locale = CrossPlatform::getLocale();
		std::string lang = locale.substr(0, locale.find_first_of('-'));
		// Try to load full locale
		if (FileMap::fileExists("Language/" + locale + ".yml"))
		{
			currentLang = locale;
		}
		else
		{
			// Try to load language locale
			if (FileMap::fileExists("Language/" + lang + ".yml"))
			{
				currentLang = lang;
			}
			// Give up, use default
			else
			{
				currentLang = defaultLang;
			}
		}
	}
	else
	{
		// Use options language
		if (FileMap::fileExists("Language/" + Options::language + ".yml"))
		{
			currentLang = Options::language;
		}
		// Language not found, use default
		else
		{
			currentLang = defaultLang;
		}
	}
	Options::language = currentLang;

	const std::string dirLanguage = "Language/";
	const std::string dirLanguageAndroid = "Language/Android/";
	const std::string dirLanguageOXCE = "Language/OXCE/";
	const std::string dirLanguageTechnical = "Language/Technical/";

	const std::string defaultLangYml = defaultLang + ".yml";
	const std::string currentLangYml = currentLang + ".yml";

	// get vertical VFS map slices for the four filenames,
	// then submit frecs in lockstep to the _lang->loadFile().

	auto slice = FileMap::getSlice(dirLanguage + defaultLangYml);
	auto sliceAndroid = FileMap::getSlice(dirLanguageAndroid + defaultLangYml);
	auto sliceOXCE = FileMap::getSlice(dirLanguageOXCE + defaultLangYml);
	auto sliceTechnical = FileMap::getSlice(dirLanguageTechnical + defaultLangYml);

	auto slice2 = FileMap::getSlice(dirLanguage + currentLangYml);
	auto sliceAndroid2 = FileMap::getSlice(dirLanguageAndroid + currentLangYml);
	auto sliceOXCE2 = FileMap::getSlice(dirLanguageOXCE + currentLangYml);
	auto sliceTechnical2 = FileMap::getSlice(dirLanguageTechnical + currentLangYml);

	bool twoLangs = currentLang != defaultLang;
	for (size_t i = 0; i < slice.size(); ++i) {
		if (slice[i]) { _lang->loadFile(slice[i]); }
		if (twoLangs && slice2[i]) { _lang->loadFile(slice2[i]); }
		if (sliceAndroid[i]) { _lang->loadFile(sliceAndroid[i]); }
		if (twoLangs && sliceAndroid2[i]) { _lang->loadFile(sliceAndroid2[i]); }
		if (sliceOXCE[i]) { _lang->loadFile(sliceOXCE[i]); }
		if (twoLangs && sliceOXCE2[i]) { _lang->loadFile(sliceOXCE2[i]); }
		if (sliceTechnical[i]) { _lang->loadFile(sliceTechnical[i]); }
		if (twoLangs && sliceTechnical2[i]) { _lang->loadFile(sliceTechnical2[i]); }
	}

	_lang->loadRule(_mod->getExtraStrings(), defaultLang);
	if (twoLangs)
		_lang->loadRule(_mod->getExtraStrings(), currentLang);
}

/**
 * Initializes the audio subsystem.
 */
void Game::initAudio()
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Log(LOG_ERROR) << SDL_GetError();
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Options::mute = true;
		return;
	}

	Uint16 format = MIX_DEFAULT_FORMAT;
	if (Options::audioBitDepth == 8)
		format = AUDIO_S8;

	if (Options::audioSampleRate % 11025 != 0)
	{
		Log(LOG_WARNING) << "Custom sample rate " << Options::audioSampleRate << "Hz, audio that doesn't match will be distorted!";
		Log(LOG_WARNING) << "SDL_mixer only supports multiples of 11025Hz.";
	}
	int minChunk = Options::audioSampleRate / 11025 * 512;
	Options::audioChunkSize = std::max(minChunk, Options::audioChunkSize);

	if (Mix_OpenAudio(Options::audioSampleRate, format, MIX_DEFAULT_CHANNELS, Options::audioChunkSize) != 0)
	{
		Log(LOG_ERROR) << Mix_GetError();
		Log(LOG_WARNING) << "Sound device failed, audio disabled.";
		Options::mute = true;
	}
	else
	{
		Mix_AllocateChannels(16);
		// Set up reserved channels:
		// 0 = not used?
		// 1-2 = UI
		// 3 = ambient
		// 4 = unit responses (OXCE only)
		Mix_ReserveChannels(5);
		Mix_GroupChannels(1, 2, 0);
		Log(LOG_INFO) << "SDL_mixer initialized successfully.";
		setVolume(Options::soundVolume, Options::musicVolume, Options::uiVolume);
	}
}

}
