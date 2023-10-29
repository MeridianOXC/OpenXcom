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
#include "../Ufopaedia/UfopaediaStartState.h"
#include "../Menu/NotesState.h"
#include "../Menu/TestState.h"
#include <algorithm>
#include "../fallthrough.h"

namespace OpenXcom
{

const double Game::VOLUME_GRADIENT = 10.0;

/**
 * Starts up all the SDL subsystems,
 * creates the display screen and sets up the cursor.
 * @param title Title of the game window.
 */
Game::Game(const std::string &title) : _screen(0), _cursor(0), _lang(0), _save(0), _mod(0), _quit(false), _init(false), _update(false),  _mouseActive(true), _timeUntilNextFrame(0),
	_ctrl(false), _alt(false), _shift(false), _rmb(false), _mmb(false)
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

	// Set up unicode
	Unicode::getUtf8Locale();

	// Create display
	_screen = new Screen();

	// Actually, you can create a window icon only after the screen is here
	CrossPlatform::setWindowIcon(IDI_ICON1, "openxcom.png", _screen->getWindow());

	// And only then you can think about grabbing the mouse
	SDL_bool captureMouse = Options::captureMouse? SDL_TRUE : SDL_FALSE;
	SDL_SetWindowGrab(_screen->getWindow(), captureMouse);

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

	_timeOfLastFrame = 0;
}

/**
 * Deletes the display screen, cursor, states and shuts down all the SDL subsystems.
 */
Game::~Game()
{
	Sound::stop();
	Music::stop();

	for (auto* state : _states)
	{
		delete state;
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

	int numTouchDevices = SDL_GetNumTouchDevices();
	if (!numTouchDevices)
	{
		// Workaround for Macs that don't report their touch capabilities,
		// but still report finger presses for their touchpads.
		SDL_EventState(SDL_FINGERDOWN, SDL_IGNORE);
		SDL_EventState(SDL_FINGERUP, SDL_IGNORE);
		SDL_EventState(SDL_FINGERMOTION, SDL_IGNORE);
		SDL_EventState(SDL_MULTIGESTURE, SDL_IGNORE);
		// FIXME: Adjust scaling so that there's no need for these lines.
	}
	std::vector<SDL_TouchID> touchDevices;
	for(int i = 0; i < numTouchDevices; ++i)
	{
		touchDevices.push_back(SDL_GetTouchDevice(i));
	}
	bool hadFingerUp = true;
	bool isTouched = false;
	SDL_Event reservedMUpEvent;
	Log(LOG_INFO) << "SDL reports this number of touch devices present: " << SDL_GetNumTouchDevices();

	while (!_quit)
	{
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

		// This is a hack to check if we've missed the fingerUp event.
		// Sometimes the fingerUp event doesn't get sent, which causes all sorts of
		// fun things, like stuck buttons and whatnot. This code tries to check if
		// there should have been such event (i.e. there's no fingers present on the
		// touchscreen) and sends the appropriate event.
		// Then again, if the fingerUp is not registered by SDL, then we're screwed.
		isTouched = false;
		if (Options::fakeEvents)
		{
			if (!hadFingerUp)
			{
				isTouched = CrossPlatform::getPointerState(0, 0) > 0;
				if (!isTouched)
				{
					// NOTE: This code only sends ONE mousebuttonup event. May be a source of bugs.
					// We shouldn't end up here, but whatever.
					reservedMUpEvent.type = SDL_MOUSEBUTTONUP;
					if (Options::logTouch)
					{
						Log(LOG_INFO) << "Sending fake mouseButtonUp event; event details: x: " << reservedMUpEvent.button.x << ", y: " << reservedMUpEvent.button.y;
					}
					Action fakeAction = Action(&reservedMUpEvent, _screen->getXScale(), _screen->getYScale(), _screen->getCursorTopBlackBand(), _screen->getCursorLeftBlackBand());
					// Screen and fpsCounter don't care for our mouse events.
					_cursor->handle(&fakeAction);
					_states.back()->handle(&fakeAction);
					hadFingerUp = true;
				}
			}
		}

		// Process events
		while (SDL_PollEvent(&_event))
		{
			if (CrossPlatform::isQuitShortcut(_event))
				_event.type = SDL_QUIT;
			switch (_event.type)
			{
				case SDL_QUIT:
					quit();
					break;
				/* Don't pause/resume twice, let Music handle the music */
				case SDL_APP_WILLENTERBACKGROUND:
                case SDL_APP_DIDENTERBACKGROUND:
					Music::pause();
					// Workaround for SDL2_mixer bug https://bugzilla.libsdl.org/show_bug.cgi?id=2480
					//SDL_LockAudio();
					// Probably won't do a thing, but still
					runningState = PAUSED;
					break;
				case SDL_APP_WILLENTERFOREGROUND:
                case SDL_APP_DIDENTERFOREGROUND:
					runningState = RUNNING;
					// Workaround for SDL2_mixer bug https://bugzilla.libsdl.org/show_bug.cgi?id=2480
					//SDL_UnlockAudio();
					Music::resume();
					break;
				/* Watch for these messages for debugging purposes */
				case SDL_APP_LOWMEMORY:
					Log(LOG_WARNING) << "Warning! We're low on memory! Better make a backup!";
					break;
				case SDL_APP_TERMINATING:
					Log(LOG_WARNING) << "The OS is not happy with us! We're gonna die!";
					break;
				// Process touch-related events first, because it's all a terrible hack and I'm a terrible person. --sfalexrog
				// OpenXcom is designed around using mouse as an input device, so it doesn't really care about finger events.
				// But that shouldn't be a problem, since SDL2 already has mouse emulation built in! So what's all that code about?
				// Well, actually the SDL2 code tries to be "smart" about the position of mouse pointer, and scales it to fit
				// into the "virtual viewport". And OpenXcom, being based on SDL1.2, already does it itself, which causes all sorts
				// of problems. Also, SDL2's code works fine while you have only one finger on the screen, less so when you're
				// taking advantage of your multitouch display. This code only creates events for the first finger on the screen,
				// ignoring all others (which also might be not a good thing, since if you're using multitouch, you probably don't
				// want to have any single finger input anyway, but oh well.)
				case SDL_FINGERDOWN:
					// Begin tracking our finger.
					hadFingerUp = false;
					[[gnu::fallthrough]];
				case SDL_FINGERUP:
					// Okay, maybe we don't need to ask twice.
					// We don't set hadFingerUp here because it's set down the path.
					[[gnu::fallthrough]];
				case SDL_FINGERMOTION:
				{
					// For now we're translating events from the first finger into mouse events.
					// FIXME: Note that we're using SDL_FingerID of 0 to specify this "first finger".
					// This will likely break with things like active styluses.
					SDL_Event fakeEvent;

					fakeEvent.type = SDL_FIRSTEVENT; // This one is used internally by SDL, for us it's an empty event we don't handle
					if ((_event.type == SDL_FINGERMOTION) ||
						(_event.type == SDL_FINGERDOWN) ||
						(_event.type == SDL_FINGERUP))
					{
						if (Options::logTouch)
						{
							Log(LOG_INFO) << "Got a TouchFinger event; details: ";
							switch (_event.type)
							{
								case SDL_FINGERMOTION:
									Log(LOG_INFO) << " type: SDL_FINGERMOTION";
									break;
								case SDL_FINGERDOWN:
									Log(LOG_INFO) << " type: SDL_FINGERDOWN";
									break;
								case SDL_FINGERUP:
									Log(LOG_INFO) << " type: SDL_FINGERUP";
									break;
								default:
									Log(LOG_INFO) << " type: UNKNOWN!";
							}
							Log(LOG_INFO) << " timestamp: " << _event.tfinger.timestamp << ", touchID: " << _event.tfinger.touchId << ", fingerID: " << _event.tfinger.fingerId;
							Log(LOG_INFO) << " x: " << _event.tfinger.x << ", y: " << _event.tfinger.y << ", dx: " << _event.tfinger.dx << ", dy: " << _event.tfinger.dy;
						}
						// FIXME: Better check the truthness of the following sentence!
						// On Android, fingerId of 0 corresponds to the first finger on the screen.
						// Finger index of 0 _should_ mean the first finger on the screen,
						// but that might be platform-dependent as well.
						SDL_Finger *finger = SDL_GetTouchFinger(_event.tfinger.touchId, 0);
						// If the event was fired when the user lifted his finger, we might not get an SDL_Finger struct,
						// because the finger's not even there. So we should also check if the corresponding touchscreen
						// no longer registers any presses.
						int numFingers = SDL_GetNumTouchFingers(_event.tfinger.touchId);
						if ((numFingers == 0) || (finger && (finger->id == _event.tfinger.fingerId)))
						{
							// Note that we actually handle fingermotion, so emulating it may cause bugs.
							if (_event.type == SDL_FINGERMOTION)
							{
								fakeEvent.type = SDL_MOUSEMOTION;
								fakeEvent.motion.x = _event.tfinger.x * Options::displayWidth;
								fakeEvent.motion.y = _event.tfinger.y * Options::displayHeight;
								fakeEvent.motion.xrel = _event.tfinger.dx * Options::displayWidth;
								fakeEvent.motion.yrel = _event.tfinger.dy * Options::displayHeight;
								fakeEvent.motion.timestamp = _event.tfinger.timestamp;
								fakeEvent.motion.state = SDL_BUTTON(1);
							}
							else
							{
								if (_event.type == SDL_FINGERDOWN)
								{
									fakeEvent.type = SDL_MOUSEBUTTONDOWN;
									fakeEvent.button.type = SDL_MOUSEBUTTONDOWN;
									fakeEvent.button.state = SDL_PRESSED;
								}
								else
								{
									hadFingerUp = true;
									fakeEvent.type = SDL_MOUSEBUTTONUP;
									fakeEvent.button.type = SDL_MOUSEBUTTONUP;
									fakeEvent.button.state = SDL_RELEASED;
								}
								fakeEvent.button.timestamp = _event.tfinger.timestamp;
								fakeEvent.button.x = _event.tfinger.x * Options::displayWidth;
								fakeEvent.button.y = _event.tfinger.y * Options::displayHeight;
								fakeEvent.button.button = SDL_BUTTON_LEFT;
							}
						}

					}
					// FIXME: An alternative to this code duplication is very welcome.
					if (fakeEvent.type != SDL_FIRSTEVENT)
					{
						// Preserve current event, we might need it.
						reservedMUpEvent = fakeEvent;
						Action fakeAction = Action(&fakeEvent, _screen->getXScale(), _screen->getYScale(), _screen->getCursorTopBlackBand(), _screen->getCursorLeftBlackBand());
						// Safely ignore _screen and _fpscounter
						// Might want to update _cursor, though.
						_cursor->handle(&fakeAction);
						_states.back()->handle(&fakeAction);
					}
				}
				[[gnu::fallthrough]];
				case SDL_MULTIGESTURE:
					if (Options::logTouch)
					{
						Log(LOG_INFO) << "Got a MultiGesture event, details:";
						Log(LOG_INFO) << " timestamp: " << _event.mgesture.timestamp << ", touchID: " << _event.mgesture.touchId;
						Log(LOG_INFO) << " numFingers: " << _event.mgesture.numFingers << ", x: " << _event.mgesture.x << ", y: " << _event.mgesture.y;
						Log(LOG_INFO) << " dDist: " << _event.mgesture.dDist << ", dTheta: " << _event.mgesture.dTheta;
					}
					[[gnu::fallthrough]];

#if 0
				// SDL2 handles things differently, so this is basically commented out for historical purposes.
				case SDL_ACTIVEEVENT:
					// An event other than SDL_APPMOUSEFOCUS change happened.
					if (reinterpret_cast<SDL_ActiveEvent*>(&_event)->state & ~SDL_APPMOUSEFOCUS)
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
							Options::newDisplayWidth = Options::displayWidth = std::max(Screen::ORIGINAL_WIDTH, _event.resize.w);
							Options::newDisplayHeight = Options::displayHeight = std::max(Screen::ORIGINAL_HEIGHT, _event.resize.h);
							int dX = 0, dY = 0;
							Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, false);
							Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, false);
							for (auto* state : _states)
							{
								state->resize(dX, dY);
							}
							_screen->resetDisplay();
						}
						else
						{
							startupEvent = false;
						}
					}
					break;
#endif
				case SDL_WINDOWEVENT:
					switch(_event.window.event)
					{
						case SDL_WINDOWEVENT_RESIZED:
							// It should be better to handle SDL_WINDOWEVENT_SIZE_CHANGED, but
							// it won't tell the new width and height.
							// New width is in data1, new height is in data2.
							// Otherwise the code is carbon-copied from SDL1.2 resize code.

							// Okay, if you got this event, this probably means that your window IS resizable.
							//if (Options::allowResize)
							{
								int newWidth, newHeight;
								SDL_GetWindowSize(_screen->getWindow(), &newWidth, &newHeight);
								Options::newDisplayWidth = Options::displayWidth = std::max(Screen::ORIGINAL_WIDTH, newWidth);
								Options::newDisplayHeight = Options::displayHeight = std::max(Screen::ORIGINAL_HEIGHT, newHeight);
								int dX = 0, dY = 0;
								Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, false);
								Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, false);
								for (auto* state : _states)
								{
									state->resize(dX, dY);
								}
								_screen->resetDisplay();
							}
							break;
						case SDL_WINDOWEVENT_FOCUS_LOST:
							runningState = kbFocusRun[Options::pauseMode];
							break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							runningState = RUNNING;
							break;
						case SDL_WINDOWEVENT_MINIMIZED:
						case SDL_WINDOWEVENT_HIDDEN:
							runningState = stateRun[Options::pauseMode];
							break;
						case SDL_WINDOWEVENT_SHOWN:
						case SDL_WINDOWEVENT_EXPOSED:
						case SDL_WINDOWEVENT_RESTORED:
							runningState = RUNNING;
					}
					[[gnu::fallthrough]];
				case SDL_MOUSEMOTION:
					// With SDL2 we can have both events from a real mouse
					// and events from a touch-emulated mouse.
					// This code should prevent these events from
					// interfering with each other.
					if (_event.motion.which == SDL_TOUCH_MOUSEID)
					{
						if (Options::logTouch)
						{
							Log(LOG_INFO) << "Got a spurious MouseID event; ignoring...";
						}
						break;
					}
					[[gnu::fallthrough]];
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if (_event.button.which == SDL_TOUCH_MOUSEID)
					{
						if (Options::logTouch)
						{
							Log(LOG_INFO) << "Got a spurious MouseID event; ignoring...";
						}
						break;
					}
					[[gnu::fallthrough]];
				case SDL_MOUSEWHEEL:
					if (_event.wheel.which == SDL_TOUCH_MOUSEID)
					{
						break;
					}
					// Skip mouse events if they're disabled
					if (!_mouseActive) continue;
					// re-gain focus on mouse-over or keypress.
					runningState = RUNNING;
					// Go on, feed the event to others
					FALLTHROUGH;
				default:
					Action action = Action(&_event, _screen->getXScale(), _screen->getYScale(), _screen->getCursorTopBlackBand(), _screen->getCursorLeftBlackBand());
					_screen->handle(&action);
					_cursor->handle(&action);
					_fpsCounter->handle(&action);
					if (action.getDetails()->type == SDL_KEYDOWN)
					{
						// "ctrl-g" grab input
						// (Since we're on Android, we're having no ctrl-g

						if (action.getDetails()->key.keysym.sym == SDLK_g && isCtrlPressed())
						{
							Options::captureMouse = !Options::captureMouse;
							SDL_bool captureMouse = Options::captureMouse ? SDL_TRUE : SDL_FALSE;
							SDL_SetWindowGrab(_screen->getWindow(), captureMouse);
						}
						// "ctrl-n" notes UI
						else if (action.getDetails()->key.keysym.sym == SDLK_n && isCtrlPressed() && !isAltPressed())
						{
							if (_save && !containsNotesState())
							{
								if (_save->getSavedBattle())
								{
									if (!_save->getSavedBattle()->isBattlescapeStateBusy())
									{
										pushState(new NotesState(OPT_BATTLESCAPE));
									}
								}
								else
								{
									pushState(new NotesState(OPT_GEOSCAPE));
								}
							}
						}
						else if (Options::debug)
						{
							if (action.getDetails()->key.keysym.sym == SDLK_t && isCtrlPressed())
							{
								pushState(new TestState);
							}
							// "ctrl-u" debug UI
							else if (action.getDetails()->key.keysym.sym == SDLK_u && isCtrlPressed())
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
			if (Options::FPS > 0)
			{
				// Update our FPS delay time based on the time of the last draw.
				int fps = SDL_GetWindowFlags(getScreen()->getWindow()) & SDL_WINDOW_INPUT_FOCUS ? Options::FPS : Options::FPSInactive;
				_timeUntilNextFrame = (1000.0f / fps) - (SDL_GetTicks() - _timeOfLastFrame);
			}
			else
			{
				_timeUntilNextFrame = 0;
			}

			if (_init && _timeUntilNextFrame <= 0)
			{
				// make a note of when this frame update occurred.
				_timeOfLastFrame = SDL_GetTicks();
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

		// Save on CPU
		switch (runningState)
		{
			case RUNNING:
				SDL_Delay(1); //Save CPU from going 100%
				break;
			case SLOWED: case PAUSED:
				SDL_Delay(100); break; //More slowing down.
		}
	}

	Options::save();
}

/**
 * Stops the state machine and the game is shut down.
 */
void Game::quit()
{
	// Hard-learned lesson: there's a billion+ situations, where this causes a corrupted save and subsequent crashes. It's not worth it!
#if 0
	// Always save ironman
	if (_save != 0 && _save->isIronman() && !_save->getName().empty())
	{
		std::string filename = CrossPlatform::sanitizeFilename(_save->getName()) + ".sav";
		_save->save(filename, _mod);
	}
#endif
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
 * Sets a new saved game for the game to use.
 * @param save Pointer to the saved game.
 */
void Game::setSavedGame(SavedGame *save)
{
	delete _save;
	_save = save;
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
 * Returns whether a UfopaediaStartState is in the background.
 * @return Is there a UfopaediaStartState in the background?
 */
bool Game::containsUfopaediaStartState() const
{
	for (auto* state : _states)
	{
		auto* pedia = dynamic_cast<UfopaediaStartState*>(state);
		if (pedia)
		{
			return true;
		}
	}
	return false;
}

/**
 * Returns whether a NotesState is in the background.
 * @return Is there a NotesState in the background?
 */
bool Game::containsNotesState() const
{
	for (auto* state : _states)
	{
		auto* notes = dynamic_cast<NotesState*>(state);
		if (notes)
		{
			return true;
		}
	}
	return false;
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

	// No language set, detect based on system
	if (Options::language.empty())
	{
		std::string locale = CrossPlatform::getLocale();
		std::string lang = locale.substr(0, locale.find_first_of('-'));
		// Try to load full locale
		if (Language::isSupported(locale) && FileMap::fileExists("Language/" + locale + ".yml"))
		{
			currentLang = locale;
		}
		else
		{
			// Try to load language locale
			if (Language::isSupported(lang) && FileMap::fileExists("Language/" + lang + ".yml"))
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

	delete _lang;
	_lang = new Language();

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

/**
 * Is CTRL pressed?
 */
bool Game::isCtrlPressed(bool considerTouchButtons) const
{
	if (considerTouchButtons && _ctrl)
	{
		return true;
	}
	return (SDL_GetModState() & KMOD_CTRL) != 0;
}

/**
 * Is ALT pressed?
 */
bool Game::isAltPressed(bool considerTouchButtons) const
{
	if (considerTouchButtons && _alt)
	{
		return true;
	}
	return (SDL_GetModState() & KMOD_ALT) != 0;
}

/**
 * Is SHIFT pressed?
 */
bool Game::isShiftPressed(bool considerTouchButtons) const
{
	if (considerTouchButtons && _shift)
	{
		return true;
	}
	return (SDL_GetModState() & KMOD_SHIFT) != 0;
}

/**
 * Is LMB pressed?
 */
bool Game::isLeftClick(Action* action, bool considerTouchButtons) const
{
	if (considerTouchButtons)
	{
		return (action->getDetails()->button.button == SDL_BUTTON_LEFT) && !_rmb && !_mmb;
	}
	return (action->getDetails()->button.button == SDL_BUTTON_LEFT);
}

/**
 * Is RMB pressed?
 */
bool Game::isRightClick(Action* action, bool considerTouchButtons) const
{
	if (considerTouchButtons)
	{
		return (action->getDetails()->button.button == SDL_BUTTON_RIGHT) || ((action->getDetails()->button.button == SDL_BUTTON_LEFT) && _rmb);
	}
	return (action->getDetails()->button.button == SDL_BUTTON_RIGHT);
}

/**
 * Is MMB pressed?
 */
bool Game::isMiddleClick(Action* action, bool considerTouchButtons) const
{
	if (considerTouchButtons)
	{
		return (action->getDetails()->button.button == SDL_BUTTON_MIDDLE) || ((action->getDetails()->button.button == SDL_BUTTON_LEFT) && _mmb);
	}
	return (action->getDetails()->button.button == SDL_BUTTON_MIDDLE);
}

/**
 * Resets the touch button flags.
 */
void Game::resetTouchButtonFlags()
{
	_ctrl = false;
	_alt = false;
	_shift = false;
	_rmb = false;
	_mmb = false;
}

}
