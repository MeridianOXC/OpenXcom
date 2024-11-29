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
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "CrossPlatform.h"
#include <exception>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>
#include <list>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <assert.h>
#include "Logger.h"
#include "Exception.h"
#include "Options.h"
#include "Unicode.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <wininet.h>
#include <urlmon.h>
#ifndef __NO_DBGHELP
#include <dbghelp.h>
#endif
#ifdef __MINGW32__
#include <cxxabi.h>
#endif
#define EXCEPTION_CODE_CXX 0xe06d7363
#ifndef __GNUC__
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")
#ifndef __NO_DBGHELP
#pragma comment(lib, "dbghelp.lib")
#endif
#endif
#else		/* #ifdef _WIN32 */
#include <iostream>
#include <fstream>
#include <locale>
#include <SDL_image.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <pwd.h>
#ifndef __CYGWIN__
#include <execinfo.h>
#endif
#include <cxxabi.h>
#include <dlfcn.h>
#include <dirent.h>
#include "Unicode.h"
#endif		/* #ifdef _WIN32 */
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef __HAIKU__
#include <FindDirectory.h>
#include <StorageDefs.h>
#endif
#include "FileMap.h"
#include "SDL2Helpers.h"
#include "../version.h"

namespace OpenXcom
{
namespace CrossPlatform
{
	std::string errorDlg;

/**
 * Determines the available Linux error dialogs.
 */
void getErrorDialog()
{
#ifndef _WIN32
	if (system(NULL))
	{
		if (getenv("KDE_SESSION_UID") && system("which kdialog > /dev/null 2>&1") == 0)
			errorDlg = "kdialog --error ";
		else if (system("which zenity > /dev/null 2>&1") == 0)
			errorDlg = "zenity --no-wrap --error --text=";
		else if (system("which kdialog > /dev/null 2>&1") == 0)
			errorDlg = "kdialog --error ";
		else if (system("which gdialog > /dev/null 2>&1") == 0)
			errorDlg = "gdialog --msgbox ";
		else if (system("which xdialog > /dev/null 2>&1") == 0)
			errorDlg = "xdialog --msgbox ";
	}
#endif
}


#ifdef _WIN32
/**
 * Takes a Windows multibyte filesystem path and converts it to a UTF-8 string.
 * Also converts the path separator.
 * @param pathW Filesystem path.
 * @return UTF-8 string.
 */
static std::string pathFromWindows(const wchar_t *pathW) {
	int sizeW = lstrlenW(pathW);
	int sizeU8 = WideCharToMultiByte(CP_UTF8, 0, pathW, sizeW, NULL, 0, NULL, NULL);
	std::string pathU8(sizeU8, 0);
	WideCharToMultiByte(CP_UTF8, 0, pathW, sizeW, &pathU8[0], sizeU8, NULL, NULL);
	std::replace(pathU8.begin(), pathU8.end(), '\\', '/');
	return pathU8;
}

/**
 * Takes a UTF-8 string and converts it to a Windows
 * multibyte filesystem path.
 * Also converts the path separator.
 * @param path UTF-8 string.
 * @param reslash Convert forward slashes to back ones.
 * @return Filesystem path.
 */
static std::wstring pathToWindows(const std::string& path, bool reslash = true) {
	std::string src = path;
	if (reslash) {
		std::replace(src.begin(), src.end(), '/', '\\');
	}
	int sizeW = MultiByteToWideChar(CP_UTF8, 0, &src[0], (int)src.size(), NULL, 0);
	std::wstring pathW(sizeW, 0);
	MultiByteToWideChar(CP_UTF8, 0, &src[0], (int)src.size(), &pathW[0], sizeW);
	return pathW;
}
#endif

static std::vector<std::string> args;

/**
 * Converts command-line args to UTF-8 on windows
 */
void processArgs (int argc, char *argv[])
{
	args.clear();
#ifdef _WIN32
	auto cmdlineW = GetCommandLineW();
	int numArgs;
	auto argvW = CommandLineToArgvW(cmdlineW, &numArgs);
	for (int i=0; i< numArgs; ++i) { args.push_back(pathFromWindows(argvW[i])); }
#else
	for (int i=0; i< argc; ++i) { args.push_back(argv[i]); }
#endif
}

/// Returns the command-line arguments
const std::vector<std::string>& getArgs() { return args; }

/**
 * Displays a message box with an error message.
 * @param error Error message.
 */
void showError(const std::string &error)
{
#ifdef _WIN32
	auto titleW = pathToWindows("OpenXcom Error", false);
	auto errorW = pathToWindows(error, false);
	MessageBoxW(NULL, errorW.c_str(), titleW.c_str(), MB_ICONERROR | MB_OK);
#else
	if (errorDlg.empty())
	{
		std::cerr << error << std::endl;
	}
	else
	{
		std::string nError = '"' + error + '"';
		Unicode::replace(nError, "\n", "\\n");
		std::string cmd = errorDlg + nError;
		if (system(cmd.c_str()) != 0)
			std::cerr << error << std::endl;
	}
#endif
	Log(LOG_FATAL) << error;
}

#ifndef _WIN32
/**
 * Gets the user's home folder according to the system.
 * @return Absolute path to home folder.
 */
static char const *getHome()
{
	char const *home = getenv("HOME");
	if (!home)
	{
		struct passwd *const pwd = getpwuid(getuid());
		home = pwd->pw_dir;
	}
	return home;
}
#endif

/**
 * Builds a list of predefined paths for the Data folder
 * according to the running system.
 * @return List of data paths.
 */
std::vector<std::string> findDataFolders()
{
	std::vector<std::string> list;
#ifdef __MORPHOS__
	list.push_back("PROGDIR:");
	return list;
#endif

#ifdef _WIN32
	std::unordered_set<std::string> seen; // avoid dups in case cwd = dirname(exe)
	wchar_t pathW[MAX_PATH+1];
	const std::wstring oxconst = pathToWindows("OpenXcom/");
	// Get Documents folder
	if (SHGetSpecialFolderPathW(NULL, pathW, CSIDL_PERSONAL, FALSE))
	{
		PathAppendW(pathW, oxconst.c_str());
		auto path = pathFromWindows(pathW);
		Log(LOG_DEBUG) << "findDataFolders(): SHGetSpecialFolderPathW: " << path;
		if (seen.end() == seen.find(path)) { seen.insert(path); list.push_back(path); }
	}
#ifdef DATADIR
	snprintf(path, MAX_PATH, "%s\\", DATADIR);
	list.push_back(path);
#endif
	// Get binary directory
	if (GetModuleFileNameW(NULL, pathW, MAX_PATH) != 0)
	{
		PathRemoveFileSpecW(pathW);
		auto path = pathFromWindows(pathW);
		path.push_back('/');
		Log(LOG_DEBUG) << "findDataFolders(): GetModuleFileNameW/PathRemoveFileSpecW: " << path;
		if (seen.end() == seen.find(path)) { seen.insert(path); list.push_back(path); }
	}

	// Get working directory
	if (GetCurrentDirectoryW(MAX_PATH, pathW) != 0)
	{
		auto path = pathFromWindows(pathW);
		path.push_back('/');
		Log(LOG_DEBUG) << "findDataFolders(): GetCurrentDirectoryW: " << path;
		if (seen.end() == seen.find(path)) { seen.insert(path); list.push_back(path); }
	}
#else
	char const *home = getHome();
#ifdef __HAIKU__
	char data_path[B_PATH_NAME_LENGTH];
	find_directory(B_SYSTEM_SETTINGS_DIRECTORY, 0, true, data_path, sizeof(data_path)-strlen("/OpenXcom/"));
	strcat(data_path,"/OpenXcom/");
	list.push_back(data_path);
#endif
	char path[MAXPATHLEN];

	// Get user-specific data folders
	char const *const xdg_data_home = getenv("XDG_DATA_HOME");
	if (xdg_data_home && *xdg_data_home)
 	{
		snprintf(path, MAXPATHLEN, "%s/openxcom/", xdg_data_home);
 	}
 	else
 	{
#ifdef __APPLE__
		snprintf(path, MAXPATHLEN, "%s/Library/Application Support/OpenXcom/", home);
#else
		snprintf(path, MAXPATHLEN, "%s/.local/share/openxcom/", home);
#endif
 	}
 	list.push_back(path);
#ifdef DATADIR
	snprintp(path, MAXPATHLEN, "%s/" DATADIR);
	list.push_back(path);
#endif
	// Get global data folders
	char const *const xdg_data_dirs = getenv("XDG_DATA_DIRS");
	if (xdg_data_dirs && *xdg_data_dirs)
	{
		char xdg_data_dirs_copy[strlen(xdg_data_dirs)+1];
		strcpy(xdg_data_dirs_copy, xdg_data_dirs);
		char *dir = strtok(xdg_data_dirs_copy, ":");
		while (dir != 0)
		{
			snprintf(path, MAXPATHLEN, "%s/openxcom/", dir);
			list.push_back(path);
			dir = strtok(0, ":");
		}
	}
	else
	{
#ifdef __APPLE__
		list.push_back("/Users/Shared/OpenXcom/");
#else
		list.push_back("/usr/local/share/openxcom/");
		list.push_back("/usr/share/openxcom/");
#endif
	}
#ifdef INSTALLDIR
	snprintf(path, MAXPATHLEN, "%s", INSTALLDIR);
	list.push_back(path);
#endif

#ifdef __linux
	{
		char buffer[PATH_MAX];
		const ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
		// Get absolute executable path
		if (count != 0) {
			const std::string exe_path = std::string(buffer, count);
			// Get folder path
			const size_t dir_pos = exe_path.find_last_of("/");
			if (dir_pos != std::string::npos) {
				std::string dir = exe_path.substr(0, dir_pos);
				list.push_back( dir.append("/") );
				list.push_back( dir.append("/../share/openxcom/") ); // Relative FHS
			}
		}
	}
#endif
	// Get working directory
	list.push_back("./");
#endif

	return list;
}

/**
 * Builds a list of predefined paths for the User folder
 * according to the running system.
 * @return List of data paths.
 */
std::vector<std::string> findUserFolders()
{
	std::vector<std::string> list;

#ifdef __MORPHOS__
	list.push_back("PROGDIR:");
	return list;
#endif

#ifdef _WIN32
	std::unordered_set<std::string> seen;
	wchar_t pathW[MAX_PATH+1];
	const std::wstring oxconst = pathToWindows("OpenXcom/");
	const std::wstring usconst = pathToWindows("user/");

	// Get Documents folder
	if (SHGetSpecialFolderPathW(NULL, pathW, CSIDL_PERSONAL, FALSE))
	{
		PathAppendW(pathW, oxconst.c_str());
		auto path = pathFromWindows(pathW);
		Log(LOG_DEBUG) << "findUserFolders(): SHGetSpecialFolderPathW: " << path;
		if (seen.end() == seen.find(path)) { seen.insert(path); list.push_back(path); }
	}

	// Get binary directory
	if (GetModuleFileNameW(NULL, pathW, MAX_PATH) != 0)
	{
		PathRemoveFileSpecW(pathW);
		PathAppendW(pathW, usconst.c_str());
		auto path = pathFromWindows(pathW);
		Log(LOG_DEBUG) << "findUserFolders(): GetModuleFileNameW/PathRemoveFileSpecW: " << path;
		if (seen.end() == seen.find(path)) { seen.insert(path); list.push_back(path); }
	}

	// Get working directory
	if (GetCurrentDirectoryW(MAX_PATH, pathW) != 0)
	{
		PathAppendW(pathW, usconst.c_str());
		auto path = pathFromWindows(pathW);
		Log(LOG_DEBUG) << "findUserFolders(): GetCurrentDirectoryW: " << path;
		if (seen.end() == seen.find(path)) { seen.insert(path); list.push_back(path); }
	}
#else
#ifdef __HAIKU__
	char user_path[B_PATH_NAME_LENGTH];
	find_directory(B_USER_SETTINGS_DIRECTORY, 0, true, user_path, sizeof(user_path)-strlen("/OpenXcom/"));
	strcat(user_path,"/OpenXcom/");
	list.push_back(user_path);
#endif
	char const *home = getHome();
	char path[MAXPATHLEN];

	// Get user folders
	if (char const *const xdg_data_home = getenv("XDG_DATA_HOME"))
 	{
		snprintf(path, MAXPATHLEN, "%s/openxcom/", xdg_data_home);
 	}
 	else
 	{
#ifdef __APPLE__
		snprintf(path, MAXPATHLEN, "%s/Library/Application Support/OpenXcom/", home);
#else
		snprintf(path, MAXPATHLEN, "%s/.local/share/openxcom/", home);
#endif
 	}
	list.push_back(path);

	// Get old-style folder
	snprintf(path, MAXPATHLEN, "%s/.openxcom/", home);
	list.push_back(path);

	// Get working directory
	list.push_back("./user/");
#endif

	return list;
}

/**
 * Finds the Config folder according to the running system.
 * @return Config path.
 */
std::string findConfigFolder()
{
#ifdef __MORPHOS__
	return "PROGDIR:";
#endif

#if defined(_WIN32) || defined(__APPLE__)
	return "";
#elif defined (__HAIKU__)
	char settings_path[B_PATH_NAME_LENGTH];
	find_directory(B_USER_SETTINGS_DIRECTORY, 0, true, settings_path, sizeof(settings_path)-strlen("/OpenXcom/"));
	strcat(settings_path,"/OpenXcom/");
	return settings_path;
#else
	char const *home = getHome();
	char path[MAXPATHLEN];
	// Get config folders
	if (char const *const xdg_config_home = getenv("XDG_CONFIG_HOME"))
	{
		snprintf(path, MAXPATHLEN, "%s/openxcom/", xdg_config_home);
		return path;
	}
	else
	{
		snprintf(path, MAXPATHLEN, "%s/.config/openxcom/", home);
		return path;
	}
#endif
}

std::string searchDataFile(const std::string &filename)
{
	// Correct folder separator
	std::string name = filename;

	// Check current data path
	std::string path = Options::getDataFolder() + name;
	if (fileExists(path))
	{
		return path;
	}

	// Check every other path
	for (auto& dataPath : Options::getDataList())
	{
		path = dataPath + name;
		if (fileExists(path))
		{
			return path;
		}
	}

	// Give up
	return filename;
}

std::string searchDataFolder(const std::string &foldername, std::size_t size)
{
	// Correct folder separator
	std::string name = foldername;
	std::string path;

	// Set miminum possible of dirs
	std::size_t minNumOfElementsInFolder = (
		foldername == "TFTD" || foldername == "UFO" ? 9 : // At least 9 dictionaries with original data data
		foldername == "common" ? 6 : // Files: "Language/", "Palettes/", "Resources/", "Shaders/", "SoldierName/", "openxcom.png"
		foldername == "standard" ? 20 : // Now 48 mods, some buffer if some decide to drop some mods
		size
	);

	if (Options::getDataFolder() != "")
	{
		// Check current data path
		path = Options::getDataFolder() + name;
		if (folderMinSize(path, minNumOfElementsInFolder))
		{
			return path;
		}
	}

	// Check every other path
	for (auto& dataPath : Options::getDataList())
	{
		path = dataPath + name;
		if (folderMinSize(path, minNumOfElementsInFolder))
		{
			return path;
		}
	}

	// Give up
	return foldername;
}

/**
 * Creates a folder at the specified path.
 * @note Only creates the last folder on the path.
 * @param path Full path.
 * @return Folder created or not.
 */
bool createFolder(const std::string &path)
{
#ifdef _WIN32
	auto pathW = pathToWindows(path);
	int result = CreateDirectoryW(pathW.c_str(), 0);
	if (result == 0)
		return false;
	else
		return true;
#else
	mode_t process_mask = umask(0);
	int result = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	umask(process_mask);
	if (result == 0)
		return true;
	else
		return false;
#endif
}

/**
 * Adds an ending slash to a path if necessary.
 * @param path Folder path.
 * @return Terminated path.
 */
std::string convertPath(const std::string &path)
{
	if (!path.empty() && path.at(path.size()-1) != '/')
		return path + '/';
	return path;
}
#ifdef _WIN32
static time_t FILETIME2mtime(FILETIME& ft) {
	const long long int TICKS_PER_SECOND = 10000000;
	const long long int EPOCH_DIFFERENCE = 11644473600LL;
	long long int input = 0, temp = 0;

	input = ((long long int)ft.dwHighDateTime)<<32;
	input += ft.dwLowDateTime;
	temp = input / TICKS_PER_SECOND;
	temp = temp - EPOCH_DIFFERENCE;
	return (time_t) temp;
}
#endif
/**
 * Gets the name of all the files
 * contained in a certain folder.
 * @param path Full path to folder.
 * @param ext Extension of files ("" if it doesn't matter).
 * @return Ordered list of all the files in the form of tuple(filename, is_folder, mtime).
 */
std::vector<std::tuple<std::string, bool, time_t>> getFolderContents(const std::string &path, const std::string &ext)
{
	std::vector<std::tuple<std::string, bool, time_t>> files;
#ifdef _WIN32
	auto search_path = path + "/*";
	if (!ext.empty()) { search_path += "." + ext; }
	Log(LOG_VERBOSE) << "getFolderContents("<<path<<", "<<ext<<") -> " << search_path;
	auto pathW = pathToWindows(search_path);
	WIN32_FIND_DATAW ffd;
	auto handle = FindFirstFileW(pathW.c_str(), &ffd);
	if (handle == INVALID_HANDLE_VALUE) {
		Log(LOG_VERBOSE) << "getFolderContents("<<path<<", "<<ext<<"): fail outright.";
		return files;
	}
	do  {
		auto filename = pathFromWindows(ffd.cFileName);
		if ((filename == ".") || (filename == "..")) { continue; }
		time_t mtime = FILETIME2mtime(ffd.ftLastWriteTime);
		bool is_folder = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		files.push_back(std::make_tuple(filename, is_folder, mtime));
		Log(LOG_VERBOSE) << "getFolderContents("<<path<<", "<<ext<<"): got '"<<filename<<"'";
	} while (FindNextFileW(handle, &ffd) != 0);
	FindClose(handle);
	Log(LOG_VERBOSE) << "getFolderContents("<<path<<", "<<ext<<"): total "<<files.size();
#else
	DIR *dp = opendir(path.c_str());

	if (dp == 0)
	{
	#ifdef __MORPHOS__
		return files;
	#else
		std::string errorMessage("Failed to open directory: " + path);
		throw Exception(errorMessage);
	#endif
	}
	struct dirent *dirp;
	while ((dirp = readdir(dp)) != 0) {
		std::string filename = dirp->d_name;
		if (filename[0] == '.') //allowed by C++11 for empty string as it equal '\0'
		{
			//skip ".", "..", ".git", ".svn", ".bashrc", ".ssh" etc.
			continue;
		}
		if (!compareExt(filename, ext))	{ continue; }
		std::string fullpath = path + "/" + filename;
		bool is_directory = folderExists(fullpath);
		time_t mtime = getDateModified(fullpath);
		files.push_back(std::make_tuple(filename, is_directory, mtime));
	}
	closedir(dp);
#endif
	std::sort(files.begin(), files.end(),
		[](const std::tuple<std::string,bool,time_t>& a,
           const std::tuple<std::string,bool,time_t>& b) -> bool
       {
         return std::get<0>(a) > std::get<0>(b);
       });
	return files;
}

/**
 * Gets the contents of a folder and checks
 * if they meet a required minimum size.
 * @param path Full path to folder.
 * @param size Size of the folder (number of contents).
 * @return False if the folder doesn't exist or doesn't meet the size.
 */
bool folderMinSize(const std::string &path, std::size_t size)
{
	if (!folderExists(path))
	{
		return false;
	}
	if (size == 0)
	{
		return true;
	}

	return (getFolderContents(path).size() >= size);
}

/**
 * Checks if a certain path exists and is a folder.
 * @param path Full path to folder.
 * @return Does it exist?
 */
bool folderExists(const std::string &path)
{
#ifdef _WIN32
	auto pathW = pathToWindows(path);
	bool rv = (PathIsDirectoryW(pathW.c_str()) != FALSE);
	Log(LOG_VERBOSE) << "folderExists("<<path<<")? " << (rv ? "yeah" : "nope");
	return rv;
#elif __MORPHOS__
	BPTR l = Lock( path.c_str(), SHARED_LOCK );
	if ( l != NULL )
	{
		UnLock( l );
		return 1;
	}
	return 0;
#else
	struct stat info;
	return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode));
#endif
}

/**
 * Checks if a certain path exists and is a file.
 * @param path Full path to file.
 * @return Does it exist?
 */
bool fileExists(const std::string &path)
{
#ifdef _WIN32
	auto pathW = pathToWindows(path);
	bool rv = (PathFileExistsW(pathW.c_str()) != FALSE);
	Log(LOG_VERBOSE) << "fileExists("<<path<<")? " << (rv?"yeah":"nope");
	return rv;
#elif __MORPHOS__
	BPTR l = Lock( path.c_str(), SHARED_LOCK );
	if ( l != NULL )
	{
		UnLock( l );
		return 1;
	}
	return 0;
#else
	struct stat info;
	return (stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode));
#endif
}

/**
 * Removes a file from the specified path.
 * @param path Full path to file.
 * @return True if the operation succeeded, False otherwise.
 */
bool deleteFile(const std::string &path)
{
#ifdef _WIN32
	auto pathW = pathToWindows(path);
	return (DeleteFileW(pathW.c_str()) != 0);
#else
	return (remove(path.c_str()) == 0);
#endif
}

/**
 * Returns only the filename from a specified path.
 * @param path Full path.
 * @return Filename component.
 */
std::string baseFilename(const std::string &path)
{
	size_t sep = path.find_last_of('/');
	std::string filename;
	if (sep == std::string::npos)
	{
		filename = path;
	}
	else if (sep == path.size() - 1)
	{
		return baseFilename(path.substr(0, path.size() - 1));
	}
	else
	{
		filename = path.substr(sep + 1);
	}
	return filename;
}

/**
 * Returns the directory from a specified path.
 * @param path Full path.
 * @return Directory component.
 */
std::string dirFilename(const std::string &path)
{
	size_t sep = path.find_last_of('/');
	std::string filename;
	if (sep == std::string::npos)
	{
		filename = "";
	}
	else if (sep == path.size() - 1)
	{
		return dirFilename(path.substr(0, path.size() - 1));
	}
	else
	{
		filename = path.substr(0, sep + 1);
	}
	return filename;
}

/**
 * Replaces invalid filesystem characters with _.
 * @param filename Original filename.
 * @return Filename without invalid characters.
 */
std::string sanitizeFilename(const std::string &filename)
{
	std::string newFilename = filename;
	for (std::string::iterator i = newFilename.begin(); i != newFilename.end(); ++i)
	{
		if ((*i) == '<' ||
			(*i) == '>' ||
			(*i) == ':' ||
			(*i) == '*' ||
			(*i) == '|' ||
			(*i) == '"' ||
			(*i) == '\'' ||
			(*i) == '/' ||
			(*i) == '?'||
			(*i) == '\0'||
			(*i) == '\\')
		{
			*i = '_';
		}
	}
	return newFilename;
}

/**
 * Removes the extension from a filename. Only the
 * last dot is considered.
 * @param filename Original filename.
 * @return Filename without the extension.
 */
std::string noExt(const std::string &filename)
{
	size_t dot = filename.find_last_of('.');
	if (dot == std::string::npos)
	{
		return filename;
	}
	return filename.substr(0, dot);
}

/**
 * Returns the extension from a filename. Only the
 * last dot is considered.
 * @param filename Original filename.
 * @return Extension component, includes dot.
 */
std::string getExt(const std::string &filename)
{
	size_t dot = filename.find_last_of('.');
	if (dot == std::string::npos)
	{
		return "";
	}
	return filename.substr(dot);
}

/**
 * Compares the extension in a filename (case-insensitive).
 * @param filename Filename to compare.
 * @param extension Extension to compare to.
 * @return If the extensions match.
 */
bool compareExt(const std::string &filename, const std::string &extension)
{
	if (extension.empty())
		return true;
	int j = filename.length() - extension.length();
	if (j <= 0)
		return false;
	if (filename[j - 1] != '.')
		return false;
	for (size_t i = 0; i < extension.length(); ++i)
	{
		if (::tolower(filename[j + i]) != ::tolower(extension[i]))
			return false;
	}
	return true;
}

/**
 * Gets the current locale of the system in language-COUNTRY format.
 * @return Locale string.
 */
std::string getLocale()
{
#ifdef _WIN32
	char language[9], country[9];

	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, language, 9);
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, country, 9);

	std::ostringstream locale;
	locale << language << "-" << country;
	return locale.str();
#else
	std::locale l;
	try
	{
		l = std::locale("");
	}
	catch (const std::runtime_error &)
	{
		return "x-";
	}
	std::string name = l.name();
	size_t dash = name.find_first_of('_'), dot = name.find_first_of('.');
	if (dot != std::string::npos)
	{
		name = name.substr(0, dot - 1);
	}
	if (dash != std::string::npos)
	{
		std::string language = name.substr(0, dash - 1);
		std::string country = name.substr(dash - 1);
		std::ostringstream locale;
		locale << language << "-" << country;
		return locale.str();
	}
	else
	{
		return name + "-";
	}
#endif
}

/**
 * Checks if the system's default quit shortcut was pressed.
 * @param ev SDL event.
 * @return Is quitting necessary?
 */
bool isQuitShortcut(const SDL_Event &ev)
{
#ifdef _WIN32
	// Alt + F4
	return (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F4 && ev.key.keysym.mod & KMOD_ALT);
#elif __APPLE__
	// Command + Q
	return (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_q && ev.key.keysym.mod & KMOD_LMETA);
#else
	//TODO add other OSs shortcuts.
    (void)ev;
	return false;
#endif
}
/**
 * Gets the last modified date of a file.
 * @param path Full path to file.
 * @return The timestamp in integral format.
 */
time_t getDateModified(const std::string &path)
{
#ifdef _WIN32
	time_t rv = 0;
	auto pathW = pathToWindows(path);
	auto fh = CreateFileW(pathW.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (fh == INVALID_HANDLE_VALUE) {
		return 0;
	}
	FILETIME ftCreate, ftAccess, ftWrite;
	if (GetFileTime(fh, &ftCreate, &ftAccess, &ftWrite)) {
		rv = FILETIME2mtime(ftWrite);
	}
	CloseHandle(fh);
	return rv;
#else
	struct stat info;
	if (stat(path.c_str(), &info) == 0)
	{
		return info.st_mtime;
	}
	else
	{
		return 0;
	}
#endif
}

/**
 * Converts a date/time into a human-readable string
 * using the ISO 8601 standard.
 * @param time Value in timestamp format.
 * @return String pair with date and time.
 */
std::pair<std::string, std::string> timeToString(time_t time)
{
	char localDate[25], localTime[25];

/*#ifdef _WIN32
	LARGE_INTEGER li;
	li.QuadPart = time;
	FILETIME ft;
	ft.dwHighDateTime = li.HighPart;
	ft.dwLowDateTime = li.LowPart;
	SYSTEMTIME st;
	FileTimeToLocalFileTime(&ft, &ft);
	FileTimeToSystemTime(&ft, &st);

	GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, localDate, 25);
	GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, localTime, 25);
#endif*/

	struct tm *timeinfo = localtime(&(time));
	strftime(localDate, 25, "%Y-%m-%d", timeinfo);
	strftime(localTime, 25, "%H:%M", timeinfo);

	return std::make_pair(localDate, localTime);
}

/**
 * Moves a file from one path to another,
 * replacing any existing file.
 * @param src Source path.
 * @param dest Destination path.
 * @return True if the operation succeeded, False otherwise.
 */
bool moveFile(const std::string &src, const std::string &dest)
{
#ifdef _WIN32
	auto srcW = pathToWindows(src);
	auto dstW = pathToWindows(dest);
	return (MoveFileExW(srcW.c_str(), dstW.c_str(), MOVEFILE_REPLACE_EXISTING) != 0);
#else
	// TODO In fact all remaining uses of this are renaming files inside a single directory
	// so we may as well uncomment the rename() and drop the rest.
	//return (rename(src.c_str(), dest.c_str()) == 0);
	std::ifstream srcStream;
	std::ofstream destStream;
	srcStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	destStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	try
	{
		srcStream.open(src.c_str(), std::ios::binary);
		destStream.open(dest.c_str(), std::ios::binary);
		destStream << srcStream.rdbuf();
		srcStream.close();
		destStream.close();
	}
	catch (const std::fstream::failure &)
	{
		return false;
	}
	return deleteFile(src);
#endif
}

/**
 * Copies a file from one path to another,
 * replacing any existing file.
 * @param src Source path.
 * @param dest Destination path.
 * @return True if the operation succeeded, False otherwise.
 */
bool copyFile(const std::string& src, const std::string& dest)
{
#ifdef _WIN32
	auto srcW = pathToWindows(src);
	auto dstW = pathToWindows(dest);
	return (CopyFileW(srcW.c_str(), dstW.c_str(), false) != 0);
#endif
	return false;
}

/**
 * Writes a file.
 * @param filename - where to writeFile
 * @param data - what to writeFile
 * @return if we did write it.
 */
bool writeFile(const std::string& filename, const std::string& data) {
	// Even SDL1 file IO accepts UTF-8 file names on windows.
	SDL_RWops *rwops = SDL_RWFromFile(filename.c_str(), "w");
	if (!rwops) {
		Log(LOG_ERROR) << "Failed to write " << filename << ": " << SDL_GetError();
		return false;
	}
	if (1 != SDL_RWwrite(rwops, data.c_str(), data.size(), 1)) {
		Log(LOG_ERROR) << "Failed to write " << filename << ": " << SDL_GetError();
		SDL_RWclose(rwops);
		return false;
	}
	SDL_RWclose(rwops);
	return true;
}

/**
 * Writes a file.
 * @param filename - where to writeFile
 * @param data - what to writeFile
 * @return if we did write it.
 */
bool writeFile(const std::string& filename, const std::vector<unsigned char>& data) {
	// Even SDL1 file IO accepts UTF-8 file names on windows.
	SDL_RWops *rwops = SDL_RWFromFile(filename.c_str(), "wb");
	if (!rwops) {
		Log(LOG_ERROR) << "Failed to write " << filename << ": " << SDL_GetError();
		return false;
	}
	if (1 != SDL_RWwrite(rwops, data.data(), data.size(), 1)) {
		Log(LOG_ERROR) << "Failed to write " << filename << ": " << SDL_GetError();
		SDL_RWclose(rwops);
		return false;
	}
	SDL_RWclose(rwops);
	return true;
}

/**
 * Fully reads a file and returns a stream
 * @param filename - what to readFile
 * @return the istream
 */
std::unique_ptr<std::istream> readFile(const std::string& filename)
{
	return std::unique_ptr<std::istream>(new StreamData(readFileRaw(filename)));
}

/**
 * Fully reads a file and returns a pointer to the data
 * @param filename - what to readFile
 * @param pSize - returned data size
 * @return pointer to file data
 */
RawData readFileRaw(const std::string& filename)
{
	SDL_RWops* rwops = SDL_RWFromFile(filename.c_str(), "r");
	if (!rwops)
	{
		std::string err = "Failed to read " + filename + ": " + SDL_GetError();
		Log(LOG_ERROR) << err;
		throw Exception(err);
	}
	size_t s;
	char* data = (char*)SDL_LoadFile_RW(rwops, &s, SDL_TRUE);
	if (data == NULL)
	{
		std::string err = "Failed to read " + filename + ": " + SDL_GetError();
		Log(LOG_ERROR) << err;
		throw Exception(err);
	}
	return RawData(data, s, SDL_free);
}

/**
 * Gets an istream to a file's bytes at least up to and including first "\n---" sequence.
 * To be used only for savegames.
 * @param filename - what to read
 * @return the istream
 */
std::unique_ptr<std::istream> getYamlSaveHeader(const std::string& filename)
{
	return std::unique_ptr<std::istream>(new StreamData(getYamlSaveHeaderRaw(filename)));
}

/**
 * Reads a file up to and including first "\n---" sequence.
 * To be used only for savegames.
 * @param filename - what to read
 * @param pSize - returned data size
 * @return pointer to file data
 */
RawData getYamlSaveHeaderRaw(const std::string& filename)
{
	SDL_RWops* rwops = SDL_RWFromFile(filename.c_str(), "r");
	if (!rwops)
	{
		std::string err = "Failed to read " + filename + ": " + SDL_GetError();
		Log(LOG_ERROR) << err;
		throw Exception(err);
	}
	const size_t chunksize = 4096;
	size_t size = 0;
	size_t offs = 0;
	char* data = (char*)SDL_malloc(chunksize + 1);
	if (data == NULL)
	{
		std::string err(SDL_GetError());
		Log(LOG_ERROR) << err;
		throw Exception(err);
	}
	while (true)
	{
		auto actually_read = SDL_RWread(rwops, data + offs, 1, chunksize);
		if (actually_read == 0 || actually_read == -1)
		{
			break;
		}
		size += actually_read;
		data[size] = 0;
		size_t search_from = offs > 4 ? offs - 4 : 0;
		if (NULL != strstr(data + search_from, "\n---"))
		{
			break;
		}
		char* newdata = (char*)SDL_realloc(data, size + chunksize + 1);
		if (newdata == NULL)
		{
			std::string err(SDL_GetError());
			Log(LOG_ERROR) << err;
			throw Exception(err);
		}
		data = newdata;
		offs = size;
	}
	SDL_RWclose(rwops);
	return RawData(data, size, SDL_free);
}

/**
 * Notifies the user that maybe he should have a look.
 */
void flashWindow()
{
#ifdef _WIN32
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWMInfo(&wminfo))
	{
		HWND hwnd = wminfo.window;
		FlashWindow(hwnd, true);
	}
#endif
}

/**
 * Gets the executable path in DOS-style (short) form.
 * For non-Windows systems, just use a dummy path.
 * @return Executable path.
 */
std::string getDosPath()
{
#ifdef _WIN32
	std::string path, bufstr;
	char buf[MAX_PATH];
	if (GetModuleFileNameA(0, buf, MAX_PATH) != 0)
	{
		bufstr = buf;
		size_t c1 = bufstr.find_first_of('\\');
		path += bufstr.substr(0, c1+1);
		size_t c2 = bufstr.find_first_of('\\', c1+1);
		while (c2 != std::string::npos)
		{
			std::string dirname = bufstr.substr(c1+1, c2-c1-1);
			if (dirname == "..")
			{
				path = path.substr(0, path.find_last_of('\\', path.length()-2));
			}
			else
			{
				if (dirname.length() > 8)
					dirname = dirname.substr(0, 6) + "~1";
				std::transform(dirname.begin(), dirname.end(), dirname.begin(), toupper);
				path += dirname;
			}
			c1 = c2;
			c2 = bufstr.find_first_of('\\', c1+1);
			if (c2 != std::string::npos)
				path += '\\';
		}
	}
	else
	{
		path = "C:\\GAMES\\OPENXCOM";
	}
	return path;
#else
	return "C:\\GAMES\\OPENXCOM";
#endif
}

/**
 * Sets the window titlebar icon.
 * For Windows, use the embedded resource icon.
 * For other systems, use a PNG icon.
 * @param winResource ID for Windows icon.
 * @param unixPath Path to PNG icon for Unix.
 */
#ifdef _WIN32
void setWindowIcon(int winResource, const std::string &)
{
	HINSTANCE handle = GetModuleHandle(NULL);
	HICON icon = LoadIcon(handle, MAKEINTRESOURCE(winResource));

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWMInfo(&wminfo))
	{
		HWND hwnd = wminfo.window;
		SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)icon);
	}
}
#else
void setWindowIcon(int, const std::string &unixPath)
{
	SDL_Surface *icon = IMG_Load_RW(FileMap::getRWops(unixPath), SDL_TRUE);
	if (icon != 0)
	{
		SDL_WM_SetIcon(icon, NULL);
		SDL_FreeSurface(icon);
	}
}
#endif

/**
 * Logs the stack back trace leading up to this function call.
 * @param ctx Pointer to stack context (PCONTEXT on Windows), NULL to use current context.
 */
void stackTrace(void *ctx)
{
#ifdef _WIN32
# ifndef __NO_DBGHELP
	const int MAX_SYMBOL_LENGTH = 1024;
	CONTEXT context;
	if (ctx != 0)
	{
		context = *((PCONTEXT)ctx);
	}
	else
	{
#  ifdef _M_IX86
		memset(&context, 0, sizeof(CONTEXT));
		context.ContextFlags = CONTEXT_CONTROL;
#   ifdef __MINGW32__
		asm("Label:\n\t"
			"movl %%ebp,%0;\n\t"
			"movl %%esp,%1;\n\t"
			"movl $Label,%%eax;\n\t"
			"movl %%eax,%2;\n\t"
			: "=r" (context.Ebp), "=r" (context.Esp), "=r" (context.Eip)
			: //no input
			: "eax");
#   else
		_asm {
		Label:
			mov[context.Ebp], ebp;
			mov[context.Esp], esp;
			mov eax, [Label];
			mov[context.Eip], eax;
		}
#   endif
#  else /* no  _M_IX86 */
		RtlCaptureContext(&context);
#  endif
	}
	HANDLE thread = GetCurrentThread();
	HANDLE process = GetCurrentProcess();
	STACKFRAME64 frame;
	memset(&frame, 0, sizeof(STACKFRAME64));
	DWORD image;
#  ifdef _M_IX86
	image = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset = context.Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Esp;
	frame.AddrStack.Mode = AddrModeFlat;
#  elif _M_X64
	image = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset = context.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
#  elif _M_IA64
	image = IMAGE_FILE_MACHINE_IA64;
	frame.AddrPC.Offset = context.StIIP;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.IntSp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrBStore.Offset = context.RsBSP;
	frame.AddrBStore.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.IntSp;
	frame.AddrStack.Mode = AddrModeFlat;
#  else
	Log(LOG_FATAL) << "Unfortunately, no stack trace information is available";
	return;
#  endif
	SYMBOL_INFO *symbol = (SYMBOL_INFO *)malloc(sizeof(SYMBOL_INFO) + (MAX_SYMBOL_LENGTH - 1) * sizeof(TCHAR));
	symbol->MaxNameLen = MAX_SYMBOL_LENGTH;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	IMAGEHLP_LINE64 *line = (IMAGEHLP_LINE64 *)malloc(sizeof(IMAGEHLP_LINE64));
	line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	DWORD displacement;
	SymInitialize(process, NULL, TRUE);
	while (StackWalk64(image, process, thread, &frame, &context, NULL, NULL, NULL, NULL))
	{
		if (SymFromAddr(process, frame.AddrPC.Offset, NULL, symbol))
		{
			std::string symname = symbol->Name;
#  ifdef __MINGW32__
			symname = "_" + symname;
			int status = 0;
			size_t outSz = 0;
			char* demangled = abi::__cxa_demangle(symname.c_str(), 0, &outSz, &status);
			if (status == 0)
			{
				symname = demangled;
				if (outSz > 0)
					free(demangled);
			}
			else
			{
				symname = symbol->Name;
			}
#  endif
			if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement, line))
			{
				std::string filename = line->FileName;
				size_t n = filename.find_last_of('\\');
				if (n != std::string::npos)
				{
					filename = filename.substr(n + 1);
				}
				Log(LOG_FATAL) << "0x" << std::hex << symbol->Address << std::dec << " " << symname << " (" << filename << ":" << line->LineNumber << ")";
			}
			else
			{
				Log(LOG_FATAL) << "0x" << std::hex << symbol->Address << std::dec << " " << symname;
			}
		}
		else
		{
			Log(LOG_FATAL) << "??";
		}
	}
	DWORD err = GetLastError();
	if (err)
	{
		Log(LOG_FATAL) << "Unfortunately, no stack trace information is available";
	}
	SymCleanup(process);
# else /* __NO_DBGHELP */
	Log(LOG_FATAL) << "Unfortunately, no stack trace information is available";
# endif
#elif __CYGWIN__
	Log(LOG_FATAL) << "Unfortunately, no stack trace information is available";
#else    /* not _WIN32 or __CYGWIN__ */
	void *frames[32];
	char buf[1024];
	int  frame_count = backtrace(frames, 32);
	char *demangled = NULL;
	const char *mangled = NULL;
	int status;
	size_t sym_offset;

	for (int i = 0; i < frame_count; i++) {
		Dl_info dl_info;
		if (dladdr(frames[i], &dl_info )) {
			demangled = NULL;
			mangled = dl_info.dli_sname;
			if ( mangled != NULL) {
				sym_offset = (char *)frames[i] - (char *)dl_info.dli_saddr;
				demangled = abi::__cxa_demangle( dl_info.dli_sname, NULL, 0, &status);
				snprintf(buf, sizeof(buf), "%s(%s+0x%zx) [%p]",
						dl_info.dli_fname,
						status == 0 ? demangled : mangled,
						sym_offset, frames[i] );
			} else { // symbol not found
				sym_offset = (char *)frames[i] - (char *)dl_info.dli_fbase;
				snprintf(buf, sizeof(buf), "%s(+0x%zx) [%p]", dl_info.dli_fname, sym_offset, frames[i]);
			}
			free(demangled);
			Log(LOG_FATAL) << buf;
		} else { // object not found
			snprintf(buf, sizeof(buf), "? ? [%p]", frames[i]);
			Log(LOG_FATAL) << buf;
		}
	}
#endif
	ctx = (void*)ctx;
}

/**
 * Generates a timestamp of the current time.
 * @return String in D-M-Y_H-M-S format.
 */
std::string now()
{
	const int MAX_LEN = 25, MAX_RESULT = 80;
	char result[MAX_RESULT] = { 0 };
#ifdef _WIN32
	char date[MAX_LEN], time[MAX_LEN];
	if (GetDateFormatA(LOCALE_INVARIANT, 0, 0, "dd'-'MM'-'yyyy", date, MAX_LEN) == 0)
		return "00-00-0000";
	if (GetTimeFormatA(LOCALE_INVARIANT, TIME_FORCE24HOURFORMAT, 0, "HH'-'mm'-'ss", time, MAX_LEN) == 0)
		return "00-00-00";
	sprintf(result, "%s_%s", date, time);
#else
	char buffer[MAX_LEN];
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, MAX_LEN, "%d-%m-%Y_%H-%M-%S", timeinfo);
	sprintf(result, "%s", buffer);
#endif
	return result;
}

/**
 * Logs the details of this crash and shows an error.
 * @param ex Pointer to exception data (PEXCEPTION_POINTERS on Windows, signal int on Unix)
 * @param err Exception message, if any.
 */
void crashDump(void *ex, const std::string &err)
{
	std::ostringstream error;
#ifdef _MSC_VER
	PEXCEPTION_POINTERS exception = (PEXCEPTION_POINTERS)ex;
	std::exception *cppException = 0;
	switch (exception->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_CODE_CXX:
		cppException = (std::exception *)exception->ExceptionRecord->ExceptionInformation[1];
		error << cppException->what();
		break;
	case EXCEPTION_ACCESS_VIOLATION:
		error << "Memory access violation.";
		break;
	default:
		error << "code 0x" << std::hex << exception->ExceptionRecord->ExceptionCode;
		break;
	}
	Log(LOG_FATAL) << "A fatal error has occurred: " << error.str();
	if (ex)
	{
		stackTrace(exception->ContextRecord);
	}
	std::string dumpName = Options::getUserFolder();
	dumpName += now() + ".dmp";
	HANDLE dumpFile = CreateFileA(dumpName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	MINIDUMP_EXCEPTION_INFORMATION exceptionInformation;
	exceptionInformation.ThreadId = GetCurrentThreadId();
	exceptionInformation.ExceptionPointers = exception;
	exceptionInformation.ClientPointers = FALSE;
	if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpNormal, exception ? &exceptionInformation : NULL, NULL, NULL))
	{
		Log(LOG_FATAL) << "Crash dump generated at " << dumpName;
	}
	else
	{
		Log(LOG_FATAL) << "No crash dump generated: " << GetLastError();
	}
#else
	if (ex == 0)
	{
		error << err;
	}
	else
	{
		int signal = *((int*)ex);
		switch (signal)
		{
		case SIGSEGV:
			error << "Segmentation fault.";
			break;
		default:
			error << "signal " << signal;
			break;
		}
	}
	Log(LOG_FATAL) << "A fatal error has occurred: " << error.str();
	stackTrace(0);
#endif
	std::ostringstream msg;
	msg << "OpenXcom has crashed: " << error.str() << std::endl;
	msg << "Log file: " << getLogFileName() << std::endl;
	msg << "If this error was unexpected, please report it on the OpenXcom forum (OXCE board)." << std::endl;
	msg << "The following can help us solve the problem:" << std::endl;
	msg << "1. a saved game from just before the crash (helps 98%)" << std::endl;
	msg << "2. a detailed description how to reproduce the crash (helps 80%)" << std::endl;
	msg << "3. a log file (helps 10%)" << std::endl;
	msg << "4. a screenshot of this error message (helps 5%)";
	showError(msg.str());
}

/**
 * Opens a file or web path in the system default browser.
 */
bool openExplorer(const std::string &url)
{
#ifdef _WIN32
	HINSTANCE ret = ShellExecuteW(NULL, L"open", Unicode::convMbToWc(url, CP_UTF8).c_str(), NULL, NULL, SW_SHOWNORMAL);
	// The return value is not a true HINSTANCE. If the function succeeds, it returns a value greater than 32.
	return (static_cast<int>(reinterpret_cast<uintptr_t>(ret)) > 32);
#elif __MOBILE__
	return false;
#elif __APPLE__
	std::string cmd = "open \"" + url + "\"";
	return (system(cmd.c_str()) == 0);
#else
	std::string cmd = "xdg-open \"" + url + "\"";
	return (system(cmd.c_str()) == 0);
#endif
}


/**
 * Appends a file, logs nothing to avoid recursion.
 * @param filename - where to writeFile
 * @param data - what to writeFile
 * @return if we did write it.
 */
static bool logToFile(const std::string& filename, const std::string& data) {
	// Even SDL1 file IO accepts UTF-8 file names on windows.
	SDL_RWops *rwops = SDL_RWFromFile(filename.c_str(), "a+");
	if (rwops) {
		auto rv = SDL_RWwrite(rwops, data.c_str(), data.size(), 1);
		SDL_RWclose(rwops);
		return rv == 1;
	}
	return false;
}

static const size_t LOG_BUFFER_LIMIT = 1<<10;
static std::list<std::pair<int, std::string>> logBuffer;
static std::string logFileName;
const std::string& getLogFileName() { return logFileName; }

/**
 * Setting the log file name and setting the effective reportingLevel
 * to not LOG_UNCENSORED turns off buffering of the log messages,
 * and turns on writing them to the actual log (and flushes the buffer).
 */
void setLogFileName(const std::string& name) {
	deleteFile(name);
	size_t sz = logBuffer.size();
	Log(LOG_DEBUG) << "setLogFileName("<<name<<") was '"<<logFileName<<"'; "<<sz<<" in buffer";
	logFileName = name;
}
void log(int level, const std::ostringstream& baremsgstream) {
	std::ostringstream msgstream;
	msgstream << "[" << CrossPlatform::now() << "]" << "\t"
			  << "[" << Logger::toString(level) << "]" << "\t"
			  << baremsgstream.str() << std::endl;
	auto msg = msgstream.str();

	int effectiveLevel = Logger::reportingLevel();
	if (effectiveLevel >= LOG_DEBUG) {
		fwrite(msg.c_str(), msg.size(), 1, stderr);
		fflush(stderr);
	}
	if (logBuffer.size() > LOG_BUFFER_LIMIT) { // drop earliest message so as to not eat all memory
		logBuffer.pop_front();
	}
	if (logFileName.empty() || effectiveLevel == LOG_UNCENSORED) { // no log file; accumulate.
		logBuffer.push_back(std::make_pair(level, msg));
		return;
	}
	// attempt to flush the buffer
	bool failed = false;
	while (!logBuffer.empty()) {
		if (effectiveLevel >= logBuffer.front().first) {
			if (!logToFile(logFileName, logBuffer.front().second)) {
				std::string err = "Failed to append to '" + logFileName + "': " + SDL_GetError();
				logBuffer.push_back(std::make_pair(LOG_ERROR, err));
				failed = true;
				break;
			}
		}
		logBuffer.pop_front();
	}
	// retain the current message if write fails.
	if (failed || !logToFile(logFileName, msg)) {
		logBuffer.push_back(std::make_pair(level, msg));
	}
}

#if defined(EMBED_ASSETS)
# if defined(_WIN32)
# include "../resource.h"
static void *CommonZipAssetPtr = 0;
static size_t CommonZipAssetSize = 0;
static void *StandardZipAssetPtr = 0;
static size_t StandardZipAssetSize = 0;
static void *getWindowsResource(int res_id, size_t *size) {
    HMODULE handle = GetModuleHandle(NULL);
    HRSRC rc = FindResource(handle, MAKEINTRESOURCE(res_id), MAKEINTRESOURCE(10));
	if (!rc) { return NULL; }
    HGLOBAL rcData = LoadResource(handle, rc);
	if (!rcData) { return NULL; }
    *size = SizeofResource(handle, rc);
    return LockResource(rcData);
}
# elif defined(__MOBILE__)
/* This space is intentionally left blank */
# else
extern "C" {
	extern uint8_t common_zip[];
	extern int common_zip_size;
	extern uint8_t standard_zip[];
	extern int standard_zip_size;
}
# endif
#endif
SDL_RWops *getEmbeddedAsset(const std::string& assetName) {
	std::string log_ctx = "getEmbeddedAsset('" + assetName + "'): ";
	if (assetName.size() == 0 || assetName[0] == '/') {
		Log(LOG_WARNING) << log_ctx << "ignoring bogus asset name";
		return NULL;
	}
#if defined(EMBED_ASSETS)
	SDL_RWops *rv = NULL;
# if defined(_WIN32)
	if (assetName == "common.zip") {
		if (!CommonZipAssetPtr) {
			CommonZipAssetPtr = getWindowsResource(IDZ_COMMON_ZIP, &CommonZipAssetSize);
		}
		if (CommonZipAssetPtr) {
			rv = SDL_RWFromConstMem(CommonZipAssetPtr, CommonZipAssetSize);
		}
	} else if (assetName == "standard.zip") {
		if (!StandardZipAssetPtr) {
			StandardZipAssetPtr = getWindowsResource(IDZ_STANDARD_ZIP, &StandardZipAssetSize);
		}
		if (StandardZipAssetPtr) {
			rv = SDL_RWFromConstMem(StandardZipAssetPtr, StandardZipAssetSize);
		}
	}
# elif defined(__MOBILE__)
	rv = SDL_RWFromFile(assetName, "rb");
# else
	if (assetName == "common.zip") {
		rv = SDL_RWFromConstMem(common_zip, common_zip_size);
	} else if (assetName == "standard.zip") {
		rv = SDL_RWFromConstMem(standard_zip, standard_zip_size);
	}
# endif
	if (rv == NULL) {
		Log(LOG_ERROR) << log_ctx << "embedded asset not found: "<< SDL_GetError();
	}
	return rv;
#else
	/* Asset embedding disabled. */
	Log(LOG_VERBOSE) << log_ctx << "assets were not embedded.";
	return NULL;
#endif
}

/**
 * Tests the internet connection.
 * @param url URL to test.
 * @return True if the operation succeeded, False otherwise.
 */
bool testInternetConnection(const std::string& url)
{
#ifdef _WIN32
	auto urlW = pathToWindows(url, false);
	bool bConnect = InternetCheckConnectionW(urlW.c_str(), FLAG_ICC_FORCE_CONNECTION, 0);
	return bConnect;
#else
	return false;
#endif
}

/**
 * Downloads a file from a given URL to the filesystem.
 * @param url Source URL.
 * @param filename Destination file name.
 * @return True if the operation succeeded, False otherwise.
 */
bool downloadFile(const std::string& url, const std::string& filename)
{
#ifdef _WIN32
	auto urlW = pathToWindows(url, false);
	auto filenameW = pathToWindows(filename, true);
	DeleteUrlCacheEntryW(urlW.c_str());
	HRESULT hr = URLDownloadToFileW(NULL, urlW.c_str(), filenameW.c_str(), 0, NULL);
	return SUCCEEDED(hr);
#else
	return false;
#endif
}

/**
 * Parse string with version number.
 */
std::array<int, 4> parseVersion(const std::string& newVersion)
{
	std::array<int, 4> newOxceVersion = {};

	std::string each;
	char split_char = '.';
	std::istringstream ss(newVersion);
	std::size_t j = 0;
	while (std::getline(ss, each, split_char)) {
		if (j == newOxceVersion.size())
		{
			break;
		}

		try {
			int i = std::stoi(each);
			newOxceVersion[j] = i;
		}
		catch (...) {

		}
		++j;
	}
	return newOxceVersion;
}

/**
 * Is the given version number higher than the current version number?
 * @param newVersion Version to compare.
 * @return True if given version is higher than current version.
 */
bool isHigherThanCurrentVersion(const std::string& newVersion)
{
	return isHigherThanCurrentVersion(parseVersion(newVersion), { OPENXCOM_VERSION_NUMBER });
}

/**
 * Is the given version number higher than the given version number?
 * @param newVersion Version to compare.
 * @param ver Given available version.
 * @return True if given version is higher than given version.
 */
bool isHigherThanCurrentVersion(const std::array<int, 4>& newOxceVersion, const int (&ver)[4])
{
	bool isHigher = false;

	for (size_t k = 0; k < std::size(ver); ++k)
	{
		if (newOxceVersion[k] > ver[k])
		{
			isHigher = true;
			break;
		}
		else if (newOxceVersion[k] < ver[k])
		{
			break;
		}
	}

	return isHigher;
}

/**
 * Is the given version number lower than the minimum required version number?
 * @param dataVersion Version to compare.
 * @return True if the given version number is lower than the minimum required version number.
 */
bool isLowerThanRequiredVersion(const std::string& dataVersion)
{
	return isLowerThanRequiredVersion(parseVersion(dataVersion), { MIN_REQUIRED_RULESET_VERSION_NUMBER });
}

/**
 * Is the first version number lower than the second version number?
 * @param dataVersion Version to compare.
 * @param ver Minimum required version.
 * @return True if the first version number is lower than the second version number.
 */
bool isLowerThanRequiredVersion(const std::array<int, 4>& dataVersion, const int(&ver)[4])
{
	bool isLower = false;

	for (size_t k = 0; k < std::size(ver); ++k)
	{
		if (dataVersion[k] < ver[k])
		{
			isLower = true;
			break;
		}
		else if (dataVersion[k] > ver[k])
		{
			break;
		}
	}

	return isLower;
}

/**
 * Gets the path to the executable file.
 * @return Path to the EXE file.
 */
std::string getExeFolder()
{
#ifdef _WIN32
	wchar_t dest[MAX_PATH + 1];
	if (GetModuleFileNameW(NULL, dest, MAX_PATH) != 0)
	{
		PathRemoveFileSpecW(dest);
		auto ret = pathFromWindows(dest) + "/";
		return ret;
	}
#endif
	return std::string();
}

/**
 * Gets the file name of the executable file.
 * @param includingPath Including full path or just the file name?
 * @return Name of the EXE file.
 */
std::string getExeFilename(bool includingPath)
{
#ifdef _WIN32
	wchar_t dest[MAX_PATH + 1];
	if (GetModuleFileNameW(NULL, dest, MAX_PATH) != 0)
	{
		if (includingPath)
		{
			auto ret = pathFromWindows(dest);
			return ret;
		}
		else
		{
			auto filename = PathFindFileNameW(dest);
			auto ret = pathFromWindows(filename);
			return ret;
		}
	}
#endif
	return std::string();
}

/**
 * Starts the update process.
 */
void startUpdateProcess()
{
#ifdef _WIN32
	auto operationW = pathToWindows("open", false);
	auto fileW = pathToWindows("oxce-upd.bat", false);
	ShellExecuteW(NULL, operationW.c_str(), fileW.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
}



#ifdef OXCE_AUTO_TEST

static auto dummy = ([]
{
	auto create = [](int i, int j, int k, int l)
	{
		return std::array<int, 4>{{i, j, k, l}};
	};

	assert(parseVersion("0.0.0.0") == create(0, 0, 0, 0));
	assert(parseVersion("1.0.0.0") == create(1, 0, 0, 0));
	assert(parseVersion("1.2.0.0") == create(1, 2, 0, 0));
	assert(parseVersion("1.2.3.4") == create(1, 2, 3, 4));
	assert(parseVersion("1.2.3.4.5") == create(1, 2, 3, 4));
	assert(parseVersion("1.2.3") == create(1, 2, 3, 0));
	assert(parseVersion("1.2") == create(1, 2, 0, 0));
	assert(parseVersion("1.A.2") == create(1, 0, 2, 0));
	assert(parseVersion(".2") == create(0, 2, 0, 0));


	assert(isHigherThanCurrentVersion(create(1, 2, 0, 0), {1, 1, 0, 0}));
	assert(isHigherThanCurrentVersion(create(1, 2, 1, 3), {1, 2, 0, 4}));
	assert(isHigherThanCurrentVersion(create(1, 2, 1, 3), {1, 2, 1, 2}));
	assert(!isHigherThanCurrentVersion(create(1, 2, 1, 3), {1, 2, 1, 3}));
	assert(!isHigherThanCurrentVersion(create(1, 2, 1, 3), {1, 2, 1, 4}));
	assert(!isHigherThanCurrentVersion(create(1, 2, 1, 3), {1, 2, 2, 2}));
	assert(!isHigherThanCurrentVersion(create(1, 2, 1, 3), {1, 3, 1, 2}));

	return 0;
})();

static auto dummyPaths = ([]
{
	assert(CrossPlatform::baseFilename("aaa/bbb/ccc") == "ccc");
	assert(CrossPlatform::baseFilename("aaa/bbb/ccc/") == "ccc");
	assert(CrossPlatform::baseFilename("aaa/bbb/ccc//") == "ccc");
	assert(CrossPlatform::baseFilename("/ccc") == "ccc");
	assert(CrossPlatform::baseFilename("ccc") == "ccc");

	assert(CrossPlatform::dirFilename("aaa/bbb/ccc") == "aaa/bbb/");
	assert(CrossPlatform::dirFilename("aaa/bbb/ccc/") == "aaa/bbb/");
	assert(CrossPlatform::dirFilename("aaa/bbb/ccc//") == "aaa/bbb/");
	assert(CrossPlatform::dirFilename("/ccc") == "/");
	assert(CrossPlatform::dirFilename("ccc") == "");
	return 0;
})();

static auto dummyRawFile = ([]
{
	{
		char text[] = "test";
		StreamData raw(RawData{text, std::strlen(text), +[](void*){}});

		assert(raw.get() == 't');
		assert(raw.get() == 'e');
		assert(raw.get() == 's');
		assert(raw.get() == 't');
		assert(raw.get() == std::char_traits<char>::eof());
	}

	{
		char text[] = "test123";
		StreamData raw(RawData{text, std::strlen(text), +[](void*){}});

		char dummy1[10] = { };
		assert(raw.read(dummy1, 4) && std::strcmp(dummy1, "test") == 0);

		char dummy2[10] = { };
		assert(raw.read(dummy2, 3) && std::strcmp(dummy2, "123") == 0);

	}

	{
		char text[] = "test123";
		StreamData raw(RawData{text, std::strlen(text), +[](void*){}});

		char dummy1[10] = { };
		assert(!raw.read(dummy1, 10) && std::strcmp(dummy1, "test123") == 0);
	}

	{
		char text[] = "test123";
		StreamData raw(RawData{text, std::strlen(text), +[](void*){}});

		raw.seekg(0, std::ios::end);
		std::streamoff end = raw.tellg();
		raw.seekg(0, std::ios::beg);
		std::streamoff begin = raw.tellg();

		assert(end-begin == (int)std::strlen(text));
	}

	{
		char text[] = "test123\0ErrorErrorErrorErrorErrorError";
		StreamData raw(RawData{text, std::strlen(text), +[](void*){}});

		char dummy1[11] = { };

		raw.clear();
		raw.seekg(std::strlen(text), std::ios::beg);
		assert(!!raw);
		assert(!raw.read(dummy1, 10));

		raw.clear();
		raw.seekg(20, std::ios::beg);
		assert(!raw);
		assert(!raw.read(dummy1, 10));

		raw.clear();
		raw.seekg(-5, std::ios::beg);
		assert(!raw);
		assert(!raw.read(dummy1, 10));

		raw.clear();
		raw.seekg(0, std::ios::beg);
		assert(!!raw);
		assert(raw.read(dummy1, 5));
	}

	{
		static int calledDelete = 0;
		char text[] = "test123";
		StreamData raw(RawData{text, std::strlen(text), +[](void*){ ++calledDelete; }});

		assert(raw.get() == 't');

		raw.extractRawData();

		assert(raw.get() == std::char_traits<char>::eof());
		assert(calledDelete == 1);
	}

	{
		static int calledDelete = 0;
		char text[] = "0123";
		StreamData raw(RawData{text, std::strlen(text), +[](void*){ ++calledDelete; }});


		assert(raw.get() == '0');

		StreamData raw2 = std::move(raw);

		assert(raw.get() ==  std::char_traits<char>::eof());
		assert(!!raw2);
		assert(raw2.get() == '1');

		raw = std::move(raw2);

		assert(raw.get() == '2');
		assert(raw2.get() == std::char_traits<char>::eof());

		assert(raw.get() == '3');
		assert(!!raw);
		assert(raw.get() == std::char_traits<char>::eof());
		assert(!raw);

		{
			StreamData raw3 = std::move(raw);
			assert(!raw3);

			assert(calledDelete == 0);
		}

		assert(calledDelete == 1);
	}

	return 0;
})();
#endif



}
}
