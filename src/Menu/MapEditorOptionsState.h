#pragma once
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
#include "../Engine/OptionInfo.h"
#include "../Engine/State.h"
#include "OptionsBaseState.h"

namespace OpenXcom
{

class TextButton;
class TextList;
class Window;
class Text;

class MapEditorOptionsState : public State
{
private :
	OptionsOrigin _origin;
	TextButton *_btnInfo, *_btnLoad, *_btnSave, *_btnAbandon, *_btnOptions, *_btnCancel;
    TextList *_lstOptions;
	std::vector<OptionInfo> _settings;
	//bool _isTFTD;
	Window *_window;
	Text *_txtTitle, *_txtTooltip;

public :
    /// Creates the options menu window for the map editor
    MapEditorOptionsState(OptionsOrigin origin);
    /// Cleans up the map editor options menu
    ~MapEditorOptionsState();
	/// Fills settings list.
	void init() override;
    /// Shows the info for the map being edited
    void btnInfoClick(Action *action);
    /// Opens the menu for loading a different map in the editor
    void btnLoadClick(Action *action);
    /// Saves the current map
    void btnSaveClick(Action *action);
    /// Opens the options menu
    void btnOptionsClick(Action *action);
    /// Opens the abandon game window
    void btnAbandonClick(Action *action);
    /// Returns to the map editor
    void btnCancelClick(Action *action);
	/// Handler for clicking a setting on the list.
	void lstOptionsClick(Action *action);
	/// Handler for moving the mouse over a setting.
	void lstOptionsMouseOver(Action *action);
	/// Handler for moving the mouse outside the settings.
	void lstOptionsMouseOut(Action *action);

};

}