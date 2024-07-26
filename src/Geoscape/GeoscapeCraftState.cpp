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
#include "GeoscapeCraftState.h"
#include <sstream>
#include "../fmath.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Waypoint.h"
#include "SelectDestinationState.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "Globe.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Geoscape Craft window.
 * @param game Pointer to the core game.
 * @param craft Pointer to the craft to display.
 * @param globe Pointer to the Geoscape globe.
 * @param waypoint Pointer to the last UFO position (if redirecting the craft).
 */
GeoscapeCraftState::GeoscapeCraftState(Craft *craft, Globe *globe, Waypoint *waypoint, bool useCustomSound) : _craft(craft), _globe(globe), _waypoint(waypoint)
{
	_screen = false;

	_weaponNum = _craft->getRules()->getWeapons();
	if (_weaponNum > RuleCraft::WeaponMax)
		_weaponNum = RuleCraft::WeaponMax;

	const int offset_upper = -8;
	const int offset_lower = 120;

	if (useCustomSound)
	{
		int soundId = craft->getRules()->getSelectSound();
		if (soundId != Mod::NO_SOUND)
		{
			_customSound = _game->getMod()->getSound("GEO.CAT", soundId);
		}
	}

	// Create objects
	_window = new Window(this, 240, 192, 4, 4, POPUP_BOTH);
	_txtTitle = new Text(210, 17, 32, offset_upper + 20);
	_txtStatus = new Text(210, 17, 32, offset_upper + 36);
	_txtBase = new Text(210, 9, 32, offset_upper + 52);
	_txtSpeed = new Text(210, 9, 32, offset_upper + 60);
	_txtMaxSpeed = new Text(210, 9, 32, offset_upper + 68);
	_txtAltitude = new Text(210, 9, 32, offset_upper + 76);
	_txtSoldier = new Text(80, 9, 164, offset_upper + 60);
	_txtHWP = new Text(80, 9, 164, offset_upper + 68);
	_txtFuel = new Text(130, 9, 32, offset_upper + 84);
	_txtDamage = new Text(80, 9, 164, offset_upper + 84);
	_txtShield = new Text(80, 9, 164, offset_upper + 76);

	for (int i = 0; i < _weaponNum; ++i)
	{
		_txtWeaponName[i] = new Text(130, 9, 32, offset_upper + 92 + 8*i);
		_txtWeaponAmmo[i] = new Text(80, 9, 164, offset_upper + 92 + 8*i);
	}
	_txtRedirect = new Text(230, 17, 13, offset_lower + 0);
	_txtETA = new Text(230, 9, 13, offset_lower + 4);
	_btnBase = new TextButton(192, 12, 32, offset_lower + 14);
	_btnTarget = new TextButton(192, 12, 32, offset_lower + 28);
	_btnPatrol = new TextButton(192, 12, 32, offset_lower + 42);
	_btnCancel = new TextButton(192, 12, 32, offset_lower + 56);

	// Set palette
	setInterface("geoCraft");

	add(_window, "window", "geoCraft");
	add(_btnBase, "button", "geoCraft");
	add(_btnTarget, "button", "geoCraft");
	add(_btnPatrol, "button", "geoCraft");
	add(_btnCancel, "button", "geoCraft");
	add(_txtTitle, "text1", "geoCraft");
	add(_txtStatus, "text1", "geoCraft");
	add(_txtBase, "text3", "geoCraft");
	add(_txtSpeed, "text3", "geoCraft");
	add(_txtMaxSpeed, "text3", "geoCraft");
	add(_txtAltitude, "text3", "geoCraft");
	add(_txtFuel, "text3", "geoCraft");
	add(_txtDamage, "text3", "geoCraft");
	add(_txtShield, "text3", "geoCraft");
	for (int i = 0; i < _weaponNum; ++i)
	{
		add(_txtWeaponName[i], "text3", "geoCraft");
		add(_txtWeaponAmmo[i], "text3", "geoCraft");
	}
	add(_txtRedirect, "text3", "geoCraft");
	add(_txtETA, "text3", "geoCraft");
	add(_txtSoldier, "text3", "geoCraft");
	add(_txtHWP, "text3", "geoCraft");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "geoCraft");

	_btnBase->setText(tr("STR_RETURN_TO_BASE"));
	_btnBase->onMouseClick((ActionHandler)&GeoscapeCraftState::btnBaseClick);

	_btnTarget->setText(tr("STR_SELECT_NEW_TARGET"));
	_btnTarget->onMouseClick((ActionHandler)&GeoscapeCraftState::btnTargetClick);

	_btnPatrol->setText(_craft->getRules()->canAutoPatrol() ? tr("STR_AUTO_PATROL") : tr("STR_PATROL"));
	_btnPatrol->onMouseClick((ActionHandler)&GeoscapeCraftState::btnPatrolClick);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&GeoscapeCraftState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&GeoscapeCraftState::btnCancelClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setText(_craft->getName(_game->getLanguage()));

	_txtStatus->setWordWrap(true);
	std::string status;
	if (_waypoint != 0)
	{
		status = tr("STR_INTERCEPTING_UFO").arg(_waypoint->getId());
	}
	else if (_craft->getLowFuel())
	{
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
	}
	else if (_craft->getMissionComplete())
	{
		status = tr("STR_MISSION_COMPLETE_RETURNING_TO_BASE");
	}
	else if (_craft->getDestination() == 0)
	{
		status = tr("STR_PATROLLING");
	}
	else if (_craft->getDestination() == (Target*)_craft->getBase())
	{
		status = tr("STR_RETURNING_TO_BASE");
	}
	else
	{
		Ufo *u = dynamic_cast<Ufo*>(_craft->getDestination());
		if (u != 0)
		{
			if (_craft->isInDogfight())
			{
				status = tr("STR_TAILING_UFO");
			}
			else if (u->getStatus() == Ufo::FLYING)
			{
				status = tr("STR_INTERCEPTING_UFO").arg(u->getId());
			}
			else
			{
				status = tr("STR_DESTINATION_UC_").arg(u->getName(_game->getLanguage()));
			}
		}
		else
		{
			status = tr("STR_DESTINATION_UC_").arg(_craft->getDestination()->getName(_game->getLanguage()));
		}
	}
	_txtStatus->setText(tr("STR_STATUS_").arg(status));

	_txtBase->setText(tr("STR_BASE_UC").arg(_craft->getBase()->getName()));

	{
		int speed = _craft->getSpeed();
		if (_craft->isInDogfight())
		{
			Ufo *ufo = dynamic_cast<Ufo*>(_craft->getDestination());
			if (ufo)
			{
				speed = ufo->getSpeed();
			}
		}
		_txtSpeed->setText(tr("STR_SPEED_").arg(Unicode::formatNumber(speed)));
	}

	_txtMaxSpeed->setText(tr("STR_MAXIMUM_SPEED_UC").arg(Unicode::formatNumber(_craft->getCraftStats().speedMax)));

	std::string altitude = _craft->getAltitude();
	if (_craft->getRules()->isWaterOnly() && !_globe->insideLand(_craft->getLongitude(), _craft->getLatitude()))
	{
		altitude = "STR_AIRBORNE";
	}
	_txtAltitude->setText(tr("STR_ALTITUDE_").arg(tr(altitude)));

	_txtFuel->setText(tr("STR_FUEL").arg(Unicode::formatPercentage(_craft->getFuelPercentage())));

	_txtDamage->setText(tr("STR_DAMAGE_UC_").arg(Unicode::formatPercentage(_craft->getDamagePercentage())));

	_txtShield->setText(tr("STR_SHIELD").arg(Unicode::formatPercentage(_craft->getShieldPercentage())));
	_txtShield->setVisible(_craft->getShieldCapacity() != 0);

	for (int i = 0; i < _weaponNum; ++i)
	{
		const std::string &wName = _craft->getRules()->getWeaponSlotString(i);
		if (wName.empty())
		{
			_txtWeaponName[i]->setVisible(false);
			_txtWeaponAmmo[i]->setVisible(false);
			continue;
		}

		CraftWeapon *w1 = _craft->getWeapons()->at(i);
		if (w1 != 0)
		{
			_txtWeaponName[i]->setText(tr(wName).arg(tr(w1->getRules()->getType())));
			if (w1->getRules()->getAmmoMax())
				_txtWeaponAmmo[i]->setText(tr("STR_ROUNDS_").arg(w1->getAmmo()));
			else
				_txtWeaponAmmo[i]->setVisible(false);
		}
		else
		{
			_txtWeaponName[i]->setText(tr(wName).arg(tr("STR_NONE_UC")));
			_txtWeaponAmmo[i]->setVisible(false);
		}
	}

	_txtRedirect->setBig();
	_txtRedirect->setAlign(ALIGN_CENTER);
	_txtRedirect->setText(tr("STR_REDIRECT_CRAFT"));

	std::ostringstream ss11;
	ss11 << tr("STR_SOLDIERS_UC") << ">" << Unicode::TOK_COLOR_FLIP << _craft->getNumTotalSoldiers();
	_txtSoldier->setText(ss11.str());

	std::ostringstream ss12;
	ss12 << tr("STR_HWPS") << ">" << Unicode::TOK_COLOR_FLIP << _craft->getNumTotalVehicles();
	_txtHWP->setText(ss12.str());

	if (_waypoint == 0)
	{
		_txtRedirect->setVisible(false);

		// ETA display
		if (Options::oxceShowETAMode > 0 && _craft->getDestination())
		{
			MovingTarget* mt = dynamic_cast<MovingTarget*>(_craft->getDestination());
			if (Options::oxceShowETAMode == 1 && mt && mt->getSpeed() > 0)
			{
				// don't show ETA for moving targets (i.e. UFOs and crafts)
			}
			else
			{
				int speed = _craft->getCraftStats().speedMax;
				int distance = XcomDistance(_craft->getDistance(_craft->getDestination()));
				int etaInHoursHelper = (distance + (speed / 2)) / speed;
				int days = etaInHoursHelper / 24;
				int hours = etaInHoursHelper % 24;
				std::ostringstream ssStatus;
				if (days > 0) ssStatus << tr("STR_DAY_SHORT").arg(days);
				if (hours > 0 || days == 0)
				{
					if (days > 0) ssStatus << "/";
					ssStatus << tr("STR_HOUR_SHORT").arg(hours);
				}
				_txtETA->setAlign(ALIGN_CENTER);
				_txtETA->setText(tr("STR_ETA").arg(ssStatus.str()));
			}
		}
	}
	else
	{
		_btnCancel->setText(tr("STR_GO_TO_LAST_KNOWN_UFO_POSITION"));
	}

	if (_craft->getLowFuel() || _craft->getMissionComplete())
	{
		_btnBase->setVisible(false);
		_btnTarget->setVisible(false);
		_btnPatrol->setVisible(false);
	}

	if (_craft->getRules()->getMaxUnitsLimit() == 0)
		_txtSoldier->setVisible(false);
	if (_craft->getRules()->getMaxVehiclesAndLargeSoldiersLimit() == 0)
		_txtHWP->setVisible(false);
}

/**
 *
 */
GeoscapeCraftState::~GeoscapeCraftState()
{

}

/**
 * Returns the craft back to its base.
 * @param action Pointer to an action.
 */
void GeoscapeCraftState::btnBaseClick(Action *)
{
	_game->popState();
	_craft->returnToBase();
	delete _waypoint;
	if (_craft->getRules()->canAutoPatrol())
	{
		// cancel auto-patrol
		_craft->setIsAutoPatrolling(false);
	}
}

/**
 * Changes the craft's target.
 * @param action Pointer to an action.
 */
void GeoscapeCraftState::btnTargetClick(Action *)
{
	_game->popState();
	_game->pushState(new SelectDestinationState(std::vector{ _craft }, _globe));
	delete _waypoint;
}

/**
 * Sets the craft to patrol the current location.
 * @param action Pointer to an action.
 */
void GeoscapeCraftState::btnPatrolClick(Action *)
{
	_game->popState();
	_craft->setDestination(0);
	delete _waypoint;
	if (_craft->getRules()->canAutoPatrol())
	{
		// start auto-patrol
		_craft->setLatitudeAuto(_craft->getLatitude());
		_craft->setLongitudeAuto(_craft->getLongitude());
		_craft->setIsAutoPatrolling(true);
	}
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void GeoscapeCraftState::btnCancelClick(Action *)
{
	// Go to the last known UFO position
	if (_waypoint != 0)
	{
		_waypoint->setId(_game->getSavedGame()->getId("STR_WAY_POINT"));
		_game->getSavedGame()->getWaypoints()->push_back(_waypoint);
		_craft->setDestination(_waypoint);
	}
	// Cancel
	_game->popState();
}

}
