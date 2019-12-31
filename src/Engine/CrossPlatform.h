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
#include <istream>
#include <SDL.h>
#include <string>
#include <vector>
#include <memory>

namespace OpenXcom
{

/**
 * Generic purpose functions that need different
 * implementations for different platforms.
 */
namespace CrossPlatform
{
	/// Retrieve and decode command-line arguments
	void processArgs (int argc, char *argv[]);
	/// Returns the command-line arguments
	const std::vector<std::string>& getArgs();
	/// Gets the available error dialog.
	void getErrorDialog();
	/// Displays an error message.
	void showError(const std::string &error);
	/// Finds the game's data folders in the system.
	std::vector<std::string> findDataFolders();
	/// Finds the game's user folders in the system.
	std::vector<std::string> findUserFolders();
	/// Finds the game's config folder in the system.
	std::string findConfigFolder();
	/// Searches the data folders and returns the full path for a data file
	/// when given a relative path, like "units/zombie.pck".  returns the passed-in
	/// filename if the file is not found
	std::string searchDataFile(const std::string &filename);
	/// Searches the data folders and returns the full path for a folder
	/// when given a relative path, like "common".  returns the passed-in
	/// dir name if the folder is not found
	std::string searchDataFolder(const std::string &foldername);
	/// Creates a folder.
	bool createFolder(const std::string &path);
	/// Terminates a path.
	std::string convertPath(const std::string &path);
	/// Returns the list of files in a folder as a vector of tuples (filename, id_dir, mtime)
	std::vector<std::tuple<std::string, bool, time_t>> getFolderContents(const std::string &path, const std::string &ext = "");
	/// Checks if the path is an existing folder.
	bool folderExists(const std::string &path);
	/// Checks if the path is an existing file.
	bool fileExists(const std::string &path);
	/// Deletes the specified file.
	bool deleteFile(const std::string &path);
	/// Gets the pathless filename of a file.
	std::string baseFilename(const std::string &path);
	/// Sanitizes the characters in a filename.
	std::string sanitizeFilename(const std::string &filename);
	/// Removes the extension from a filename.
	std::string noExt(const std::string &file);
	/// Gets the extension from a filename.
	std::string getExt(const std::string &file);
	/// Compares the extension of a filename.
	bool compareExt(const std::string &file, const std::string &extension);
	/// Gets the system locale.
	std::string getLocale();
	/// Checks if an event is a quit shortcut.
	bool isQuitShortcut(const SDL_Event &ev);
	/// Gets the modified date of a file.
	time_t getDateModified(const std::string &path);
	/// Converts a timestamp to a string.
	std::pair<std::string, std::string> timeToString(time_t time);
	/// Move/rename a file between paths.
	bool moveFile(const std::string &src, const std::string &dest);
	/// Copy a file between paths.
	bool copyFile(const std::string &src, const std::string &dest);
	/// Writes out a file
	bool writeFile(const std::string& filename, const std::string& data);
	bool writeFile(const std::string& filename, const std::vector<unsigned char>& data);
	/// Reads in a file
	std::unique_ptr<std::istream> readFile(const std::string& filename);
	/// Reads file until "\n---" sequence is met or to the end. To be used only for savegames.
	std::unique_ptr<std::istream> getYamlSaveHeader (const std::string& filename);
	/// Flashes the game window.
	void flashWindow();
	/// Gets the DOS-style executable path.
	std::string getDosPath();
	/// Sets the window icon.
	void setWindowIcon(int winResource, const std::string &unixPath);
	/// Produces a stack trace.
	void stackTrace(void *ctx);
	/// Produces a quick timestamp.
	std::string now();
	/// Produces a crash dump.
	void crashDump(void *ex, const std::string &err);
	/// Log something.
	void log(int, const std::ostringstream& msg);
	/// The log file name
	void setLogFileName(const std::string &path);
	const std::string& getLogFileName();
	/// Get an SDL_RWops to an embedded asset. NULL if not there.
	SDL_RWops *getEmbeddedAsset(const std::string& assetName);
	/// Tests the internet connection.
	bool testInternetConnection(const std::string& url);
	/// Downloads a file from a given URL to the filesystem.
	bool downloadFile(const std::string& url, const std::string& filename);
	/// Is the given version number higher than the current version number?
	bool isHigherThanCurrentVersion(const std::string& newVersion);
	/// Gets the path to the executable file.
	std::string getExeFolder();
	/// Gets the file name of the executable file.
	std::string getExeFilename(bool includingPath);
	/// Starts the update process.
	void startUpdateProcess();

	void memProtect(const void* ptr, size_t size);
	void memUnprotect();
	void memUpdateProtect();
	bool handleGuardPage(void* exc);
}

}
