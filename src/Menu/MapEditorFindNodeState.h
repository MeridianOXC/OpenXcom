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
#include "../Engine/State.h"
#include "../Battlescape/MapEditor.h"

#include <vector>
#include <string>
#include <map>

namespace OpenXcom
{

class MapEditorState;
class Window;
class SavedBattleGame;
class Text;
class TextButton;
class BattlescapeButton;
class ComboBox;
class InteractiveSurface;

enum NodeChangeType;

class MapEditorFindNodeState : public State
{
private :
    MapEditorState *_mapEditorState;
	Window *_window;
    SavedBattleGame *_save;
	Text *_txtFind, *_txtWithNodeParameter, *_txtValueEqualToFind, *_txtActionFind;
	TextButton *_btnFind;
    ComboBox *_cbxNodeParameterFind, *_cbxNodeValueFind, *_cbxCurrentSelection, *_cbxHandleSelection;
    Text *_txtReplace, *_txtValueEqualToReplace;
    TextButton *_btnReplace, *_btnCancel;
    ComboBox *_cbxNodeParameterReplace, *_cbxNodeValueReplace;
    std::map<ComboBox*, size_t> _selectedParameters;
    std::vector<NodeChangeType> _nodeParameters;

public :
    /// Creates the Map Editor Info window
    MapEditorFindNodeState(MapEditorState *mapEditorState);
    /// Creates the Map Editor Info window
    ~MapEditorFindNodeState();
    /// Handles updating the node values dropdowns according to parameters chosen
    void cbxNodeParameterChange(Action *action);
    /// Handles clicking the find or replace buttons
    void btnFindClick(Action *action);
    /// Returns to the previous menu
    void btnCancelClick(Action *action);
    /// Selects nodes according to the parameters chosen
    void selectNodes(); 
    /// Replaces node data according to the parameters chosen
    void replaceNodes();
};

}