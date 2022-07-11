/*
 * Copyright 2010-2022 OpenXcom Developers.
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
#include "ManufactureAllocateEngineers.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleSoldier.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Production.h"
#include "../Mod/RuleManufacture.h"
#include "../Basescape/ManufactureInfoStateFtA.h"
#include "SoldierInfoState.h"
#include <algorithm>
#include <climits>

namespace OpenXcom
{

/**
	* Initializes all the elements in the CovertOperation Soldiers screen.
	* @param base Pointer to the base to get info from.
	* @param operation Pointer to starting (not committed) covert operation.
	*/
ManufactureAllocateEngineers::ManufactureAllocateEngineers(Base* base, ManufactureInfoStateFtA* planningProject)
	: _base(base), _planningProject(planningProject), _otherCraftColor(0), _origSoldierOrder(*_base->getSoldiers()), _dynGetter(NULL)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(300, 17, 16, 7);
	_txtName = new Text(114, 9, 16, 32);
	_txtRank = new Text(102, 9, 122, 32);
	_txtCraft = new Text(84, 9, 220, 32);
	_txtUsed = new Text(95, 9, 122, 24);
	_cbxSortBy = new ComboBox(this, 148, 16, 8, 176, true);
	_lstEngineers = new TextList(288, 128, 8, 40);

	// Set palette
	setInterface("craftSoldiers");

	add(_window, "window", "craftSoldiers");
	add(_btnOk, "button", "craftSoldiers");
	add(_txtTitle, "text", "craftSoldiers");
	add(_txtName, "text", "craftSoldiers");
	add(_txtRank, "text", "craftSoldiers");
	add(_txtCraft, "text", "craftSoldiers");
	add(_txtUsed, "text", "craftSoldiers");
	add(_lstEngineers, "list", "craftSoldiers");
	add(_cbxSortBy, "button", "craftSoldiers");

	_otherCraftColor = _game->getMod()->getInterface("craftSoldiers")->getElement("otherCraft")->color;

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftSoldiers");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ManufactureAllocateEngineers::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ManufactureAllocateEngineers::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&ManufactureAllocateEngineers::btnDeassignProjectEngineersClick, Options::keyRemoveSoldiersFromCraft);

	_txtTitle->setBig();
	_txtTitle->setText(tr(planningProject->getManufactureRules()->getName()));
	_txtTitle->setWordWrap(true);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);

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
	PUSH_IN("STR_MISSIONS2", missionsStat);
	PUSH_IN("STR_KILLS2", killsStat);
	PUSH_IN("STR_WOUND_RECOVERY2", woundRecoveryStat);
	if (_game->getMod()->isManaFeatureEnabled() && !_game->getMod()->getReplenishManaAfterMission())
	{
		PUSH_IN("STR_MANA_MISSING", manaMissingStat);
	}
	PUSH_IN("STR_TIME_UNITS", tuStat);
	PUSH_IN("STR_STAMINA", staminaStat);
	PUSH_IN("STR_HEALTH", healthStat);
	PUSH_IN("STR_BRAVERY", braveryStat);
	PUSH_IN("STR_REACTIONS", reactionsStat);
	PUSH_IN("STR_FIRING_ACCURACY", firingStat);
	PUSH_IN("STR_THROWING_ACCURACY", throwingStat);
	PUSH_IN("STR_MELEE_ACCURACY", meleeStat);
	PUSH_IN("STR_STRENGTH", strengthStat);
	if (_game->getMod()->isManaFeatureEnabled())
	{
		// "unlock" is checked later
		PUSH_IN("STR_MANA_POOL", manaStat);
	}
	PUSH_IN("STR_PSIONIC_STRENGTH", psiStrengthStat);
	PUSH_IN("STR_PSIONIC_SKILL", psiSkillStat);

#undef PUSH_IN

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&ManufactureAllocateEngineers::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_lstEngineers->setColumns(3, 106, 98, 76);
	_lstEngineers->setAlign(ALIGN_RIGHT, 3);
	_lstEngineers->setSelectable(true);
	_lstEngineers->setBackground(_window);
	_lstEngineers->setMargin(8);
	_lstEngineers->onMouseClick((ActionHandler)&ManufactureAllocateEngineers::lstEngineersClick, 0);
}

/**
	* cleans up dynamic state
	*/
ManufactureAllocateEngineers::~ManufactureAllocateEngineers()
{
	for (std::vector<SortFunctor*>::iterator it = _sortFunctors.begin();
		it != _sortFunctors.end(); ++it)
	{
		delete (*it);
	}
}

/**
	* Sorts the soldiers list by the selected criterion
	* @param action Pointer to an action.
	*/
void ManufactureAllocateEngineers::cbxSortByChange(Action*)
{
	bool ctrlPressed = _game->isCtrlPressed();
	size_t selIdx = _cbxSortBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}

	SortFunctor* compFunc = _sortFunctors[selIdx];
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
					[](const Soldier* a, const Soldier* b)
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
		for (std::vector<Soldier*>::const_iterator it = _origSoldierOrder.begin();
			it != _origSoldierOrder.end(); ++it)
		{
			std::vector<Soldier*>::iterator soldierIt =
				std::find(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), *it);
			if (soldierIt != _base->getSoldiers()->end())
			{
				Soldier* s = *soldierIt;
				_base->getSoldiers()->erase(soldierIt);
				_base->getSoldiers()->insert(_base->getSoldiers()->end(), s);
			}
		}
	}

	size_t originalScrollPos = _lstEngineers->getScroll();
	initList(originalScrollPos);
}

/**
	* Returns to the previous screen.
	* @param action Pointer to an action.
	*/
void ManufactureAllocateEngineers::btnOkClick(Action*)
{
	_game->popState();
}

/**
	* Shows the soldiers in a list at specified offset/scroll.
	*/
void ManufactureAllocateEngineers::initList(size_t scrl)
{
	int row = 0;
	_lstEngineers->clearList();

	if (_dynGetter != NULL)
	{
		_lstEngineers->setColumns(4, 106, 98, 60, 16);
	}
	else
	{
		_lstEngineers->setColumns(3, 106, 98, 76);
	}

	auto recovery = _base->getSumRecoveryPerDay();
	bool isBusy = false, isFree = false;
	unsigned int it = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getRoleRank(ROLE_ENGINEER) > 0)
		{
			_scientistsNumbers.push_back(it); // don't forget soldier's number on the base!
			std::string duty = (*i)->getCurrentDuty(_game->getLanguage(), recovery, isBusy, isFree);
			if (_dynGetter != NULL)
			{
				// call corresponding getter
				int dynStat = (*_dynGetter)(_game, *i);
				std::ostringstream ss;
				ss << dynStat;
				_lstEngineers->addRow(4, (*i)->getName(true, 19).c_str(), tr((*i)->getRankString(true)).c_str(), duty.c_str(), ss.str().c_str());
			}
			else
			{
				_lstEngineers->addRow(3, (*i)->getName(true, 19).c_str(), tr((*i)->getRankString(true)).c_str(), duty.c_str());
			}

			Uint8 color;
			bool matched = false;
			auto engineers = _planningProject->getEngineers();
			auto iter = std::find(std::begin(engineers), std::end(engineers), (*i));
			if (iter != std::end(engineers))
			{
				matched = true;
			}

			if (matched)
			{
				color = _lstEngineers->getSecondaryColor();
				_lstEngineers->setCellText(row, 2, tr("STR_ASSIGNED_UC"));
			}
			else if (isBusy || !isFree)
			{
				color = _otherCraftColor;
			}
			else
			{
				color = _lstEngineers->getColor();
			}
			_lstEngineers->setRowColor(row, color);
			row++;
		}
		it++;
	}
	if (scrl)
		_lstEngineers->scrollTo(scrl);
	_lstEngineers->draw();

	_txtUsed->setText(tr("STR_ALLOCATED_TO_PROJECT").arg(_planningProject->getEngineers().size()));
}

/**
	* Shows the soldiers in a list.
	*/
void ManufactureAllocateEngineers::init()
{
	State::init();
	_base->prepareSoldierStatsWithBonuses(); // refresh stats for sorting
	initList(0);
}

/**
	* Shows the selected soldier's info.
	* @param action Pointer to an action.
	*/
void ManufactureAllocateEngineers::lstEngineersClick(Action* action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstEngineers->getArrowsLeftEdge() && mx < _lstEngineers->getArrowsRightEdge())
	{
		return;
	}
	int row = _lstEngineers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Soldier* s = _base->getSoldiers()->at(_scientistsNumbers.at(row));
		Uint8 color = _lstEngineers->getColor();
		bool isBusy = false, isFree = false, matched = false;
		std::string duty = s->getCurrentDuty(_game->getLanguage(), _base->getSumRecoveryPerDay(), isBusy, isFree, WORK);
		auto engineers = _planningProject->getEngineers();
		auto iter = std::find(std::begin(engineers), std::end(engineers), s);
		if (iter != std::end(engineers))
		{
			matched = true;
		}
		if (matched)
		{
			for (int k = 0; k < _planningProject->getEngineers().size(); k++)
			{
				_planningProject->removeEngineer(s);
			}

			_lstEngineers->setCellText(row, 2, duty);
			if (isBusy || !isFree || s->getCraft() != 0)
			{
				color = _otherCraftColor;

			}
			else
			{
				color = _lstEngineers->getColor();
			}
		}
		else if (s->hasFullHealth() && !isBusy)
		{
			_lstEngineers->setCellText(row, 2, tr("STR_ASSIGNED_UC"));
			color = _lstEngineers->getSecondaryColor();
			_planningProject->addEngineer(s);
		}

		_lstEngineers->setRowColor(row, color);
		_txtUsed->setText(tr("STR_ALLOCATED_TO_PROJECT").arg(_planningProject->getEngineers().size()));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->pushState(new SoldierInfoState(_base, _scientistsNumbers.at(row)));
	}
}

/**
	* De-assign all soldiers from the current craft.
	* @param action Pointer to an action.
	*/
void ManufactureAllocateEngineers::btnDeassignProjectEngineersClick(Action* action)
{
	for (auto e : _planningProject->getEngineers())
	{
		_planningProject->removeEngineer(e);
	}
	_txtUsed->setText(tr("STR_ALLOCATED_TO_PROJECT").arg(_planningProject->getEngineers().size()));
}

}
