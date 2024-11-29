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

#include "Options.h"
#include "../version.h"
#include "../md5.h"
#include <SDL.h>
#include <SDL_keysym.h>
#include <SDL_mixer.h>
#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "../Engine/Yaml.h"
#include "Exception.h"
#include "Logger.h"
#include "CrossPlatform.h"
#include "../Menu/ModConfirmExtendedState.h"
#include "FileMap.h"
#include "Screen.h"

namespace OpenXcom
{

namespace Options
{
#define OPT
#include "Options.inc.h"
#undef OPT

std::string _dataFolder;
std::vector<std::string> _dataList;
std::string _userFolder;
std::string _configFolder;
std::vector<std::string> _userList;
std::map<std::string, std::string> _commandLine;
std::vector<OptionInfo> _info;
std::map<std::string, ModInfo> _modInfos;
std::string _masterMod;
int _passwordCheck = -1;
bool _loadLastSave = false;
std::string _loadThisSave = "";
bool _loadLastSaveExpended = false;

/**
 * Sets up the options by creating their OptionInfo metadata.
 */
void create()
{
	if (!_info.empty()) { _info.clear(); }

	////////////////////////////////////////////////////////////
	//					OXC - OpenXcom
	////////////////////////////////////////////////////////////

	createOptionsOXC();
	createAdvancedOptionsOXC();
	createControlsOXC();

	////////////////////////////////////////////////////////////
	//					OXCE - OpenXcom Extended
	////////////////////////////////////////////////////////////

	createOptionsOXCE();
	createAdvancedOptionsOXCE();
	createControlsOXCE();

	////////////////////////////////////////////////////////////
	//					OTHER - your fork name here
	////////////////////////////////////////////////////////////

	createOptionsOTHER();
	createAdvancedOptionsOTHER();
	createControlsOTHER();
}

void createOptionsOXC()
{
#ifdef DINGOO
	_info.push_back(OptionInfo(OPTION_OXC, "displayWidth", &displayWidth, Screen::ORIGINAL_WIDTH));
	_info.push_back(OptionInfo(OPTION_OXC, "displayHeight", &displayHeight, Screen::ORIGINAL_HEIGHT));
	_info.push_back(OptionInfo(OPTION_OXC, "fullscreen", &fullscreen, true));
	_info.push_back(OptionInfo(OPTION_OXC, "asyncBlit", &asyncBlit, false));
	_info.push_back(OptionInfo(OPTION_OXC, "keyboardMode", (int*)&keyboardMode, KEYBOARD_OFF));
#else
	_info.push_back(OptionInfo(OPTION_OXC, "displayWidth", &displayWidth, Screen::ORIGINAL_WIDTH*2));
	_info.push_back(OptionInfo(OPTION_OXC, "displayHeight", &displayHeight, Screen::ORIGINAL_HEIGHT*2));
	_info.push_back(OptionInfo(OPTION_OXC, "fullscreen", &fullscreen, false));
	_info.push_back(OptionInfo(OPTION_OXC, "asyncBlit", &asyncBlit, true));
	_info.push_back(OptionInfo(OPTION_OXC, "keyboardMode", (int*)&keyboardMode, KEYBOARD_ON));
#endif

#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXC, "maxFrameSkip", &maxFrameSkip, 0, "STR_FRAMESKIP", "STR_GENERAL"));
#else
	_info.push_back(OptionInfo(OPTION_OXC, "maxFrameSkip", &maxFrameSkip, 0));
#endif
	_info.push_back(OptionInfo(OPTION_OXC, "traceAI", &traceAI, false));
	_info.push_back(OptionInfo(OPTION_OXC, "verboseLogging", &verboseLogging, false));
	_info.push_back(OptionInfo(OPTION_OXC, "StereoSound", &StereoSound, true));
	//_info.push_back(OptionInfo(OPTION_OXC, "baseXResolution", &baseXResolution, Screen::ORIGINAL_WIDTH));
	//_info.push_back(OptionInfo(OPTION_OXC, "baseYResolution", &baseYResolution, Screen::ORIGINAL_HEIGHT));
	//_info.push_back(OptionInfo(OPTION_OXC, "baseXGeoscape", &baseXGeoscape, Screen::ORIGINAL_WIDTH));
	//_info.push_back(OptionInfo(OPTION_OXC, "baseYGeoscape", &baseYGeoscape, Screen::ORIGINAL_HEIGHT));
	//_info.push_back(OptionInfo(OPTION_OXC, "baseXBattlescape", &baseXBattlescape, Screen::ORIGINAL_WIDTH));
	//_info.push_back(OptionInfo(OPTION_OXC, "baseYBattlescape", &baseYBattlescape, Screen::ORIGINAL_HEIGHT));
	_info.push_back(OptionInfo(OPTION_OXC, "geoscapeScale", &geoscapeScale, 0));
	_info.push_back(OptionInfo(OPTION_OXC, "battlescapeScale", &battlescapeScale, 0));
	_info.push_back(OptionInfo(OPTION_OXC, "useScaleFilter", &useScaleFilter, false));
	_info.push_back(OptionInfo(OPTION_OXC, "useHQXFilter", &useHQXFilter, false));
	_info.push_back(OptionInfo(OPTION_OXC, "useXBRZFilter", &useXBRZFilter, false));
	_info.push_back(OptionInfo(OPTION_OXC, "useOpenGL", &useOpenGL, false));
	_info.push_back(OptionInfo(OPTION_OXC, "checkOpenGLErrors", &checkOpenGLErrors, false));
	_info.push_back(OptionInfo(OPTION_OXC, "useOpenGLShader", &useOpenGLShader, "Shaders/Raw.OpenGL.shader"));
	_info.push_back(OptionInfo(OPTION_OXC, "useOpenGLSmoothing", &useOpenGLSmoothing, true));
	_info.push_back(OptionInfo(OPTION_OXC, "debug", &debug, false));
	_info.push_back(OptionInfo(OPTION_OXC, "debugUi", &debugUi, false));
	_info.push_back(OptionInfo(OPTION_OXC, "soundVolume", &soundVolume, 2*(MIX_MAX_VOLUME/3)));
	_info.push_back(OptionInfo(OPTION_OXC, "musicVolume", &musicVolume, 2*(MIX_MAX_VOLUME/3)));
	_info.push_back(OptionInfo(OPTION_OXC, "uiVolume", &uiVolume, MIX_MAX_VOLUME/3));
	_info.push_back(OptionInfo(OPTION_OXC, "language", &language, ""));
	_info.push_back(OptionInfo(OPTION_OXC, "battleScrollSpeed", &battleScrollSpeed, 8));
#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXC, "battleEdgeScroll", (int*)&battleEdgeScroll, SCROLL_NONE));
	_info.push_back(OptionInfo(OPTION_OXC, "battleDragScrollButton", &battleDragScrollButton, SDL_BUTTON_LEFT));
#else
	_info.push_back(OptionInfo(OPTION_OXC, "battleEdgeScroll", (int*)&battleEdgeScroll, SCROLL_AUTO));
	_info.push_back(OptionInfo(OPTION_OXC, "battleDragScrollButton", &battleDragScrollButton, 0)); // different default in OXCE
#endif
	_info.push_back(OptionInfo(OPTION_OXC, "dragScrollTimeTolerance", &dragScrollTimeTolerance, 300)); // miliSecond
	_info.push_back(OptionInfo(OPTION_OXC, "dragScrollPixelTolerance", &dragScrollPixelTolerance, 10)); // count of pixels
	_info.push_back(OptionInfo(OPTION_OXC, "battleFireSpeed", &battleFireSpeed, 6));
	_info.push_back(OptionInfo(OPTION_OXC, "battleXcomSpeed", &battleXcomSpeed, 30));
	battleXcomSpeedOrig = -1;
	_info.push_back(OptionInfo(OPTION_OXC, "battleAlienSpeed", &battleAlienSpeed, 30));
#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXC, "battleNewPreviewPath", (int*)&battleNewPreviewPath, PATH_FULL)); // for android, set full preview by default
#else
	_info.push_back(OptionInfo(OPTION_OXC, "battleNewPreviewPath", (int*)&battleNewPreviewPath, PATH_NONE)); // requires double-click to confirm moves
#endif
	_info.push_back(OptionInfo(OPTION_OXC, "fpsCounter", &fpsCounter, false));
	_info.push_back(OptionInfo(OPTION_OXC, "globeDetail", &globeDetail, true));
	_info.push_back(OptionInfo(OPTION_OXC, "globeRadarLines", &globeRadarLines, true));
	_info.push_back(OptionInfo(OPTION_OXC, "globeFlightPaths", &globeFlightPaths, true));
	_info.push_back(OptionInfo(OPTION_OXC, "globeAllRadarsOnBaseBuild", &globeAllRadarsOnBaseBuild, true));
	_info.push_back(OptionInfo(OPTION_OXC, "audioSampleRate", &audioSampleRate, 22050));
	_info.push_back(OptionInfo(OPTION_OXC, "audioBitDepth", &audioBitDepth, 16));
	_info.push_back(OptionInfo(OPTION_OXC, "audioChunkSize", &audioChunkSize, 1024));
	_info.push_back(OptionInfo(OPTION_OXC, "pauseMode", &pauseMode, 0));
	_info.push_back(OptionInfo(OPTION_OXC, "battleNotifyDeath", &battleNotifyDeath, false));
	_info.push_back(OptionInfo(OPTION_OXC, "showFundsOnGeoscape", &showFundsOnGeoscape, false));
	_info.push_back(OptionInfo(OPTION_OXC, "allowResize", &allowResize, false));
	_info.push_back(OptionInfo(OPTION_OXC, "windowedModePositionX", &windowedModePositionX, 0));
	_info.push_back(OptionInfo(OPTION_OXC, "windowedModePositionY", &windowedModePositionY, 0));
	_info.push_back(OptionInfo(OPTION_OXC, "borderless", &borderless, false));
	_info.push_back(OptionInfo(OPTION_OXC, "captureMouse", (bool*)&captureMouse, false));
	_info.push_back(OptionInfo(OPTION_OXC, "battleTooltips", &battleTooltips, true));
	_info.push_back(OptionInfo(OPTION_OXC, "keepAspectRatio", &keepAspectRatio, true));
	_info.push_back(OptionInfo(OPTION_OXC, "nonSquarePixelRatio", &nonSquarePixelRatio, false));
	_info.push_back(OptionInfo(OPTION_OXC, "cursorInBlackBandsInFullscreen", &cursorInBlackBandsInFullscreen, false));
	_info.push_back(OptionInfo(OPTION_OXC, "cursorInBlackBandsInWindow", &cursorInBlackBandsInWindow, true));
	_info.push_back(OptionInfo(OPTION_OXC, "cursorInBlackBandsInBorderlessWindow", &cursorInBlackBandsInBorderlessWindow, false));
	_info.push_back(OptionInfo(OPTION_OXC, "saveOrder", (int*)&saveOrder, SORT_DATE_DESC));
	_info.push_back(OptionInfo(OPTION_OXC, "geoClockSpeed", &geoClockSpeed, 80));
	_info.push_back(OptionInfo(OPTION_OXC, "dogfightSpeed", &dogfightSpeed, 30));
	_info.push_back(OptionInfo(OPTION_OXC, "geoScrollSpeed", &geoScrollSpeed, 20));
#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXC, "geoDragScrollButton", &geoDragScrollButton, SDL_BUTTON_LEFT));
#else
	_info.push_back(OptionInfo(OPTION_OXC, "geoDragScrollButton", &geoDragScrollButton, SDL_BUTTON_MIDDLE));
#endif
	_info.push_back(OptionInfo(OPTION_OXC, "preferredMusic", (int*)&preferredMusic, MUSIC_AUTO));
	_info.push_back(OptionInfo(OPTION_OXC, "preferredSound", (int*)&preferredSound, SOUND_AUTO));
	_info.push_back(OptionInfo(OPTION_OXC, "preferredVideo", (int*)&preferredVideo, VIDEO_FMV));
	_info.push_back(OptionInfo(OPTION_OXC, "wordwrap", (int*)&wordwrap, WRAP_AUTO));
	_info.push_back(OptionInfo(OPTION_OXC, "musicAlwaysLoop", &musicAlwaysLoop, false));
#ifdef __MOBILE
	_info.push_back(OptionInfo(OPTION_OXC, "touchEnabled", &touchEnabled, true));
#else
	_info.push_back(OptionInfo(OPTION_OXC, "touchEnabled", &touchEnabled, false));
#endif
	_info.push_back(OptionInfo(OPTION_OXC, "rootWindowedMode", &rootWindowedMode, false));
	_info.push_back(OptionInfo(OPTION_OXC, "backgroundMute", &backgroundMute, false));
	_info.push_back(OptionInfo(OPTION_OXC, "soldierDiaries", &soldierDiaries, true));
}

void createAdvancedOptionsOXC()
{
	// advanced options
	_info.push_back(OptionInfo(OPTION_OXC, "playIntro", &playIntro, true, "STR_PLAYINTRO", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "autosave", &autosave, true, "STR_AUTOSAVE", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "autosaveFrequency", &autosaveFrequency, 5, "STR_AUTOSAVE_FREQUENCY", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "newSeedOnLoad", &newSeedOnLoad, false, "STR_NEWSEEDONLOAD", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "lazyLoadResources", &lazyLoadResources, true, "STR_LAZY_LOADING", "STR_GENERAL")); // exposed in OXCE
	_info.push_back(OptionInfo(OPTION_OXC, "mousewheelSpeed", &mousewheelSpeed, 3, "STR_MOUSEWHEEL_SPEED", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "changeValueByMouseWheel", &changeValueByMouseWheel, 0, "STR_CHANGEVALUEBYMOUSEWHEEL", "STR_GENERAL"));

// this should probably be any small screen touch-device, i don't know the defines for all of them so i'll cover android and IOS as i imagine they're more common
#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXC, "maximizeInfoScreens", &maximizeInfoScreens, true, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
#elif __APPLE__
	// todo: ask grussel how badly i messed this up.
	#include "TargetConditionals.h"
	#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
		_info.push_back(OptionInfo(OPTION_OXC, "maximizeInfoScreens", &maximizeInfoScreens, true, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
	#else
		_info.push_back(OptionInfo(OPTION_OXC, "maximizeInfoScreens", &maximizeInfoScreens, false, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
	#endif
#else
	_info.push_back(OptionInfo(OPTION_OXC, "maximizeInfoScreens", &maximizeInfoScreens, false, "STR_MAXIMIZE_INFO_SCREENS", "STR_GENERAL"));
#endif

#ifdef __MORPHOS__
	_info.push_back(OptionInfo(OPTION_OXC, "FPS", &FPS, 15, "STR_FPS_LIMIT", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "FPSInactive", &FPSInactive, 15, "STR_FPS_INACTIVE_LIMIT", "STR_GENERAL"));
#else
	_info.push_back(OptionInfo(OPTION_OXC, "FPS", &FPS, 60, "STR_FPS_LIMIT", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "FPSInactive", &FPSInactive, 30, "STR_FPS_INACTIVE_LIMIT", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "vSyncForOpenGL", &vSyncForOpenGL, true, "STR_VSYNC_FOR_OPENGL", "STR_GENERAL")); // exposed in OXCE
#endif

	_info.push_back(OptionInfo(OPTION_OXC, "geoDragScrollInvert", &geoDragScrollInvert, false, "STR_DRAGSCROLLINVERT", "STR_GEOSCAPE")); // true drags away from the cursor, false drags towards (like a grab)
	_info.push_back(OptionInfo(OPTION_OXC, "aggressiveRetaliation", &aggressiveRetaliation, false, "STR_AGGRESSIVERETALIATION", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "customInitialBase", &customInitialBase, false, "STR_CUSTOMINITIALBASE", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "allowBuildingQueue", &allowBuildingQueue, false, "STR_ALLOWBUILDINGQUEUE", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "craftLaunchAlways", &craftLaunchAlways, false, "STR_CRAFTLAUNCHALWAYS", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "storageLimitsEnforced", &storageLimitsEnforced, false, "STR_STORAGELIMITSENFORCED", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "canSellLiveAliens", &canSellLiveAliens, false, "STR_CANSELLLIVEALIENS", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "anytimePsiTraining", &anytimePsiTraining, false, "STR_ANYTIMEPSITRAINING", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "globeSeasons", &globeSeasons, false, "STR_GLOBESEASONS", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "globeSurfaceCache", &globeSurfaceCache, true)); //hidden for now
	_info.push_back(OptionInfo(OPTION_OXC, "psiStrengthEval", &psiStrengthEval, false, "STR_PSISTRENGTHEVAL", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "canTransferCraftsWhileAirborne", &canTransferCraftsWhileAirborne, false, "STR_CANTRANSFERCRAFTSWHILEAIRBORNE", "STR_GEOSCAPE")); // When the craft can reach the destination base with its fuel
	_info.push_back(OptionInfo(OPTION_OXC, "retainCorpses", &retainCorpses, false, "STR_RETAINCORPSES", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "fieldPromotions", &fieldPromotions, false, "STR_FIELDPROMOTIONS", "STR_GEOSCAPE"));
	//_info.push_back(OptionInfo(OPTION_OXC, "meetingPoint", &meetingPoint, false, "STR_MEETINGPOINT", "STR_GEOSCAPE")); // intentionally disabled in OXCE

	_info.push_back(OptionInfo(OPTION_OXC, "battleDragScrollInvert", &battleDragScrollInvert, false, "STR_DRAGSCROLLINVERT", "STR_BATTLESCAPE")); // true drags away from the cursor, false drags towards (like a grab)
	_info.push_back(OptionInfo(OPTION_OXC, "sneakyAI", &sneakyAI, false, "STR_SNEAKYAI", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "battleUFOExtenderAccuracy", &battleUFOExtenderAccuracy, false, "STR_BATTLEUFOEXTENDERACCURACY", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "showMoreStatsInInventoryView", &showMoreStatsInInventoryView, false, "STR_SHOWMORESTATSININVENTORYVIEW", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "battleHairBleach", &battleHairBleach, true, "STR_BATTLEHAIRBLEACH", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "battleInstantGrenade", &battleInstantGrenade, false, "STR_BATTLEINSTANTGRENADE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "includePrimeStateInSavedLayout", &includePrimeStateInSavedLayout, false, "STR_INCLUDE_PRIMESTATE_IN_SAVED_LAYOUT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "battleExplosionHeight", &battleExplosionHeight, 0, "STR_BATTLEEXPLOSIONHEIGHT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "battleAutoEnd", &battleAutoEnd, false, "STR_BATTLEAUTOEND", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "battleSmoothCamera", &battleSmoothCamera, false, "STR_BATTLESMOOTHCAMERA", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "disableAutoEquip", &disableAutoEquip, false, "STR_DISABLEAUTOEQUIP", "STR_BATTLESCAPE"));
#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXC, "battleConfirmFireMode", &battleConfirmFireMode, true, "STR_BATTLECONFIRMFIREMODE", "STR_BATTLESCAPE"));
#else
	_info.push_back(OptionInfo(OPTION_OXC, "battleConfirmFireMode", &battleConfirmFireMode, false, "STR_BATTLECONFIRMFIREMODE", "STR_BATTLESCAPE"));
#endif
	_info.push_back(OptionInfo(OPTION_OXC, "weaponSelfDestruction", &weaponSelfDestruction, false, "STR_WEAPONSELFDESTRUCTION", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "allowPsionicCapture", &allowPsionicCapture, false, "STR_ALLOWPSIONICCAPTURE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "allowPsiStrengthImprovement", &allowPsiStrengthImprovement, false, "STR_ALLOWPSISTRENGTHIMPROVEMENT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "strafe", &strafe, false, "STR_STRAFE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "forceFire", &forceFire, true, "STR_FORCE_FIRE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "skipNextTurnScreen", &skipNextTurnScreen, false, "STR_SKIPNEXTTURNSCREEN", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "noAlienPanicMessages", &noAlienPanicMessages, false, "STR_NOALIENPANICMESSAGES", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "alienBleeding", &alienBleeding, false, "STR_ALIENBLEEDING", "STR_BATTLESCAPE"));
}

void createControlsOXC()
{
	// controls
	_info.push_back(OptionInfo(OPTION_OXC, "keyOk", &keyOk, SDLK_RETURN, "STR_OK", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyCancel", &keyCancel, SDLK_ESCAPE, "STR_CANCEL", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyScreenshot", &keyScreenshot, SDLK_F12, "STR_SCREENSHOT", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyFps", &keyFps, SDLK_F7, "STR_FPS_COUNTER", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyQuickSave", &keyQuickSave, SDLK_F5, "STR_QUICK_SAVE", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyQuickLoad", &keyQuickLoad, SDLK_F9, "STR_QUICK_LOAD", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoLeft", &keyGeoLeft, SDLK_LEFT, "STR_ROTATE_LEFT", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoRight", &keyGeoRight, SDLK_RIGHT, "STR_ROTATE_RIGHT", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoUp", &keyGeoUp, SDLK_UP, "STR_ROTATE_UP", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoDown", &keyGeoDown, SDLK_DOWN, "STR_ROTATE_DOWN", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoZoomIn", &keyGeoZoomIn, SDLK_PLUS, "STR_ZOOM_IN", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoZoomOut", &keyGeoZoomOut, SDLK_MINUS, "STR_ZOOM_OUT", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoSpeed1", &keyGeoSpeed1, SDLK_1, "STR_5_SECONDS", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoSpeed2", &keyGeoSpeed2, SDLK_2, "STR_1_MINUTE", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoSpeed3", &keyGeoSpeed3, SDLK_3, "STR_5_MINUTES", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoSpeed4", &keyGeoSpeed4, SDLK_4, "STR_30_MINUTES", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoSpeed5", &keyGeoSpeed5, SDLK_5, "STR_1_HOUR", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoSpeed6", &keyGeoSpeed6, SDLK_6, "STR_1_DAY", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoIntercept", &keyGeoIntercept, SDLK_i, "STR_INTERCEPT", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoBases", &keyGeoBases, SDLK_b, "STR_BASES", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoGraphs", &keyGeoGraphs, SDLK_g, "STR_GRAPHS", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoUfopedia", &keyGeoUfopedia, SDLK_u, "STR_UFOPAEDIA_UC", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoOptions", &keyGeoOptions, SDLK_ESCAPE, "STR_OPTIONS_UC", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoFunding", &keyGeoFunding, SDLK_f, "STR_FUNDING_UC", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoToggleDetail", &keyGeoToggleDetail, SDLK_TAB, "STR_TOGGLE_COUNTRY_DETAIL", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyGeoToggleRadar", &keyGeoToggleRadar, SDLK_r, "STR_TOGGLE_RADAR_RANGES", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect1", &keyBaseSelect1, SDLK_1, "STR_SELECT_BASE_1", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect2", &keyBaseSelect2, SDLK_2, "STR_SELECT_BASE_2", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect3", &keyBaseSelect3, SDLK_3, "STR_SELECT_BASE_3", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect4", &keyBaseSelect4, SDLK_4, "STR_SELECT_BASE_4", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect5", &keyBaseSelect5, SDLK_5, "STR_SELECT_BASE_5", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect6", &keyBaseSelect6, SDLK_6, "STR_SELECT_BASE_6", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect7", &keyBaseSelect7, SDLK_7, "STR_SELECT_BASE_7", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBaseSelect8", &keyBaseSelect8, SDLK_8, "STR_SELECT_BASE_8", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleLeft", &keyBattleLeft, SDLK_LEFT, "STR_SCROLL_LEFT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleRight", &keyBattleRight, SDLK_RIGHT, "STR_SCROLL_RIGHT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleUp", &keyBattleUp, SDLK_UP, "STR_SCROLL_UP", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleDown", &keyBattleDown, SDLK_DOWN, "STR_SCROLL_DOWN", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleLevelUp", &keyBattleLevelUp, SDLK_PAGEUP, "STR_VIEW_LEVEL_ABOVE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleLevelDown", &keyBattleLevelDown, SDLK_PAGEDOWN, "STR_VIEW_LEVEL_BELOW", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterUnit", &keyBattleCenterUnit, SDLK_HOME, "STR_CENTER_SELECTED_UNIT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattlePrevUnit", &keyBattlePrevUnit, SDLK_UNKNOWN, "STR_PREVIOUS_UNIT", "STR_BATTLESCAPE")); // different default in OXCE
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleNextUnit", &keyBattleNextUnit, SDLK_TAB, "STR_NEXT_UNIT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleDeselectUnit", &keyBattleDeselectUnit, SDLK_BACKSLASH, "STR_DESELECT_UNIT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleUseLeftHand", &keyBattleUseLeftHand, SDLK_q, "STR_USE_LEFT_HAND", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleUseRightHand", &keyBattleUseRightHand, SDLK_e, "STR_USE_RIGHT_HAND", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleInventory", &keyBattleInventory, SDLK_i, "STR_INVENTORY", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleMap", &keyBattleMap, SDLK_m, "STR_MINIMAP", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleOptions", &keyBattleOptions, SDLK_ESCAPE, "STR_OPTIONS", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleEndTurn", &keyBattleEndTurn, SDLK_BACKSPACE, "STR_END_TURN", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleAbort", &keyBattleAbort, SDLK_a, "STR_ABORT_MISSION", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleStats", &keyBattleStats, SDLK_s, "STR_UNIT_STATS", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleKneel", &keyBattleKneel, SDLK_k, "STR_KNEEL", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleReload", &keyBattleReload, SDLK_r, "STR_RELOAD", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattlePersonalLighting", &keyBattlePersonalLighting, SDLK_l, "STR_TOGGLE_PERSONAL_LIGHTING", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleReserveNone", &keyBattleReserveNone, SDLK_F1, "STR_DONT_RESERVE_TIME_UNITS", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleReserveSnap", &keyBattleReserveSnap, SDLK_F2, "STR_RESERVE_TIME_UNITS_FOR_SNAP_SHOT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleReserveAimed", &keyBattleReserveAimed, SDLK_F3, "STR_RESERVE_TIME_UNITS_FOR_AIMED_SHOT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleReserveAuto", &keyBattleReserveAuto, SDLK_F4, "STR_RESERVE_TIME_UNITS_FOR_AUTO_SHOT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleReserveKneel", &keyBattleReserveKneel, SDLK_j, "STR_RESERVE_TIME_UNITS_FOR_KNEEL", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleZeroTUs", &keyBattleZeroTUs, SDLK_DELETE, "STR_EXPEND_ALL_TIME_UNITS", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy1", &keyBattleCenterEnemy1, SDLK_1, "STR_CENTER_ON_ENEMY_1", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy2", &keyBattleCenterEnemy2, SDLK_2, "STR_CENTER_ON_ENEMY_2", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy3", &keyBattleCenterEnemy3, SDLK_3, "STR_CENTER_ON_ENEMY_3", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy4", &keyBattleCenterEnemy4, SDLK_4, "STR_CENTER_ON_ENEMY_4", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy5", &keyBattleCenterEnemy5, SDLK_5, "STR_CENTER_ON_ENEMY_5", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy6", &keyBattleCenterEnemy6, SDLK_6, "STR_CENTER_ON_ENEMY_6", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy7", &keyBattleCenterEnemy7, SDLK_7, "STR_CENTER_ON_ENEMY_7", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy8", &keyBattleCenterEnemy8, SDLK_8, "STR_CENTER_ON_ENEMY_8", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy9", &keyBattleCenterEnemy9, SDLK_9, "STR_CENTER_ON_ENEMY_9", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleCenterEnemy10", &keyBattleCenterEnemy10, SDLK_0, "STR_CENTER_ON_ENEMY_10", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyBattleVoxelView", &keyBattleVoxelView, SDLK_F10, "STR_SAVE_VOXEL_VIEW", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyInvCreateTemplate", &keyInvCreateTemplate, SDLK_c, "STR_CREATE_INVENTORY_TEMPLATE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyInvApplyTemplate", &keyInvApplyTemplate, SDLK_v, "STR_APPLY_INVENTORY_TEMPLATE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyInvClear", &keyInvClear, SDLK_x, "STR_CLEAR_INVENTORY", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXC, "keyInvAutoEquip", &keyInvAutoEquip, SDLK_z, "STR_AUTO_EQUIP", "STR_BATTLESCAPE"));
}

void createOptionsOXCE()
{
	// OXCE hidden
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceModValidationLevel", &oxceModValidationLevel, (int)LOG_WARNING));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceRawScreenShots", &oxceRawScreenShots, false));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceFirstPersonViewFisheyeProjection", &oxceFirstPersonViewFisheyeProjection, false));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceThumbButtons", &oxceThumbButtons, true));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceThrottleMouseMoveEvent", &oxceThrottleMouseMoveEvent, 0));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceDisableThinkingProgressBar", &oxceDisableThinkingProgressBar, false));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceSortDiscoveredVectorByName", &oxceSortDiscoveredVectorByName, false));

	_info.push_back(OptionInfo(OPTION_OXCE, "oxceEmbeddedOnly", &oxceEmbeddedOnly, true));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceListVFSContents", &oxceListVFSContents, false));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceEnablePaletteFlickerFix", &oxceEnablePaletteFlickerFix, false));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceRecommendedOptionsWereSet", &oxceRecommendedOptionsWereSet, false));
	_info.push_back(OptionInfo(OPTION_OXCE, "password", &password, "secret"));

	// OXCE hidden but moddable
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceStartUpTextMode", &oxceStartUpTextMode, 0, "", "HIDDEN"));

	_info.push_back(OptionInfo(OPTION_OXCE, "oxceGeoscapeDebugLogMaxEntries", &oxceGeoscapeDebugLogMaxEntries, 1000, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceGeoSlowdownFactor", &oxceGeoSlowdownFactor, 1, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceGeoShowScoreInsteadOfFunds", &oxceGeoShowScoreInsteadOfFunds, false, "", "HIDDEN"));

	_info.push_back(OptionInfo(OPTION_OXCE, "oxceBaseInfoDefenseScaleMultiplier", &oxceBaseInfoDefenseScaleMultiplier, 100, "", "HIDDEN"));
#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceBaseManufactureInfinityButton", &oxceBaseManufactureInfinityButton, true, "", "HIDDEN"));
#else
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceBaseManufactureInfinityButton", &oxceBaseManufactureInfinityButton, false, "", "HIDDEN"));
#endif

	_info.push_back(OptionInfo(OPTION_OXCE, "oxceDisableAlienInventory", &oxceDisableAlienInventory, false, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceDisableHitLog", &oxceDisableHitLog, false, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceDisableInventoryTuCost", &oxceDisableInventoryTuCost, false, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceDisableProductionDependencyTree", &oxceDisableProductionDependencyTree, false, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceDisableStatsForNerds", &oxceDisableStatsForNerds, false, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceDisableTechTreeViewer", &oxceDisableTechTreeViewer, false, "", "HIDDEN"));

	_info.push_back(OptionInfo(OPTION_OXCE, "oxceTogglePersonalLightType", &oxceTogglePersonalLightType, 1, "", "HIDDEN")); // per battle
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceToggleNightVisionType", &oxceToggleNightVisionType, 1, "", "HIDDEN"));     // per battle
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceToggleBrightnessType", &oxceToggleBrightnessType, 0, "", "HIDDEN"));       // not persisted

	_info.push_back(OptionInfo(OPTION_OXCE, "oxceEnableUnitResponseSounds", &oxceEnableUnitResponseSounds, true, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceHiddenMovementBackgroundChangeFrequency", &oxceHiddenMovementBackgroundChangeFrequency, 1, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceInventoryShowUnitSlot", &oxceInventoryShowUnitSlot, false, "", "HIDDEN"));

	// TODO: needs restart (or code change) to work properly
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceMaxEquipmentLayoutTemplates", &oxceMaxEquipmentLayoutTemplates, 20, "", "HIDDEN"));
}

void createAdvancedOptionsOXCE()
{
	// OXCE options general
#ifdef _WIN32
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceUpdateCheck", &oxceUpdateCheck, false, "STR_UPDATE_CHECK", "STR_GENERAL"));
#endif

	_info.push_back(OptionInfo(OPTION_OXCE, "autosaveSlots", &autosaveSlots, 1, "STR_AUTOSAVE_SLOTS", "STR_GENERAL")); // OXCE only
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceGeoAutosaveFrequency", &oxceGeoAutosaveFrequency, 0, "STR_GEO_AUTOSAVE_FREQUENCY", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceGeoAutosaveSlots", &oxceGeoAutosaveSlots, 1, "STR_GEO_AUTOSAVE_SLOTS", "STR_GENERAL"));

#ifdef __MOBILE__
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceLinks", &oxceLinks, true, "STR_OXCE_LINKS", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceFatFingerLinks", &oxceFatFingerLinks, true, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceQuickSearchButton", &oxceQuickSearchButton, true, "", "HIDDEN"));
#else
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceLinks", &oxceLinks, false, "STR_OXCE_LINKS", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceFatFingerLinks", &oxceFatFingerLinks, false, "", "HIDDEN"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceQuickSearchButton", &oxceQuickSearchButton, false, "", "HIDDEN"));
#endif

	_info.push_back(OptionInfo(OPTION_OXCE, "oxceHighlightNewTopics", &oxceHighlightNewTopics, true, "STR_HIGHLIGHT_NEW", "STR_GENERAL"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxcePediaShowClipSize", &oxcePediaShowClipSize, false, "STR_PEDIA_SHOW_CLIP_SIZE", "STR_GENERAL"));

	// OXCE options geoscape
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceInterceptTableSize", &oxceInterceptTableSize, 8, "STR_INTERCEPT_TABLE_SIZE", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceEnableSlackingIndicator", &oxceEnableSlackingIndicator, true, "STR_SHOW_SLACKING_INDICATOR", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceInterceptGuiMaintenanceTime", &oxceInterceptGuiMaintenanceTime, 2, "STR_SHOW_MAINTENANCE_TIME", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceShowETAMode", &oxceShowETAMode, 0, "STR_SHOW_ETA", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceUfoLandingAlert", &oxceUfoLandingAlert, false, "STR_UFO_LANDING_ALERT", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceRememberDisabledCraftWeapons", &oxceRememberDisabledCraftWeapons, false, "STR_REMEMBER_DISABLED_CRAFT_WEAPONS", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceGeoscapeEventsInstantDelivery", &oxceGeoscapeEventsInstantDelivery, true, "STR_GEO_EVENT_INSTANT_DELIVERY", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceShowBaseNameInPopups", &oxceShowBaseNameInPopups, false, "STR_SHOW_BASE_NAME_IN_POPUPS", "STR_GEOSCAPE"));

	// OXCE options basescape
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceAlternateCraftEquipmentManagement", &oxceAlternateCraftEquipmentManagement, false, "STR_ALTERNATE_CRAFT_EQUIPMENT_MANAGEMENT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceBaseInfoScaleEnabled", &oxceBaseInfoScaleEnabled, false, "STR_BASE_INFO_SCALE", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceResearchScrollSpeed", &oxceResearchScrollSpeed, 1, "STR_RESEARCH_SCROLL_SPEED", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceResearchScrollSpeedWithCtrl", &oxceResearchScrollSpeedWithCtrl, 10, "STR_RESEARCH_SCROLL_SPEED_CTRL", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceManufactureFilterSuppliesOK", &oxceManufactureFilterSuppliesOK, false, "STR_MANUFACTURE_FILTER_SUPPLIES_OK", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceManufactureScrollSpeed", &oxceManufactureScrollSpeed, 1, "STR_MANUFACTURE_SCROLL_SPEED", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceManufactureScrollSpeedWithCtrl", &oxceManufactureScrollSpeedWithCtrl, 10, "STR_MANUFACTURE_SCROLL_SPEED_CTRL", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxcePersonalLayoutIncludingArmor", &oxcePersonalLayoutIncludingArmor, true, "STR_PERSONAL_LAYOUT_INCLUDING_ARMOR", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceManualPromotions", &oxceManualPromotions, false, "STR_MANUALPROMOTIONS", "STR_BASESCAPE"));

	// OXCE options battlescape
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceWoundedDefendBaseIf", &oxceWoundedDefendBaseIf, 100, "STR_WOUNDED_DEFEND_BASE_IF", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxcePlayBriefingMusicDuringEquipment", &oxcePlayBriefingMusicDuringEquipment, false, "STR_PLAY_BRIEFING_MUSIC_DURING_EQUIPMENT", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceNightVisionColor", &oxceNightVisionColor, 5, "STR_NIGHT_VISION_COLOR", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceAutoNightVisionThreshold", &oxceAutoNightVisionThreshold, 15, "STR_AUTO_NIGHT_VISION_THRESHOLD", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceShowAccuracyOnCrosshair", &oxceShowAccuracyOnCrosshair, 1, "STR_SHOW_ACCURACY_ON_CROSSHAIR", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceAutoSell", &oxceAutoSell, false, "STR_AUTO_SELL", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceAutomaticPromotions", &oxceAutomaticPromotions, true, "STR_AUTOMATICPROMOTIONS", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceEnableOffCentreShooting", &oxceEnableOffCentreShooting, false, "STR_OFF_CENTRE_SHOOTING", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "oxceCrashedOrLanded", &oxceCrashedOrLanded, 0, "STR_CRASHED_OR_LANDED", "STR_BATTLESCAPE"));
}

void createControlsOXCE()
{
	// OXCE controls general
	_info.push_back(OptionInfo(OPTION_OXCE, "keyToggleQuickSearch", &keyToggleQuickSearch, SDLK_q, "STR_TOGGLE_QUICK_SEARCH", "STR_GENERAL"));

	// OXCE controls geoscape
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGeoUfoTracker", &keyGeoUfoTracker, SDLK_t, "STR_UFO_TRACKER", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGeoTechTreeViewer", &keyGeoTechTreeViewer, SDLK_q, "STR_TECH_TREE_VIEWER", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGeoGlobalProduction", &keyGeoGlobalProduction, SDLK_p, "STR_PRODUCTION_OVERVIEW", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGeoGlobalResearch", &keyGeoGlobalResearch, SDLK_c, "STR_RESEARCH_OVERVIEW", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGeoGlobalAlienContainment", &keyGeoGlobalAlienContainment, SDLK_j, "STR_PRISONER_OVERVIEW", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGeoDailyPilotExperience", &keyGeoDailyPilotExperience, SDLK_e, "STR_DAILY_PILOT_EXPERIENCE", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGraphsZoomIn", &keyGraphsZoomIn, SDLK_KP_PLUS, "STR_GRAPHS_ZOOM_IN", "STR_GEOSCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyGraphsZoomOut", &keyGraphsZoomOut, SDLK_KP_MINUS, "STR_GRAPHS_ZOOM_OUT", "STR_GEOSCAPE"));

	// OXCE controls basescape
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeBuildNewBase", &keyBasescapeBuildNewBase, SDLK_n, "STR_BUILD_NEW_BASE_UC", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeBaseInformation", &keyBasescapeBaseInfo, SDLK_i, "STR_BASE_INFORMATION", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeSoldiers", &keyBasescapeSoldiers, SDLK_s, "STR_SOLDIERS_UC", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeEquipCraft", &keyBasescapeCrafts, SDLK_e, "STR_EQUIP_CRAFT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeBuildFacilities", &keyBasescapeFacilities, SDLK_f, "STR_BUILD_FACILITIES", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeResearch", &keyBasescapeResearch, SDLK_r, "STR_RESEARCH", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeManufacture", &keyBasescapeManufacture, SDLK_m, "STR_MANUFACTURE", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeTransfer", &keyBasescapeTransfer, SDLK_t, "STR_TRANSFER_UC", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapePurchase", &keyBasescapePurchase, SDLK_p, "STR_PURCHASE_RECRUIT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBasescapeSell", &keyBasescapeSell, SDLK_l, "STR_SELL_SACK_UC", "STR_BASESCAPE"));

	_info.push_back(OptionInfo(OPTION_OXCE, "keyRemoveSoldiersFromTraining", &keyRemoveSoldiersFromTraining, SDLK_x, "STR_REMOVE_SOLDIERS_FROM_TRAINING", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyAddSoldiersToTraining", &keyAddSoldiersToTraining, SDLK_z, "STR_ADD_SOLDIERS_TO_TRAINING", "STR_BASESCAPE"));

	_info.push_back(OptionInfo(OPTION_OXCE, "keyCraftLoadoutSave", &keyCraftLoadoutSave, SDLK_F5, "STR_SAVE_CRAFT_LOADOUT_TEMPLATE", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyCraftLoadoutLoad", &keyCraftLoadoutLoad, SDLK_F9, "STR_LOAD_CRAFT_LOADOUT_TEMPLATE", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyRemoveSoldiersFromAllCrafts", &keyRemoveSoldiersFromAllCrafts, SDLK_x, "STR_REMOVE_SOLDIERS_FROM_ALL_CRAFTS", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyRemoveSoldiersFromCraft", &keyRemoveSoldiersFromCraft, SDLK_z, "STR_REMOVE_SOLDIERS_FROM_CRAFT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyRemoveEquipmentFromCraft", &keyRemoveEquipmentFromCraft, SDLK_x, "STR_REMOVE_EQUIPMENT_FROM_CRAFT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyRemoveArmorFromAllCrafts", &keyRemoveArmorFromAllCrafts, SDLK_x, "STR_REMOVE_ARMOR_FROM_ALL_CRAFTS", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyRemoveArmorFromCraft", &keyRemoveArmorFromCraft, SDLK_z, "STR_REMOVE_ARMOR_FROM_CRAFT", "STR_BASESCAPE"));

	_info.push_back(OptionInfo(OPTION_OXCE, "keyInventorySave", &keyInventorySave, SDLK_F5, "STR_SAVE_EQUIPMENT_TEMPLATE", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyInventoryLoad", &keyInventoryLoad, SDLK_F9, "STR_LOAD_EQUIPMENT_TEMPLATE", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyInvSavePersonalEquipment", &keyInvSavePersonalEquipment, SDLK_s, "STR_SAVE_PERSONAL_EQUIPMENT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyInvLoadPersonalEquipment", &keyInvLoadPersonalEquipment, SDLK_l, "STR_LOAD_PERSONAL_EQUIPMENT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyInvShowPersonalEquipment", &keyInvShowPersonalEquipment, SDLK_p, "STR_PERSONAL_EQUIPMENT", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyInventoryArmor", &keyInventoryArmor, SDLK_a, "STR_INVENTORY_ARMOR", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyInventoryAvatar", &keyInventoryAvatar, SDLK_m, "STR_INVENTORY_AVATAR", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyInventoryDiaryLight", &keyInventoryDiaryLight, SDLK_d, "STR_INVENTORY_DIARY_LIGHT", "STR_BASESCAPE"));

	_info.push_back(OptionInfo(OPTION_OXCE, "keySellAll", &keySellAll, SDLK_x, "STR_SELL_ALL", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keySellAllButOne", &keySellAllButOne, SDLK_z, "STR_SELL_ALL_BUT_ONE", "STR_BASESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyTransferAll", &keyTransferAll, SDLK_x, "STR_TRANSFER_ALL", "STR_BASESCAPE"));

	_info.push_back(OptionInfo(OPTION_OXCE, "keyMarkAllAsSeen", &keyMarkAllAsSeen, SDLK_x, "STR_MARK_ALL_AS_SEEN", "STR_BASESCAPE"));

	// OXCE controls battlescape
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleUnitUp", &keyBattleUnitUp, SDLK_UNKNOWN, "STR_UNIT_LEVEL_ABOVE", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleUnitDown", &keyBattleUnitDown, SDLK_UNKNOWN, "STR_UNIT_LEVEL_BELOW", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleShowLayers", &keyBattleShowLayers, SDLK_UNKNOWN, "STR_MULTI_LEVEL_VIEW", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleUseSpecial", &keyBattleUseSpecial, SDLK_w, "STR_USE_SPECIAL_ITEM", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleActionItem1", &keyBattleActionItem1, SDLK_1, "STR_ACTION_ITEM_1", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleActionItem2", &keyBattleActionItem2, SDLK_2, "STR_ACTION_ITEM_2", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleActionItem3", &keyBattleActionItem3, SDLK_3, "STR_ACTION_ITEM_3", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleActionItem4", &keyBattleActionItem4, SDLK_4, "STR_ACTION_ITEM_4", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyBattleActionItem5", &keyBattleActionItem5, SDLK_5, "STR_ACTION_ITEM_5", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyNightVisionToggle", &keyNightVisionToggle, SDLK_SCROLLOCK, "STR_TOGGLE_NIGHT_VISION", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keyNightVisionHold", &keyNightVisionHold, SDLK_SPACE, "STR_HOLD_NIGHT_VISION", "STR_BATTLESCAPE"));
	_info.push_back(OptionInfo(OPTION_OXCE, "keySelectMusicTrack", &keySelectMusicTrack, SDLK_END, "STR_SELECT_MUSIC_TRACK", "STR_BATTLESCAPE"));
}

void createOptionsOTHER()
{
	// your fork's hidden options here
}

void createAdvancedOptionsOTHER()
{
	// your fork's advanced options here
}

void createControlsOTHER()
{
	// your fork's controls here
}


// we can get fancier with these detection routines, but for now just look for
// *something* in the data folders.  case sensitivity can make actually verifying
// that the *correct* files are there complex.
static bool _gameIsInstalled(const std::string &gameName)
{
	// look for game data in either the data or user directories
	std::string dataGameFolder = CrossPlatform::searchDataFolder(gameName, 8);
	std::string dataGameZipFile = CrossPlatform::searchDataFile(gameName + ".zip");
	std::string userGameFolder = _userFolder + gameName;
	std::string userGameZipFile = _userFolder + gameName + ".zip";
	return (CrossPlatform::folderMinSize(dataGameFolder, 8))
	    || (CrossPlatform::folderMinSize(userGameFolder, 8))
		||  CrossPlatform::fileExists( dataGameZipFile )
		||  CrossPlatform::fileExists( userGameZipFile );
}

static bool _ufoIsInstalled()
{
	return _gameIsInstalled("UFO");
}

static bool _tftdIsInstalled()
{
	return _gameIsInstalled("TFTD");
}

static void _setDefaultMods()
{
	bool haveUfo = _ufoIsInstalled();
	if (haveUfo)
	{
		mods.push_back(std::pair<std::string, bool>("xcom1", true));
	}

	if (_tftdIsInstalled())
	{
		mods.push_back(std::pair<std::string, bool>("xcom2", !haveUfo));
	}
}

/**
 * Resets the options back to their defaults.
 * @param includeMods Reset mods to default as well.
 */
void resetDefault(bool includeMods)
{
	for (auto& optionInfo : _info)
	{
		optionInfo.reset();
	}
	backupDisplay();

	if (includeMods)
	{
		mods.clear();
		if (!_dataList.empty())
		{
			_setDefaultMods();
		}
	}
}

/**
 * Loads options from a set of command line arguments,
 * in the format "-option value".
 * @param argc Number of arguments.
 * @param argv Array of argument strings.
 */
static void loadArgs()
{
	auto& argv = CrossPlatform::getArgs();
	for (size_t i = 1; i < argv.size(); ++i)
	{
		auto& arg = argv[i];
		if ((arg[0] == '-' || arg[0] == '/') && arg.length() > 1)
		{
			if (arg == "--")
			{
				break;
			}

			std::string argname;
			if (arg[1] == '-' && arg.length() > 2)
				argname = arg.substr(2, arg.length()-1);
			else
				argname = arg.substr(1, arg.length()-1);
			std::transform(argname.begin(), argname.end(), argname.begin(), ::tolower);
			if (argname == "cont" || argname == "continue")
			{
				_loadLastSave = true;
				continue;
			}
			if (argv.size() > i + 1)
			{
				++i; // we'll be consuming the next argument too
				Log(LOG_DEBUG) << "loadArgs(): "<< argname <<" -> " << argv[i];
				if (argname == "data")
				{
					_dataFolder = argv[i];
					if (_dataFolder.size() > 1 && _dataFolder[_dataFolder.size() - 1] != '/') {
						_dataFolder.push_back('/');
					}
				}
				else if (argname == "user")
				{
					_userFolder = argv[i];
					if (_userFolder.size() > 1 && _userFolder[_userFolder.size() - 1] != '/') {
						_userFolder.push_back('/');
					}
				}
				else if (argname == "cfg" || argname == "config")
				{
					_configFolder = argv[i];
					if (_configFolder.size() > 1 && _configFolder[_configFolder.size() - 1] != '/') {
						_configFolder.push_back('/');
					}
				}
				else if (argname == "master")
				{
					_masterMod = argv[i];
				}
				else if (argname == "load")
				{
					_loadLastSave = true;
					_loadThisSave = argv[i];
				}
				else
				{
					//save this command line option for now, we will apply it later
					_commandLine[argname] = argv[i];
				}
			}
			else
			{
				Log(LOG_WARNING) << "Unknown option: " << argname;
			}
		}
	}
}

/*
 * Displays command-line help when appropriate.
 * @param argc Number of arguments.
 * @param argv Array of argument strings.
 */
static bool showHelp()
{
	std::ostringstream help;
	help << "OpenXcom " << OPENXCOM_VERSION_SHORT << std::endl;
	help << "Usage: openxcom [OPTION]..." << std::endl << std::endl;
	help << "-data PATH" << std::endl;
	help << "        use PATH as the default Data Folder instead of auto-detecting" << std::endl << std::endl;
	help << "-user PATH" << std::endl;
	help << "        use PATH as the default User Folder instead of auto-detecting" << std::endl << std::endl;
	help << "-cfg PATH  or  -config PATH" << std::endl;
	help << "        use PATH as the default Config Folder instead of auto-detecting" << std::endl << std::endl;
	help << "-master MOD" << std::endl;
	help << "        set MOD to the current master mod (eg. -master xcom2)" << std::endl << std::endl;
	help << "-KEY VALUE" << std::endl;
	help << "        override option KEY with VALUE (eg. -displayWidth 640)" << std::endl << std::endl;
	help << "-continue" << std::endl;
	help << "        load last save" << std::endl << std::endl;
	help << "-load FILENAME" << std::endl;
	help << "        load the specified FILENAME (from the corresponding master mod subfolder)" << std::endl << std::endl;
	help << "-version" << std::endl;
	help << "        show version number" << std::endl << std::endl;
	help << "-help" << std::endl;
	help << "-?" << std::endl;
	help << "        show command-line help" << std::endl;
	auto& argv = CrossPlatform::getArgs();
	for (size_t i = 1; i < argv.size(); ++i)
	{
		auto& arg = argv[i];
		if ((arg[0] == '-' || arg[0] == '/') && arg.length() > 1)
		{
			if (arg == "--")
			{
				break;
			}

			std::string argname;
			if (arg[1] == '-' && arg.length() > 2)
				argname = arg.substr(2, arg.length()-1);
			else
				argname = arg.substr(1, arg.length()-1);
			std::transform(argname.begin(), argname.end(), argname.begin(), ::tolower);
			if (argname == "help" || argname == "?")
			{
				std::cout << help.str();
				return true;
			}
			if (argname == "version")
			{
				std::cout << OPENXCOM_VERSION_SHORT << OPENXCOM_VERSION_GIT << std::endl;
				return true;
			}
			if (argname == "cont" || argname == "continue")
			{
				continue;
			}

			// skip next option argument, only couple options do not have it.
			++i;
		}
		else
		{
			std::cerr << "Unknown parameter '" << arg << "'" << std::endl;
			return true;
		}
	}
	return false;
}

const std::map<std::string, ModInfo> &getModInfos() { return _modInfos; }

/**
 * Splits the game's User folder by master mod,
 * creating a subfolder for each one.
 * Moving the saves from userFolder into subfolders
 * has been removed.
 */
static void userSplitMasters()
{
	for (const auto& pair : _modInfos) {
		if (pair.second.isMaster()) {
			std::string masterFolder = _userFolder + pair.first;
			if (!CrossPlatform::folderExists(masterFolder)) {
				CrossPlatform::createFolder(masterFolder);
			}
		}
	}
}

/**
 * Handles the initialization of setting up default options
 * and finding and loading any existing ones.
 * @param argc Number of arguments.
 * @param argv Array of argument strings.
 * @return Do we start the game?
 */
bool init()
{
	if (showHelp())
		return false;
	create();
	resetDefault(true);
	loadArgs();
	setFolders();
	_setDefaultMods();
	updateOptions();

	// set up the logging reportingLevel
#ifndef NDEBUG
	Logger::reportingLevel() = LOG_DEBUG;
#else
	Logger::reportingLevel() = LOG_INFO;
#endif

	if (Options::verboseLogging)
		Logger::reportingLevel() = LOG_VERBOSE;

	// this enables writes to the log file and filters already emitted messages
	CrossPlatform::setLogFileName(getUserFolder() + "openxcom.log");

	Log(LOG_INFO) << "OpenXcom Version: " << OPENXCOM_VERSION_SHORT << OPENXCOM_VERSION_GIT;
#ifdef _WIN64
	Log(LOG_INFO) << "Platform: Windows 64 bit";
#elif _WIN32
	Log(LOG_INFO) << "Platform: Windows 32 bit";
#elif __APPLE__
	Log(LOG_INFO) << "Platform: OSX";
#elif  __ANDROID_API__
	Log(LOG_INFO) << "Platform: Android " << __ANDROID_API__;
#elif __linux__
	Log(LOG_INFO) << "Platform: Linux";
#else
	Log(LOG_INFO) << "Platform: Unix-like";
#endif

	Log(LOG_INFO) << "Data folder is: " << _dataFolder;
	Log(LOG_INFO) << "Data search is: ";
	for (const auto& dataPath : _dataList)
	{
		Log(LOG_INFO) << "- " << dataPath;
	}
	Log(LOG_INFO) << "User folder is: " << _userFolder;
	Log(LOG_INFO) << "Config folder is: " << _configFolder;
	Log(LOG_INFO) << "Options loaded successfully.";

	FileMap::clear(false, Options::oxceEmbeddedOnly);
	return true;
}

// called from the dos screen state (StartState)
void refreshMods()
{
	if (Options::reload)
	{
		_masterMod = "";
	}

	_modInfos.clear();
	SDL_RWops *rwops = CrossPlatform::getEmbeddedAsset("standard.zip");
	if (rwops) {
		Log(LOG_INFO) << "Scanning embedded standard mods...";
		FileMap::scanModZipRW(rwops, "exe:standard.zip");
	}
	if (Options::oxceEmbeddedOnly && rwops) {
		Log(LOG_INFO) << "Modding embedded resources is disabled, set 'oxceEmbeddedOnly: false' in options.cfg to enable.";
	} else {
		Log(LOG_INFO) << "Scanning standard mods in '" << getDataFolder() << "'...";
		FileMap::scanModDir(getDataFolder(), "standard", true);
	}
	Log(LOG_INFO) << "Scanning user mods in '" << getUserFolder() << "'...";
	FileMap::scanModDir(getUserFolder(), "mods", false);
#ifdef __MOBILE__
	if (getDataFolder() == getUserFolder())
	{
		Log(LOG_INFO) << "Skipped scanning user mods in the data folder, because it's the same folder as the user folder.";
	}
	else
	{
		Log(LOG_INFO) << "Scanning user mods in '" << getDataFolder() << "'...";
		FileMap::scanModDir(getDataFolder(), "mods", false);
	}
#endif

	// Check mods' dependencies on other mods and extResources (UFO, TFTD, etc),
	// also breaks circular dependency loops.
	FileMap::checkModsDependencies();

	// Now we can get the list of ModInfos from the FileMap -
	// those are the mods that can possibly be loaded.
	_modInfos = FileMap::getModInfos();

	// remove mods from list that no longer exist
	bool nonMasterModFound = false;
	std::map<std::string, bool> corruptedMasters;
	for (auto i = mods.begin(); i != mods.end();)
	{
		auto modIt = _modInfos.find(i->first);
		if (_modInfos.end() == modIt)
		{
			Log(LOG_VERBOSE) << "removing references to missing mod: " << i->first;
			i = mods.erase(i);
			continue;
		}
		else
		{
			if ((*modIt).second.isMaster())
			{
				if (nonMasterModFound)
				{
					Log(LOG_ERROR) << "Removing master mod '" << i->first << "' from the list, because it is on a wrong position. It will be re-added automatically.";
					corruptedMasters[i->first] = i->second;
					i = mods.erase(i);
					continue;
				}
			}
			else
			{
				nonMasterModFound = true;
			}
		}
		++i;
	}
	// re-insert corrupted masters at the beginning of the list
	for (const auto& pair : corruptedMasters)
	{
		std::pair<std::string, bool> newMod(pair.first, pair.second);
		mods.insert(mods.begin(), newMod);
	}

	// add in any new mods picked up from the scan and ensure there is but a single
	// master active
	std::string activeMaster;
	std::string inactiveMaster;
	for (auto i = _modInfos.cbegin(); i != _modInfos.cend(); ++i)
	{
		bool found = false;
		for (auto j = mods.begin(); j != mods.end(); ++j)
		{
			if (i->first == j->first)
			{
				found = true;
				if (i->second.isMaster())
				{
					if (!_masterMod.empty())
					{
						j->second = (_masterMod == j->first);
					}
					if (j->second)
					{
						if (!activeMaster.empty())
						{
							Log(LOG_WARNING) << "Too many active masters detected; turning off " << j->first;
							j->second = false;
						}
						else
						{
							activeMaster = j->first;
						}
					}
					else
					{
						// prefer activating standard masters over a possibly broken
						// third party master
						if (inactiveMaster.empty() || j->first == "xcom1" || j->first == "xcom2")
						{
							inactiveMaster = j->first;
						}
					}
				}

				break;
			}
		}
		if (found)
		{
			continue;
		}

		// not active by default
		std::pair<std::string, bool> newMod(i->first, false);
		if (i->second.isMaster())
		{
			// it doesn't matter what order the masters are in since
			// only one can be active at a time anyway
			mods.insert(mods.begin(), newMod);

			if (inactiveMaster.empty())
			{
				inactiveMaster = i->first;
			}
		}
		else
		{
			mods.push_back(newMod);
		}
	}

	if (activeMaster.empty())
	{
		if (inactiveMaster.empty())
		{
			Log(LOG_ERROR) << "no mod masters available";
			throw Exception("No X-COM installations found");
		}
		else
		{
			Log(LOG_INFO) << "no master already active; activating " << inactiveMaster;
			std::find(mods.begin(), mods.end(), std::pair<std::string, bool>(inactiveMaster, false))->second = true;
			_masterMod = inactiveMaster;
		}
	}
	else
	{
		_masterMod = activeMaster;
	}
	save();
}

void updateMods()
{
	setDataFolder(CrossPlatform::dirFilename(CrossPlatform::searchDataFolder("common")));

	// pick up stuff in common before-hand
	FileMap::clear(false, Options::oxceEmbeddedOnly);

	refreshMods();

	// check active mods that don't meet the enforced OXCE requirements
	auto* masterInf = getActiveMasterInfo();
	auto activeModsList = getActiveMods();
	bool forceQuit = false;
	for (auto* modInf : activeModsList)
	{
		if (ModConfirmExtendedState::isModNotValid(modInf, masterInf))
		{
			Log(LOG_ERROR) << "- " << modInf->getId() << " v" << modInf->getVersion();
			if (!modInf->isEngineOk())
			{
				forceQuit = true;
				if (modInf->getRequiredExtendedEngine() != OPENXCOM_VERSION_ENGINE)
				{
					Log(LOG_ERROR) << "Mod '" << modInf->getName() << "' require OXC " << modInf->getRequiredExtendedEngine() << " engine to run";
				}
				else
				{
					Log(LOG_ERROR) << "Mod '" << modInf->getName() << "' enforces at least OXC " << OPENXCOM_VERSION_ENGINE << " v" << modInf->getRequiredExtendedVersion();
				}
			}
			if (!modInf->isParentMasterOk(masterInf))
			{
				Log(LOG_ERROR) << "Mod '" << modInf->getName() << "' require version " << modInf->getRequiredMasterVersion() << " of master mod to run (current one is " << masterInf->getVersion() << ")";
			}
		}
	}
	if (forceQuit)
	{
		throw Exception("Incompatible mods are active. Please upgrade OpenXcom.");
	}

	FileMap::setup(activeModsList, Options::oxceEmbeddedOnly);
	userSplitMasters();

	Log(LOG_INFO) << "Active mods:";
	auto activeMods = getActiveMods();
	for (auto* modInf : activeMods)
	{
		Log(LOG_INFO) << "- " << modInf->getId() << " v" << modInf->getVersion();
	}
}

/**
 * Is the password correct?
 * @return Mostly false.
 */
bool isPasswordCorrect()
{
	if (_passwordCheck < 0)
	{
		std::string md5hash = md5(Options::password);
		if (md5hash == "52bd8e15118862c40fc0d6107e197f42")
			_passwordCheck = 1;
		else
			_passwordCheck = 0;
	}

	return _passwordCheck > 0;
}

/**
 * Gets the currently active master mod.
 * @return Mod id.
 */
std::string getActiveMaster()
{
	return _masterMod;
}

/**
 * Gets the master mod info.
 */
const ModInfo* getActiveMasterInfo()
{
	return &_modInfos.at(_masterMod);
}

/**
 * Gets the xcom ruleset info.
 */
const ModInfo* getXcomRulesetInfo()
{
	if (_modInfos.find("xcom1") != _modInfos.end())
		return &_modInfos.at("xcom1");
	else if (_modInfos.find("xcom2") != _modInfos.end())
		return &_modInfos.at("xcom2");
	else return nullptr;
}

bool getLoadLastSave()
{
	return _loadLastSave && !_loadLastSaveExpended;
}

const std::string& getLoadThisSave()
{
	return _loadThisSave;
}

void expendLoadLastSave()
{
	_loadLastSaveExpended = true;
}

/**
 * Sets up the game's Data folder where the data files
 * are loaded from and the User folder and Config
 * folder where settings and saves are stored in.
 */
void setFolders()
{
	_dataList = CrossPlatform::findDataFolders();
	if (!_dataFolder.empty())
	{
		_dataList.insert(_dataList.begin(), _dataFolder);
		Log(LOG_DEBUG) << "setFolders(): inserting " << _dataFolder;
	}
	if (_userFolder.empty())
	{
		std::vector<std::string> user = CrossPlatform::findUserFolders();

		if (_configFolder.empty())
		{
			_configFolder = CrossPlatform::findConfigFolder();
		}

		// Look for an existing user folder
		for (std::vector<std::string>::reverse_iterator i = user.rbegin(); i != user.rend(); ++i)
		{
			if (CrossPlatform::folderExists(*i))
			{
				_userFolder = *i;
				break;
			}
		}

		// Set up folders
		if (_userFolder.empty())
		{
			for (const auto& userFolder : user)
			{
				if (CrossPlatform::createFolder(userFolder))
				{
					_userFolder = userFolder;
					break;
				}
			}
		}
	}
	if (!_userFolder.empty())
	{
		// create mod folder if it doesn't already exist
		CrossPlatform::createFolder(_userFolder + "mods");
	}

	if (_configFolder.empty())
	{
		_configFolder = _userFolder;
	}
}

/**
 * Updates the game's options with those in the configuration
 * file, if it exists yet, and any supplied on the command line.
 */
void updateOptions()
{
	// Load existing options
	if (CrossPlatform::folderExists(_configFolder))
	{
		if (CrossPlatform::fileExists(_configFolder + "options.cfg"))
		{
			load();
#ifndef EMBED_ASSETS
			Options::oxceEmbeddedOnly = false;
#endif
		}
		else
		{
			save();
		}
	}
	// Create config folder and save options
	else
	{
		CrossPlatform::createFolder(_configFolder);
		save();
	}

	// now apply options set on the command line, overriding defaults and those loaded from config file
	//if (!_commandLine.empty())
	for (auto& optionInfo : _info)
	{
		optionInfo.load(_commandLine, true);
	}
}

/**
 * Loads options from a YAML file.
 * @param filename YAML filename.
 * @return Was the loading successful?
 */
bool load(const std::string &filename)
{
	std::string s = _configFolder + filename + ".cfg";
	try
	{
		YAML::YamlRootNodeReader reader(s);
		// Ignore old options files
		if (reader["options"]["NewBattleMission"])
		{
			return false;
		}
		for (auto& optionInfo : _info)
		{
			optionInfo.load(reader["options"]);
		}

		mods.clear();
		for (const auto& mod : reader["mods"].children())
		{
			std::string id = mod["id"].readVal<std::string>();
			bool active = mod["active"].readVal(false);
			mods.push_back(std::pair<std::string, bool>(id, active));
		}
		if (mods.empty())
		{
			_setDefaultMods();
		}
	}
	catch (YAML::Exception &e)
	{
		Log(LOG_WARNING) << e.what();
		return false;
	}
	return true;
}

/**
 * Saves options to a YAML file.
 * @param filename YAML filename.
 * @return Was the saving successful?
 */
bool save(const std::string &filename)
{
	std::string yaml;
	try
	{
		YAML::YamlRootNodeWriter writer;
		writer.setAsMap();
		auto modsWriter = writer["mods"];
		modsWriter.setAsSeq();
		for (const auto& pair : mods)
		{
			auto modWriter = modsWriter.write();
			modWriter.setAsMap();
			modWriter.write("active", pair.second);
			modWriter.write("id", pair.first);
		}
		auto optionsWriter = writer["options"];
		optionsWriter.setAsMap();
		auto sortedInfo = _info;
		std::sort(sortedInfo.begin(), sortedInfo.end(), [](const OptionInfo& a, const OptionInfo& b) { return a.id() < b.id(); });
		for (const auto& optionInfo : sortedInfo)
		{
			optionInfo.save(optionsWriter);
		}
		yaml = writer.emit().yaml;
	}
	catch (YAML::Exception &e)
	{
		Log(LOG_WARNING) << e.what();
		return false;
	}
	std::string filepath = _configFolder + filename + ".cfg";
	if (!CrossPlatform::writeFile(filepath, yaml + "\n"))
	{
		Log(LOG_WARNING) << "Failed to save " << filepath;
		return false;
	}
	return true;
}

/**
 * Returns the game's current Data folder where resources
 * and X-Com files are loaded from.
 * @return Full path to Data folder.
 */
std::string getDataFolder()
{
	return _dataFolder;
}

/**
 * Changes the game's current Data folder where resources
 * and X-Com files are loaded from.
 * @param folder Full path to Data folder.
 */
void setDataFolder(const std::string &folder)
{
	_dataFolder = folder;
	Log(LOG_DEBUG) << "setDataFolder(" << folder <<");";
}

/**
 * Returns the game's list of possible Data folders.
 * @return List of Data paths.
 */
const std::vector<std::string> &getDataList()
{
	return _dataList;
}

/**
 * Returns the game's User folder where
 * saves are stored in.
 * @return Full path to User folder.
 */
std::string getUserFolder()
{
	return _userFolder;
}

/**
 * Returns the game's Config folder where
 * settings are stored in. Normally the same
 * as the User folder.
 * @return Full path to Config folder.
 */
std::string getConfigFolder()
{
	return _configFolder;
}

/**
 * Returns the game's User folder for the
 * currently loaded master mod.
 * @return Full path to User folder.
 */
std::string getMasterUserFolder()
{
	return _userFolder + _masterMod + "/";
}

/**
 * Returns the game's list of all available option information.
 * @return List of OptionInfo's.
 */
const std::vector<OptionInfo> &getOptionInfo()
{
	return _info;
}

/**
 * Returns a list of currently active mods.
 * They must be enabled and activable.
 * @sa ModInfo::canActivate
 * @return List of info for the active mods.
 */
std::vector<const ModInfo *> getActiveMods()
{
	std::vector<const ModInfo*> activeMods;
	for (const auto& pair : mods)
	{
		if (pair.second)
		{
			const ModInfo *info = &_modInfos.at(pair.first);
			if (info->canActivate(_masterMod))
			{
				activeMods.push_back(info);
			}
		}
	}
	return activeMods;
}

/**
 * Saves display settings temporarily to be able
 * to revert to old ones.
 */
void backupDisplay()
{
	Options::newDisplayWidth = Options::displayWidth;
	Options::newDisplayHeight = Options::displayHeight;
	Options::newBattlescapeScale = Options::battlescapeScale;
	Options::newGeoscapeScale = Options::geoscapeScale;
	Options::newOpenGL = Options::useOpenGL;
	Options::newScaleFilter = Options::useScaleFilter;
	Options::newHQXFilter = Options::useHQXFilter;
	Options::newOpenGLShader = Options::useOpenGLShader;
	Options::newXBRZFilter = Options::useXBRZFilter;
	Options::newRootWindowedMode = Options::rootWindowedMode;
	Options::newWindowedModePositionX = Options::windowedModePositionX;
	Options::newWindowedModePositionY = Options::windowedModePositionY;
	Options::newFullscreen = Options::fullscreen;
	Options::newAllowResize = Options::allowResize;
	Options::newBorderless = Options::borderless;
}

/**
 * Switches old/new display options for temporarily
 * testing a new display setup.
 */
void switchDisplay()
{
	std::swap(displayWidth, newDisplayWidth);
	std::swap(displayHeight, newDisplayHeight);
	std::swap(useOpenGL, newOpenGL);
	std::swap(useScaleFilter, newScaleFilter);
	std::swap(battlescapeScale, newBattlescapeScale);
	std::swap(geoscapeScale, newGeoscapeScale);
	std::swap(useHQXFilter, newHQXFilter);
	std::swap(useOpenGLShader, newOpenGLShader);
	std::swap(useXBRZFilter, newXBRZFilter);
	std::swap(rootWindowedMode, newRootWindowedMode);
	std::swap(windowedModePositionX, newWindowedModePositionX);
	std::swap(windowedModePositionY, newWindowedModePositionY);
	std::swap(fullscreen, newFullscreen);
	std::swap(allowResize, newAllowResize);
	std::swap(borderless, newBorderless);
}

}

}
