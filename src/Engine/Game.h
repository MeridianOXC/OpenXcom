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
#include <list>
#include <string>
#include <SDL.h>

namespace OpenXcom
{

class State;
class Screen;
class Cursor;
class Language;
class SavedGame;
class Mod;
class ModInfo;
class FpsCounter;
class Action;
class GeoscapeState;

/**
 * The core of the game engine, manages the game's entire contents and structure.
 * Takes care of encapsulating all the core SDL commands, provides access to all
 * the game's resources and contains a stack state machine to handle all the
 * initializations, events and blits of each state, as well as transitions.
 */
class Game
{
private:
	SDL_Event _event;
	Screen *_screen;
	Cursor *_cursor;
	Language *_lang;
	std::list<State*> _states, _deleted;
	SavedGame *_save;
	Mod *_mod;
	bool _quit, _init, _update;
	FpsCounter *_fpsCounter;
	bool _mouseActive;
	unsigned int _timeOfLastFrame;
	int _timeUntilNextFrame;
	bool _ctrl, _alt, _shift, _rmb, _mmb;
	int _scrollStep;
	static const double VOLUME_GRADIENT;

public:
	/// Creates a new game and initializes SDL.
	Game(const std::string &title);
	/// Cleans up all the game's resources and shuts down SDL.
	~Game();
	/// Starts the game's state machine.
	void run();
	/// Quits the game.
	void quit();
	/// Sets the game's audio volume.
	void setVolume(int sound, int music, int ui);
	/// Adjusts a linear volume level to an exponential one.
	static double volumeExponent(int volume);
	/// Gets the game's display screen.
	Screen *getScreen() const { return _screen; }
	/// Gets the game's cursor.
	Cursor *getCursor() const { return _cursor; }
	/// Gets the FpsCounter.
	FpsCounter *getFpsCounter() const { return _fpsCounter; }
	/// Resets the state stack to a new state.
	void setState(State *state);
	/// Pushes a new state into the state stack.
	void pushState(State *state);
	/// Pops the last state from the state stack.
	void popState();
	/// Gets the currently loaded language.
	Language *getLanguage() const { return _lang; }
	/// Gets the currently loaded saved game.
	SavedGame *getSavedGame() const { return _save; }
	/// Sets a new saved game for the game.
	void setSavedGame(SavedGame *save);
	/// Gets the currently loaded mod.
	Mod *getMod() const { return _mod; }
	/// Loads the mods specified in the game options.
	void loadMods();
	/// Sets whether the mouse cursor is activated.
	void setMouseActive(bool active);
	/// Returns whether current state is the param state
	bool isState(State *state) const;
	/// Returns whether a UfopaediaStartState is in the background.
	bool containsUfopaediaStartState() const;
	/// Returns whether a NotesState is in the background.
	bool containsNotesState() const;
	/// Returns the GeoscapeState from the background (if available).
	GeoscapeState* getGeoscapeState() const;
	/// Returns whether the game is shutting down.
	bool isQuitting() const;
	/// Loads the default and current language.
	void loadLanguages();
	/// Sets up the audio.
	void initAudio();
	/// Sets the update flag.
	void setUpdateFlag(bool update) { _update = update; }
	/// Returns the update flag.
	bool getUpdateFlag() const { return _update; }

	/// Is CTRL pressed?
	bool isCtrlPressed(bool considerTouchButtons = false) const;
	/// Is ALT pressed?
	bool isAltPressed(bool considerTouchButtons = false) const;
	/// Is SHIFT pressed?
	bool isShiftPressed(bool considerTouchButtons = false) const;

	/// Is LMB pressed?
	bool isLeftClick(Action* action, bool considerTouchButtons = false) const;
	/// Is RMB pressed?
	bool isRightClick(Action* action, bool considerTouchButtons = false) const;
	/// Is MMB pressed?
	bool isMiddleClick(Action* action, bool considerTouchButtons = false) const;

	/// Resets the touch button flags.
	void resetTouchButtonFlags();

	/// Sets the _ctrl flag.
	void setCtrlPressedFlag(bool newValue) { _ctrl = newValue; }
	void toggleCtrlPressedFlag() { _ctrl = !_ctrl; }
	/// Sets the _alt flag.
	void setAltPressedFlag(bool newValue) { _alt = newValue; }
	void toggleAltPressedFlag() { _alt = !_alt; }
	/// Sets the _shift flag.
	void setShiftPressedFlag(bool newValue) { _shift = newValue; }
	void toggleShiftPressedFlag() { _shift = !_shift; }

	/// Gets the _ctrl flag.
	bool getCtrlPressedFlag() const { return _ctrl; }
	/// Gets the _alt flag.
	bool getAltPressedFlag() const { return _alt; }
	/// Gets the _shift flag.
	bool getShiftPressedFlag() const { return _shift; }

	/// Sets the _rmb flag.
	void setRMBFlag(bool newValue) { _rmb = newValue; }
	void toggleRMBFlag() { _rmb = !_rmb; }
	/// Sets the _mmb flag.
	void setMMBFlag(bool newValue) { _mmb = newValue; }
	void toggleMMBFlag() { _mmb = !_mmb; }

	/// Gets the _rmb flag.
	bool getRMBFlag() const { return _rmb; }
	/// Gets the _mmb flag.
	bool getMMBFlag() const { return _mmb; }

	/// Sets the scroll step value.
	void setScrollStep(int newValue) { _scrollStep = newValue; }

	/// Gets the scroll step value.
	int getScrollStep() const { return _scrollStep; }
};

}
