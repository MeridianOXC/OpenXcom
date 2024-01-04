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
#include "BaseDestroyedState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Region.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Ufo.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/RuleRegion.h"
#include "../Engine/Options.h"
#include "../Basescape/SellState.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

BaseDestroyedState::BaseDestroyedState(Base *base, const Ufo* ufo, bool missiles, bool partialDestruction) :
	_base(base), _missiles(missiles), _partialDestruction(partialDestruction)
{
	_screen = false;

	int soundId = ufo->getRules()->getHitSound();
	if (soundId != Mod::NO_SOUND)
	{
		_customSound = _game->getMod()->getSound("GEO.CAT", soundId);
	}

	// Create objects
	_window = new Window(this, 256, 160, 32, 20);
	_btnOk = new TextButton(100, 20, 110, 142);
	_txtMessage = new Text(224, 48, 48, _partialDestruction ? 42 : 76);
	_lstDestroyedFacilities = new TextList(208, 40, 48, 92);

	// Set palette
	setInterface("baseDestroyed");

	add(_window, "window", "baseDestroyed");
	add(_btnOk, "button", "baseDestroyed");
	add(_txtMessage, "text", "baseDestroyed");
	add(_lstDestroyedFacilities, "text", "baseDestroyed");

	centerAllSurfaces();

	// Set up objects
	if (ufo->getRules()->getHitImage().empty())
	{
		setWindowBackground(_window, "baseDestroyed");
	}
	else
	{
		setWindowBackgroundImage(_window, ufo->getRules()->getHitImage());
	}

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BaseDestroyedState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&BaseDestroyedState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&BaseDestroyedState::btnOkClick, Options::keyCancel);

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setBig();
	_txtMessage->setWordWrap(true);

	_txtMessage->setText(tr("STR_THE_ALIENS_HAVE_DESTROYED_THE_UNDEFENDED_BASE").arg(_base->getName()));
	if (_missiles)
	{
		if (_partialDestruction)
		{
			_txtMessage->setText(tr("STR_ALIEN_MISSILES_HAVE_DAMAGED_OUR_BASE").arg(_base->getName()));
		}
		else
		{
			_txtMessage->setText(tr("STR_ALIEN_MISSILES_HAVE_DESTROYED_OUR_BASE").arg(_base->getName()));
		}
	}

	_lstDestroyedFacilities->setColumns(2, 162, 14);
	_lstDestroyedFacilities->setBackground(_window);
	_lstDestroyedFacilities->setSelectable(true);
	_lstDestroyedFacilities->setMargin(8);
	_lstDestroyedFacilities->setVisible(false);

	if (_missiles && _partialDestruction)
	{
		for (const auto& each : *_base->getDestroyedFacilitiesCache())
		{
			std::ostringstream ss;
			ss << each.second;
			_lstDestroyedFacilities->addRow(2, tr(each.first->getType()).c_str(), ss.str().c_str());
		}
		_lstDestroyedFacilities->setVisible(true);
	}

	if (_partialDestruction)
	{
		// don't remove the alien mission yet, there might be more attacks coming
		return;
	}

	AlienMission* am = _base->getRetaliationMission();
	if (!am)
	{
		// backwards-compatibility
		RuleRegion* regionRule = _game->getSavedGame()->getRegions()->front()->getRules(); // wrong, but that's how it is in OXC
		for (const auto* region : *_game->getSavedGame()->getRegions())
		{
			if (region->getRules()->insideRegion(_base->getLongitude(), _base->getLatitude()))
			{
				regionRule = region->getRules();
				break;
			}
		}
		am = _game->getSavedGame()->findAlienMission(regionRule->getType(), OBJECTIVE_RETALIATION);
	}
	_game->getSavedGame()->deleteRetaliationMission(am, _base);
}

/**
 *
 */
BaseDestroyedState::~BaseDestroyedState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BaseDestroyedState::btnOkClick(Action *)
{
	_game->popState();

	if (_partialDestruction)
	{
		if (_game->getSavedGame()->getMonthsPassed() > -1 && Options::storageLimitsEnforced && _base != 0 && _base->storesOverfull())
		{
			_game->pushState(new SellState(_base, 0, OPT_BATTLESCAPE));
			_game->pushState(new ErrorMessageState(tr("STR_STORAGE_EXCEEDED").arg(_base->getName()), _palette, _game->getMod()->getInterface("debriefing")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("debriefing")->getElement("errorPalette")->color));
		}

		// the base was damaged, but survived
		return;
	}

	for (auto xbaseIt = _game->getSavedGame()->getBases()->begin(); xbaseIt != _game->getSavedGame()->getBases()->end(); ++xbaseIt)
	{
		Base* xbase = (*xbaseIt);
		if (xbase == _base)
		{
			_game->getSavedGame()->stopHuntingXcomCrafts(xbase); // destroyed together with the base
			delete xbase;
			_game->getSavedGame()->getBases()->erase(xbaseIt);
			break;
		}
	}
}

}
