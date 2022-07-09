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
#include "SoldiersState.h"
#include <climits>
#include "../Engine/Screen.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Mod/RuleSoldier.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Engine/Action.h"
#include "../Geoscape/AllocatePsiTrainingState.h"
#include "../Geoscape/AllocateTrainingState.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/CovertOperation.h"
#include "SoldierInfoState.h"
#include "SoldierMemorialState.h"
#include "SoldierTransformationState.h"
#include "SoldierTransformationListState.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Savegame/SavedBattleGame.h"
#include <algorithm>
#include "../Engine/Unicode.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldiers screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
SoldiersState::SoldiersState(Base *base) : _base(base), _origSoldierOrder(*_base->getSoldiers()), _dynGetter(NULL)
{
	bool isPsiBtnVisible = Options::anytimePsiTraining && _base->getAvailablePsiLabs() > 0;
	bool isTrnBtnVisible = _base->getAvailableTraining() > 0;
	_ftaUI = _game->getMod()->getIsFTAGame();
	std::vector<RuleSoldierTransformation* > availableTransformations;
	_game->getSavedGame()->getAvailableTransformations(availableTransformations, _game->getMod(), _base);
	bool isTransformationAvailable = availableTransformations.size() > 0;

	// if both training buttons would be displayed, or if there are any transformations, switch to combobox
	bool showCombobox = isTransformationAvailable || (isPsiBtnVisible && isTrnBtnVisible);
	if (_ftaUI)
	{
		showCombobox = true;
	}
	// 3 buttons or 2 buttons?
	bool showThreeButtons = !showCombobox && (isPsiBtnVisible || isTrnBtnVisible);

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	if (showThreeButtons)
	{
		_btnOk = new TextButton(96, 16, 216, 176);
		_btnMemorial = new TextButton(96, 16, 8, 176);
	}
	else
	{
		_btnOk = new TextButton(148, 16, 164, 176);
		_btnMemorial = new TextButton(148, 16, 8, 176);
	}
	_btnPsiTraining = new TextButton(96, 16, 112, 176);
	_btnTraining = new TextButton(96, 16, 112, 176);
	_cbxScreenActions = new ComboBox(this, 148, 16, 8, 176, true);
	_txtTitle = new Text(168, 17, 16, 8);
	_cbxSortBy = new ComboBox(this, 120, 16, 192, 8, false);
	_txtName = new Text(114, 9, 16, 32);
	_txtRank = new Text(102, 9, 122, 32);
	_txtCraft = new Text(82, 9, 220, 32);
	_lstSoldiers = new TextList(288, 128, 8, 40);

	// Set palette
	setInterface("soldierList");

	add(_window, "window", "soldierList");
	add(_btnOk, "button", "soldierList");
	add(_txtTitle, "text1", "soldierList");
	add(_txtName, "text2", "soldierList");
	add(_txtRank, "text2", "soldierList");
	add(_txtCraft, "text2", "soldierList");
	add(_lstSoldiers, "list", "soldierList");
	add(_cbxSortBy, "button", "soldierList");
	if (showCombobox)
	{
		add(_cbxScreenActions, "button", "soldierList");
	}
	else
	{
		add(_btnMemorial, "button", "soldierList");
		add(_btnPsiTraining, "button", "soldierList");
		add(_btnTraining, "button", "soldierList");
	}

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierList");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&SoldiersState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SoldiersState::btnOkClick, Options::keyCancel);
	//_btnOk->onKeyboardPress((ActionHandler)&SoldiersState::btnInventoryClick, Options::keyBattleInventory);

	_btnPsiTraining->setText(tr("STR_PSI_TRAINING"));
	_btnPsiTraining->onMouseClick((ActionHandler)&SoldiersState::btnPsiTrainingClick);
	_btnPsiTraining->setVisible(isPsiBtnVisible);

	_btnTraining->setText(tr("STR_TRAINING"));
	_btnTraining->onMouseClick((ActionHandler)&SoldiersState::btnTrainingClick);
	_btnTraining->setVisible(isTrnBtnVisible);

	_btnMemorial->setText(tr("STR_MEMORIAL"));
	_btnMemorial->onMouseClick((ActionHandler)&SoldiersState::btnMemorialClick);

	_availableOptions.clear();
	if (showCombobox)
	{
		_btnMemorial->setVisible(false);
		_btnPsiTraining->setVisible(false);
		_btnTraining->setVisible(false);

		if (_ftaUI)
		{
			_availableOptions.push_back("STR_PERSONNEL_INFO");
		}
		_availableOptions.push_back("STR_SOLDIER_INFO");
		if (_ftaUI)
		{
			_availableOptions.push_back("STR_PILOT_INFO");
			_availableOptions.push_back("STR_AGENT_INFO");
			_availableOptions.push_back("STR_SCIENTIST_INFO");
			_availableOptions.push_back("STR_ENGINEER_INFO");
		}
		_availableOptions.push_back("STR_MEMORIAL");

		if (isPsiBtnVisible)
			_availableOptions.push_back("STR_PSI_TRAINING");

		if (isTrnBtnVisible)
			_availableOptions.push_back("STR_TRAINING");

		if (isTransformationAvailable)
			_availableOptions.push_back("STR_TRANSFORMATIONS_OVERVIEW");

		bool refreshDeadSoldierStats = false;
		for (auto transformationRule : availableTransformations)
		{
			_availableOptions.push_back(transformationRule->getName());
			if (transformationRule->isAllowingDeadSoldiers())
			{
				refreshDeadSoldierStats = true;
			}
		}
		if (refreshDeadSoldierStats)
		{
			for (auto& deadMan : *_game->getSavedGame()->getDeadSoldiers())
			{
				deadMan->prepareStatsWithBonuses(_game->getMod()); // refresh stats for sorting
			}
		}

		_cbxScreenActions->setOptions(_availableOptions, true);
		_cbxScreenActions->setSelected(0);
		_cbxScreenActions->onChange((ActionHandler)&SoldiersState::cbxScreenActionsChange);
	}

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_LEFT);
	_txtTitle->setText(tr("STR_SOLDIER_LIST"));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setText(tr("STR_RANK"));

	if (_game->getMod()->getIsFTAGame())
	{
		_txtCraft->setText(tr("STR_ASSIGNMENT"));
	}
	else
	{
		_txtCraft->setText(tr("STR_CRAFT"));
	}

	// populate sort options
	std::vector<std::string> sortOptions;
	sortOptions.push_back(tr("STR_ORIGINAL_ORDER"));
	_sortFunctors.push_back(NULL);
	bool showPsiStats = true;
	if (_ftaUI)
	{
		showPsiStats = _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements());
	}

#define PUSH_IN(strId, functor) \
	sortOptions.push_back(tr(strId)); \
	_sortFunctors.push_back(new SortFunctor(_game, functor));

	PUSH_IN("STR_ID", idStat);
	PUSH_IN("STR_NAME_UC", nameStat);
	PUSH_IN("STR_SOLDIER_TYPE", typeStat);
	if (_ftaUI)
	{
		PUSH_IN("STR_ROLE_UC", roleStat);
		PUSH_IN("STR_RANK", roleRankStat);
	}
	else
	{
		PUSH_IN("STR_RANK", rankStat);
	}
	// FINNIKTODO-ROLES - add rank per roles
	PUSH_IN("STR_IDLE_DAYS", idleDaysStat);
	PUSH_IN("STR_MISSIONS2", missionsStat);
	PUSH_IN("STR_KILLS2", killsStat);
	PUSH_IN("STR_WOUND_RECOVERY2", woundRecoveryStat);
	if (_game->getMod()->isManaFeatureEnabled() && !_game->getMod()->getReplenishManaAfterMission() && showPsiStats)
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
	if (showPsiStats)
	{
		if (_game->getMod()->isManaFeatureEnabled())
		{
			// "unlock" is checked later
			PUSH_IN("STR_MANA_POOL", manaStat);
		}
		PUSH_IN("STR_PSIONIC_STRENGTH", psiStrengthStat);
		PUSH_IN("STR_PSIONIC_SKILL", psiSkillStat);
	}
	if (_ftaUI)
	{
		//pilot section
		PUSH_IN("STR_MANEUVERING", maneuveringStat);
		PUSH_IN("STR_MISSILE_OPERATION", missilesStat);
		PUSH_IN("STR_DOGFIGHT", dogfightStat);
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
		
		// scientist section
		PUSH_IN("STR_PHYSICS_UC", physicsStat);
		PUSH_IN("STR_CHEMISTRY_UC", chemistryStat);
		PUSH_IN("STR_BIOLOGY_UC", biologyStat);
		PUSH_IN("STR_INSIGHT_UC", insightStat);
		PUSH_IN("STR_DATA_ANALISIS_UC", dataStat);
		PUSH_IN("STR_COMPUTER_SCIENCE_UC", computersStat);
		PUSH_IN("STR_TACTICS_UC", tacticsStat);
		PUSH_IN("STR_MATERIAL_SCIENCE_UC", materialsStat);
		PUSH_IN("STR_PSYCHOLOGY_UC", psychologyStat);
		PUSH_IN("STR_DESIGNING_UC", designingStat);
		if (showPsiStats)
		{
			PUSH_IN("STR_PSIONICS_UC", psionicsStat);	
		}
		if (_game->getSavedGame()->isResearched(_game->getMod()->getXenologyUnlockResearch()))
		{
			PUSH_IN("STR_XENOLINGUISTICS_UC", xenolinguisticsStat);
		}
	}

#undef PUSH_IN

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&SoldiersState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_lstSoldiers->setColumns(3, 106, 98, 76);
	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)&SoldiersState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)&SoldiersState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)&SoldiersState::lstSoldiersClick);
	_lstSoldiers->onMousePress((ActionHandler)&SoldiersState::lstSoldiersMousePress);
}

/**
 * cleans up dynamic state
 */
SoldiersState::~SoldiersState()
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
void SoldiersState::cbxSortByChange(Action *action)
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
 * Updates the soldiers list
 * after going to other screens.
 */
void SoldiersState::init()
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
void SoldiersState::initList(size_t scrl)
{
	_lstSoldiers->clearList();

	_filteredListOfSoldiers.clear();
	_soldierNumbers.clear();
	int i = 0;

	std::string selAction = "STR_SOLDIER_INFO";
	if (_ftaUI)
	{
		std::string selAction = "STR_PERSONNEL_INFO";
	}
	
	if (!_availableOptions.empty())
	{
		selAction = _availableOptions.at(_cbxScreenActions->getSelected());
	}

	int offset = 0;
	if (selAction == "STR_PERSONNEL_INFO" ||
		selAction == "STR_SOLDIER_INFO" ||
		selAction == "STR_PILOT_INFO" ||
		selAction == "STR_AGENT_INFO" ||
		selAction == "STR_SCIENTIST_INFO" ||
		selAction == "STR_ENGINEER_INFO")
	{
		if (_ftaUI)
		{
			offset = 20;
			for (auto &soldier : *_base->getSoldiers())
			{
				if (selAction == "STR_PERSONNEL_INFO" ||
					(soldier->getRoleRank(ROLE_SOLDIER) > 0 && selAction == "STR_SOLDIER_INFO") ||
					(soldier->getRoleRank(ROLE_PILOT) > 0 && selAction == "STR_PILOT_INFO") ||
					(soldier->getRoleRank(ROLE_AGENT) > 0 && selAction == "STR_AGENT_INFO") ||
					(soldier->getRoleRank(ROLE_SCIENTIST) > 0 && selAction == "STR_SCIENTIST_INFO") ||
					(soldier->getRoleRank(ROLE_ENGINEER) > 0 && selAction == "STR_ENGINEER_INFO"))
				{
					_filteredListOfSoldiers.push_back(soldier);
					_soldierNumbers.push_back(i); // don't forget soldier's number on the base!
				}
				i++;
			}
		}
		else
		{
			_filteredListOfSoldiers = *_base->getSoldiers();
			_lstSoldiers->setArrowColumn(188, ARROW_VERTICAL);
		}
	}
	else
	{
		offset = 20;
		_lstSoldiers->setArrowColumn(-1, ARROW_VERTICAL);

		// filtered list of soldiers eligible for transformation
		RuleSoldierTransformation *transformationRule = _game->getMod()->getSoldierTransformation(selAction);
		if (transformationRule)
		{
			for (auto& soldier : *_base->getSoldiers())
			{
				if ((soldier->getCraft() && soldier->getCraft()->getStatus() == "STR_OUT") || soldier->getCovertOperation() != 0)
				{
					// soldiers outside of the base are not eligible
					continue;
				}
				if (soldier->isEligibleForTransformation(transformationRule))
				{
					_filteredListOfSoldiers.push_back(soldier);
					_soldierNumbers.push_back(i); // don't forget soldier's number on the base!
				}
				i++;
			}
			if (!_ftaUI) // sorry, we don't like necromancy!
			{
				for (auto &deadMan : *_game->getSavedGame()->getDeadSoldiers())
				{
					if (deadMan->isEligibleForTransformation(transformationRule))
					{
						_filteredListOfSoldiers.push_back(deadMan);
					}
				}
			}
		}
	}

	if (_dynGetter != NULL)
	{
		_lstSoldiers->setColumns(4, 106, 98 - offset, 60 + offset, 16);
	}
	else
	{
		_lstSoldiers->setColumns(3, 106, 98 - offset, 76 + offset);
	}
	_txtCraft->setX(_txtRank->getX() + 98 - offset);

	auto recovery = _base->getSumRecoveryPerDay();
	bool isBusy = false, isFree = false;
	unsigned int row = 0;
	for (std::vector<Soldier*>::iterator i = _filteredListOfSoldiers.begin(); i != _filteredListOfSoldiers.end(); ++i)
	{
		std::string duty = (*i)->getCurrentDuty(_game->getLanguage(), recovery, isBusy, isFree);
		if (_dynGetter != NULL)
		{
			// call corresponding getter
			int dynStat = (*_dynGetter)(_game, *i);
			std::ostringstream ss;
			ss << dynStat;
			_lstSoldiers->addRow(4, (*i)->getName(true).c_str(), tr((*i)->getRankString(_ftaUI)).c_str(), duty.c_str(), ss.str().c_str());
		}
		else
		{
			_lstSoldiers->addRow(3, (*i)->getName(true).c_str(), tr((*i)->getRankString(_ftaUI)).c_str(), duty.c_str());
		}

		if ((*i)->getCraft() == 0)
		{
			_lstSoldiers->setRowColor(row, _lstSoldiers->getSecondaryColor());
		}
		if ((*i)->getCovertOperation() != 0)
		{
			_lstSoldiers->setRowColor(row, _lstSoldiers->getColor());
		}
		if ((*i)->getDeath())
		{
			_lstSoldiers->setRowColor(row, _txtCraft->getColor());
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
void SoldiersState::lstItemsLeftArrowClick(Action *action)
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
void SoldiersState::moveSoldierUp(Action *action, unsigned int row, bool max)
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
void SoldiersState::lstItemsRightArrowClick(Action *action)
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
void SoldiersState::moveSoldierDown(Action *action, unsigned int row, bool max)
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
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens the Psionic Training screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnPsiTrainingClick(Action *)
{
	_game->pushState(new AllocatePsiTrainingState(_base));
}

/**
 * Opens the Martial Training screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnTrainingClick(Action *)
{
	_game->pushState(new AllocateTrainingState(_base));
}

/**
 * Opens the Memorial screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnMemorialClick(Action *)
{
	_game->pushState(new SoldierMemorialState);
}

/**
 * Opens the selected screen from the combo box
 * @param action Pointer to an action
 */
void SoldiersState::cbxScreenActionsChange(Action *action)
{
	const std::string selAction = _availableOptions.at(_cbxScreenActions->getSelected());

	if (selAction == "STR_MEMORIAL")
	{
		_cbxScreenActions->setSelected(0);
		_game->pushState(new SoldierMemorialState);
	}
	else if (selAction == "STR_PSI_TRAINING")
	{
		_cbxScreenActions->setSelected(0);
		_game->pushState(new AllocatePsiTrainingState(_base));
	}
	else if (selAction == "STR_TRAINING")
	{
		_cbxScreenActions->setSelected(0);
		_game->pushState(new AllocateTrainingState(_base));
	}
	else if (selAction == "STR_TRANSFORMATIONS_OVERVIEW")
	{
		_game->pushState(new SoldierTransformationListState(_base, _cbxScreenActions));
	}
	else
	{
		_cbxSortBy->setSelected(0);
		initList(0);
	}
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void SoldiersState::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}

	std::string selAction = "STR_SOLDIER_INFO";
	if (!_availableOptions.empty())
	{
		selAction = _availableOptions.at(_cbxScreenActions->getSelected());
	}
	if (selAction == "STR_PERSONNEL_INFO" ||
		selAction == "STR_SOLDIER_INFO" ||
		selAction == "STR_PILOT_INFO" ||
		selAction == "STR_AGENT_INFO" ||
		selAction == "STR_SCIENTIST_INFO" ||
		selAction == "STR_ENGINEER_INFO")
	{
		_game->pushState(new SoldierInfoState(_base, _soldierNumbers.at(_lstSoldiers->getSelectedRow())));
	}
	else
	{
		RuleSoldierTransformation *transformationRule = _game->getMod()->getSoldierTransformation(selAction);
		if (transformationRule)
		{
			_game->pushState(new SoldierTransformationState(
				transformationRule,
				_base,
				_filteredListOfSoldiers.at(_lstSoldiers->getSelectedRow()),
				&_filteredListOfSoldiers));
		}
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void SoldiersState::lstSoldiersMousePress(Action *action)
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

}
