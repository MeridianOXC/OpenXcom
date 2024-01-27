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
#include "DismantleFacilityState.h"
#include "../Engine/Game.h"
#include "../Engine/Sound.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/ItemContainer.h"
#include "BaseView.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in a Dismantle Facility window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param view Pointer to the baseview to update.
 * @param fac Pointer to the facility to dismantle.
 */
DismantleFacilityState::DismantleFacilityState(Base *base, BaseView *view, BaseFacility *fac) : _base(base), _view(view), _fac(fac)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 152, 80, 20, 60);
	_btnOk = new TextButton(44, 16, 36, 115);
	_btnCancel = new TextButton(44, 16, 112, 115);
	_txtTitle = new Text(142, 9, 25, 75);
	_txtFacility = new Text(142, 9, 25, 85);
	_txtRefundValue = new Text(142, 9, 25, 100);

	// Set palette
	setInterface("dismantleFacility");

	add(_window, "window", "dismantleFacility");
	add(_btnOk, "button", "dismantleFacility");
	add(_btnCancel, "button", "dismantleFacility");
	add(_txtTitle, "text", "dismantleFacility");
	add(_txtFacility, "text", "dismantleFacility");
	add(_txtRefundValue, "text", "dismantleFacility");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "dismantleFacility");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&DismantleFacilityState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&DismantleFacilityState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&DismantleFacilityState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&DismantleFacilityState::btnCancelClick, Options::keyCancel);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_DISMANTLE"));

	_txtFacility->setAlign(ALIGN_CENTER);
	_txtFacility->setText(tr(_fac->getRules()->getType()));

	int refundValue = 0;
	if (_fac->getBuildTime() > _fac->getRules()->getBuildTime())
	{
		// if only queued... full refund (= build cost)
		refundValue = _fac->getRules()->getBuildCost();
	}
	else
	{
		// if already building or already built... partial refund (by default 0, used in mods)
		refundValue = _fac->getRules()->getRefundValue();
	}

	_txtRefundValue->setAlign(ALIGN_CENTER);
	_txtRefundValue->setText(tr("STR_REFUND_VALUE").arg(Unicode::formatFunding(refundValue)));
	_txtRefundValue->setVisible(refundValue != 0);
}

/**
 *
 */
DismantleFacilityState::~DismantleFacilityState()
{

}

/**
 * Dismantles the facility and returns
 * to the previous screen.
 * @param action Pointer to an action.
 */
void DismantleFacilityState::btnOkClick(Action *)
{
	if (!_fac->getRules()->isLift())
	{
		const std::map<std::string, std::pair<int, int> > &itemCost = _fac->getRules()->getBuildCostItems();

		if (_fac->getBuildTime() > _fac->getRules()->getBuildTime())
		{
			// Give full refund if this is a (not yet started) queued build.
			_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _fac->getRules()->getBuildCost());
			for (auto& pair : itemCost)
			{
				_base->getStorageItems()->addItem(pair.first, pair.second.first);
			}
		}
		else
		{
			// Give partial refund if this is a started build or a completed facility.
			_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _fac->getRules()->getRefundValue());
			for (auto& pair : itemCost)
			{
				_base->getStorageItems()->addItem(pair.first, pair.second.second);
			}
		}
		if (_fac->getAmmo() > 0)
		{
			// Full refund of loaded ammo
			_base->getStorageItems()->addItem(_fac->getRules()->getAmmoItem(), _fac->getAmmo());
			_fac->setAmmo(0);
		}

		for (auto facIt = _base->getFacilities()->begin(); facIt != _base->getFacilities()->end(); ++facIt)
		{
			if (*facIt == _fac)
			{
				_base->getFacilities()->erase(facIt);
				// Determine if we leave behind any facilities when this one is removed
				if (_fac->getBuildTime() == 0 && _fac->getRules()->getLeavesBehindOnSell().size() != 0)
				{
					const auto& facList = _fac->getRules()->getLeavesBehindOnSell();
					if (facList.at(0)->getPlaceSound() != Mod::NO_SOUND)
					{
						_game->getMod()->getSound("GEO.CAT", facList.at(0)->getPlaceSound())->play();
					}
					// Make sure the size of the facilities left behind matches the one we removed
					if (facList.at(0)->getSizeX() == _fac->getRules()->getSizeX() && facList.at(0)->getSizeY() == _fac->getRules()->getSizeY()) // equal size facilities
					{
						BaseFacility *fac = new BaseFacility(facList.at(0), _base);
						fac->setX(_fac->getX());
						fac->setY(_fac->getY());
						if (_fac->getRules()->getRemovalTime() <= -1)
						{
							fac->setBuildTime(fac->getRules()->getBuildTime());
						}
						else
						{
							fac->setBuildTime(_fac->getRules()->getRemovalTime());
						}
						if (fac->getBuildTime() != 0)
						{
							fac->setIfHadPreviousFacility(true);
						}
						_base->getFacilities()->push_back(fac);
					}
					else
					{
						size_t j = 0;
						// Otherwise, assume the list of facilities is size 1, and just iterate over it to fill in the old facility's place
						for (int y = _fac->getY(); y != _fac->getY() + _fac->getRules()->getSizeY(); ++y)
						{
							for (int x = _fac->getX(); x != _fac->getX() + _fac->getRules()->getSizeX(); ++x)
							{
								BaseFacility *fac = new BaseFacility(facList.at(j), _base);
								fac->setX(x);
								fac->setY(y);
								if (_fac->getRules()->getRemovalTime() <= -1)
								{
									fac->setBuildTime(fac->getRules()->getBuildTime());
								}
								else
								{
									fac->setBuildTime(_fac->getRules()->getRemovalTime());
								}
								if (fac->getBuildTime() != 0)
								{
									fac->setIfHadPreviousFacility(true);
								}
								_base->getFacilities()->push_back(fac);

								++j;
								if (j == facList.size())
								{
									j = 0;
								}
							}
						}
					}
				}
				_view->resetSelectedFacility();
				delete _fac;
				// Reset the basescape view in case new facilities were created by removing the old one
				_view->setBase(_base);
				if (Options::allowBuildingQueue) _view->reCalcQueuedBuildings();
				break;
			}
		}
	}
	// Remove whole base if it's the access lift
	else
	{
		for (auto xbaseIt = _game->getSavedGame()->getBases()->begin(); xbaseIt != _game->getSavedGame()->getBases()->end(); ++xbaseIt)
		{
			if (*xbaseIt == _base)
			{
				_game->getSavedGame()->getBases()->erase(xbaseIt);
				delete _base;
				break;
			}
		}
	}
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void DismantleFacilityState::btnCancelClick(Action *)
{
	_game->popState();
}

}
