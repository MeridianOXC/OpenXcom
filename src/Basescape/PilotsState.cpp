/*
 * Copyright 2010-2020 OpenXcom Developers.
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
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Unicode.h"
#include "../Geoscape/AllocatePsiTrainingState.h"
#include "../Geoscape/AllocateTrainingState.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Savegame/Base.h"
#include "../Savegame/CovertOperation.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "SoldierInfoState.h"
#include "SoldierMemorialState.h"
#include "SoldierTransformationListState.h"
#include "SoldierTransformationState.h"
#include "PilotsState.h"
#include <algorithm>
#include <climits>

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldiers screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
PilotsState::PilotsState(Base *base) : _base(base), _origPilotOrder(*_base->getSoldiers()), _dynGetter(NULL)
{

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(168, 17, 16, 8);
	_btnOk = new TextButton(288, 16, 16, 176);
	_cbxSortBy = new ComboBox(this, 120, 16, 192, 8, false);
	_txtName = new Text(114, 9, 16, 32);
	_txtRank = new Text(102, 9, 122, 32);
	_txtCraft = new Text(82, 9, 214, 32);
	_lstPilots = new TextList(288, 128, 8, 40);

	// Set palette
	setInterface("pilotsList");

	add(_window, "window", "pilotsList");
	add(_btnOk, "button", "pilotsList");
	add(_txtTitle, "text1", "pilotsList");
	add(_txtName, "text2", "pilotsList");
	add(_txtRank, "text2", "pilotsList");
	add(_txtCraft, "text2", "pilotsList");
	add(_lstPilots, "list", "pilotsList");
	add(_cbxSortBy, "button", "pilotsList");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "pilotsList");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&PilotsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&PilotsState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_LEFT);
	_txtTitle->setText(tr("STR_PILOT_LIST"));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setText(tr("STR_RANK"));
	_txtCraft->setText(tr("STR_ASSIGNMENT"));

	// populate sort options
	std::vector<std::string> sortOptions;
	sortOptions.push_back(tr("STR_ORIGINAL_ORDER"));
	_sortFunctors.push_back(NULL);

#define PUSH_IN(strId, functor)       \
	sortOptions.push_back(tr(strId)); \
	_sortFunctors.push_back(new SortFunctor(_game, functor));

	PUSH_IN("STR_ID", idStat);
	PUSH_IN("STR_NAME_UC", nameStat);
	PUSH_IN("STR_SOLDIER_TYPE", typeStat);
	PUSH_IN("STR_RANK", rankStat);
	PUSH_IN("STR_IDLE_DAYS", idleDaysStat);
	PUSH_IN("STR_MISSIONS2", missionsStat); //show combat flights instead #FINNIKTODO
	PUSH_IN("STR_KILLS2", killsStat); //show shot down targets instead #FINNIKTODO
	PUSH_IN("STR_WOUND_RECOVERY2", woundRecoveryStat);
	PUSH_IN("STR_MANEUVERING", maneuveringStat);
	PUSH_IN("STR_MISSILE_OPERATING", missilesStat);
	PUSH_IN("STR_DOGFIGHT", dogfightStat);
	PUSH_IN("STR_BRAVERY", braveryStat);
	PUSH_IN("STR_TRACKING", trackingStat);
	PUSH_IN("STR_COOPERATION", cooperationStat);
	if (_game->getSavedGame()->isResearched(_game->getMod()->getBeamOperationsUnlockResearch()))
	{
		PUSH_IN("STR_BEAMS_OPERATION", beamsStat);
	}
	if (_game->getSavedGame()->isResearched(_game->getMod()->getCraftSynapseUnlockResearch()))
	{
		PUSH_IN("STR_SYNAPTIC_CONNECTIVITY", synapticStat);
	}
	if (_game->getSavedGame()->isResearched(_game->getMod()->getGravControlUnlockResearch()))
	{
		PUSH_IN("STR_GRAVITY_MANIPULATION", gravityStat);
	}

#undef PUSH_IN

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&PilotsState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_lstPilots->setColumns(3, 114, 92, 74);
	_lstPilots->setAlign(ALIGN_RIGHT, 3);
	_lstPilots->setSelectable(true);
	_lstPilots->setBackground(_window);
	_lstPilots->setMargin(8);
	_lstPilots->onMouseClick((ActionHandler)&PilotsState::lstPilotsClick);
}

/**
 * cleans up dynamic state
 */
PilotsState::~PilotsState()
{
	for (std::vector<SortFunctor *>::iterator it = _sortFunctors.begin(); it != _sortFunctors.end(); ++it)
	{
		delete (*it);
	}
}

/**
 * Sorts the soldiers list by the selected criterion
 * @param action Pointer to an action.
 */
void PilotsState::cbxSortByChange(Action *action)
{
	bool ctrlPressed = _game->isCtrlPressed();
	size_t selIdx = _cbxSortBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}

	SortFunctor *compFunc = _sortFunctors[selIdx];
	_dynGetter = NULL;
	if (compFunc)
	{
		if (selIdx != 2)
		{
			_dynGetter = compFunc->getGetter();
		}

		// if CTRL is pressed, we only want to show the dynamic column, without actual sorting
		if (!ctrlPressed)
		{
			if (selIdx == 2)
			{
				std::stable_sort(_base->getSoldiers()->begin(), _base->getSoldiers()->end(),
								 [](const Soldier *a, const Soldier *b)
								 {
									 return Unicode::naturalCompare(a->getName(), b->getName());
								 });
			}
			else
			{
				std::stable_sort(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), *compFunc);
			}
			if (_game->isShiftPressed())
			{
				std::reverse(_base->getSoldiers()->begin(), _base->getSoldiers()->end());
			}
		}
	}
	else
	{
		// restore original ordering, ignoring (of course) those
		// soldiers that have been sacked since this state started
		for (std::vector<Soldier *>::const_iterator it = _origPilotOrder.begin();
			 it != _origPilotOrder.end(); ++it)
		{
			std::vector<Soldier *>::iterator soldierIt =
				std::find(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), *it);
			if (soldierIt != _base->getSoldiers()->end())
			{
				Soldier *s = *soldierIt;
				_base->getSoldiers()->erase(soldierIt);
				_base->getSoldiers()->insert(_base->getSoldiers()->end(), s);
			}
		}
	}

	size_t originalScrollPos = _lstPilots->getScroll();
	initList(originalScrollPos);
}

/**
 * Updates the soldiers list
 * after going to other screens.
 */
void PilotsState::init()
{
	State::init();

	// resets the savegame when coming back from the inventory
	_game->getSavedGame()->setBattleGame(0);
	_base->setInBattlescape(false);

	_base->prepareSoldierStatsWithBonuses(); // refresh stats for sorting
	initList(0);
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void PilotsState::initList(size_t scrl)
{
	_lstPilots->clearList();
	_soldierNumbers.clear();

	std::string selAction = "STR_PILOT_INFO";

	int offset = 0;
	//offset = 20;

	if (_dynGetter != NULL)
	{
		_lstPilots->setColumns(4, 106, 98 - offset, 60 + offset, 16);
	}
	else
	{
		_lstPilots->setColumns(3, 106, 98 - offset, 76 + offset);
	}
	_txtCraft->setX(_txtRank->getX() + 98 - offset);

	auto recovery = _base->getSumRecoveryPerDay();
	bool isBusy = false, isFree = false;
	unsigned int row = 0;
	unsigned int it = 0;
	for (std::vector<Soldier *>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getRoleRank(ROLE_PILOT) > 0) // only licensed pilots
		{
			_soldierNumbers.push_back(it); // don't forget soldier's number on the base!
			std::string duty = (*i)->getCurrentDuty(_game->getLanguage(), recovery, isBusy, isFree);
			if (_dynGetter != NULL)
			{
				// call corresponding getter
				int dynStat = (*_dynGetter)(_game, *i);
				std::ostringstream ss;
				ss << dynStat;
				_lstPilots->addRow(4, (*i)->getName(true).c_str(), tr((*i)->getRankString(true)).c_str(), duty.c_str(), ss.str().c_str());
			}
			else
			{
				_lstPilots->addRow(3, (*i)->getName(true).c_str(), tr((*i)->getRankString(true)).c_str(), duty.c_str());
			}

			if ((*i)->getCraft() == 0)
			{
				_lstPilots->setRowColor(row, _lstPilots->getSecondaryColor());
			}
			if ((*i)->getCovertOperation() != 0)
			{
				_lstPilots->setRowColor(row, _lstPilots->getColor());
			}
			if ((*i)->getDeath())
			{
				_lstPilots->setRowColor(row, _txtCraft->getColor());
			}
			row++;
		}
		it++;
	}
	if (scrl)
		_lstPilots->scrollTo(scrl);
	_lstPilots->draw();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void PilotsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void PilotsState::lstPilotsClick(Action *action)
{
	_game->pushState(new SoldierInfoState(_base, _soldierNumbers.at(_lstPilots->getSelectedRow())));
}

}
