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
#include <sstream>
#include <climits>
#include "PsiTrainingState.h"
#include "AllocatePsiTrainingState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Interface/TextList.h"
#include "../Savegame/Soldier.h"
#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Mod/Mod.h"
#include "../Basescape/SoldierSortUtil.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Initializes all the elements in the Psi Training screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to handle.
 */
AllocatePsiTrainingState::AllocatePsiTrainingState(Base *base) : _sel(0), _base(base), _origSoldierOrder(*_base->getSoldiers())
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 8);
	_txtRemaining = new Text(300, 10, 10, 24);
	_txtName = new Text(64, 10, 10, 40);
	_txtPsiStrength = new Text(80, 20, 124, 32);
	_txtPsiSkill = new Text(80, 20, 188, 32);
	_txtTraining = new Text(48, 20, 270, 32);
	_btnOk = new TextButton(148, 16, 164, 176);
	_lstSoldiers = new TextList(290, 112, 8, 52);
	_cbxSortBy = new ComboBox(this, 148, 16, 8, 176, true);

	// Set palette
	setInterface("allocatePsi");

	add(_window, "window", "allocatePsi");
	add(_btnOk, "button", "allocatePsi");
	add(_txtName, "text", "allocatePsi");
	add(_txtTitle, "text", "allocatePsi");
	add(_txtRemaining, "text", "allocatePsi");
	add(_txtPsiStrength, "text", "allocatePsi");
	add(_txtPsiSkill, "text", "allocatePsi");
	add(_txtTraining, "text", "allocatePsi");
	add(_lstSoldiers, "list", "allocatePsi");
	add(_cbxSortBy, "button", "allocatePsi");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&AllocatePsiTrainingState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&AllocatePsiTrainingState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PSIONIC_TRAINING"));

	_labSpace = base->getAvailablePsiLabs() - base->getUsedPsiLabs();
	_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));

	_txtName->setText(tr("STR_NAME"));

	_txtPsiStrength->setText(tr("STR_PSIONIC__STRENGTH"));

	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_IMPROVEMENT"));

	_txtTraining->setText(tr("STR_IN_TRAINING"));

	// populate sort options
	std::vector<std::wstring> sortOptions;
	sortOptions.push_back(tr("STR_ORIGINAL_ORDER"));
	_sortFunctors.push_back(NULL);

#define PUSH_IN(strId, functor) \
	sortOptions.push_back(tr(strId)); \
	_sortFunctors.push_back(new SortFunctor(_game, functor));

	PUSH_IN("STR_ID", idStat);
	PUSH_IN("STR_FIRST_LETTER", nameStat);
	PUSH_IN("STR_SOLDIER_TYPE", typeStat);
	PUSH_IN("STR_RANK", rankStat);
	PUSH_IN("STR_MISSIONS2", missionsStat);
	PUSH_IN("STR_KILLS2", killsStat);
	PUSH_IN("STR_WOUND_RECOVERY2", woundRecoveryStat);
	PUSH_IN("STR_TIME_UNITS", tuStat);
	PUSH_IN("STR_STAMINA", staminaStat);
	PUSH_IN("STR_HEALTH", healthStat);
	PUSH_IN("STR_BRAVERY", braveryStat);
	PUSH_IN("STR_REACTIONS", reactionsStat);
	PUSH_IN("STR_FIRING_ACCURACY", firingStat);
	PUSH_IN("STR_THROWING_ACCURACY", throwingStat);
	PUSH_IN("STR_MELEE_ACCURACY", meleeStat);
	PUSH_IN("STR_STRENGTH", strengthStat);
	PUSH_IN("STR_PSIONIC_STRENGTH", psiStrengthStat);
	PUSH_IN("STR_PSIONIC_SKILL", psiSkillStat);

#undef PUSH_IN

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&AllocatePsiTrainingState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
	_lstSoldiers->setArrowColumn(240, ARROW_VERTICAL);
	_lstSoldiers->setColumns(4, 114, 80, 62, 30);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(2);
	_lstSoldiers->onLeftArrowClick((ActionHandler)&AllocatePsiTrainingState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)&AllocatePsiTrainingState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)&AllocatePsiTrainingState::lstSoldiersClick);
}
/**
 * cleans up dynamic state
 */
AllocatePsiTrainingState::~AllocatePsiTrainingState()
{
	for (std::vector<SortFunctor *>::iterator it = _sortFunctors.begin(); it != _sortFunctors.end(); ++it)
	{
		delete(*it);
	}
}

/**
 * Sorts the soldiers list by the selected criterion
 * @param action Pointer to an action.
 */
void AllocatePsiTrainingState::cbxSortByChange(Action *action)
{
	size_t selIdx = _cbxSortBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}

	SortFunctor *compFunc = _sortFunctors[selIdx];
	if (compFunc)
	{
		std::stable_sort(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), *compFunc);
		bool shiftPressed = SDL_GetModState() & KMOD_SHIFT;
		if (shiftPressed)
		{
			std::reverse(_base->getSoldiers()->begin(), _base->getSoldiers()->end());
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
void AllocatePsiTrainingState::btnOkClick(Action *)
{
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		(*i)->calcStatString(_game->getMod()->getStatStrings(), (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements())));
	}
	_game->popState();
}

/**
 * Updates the soldiers list
 * after going to other screens.
 */
void AllocatePsiTrainingState::init()
{
	State::init();
	initList(0);
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void AllocatePsiTrainingState::initList(size_t scrl)
{
	int row = 0;
	_lstSoldiers->clearList();
	for (std::vector<Soldier*>::const_iterator s = _base->getSoldiers()->begin(); s != _base->getSoldiers()->end(); ++s)
	{
		std::wostringstream ssStr;
		std::wostringstream ssSkl;
		_soldiers.push_back(*s);
		if ((*s)->getCurrentStats()->psiSkill > 0 || (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements())))
		{
			ssStr << L"   " << (*s)->getCurrentStats()->psiStrength;
			if (Options::allowPsiStrengthImprovement) ssStr << "/+" << (*s)->getPsiStrImprovement();
		}
		else
		{
			ssStr << tr("STR_UNKNOWN");
		}
		if ((*s)->getCurrentStats()->psiSkill > 0)
		{
			ssSkl << (*s)->getCurrentStats()->psiSkill << "/+" << (*s)->getImprovement();
		}
		else
		{
			ssSkl << "0/+0";
		}
		if ((*s)->isInPsiTraining())
		{
			_lstSoldiers->addRow(4, (*s)->getName(true).c_str(), ssStr.str().c_str(), ssSkl.str().c_str(), tr("STR_YES").c_str());
			_lstSoldiers->setRowColor(row, _lstSoldiers->getSecondaryColor());
		}
		else
		{
			_lstSoldiers->addRow(4, (*s)->getName(true).c_str(), ssStr.str().c_str(), ssSkl.str().c_str(), tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(row, _lstSoldiers->getColor());
		}
		row++;
	}
	if (scrl)
		_lstSoldiers->scrollTo(scrl);
	_lstSoldiers->draw();
}


/**
 * Reorders a soldier up.
 * @param action Pointer to an action.
 */
void AllocatePsiTrainingState::lstItemsLeftArrowClick(Action *action)
{
	unsigned int row = _lstSoldiers->getSelectedRow();
	if (row > 0)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			moveSoldierUp(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			moveSoldierUp(action, row, true);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		{
			moveSoldierUp(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		{
			moveSoldierDown(action, row);
		}
	}
	_cbxSortBy->setText(tr("STR_SORT_BY"));
	_cbxSortBy->setSelected(-1);
}

/**
 * Moves a soldier up on the list.
 * @param action Pointer to an action.
 * @param row Selected soldier row.
 * @param max Move the soldier to the top?
 */
void AllocatePsiTrainingState::moveSoldierUp(Action *action, unsigned int row, bool max)
{
	Soldier *s = _base->getSoldiers()->at(row);
	if (max)
	{
		_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
		_base->getSoldiers()->insert(_base->getSoldiers()->begin(), s);
	}
	else
	{
		_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
		_base->getSoldiers()->at(row - 1) = s;
		if (row != _lstSoldiers->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() - static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstSoldiers->scrollUp(false);
		}
	}
	initList(_lstSoldiers->getScroll());
}

/**
 * Reorders a soldier down.
 * @param action Pointer to an action.
 */
void AllocatePsiTrainingState::lstItemsRightArrowClick(Action *action)
{
	unsigned int row = _lstSoldiers->getSelectedRow();
	size_t numSoldiers = _base->getSoldiers()->size();
	if (0 < numSoldiers && INT_MAX >= numSoldiers && row < numSoldiers - 1)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			moveSoldierDown(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			moveSoldierDown(action, row, true);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		{
			moveSoldierUp(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		{
			moveSoldierDown(action, row);
		}
	}
	_cbxSortBy->setText(tr("STR_SORT_BY"));
	_cbxSortBy->setSelected(-1);
}

/**
 * Moves a soldier down on the list.
 * @param action Pointer to an action.
 * @param row Selected soldier row.
 * @param max Move the soldier to the bottom?
 */
void AllocatePsiTrainingState::moveSoldierDown(Action *action, unsigned int row, bool max)
{
	Soldier *s = _base->getSoldiers()->at(row);
	if (max)
	{
		_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
		_base->getSoldiers()->insert(_base->getSoldiers()->end(), s);
	}
	else
	{
		_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
		_base->getSoldiers()->at(row + 1) = s;
		if (row != _lstSoldiers->getVisibleRows() - 1 + _lstSoldiers->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() + static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstSoldiers->scrollDown(false);
		}
	}
	initList(_lstSoldiers->getScroll());
}

/**
 * Assigns / removes a soldier from Psi Training.
 * @param action Pointer to an action.
 */
void AllocatePsiTrainingState::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}

	_sel = _lstSoldiers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (!_base->getSoldiers()->at(_sel)->isInPsiTraining())
		{
			if (_base->getUsedPsiLabs() < _base->getAvailablePsiLabs())
			{
				_lstSoldiers->setCellText(_sel, 3, tr("STR_YES"));
				_lstSoldiers->setRowColor(_sel, _lstSoldiers->getSecondaryColor());
				_labSpace--;
				_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));
				_base->getSoldiers()->at(_sel)->setPsiTraining();
			}
		}
		else
		{
			_lstSoldiers->setCellText(_sel, 3, tr("STR_NO"));
			_lstSoldiers->setRowColor(_sel, _lstSoldiers->getColor());
			_labSpace++;
			_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));
			_base->getSoldiers()->at(_sel)->setPsiTraining();
		}
	}
}

}
