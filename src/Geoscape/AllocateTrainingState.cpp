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
#include <sstream>
#include <climits>
#include "TrainingState.h"
#include "AllocateTrainingState.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Mod/RuleInterface.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
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
#include "../Engine/Unicode.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Psi Training screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to handle.
 */
AllocateTrainingState::AllocateTrainingState(Base *base) : _sel(0), _base(base), _origSoldierOrder(*_base->getSoldiers())
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 8);
	_txtRemaining = new Text(300, 10, 10, 24);
	_txtName = new Text(64, 10, 10, 40);
	_txtTraining = new Text(48, 20, 270, 32);
	_btnOk = new TextButton(148, 16, 164, 176);
	_lstSoldiers = new TextList(290, 112, 8, 52);
	_txtTu = new Text(18, 10, 120, 40);
	_txtStamina = new Text(18, 10, 138, 40);
	_txtHealth = new Text(18, 10, 156, 40);
	_txtFiring = new Text(18, 10, 174, 40);
	_txtThrowing = new Text(18, 10, 192, 40);
	_txtMelee = new Text(18, 10, 210, 40);
	_txtStrength = new Text(18, 10, 228, 40);
	_cbxSortBy = new ComboBox(this, 148, 16, 8, 176, true);
	_btnPlus = new ToggleTextButton(18, 16, 294, 8);

	// Set palette
	setInterface("allocateMartial");

	add(_window, "window", "allocateMartial");
	add(_btnOk, "button", "allocateMartial");
	add(_txtName, "text", "allocateMartial");
	add(_txtTitle, "text", "allocateMartial");
	add(_txtRemaining, "text", "allocateMartial");
	add(_txtTraining, "text", "allocateMartial");
	add(_lstSoldiers, "list", "allocateMartial");
	add(_txtTu, "text", "allocateMartial");
	add(_txtStamina, "text", "allocateMartial");
	add(_txtHealth, "text", "allocateMartial");
	add(_txtFiring, "text", "allocateMartial");
	add(_txtThrowing, "text", "allocateMartial");
	add(_txtMelee, "text", "allocateMartial");
	add(_txtStrength, "text", "allocateMartial");
	add(_cbxSortBy, "button", "allocateMartial");
	add(_btnPlus, "button", "allocateMartial");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "allocateMartial");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&AllocateTrainingState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&AllocateTrainingState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)& AllocateTrainingState::btnDeassignAllSoldiersClick, Options::keyRemoveSoldiersFromTraining);
	_btnOk->onKeyboardPress((ActionHandler)&AllocateTrainingState::btnAssignAllSoldiersClick, Options::keyAddSoldiersToTraining);

	_btnPlus->setText("+");
	_btnPlus->setPressed(false);
	if (_game->getMod()->getSoldierBonusList().empty())
	{
		// no soldier bonuses in the mod = button not needed
		_btnPlus->setVisible(false);
	}
	else
	{
		_btnPlus->onMouseClick((ActionHandler)&AllocateTrainingState::btnPlusClick, 0);
	}

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PHYSICAL_TRAINING"));

	_space = base->getAvailableTraining() - base->getUsedTraining();
	_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));

	_txtName->setText(tr("STR_NAME"));
	_txtTu->setText(tr("STR_TIME_UNITS_ABBREVIATION"));
	_txtStamina->setText(tr("STR_STAMINA_ABBREVIATION"));
	_txtHealth->setText(tr("STR_HEALTH_ABBREVIATION"));
	_txtFiring->setText(tr("STR_FIRING_ACCURACY_ABBREVIATION"));
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY_ABBREVIATION"));
	_txtMelee->setText(tr("STR_MELEE_ACCURACY_ABBREVIATION"));
	_txtStrength->setText(tr("STR_STRENGTH_ABBREVIATION"));
	_txtTraining->setText(tr("STR_IN_TRAINING"));

	// populate sort options
	std::vector<std::string> sortOptions;
	sortOptions.push_back(tr("STR_ORIGINAL_ORDER"));
	_sortFunctors.push_back(NULL);
	_sortFunctorsPlus.push_back(NULL);

#define PUSH_IN(strId, functor) \
	sortOptions.push_back(tr(strId)); \
	_sortFunctors.push_back(new SortFunctor(_game, functor)); \
	_sortFunctorsPlus.push_back(new SortFunctor(_game, functor));

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

#undef PUSH_IN

#define PUSH_IN(strId, functor, functorPlus) \
	sortOptions.push_back(tr(strId)); \
	_sortFunctors.push_back(new SortFunctor(_game, functor)); \
	_sortFunctorsPlus.push_back(new SortFunctor(_game, functorPlus));

	PUSH_IN("STR_TIME_UNITS", tuStatBase, tuStatPlus);
	PUSH_IN("STR_STAMINA", staminaStatBase, staminaStatPlus);
	PUSH_IN("STR_HEALTH", healthStatBase, healthStatPlus);
	PUSH_IN("STR_BRAVERY", braveryStatBase, braveryStatPlus);
	PUSH_IN("STR_REACTIONS", reactionsStatBase, reactionsStatPlus);
	PUSH_IN("STR_FIRING_ACCURACY", firingStatBase, firingStatPlus);
	PUSH_IN("STR_THROWING_ACCURACY", throwingStatBase, throwingStatPlus);
	PUSH_IN("STR_MELEE_ACCURACY", meleeStatBase, meleeStatPlus);
	PUSH_IN("STR_STRENGTH", strengthStatBase, strengthStatPlus);
	if (_game->getMod()->isManaFeatureEnabled())
	{
		// "unlock" is checked later
		PUSH_IN("STR_MANA_POOL", manaStatBase, manaStatPlus);
	}
	PUSH_IN("STR_PSIONIC_STRENGTH", psiStrengthStatBase, psiStrengthStatPlus);
	PUSH_IN("STR_PSIONIC_SKILL", psiSkillStatBase, psiSkillStatPlus);

#undef PUSH_IN

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&AllocateTrainingState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_lstSoldiers->setArrowColumn(238, ARROW_VERTICAL);
	_lstSoldiers->setColumns(9, 110, 18, 18, 18, 18, 18, 18, 42, 40);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(2);
	_lstSoldiers->onLeftArrowClick((ActionHandler)&AllocateTrainingState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)&AllocateTrainingState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)&AllocateTrainingState::lstSoldiersClick);
	_lstSoldiers->onMousePress((ActionHandler)&AllocateTrainingState::lstSoldiersMousePress);
}

/**
 * cleans up dynamic state
 */
AllocateTrainingState::~AllocateTrainingState()
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
void AllocateTrainingState::cbxSortByChange(Action *action)
{
	size_t selIdx = _cbxSortBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}

	SortFunctor *compFunc = _btnPlus->getPressed() ? _sortFunctorsPlus[selIdx] : _sortFunctors[selIdx];
	if (compFunc)
	{
		if (selIdx == 2)
		{
			std::stable_sort(_base->getSoldiers()->begin(), _base->getSoldiers()->end(),
				[](const Soldier* a, const Soldier* b)
				{
					return Unicode::naturalCompare(a->getName(), b->getName());
				}
			);
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
void AllocateTrainingState::btnOkClick(Action *)
{
	// Note: statString updates not necessary
	_game->popState();
}

/**
 * Updates the soldier stats. Sorts the list if applicable.
 * @param action Pointer to an action.
 */
void AllocateTrainingState::btnPlusClick(Action *action)
{
	size_t selIdx = _cbxSortBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		size_t originalScrollPos = _lstSoldiers->getScroll();
		initList(originalScrollPos);
	}
	else
	{
		cbxSortByChange(action);
	}
}

/**
 * The soldier info could maybe change (armor? something else?)
 * after going into other screens.
 */
void AllocateTrainingState::init()
{
	State::init();
	_base->prepareSoldierStatsWithBonuses(); // refresh stats for sorting
	initList(0);
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void AllocateTrainingState::initList(size_t scrl)
{
	int row = 0;
	_lstSoldiers->clearList();
	for (std::vector<Soldier*>::const_iterator s = _base->getSoldiers()->begin(); s != _base->getSoldiers()->end(); ++s)
	{
		UnitStats* stats = _btnPlus->getPressed() ? (*s)->getStatsWithSoldierBonusesOnly() : (*s)->getCurrentStats();

		std::ostringstream tu;
		tu << stats->tu;
		std::ostringstream stamina;
		stamina << stats->stamina;
		std::ostringstream health;
		health << stats->health;
		std::ostringstream firing;
		firing << stats->firing;
		std::ostringstream throwing;
		throwing << stats->throwing;
		std::ostringstream melee;
		melee << stats->melee;
		std::ostringstream strength;
		strength << stats->strength;

		bool isDone = (*s)->isFullyTrained();
		bool isWounded = (*s)->isWounded();
		bool isQueued = isWounded && (*s)->getReturnToTrainingWhenHealed();
		bool isTraining = (*s)->isInTraining();

		std::string status;
		if (isDone)
			status = tr("STR_NO_DONE");
		else if (isQueued)
			status = tr("STR_NO_QUEUED");
		else if (isWounded)
			status = tr("STR_NO_WOUNDED");
		else if (isTraining)
			status = tr("STR_YES");
		else
			status = tr("STR_NO");

		_lstSoldiers->addRow(9,
			(*s)->getName(true).c_str(),
			tu.str().c_str(),
			stamina.str().c_str(),
			health.str().c_str(),
			firing.str().c_str(),
			throwing.str().c_str(),
			melee.str().c_str(),
			strength.str().c_str(),
			status.c_str());
		_lstSoldiers->setRowColor(row, isTraining ? _lstSoldiers->getSecondaryColor() : _lstSoldiers->getColor());
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
void AllocateTrainingState::lstItemsLeftArrowClick(Action *action)
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
void AllocateTrainingState::moveSoldierUp(Action *action, unsigned int row, bool max)
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
void AllocateTrainingState::lstItemsRightArrowClick(Action *action)
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
void AllocateTrainingState::moveSoldierDown(Action *action, unsigned int row, bool max)
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
void AllocateTrainingState::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}

	_sel = _lstSoldiers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		auto soldier = _base->getSoldiers()->at(_sel);

		// can't put fully trained soldiers back into training
		if (soldier->isFullyTrained()) return;

		// wounded soldiers can be queued/dequeued
		if (soldier->isWounded())
		{
			if (soldier->getReturnToTrainingWhenHealed())
			{
				_lstSoldiers->setCellText(_sel, 8, tr("STR_NO_WOUNDED").c_str());
				soldier->setReturnToTrainingWhenHealed(false);
			}
			else
			{
				_lstSoldiers->setCellText(_sel, 8, tr("STR_NO_QUEUED").c_str());
				soldier->setReturnToTrainingWhenHealed(true);
			}
			return;
		}

		// healthy soldiers can be assigned/deassigned
		if (!soldier->isInTraining())
		{
			if (_base->getUsedTraining() < _base->getAvailableTraining())
			{
				_lstSoldiers->setCellText(_sel, 8, tr("STR_YES").c_str());
				_lstSoldiers->setRowColor(_sel, _lstSoldiers->getSecondaryColor());
				_space--;
				_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));
				soldier->setTraining(true);
			}
		}
		else
		{
			_lstSoldiers->setCellText(_sel, 8, tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(_sel, _lstSoldiers->getColor());
			_space++;
			_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));
			soldier->setTraining(false);
		}
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void AllocateTrainingState::lstSoldiersMousePress(Action *action)
{
	if (Options::changeValueByMouseWheel == 0)
		return;
	unsigned int row = _lstSoldiers->getSelectedRow();
	size_t numSoldiers = _base->getSoldiers()->size();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP &&
		row > 0)
	{
		if (action->getAbsoluteXMouse() >= _lstSoldiers->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstSoldiers->getArrowsRightEdge())
		{
			moveSoldierUp(action, row);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN &&
		0 < numSoldiers && INT_MAX >= numSoldiers && row < numSoldiers - 1)
	{
		if (action->getAbsoluteXMouse() >= _lstSoldiers->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstSoldiers->getArrowsRightEdge())
		{
			moveSoldierDown(action, row);
		}
	}
}

/**
 * Removes all soldiers from Martial Training.
 * @param action Pointer to an action.
 */
void AllocateTrainingState::btnDeassignAllSoldiersClick(Action* action)
{
	int row = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		(*i)->setTraining(false);
		(*i)->setReturnToTrainingWhenHealed(false);

		std::string status;
		if ((*i)->isFullyTrained())
			status = tr("STR_NO_DONE");
		else if ((*i)->isWounded())
			status = tr("STR_NO_WOUNDED");
		else
			status = tr("STR_NO");

		_lstSoldiers->setCellText(row, 8, tr(status).c_str());
		_lstSoldiers->setRowColor(row, _lstSoldiers->getColor());
		row++;
	}
	_space = _base->getAvailableTraining() - _base->getUsedTraining();
	_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));
}

/**
 * Adds all soldiers to Martial Training.
 * @param action Pointer to an action.
 */
void AllocateTrainingState::btnAssignAllSoldiersClick(Action* action)
{
	int row = 0;
	for (auto* soldier : *_base->getSoldiers())
	{
		if (soldier->isFullyTrained())
		{
			// can't put fully trained soldiers back into training
		}
		else if (soldier->isWounded())
		{
			// wounded soldiers can be queued
			if (!soldier->getReturnToTrainingWhenHealed())
			{
				_lstSoldiers->setCellText(row, 8, tr("STR_NO_QUEUED").c_str());
				soldier->setReturnToTrainingWhenHealed(true);
			}
		}
		else if (_space > 0 && !soldier->isInTraining())
		{
			// healthy soldiers can be assigned
			_lstSoldiers->setCellText(row, 8, tr("STR_YES").c_str());
			_lstSoldiers->setRowColor(row, _lstSoldiers->getSecondaryColor());
			_space--;
			soldier->setTraining(true);
		}
		row++;
	}
	_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));
}

}
