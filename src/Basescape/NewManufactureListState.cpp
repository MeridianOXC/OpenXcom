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
#include <algorithm>
#include <locale>
#include "NewManufactureListState.h"
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleManufacture.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "ManufactureStartState.h"
#include "TechTreeViewerState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the productions list screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
NewManufactureListState::NewManufactureListState(Base *base) : _base(base), _showRequirements(false), _detailClicked(false), _lstScroll(0)
{
	_screen = false;

	_window = new Window(this, 320, 156, 0, 22, POPUP_BOTH);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 35);
	_btnOk = new TextButton(148, 16, 164, 154);
	_btnShowOnlyNew = new ToggleTextButton(148, 16, 8, 154);
	_txtTitle = new Text(320, 17, 0, 30);
	_txtItem = new Text(156, 9, 10, 62);
	_txtCategory = new Text(130, 9, 166, 62);
	_lstManufacture = new TextList(288, 80, 8, 70);
	_cbxFilter = new ComboBox(this, 146, 16, 10, 46);
	_cbxCategory = new ComboBox(this, 146, 16, 166, 46);

	// Set palette
	setInterface("selectNewManufacture");

	_hiddenColor = _game->getMod()->getInterface("selectNewManufacture")->getElement("listExtended")->color;
	_facilityRequiredColor = _game->getMod()->getInterface("selectNewManufacture")->getElement("listExtended")->color2;

	add(_window, "window", "selectNewManufacture");
	add(_btnQuickSearch, "button", "selectNewManufacture");
	add(_btnOk, "button", "selectNewManufacture");
	add(_btnShowOnlyNew, "button", "selectNewManufacture");
	add(_txtTitle, "text", "selectNewManufacture");
	add(_txtItem, "text", "selectNewManufacture");
	add(_txtCategory, "text", "selectNewManufacture");
	add(_lstManufacture, "list", "selectNewManufacture");
	add(_cbxFilter, "catBox", "selectNewManufacture");
	add(_cbxCategory, "catBox", "selectNewManufacture");

	centerAllSurfaces();

	_window->setBackground(_game->getMod()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_PRODUCTION_ITEMS"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtItem->setText(tr("STR_ITEM"));

	_txtCategory->setText(tr("STR_CATEGORY"));

	_lstManufacture->setColumns(3, 156, 120, 10);
	_lstManufacture->setSelectable(true);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setMargin(2);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdClickLeft, SDL_BUTTON_LEFT);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdClickRight, SDL_BUTTON_RIGHT);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdClickMiddle, SDL_BUTTON_MIDDLE);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&NewManufactureListState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&NewManufactureListState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&NewManufactureListState::btnMarkAllAsSeenClick, Options::keyMarkAllAsSeen);

	_btnShowOnlyNew->setText(tr("STR_SHOW_ONLY_NEW"));
	_btnShowOnlyNew->onMouseClick((ActionHandler)&NewManufactureListState::btnShowOnlyNewClick);

	std::vector<std::string> filterOptions;
	filterOptions.push_back("STR_FILTER_DEFAULT");
	filterOptions.push_back("STR_FILTER_DEFAULT_SUPPLIES_OK");
	filterOptions.push_back("STR_FILTER_DEFAULT_NO_SUPPLIES");
	filterOptions.push_back("STR_FILTER_FACILITY_REQUIRED");
	filterOptions.push_back("STR_FILTER_HIDDEN");
	_cbxFilter->setOptions(filterOptions);
	_cbxFilter->onChange((ActionHandler)&NewManufactureListState::cbxFilterChange);

	_catStrings.push_back("STR_ALL_ITEMS");
	_cbxCategory->setOptions(_catStrings);

	_btnQuickSearch->setText(L""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&NewManufactureListState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&NewManufactureListState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
}

/**
 * Initializes state (fills list of possible productions).
 */
void NewManufactureListState::init()
{
	State::init();
	fillProductionList(!_detailClicked);
	_lstManufacture->scrollTo(_lstScroll);
}

/**
 * Returns to the previous screen.
 * @param action A pointer to an Action.
 */
void NewManufactureListState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens the Production settings screen.
 * @param action A pointer to an Action.
 */
void NewManufactureListState::lstProdClickLeft(Action *)
{
	_lstScroll = _lstManufacture->getScroll();

	ManufacturingFilterType basicFilter = (ManufacturingFilterType)(_cbxFilter->getSelected());
	if (basicFilter == MANU_FILTER_FACILITY_REQUIRED)
		return;

	RuleManufacture *rule = 0;
	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		if ((*it)->getName() == _displayedStrings[_lstManufacture->getSelectedRow()])
		{
			rule = (*it);
			break;
		}
	}

	// check and display error messages only further down the chain
	_detailClicked = true;
	_game->pushState(new ManufactureStartState(_base, rule));
}

/**
* Changes the status (new -> normal -> hidden -> new).
* @param action A pointer to an Action.
*/
void NewManufactureListState::lstProdClickRight(Action *)
{
	ManufacturingFilterType basicFilter = (ManufacturingFilterType)(_cbxFilter->getSelected());
	if (basicFilter == MANU_FILTER_FACILITY_REQUIRED)
	{
		// display either category or requirements
		_showRequirements = !_showRequirements;
		const std::vector<std::string> &baseFunc = _base->getProvidedBaseFunc();

		for (size_t row = 0; row < _lstManufacture->getRows(); ++row)
		{
			RuleManufacture *info = _game->getMod()->getManufacture(_displayedStrings[row]);
			if (info)
			{
				if (_showRequirements)
				{
					std::wostringstream ss;
					int count = 0;
					for (std::vector<std::string>::const_iterator iter = info->getRequireBaseFunc().begin(); iter != info->getRequireBaseFunc().end(); ++iter)
					{
						if (std::find(baseFunc.begin(), baseFunc.end(), *iter) != baseFunc.end())
						{
							continue;
						}
						if (count > 0)
						{
							ss << ", ";
						}
						ss << tr(*iter);
						count++;
					}
					_lstManufacture->setCellText(row, 1, ss.str().c_str());
				}
				else
				{
					_lstManufacture->setCellText(row, 1, tr(info->getCategory()));
				}
			}
		}
	}
	else
	{
		// change status
		const std::string rule = _displayedStrings[_lstManufacture->getSelectedRow()];
		int oldState = _game->getSavedGame()->getManufactureRuleStatus(rule);
		int newState = (oldState + 1) % RuleManufacture::MANU_STATUSES;
		_game->getSavedGame()->setManufactureRuleStatus(rule, newState);

		if (newState == RuleManufacture::MANU_STATUS_HIDDEN)
		{
			_lstManufacture->setRowColor(_lstManufacture->getSelectedRow(), _hiddenColor);
		}
		else if (newState == RuleManufacture::MANU_STATUS_NEW)
		{
			_lstManufacture->setRowColor(_lstManufacture->getSelectedRow(), _lstManufacture->getSecondaryColor());
		}
		else
		{
			_lstManufacture->setRowColor(_lstManufacture->getSelectedRow(), _lstManufacture->getColor());
		}
	}
}

/**
* Opens the TechTreeViewer for the corresponding topic.
* @param action Pointer to an action.
*/
void NewManufactureListState::lstProdClickMiddle(Action *)
{
	const RuleManufacture *selectedTopic = _game->getMod()->getManufacture(_displayedStrings[_lstManufacture->getSelectedRow()]);
	_lstScroll = _lstManufacture->getScroll();
	_game->pushState(new TechTreeViewerState(0, selectedTopic));
}

/**
* Updates the production list to match the basic filter
*/

void NewManufactureListState::cbxFilterChange(Action *)
{
	_lstScroll = 0;
	fillProductionList(true);
}

/**
 * Updates the production list to match the category filter
 */

void NewManufactureListState::cbxCategoryChange(Action *)
{
	_lstScroll = 0;
	fillProductionList(false);
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void NewManufactureListState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText(L"");
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
void NewManufactureListState::btnQuickSearchApply(Action *)
{
	_lstScroll = 0;
	fillProductionList(false);
}

/**
* Filter to display only new items.
* @param action Pointer to an action.
*/
void NewManufactureListState::btnShowOnlyNewClick(Action *)
{
	_lstScroll = 0;
	fillProductionList(false);
}

/**
* Marks all items as seen
* @param action Pointer to an action.
*/
void NewManufactureListState::btnMarkAllAsSeenClick(Action *)
{
	ManufacturingFilterType basicFilter = (ManufacturingFilterType)(_cbxFilter->getSelected());
	if (basicFilter == MANU_FILTER_FACILITY_REQUIRED)
		return;

	for (std::vector<std::string>::const_iterator i = _displayedStrings.begin(); i != _displayedStrings.end(); ++i)
	{
		// mark all (new) manufacture items as normal
		if (_game->getSavedGame()->getManufactureRuleStatus((*i)) != RuleManufacture::MANU_STATUS_HIDDEN)
		{
			_game->getSavedGame()->setManufactureRuleStatus((*i), RuleManufacture::MANU_STATUS_NORMAL);
		}
	}

	fillProductionList(false);
}

/**
 * Fills the list of possible productions.
 */
void NewManufactureListState::fillProductionList(bool refreshCategories)
{
	std::locale myLocale = CrossPlatform::testLocale();
	std::wstring searchString = _btnQuickSearch->getText();
	CrossPlatform::upperCase(searchString, myLocale);

	if (refreshCategories)
	{
		_cbxCategory->onChange(0);
		_cbxCategory->setSelected(0);
	}

	_showRequirements = false;
	_detailClicked = false;

	_lstManufacture->clearList();
	_possibleProductions.clear();
	ManufacturingFilterType basicFilter = (ManufacturingFilterType)(_cbxFilter->getSelected());
	_game->getSavedGame()->getAvailableProductions(_possibleProductions, _game->getMod(), _base, basicFilter);
	_displayedStrings.clear();

	ItemContainer * itemContainer (_base->getStorageItems());
	int row = 0;
	bool hasUnseen = false;
	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		if (((*it)->getCategory() == _catStrings[_cbxCategory->getSelected()]) || (_catStrings[_cbxCategory->getSelected()] == "STR_ALL_ITEMS"))
		{
			// filter
			bool isHidden = _game->getSavedGame()->getManufactureRuleStatus((*it)->getName()) == RuleManufacture::MANU_STATUS_HIDDEN;
			if (basicFilter == MANU_FILTER_DEFAULT && isHidden)
				continue;
			if (basicFilter == MANU_FILTER_DEFAULT_SUPPLIES_OK && isHidden)
				continue;
			if (basicFilter == MANU_FILTER_DEFAULT_NO_SUPPLIES && isHidden)
				continue;
			if (basicFilter == MANU_FILTER_HIDDEN && !isHidden)
				continue;

			bool isNew = _game->getSavedGame()->getManufactureRuleStatus((*it)->getName()) == RuleManufacture::MANU_STATUS_NEW;
			if (_btnShowOnlyNew->getPressed())
			{
				if (!isNew)
				{
					continue;
				}
			}

			// quick search
			if (searchString != L"")
			{
				std::wstring projectName = tr((*it)->getName());
				CrossPlatform::upperCase(projectName, myLocale);
				if (projectName.find(searchString) == std::string::npos)
				{
					continue;
				}
			}

			// supplies calculation
			int productionPossible = 10; // max
			if ((*it)->getManufactureCost() > 0)
			{
				int byFunds = _game->getSavedGame()->getFunds() / (*it)->getManufactureCost();
				productionPossible = std::min(productionPossible, byFunds);
			}
			const std::map<std::string, int> & requiredItems ((*it)->getRequiredItems());
			for (std::map<std::string, int>::const_iterator iter = requiredItems.begin(); iter != requiredItems.end(); ++iter)
			{
				productionPossible = std::min(productionPossible, itemContainer->getItem(iter->first) / iter->second);
			}
			std::wostringstream ss;
			if (productionPossible <= 0)
			{
				if (basicFilter == MANU_FILTER_DEFAULT_SUPPLIES_OK)
					continue;
				ss << L'-';
			}
			else
			{
				if (basicFilter == MANU_FILTER_DEFAULT_NO_SUPPLIES)
					continue;
				if (productionPossible < 10)
				{
					ss << productionPossible;
				}
				else
				{
					ss << L'+';
				}
			}

			_lstManufacture->addRow(3, tr((*it)->getName()).c_str(), tr((*it)->getCategory()).c_str(), ss.str().c_str());
			_displayedStrings.push_back((*it)->getName().c_str());

			// colors
			if (basicFilter == MANU_FILTER_FACILITY_REQUIRED)
			{
				_lstManufacture->setRowColor(row, _facilityRequiredColor);
			}
			else
			{
				if (isHidden)
				{
					_lstManufacture->setRowColor(row, _hiddenColor);
				}
				else if (isNew)
				{
					_lstManufacture->setRowColor(row, _lstManufacture->getSecondaryColor());
					hasUnseen = true;
				}
			}
			row++;
		}
	}

	std::wstring label = tr("STR_SHOW_ONLY_NEW");
	_btnShowOnlyNew->setText((hasUnseen ? L"* " : L"") + label);

	if (refreshCategories)
	{
		_catStrings.clear();
		_catStrings.push_back("STR_ALL_ITEMS");

		for (size_t r = 0; r < _lstManufacture->getRows(); ++r)
		{
			RuleManufacture *info = _game->getMod()->getManufacture(_displayedStrings[r]);
			if (info)
			{
				bool addCategory = true;
				for (size_t x = 0; x < _catStrings.size(); ++x)
				{
					if (info->getCategory().c_str() == _catStrings[x])
					{
						addCategory = false;
						break;
					}
				}
				if (addCategory)
				{
					_catStrings.push_back(info->getCategory().c_str());
				}
			}
		}

		_cbxCategory->setOptions(_catStrings);
		_cbxCategory->onChange((ActionHandler)&NewManufactureListState::cbxCategoryChange);
	}
}

}
