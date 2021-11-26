#pragma once
/*
 * Copyright 2010-2021 OpenXcom Developers.
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
#include <vector>
#include <string>
#include <tuple>
#include <map>

namespace OpenXcom
{

struct FileData
{
    std::string name;
    bool isFolder;
    time_t timestamp;
    bool selected;

    FileData(std::tuple<std::string, bool, time_t> data)
    {
        name = std::get<0>(data);
        isFolder = std::get<1>(data);
        timestamp = std::get<2>(data);
        selected = false;
    }
};

class Window;
class Text;
class ArrowButton;
class TextButton;
class TextEdit;
class TextList;
class Frame;

class FileBrowserState : public State
{
private:
    State *_parent;
    bool _saveMode;

    Window *_window;
    Text *_txtTitle, *_txtDirectory, *_txtSearch;
    Text *_txtFilename, *_txtFiletype, *_txtFiledate;
    ArrowButton *_sortName, *_sortType, *_sortDate;
    TextButton *_btnCut, *_btnCopy, *_btnPaste, *_btnDelete, *_btnClose;
    std::vector<TextButton*> _rightClickMenu;
    TextButton *_btnOk, *_btnCancel;
    TextEdit *_edtQuickSearch;
    TextList *_lstBrowser;
    Frame *_frameBrowser;

    std::string _currentDirectory;
    std::vector<FileData> _fileData;
    std::map<size_t, size_t> _fileDataIndexMap;
    bool _foldersFirst, _sortByName, _reverseSort;
    int _clickedRow, _firstClickTime;
    bool _mouseOverRightClickMenu;
    std::string _directoryToCopyFrom;
    std::vector<std::string> _filesToCopy;
    bool _moveFiles;

public:
    /// Creates the File Browser window
    FileBrowserState(State *parent, bool saveMode, std::string fileName = "");
    /// Cleans up the File Browser window
    ~FileBrowserState();
    /// Initializes the data in the File Browser window
    void init() override;
	/// Handles keypresses.
	void handle(Action *action) override;
    /// Populates the list of files and folders in the browser
    void populateBrowserList(std::string directory, bool forceRefresh = false);
    /// Highlights selected files in the browser window
    void highlightSelectedFiles();
    /// Handles clicking the sort arrows
    void sortArrowClick(Action *action);
    /// Handles focusing the quick search filter
    void edtQuickSearchFocus(Action *action);
    /// Handles applying the quick search filter
    void edtQuickSearchApply(Action *action);
    /// Handles clicking on the list of the browser window
    void lstBrowserClick(Action *action);
    /// Handles right-clicking to bring up a menu of actions
    void rightClickMenuOpen(Action *action);
    /// Handles clicking the select button
    void btnSelectClick(Action *action);
    /// Handles clicking the cut button
    void btnCutClick(Action *action);
    /// Handles clicking the copy button
    void btnCopyClick(Action *action);
    /// Marks files for cutting or copying
    void markSelectedFiles();
    /// Handles clicking the paste button
    void btnPasteClick(Action *action);
    /// Handles clicking the delete button
    void btnDeleteClick(Action *action);
    /// Handles clicking the close button
    void btnCloseClick(Action *action);
    /// Handles clicking the OK button
    void btnOkClick(Action *action);
    /// Returns to the Main Menu
    void btnCancelClick(Action *action);

};

}