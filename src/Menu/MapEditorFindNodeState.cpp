/*
 * Copyright 2010-2020 OpenXcom Developers.
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
#include <sstream>
#include "MapEditorFindNodeState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Battlescape/Map.h"
//#include "../Battlescape/MapEditor.h" // already included in header
#include "../Battlescape/MapEditorState.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the find and replace tiles window for the map editor
 * @param game Pointer to the core game.
 */
MapEditorFindNodeState::MapEditorFindNodeState(MapEditorState *mapEditorState) : _mapEditorState(mapEditorState)
{
    _screen = false;

    int windowX = 32;
    int windowY = 0;
    _window = new Window(this, 256, 200, 32, 0, POPUP_BOTH);

    _save = _game->getSavedGame()->getSavedBattle();

    _txtFind = new Text(256, 17, windowX, windowY + 9);

    _txtWithNodeParameter = new Text(118, 10, windowX + 9, windowY + 26);
    _txtValueEqualToFind = new Text(118, 10, windowX + 128 + 3, windowY + 26);
    _txtActionFind = new Text(118, 10, windowX + 128 + 3, windowY + 72);

    _cbxNodeParameterFind = new ComboBox(this, 120, 16, windowX + 7, windowY + 36, false);
    _cbxNodeValueFind = new ComboBox(this, 120, 16, windowX + 128 + 1, windowY + 36, false);
    _cbxCurrentSelection = new ComboBox(this, 120, 16, windowX + 128 + 1, windowY + 54, false);
    _cbxHandleSelection = new ComboBox(this, 120, 16, windowX + 128 + 1, windowY + 82, false);

    _btnFind = new TextButton(84, 16, windowX + 165, windowY + 100);

    _txtReplace = new Text(256, 17, windowX, windowY + 132);

    _txtValueEqualToReplace = new Text(118, 10, windowX + 9, windowY + 167);

    _cbxNodeParameterReplace = new ComboBox(this, 120, 16, windowX + 7, windowY + 149, true);
    _cbxNodeValueReplace = new ComboBox(this, 120, 16, windowX + 7, windowY + 177, true);

    _btnReplace = new TextButton(84, 16, windowX + 165, windowY + 159);

    _btnCancel = new TextButton(84, 16, windowX + 165, windowY + 177);

	// Set palette
	setInterface("optionsMenu", false, _save);

    add(_window, "window", "optionsMenu");
    add(_txtFind, "text", "optionsMenu");
    add(_txtWithNodeParameter, "text", "optionsMenu");
    add(_txtValueEqualToFind, "text", "optionsMenu");
    add(_txtActionFind, "text", "optionsMenu");
    add(_btnFind, "button", "optionsMenu");
    add(_txtReplace, "text", "optionsMenu");
    add(_txtValueEqualToReplace, "text", "optionsMenu");
    add(_btnReplace, "button", "optionsMenu");
    // add combo boxes in reverse order so they properly layer over each other when open
    add(_cbxNodeParameterReplace, "infoBoxOKButton", "battlescape");
    add(_cbxNodeValueReplace, "infoBoxOKButton", "battlescape");
	add(_cbxHandleSelection, "infoBoxOKButton", "battlescape");
	add(_cbxCurrentSelection, "infoBoxOKButton", "battlescape");
	add(_cbxNodeValueFind, "infoBoxOKButton", "battlescape");
    add(_cbxNodeParameterFind, "infoBoxOKButton", "battlescape");
    add(_btnCancel, "button", "optionsMenu");

    centerAllSurfaces();

    // Set up objects
	applyBattlescapeTheme("mainMenu");
	setWindowBackground(_window, "mainMenu");

	_txtFind->setAlign(ALIGN_CENTER);
	_txtFind->setBig();
	_txtFind->setText(tr("STR_FIND_NODES"));

    _txtWithNodeParameter->setText(tr("STR_FIND_NODES_PARAMETER"));
    _txtValueEqualToFind->setText(tr("STR_FIND_NODES_WITH_VALUE"));
    _txtActionFind->setText(tr("STR_FIND_NODES_THEN"));

    _btnFind->setText(tr("STR_FIND_NODES_BUTTON"));
	_btnFind->onMouseClick((ActionHandler)&MapEditorFindNodeState::btnFindClick);
	_btnFind->onKeyboardPress((ActionHandler)&MapEditorFindNodeState::btnFindClick, Options::keyOk);

	_txtReplace->setAlign(ALIGN_CENTER);
	_txtReplace->setBig();
	_txtReplace->setText(tr("STR_FIND_NODES_AND_REPLACE"));

    _txtValueEqualToReplace->setText(tr("STR_REPLACE_NODES_VALUE"));

    _btnReplace->setText(tr("STR_REPLACE_NODES_BUTTON"));
	_btnReplace->onMouseClick((ActionHandler)&MapEditorFindNodeState::btnFindClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorFindNodeState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorFindNodeState::btnCancelClick, Options::keyCancel);

    std::vector<std::string> lstOptions;
    lstOptions.clear();
    lstOptions.push_back(tr("STR_FIND_NODE_TYPE"));
    lstOptions.push_back(tr("STR_FIND_NODE_RANK"));
    lstOptions.push_back(tr("STR_FIND_NODE_PATROL"));
    lstOptions.push_back(tr("STR_FIND_NODE_SPAWN"));
    lstOptions.push_back(tr("STR_FIND_NODE_TARGET"));
    lstOptions.push_back(tr("STR_FIND_NODE_LINKS"));
    _cbxNodeParameterFind->setOptions(lstOptions, false);
    _cbxNodeParameterFind->setSelected(0);
    _cbxNodeParameterFind->onChange((ActionHandler)&MapEditorFindNodeState::cbxNodeParameterChange);

    lstOptions.erase(lstOptions.end() - 1);
    lstOptions.push_back(tr("STR_REPLACE_NODE_LINKS"));
    _cbxNodeParameterReplace->setOptions(lstOptions, false);
    _cbxNodeParameterReplace->setSelected(0);
    _cbxNodeParameterReplace->onChange((ActionHandler)&MapEditorFindNodeState::cbxNodeParameterChange);

    // create a list of possible changes to match the parameters of nodes, to make it easier to use a switch-case structure later
    _nodeParameters.clear();
    _nodeParameters.push_back(NCT_TYPE);
    _nodeParameters.push_back(NCT_RANK);
    _nodeParameters.push_back(NCT_FLAG);
    _nodeParameters.push_back(NCT_PRIORITY);
    _nodeParameters.push_back(NCT_RESERVED);
    _nodeParameters.push_back(NCT_LINKS);

    lstOptions.clear();
    lstOptions.push_back(tr("STR_FIND_NODES_ANYWHERE"));
    Uint8 colorDisabled = 8; // TODO move to elements ruleset
    if (_game->getMapEditor()->getSelectedNodes()->empty())
    {
        _cbxCurrentSelection->setColor(colorDisabled);
        _cbxCurrentSelection->setHighContrast(false);
    }
    else
    {
        lstOptions.push_back(tr("STR_FIND_NODES_IN_SELECTION"));
        lstOptions.push_back(tr("STR_FIND_NODES_OUTSIDE_SELECTION"));
    }
    _cbxCurrentSelection->setOptions(lstOptions, false);
    _cbxCurrentSelection->setSelected(0);

    lstOptions.clear();
    lstOptions.push_back(tr("STR_CREATE_NEW_SELECTION"));
    if (_game->getMapEditor()->getSelectedNodes()->empty())
    {
        _cbxHandleSelection->setColor(colorDisabled);
        _cbxHandleSelection->setHighContrast(false);
    }
    else
    {
        lstOptions.push_back(tr("STR_ADD_TO_SELECTION"));
        lstOptions.push_back(tr("STR_REMOVE_FROM_SELECTION"));
    }
    _cbxHandleSelection->setOptions(lstOptions, false);
    _cbxHandleSelection->setSelected(0);

    // Initialize the node value combo boxes using the method we designed for that
    _selectedParameters[_cbxNodeParameterFind] = -1;
    _selectedParameters[_cbxNodeParameterReplace] = -1;
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action action = Action(&ev, 0.0, 0.0, 0, 0);
    action.setSender(_cbxNodeParameterFind);
    cbxNodeParameterChange(&action);
    action.setSender(_cbxNodeParameterReplace);
    cbxNodeParameterChange(&action);
}

/**
 * Cleans up the state
 */
MapEditorFindNodeState::~MapEditorFindNodeState()
{

}

/**
 * Handles updating the node values dropdowns according to parameters chosen
 * @param action Pointer to an action.
 */
void MapEditorFindNodeState::cbxNodeParameterChange(Action *action)
{
    // Figure out which of the two parameter boxes was changed
    // And set which of the two value parameter boxes we're determining the options for
    ComboBox *sender;
    ComboBox *cbxToChange;
    if (action->getSender() == _cbxNodeParameterFind)
    {
        sender = _cbxNodeParameterFind;
        cbxToChange = _cbxNodeValueFind;
    }
    else
    {
        sender = _cbxNodeParameterReplace;
        cbxToChange = _cbxNodeValueReplace;
    }

    // If we're keeping the same parameter as before, we don't need to reset the values
    if (_selectedParameters[sender] == sender->getSelected())
    {
        return;
    }
    else
    {
        _selectedParameters[sender] = sender->getSelected();
    }

    // Figure out what values we need to populate the value combo boxes
    NodeChangeType parameter = _nodeParameters.at(sender->getSelected());
    std::vector<std::string> listOptions;
    listOptions.clear();
    switch(parameter)
    {
        case NCT_TYPE:
            {
                listOptions.push_back(tr("STR_NODE_TYPE_ANY"));
                listOptions.push_back(tr("STR_NODE_TYPE_SMALL"));
                listOptions.push_back(tr("STR_NODE_TYPE_FLYING"));
                listOptions.push_back(tr("STR_NODE_TYPE_FLYINGSMALL"));
            }

            break;

        case NCT_RANK:
            {
                listOptions.push_back(tr("STR_NODE_RANK_CIVSCOUT"));
                listOptions.push_back(tr("STR_NODE_RANK_XCOM"));
                listOptions.push_back(tr("STR_NODE_RANK_SOLDIER"));
                listOptions.push_back(tr("STR_NODE_RANK_NAVIGATOR"));
                listOptions.push_back(tr("STR_NODE_RANK_LEADERCOMMANDER"));
                listOptions.push_back(tr("STR_NODE_RANK_ENGINEER"));
                listOptions.push_back(tr("STR_NODE_RANK_TERRORIST0"));
                listOptions.push_back(tr("STR_NODE_RANK_MEDIC"));
                listOptions.push_back(tr("STR_NODE_RANK_TERRORIST1"));
            }

            break;

        case NCT_FLAG:
        case NCT_PRIORITY:
        case NCT_RESERVED:
            {
                for (int i = 0; i < 11; ++i)
                {
                    listOptions.push_back(std::to_string(i));
                }
            }

            break;

        case NCT_LINKS:
            {
                listOptions.push_back(tr("STR_LINK_UNUSED"));
                listOptions.push_back(tr("STR_LINK_NORTH"));
                listOptions.push_back(tr("STR_LINK_EAST"));
                listOptions.push_back(tr("STR_LINK_SOUTH"));
                listOptions.push_back(tr("STR_LINK_WEST"));
                for (auto node : *_save->getNodes())
                {
                    listOptions.push_back(std::to_string(node->getID()));
                }
            }

            break;

        default:
            {
                listOptions.push_back("--");
            }

            break;
    }

    cbxToChange->setOptions(listOptions);
    cbxToChange->setSelected(0);
}

/**
 * Handles clicking the find button
 * @param action Pointer to an action.
 */
void MapEditorFindNodeState::btnFindClick(Action *action)
{
    selectNodes();

    bool ctrlPressed = (SDL_GetModState() & KMOD_CTRL) != 0;
    if (action->getSender() == _btnReplace || ctrlPressed)
    {
        replaceNodes();
    }

    _game->popState();
}

/**
 * Returns to the previous menu
 * @param action Pointer to an action.
 */
void MapEditorFindNodeState::btnCancelClick(Action *action)
{
    _game->popState();
}

/**
 * Selects nodes according to the parameters chosen
 * @param action Pointer to an action.
 */
void MapEditorFindNodeState::selectNodes()
{
    NodeChangeType parameter = _nodeParameters.at(_cbxNodeParameterFind->getSelected());
    int valueToCheck = (int)_cbxNodeValueFind->getSelected();

    size_t currentSelection = _cbxCurrentSelection->getSelected();
    bool searchInsideSelection = currentSelection != 2;
    bool searchOutsideSelection = currentSelection != 1;

    size_t handleSelection = _cbxHandleSelection->getSelected();
    bool addMatches = handleSelection != 2;
    bool keepSelection = handleSelection != 0;

    for (auto node : *_save->getNodes())
    {
        // Check whether the node matches our filter parameters
        bool isNodeAMatch = false;
        switch (parameter)
        {
            case NCT_TYPE:
                {
                    std::vector<int> nodeTypes = {0, 2, 1, 3}; // to have byte flag for type match order in drop-down
                    isNodeAMatch = node->getType() == nodeTypes.at(valueToCheck); 
                }

                break;

            case NCT_RANK:
                {
                    isNodeAMatch = node->getRank() == valueToCheck;
                }

                break;

            case NCT_FLAG:
                {
                    isNodeAMatch = node->getFlags() == valueToCheck;
                }

                break;

            case NCT_PRIORITY:
                {
                    isNodeAMatch = node->getPriority() == valueToCheck;
                }

                break;

            case NCT_RESERVED:
                {
                    isNodeAMatch = (node->isTarget() ? 5 : 0) == valueToCheck;
                }

                break;

            case NCT_LINKS:
                {
                    for (int i = 0; i < 5; ++i)
                    {
                        int linkID = node->getNodeLinks()->at(i);
                        linkID = linkID < 0 ? -linkID - 1 : linkID + 5;
                        isNodeAMatch = isNodeAMatch || linkID == valueToCheck;
                    }
                }

                break;

            default:

                break;            
        }

        std::vector<Node*>::iterator it = std::find_if(_game->getMapEditor()->getSelectedNodes()->begin(), _game->getMapEditor()->getSelectedNodes()->end(), 
                                                [&](const Node *n){ return n == node; });
        bool isNodeInSelection = it != _game->getMapEditor()->getSelectedNodes()->end();
        isNodeAMatch = isNodeAMatch && ((isNodeInSelection && searchInsideSelection) || (!isNodeInSelection && searchOutsideSelection));
        
        // adding to selection
        if (isNodeAMatch
            && addMatches
            && !isNodeInSelection && searchOutsideSelection) // don't add to selection things that are already there
        {
            _game->getMapEditor()->getSelectedNodes()->push_back(node);
        }
        // removing from selection
        else if (isNodeInSelection
                && ((!keepSelection && !isNodeAMatch) // clear out things that aren't a match if we aren't keeping them
                || (!addMatches && isNodeAMatch))) // opposite of adding matches is removing them!
        {
            _game->getMapEditor()->getSelectedNodes()->erase(it);
        }
    }
}

/**
 * Replaces node values according to the parameters chosen
 * @param action Pointer to an action.
 */
void MapEditorFindNodeState::replaceNodes()
{
    NodeChangeType parameter = _nodeParameters.at(_cbxNodeParameterReplace->getSelected());
    int valueToSet = (int)_cbxNodeValueReplace->getSelected();

    std::vector<int> data;
    data.clear();

    switch (parameter)
    {
        case NCT_TYPE:
            {
                std::vector<int> nodeTypes = {0, 2, 1, 3}; // to have byte flag for type match order in drop-down
                data.push_back(nodeTypes.at(valueToSet)); 
            }

            break;

        case NCT_RANK:
        case NCT_FLAG:
        case NCT_PRIORITY:
        case NCT_RESERVED:
            {
                data.push_back(valueToSet);
            }

            break;

        case NCT_LINKS:
            {
                std::vector<int> knownLinks;
                knownLinks.clear();
                knownLinks.push_back(-1); // unused link
                knownLinks.push_back(-2); // exit north
                knownLinks.push_back(-3); // exit east
                knownLinks.push_back(-4); // exit south
                knownLinks.push_back(-5); // exit west
                for (auto otherNode : *_save->getNodes())
                {
                    knownLinks.push_back(otherNode->getID());
                }

                data.push_back(0); // to be set later according to the node we're changing
                data.push_back(knownLinks.at(valueToSet));
            }

            break;

        default:
            {
                // don't do anything if we somehow get here with no change type
                return;
            }

            break;            
    }

	for (auto node : *_game->getMapEditor()->getSelectedNodes())
	{
        // we need some extra data for where the new links go
        if (parameter == NCT_LINKS)
        {
            // figure out which, if any, links on this node are open
            int linkIndex = _game->getMapEditor()->getNextNodeConnectionIndex(node);

            // we searched for a link, so let's replace that link instead
            if (_nodeParameters.at(_cbxNodeParameterFind->getSelected()) == NCT_LINKS)
            {
                int valueToCheck = (int)_cbxNodeValueFind->getSelected();
                for (int i = 0; i < 5; ++i)
                {
                    int linkID = node->getNodeLinks()->at(i);
                    linkID = linkID < 0 ? -linkID - 1 : linkID + 5;
                    if (linkID == valueToCheck)
                    {
                        linkIndex = i;
                        break;
                    }
                }
            }

            data.at(0) = linkIndex;
        }

		_game->getMapEditor()->changeNodeData(MET_DO, node, parameter, data);
	}

	_game->getMapEditor()->confirmChanges(true);
}

}