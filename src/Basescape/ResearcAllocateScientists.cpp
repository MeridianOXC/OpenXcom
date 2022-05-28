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
#include "../Mod/RuleCovertOperation.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleSoldier.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "ResearcAllocateScientists.h"
#include "SoldierInfoState.h"
#include <algorithm>
#include <climits>

namespace OpenXcom
{

/**
 * Initializes all the elements in the CovertOperation Soldiers screen.
 * @param base Pointer to the base to get info from.
 * @param operation Pointer to starting (not comitted) covert operation.
 */
ResearcAllocateScientists::ResearcAllocateScientists(Base *base, CovertOperationStartState *operation)
	: _base(base), _operation(operation), _otherCraftColor(0), _origSoldierOrder(*_base->getSoldiers()), _dynGetter(NULL)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(300, 17, 16, 7);
	_txtName = new Text(114, 9, 16, 32);
	_txtRank = new Text(102, 9, 122, 32);
	_txtCraft = new Text(84, 9, 220, 32);
	_txtAvailable = new Text(99, 9, 16, 24);
	_txtUsed = new Text(95, 9, 122, 24);
	_txtChances = new Text(110, 9, 205, 24);
	_cbxSortBy = new ComboBox(this, 148, 16, 8, 176, true);
	_lstSoldiers = new TextList(288, 128, 8, 40);

	// Set palette
	setInterface("craftSoldiers");

	add(_window, "window", "craftSoldiers");
	add(_btnOk, "button", "craftSoldiers");
	add(_txtTitle, "text", "craftSoldiers");
	add(_txtName, "text", "craftSoldiers");
	add(_txtRank, "text", "craftSoldiers");
	add(_txtCraft, "text", "craftSoldiers");
	add(_txtAvailable, "text", "craftSoldiers");
	add(_txtUsed, "text", "craftSoldiers");
	add(_txtChances, "text", "craftSoldiers");
	add(_lstSoldiers, "list", "craftSoldiers");
	add(_cbxSortBy, "button", "craftSoldiers");

	_otherCraftColor = _game->getMod()->getInterface("craftSoldiers")->getElement("otherCraft")->color;

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftSoldiers");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ResearcAllocateScientists::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ResearcAllocateScientists::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&ResearcAllocateScientists::btnDeassignCraftSoldiersClick, Options::keyRemoveSoldiersFromCraft);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_SQUAD_FOR_OPERATION").arg(tr(operation->getRule()->getName())));
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
	_cbxSortBy->onChange((ActionHandler)&CovertOperationSoldiersState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_lstSoldiers->setColumns(3, 106, 98, 76);
	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onMouseClick((ActionHandler)&CovertOperationSoldiersState::lstSoldiersClick, 0);
}

/**
 * cleans up dynamic state
 */
ResearcAllocateScientists::~ResearcAllocateScientists()
{
	for (std::vector<SortFunctor *>::iterator it = _sortFunctors.begin();
		 it != _sortFunctors.end(); ++it)
	{
		delete (*it);
	}
}

/**
 * Sorts the soldiers list by the selected criterion
 * @param action Pointer to an action.
 */
void ResearcAllocateScientists::cbxSortByChange(Action *)
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
		for (std::vector<Soldier *>::const_iterator it = _origSoldierOrder.begin();
			 it != _origSoldierOrder.end(); ++it)
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

	size_t originalScrollPos = _lstSoldiers->getScroll();
	initList(originalScrollPos);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ResearcAllocateScientists::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void ResearcAllocateScientists::initList(size_t scrl)
{
	int row = 0;
	_lstSoldiers->clearList();

	if (_dynGetter != NULL)
	{
		_lstSoldiers->setColumns(4, 106, 98, 60, 16);
	}
	else
	{
		_lstSoldiers->setColumns(3, 106, 98, 76);
	}

	auto recovery = _base->getSumRecoveryPerDay();
	bool isBusy = false, isFree = false;
	for (std::vector<Soldier *>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getRoleRank(ROLE_SOLDIER) > 0) // let's make it simple for now
		{
			std::string duty = (*i)->getCurrentDuty(_game->getLanguage(), recovery, isBusy, isFree);
			if (_dynGetter != NULL)
			{
				// call corresponding getter
				int dynStat = (*_dynGetter)(_game, *i);
				std::ostringstream ss;
				ss << dynStat;
				_lstSoldiers->addRow(4, (*i)->getName(true, 19).c_str(), tr((*i)->getRankString(true)).c_str(), duty.c_str(), ss.str().c_str());
			}
			else
			{
				_lstSoldiers->addRow(3, (*i)->getName(true, 19).c_str(), tr((*i)->getRankString(true)).c_str(), duty.c_str());
			}

			Uint8 color;
			auto opSoldiers = _operation->getSoldiers();
			bool matched = false;

			auto iter = std::find(std::begin(opSoldiers), std::end(opSoldiers), (*i));
			if (iter != std::end(opSoldiers))
			{
				matched = true;
			}

			if (matched)
			{
				color = _lstSoldiers->getSecondaryColor();
				_lstSoldiers->setCellText(row, 2, tr("STR_ASSIGNED_UC"));
			}
			else if (isBusy || !isFree)
			{
				color = _otherCraftColor;
			}
			else
			{
				color = _lstSoldiers->getColor();
			}
			_lstSoldiers->setRowColor(row, color);
			row++;
		}
	}
	if (scrl)
		_lstSoldiers->scrollTo(scrl);
	_lstSoldiers->draw();

	if (_operation->getRule()->getOptionalSoldierSlots() > 0)
	{
		std::ostringstream ss;
		ss << _operation->getRule()->getSoldierSlots() << "+" << _operation->getRule()->getOptionalSoldierSlots();
		_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(ss.str()));
	}
	else
	{
		_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(_operation->getRule()->getSoldierSlots()));
	}

	_txtUsed->setText(tr("STR_SPACE_USED").arg(_operation->getSoldiers().size()));
	bool mod = _game->getSavedGame()->getDebugMode();
	_txtChances->setText(tr("STR_OPERATION_CHANCES_US").arg(tr(_operation->getOperationOddsString(mod))));
}

/**
 * Shows the soldiers in a list.
 */
void ResearcAllocateScientists::init()
{
	State::init();
	_base->prepareSoldierStatsWithBonuses(); // refresh stats for sorting
	initList(0);
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void ResearcAllocateScientists::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}
	int row = _lstSoldiers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Soldier *s = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());
		auto assigned = _operation->getSoldiers();
		Uint8 color;

		auto opSoldiers = _operation->getSoldiers();
		bool matched = false;
		auto iter = std::find(std::begin(opSoldiers), std::end(opSoldiers), s);
		bool isBusy = false, isFree = false;
		std::string duty = s->getCurrentDuty(_game->getLanguage(), _base->getSumRecoveryPerDay(), isBusy, isFree);

		if (iter != std::end(opSoldiers))
		{
			matched = true;
		}
		if (matched)
		{
			_operation->removeSoldier(s);

			_lstSoldiers->setCellText(row, 2, duty);
			if (s->getCraft() == 0)
			{
				color = _lstSoldiers->getColor();
			}
			else
			{
				color = _otherCraftColor;
			}
		}
		else if (isBusy || !isFree)
		{
			color = _otherCraftColor;
		}
		else if (s->hasFullHealth() && !isBusy)
		{
			int space = (_operation->getRule()->getSoldierSlots() + _operation->getRule()->getOptionalSoldierSlots()) - opSoldiers.size();
			int armorSize = s->getArmor()->getSize();
			if (space > 0)
			{
				_operation->addSoldier(s);
				_lstSoldiers->setCellText(row, 2, tr("STR_ASSIGNED_UC"));
				color = _lstSoldiers->getSecondaryColor();
			}
			else
			{
				if (s->getCraft() == 0)
				{
					color = _lstSoldiers->getColor();
				}
				else
				{
					color = _otherCraftColor;
				}
			}
		}
		else
		{
			color = _lstSoldiers->getColor();
		}
		_lstSoldiers->setRowColor(row, color);

		_txtUsed->setText(tr("STR_SPACE_USED").arg(_operation->getSoldiers().size()));
		bool mod = _game->getSavedGame()->getDebugMode();
		_txtChances->setText(tr("STR_OPERATION_CHANCES_US").arg(tr(_operation->getOperationOddsString(mod))));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->pushState(new SoldierInfoState(_base, row));
	}
}

/**
 * De-assign all soldiers from the current craft.
 * @param action Pointer to an action.
 */
void ResearcAllocateScientists::btnDeassignCraftSoldiersClick(Action *action)
{
	// Craft* c = _base->getCrafts()->at(0); //#FINNIKTODO
	// int row = 0;
	// for (auto s : *_base->getSoldiers())
	//{
	//	if (s->getCraft() == c)
	//	{
	//		s->setCraft(0);
	//		_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
	//		_lstSoldiers->setRowColor(row, _lstSoldiers->getColor());
	//	}
	//	row++;
	// }

	//_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	//_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

}
