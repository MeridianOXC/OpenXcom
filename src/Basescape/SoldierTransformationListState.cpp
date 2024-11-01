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
#include "SoldierTransformationListState.h"
#include <algorithm>
#include <climits>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Unicode.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Transformations Overview screen.
 * @param base Pointer to the base to get info from.
 * @param screenActions Pointer to the list of transformations in the parent SoldiersState.
 */
SoldierTransformationListState::SoldierTransformationListState(Base *base, ComboBox *screenActions) : _base(base), _screenActions(screenActions)
{
	// Calculate once
	_game->getSavedGame()->getAvailableTransformations(_availableTransformations, _game->getMod(), _base);

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(310, 17, 5, 8);
	_cbxSoldierType = new ComboBox(this, 148, 16, 8, 30, false);
	_cbxSoldierStatus = new ComboBox(this, 148, 16, 164, 30, false);
	_btnQuickSearch = new TextEdit(this, 48, 9, 16, 48);
	_txtProject = new Text(140, 9, 16, 58);
	_txtNumber = new Text(78, 17, 160, 50);
	_txtSoldierNumber = new Text(78, 17, 238, 50);
	_lstTransformations = new TextList(288, 104, 8, 68);
	_btnOnlyEligible = new ToggleTextButton(148, 16, 8, 176);
	_btnOK = new TextButton(148, 16, 164, 176);

	// Set palette
	setInterface("transformationList");

	add(_window, "window", "transformationList");
	add(_txtTitle, "text1", "transformationList");
	add(_btnQuickSearch, "button", "transformationList");
	add(_txtProject, "text2", "transformationList");
	add(_txtNumber, "text2", "transformationList");
	add(_txtSoldierNumber, "text2", "transformationList");
	add(_lstTransformations, "list", "transformationList");
	add(_btnOnlyEligible, "button", "transformationList");
	add(_btnOK, "button", "transformationList");
	add(_cbxSoldierType, "button2", "transformationList");
	add(_cbxSoldierStatus, "button2", "transformationList");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "transformationList");

	_btnOnlyEligible->setText(tr("STR_SHOW_ONLY_ELIGIBLE"));
	_btnOnlyEligible->onMouseClick((ActionHandler)&SoldierTransformationListState::btnOnlyEligibleClick);

	_btnOK->setText(tr("STR_OK"));
	_btnOK->onMouseClick((ActionHandler)&SoldierTransformationListState::btnOkClick);
	_btnOK->onKeyboardPress((ActionHandler)&SoldierTransformationListState::btnOkClick, Options::keyCancel);
	_btnOK->onKeyboardPress((ActionHandler)&SoldierTransformationListState::btnOkClick, Options::keyOk);

	std::vector<std::string> availableOptions;
	availableOptions.push_back("STR_ALL_SOLDIER_TYPES");
	for (auto& soldierType : _game->getMod()->getSoldiersList())
	{
		availableOptions.push_back(soldierType);
	}

	_cbxSoldierType->setOptions(availableOptions, true);
	_cbxSoldierType->setSelected(0);
	_cbxSoldierType->onChange((ActionHandler)&SoldierTransformationListState::cbxSoldierTypeChange);

	availableOptions.clear();
	availableOptions.push_back("STR_ALL_SOLDIERS");
	availableOptions.push_back("STR_HEALTHY_SOLDIERS");
	availableOptions.push_back("STR_WOUNDED_SOLDIERS");
	availableOptions.push_back("STR_DEAD_SOLDIERS");

	_cbxSoldierStatus->setOptions(availableOptions, true);
	_cbxSoldierStatus->setSelected(0);
	_cbxSoldierStatus->onChange((ActionHandler)&SoldierTransformationListState::cbxSoldierStatusChange);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFORMATIONS_OVERVIEW"));

	_txtProject->setAlign(ALIGN_LEFT);
	_txtProject->setText(tr("STR_PROJECT_NAME"));

	_txtNumber->setAlign(ALIGN_CENTER);
	_txtNumber->setWordWrap(true);
	_txtNumber->setText(tr("STR_AVAILABLE_MATERIALS"));

	_txtSoldierNumber->setAlign(ALIGN_CENTER);
	_txtSoldierNumber->setWordWrap(true);
	_txtSoldierNumber->setText(tr("STR_ELIGIBLE_SOLDIERS"));

	_lstTransformations->setColumns(3, 178, 10, 78);
	_lstTransformations->setAlign(ALIGN_RIGHT, 1);
	_lstTransformations->setAlign(ALIGN_RIGHT, 2);
	_lstTransformations->setSelectable(true);
	_lstTransformations->setBackground(_window);
	_lstTransformations->setMargin(8);
	_lstTransformations->onMouseClick((ActionHandler)&SoldierTransformationListState::lstTransformationsClick);
	_lstTransformations->onMouseClick((ActionHandler)&SoldierTransformationListState::lstTransformationsClick, SDL_BUTTON_MIDDLE);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SoldierTransformationListState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(Options::oxceQuickSearchButton);

	_btnOK->onKeyboardRelease((ActionHandler)&SoldierTransformationListState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	initList();
}

/**
 * Cleans up dynamic state.
 */
SoldierTransformationListState::~SoldierTransformationListState()
{

}

/**
 * Inits the list.
 */
void SoldierTransformationListState::initList()
{
	_lstTransformations->clearList();
	_transformationIndices.clear();

	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	int currentIndex = -1;
	ItemContainer* itemContainer(_base->getStorageItems());
	for (const auto* transformationRule : _availableTransformations)
	{
		++currentIndex;

		// quick search
		if (!searchString.empty())
		{
			std::string transformationName = tr(transformationRule->getName());
			Unicode::upperCase(transformationName);
			if (transformationName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		if (_cbxSoldierType->getSelected() > 0 &&
			std::find(
				transformationRule->getAllowedSoldierTypes().begin(),
				transformationRule->getAllowedSoldierTypes().end(),
				_game->getMod()->getSoldiersList().at(_cbxSoldierType->getSelected() - 1)) == transformationRule->getAllowedSoldierTypes().end())
		{
			continue;
		}

		switch (_cbxSoldierStatus->getSelected())
		{
		case 1:
			if (!transformationRule->isAllowingAliveSoldiers()) continue;
			break;
		case 2:
			if (!transformationRule->isAllowingWoundedSoldiers()) continue;
			break;
		case 3:
			if (!transformationRule->isAllowingDeadSoldiers()) continue;
			break;
		default:
			break;
		}

		std::ostringstream col1;
		std::ostringstream col2;

		// supplies calculation
		int projectsPossible = 10; // max
		if (transformationRule->getCost() > 0)
		{
			int byFunds = _game->getSavedGame()->getFunds() / transformationRule->getCost();
			projectsPossible = std::min(projectsPossible, byFunds);
		}
		for (auto& item : transformationRule->getRequiredItems())
		{
			RuleItem* itemRule = _game->getMod()->getItem(item.first);
			projectsPossible = std::min(projectsPossible, itemContainer->getItem(itemRule) / item.second);
		}
		if (projectsPossible <= 0)
		{
			col1 << '-';
		}
		else
		{
			if (projectsPossible < 10)
			{
				col1 << projectsPossible;
			}
			else
			{
				col1 << '+';
			}
		}

		int eligibleSoldiers = 0;
		for (const auto* soldier : *_base->getSoldiers())
		{
			if (soldier->getCraft() && soldier->getCraft()->getStatus() == "STR_OUT")
			{
				// soldiers outside of the base are not eligible
				continue;
			}
			if (soldier->isEligibleForTransformation(transformationRule))
			{
				++eligibleSoldiers;
			}
		}
		for (const auto* deadMan : *_game->getSavedGame()->getDeadSoldiers())
		{
			if (deadMan->isEligibleForTransformation(transformationRule))
			{
				++eligibleSoldiers;
			}
		}

		if (eligibleSoldiers <= 0)
		{
			if (_btnOnlyEligible->getPressed()) continue;

			col2 << '-';
		}
		else
		{
			if (eligibleSoldiers < 10)
			{
				col2 << eligibleSoldiers;
			}
			else
			{
				col2 << '+';
			}
		}

		_lstTransformations->addRow(3, tr(transformationRule->getName()).c_str(), col1.str().c_str(), col2.str().c_str());
		_transformationIndices.push_back(currentIndex);
	}
}

/**
 * Quick search toggle.
 * @param action Pointer to an action.
 */
void SoldierTransformationListState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText("");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
 * Quick search.
 * @param action Pointer to an action.
 */
void SoldierTransformationListState::btnQuickSearchApply(Action *)
{
	initList();
}

/**
 * Filters the list of transformations by soldier type.
 * @param action Pointer to an action
 */
void SoldierTransformationListState::cbxSoldierTypeChange(Action *)
{
	initList();
}

/**
 * Filters the list of transformations by soldier status.
 * @param action Pointer to an action
 */
void SoldierTransformationListState::cbxSoldierStatusChange(Action *)
{
	initList();
}

/**
 * Toggles showing transformations with eligible soldiers and all transformations.
 * @param action Pointer to an action.
 */
void SoldierTransformationListState::btnOnlyEligibleClick(Action *)
{
	initList();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierTransformationListState::btnOkClick(Action *)
{
	_screenActions->setSelected(0);
	_game->popState();
}

/**
 * Selects a transformation (L-click) or shows info about a transformation (M-click).
 * @param action Pointer to an action.
 */
void SoldierTransformationListState::lstTransformationsClick(Action *action)
{
	int transformationIndex = _transformationIndices.at(_lstTransformations->getSelectedRow());

	if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		std::string articleId = _availableTransformations.at(transformationIndex)->getName();
		Ufopaedia::openArticle(_game, articleId);
		return;
	}

	_screenActions->setSelected(_screenActions->getSelected() + transformationIndex + 1);
	_game->popState();
}

}
