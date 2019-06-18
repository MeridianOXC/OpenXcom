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
class InteractiveSurface;
class Text;
class Bar;
class NumberText;
class BattleUnit;
class SavedBattleGame;
class BattleItem;
class Timer;
class WarningMessage;
class BattlescapeGame;

/**
 * Battlescape screen which shows the tactical battle.
 */
class BattlescapeState : public State
{
private:
	Surface *_rank, *_rankTiny;
	InteractiveSurface *_icons;
	Map *_map;
	BattlescapeButton *_btnUnitUp, *_btnUnitDown, *_btnMapUp, *_btnMapDown, *_btnShowMap, *_btnKneel;
	BattlescapeButton *_btnInventory, *_btnCenter, *_btnNextSoldier, *_btnNextStop, *_btnShowLayers, *_btnHelp;
	BattlescapeButton *_btnEndTurn, *_btnAbort, *_btnLaunch, *_btnPsi, *_btnSpecial, *_btnSkills, *_reserve;
	BattlescapeButton *_btnCtrl, *_btnAlt, *_btnShift, *_btnRMB, *_btnMMB;
	bool _touchButtonsEnabled, _touchButtonsEnabledLastTurn;
	InteractiveSurface *_btnStats;
	BattlescapeButton *_btnReserveNone, *_btnReserveSnap, *_btnReserveAimed, *_btnReserveAuto, *_btnReserveKneel, *_btnZeroTUs;
	InteractiveSurface *_btnLeftHandItem, *_btnRightHandItem;

	static const int SPECIAL_BUTTONS_MAX = 3;
	int _posSpecialActions[SPECIAL_BUTTONS_MAX];

	static const int VISIBLE_MAX = 10;
	std::string _txtVisibleUnitTooltip[VISIBLE_MAX+2];
	InteractiveSurface *_btnVisibleUnit[VISIBLE_MAX];
	NumberText *_numVisibleUnit[VISIBLE_MAX];
	BattleUnit *_visibleUnit[VISIBLE_MAX];

	WarningMessage *_warning;
	Text *_txtName;
	NumberText *_numTimeUnits, *_numEnergy, *_numHealth, *_numMorale, *_numLayers;
	std::vector<NumberText*> _numAmmoLeft, _numAmmoRight;
	std::vector<NumberText*> _numMedikitLeft, _numMedikitRight;
	NumberText *_numTwoHandedIndicatorLeft, *_numTwoHandedIndicatorRight;
	Uint8 _twoHandedRed, _twoHandedGreen;
	Bar *_barTimeUnits, *_barEnergy, *_barHealth, *_barMorale, *_barMana;
	bool _manaBarVisible;
	Timer *_animTimer, *_gameTimer;
	SavedBattleGame *_save;
	Text *_txtDebug, *_txtTooltip;
	Uint8 _tooltipDefaultColor;
	Uint8 _medikitRed, _medikitGreen, _medikitBlue, _medikitOrange;
	std::vector<State*> _popups;
	BattlescapeGame *_battleGame;
	bool _firstInit, _paletteResetNeeded, _paletteResetRequested;
	bool _isMouseScrolling, _isMouseScrolled;
	int _xBeforeMouseScrolling, _yBeforeMouseScrolling;
	Position _mapOffsetBeforeMouseScrolling;
	Uint32 _mouseScrollingStartTime;
	int _totalMouseMoveX, _totalMouseMoveY;
	bool _mouseMovedOverThreshold;
	bool _mouseOverIcons;
	std::string _currentTooltip;
	Position _cursorPosition;
	Uint8 _barHealthColor;
	int _autosave;
	int _numberOfDirectlyVisibleUnits, _numberOfEnemiesTotal, _numberOfEnemiesTotalPlusWounded;
	Uint8 _indicatorTextColor, _indicatorGreen, _indicatorBlue, _indicatorPurple;
	/// Popups a context sensitive list of actions the user can choose from.
	void handleItemClick(BattleItem *item, bool rightClick);
	/// Shifts the red colors of the visible unit buttons backgrounds.
	void blinkVisibleUnitButtons();
	/// Draw hand item with ammo number.
	void drawItem(BattleItem *item, Surface *hand, std::vector<NumberText*> &ammoText, std::vector<NumberText*> &medikitText, NumberText *twoHandedText, bool drawReactionIndicator, bool drawNoReactionIndicator);
	/// Draw both hands sprites.
	void drawHandsItems();
	/// Shifts the colors of the health bar when unit has fatal wounds.
	void blinkHealthBar();
	/// Shows the unit kneel state.
	void toggleKneelButton(BattleUnit* unit);
public:
	/// Selects the next soldier.
	void selectNextPlayerUnit(bool checkReselect = false, bool setReselect = false, bool checkInventory = false, bool checkFOV = true);
	/// Selects the previous soldier.
	void selectPreviousPlayerUnit(bool checkReselect = false, bool setReselect = false, bool checkInventory = false);
	static const int DEFAULT_ANIM_SPEED = 100;
	/// Creates the Battlescape state.
	BattlescapeState();
	/// Cleans up the Battlescape state.
	~BattlescapeState();
	void resetPalettes();
	/// Initializes the battlescapestate.
	void init() override;
	/// Runs the timers and handles popups.
	void think() override;
	/// Handler for moving mouse over the map.
	void mapOver(Action *action);
	/// Handler for pressing the map.
	void mapPress(Action *action);
	/// Handler for clicking the map.
	void mapClick(Action *action);
	/// Handler for entering with mouse to the map surface.
	void mapIn(Action *action);
	/// Handler for clicking the Unit Up button.
	void btnUnitUpClick(Action *action);
	/// Handler for clicking the Unit Down button.
	void btnUnitDownClick(Action *action);
	/// Handler for clicking the Map Up button.
	void btnMapUpClick(Action *action);
	/// Handler for clicking the Map Down button.
	void btnMapDownClick(Action *action);
	/// Handler for clicking the Show Map button.
	void btnShowMapClick(Action *action);
	/// Handler for clicking the Kneel button.
	void btnKneelClick(Action *action);
	/// Handler for clicking the Soldier button.
	void btnInventoryClick(Action *action);
	/// Handler for clicking the Center button.
	void btnCenterClick(Action *action);
	/// Handler for clicking the Next Soldier button.
	void btnNextSoldierClick(Action *action);
	/// Handler for clicking the Next Stop button.
	void btnNextStopClick(Action *action);
	/// Handler for clicking the Previous Soldier button.
	void btnPrevSoldierClick(Action *action);
	/// Handler for clicking the Show Layers button.
	void btnShowLayersClick(Action *action);
	void btnShowLayersClickOrig(Action *action);
	/// Handler for clicking the Ufopaedia button.
	void btnUfopaediaClick(Action *action);
	/// Handler for clicking the Help button.
	void btnHelpClick(Action *action);
	/// Handler for clicking the End Turn button.
	void btnEndTurnClick(Action *action);
	/// Handler for clicking the Abort button.
	void btnAbortClick(Action *action);
	/// Handler for clicking the stats.
	void btnStatsClick(Action *action);
	/// Handler for clicking the left hand item button.
	void btnLeftHandItemClick(Action *action);
	/// Handler for clicking the right hand item button.
	void btnRightHandItemClick(Action *action);
	/// Handler for clicking a visible unit button.
	void btnVisibleUnitClick(Action *action);

	/// Handler for clicking the CTRL touch button.
	void btnCtrlClick(Action* action);
	/// Handler for clicking the ALT touch button.
	void btnAltClick(Action* action);
	/// Handler for clicking the SHIFT touch button.
	void btnShiftClick(Action* action);
	/// Handler for clicking the RMB touch button.
	void btnRMBClick(Action* action);
	/// Handler for clicking the MMB touch button.
	void btnMMBClick(Action* action);
	/// Handler for pressing the toggle touch buttons hotkey.
	void btnTouchButtonsClick(Action* action);
	/// Toggles the touch buttons.
	void toggleTouchButtons(bool deactivate, bool tryToReactivate);

	/// Handler for clicking the launch rocket button.
	void btnLaunchClick(Action *action);
	/// Handler for clicking the use psi button.
	void btnPsiClick(Action *action);
	/// Handler for clicking the use special weapon button.
	void btnSpecialClick(Action *action);
	/// Handler for clicking the skills menu button.
	void btnSkillsClick(Action *action);
	/// Handler for clicking a reserved button.
	void btnReserveClick(Action *action);
	/// Handler for clicking the reload button.
	void btnReloadClick(Action *action);
	/// Handler for clicking the [SelectMusicTrack] button.
	void btnSelectMusicTrackClick(Action *action);
	/// Handler for clicking the lighting button.
	void btnPersonalLightingClick(Action *action);
	/// Handler for toggling the "night vision" mode.
	void btnNightVisionClick(Action *action);
	/// Determines whether a playable unit is selected.
	bool playableUnitSelected();
	/// Updates soldier name/rank/tu/energy/health/morale.
	void updateSoldierInfo(bool checkFOV = true);
	/// Updates the special/psi/skill button display based on the battle unit
	void updateUiButton(const BattleUnit* battleUnit);
	/// Animates map objects on the map, also smoke,fire, ...
	void animate();
	/// Handles the battle game state.
	void handleState();
	/// Sets the state timer interval.
	void setStateInterval(Uint32 interval);
	/// Gets game.
	Game *getGame() const;
	/// Gets map.
	Map *getMap() const;
	/// Show debug message.
	void debug(const std::string &message);
	/// Show bug hunt message.
	void bugHuntMessage();
	/// Show warning message.
	void warning(const std::string &message);
	/// Show warning message, no translation.
	void warningRaw(const std::string &message);
	/// Show warning message that stay longer on screen, no translation.
	void warningLongRaw(const std::string &message);
	/// Gets melee damage preview.
	std::string getMeleeDamagePreview(BattleUnit *actor, BattleItem *weapon) const;
	/// Handles keypresses.
	void handle(Action *action) override;
	/// Displays a popup window.
	void popup(State *state);
	/// Finishes a battle.
	void finishBattle(bool abort, int inExitArea);
	/// Show the launch button.
	void showLaunchButton(bool show);
	/// Reset visiblity of special buttons like psi or skill.
	void resetUiButton();
	/// Clears mouse-scrolling state.
	void clearMouseScrollingState();
	/// Returns a pointer to the battlegame, in case we need its functions.
	BattlescapeGame *getBattleGame();
	/// Saves a map as used by the AI.
	void saveAIMap();
	/// Saves each layer of voxels on the bettlescape as a png.
	void saveVoxelMap();
	/// Saves a first-person voxel view of the battlescape.
	void saveVoxelView();
	/// Handler for the mouse moving over the icons, disables the tile selection cube.
	void mouseInIcons(Action *action);
	/// Handler for the mouse going out of the icons, enabling the tile selection cube.
	void mouseOutIcons(Action *action);
	/// Checks if the mouse is over the icons.
	bool getMouseOverIcons() const;
	/// Is the player allowed to press buttons?
	bool allowButtons(bool allowSaving = false) const;
	/// Handler for clicking the reserve TUs to kneel button.
	void btnReserveKneelClick(Action *action);
	/// Handler for clicking the expend all TUs button.
	void btnZeroTUsClick(Action *action);
	/// Handler for showing tooltip.
	void txtTooltipIn(Action *action);
	/// Handler for showing tooltip with extra information (used for medikit-type equipment)
	void txtTooltipInExtra(Action *action, bool leftHand, bool special = false);
	void txtTooltipInExtraLeftHand(Action *action);
	void txtTooltipInExtraRightHand(Action *action);
	void txtTooltipInExtraSpecial(Action *action);
	/// Handler for showing tooltip with extra information (about current turn)
	void txtTooltipInEndTurn(Action *action);
	/// Handler for hiding tooltip.
	void txtTooltipOut(Action *action);
	/// Update the resolution settings, we just resized the window.
	void resize(int &dX, int &dY) override;
	/// Move the mouse back to where it started after we finish drag scrolling.
	void stopScrolling(Action *action);
	/// Autosave next turn.
	void autosave(int currentTurn);
	/// Is busy?
	bool isBusy() const;
};

}
