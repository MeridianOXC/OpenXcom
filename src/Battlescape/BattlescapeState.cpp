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
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <SDL_gfxPrimitives.h>
#include "Map.h"
#include "Camera.h"
#include "BattlescapeState.h"
#include "AbortMissionState.h"
#include "TileEngine.h"
#include "ActionMenuState.h"
#include "SkillMenuState.h"
#include "UnitInfoState.h"
#include "InventoryState.h"
#include "AlienInventoryState.h"
#include "Pathfinding.h"
#include "BattlescapeGame.h"
#include "WarningMessage.h"
#include "InfoboxState.h"
#include "TurnDiaryState.h"
#include "DebriefingState.h"
#include "MiniMapState.h"
#include "BattlescapeGenerator.h"
#include "BriefingState.h"
#include "ExtendedBattlescapeLinksState.h"
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
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/Bar.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/NumberText.h"
#include "../Menu/CutsceneState.h"
#include "../Menu/PauseState.h"
#include "../Menu/LoadGameState.h"
#include "../Menu/SaveGameState.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleUfo.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/HitLog.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Savegame/Ufo.h"
#include "../Mod/RuleEnviroEffects.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleVideo.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape screen.
 * @param game Pointer to the core game.
 */
BattlescapeState::BattlescapeState() :
	_reserve(0), _touchButtonsEnabled(false), _touchButtonsEnabledLastTurn(false), _manaBarVisible(false),
	_firstInit(true), _paletteResetNeeded(false), _paletteResetRequested(false),
	_isMouseScrolling(false), _isMouseScrolled(false),
	_xBeforeMouseScrolling(0), _yBeforeMouseScrolling(0),
	_totalMouseMoveX(0), _totalMouseMoveY(0), _mouseMovedOverThreshold(0), _mouseOverIcons(false),
	_autosave(0),
	_numberOfDirectlyVisibleUnits(0), _numberOfEnemiesTotal(0), _numberOfEnemiesTotalPlusWounded(0)
{
	std::fill_n(_visibleUnit, 10, (BattleUnit*)(0));

	const int screenWidth = Options::baseXResolution;
	const int screenHeight = Options::baseYResolution;
	const int iconsWidth = _game->getMod()->getInterface("battlescape")->getElement("icons")->w;
	const int iconsHeight = _game->getMod()->getInterface("battlescape")->getElement("icons")->h;
	const int visibleMapHeight = screenHeight - iconsHeight;
	const int x = screenWidth/2 - iconsWidth/2;
	const int y = screenHeight - iconsHeight;

	_indicatorTextColor = _game->getMod()->getInterface("battlescape")->getElement("visibleUnits")->color;
	_indicatorGreen = _game->getMod()->getInterface("battlescape")->getElement("squadsightUnits")->color;
	_indicatorBlue = _game->getMod()->getInterface("battlescape")->getElement("woundedUnits")->color;
	_indicatorPurple = _game->getMod()->getInterface("battlescape")->getElement("passingOutUnits")->color;

	_twoHandedRed = _game->getMod()->getInterface("battlescape")->getElement("twoHandedRed")->color;
	_twoHandedGreen = _game->getMod()->getInterface("battlescape")->getElement("twoHandedGreen")->color;

	_tooltipDefaultColor = _game->getMod()->getInterface("battlescape")->getElement("textTooltip")->color;

	_medikitRed = _game->getMod()->getInterface("battlescape")->getElement("medikitRed")->color;
	_medikitGreen = _game->getMod()->getInterface("battlescape")->getElement("medikitGreen")->color;
	_medikitBlue = _game->getMod()->getInterface("battlescape")->getElement("medikitBlue")->color;
	_medikitOrange = _game->getMod()->getInterface("battlescape")->getElement("medikitOrange")->color;

	// Create buttonbar - this should be on the centerbottom of the screen
	_icons = new InteractiveSurface(iconsWidth, iconsHeight, x, y);

	// Create the battlemap view
	// the actual map height is the total height minus the height of the buttonbar
	_map = new Map(_game, screenWidth, screenHeight, 0, 0, visibleMapHeight);

	_numLayers = new NumberText(3, 5, x + 232, y + 6);
	_rank = new Surface(26, 23, x + 107, y + 33);

	// Create buttons
	_btnUnitUp = new BattlescapeButton(32, 16, x + 48, y);
	_btnUnitDown = new BattlescapeButton(32, 16, x + 48, y + 16);
	_btnMapUp = new BattlescapeButton(32, 16, x + 80, y);
	_btnMapDown = new BattlescapeButton(32, 16, x + 80, y + 16);
	_btnShowMap = new BattlescapeButton(32, 16, x + 112, y);
	_btnKneel = new BattlescapeButton(32, 16, x + 112, y + 16);
	_btnInventory = new BattlescapeButton(32, 16, x + 144, y);
	_btnCenter = new BattlescapeButton(32, 16, x + 144, y + 16);
	_btnNextSoldier = new BattlescapeButton(32, 16, x + 176, y);
	_btnNextStop = new BattlescapeButton(32, 16, x + 176, y + 16);
	_btnShowLayers = new BattlescapeButton(32, 16, x + 208, y);
	_btnHelp = new BattlescapeButton(32, 16, x + 208, y + 16);
	_btnEndTurn = new BattlescapeButton(32, 16, x + 240, y);
	_btnAbort = new BattlescapeButton(32, 16, x + 240, y + 16);
	_btnStats = new InteractiveSurface(164, 23, x + 107, y + 33);
	_btnReserveNone = new BattlescapeButton(17, 11, x + 60, y + 33);
	_btnReserveSnap = new BattlescapeButton(17, 11, x + 78, y + 33);
	_btnReserveAimed = new BattlescapeButton(17, 11, x + 60, y + 45);
	_btnReserveAuto = new BattlescapeButton(17, 11, x + 78, y + 45);
	_btnReserveKneel = new BattlescapeButton(10, 23, x + 96, y + 33);
	_btnZeroTUs = new BattlescapeButton(10, 23, x + 49, y + 33);
	_btnLeftHandItem = new InteractiveSurface(32, 48, x + 8, y + 4);
	_btnRightHandItem = new InteractiveSurface(32, 48, x + 280, y + 4);
	_numAmmoLeft.reserve(RuleItem::AmmoSlotMax);
	_numAmmoRight.reserve(RuleItem::AmmoSlotMax);
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		_numAmmoLeft.push_back(new NumberText(30, 5, x + 8, y + 4 + 6 * slot));
		_numAmmoRight.push_back(new NumberText(30, 5, x + 280, y + 4 + 6 * slot));
	}
	_numMedikitLeft.reserve(RuleItem::MedikitSlots);
	_numMedikitRight.reserve(RuleItem::MedikitSlots);
	for (int slot = 0; slot < RuleItem::MedikitSlots; ++slot)
	{
		_numMedikitLeft.push_back(new NumberText(30, 5, x + 9, y + 32 + 7 * slot));
		_numMedikitRight.push_back(new NumberText(30, 5, x + 281, y + 32 + 7 * slot));
	}
	_numTwoHandedIndicatorLeft = new NumberText(10, 5, x + 36, y + 46);
	_numTwoHandedIndicatorRight = new NumberText(10, 5, x + 308, y + 46);
	const int visibleUnitX = _game->getMod()->getInterface("battlescape")->getElement("visibleUnits")->x;
	const int visibleUnitY = _game->getMod()->getInterface("battlescape")->getElement("visibleUnits")->y;
	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[i] = new InteractiveSurface(15, 12, x + visibleUnitX, y + visibleUnitY - (i * 13));
		_numVisibleUnit[i] = new NumberText(15, 12, _btnVisibleUnit[i]->getX() + 6 , _btnVisibleUnit[i]->getY() + 4);
	}
	_numVisibleUnit[9]->setX(_numVisibleUnit[9]->getX() - 2); // center number 10
	_warning = new WarningMessage(224, 24, x + 48, y + 32);
	_btnLaunch = new BattlescapeButton(32, 24, screenWidth - 32, 0); // we need screenWidth, because that is independent of the black bars on the screen
	_btnLaunch->setVisible(false);
	_btnPsi = new BattlescapeButton(32, 24, screenWidth - 32, 25); // we need screenWidth, because that is independent of the black bars on the screen
	_btnPsi->setVisible(false);
	_btnSpecial = new BattlescapeButton(32, 24, screenWidth - 32, 25); // we need screenWidth, because that is independent of the black bars on the screen
	_btnSpecial->setVisible(false);
	_btnSkills = new BattlescapeButton(32, 24, screenWidth - 32, 25); // we need screenWidth, because that is independent of the black bars on the screen
	_btnSkills->setVisible(false);

	{
		auto posX = (screenWidth - 32);
		for (auto& pos :  _posSpecialActions)
		{
			pos = posX;
			posX -= 32;
		}
	}


	// Reset touch flags
	_game->resetTouchButtonFlags();

	_btnCtrl = new BattlescapeButton(32, 24, 2, 10);
	_btnAlt = new BattlescapeButton(32, 24, 2, 35);
	_btnShift = new BattlescapeButton(32, 24, 2, 60);
	_btnRMB = new BattlescapeButton(32, 24, 2, 85);
	_btnMMB = new BattlescapeButton(32, 24, 2, 110);

	// Create soldier stats summary
	_rankTiny = new Surface(7, 7, x + 135, y + 33);
	_txtName = new Text(136, 10, x + 135, y + 32);

	_manaBarVisible = _game->getMod()->isManaFeatureEnabled()
		&& _game->getMod()->isManaBarEnabled()
		&& _game->getSavedGame()->isManaUnlocked(_game->getMod());
	int step = _manaBarVisible ? 3 : 4;

	_numTimeUnits = new NumberText(15, 5, x + 136, y + 42);
	_barTimeUnits = new Bar(102, 3, x + 170, y + 41);

	_numEnergy = new NumberText(15, 5, x + 154, y + 42);
	_barEnergy = new Bar(102, 3, x + 170, y + 41 + step);

	_numHealth = new NumberText(15, 5, x + 136, y + 50);
	_barHealth= new Bar(102, 3, x + 170, y + 41 + step*2);

	_numMorale = new NumberText(15, 5, x + 154, y + 50);
	_barMorale = new Bar(102, 3, x + 170, y + 41 + step*3);

	if (_manaBarVisible)
	{
		_barMana = new Bar(102, 3, x + 170, y + 41 + step*4);
	}

	_txtDebug = new Text(300, 10, 20, 0);
	_txtTooltip = new Text(300, 10, x + 2, y - 10);

	// Palette transformations
	auto enviro = _game->getSavedGame()->getSavedBattle()->getEnviroEffects();
	if (enviro)
	{
		for (auto& change : enviro->getPaletteTransformations())
		{
			Palette *origPal = _game->getMod()->getPalette(change.first, false);
			Palette *newPal = _game->getMod()->getPalette(change.second, false);
			if (origPal && newPal)
			{
				origPal->copyFrom(newPal);
				_paletteResetNeeded = true;
			}
		}
	}

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	if (_game->getMod()->getInterface("battlescape")->getElement("pathfinding"))
	{
		Element *pathing = _game->getMod()->getInterface("battlescape")->getElement("pathfinding");

		Pathfinding::green = pathing->color;
		Pathfinding::yellow = pathing->color2;
		Pathfinding::red = pathing->border;
	}

	add(_map);
	add(_icons);

	// Add in custom reserve buttons
	Surface *icons = _game->getMod()->getSurface("ICONS.PCK");
	if (_game->getMod()->getSurface("TFTDReserve", false))
	{
		Surface *tftdIcons = _game->getMod()->getSurface("TFTDReserve");
		tftdIcons->blitNShade(icons, 48, 176);
	}

	// there is some cropping going on here, because the icons image is 320x200 while we only need the bottom of it.
	auto crop = icons->getCrop();
	crop.getCrop()->x = 0;
	crop.getCrop()->y = 200 - iconsHeight;
	crop.getCrop()->w = iconsWidth;
	crop.getCrop()->h = iconsHeight;
	// we need to blit the icons before we add the battlescape buttons, as they copy the underlying parent surface.
	crop.blit(_icons);

	// this is a hack to fix the single transparent pixel on TFTD's icon panel.
	if (_game->getMod()->getInterface("battlescape")->getElement("icons")->TFTDMode)
	{
		_icons->setPixel(46, 44, 8);
	}

	// custom OXCE links button
	if (Options::oxceLinks && _game->getMod()->getSurface("oxceLinks", false))
	{
		Surface* oxceLinks = _game->getMod()->getSurface("oxceLinks");
		oxceLinks->blitNShade(_icons, 208, 0);

		_numLayers->setVisible(false);
	}

	add(_rank, "rank", "battlescape", _icons);
	add(_rankTiny, "rank", "battlescape", _icons);
	add(_btnUnitUp, "buttonUnitUp", "battlescape", _icons);
	add(_btnUnitDown, "buttonUnitDown", "battlescape", _icons);
	add(_btnMapUp, "buttonMapUp", "battlescape", _icons);
	add(_btnMapDown, "buttonMapDown", "battlescape", _icons);
	add(_btnShowMap, "buttonShowMap", "battlescape", _icons);
	add(_btnKneel, "buttonKneel", "battlescape", _icons);
	add(_btnInventory, "buttonInventory", "battlescape", _icons);
	add(_btnCenter, "buttonCenter", "battlescape", _icons);
	add(_btnNextSoldier, "buttonNextSoldier", "battlescape", _icons);
	add(_btnNextStop, "buttonNextStop", "battlescape", _icons);
	add(_btnShowLayers, "buttonShowLayers", "battlescape", _icons);
	add(_numLayers, "numLayers", "battlescape", _icons);
	add(_btnHelp, "buttonHelp", "battlescape", _icons);
	add(_btnEndTurn, "buttonEndTurn", "battlescape", _icons);
	add(_btnAbort, "buttonAbort", "battlescape", _icons);
	add(_btnStats, "buttonStats", "battlescape", _icons);
	add(_txtName, "textName", "battlescape", _icons);
	// need to do this here, because of TFTD
	if (_game->getMod()->getSurface("AvatarBackground", false))
	{
		// put tiny rank icon where name used to be
		_rankTiny->setX(_txtName->getX());
		_rankTiny->setY(_txtName->getY() + 1);
		// move name more to the right
		_txtName->setWidth(_txtName->getWidth() - 8);
		_txtName->setX(_txtName->getX() + 8);
	}
	add(_numTimeUnits, "numTUs", "battlescape", _icons);
	add(_numEnergy, "numEnergy", "battlescape", _icons);
	add(_numHealth, "numHealth", "battlescape", _icons);
	add(_numMorale, "numMorale", "battlescape", _icons);
	add(_barTimeUnits, "barTUs", "battlescape", _icons);
	add(_barEnergy, "barEnergy", "battlescape", _icons);
	add(_barHealth, "barHealth", "battlescape", _icons);
	add(_barMorale, "barMorale", "battlescape", _icons);
	if (_manaBarVisible)
	{
		add(_barMana, "barMana", "battlescape", _icons);
	}
	add(_btnReserveNone, "buttonReserveNone", "battlescape", _icons);
	add(_btnReserveSnap, "buttonReserveSnap", "battlescape", _icons);
	add(_btnReserveAimed, "buttonReserveAimed", "battlescape", _icons);
	add(_btnReserveAuto, "buttonReserveAuto", "battlescape", _icons);
	add(_btnReserveKneel, "buttonReserveKneel", "battlescape", _icons);
	add(_btnZeroTUs, "buttonZeroTUs", "battlescape", _icons);
	add(_btnLeftHandItem, "buttonLeftHand", "battlescape", _icons);
	add(_btnRightHandItem, "buttonRightHand", "battlescape", _icons);
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		add(_numAmmoLeft[slot], "numAmmoLeft", "battlescape", _icons);
		add(_numAmmoRight[slot], "numAmmoRight", "battlescape", _icons);
	}
	for (int slot = 0; slot < RuleItem::MedikitSlots; ++slot)
	{
		add(_numMedikitLeft[slot], "numMedikitLeft", "battlescape", _icons);
		add(_numMedikitRight[slot], "numMedikitRight", "battlescape", _icons);
	}
	add(_numTwoHandedIndicatorLeft, "numTwoHandedIndicatorLeft", "battlescape", _icons);
	add(_numTwoHandedIndicatorRight, "numTwoHandedIndicatorRight", "battlescape", _icons);
	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		add(_btnVisibleUnit[i]);
		add(_numVisibleUnit[i]);
	}
	add(_warning, "warning", "battlescape", _icons);
	add(_txtDebug);
	add(_txtTooltip, "textTooltip", "battlescape", _icons);
	add(_btnLaunch);
	_game->getMod()->getSurfaceSet("SPICONS.DAT")->getFrame(0)->blitNShade(_btnLaunch, 0, 0);
	add(_btnPsi);
	_game->getMod()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blitNShade(_btnPsi, 0, 0);
	add(_btnSpecial);
	_game->getMod()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blitNShade(_btnSpecial, 0, 0); // use psi button for default
	add(_btnSkills);
	_game->getMod()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blitNShade(_btnSkills, 0, 0); // use psi button for default

	add(_btnCtrl);
	add(_btnAlt);
	add(_btnShift);
	add(_btnRMB);
	add(_btnMMB);

	_game->getMod()->getSurfaceSet("Touch")->getFrame(0)->blitNShade(_btnCtrl, 0, 0);
	_game->getMod()->getSurfaceSet("Touch")->getFrame(2)->blitNShade(_btnAlt, 0, 0);
	_game->getMod()->getSurfaceSet("Touch")->getFrame(4)->blitNShade(_btnShift, 0, 0);
	_game->getMod()->getSurfaceSet("Touch")->getFrame(6)->blitNShade(_btnRMB, 0, 0);
	_game->getMod()->getSurfaceSet("Touch")->getFrame(8)->blitNShade(_btnMMB, 0, 0);

	_btnCtrl->initSurfaces(_game->getMod()->getSurfaceSet("Touch")->getFrame(1));
	_btnAlt->initSurfaces(_game->getMod()->getSurfaceSet("Touch")->getFrame(3));
	_btnShift->initSurfaces(_game->getMod()->getSurfaceSet("Touch")->getFrame(5));
	_btnRMB->initSurfaces(_game->getMod()->getSurfaceSet("Touch")->getFrame(7));
	_btnMMB->initSurfaces(_game->getMod()->getSurfaceSet("Touch")->getFrame(9));

	// Set up objects
	_save = _game->getSavedGame()->getSavedBattle();
	_map->init();
	_map->onMouseOver((ActionHandler)&BattlescapeState::mapOver);
	_map->onMousePress((ActionHandler)&BattlescapeState::mapPress);
	_map->onMouseClick((ActionHandler)&BattlescapeState::mapClick, 0);
	_map->onMouseIn((ActionHandler)&BattlescapeState::mapIn);

	_numLayers->setColor(Palette::blockOffset(1)-2);
	_numLayers->setValue(1);

	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		_numAmmoLeft[slot]->setValue(999);
		_numAmmoRight[slot]->setValue(999);
	}
	for (int slot = 0; slot < RuleItem::MedikitSlots; ++slot)
	{
		_numMedikitLeft[slot]->setValue(999);
		_numMedikitRight[slot]->setValue(999);
	}
	_numTwoHandedIndicatorLeft->setValue(2);
	_numTwoHandedIndicatorRight->setValue(2);

	_icons->onMouseIn((ActionHandler)&BattlescapeState::mouseInIcons);
	_icons->onMouseOut((ActionHandler)&BattlescapeState::mouseOutIcons);

	_btnUnitUp->onMouseClick((ActionHandler)&BattlescapeState::btnUnitUpClick);
	_btnUnitUp->setTooltip("STR_UNIT_LEVEL_ABOVE");
	_btnUnitUp->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnUnitUp->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnUnitDown->onMouseClick((ActionHandler)&BattlescapeState::btnUnitDownClick);
	_btnUnitDown->setTooltip("STR_UNIT_LEVEL_BELOW");
	_btnUnitDown->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnUnitDown->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnMapUp->onMouseClick((ActionHandler)&BattlescapeState::btnMapUpClick);
	_btnMapUp->onKeyboardPress((ActionHandler)&BattlescapeState::btnMapUpClick, Options::keyBattleLevelUp);
	_btnMapUp->setTooltip("STR_VIEW_LEVEL_ABOVE");
	_btnMapUp->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnMapUp->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnMapDown->onMouseClick((ActionHandler)&BattlescapeState::btnMapDownClick);
	_btnMapDown->onKeyboardPress((ActionHandler)&BattlescapeState::btnMapDownClick, Options::keyBattleLevelDown);
	_btnMapDown->setTooltip("STR_VIEW_LEVEL_BELOW");
	_btnMapDown->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnMapDown->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnShowMap->onMouseClick((ActionHandler)&BattlescapeState::btnShowMapClick);
	_btnShowMap->onKeyboardPress((ActionHandler)&BattlescapeState::btnShowMapClick, Options::keyBattleMap);
	_btnShowMap->setTooltip("STR_MINIMAP");
	_btnShowMap->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnShowMap->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnKneel->onMouseClick((ActionHandler)&BattlescapeState::btnKneelClick);
	_btnKneel->onKeyboardPress((ActionHandler)&BattlescapeState::btnKneelClick, Options::keyBattleKneel);
	_btnKneel->setTooltip("STR_KNEEL");
	_btnKneel->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnKneel->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);
	_btnKneel->allowToggleInversion();

	_btnInventory->onMouseClick((ActionHandler)&BattlescapeState::btnInventoryClick);
	_btnInventory->onKeyboardPress((ActionHandler)&BattlescapeState::btnInventoryClick, Options::keyBattleInventory);
	_btnInventory->setTooltip("STR_INVENTORY");
	_btnInventory->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnInventory->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnCenter->onMouseClick((ActionHandler)&BattlescapeState::btnCenterClick);
	_btnCenter->onKeyboardPress((ActionHandler)&BattlescapeState::btnCenterClick, Options::keyBattleCenterUnit);
	_btnCenter->setTooltip("STR_CENTER_SELECTED_UNIT");
	_btnCenter->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnCenter->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnNextSoldier->onMouseClick((ActionHandler)&BattlescapeState::btnNextSoldierClick, SDL_BUTTON_LEFT);
	_btnNextSoldier->onMouseClick((ActionHandler)&BattlescapeState::btnPrevSoldierClick, SDL_BUTTON_RIGHT);
	_btnNextSoldier->onKeyboardPress((ActionHandler)&BattlescapeState::btnNextSoldierClick, Options::keyBattleNextUnit);
	_btnNextSoldier->onKeyboardPress((ActionHandler)&BattlescapeState::btnPrevSoldierClick, Options::keyBattlePrevUnit);
	_btnNextSoldier->setTooltip("STR_NEXT_UNIT");
	_btnNextSoldier->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnNextSoldier->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnNextStop->onMouseClick((ActionHandler)&BattlescapeState::btnNextStopClick);
	_btnNextStop->onKeyboardPress((ActionHandler)&BattlescapeState::btnNextStopClick, Options::keyBattleDeselectUnit);
	_btnNextStop->setTooltip("STR_DESELECT_UNIT");
	_btnNextStop->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnNextStop->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnShowLayers->onMouseClick((ActionHandler)&BattlescapeState::btnShowLayersClick);
	_btnShowLayers->setTooltip(Options::oxceLinks ? "STR_EXTENDED_LINKS" : "STR_MULTI_LEVEL_VIEW");
	_btnShowLayers->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnShowLayers->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);
	_btnShowLayers->onKeyboardPress((ActionHandler)&BattlescapeState::btnShowLayersClickOrig, Options::keyBattleShowLayers);
	_btnShowLayers->onKeyboardPress((ActionHandler)&BattlescapeState::btnUfopaediaClick, Options::keyGeoUfopedia);

	_btnHelp->onMouseClick((ActionHandler)&BattlescapeState::btnHelpClick);
	if (!_save->isPreview())
	{
		_btnHelp->onKeyboardPress((ActionHandler)&BattlescapeState::btnHelpClick, Options::keyBattleOptions);
	}
	_btnHelp->setTooltip("STR_OPTIONS");
	_btnHelp->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnHelp->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnEndTurn->onMouseClick((ActionHandler)&BattlescapeState::btnEndTurnClick);
	_btnEndTurn->onKeyboardPress((ActionHandler)&BattlescapeState::btnEndTurnClick, Options::keyBattleEndTurn);
	if (_save->isPreview())
	{
		_btnEndTurn->onKeyboardPress((ActionHandler)&BattlescapeState::btnEndTurnClick, Options::keyBattleOptions); // Esc
	}
	_btnEndTurn->setTooltip("STR_END_TURN");
	_btnEndTurn->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipInEndTurn);
	_btnEndTurn->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnAbort->onMouseClick((ActionHandler)&BattlescapeState::btnAbortClick);
	_btnAbort->onKeyboardPress((ActionHandler)&BattlescapeState::btnAbortClick, Options::keyBattleAbort);
	_btnAbort->setTooltip("STR_ABORT_MISSION");
	_btnAbort->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnAbort->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnStats->onMouseClick((ActionHandler)&BattlescapeState::btnStatsClick);
	_btnStats->onMouseClick((ActionHandler)&BattlescapeState::btnStatsClick, SDL_BUTTON_RIGHT);
	_btnStats->onKeyboardPress((ActionHandler)&BattlescapeState::btnStatsClick, Options::keyBattleStats);
	_btnStats->setTooltip("STR_UNIT_STATS");
	_btnStats->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnStats->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnLeftHandItem->onMouseClick((ActionHandler)&BattlescapeState::btnLeftHandItemClick);
	_btnLeftHandItem->onMouseClick((ActionHandler)&BattlescapeState::btnLeftHandItemClick, SDL_BUTTON_RIGHT);
	_btnLeftHandItem->onMouseClick((ActionHandler)&BattlescapeState::btnLeftHandItemClick, SDL_BUTTON_MIDDLE);
	_btnLeftHandItem->onKeyboardPress((ActionHandler)&BattlescapeState::btnLeftHandItemClick, Options::keyBattleUseLeftHand);
	_btnLeftHandItem->setTooltip("STR_USE_LEFT_HAND");
	_btnLeftHandItem->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipInExtraLeftHand);
	_btnLeftHandItem->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnRightHandItem->onMouseClick((ActionHandler)&BattlescapeState::btnRightHandItemClick);
	_btnRightHandItem->onMouseClick((ActionHandler)&BattlescapeState::btnRightHandItemClick, SDL_BUTTON_RIGHT);
	_btnRightHandItem->onMouseClick((ActionHandler)&BattlescapeState::btnRightHandItemClick, SDL_BUTTON_MIDDLE);
	_btnRightHandItem->onKeyboardPress((ActionHandler)&BattlescapeState::btnRightHandItemClick, Options::keyBattleUseRightHand);
	_btnRightHandItem->setTooltip("STR_USE_RIGHT_HAND");
	_btnRightHandItem->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipInExtraRightHand);
	_btnRightHandItem->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnReserveNone->onMouseClick((ActionHandler)&BattlescapeState::btnReserveClick);
	_btnReserveNone->onKeyboardPress((ActionHandler)&BattlescapeState::btnReserveClick, Options::keyBattleReserveNone);
	_btnReserveNone->setTooltip("STR_DONT_RESERVE_TIME_UNITS");
	_btnReserveNone->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnReserveNone->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnReserveSnap->onMouseClick((ActionHandler)&BattlescapeState::btnReserveClick);
	_btnReserveSnap->onKeyboardPress((ActionHandler)&BattlescapeState::btnReserveClick, Options::keyBattleReserveSnap);
	_btnReserveSnap->setTooltip("STR_RESERVE_TIME_UNITS_FOR_SNAP_SHOT");
	_btnReserveSnap->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnReserveSnap->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnReserveAimed->onMouseClick((ActionHandler)&BattlescapeState::btnReserveClick);
	_btnReserveAimed->onKeyboardPress((ActionHandler)&BattlescapeState::btnReserveClick, Options::keyBattleReserveAimed);
	_btnReserveAimed->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AIMED_SHOT");
	_btnReserveAimed->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnReserveAimed->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnReserveAuto->onMouseClick((ActionHandler)&BattlescapeState::btnReserveClick);
	_btnReserveAuto->onKeyboardPress((ActionHandler)&BattlescapeState::btnReserveClick, Options::keyBattleReserveAuto);
	_btnReserveAuto->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AUTO_SHOT");
	_btnReserveAuto->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnReserveAuto->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnReserveKneel->onMouseClick((ActionHandler)&BattlescapeState::btnReserveKneelClick);
	_btnReserveKneel->onKeyboardPress((ActionHandler)&BattlescapeState::btnReserveKneelClick, Options::keyBattleReserveKneel);
	_btnReserveKneel->setTooltip("STR_RESERVE_TIME_UNITS_FOR_KNEEL");
	_btnReserveKneel->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnReserveKneel->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);
	_btnReserveKneel->allowToggleInversion();

	_btnZeroTUs->onMouseClick((ActionHandler)&BattlescapeState::btnZeroTUsClick, SDL_BUTTON_RIGHT);
	_btnZeroTUs->onKeyboardPress((ActionHandler)&BattlescapeState::btnZeroTUsClick, Options::keyBattleZeroTUs);
	_btnZeroTUs->setTooltip("STR_EXPEND_ALL_TIME_UNITS");
	_btnZeroTUs->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
	_btnZeroTUs->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);
	_btnZeroTUs->allowClickInversion();

	// shortcuts without a specific button
	_btnStats->onKeyboardPress((ActionHandler)&BattlescapeState::btnReloadClick, Options::keyBattleReload);
	_btnStats->onKeyboardPress((ActionHandler)&BattlescapeState::btnSelectMusicTrackClick, Options::keySelectMusicTrack);
	_btnStats->onKeyboardPress((ActionHandler)&BattlescapeState::btnPersonalLightingClick, Options::keyBattlePersonalLighting);
	_btnStats->onKeyboardPress((ActionHandler)&BattlescapeState::btnNightVisionClick, Options::keyNightVisionToggle);
	//_btnStats->onKeyboardPress((ActionHandler)&BattlescapeState::btnTouchButtonsClick, SDLK_t); // for debugging only

	// automatic night vision
	if (_save->getGlobalShade() > Options::oxceAutoNightVisionThreshold)
	{
		bool ignore = (enviro && enviro->ignoreAutoNightVisionUserSetting());
		if (!ignore)
		{
			// turn personal lights off
			//_save->getTileEngine()->togglePersonalLighting();
			// turn night vision on
			_map->toggleNightVision();
		}
	}

	SDLKey buttons[] = {Options::keyBattleCenterEnemy1,
						Options::keyBattleCenterEnemy2,
						Options::keyBattleCenterEnemy3,
						Options::keyBattleCenterEnemy4,
						Options::keyBattleCenterEnemy5,
						Options::keyBattleCenterEnemy6,
						Options::keyBattleCenterEnemy7,
						Options::keyBattleCenterEnemy8,
						Options::keyBattleCenterEnemy9,
						Options::keyBattleCenterEnemy10};
	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		std::ostringstream tooltip;
		_btnVisibleUnit[i]->onMouseClick((ActionHandler)&BattlescapeState::btnVisibleUnitClick);
		_btnVisibleUnit[i]->onKeyboardPress((ActionHandler)&BattlescapeState::btnVisibleUnitClick, buttons[i]);
		tooltip << "STR_CENTER_ON_ENEMY_" << (i+1);
		_txtVisibleUnitTooltip[i] = tooltip.str();
		_btnVisibleUnit[i]->setTooltip(_txtVisibleUnitTooltip[i]);
		_btnVisibleUnit[i]->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
		_btnVisibleUnit[i]->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);
		_numVisibleUnit[i]->setColor(_indicatorTextColor);
		_numVisibleUnit[i]->setValue(i+1);
	}
	_txtVisibleUnitTooltip[VISIBLE_MAX] = "STR_CENTER_ON_WOUNDED_FRIEND";
	_txtVisibleUnitTooltip[VISIBLE_MAX+1] = "STR_CENTER_ON_DIZZY_FRIEND";

	_warning->setColor(_game->getMod()->getInterface("battlescape")->getElement("warning")->color2);
	_warning->setTextColor(_game->getMod()->getInterface("battlescape")->getElement("warning")->color);
	_btnLaunch->onMouseClick((ActionHandler)&BattlescapeState::btnLaunchClick);
	_btnPsi->onMouseClick((ActionHandler)&BattlescapeState::btnPsiClick);

	_btnSpecial->onMouseClick((ActionHandler)&BattlescapeState::btnSpecialClick);
	_btnSpecial->onMouseClick((ActionHandler)&BattlescapeState::btnSpecialClick, SDL_BUTTON_MIDDLE);
	_btnSpecial->onKeyboardPress((ActionHandler)&BattlescapeState::btnSpecialClick, Options::keyBattleUseSpecial);
	_btnSpecial->setTooltip("STR_USE_SPECIAL_ITEM");
	_btnSpecial->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipInExtraSpecial);
	_btnSpecial->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);

	_btnSkills->onMouseClick((ActionHandler)&BattlescapeState::btnSkillsClick);
	_btnSkills->onKeyboardPress((ActionHandler)&BattlescapeState::btnSkillsClick, Options::keyBattleUseSpecial);

	_btnCtrl->onMouseClick((ActionHandler)&BattlescapeState::btnCtrlClick);
	_btnAlt->onMouseClick((ActionHandler)&BattlescapeState::btnAltClick);
	_btnShift->onMouseClick((ActionHandler)&BattlescapeState::btnShiftClick);
	_btnRMB->onMouseClick((ActionHandler)&BattlescapeState::btnRMBClick);
	_btnMMB->onMouseClick((ActionHandler)&BattlescapeState::btnMMBClick);

	_btnCtrl->allowToggleInversion();
	_btnAlt->allowToggleInversion();
	_btnShift->allowToggleInversion();
	_btnRMB->allowToggleInversion();
	_btnMMB->allowToggleInversion();

	_btnCtrl->setVisible(false);
	_btnAlt->setVisible(false);
	_btnShift->setVisible(false);
	_btnRMB->setVisible(false);
	_btnMMB->setVisible(false);

	_txtName->setHighContrast(true);

	_barTimeUnits->setScale(1.0);
	_barEnergy->setScale(1.0);
	_barHealth->setScale(1.0);
	_barMorale->setScale(1.0);
	if (_manaBarVisible)
	{
		_barMana->setScale(1.0);
	}

	_txtDebug->setColor(Palette::blockOffset(8));
	_txtDebug->setHighContrast(true);

	_txtTooltip->setHighContrast(true);

	_btnReserveNone->setGroup(&_reserve);
	_btnReserveSnap->setGroup(&_reserve);
	_btnReserveAimed->setGroup(&_reserve);
	_btnReserveAuto->setGroup(&_reserve);

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

	_animTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_animTimer->onTimer((StateHandler)&BattlescapeState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_gameTimer->onTimer((StateHandler)&BattlescapeState::handleState);

	_battleGame = new BattlescapeGame(_save, this);

	_barHealthColor = _barHealth->getColor();
}


/**
 * Deletes the battlescapestate.
 */
BattlescapeState::~BattlescapeState()
{
	delete _animTimer;
	delete _gameTimer;
	delete _battleGame;

	resetPalettes();
}

void BattlescapeState::resetPalettes()
{
	if (_paletteResetNeeded)
	{
		for (auto origPal : _game->getMod()->getPalettes())
		{
			if (origPal.first.find("PAL_") == 0)
			{
				std::string backupName = "BACKUP_" + origPal.first;
				Palette *backupPal = _game->getMod()->getPalette(backupName, false);
				if (backupPal)
				{
					origPal.second->copyFrom(backupPal);
				}
			}
		}
		_paletteResetNeeded = false;
	}
}

/**
 * Initializes the battlescapestate.
 */
void BattlescapeState::init()
{
	if (_paletteResetRequested)
	{
		_paletteResetRequested = false;

		resetPalettes();
		_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);
		for (auto surface : _surfaces)
		{
			surface->setPalette(_palette);
		}
	}

	if (_save->getAmbientSound() != Mod::NO_SOUND)
	{
		_game->getMod()->getSoundByDepth(_save->getDepth(), _save->getAmbientSound())->loop();
		_game->setVolume(Options::soundVolume, Options::musicVolume, Options::uiVolume);
	}

	State::init();
	_animTimer->start();
	_gameTimer->start();
	_map->setFocus(true);
	_map->draw();
	_battleGame->init();
	updateSoldierInfo();

	switch (_save->getTUReserved())
	{
	case BA_SNAPSHOT:
		_reserve = _btnReserveSnap;
		break;
	case BA_AIMEDSHOT:
		_reserve = _btnReserveAimed;
		break;
	case BA_AUTOSHOT:
		_reserve = _btnReserveAuto;
		break;
	default:
		_reserve = _btnReserveNone;
		break;
	}
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

		if (!playableUnitSelected())
		{
			selectNextPlayerUnit();
		}
		if (playableUnitSelected())
		{
			_battleGame->setupCursor();
			_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
		}
		_firstInit = false;
		_btnReserveNone->setGroup(&_reserve);
		_btnReserveSnap->setGroup(&_reserve);
		_btnReserveAimed->setGroup(&_reserve);
		_btnReserveAuto->setGroup(&_reserve);
	}
	_txtTooltip->setText("");
	_btnReserveKneel->toggle(_save->getKneelReserved());
	_battleGame->setKneelReserved(_save->getKneelReserved());
	if (_autosave > 0 && !_save->isPreview())
	{
		int currentTurn = _autosave;
		_autosave = 0;
		if (_game->getSavedGame()->isIronman())
		{
			_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_IRONMAN, _palette));
		}
		else if (Options::autosave)
		{
			_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_AUTO_BATTLESCAPE, _palette, currentTurn));
		}
	}
}

/**
 * Runs the timers and handles popups.
 */
void BattlescapeState::think()
{
	static bool popped = false;

	if (_gameTimer->isRunning())
	{
		if (_popups.empty())
		{
			State::think();
			_battleGame->think();
			_animTimer->think(this, 0);
			_gameTimer->think(this, 0);
			if (popped)
			{
				_battleGame->handleNonTargetAction();
				popped = false;
			}
		}
		else
		{
			// Handle popups
			_game->pushState(*_popups.begin());
			_popups.erase(_popups.begin());
			popped = true;
			return;
		}
	}
}

/**
 * Processes any mouse moving over the map.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapOver(Action *action)
{
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
}

/**
 * Processes any presses on the map.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapPress(Action *action)
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
		_mouseScrollingStartTime = SDL_GetTicks();
	}
}

/**
 * Processes any clicks on the map to
 * command units.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapClick(Action *action)
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

	// right-click aborts walking state
	if (_game->isRightClick(action))
	{
		if (_battleGame->cancelCurrentAction())
		{
			return;
		}
	}

	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons) return;


	// don't accept leftclicks if there is no cursor or there is an action busy
	if (_map->getCursorType() == CT_NONE || _battleGame->isBusy()) return;

	Position pos;
	_map->getSelectorPosition(&pos);

	if (_save->getDebugMode())
	{
		std::ostringstream ss;
		ss << "Clicked " << pos;
		debug(ss.str());
	}

	if (_save->getTile(pos) != 0) // don't allow to click into void
	{
		if (_game->isRightClick(action, true) && playableUnitSelected())
		{
			_battleGame->secondaryAction(pos);
		}
		else if (_game->isLeftClick(action, true))
		{
			_battleGame->primaryAction(pos);
		}
		else if (_game->isMiddleClick(action, true))
		{
			_battleGame->cancelCurrentAction();
			BattleUnit *bu = _save->selectUnit(pos);
			if (bu && (bu->getVisible() || _save->getDebugMode()))
			{
				if (_save->getDebugMode() && _game->isCtrlPressed())
				{
					// mind probe
					popup(new UnitInfoState(bu, this, false, true));
				}
				else
				{
					_game->pushState(new AlienInventoryState(bu));
				}
			}
		}
	}
}

/**
 * Handles mouse entering the map surface.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapIn(Action *)
{
	_isMouseScrolling = false;
	_map->setButtonsPressed(Options::battleDragScrollButton, false);
}

/**
 * Moves the selected unit up.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitUpClick(Action *)
{
	if (playableUnitSelected() && _save->getPathfinding()->validateUpDown(_save->getSelectedUnit(), _save->getSelectedUnit()->getPosition(), Pathfinding::DIR_UP))
	{
		_battleGame->cancelAllActions();
		_battleGame->moveUpDown(_save->getSelectedUnit(), Pathfinding::DIR_UP);
	}
}

/**
 * Moves the selected unit down.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitDownClick(Action *)
{
	if (playableUnitSelected() && _save->getPathfinding()->validateUpDown(_save->getSelectedUnit(), _save->getSelectedUnit()->getPosition(), Pathfinding::DIR_DOWN))
	{
		_battleGame->cancelAllActions();
		_battleGame->moveUpDown(_save->getSelectedUnit(), Pathfinding::DIR_DOWN);
	}
}

/**
 * Shows the next map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapUpClick(Action *)
{
	if (_save->getSide() == FACTION_PLAYER || _save->getDebugMode() || _save->getSide() == FACTION_ALIEN_PLAYER)
		_map->getCamera()->up();
}

/**
 * Shows the previous map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapDownClick(Action *)
{
	if (_save->getSide() == FACTION_PLAYER || _save->getDebugMode() || _save->getSide() == FACTION_ALIEN_PLAYER)
		_map->getCamera()->down();
}

/**
 * Shows the minimap.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnShowMapClick(Action *)
{
	//MiniMapState
	if (allowButtons())
		_game->pushState (new MiniMapState (_map->getCamera(), _save));
}

void BattlescapeState::toggleKneelButton(BattleUnit* unit)
{
	if (_btnKneel->isTFTDMode())
	{
		_btnKneel->toggle(unit && unit->isKneeled());
	}
	else
	{
		_game->getMod()->getSurfaceSet("KneelButton")->getFrame((unit && unit->isKneeled()) ? 1 : 0)->blitNShade(_btnKneel, 0, 0);
	}
}

/**
 * Toggles the current unit's kneel/standup status.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnKneelClick(Action *)
{
	if (allowButtons())
	{
		BattleUnit *bu = _save->getSelectedUnit();
		if (bu)
		{
			_battleGame->kneel(bu);
			toggleKneelButton(bu);

			// update any path preview when unit kneels
			if (_battleGame->getPathfinding()->isPathPreviewed())
			{
				_battleGame->getPathfinding()->calculate(_battleGame->getCurrentAction()->actor, _battleGame->getCurrentAction()->target, _battleGame->getCurrentAction()->getMoveType());
				_battleGame->getPathfinding()->removePreview();
				_battleGame->getPathfinding()->previewPath();
			}
		}
	}
}

/**
 * Goes to the soldier info screen.
 * Additionally resets TUs for current side in debug mode.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnInventoryClick(Action *)
{
	if (_save->isPreview())
	{
		return;
	}
#if 0
	if (_save->getDebugMode())
	{
		for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
			if ((*i)->getOriginalFaction() == _save->getSide())
				(*i)->prepareNewTurn();
		updateSoldierInfo();
	}
#endif
	if (playableUnitSelected()
		&& (_save->getSelectedUnit()->hasInventory() || _save->getDebugMode()))
	{
		_battleGame->cancelAllActions();
		_game->pushState(new InventoryState(true, this, 0));
	}
}

/**
 * Centers on the currently selected soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnCenterClick(Action *)
{
	if (playableUnitSelected())
	{
		_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
		_map->refreshSelectorPosition();
	}
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextSoldierClick(Action *action)
{
	if (_game->isRightClick(action, true))
	{
		btnPrevSoldierClick(action);
		return;
	}

	if (allowButtons())
	{
		selectNextPlayerUnit(true, false);
		_map->refreshSelectorPosition();
	}
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextStopClick(Action *)
{
	if (allowButtons())
	{
		selectNextPlayerUnit(true, true);
		_map->refreshSelectorPosition();
	}
}

/**
 * Selects next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPrevSoldierClick(Action *)
{
	if (allowButtons())
	{
		selectPreviousPlayerUnit(true);
		_map->refreshSelectorPosition();
	}
}

/**
 * Selects the next soldier.
 * @param checkReselect When true, don't select a unit that has been previously flagged.
 * @param setReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectNextPlayerUnit(bool checkReselect, bool setReselect, bool checkInventory, bool checkFOV)
{
	if (allowButtons())
	{
		BattleUnit *unit = _save->selectNextPlayerUnit(checkReselect, setReselect, checkInventory);
		updateSoldierInfo(checkFOV);
		if (unit) _map->getCamera()->centerOnPosition(unit->getPosition());
		_battleGame->cancelAllActions();
		_battleGame->getCurrentAction()->actor = unit;
		_battleGame->setupCursor();
	}
}

/**
 * Selects the previous soldier.
 * @param checkReselect When true, don't select a unit that has been previously flagged.
 * @param setReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectPreviousPlayerUnit(bool checkReselect, bool setReselect, bool checkInventory)
{
	if (allowButtons())
	{
		BattleUnit *unit = _save->selectPreviousPlayerUnit(checkReselect, setReselect, checkInventory);
		updateSoldierInfo();
		if (unit) _map->getCamera()->centerOnPosition(unit->getPosition());
		_battleGame->cancelAllActions();
		_battleGame->getCurrentAction()->actor = unit;
		_battleGame->setupCursor();
	}
}

/**
 * Shows/hides all map layers.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnShowLayersClick(Action *)
{
	if (Options::oxceLinks)
	{
		_game->pushState(new ExtendedBattlescapeLinksState(this, _save));
	}
	else
	{
		btnShowLayersClickOrig(nullptr);
	}
}

void BattlescapeState::btnShowLayersClickOrig(Action *)
{
	_numLayers->setValue(_map->getCamera()->toggleShowAllLayers());
}

/**
 * Opens the Ufopaedia.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUfopaediaClick(Action *)
{
	if (allowButtons())
	{
		Ufopaedia::open(_game);
	}
}

/**
 * Shows options.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnHelpClick(Action *)
{
	if (_save->isPreview())
	{
		// Notes for future explorers:
		// 1. saving makes no sense
		// 2. loading could be enabled, but needs changes in the Game's _states management; make sure you know what you're doing!
		return;
	}

	if (allowButtons(true))
		_game->pushState(new PauseState(OPT_BATTLESCAPE));
}

/**
 * Requests the end of turn. This will add a 0 to the end of the state queue,
 * so all ongoing actions, like explosions are finished first before really switching turn.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnEndTurnClick(Action *)
{
	if (allowButtons())
	{
		// Temporarily deactivate the touch buttons at the end of the player's turn
		toggleTouchButtons(true, false);

		_txtTooltip->setText("");
		_battleGame->requestEndTurn(false);
	}
}

/**
 * Aborts the game.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnAbortClick(Action *)
{
	if (_save->isPreview())
	{
		if (!_save->getCraftForPreview())
		{
			// base preview
			return;
		}
		// Notes for future explorers:
		// - there are craft, which can have multiple layouts (one is chosen randomly at the start of a battle)
		// - these layouts are not forced to be compatible
		// - thus custom craft deployment for one layout may not be compatible with another layout either
		// - so instead of having multiple custom deployments per craft, I decided to not support it for such craft at all
		// - if you want to add partial or full support for it... make sure you don't forget all the corner cases
		if (_save->getCraftForPreview()->getRules()->getBattlescapeTerrainData()->getMapBlocks()->size() > 1)
		{
			return;
		}
	}

	if (allowButtons())
		_game->pushState(new AbortMissionState(_save, this));
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnStatsClick(Action *action)
{
	if (playableUnitSelected())
	{
		bool scroll = false;
		if (SCROLL_TRIGGER == Options::battleEdgeScroll &&
			SDL_MOUSEBUTTONUP == action->getDetails()->type && _game->isLeftClick(action))
		{
			int posX = action->getXMouse();
			int posY = action->getYMouse();
			if ((posX < (Camera::SCROLL_BORDER * action->getXScale()) && posX > 0)
				|| (posX > (_map->getWidth() - Camera::SCROLL_BORDER) * action->getXScale())
				|| (posY < (Camera::SCROLL_BORDER * action->getYScale()) && posY > 0)
				|| (posY > (_map->getHeight() - Camera::SCROLL_BORDER) * action->getYScale()))
				// To avoid handling this event as a click
				// on the stats button when the mouse is on the scroll-border
				scroll = true;
		}
		if (!scroll)
		{
			if (_game->isRightClick(action))
			{
				_save->setNameDisplay(!_save->isNameDisplay());
				updateSoldierInfo();
			}
			else
			{
				_battleGame->cancelAllActions();
				popup(new UnitInfoState(_save->getSelectedUnit(), this, false, false));
			}
		}
	}
}

/**
 * Shows an action popup menu. When clicked, creates the action.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnLeftHandItemClick(Action *action)
{
	if (playableUnitSelected())
	{
		// concession for touch devices:
		// click on the item to cancel action, and don't pop up a menu to select a new one
		// TODO: wrap this in an IFDEF ?
		if (_battleGame->getCurrentAction()->targeting)
		{
			_battleGame->cancelCurrentAction();
			return;
		}

		_battleGame->cancelCurrentAction();

		_save->getSelectedUnit()->setActiveLeftHand();
		_map->draw();

		bool rightClick = _game->isRightClick(action, true);
		if (rightClick)
		{
			_save->getSelectedUnit()->toggleLeftHandForReactions();
			return;
		}

		BattleItem *leftHandItem = _save->getSelectedUnit()->getLeftHandWeapon();
		if (!leftHandItem)
		{
			auto typesToCheck = {BT_MELEE, BT_PSIAMP, BT_FIREARM, BT_MEDIKIT, BT_SCANNER, BT_MINDPROBE};
			for (auto &type : typesToCheck)
			{
				leftHandItem = _save->getSelectedUnit()->getSpecialWeapon(type);
				if (leftHandItem && leftHandItem->getRules()->isSpecialUsingEmptyHand())
				{
					break;
				}
				leftHandItem = 0;
			}
		}
		bool middleClick = _game->isMiddleClick(action, true);
		handleItemClick(leftHandItem, middleClick);
	}
}

/**
 * Shows an action popup menu. When clicked, create the action.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnRightHandItemClick(Action *action)
{
	if (playableUnitSelected())
	{
		// concession for touch devices:
		// click on the item to cancel action, and don't pop up a menu to select a new one
		// TODO: wrap this in an IFDEF ?
		if (_battleGame->getCurrentAction()->targeting)
		{
			_battleGame->cancelCurrentAction();
			return;
		}

		_battleGame->cancelCurrentAction();

		_save->getSelectedUnit()->setActiveRightHand();
		_map->draw();

		bool rightClick = _game->isRightClick(action, true);
		if (rightClick)
		{
			_save->getSelectedUnit()->toggleRightHandForReactions();
			return;
		}

		BattleItem *rightHandItem = _save->getSelectedUnit()->getRightHandWeapon();
		if (!rightHandItem)
		{
			auto typesToCheck = {BT_MELEE, BT_PSIAMP, BT_FIREARM, BT_MEDIKIT, BT_SCANNER, BT_MINDPROBE};
			for (auto &type : typesToCheck)
			{
				rightHandItem = _save->getSelectedUnit()->getSpecialWeapon(type);
				if (rightHandItem && rightHandItem->getRules()->isSpecialUsingEmptyHand())
				{
					break;
				}
				rightHandItem = 0;
			}
		}
		bool middleClick = _game->isMiddleClick(action, true);
		handleItemClick(rightHandItem, middleClick);
	}
}

/**
 * Centers on the unit corresponding to this button.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnVisibleUnitClick(Action *action)
{
	int btnID = -1;

	// got to find out which button was pressed
	for (int i = 0; i < VISIBLE_MAX && btnID == -1; ++i)
	{
		if (action->getSender() == _btnVisibleUnit[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		auto position = _visibleUnit[btnID]->getPosition();
		if (position == TileEngine::invalid)
		{
			bool found = false;
			for (auto& unit : *_save->getUnits())
			{
				if (!unit->isOut())
				{
					for (auto& invItem : *unit->getInventory())
					{
						if (invItem->getUnit() && invItem->getUnit() == _visibleUnit[btnID])
						{
							position = unit->getPosition(); // position of a unit that has the wounded unit in the inventory
							found = true;
							break;
						}
					}
				}
				if (found) break;
			}
		}
		_map->getCamera()->centerOnPosition(position);
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}


void BattlescapeState::btnCtrlClick(Action* action)
{
	if (allowButtons())
	{
		_game->toggleCtrlPressedFlag();
		_btnCtrl->toggle(_game->getCtrlPressedFlag());
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

void BattlescapeState::btnAltClick(Action* action)
{
	if (allowButtons())
	{
		_game->toggleAltPressedFlag();
		_btnAlt->toggle(_game->getAltPressedFlag());
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

void BattlescapeState::btnShiftClick(Action* action)
{
	if (allowButtons())
	{
		_game->toggleShiftPressedFlag();
		_btnShift->toggle(_game->getShiftPressedFlag());
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

void BattlescapeState::btnRMBClick(Action* action)
{
	if (allowButtons())
	{
		_game->toggleRMBFlag();
		_btnRMB->toggle(_game->getRMBFlag());
		if (_game->getRMBFlag() && _game->getMMBFlag())
		{
			_game->toggleMMBFlag();
			_btnMMB->toggle(_game->getMMBFlag());
		}
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

void BattlescapeState::btnMMBClick(Action* action)
{
	if (allowButtons())
	{
		_game->toggleMMBFlag();
		_btnMMB->toggle(_game->getMMBFlag());
		if (_game->getRMBFlag() && _game->getMMBFlag())
		{
			_game->toggleRMBFlag();
			_btnRMB->toggle(_game->getRMBFlag());
		}
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Toggles touch buttons.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnTouchButtonsClick(Action *)
{
	if (allowButtons())
		toggleTouchButtons(false, false);
}

void BattlescapeState::toggleTouchButtons(bool deactivate, bool tryToReactivate)
{
	// Reset touch flags
	_game->resetTouchButtonFlags();

	// Reset touch buttons
	_btnCtrl->toggle(_game->getCtrlPressedFlag());
	_btnAlt->toggle(_game->getAltPressedFlag());
	_btnShift->toggle(_game->getShiftPressedFlag());
	_btnRMB->toggle(_game->getRMBFlag());
	_btnMMB->toggle(_game->getMMBFlag());

	if (tryToReactivate)
	{
		_touchButtonsEnabled = _touchButtonsEnabledLastTurn;
		_touchButtonsEnabledLastTurn = false;
	}
	else if (deactivate)
	{
		_touchButtonsEnabledLastTurn = _touchButtonsEnabled;
		_touchButtonsEnabled = false;
	}
	else
	{
		_touchButtonsEnabled = !_touchButtonsEnabled;
	}

	_btnCtrl->setVisible(_touchButtonsEnabled);
	_btnAlt->setVisible(_touchButtonsEnabled);
	_btnShift->setVisible(_touchButtonsEnabled);
	_btnRMB->setVisible(_touchButtonsEnabled);
	_btnMMB->setVisible(_touchButtonsEnabled);
}


/**
 * Launches the blaster bomb.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnLaunchClick(Action *action)
{
	_battleGame->launchAction();
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Uses psionics.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPsiClick(Action *action)
{
	_battleGame->psiButtonAction();
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Shows action menu for special weapons.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnSpecialClick(Action *action)
{
	if (playableUnitSelected())
	{
		// concession for touch devices:
		// click on the item to cancel action, and don't pop up a menu to select a new one
		// TODO: wrap this in an IFDEF ?
		if (_battleGame->getCurrentAction()->targeting)
		{
			_battleGame->cancelCurrentAction();
			return;
		}

		_battleGame->cancelCurrentAction();

		BattleType type;
		BattleItem *specialItem = _save->getSelectedUnit()->getSpecialIconWeapon(type);
		if (!specialItem)
		{
			// Note: this is a hack to access the soldier skills button via the same hotkey as the special weapon button
			if (_btnSkills->getVisible())
			{
				btnSkillsClick(action);
			}
			return;
		}

		_map->draw();
		bool middleClick = _game->isMiddleClick(action, true);
		handleItemClick(specialItem, middleClick);
	}
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Shows action menu for the skills feature.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnSkillsClick(Action *action)
{
	if (playableUnitSelected() && !_battleGame->isBusy())
	{
		popup(new SkillMenuState(_battleGame->getCurrentAction(), _icons->getX(), _icons->getY() + 16));
	}
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Reserves time units.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReserveClick(Action *action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);

		if (_reserve == _btnReserveNone)
			_battleGame->setTUReserved(BA_NONE);
		else if (_reserve == _btnReserveSnap)
			_battleGame->setTUReserved(BA_SNAPSHOT);
		else if (_reserve == _btnReserveAimed)
			_battleGame->setTUReserved(BA_AIMEDSHOT);
		else if (_reserve == _btnReserveAuto)
			_battleGame->setTUReserved(BA_AUTOSHOT);

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
}

/**
 * Reloads the weapon in hand.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReloadClick(Action *)
{
	if (playableUnitSelected() && _save->getSelectedUnit()->reloadAmmo())
	{
		_game->getMod()->getSoundByDepth(_save->getDepth(), _save->getSelectedUnit()->getReloadSound())->play(-1, getMap()->getSoundAngle(_save->getSelectedUnit()->getPosition()));
		updateSoldierInfo();
	}
}

/**
 * Opens the jukebox.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnSelectMusicTrackClick(Action *)
{
	if (allowButtons())
	{
		_game->pushState(new SelectMusicTrackState(SMT_BATTLESCAPE));
	}
}

/**
 * Toggles soldier's personal lighting.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPersonalLightingClick(Action *)
{
	if (allowButtons())
		_save->getTileEngine()->togglePersonalLighting();
}

/**
 * Toggles night vision (purely cosmetic).
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNightVisionClick(Action *action)
{
	if (allowButtons())
		_map->toggleNightVision();
}

/**
 * Determines whether a playable unit is selected. Normally only player side units can be selected, but in debug mode one can play with aliens too :)
 * Is used to see if action buttons will work.
 * @return Whether a playable unit is selected.
 */
bool BattlescapeState::playableUnitSelected()
{
	return _save->getSelectedUnit() != 0 && allowButtons();
}

/**
 * Draw hand item with ammo number.
 */
void BattlescapeState::drawItem(BattleItem* item, Surface* hand, std::vector<NumberText*> &ammoText, std::vector<NumberText*> &medikitText, NumberText *twoHandedText, bool drawReactionIndicator)
{
	hand->clear();
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		ammoText[slot]->setVisible(false);
	}
	for (int slot = 0; slot < RuleItem::MedikitSlots; ++slot)
	{
		medikitText[slot]->setVisible(false);
	}
	twoHandedText->setVisible(false);
	if (item)
	{
		const RuleItem *rule = item->getRules();
		rule->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), hand, item, _save, _save->getAnimFrame());
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			if (item->isAmmoVisibleForSlot(slot))
			{
				auto ammo = item->getAmmoForSlot(slot);
				if (!ammo)
				{
					ammoText[slot]->setVisible(true);
					ammoText[slot]->setValue(0);
				}
				else
				{
					ammoText[slot]->setVisible(true);
					ammoText[slot]->setValue(ammo->getAmmoQuantity());
				}
			}
		}
		twoHandedText->setVisible(rule->isTwoHanded());
		twoHandedText->setColor(rule->isBlockingBothHands() ? _twoHandedRed : _twoHandedGreen);
		if (rule->getBattleType() == BT_MEDIKIT)
		{
			medikitText[0]->setVisible(true);
			medikitText[0]->setValue(item->getPainKillerQuantity());
			medikitText[1]->setVisible(true);
			medikitText[1]->setValue(item->getStimulantQuantity());
			medikitText[2]->setVisible(true);
			medikitText[2]->setValue(item->getHealQuantity());
		}

		// primed grenade indicator (static)
		/*
		if (item->getFuseTimer() >= 0)
		{
			Surface *tempSurface = _game->getMod()->getSurfaceSet("SCANG.DAT")->getFrame(6);
			tempSurface->setX((RuleInventory::HAND_W - rule->getInventoryWidth()) * RuleInventory::SLOT_W / 2);
			tempSurface->setY((RuleInventory::HAND_H - rule->getInventoryHeight()) * RuleInventory::SLOT_H / 2);
			tempSurface->blit(hand);
		}
		*/
		// primed grenade indicator (animated)
		if (item->getFuseTimer() >= 0)
		{
			const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
			Surface *tempSurface = _game->getMod()->getSurfaceSet("SCANG.DAT")->getFrame(6);
			int x = (RuleInventory::HAND_W - rule->getInventoryWidth()) * RuleInventory::SLOT_W / 2;
			int y = (RuleInventory::HAND_H - rule->getInventoryHeight()) * RuleInventory::SLOT_H / 2;
			tempSurface->blitNShade(hand, x, y, Pulsate[_save->getAnimFrame() % 8], false, item->isFuseEnabled() ? 0 : 32);
		}
	}
	if (drawReactionIndicator)
	{
 		if (Surface* reactionIndicator = _game->getMod()->getSurface("reactionIndicator", false))
		{
			reactionIndicator->blitNShade(hand, 0, 0);
		}
		else
		{
			Surface* tempSurface = _game->getMod()->getSurfaceSet("SCANG.DAT")->getFrame(0);
			tempSurface->blitNShade(hand, 28, 0);
		}
	}
}

/**
 * Draw both hands sprites.
 */
void BattlescapeState::drawHandsItems()
{
	BattleUnit *battleUnit = _battleGame->playableUnitSelected() ? _save->getSelectedUnit() : nullptr;
	bool left = false;
	bool right = false;
	BattleItem* leftHandItem = nullptr;
	BattleItem* rightHandItem = nullptr;
	if (battleUnit)
	{
		left = battleUnit->isLeftHandPreferredForReactions();
		right = battleUnit->isRightHandPreferredForReactions();
		leftHandItem = battleUnit->getLeftHandWeapon();
		rightHandItem = battleUnit->getRightHandWeapon();
		if (!leftHandItem || !rightHandItem)
		{
			// even if both hands are empty, draw the special item just in one hand
			BattleItem** emptyHandItemPtr = leftHandItem ? &rightHandItem : &leftHandItem;
			auto typesToCheck = { BT_MELEE, BT_PSIAMP, BT_FIREARM, BT_MEDIKIT, BT_SCANNER, BT_MINDPROBE };
			for (auto& type : typesToCheck)
			{
				*emptyHandItemPtr = battleUnit->getSpecialWeapon(type);
				if (*emptyHandItemPtr && (*emptyHandItemPtr)->getRules()->showSpecialInEmptyHand() && (*emptyHandItemPtr)->getRules()->isSpecialUsingEmptyHand())
				{
					break;
				}
				*emptyHandItemPtr = nullptr;
			}
		}
	}
	drawItem(leftHandItem, _btnLeftHandItem, _numAmmoLeft, _numMedikitLeft, _numTwoHandedIndicatorLeft, left);
	drawItem(rightHandItem, _btnRightHandItem, _numAmmoRight, _numMedikitRight, _numTwoHandedIndicatorRight, right);
}

/**
 * Updates a soldier's name/rank/tu/energy/health/morale.
 */
void BattlescapeState::updateSoldierInfo(bool checkFOV)
{
	BattleUnit *battleUnit = _save->getSelectedUnit();

	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);
		_visibleUnit[i] = 0;
	}

	bool playableUnit = _battleGame->playableUnitSelected();
	_rank->setVisible(playableUnit);
	_rankTiny->setVisible(playableUnit);
	_numTimeUnits->setVisible(playableUnit);
	_barTimeUnits->setVisible(playableUnit);
	_barTimeUnits->setVisible(playableUnit);
	_numEnergy->setVisible(playableUnit);
	_barEnergy->setVisible(playableUnit);
	_barEnergy->setVisible(playableUnit);
	_numHealth->setVisible(playableUnit);
	_barHealth->setVisible(playableUnit);
	_barHealth->setVisible(playableUnit);
	_numMorale->setVisible(playableUnit);
	_barMorale->setVisible(playableUnit);
	_barMorale->setVisible(playableUnit);
	if (_manaBarVisible)
	{
		_barMana->setVisible(playableUnit);
	}
	_btnLeftHandItem->setVisible(playableUnit);
	_btnRightHandItem->setVisible(playableUnit);

	drawHandsItems();

	if (!playableUnit)
	{
		_txtName->setText("");
		resetUiButton();
		toggleKneelButton(0);
		return;
	}

	_txtName->setText(battleUnit->getName(_game->getLanguage(), false));
	Soldier *soldier = battleUnit->getGeoscapeSoldier();
	if (soldier != 0)
	{
		if (soldier->hasCallsign() && !_save->isNameDisplay())
		{
			_txtName->setText(soldier->getCallsign());
		}
		// presence of custom background determines what should happen
		Surface *customBg = _game->getMod()->getSurface("AvatarBackground", false);
		if (customBg == 0)
		{
			// show rank (vanilla behaviour)
			SurfaceSet *texture = _game->getMod()->getSurfaceSet("SMOKE.PCK");
			auto frame = texture->getFrame(soldier->getRankSpriteBattlescape());
			if (frame)
			{
				frame->blitNShade(_rank, 0, 0);
			}
		}
		else
		{
			// show tiny rank (modded)
			SurfaceSet *texture = _game->getMod()->getSurfaceSet("TinyRanks");
			Surface *spr = texture->getFrame(soldier->getRankSpriteTiny());
			if (spr)
			{
				spr->blitNShade(_rankTiny, 0, 0);
			}

			// use custom background (modded)
			customBg->blitNShade(_rank, 0, 0);

			// show avatar
			Armor *customArmor = soldier->getArmor();
			if (!soldier->getRules()->getArmorForAvatar().empty())
			{
				customArmor = _game->getMod()->getArmor(soldier->getRules()->getArmorForAvatar());
			}
			if (customArmor->hasLayersDefinition())
			{
				for (const auto& layer : soldier->getArmorLayers(customArmor))
				{
					auto surf = _game->getMod()->getSurface(layer, true);

					auto crop = surf->getCrop();
					crop.getCrop()->x = soldier->getRules()->getAvatarOffsetX();
					crop.getCrop()->y = soldier->getRules()->getAvatarOffsetY();
					crop.getCrop()->w = 26;
					crop.getCrop()->h = 23;

					crop.blit(_rank);
				}
			}
			else
			{
				std::string look = soldier->getArmor()->getSpriteInventory();
				if (!soldier->getRules()->getArmorForAvatar().empty())
				{
					look = _game->getMod()->getArmor(soldier->getRules()->getArmorForAvatar())->getSpriteInventory();
				}
				const std::string gender = soldier->getGender() == GENDER_MALE ? "M" : "F";
				std::stringstream ss;
				Surface *surf = 0;

				for (int i = 0; i <= RuleSoldier::LookVariantBits; ++i)
				{
					ss.str("");
					ss << look;
					ss << gender;
					ss << (int)soldier->getLook() + (soldier->getLookVariant() & (RuleSoldier::LookVariantMask >> i)) * 4;
					ss << ".SPK";
					surf = _game->getMod()->getSurface(ss.str(), false);
					if (surf)
					{
						break;
					}
				}
				if (!surf)
				{
					ss.str("");
					ss << look;
					ss << ".SPK";
					surf = _game->getMod()->getSurface(ss.str(), false);
				}
				if (!surf)
				{
					surf = _game->getMod()->getSurface(look, true);
				}

				// crop
				auto crop = surf->getCrop();
				crop.getCrop()->x = soldier->getRules()->getAvatarOffsetX();
				crop.getCrop()->y = soldier->getRules()->getAvatarOffsetY();
				crop.getCrop()->w = 26;
				crop.getCrop()->h = 23;

				crop.blit(_rank);
			}
		}
	}
	else
	{
		_rank->clear();
		_rankTiny->clear();
	}
	_numTimeUnits->setValue(battleUnit->getTimeUnits());
	_barTimeUnits->setMax(battleUnit->getBaseStats()->tu);
	_barTimeUnits->setValue(battleUnit->getTimeUnits());
	_numEnergy->setValue(battleUnit->getEnergy());
	_barEnergy->setMax(battleUnit->getBaseStats()->stamina);
	_barEnergy->setValue(battleUnit->getEnergy());
	_numHealth->setValue(battleUnit->getHealth());
	_barHealth->setMax(battleUnit->getBaseStats()->health);
	_barHealth->setValue(battleUnit->getHealth());
	_barHealth->setValue2(battleUnit->getStunlevel());
	_numMorale->setValue(battleUnit->getMorale());
	_barMorale->setMax(100);
	_barMorale->setValue(battleUnit->getMorale());
	if (_manaBarVisible)
	{
		_barMana->setMax(battleUnit->getBaseStats()->mana);
		_barMana->setValue(battleUnit->getMana());
	}

	toggleKneelButton(battleUnit);

	if (checkFOV)
	{
		_save->getTileEngine()->calculateFOV(_save->getSelectedUnit());
	}

	// go through all units visible to the selected soldier (or other unit, e.g. mind-controlled enemy)
	int j = 0;
	for (std::vector<BattleUnit*>::iterator i = battleUnit->getVisibleUnits()->begin(); i != battleUnit->getVisibleUnits()->end() && j < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[j]);
		_btnVisibleUnit[j]->setVisible(true);
		_numVisibleUnit[j]->setVisible(true);
		_visibleUnit[j] = (*i);
		++j;
	}

	// remember where red indicators turn green
	_numberOfDirectlyVisibleUnits = j;

	// go through all units on the map
	// FOR HUMAN_PLAYER (HOST)
	for (std::vector<BattleUnit *>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end() && _save->getSide() == FACTION_PLAYER && j < VISIBLE_MAX; ++i)
	{
		// check if they are hostile and visible (by any friendly unit)
		if (((*i)->getOriginalFaction() == FACTION_HOSTILE || (*i)->getOriginalFaction() == FACTION_ALIEN_PLAYER) && !(*i)->isOut() && (*i)->getVisible())
		{
			bool alreadyShown = false;
			// check if they are not already shown (e.g. because we see them directly)
			for (std::vector<BattleUnit*>::iterator k = battleUnit->getVisibleUnits()->begin(); k != battleUnit->getVisibleUnits()->end(); ++k)
			{
				if ((*i)->getId() == (*k)->getId())
				{
					alreadyShown = true;
				}
			}
			if (!alreadyShown)
			{
				_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[j]);
				_btnVisibleUnit[j]->setVisible(true);
				_numVisibleUnit[j]->setVisible(true);
				_visibleUnit[j] = (*i);
				++j;
			}
		}
	}
	// FOR ALEIN_PLAYER (CLIENT)
	for (std::vector<BattleUnit *>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end() && _save->getSide() == FACTION_ALIEN_PLAYER && j < VISIBLE_MAX; ++i)
	{
		// check if they are hostile and visible (by any friendly unit)
		if (((*i)->getOriginalFaction() == FACTION_PLAYER) && !(*i)->isOut() && (*i)->getVisible())
		{
			bool alreadyShown = false;
			// check if they are not already shown (e.g. because we see them directly)
			for (std::vector<BattleUnit *>::iterator k = battleUnit->getVisibleUnits()->begin(); k != battleUnit->getVisibleUnits()->end(); ++k)
			{
				if ((*i)->getId() == (*k)->getId())
				{
					alreadyShown = true;
				}
			}
			if (!alreadyShown)
			{
				_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[j]);
				_btnVisibleUnit[j]->setVisible(true);
				_numVisibleUnit[j]->setVisible(true);
				_visibleUnit[j] = (*i);
				++j;
			}
		}
	}

	// remember where green indicators turn blue
	_numberOfEnemiesTotal = j;

	{   // FOR HUMAN_PLAYER (HOST)
		// go through all wounded units under player's control (incl. unconscious)
		for (std::vector<BattleUnit *>::iterator i = _battleGame->getSave()->getUnits()->begin(); i != _battleGame->getSave()->getUnits()->end() && _save->getSide() == FACTION_PLAYER && j < VISIBLE_MAX; ++i)
		{
			if ((*i)->getFaction() == FACTION_PLAYER && (*i)->getStatus() != STATUS_DEAD && !(*i)->isIgnored() && (*i)->getFatalWounds() > 0 && (*i)->indicatorsAreEnabled())
			{
				_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[VISIBLE_MAX]);
				_btnVisibleUnit[j]->setVisible(true);
				_numVisibleUnit[j]->setVisible(true);
				_visibleUnit[j] = (*i);
				++j;
			}
		}
		// FOR ALEIN_PLAYER (CLIENT)
		// go through all wounded units under player's control (incl. unconscious)
		for (std::vector<BattleUnit *>::iterator i = _battleGame->getSave()->getUnits()->begin(); i != _battleGame->getSave()->getUnits()->end() && _save->getSide() == FACTION_ALIEN_PLAYER && j < VISIBLE_MAX; ++i)
		{
			if ((*i)->getFaction() == FACTION_ALIEN_PLAYER && (*i)->getStatus() != STATUS_DEAD && !(*i)->isIgnored() && (*i)->getFatalWounds() > 0 && (*i)->indicatorsAreEnabled())
			{
				_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[VISIBLE_MAX]);
				_btnVisibleUnit[j]->setVisible(true);
				_numVisibleUnit[j]->setVisible(true);
				_visibleUnit[j] = (*i);
				++j;
			}
		}
	}

	// remember where blue indicators turn purple
	_numberOfEnemiesTotalPlusWounded = j;

	{
		// FOR HUMAN_PLAYER (HOST)
		// first show all stunned allies with negative health regen (usually caused by high stun level)
		for (std::vector<BattleUnit*>::iterator i = _battleGame->getSave()->getUnits()->begin(); i != _battleGame->getSave()->getUnits()->end() && j < VISIBLE_MAX; ++i)
		{
			if ((*i)->getOriginalFaction() == FACTION_PLAYER && (*i)->getStatus() == STATUS_UNCONSCIOUS && (*i)->hasNegativeHealthRegen() && (*i)->indicatorsAreEnabled())
			{
				_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[VISIBLE_MAX + 1]);
				_btnVisibleUnit[j]->setVisible(true);
				_numVisibleUnit[j]->setVisible(true);
				_visibleUnit[j] = (*i);
				++j;
			}
		}
		// FOR ALEIN_PLAYER (CLIENT)
		for (std::vector<BattleUnit *>::iterator i = _battleGame->getSave()->getUnits()->begin(); i != _battleGame->getSave()->getUnits()->end() && j < VISIBLE_MAX; ++i)
		{
			if ((*i)->getOriginalFaction() == FACTION_ALIEN_PLAYER && (*i)->getStatus() == STATUS_UNCONSCIOUS && (*i)->hasNegativeHealthRegen() && (*i)->indicatorsAreEnabled())
			{
				_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[VISIBLE_MAX + 1]);
				_btnVisibleUnit[j]->setVisible(true);
				_numVisibleUnit[j]->setVisible(true);
				_visibleUnit[j] = (*i);
				++j;
			}
		}
		// FOR HUMAN_PLAYER (HOST)
		// then show all standing units under player's control with high stun level
		for (std::vector<BattleUnit*>::iterator i = _battleGame->getSave()->getUnits()->begin(); i != _battleGame->getSave()->getUnits()->end() && j < VISIBLE_MAX; ++i)
		{
			if ((*i)->getFaction() == FACTION_PLAYER && !((*i)->isOut()) && (*i)->getHealth() > 0 && (*i)->indicatorsAreEnabled())
			{
				if ((*i)->getStunlevel() * 100 / (*i)->getHealth() >= 75)
				{
					_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[VISIBLE_MAX+1]);
					_btnVisibleUnit[j]->setVisible(true);
					_numVisibleUnit[j]->setVisible(true);
					_visibleUnit[j] = (*i);
					++j;
				}
			}
		}
		// FOR ALEIN_PLAYER (CLIENT)
		for (std::vector<BattleUnit *>::iterator i = _battleGame->getSave()->getUnits()->begin(); i != _battleGame->getSave()->getUnits()->end() && j < VISIBLE_MAX; ++i)
		{
			if ((*i)->getFaction() == FACTION_ALIEN_PLAYER && !((*i)->isOut()) && (*i)->getHealth() > 0 && (*i)->indicatorsAreEnabled())
			{
				if ((*i)->getStunlevel() * 100 / (*i)->getHealth() >= 75)
				{
					_btnVisibleUnit[j]->setTooltip(_txtVisibleUnitTooltip[VISIBLE_MAX + 1]);
					_btnVisibleUnit[j]->setVisible(true);
					_numVisibleUnit[j]->setVisible(true);
					_visibleUnit[j] = (*i);
					++j;
				}
			}
		}
	}

	updateUiButton(battleUnit);
}

void BattlescapeState::updateUiButton(const BattleUnit *battleUnit)
{
	BattleItem *psiWeapon = battleUnit->getSpecialWeapon(BT_PSIAMP);

	BattleType type = BT_NONE;
	BattleItem *specialWeapon = battleUnit->getSpecialIconWeapon(type); // updates type!
	bool hasSpecialWeapon = specialWeapon && type != BT_NONE && type != BT_AMMO && type != BT_GRENADE && type != BT_PROXIMITYGRENADE && type != BT_FLARE && type != BT_CORPSE;

	// if we have psi amp with icon then it will show one button only, but if we have two psi amps and one with icon is second (this is important) then we will show both buttons.
	bool hasPsiWeapon = psiWeapon != 0 && psiWeapon != specialWeapon;

	bool hasSkills = false;
	auto soldier = battleUnit->getGeoscapeSoldier();
	if (soldier)
	{
		hasSkills = soldier->getRules()->isSkillMenuDefined();
	}

	resetUiButton();

	int offset = 0;
	auto show = [&](BattlescapeButton* btn, int spriteIndex)
	{
		if (offset < SPECIAL_BUTTONS_MAX)
		{
			_game->getMod()->getSurfaceSet("SPICONS.DAT")->getFrame(spriteIndex)->blitNShade(btn, 0, 0);
			btn->setVisible(true);
			btn->setX(_posSpecialActions[offset]);
			++offset;
		}
	};


	if (hasSpecialWeapon)
	{
		show(_btnSpecial, specialWeapon->getRules()->getSpecialIconSprite());
	}
	if (hasSkills)
	{
		show(_btnSkills, soldier->getRules()->getSkillIconSprite());
	}
	if (hasPsiWeapon)
	{
		show(_btnPsi, 1);
	}
}

void BattlescapeState::resetUiButton()
{
	BattlescapeButton* btns[] = {
		_btnPsi,
		_btnSkills,
		_btnSpecial,
	};

	for (auto* btn : btns)
	{
		btn->setVisible(false);
		btn->setX(_posSpecialActions[0]);
	}
}

/**
 * Shifts the red colors of the visible unit buttons backgrounds.
 */
void BattlescapeState::blinkVisibleUnitButtons()
{
	static int delta = 1, color = 32;

	for (int i = 0; i < VISIBLE_MAX;  ++i)
	{
		if (_btnVisibleUnit[i]->getVisible() == true)
		{
			_btnVisibleUnit[i]->drawRect(0, 0, 15, 12, 15);
			int bgColor = i < _numberOfDirectlyVisibleUnits ? color : i < _numberOfEnemiesTotal ? _indicatorGreen : i < _numberOfEnemiesTotalPlusWounded ? _indicatorBlue : _indicatorPurple;
			_btnVisibleUnit[i]->drawRect(1, 1, 13, 10, bgColor);
		}
	}

	if (color == 44) delta = -2;
	if (color == 32) delta = 1;

	color += delta;
}

/**
 * Shifts the colors of the health bar when unit has fatal wounds.
 */
void BattlescapeState::blinkHealthBar()
{
	static Uint8 color = 0, maxcolor = 3, step = 0;

	step = 1 - step;	// 1, 0, 1, 0, ...
	BattleUnit *bu = _save->getSelectedUnit();
	if (step == 0 || bu == 0 || !_barHealth->getVisible()) return;

	if (++color > maxcolor) color = maxcolor - 3;

	for (int i = 0; i < BODYPART_MAX; i++)
	{
		if (bu->getFatalWound((UnitBodyPart)i) > 0)
		{
			_barHealth->setColor(_barHealthColor + color);
			return;
		}
	}
	if (_barHealth->getColor() != _barHealthColor) // avoid redrawing if we don't have to
		_barHealth->setColor(_barHealthColor);
}

/**
 * Popups a context sensitive list of actions the user can choose from.
 * Some actions result in a change of gamestate.
 * @param item Item the user clicked on (righthand/lefthand)
 * @param middleClick was it a middle click?
 */
void BattlescapeState::handleItemClick(BattleItem *item, bool middleClick)
{
	// make sure there is an item, and the battlescape is in an idle state
	if (item && !_battleGame->isBusy())
	{
		if (middleClick)
		{
			std::string articleId = item->getRules()->getType();
			Ufopaedia::openArticle(_game, articleId);
		}
		else
		{
			_battleGame->getCurrentAction()->weapon = item;
			popup(new ActionMenuState(_battleGame->getCurrentAction(), _icons->getX(), _icons->getY() + 16));
			if (item->getRules()->getBattleType() == BT_FIREARM)
			{
				_battleGame->playUnitResponseSound(_battleGame->getCurrentAction()->actor, 2); // "select weapon" sound
			}
		}
	}
}

/**
 * Animates map objects on the map, also smoke,fire, ...
 */
void BattlescapeState::animate()
{
	_map->animate(!_battleGame->isBusy());

	blinkVisibleUnitButtons();
	blinkHealthBar();

	if (!_map->getProjectile())
	{
		drawHandsItems();
	}
}

/**
 * Handles the battle game state.
 */
void BattlescapeState::handleState()
{
	_battleGame->handleState();
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval An interval in ms.
 */
void BattlescapeState::setStateInterval(Uint32 interval)
{
	_gameTimer->setInterval(interval);
}

/**
 * Gets pointer to the game. Some states need this info.
 * @return Pointer to game.
 */
Game *BattlescapeState::getGame() const
{
	return _game;
}

/**
 * Gets pointer to the map. Some states need this info.
 * @return Pointer to map.
 */
Map *BattlescapeState::getMap() const
{
	return _map;
}

/**
 * Shows a debug message in the topleft corner.
 * @param message Debug message.
 */
void BattlescapeState::debug(const std::string &message)
{
	if (_save->getDebugMode())
	{
		_txtDebug->setText(message);
	}
}

/**
* Shows a bug hunt message in the topleft corner.
*/
void BattlescapeState::bugHuntMessage()
{
	if (_save->getBughuntMode())
	{
		_txtDebug->setText(tr("STR_BUG_HUNT_ACTIVATED"));
	}
}

/**
 * Shows a warning message.
 * @param message Warning message.
 */
void BattlescapeState::warning(const std::string &message)
{
	_warning->showMessage(tr(message));
}

/**
 * Shows a warning message without automatic translation.
 * @param message Warning message.
 */
void BattlescapeState::warningRaw(const std::string &message)
{
	_warning->showMessage(message);
}

/**
 * Shows a warning message without automatic translation.
 * @param message Warning message.
 */
void BattlescapeState::warningLongRaw(const std::string &message)
{
	_warning->showMessage(message, 8);
}

/**
 * Gets melee damage preview.
 * @param actor Selected unit.
 * @param weapon Weapon to use for calculation.
 */
std::string BattlescapeState::getMeleeDamagePreview(BattleUnit *actor, BattleItem *weapon) const
{
	if (!weapon)
		return "";

	bool discovered = false;
	if (_game->getSavedGame()->getMonthsPassed() == -1)
	{
		discovered = true; // new battle mode
	}
	else
	{
		ArticleDefinition *article = _game->getMod()->getUfopaediaArticle(weapon->getRules()->getType(), false);
		if (article && Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
		{
			discovered = true; // pedia article unlocked
		}
	}

	std::ostringstream ss;
	if (discovered)
	{
		int totalDamage = 0;
		const RuleDamageType *dmgType;
		auto attack = BattleActionAttack::GetBeforeShoot(BA_HIT, actor, weapon);
		if (weapon->getRules()->getBattleType() == BT_MELEE)
		{
			totalDamage += weapon->getRules()->getPowerBonus(attack);
			dmgType = weapon->getRules()->getDamageType();
		}
		else
		{
			totalDamage += weapon->getRules()->getMeleeBonus(attack);
			dmgType = weapon->getRules()->getMeleeType();
		}

		ss << tr(weapon->getRules()->getType());
		ss << "\n";
		ss << dmgType->getRandomDamage(totalDamage, 1);
		ss << "-";
		ss << dmgType->getRandomDamage(totalDamage, 2);
		if (dmgType->RandomType == DRT_UFO_WITH_TWO_DICE)
			ss << "*";
	}
	else
	{
		ss << tr(weapon->getRules()->getType());
		ss << "\n?-?";
	}

	return ss.str();
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
inline void BattlescapeState::handle(Action *action)
{
	if (!_firstInit)
	{
		if (_game->getCursor()->getVisible() || ((action->getDetails()->type == SDL_MOUSEBUTTONDOWN || action->getDetails()->type == SDL_MOUSEBUTTONUP) && _game->isRightClick(action)))
		{
			State::handle(action);

			if (Options::touchEnabled == false && _isMouseScrolling && !Options::battleDragScrollInvert)
			{
				_map->setSelectorPosition((_cursorPosition.x - _game->getScreen()->getCursorLeftBlackBand()) / action->getXScale(), (_cursorPosition.y - _game->getScreen()->getCursorTopBlackBand()) / action->getYScale());
			}

			if (Options::oxceThumbButtons && action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
			{
				if (action->getDetails()->button.button == SDL_BUTTON_X1)
				{
					btnNextSoldierClick(action);
				}
				else if (action->getDetails()->button.button == SDL_BUTTON_X2)
				{
					btnPrevSoldierClick(action);
				}
			}

			if (action->getDetails()->type == SDL_KEYDOWN)
			{
				SDLKey key = action->getDetails()->key.keysym.sym;
				bool ctrlPressed = _game->isCtrlPressed();
				bool shiftPressed = _game->isShiftPressed();
				bool altPressed = _game->isAltPressed();

				// "ctrl-b" - reopen briefing
				if (key == SDLK_b && ctrlPressed)
				{
					_game->pushState(new BriefingState(0, 0, true));
				}
				// "ctrl-h" - show hit log
				else if (key == SDLK_h && ctrlPressed)
				{
					if (_save->getSide() == FACTION_PLAYER || _save->getSide() == FACTION_ALIEN_PLAYER)
					{
						if (Options::oxceDisableHitLog)
						{
							_game->pushState(new InfoboxState(tr("STR_THIS_FEATURE_IS_DISABLED_4")));
						}
						else if (altPressed)
						{
							// turn diary
							_game->pushState(new TurnDiaryState(_save->getHitLog()));
						}
						else
						{
							// hit log
							std::string hitLogText = _save->getHitLog()->getHitLogText();
							if (!hitLogText.empty())
								_game->pushState(new InfoboxState(hitLogText));
						}
					}
				}
				// "ctrl-Home" - reset default palettes
				else if (key == SDLK_HOME && ctrlPressed)
				{
					_paletteResetRequested = true;
					init();
				}
				// "ctrl-End" - toggle between various debug vision/brightness modes
				else if (key == SDLK_END && ctrlPressed)
				{
					_map->toggleDebugVisionMode(); //jopper
				}
				// "ctrl-x" - mute/unmute unit response sounds
				else if (key == SDLK_x && ctrlPressed)
				{
					if (_game->getMod()->getEnableUnitResponseSounds())
					{
						Options::oxceEnableUnitResponseSounds = !Options::oxceEnableUnitResponseSounds;
					}
				}
				// "ctrl-e" - experience log
				else if (key == SDLK_e && ctrlPressed)
				{
					std::ostringstream ss;
					ss << tr("STR_NO_EXPERIENCE_YET");
					ss << "\n\n";
					bool first = true;
					for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
					{
						if ((*i)->getOriginalFaction() == FACTION_PLAYER && !(*i)->isOut())
						{
							if ((*i)->getGeoscapeSoldier() && !(*i)->hasGainedAnyExperience())
							{
								if (!first) ss << ", ";
								ss << (*i)->getName(_game->getLanguage());
								first = false;
							}
						}
					}
					_game->pushState(new InfoboxState(ss.str()));
				}
				// "ctrl-m" - melee damage preview
				else if (key == SDLK_m && ctrlPressed)
				{
					BattleUnit *actor = _save->getSelectedUnit();
					if (actor)
					{
						BattleItem *leftWeapon = actor->getLeftHandWeapon();

						BattleItem *rightWeapon = actor->getRightHandWeapon();

						BattleItem *specialWeapon = 0;
						auto typesToCheck = {BT_MELEE, BT_PSIAMP, BT_FIREARM, BT_MEDIKIT, BT_SCANNER, BT_MINDPROBE};
						for (auto &type : typesToCheck)
						{
							specialWeapon = actor->getSpecialWeapon(type);
							if (specialWeapon && specialWeapon->getRules()->isSpecialUsingEmptyHand())
							{
								break;
							}
							specialWeapon = 0;
						}

						BattleType type;
						BattleItem *anotherSpecialWeapon = actor->getSpecialIconWeapon(type);
						if (anotherSpecialWeapon && anotherSpecialWeapon == specialWeapon)
						{
							anotherSpecialWeapon = 0;
						}

						std::ostringstream ss;
						bool first = true;
						if (leftWeapon)
						{
							ss << getMeleeDamagePreview(actor, leftWeapon);
							first = false;
						}
						if (rightWeapon)
						{
							if (!first) ss << "\n\n";
							ss << getMeleeDamagePreview(actor, rightWeapon);
							first = false;
						}
						if (specialWeapon)
						{
							if (!first) ss << "\n\n";
							ss << getMeleeDamagePreview(actor, specialWeapon);
							first = false;
						}
						if (anotherSpecialWeapon)
						{
							if (!first) ss << "\n\n";
							ss << getMeleeDamagePreview(actor, anotherSpecialWeapon);
							first = false;
						}

						_game->pushState(new InfoboxState(ss.str()));
					}
				}
				// "ctrl-w" - warp unit
				else if ((_save->getDebugMode() || _save->isPreview()) && key == SDLK_w && ctrlPressed)
				{
					BattleUnit *unit = _save->getSelectedUnit();
					if (unit)
					{
						Position newPos;
						_map->getSelectorPosition(&newPos);
						if (_save->getBattleGame()->getTileEngine()->isPositionValidForUnit(newPos, unit))
						{
							debug("Beam me up Scotty");
							_save->getPathfinding()->removePreview();

							unit->setTile(_save->getTile(newPos), _save);
							unit->setPosition(newPos);

							//free refresh as bonus
							unit->updateUnitStats(true, false);
							_save->getTileEngine()->calculateLighting(LL_UNITS);
							_save->getBattleGame()->handleState();
							updateSoldierInfo(true);
						}
					}
				}
				if (Options::debug)
				{
					// "ctrl-d" - enable debug mode
					if (key == SDLK_d && ctrlPressed)
					{
						_save->setDebugMode();
						debug("Debug Mode");
					}
					// "ctrl-v" - reset tile visibility
					else if (_save->getDebugMode() && key == SDLK_v && ctrlPressed)
					{
						debug("Resetting tile visibility");
						_save->resetTiles();
					}
					else if (_save->getDebugMode() && (key == SDLK_k || key == SDLK_j) && ctrlPressed)
					{
						bool stunOnly = (key == SDLK_j);
						BattleUnit *unitUnderTheCursor = nullptr;
						if (shiftPressed || altPressed)
						{
							Position newPos;
							_map->getSelectorPosition(&newPos);
							Tile *tile = _save->getTile(newPos);
							if (tile)
							{
								unitUnderTheCursor = tile->getOverlappingUnit(_save);
							}
						}
						if (shiftPressed)
						{
							// kill (ctrl-shift-k) or stun (ctrl-shift-j) just a single unit (under the cursor)
							if (unitUnderTheCursor && !unitUnderTheCursor->isOut())
							{
								debug("Bingo!");
								unitUnderTheCursor->damage(Position(0, 0, 0), 1000, _game->getMod()->getDamageType(stunOnly ? DT_STUN : DT_AP), _save, {});
							}
						}
						else
						{
							if (stunOnly)
							{
								// "ctrl-j" - stun all aliens
								debug("Deploying Celine Dion album");
							}
							else
							{
								// "ctrl-k" - kill all aliens
								debug("Influenza bacterium dispersed");
							}
							for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
							{
								if (unitUnderTheCursor && unitUnderTheCursor == (*i))
								{
									// kill (ctrl-alt-k) or stun (ctrl-alt-j) all aliens EXCEPT the one under the cursor
									continue;
								}
								if ((*i)->getOriginalFaction() == FACTION_HOSTILE && !(*i)->isOut())
								{
									(*i)->damage(Position(0, 0, 0), 1000, _game->getMod()->getDamageType(stunOnly ? DT_STUN : DT_AP), _save, { });
								}
							}
						}
						_save->getBattleGame()->checkForCasualties(nullptr, BattleActionAttack{}, true, false);
						_save->getBattleGame()->handleState();
					}
					// f11 - voxel map dump
					else if (key == SDLK_F11)
					{
						saveVoxelMap();
					}
					// f9 - ai
					else if (key == SDLK_F9 && Options::traceAI)
					{
						saveAIMap();
					}
				}
				// quick save and quick load
				if (!_game->getSavedGame()->isIronman() && !_save->isPreview())
				{
					if (key == Options::keyQuickSave)
					{
						_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
					}
					else if (key == Options::keyQuickLoad)
					{
						_game->pushState(new LoadGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
					}
				}

				// voxel view dump
				if (key == Options::keyBattleVoxelView)
				{
					saveVoxelView();
				}
			}
		}
	}
}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap()
{
	Uint32 start = SDL_GetTicks();
	BattleUnit *unit = _save->getSelectedUnit();
	if (!unit) return;

	int w = _save->getMapSizeX();
	int h = _save->getMapSizeY();

	SDL_Surface *img = SDL_AllocSurface(0, w * 8, h * 8, 24, 0xff, 0xff00, 0xff0000, 0);
	Log(LOG_INFO) << "unit = " << unit->getId();
	memset(img->pixels, 0, img->pitch * img->h);

	Position tilePos(unit->getPosition());
	SDL_Rect r;
	r.h = 8;
	r.w = 8;

	for (int y = 0; y < h; ++y)
	{
		tilePos.y = y;
		for (int x = 0; x < w; ++x)
		{
			tilePos.x = x;
			Tile *t = _save->getTile(tilePos);

			if (!t) continue;
			if (!t->isDiscovered(O_FLOOR)) continue;

		}
	}

	for (int y = 0; y < h; ++y)
	{
		tilePos.y = y;
		for (int x = 0; x < w; ++x)
		{
			tilePos.x = x;
			Tile *t = _save->getTile(tilePos);

			if (!t) continue;
			if (!t->isDiscovered(O_FLOOR)) continue;

			r.x = x * r.w;
			r.y = y * r.h;

			if (t->getTUCost(O_FLOOR, MT_FLY) != Pathfinding::INVALID_MOVE_COST && t->getTUCost(O_OBJECT, MT_FLY) != Pathfinding::INVALID_MOVE_COST)
			{
				SDL_FillRect(img, &r, SDL_MapRGB(img->format, 255, 0, 0x20));
				characterRGBA(img, r.x, r.y,'*' , 0x7f, 0x7f, 0x7f, 0x7f);
			} else
			{
				if (!t->getUnit()) SDL_FillRect(img, &r, SDL_MapRGB(img->format, 0x50, 0x50, 0x50)); // gray for blocked tile
			}

			for (int z = tilePos.z; z >= 0; --z)
			{
				Position pos(tilePos.x, tilePos.y, z);
				t = _save->getTile(pos);
				BattleUnit *wat = t->getUnit();
				if (wat)
				{
					switch(wat->getFaction())
					{
					case FACTION_HOSTILE:
						// #4080C0 is Volutar Blue
						characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'a' : 'A', 0x40, 0x80, 0xC0, 0xff);
						break;
					case FACTION_ALIEN_PLAYER:
						// #4080C0 is Volutar Blue
						characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'a' : 'A', 0x40, 0x80, 0xC0, 0xff);
						break;
					case FACTION_PLAYER:
						characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'x' : 'X', 255, 255, 127, 0xff);
						break;
					case FACTION_NEUTRAL:
						characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'c' : 'C', 255, 127, 127, 0xff);
						break;
					}
					break;
				}
				pos.z--;
				if (z > 0 && !t->hasNoFloor(_save)) break; // no seeing through floors
			}

			if (t->getMapData(O_NORTHWALL) && t->getMapData(O_NORTHWALL)->getTUCost(MT_FLY) == Pathfinding::INVALID_MOVE_COST)
			{
				lineRGBA(img, r.x, r.y, r.x+r.w, r.y, 0x50, 0x50, 0x50, 255);
			}

			if (t->getMapData(O_WESTWALL) && t->getMapData(O_WESTWALL)->getTUCost(MT_FLY) == Pathfinding::INVALID_MOVE_COST)
			{
				lineRGBA(img, r.x, r.y, r.x, r.y+r.h, 0x50, 0x50, 0x50, 255);
			}
		}
	}

	std::ostringstream ss;

	ss.str("");
	ss << "z = " << tilePos.z;
	stringRGBA(img, 12, 12, ss.str().c_str(), 0, 0, 0, 0x7f);

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getMasterUserFolder() << "AIExposure" << std::setfill('0') << std::setw(3) << i << ".png";
		i++;
	}
	while (CrossPlatform::fileExists(ss.str()));

	std::vector<unsigned char> out;
	unsigned error = lodepng::encode(out, (const unsigned char*)img->pixels, img->w, img->h, LCT_RGB);
	if (error)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

	SDL_FreeSurface(img);

	CrossPlatform::writeFile(ss.str(), out);
	Log(LOG_INFO) << "saveAIMap() completed in " << SDL_GetTicks() - start << "ms.";
}

/**
 * Saves a first-person voxel view of the battlescape.
 */
void BattlescapeState::saveVoxelView()
{
	static const unsigned char pal[30]=
	//			ground		west wall	north wall		object		enemy unit						xcom unit	neutral unit
	{0,0,0, 224,224,224,  192,224,255,  255,224,192, 128,255,128, 192,0,255,  0,0,0, 255,255,255,  224,192,0,  255,64,128 };

	BattleUnit * bu = _save->getSelectedUnit();
	if (bu==0) return; //no unit selected
	std::vector<Position> _trajectory;

	double ang_x,ang_y;
	bool black;
	Tile *tile = 0;
	std::ostringstream ss;
	std::vector<unsigned char> image;
	int test;
	Position originVoxel = getBattleGame()->getTileEngine()->getSightOriginVoxel(bu);

	Position targetVoxel,hitPos;
	double dist = 0;
	bool _debug = _save->getDebugMode();
	double dir = ((double)bu->getDirection()+4)/4*M_PI;
	image.clear();
	for (int y = -256+32; y < 256+32; ++y)
	{
		ang_y = (((double)y)/640*M_PI+M_PI/2);
		for (int x = -256; x < 256; ++x)
		{
			ang_x = ((double)x/1024)*M_PI+dir;

			targetVoxel.x=originVoxel.x + (int)(-sin(ang_x)*1024*sin(ang_y));
			targetVoxel.y=originVoxel.y + (int)(cos(ang_x)*1024*sin(ang_y));
			targetVoxel.z=originVoxel.z + (int)(cos(ang_y)*1024);

			_trajectory.clear();
			test = _save->getTileEngine()->calculateLineVoxel(originVoxel, targetVoxel, false, &_trajectory, bu, nullptr, !_debug) +1;
			black = true;
			if (test!=0 && test!=6)
			{
				tile = _save->getTile(_trajectory.at(0).toTile());
				if (_debug
					|| (tile->isDiscovered(O_WESTWALL) && test == 2)
					|| (tile->isDiscovered(O_NORTHWALL) && test == 3)
					|| (tile->isDiscovered(O_FLOOR) && (test == 1 || test == 4))
					|| test==5
					)
				{
					if (test==5)
					{
						if (tile->getUnit())
						{
							if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
							else
							if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
						}
						else
						{
							tile = _save->getBelowTile(tile);
							if (tile && tile->getUnit())
							{
								if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
								else
								if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
							}
						}
					}
					hitPos =_trajectory.at(0);
					dist = Position::distance(hitPos, originVoxel);
					black = false;
				}
			}

			if (black)
			{
				dist = 0;
			}
			else
			{
				if (dist>1000) dist=1000;
				if (dist<1) dist=1;
				dist=(1000-(log(dist))*140)/700;//140

				if (hitPos.x%16==15)
				{
					dist*=0.9;
				}
				if (hitPos.y%16==15)
				{
					dist*=0.9;
				}
				if (hitPos.z%24==23)
				{
					dist*=0.9;
				}
				if (dist > 1) dist = 1;
				if (tile) dist *= (16 - (double)tile->getShade())/16;
			}

			image.push_back((int)((double)(pal[test*3+0])*dist));
			image.push_back((int)((double)(pal[test*3+1])*dist));
			image.push_back((int)((double)(pal[test*3+2])*dist));
		}
	}

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getMasterUserFolder() << "fpslook" << std::setfill('0') << std::setw(3) << i << ".png";
		i++;
	}
	while (CrossPlatform::fileExists(ss.str()));

	std::vector<unsigned char> out;
	unsigned error = lodepng::encode(out, image, 512, 512, LCT_RGB);
	if (error)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}
	CrossPlatform::writeFile(ss.str(), out);
	return;
}

/**
 * Saves each layer of voxels on the bettlescape as a png.
 */
void BattlescapeState::saveVoxelMap()
{
	std::ostringstream ss;
	std::vector<unsigned char> image;
	static const unsigned char pal[30]=
	{255,255,255, 224,224,224,  128,160,255,  255,160,128, 128,255,128, 192,0,255,  255,255,255, 255,255,255,  224,192,0,  255,64,128 };

	Tile *tile;

	for (int z = 0; z < _save->getMapSizeZ()*12; ++z)
	{
		image.clear();

		for (int y = 0; y < _save->getMapSizeY()*16; ++y)
		{
			for (int x = 0; x < _save->getMapSizeX()*16; ++x)
			{
				int test = _save->getTileEngine()->voxelCheck(Position(x,y,z*2),0,0) +1;
				float dist=1;
				if (x%16==15)
				{
					dist*=0.9f;
				}
				if (y%16==15)
				{
					dist*=0.9f;
				}

				if (test == V_OUTOFBOUNDS)
				{
					tile = _save->getTile(Position(x/16, y/16, z/12));
					if (tile->getUnit())
					{
						if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
						else
						if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
					}
					else
					{
						tile = _save->getBelowTile(tile);
						if (tile && tile->getUnit())
						{
							if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
							else
							if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
						}
					}
				}

				image.push_back((int)((float)pal[test*3+0]*dist));
				image.push_back((int)((float)pal[test*3+1]*dist));
				image.push_back((int)((float)pal[test*3+2]*dist));
			}
		}

		ss.str("");
		ss << Options::getMasterUserFolder() << "voxel" << std::setfill('0') << std::setw(2) << z << ".png";
		std::vector<unsigned char> out;
		unsigned error = lodepng::encode(out, image, _save->getMapSizeX()*16, _save->getMapSizeY()*16, LCT_RGB);
		if (error)
		{
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
		}
		CrossPlatform::writeFile(ss.str(), out);
	}
	return;
}

/**
 * Adds a new popup window to the queue
 * (this prevents popups from overlapping).
 * @param state Pointer to popup state.
 */
void BattlescapeState::popup(State *state)
{
	_popups.push_back(state);
}

/**
 * Finishes up the current battle, shuts down the battlescape
 * and presents the debriefing screen for the mission.
 * @param abort Was the mission aborted?
 * @param inExitArea Number of soldiers in the exit area OR number of survivors when battle finished due to either all aliens or objective being destroyed.
 */
void BattlescapeState::finishBattle(bool abort, int inExitArea)
{
	bool isPreview = _save->isPreview();

	while (!_game->isState(this))
	{
		_game->popState();
	}
	_game->getCursor()->setVisible(true);
	if (_save->getAmbientSound() != Mod::NO_SOUND)
	{
		_game->getMod()->getSoundByDepth(0, _save->getAmbientSound())->stopLoop();
	}

	// reset touch flags
	_game->resetTouchButtonFlags();

	// dear civilians and summoned player units,
	// please drop all borrowed xcom equipment now, so that we can recover it
	// thank you!
	std::vector<BattleItem*> itemsToDrop;
	for (auto* unit : *_save->getUnits())
	{
		bool relevantUnitType = unit->getOriginalFaction() == FACTION_NEUTRAL || unit->isSummonedPlayerUnit();
		if (relevantUnitType && !unit->isOut())
		{
			itemsToDrop.clear();
			for (auto* item : *unit->getInventory())
			{
				if (item->getXCOMProperty() || item->getUnit())
				{
					itemsToDrop.push_back(item);
				}
			}
			for (auto* xcomItem : itemsToDrop)
			{
				_save->getTileEngine()->itemDrop(unit->getTile(), xcomItem, false);
			}
		}
	}

	// let's count summoned player-controlled VIPs before we remove them :)
	_battleGame->tallySummonedVIPs();
	// this removes player-controlled VIPs (not civilian VIPs)
	_battleGame->removeSummonedPlayerUnits();

	AlienDeployment *ruleDeploy = _game->getMod()->getDeployment(_save->getMissionType());
	if (!ruleDeploy)
	{
		for (std::vector<Ufo*>::iterator ufo =_game->getSavedGame()->getUfos()->begin(); ufo != _game->getSavedGame()->getUfos()->end(); ++ufo)
		{
			if ((*ufo)->isInBattlescape())
			{
				std::string ufoMissionName = (*ufo)->getRules()->getType();
				if (!_save->getAlienCustomMission().empty())
				{
					// fake underwater UFO
					ufoMissionName = _save->getAlienCustomMission();
				}
				ruleDeploy = _game->getMod()->getDeployment(ufoMissionName);
				break;
			}
		}
	}
	std::string nextStage;
	if (ruleDeploy)
	{
		nextStage = ruleDeploy->getNextStage();
	}

	if (!nextStage.empty() && inExitArea && !isPreview)
	{
		// if there is a next mission stage + we have people in exit area OR we killed all aliens, load the next stage
		_popups.clear();
		_save->setMissionType(nextStage);
		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.nextStage();
		_game->popState();
		_game->pushState(new BriefingState(0, 0));
	}
	else
	{
		_popups.clear();
		_animTimer->stop();
		_gameTimer->stop();
		_game->popState();
		if (isPreview)
		{
			// skip Debriefing
			Options::baseXResolution = Options::baseXGeoscape;
			Options::baseYResolution = Options::baseYGeoscape;
			_game->getScreen()->resetDisplay(false);

			// Restore the cursor in case something weird happened
			_game->getCursor()->setVisible(true);

			// delete SavedBattleGame
			_game->getSavedGame()->setBattleGame(0);

			// unmark all craft and all bases (current craft would be enough, but better safe than sorry)
			for (auto* base : *_game->getSavedGame()->getBases())
			{
				base->setInBattlescape(false);
				for (auto* craft : *base->getCrafts())
				{
					craft->setInBattlescape(false);
				}
			}

			// reset the music
			_game->getMod()->playMusic("GMGEO");
			return;
		}
		_game->pushState(new DebriefingState);
		std::string cutscene;
		if (ruleDeploy)
		{
			if (abort)
			{
				cutscene = ruleDeploy->getAbortCutscene();
			}
			else if (inExitArea == 0)
			{
				cutscene = ruleDeploy->getLoseCutscene();
			}
			else
			{
				cutscene = ruleDeploy->getWinCutscene();
			}
		}
		if (!cutscene.empty())
		{
			// if cutscene is "wingame" or "losegame", then the DebriefingState
			// pushed above will get popped without being shown.  otherwise
			// it will get shown after the cutscene.
			_game->pushState(new CutsceneState(cutscene));

			const RuleVideo *videoRule = _game->getMod()->getVideo(cutscene, true);
			if (videoRule->getWinGame())
			{
				_game->getSavedGame()->setEnding(END_WIN);
			}
			else if (videoRule->getLoseGame())
			{
				_game->getSavedGame()->setEnding(END_LOSE);
			}
			// Autosave if game is over
			if (_game->getSavedGame()->getEnding() != END_NONE && _game->getSavedGame()->isIronman())
			{
				_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_IRONMAN, _palette));
			}
		}
	}
}

/**
 * Shows the launch button.
 * @param show Show launch button?
 */
void BattlescapeState::showLaunchButton(bool show)
{
	_btnLaunch->setVisible(show);
}

/**
 * Clears mouse-scrolling state (isMouseScrolling).
 */
void BattlescapeState::clearMouseScrollingState()
{
	_isMouseScrolling = false;
}

/**
 * Returns a pointer to the battlegame, in case we need its functions.
 */
BattlescapeGame *BattlescapeState::getBattleGame()
{
	return _battleGame;
}

/**
 * Handler for the mouse moving over the icons, disabling the tile selection cube.
 * @param action Pointer to an action.
 */
void BattlescapeState::mouseInIcons(Action *)
{
	_mouseOverIcons = true;
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 * @param action Pointer to an action.
 */
void BattlescapeState::mouseOutIcons(Action *)
{
	_mouseOverIcons = false;
}

/**
 * Checks if the mouse is over the icons.
 * @return True, if the mouse is over the icons.
 */
bool BattlescapeState::getMouseOverIcons() const
{
	return _mouseOverIcons;
}

/**
 * Determines whether the player is allowed to press buttons.
 * Buttons are disabled in the middle of a shot, during the alien turn,
 * and while a player's units are panicking.
 * The save button is an exception as we want to still be able to save if something
 * goes wrong during the alien turn, and submit the save file for dissection.
 * @param allowSaving True, if the help button was clicked.
 * @return True if the player can still press buttons.
 */
bool BattlescapeState::allowButtons(bool allowSaving) const // Joper
{
	return ((allowSaving || _save->getSide() == FACTION_PLAYER || _save->getDebugMode() || _save->getSide() == FACTION_ALIEN_PLAYER)
		&& (_battleGame->getPanicHandled() || _firstInit )
		&& (allowSaving || !_battleGame->isBusy() || _firstInit)
		&& (_map->getProjectile() == 0));
}

/**
 * Reserves time units for kneeling.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReserveKneelClick(Action *action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);
		_battleGame->setKneelReserved(!_battleGame->getKneelReserved());

		_btnReserveKneel->toggle(_battleGame->getKneelReserved());

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
}

/**
 * Removes all time units.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnZeroTUsClick(Action *action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);
		if (_battleGame->getSave()->getSelectedUnit())
		{
			_battleGame->getSave()->getSelectedUnit()->clearTimeUnits();
			updateSoldierInfo();
		}
	}
}

/**
* Shows a tooltip with extra information (used for medikit-type equipment).
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipInExtra(Action *action, bool leftHand, bool special)
{
	if (allowButtons() && Options::battleTooltips)
	{
		// no one selected... do normal tooltip
		if(!playableUnitSelected())
		{
			_currentTooltip = action->getSender()->getTooltip();
			_txtTooltip->setText(tr(_currentTooltip));
			return;
		}

		BattleUnit *selectedUnit = _save->getSelectedUnit();
		BattleItem *weapon;
		if (leftHand)
		{
			weapon = selectedUnit->getLeftHandWeapon();
		}
		else if (special)
		{
			BattleType type;
			weapon = selectedUnit->getSpecialIconWeapon(type);
		}
		else
		{
			weapon = selectedUnit->getRightHandWeapon();
		}

		// no weapon selected... do normal tooltip
		if(!weapon)
		{
			_currentTooltip = action->getSender()->getTooltip();
			_txtTooltip->setText(tr(_currentTooltip));
			return;
		}

		auto weaponRule = weapon->getRules();

		// find the target unit
		if (weaponRule->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit *targetUnit = 0;
			TileEngine *tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();
			const std::vector<BattleUnit*> *units = _game->getSavedGame()->getSavedBattle()->getUnits();

			// search for target on the ground
			bool onGround = false;
			for (std::vector<BattleUnit*>::const_iterator i = units->begin(); i != units->end() && !targetUnit; ++i)
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if ((*i)->getPosition() == selectedUnit->getPosition() && *i != selectedUnit && (*i)->getStatus() == STATUS_UNCONSCIOUS && ((*i)->isWoundable() || weaponRule->getAllowTargetImmune()) && weaponRule->getAllowTargetGround())
				{
					if ((*i)->getArmor()->getSize() != 1)
					{
						// never EVER apply anything to 2x2 units on the ground
						continue;
					}
					if ((weaponRule->getAllowTargetFriendGround() && (*i)->getOriginalFaction() == FACTION_PLAYER) ||
						(weaponRule->getAllowTargetFriendGround() && (*i)->getOriginalFaction() == FACTION_ALIEN_PLAYER) ||
						(weaponRule->getAllowTargetNeutralGround() && (*i)->getOriginalFaction() == FACTION_NEUTRAL) ||
						(weaponRule->getAllowTargetHostileGround() && (*i)->getOriginalFaction() == FACTION_HOSTILE))
					{
						targetUnit = *i;
						onGround = true;
					}
				}
			}

			// search for target in front of the selected unit
			if (!targetUnit && weaponRule->getAllowTargetStanding())
			{
				Position dest;
				if (tileEngine->validMeleeRange(
					selectedUnit->getPosition(),
					selectedUnit->getDirection(),
					selectedUnit,
					0, &dest, false))
				{
					Tile *tile = _game->getSavedGame()->getSavedBattle()->getTile(dest);
					if (tile != 0 && tile->getUnit() && (tile->getUnit()->isWoundable() || weaponRule->getAllowTargetImmune()))
					{
						if ((weaponRule->getAllowTargetFriendStanding() && tile->getUnit()->getOriginalFaction() == FACTION_PLAYER) ||
							(weaponRule->getAllowTargetFriendGround() && tile->getUnit()->getOriginalFaction() == FACTION_ALIEN_PLAYER) ||
							(weaponRule->getAllowTargetNeutralStanding() && tile->getUnit()->getOriginalFaction() == FACTION_NEUTRAL) ||
							(weaponRule->getAllowTargetHostileStanding() && tile->getUnit()->getOriginalFaction() == FACTION_HOSTILE))
						{
							targetUnit = tile->getUnit();
						}
					}
				}
			}

			_currentTooltip = action->getSender()->getTooltip();
			std::ostringstream tooltipExtra;
			tooltipExtra << tr(_currentTooltip);

			// target unit found
			if (targetUnit)
			{
				if (targetUnit->getOriginalFaction() == FACTION_HOSTILE) {
					_txtTooltip->setColor(Palette::blockOffset(_medikitRed));
					tooltipExtra << tr("STR_TARGET_ENEMY");
				} else if (targetUnit->getOriginalFaction() == FACTION_NEUTRAL) {
					_txtTooltip->setColor(Palette::blockOffset(_medikitOrange));
					tooltipExtra << tr("STR_TARGET_NEUTRAL");
				} else if (targetUnit->getOriginalFaction() == FACTION_ALIEN_PLAYER)
				{	_txtTooltip->setColor(Palette::blockOffset(_medikitRed));
					tooltipExtra << tr("STR_TARGET_ENEMY_PLAYER"); //jopper Changes medkit pallete i guess. NOt sure rightnow.
				} else if (targetUnit->getOriginalFaction() == FACTION_PLAYER) {
					_txtTooltip->setColor(Palette::blockOffset(_medikitGreen));
					tooltipExtra << tr("STR_TARGET_FRIEND");
				}
				if (onGround) tooltipExtra << tr("STR_TARGET_ON_THE_GROUND");
				_txtTooltip->setText(tooltipExtra.str());
			}
			else
			{
				// target unit not found => selected unit is the target (if self-heal is possible)
				if (weaponRule->getAllowTargetSelf())
				{
					targetUnit = selectedUnit;
					_txtTooltip->setColor(Palette::blockOffset(_medikitBlue));
					tooltipExtra << tr("STR_TARGET_YOURSELF");
					if (onGround) tooltipExtra << tr("STR_TARGET_ON_THE_GROUND");
					_txtTooltip->setText(tooltipExtra.str());
				}
				else
				{
					// cannot use the weapon (medikit) on anyone
					_currentTooltip = action->getSender()->getTooltip();
					_txtTooltip->setText(tr(_currentTooltip));
				}
			}
		}
		else
		{
			// weapon is not of medikit battle type
			_currentTooltip = action->getSender()->getTooltip();
			_txtTooltip->setText(tr(_currentTooltip));
		}
	}
}

/**
* Shows a tooltip with extra information (used for medikit-type equipment).
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipInExtraLeftHand(Action *action)
{
	txtTooltipInExtra(action, true);
}

/**
* Shows a tooltip with extra information (used for medikit-type equipment).
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipInExtraRightHand(Action *action)
{
	txtTooltipInExtra(action, false);
}

/**
* Shows a tooltip with extra information (used for medikit-type equipment).
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipInExtraSpecial(Action *action)
{
	txtTooltipInExtra(action, false, true);
}

/**
* Shows a tooltip for the End Turn button.
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipInEndTurn(Action *action)
{
	if (allowButtons() && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();

		std::ostringstream ss;
		ss << tr(_currentTooltip);
		ss << " ";
		ss << _save->getTurn();
		if (_save->getTurnLimit() > 0)
		{
			ss << "/" << _save->getTurnLimit();
		}

		_txtTooltip->setText(ss.str().c_str());
	}
}

/**
* Shows a tooltip for the appropriate button.
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipIn(Action *action)
{
	if (allowButtons() && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtTooltip->setText(tr(_currentTooltip));
	}
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void BattlescapeState::txtTooltipOut(Action *action)
{
	// reset color
	_txtTooltip->setColor(_tooltipDefaultColor);

	if (allowButtons() && Options::battleTooltips)
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
void BattlescapeState::resize(int &dX, int &dY)
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
		if (*i == _btnCtrl || *i == _btnAlt || *i == _btnShift || *i == _btnRMB || *i == _btnMMB)
		{
			continue;
		}
		if (*i != _map && (*i) != _btnPsi && *i != _btnLaunch && *i != _btnSpecial && *i != _btnSkills && *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX / 2);
			(*i)->setY((*i)->getY() + dY);
		}
		else if (*i != _map && *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX);
		}
	}

	for (auto& pos : _posSpecialActions)
	{
		pos += dX;
	}

}

/**
 * Move the mouse back to where it started after we finish drag scrolling.
 * @param action Pointer to an action.
 */
void BattlescapeState::stopScrolling(Action *action)
{
	if (Options::battleDragScrollInvert)
	{
		SDL_WarpMouse(_xBeforeMouseScrolling, _yBeforeMouseScrolling);
		action->setMouseAction(_xBeforeMouseScrolling, _yBeforeMouseScrolling, _map->getX(), _map->getY());
		_battleGame->setupCursor();
		if (_battleGame->getCurrentAction()->actor == 0 && (_save->getSide() == FACTION_PLAYER || _save->getSide() == FACTION_ALIEN_PLAYER || _save->getDebugMode()))
		{
			getMap()->setCursorType(CT_NORMAL);
		}
	}
	else
	{
		SDL_WarpMouse(_cursorPosition.x, _cursorPosition.y);
		action->setMouseAction(_cursorPosition.x, _cursorPosition.y, _map->getX(), _map->getY());
		_map->setSelectorPosition(action->getAbsoluteXMouse(), action->getAbsoluteYMouse());
	}
	// reset our "mouse position stored" flag
	_cursorPosition.z = 0;
}

/**
 * Autosave the game the next time the battlescape is displayed.
 */
void BattlescapeState::autosave(int currentTurn)
{
	_autosave = currentTurn;
}

/**
 * Is busy?
 */
bool BattlescapeState::isBusy() const
{
	return (_map->getCursorType() == CT_NONE || _battleGame->isBusy());
}

}
