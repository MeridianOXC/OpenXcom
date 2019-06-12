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
#include "../Engine/State.h"

namespace OpenXcom
{

class TextButton;
class Window;
class Text;

// Utility class for enqueuing a state in the stack that goes to the main menu
class GoToMainMenuState : public State
{
private:
	bool _updateCheck;
public:
	GoToMainMenuState(bool updateCheck = false);
	~GoToMainMenuState();
	void init() override;
};

/**
 * Main Menu window displayed when first
 * starting the game.
 */
class MainMenuState : public State
{
private:
	TextButton *_btnNewGame, *_btnNewBattle, *_btnLoad, *_btnOptions, *_btnMods, *_btnQuit, *_btnUpdate, *_btnModTools;
	Window *_window;
	Text *_txtTitle, *_txtUpdateInfo;
#ifdef _WIN32
	bool _debugInVisualStudio;
	std::string _newVersion;
#endif
public:
	/// Creates the Main Menu state.
	MainMenuState(bool updateCheck = false);
	/// Cleans up the Main Menu state.
	~MainMenuState();
	/// Handler for clicking the New Game button.
	void btnNewGameClick(Action *action);
	/// Handler for clicking the New Battle button.
	void btnNewBattleClick(Action *action);
	/// Handler for clicking the Load Saved Game button.
	void btnLoadClick(Action *action);
	/// Handler for clicking the Options button.
	void btnOptionsClick(Action *action);
	/// Handler for clicking the Mods button.
	void btnModsClick(Action *action);
	/// Handler for clicking the Quit button.
	void btnQuitClick(Action *action);
	/// Handler for clicking the Update button.
	void btnUpdateClick(Action* action);
	/// Handler for clicking the Mod Tools button.
	void btnModToolsClick(Action *action);
	/// Update the resolution settings, we just resized the window.
	void resize(int &dX, int &dY) override;
	void init() override;
};

}
