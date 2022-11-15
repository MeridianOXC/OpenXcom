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
#include "DogfightState.h"
#include <cmath>
#include <sstream>
#include "GeoscapeState.h"
#include "../Engine/Game.h"
#include "../Engine/Screen.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Surface.h"
#include "../Engine/Action.h"
#include "../Interface/ImageButton.h"
#include "../Interface/Text.h"
#include "../Engine/Timer.h"
#include "../Engine/Collections.h"
#include "Globe.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleSoldier.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/Ufo.h"
#include "../Mod/RuleUfo.h"
#include "../Mod/AlienRace.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Savegame/Base.h"
#include "../Savegame/CraftWeaponProjectile.h"
#include "../Savegame/Country.h"
#include "../Mod/RuleCountry.h"
#include "../Savegame/Region.h"
#include "../Mod/RuleRegion.h"
#include "../Savegame/AlienMission.h"
#include "DogfightErrorState.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

// UFO blobs graphics ...
const int DogfightState::_ufoBlobs[8][13][13] =
{
		/*0 STR_VERY_SMALL */
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 3, 5, 3, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
		/*1 STR_SMALL */
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 4, 5, 4, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
		/*2 STR_MEDIUM_UC */
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 5, 5, 5, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
		/*3 STR_LARGE */
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
		/*4 STR_VERY_LARGE */
	{
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}
	},
		/*5 STR_HUGE */
	{
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0}
	},
		/*6 STR_VERY_HUGE :p */
	{
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0},
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0}
	},
		/*7 STR_ENOURMOUS */
	{
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0},
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0}
	}
};

// Projectile blobs
const int DogfightState::_projectileBlobs[4][6][3] =
{
		/*0 STR_STINGRAY_MISSILE ?*/
	{
		{0, 1, 0},
		{1, 9, 1},
		{1, 4, 1},
		{0, 3, 0},
		{0, 2, 0},
		{0, 1, 0}
	},
		/*1 STR_AVALANCHE_MISSILE ?*/
	{
		{1, 2, 1},
		{2, 9, 2},
		{2, 5, 2},
		{1, 3, 1},
		{0, 2, 0},
		{0, 1, 0}
	},
		/*2 STR_CANNON_ROUND ?*/
	{
		{0, 0, 0},
		{0, 7, 0},
		{0, 2, 0},
		{0, 1, 0},
		{0, 0, 0},
		{0, 0, 0}
	},
		/*3 STR_FUSION_BALL ?*/
	{
		{2, 4, 2},
		{4, 9, 4},
		{2, 4, 2},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0}
	}
};
/**
 * Initializes all the elements in the Dogfight window.
 * @param game Pointer to the core game.
 * @param state Pointer to the Geoscape.
 * @param craft Pointer to the craft intercepting.
 * @param ufo Pointer to the UFO being intercepted.
 * @param ufoIsAttacking Is UFO the aggressor?
 */
DogfightState::DogfightState(GeoscapeState *state, Craft *craft, Ufo *ufo, bool ufoIsAttacking) :
	_state(state), _craft(craft), _ufo(ufo),
	_ufoIsAttacking(ufoIsAttacking), _disableDisengage(false), _disableCautious(false), _craftIsDefenseless(false), _selfDestructPressed(false),
	_timeout(50), _currentDist(640), _targetDist(560),
	_end(false), _endUfoHandled(false), _endCraftHandled(false), _ufoBreakingOff(false), _destroyUfo(false), _destroyCraft(false),
	_minimized(false), _endDogfight(false), _animatingHit(false), _waitForPoly(false), _waitForAltitude(false), _ufoSize(0), _craftHeight(0), _currentCraftDamageColor(0),
	_interceptionNumber(0), _interceptionsCount(0), _x(0), _y(0), _minimizedIconX(0), _minimizedIconY(0), _firedAtLeastOnce(false), _experienceAwarded(false),
	_delayedRecolorDone(false)
{
	_screen = false;
	_craft->setInDogfight(true);
	_weaponNum = _craft->getRules()->getWeapons();
	if (_weaponNum > RuleCraft::WeaponMax)
		_weaponNum = RuleCraft::WeaponMax;

	for(int i = 0; i < _weaponNum; ++i)
	{
		_weaponEnabled[i] = true;
		_weaponFireCountdown[i] = 0;
		_tractorLockedOn[i] = false;

		CraftWeapon* w = _craft->getWeapons()->at(i);
		if (w)
		{
			_weaponEnabled[i] = !w->isDisabled();
		}
	}

	// pilot modifiers
	const std::vector<Soldier*> pilots = _craft->getPilotList(false);

	for (std::vector<Soldier*>::const_iterator p = pilots.begin(); p != pilots.end(); ++p)
	{
		(*p)->prepareStatsWithBonuses(_game->getMod()); // refresh soldier bonuses
	}
	_pilotAccuracyBonus = _craft->getPilotAccuracyBonus(pilots, _game->getMod());
	_pilotDodgeBonus = _craft->getPilotDodgeBonus(pilots, _game->getMod());
	_pilotApproachSpeedModifier = _craft->getPilotApproachSpeedModifier(pilots, _game->getMod());

	_craftAccelerationBonus = 2; // vanilla
	if (!pilots.empty())
	{
		_craftAccelerationBonus = std::min(4, (_craft->getCraftStats().accel / 3) + 1);
	}

	// HK options
	if (_ufoIsAttacking)
	{
		if (_ufo->getCraftStats().speedMax >= _craft->getCraftStats().speedMax)
		{
			_disableDisengage = true;
		}
		if (_weaponNum == 0)
		{
			_disableCautious = true;
		}
		// make sure the HK attacks its primary target first!
		{
			Craft* target = dynamic_cast<Craft*>(_ufo->getDestination());
			if (target)
			{
				if (_craft != target)
				{
					// push secondary targets a tiny bit away from the HK
					_currentDist += 16;
				}
				else
				{
					// approach primary target at maximum approach speed
					_pilotApproachSpeedModifier = 4;
				}
			}
		}
	}

	// Create objects
	_window = new Surface(160, 96, _x, _y);
	_battle = new Surface(77, 74, _x + 3, _y + 3);
	for(int i = 0; i < _weaponNum; ++i)
	{
		const int w_off = i % 2 ? 64 : 4;
		const int r_off = i % 2 ? 43 : 19;
		const int y_off = 52 - (i / 2) * 28;

		_weapon[i] = new InteractiveSurface(15, 17, _x + w_off, _y + y_off);
		_range[i] = new Surface(21, 74, _x + r_off, _y + 3);
		_txtAmmo[i] = new Text(16, 9, _x + w_off, _y + y_off + 18);
	}
	_craftSprite = new Surface(22, 25, _x + 93, _y + 40);
	_damage = new Surface(22, 25, _x + 93, _y + 40);
	_craftShield = new Surface(22, 25, _x + 93, _y + 40);

	_btnMinimize = new InteractiveSurface(12, 12, _x, _y);
	_preview = new InteractiveSurface(160, 96, _x, _y);
	_btnStandoff = new ImageButton(36, 15, _x + 83, _y + 4);
	_btnCautious = new ImageButton(36, 15, _x + 120, _y + 4);
	_btnStandard = new ImageButton(36, 15, _x + 83, _y + 20);
	_btnAggressive = new ImageButton(36, 15, _x + 120, _y + 20);
	_btnDisengage = new ImageButton(36, 15, _x + 120, _y + 36);
	_btnUfo = new ImageButton(36, 17, _x + 120, _y + 52);
	_txtDistance = new Text(40, 9, _x + 116, _y + 72);
	_txtStatus = new Text(154, 9, _x + 4, _y + 85);
	_btnMinimizedIcon = new InteractiveSurface(32, 20, _minimizedIconX, _minimizedIconY);
	_txtInterceptionNumber = new Text(16, 9, _minimizedIconX + 18, _minimizedIconY + 6);

	_mode = _ufoIsAttacking ? _btnAggressive : _btnStandoff;
	_craftDamageAnimTimer = new Timer(500);

	moveWindow();

	// Set palette
	setInterface("dogfight");

	add(_window);
	add(_battle);
	for(int i = 0; i < _weaponNum; ++i)
	{
		add(_weapon[i]);
		add(_range[i]);
	}
	add(_craftSprite);
	add(_damage);
	add(_craftShield);
	add(_btnMinimize);
	add(_btnStandoff, "standoffButton", "dogfight", _window);
	add(_btnCautious, "cautiousButton", "dogfight", _window);
	add(_btnStandard, "standardButton", "dogfight", _window);
	add(_btnAggressive, "aggressiveButton", "dogfight", _window);
	add(_btnDisengage, "disengageButton", "dogfight", _window);
	add(_btnUfo, "ufoButton", "dogfight", _window);
	for(int i = 0; i < _weaponNum; ++i)
	{
		add(_txtAmmo[i], "numbers", "dogfight", _window);
	}
	add(_txtDistance, "distance", "dogfight", _window);
	add(_preview);
	add(_txtStatus, "text", "dogfight", _window);
	add(_btnMinimizedIcon);
	add(_txtInterceptionNumber, "minimizedNumber", "dogfight");

	_btnStandoff->invalidate(false);
	_btnCautious->invalidate(false);
	_btnStandard->invalidate(false);
	_btnAggressive->invalidate(false);
	_btnDisengage->invalidate(false);
	_btnUfo->invalidate(false);

	// Set up objects
	RuleInterface *dogfightInterface = _game->getMod()->getInterface("dogfight");

	auto crop = _game->getMod()->getSurface("INTERWIN.DAT")->getCrop();
	crop.setX(0);
	crop.setY(0);
	crop.getCrop()->x = 0;
	crop.getCrop()->y = 0;
	crop.getCrop()->w = _window->getWidth();
	crop.getCrop()->h = _window->getHeight();
	_window->drawRect(crop.getCrop(), 15);
	crop.blit(_window);

	if (_ufoIsAttacking)
	{
		_window->drawRect(_btnStandoff->getX() + 2, _btnStandoff->getY() + 2, _btnStandoff->getWidth() - 4, _btnStandoff->getHeight() - 4, dogfightInterface->getElement("standoffButton")->color + 4);
		if (_disableCautious)
		{
			_window->drawRect(_btnCautious->getX() + 2, _btnCautious->getY() + 2, _btnCautious->getWidth() - 4, _btnCautious->getHeight() - 4, dogfightInterface->getElement("cautiousButton")->color + 4);
		}
		_window->drawRect(_btnStandard->getX() + 2, _btnStandard->getY() + 2, _btnStandard->getWidth() - 4, _btnStandard->getHeight() - 4, dogfightInterface->getElement("standardButton")->color + 4);
		if (_disableDisengage)
		{
			_window->drawRect(_btnDisengage->getX() + 2, _btnDisengage->getY() + 2, _btnDisengage->getWidth() - 4, _btnDisengage->getHeight() - 4, dogfightInterface->getElement("disengageButton")->color + 4);
		}
		int offset = dogfightInterface->getElement("minimizeButtonDummy")->TFTDMode ? 1 : 0;
		_window->drawRect(_btnMinimize->getX() + 1 + offset, _btnMinimize->getY() + 1, _btnMinimize->getWidth() - 2 - offset, _btnMinimize->getHeight() - 2, dogfightInterface->getElement("minimizeButtonDummy")->color + 4);
	}

	_preview->drawRect(crop.getCrop(), 15);
	crop.getCrop()->y = dogfightInterface->getElement("previewTop")->y;
	crop.getCrop()->h = dogfightInterface->getElement("previewTop")->h;
	crop.blit(_preview);
	crop.setY(_window->getHeight() - dogfightInterface->getElement("previewBot")->h);
	crop.getCrop()->y = dogfightInterface->getElement("previewBot")->y;
	crop.getCrop()->h = dogfightInterface->getElement("previewBot")->h;
	crop.blit(_preview);

	if (ufo->getRules()->getModSprite().empty())
	{
		crop.getCrop()->y = dogfightInterface->getElement("previewMid")->y + dogfightInterface->getElement("previewMid")->h * _ufo->getRules()->getSprite();
		crop.getCrop()->h = dogfightInterface->getElement("previewMid")->h;
	}
	else
	{
		crop = _game->getMod()->getSurface(ufo->getRules()->getModSprite())->getCrop();
	}
	crop.setX(dogfightInterface->getElement("previewTop")->x);
	crop.setY(dogfightInterface->getElement("previewTop")->h);
	crop.blit(_preview);

	_preview->setVisible(false);
	_preview->onMouseClick((ActionHandler)&DogfightState::previewClick);

	_btnMinimize->onMouseClick((ActionHandler)&DogfightState::btnMinimizeClick);
	_btnMinimize->setVisible(!_ufoIsAttacking);

	_btnStandoff->copy(_window);
	_btnStandoff->setGroup(&_mode);
	_btnStandoff->onMousePress((ActionHandler)&DogfightState::btnStandoffPress);
	_btnStandoff->onMousePress((ActionHandler)&DogfightState::btnStandoffRightPress, SDL_BUTTON_RIGHT);
	_btnStandoff->setVisible(!_ufoIsAttacking);

	_btnCautious->copy(_window);
	_btnCautious->setGroup(&_mode);
	_btnCautious->onMousePress((ActionHandler)&DogfightState::btnCautiousPress);
	_btnCautious->onMousePress((ActionHandler)&DogfightState::btnCautiousRightPress, SDL_BUTTON_RIGHT);
	_btnCautious->setVisible(!_disableCautious);

	_btnStandard->copy(_window);
	_btnStandard->setGroup(&_mode);
	_btnStandard->onMousePress((ActionHandler)&DogfightState::btnStandardPress);
	_btnStandard->onMousePress((ActionHandler)&DogfightState::btnStandardRightPress, SDL_BUTTON_RIGHT);
	_btnStandard->setVisible(!_ufoIsAttacking);

	_btnAggressive->copy(_window);
	_btnAggressive->setGroup(&_mode);
	_btnAggressive->onMousePress((ActionHandler)&DogfightState::btnAggressivePress);
	_btnAggressive->onMousePress((ActionHandler)&DogfightState::btnAggressiveRightPress, SDL_BUTTON_RIGHT);
	if (_ufoIsAttacking)
	{
		btnAggressivePress(0);
	}

	_btnDisengage->copy(_window);
	_btnDisengage->onMousePress((ActionHandler)&DogfightState::btnDisengagePress);
	_btnDisengage->onMousePress((ActionHandler)&DogfightState::btnDisengageRightPress, SDL_BUTTON_RIGHT);
	_btnDisengage->setGroup(&_mode);
	_btnDisengage->setVisible(!_disableDisengage);

	_btnUfo->copy(_window);
	_btnUfo->onMouseClick((ActionHandler)&DogfightState::btnUfoClick);

	_txtDistance->setText("640");

	if (_ufoIsAttacking)
		_txtStatus->setText(tr("STR_AGGRESSIVE_ATTACK"));
	else
		_txtStatus->setText(tr("STR_STANDOFF"));

	SurfaceSet *set = _game->getMod()->getSurfaceSet("INTICON.PCK");

	// Create the minimized dogfight icon.
	Surface *frame = set->getFrame(_craft->getSkinSprite());
	frame->blitNShade(_btnMinimizedIcon, 0, 0);
	_btnMinimizedIcon->onMouseClick((ActionHandler)&DogfightState::btnMinimizedIconClick);
	_btnMinimizedIcon->setVisible(false);

	// Draw correct number on the minimized dogfight icon.
	std::ostringstream ss1;
	if (_craft->getInterceptionOrder() == 0)
	{
		int maxInterceptionOrder = 0;
		for (std::vector<Base*>::iterator baseIt = _game->getSavedGame()->getBases()->begin(); baseIt != _game->getSavedGame()->getBases()->end(); ++baseIt)
		{
			for (std::vector<Craft*>::iterator craftIt = (*baseIt)->getCrafts()->begin(); craftIt != (*baseIt)->getCrafts()->end(); ++craftIt)
			{
				if ((*craftIt)->getInterceptionOrder() > maxInterceptionOrder)
				{
					maxInterceptionOrder = (*craftIt)->getInterceptionOrder();
				}
			}
		}
		_craft->setInterceptionOrder(++maxInterceptionOrder);
	}
	ss1 << _craft->getInterceptionOrder();
	_txtInterceptionNumber->setText(ss1.str());
	_txtInterceptionNumber->setVisible(false);

	// define the colors to be used
	_colors[CRAFT_MIN] = dogfightInterface->getElement("craftRange")->color;
	_colors[CRAFT_MAX] = dogfightInterface->getElement("craftRange")->color2;
	_colors[RADAR_MIN] = dogfightInterface->getElement("radarRange")->color;
	_colors[RADAR_MAX] = dogfightInterface->getElement("radarRange")->color2;
	_colors[DAMAGE_MIN] = dogfightInterface->getElement("damageRange")->color;
	_colors[DAMAGE_MAX] = dogfightInterface->getElement("damageRange")->color2;
	_colors[BLOB_MIN] = dogfightInterface->getElement("radarDetail")->color;
	_colors[RANGE_METER] = dogfightInterface->getElement("radarDetail")->color2;
	_colors[DISABLED_WEAPON] = dogfightInterface->getElement("disabledWeapon")->color;
	_colors[DISABLED_RANGE] = dogfightInterface->getElement("disabledWeapon")->color2;
	_colors[DISABLED_AMMO] = dogfightInterface->getElement("disabledAmmo")->color;
	_colors[SHIELD_MIN] = dogfightInterface->getElement("shieldRange")->color;
	_colors[SHIELD_MAX] = dogfightInterface->getElement("shieldRange")->color2;

	for (int i = 0; i < _weaponNum; ++i)
	{
		CraftWeapon *w = _craft->getWeapons()->at(i);

		// Slot empty or no sprite, skip!
		if (w == 0 || w->getRules()->getSprite() < 0)
			continue;

		Surface *weapon = _weapon[i], *range = _range[i];
		Text *ammo = _txtAmmo[i];
		int x1, x2;
		int x_off = 2 * (i / 2 + 1);
		if (i % 2 == 0)
		{
			x1 = x_off;
			x2 = 0;
		}
		else
		{
			x1 = 0;
			x2 = 20 - x_off;
		}

		// Draw icon
		frame = set->getFrame(w->getRules()->getSprite() + 5);
		frame->blitNShade(weapon, 0, 0);

		// Just an equipment, it doesn't have any ammo (weapon) or range (tractor beam), skip!
		if (w->getRules()->getAmmoMax() == 0 && w->getRules()->getTractorBeamPower() == 0)
			continue;

		// Used for weapon toggling.
		// Only relevant for weapons and tractor beams, not for equipment.
		_weapon[i]->onMouseClick((ActionHandler)& DogfightState::weaponClick);

		// Draw ammo.
		// Only relevant for weapons, not for tractor beams.
		if (w->getRules()->getAmmoMax() > 0)
		{
			std::ostringstream ss;
			ss << w->getAmmo();
			ammo->setText(ss.str());
		}

		// Draw range (1 km = 1 pixel)
		// Only relevant for weapons and tractor beams.
		Uint8 color = _colors[RANGE_METER];
		range->lock();

		int rangeY = range->getHeight() - w->getRules()->getRange();
		int connectY = weapon->getHeight() / 2 + weapon->getY() - range->getY();
		for (int x = x1; x <= x1 + 20 - x_off; x += 2)
		{
			range->setPixel(x, rangeY, color);
		}

		int minY = 0, maxY = 0;
		if (rangeY < connectY)
		{
			minY = rangeY;
			maxY = connectY;
		}
		else if (rangeY > connectY)
		{
			minY = connectY;
			maxY = rangeY;
		}
		for (int y = minY; y <= maxY; ++y)
		{
			range->setPixel(x1 + x2, y, color);
		}
		for (int x = x2; x <= x2 + x_off; ++x)
		{
			range->setPixel(x, connectY, color);
		}
		range->unlock();
	}

	for (int i = 0; i < _weaponNum; ++i)
	{
		if (_craft->getWeapons()->at(i) == 0)
		{
			_weapon[i]->setVisible(false);
			_range[i]->setVisible(false);
			_txtAmmo[i]->setVisible(false);
		}
	}

	// Draw damage indicator.
	frame = set->getFrame(_craft->getSkinSprite() + 11);
	frame->blitNShade(_craftSprite, 0, 0);

	_craftDamageAnimTimer->onTimer((StateHandler)&DogfightState::animateCraftDamage);

	// don't set these variables if the ufo is already engaged in a dogfight
	if (!_ufo->getEscapeCountdown())
	{
		_ufo->setFireCountdown(0);
		int escapeCountdown = _ufo->getRules()->getBreakOffTime() + RNG::generate(0, _ufo->getRules()->getBreakOffTime()) - 30 * _game->getSavedGame()->getDifficultyCoefficient();
		{
			int diff = _game->getSavedGame()->getDifficulty();
			auto& custom = _game->getMod()->getUfoEscapeCountdownCoefficients();
			if (custom.size() > (size_t)diff)
			{
				escapeCountdown = _ufo->getRules()->getBreakOffTime() + RNG::generate(0, _ufo->getRules()->getBreakOffTime());
				escapeCountdown = escapeCountdown * custom[diff] / 100;
			}
		}
		_ufo->setEscapeCountdown(std::max(1, escapeCountdown));
	}

	for (int i = 0; i < _weaponNum; ++i)
	{
		if (_craft->getWeapons()->at(i))
		{
			if (!_ufoIsAttacking)
			{
				_weaponFireInterval[i] = _craft->getWeapons()->at(i)->getRules()->getStandardReload();
			}
			else
			{
				_weaponFireInterval[i] = _craft->getWeapons()->at(i)->getRules()->getAggressiveReload();
			}
		}
	}

	// Set UFO size - going to be moved to Ufo class to implement simultaneous dogfights.
	std::string ufoSize = _ufo->getRules()->getSize();
	if (ufoSize.compare("STR_VERY_SMALL") == 0)
	{
		_ufoSize = 0;
	}
	else if (ufoSize.compare("STR_SMALL") == 0)
	{
		_ufoSize = 1;
	}
	else if (ufoSize.compare("STR_MEDIUM_UC") == 0)
	{
		_ufoSize = 2;
	}
	else if (ufoSize.compare("STR_LARGE") == 0)
	{
		_ufoSize = 3;
	}
	else
	{
		_ufoSize = 4;
	}

	// Get crafts height. Used for damage indication.
	for (int y = 0; y < _craftSprite->getHeight(); ++y)
	{
		for (int x = 0; x < _craftSprite->getWidth(); ++x)
		{
			Uint8 pixelColor = _craftSprite->getPixel(x, y);
			if (pixelColor >= _colors[CRAFT_MIN] && pixelColor < _colors[CRAFT_MAX])
			{
				++_craftHeight;
				break;
			}
		}
	}

	drawCraftDamage();
	drawCraftShield();

	// Set this as the interception handling UFO shield recharge if no other is doing it
	if (_ufo->getShieldRechargeHandle() == 0)
	{
		_ufo->setShieldRechargeHandle(_interceptionNumber);
	}
}

/**
 * Cleans up the dogfight state.
 */
DogfightState::~DogfightState()
{
	delete _craftDamageAnimTimer;
	while (!_projectiles.empty())
	{
		delete _projectiles.back();
		_projectiles.pop_back();
	}
}

/**
 * Returns true if this is a hunter-killer dogfight. Otherwise returns false.
 * @return Is this a hunter-killer dogfight?
 */
bool DogfightState::isUfoAttacking() const
{
	return _ufoIsAttacking;
}

/**
 * Runs the higher level dogfight functionality.
 */
void DogfightState::think()
{
	if (!_delayedRecolorDone)
	{
		// can't be done in the constructor (recoloring the ammo text doesn't work)
		for (int i = 0; i < _weaponNum; ++i)
		{
			if (_craft->getWeapons()->at(i) && !_weaponEnabled[i])
			{
				recolor(i, _weaponEnabled[i]);
			}
		}
		_delayedRecolorDone = true;

		// Note: init() is never called for DogfightState, so we'll do it here instead
		{
			auto& sounds = _game->getMod()->getStartDogfightSounds();
			int soundId = sounds.empty() ? Mod::NO_SOUND : sounds[RNG::generate(0, sounds.size() - 1)];
			if (soundId != Mod::NO_SOUND)
			{
				auto* customSound = _game->getMod()->getSound("GEO.CAT", soundId);
				customSound->play();
			}
		}
	}
	if (!_endDogfight)
	{
		update();
		_craftDamageAnimTimer->think(this, 0);
	}
	if (!_ufoIsAttacking || _ufo->getStatus() == Ufo::LANDED)
	{
		if (!_craft->isInDogfight() || _craft->getDestination() != _ufo || _ufo->getStatus() == Ufo::LANDED)
		{
			endDogfight();
		}
	}
}

/**
 * Animates interceptor damage by changing the color and redrawing the image.
 */
void DogfightState::animateCraftDamage()
{
	if (_minimized)
	{
		return;
	}
	--_currentCraftDamageColor;
	if (_currentCraftDamageColor < _colors[DAMAGE_MIN])
	{
		_currentCraftDamageColor = _colors[DAMAGE_MAX];
	}
	drawCraftDamage();
}

/**
 * Draws interceptor damage according to percentage of HP's left.
 */
void DogfightState::drawCraftDamage()
{
	if (_craft->getDamagePercentage() != 0)
	{
		if (!_craftDamageAnimTimer->isRunning())
		{
			_craftDamageAnimTimer->start();
			if (_currentCraftDamageColor < _colors[DAMAGE_MIN])
			{
				_currentCraftDamageColor = _colors[DAMAGE_MIN];
			}
		}
		int damagePercentage = _craft->getDamagePercentage();
		int rowsToColor = (int)floor((double)_craftHeight * (double)(damagePercentage / 100.));
		if (rowsToColor == 0)
		{
			return;
		}
		int rowsColored = 0;
		bool rowColored = false;
		for (int y = 0; y < _damage->getHeight(); ++y)
		{
			rowColored = false;
			for (int x = 0; x < _damage->getWidth(); ++x)
			{
				int craftPixel = _craftSprite->getPixel(x, y);
				if (craftPixel != 0)
				{
					_damage->setPixel(x, y, _currentCraftDamageColor);
					rowColored = true;
				}
			}
			if (rowColored)
			{
				++rowsColored;
			}
			if (rowsColored == rowsToColor)
			{
				break;
			}
		}
	}

}

/**
 * Draws the craft's shield over it's sprite according to the shield remaining
 */
void DogfightState::drawCraftShield()
{
	if (_craft->getShieldCapacity() == 0)
		return;

	int maxRow = _craftHeight - ((_craftHeight * _craft->getShield()) / _craft->getShieldCapacity());
	bool startColoring = false;
	for (int y = _craftShield->getHeight(); y >= 0; --y)
	{
		for (int x = 0; x < _craftShield->getWidth(); ++x)
		{
			int craftPixel = _craftSprite->getPixel(x, y);
			if (!startColoring && craftPixel != 0)
			{
				maxRow += (y - _craftHeight + 1);
				startColoring = true;
			}
			if (y < maxRow && craftPixel != 0)
			{
				_craftShield->setPixel(x, y, 0);
			}
			if (y >= maxRow && craftPixel != 0)
			{
				Uint8 shieldColor = std::min(_colors[SHIELD_MAX], _colors[SHIELD_MIN] + (craftPixel - _colors[CRAFT_MIN]));
				_craftShield->setPixel(x, y, shieldColor);
			}
		}
	}
}

/**
 * Animates the window with a palette effect.
 */
void DogfightState::animate()
{
	// Animate radar waves and other stuff.
	for (int x = 0; x < _window->getWidth(); ++x)
	{
		for (int y = 0; y < _window->getHeight(); ++y)
		{
			Uint8 radarPixelColor = _window->getPixel(x, y);
			if (radarPixelColor >= _colors[RADAR_MIN] && radarPixelColor < _colors[RADAR_MAX])
			{
				++radarPixelColor;
				if (radarPixelColor >= _colors[RADAR_MAX])
				{
					radarPixelColor = _colors[RADAR_MIN];
				}
				_window->setPixel(x, y, radarPixelColor);
			}
		}
	}

	_battle->clear();

	// Draw UFO.
	if (!_ufo->isDestroyed())
	{
		drawUfo();
	}

	// Draw projectiles.
	for (std::vector<CraftWeaponProjectile*>::iterator it = _projectiles.begin(); it != _projectiles.end(); ++it)
	{
		drawProjectile((*it));
	}

	// Clears text after a while
	if (_timeout == 0)
	{
		_txtStatus->setText("");
	}
	else
	{
		_timeout--;
	}

	// Animate UFO hit.
	bool lastHitAnimFrame = false;
	if (_animatingHit && _ufo->getHitFrame() > 0)
	{
		_ufo->setHitFrame(_ufo->getHitFrame() - 1);
		if (_ufo->getHitFrame() == 0)
		{
			_animatingHit = false;
			lastHitAnimFrame = true;
		}
	}

	// Animate UFO crash landing.
	if (_ufo->isCrashed() && _ufo->getHitFrame() == 0 && !lastHitAnimFrame)
	{
		--_ufoSize;
	}
}

/**
 * Updates all the elements in the dogfight, including ufo movement,
 * weapons fire, projectile movement, ufo escape conditions,
 * craft and ufo destruction conditions, and retaliation mission generation, as applicable.
 */
void DogfightState::update()
{
	bool finalRun = false;
	// Check if craft is not low on fuel when window minimized, and
	// Check if crafts destination hasn't been changed when window minimized.
	if (!_ufoIsAttacking)
	{
		Ufo* u = dynamic_cast<Ufo*>(_craft->getDestination());
		if (u != _ufo || !_craft->isInDogfight() || _craft->getLowFuel() || (_minimized && _ufo->isCrashed()))
		{
			endDogfight();
			return;
		}
	}

	if (!_minimized)
	{
		animate();
		if (!_ufo->isCrashed() && !_ufo->isDestroyed() && !_craft->isDestroyed() && !_ufo->getInterceptionProcessed())
		{
			_ufo->setInterceptionProcessed(true);
			int escapeCounter = _ufo->getEscapeCountdown();
			if (_ufoIsAttacking)
			{
				if (_disableDisengage && _ufo->getSoftlockShotCounter() >= _ufo->getRules()->getSoftlockThreshold())
				{
					escapeCounter = 1; // game is in softlock, stop being a hunter-killer and disengage!
				}
				else if (_ufo->getDamage() > _ufo->getCraftStats().damageMax / 3 && _ufo->getHuntBehavior() != 1)
				{
					// TODO: rethink: unhardcode run away thresholds?
					if (_craft->getDamage() > _craft->getDamageMax() / 2)
					{
						escapeCounter = 999; // it's gonna be tight, continue shooting...
					}
					else
					{
						escapeCounter = 1; // we're badly hurt and xcom isn't, abort immediately!
					}
				}
				else
				{
					escapeCounter = 999; // we're still ok, continue shooting...
				}
			}

			if (escapeCounter > 0)
			{
				escapeCounter--;
				_ufo->setEscapeCountdown(escapeCounter);
				// Check if UFO is breaking off.
				if (escapeCounter == 0)
				{
					_ufo->setSpeed(_ufo->getCraftStats().speedMax);
					if (_ufoIsAttacking && _ufo->isHunterKiller())
					{
						// stop being a hunter-killer and run away!
						_ufo->resetOriginalDestination(_craft);
						_ufo->setHunterKiller(false);
					}
				}
			}
			if (_ufo->getFireCountdown() > 0)
			{
				_ufo->setFireCountdown(_ufo->getFireCountdown() - 1);
			}
		}
	}
	// Crappy craft is chasing UFO.
	int speedMinusTractors = std::max(0, _ufo->getSpeed() - _ufo->getTractorBeamSlowdown());
	if (speedMinusTractors > _craft->getCraftStats().speedMax)
	{
		if (!_ufoIsAttacking || !_ufo->isHunterKiller())
		{
			_ufoBreakingOff = true;
			finalRun = true;
			setStatus("STR_UFO_OUTRUNNING_INTERCEPTOR");
		}
	}
	else
	{
		_ufoBreakingOff = false;
	}

	bool projectileInFlight = false;
	if (!_minimized)
	{
		int distanceChange = 0;

		// Update distance
		if (!_ufoBreakingOff)
		{
			if (_currentDist < _targetDist && !_ufo->isCrashed() && !_craft->isDestroyed())
			{
				distanceChange = 2 * _craftAccelerationBonus; // disengage speed
				if (_currentDist + distanceChange >_targetDist)
				{
					distanceChange = _targetDist - _currentDist;
				}
			}
			else if (_currentDist > _targetDist && !_ufo->isCrashed() && !_craft->isDestroyed())
			{
				distanceChange = -1 * _pilotApproachSpeedModifier; // engage speed
			}

			// don't let the interceptor mystically push or pull its fired projectiles
			for (std::vector<CraftWeaponProjectile*>::iterator it = _projectiles.begin(); it != _projectiles.end(); ++it)
			{
				if ((*it)->getGlobalType() != CWPGT_BEAM && (*it)->getDirection() == D_UP) (*it)->setPosition((*it)->getPosition() + distanceChange);
			}
		}
		else
		{
			distanceChange = 4; // ufo breaking off speed

			// UFOs can try to outrun our missiles, don't adjust projectile positions here
			// If UFOs ever fire anything but beams, those positions need to be adjust here though.
		}

		_currentDist += distanceChange;

		if (_game->getMod()->getShowDogfightDistanceInKm())
		{
			_txtDistance->setText(tr("STR_KILOMETERS").arg(_currentDist / 8));
		}
		else
		{
			std::ostringstream ss;
			ss << _currentDist;
			_txtDistance->setText(ss.str());
		}

		// Check and recharge craft shields
		// Check if the UFO's shields are being handled by an interception window
		if (_ufo->getShieldRechargeHandle() == 0)
		{
			_ufo->setShieldRechargeHandle(_interceptionNumber);
		}

		// UFO shields
		if ((_ufo->getShield() != 0) && (_interceptionNumber == _ufo->getShieldRechargeHandle()))
		{
			int total = _ufo->getCraftStats().shieldRecharge / 100;
			if (RNG::percent(_ufo->getCraftStats().shieldRecharge % 100))
				total++;
			_ufo->setShield(_ufo->getShield() + total);
		}

		// Player craft shields
		if (_craft->getShield() != 0)
		{
			int total = _craft->getCraftStats().shieldRecharge / 100;
			if (RNG::percent(_craft->getCraftStats().shieldRecharge % 100))
				total++;
			if (total != 0)
			{
				_craft->setShield(_craft->getShield() + total);
				drawCraftShield();
			}
		}

		// Move projectiles and check for hits.
		for (std::vector<CraftWeaponProjectile*>::iterator it = _projectiles.begin(); it != _projectiles.end(); ++it)
		{
			CraftWeaponProjectile *p = (*it);
			p->move();
			// Projectiles fired by interceptor.
			if (p->getDirection() == D_UP)
			{
				// Projectile reached the UFO - determine if it's been hit.
				if (((p->getPosition() >= _currentDist) || (p->getGlobalType() == CWPGT_BEAM && p->toBeRemoved())) && !_ufo->isCrashed() && !p->getMissed())
				{
					// UFO hit.
					int chanceToHit = (p->getAccuracy() * (100 + 300 / (5 - _ufoSize)) + 100) / 200; // vanilla xcom
					chanceToHit -= _ufo->getCraftStats().avoidBonus;
					chanceToHit += _craft->getCraftStats().hitBonus;
					chanceToHit += _pilotAccuracyBonus;
					if (RNG::percent(chanceToHit))
					{
						// Formula delivered by Volutar, altered by Extended version.
						int power = p->getDamage() * (_craft->getCraftStats().powerBonus + 100) / 100;

						// Handle UFO shields
						int damage = RNG::generate(power / 2, power);
						int shieldDamage = 0;
						if (_ufo->getShield() != 0)
						{
							shieldDamage = damage * p->getShieldDamageModifier() / 100;
							if (p->getShieldDamageModifier() == 0)
							{
								damage = 0;
							}
							else
							{
								// scale down by bleed-through factor and scale up by shield-effectiveness factor
								damage = std::max(0, shieldDamage - _ufo->getShield()) * _ufo->getCraftStats().shieldBleedThrough / p->getShieldDamageModifier();
							}
							_ufo->setShield(_ufo->getShield() - shieldDamage);
						}

						damage = std::max(0, damage - _ufo->getCraftStats().armor);
						_ufo->setDamage(_ufo->getDamage() + damage, _game->getMod());
						_state->handleDogfightExperience(); // called after setDamage
						if (_ufo->isCrashed())
						{
							_ufo->setShotDownByCraftId(_craft->getUniqueId());
							_ufo->setSpeed(0);
							_ufo->setDestination(0);
							// if the ufo got destroyed here, these no longer apply
							_ufoBreakingOff = false;
							finalRun = false;
							_end = false;
						}
						if (_ufo->getHitFrame() == 0)
						{
							_animatingHit = true;
							_ufo->setHitFrame(3);
						}

						// How hard was the ufo hit?
						if (_ufo->getShield() != 0)
						{
							setStatus("STR_UFO_SHIELD_HIT");
						}
						else
						{
							if (damage == 0)
							{
								if (shieldDamage == 0)
								{
									setStatus("STR_UFO_HIT_NO_DAMAGE");
								}
								else
								{
									setStatus("STR_UFO_SHIELD_DOWN");
								}
							}
							else
							{
								if (damage < _ufo->getCraftStats().damageMax / 2 * _game->getMod()->getUfoGlancingHitThreshold() / 100)
								{
									setStatus("STR_UFO_HIT_GLANCING");
								}
								else
								{
									setStatus("STR_UFO_HIT");
								}
							}
						}

						_game->getMod()->getSound("GEO.CAT", Mod::UFO_HIT)->play();
						p->remove();
					}
					// Missed.
					else
					{
						if (p->getGlobalType() == CWPGT_BEAM)
						{
							p->remove();
						}
						else
						{
							p->setMissed(true);
						}
					}
				}
				// Check if projectile passed it's maximum range.
				if (p->getGlobalType() == CWPGT_MISSILE && p->getPosition() / 8 >= p->getRange())
				{
					p->remove();
				}
				else if (!_ufo->isCrashed())
				{
					projectileInFlight = true;
				}
			}
			// Projectiles fired by UFO.
			else if (p->getDirection() == D_DOWN)
			{
				if (p->getGlobalType() == CWPGT_MISSILE || (p->getGlobalType() == CWPGT_BEAM && p->toBeRemoved()))
				{
					int chancetoHit = p->getAccuracy(); // vanilla xcom
					chancetoHit -= _craft->getCraftStats().avoidBonus;
					chancetoHit += _ufo->getCraftStats().hitBonus;
					chancetoHit -= _pilotDodgeBonus;
					// evasive maneuvers
					if (_ufoIsAttacking && _mode == _btnCautious)
					{
						// HK's chance to hit is halved, but craft's reload time is doubled too
						chancetoHit = chancetoHit / 2;
					}
					if (RNG::percent(chancetoHit) || _selfDestructPressed)
					{
						// Formula delivered by Volutar, altered by Extended version.
						int power = p->getDamage() * (_ufo->getCraftStats().powerBonus + 100) / 100;
						int damage = RNG::generate(0, power);

						if (_craft->getShield() != 0)
						{
							int shieldBleedThroughDamage = std::max(0, damage - _craft->getShield()) * _craft->getCraftStats().shieldBleedThrough / 100;
							_craft->setShield(_craft->getShield() - damage);
							damage = shieldBleedThroughDamage;
							drawCraftShield();
							setStatus("STR_INTERCEPTOR_SHIELD_HIT");
						}

						damage = std::max(0, damage - _craft->getCraftStats().armor);

						// if a totally crappy HK is attacking a completely defenseless craft, avoid endless fight
						if (_selfDestructPressed)
						{
							damage = _craft->getCraftStats().damageMax;
						}

						if (damage)
						{
							_craft->setDamage(_craft->getDamage() + damage);
							drawCraftDamage();
							setStatus("STR_INTERCEPTOR_DAMAGED");
							_game->getMod()->getSound("GEO.CAT", Mod::INTERCEPTOR_HIT)->play(); //10
							if (_mode == _btnCautious && _craft->getDamagePercentage() >= 50 && !_ufoIsAttacking)
							{
								_targetDist = STANDOFF_DIST;
							}
						}
					}
					p->remove();
				}
			}
		}

		// Remove projectiles that hit or missed their target.
		Collections::deleteIf(_projectiles, _projectiles.size(),
			[&](CraftWeaponProjectile* cwp)
			{
				return cwp->toBeRemoved() == true || (cwp->getMissed() == true && cwp->getPosition() <= 0);
			}
		);

		// Check if the situation is hopeless for the craft
		if (_disableDisengage && !_craftIsDefenseless)
		{
			if (_projectiles.empty())
			{
				bool hasNoAmmo = true;
				for (auto cw : *_craft->getWeapons())
				{
					if (cw && cw->getAmmo() > 0)
					{
						hasNoAmmo = false;
						break;
					}
				}
				// no projectiles in the air and no ammo left
				if (hasNoAmmo)
				{
					_craftIsDefenseless = true;

					// self-destruct button
					int offset = _game->getMod()->getInterface("dogfight")->getElement("minimizeButtonDummy")->TFTDMode ? 1 : 0;
					_btnMinimize->drawRect(1 + offset, 1, _btnMinimize->getWidth() - 2 - offset, _btnMinimize->getHeight() - 2, _colors[DAMAGE_MAX]);
					_btnMinimize->setVisible(true);
				}
			}
		}

		// Handle weapons and craft distance.
		for (int i = 0; i < _weaponNum; ++i)
		{
			CraftWeapon *w = _craft->getWeapons()->at(i);
			if (w == 0)
			{
				continue;
			}
			int wTimer = _weaponFireCountdown[i];

			// Handle weapon firing
			if (wTimer == 0 && _currentDist <= w->getRules()->getRange() * 8 && w->getAmmo() > 0 && _mode != _btnStandoff
				&& _mode != _btnDisengage && !_ufo->isCrashed() && !_craft->isDestroyed())
			{
				if (_weaponEnabled[i])
				{
					fireWeapon(i);
					projectileInFlight = true;
				}
			}
			else if (wTimer > 0)
			{
				--_weaponFireCountdown[i];
			}

			// Handle craft tractor beams
			if (w->getRules()->getTractorBeamPower() != 0)
			{
				if (_currentDist <= w->getRules()->getRange() * 8 && _mode != _btnStandoff
					&& _mode != _btnDisengage && !_ufo->isCrashed() && !_craft->isDestroyed()
					&& _weaponEnabled[i])
				{
					if (!_tractorLockedOn[i])
					{
						_tractorLockedOn[i] = true;
						int tractorBeamSlowdown = _ufo->getTractorBeamSlowdown();
						tractorBeamSlowdown += w->getRules()->getTractorBeamPower() * _game->getMod()->getUfoTractorBeamSizeModifier(_ufoSize) / 100;
						_ufo->setTractorBeamSlowdown(tractorBeamSlowdown);
						setStatus("STR_TRACTOR_BEAM_ENGAGED");
					}
				}
				else
				{
					if (_tractorLockedOn[i])
					{
						_tractorLockedOn[i] = false;
						int tractorBeamSlowdown = _ufo->getTractorBeamSlowdown();
						tractorBeamSlowdown -= w->getRules()->getTractorBeamPower() * _game->getMod()->getUfoTractorBeamSizeModifier(_ufoSize) / 100;
						_ufo->setTractorBeamSlowdown(tractorBeamSlowdown);
						setStatus("STR_TRACTOR_BEAM_DISENGAGED");
					}
				}
			}

			if (w->getAmmo() == 0 && !projectileInFlight && !_craft->isDestroyed())
			{
				// Handle craft distance according to option set by user and available ammo.
				if (_mode == _btnCautious && !_ufoIsAttacking)
				{
					minimumDistance();
				}
				else if (_mode == _btnStandard)
				{
					maximumDistance();
				}
			}
		}

		// Handle UFO firing.
		if (_currentDist <= _ufo->getRules()->getWeaponRange() * 8 && !_ufo->isCrashed() && !_craft->isDestroyed())
		{
			if (_ufo->getShootingAt() == 0)
			{
				_ufo->setShootingAt(_interceptionNumber);
			}
			if (_ufo->getShootingAt() == _interceptionNumber)
			{
				if (_ufo->getFireCountdown() == 0)
				{
					ufoFireWeapon();
				}
			}
		}
		else if (_ufo->getShootingAt() == _interceptionNumber)
		{
			_ufo->setShootingAt(0);
		}
	}

	// Check when battle is over.
	if (_end == true && (((_currentDist > 640 || _minimized) && (_mode == _btnDisengage || _ufoBreakingOff == true)) || (_timeout == 0 && (_ufo->isCrashed() || _craft->isDestroyed()))))
	{
		if (_ufoBreakingOff)
		{
			_ufo->move();
			// TODO: rethink: give hunter-killers opportunity to escape?
			if (!_ufoIsAttacking)
			{
				_craft->setDestination(_ufo);
			}
		}
		if (!_destroyCraft && (_destroyUfo || _mode == _btnDisengage))
		{
			// keep original target if attacked by a HK (and didn't disengage manually)
			bool keepOriginalTarget = _ufoIsAttacking && _craft->getDestination() != _ufo;
			if (!keepOriginalTarget || _mode == _btnDisengage)
			{
				_craft->returnToBase();
			}

			// Need to give the craft at least one step advantage over the hunter-killer (to be able to escape)
			if (_ufoIsAttacking)
			{
				bool returnedToBase = _craft->think();
				if (returnedToBase)
				{
					_game->getSavedGame()->stopHuntingXcomCraft(_craft); // hiding in the base is good enough, obviously
				}
			}
		}
		if (_ufo->isCrashed())
		{
			std::vector<Craft*> followers = _ufo->getCraftFollowers();
			for (std::vector<Craft*>::iterator i = followers.begin(); i != followers.end(); ++i)
			{
				if ((*i)->getNumTotalUnits() == 0 || !(*i)->getRules()->getAllowLanding())
				{
					(*i)->returnToBase();
				}
			}
		}
		endDogfight();
	}

	if (_currentDist > 640 && _ufoBreakingOff)
	{
		finalRun = true;
	}

	if (!_end)
	{
		if (_endCraftHandled)
		{
			finalRun = true;
		}
		else if (_craft->isDestroyed())
		{
			// End dogfight if craft is destroyed.
			setStatus("STR_INTERCEPTOR_DESTROYED");
			if (_ufoIsAttacking)
			{
				_craft->evacuateCrew(_game->getMod());
			}
			_timeout += 30;
			_game->getMod()->getSound("GEO.CAT", Mod::INTERCEPTOR_EXPLODE)->play();
			finalRun = true;
			_destroyCraft = true;
			_endCraftHandled = true;
			_ufo->setShootingAt(0);
		}

		if (_endUfoHandled)
		{
			finalRun = true;
		}
		else if (_ufo->isCrashed())
		{
			// End dogfight if UFO is crashed or destroyed.
			_endUfoHandled = true;

			if (_ufo->getShotDownByCraftId() == _craft->getUniqueId())
			{
				AlienRace *race = _game->getMod()->getAlienRace(_ufo->getAlienRace());
				AlienMission *mission = _ufo->getMission();
				mission->ufoShotDown(*_ufo);
				// Check for retaliation trigger.
				int retaliationOdds = mission->getRules().getRetaliationOdds();
				if (retaliationOdds == -1)
				{
					retaliationOdds = 100 - (4 * (24 - _game->getSavedGame()->getDifficultyCoefficient()) - race->getRetaliationAggression());
					{
						int diff = _game->getSavedGame()->getDifficulty();
						auto& custom = _game->getMod()->getRetaliationTriggerOdds();
						if (custom.size() > (size_t)diff)
						{
							retaliationOdds = custom[diff] + race->getRetaliationAggression();
						}
					}
				}
				// Have mercy on beginners
				if (_game->getSavedGame()->getMonthsPassed() < Mod::DIFFICULTY_BASED_RETAL_DELAY[_game->getSavedGame()->getDifficulty()])
				{
					retaliationOdds = 0;
				}

				if (RNG::percent(retaliationOdds))
				{
					// Spawn retaliation mission.
					std::string targetRegion;
					int retaliationUfoMissionRegionOdds = 50 - 6 * _game->getSavedGame()->getDifficultyCoefficient();
					{
						int diff = _game->getSavedGame()->getDifficulty();
						auto& custom = _game->getMod()->getRetaliationBaseRegionOdds();
						if (custom.size() > (size_t)diff)
						{
							retaliationUfoMissionRegionOdds = 100 - custom[diff];
						}
					}
					if (RNG::percent(retaliationUfoMissionRegionOdds))
					{
						// Attack on UFO's mission region
						targetRegion = _ufo->getMission()->getRegion();
					}
					else
					{
						// Try to find and attack the originating base.
						targetRegion = _game->getSavedGame()->locateRegion(*_craft->getBase())->getRules()->getType();
						// TODO: If the base is removed, the mission is canceled.
					}
					// Difference from original: No retaliation until final UFO lands (Original: Is spawned).
					if (!_game->getSavedGame()->findAlienMission(targetRegion, OBJECTIVE_RETALIATION, race))
					{
						auto* retalWeights = race->retaliationMissionWeights(_game->getSavedGame()->getMonthsPassed());
						std::string retalMission = retalWeights ? retalWeights->choose() : "";
						const RuleAlienMission *rule = _game->getMod()->getAlienMission(retalMission, false);
						if (!rule)
						{
							rule = _game->getMod()->getRandomMission(OBJECTIVE_RETALIATION, _game->getSavedGame()->getMonthsPassed());
						}

						if (rule)
						{
							AlienMission *newMission = new AlienMission(*rule);
							newMission->setId(_game->getSavedGame()->getId("ALIEN_MISSIONS"));
							newMission->setRegion(targetRegion, *_game->getMod());
							newMission->setRace(_ufo->getAlienRace());
							newMission->start(*_game, *_state->getGlobe(), newMission->getRules().getWave(0).spawnTimer); // fixed delay for first scout
							_game->getSavedGame()->getAlienMissions().push_back(newMission);
						}
					}
				}
			}

			if (_ufo->isDestroyed())
			{
				if (_ufo->getShotDownByCraftId() == _craft->getUniqueId())
				{
					for (std::vector<Country*>::iterator country = _game->getSavedGame()->getCountries()->begin(); country != _game->getSavedGame()->getCountries()->end(); ++country)
					{
						if ((*country)->getRules()->insideCountry(_ufo->getLongitude(), _ufo->getLatitude()))
						{
							(*country)->addActivityXcom(_ufo->getRules()->getScore()*2);
							break;
						}
					}
					for (std::vector<Region*>::iterator region = _game->getSavedGame()->getRegions()->begin(); region != _game->getSavedGame()->getRegions()->end(); ++region)
					{
						if ((*region)->getRules()->insideRegion(_ufo->getLongitude(), _ufo->getLatitude()))
						{
							(*region)->addActivityXcom(_ufo->getRules()->getScore()*2);
							break;
						}
					}
					setStatus("STR_UFO_DESTROYED");
					_game->getMod()->getSound("GEO.CAT", Mod::UFO_EXPLODE)->play(); //11
				}
				_destroyUfo = true;
			}
			else
			{
				if (_ufo->getShotDownByCraftId() == _craft->getUniqueId())
				{
					setStatus("STR_UFO_CRASH_LANDS");
					_game->getMod()->getSound("GEO.CAT", Mod::UFO_CRASH)->play(); //10
					for (std::vector<Country*>::iterator country = _game->getSavedGame()->getCountries()->begin(); country != _game->getSavedGame()->getCountries()->end(); ++country)
					{
						if ((*country)->getRules()->insideCountry(_ufo->getLongitude(), _ufo->getLatitude()))
						{
							(*country)->addActivityXcom(_ufo->getRules()->getScore());
							break;
						}
					}
					for (std::vector<Region*>::iterator region = _game->getSavedGame()->getRegions()->begin(); region != _game->getSavedGame()->getRegions()->end(); ++region)
					{
						if ((*region)->getRules()->insideRegion(_ufo->getLongitude(), _ufo->getLatitude()))
						{
							(*region)->addActivityXcom(_ufo->getRules()->getScore());
							break;
						}
					}
				}
				bool survived = true;
				bool fakeUnderwaterTexture = _state->getGlobe()->insideFakeUnderwaterTexture(_ufo->getLongitude(), _ufo->getLatitude());
				if (!_state->getGlobe()->insideLand(_ufo->getLongitude(), _ufo->getLatitude()))
				{
					survived = false; // destroyed on real water
				}
				else if (fakeUnderwaterTexture)
				{
					if (RNG::percent(_ufo->getRules()->getSplashdownSurvivalChance()))
					{
						setStatus("STR_UFO_SURVIVED_SPLASHDOWN");
					}
					else
					{
						survived = false; // destroyed on fake water
						setStatus("STR_UFO_DESTROYED_BY_SPLASHDOWN");
					}
				}
				if (!survived)
				{
					_ufo->setStatus(Ufo::DESTROYED);
					_destroyUfo = true;
				}
				else
				{
					_ufo->setSecondsRemaining(RNG::generate(24, 96)*3600);
					_ufo->setAltitude("STR_GROUND");
					if (_ufo->getCrashId() == 0)
					{
						_ufo->setCrashId(_game->getSavedGame()->getId("STR_CRASH_SITE"));
						if (_ufo->isHunterKiller())
						{
							// stop being a hunter-killer
							_ufo->resetOriginalDestination(_craft);
							_ufo->setHunterKiller(false);
						}
					}
				}
			}
			_timeout += 30;
			if (_ufo->getShotDownByCraftId() != _craft->getUniqueId())
			{
				_timeout += 50;
				_ufo->setHitFrame(3);
			}
			finalRun = true;

			if (_ufo->getStatus() == Ufo::LANDED)
			{
				_timeout += 30;
				finalRun = true;
				_ufo->setShootingAt(0);
			}
		}
		else if (_ufo->getCraftStats().speedMax - _ufo->getTractorBeamSlowdown() == 0) // UFO brought down by tractor beam
		{
			_endUfoHandled = true;

			bool survived = true;
			if (!_state->getGlobe()->insideLand(_ufo->getLongitude(), _ufo->getLatitude()))
			{
				survived = false; // destroyed on real water
			}
			else
			{
				bool fakeUnderwaterTexture = _state->getGlobe()->insideFakeUnderwaterTexture(_ufo->getLongitude(), _ufo->getLatitude());
				if (fakeUnderwaterTexture && !RNG::percent(_ufo->getRules()->getSplashdownSurvivalChance()))
				{
					survived = false; // destroyed on fake water
				}
			}
			if (_ufo->getRules()->isUnmanned())
			{
				survived = false; // unmanned UFOs (drones, missiles, etc.) can't be forced to land
			}
			if (!survived) // Brought it down over water (and didn't survive splashdown)
			{
				finalRun = true;
				_ufo->setDamage(_ufo->getCraftStats().damageMax, _game->getMod());
				_state->handleDogfightExperience(); // called after setDamage
				_ufo->setShotDownByCraftId(_craft->getUniqueId());
				_ufo->setSpeed(0);
				_ufo->setStatus(Ufo::DESTROYED);
				_destroyUfo = true;
				for (std::vector<Country*>::iterator country = _game->getSavedGame()->getCountries()->begin(); country != _game->getSavedGame()->getCountries()->end(); ++country)
				{
					if ((*country)->getRules()->insideCountry(_ufo->getLongitude(), _ufo->getLatitude()))
					{
						(*country)->addActivityXcom(_ufo->getRules()->getScore());
						break;
					}
				}
				for (std::vector<Region*>::iterator region = _game->getSavedGame()->getRegions()->begin(); region != _game->getSavedGame()->getRegions()->end(); ++region)
				{
					if ((*region)->getRules()->insideRegion(_ufo->getLongitude(), _ufo->getLatitude()))
					{
						(*region)->addActivityXcom(_ufo->getRules()->getScore());
						break;
					}
				}
			}
			else // Brought it down over land (or survived splashdown)
			{
				finalRun = true;
				_ufo->setSecondsRemaining(RNG::generate(30, 120)*60);
				_ufo->setShootingAt(0);
				_ufo->setStatus(Ufo::LANDED);
				_ufo->setAltitude("STR_GROUND");
				_ufo->setSpeed(0);
				_ufo->setTractorBeamSlowdown(0);
				if (_ufo->getLandId() == 0)
				{
					_ufo->setLandId(_game->getSavedGame()->getId("STR_LANDING_SITE"));
				}
			}
		}
	}

	if (!projectileInFlight && finalRun)
	{
		_end = true;
	}
}

/**
 * Fires a shot from the first weapon
 * equipped on the craft.
 */
void DogfightState::fireWeapon(int i)
{
	CraftWeapon *w1 = _craft->getWeapons()->at(i);
	if (w1->setAmmo(w1->getAmmo() - 1))
	{
		_weaponFireCountdown[i] = _weaponFireInterval[i];

		std::ostringstream ss;
		ss << w1->getAmmo();
		_txtAmmo[i]->setText(ss.str());

		CraftWeaponProjectile *p = w1->fire();
		p->setDirection(D_UP);
		p->setHorizontalPosition((i % 2 ? HP_RIGHT : HP_LEFT) * (1 + 2 * (i / 2)));
		_projectiles.push_back(p);

		_game->getMod()->getSound("GEO.CAT", w1->getRules()->getSound())->play();
		_firedAtLeastOnce = true;
	}
}

/**
 *	Each time a UFO will try to fire it's cannons
 *	a calculation is made. There's only 10% chance
 *	that it will actually fire.
 */
void DogfightState::ufoFireWeapon()
{
	int fireCountdown = std::max(1, (_ufo->getRules()->getWeaponReload() - 2 * _game->getSavedGame()->getDifficultyCoefficient()));
	{
		int diff = _game->getSavedGame()->getDifficulty();
		auto& custom = _game->getMod()->getUfoFiringRateCoefficients();
		if (custom.size() > (size_t)diff)
		{
			fireCountdown = std::max(1, _ufo->getRules()->getWeaponReload() * custom[diff] / 100);
		}
	}
	_ufo->setFireCountdown(RNG::generate(0, fireCountdown) + fireCountdown);

	setStatus("STR_UFO_RETURN_FIRE");
	CraftWeaponProjectile *p = new CraftWeaponProjectile();
	p->setType(CWPT_PLASMA_BEAM);
	p->setAccuracy(60);
	p->setDamage(_ufo->getRules()->getWeaponPower());
	p->setDirection(D_DOWN);
	p->setHorizontalPosition(HP_CENTER);
	p->setPosition(_currentDist - (_ufo->getRules()->getRadius() / 2));
	_projectiles.push_back(p);
	if (_ufoIsAttacking && _disableDisengage)
	{
		_ufo->increaseSoftlockShotCounter();
	}

	if (_ufo->getRules()->getFireSound() == -1)
	{
		_game->getMod()->getSound("GEO.CAT", Mod::UFO_FIRE)->play();
	}
	else
	{
		_game->getMod()->getSound("GEO.CAT", _ufo->getRules()->getFireSound())->play();
	}
}

/**
 * Sets the craft to the minimum distance
 * required to fire a weapon.
 */
void DogfightState::minimumDistance()
{
	int max = 0;
	for (std::vector<CraftWeapon*>::iterator i = _craft->getWeapons()->begin(); i < _craft->getWeapons()->end(); ++i)
	{
		if (*i == 0)
			continue;
		if ((*i)->getRules()->getRange() > max && (*i)->getAmmo() > 0)
		{
			max = (*i)->getRules()->getRange();
		}
	}
	if (max == 0)
	{
		_targetDist = STANDOFF_DIST;
	}
	else
	{
		_targetDist = max * 8;
	}
}

/**
 * Sets the craft to the maximum distance
 * required to fire a weapon.
 */
void DogfightState::maximumDistance()
{
	int min = 1000;
	for (std::vector<CraftWeapon*>::iterator i = _craft->getWeapons()->begin(); i < _craft->getWeapons()->end(); ++i)
	{
		if (*i == 0)
			continue;
		if ((*i)->getRules()->getRange() < min && (*i)->getAmmo() > 0)
		{
			min = (*i)->getRules()->getRange();
		}
	}
	if (_ufoIsAttacking)
	{
		// If the UFO is actively hunting us, consider its weapon range too
		if (_ufo->getRules()->getWeaponRange() > 0 && _ufo->getRules()->getWeaponRange() < min)
		{
			min = _ufo->getRules()->getWeaponRange();
		}
	}
	if (min == 1000)
	{
		_targetDist = STANDOFF_DIST;
	}
	else
	{
		_targetDist = min * 8;
	}
}

/**
 * Sets the craft to the distance relevant for aggressive attack.
 */
void DogfightState::aggressiveDistance()
{
	maximumDistance();
	if (_targetDist > AGGRESSIVE_DIST)
	{
		_targetDist = AGGRESSIVE_DIST;
	}
}

/**
 * Updates the status text and restarts
 * the text timeout counter.
 * @param status New status text.
 */
void DogfightState::setStatus(const std::string &status)
{
	_txtStatus->setText(tr(status));
	_timeout = 50;
}

/**
 * Minimizes the dogfight window.
 * @param action Pointer to an action.
 */
void DogfightState::btnMinimizeClick(Action *)
{
	if (_craftIsDefenseless)
	{
		_selfDestructPressed = !_selfDestructPressed;
		if (_selfDestructPressed)
			setStatus("STR_SELF_DESTRUCT_ACTIVATED");
		else
			setStatus("STR_SELF_DESTRUCT_CANCELLED");
		int offset = _game->getMod()->getInterface("dogfight")->getElement("minimizeButtonDummy")->TFTDMode ? 1 : 0;
		int color = _selfDestructPressed ? DAMAGE_MIN : DAMAGE_MAX;
		_btnMinimize->drawRect(1 + offset, 1, _btnMinimize->getWidth() - 2 - offset, _btnMinimize->getHeight() - 2, _colors[color]);
		return;
	}

	if (!_ufo->isCrashed() && !_craft->isDestroyed() && !_ufoBreakingOff)
	{
		if (_currentDist >= STANDOFF_DIST)
		{
			setMinimized(true);
			_ufo->setShieldRechargeHandle(0);
		}
		else
		{
			setStatus("STR_MINIMISE_AT_STANDOFF_RANGE_ONLY");
		}
	}
}

/**
 * Switches to Standoff mode (maximum range).
 * @param action Pointer to an action.
 */
void DogfightState::btnStandoffPress(Action *)
{
	if (!_ufo->isCrashed() && !_craft->isDestroyed() && !_ufoBreakingOff)
	{
		_end = false;
		setStatus("STR_STANDOFF");
		_targetDist = STANDOFF_DIST;
	}
}

void DogfightState::btnStandoffRightPress(Action *)
{
	_state->handleDogfightMultiAction(0);
}

void DogfightState::btnStandoffSimulateLeftPress(Action *action)
{
	if (_btnStandoff->getVisible())
	{
		_btnStandoff->mousePress(action, this);
	}
}

/**
 * Switches to Cautious mode (maximum weapon range).
 * @param action Pointer to an action.
 */
void DogfightState::btnCautiousPress(Action *)
{
	if (!_ufo->isCrashed() && !_craft->isDestroyed() && !_ufoBreakingOff)
	{
		_end = false;
		if (!_ufoIsAttacking)
		{
			setStatus("STR_CAUTIOUS_ATTACK");
			for (int i = 0; i < _weaponNum; ++i)
			{
				CraftWeapon* w = _craft->getWeapons()->at(i);
				if (w != 0)
				{
					_weaponFireInterval[i] = w->getRules()->getCautiousReload();
				}
			}
			minimumDistance();
		}
		else
		{
			setStatus("STR_EVASIVE_MANEUVERS");
			for (int i = 0; i < _weaponNum; ++i)
			{
				CraftWeapon* w = _craft->getWeapons()->at(i);
				if (w != 0)
				{
					// double the craft's reload time to balance halving the HK's chance to hit
					_weaponFireInterval[i] = w->getRules()->getAggressiveReload() * 2;
				}
			}
			// same distance as aggressive (by design)
			aggressiveDistance();
		}
	}
}

void DogfightState::btnCautiousRightPress(Action *)
{
	_state->handleDogfightMultiAction(1);
}

void DogfightState::btnCautiousSimulateLeftPress(Action *action)
{
	if (_btnCautious->getVisible())
	{
		_btnCautious->mousePress(action, this);
	}
}

/**
 * Switches to Standard mode (minimum weapon range).
 * @param action Pointer to an action.
 */
void DogfightState::btnStandardPress(Action *)
{
	if (!_ufo->isCrashed() && !_craft->isDestroyed() && !_ufoBreakingOff)
	{
		_end = false;
		setStatus("STR_STANDARD_ATTACK");
		for (int i = 0; i < _weaponNum; ++i)
		{
			CraftWeapon* w = _craft->getWeapons()->at(i);
			if (w != 0)
			{
				_weaponFireInterval[i] = w->getRules()->getStandardReload();
			}
		}
		maximumDistance();
	}
}

void DogfightState::btnStandardRightPress(Action *)
{
	_state->handleDogfightMultiAction(2);
}

void DogfightState::btnStandardSimulateLeftPress(Action *action)
{
	if (_btnStandard->getVisible())
	{
		_btnStandard->mousePress(action, this);
	}
}

/**
 * Switches to Aggressive mode (minimum range).
 * @param action Pointer to an action.
 */
void DogfightState::btnAggressivePress(Action *)
{
	if (!_ufo->isCrashed() && !_craft->isDestroyed() && !_ufoBreakingOff)
	{
		_end = false;
		setStatus("STR_AGGRESSIVE_ATTACK");
		for (int i = 0; i < _weaponNum; ++i)
		{
			CraftWeapon* w = _craft->getWeapons()->at(i);
			if (w != 0)
			{
				_weaponFireInterval[i] = w->getRules()->getAggressiveReload();
			}
		}
		aggressiveDistance();
	}
}

void DogfightState::btnAggressiveRightPress(Action *)
{
	_state->handleDogfightMultiAction(3);
}

void DogfightState::btnAggressiveSimulateLeftPress(Action *action)
{
	if (_btnAggressive->getVisible())
	{
		_btnAggressive->mousePress(action, this);
	}
}

/**
 * Disengages from the UFO.
 * @param action Pointer to an action.
 */
void DogfightState::btnDisengagePress(Action *)
{
	if (!_ufo->isCrashed() && !_craft->isDestroyed() && !_ufoBreakingOff)
	{
		_end = true;
		setStatus("STR_DISENGAGING");
		_targetDist = 800;
	}
}

void DogfightState::btnDisengageRightPress(Action *)
{
	_state->handleDogfightMultiAction(4);
}

void DogfightState::btnDisengageSimulateLeftPress(Action *action)
{
	if (_btnDisengage->getVisible())
	{
		_btnDisengage->mousePress(action, this);
	}
}

/**
 * Shows a front view of the UFO.
 * @param action Pointer to an action.
 */
void DogfightState::btnUfoClick(Action *)
{
	_preview->setVisible(true);
	// Disable all other buttons to prevent misclicks
	_btnStandoff->setVisible(false);
	_btnCautious->setVisible(false);
	_btnStandard->setVisible(false);
	_btnAggressive->setVisible(false);
	_btnDisengage->setVisible(false);
	_btnUfo->setVisible(false);
	_btnMinimize->setVisible(false);
	for (int i = 0; i < _weaponNum; ++i)
	{
		_weapon[i]->setVisible(false);
	}
}

/**
 * Hides the front view of the UFO.
 * @param action Pointer to an action.
 */
void DogfightState::previewClick(Action *)
{
	_preview->setVisible(false);
	// Reenable all other buttons to prevent misclicks
	_btnStandoff->setVisible(!_ufoIsAttacking);
	_btnCautious->setVisible(!_disableCautious);
	_btnStandard->setVisible(!_ufoIsAttacking);
	_btnAggressive->setVisible(true);
	_btnDisengage->setVisible(!_disableDisengage);
	_btnUfo->setVisible(true);
	_btnMinimize->setVisible(!_ufoIsAttacking || _craftIsDefenseless);
	for (int i = 0; i < _weaponNum; ++i)
	{
		_weapon[i]->setVisible(true);
	}
}

/*
 * Draws the UFO blob on the radar screen.
 * Currently works only for original sized blobs
 * 13 x 13 pixels.
 */
void DogfightState::drawUfo()
{
	if (_ufoSize < 0 || _ufo->isDestroyed())
	{
		return;
	}
	int currentUfoXposition =  _battle->getWidth() / 2 - 6;
	int currentUfoYposition = _battle->getHeight() - (_currentDist / 8) - 6;
	for (int y = 0; y < 13; ++y)
	{
		for (int x = 0; x < 13; ++x)
		{
			Uint8 pixelOffset = _ufoBlobs[_ufoSize + _ufo->getHitFrame()][y][x];
			if (pixelOffset == 0)
			{
				continue;
			}
			else
			{
				if (_ufo->isCrashed() || _ufo->getHitFrame() > 0)
				{
					pixelOffset *= 2;
				}

				Uint8 radarPixelColor = _window->getPixel(currentUfoXposition + x + 3, currentUfoYposition + y + 3); // + 3 cause of the window frame
				Uint8 color = radarPixelColor - pixelOffset;
				if (color < _colors[BLOB_MIN])
				{
					color = _colors[BLOB_MIN];
				}

				if (_ufo->getShield() != 0 && _ufo->getCraftStats().shieldCapacity != 0)
				{
					Uint8 shieldColor =
						_colors[SHIELD_MAX]
						- ((_colors[SHIELD_MAX] - _colors[SHIELD_MIN]) * _ufo->getShield() / _ufo->getCraftStats().shieldCapacity)
						+ (color - _colors[BLOB_MIN]);
					if (shieldColor < _colors[SHIELD_MIN])
					{
						shieldColor = _colors[SHIELD_MIN];
					}
					if (shieldColor <=  _colors[SHIELD_MAX])
					{
						color = shieldColor;
					}
				}

				_battle->setPixel(currentUfoXposition + x, currentUfoYposition + y, color);
			}
		}
	}
}

/*
 * Draws projectiles on the radar screen.
 * Depending on what type of projectile it is, it's
 * shape will be different. Currently works for
 * original sized blobs 3 x 6 pixels.
 */
void DogfightState::drawProjectile(const CraftWeaponProjectile* p)
{
	int xPos = _battle->getWidth() / 2 + p->getHorizontalPosition();
	// Draw missiles.
	if (p->getGlobalType() == CWPGT_MISSILE)
	{
		xPos -= 1;
		int yPos = _battle->getHeight() - p->getPosition() / 8;
		for (int x = 0; x < 3; ++x)
		{
			for (int y = 0; y < 6; ++y)
			{
				int pixelOffset = _projectileBlobs[p->getType()][y][x];
				if (pixelOffset == 0)
				{
					continue;
				}
				else
				{
					Uint8 radarPixelColor = _window->getPixel(xPos + x + 3, yPos + y + 3); // + 3 cause of the window frame
					Uint8 color = radarPixelColor - pixelOffset;
					if (color < _colors[BLOB_MIN])
					{
						color = _colors[BLOB_MIN];
					}
					_battle->setPixel(xPos + x, yPos + y, color);
				}
			}
		}
	}
	// Draw beams.
	else if (p->getGlobalType() == CWPGT_BEAM)
	{
		int yStart = _battle->getHeight() - 2;
		int yEnd = _battle->getHeight() - (_currentDist / 8);
		Uint8 pixelOffset = p->getState();
		for (int y = yStart; y > yEnd; --y)
		{
			Uint8 radarPixelColor = _window->getPixel(xPos + 3, y + 3);

			int beamPower = 0;
			if (p->getType() == CWPT_PLASMA_BEAM)
			{
				beamPower = _ufo->getRules()->getWeaponPower() / _game->getMod()->getUfoBeamWidthParameter();
			}

			for (int x = 0; x <= std::min(beamPower, 3); x++)
			{
				Uint8 color = radarPixelColor - pixelOffset - beamPower + 2 * x;
				if (color < _colors[BLOB_MIN])
				{
					color = _colors[BLOB_MIN];
				}
				if (color > radarPixelColor)
				{
					color = radarPixelColor;
				}
				_battle->setPixel(xPos + x, y, color);
				_battle->setPixel(xPos - x, y, color);
			}
		}
	}
}

/**
 * Toggles usage of weapons.
 * @param action Pointer to an action.
 */
void DogfightState::weaponClick(Action * a)
{
	for(int i = 0; i < _weaponNum; ++i)
	{
		if (a->getSender() == _weapon[i])
		{
			_weaponEnabled[i] = !_weaponEnabled[i];
			recolor(i, _weaponEnabled[i]);

			if (Options::oxceRememberDisabledCraftWeapons)
			{
				CraftWeapon* w = _craft->getWeapons()->at(i);
				if (w)
				{
					w->setDisabled(!_weaponEnabled[i]);
				}
			}
			return;
		}
	}
}

/**
 * Changes colors of weapon icons, range indicators and ammo texts base on current weapon state.
 * @param weaponNo - number of weapon for which colors must be changed.
 * @param currentState - state of weapon (enabled = true, disabled = false).
 */
void DogfightState::recolor(const int weaponNo, const bool currentState)
{
	InteractiveSurface *weapon = _weapon[weaponNo];
	Text *ammo = _txtAmmo[weaponNo];
	Surface *range = _range[weaponNo];

	if (currentState)
	{
		weapon->offset(-_colors[DISABLED_WEAPON]);
		ammo->offset(-_colors[DISABLED_AMMO]);
		range->offset(-_colors[DISABLED_RANGE]);
	}
	else
	{
		weapon->offset(_colors[DISABLED_WEAPON]);
		ammo->offset(_colors[DISABLED_AMMO]);
		range->offset(_colors[DISABLED_RANGE]);
	}
}

/**
 * Returns true if state is minimized. Otherwise returns false.
 * @return Is the dogfight minimized?
 */
bool DogfightState::isMinimized() const
{
	return _minimized;
}

/**
 * Sets the state to minimized/maximized status.
 * @param minimized Is the dogfight minimized?
 */
void DogfightState::setMinimized(const bool minimized)
{
	// set these to the same as the incoming minimized state
	_minimized = minimized;
	_btnMinimizedIcon->setVisible(minimized);
	_txtInterceptionNumber->setVisible(minimized);

	// set these to the opposite of the incoming minimized state
	_window->setVisible(!minimized);
	_btnStandoff->setVisible(!minimized);
	_btnCautious->setVisible(!minimized);
	_btnStandard->setVisible(!minimized);
	_btnAggressive->setVisible(!minimized);
	_btnDisengage->setVisible(!minimized);
	_btnUfo->setVisible(!minimized);
	_btnMinimize->setVisible(!minimized);
	_battle->setVisible(!minimized);
	for (int i = 0; i < _weaponNum; ++i)
	{
		_weapon[i]->setVisible(!minimized);
		_range[i]->setVisible(!minimized);
		_txtAmmo[i]->setVisible(!minimized);
	}
	_craftSprite->setVisible(!minimized);
	_damage->setVisible(!minimized);
	_craftShield->setVisible(!minimized);
	_txtDistance->setVisible(!minimized);
	_txtStatus->setVisible(!minimized);

	// set to false regardless
	_preview->setVisible(false);
}

/**
 * Maximizes the interception window.
 * @param action Pointer to an action.
 */
void DogfightState::btnMinimizedIconClick(Action *)
{
	if (_craft->getRules()->isWaterOnly() && _ufo->getAltitudeInt() > _craft->getRules()->getMaxAltitude())
	{
		_state->popup(new DogfightErrorState(_craft, tr("STR_UNABLE_TO_ENGAGE_DEPTH")));
		setWaitForAltitude(true);
	}
	else if (_craft->getRules()->isWaterOnly() && !_state->getGlobe()->insideLand(_craft->getLongitude(), _craft->getLatitude()))
	{
		_state->popup(new DogfightErrorState(_craft, tr("STR_UNABLE_TO_ENGAGE_AIRBORNE")));
		setWaitForPoly(true);
	}
	else
	{
		setMinimized(false);
	}
}

/**
 * Sets interception number. Used to draw proper number when window minimized.
 * @param number ID number.
 */
void DogfightState::setInterceptionNumber(const int number)
{
	_interceptionNumber = number;
}

/**
 * Sets interceptions count. Used to properly position the window.
 * @param count Amount of interception windows.
 */
void DogfightState::setInterceptionsCount(const size_t count)
{
	_interceptionsCount = count;
	calculateWindowPosition();
	moveWindow();
}

/**
 * Calculates dogfight window position according to
 * number of active interceptions.
 */
void DogfightState::calculateWindowPosition()
{
	_minimizedIconX = 5;
	_minimizedIconY = (5 * _interceptionNumber) + (16 * (_interceptionNumber - 1));

	if (_interceptionsCount == 1)
	{
		_x = 80;
		_y = 52;
	}
	else if (_interceptionsCount == 2)
	{
		if (_interceptionNumber == 1)
		{
			_x = 80;
			_y = 0;
		}
		else // 2
		{
			_x = 80;
			//_y = (_game->getScreen()->getHeight() / 2) - 96;
			_y = 200 - _window->getHeight();//96;
		}
	}
	else if (_interceptionsCount == 3)
	{
		if (_interceptionNumber == 1)
		{
			_x = 80;
			_y = 0;
		}
		else if (_interceptionNumber == 2)
		{
			_x = 0;
			//_y = (_game->getScreen()->getHeight() / 2) - 96;
			_y = 200 - _window->getHeight();//96;
		}
		else // 3
		{
			//_x = (_game->getScreen()->getWidth() / 2) - 160;
			//_y = (_game->getScreen()->getHeight() / 2) - 96;
			_x = 320 - _window->getWidth();//160;
			_y = 200 - _window->getHeight();//96;
		}
	}
	else
	{
		if (_interceptionNumber == 1)
		{
			_x = 0;
			_y = 0;
		}
		else if (_interceptionNumber == 2)
		{
			//_x = (_game->getScreen()->getWidth() / 2) - 160;
			_x = 320 - _window->getWidth();//160;
			_y = 0;
		}
		else if (_interceptionNumber == 3)
		{
			_x = 0;
			//_y = (_game->getScreen()->getHeight() / 2) - 96;
			_y = 200 - _window->getHeight();//96;
		}
		else // 4
		{
			//_x = (_game->getScreen()->getWidth() / 2) - 160;
			//_y = (_game->getScreen()->getHeight() / 2) - 96;
			_x = 320 - _window->getWidth();//160;
			_y = 200 - _window->getHeight();//96;
		}
	}
	_x += _game->getScreen()->getDX();
	_y += _game->getScreen()->getDY();
}

/**
 * Relocates all dogfight window elements to
 * calculated position. This is used when multiple
 * interceptions are running.
 */
void DogfightState::moveWindow()
{
	int x = _window->getX() - _x;
	int y = _window->getY() - _y;
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		(*i)->setX((*i)->getX() - x);
		(*i)->setY((*i)->getY() - y);
	}
	_btnMinimizedIcon->setX(_minimizedIconX); _btnMinimizedIcon->setY(_minimizedIconY);
	_txtInterceptionNumber->setX(_minimizedIconX + 18); _txtInterceptionNumber->setY(_minimizedIconY + 6);
}

/**
 * Checks whether the dogfight should end.
 * @return Returns true if the dogfight should end, otherwise returns false.
 */
bool DogfightState::dogfightEnded() const
{
	return _endDogfight;
}

/**
 * Returns the UFO associated to this dogfight.
 * @return Returns pointer to UFO object associated to this dogfight.
 */
Ufo *DogfightState::getUfo() const
{
	return _ufo;
}

/**
 * Returns the craft associated to this dogfight.
 * @return Returns pointer to craft object associated to this dogfight.
 */
Craft *DogfightState::getCraft() const
{
	return _craft;
}

/**
 * Ends the dogfight.
 */
void DogfightState::endDogfight()
{
	if (_endDogfight)
		return;
	if (_craft)
	{
		_craft->setInDogfight(false);
		_craft->setInterceptionOrder(0);
	}
	// set the ufo as "free" for the next engagement (as applicable)
	if (_ufo)
	{
		_ufo->setInterceptionProcessed(false);
		_ufo->setShieldRechargeHandle(0);
	}
	_endDogfight = true;
}

/**
 * Returns interception number.
 * @return interception number
 */
int DogfightState::getInterceptionNumber() const
{
	return _interceptionNumber;
}

void DogfightState::setWaitForPoly(bool wait)
{
	_waitForPoly = wait;
}

bool DogfightState::getWaitForPoly() const
{
	return _waitForPoly;
}

void DogfightState::setWaitForAltitude(bool wait)
{
	_waitForAltitude = wait;
}

bool DogfightState::getWaitForAltitude() const
{
	return _waitForAltitude;
}

void DogfightState::awardExperienceToPilots()
{
	if (_firedAtLeastOnce && !_experienceAwarded && _craft && _ufo && (_ufo->isCrashed() || _ufo->isDestroyed()))
	{
		bool psiStrengthEval = (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()));
		const std::vector<Soldier*> pilots = _craft->getPilotList(false);
		for (std::vector<Soldier*>::const_iterator it = pilots.begin(); it != pilots.end(); ++it)
		{
			if ((*it)->getCurrentStats()->firing < (*it)->getRules()->getStatCaps().firing)
			{
				if (RNG::percent((*it)->getRules()->getDogfightExperience().firing))
				{
					(*it)->getCurrentStats()->firing++;
					(*it)->getDailyDogfightExperienceCache()->firing++;
				}
			}
			if ((*it)->getCurrentStats()->reactions < (*it)->getRules()->getStatCaps().reactions)
			{
				if (RNG::percent((*it)->getRules()->getDogfightExperience().reactions))
				{
					(*it)->getCurrentStats()->reactions++;
					(*it)->getDailyDogfightExperienceCache()->reactions++;
				}
			}
			if ((*it)->getCurrentStats()->bravery < (*it)->getRules()->getStatCaps().bravery)
			{
				if (RNG::percent((*it)->getRules()->getDogfightExperience().bravery))
				{
					(*it)->getCurrentStats()->bravery += 10; // increase by 10 to keep OCD at bay
					(*it)->getDailyDogfightExperienceCache()->bravery += 10;
				}
			}
			(*it)->calcStatString(_game->getMod()->getStatStrings(), psiStrengthEval);
		}
		_experienceAwarded = true;
	}
}

}
