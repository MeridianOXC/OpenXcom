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
#include "CraftSoldiersState.h"
#include <algorithm>
#include <climits>
#include <algorithm>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Menu/ErrorMessageState.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "SoldierInfoState.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleSoldier.h"
#include "../Engine/Unicode.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Soldiers screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftSoldiersState::CraftSoldiersState(Base *base, size_t craft)
		:  _base(base), _craft(craft), _otherCraftColor(0), _origSoldierOrder(*_base->getSoldiers()), _dynGetter(NULL)
{
	Craft *c = _base->getCrafts()->at(_craft);
	bool hidePreview = _game->getSavedGame()->getMonthsPassed() == -1 || !c->getRules()->getAllowLanding();
	int pilots = c->getRules()->getPilots();
	_isInterceptor = pilots > 0 && !c->getRules()->getAllowLanding();
	_isMultipurpose = pilots > 0 && c->getRules()->getAllowLanding() && pilots < c->getSpaceAvailable();
	_ftaUI = _game->getMod()->getIsFTAGame();

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(hidePreview ? 148 : 38, 16, hidePreview ? 164 : 274, 176);
	_btnPreview = new TextButton(102, 16, 164, 176);
	_txtTitle = new Text(_ftaUI ? 300 : 168, 17, 16, 7);
	_txtName = new Text(114, 9, 16, 32);
	_txtRank = new Text(102, 9, 122, 32);
	_txtCraft = new Text(84, 9, 220, 32);
	_txtAvailable = new Text(110, 9, 16, 24);
	_txtUsed = new Text(110, 9, 122, 24);
	if (_ftaUI)
	{
		_cbxSortBy = new ComboBox(this, 120, 16, 192, 8, false);
		_cbxScreenActions = new ComboBox(this, 148, 16, 8, 176, true);
	}
	else
	{
		_cbxSortBy = new ComboBox(this, 148, 16, 8, 176, true);
		_cbxScreenActions = new ComboBox(this, 0, 0, 1, 1, true); //would be hidden anyway
	}
	_lstSoldiers = new TextList(288, 128, 8, 40);

	// Set palette
	setInterface("craftSoldiers");

	add(_window, "window", "craftSoldiers");
	add(_btnOk, "button", "craftSoldiers");
	add(_btnPreview, "button", "craftSoldiers");
	add(_txtTitle, "text", "craftSoldiers");
	add(_txtName, "text", "craftSoldiers");
	add(_txtRank, "text", "craftSoldiers");
	add(_txtCraft, "text", "craftSoldiers");
	add(_txtAvailable, "text", "craftSoldiers");
	add(_txtUsed, "text", "craftSoldiers");
	add(_lstSoldiers, "list", "craftSoldiers");
	add(_cbxSortBy, "button", "craftSoldiers");
	add(_cbxScreenActions, "button", "craftSoldiers");

	_otherCraftColor = _game->getMod()->getInterface("craftSoldiers")->getElement("otherCraft")->color;

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftSoldiers");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftSoldiersState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftSoldiersState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&CraftSoldiersState::btnDeassignAllSoldiersClick, Options::keyRemoveSoldiersFromAllCrafts);
	_btnOk->onKeyboardPress((ActionHandler)&CraftSoldiersState::btnDeassignCraftSoldiersClick, Options::keyRemoveSoldiersFromCraft);

	_btnPreview->setText(tr("STR_CRAFT_DEPLOYMENT_PREVIEW"));
	_btnPreview->setVisible(!hidePreview);
	_btnPreview->onMouseClick((ActionHandler)&CraftSoldiersState::btnPreviewClick);

	_txtTitle->setBig();
	_txtTitle->setText(_ftaUI ? tr("STR_SELECT_SQUAD_UC") : tr("STR_SELECT_SQUAD_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

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
	if (_ftaUI && _isInterceptor)
	{
		// pilot section
		PUSH_IN("STR_MANEUVERING", maneuveringStat);
		PUSH_IN("STR_MISSILE_OPERATION", missilesStat);
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
	}
	else
	{
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
	}
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

#undef PUSH_IN

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&CraftSoldiersState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_availableOptions.clear();
	if (_ftaUI)
	{
		_availableOptions.push_back("STR_ALL_ROLES");
		_availableOptions.push_back("STR_RECOMMENDED_ROLES");
	}
	else
	{
		_cbxScreenActions->setVisible(false);
	}
	_cbxScreenActions->setOptions(_availableOptions, true);
	_cbxScreenActions->setSelected(1);
	_cbxScreenActions->onChange((ActionHandler)&CraftSoldiersState::cbxScreenActionsChange);

	_lstSoldiers->setArrowColumn(188, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 106, 98, 76);
	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)&CraftSoldiersState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)&CraftSoldiersState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)&CraftSoldiersState::lstSoldiersClick, 0);
	_lstSoldiers->onMousePress((ActionHandler)&CraftSoldiersState::lstSoldiersMousePress);
}

/**
 * cleans up dynamic state
 */
CraftSoldiersState::~CraftSoldiersState()
{
	for (std::vector<SortFunctor *>::iterator it = _sortFunctors.begin();
		it != _sortFunctors.end(); ++it)
	{
		delete(*it);
	}
}

/**
 * Sorts the soldiers list by the selected criterion
 * @param action Pointer to an action.
 */
void CraftSoldiersState::cbxSortByChange(Action *)
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
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Shows the battlescape preview.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::btnPreviewClick(Action *)
{
	Craft* c = _base->getCrafts()->at(_craft);
	if (c->getSpaceUsed() <= 0)
	{
		// at least one unit must be onboard
		return;
	}

	SavedBattleGame* bgame = new SavedBattleGame(_game->getMod(), _game->getLanguage(), true);
	_game->getSavedGame()->setBattleGame(bgame);
	BattlescapeGenerator bgen = BattlescapeGenerator(_game);
	bgame->setMissionType(c->getRules()->getCustomPreviewType());
	bgame->setCraftForPreview(c);
	bgen.setCraft(c);
	bgen.run();

	// needed for preview of craft deployment tiles
	bgame->setCraftPos(bgen.getCraftPos());
	bgame->setCraftZ(bgen.getCraftZ());
	bgame->calculateCraftTiles();

	_game->pushState(new BriefingState(c));
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void CraftSoldiersState::initList(size_t scrl)
{
	int row = 0;
	_soldierNumbers.clear();
	_lstSoldiers->clearList();

	std::string selAction = "STR_RECOMMENDED_ROLES";
	if (!_availableOptions.empty())
	{
		selAction = _availableOptions.at(_cbxScreenActions->getSelected());
	}

	if (_dynGetter != NULL)
	{
		_lstSoldiers->setColumns(4, 106, 98, 60, 16);
	}
	else
	{
		_lstSoldiers->setColumns(3, 106, 98, 76);
	}

	Craft *c = _base->getCrafts()->at(_craft);
	auto recovery = _base->getSumRecoveryPerDay();
	bool isBusy = false, isFree = false;
	int it = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if (((*i)->getRoleRank(ROLE_SOLDIER) > 0 && !_isInterceptor) //case for dropship
			|| (_isInterceptor && (*i)->getRoleRank(ROLE_PILOT) > 0) //case for interceptor
			|| (_isMultipurpose && ((*i)->getRoleRank(ROLE_PILOT) > 0 || (*i)->getRoleRank(ROLE_SOLDIER) > 0)) //case for multipurpose craft
			|| selAction == "STR_ALL_ROLES" //case we wank to see everyone
			|| !_ftaUI)
		{
			_soldierNumbers.push_back(it); // don't forget soldier's number on the base!
			std::string duty = (*i)->getCurrentDuty(_game->getLanguage(), recovery, isBusy, isFree);
			if (_dynGetter != NULL)
			{
				// call corresponding getter
				int dynStat = (*_dynGetter)(_game, *i);
				std::ostringstream ss;
				ss << dynStat;
				_lstSoldiers->addRow(4, (*i)->getName(true, 19).c_str(), tr((*i)->getRankString(_ftaUI)).c_str(), duty.c_str(), ss.str().c_str());
			}
			else
			{
				_lstSoldiers->addRow(3, (*i)->getName(true, 19).c_str(), tr((*i)->getRankString(_ftaUI)).c_str(), duty.c_str());
			}

			Uint8 color;
			if ((*i)->getCraft() == c)
			{
				color = _lstSoldiers->getSecondaryColor();
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
		it++;
	}
	if (scrl)
		_lstSoldiers->scrollTo(scrl);
	_lstSoldiers->draw();

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

/**
 * Shows the soldiers in a list.
 */
void CraftSoldiersState::init()
{
	State::init();
	_base->prepareSoldierStatsWithBonuses(); // refresh stats for sorting
	initList(0);

	// update the label to indicate presence of a saved craft deployment
	Craft* c = _base->getCrafts()->at(_craft);
	if (c->hasCustomDeployment())
		_btnPreview->setText(tr("STR_CRAFT_DEPLOYMENT_PREVIEW_SAVED"));
	else
		_btnPreview->setText(tr("STR_CRAFT_DEPLOYMENT_PREVIEW"));
}

/**
 * Reorders a soldier up.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstItemsLeftArrowClick(Action *action)
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
void CraftSoldiersState::moveSoldierUp(Action *action, unsigned int row, bool max)
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
void CraftSoldiersState::lstItemsRightArrowClick(Action *action)
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
void CraftSoldiersState::moveSoldierDown(Action *action, unsigned int row, bool max)
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
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}
	int row = _lstSoldiers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Craft *c = _base->getCrafts()->at(_craft);
		Soldier *s = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());
		auto r = c->getRules()->getRequiredRole();
		Uint8 color = _lstSoldiers->getColor();

		bool isBusy = false, isFree = false;
		std::string duty = s->getCurrentDuty(_game->getLanguage(), _base->getSumRecoveryPerDay(), isBusy, isFree);

		if (s->getCraft() == c)
		{
			s->setCraft(0);
			_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
		}
		else if (isBusy)
		{
			return;
		}
		else if (s->hasFullHealth())
		{
			if (_isInterceptor && _ftaUI && s->getRoleRank(r) == 0)
			{
				_game->pushState(new ErrorMessageState(tr("STR_ROLE_IS_NOT_ALLOWED_PILOTING"),
					_palette,
					_game->getMod()->getInterface("soldierInfo")->getElement("errorMessage")->color,
					"BACK01.SCR",
					_game->getMod()->getInterface("soldierInfo")->getElement("errorPalette")->color));
				return;
			}

			auto space = c->getSpaceAvailable();
			if (c->validateAddingSoldier(space, s))
			{
				s->setCraft(c, true);
				_lstSoldiers->setCellText(row, 2, c->getName(_game->getLanguage()));
				color = _lstSoldiers->getSecondaryColor();

				// update the label to indicate absence of a saved craft deployment
				_btnPreview->setText(tr("STR_CRAFT_DEPLOYMENT_PREVIEW"));
			}
			else if (space > 0)
			{
				_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_CRAFT_SPACE"),
					_palette,
					_game->getMod()->getInterface("soldierInfo")->getElement("errorMessage")->color,
					"BACK01.SCR",
					_game->getMod()->getInterface("soldierInfo")->getElement("errorPalette")->color));
			}
		}
		_lstSoldiers->setRowColor(row, color);

		_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
		_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->pushState(new SoldierInfoState(_base, _soldierNumbers.at(row)));
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstSoldiersMousePress(Action *action)
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

void CraftSoldiersState::cbxScreenActionsChange(Action *action)
{
	_cbxSortBy->setSelected(0);
	initList(0);
}

/**
 * De-assign all soldiers from all craft located in the base (i.e. not out on a mission).
 * @param action Pointer to an action.
 */
void CraftSoldiersState::btnDeassignAllSoldiersClick(Action *action)
{
	Uint8 color = _lstSoldiers->getColor();

	int row = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getCovertOperation() != 0)
		{ }
		else
		{
			color = _lstSoldiers->getColor();
			if ((*i)->getCraft() && (*i)->getCraft()->getStatus() != "STR_OUT")
			{
				(*i)->setCraft(0);
				_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
			}
			else if ((*i)->getCraft() && (*i)->getCraft()->getStatus() == "STR_OUT")
			{
				color = _otherCraftColor;
			}
			_lstSoldiers->setRowColor(row, color);
		}
		row++;
	}

	Craft *c = _base->getCrafts()->at(_craft);
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

/**
 * De-assign all soldiers from the current craft.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::btnDeassignCraftSoldiersClick(Action *action)
{
	Craft *c = _base->getCrafts()->at(_craft);
	int row = 0;
	for (auto s : *_base->getSoldiers())
	{
		if (s->getCraft() == c)
		{
			s->setCraft(0);
			_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
			_lstSoldiers->setRowColor(row, _lstSoldiers->getColor());
		}
		row++;
	}

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

}
