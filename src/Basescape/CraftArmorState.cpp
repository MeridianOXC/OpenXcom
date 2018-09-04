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
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Mod/Armor.h"
#include "SoldierArmorState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleSoldier.h"
#include "../Ufopaedia/Ufopaedia.h"
#include <algorithm>

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
	_window->setBackground(_game->getMod()->getSurface("BACK14.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftArmorState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftArmorState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&CraftArmorState::btnDeequipAllArmorClick, Options::keyResetAll);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_ARMOR"));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtCraft->setText(tr("STR_CRAFT"));

	_txtArmor->setText(tr("STR_ARMOR"));

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
	_lstSoldiers->onMousePress((ActionHandler)&CraftArmorState::lstSoldiersClick);
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
	bool ctrlPressed = SDL_GetModState() & KMOD_CTRL;
	size_t selIdx = _cbxSortBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}

	SortFunctor *compFunc = _sortFunctors[selIdx];
	if (compFunc)
	{
		// if CTRL is pressed, we only want to show the dynamic column, without actual sorting
		if (!ctrlPressed)
		{
			std::stable_sort(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), *compFunc);
			bool shiftPressed = SDL_GetModState() & KMOD_SHIFT;
			if (shiftPressed)
			{
				std::reverse(_base->getSoldiers()->begin(), _base->getSoldiers()->end());
			}
		}
	}
	else
	{
		_dynGetter = NULL;
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

	if (compFunc)
	{
		_dynGetter = compFunc->getGetter();
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
	float absBonus = _base->getSickBayAbsoluteBonus();
	float relBonus = _base->getSickBayRelativeBonus();
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if (_dynGetter != NULL)
		{
			// call corresponding getter
			int dynStat = (*_dynGetter)(_game, *i);
			std::wostringstream ss;
			ss << dynStat;
			_lstSoldiers->addRow(4, (*i)->getName(true).c_str(), (*i)->getCraftString(_game->getLanguage(), absBonus, relBonus).c_str(), tr((*i)->getArmor()->getType()).c_str(), ss.str().c_str());
		}
		else
		{
			_lstSoldiers->addRow(3, (*i)->getName(true).c_str(), (*i)->getCraftString(_game->getLanguage(), absBonus, relBonus).c_str(), tr((*i)->getArmor()->getType()).c_str());
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
			_savedScrollPosition = _lstSoldiers->getScroll();
			_game->pushState(new SoldierArmorState(_base, _lstSoldiers->getSelectedRow(), SA_GEOSCAPE));
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			SavedGame *save;
			save = _game->getSavedGame();
			Armor *a = _game->getMod()->getArmor(save->getLastSelectedArmor());
			if (a && (a->getUnits().empty() || std::find(a->getUnits().begin(), a->getUnits().end(), s->getRules()->getType()) != a->getUnits().end()))
			{
				if (save->getMonthsPassed() != -1)
				{
					if (_base->getStorageItems()->getItem(a->getStoreItem()) > 0 || a->getStoreItem() == Armor::NONE)
					{
						if (s->getArmor()->getStoreItem() != Armor::NONE)
						{
							_base->getStorageItems()->addItem(s->getArmor()->getStoreItem());
						}
						if (a->getStoreItem() != Armor::NONE)
						{
							_base->getStorageItems()->removeItem(a->getStoreItem());
						}

						s->setArmor(a);
						_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
					}
				}
				else
				{
					s->setArmor(a);
					_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
				}
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
		{
			std::string articleId = s->getArmor()->getType();
			Ufopaedia::openArticle(_game, articleId);
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
			Armor *a = _game->getMod()->getArmor((*i)->getRules()->getArmor());

			if (_base->getStorageItems()->getItem(a->getStoreItem()) > 0 || a->getStoreItem() == Armor::NONE)
			{
				if ((*i)->getArmor()->getStoreItem() != Armor::NONE)
				{
					_base->getStorageItems()->addItem((*i)->getArmor()->getStoreItem());
				}
				if (a->getStoreItem() != Armor::NONE)
				{
					_base->getStorageItems()->removeItem(a->getStoreItem());
				}

				(*i)->setArmor(a);
				_lstSoldiers->setCellText(row, 2, tr(a->getType()));
			}
		}
		row++;
	}
}

}
