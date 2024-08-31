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
#include "InterceptState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/AlienBase.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "Globe.h"
#include "SelectDestinationState.h"
#include "ConfirmDestinationState.h"
#include "../Basescape/BasescapeState.h"
#include "../Basescape/CraftInfoState.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Intercept window.
 * @param game Pointer to the core game.
 * @param globe Pointer to the Geoscape globe.
 * @param base Pointer to base to show contained crafts (NULL to show all crafts).
 * @param target Pointer to target to intercept (NULL to ask user for target).
 */
InterceptState::InterceptState(Globe *globe, bool useCustomSound, Base *base, Target *target) : _globe(globe), _base(base), _target(target)
{
	const int WIDTH_CRAFT = 72;
	const int WIDTH_STATUS = 94;
	const int WIDTH_BASE = 74;
	const int WIDTH_WEAPONS = 48;
	_screen = false;

	if (useCustomSound)
	{
		auto& sounds = _game->getMod()->getSelectBaseSounds();
		int soundId = sounds.empty() ? Mod::NO_SOUND : sounds[RNG::generate(0, sounds.size() - 1)];
		if (soundId != Mod::NO_SOUND)
		{
			_customSound = _game->getMod()->getSound("GEO.CAT", soundId);
		}
	}

	int extraRows = Clamp(Options::oxceInterceptTableSize, 8, 80) - 8;
	int extraHeight = 8 * extraRows;
	int offset = extraHeight / 2;

	// Create objects
	if (Options::oxceInterceptGuiMaintenanceTime > 0)
	{
		_window = new Window(this, 320, 140 + extraHeight, 0, 30 - offset, POPUP_HORIZONTAL);
		_btnCancel = new TextButton(_base ? 142 : 288, 16, 16, 146 + offset);
		_btnGotoBase = new TextButton(142, 16, 162, 146 + offset);
		_txtTitle = new Text(300, 17, 10, 46 - offset);
		int x = 14;
		_txtCraft = new Text(WIDTH_CRAFT, 9, x, 70 - offset);
		x += WIDTH_CRAFT;
		_txtStatus = new Text(WIDTH_STATUS, 9, x, 70 - offset);
		x += WIDTH_STATUS;
		_txtBase = new Text(WIDTH_BASE, 9, x, 70 - offset);
		x += WIDTH_BASE;
		_txtWeapons = new Text(WIDTH_WEAPONS+4, 17, x-4, 62 - offset);
		_lstCrafts = new TextList(290, 64 + extraHeight, 12, 78 - offset);
	}
	else
	{
		_window = new Window(this, 320, 140 + extraHeight, 0, 30 - offset, POPUP_HORIZONTAL);
		_btnCancel = new TextButton(_base ? 142 : 288, 16, 16, 146 + offset);
		_btnGotoBase = new TextButton(142, 16, 162, 146 + offset);
		_txtTitle = new Text(300, 17, 10, 46 - offset);
		_txtCraft = new Text(86, 9, 14, 70 - offset);
		_txtStatus = new Text(70, 9, 100, 70 - offset);
		_txtBase = new Text(80, 9, 170, 70 - offset);
		_txtWeapons = new Text(80, 17, 238, 62 - offset);
		_lstCrafts = new TextList(288, 64 + extraHeight, 8, 78 - offset);
	}

	// Set palette
	setInterface("intercept");

	add(_window, "window", "intercept");
	add(_btnCancel, "button", "intercept");
	add(_btnGotoBase, "button", "intercept");
	add(_txtTitle, "text1", "intercept");
	add(_txtCraft, "text2", "intercept");
	add(_txtStatus, "text2", "intercept");
	add(_txtBase, "text2", "intercept");
	add(_txtWeapons, "text2", "intercept");
	add(_lstCrafts, "list", "intercept");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "intercept");

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&InterceptState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&InterceptState::btnCancelClick, Options::keyCancel);
	_btnCancel->onKeyboardPress((ActionHandler)&InterceptState::btnCancelClick, Options::keyGeoIntercept);

	_btnGotoBase->setText(tr("STR_GO_TO_BASE"));
	_btnGotoBase->onMouseClick((ActionHandler)&InterceptState::btnGotoBaseClick);
	_btnGotoBase->setVisible(_base != 0);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_LAUNCH_INTERCEPTION"));

	_txtCraft->setText(tr("STR_CRAFT"));

	_txtStatus->setText(tr("STR_STATUS"));

	_txtBase->setText(tr("STR_BASE"));

	if (Options::oxceInterceptGuiMaintenanceTime > 0)
	{
		_txtWeapons->setAlign(ALIGN_RIGHT);
	}
	_txtWeapons->setText(tr("STR_WEAPONS_CREW_HWPS"));

	if (Options::oxceInterceptGuiMaintenanceTime > 0)
	{
		_lstCrafts->setColumns(4, WIDTH_CRAFT, WIDTH_STATUS, WIDTH_BASE, WIDTH_WEAPONS);
		_lstCrafts->setAlign(ALIGN_RIGHT, 3);
	}
	else
	{
		_lstCrafts->setColumns(4, 86, 70, 80, 46);
	}
	_lstCrafts->setSelectable(true);
	_lstCrafts->setBackground(_window);
	if (Options::oxceInterceptGuiMaintenanceTime > 0)
	{
		_lstCrafts->setMargin(2);
	}
	else
	{
		_lstCrafts->setMargin(6);
	}
	_lstCrafts->onMouseClick((ActionHandler)&InterceptState::lstCraftsLeftClick);
	_lstCrafts->onMouseClick((ActionHandler)&InterceptState::lstCraftsRightClick, SDL_BUTTON_RIGHT);
	_lstCrafts->onMouseClick((ActionHandler)&InterceptState::lstCraftsMiddleClick, SDL_BUTTON_MIDDLE);

	//clear list of selected crafts before creating a new wing
	_selCrafts.clear();

	int row = 0;
	for (auto* xbase : *_game->getSavedGame()->getBases())
	{
		if (_base != 0 && xbase != _base)
			continue;
		for (auto* xcraft : *xbase->getCrafts())
		{
			std::ostringstream ssStatus;
			std::string status = xcraft->getStatus();

			bool hasEnoughPilots = xcraft->arePilotsOnboard();
			if (status == "STR_OUT")
			{
				// QoL: let's give the player a bit more info
				if (xcraft->getDestination() == 0 || xcraft->getIsAutoPatrolling())
				{
					ssStatus << tr("STR_PATROLLING");
				}
				else if (xcraft->getLowFuel() || xcraft->getMissionComplete() || xcraft->getDestination() == (Target*)xcraft->getBase())
				{
					ssStatus << tr("STR_RETURNING");
					//ssStatus << tr("STR_RETURNING_TO_BASE"); // vanilla craft info
				}
				else
				{
					Ufo *u = dynamic_cast<Ufo*>(xcraft->getDestination());
					MissionSite *m = dynamic_cast<MissionSite*>(xcraft->getDestination());
					AlienBase *b = dynamic_cast<AlienBase*>(xcraft->getDestination());
					Craft *craftTarget = dynamic_cast<Craft*>(xcraft->getDestination());
					if (u != 0)
					{
						if (xcraft->isInDogfight())
						{
							ssStatus << tr("STR_TAILING_UFO");
						}
						else if (u->getStatus() == Ufo::FLYING)
						{
							ssStatus << tr("STR_INTERCEPTING");
							//ssStatus << tr("STR_INTERCEPTING_UFO").arg(u->getId()); // vanilla craft info
						}
						else
						{
							ssStatus << tr("STR_EN_ROUTE");
						}
					}
					else if (craftTarget != 0)
					{
						ssStatus << tr("STR_ESCORTING");
					}
					else if (m != 0 || b != 0)
					{
						ssStatus << tr("STR_EN_ROUTE");
					}
					else
					{
						ssStatus << tr(status); // "STR_OUT"
					}
				}
			}
			else
			{
				if (!hasEnoughPilots && status == "STR_READY")
				{
					ssStatus << tr("STR_PILOT_MISSING");
				}
				else
				{
					ssStatus << tr(status);
				}
			}
			if (status != "STR_READY" && status != "STR_OUT")
			{
				unsigned int maintenanceHours = 0;

				if (Options::oxceInterceptGuiMaintenanceTime == 2 || xcraft->getStatus() == "STR_REPAIRS")
				{
					maintenanceHours += xcraft->calcRepairTime();
				}
				if (Options::oxceInterceptGuiMaintenanceTime == 2 || xcraft->getStatus() == "STR_REFUELLING")
				{
					maintenanceHours += xcraft->calcRefuelTime();
				}
				if (Options::oxceInterceptGuiMaintenanceTime == 2 || xcraft->getStatus() == "STR_REARMING")
				{
					// Note: if the craft is already refueling, don't count any potential rearm time (can be > 0 if ammo is missing)
					if (xcraft->getStatus() != "STR_REFUELLING")
					{
						maintenanceHours += xcraft->calcRearmTime();
					}
				}

				int days = maintenanceHours / 24;
				int hours = maintenanceHours % 24;
				if (maintenanceHours > 0 && Options::oxceInterceptGuiMaintenanceTime > 0)
				{
					ssStatus << " (";
					if (days > 0)
					{
						ssStatus << tr("STR_DAY_SHORT").arg(days);
					}
					if (hours > 0)
					{
						if (days > 0)
						{
							ssStatus << "/";
						}
						ssStatus << tr("STR_HOUR_SHORT").arg(hours);
					}
					ssStatus << ")";
				}
			}
			else
			{
				// ETA display
				if (Options::oxceShowETAMode > 0 && xcraft->getDestination() && Options::oxceInterceptGuiMaintenanceTime > 0)
				{
					MovingTarget* mt = dynamic_cast<MovingTarget*>(xcraft->getDestination());
					if (Options::oxceShowETAMode == 1 && mt && mt->getSpeed() > 0)
					{
						// don't show ETA for moving targets (i.e. UFOs and crafts)
					}
					else
					{
						int speed = xcraft->getCraftStats().speedMax;
						int distance = XcomDistance(xcraft->getDistance(xcraft->getDestination()));
						int etaInHoursHelper = (distance + (speed / 2)) / speed;
						int days = etaInHoursHelper / 24;
						int hours = etaInHoursHelper % 24;
						ssStatus << " (";
						if (days > 0) ssStatus << tr("STR_DAY_SHORT").arg(days);
						if (hours > 0 || days == 0)
						{
							if (days > 0) ssStatus << "/";
							ssStatus << tr("STR_HOUR_SHORT").arg(hours);
						}
						ssStatus << ")";
					}
				}
			}

			std::ostringstream ss;
			if (xcraft->getNumWeapons() > 0)
			{
				ss << Unicode::TOK_COLOR_FLIP << xcraft->getNumWeapons() << Unicode::TOK_COLOR_FLIP;
			}
			else
			{
				ss << 0;
			}
			ss << "/";
			if (xcraft->getNumTotalSoldiers() > 0)
			{
				ss << Unicode::TOK_COLOR_FLIP << xcraft->getNumTotalSoldiers() << Unicode::TOK_COLOR_FLIP;
			}
			else
			{
				ss << 0;
			}
			ss << "/";
			if (xcraft->getNumTotalVehicles() > 0)
			{
				ss << Unicode::TOK_COLOR_FLIP << xcraft->getNumTotalVehicles() << Unicode::TOK_COLOR_FLIP;
			}
			else
			{
				ss << 0;
			}
			_crafts.push_back(xcraft);
			_lstCrafts->addRow(4, xcraft->getName(_game->getLanguage()).c_str(), ssStatus.str().c_str(), xbase->getName().c_str(), ss.str().c_str());
			if (hasEnoughPilots && status == "STR_READY")
			{
				_lstCrafts->setCellColor(row, 1, _lstCrafts->getSecondaryColor());
			}
			row++;
		}
	}
}

/**
 *
 */
InterceptState::~InterceptState()
{

}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void InterceptState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Goes to the base for the respective craft.
 * @param action Pointer to an action.
 */
void InterceptState::btnGotoBaseClick(Action *)
{
	_game->popState();
	_game->pushState(new BasescapeState(_base, _globe));
}

/**
 * Pick a target for the selected craft.
 * @param action Pointer to an action.
 */
void InterceptState::lstCraftsLeftClick(Action *)
{
	// condition used in shift and non-shift paths
	auto allowStart = [&](Craft* c)
	{
		return c->getStatus() == "STR_READY" || (
			 (c->getStatus() == "STR_OUT" || Options::craftLaunchAlways) &&
			 !c->getLowFuel() &&
			 !c->getMissionComplete() );
	};

	unsigned int row;
	row = _lstCrafts->getSelectedRow();
	Craft* c = _crafts[row];

	// add and remove crafts to the wing to be created
	if (_game->isShiftPressed())
	{
		// add craft to the list when it is not included yet
		// limit to 4 (3+1 due to the dogfight window)
		// need more sanity checks?
		if (allowStart(c) && (_selCrafts.size() < 3) && !( std::find(_selCrafts.begin(), _selCrafts.end(), (Craft*) c) != _selCrafts.end()))
		{
			_selCrafts.push_back(c);
			_lstCrafts->setCellColor(row, 0, _lstCrafts->getSecondaryColor());
		}
		// remove craft from the wing to be created when it is already included
		else if (( std::find(_selCrafts.begin(), _selCrafts.end(), (Craft*) c) != _selCrafts.end()))
		{
			auto craftIt = std::find (_selCrafts.begin(), _selCrafts.end(), (Craft*) c);
			_selCrafts.erase(craftIt);
			_lstCrafts->setCellColor(row, 0, _lstCrafts->getColor());
 		}
	}
	// add last craft to the wing (may already be part of the wing) and launch
	else
	{
		if (allowStart(c))
 		{
			//remove itself from the vector assuming it is already inside, so it can be put in front of the vector
			auto craftIt = std::find (_selCrafts.begin(), _selCrafts.end(), c);
			if ( craftIt != _selCrafts.end() )
			{
				_selCrafts.erase(craftIt);
			}
			_selCrafts.insert(_selCrafts.begin(), c);

			_game->popState();
			if (_target == 0)
			{
				_game->pushState(new SelectDestinationState(_selCrafts, _globe));
			}
			else
			{
				_game->pushState(new ConfirmDestinationState(_selCrafts, _target));
			}
		 }
	}
}

/**
 * Centers on the selected craft.
 * @param action Pointer to an action.
 */
void InterceptState::lstCraftsRightClick(Action *)
{
	Craft* c = _crafts[_lstCrafts->getSelectedRow()];
	if (c->getStatus() == "STR_OUT")
	{
		_globe->center(c->getLongitude(), c->getLatitude());
		_game->popState();
	}
	else
	{
		_game->popState();

		bool found = false;
		for (auto* xbase : *_game->getSavedGame()->getBases())
		{
			if (_base != 0 && xbase != _base)
				continue;
			for (size_t ci = 0; ci < xbase->getCrafts()->size(); ++ci)
			{
				if (c == xbase->getCrafts()->at(ci))
				{
					_game->pushState(new CraftInfoState(xbase, ci));
					found = true;
					break;
				}
			}
			if (found) break;
		}
	}
}

/**
* Opens the corresponding Ufopaedia article.
* @param action Pointer to an action.
*/
void InterceptState::lstCraftsMiddleClick(Action *)
{
	Craft* c = _crafts[_lstCrafts->getSelectedRow()];
	if (c)
	{
		std::string articleId = c->getRules()->getType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

}
