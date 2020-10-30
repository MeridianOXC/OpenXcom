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
#include <algorithm>
#include <sstream>
#include <string>
#include <iomanip>
#include <SDL_gfxPrimitives.h>
#include "Map.h"
#include "Camera.h"
#include "TileEngine.h"
#include "MapEditor.h"
#include "MapEditorState.h"
#include "WarningMessage.h"
#include "../lodepng.h"
#include "../fmath.h"
#include "../Geoscape/SelectMusicTrackState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Action.h"
#include "../Engine/Script.h"
#include "../Engine/Logger.h"
#include "../Engine/Timer.h"
#include "../Engine/CrossPlatform.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Menu/MapEditorMenuState.h"
#include "../Menu/MapEditorOptionsState.h"
#include "../Menu/MapEditorInfoState.h"
#include "../Menu/MapEditorSaveAsState.h"
#include "../Mod/MapData.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/Mod.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Mod/RuleInterface.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Initializes all the elements in the Map Editor screen.
 * @param game Pointer to the core game.
 * @param editor Pointer to the data structure for the in-game map editor
 */
MapEditorState::MapEditorState(MapEditor *editor) : _firstInit(true), _isMouseScrolling(false), _isMouseScrolled(false), _xBeforeMouseScrolling(0), _yBeforeMouseScrolling(0), _totalMouseMoveX(0), _totalMouseMoveY(0), _mouseMovedOverThreshold(0),
	_mouseScrollSelect(false), _mouseScrollPainting(false), _mouseOverIcons(false), _autosave(false),
	_editor(editor), _tileSelectionColumns(5), _tileSelectionRows(3), _tileSelectionCurrentPage(0), _tileSelectionLastPage(0), _selectedTileIndex(0), _routeMode(false)
{
	const int screenWidth = Options::baseXResolution;
	const int screenHeight = Options::baseYResolution;

	_tooltipDefaultColor = _game->getMod()->getInterface("battlescape")->getElement("textTooltip")->color;

	// Create the battlemap view
	_map = new Map(_game, screenWidth, screenHeight, 0, 0, screenHeight, true);

	// Create buttons
	//_btnMapUp = new BattlescapeButton(32, 16, x + 80, y);
	//_btnMapDown = new BattlescapeButton(32, 16, x + 80, y + 16);

	_txtTooltip = new Text(300, 20, 2, screenHeight - 60);
	_txtDebug = new Text(158, 32, screenWidth - 160, 40);

	SurfaceSet *icons = _game->getMod()->getSurfaceSet("MapEditorIcons");

	// TODO: create interface ruleset for the map editor for all these hardcoded colors
	_iconsLowerLeft = new InteractiveSurface(160, 40, 0, screenHeight - 40);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i)->blitNShade(_iconsLowerLeft, i * 32, 0);
	}
	_btnOptions = new BattlescapeButton(32, 40, 0, screenHeight - 40);
	_btnOptions->setColor(232);
	_btnSave = new BattlescapeButton(32, 40, 32, screenHeight - 40);
	_btnSave->setColor(232);
	_btnLoad = new BattlescapeButton(32, 40, 64, screenHeight - 40);
	_btnLoad->setColor(232);
	_btnUndo = new BattlescapeButton(32, 40, 96, screenHeight - 40);
	_btnUndo->setColor(232);
	_btnRedo = new BattlescapeButton(32, 40, 128, screenHeight - 40);
	_btnRedo->setColor(232);

	_iconsLowerRight = new InteractiveSurface(160, 40, screenWidth - 160, screenHeight - 40);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i + 5)->blitNShade(_iconsLowerRight, i * 32, 0);
	}
	_btnFill = new BattlescapeButton(32, 40, screenWidth - 160, screenHeight - 40);
	_btnFill->setColor(232);
	_btnClear = new BattlescapeButton(32, 40, screenWidth - 128, screenHeight - 40);
	_btnClear->setColor(232);
	_btnCut = new BattlescapeButton(32, 40, screenWidth - 96, screenHeight - 40);
	_btnCut->setColor(232);
	_btnCopy = new BattlescapeButton(32, 40, screenWidth - 64, screenHeight - 40);
	_btnCopy->setColor(232);
	_btnPaste = new BattlescapeButton(32, 40, screenWidth - 32, screenHeight - 40);
	_btnPaste->setColor(232);

	_tileEditMode = _btnClear;
	_btnFill->setGroup(&_tileEditMode);
	_btnClear->setGroup(&_tileEditMode);

	_iconsUpperRight = new InteractiveSurface(160, 40, screenWidth - 160, 0);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i + 10)->blitNShade(_iconsUpperRight, i * 32, 0);
	}
	_btnSelectedTile = new BattlescapeButton(32, 40, screenWidth - 160, 0);
	_btnSelectedTile->setColor(232);
	_btnTileFilterGround = new BattlescapeButton(32, 40, screenWidth - 128, 0);
	_btnTileFilterGround->setColor(232);
	_btnTileFilterWestWall = new BattlescapeButton(32, 40, screenWidth - 96, 0);
	_btnTileFilterWestWall->setColor(232);
	_btnTileFilterNorthWall = new BattlescapeButton(32, 40, screenWidth - 64, 0);
	_btnTileFilterNorthWall->setColor(232);
	_btnTileFilterObject = new BattlescapeButton(32, 40, screenWidth - 32, 0);
	_btnTileFilterObject->setColor(232);

	_tileFilters[O_FLOOR] = _btnTileFilterGround;
	_tileFilters[O_WESTWALL] = _btnTileFilterWestWall;
	_tileFilters[O_NORTHWALL] = _btnTileFilterNorthWall;
	_tileFilters[O_OBJECT] = _btnTileFilterObject;

	_backgroundTileSelection = new InteractiveSurface(32, 40, 0, 0);
	icons->getFrame(15)->blitNShade(_backgroundTileSelection, 0, 0);
	_backgroundTileSelectionNavigation = new InteractiveSurface(96, 40, 32, 0);
	for (int i = 0; i < 3; ++i)
	{
		icons->getFrame(i + 16)->blitNShade(_backgroundTileSelectionNavigation, i * 32, 0);
	}
	_tileSelectionColumns = screenWidth / 2 / 32;
	_tileSelectionRows = (screenHeight - 2 * 40) / 40;
	int tileSelectionWidth = _tileSelectionColumns * 32;
	int tileSelectionHeight = _tileSelectionRows * 40;
	_tileSelection = new InteractiveSurface(32, 40, 0, 0);
	_panelTileSelection = new InteractiveSurface(tileSelectionWidth, tileSelectionHeight, 0, 40);
	_tileSelectionPageCount = new BattlescapeButton(32, 12, 64, 28);
	_tileSelectionLeftArrow = new BattlescapeButton(32, 12, 32, 28);
	_tileSelectionRightArrow = new BattlescapeButton(32, 12, 96, 28);
	_txtSelectionPageCount = new Text(32, 12, 64, 28);
	_tileSelectionGrid.clear();
	for (int i = 0; i < _tileSelectionRows; ++i)
	{
		for (int j = 0; j < _tileSelectionColumns; ++j)
		{
			// select which of the background panel frames is appropriate for this position on the grid
			int panelSpriteOffset = 20;
			if (i % (_tileSelectionRows - 1) != 0) // we're in a middle row
				panelSpriteOffset += 3;
			else if (i / (_tileSelectionRows - 1) == 1) // we're on the bottom row
				panelSpriteOffset += 6;
			// else we're on the top row

			if (j % (_tileSelectionColumns - 1) != 0) // we're in a middle column
				panelSpriteOffset += 1;
			else if (j / (_tileSelectionColumns - 1) == 1) // we're on the right edge
				panelSpriteOffset += 2;
			// else we're on the left edge

			// draw the background
			icons->getFrame(panelSpriteOffset)->blitNShade(_panelTileSelection, j * 32, i * 40);

			_tileSelectionGrid.push_back(new InteractiveSurface(32, 40, j * 32, i * 40 + 40));
		}
	}

	_iconModeSwitch = new InteractiveSurface(32, 40, 128, 0);
	icons->getFrame(19)->blitNShade(_iconModeSwitch, 0, 0);
	_btnModeSwitch = new BattlescapeButton(32, 40, 128, 0);
	_btnModeSwitch->setColor(232);

	// Elements for route editing mode
	// We keep the lower-left icons the same between modes, no need to make a new set for that

	_iconsLowerRightNodes = new InteractiveSurface(160, 40, screenWidth - 160, screenHeight - 40);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i + 35)->blitNShade(_iconsLowerRightNodes, i * 32, 0);
	}
	_btnNodeNew = new BattlescapeButton(32, 40, screenWidth - 160, screenHeight - 40);
	_btnNodeNew->setColor(232);
	_btnNodeDelete = new BattlescapeButton(32, 40, screenWidth - 128, screenHeight - 40);
	_btnNodeDelete->setColor(232);
	_btnNodeCut = new BattlescapeButton(32, 40, screenWidth - 96, screenHeight - 40);
	_btnNodeCut->setColor(232);
	_btnNodeCopy = new BattlescapeButton(32, 40, screenWidth - 64, screenHeight - 40);
	_btnNodeCopy->setColor(232);
	_btnNodePaste = new BattlescapeButton(32, 40, screenWidth - 32, screenHeight - 40);
	_btnNodePaste->setColor(232);

	_nodeEditMode = _btnNodeDelete;
	_btnNodeNew->setGroup(&_nodeEditMode);
	//_btnNodeDelete->setGroup(&_nodeEditMode); // this one will have group set when first switching to route mode

	_iconsUpperRightNodes = new InteractiveSurface(160, 40, screenWidth - 160, 0);
	for (int i = 0; i < 5; ++i)
	{
		icons->getFrame(i + 40)->blitNShade(_iconsUpperRightNodes, i * 32, 0);
	}
	_btnSelectedNode = new BattlescapeButton(32, 40, screenWidth - 160, 0);
	_btnSelectedNode->setColor(232);
	_btnNodeFilterSelect = new BattlescapeButton(32, 40, screenWidth - 128, 0);
	_btnNodeFilterSelect->setColor(232);
	_btnNodeFilterMove = new BattlescapeButton(32, 40, screenWidth - 96, 0);
	_btnNodeFilterMove->setColor(232);
	_btnNodeFilterOneWayConnect = new BattlescapeButton(32, 40, screenWidth - 64, 0);
	_btnNodeFilterOneWayConnect->setColor(232);
	_btnNodeFilterTwoWayConnect = new BattlescapeButton(32, 40, screenWidth - 32, 0);
	_btnNodeFilterTwoWayConnect->setColor(232);

	_nodeFilterMode = _btnNodeFilterSelect;
	//_btnNodeFilterSelect->setGroup(&_nodeFilterMode); // this one will have group set when first switching to route mode
	_btnNodeFilterMove->setGroup(&_nodeFilterMode);
	_btnNodeFilterOneWayConnect->setGroup(&_nodeFilterMode);
	_btnNodeFilterTwoWayConnect->setGroup(&_nodeFilterMode);

	//// TODO: sprites for the buttons
	_iconsUpperLeftNodes = new InteractiveSurface(64, 40, 0, 0);
	icons->getFrame(45)->blitNShade(_iconsUpperLeftNodes, 0, 0);
	icons->getFrame(46)->blitNShade(_iconsUpperLeftNodes, 32, 0);
	_btnRouteInformation = new BattlescapeButton(32, 40, 0, 0);
	_btnRouteConnections = new BattlescapeButton(32, 40, 32, 0);
	int nodePanelWidth = 5 * 32;
	_panelRouteInformation = new InteractiveSurface(nodePanelWidth, tileSelectionHeight, 0, 40);
	// General information panel
	_txtNodeID = new Text(144, 10, 4, 44);
	_txtNodeType = new Text(144, 10, 4, 54);
	_cbxNodeType = new ComboBox(this, 144, 16, 8 - 160, 63, false);
	_txtNodeRank = new Text(144, 10, 4, 80);
	_cbxNodeRank = new ComboBox(this, 144, 16, 8 - 160, 89, false);
	_txtNodeFlag = new Text(144, 10, 4, 110);
	_cbxNodeFlag = new ComboBox(this, 32, 16, 124 - 160, 106, false);
	_txtNodePriority = new Text(144, 10, 4, 127);
	_cbxNodePriority = new ComboBox(this, 32, 16, 124 - 160, 123, false);
	_txtNodeReserved = new Text(144, 10, 4, 144);
	_cbxNodeReserved = new ComboBox(this, 32, 16, 124 - 160, 140, false);
	// Node links panel
	_txtNodeLinks = new Text(144, 10, 4, 54);
	_cbxNodeLinks.clear();
	_cbxNodeLinkTypes.clear();
	for (int i = 0; i < 5; ++i)
	{
		_cbxNodeLinks.push_back(new ComboBox(this, 68, 16, 8 - 160, 64 + i * 18, false));
		_cbxNodeLinkTypes.push_back(new ComboBox(this, 68, 16, 84 - 160, 64 + i * 18, false));
	}

	// Draw the background for the panel
	for (int i = 0; i < _tileSelectionRows; ++i)
	{
		for (int j = 0; j < nodePanelWidth / 32; ++j) // the node panel width is measured in pixels, covert to width of sprite
		{
			// select which of the background panel frames is appropriate for this position on the grid
			int panelSpriteOffset = 50;
			if (i % (_tileSelectionRows - 1) != 0) // we're in a middle row
				panelSpriteOffset += 3;
			else if (i / (_tileSelectionRows - 1) == 1) // we're on the bottom row
				panelSpriteOffset += 6;
			// else we're on the top row

			if (j % (nodePanelWidth / 32 - 1) != 0) // we're in a middle column
				panelSpriteOffset += 1;
			else if (j / (nodePanelWidth / 32 - 1) == 1) // we're on the right edge
				panelSpriteOffset += 2;
			// else we're on the left edge

			// draw the background
			icons->getFrame(panelSpriteOffset)->blitNShade(_panelRouteInformation, j * 32, i * 40);
		}
	}

	_message = new WarningMessage(screenWidth, 10, 0, screenHeight - 50);

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_map);
	add(_txtTooltip, "textTooltip", "battlescape");
	add(_txtDebug, "textTooltip", "battlescape");
	add(_iconsLowerLeft);
	add(_iconsLowerRight);
	add(_iconsUpperRight);
	add(_btnOptions, "", "battlescape", _iconsLowerLeft);
	add(_btnSave, "", "battlescape", _iconsLowerLeft);
	add(_btnLoad, "", "battlescape", _iconsLowerLeft);
	add(_btnUndo, "", "battlescape", _iconsLowerLeft);
	add(_btnRedo, "", "battlescape", _iconsLowerLeft);
	add(_btnFill, "", "battlescape", _iconsLowerRight);
	add(_btnClear, "", "battlescape", _iconsLowerRight);
	add(_btnCut, "", "battlescape", _iconsLowerRight);
	add(_btnCopy, "", "battlescape", _iconsLowerRight);
	add(_btnPaste, "", "battlescape", _iconsLowerRight);
	add(_btnSelectedTile, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterGround, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterWestWall, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterNorthWall, "", "battlescape", _iconsUpperRight);
	add(_btnTileFilterObject, "", "battlescape", _iconsUpperRight);
	add(_backgroundTileSelection);
	add(_backgroundTileSelectionNavigation);
	add(_tileSelection, "", "battlescape", _backgroundTileSelection);
	add(_panelTileSelection);
	add(_tileSelectionPageCount, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_tileSelectionLeftArrow, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_tileSelectionRightArrow, "", "battlescape", _backgroundTileSelectionNavigation);
	add(_txtSelectionPageCount);
	for (auto i : _tileSelectionGrid)
	{
		add(i);
		i->onMouseClick((ActionHandler)&MapEditorState::tileSelectionGridClick);
	}
	add(_iconModeSwitch);
	add(_btnModeSwitch, "", "battlescape", _iconModeSwitch);

	add(_iconsLowerRightNodes);
	add(_iconsUpperRightNodes);
	add(_iconsUpperLeftNodes);
	add(_btnNodeNew, "", "battlescape", _iconsLowerRightNodes);
	add(_btnNodeDelete, "", "battlescape", _iconsLowerRightNodes);
	add(_btnNodeCut, "", "battlescape", _iconsLowerRightNodes);
	add(_btnNodeCopy, "", "battlescape", _iconsLowerRightNodes);
	add(_btnNodePaste, "", "battlescape", _iconsLowerRightNodes);
	add(_btnSelectedNode, "", "battlescape", _iconsUpperRightNodes);
	add(_btnNodeFilterSelect, "", "battlescape", _iconsUpperRightNodes);
	add(_btnNodeFilterMove, "", "battlescape", _iconsUpperRightNodes);
	add(_btnNodeFilterOneWayConnect, "", "battlescape", _iconsUpperRightNodes);
	add(_btnNodeFilterTwoWayConnect, "", "battlescape", _iconsUpperRightNodes);
	add(_btnRouteInformation, "", "battlescape", _iconsUpperLeftNodes);
	add(_btnRouteConnections, "", "battlescape", _iconsUpperLeftNodes);
	add(_panelRouteInformation);
	add(_txtNodeID, "textTooltip", "battlescape");
	add(_txtNodeType, "textTooltip", "battlescape");
	add(_txtNodeRank, "textTooltip", "battlescape");
	add(_txtNodeFlag, "textTooltip", "battlescape");
	add(_txtNodePriority, "textTooltip", "battlescape");
	add(_txtNodeReserved, "textTooltip", "battlescape");
	add(_cbxNodeReserved, "infoBoxOKButton", "battlescape");
	add(_cbxNodePriority, "infoBoxOKButton", "battlescape");
	add(_cbxNodeFlag, "infoBoxOKButton", "battlescape");
	add(_cbxNodeRank, "infoBoxOKButton", "battlescape");
	add(_cbxNodeType, "infoBoxOKButton", "battlescape");
	add(_txtNodeLinks, "textTooltip", "battlescape");
	for (int i = 4; i >= 0; --i)
	{
		add(_cbxNodeLinks.at(i), "infoBoxOKButton", "battlescape");
		add(_cbxNodeLinkTypes.at(i), "infoBoxOKButton", "battlescape");
	}
	add(_message, "warning", "battlescape");

	// Set up objects
	_save = _game->getSavedGame()->getSavedBattle();
	_editor->setSave(_save);
	_editor->setSelectedMapDataID(-1);

	_map->init();
	_map->onMouseOver((ActionHandler)&MapEditorState::mapOver);
	_map->onMousePress((ActionHandler)&MapEditorState::mapPress);
	_map->onMouseClick((ActionHandler)&MapEditorState::mapClick, 0);
	_map->onMouseIn((ActionHandler)&MapEditorState::mapIn);

	//_btnMapUp->onMouseClick((ActionHandler)&MapEditorState::btnMapUpClick);
	//_btnMapUp->onKeyboardPress((ActionHandler)&MapEditorState::btnMapUpClick, Options::keyBattleLevelUp);
	//_btnMapUp->setTooltip("STR_VIEW_LEVEL_ABOVE");
	//_btnMapUp->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	//_btnMapUp->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnMapDown->onMouseClick((ActionHandler)&MapEditorState::btnMapDownClick);
	//_btnMapDown->onKeyboardPress((ActionHandler)&MapEditorState::btnMapDownClick, Options::keyBattleLevelDown);
	//_btnMapDown->setTooltip("STR_VIEW_LEVEL_BELOW");
	//_btnMapDown->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	//_btnMapDown->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_txtTooltip->setHighContrast(true);
	_txtTooltip->setWordWrap(true);
	_txtTooltip->setVerticalAlign(ALIGN_BOTTOM);

	updateDebugText();
	_txtDebug->setColor(_tooltipDefaultColor);
	_txtDebug->setHighContrast(true);
	_txtDebug->setAlign(ALIGN_RIGHT);

	// Set music
	if (!Options::oxcePlayBriefingMusicDuringEquipment)
	{
		if (_save->getMusic().empty())
		{
			_game->getMod()->playMusic("GMTACTIC");
		}
		else
		{
			_game->getMod()->playMusic(_save->getMusic());
		}
	}

	_iconsLowerLeft->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsLowerLeft->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsLowerRight->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsLowerRight->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsUpperRight->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsUpperRight->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_backgroundTileSelection->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_backgroundTileSelection->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_backgroundTileSelectionNavigation->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_backgroundTileSelectionNavigation->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_panelTileSelection->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_panelTileSelection->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconModeSwitch->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconModeSwitch->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsMousedOver.clear();

	_btnOptions->onMouseClick((ActionHandler)&MapEditorState::btnOptionsClick);
	_btnOptions->onKeyboardPress((ActionHandler)&MapEditorState::btnOptionsClick, Options::keyBattleOptions);
	_btnOptions->setTooltip("STR_OPTIONS");
	_btnOptions->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnOptions->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnSave->onMouseClick((ActionHandler)&MapEditorState::btnSaveClick);
	//_btnSave->onKeyboardPress((ActionHandler)&MapEditorState::btnSaveClick, SDLK_s); // change to options
	_btnSave->setTooltip("STR_TOOLTIP_SAVE_MAP");
	_btnSave->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnSave->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnLoad->onMouseClick((ActionHandler)&MapEditorState::btnLoadClick);
	//_btnLoad->onKeyboardPress((ActionHandler)&MapEditorState::btnLoadClick, SDLK_o); // change to options
	_btnLoad->setTooltip("STR_TOOLTIP_LOAD_MAP");
	_btnLoad->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnLoad->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnUndo->onMouseClick((ActionHandler)&MapEditorState::btnUndoClick);
	//_btnUndo->onKeyboardPress((ActionHandler)&MapEditorState::btnUndoClick, SDLK_z); // change to options
	_btnUndo->setTooltip("STR_TOOLTIP_UNDO");
	_btnUndo->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnUndo->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnRedo->onMouseClick((ActionHandler)&MapEditorState::btnRedoClick);
	//_btnRedo->onKeyboardPress((ActionHandler)&MapEditorState::btnRedoClick, SDLK_z); // change to options
	_btnRedo->setTooltip("STR_TOOLTIP_REDO");
	_btnRedo->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnRedo->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnFill->onMouseClick((ActionHandler)&MapEditorState::btnFillClick);
	//_btnFill->onKeyboardPress((ActionHandler)&MapEditorState::btnFillClick, SDLK_z); // change to options
	_btnFill->setTooltip("STR_TOOLTIP_FILL");
	_btnFill->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnFill->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnClear->onMouseClick((ActionHandler)&MapEditorState::btnClearClick);
	//_btnClear->onKeyboardPress((ActionHandler)&MapEditorState::btnClearClick, SDLK_z); // change to options
	_btnClear->setTooltip("STR_TOOLTIP_CLEAR");
	_btnClear->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnClear->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnCut->onMouseClick((ActionHandler)&MapEditorState::btnCutClick);
	//_btnCut->onKeyboardPress((ActionHandler)&MapEditorState::btnCutClick, SDLK_x); // change to options
	_btnCut->setTooltip("STR_TOOLTIP_CUT");
	_btnCut->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnCut->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnCopy->onMouseClick((ActionHandler)&MapEditorState::btnCopyClick);
	//_btnCopy->onKeyboardPress((ActionHandler)&MapEditorState::btnCopyClick, SDLK_c); // change to options
	_btnCopy->setTooltip("STR_TOOLTIP_COPY");
	_btnCopy->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnCopy->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnPaste->onMouseClick((ActionHandler)&MapEditorState::btnPasteClick);
	//_btnPaste->onKeyboardPress((ActionHandler)&MapEditorState::btnPasteClick, SDLK_v); // change to options
	_btnPaste->setTooltip("STR_TOOLTIP_PASTE");
	_btnPaste->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnPaste->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_btnSelectedTile->onMouseClick((ActionHandler)&MapEditorState::btnSelectedTileClick);
	//_btnSelectedTile->onKeyboardPress((ActionHandler)&MapEditorState::btnSelectedTileClick, SDLK_v); // change to options
	_btnSelectedTile->setTooltip("STR_TOOLTIP_SELECTED_TILE");
	_btnSelectedTile->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnSelectedTile->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterGround->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterGround->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_1); // change to options
	_btnTileFilterGround->setTooltip("STR_TOOLTIP_TILE_GROUND");
	_btnTileFilterGround->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterGround->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterWestWall->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterWestWall->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_2); // change to options
	_btnTileFilterWestWall->setTooltip("STR_TOOLTIP_TILE_WESTWALL");
	_btnTileFilterWestWall->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterWestWall->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterNorthWall->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterNorthWall->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_3); // change to options
	_btnTileFilterNorthWall->setTooltip("STR_TOOLTIP_TILE_NORTHWALL");
	_btnTileFilterNorthWall->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterNorthWall->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	_btnTileFilterObject->onMouseClick((ActionHandler)&MapEditorState::btnTileFilterClick);
	_btnTileFilterObject->onKeyboardPress((ActionHandler)&MapEditorState::btnTileFilterClick, SDLK_4); // change to options
	_btnTileFilterObject->setTooltip("STR_TOOLTIP_TILE_OBJECT");
	_btnTileFilterObject->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnTileFilterObject->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	//_tileSelection->setColor(232); // Goal of background color 235
	_tileSelection->onMouseClick((ActionHandler)&MapEditorState::tileSelectionClick);
	_tileSelection->setTooltip("STR_TOOLTIP_TILE_SELECTION");
	_tileSelection->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_tileSelection->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	int mapDataObjects = 0;
	for (auto mapDataSet : *_save->getMapDataSets())
	{
		mapDataObjects += mapDataSet->getSize();
	}
	_tileSelectionLastPage = mapDataObjects / (_tileSelectionColumns * _tileSelectionRows);
	_txtSelectionPageCount->setSmall();
	_txtSelectionPageCount->setAlign(ALIGN_CENTER);
	_txtSelectionPageCount->setVerticalAlign(ALIGN_MIDDLE);
	_txtSelectionPageCount->setWordWrap(true);
	_txtSelectionPageCount->setColor(224);
	_txtSelectionPageCount->setHighContrast(true);
	std::ostringstream ss;
	ss << _tileSelectionCurrentPage + 1 << "/" << _tileSelectionLastPage + 1;
	_txtSelectionPageCount->setText(ss.str().c_str());
	_txtSelectionPageCount->setVisible(false);

	_tileSelectionPageCount->setVisible(false);
	_tileSelectionPageCount->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);

	if (_tileSelectionCurrentPage == _tileSelectionLastPage)
	{
		size_t arrowColor = 1;
		icons->getFrame(16)->blitNShade(_tileSelectionLeftArrow, 0, -28, 0, false, arrowColor);
		icons->getFrame(18)->blitNShade(_tileSelectionRightArrow, 0, -28, 0, false, arrowColor);
	}

	_tileSelectionLeftArrow->onMouseClick((ActionHandler)&MapEditorState::tileSelectionLeftArrowClick);
	_tileSelectionLeftArrow->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	_tileSelectionLeftArrow->setVisible(false);

	_tileSelectionRightArrow->onMouseClick((ActionHandler)&MapEditorState::tileSelectionRightArrowClick);
	_tileSelectionRightArrow->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	_tileSelectionRightArrow->setVisible(false);

	//_panelTileSelection->setColor(232); // nice purple
	_panelTileSelection->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_panelTileSelection->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_panelTileSelection->onMousePress((ActionHandler)&MapEditorState::tileSelectionMousePress);
	drawTileSelectionGrid();
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(false);
	}
	_panelTileSelection->setVisible(false);

	_backgroundTileSelectionNavigation->setVisible(false);

	_btnModeSwitch->onMouseClick((ActionHandler)&MapEditorState::btnModeSwitchClick);
	//_btnOptions->onKeyboardPress((ActionHandler)&MapEditorState::btnOptionsClick, Options::keyBattleOptions);
	_btnModeSwitch->setTooltip("STR_TOOLTIP_SWITCH_MODES");
	_btnModeSwitch->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnModeSwitch->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

	// Route mode elements
	_iconsLowerRightNodes->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsLowerRightNodes->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsLowerRightNodes->setVisible(false);

	_iconsUpperRightNodes->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsUpperRightNodes->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsUpperRightNodes->setVisible(false);

	_iconsUpperLeftNodes->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_iconsUpperLeftNodes->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);
	_iconsUpperLeftNodes->setVisible(false);

	_btnSelectedNode->setVisible(false);

	_btnNodeNew->onMouseClick((ActionHandler)&MapEditorState::btnNodeNewClick);
	//_btnNodeNew->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeNewClick, SDLK_1); // change to options
	_btnNodeNew->setTooltip("STR_TOOLTIP_NODE_NEW");
	_btnNodeNew->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeNew->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeNew->setVisible(false);

	_btnNodeDelete->onMouseClick((ActionHandler)&MapEditorState::btnNodeDeleteClick);
	//_btnNodeDelete->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeDeleteClick, SDLK_1); // change to options
	_btnNodeDelete->setTooltip("STR_TOOLTIP_NODE_DELETE");
	_btnNodeDelete->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeDelete->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeDelete->setVisible(false);

	//_btnNodeCut->onMouseClick((ActionHandler)&MapEditorState::btnNodeCutClick);
	//_btnNodeCut->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeCutClick, SDLK_1); // change to options
	_btnNodeCut->setTooltip("STR_TOOLTIP_NODE_CUT");
	_btnNodeCut->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeCut->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeCut->setVisible(false);

	//_btnNodeCopy->onMouseClick((ActionHandler)&MapEditorState::btnNodeCopyClick);
	//_btnNodeCopy->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeCopyClick, SDLK_1); // change to options
	_btnNodeCopy->setTooltip("STR_TOOLTIP_NODE_COPY");
	_btnNodeCopy->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeCopy->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeCopy->setVisible(false);

	//_btnNodePaste->onMouseClick((ActionHandler)&MapEditorState::btnNodePasteClick);
	//_btnNodePaste->onKeyboardPress((ActionHandler)&MapEditorState::btnNodePasteClick, SDLK_1); // change to options
	_btnNodePaste->setTooltip("STR_TOOLTIP_NODE_PASTE");
	_btnNodePaste->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodePaste->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodePaste->setVisible(false);

	_btnNodeFilterSelect->onMouseClick((ActionHandler)&MapEditorState::btnNodeFilterClick);
	_btnNodeFilterSelect->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeFilterClick, SDLK_1); // change to options
	_btnNodeFilterSelect->setTooltip("STR_TOOLTIP_NODE_SELECT");
	_btnNodeFilterSelect->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeFilterSelect->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeFilterSelect->setVisible(false);

	_btnNodeFilterMove->onMouseClick((ActionHandler)&MapEditorState::btnNodeFilterClick);
	_btnNodeFilterMove->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeFilterClick, SDLK_2); // change to options
	_btnNodeFilterMove->setTooltip("STR_TOOLTIP_NODE_MOVE");
	_btnNodeFilterMove->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeFilterMove->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeFilterMove->setVisible(false);

	_btnNodeFilterOneWayConnect->onMouseClick((ActionHandler)&MapEditorState::btnNodeFilterClick);
	_btnNodeFilterOneWayConnect->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeFilterClick, SDLK_3); // change to options
	_btnNodeFilterOneWayConnect->setTooltip("STR_TOOLTIP_NODE_ONEWAYCONNECT");
	_btnNodeFilterOneWayConnect->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeFilterOneWayConnect->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeFilterOneWayConnect->setVisible(false);

	_btnNodeFilterTwoWayConnect->onMouseClick((ActionHandler)&MapEditorState::btnNodeFilterClick);
	_btnNodeFilterTwoWayConnect->onKeyboardPress((ActionHandler)&MapEditorState::btnNodeFilterClick, SDLK_4); // change to options
	_btnNodeFilterTwoWayConnect->setTooltip("STR_TOOLTIP_NODE_TWOWAYCONNECT");
	_btnNodeFilterTwoWayConnect->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnNodeFilterTwoWayConnect->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnNodeFilterTwoWayConnect->setVisible(false);

	//_btnRouteInformation->setText(tr("STR_INFO"));
	_btnRouteInformation->onMouseClick((ActionHandler)&MapEditorState::toggleNodeInfoPanel);
	_btnRouteInformation->setTooltip("STR_TOOLTIP_NODE_INFO");
	_btnRouteInformation->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnRouteInformation->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnRouteInformation->setVisible(false);

	//_btnRouteConnections->setText(tr("STR_LINKS"));
	_btnRouteConnections->onMouseClick((ActionHandler)&MapEditorState::toggleNodeInfoPanel);
	_btnRouteConnections->setTooltip("STR_TOOLTIP_NODE_LINK");
	_btnRouteConnections->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_btnRouteConnections->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_btnRouteConnections->setVisible(false);

	_panelRouteInformation->onMouseIn((ActionHandler)&MapEditorState::mouseInIcons);
	_panelRouteInformation->onMouseOut((ActionHandler)&MapEditorState::mouseOutIcons);

	_txtNodeID->setColor(_tooltipDefaultColor);
	_txtNodeID->setHighContrast(true);
	_txtNodeID->setText(tr("STR_NODE_ID"));

	_txtNodeType->setColor(_tooltipDefaultColor);
	_txtNodeType->setHighContrast(true);
	_txtNodeType->setText(tr("STR_NODE_TYPE"));

	_txtNodeRank->setColor(_tooltipDefaultColor);
	_txtNodeRank->setHighContrast(true);
	_txtNodeRank->setText(tr("STR_NODE_RANK"));

	_txtNodeFlag->setColor(_tooltipDefaultColor);
	_txtNodeFlag->setHighContrast(true);
	_txtNodeFlag->setText(tr("STR_NODE_FLAG")); // patrol priority is called "flag" in Node

	_txtNodePriority->setColor(_tooltipDefaultColor);
	_txtNodePriority->setHighContrast(true);
	_txtNodePriority->setText(tr("STR_NODE_PRIORITY")); // spawn priority is called "priority" in Node

	_txtNodeReserved->setColor(_tooltipDefaultColor);
	_txtNodeReserved->setHighContrast(true);
	_txtNodeReserved->setText(tr("STR_NODE_RESERVED")); // whether the node is flagged for aliens attackin in base defense is called "reserved" in Node

	_txtNodeLinks->setColor(_tooltipDefaultColor);
	_txtNodeLinks->setHighContrast(true);
	_txtNodeLinks->setText(tr("STR_NODE_LINKS"));

	_nodeTypeStrings.clear();
	_nodeTypeStrings.push_back("STR_NODE_TYPE_ANY");
	_nodeTypeStrings.push_back("STR_NODE_TYPE_SMALL");
	_nodeTypeStrings.push_back("STR_NODE_TYPE_FLYING");
	_nodeTypeStrings.push_back("STR_NODE_TYPE_FLYINGSMALL");

	// Set up node types according to their bit flag values
	// Numbers match bit flags from Node.h:
	// static const int TYPE_FLYING = 0x01; // non-flying unit can not spawn here when this bit is set
	// static const int TYPE_SMALL = 0x02; // large unit can not spawn here when this bit is set
	_nodeTypes.clear();
	_nodeTypes.push_back(0);
	_nodeTypes.push_back(2);
	_nodeTypes.push_back(1);
	_nodeTypes.push_back(3);

	std::vector<std::string> numberStrings;
	numberStrings.clear();
	for (int i = 0; i < 10; ++i)
	{
		numberStrings.push_back(std::to_string(i));
	}

	_nodeRankStrings.clear();
	_nodeRankStrings.push_back("STR_NODE_RANK_CIVSCOUT");
	_nodeRankStrings.push_back("STR_NODE_RANK_XCOM");
	_nodeRankStrings.push_back("STR_NODE_RANK_SOLDIER");
	_nodeRankStrings.push_back("STR_NODE_RANK_NAVIGATOR");
	_nodeRankStrings.push_back("STR_NODE_RANK_LEADERCOMMANDER");
	_nodeRankStrings.push_back("STR_NODE_RANK_ENGINEER");
	_nodeRankStrings.push_back("STR_NODE_RANK_TERRORIST0");
	_nodeRankStrings.push_back("STR_NODE_RANK_MEDIC");
	_nodeRankStrings.push_back("STR_NODE_RANK_TERRORIST1");

	_cbxNodeType->onChange((ActionHandler)&MapEditorState::cbxNodeTypeChange);
	_cbxNodeType->setTooltip("STR_TOOLTIP_NODE_TYPE");
	_cbxNodeType->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_cbxNodeType->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_cbxNodeType->setOptions(_nodeTypeStrings, true);

	_cbxNodeRank->onChange((ActionHandler)&MapEditorState::cbxNodeRankChange);
	_cbxNodeRank->setTooltip("STR_TOOLTIP_NODE_RANK");
	_cbxNodeRank->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_cbxNodeRank->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_cbxNodeRank->setOptions(_nodeRankStrings, true);

	_cbxNodeFlag->onChange((ActionHandler)&MapEditorState::cbxNodeFlagChange);
	_cbxNodeFlag->setTooltip("STR_TOOLTIP_NODE_FLAG");
	_cbxNodeFlag->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_cbxNodeFlag->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_cbxNodeFlag->setOptions(numberStrings, false);

	_cbxNodePriority->onChange((ActionHandler)&MapEditorState::cbxNodePriorityChange);
	_cbxNodePriority->setTooltip("STR_TOOLTIP_NODE_PRIORITY");
	_cbxNodePriority->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_cbxNodePriority->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_cbxNodePriority->setOptions(numberStrings, false);

	_cbxNodeReserved->onChange((ActionHandler)&MapEditorState::cbxNodeReservedChange);
	_cbxNodeReserved->setTooltip("STR_TOOLTIP_NODE_RESERVED");
	_cbxNodeReserved->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
	_cbxNodeReserved->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	_cbxNodeReserved->setOptions(numberStrings, false);

	// TODO: remove magic number 5
	for (int i = 0; i < 5; ++i)
	{
		_cbxNodeLinks.at(i)->onChange((ActionHandler)&MapEditorState::cbxNodeLinksChange);
		_cbxNodeLinks.at(i)->setTooltip("STR_TOOLTIP_NODE_LINKS");
		_cbxNodeLinks.at(i)->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
		_cbxNodeLinks.at(i)->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);

		_cbxNodeLinkTypes.at(i)->onChange((ActionHandler)&MapEditorState::cbxNodeLinkTypesChange);
		_cbxNodeLinkTypes.at(i)->setTooltip("STR_TOOLTIP_NODE_LINKTYPES");
		_cbxNodeLinkTypes.at(i)->onMouseIn((ActionHandler)&MapEditorState::txtTooltipIn);
		_cbxNodeLinkTypes.at(i)->onMouseOut((ActionHandler)&MapEditorState::txtTooltipOut);
	}

	_message->setTextColor(_tooltipDefaultColor);

	updateNodePanels();
	toggleNodeInfoPanel(0, true);

	_animTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_animTimer->onTimer((StateHandler)&MapEditorState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_gameTimer->onTimer((StateHandler)&MapEditorState::handleState);

	_map->enableObstacles();

	_scrollStartPosition = _scrollPreviousPosition = _scrollCurrentPosition = Position(-1, -1, -1);
	_proposedSelection.clear();
}


/**
 * Deletes the MapEditorState.
 */
MapEditorState::~MapEditorState()
{
	delete _animTimer;
	delete _gameTimer;
}

/**
 * Initializes the battlescapestate.
 */
void MapEditorState::init()
{
	State::init();
	_animTimer->start();
	_gameTimer->start();
	_map->setFocus(true);
	_map->draw();

	int midpointX = _save->getMapSizeX() / 2;
	int midpointY = _save->getMapSizeY() / 2;

	if (_firstInit)
	{
		// Set music
		if (Options::oxcePlayBriefingMusicDuringEquipment)
		{
			if (_save->getMusic() == "")
			{
				_game->getMod()->playMusic("GMTACTIC");
			}
			else
			{
				_game->getMod()->playMusic(_save->getMusic());
			}
		}

		_map->getCamera()->centerOnPosition(Position(midpointX, midpointY, 0), true);
		_map->refreshSelectorPosition();
		_map->setCursorType(CT_NORMAL);

		_editor->setSelectedObject(O_MAX);

		_firstInit = false;
	}
	_txtTooltip->setText("");

	//if (_autosave)
	//{
	//	_autosave = false;
	//	if (_game->getSavedGame()->isIronman())
	//	{
	//		_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_IRONMAN, _palette));
	//	}
	//	else if (Options::autosave)
	//	{
	//		_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_AUTO_BATTLESCAPE, _palette));
	//	}
	//}
}

/**
 * Runs the timers and handles popups.
 */
void MapEditorState::think()
{
	static bool popped = false;


	if (_gameTimer->isRunning())
	{
        State::think();
        _animTimer->think(this, 0);
        _gameTimer->think(this, 0);

		// Check to see if there's any other messages we need to read from the editor
		if (!_message->getVisible())
		{
			std::string message = _editor->getMessage();
			if (!message.empty())
			{
				_message->showMessage(tr(message));
			}
		}

		// switch right-click from performing an action to drag-selecting if we held it down long enough (TODO: and the option is turned on)
		if (!_isMouseScrolling && !_mouseScrollSelect && SDL_GetMouseState(0,0)&SDL_BUTTON(SDL_BUTTON_RIGHT)
			&& ((int)(SDL_GetTicks() - _mouseScrollingStartTime) > (Options::dragScrollTimeTolerance)))
		{
			// TODO: change painting to option for swapping between modes
			_mouseScrollSelect = _mouseScrollPainting = true;
			//handleSelections(action);
		}
	}

	if ((!_routeMode && _editor->getTileRegisterPosition() == 0 && _btnUndo->getColor() != 8) ||
		(_routeMode && _editor->getNodeRegisterPosition() == 0 && _btnUndo->getColor() != 8))
	{
		_btnUndo->offset(8 - 232, 0, 255, 1);
		_btnUndo->setColor(8); // change to disabled button color
	}
	else if ((!_routeMode && _editor->getTileRegisterPosition() > 0 && _btnUndo->getColor() != 232) ||
			(_routeMode && _editor->getNodeRegisterPosition() > 0 && _btnUndo->getColor() != 232))
	{
		_btnUndo->offset(232 - 8, 0, 255, 1);
		_btnUndo->setColor(232); // change to default color
	}

	if ((!_routeMode &&_editor->getTileRegisterPosition() == _editor->getTileRegisterSize() && _btnRedo->getColor() != 8) ||
		(_routeMode &&_editor->getNodeRegisterPosition() == _editor->getNodeRegisterSize() && _btnRedo->getColor() != 8))
	{
		_btnRedo->offset(8 - 232, 0, 255, 1);
		_btnRedo->setColor(8);
	}
	else if ((!_routeMode && _editor->getTileRegisterPosition() < _editor->getTileRegisterSize() && _btnRedo->getColor() != 232) ||
			(_routeMode && _editor->getNodeRegisterPosition() < _editor->getNodeRegisterSize() && _btnRedo->getColor() != 232))
	{
		_btnRedo->offset(232 - 8, 0, 255, 1);
		_btnRedo->setColor(232);
	}

	updateDebugText();
}

/**
 * Processes any mouse moving over the map.
 * @param action Pointer to an action.
 */
void MapEditorState::mapOver(Action *action)
{
	// handling moving the map by drag-scroll
	if (_isMouseScrolling && action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// (checking: is the dragScroll-mouse-button still pressed?)
		// However if the SDL is also missed the release event, then it is to no avail :(
		if ((SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{ // so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}
			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action);
			return;
		}

		_isMouseScrolled = true;

		if (Options::touchEnabled == false)
		{
			// Set the mouse cursor back
			SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
			SDL_WarpMouse(_game->getScreen()->getWidth() / 2, _game->getScreen()->getHeight() / 2 - _map->getIconHeight() / 2);
			SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
		}

		// Check the threshold
		_totalMouseMoveX += action->getDetails()->motion.xrel;
		_totalMouseMoveY += action->getDetails()->motion.yrel;
		if (!_mouseMovedOverThreshold)
		{
			_mouseMovedOverThreshold = ((std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance) || (std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance));
		}

		// Scrolling
		if (Options::battleDragScrollInvert)
		{
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			int scrollX = -(int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = -(int)((double)_totalMouseMoveY / action->getYScale());
			Position delta2 = _map->getCamera()->getMapOffset();
			_map->getCamera()->scrollXY(scrollX, scrollY, true);
			delta2 = _map->getCamera()->getMapOffset() - delta2;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = -(int) (delta2.x * action->getXScale());
				_totalMouseMoveY = -(int) (delta2.y * action->getYScale());
			}

			if (Options::touchEnabled == false)
			{
				action->getDetails()->motion.x = _xBeforeMouseScrolling;
				action->getDetails()->motion.y = _yBeforeMouseScrolling;
			}
			_map->setCursorType(CT_NONE);
		}
		else
		{
			Position delta = _map->getCamera()->getMapOffset();
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			int scrollX = (int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = (int)((double)_totalMouseMoveY / action->getYScale());
			Position delta2 = _map->getCamera()->getMapOffset();
			_map->getCamera()->scrollXY(scrollX, scrollY, true);
			delta2 = _map->getCamera()->getMapOffset() - delta2;
			delta = _map->getCamera()->getMapOffset() - delta;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = (int) (delta2.x * action->getXScale());
				_totalMouseMoveY = (int) (delta2.y * action->getYScale());
			}

			int barWidth = _game->getScreen()->getCursorLeftBlackBand();
			int barHeight = _game->getScreen()->getCursorTopBlackBand();
			int cursorX = _cursorPosition.x + Round(delta.x * action->getXScale());
			int cursorY = _cursorPosition.y + Round(delta.y * action->getYScale());
			_cursorPosition.x = Clamp(cursorX, barWidth, _game->getScreen()->getWidth() - barWidth - (int)(Round(action->getXScale())));
			_cursorPosition.y = Clamp(cursorY, barHeight, _game->getScreen()->getHeight() - barHeight - (int)(Round(action->getYScale())));

			if (Options::touchEnabled == false)
			{
				action->getDetails()->motion.x = _cursorPosition.x;
				action->getDetails()->motion.y = _cursorPosition.y;
			}
		}

		// We don't want to look the mouse-cursor jumping :)
		_game->getCursor()->handle(action);
	}
	// handling making selections of tiles/nodes by drag-scroll
	else if (_mouseScrollSelect && action->getDetails()->type == SDL_MOUSEMOTION)
	{
		_map->getSelectorPosition(&_scrollCurrentPosition);

		if (_scrollCurrentPosition != _scrollPreviousPosition)
		{
			handleSelections(action);
		}
	}
}

/**
 * Processes any presses on the map.
 * @param action Pointer to an action.
 */
void MapEditorState::mapPress(Action *action)
{
	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons) return;

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;
		SDL_GetMouseState(&_xBeforeMouseScrolling, &_yBeforeMouseScrolling);
		_mapOffsetBeforeMouseScrolling = _map->getCamera()->getMapOffset();

		if (!Options::battleDragScrollInvert && _cursorPosition.z == 0)
		{
			_cursorPosition.x = action->getDetails()->motion.x;
			_cursorPosition.y = action->getDetails()->motion.y;
			// the Z is irrelevant to our mouse position, but we can use it as a boolean to check if the position is set or not
			_cursorPosition.z = 1;
		}

		_totalMouseMoveX = 0; _totalMouseMoveY = 0;
		_mouseMovedOverThreshold = false;
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_mouseScrollSelect = true;
		//if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		//{
		//	_mouseScrollPainting = true;
		//}
	}

	_mouseScrollingStartTime = SDL_GetTicks();

	// set position on map where we started dragging the mouse
	// but don't reset it when we use the mousewheel to move up and down!
	if (action->getDetails()->button.button != SDL_BUTTON_WHEELUP && action->getDetails()->button.button != SDL_BUTTON_WHEELDOWN && !_isMouseScrolling)
	{
		_map->getSelectorPosition(&_scrollStartPosition);
		_scrollCurrentPosition = _scrollPreviousPosition = _scrollStartPosition;

		_proposedSelection.push_back(_scrollStartPosition);
		handleSelections(action);

		if (getRouteMode())
		{
			for (auto node : *_editor->getSelectedNodes())
			{
				_editor->getSelectedTiles()->push_back(_save->getTile(node->getPosition()));
			}
		}
	}
}

/**
 * Processes any clicks on the map to edit it
 * @param action Pointer to an action.
 */
void MapEditorState::mapClick(Action *action)
{
	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now an other button is used)
	if (_isMouseScrolling)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
		&& (SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{   // so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}
			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action);
		}
	}

	// DragScroll-Button release: release mouse-scroll-mode
	if (_isMouseScrolling)
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_isMouseScrolling = false;
			stopScrolling(action);
		}
		else
		{
			return;
		}
		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
		{
			_isMouseScrolled = false;
			stopScrolling(action);
		}
		if (_isMouseScrolled) return;
	}

	// Finish handling of selecting an area of the map
	if (_mouseScrollSelect)
	{
		// Just like for drag-scrolling, make sure we didn't miss the mouse-release event for drag-selections
		// Or we just need to stop the drag-select normally
		if ((action->getDetails()->button.button != SDL_BUTTON_LEFT && action->getDetails()->button.button != SDL_BUTTON_RIGHT
		&& (SDL_GetMouseState(0,0)&SDL_BUTTON(SDL_BUTTON_LEFT)) == 0
		&& (SDL_GetMouseState(0,0)&SDL_BUTTON(SDL_BUTTON_RIGHT)) == 0)
		|| action->getDetails()->button.button == SDL_BUTTON_LEFT // left-clicks are always handled like selections
		|| (action->getDetails()->button.button == SDL_BUTTON_RIGHT 
		&& ((int)(SDL_GetTicks() - _mouseScrollingStartTime) > (Options::dragScrollTimeTolerance))) // right-clicks need to be over a certain duration (TODO: and have the option turned on)
		)
		{
			stopSelections(action);
		}

		// return so other clicks don't do anything if we're holding another button during our drag-selection
		return;
	}

	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons) return;

	// don't accept leftclicks if there is no cursor or there is an action busy
	if (_map->getCursorType() == CT_NONE) return;

	Position pos;
	_map->getSelectorPosition(&pos);

	// Have the map editor capture the mouse input here for editing tiles
	if (_editor)
	{
		Tile *selectedTile = _save->getTile(pos);
		bool ctrlPressed = (SDL_GetModState() & KMOD_CTRL) != 0;
		bool shiftPressed = (SDL_GetModState() & KMOD_SHIFT) != 0;

		// Dealing with nodes
		if (getRouteMode())
		{
			Node *clickedNode = 0;

			for (auto node : *_save->getNodes())
			{
				if (!_editor->isNodeActive(node))
				{
					continue;
				}

				Position nodePos = node->getPosition();
				if (nodePos == pos)
				{
					clickedNode = node;
					break;
				}
			}

			// determine what action we're taking by what modes are selected
			if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
			{
				std::vector<int> data;
				
				// new node mode: create new node, move, or make links
				if (_nodeEditMode == _btnNodeNew)
				{
					// select node
					if (clickedNode && _nodeFilterMode == _btnNodeFilterSelect)
					{
						_editor->getSelectedNodes()->clear();
						_editor->getSelectedNodes()->push_back(clickedNode);
					}
					// creating new: no clicked node and not the move node filter
					else if (selectedTile && !clickedNode && _nodeFilterMode != _btnNodeFilterMove)
					{
						int segment = 0;
						int type = 0;
						int rank = 0;
						int flags = 0;
						int reserved = 0;
						int priority = 0;

						data.clear();
						data.push_back(pos.x);
						data.push_back(pos.y);
						data.push_back(pos.z);
						data.push_back(segment);
						data.push_back(type);
						data.push_back(rank);
						data.push_back(flags);
						data.push_back(reserved);
						data.push_back(priority);
						for (int i = 0; i < 5; ++i)
						{
							int linkID = -1;
							// make links back to the selected nodes if in two-way connection mode
							// we can only make 5 links, so we connect back to the first 5 selected nodes
							if (_nodeFilterMode == _btnNodeFilterTwoWayConnect && i < (int)_editor->getSelectedNodes()->size())
							{
								linkID = _editor->getSelectedNodes()->at(i)->getID();
							}
							data.push_back(linkID);
						}
						for (int i = 0; i < 5; ++i)
						{
							data.push_back(0);
						}

						_editor->changeNodeData(MET_DO, 0, NCT_NEW, data);

						// make links from the selected nodes to the new one if we're in one of the connection modes
						if (_nodeFilterMode != _btnNodeFilterSelect)
						{
							for (auto node : *_editor->getSelectedNodes())
							{
								data.clear();
								data.push_back(_editor->getNextNodeConnectionIndex(node, true));
								data.push_back(_save->getNodes()->size() - 1);
								_editor->changeNodeData(MET_DO, node, NCT_LINKS, data);
							}
						}

					}
					// moving: no clicked node and is the move node filter
					// we check for selectedTile to make sure we're moving inside the map
					// TODO: moving more than one node when drag-edits become a thing (?)
					else if (selectedTile && !clickedNode && selectedTile)
					{
						if (_editor->getSelectedNodes()->size() == 1)
						{
							data.clear();
							data.push_back(pos.x);
							data.push_back(pos.y);
							data.push_back(pos.z);
							_editor->changeNodeData(MET_DO, _editor->getSelectedNodes()->front(), NCT_POS, data);
						}
						// maybe handle multiple nodes selected by using a waypoint system:
						// 1st click sets reference point, 2nd click moves w.r.t. reference point
						else if (_editor->getSelectedNodes()->size() > 1)
						{
							if (_map->getWaypoints()->size() < 2)
							{
								_map->getWaypoints()->push_back(pos);
							}
							else
							{
								Position delta = _map->getWaypoints()->back() - _map->getWaypoints()->front();
								for (auto node : *_editor->getSelectedNodes())
								{
									// get the new position of the node as the difference between the two waypoints we placed added to its current position
									// but make sure it stays within the map!
									Position newPosition = node->getPosition() + delta;
									newPosition.x = std::max(0, std::min(_save->getMapSizeX() - 1, (int)newPosition.x));
									newPosition.y = std::max(0, std::min(_save->getMapSizeY() - 1, (int)newPosition.y));
									newPosition.z = std::max(0, std::min(_save->getMapSizeZ() - 1, (int)newPosition.z));

									// make sure this new position doesn't overlap with any other nodes we're moving
									bool overlap = false;
									for (auto otherNode : *_save->getNodes())
									{
										if (newPosition == otherNode->getPosition())
										{
											overlap = true;
											break;
										}
									}
									if (overlap)
									{
										continue;
									}

									data.clear();
									data.push_back(newPosition.x);
									data.push_back(newPosition.y);
									data.push_back(newPosition.z);
									_editor->changeNodeData(MET_DO, node, NCT_POS, data);
								}

								_map->getWaypoints()->clear();
							}
						}
					}
					// make links
					else if (_nodeFilterMode == _btnNodeFilterOneWayConnect || _nodeFilterMode == _btnNodeFilterTwoWayConnect)
					{
						int linkID = -1;
						// link to a node
						if (clickedNode)
						{
							linkID = clickedNode->getID();
						}
						// link to map exit
						else if (!selectedTile)
						{
							linkID = _editor->getExitLinkDirection(pos);
						}

						if (linkID != -1)
						{
							for (auto node : *_editor->getSelectedNodes())
							{
								// don't link back to the same node
								if (clickedNode == node)
								{
									continue;
								}

								// make the one-way links and links to the exit
								// don't make the link if it's already linked
								if (_editor->getConnectionIndex(node, linkID) == -1)
								{
									data.clear();
									data.push_back(_editor->getNextNodeConnectionIndex(node, true));
									data.push_back(linkID);
									_editor->changeNodeData(MET_DO, node, NCT_LINKS, data);
								}

								// make the second part of the two way links
								if (_nodeFilterMode == _btnNodeFilterTwoWayConnect &&
									clickedNode &&  _editor->getConnectionIndex(clickedNode, node->getID()) == -1)
								{
									data.clear();
									data.push_back(_editor->getNextNodeConnectionIndex(clickedNode, true));
									data.push_back(node->getID());
									_editor->changeNodeData(MET_DO, clickedNode, NCT_LINKS, data);
								}
							}
						}
					}
				}
				// delete node mode: remove node or connections
				else
				{
					// move or select mode: we delete nodes
					if (clickedNode && (_nodeFilterMode == _btnNodeFilterSelect || _nodeFilterMode == _btnNodeFilterMove))
					{
						data.clear();
						// check to see if we're clicking a selected node or a different one
						std::vector<Node*>::iterator it = std::find(_editor->getSelectedNodes()->begin(), _editor->getSelectedNodes()->end(), clickedNode);
						// one of the selected nodes: set all of them as inactive
						if (it != _editor->getSelectedNodes()->end())
						{
							for (auto node : *_editor->getSelectedNodes())
							{
								_editor->changeNodeData(MET_DO, node, NCT_DELETE, data);
							}

							_editor->getSelectedNodes()->clear();
						}
						// a different node: just set that one as inactive
						else
						{
							_editor->changeNodeData(MET_DO, clickedNode, NCT_DELETE, data);
						}
					}
					// remove links
					else if (_nodeFilterMode == _btnNodeFilterOneWayConnect || _nodeFilterMode == _btnNodeFilterTwoWayConnect)
					{
						int linkID = -1;
						// de-link from a node
						if (clickedNode)
						{
							linkID = clickedNode->getID();
						}
						// de-link from map exit
						else if (!selectedTile)
						{
							linkID = _editor->getExitLinkDirection(pos);
						}

						if (linkID != -1)
						{
							for (auto node : *_editor->getSelectedNodes())
							{
								// we don't need to de-link from the same node
								if (clickedNode == node)
								{
									continue;
								}

								// remove one-way links and those to map exits
								int linkIndex = _editor->getConnectionIndex(node, linkID);
								if (linkIndex != -1)
								{
									data.clear();
									data.push_back(linkIndex);
									data.push_back(-1);
									_editor->changeNodeData(MET_DO, node, NCT_LINKS, data);
								}

								// remove the second part of two-way links
								if (clickedNode)
								{
									linkIndex = _editor->getConnectionIndex(clickedNode, node->getID());
								}
								if (_nodeFilterMode == _btnNodeFilterTwoWayConnect && clickedNode && linkIndex != -1)
								{
									data.clear();
									data.push_back(linkIndex);
									data.push_back(-1);
									_editor->changeNodeData(MET_DO, clickedNode, NCT_LINKS, data);
								}
							}
						}

					}
				}

				_editor->confirmChanges(true);
			}

			updateNodePanels();
			updateDebugText();
		}
		// Editing tiles
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT && selectedTile)
		{
			bool singleTile = _editor->getSelectedTiles()->empty();
			if (singleTile)
			{
				_editor->getSelectedTiles()->push_back(selectedTile);
			}
			// if we didn't right-click inside our selection, do nothing
			else if (std::find_if(_editor->getSelectedTiles()->begin(), _editor->getSelectedTiles()->end(),
						[&](const Tile* tile){ return tile == selectedTile; }) == _editor->getSelectedTiles()->end())
			{
				return;
			}

			for (auto tile : *_editor->getSelectedTiles())
			{
				int dataIDs[O_MAX];
				int dataSetIDs[O_MAX];

				std::vector<TilePart> parts = {O_FLOOR, O_WESTWALL, O_NORTHWALL, O_OBJECT};
				for (auto part : parts)
				{
					int partIndex = (int)part;
					tile->getMapData(&dataIDs[partIndex], &dataSetIDs[partIndex], part);
				}

				// If the editor has a selected MapDataID, we're placing a tile
				if (_editor->getSelectedMapDataID() != -1)
				{
					int dataID;
					int dataSetID;
					MapData *mapData = _editor->getMapDataFromIndex(_editor->getSelectedMapDataID(), &dataSetID, &dataID);
					// If no tile filter is selected, we're checking the part of the MapDataID's object
					// Otherwise, take it from the filter
					int partIndex = _editor->getSelectedObject() == O_MAX ? (int)mapData->getObjectType() : (int)_editor->getSelectedObject();

					dataIDs[partIndex] = dataID;
					dataSetIDs[partIndex] = dataSetID;
				}
				// Otherwise we're clearing a tile
				else
				{
					// No tile filter selected means we're clearing the whole tile
					if (_editor->getSelectedObject() == O_MAX)
					{
						for (int i = 0; i < O_MAX; ++i)
						{
							dataIDs[i] = -1;
							dataSetIDs[i] = -1;
						}
					}
					// Selected tile filter means targeted removal of tile part
					else
					{
						dataIDs[(int)_editor->getSelectedObject()] = -1;
						dataSetIDs[(int)_editor->getSelectedObject()] = -1;
					}
				}

				_editor->changeTileData(MET_DO, tile, dataIDs, dataSetIDs);
			}

			_editor->confirmChanges(false);
			if (singleTile)
			{
				_editor->getSelectedTiles()->clear();
				_map->resetObstacles();
				_map->enableObstacles();
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			//if (!shiftPressed)
			//{
			//	_editor->getSelectedTiles()->clear();
			//	_map->resetObstacles();
			//	_map->enableObstacles();
			//}
			//
			//if (selectedTile)
			//{
			//	_editor->getSelectedTiles()->push_back(selectedTile);
			//	for (int i = 0; i < O_MAX; ++i)
			//	{
			//		selectedTile->setObstacle(i);
			//	}
			//}
		}
	}
}

/**
 * Handles mouse entering the map surface.
 * @param action Pointer to an action.
 */
void MapEditorState::mapIn(Action *)
{
	_isMouseScrolling = false;
	_map->setButtonsPressed(Options::battleDragScrollButton, false);
}

/**
 * Shows the next map layer.
 * @param action Pointer to an action.
 */
//void MapEditorState::btnMapUpClick(Action *)
//{
//	_map->getCamera()->up();
//}

/**
 * Shows the previous map layer.
 * @param action Pointer to an action.
 */
//void MapEditorState::btnMapDownClick(Action *)
//{
//	_map->getCamera()->down();
//}

/**
 * Shows/hides all map layers.
 * @param action Pointer to an action.
 */
//void MapEditorState::btnShowLayersClick(Action *)
//{
//	_numLayers->setValue(_map->getCamera()->toggleShowAllLayers());
//}

/**
 * Shows options.
 * @param action Pointer to an action.
 */
void MapEditorState::btnOptionsClick(Action *action)
{
	// Pressing escape closes the tile selection UI first
	if (_panelTileSelection->getVisible() && action->getDetails()->type == SDL_KEYDOWN)
	{
		tileSelectionClick(action);
		return;
	}
	else if (_panelRouteInformation->getVisible() && action->getDetails()->type == SDL_KEYDOWN)
	{
		toggleNodeInfoPanel(0, true);
		return;
	}

	_game->pushState(new MapEditorOptionsState(OPT_MAPEDITOR));
}

/**
 * Opens the jukebox.
 * @param action Pointer to an action.
 */
void MapEditorState::btnSelectMusicTrackClick(Action *)
{
	_game->pushState(new SelectMusicTrackState(SMT_BATTLESCAPE));
}

/**
 * Saves the edited map.
 * @param action Pointer to an action.
 */
void MapEditorState::btnSaveClick(Action *action)
{
	_editor->saveMapFile(_editor->getMapName());
}

/**
 * Brings up the menu to open a different map in the editor
 * @param action Pointer to an action.
 */
void MapEditorState::btnLoadClick(Action *action)
{
	_game->pushState(new MapEditorMenuState());
}

/**
 * Un-does the action on the top of the editor's register
 * @param action Pointer to an action.
 */
void MapEditorState::btnUndoClick(Action *action)
{
	_editor->undo(_routeMode);
	if (_txtTooltip->getVisible())
		txtTooltipIn(action);
	_map->draw(); // update map
	updateNodePanels(); // update node information
}

/**
 * Re-does the next action on the editor's register
 * @param action Pointer to an action.
 */
void MapEditorState::btnRedoClick(Action *action)
{
	_editor->redo(_routeMode);
	if (_txtTooltip->getVisible())
		txtTooltipIn(action);
	_map->draw(); // update map
	updateNodePanels(); // update node information
}

/**
 * Handles pressing the fill button
 * @param action Pointer to an action.
 */
void MapEditorState::btnFillClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	_editor->setSelectedMapDataID(_selectedTileIndex);
	_tileEditMode = _btnFill;
}

/**
 * Handles pressing the clear button
 * @param action Pointer to an action.
 */
void MapEditorState::btnClearClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	_editor->setSelectedMapDataID(-1);
	_tileEditMode = _btnClear;
}

/**
 * Handles pressing the tile filter buttons
 * @param action Pointer to an action.
 */
void MapEditorState::btnTileFilterClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	TilePart clickedFilter = O_MAX;

	if (action->getSender() == _btnTileFilterGround)
	{
		clickedFilter = O_FLOOR;
	}
	else if (action->getSender() == _btnTileFilterWestWall)
	{
		clickedFilter = O_WESTWALL;
	}
	else if (action->getSender() == _btnTileFilterNorthWall)
	{
		clickedFilter = O_NORTHWALL;
	}
	else if (action->getSender() == _btnTileFilterObject)
	{
		clickedFilter = O_OBJECT;
	}

	// Clicking on the currently selected filter: de-select the filter
	if (_editor->getSelectedObject() == clickedFilter)
	{
		_editor->setSelectedObject(O_MAX);

		_btnTileFilterGround->setGroup(0);
		_btnTileFilterWestWall->setGroup(0);
		_btnTileFilterNorthWall->setGroup(0);
		_btnTileFilterObject->setGroup(0);
		
		action->getDetails()->type = SDL_MOUSEBUTTONUP;
		_tileObjectSelected->mouseRelease(action, this);
		_tileObjectSelected->draw();
		_tileObjectSelected = 0;

		return;
	}

	_tileObjectSelected = _tileFilters[clickedFilter];
	// If no filter was previously selected, the filters need to be re-grouped
	if (_editor->getSelectedObject() == O_MAX)
	{
		_btnTileFilterGround->setGroup(&_tileObjectSelected);
		_btnTileFilterWestWall->setGroup(&_tileObjectSelected);
		_btnTileFilterNorthWall->setGroup(&_tileObjectSelected);
		_btnTileFilterObject->setGroup(&_tileObjectSelected);
	}

	_editor->setSelectedObject(clickedFilter);

	// consume the event to keep the button held down
	action->getDetails()->type = SDL_NOEVENT;
}

/**
 * Switches between tile and route mode
 * @param action Pointer to an action.
 */
void MapEditorState::btnModeSwitchClick(Action *action)
{
	toggleRouteMode(action);
}

/*
 * Handler for pressing the node new button.
 * @param action Pointer to an action.
 */
void MapEditorState::btnNodeNewClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	_nodeEditMode = _btnNodeNew;
}

/*
 * Handler for pressing the node delete button.
 * @param action Pointer to an action.
 */
void MapEditorState::btnNodeDeleteClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	_nodeEditMode = _btnNodeDelete;
}

/**
 * Handler for pressing the node filter buttons.
 * @param action Pointer to an action.
 */
void MapEditorState::btnNodeFilterClick(Action *action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0.0, 0.0, 0, 0);
	action->getSender()->mousePress(&a, this);

	InteractiveSurface *clickedFilter = action->getSender();

	if (clickedFilter == _btnNodeFilterSelect)
	{
		_nodeFilterMode = _btnNodeFilterSelect;
	}
	else if (clickedFilter == _btnNodeFilterMove)
	{
		_nodeFilterMode = _btnNodeFilterMove;
	}
	else if (clickedFilter == _btnNodeFilterOneWayConnect)
	{
		_nodeFilterMode = _btnNodeFilterOneWayConnect;
	}
	else if (clickedFilter == _btnNodeFilterTwoWayConnect)
	{
		_nodeFilterMode = _btnNodeFilterTwoWayConnect;
	}

	// clear any waypoints we made for moving muliple nodes
	_map->getWaypoints()->clear();

	// update the node links panel in case we selected a connection mode and need to highlight the next available link
	updateNodePanels();
}

/**
 * Handler for changing the node type combo box
 * @param action Pointer to an action.
 */
void MapEditorState::cbxNodeTypeChange(Action *action)
{
	if (_editor->getSelectedNodes()->empty())
	{
		return;
	}

	size_t selIdx = _cbxNodeType->getSelected();
	std::vector<int> data;
	data.push_back(_nodeTypes.at(selIdx));
	for (auto node : *_editor->getSelectedNodes())
	{
		_editor->changeNodeData(MET_DO, node, NCT_TYPE, data);
	}
	_editor->confirmChanges(true);
}

/**
 * Handler for changing the node rank combo box
 * @param action Pointer to an action.
 */
void MapEditorState::cbxNodeRankChange(Action *action)
{
	if (_editor->getSelectedNodes()->empty())
	{
		return;
	}
	
	size_t selIdx = _cbxNodeRank->getSelected();
	std::vector<int> data;
	data.push_back((int)selIdx);
	for (auto node : *_editor->getSelectedNodes())
	{
		_editor->changeNodeData(MET_DO, node, NCT_RANK, data);
	}
	_editor->confirmChanges(true);
}

/**
 * Handler for changing the node flag combo box
 * @param action Pointer to an action.
 */
void MapEditorState::cbxNodeFlagChange(Action *action)
{
	if (_editor->getSelectedNodes()->empty())
	{
		return;
	}
	
	size_t selIdx = _cbxNodeFlag->getSelected();
	std::vector<int> data;
	data.push_back((int)selIdx);
	for (auto node : *_editor->getSelectedNodes())
	{
		_editor->changeNodeData(MET_DO, node, NCT_FLAG, data);
	}
	_editor->confirmChanges(true);
}

/**
 * Handler for changing the node priority combo box
 * @param action Pointer to an action.
 */
void MapEditorState::cbxNodePriorityChange(Action *action)
{
	if (_editor->getSelectedNodes()->empty())
	{
		return;
	}
	
	size_t selIdx = _cbxNodePriority->getSelected();
	std::vector<int> data;
	data.push_back((int)selIdx);
	for (auto node : *_editor->getSelectedNodes())
	{
		_editor->changeNodeData(MET_DO, node, NCT_PRIORITY, data);
	}
	_editor->confirmChanges(true);
}

/**
 * Handler for changing the node reserved combo box
 * @param action Pointer to an action.
 */
void MapEditorState::cbxNodeReservedChange(Action *action)
{
	if (_editor->getSelectedNodes()->empty())
	{
		return;
	}
	
	size_t selIdx = _cbxNodeReserved->getSelected();
	std::vector<int> data;
	data.push_back((int)selIdx);
	for (auto node : *_editor->getSelectedNodes())
	{
		_editor->changeNodeData(MET_DO, node, NCT_RESERVED, data);
	}
	_editor->confirmChanges(true);
}

/**
 * Handler for changing the node links combo boxes
 * @param action Pointer to an action.
 */
void MapEditorState::cbxNodeLinksChange(Action *action)
{
	if (_editor->getSelectedNodes()->empty())
	{
		return;
	}
	
	int linkID;
	for (linkID = 0; linkID < 5; ++linkID)
	{
		if (action->getSender() == _cbxNodeLinks.at(linkID))
		{
			break;
		}
	}
	size_t selIdx = _cbxNodeLinks.at(linkID)->getSelected();

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

	std::vector<int> data;
	data.push_back(linkID);
	data.push_back(knownLinks.at(selIdx));
	for (auto node : *_editor->getSelectedNodes())
	{
		_editor->changeNodeData(MET_DO, node, NCT_LINKS, data);
	}
	_editor->confirmChanges(true);

	// update the node links panel in case we're in a connection mode and need to highlight the next available link
	updateNodePanels();
}

/**
 * Handler for changing the node link types combo boxes
 * @param action Pointer to an action.
 */
void MapEditorState::cbxNodeLinkTypesChange(Action *action)
{
	if (_editor->getSelectedNodes()->empty())
	{
		return;
	}
	
	int linkID;
	for (linkID = 0; linkID < 5; ++linkID)
	{
		if (action->getSender() == _cbxNodeLinkTypes.at(linkID))
		{
			break;
		}
	}
	size_t selIdx = _cbxNodeLinkTypes.at(linkID)->getSelected();

	std::vector<int> data;
	data.push_back(linkID);
	data.push_back(_nodeTypes.at(selIdx));
	for (auto node : *_editor->getSelectedNodes())
	{
		_editor->changeNodeData(MET_DO, node, NCT_LINKTYPES, data);
	}
	_editor->confirmChanges(true);
}

/**
 * Animates map objects on the map
 */
void MapEditorState::animate()
{
	_map->animate(true);
}

/**
 * Handles the battle game state.
 */
void MapEditorState::handleState()
{
    _map->invalidate();
	//_battleGame->handleState();
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval An interval in ms.
 */
void MapEditorState::setStateInterval(Uint32 interval)
{
	_gameTimer->setInterval(interval);
}

/**
 * Gets pointer to the game. Some states need this info.
 * @return Pointer to game.
 */
Game *MapEditorState::getGame() const
{
	return _game;
}

/**
 * Gets pointer to the map. Some states need this info.
 * @return Pointer to map.
 */
Map *MapEditorState::getMap() const
{
	return _map;
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
inline void MapEditorState::handle(Action *action)
{
	if (!_firstInit)
	{
		if (_game->getCursor()->getVisible() || ((action->getDetails()->type == SDL_MOUSEBUTTONDOWN || action->getDetails()->type == SDL_MOUSEBUTTONUP) && action->getDetails()->button.button == SDL_BUTTON_RIGHT))
		{
			State::handle(action);

			if (Options::touchEnabled == false && _isMouseScrolling && !Options::battleDragScrollInvert)
			{
				_map->setSelectorPosition((_cursorPosition.x - _game->getScreen()->getCursorLeftBlackBand()) / action->getXScale(), (_cursorPosition.y - _game->getScreen()->getCursorTopBlackBand()) / action->getYScale());
			}

			if (action->getDetails()->type == SDL_KEYDOWN)
			{
				SDLKey key = action->getDetails()->key.keysym.sym;
				bool ctrlPressed = (SDL_GetModState() & KMOD_CTRL) != 0;
				bool shiftPressed = (SDL_GetModState() & KMOD_SHIFT) != 0;

				if (key == SDLK_z && ctrlPressed) // change z to options
				{
					if (shiftPressed)
						btnRedoClick(action);
					else
						btnUndoClick(action);
				}
				else if (key == SDLK_s && ctrlPressed) // change s to options
				{
					if (shiftPressed || _editor->getMapName().size() == 0)
						_game->pushState(new MapEditorSaveAsState());
					else
						btnSaveClick(action);
				}
				else if (key == SDLK_o && ctrlPressed) // change o to options
				{
					btnLoadClick(action);
				}
				else if (key == SDLK_d && ctrlPressed) // change d to options
				{
					updateDebugText();
					_txtDebug->setVisible(!_txtDebug->getVisible());
				}
				else if (key == SDLK_r && ctrlPressed) // change r to options
				{
					toggleRouteMode(action);
				}
				else if (key == SDLK_i) // change i to options
				{
					_game->pushState(new MapEditorInfoState());
				}
				else if (key == Options::keySelectMusicTrack)
				{
					_game->pushState(new SelectMusicTrackState(SMT_BATTLESCAPE));
				}

				// quick save and quick load
				//if (!_game->getSavedGame()->isIronman())
				//{
				//	if (key == Options::keyQuickSave)
				//	{
				//		_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
				//	}
				//	else if (key == Options::keyQuickLoad)
				//	{
				//		_game->pushState(new LoadGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
				//	}
				//}
			}
		}
	}
}

/**
 * Updates the debug text
 */
void MapEditorState::updateDebugText()
{
	Position pos;
	_map->getSelectorPosition(&pos);

	if (getRouteMode())
	{
		std::string selectedNodeMode;
		if (_nodeEditMode == _btnNodeNew)
		{
			if (_nodeFilterMode == _btnNodeFilterSelect)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_SELECT");
			}
			else if (_nodeFilterMode == _btnNodeFilterMove)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_MOVE");
			}
			else if (_nodeFilterMode == _btnNodeFilterOneWayConnect)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_ONEWAYCONNECT");
			}
			else if (_nodeFilterMode == _btnNodeFilterTwoWayConnect)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_TWOWAYCONNECT");
			}
		}
		else if (_nodeEditMode == _btnNodeDelete)
		{
			if (_nodeFilterMode == _btnNodeFilterSelect)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_DELETE");
			}
			else if (_nodeFilterMode == _btnNodeFilterMove)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_DELETE");
			}
			else if (_nodeFilterMode == _btnNodeFilterOneWayConnect)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_ONEWAYDELETE");
			}
			else if (_nodeFilterMode == _btnNodeFilterTwoWayConnect)
			{
				selectedNodeMode = tr("STR_DEBUG_NODES_TWOWAYDELETE");
			}
		}

		int nodeID = -1;
		int numberOfActiveNodes = _editor->getNumberOfActiveNodes();
		std::string nodePositionText = tr("STR_DEBUG_NODES_SELECTED_NODES");
		std::string nodeIDText = tr("STR_DEBUG_NODES_SELECTED_ID");
		if (_editor->getSelectedNodes()->size() == 1)
		{
			nodeID = _editor->getSelectedNodes()->front()->getID();
			pos = _editor->getSelectedNodes()->front()->getPosition();
			nodePositionText = tr("STR_DEBUG_NODES_SELECTED_NODE");
		}
		else if (_editor->getSelectedNodes()->size() > 1)
		{
			nodeID = _editor->getSelectedNodes()->size();
			nodeIDText = tr("STR_DEBUG_NODES_NUMBER_SELECTED");
		}

		_txtDebug->setText(tr("STR_DEBUG_MAP_EDITOR_NODE").arg(selectedNodeMode).arg(nodePositionText).arg(pos).arg(nodeIDText).arg(nodeID).arg(numberOfActiveNodes));
		return;
	}

	std::string selectedMode;
	if (_editor->getSelectedMapDataID() == -1)
	{
		selectedMode = tr("STR_DEBUG_CLEAR");
	}
	else
	{
		selectedMode = tr("STR_DEBUG_PLACE");
	}

	std::string selectedObject;
	switch (_editor->getSelectedObject())
	{
		case O_FLOOR:
			selectedObject = tr("STR_DEBUG_O_FLOOR");
			break;
		case O_WESTWALL:
			selectedObject = tr("STR_DEBUG_O_WESTWALL");
			break;
		case O_NORTHWALL:
			selectedObject = tr("STR_DEBUG_O_NORTHWALL");
			break;
		case O_OBJECT:
			selectedObject = tr("STR_DEBUG_O_OBJECT");
			break;
		default:
			selectedObject = tr("STR_DEBUG_O_MAX");
			break;
	}

	//Tile *selectedTile = _save->getTile(pos);
	//if (!selectedTile)
	//{
	//	pos = Position(-1, -1, -1);
	//}

	_txtDebug->setText(tr("STR_DEBUG_MAP_EDITOR").arg(selectedMode).arg(selectedObject).arg(pos));
}

/**
 * Toggles route mode on and off, updating the UI.
 */
void MapEditorState::toggleRouteMode(Action *action)
{
	setRouteMode(!getRouteMode());

	if (_panelTileSelection->getVisible())
	{
		tileSelectionClick(action);
	}

	_tileSelection->setVisible(!getRouteMode());
	_backgroundTileSelection->setVisible(!getRouteMode());
	_iconsLowerRight->setVisible(!getRouteMode());
	_btnFill->setVisible(!getRouteMode());
	_btnClear->setVisible(!getRouteMode());
	_btnCut->setVisible(!getRouteMode());
	_btnCopy->setVisible(!getRouteMode());
	_btnPaste->setVisible(!getRouteMode());
	_iconsUpperRight->setVisible(!getRouteMode());
	_btnSelectedTile->setVisible(!getRouteMode());
	_btnTileFilterGround->setVisible(!getRouteMode());
	_btnTileFilterNorthWall->setVisible(!getRouteMode());
	_btnTileFilterWestWall->setVisible(!getRouteMode());
	_btnTileFilterObject->setVisible(!getRouteMode());
	_iconsLowerRightNodes->setVisible(getRouteMode());
	_btnNodeNew->setVisible(getRouteMode());
	_btnNodeDelete->setVisible(getRouteMode());
	// the held down node edit button will stay visible unless ungrouped
	_nodeEditMode->setGroup(getRouteMode() ? &_nodeEditMode : 0);
	if (!getRouteMode())
	{		
		action->getDetails()->type = SDL_MOUSEBUTTONUP;
		_nodeEditMode->mouseRelease(action, this);
		_nodeEditMode->draw();
	}
	_btnNodeCut->setVisible(getRouteMode());
	_btnNodeCopy->setVisible(getRouteMode());
	_btnNodePaste->setVisible(getRouteMode());
	_iconsUpperRightNodes->setVisible(getRouteMode());
	_btnSelectedNode->setVisible(getRouteMode());
	_btnNodeFilterSelect->setVisible(getRouteMode());
	_btnNodeFilterMove->setVisible(getRouteMode());
	_btnNodeFilterOneWayConnect->setVisible(getRouteMode());
	_btnNodeFilterTwoWayConnect->setVisible(getRouteMode());
	// the held down node filter button will stay visible unless ungrouped
	_nodeFilterMode->setGroup(getRouteMode() ? &_nodeFilterMode : 0);
	if (!getRouteMode())
	{		
		action->getDetails()->type = SDL_MOUSEBUTTONUP;
		_nodeFilterMode->mouseRelease(action, this);
		_nodeFilterMode->draw();
	}
	_iconsUpperLeftNodes->setVisible(getRouteMode());
	_btnRouteInformation->setVisible(getRouteMode());
	_btnRouteConnections->setVisible(getRouteMode());

	if (!getRouteMode())
	{
		toggleNodeInfoPanel(0, true);
	}

	// If we used the keyboard shortcut to call the toggle, then we need to update tooltips
	if (action->getDetails()->key.keysym.sym == SDLK_r) // change to options
	{
		if (_mouseOverIcons && _iconsMousedOver.back() != _iconsLowerLeft) // lower left icons don't switch
		{
			_iconsMousedOver.clear();
			// we're not going to guess which icons are still under the mouse, just hide the tile cursor
			_mouseOverIcons = true;
			// same with buttons for the tooltip
			_currentTooltip = "";
			_txtTooltip->setText(_currentTooltip);
		}
	}

	// clear any waypoints we made for moving multiple nodes
	_map->getWaypoints()->clear();
}

/**
 * Toggles the node information panel on and off
 * @param action Pointer to an action
 * @param hide Forces hiding the panel when switching route mode off
 */
void MapEditorState::toggleNodeInfoPanel(Action *action, bool hide)
{
	// determines if we open the info or links
	bool openInfo = true;

	// This can be called without an action, so we need to be careful with calling methods on the action
	if (action)
	{
		// determine if we are hiding, switching, or opening the node info panel
		// default is opening/switching to the info window
		// if the info button was clicked and the node type is visible, that means we're closing the window
		if (action->getSender() == _btnRouteInformation && _txtNodeType->getVisible())
		{
			hide = true;
		}
		// if the connection button was clicked, we're checking the case of needing to switch to it or close it
		else if (action->getSender() == _btnRouteConnections)
		{
			// node links is visible -> we're hiding the window
			if (_txtNodeLinks->getVisible())
			{
				hide = true;
			}
			// it's not visible -> open or switch to connections/links window
			else
			{
				openInfo = false;
			}
		}
	}

	_panelRouteInformation->setVisible(!hide);
	_txtNodeID->setVisible(!hide);
	_txtNodeType->setVisible(!hide && openInfo);
	_txtNodeRank->setVisible(!hide && openInfo);
	_txtNodeFlag->setVisible(!hide && openInfo);
	_txtNodePriority->setVisible(!hide && openInfo);
	_txtNodeReserved->setVisible(!hide && openInfo);
	_cbxNodeType->setVisible(!hide && openInfo);
	_cbxNodeRank->setVisible(!hide && openInfo);
	_cbxNodeFlag->setVisible(!hide && openInfo);
	_cbxNodePriority->setVisible(!hide && openInfo);
	_cbxNodeReserved->setVisible(!hide && openInfo);

	if (_panelRouteInformation->getVisible())
	{
		_txtTooltip->setX(_panelRouteInformation->getWidth() + 2);
		_txtTooltip->setWidth(150);
	}
	else
	{
		_txtTooltip->setX(2);
		_txtTooltip->setWidth(300);
	}

	_txtNodeLinks->setVisible(!hide && !openInfo);
	for (auto i : _cbxNodeLinks)
	{
		i->setVisible(!hide && !openInfo);
	}
	for (auto i : _cbxNodeLinkTypes)
	{
		i->setVisible(!hide && !openInfo);
	}

	// TODO: remove magic numbers
	int cbx0 = (!hide && openInfo) ? 8 : 8 - 160;
	int cbx1 = (!hide && openInfo) ? 124 : 124 - 160;
	int cbx2 = (!hide && !openInfo) ? 8 : 8 - 160;
	int cbx3 = (!hide && !openInfo) ? 84 : 84 - 160;

	_cbxNodeType->setX(cbx0);
	_cbxNodeRank->setX(cbx0);
	_cbxNodeFlag->setX(cbx1);
	_cbxNodePriority->setX(cbx1);
	_cbxNodeReserved->setX(cbx1);

	for (auto i : _cbxNodeLinks)
	{
		i->setX(cbx2);
	}
	for (auto i : _cbxNodeLinkTypes)
	{
		i->setX(cbx3);
	}
}

/**
 * Sets the route mode either on or off.
 */
void MapEditorState::setRouteMode(bool routeMode)
{
	_routeMode = routeMode;
}

/**
 * Gets whether route mode is on or off.
 */
bool MapEditorState::getRouteMode()
{
	return _routeMode;
}

/**
 * Updates the info/connection panels for the selected node/nodes.
 * Used the selected nodes from the editor
 */
void MapEditorState::updateNodePanels()
{
	std::vector<std::string> linkChoices;
	linkChoices.clear();
	std::string emptyString = "--";

	// no selected nodes: clear the info/connections panels
	if (_editor->getSelectedNodes()->empty())
	{
		_txtNodeID->setText(tr("STR_NODE_ID").arg(emptyString).arg(Position(-1, -1, -1)));
		linkChoices.push_back(emptyString);

		_cbxNodeType->setOptions(linkChoices, false);
		_cbxNodeType->setSelected(0);
		_cbxNodeRank->setOptions(linkChoices, false);
		_cbxNodeRank->setSelected(0);
		_cbxNodeFlag->setOptions(linkChoices, false);
		_cbxNodeFlag->setSelected(0);
		_cbxNodePriority->setOptions(linkChoices, false);
		_cbxNodePriority->setSelected(0);
		_cbxNodeReserved->setOptions(linkChoices, false);
		_cbxNodeReserved->setSelected(0);
		for (auto i : _cbxNodeLinks)
		{
			i->setColor(_game->getMod()->getInterface("battlescape")->getElement("infoBoxOKButton")->color);
			i->setOptions(linkChoices, false);
			i->setSelected(0);
		}
		for (auto i : _cbxNodeLinkTypes)
		{
			i->setColor(_game->getMod()->getInterface("battlescape")->getElement("infoBoxOKButton")->color);
			i->setOptions(linkChoices, false);
			i->setSelected(0);
		}

		return;
	}

	// populate the information and combo boxes
	if (_editor->getSelectedNodes()->size() == 1)
	{
		_txtNodeID->setText(tr("STR_NODE_ID").arg(_editor->getSelectedNodes()->front()->getID()).arg(_editor->getSelectedNodes()->front()->getPosition()));
	}
	else
	{
		_txtNodeID->setText(tr("STR_NODE_ID").arg(emptyString).arg(Position(-1, -1, -1)));			
	}

	_cbxNodeType->setOptions(_nodeTypeStrings, true);
	_cbxNodeRank->setOptions(_nodeRankStrings, true);

	std::vector<std::string> numberStrings;
	numberStrings.clear();
	for (int i = 0; i < 10; ++i)
	{
		numberStrings.push_back(std::to_string(i));
	}
	_cbxNodeFlag->setOptions(numberStrings, false);
	_cbxNodePriority->setOptions(numberStrings, false);
	_cbxNodeReserved->setOptions(numberStrings, false);

	linkChoices.push_back("STR_LINK_UNUSED");
	linkChoices.push_back("STR_LINK_NORTH");
	linkChoices.push_back("STR_LINK_EAST");
	linkChoices.push_back("STR_LINK_SOUTH");
	linkChoices.push_back("STR_LINK_WEST");
	for (auto node : *_save->getNodes())
	{
		linkChoices.push_back(std::to_string(node->getID()));
	}
	for (int i = 0; i < 5; ++i)
	{
		_cbxNodeLinks.at(i)->setOptions(linkChoices, true);
		_cbxNodeLinkTypes.at(i)->setOptions(_nodeTypeStrings, true);		
	}


	// set the displayed value for the combo boxes
	// if there's more than one node, we'll only display the values for those that have all the same value
	// start by setting the values according to the first node selected, then change the display if the following nodes differ
	_cbxNodeType->setSelected(_editor->getSelectedNodes()->front()->getType());
	_cbxNodeRank->setSelected(_editor->getSelectedNodes()->front()->getRank());
	_cbxNodeFlag->setSelected(_editor->getSelectedNodes()->front()->getFlags());
	_cbxNodePriority->setSelected(_editor->getSelectedNodes()->front()->getPriority());
	_cbxNodeReserved->setSelected(_editor->getSelectedNodes()->front()->isTarget() ? 5 : 0);
	for (int i = 0; i < 5; ++i)
	{
		int linkID = _editor->getSelectedNodes()->front()->getNodeLinks()->at(i);
		// make sure the linked node isn't marked as inactive
		if (linkID >= 0 && linkID < (int)_save->getNodes()->size() && !_editor->isNodeActive(_save->getNodes()->at(linkID)))
		{
			linkID = -1;
		}
		// -1 = unused, -2 = north, -3 = east, -4 = south, -5 = west (from BattlescapeGenerator::loadRMP)
		linkID = linkID < 0 ? -linkID - 1 : linkID + 5;
		_cbxNodeLinks.at(i)->setSelected(linkID);

		int linkType = linkID == -1 ? 0 : _editor->getSelectedNodes()->front()->getLinkTypes()->at(i);
		_cbxNodeLinkTypes.at(i)->setSelected(linkType);
	}

	// if there's just one node, we can stop here
	if (_editor->getSelectedNodes()->size() == 1)
	{
		// if we're in one of the node linking modes, grey out all but the link that will be editied next when making a connection
		Uint8 colorDisabled = 8; // TODO move to elements ruleset
		Uint8 colorHighlighted = _game->getMod()->getInterface("battlescape")->getElement("infoBoxOKButton")->color;
		for (int i = 0; i < 5; ++i)
		{
			if ((_nodeFilterMode == _btnNodeFilterOneWayConnect || _nodeFilterMode == _btnNodeFilterTwoWayConnect) &&
				i != _editor->getNextNodeConnectionIndex(_editor->getSelectedNodes()->front()))
			{
				_cbxNodeLinks.at(i)->setColor(colorDisabled);
				_cbxNodeLinkTypes.at(i)->setColor(colorDisabled);
			}
			else
			{
				_cbxNodeLinks.at(i)->setColor(colorHighlighted);
				_cbxNodeLinkTypes.at(i)->setColor(colorHighlighted);
			}
		}
		return;
	}
	else
	{
		Uint8 colorHighlighted = _game->getMod()->getInterface("battlescape")->getElement("infoBoxOKButton")->color;
		for (int i = 0; i < 5; ++i)
		{
			_cbxNodeLinks.at(i)->setColor(colorHighlighted);
			_cbxNodeLinkTypes.at(i)->setColor(colorHighlighted);
		}
	}

	// check through selected nodes and see if we need to mark out fields that have different values across nodes
	for (auto node : *_editor->getSelectedNodes())
	{
		if (_cbxNodeType->getSelected() != (size_t)node->getType())
		{
			_cbxNodeType->setText(emptyString);
		}

		if (_cbxNodeRank->getSelected() != (size_t)node->getRank())
		{
			_cbxNodeType->setText(emptyString);
		}

		if (_cbxNodeFlag->getSelected() != (size_t)node->getFlags())
		{
			_cbxNodeFlag->setText(emptyString);
		}

		if (_cbxNodePriority->getSelected() != (size_t)node->getPriority())
		{
			_cbxNodePriority->setText(emptyString);
		}

		if (_cbxNodeReserved->getSelected() != (node->isTarget() ? 5 : 0))
		{
			_cbxNodeReserved->setText(emptyString);
		}

		for (int i = 0; i < 5; ++i)
		{
			int linkID = node->getNodeLinks()->at(i);
			linkID = linkID < 0 ? -linkID - 1 : linkID + 5;
			if (_cbxNodeLinks.at(i)->getSelected() != (size_t)linkID)
			{
				_cbxNodeLinks.at(i)->setText(emptyString);
			}

			if (_cbxNodeLinkTypes.at(i)->getSelected() != (size_t)node->getLinkTypes()->at(i))
			{
				_cbxNodeLinkTypes.at(i)->setText(emptyString);
			}
		}
	}
}

/**
 * Clears mouse-scrolling state (isMouseScrolling).
 */
void MapEditorState::clearMouseScrollingState()
{
	_isMouseScrolling = false;
}

/**
 * Gets whether or not we're mouse-scrolling to select something
 */
bool MapEditorState::isMouseScrollSelecting()
{
	return _mouseScrollSelect;
}

/**
 * Gets whether or not the mouse-scroll to select is in box or painting mode
 */
bool MapEditorState::isMouseScrollSelectionPainting()
{
	return _mouseScrollPainting; // change to options stuff later
}

/**
 * Gets where the mouse started scrolling for a selection
 */
Position MapEditorState::getScrollStartPosition()
{
	return _scrollStartPosition;
}

/**
 * Handler for the mouse moving over the icons, disabling the tile selection cube.
 * @param action Pointer to an action.
 */
void MapEditorState::mouseInIcons(Action *action)
{
	InteractiveSurface *sender = action->getSender();
	if (sender && std::find_if(_iconsMousedOver.begin(), _iconsMousedOver.end(),
		[&](const InteractiveSurface* s){ return s == sender; }) == _iconsMousedOver.end())
	{
		_iconsMousedOver.push_back(sender);
	}

	_mouseOverIcons = _iconsMousedOver.size() != 0;
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 * @param action Pointer to an action.
 */
void MapEditorState::mouseOutIcons(Action *action)
{
	InteractiveSurface *sender = action->getSender();
	if (sender)
	{
		std::vector<InteractiveSurface*>::iterator it = std::find_if(_iconsMousedOver.begin(), _iconsMousedOver.end(),
		[&](const InteractiveSurface* s){ return s == sender; });

		if (it != _iconsMousedOver.end())
		{
			_iconsMousedOver.erase(it);
		}
	}

	_mouseOverIcons = _iconsMousedOver.size() != 0;
}

/**
 * Checks if the mouse is over the icons.
 * @return True, if the mouse is over the icons.
 */
bool MapEditorState::getMouseOverIcons() const
{
	return _mouseOverIcons;
}

/**
* Shows a tooltip for the appropriate button.
* @param action Pointer to an action.
*/
void MapEditorState::txtTooltipIn(Action *action)
{
	if (Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		if (action->getSender() == _btnUndo || action->getSender() == _btnRedo)
		{
			std::ostringstream ss;
			if (!_routeMode)
			{
				ss << tr(_currentTooltip).arg(std::to_string(_editor->getTileRegisterPosition())).arg(std::to_string(_editor->getTileRegisterSize()));
			}
			else
			{
				ss << tr(_currentTooltip).arg(std::to_string(_editor->getNodeRegisterPosition())).arg(std::to_string(_editor->getNodeRegisterSize()));
			}
			_txtTooltip->setText(ss.str());
		}
		else
		{
			_txtTooltip->setText(tr(_currentTooltip));
		}
	}
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void MapEditorState::txtTooltipOut(Action *action)
{
	// reset color
	_txtTooltip->setColor(_tooltipDefaultColor);

	if (Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_txtTooltip->setText("");
		}
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void MapEditorState::resize(int &dX, int &dY)
{
	dX = Options::baseXResolution;
	dY = Options::baseYResolution;
	int divisor = 1;
	double pixelRatioY = 1.0;

	if (Options::nonSquarePixelRatio)
	{
		pixelRatioY = 1.2;
	}
	switch (Options::battlescapeScale)
	{
	case SCALE_SCREEN_DIV_6:
		divisor = 6;
		break;
	case SCALE_SCREEN_DIV_5:
		divisor = 5;
		break;
	case SCALE_SCREEN_DIV_4:
		divisor = 4;
		break;
	case SCALE_SCREEN_DIV_3:
		divisor = 3;
		break;
	case SCALE_SCREEN_DIV_2:
		divisor = 2;
		break;
	case SCALE_SCREEN:
		break;
	default:
		dX = 0;
		dY = 0;
		return;
	}

	Options::baseXResolution = std::max(Screen::ORIGINAL_WIDTH, Options::displayWidth / divisor);
	Options::baseYResolution = std::max(Screen::ORIGINAL_HEIGHT, (int)(Options::displayHeight / pixelRatioY / divisor));

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;
	_map->setWidth(Options::baseXResolution);
	_map->setHeight(Options::baseYResolution);
	_map->getCamera()->resize();
	_map->getCamera()->jumpXY(dX/2, dY/2);

	for (std::vector<Surface*>::const_iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		if (*i != _map)
		{
			(*i)->setX((*i)->getX() + dX / 2);
			(*i)->setY((*i)->getY() + dY);
		}
		else if (*i != _map)
		{
			(*i)->setX((*i)->getX() + dX);
		}
	}

}

/**
 * Move the mouse back to where it started after we finish drag scrolling.
 * Or handle any after-scrolling actions for selecting/editing multiple tiles.
 * @param action Pointer to an action.
 */
void MapEditorState::stopScrolling(Action *action)
{
	if (Options::battleDragScrollInvert)
	{
		SDL_WarpMouse(_xBeforeMouseScrolling, _yBeforeMouseScrolling);
		action->setMouseAction(_xBeforeMouseScrolling, _yBeforeMouseScrolling, _map->getX(), _map->getY());
		getMap()->setCursorType(CT_NORMAL);
	}
	else
	{
		SDL_WarpMouse(_cursorPosition.x, _cursorPosition.y);
		action->setMouseAction(_cursorPosition.x/action->getXScale(), _cursorPosition.y/action->getYScale(), 0, 0);
		_map->setSelectorPosition(_cursorPosition.x / action->getXScale(), _cursorPosition.y / action->getYScale());
	}
	// reset our "mouse position stored" flag
	_cursorPosition.z = 0;
}

/**
 * Handle making selections of multiple tiles or nodes
 * @param action Pointer to an action.
 */
void MapEditorState::handleSelections(Action *action)
{
	// don't start handling selections for right clicks until after it's been held long enough
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
	{
		return;
	}

	bool ctrlPressed = (SDL_GetModState() & KMOD_CTRL) != 0;
	bool shiftPressed = (SDL_GetModState() & KMOD_SHIFT) != 0;

	if (!_mouseScrollPainting)
	{
		_proposedSelection.clear();
		// loop over the positions in the rectangular box between where we started scrolling and where the cursor is right now
		for (int z = std::min(_scrollStartPosition.z, _scrollCurrentPosition.z); z <= std::max(_scrollStartPosition.z, _scrollCurrentPosition.z); ++z)
		{
			for (int y = std::min(_scrollStartPosition.y, _scrollCurrentPosition.y); y <= std::max(_scrollStartPosition.y, _scrollCurrentPosition.y); ++y)
			{
				for (int x = std::min(_scrollStartPosition.x, _scrollCurrentPosition.x); x <= std::max(_scrollStartPosition.x, _scrollCurrentPosition.x); ++x)
				{
					_proposedSelection.push_back(Position(x, y, z));
				}
			}
		}
	}
	// checking just the position under the cursor
	else
	{
		if (std::find_if(_proposedSelection.begin(), _proposedSelection.end(),
						[&](const Position p){ return p == _scrollCurrentPosition; }) == _proposedSelection.end())
		{
			_proposedSelection.push_back(_scrollCurrentPosition);
		}
	}

	// Determine which tiles/nodes we need to highlight for previewing the selection
	// holding SHIFT means tiles/nodes get highlighted no matter what
	// holding only CTRL means they should *not* be highlighted if they're in the box
	// holding both means the tile/node needs to both be within our rectangular box and the already-selected area
	// holding neither means we only highlight in the box
	_map->resetObstacles();
	_map->enableObstacles();

	if (shiftPressed || !ctrlPressed)
	{
		// loop over the positions in our proposed selection
		for (auto pos : _proposedSelection)
		{
			// holding CTRL means we're either removing from selection or getting the intersection of two selections
			bool highlight = !ctrlPressed;
			Tile *selectedTile = _save->getTile(pos);
			if (selectedTile)
			{
				// check if we're intersecting with some already-selected tiles
				// highlight if CTRL isn't held or the tile isn't in the already-selected set
				highlight = highlight || std::find_if(_editor->getSelectedTiles()->begin(), _editor->getSelectedTiles()->end(),
											[&](const Tile* tile){ return tile == selectedTile; }) != _editor->getSelectedTiles()->end();
				if (highlight)
				{
					for (int i = 0; i < O_MAX; ++i)
					{
						selectedTile->setObstacle(i);
					}
				}
			}
		}
	}

	if (shiftPressed != ctrlPressed)
	{
		// loop over the tiles we have already selected
		for (auto tile : *_editor->getSelectedTiles())
		{
			Position pos = tile->getPosition();
			// highlight if we're holding SHIFT or the tile is outside the proposed selection
			bool highlight = shiftPressed || std::find_if(_proposedSelection.begin(), _proposedSelection.end(),
												[&](const Position p){ return p == pos; }) == _proposedSelection.end();

			if (highlight)
			{
				for (int i = 0; i < O_MAX; ++i)
				{
					tile->setObstacle(i);
				}
			}
		}
	}
	// TODO: replace looping over all tile parts for filtered tile part

	_scrollPreviousPosition = _scrollCurrentPosition;
}

/// Handle finishing making mouse-drag selections or actions
void MapEditorState::stopSelections(Action *action)
{
	handleSelections(action);

	// Anything in our previewed highlight gets put in the editor's selected tiles
	_editor->getSelectedTiles()->clear();
	if (getRouteMode())
	{
		_editor->getSelectedNodes()->clear();
	}

	for (int z = 0; z < _save->getMapSizeZ(); z++)
	{
		for (int y = 0; y < _save->getMapSizeY(); y++)
		{
			for (int x = 0; x < _save->getMapSizeX(); x++)
			{
				Tile *tile = _save->getTile(Position(x, y, z));
				if (tile->isObstacle())
				{
					_editor->getSelectedTiles()->push_back(tile);

					if (getRouteMode())
					{
						Node *selectedNode = 0;

						for (auto node : *_save->getNodes())
						{
							if (!_editor->isNodeActive(node))
							{
								continue;
							}

							Position nodePos = node->getPosition();
							if (nodePos == Position(x, y, z))
							{
								selectedNode = node;
								break;
							}
						}

						if (selectedNode)
						{
							_editor->getSelectedNodes()->push_back(selectedNode);
						}
					}
				}
			}
		}
	}

	if (getRouteMode())
	{
		_editor->getSelectedTiles()->clear();
		_map->resetObstacles();
		_map->enableObstacles();
		updateNodePanels();

		if (_editor->getSelectedNodes()->size() < 2)
		{
			_map->getWaypoints()->clear();
		}
	}

	_proposedSelection.clear();
	_mouseScrollSelect = _mouseScrollPainting = false;
	_scrollStartPosition = _scrollPreviousPosition = _scrollCurrentPosition = Position(-1, -1, -1);
}

/**
 * Autosave the game the next time the battlescape is displayed.
 */
//void MapEditorState::autosave()
//{
//	_autosave = true;
//}

/**
 * Gets the pointer to the Map Editor
 */
MapEditor *MapEditorState::getMapEditor()
{
	return _editor;
}

/**
 * Toggles the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionClick(Action *action)
{
	_backgroundTileSelectionNavigation->setVisible(!_backgroundTileSelectionNavigation->getVisible());
	_txtSelectionPageCount->setVisible(!_txtSelectionPageCount->getVisible());
	_tileSelectionPageCount->setVisible(!_tileSelectionPageCount->getVisible());
	_tileSelectionLeftArrow->setVisible(!_tileSelectionLeftArrow->getVisible());
	_tileSelectionRightArrow->setVisible(!_tileSelectionRightArrow->getVisible());
	for (auto i : _tileSelectionGrid)
	{
		i->setVisible(!_panelTileSelection->getVisible());
	}
	_panelTileSelection->setVisible(!_panelTileSelection->getVisible());

	if (_panelTileSelection->getVisible())
	{
		_txtTooltip->setX(_panelTileSelection->getWidth() + 2);
		_txtTooltip->setWidth(150);
	}
	else
	{
		_txtTooltip->setX(2);
		_txtTooltip->setWidth(300);
	}

	drawTileSpriteOnSurface(_tileSelection, _selectedTileIndex);

	// Make sure the hidden surfaces are no longer stored as "moused over"
	InteractiveSurface *surface = _backgroundTileSelectionNavigation;
	action->setSender(surface);
	mouseOutIcons(action);
	surface = _panelTileSelection;
	action->setSender(surface);
	mouseOutIcons(action);
}

/**
 * Draws the tile images on the selection grid
 */
void MapEditorState::drawTileSelectionGrid()
{
	for (int i = 0; i < (int)_tileSelectionGrid.size(); ++i)
	{
		_tileSelectionGrid.at(i)->draw();
		drawTileSpriteOnSurface(_tileSelectionGrid.at(i), i + _tileSelectionCurrentPage * _tileSelectionRows * _tileSelectionColumns);
	}
}

/**
 * Moves the tile selection UI left one page
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionLeftArrowClick(Action *action)
{
	if (_tileSelectionCurrentPage == 0)
		return;

	--_tileSelectionCurrentPage;
	drawTileSelectionGrid();
	std::ostringstream ss;
	ss << _tileSelectionCurrentPage + 1 << "/" << _tileSelectionLastPage + 1;
	_txtSelectionPageCount->setText(ss.str().c_str());
}

/**
 * Moves the tile selection UI right one page
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionRightArrowClick(Action *action)
{
	if (_tileSelectionCurrentPage == _tileSelectionLastPage)
		return;

	++_tileSelectionCurrentPage;
	drawTileSelectionGrid();
	std::ostringstream ss;
	ss << _tileSelectionCurrentPage + 1 << "/" << _tileSelectionLastPage + 1;
	_txtSelectionPageCount->setText(ss.str().c_str());
}

/**
 * Selects the tile from the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionGridClick(Action *action)
{
	int mouseX = (int)action->getAbsoluteXMouse();
	int mouseY = (int)action->getAbsoluteYMouse();

	int index = _tileSelectionColumns * ((mouseY - 40) / 40) + (mouseX / 32);
	index += _tileSelectionCurrentPage * _tileSelectionRows * _tileSelectionColumns;
	if (drawTileSpriteOnSurface(_tileSelection, index))
	{
		_selectedTileIndex = index;
		if (_editor->getSelectedMapDataID() != -1)
			_editor->setSelectedMapDataID(index);
	}
}

/**
 * Handles mouse wheel scrolling of the tile selection UI
 * @param action Pointer to an action.
 */
void MapEditorState::tileSelectionMousePress(Action *action)
{
	if (Options::battleDragScrollButton != SDL_BUTTON_MIDDLE || (SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		{
			tileSelectionLeftArrowClick(action);
			// Consume the event so the map doesn't scroll up or down
			action->getDetails()->type = SDL_NOEVENT;
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		{
			tileSelectionRightArrowClick(action);
			// Consume the event so the map doesn't scroll up or down
			action->getDetails()->type = SDL_NOEVENT;
		}
	}
}

/**
 * Draws a tile sprite on a given surface
 * @param surface Pointer to the surface.
 * @param index Index of the tile object.
 * @return Whether or not we found and drew the proper tile object.
 */
bool MapEditorState::drawTileSpriteOnSurface(Surface *surface, int index)
{
	int mapDataSetID = 0;
	int mapDataID = 0;
	MapData *mapData = _editor->getMapDataFromIndex(index, &mapDataSetID, &mapDataID);

	if (mapData)
	{
		surface->draw();
		_save->getMapDataSets()->at(mapDataSetID)->getSurfaceset()->getFrame(mapData->getSprite(0))->blitNShade(surface, 0, 0);
		return true;
	}

	return false;
}

}