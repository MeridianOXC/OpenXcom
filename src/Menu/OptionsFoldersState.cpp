/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include "OptionsFoldersState.h"
#include "../Engine/FileMap.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Folders Options screen.
 * @param origin Game section that originated this state.
 */
OptionsFoldersState::OptionsFoldersState(OptionsOrigin origin) : OptionsBaseState(origin)
{
	setCategory(_btnFolders);

	// Create object
	_txtDataFolder = new Text(218, 9, 94, 8);
	_txtUserFolder = new Text(218, 9, 94, 59);
	_txtSaveFolder = new Text(218, 9, 94, 86);
	_txtConfigFolder = new Text(218, 9, 94, 121);

	_txtDataFolderPath = new Text(218, 41, 94, 18);
	_txtUserFolderPath = new Text(218, 17, 94, 69);
	_txtSaveFolderPath = new Text(218, 25, 94, 96);
	_txtConfigFolderPath = new Text(218, 17, 94, 131);

	add(_txtDataFolder, "text1", "foldersMenu");
	add(_txtUserFolder, "text1", "foldersMenu");
	add(_txtSaveFolder, "text1", "foldersMenu");
	add(_txtConfigFolder, "text1", "foldersMenu");

	add(_txtDataFolderPath, "text2", "foldersMenu");
	add(_txtUserFolderPath, "text2", "foldersMenu");
	add(_txtSaveFolderPath, "text2", "foldersMenu");
	add(_txtConfigFolderPath, "text2", "foldersMenu");

	centerAllSurfaces();

	// Set up object
	_txtDataFolder->setText(tr("STR_DATA_FOLDER"));
	_txtUserFolder->setText(tr("STR_USER_FOLDER"));
	_txtSaveFolder->setText(tr("STR_SAVE_FOLDER"));
	_txtConfigFolder->setText(tr("STR_CONFIG_FOLDER"));

	std::string file1 = "GEODATA/BACKPALS.DAT";
	std::string file2 = "openxcom.png";

	std::string origDataFolder = "<error1>";
	if (FileMap::fileExists(file1))
	{
		origDataFolder = FileMap::at(file1)->fullpath;
		origDataFolder = origDataFolder.substr(0, origDataFolder.find(file1));
	}
	std::string oxceDataFolder = "<error2>";
	if (FileMap::fileExists(file2))
	{
		oxceDataFolder = FileMap::at(file2)->fullpath;
		oxceDataFolder = oxceDataFolder.substr(0, oxceDataFolder.find(file2));
	}
	std::string dataFolder = origDataFolder + "\n\n" + oxceDataFolder;

	_txtDataFolderPath->setText(dataFolder);
	_txtDataFolderPath->setWordWrap(true);
	_txtDataFolderPath->setTooltip("STR_DATA_FOLDER_DESC");
	_txtDataFolderPath->onMouseIn((ActionHandler)&OptionsFoldersState::txtTooltipIn);
	_txtDataFolderPath->onMouseOut((ActionHandler)&OptionsFoldersState::txtTooltipOut);

	_txtUserFolderPath->setText(Options::getUserFolder());
	_txtUserFolderPath->setWordWrap(true);
	_txtUserFolderPath->setTooltip("STR_USER_FOLDER_DESC");
	_txtUserFolderPath->onMouseIn((ActionHandler)&OptionsFoldersState::txtTooltipIn);
	_txtUserFolderPath->onMouseOut((ActionHandler)&OptionsFoldersState::txtTooltipOut);

	_txtSaveFolderPath->setText(Options::getMasterUserFolder());
	_txtSaveFolderPath->setWordWrap(true);
	_txtSaveFolderPath->setTooltip("STR_SAVE_FOLDER_DESC");
	_txtSaveFolderPath->onMouseIn((ActionHandler)&OptionsFoldersState::txtTooltipIn);
	_txtSaveFolderPath->onMouseOut((ActionHandler)&OptionsFoldersState::txtTooltipOut);

	_txtConfigFolderPath->setText(Options::getConfigFolder());
	_txtConfigFolderPath->setWordWrap(true);
	_txtConfigFolderPath->setTooltip("STR_CONFIG_FOLDER_DESC");
	_txtConfigFolderPath->onMouseIn((ActionHandler)&OptionsFoldersState::txtTooltipIn);
	_txtConfigFolderPath->onMouseOut((ActionHandler)&OptionsFoldersState::txtTooltipOut);
}

/**
 *
 */
OptionsFoldersState::~OptionsFoldersState()
{

}

}
