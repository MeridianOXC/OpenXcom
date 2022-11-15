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
#include "CraftArmorState.h"
#include <climits>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Engine/Action.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Menu/ErrorMessageState.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Mod/Armor.h"
#include "SoldierArmorState.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleSoldier.h"
#include "../Ufopaedia/Ufopaedia.h"
#include <algorithm>
#include "../Engine/Unicode.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Armor screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftArmorState::CraftArmorState(Base *base, size_t craft) : _base(base), _craft(craft), _savedScrollPosition(0), _origSoldierOrder(*_base->getSoldiers()), _dynGetter(NULL)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(300, 17, 16, 7);
	_txtName = new Text(114, 9, 16, 32);
	_txtCraft = new Text(76, 9, 122, 32);
	_txtArmor = new Text(100, 9, 192, 32);
	_lstSoldiers = new TextList(288, 128, 8, 40);
	_cbxSortBy = new ComboBox(this, 148, 16, 8, 176, true);

	// Set palette
	setInterface("craftArmor");

	add(_window, "window", "craftArmor");
	add(_btnOk, "button", "craftArmor");
	add(_txtTitle, "text", "craftArmor");
	add(_txtName, "text", "craftArmor");
	add(_txtCraft, "text", "craftArmor");
	add(_txtArmor, "text", "craftArmor");
	add(_lstSoldiers, "list", "craftArmor");
	add(_cbxSortBy, "button", "craftArmor");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftArmor");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftArmorState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftArmorState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&CraftArmorState::btnDeequipAllArmorClick, Options::keyRemoveArmorFromAllCrafts);
	_btnOk->onKeyboardPress((ActionHandler)&CraftArmorState::btnDeequipCraftArmorClick, Options::keyRemoveArmorFromCraft);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_ARMOR"));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtCraft->setText(tr("STR_CRAFT"));

	_txtArmor->setText(tr("STR_ARMOR"));

	// populate sort options
	std::vector<std::string> sortOptions;
	sortOptions.push_back(tr("STR_ORIGINAL_ORDER"));
	_sortFunctors.push_back(NULL);

#define PUSH_IN(strId, functor) \
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
	_cbxSortBy->onChange((ActionHandler)&CraftArmorState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	//_lstSoldiers->setArrowColumn(-1, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 106, 70, 104);
	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)&CraftArmorState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)&CraftArmorState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)&CraftArmorState::lstSoldiersClick, 0);
	_lstSoldiers->onMousePress((ActionHandler)&CraftArmorState::lstSoldiersMousePress);
}

/**
 * cleans up dynamic state
 */
CraftArmorState::~CraftArmorState()
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
void CraftArmorState::cbxSortByChange(Action *action)
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

	initList(_lstSoldiers->getScroll());
}

/**
 * The soldier armors can change
 * after going into other screens.
 */
void CraftArmorState::init()
{
	State::init();
	_base->prepareSoldierStatsWithBonuses(); // refresh stats for sorting
	initList(_savedScrollPosition);

	int row = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		_lstSoldiers->setCellText(row, 2, tr((*i)->getArmor()->getType()));
		row++;
	}
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void CraftArmorState::initList(size_t scrl)
{
	Uint8 otherCraftColor = _game->getMod()->getInterface("craftArmor")->getElement("otherCraft")->color;
	int row = 0;
	_lstSoldiers->clearList();

	if (_dynGetter != NULL)
	{
		_lstSoldiers->setArrowColumn(160, ARROW_VERTICAL);
		_lstSoldiers->setColumns(4, 106, 70, 88, 16);
	}
	else
	{
		_lstSoldiers->setArrowColumn(-1, ARROW_VERTICAL);
		_lstSoldiers->setColumns(3, 106, 70, 104);
	}

	Craft *c = _base->getCrafts()->at(_craft);
	auto recovery = _base->getSumRecoveryPerDay();
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if (_dynGetter != NULL)
		{
			// call corresponding getter
			int dynStat = (*_dynGetter)(_game, *i);
			std::ostringstream ss;
			ss << dynStat;
			_lstSoldiers->addRow(4, (*i)->getName(true).c_str(), (*i)->getCraftString(_game->getLanguage(), recovery).c_str(), tr((*i)->getArmor()->getType()).c_str(), ss.str().c_str());
		}
		else
		{
			_lstSoldiers->addRow(3, (*i)->getName(true).c_str(), (*i)->getCraftString(_game->getLanguage(), recovery).c_str(), tr((*i)->getArmor()->getType()).c_str());
		}

		Uint8 color;
		if ((*i)->getCraft() == c)
		{
			color = _lstSoldiers->getSecondaryColor();
		}
		else if ((*i)->getCraft() != 0)
		{
			color = otherCraftColor;
		}
		else
		{
			color = _lstSoldiers->getColor();
		}
		_lstSoldiers->setRowColor(row, color);
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
void CraftArmorState::lstItemsLeftArrowClick(Action *action)
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
void CraftArmorState::moveSoldierUp(Action *action, unsigned int row, bool max)
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
void CraftArmorState::lstItemsRightArrowClick(Action *action)
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
void CraftArmorState::moveSoldierDown(Action *action, unsigned int row, bool max)
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
void CraftArmorState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Shows the Select Armor window.
 * @param action Pointer to an action.
 */
void CraftArmorState::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}

	Soldier *s = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());
	if (!(s->getCraft() && s->getCraft()->getStatus() == "STR_OUT"))
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			if (_game->isCtrlPressed())
			{
				Craft* c = _base->getCrafts()->at(_craft);
				if (s->getCraft() == c)
				{
					s->setCraftAndMoveEquipment(0, _base, _game->getSavedGame()->getMonthsPassed() == -1);
					_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 1, tr("STR_NONE_UC"));
					_lstSoldiers->setRowColor(_lstSoldiers->getSelectedRow(), _lstSoldiers->getColor());
				}
				else if (s->hasFullHealth())
				{
					auto space = c->getSpaceAvailable();
					if (c->validateAddingSoldier(space, s))
					{
						s->setCraftAndMoveEquipment(c, _base, _game->getSavedGame()->getMonthsPassed() == -1, true);
						_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 1, c->getName(_game->getLanguage()));
						_lstSoldiers->setRowColor(_lstSoldiers->getSelectedRow(), _lstSoldiers->getSecondaryColor());
					}
					else if (space > 0)
					{
						_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_CRAFT_SPACE"), _palette, _game->getMod()->getInterface("soldierInfo")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("soldierInfo")->getElement("errorPalette")->color));
					}
				}
			}
			else
			{
				_savedScrollPosition = _lstSoldiers->getScroll();
				_game->pushState(new SoldierArmorState(_base, _lstSoldiers->getSelectedRow(), SA_GEOSCAPE));
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			SavedGame *save;
			save = _game->getSavedGame();
			Armor *a = _game->getMod()->getArmor(save->getLastSelectedArmor());
			bool armorUnlocked = true;
			if (a && a->getRequiredResearch() && !_game->getSavedGame()->isResearched(a->getRequiredResearch()))
			{
				armorUnlocked = false;
			}
			if (armorUnlocked && a)
			{
				Craft* craft = s->getCraft();
				if (craft && !craft->validateArmorChange(s->getArmor()->getSize(), a->getSize()))
				{
					armorUnlocked = false; // armor not valid due to craft constraints
					_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_CRAFT_SPACE"), _palette, _game->getMod()->getInterface("soldierInfo")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("soldierInfo")->getElement("errorPalette")->color));
				}
			}
			if (armorUnlocked && a && a->getCanBeUsedBy(s->getRules()))
			{
				if (save->getMonthsPassed() != -1)
				{
					if (a->getStoreItem() == nullptr || _base->getStorageItems()->getItem(a->getStoreItem()) > 0)
					{
						if (s->getArmor()->getStoreItem())
						{
							_base->getStorageItems()->addItem(s->getArmor()->getStoreItem());
						}
						if (a->getStoreItem())
						{
							_base->getStorageItems()->removeItem(a->getStoreItem());
						}

						s->setArmor(a, true);
						s->prepareStatsWithBonuses(_game->getMod()); // refresh stats for sorting
						_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
					}
				}
				else
				{
					s->setArmor(a, true);
					s->prepareStatsWithBonuses(_game->getMod()); // refresh stats for sorting
					_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
				}
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
		{
			std::string articleId = s->getArmor()->getUfopediaType();
			Ufopaedia::openArticle(_game, articleId);
		}
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void CraftArmorState::lstSoldiersMousePress(Action *action)
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
 * De-equip armor from all soldiers located in the base (i.e. not out on a mission).
 * @param action Pointer to an action.
 */
void CraftArmorState::btnDeequipAllArmorClick(Action *action)
{
	int row = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if (!((*i)->getCraft() && (*i)->getCraft()->getStatus() == "STR_OUT"))
		{
			Armor *a = (*i)->getRules()->getDefaultArmor();

			if ((*i)->getCraft() && !(*i)->getCraft()->validateArmorChange((*i)->getArmor()->getSize(), a->getSize()))
			{
				// silently ignore
				row++;
				continue;
			}
			if (a->getStoreItem() == nullptr || _base->getStorageItems()->getItem(a->getStoreItem()) > 0)
			{
				if ((*i)->getArmor()->getStoreItem())
				{
					_base->getStorageItems()->addItem((*i)->getArmor()->getStoreItem());
				}
				if (a->getStoreItem())
				{
					_base->getStorageItems()->removeItem(a->getStoreItem());
				}

				(*i)->setArmor(a, true);
				(*i)->prepareStatsWithBonuses(_game->getMod()); // refresh stats for sorting
				_lstSoldiers->setCellText(row, 2, tr(a->getType()));
			}
		}
		row++;
	}
}

/**
 * De-equip armor of all soldiers on the current craft, and also all soldiers not assigned to any craft.
 * @param action Pointer to an action.
 */
void CraftArmorState::btnDeequipCraftArmorClick(Action *action)
{
	Craft *c = _base->getCrafts()->at(_craft);
	int row = 0;
	for (auto s : *_base->getSoldiers())
	{
		if (s->getCraft() == c || s->getCraft() == 0)
		{
			Armor *a = s->getRules()->getDefaultArmor();

			if (s->getCraft() && !s->getCraft()->validateArmorChange(s->getArmor()->getSize(), a->getSize()))
			{
				// silently ignore
				row++;
				continue;
			}
			if (a->getStoreItem() == nullptr || _base->getStorageItems()->getItem(a->getStoreItem()) > 0)
			{
				if (s->getArmor()->getStoreItem())
				{
					_base->getStorageItems()->addItem(s->getArmor()->getStoreItem());
				}
				if (a->getStoreItem())
				{
					_base->getStorageItems()->removeItem(a->getStoreItem());
				}

				s->setArmor(a, true);
				s->prepareStatsWithBonuses(_game->getMod()); // refresh stats for sorting
				_lstSoldiers->setCellText(row, 2, tr(a->getType()));
			}
		}
		row++;
	}
}

}
