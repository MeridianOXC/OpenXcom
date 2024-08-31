/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "UfoTrackerState.h"
#include "InterceptState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/SavedGame.h"
#include "../Engine/Options.h"
#include "Globe.h"
#include "GeoscapeState.h"
#include "UfoDetectedState.h"
#include "TargetInfoState.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the UfoTracker window.
 * @param state Pointer to the Geoscape.
 * @param globe Pointer to the Geoscape globe.
 */
UfoTrackerState::UfoTrackerState(GeoscapeState *state, Globe *globe) : _state(state), _globe(globe)
{
	const int WIDTH_OBJECT = 88;
	const int WIDTH_SIZE = 60;
	const int WIDTH_ALTITUDE = 54;
	const int WIDTH_HEADING = 54;
	const int WIDTH_SPEED = 32;
	_screen = false;

	int extraRows = Clamp(Options::oxceInterceptTableSize, 8, 80) - 8;
	int extraHeight = 8 * extraRows;
	int offset = extraHeight / 2;

	// Create objects
	_window = new Window(this, 320, 140 + extraHeight, 0, 30 - offset, POPUP_HORIZONTAL);
	_btnCancel = new TextButton(288, 16, 16, 146 + offset);
	_txtTitle = new Text(300, 17, 10, 46 - offset);
	int x = 14;
	_txtObject = new Text(WIDTH_OBJECT, 9, x, 70 - offset);
	x += WIDTH_OBJECT;
	_txtSize = new Text(WIDTH_SIZE, 9, x, 70 - offset);
	x += WIDTH_SIZE;
	_txtAltitude = new Text(WIDTH_ALTITUDE, 9, x, 70 - offset);
	x += WIDTH_ALTITUDE;
	_txtHeading = new Text(WIDTH_HEADING, 9, x, 70 - offset);
	x += WIDTH_HEADING;
	_txtSpeed = new Text(WIDTH_SPEED+16, 17, x-16, 70 - offset); // 16 pixels overlap for translators
	_lstObjects = new TextList(290, 64 + extraHeight, 12, 78 - offset);

	// Set palette
	setInterface("ufoTracker");

	add(_window, "window", "ufoTracker");
	add(_btnCancel, "button", "ufoTracker");
	add(_txtTitle, "text1", "ufoTracker");
	add(_txtObject, "text2", "ufoTracker");
	add(_txtSize, "text2", "ufoTracker");
	add(_txtAltitude, "text2", "ufoTracker");
	add(_txtHeading, "text2", "ufoTracker");
	add(_txtSpeed, "text2", "ufoTracker");
	add(_lstObjects, "list", "ufoTracker");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "ufoTracker");

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&UfoTrackerState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&UfoTrackerState::btnCancelClick, Options::keyCancel);
	_btnCancel->onKeyboardPress((ActionHandler)&UfoTrackerState::btnCancelClick, Options::keyGeoUfoTracker);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_UFO_TRACKER"));

	_txtObject->setText(tr("STR_NAME_UC"));

	_txtSize->setText(tr("STR_SIZE_UC"));

	_txtAltitude->setText(tr("STR_ALTITUDE"));

	_txtHeading->setText(tr("STR_HEADING"));

	_txtSpeed->setAlign(ALIGN_RIGHT);
	_txtSpeed->setText(tr("STR_SPEED"));

	_lstObjects->setColumns(5, WIDTH_OBJECT, WIDTH_SIZE, WIDTH_ALTITUDE, WIDTH_HEADING, WIDTH_SPEED);
	_lstObjects->setAlign(ALIGN_RIGHT, 4);
	_lstObjects->setSelectable(true);
	_lstObjects->setBackground(_window);
	_lstObjects->setMargin(2);
	_lstObjects->onMouseClick((ActionHandler)&UfoTrackerState::lstObjectsLeftClick);
	_lstObjects->onMouseClick((ActionHandler)&UfoTrackerState::lstObjectsRightClick, SDL_BUTTON_RIGHT);
	_lstObjects->onMouseClick((ActionHandler)&UfoTrackerState::lstObjectsMiddleClick, SDL_BUTTON_MIDDLE);

	int row = 0;
	for (auto* site : *_game->getSavedGame()->getMissionSites())
	{
		if (!site->getDetected())
			continue;

		_objects.push_back(site);
		_lstObjects->addRow(1, site->getName(_game->getLanguage()).c_str());
		_lstObjects->setCellColor(row, 0, _lstObjects->getSecondaryColor());
		row++;
	}

	for (auto* ufo : *_game->getSavedGame()->getUfos())
	{
		if (!ufo->getDetected() || ufo->getStatus() == Ufo::IGNORE_ME)
			continue;

		std::ostringstream ss1;
		ss1 << tr(ufo->getRules()->getSize());

		std::ostringstream ss2;
		std::string altitude = ufo->getAltitude() == "STR_GROUND" ? "STR_GROUNDED" : ufo->getAltitude();
		ss2 << tr(altitude);

		std::ostringstream ss3;
		std::string heading = ufo->getStatus() != Ufo::FLYING ? "STR_NONE_UC" : ufo->getDirection();
		ss3 << tr(heading);

		std::ostringstream ss4;
		ss4 << Unicode::formatNumber(ufo->getSpeed());

		_objects.push_back(ufo);
		_lstObjects->addRow(5, ufo->getName(_game->getLanguage()).c_str(), ss1.str().c_str(), ss2.str().c_str(), ss3.str().c_str(), ss4.str().c_str());
		if (altitude == "STR_GROUNDED")
		{
			_lstObjects->setCellColor(row, 2, _lstObjects->getSecondaryColor());
		}
		row++;
	}

	for (auto* ab : *_game->getSavedGame()->getAlienBases())
	{
		if (!ab->isDiscovered())
			continue;

		_objects.push_back(ab);
		_lstObjects->addRow(1, ab->getName(_game->getLanguage()).c_str());
		row++;
	}
}

/**
 *
 */
UfoTrackerState::~UfoTrackerState()
{

}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void UfoTrackerState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
* Displays the right popup for a specific target.
* @param target Pointer to target.
*/
void UfoTrackerState::popupTarget(Target *target)
{
	_game->popState();

	Ufo* u = dynamic_cast<Ufo*>(target);

	if (u != 0)
	{
		_game->pushState(new UfoDetectedState(u, _state, false, u->getHyperDetected()));
	}
	else
	{
		_game->pushState(new TargetInfoState(target, _globe));
	}
}

/**
 * Displays object detail.
 * @param action Pointer to an action.
 */
void UfoTrackerState::lstObjectsLeftClick(Action *)
{
	Target* t = _objects[_lstObjects->getSelectedRow()];
	if (t)
	{
		popupTarget(t);
	}
}

/**
 * Centers on the selected object.
 * @param action Pointer to an action.
 */
void UfoTrackerState::lstObjectsRightClick(Action *)
{
	Target* t = _objects[_lstObjects->getSelectedRow()];
	if (t)
	{
		_globe->center(t->getLongitude(), t->getLatitude());
		_game->popState();
	}
}

/**
* Opens the corresponding Ufopaedia article.
* @param action Pointer to an action.
*/
void UfoTrackerState::lstObjectsMiddleClick(Action *)
{
	Target* t = _objects[_lstObjects->getSelectedRow()];
	if (t)
	{
		Ufo* u = dynamic_cast<Ufo*>(t);

		if (u != 0 && u->getHyperDetected())
		{
			std::string articleId = u->getRules()->getType();
			Ufopaedia::openArticle(_game, articleId);
		}
	}
}

}
