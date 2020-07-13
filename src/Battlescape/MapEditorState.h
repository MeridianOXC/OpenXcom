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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */
#include "../Engine/State.h"
#include "Position.h"

#include <vector>
#include <string>

namespace OpenXcom
{

class Surface;
class Map;
class ImageButton;
class BattlescapeButton;
class ComboBox;
class InteractiveSurface;
class Text;
class TextButton;
class SavedBattleGame;
class Timer;
class MapEditor;
class Node;

/**
 * Interface screen for the Map Editor
 * Very similar to a pared-down BattlescapeState
 */
class MapEditorState : public State
{
private:
	Map *_map;
	Timer *_animTimer, *_gameTimer;
	SavedBattleGame *_save;
	MapEditor *_editor;
	InteractiveSurface *_iconsLowerLeft, *_iconsLowerRight, *_iconsUpperRight;
	std::vector<InteractiveSurface*> _iconsMousedOver;
    BattlescapeButton *_btnOptions, *_btnSave, *_btnLoad, *_btnUndo, *_btnRedo;
	BattlescapeButton *_btnFill, *_btnClear, *_btnCut, *_btnCopy, *_btnPaste;
	BattlescapeButton *_tileEditMode;
	BattlescapeButton *_btnSelectedTile, *_btnTileFilterGround, *_btnTileFilterWestWall, *_btnTileFilterNorthWall, *_btnTileFilterObject;
	BattlescapeButton *_tileObjectSelected;
	std::map<TilePart, BattlescapeButton*> _tileFilters;
	InteractiveSurface *_backgroundTileSelection, *_tileSelection;
	InteractiveSurface *_panelTileSelection;
	InteractiveSurface *_backgroundTileSelectionNavigation;
	BattlescapeButton *_tileSelectionLeftArrow, *_tileSelectionRightArrow, *_tileSelectionPageCount;
	Text *_txtSelectionPageCount;
	std::vector<InteractiveSurface*> _tileSelectionGrid;
	Text *_txtTooltip, *_txtDebug;
    int _tooltipDefaultColor;
	bool _firstInit;
	bool _isMouseScrolling, _isMouseScrolled;
	int _xBeforeMouseScrolling, _yBeforeMouseScrolling;
	Position _mapOffsetBeforeMouseScrolling;
	Uint32 _mouseScrollingStartTime;
	int _totalMouseMoveX, _totalMouseMoveY;
	bool _mouseMovedOverThreshold;
	bool _mouseOverIcons;
	std::string _currentTooltip;
	Position _cursorPosition;
	bool _autosave;
	Uint8 _indicatorTextColor, _indicatorGreen, _indicatorBlue, _indicatorPurple;
	int _tileSelectionColumns, _tileSelectionRows, _tileSelectionCurrentPage, _tileSelectionLastPage;
	int _selectedTileIndex;
	// Route mode elements
	bool _routeMode;
	InteractiveSurface *_iconsUpperLeftNodes;
	BattlescapeButton *_btnRouteInformation, *_btnRouteConnections, *_nodeButtonClicked;
	InteractiveSurface *_panelRouteInformation;
	Text *_txtNodeID, *_txtNodeType, *_txtNodeRank, *_txtNodeFlag, *_txtNodePriority, *_txtNodeReserved, *_txtNodeLinks;
	ComboBox *_cbxNodeType, *_cbxNodeRank, *_cbxNodeFlag, *_cbxNodePriority, *_cbxNodeReserved;
	std::vector<ComboBox*> _cbxNodeLinks, _cbxNodeLinkTypes;
	std::vector<std::string> _nodeTypeStrings, _nodeRankStrings;
	std::vector<int> _nodeTypes;
public:
	static const int DEFAULT_ANIM_SPEED = 100;
	/// Creates the Map Editor state.
	MapEditorState(MapEditor *editor);
	/// Cleans up the Map Editor state.
	~MapEditorState();
	/// Initializes the MapEditorState.
	void init() override;
	/// Runs the timers.
	void think() override;
	/// Handler for moving mouse over the map.
	void mapOver(Action *action);
	/// Handler for pressing the map.
	void mapPress(Action *action);
	/// Handler for clicking the map.
	void mapClick(Action *action);
	/// Handler for entering with mouse to the map surface.
	void mapIn(Action *action);
    /// Handler for clicking the options button
    void btnOptionsClick(Action *action);
	/// Handler for clicking the Map Up button.
	//void btnMapUpClick(Action *action);
	/// Handler for clicking the Map Down button.
	//void btnMapDownClick(Action *action);
	/// Handler for clicking the [SelectMusicTrack] button.
	void btnSelectMusicTrackClick(Action *action);;
	/// Handler for pressing the save button.
	void btnSaveClick(Action *action);
	/// Handler for pressing the load button.
	//void btnLoadClick(Action *action);
	/// Handler for pressing the undo button.
	void btnUndoClick(Action *action);
	/// Handler for pressing the redo button.
	void btnRedoClick(Action *action);
	/// Handler for pressing the fill button.
	void btnFillClick(Action *action);
	/// Handler for pressing the clear button.
	void btnClearClick(Action *action);
	/// Handler for pressing the cut button.
	//void btnCutClick(Action *action);
	/// Handler for pressing the copy button.
	//void btnCopyClick(Action *action);
	/// Handler for pressing the paste button.
	//void btnPasteClick(Action *action);
	/// Handler for pressing the tile filter buttons.
	void btnTileFilterClick(Action *action);
	/// Handler for changing the node type combo box
	void cbxNodeTypeChange(Action *action);
	/// Handler for changing the node rank combo box
	void cbxNodeRankChange(Action *action);
	/// Handler for changing the node flag combo box
	void cbxNodeFlagChange(Action *action);
	/// Handler for changing the node priority combo box
	void cbxNodePriorityChange(Action *action);
	/// Handler for changing the node reserved combo box
	void cbxNodeReservedChange(Action *action);
	/// Handler for changing the node links combo boxes
	void cbxNodeLinksChange(Action *action);
	/// Handler for changing the node link types combo boxes
	void cbxNodeLinkTypesChange(Action *action);
	/// Animates map objects on the map
	void animate();
	/// Handles the battle game state.
	void handleState();
	/// Sets the state timer interval.
	void setStateInterval(Uint32 interval);
	/// Gets game.
	Game *getGame() const;
	/// Gets map.
	Map *getMap() const;
	/// Handles keypresses.
	void handle(Action *action) override;
	/// Updates the debug text.
	void updateDebugText();
	/// Toggles route mode on and off, updating the UI.
	void toggleRouteMode(Action *action);
	/// Toggles the node information panel on or off.
	void toggleNodeInfoPanel(Action *action, bool hide = false);
	/// Sets the route mode either on or off.
	void setRouteMode(bool routeMode);
	/// Gets whether route mode is on or off.
	bool getRouteMode();
	/// Updates the info/connection panels for the selected node/nodes.
	void updateNodePanels();
	/// Clears mouse-scrolling state.
	void clearMouseScrollingState();
	/// Returns a pointer to the battlegame, in case we need its functions.
	//BattlescapeGame *getBattleGame();
	/// Handler for the mouse moving over the icons, disables the tile selection cube.
	void mouseInIcons(Action *action);
	/// Handler for the mouse going out of the icons, enabling the tile selection cube.
	void mouseOutIcons(Action *action);
	/// Checks if the mouse is over the icons.
	bool getMouseOverIcons() const;
	/// Handler for showing tooltip.
	void txtTooltipIn(Action *action);
	/// Handler for hiding tooltip.
	void txtTooltipOut(Action *action);
	/// Update the resolution settings, we just resized the window.
	void resize(int &dX, int &dY) override;
	/// Move the mouse back to where it started after we finish drag scrolling.
	void stopScrolling(Action *action);
	/// Autosave next turn.
	//void autosave();
	/// Gets the pointer to the map editor
	MapEditor *getMapEditor();
	/// Toggles the tile selection UI
	void tileSelectionClick(Action *action);
	/// Draws the tile sprites on the selection grid
	void drawTileSelectionGrid();
	/// Moves the tile selection UI left one page
	void tileSelectionLeftArrowClick(Action *action);
	/// Moves the tile selection UI right one page
	void tileSelectionRightArrowClick(Action *action);
	/// Selects the tile from the tile selection UI
	void tileSelectionGridClick(Action *action);
	/// Handles mouse wheel scrolling for the tile selectionUI
	void tileSelectionMousePress(Action *action);
	/// Draws a tile sprite on a given surface
	bool drawTileSpriteOnSurface(Surface *surface, int index);
};

}