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
#include "MainMenuState.h"
#include <sstream>
#include "../version.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "NewGameState.h"
#include "NewBattleState.h"
#include "ListLoadState.h"
#include "OptionsVideoState.h"
#include "ModListState.h"
#include "../Engine/Options.h"
#include "../Engine/FileMap.h"
#include "../Engine/SDL2Helpers.h"
#include <fstream>

namespace OpenXcom
{

GoToMainMenuState::GoToMainMenuState(bool updateCheck) : _updateCheck(updateCheck)
{
	// empty
}

GoToMainMenuState::~GoToMainMenuState()
{
	// empty
}

void GoToMainMenuState::init()
{
	Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, true);
	_game->getScreen()->resetDisplay(false);
	_game->setState(new MainMenuState(_updateCheck));
}

/**
 * Initializes all the elements in the Main Menu window.
 * @param updateCheck Perform update check?
 */
MainMenuState::MainMenuState(bool updateCheck)
{
#ifdef _WIN32
	_debugInVisualStudio = false;
#endif

	// Create objects
	_window = new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_btnNewGame = new TextButton(92, 20, 64, 90);
	_btnNewBattle = new TextButton(92, 20, 164, 90);
	_btnLoad = new TextButton(92, 20, 64, 118);
	_btnOptions = new TextButton(92, 20, 164, 118);
	_btnMods = new TextButton(92, 20, 64, 146);
	_btnQuit = new TextButton(92, 20, 164, 146);
	_btnUpdate = new TextButton(72, 16, 209, 27);
	_txtUpdateInfo = new Text(320, 17, 0, 11);
	_txtTitle = new Text(256, 30, 32, 45);

	// Set palette
	setInterface("mainMenu");

	add(_window, "window", "mainMenu");
	add(_btnNewGame, "button", "mainMenu");
	add(_btnNewBattle, "button", "mainMenu");
	add(_btnLoad, "button", "mainMenu");
	add(_btnOptions, "button", "mainMenu");
	add(_btnMods, "button", "mainMenu");
	add(_btnQuit, "button", "mainMenu");
	add(_btnUpdate, "button", "mainMenu");
	add(_txtUpdateInfo, "text", "mainMenu");
	add(_txtTitle, "text", "mainMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "mainMenu");

	_btnNewGame->setText(tr("STR_NEW_GAME"));
	_btnNewGame->onMouseClick((ActionHandler)&MainMenuState::btnNewGameClick);

	_btnNewBattle->setText(tr("STR_NEW_BATTLE"));
	_btnNewBattle->onMouseClick((ActionHandler)&MainMenuState::btnNewBattleClick);

	_btnLoad->setText(tr("STR_LOAD_SAVED_GAME"));
	_btnLoad->onMouseClick((ActionHandler)&MainMenuState::btnLoadClick);

	_btnOptions->setText(tr("STR_OPTIONS"));
	_btnOptions->onMouseClick((ActionHandler)&MainMenuState::btnOptionsClick);

	_btnMods->setText(tr("STR_MODS"));
	_btnMods->onMouseClick((ActionHandler)&MainMenuState::btnModsClick);

	_btnQuit->setText(tr("STR_QUIT"));
	_btnQuit->onMouseClick((ActionHandler)&MainMenuState::btnQuitClick);

	_btnUpdate->setText(tr("STR_UPDATE"));
	_btnUpdate->onMouseClick((ActionHandler)& MainMenuState::btnUpdateClick);
	_btnUpdate->setVisible(false);

	_txtUpdateInfo->setAlign(ALIGN_CENTER);
	_txtUpdateInfo->setWordWrap(true);
	_txtUpdateInfo->setText(tr("STR_LATEST_VERSION_INFO"));
	_txtUpdateInfo->setVisible(false);

#ifdef _WIN32
	//_debugInVisualStudio = true; // uncomment when debugging in Visual Studio (working dir and exe dir are not the same)

	// delete (old) update batch file
	if (updateCheck && CrossPlatform::fileExists("oxce-upd.bat"))
	{
		CrossPlatform::deleteFile("oxce-upd.bat");
	}

	if (updateCheck && Options::oxceUpdateCheck)
	{
		int checkProgress = 0;
		const std::string relativeExeFilename = (_debugInVisualStudio ? "Debug/" + CrossPlatform::getExeFilename(false) : CrossPlatform::getExeFilename(false));
		// (naive) check if working directory and exe directory are the same
		if (!relativeExeFilename.empty() && CrossPlatform::fileExists(relativeExeFilename))
		{
			checkProgress = 1;
			const std::string internetConnectionCheckUrl = "https://openxcom.org/";
			if (CrossPlatform::testInternetConnection(internetConnectionCheckUrl))
			{
				checkProgress = 2;
				const std::string updateMetadataUrl = "https://openxcom.org/oxce/update.txt";
				const std::string updateMetadataFilename = Options::getUserFolder() + "oxce-update.txt";
				if (CrossPlatform::downloadFile(updateMetadataUrl, updateMetadataFilename))
				{
					checkProgress = 3;
					if (CrossPlatform::fileExists(updateMetadataFilename))
					{
						checkProgress = 4;
						try
						{
							YAML::Node doc = YAML::Load(*CrossPlatform::readFile(updateMetadataFilename));
							checkProgress = 5;
							if (doc["updateInfo"])
							{
								checkProgress = 6;
								std::string msg = doc["updateInfo"].as<std::string>();
								_txtUpdateInfo->setText(msg);
								_txtUpdateInfo->setVisible(true);
							}
							else if (doc["newVersion"])
							{
								checkProgress = 7;
								_newVersion = doc["newVersion"].as<std::string>();
								if (CrossPlatform::isHigherThanCurrentVersion(_newVersion))
									_btnUpdate->setVisible(true);
								else
									_txtUpdateInfo->setVisible(true);
							}
							CrossPlatform::deleteFile(updateMetadataFilename);
						}
						catch (YAML::Exception &e)
						{
							Log(LOG_ERROR) << e.what();
						}
					}
				}
			}
		}
		Log(LOG_INFO) << "Update check status: " << checkProgress << "; newVersion: v" << _newVersion << "; ";
	}
#endif

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	std::ostringstream title;
	title << tr("STR_OPENXCOM") << Unicode::TOK_NL_SMALL;
	title << "OpenXcom " << OPENXCOM_VERSION_SHORT << OPENXCOM_VERSION_GIT;
	_txtTitle->setText(title.str());
}

void MainMenuState::init()
{
	State::init();
	if (Options::getLoadLastSave() && _game->getSavedGame()->getList(_game->getLanguage(), true).size() > 0)
	{
		Log(LOG_INFO) << "Loading last saved game";
		btnLoadClick(NULL);
	}
}

/**
 *
 */
MainMenuState::~MainMenuState()
{

}

/**
 * Opens the New Game window.
 * @param action Pointer to an action.
 */
void MainMenuState::btnNewGameClick(Action *)
{
	_game->pushState(new NewGameState);
}

/**
 * Opens the New Battle screen.
 * @param action Pointer to an action.
 */
void MainMenuState::btnNewBattleClick(Action *)
{
	_game->pushState(new NewBattleState);
}

/**
 * Opens the Load Game screen.
 * @param action Pointer to an action.
 */
void MainMenuState::btnLoadClick(Action *)
{
	_game->pushState(new ListLoadState(OPT_MENU));
}

/**
 * Opens the Options screen.
 * @param action Pointer to an action.
 */
void MainMenuState::btnOptionsClick(Action *)
{
	Options::backupDisplay();
	_game->pushState(new OptionsVideoState(OPT_MENU));
}

/**
* Opens the Mods screen.
* @param action Pointer to an action.
*/
void MainMenuState::btnModsClick(Action *)
{
	_game->pushState(new ModListState);
}

/**
 * Quits the game.
 * @param action Pointer to an action.
 */
void MainMenuState::btnQuitClick(Action *)
{
	_game->quit();
}

/**
 * Updates OpenXcom to the newest version.
 * @param action Pointer to an action.
 */
void MainMenuState::btnUpdateClick(Action*)
{
#ifdef _WIN32
	const std::string subdir = "v" + _newVersion + "/";

#ifdef _WIN64
	const std::string relativeExeZipFileName = _debugInVisualStudio ? "Debug/exe64.zip" : "exe64.zip";
#else
	const std::string relativeExeZipFileName = _debugInVisualStudio ? "Debug/exe.zip" : "exe.zip";
#endif
	const std::string relativeExeNewFileName = _debugInVisualStudio ? "Debug/OpenXcomEx.exe.new" : "OpenXcomEx.exe.new";

	const std::string commonDirFilename = Options::getDataFolder() + "common";
	const std::string commonZipFilename = Options::getDataFolder() + "common.zip";
	const std::string commonZipUrl = "https://openxcom.org/oxce/" + subdir + "common.zip";

	const std::string standardDirFilename = Options::getDataFolder() + "standard";
	const std::string standardZipFilename = Options::getDataFolder() + "standard.zip";
	const std::string standardZipUrl = "https://openxcom.org/oxce/" + subdir + "standard.zip";

	const std::string now = CrossPlatform::now();

	const std::string exePath = CrossPlatform::getExeFolder();
	const std::string exeFilenameOnly = CrossPlatform::getExeFilename(false);
	const std::string exeFilenameFullPath = CrossPlatform::getExeFilename(true);

#ifdef _WIN64
	const std::string exeZipFilename = exePath + "exe64.zip";
#else
	const std::string exeZipFilename = exePath + "exe.zip";
#endif
	const std::string exeNewFilename = exePath + "OpenXcomEx.exe.new";
#ifdef _WIN64
	const std::string exeZipUrl = "https://openxcom.org/oxce/" + subdir + "exe64.zip";
#else
	const std::string exeZipUrl = "https://openxcom.org/oxce/" + subdir + "exe.zip";
#endif

	// stop using the common/standard zip files, so that we can back them up
	FileMap::clear(true, false);

	// 0. backup the exe
	if (CrossPlatform::fileExists(exeFilenameFullPath))
	{
		if (CrossPlatform::copyFile(exeFilenameFullPath, exeFilenameFullPath + "-" + now + ".bak"))
			Log(LOG_INFO) << "Update step 0 done.";
		else return;
	}

	// 1. backup common dir
	if (CrossPlatform::fileExists(commonDirFilename))
	{
		if (CrossPlatform::moveFile(commonDirFilename, commonDirFilename + "-" + now))
			Log(LOG_INFO) << "Update step 1 done.";
		else return;
	}

	// 2. backup common zip
	if (CrossPlatform::fileExists(commonZipFilename))
	{
		if (CrossPlatform::moveFile(commonZipFilename, commonZipFilename + "-" + now + ".bak"))
			Log(LOG_INFO) << "Update step 2 done.";
		else return;
	}

	// 3. backup standard dir
	if (CrossPlatform::fileExists(standardDirFilename))
	{
		if (CrossPlatform::moveFile(standardDirFilename, standardDirFilename + "-" + now))
			Log(LOG_INFO) << "Update step 3 done.";
		else return;
	}

	// 4. backup standard zip
	if (CrossPlatform::fileExists(standardZipFilename))
	{
		if (CrossPlatform::moveFile(standardZipFilename, standardZipFilename + "-" + now + ".bak"))
			Log(LOG_INFO) << "Update step 4 done.";
		else return;
	}

	// 5. delete exe zip
	if (CrossPlatform::fileExists(exeZipFilename))
	{
		if (CrossPlatform::deleteFile(exeZipFilename))
			Log(LOG_INFO) << "Update step 5 done.";
		else return;
	}

	// 6. delete unpacked exe zip
	if (CrossPlatform::fileExists(exeNewFilename))
	{
		if (CrossPlatform::deleteFile(exeNewFilename))
			Log(LOG_INFO) << "Update step 6 done.";
		else return;
	}

	// 7. download common zip
	if (CrossPlatform::downloadFile(commonZipUrl, commonZipFilename))
	{
		Log(LOG_INFO) << "Update step 7 done.";
	}
	else return;

	// 8. download standard zip
	if (CrossPlatform::downloadFile(standardZipUrl, standardZipFilename))
	{
		Log(LOG_INFO) << "Update step 8 done.";
	}
	else return;

	// 9. download exe zip
	if (CrossPlatform::downloadFile(exeZipUrl, exeZipFilename))
	{
		Log(LOG_INFO) << "Update step 9 done.";
	}
	else return;

	// 10. extract exe zip
	if (CrossPlatform::fileExists(exeZipFilename) && CrossPlatform::fileExists(relativeExeZipFileName))
	{
		const std::string file_to_extract = "OpenXcomEx.exe.new";
		SDL_RWops *rwo_read = FileMap::zipGetFileByName(relativeExeZipFileName, file_to_extract);
		if (!rwo_read) {
			Log(LOG_ERROR) << "Step 10a: failed to unzip file.";
			return;
		}
		size_t size = 0;
		auto data = SDL_LoadFile_RW(rwo_read, &size, SDL_TRUE);
		if (!data) {
			Log(LOG_ERROR) << "Step 10b: failed to unzip file." << SDL_GetError(); // out of memory for a copy ?
			return;
		}
		SDL_RWops *rwo_write = SDL_RWFromFile(relativeExeNewFileName.c_str(), "wb");
		if (!rwo_write) {
			Log(LOG_ERROR) << "Step 10c: failed to open exe.new file for writing." << SDL_GetError();
			return;
		}
		auto wsize = SDL_RWwrite(rwo_write, data, size, 1);
		if (wsize != 1) {
			Log(LOG_ERROR) << "Step 10d: failed to write exe.new file." << SDL_GetError();
			return;
		}
		if (SDL_RWclose(rwo_write)) {
			Log(LOG_ERROR) << "Step 10e: failed to write exe.new file." << SDL_GetError();
			return;
		}
	} else {
		Log(LOG_ERROR) << "Update step 10 failed."; // exe dir and working dir not the same
		return;
	}

	// 11. check if extracted exe exists
	if (!CrossPlatform::fileExists(exeNewFilename))
	{
		Log(LOG_ERROR) << "Update step 11 failed.";
		return;
	}

	// 12. delete exe zip (again)
	if (CrossPlatform::fileExists(exeZipFilename))
	{
		if (CrossPlatform::deleteFile(exeZipFilename))
			Log(LOG_INFO) << "Update step 12 done.";
		else return;
	}

	// 13. create the update batch file
	{
		std::ofstream batch;
		batch.open("oxce-upd.bat", std::ios::out);

		batch << "@echo OFF\n";
		batch << "echo OpenXcom is updating, please wait...\n";
		batch << "timeout 5\n";
		if (!_debugInVisualStudio)
		{
			batch << "echo Removing the old version...\n";
			batch << "del " << exeFilenameOnly << "\n";
			batch << "echo Preparing the new version...\n";
			batch << "ren OpenXcomEx.exe.new " << exeFilenameOnly << "\n";
			batch << "echo Starting the new version...\n";
			batch << "timeout 2\n";
			batch << "start " << exeFilenameOnly << "\n"; // asynchronous
			batch << "exit\n";
		}

		batch.close();
	}

	// 14. Clear the SDL event queue (i.e. ignore input from impatient users)
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		// do nothing
	}

	Log(LOG_INFO) << "Update prepared, restarting.";
	_game->setUpdateFlag(true);
	_game->quit();
#endif
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void MainMenuState::resize(int &dX, int &dY)
{
	dX = Options::baseXResolution;
	dY = Options::baseYResolution;
	Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, true);
	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;
	State::resize(dX, dY);
}

}
