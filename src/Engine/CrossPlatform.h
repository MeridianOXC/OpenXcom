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
#include <array>
#include <memory>
#include <utility>

namespace OpenXcom
{

using RawDataDeleteFun = void(*)(void*);

/**
 * Unique pointer with size to raw data buffer.
 */
class RawData
{
	std::unique_ptr<void, RawDataDeleteFun> _data;
	std::size_t _size;


public:

	/// Default constructor.
	RawData() : _data{ nullptr, +[](void*){} }, _size{ }
	{

	}

	/// Create data from pointer and size.
	RawData(void* data, std::size_t size, RawDataDeleteFun del) : _data{ data, del }, _size{ size }
	{

	}

	/// Move constructor.
	RawData(RawData&& d) : RawData()
	{
		*this = std::move(d);
	}

	/// Move assignment.
	RawData& operator=(RawData&& d)
	{
		_data = std::exchange(d._data, std::unique_ptr<void, RawDataDeleteFun>{ nullptr, +[](void*){} });
		_size = std::exchange(d._size, 0u);

		return *this;
	}


	/// Size of buffer.
	std::size_t size() const { return _size; }

	/// Data of buffer.
	const void* data() const { return _data.get(); }

	/// Data of buffer.
	void* data() { return _data.get(); }
};

/**
 * Stream to raw data buffer, owning its data buffer.
 */
class StreamData : public std::istream, private std::streambuf
{
	RawData _data;


public:

	/// Default constructor.
	StreamData() : std::istream(nullptr)
	{
		this->rdbuf(this);
	}

	/// Constructor from raw data.
	StreamData(RawData data) : StreamData()
	{
		insertRawData(std::move(data));
	}

	/// Move constructor, move whole state to new object.
	StreamData(StreamData&& f) : StreamData()
	{
		*this = std::move(f);
	}

	/// Move assignment, move whole state to new object.
	StreamData& operator=(StreamData&& f)
	{
		auto state = f.rdstate();
		auto pos = f.tellg();
		insertRawData(f.extractRawData());
		this->seekg(pos);
		this->clear(state);

		return *this;
	}


	/// Move out raw data from object.
	RawData extractRawData()
	{
		this->setg(nullptr, nullptr, nullptr);
		return std::move(_data);
	}

	/// Insert raw data into object.
	void insertRawData(RawData data)
	{
		_data = std::move(data);
		this->setg((char*)_data.data(), (char*)_data.data(), (char*)_data.data() + _data.size());
		this->clear();
	}


protected:

	/// https://stackoverflow.com/questions/35066207/how-to-implement-custom-stdstreambufs-seekoff
	virtual  std::streambuf::pos_type seekoff(
		std::streambuf::off_type off, std::ios_base::seekdir dir,
		std::ios_base::openmode) override
	{
		auto pos = gptr();
		if (dir == std::ios_base::cur)
			pos += off;
		else if (dir == std::ios_base::end)
			pos = egptr() + off;
		else if (dir == std::ios_base::beg)
			pos = eback() + off;

		// check for invalid moves
		if (pos < eback())
			return std::streambuf::pos_type(-1);
		else if (pos > egptr())
			return std::streambuf::pos_type(-1);

		setg(eback(), pos, egptr());
		return gptr() - eback();
	}

	/// https://stackoverflow.com/a/46069245/1938348
	virtual std::streambuf::pos_type seekpos(
		std::streambuf::pos_type pos,
		std::ios_base::openmode which) override
	{
		return seekoff(pos - std::streambuf::pos_type(std::streambuf::off_type(0)), std::ios_base::beg, which);
	}
};

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
	std::string searchDataFolder(const std::string &foldername, std::size_t size = 0);
	/// Creates a folder.
	bool createFolder(const std::string &path);
	/// Terminates a path.
	std::string convertPath(const std::string &path);
	/// Returns the list of files in a folder as a vector of tuples (filename, id_dir, mtime)
	std::vector<std::tuple<std::string, bool, time_t>> getFolderContents(const std::string &path, const std::string &ext = "");
	/// Checks if the path has a minimum size (number of contents, not bytes).
	bool folderMinSize(const std::string &path, std::size_t size);
	/// Checks if the path is an existing folder.
	bool folderExists(const std::string &path);
	/// Checks if the path is an existing file.
	bool fileExists(const std::string &path);
	/// Deletes the specified file.
	bool deleteFile(const std::string &path);
	/// Gets the pathless filename of a file.
	std::string baseFilename(const std::string &path);
	/// Gets the pathless dir of a file.
	std::string dirFilename(const std::string &path);
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
	/// Reads in a file
	RawData readFileRaw(const std::string& filename);
	/// Reads file until "\n---" sequence is met or to the end. To be used only for savegames.
	std::unique_ptr<std::istream> getYamlSaveHeader (const std::string& filename);
	/// Reads file until "\n---" sequence is met or to the end. To be used only for savegames.
	RawData getYamlSaveHeaderRaw(const std::string& filename);
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
	/// Opens a URL.
	bool openExplorer(const std::string &url);
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
	/// Parse string with version number.
	std::array<int, 4> parseVersion(const std::string& newVersion);
	/// Is the given version number higher than the current version number?
	bool isHigherThanCurrentVersion(const std::string& newVersion);
	/// Is the given version number higher than the given version number?
	bool isHigherThanCurrentVersion(const std::array<int, 4>& newVersion, const int (&ver)[4]);
	/// Is the given version number lower than the minimum required version number?
	bool isLowerThanRequiredVersion(const std::string& dataVersion);
	/// Is the first version number lower than the second version number?
	bool isLowerThanRequiredVersion(const std::array<int, 4>& dataVersion, const int(&ver)[4]);
	/// Gets the path to the executable file.
	std::string getExeFolder();
	/// Gets the file name of the executable file.
	std::string getExeFilename(bool includingPath);
	/// Starts the update process.
	void startUpdateProcess();
}

}
