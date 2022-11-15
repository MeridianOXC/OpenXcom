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
#include <locale>
#include "NewResearchListState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Mod/RuleResearch.h"
#include "ResearchInfoState.h"
#include "TechTreeViewerState.h"

namespace OpenXcom
{
/**
 * Initializes all the elements in the Research list screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param sortByCost Should the list be sorted by cost or listOrder?
 */
NewResearchListState::NewResearchListState(Base *base, bool sortByCost) : _base(base), _sortByCost(sortByCost), _lstScroll(0)
{
	if (Options::isPasswordCorrect())
	{
		_sortByCost = !_sortByCost;
	}

	_screen = false;

	_window = new Window(this, 230, 140, 45, 30, POPUP_BOTH);
	_btnQuickSearch = new TextEdit(this, 48, 9, 53, 38);
	_btnOK = new TextButton(103, 16, 164, 146);
	_cbxSort = new ComboBox(this, 103, 16, 53, 146);
	_btnShowOnlyNew = new ToggleTextButton(103, 16, 53, 146);
	_txtTitle = new Text(214, 16, 53, 38);
	_lstResearch = new TextList(198, 88, 53, 54);

	// Set palette
	setInterface("selectNewResearch");

	add(_window, "window", "selectNewResearch");
	add(_btnQuickSearch, "button", "selectNewResearch");
	add(_btnOK, "button", "selectNewResearch");
	add(_cbxSort, "button", "selectNewResearch");
	add(_btnShowOnlyNew, "button", "selectNewResearch");
	add(_txtTitle, "text", "selectNewResearch");
	add(_lstResearch, "list", "selectNewResearch");

	_colorNormal = _lstResearch->getColor();
	_colorNew = Options::oxceHighlightNewTopicsHidden ? _lstResearch->getSecondaryColor() : _colorNormal;

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "selectNewResearch");

	_btnOK->setText(tr("STR_OK"));
	_btnOK->onMouseClick((ActionHandler)&NewResearchListState::btnOKClick);
	_btnOK->onKeyboardPress((ActionHandler)&NewResearchListState::btnOKClick, Options::keyCancel);
	_btnOK->onKeyboardPress((ActionHandler)&NewResearchListState::btnMarkAllAsSeenClick, Options::keyMarkAllAsSeen);

	if (_game->getMod()->getEnableNewResearchSorting())
	{
		_btnShowOnlyNew->setVisible(false);
		std::vector<std::string> sortOptions;
		sortOptions.push_back("STR_SORT_DEFAULT");
		sortOptions.push_back("STR_SORT_BY_COST");
		sortOptions.push_back("STR_SORT_BY_NAME");
		sortOptions.push_back("STR_SHOW_ONLY_NEW"); // this is a filter, replacement for the hidden "Show Only New" toggle button
		_cbxSort->setOptions(sortOptions, true);
		if (_sortByCost)
		{
			_cbxSort->setSelected(1);
		}
		_cbxSort->onChange((ActionHandler)&NewResearchListState::cbxSortChange);
	}
	else
	{
		_cbxSort->setVisible(false);
		_btnShowOnlyNew->setText(tr("STR_SHOW_ONLY_NEW"));
		_btnShowOnlyNew->onMouseClick((ActionHandler)&NewResearchListState::btnShowOnlyNewClick);
	}

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_NEW_RESEARCH_PROJECTS"));

	_lstResearch->setColumns(1, 190);
	_lstResearch->setSelectable(true);
	_lstResearch->setBackground(_window);
	_lstResearch->setMargin(8);
	_lstResearch->setAlign(ALIGN_CENTER);
	_lstResearch->onMouseClick((ActionHandler)&NewResearchListState::onSelectProject, SDL_BUTTON_LEFT);
	_lstResearch->onMouseClick((ActionHandler)&NewResearchListState::onToggleProjectStatus, SDL_BUTTON_RIGHT);
	_lstResearch->onMouseClick((ActionHandler)&NewResearchListState::onOpenTechTreeViewer, SDL_BUTTON_MIDDLE);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&NewResearchListState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOK->onKeyboardRelease((ActionHandler)&NewResearchListState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
}

/**
 * Initializes the screen (fills the list).
 */
void NewResearchListState::init()
{
	State::init();
	fillProjectList(false);
}

/**
 * Selects the RuleResearch to work on.
 * @param action Pointer to an action.
 */
void NewResearchListState::onSelectProject(Action *)
{
	_lstScroll = _lstResearch->getScroll();
	_game->pushState(new ResearchInfoState(_base, _projects[_lstResearch->getSelectedRow()]));
}

/**
* Selects the RuleResearch to work on.
* @param action Pointer to an action.
*/
void NewResearchListState::onToggleProjectStatus(Action *)
{
	if (!Options::oxceHighlightNewTopicsHidden)
		return;

	// change status
	const std::string rule = _projects[_lstResearch->getSelectedRow()]->getName();
	if (_game->getSavedGame()->isResearchRuleStatusNew(rule))
	{
		// new -> normal
		_game->getSavedGame()->setResearchRuleStatus(rule, RuleResearch::RESEARCH_STATUS_NORMAL);
		_lstResearch->setRowColor(_lstResearch->getSelectedRow(), _colorNormal);
	}
	else
	{
		// normal/disabled -> new
		_game->getSavedGame()->setResearchRuleStatus(rule, RuleResearch::RESEARCH_STATUS_NEW);
		_lstResearch->setRowColor(_lstResearch->getSelectedRow(), _colorNew);
	}
}

/**
* Opens the TechTreeViewer for the corresponding topic.
* @param action Pointer to an action.
*/
void NewResearchListState::onOpenTechTreeViewer(Action *)
{
	_lstScroll = _lstResearch->getScroll();
	const RuleResearch *selectedTopic = _projects[_lstResearch->getSelectedRow()];
	_game->pushState(new TechTreeViewerState(selectedTopic, 0));
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void NewResearchListState::btnOKClick(Action *)
{
	_game->popState();
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void NewResearchListState::btnQuickSearchToggle(Action *action)
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
void NewResearchListState::btnQuickSearchApply(Action *)
{
	fillProjectList(false);
}

/**
 * Updates the research list based on the selected option.
 */
void NewResearchListState::cbxSortChange(Action *)
{
	fillProjectList(false);
}

/**
* Filter to display only new items.
* @param action Pointer to an action.
*/
void NewResearchListState::btnShowOnlyNewClick(Action *)
{
	fillProjectList(false);
}

/**
 * Marks all items as seen
 * @param action Pointer to an action.
 */
void NewResearchListState::btnMarkAllAsSeenClick(Action *)
{
	fillProjectList(true);
}

/**
 * Fills the list with possible ResearchProjects.
 */
void NewResearchListState::fillProjectList(bool markAllAsSeen)
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_projects.clear();
	_lstResearch->clearList();
	// Note: this is the *only* place where this method is called with considerDebugMode = true
	_game->getSavedGame()->getAvailableResearchProjects(_projects, _game->getMod() , _base, true);
	size_t selectedSort = _cbxSort->getSelected();
	if (selectedSort == 1 || (selectedSort == 3 && _sortByCost))
	{
		std::sort(_projects.begin(), _projects.end(), [&](RuleResearch* a, RuleResearch* b) { return a->getCost() < b->getCost(); });
	}
	else if (selectedSort == 2)
	{
		std::sort(_projects.begin(), _projects.end(), [&](RuleResearch* a, RuleResearch* b) { return Unicode::naturalCompare(tr(a->getName()), tr(b->getName())); });
	}
	else
	{
		// sort by list order
		std::sort(_projects.begin(), _projects.end(), [&](RuleResearch* a, RuleResearch* b) { return a->getListOrder() < b->getListOrder(); });
	}
	std::vector<RuleResearch*>::iterator it = _projects.begin();
	int row = 0;
	bool hasUnseen = false;
	while (it != _projects.end())
	{
		// filter
		if (_btnShowOnlyNew->getPressed() || selectedSort == 3)
		{
			if (!_game->getSavedGame()->isResearchRuleStatusNew((*it)->getName()))
			{
				it = _projects.erase(it);
				continue;
			}
		}

		// quick search
		if (!searchString.empty())
		{
			std::string projectName = tr((*it)->getName());
			Unicode::upperCase(projectName);
			if (projectName.find(searchString) == std::string::npos)
			{
				it = _projects.erase(it);
				continue;
			}
		}

		// EXPLANATION
		// -----------
		// Projects with "requires" can only be discovered/researched indirectly
		//  - this is because we can't reliably determine if they are unlocked or not
		// Example:
		//  - Alien Origins + Alien Leader => ALIEN_LEADER_PLUS is discovered
		//  - Alien Leader + Alien Origins => ALIEN_LEADER_PLUS is NOT discovered (you need to research another alien leader/commander)
		// If we wanted to allow also direct research of projects with "requires",
		// we would need to implement a slightly more complicated unlocking algorithm
		// and more importantly, we would need to remember the list of unlocked topics
		// in the save file (currently this is not done, the list is calculated on-the-fly).
		// Summary:
		//  - it would be possible to remove this condition, but more refactoring would be needed
		//  - for now, handling "requires" via zero-cost helpers (e.g. STR_LEADER_PLUS)... is enough
		if ((*it)->getRequirements().empty())
		{
			_lstResearch->addRow(1, tr((*it)->getName()).c_str());
			if (markAllAsSeen)
			{
				// mark all (new) research items as normal
				_game->getSavedGame()->setResearchRuleStatus((*it)->getName(), RuleResearch::RESEARCH_STATUS_NORMAL);
			}
			else if (_game->getSavedGame()->isResearchRuleStatusNew((*it)->getName()))
			{
				_lstResearch->setRowColor(row, _colorNew);
				hasUnseen = true;
			}
			row++;
			++it;
		}
		else
		{
			it = _projects.erase(it);
		}
	}

	std::string label = tr("STR_SHOW_ONLY_NEW");
	_btnShowOnlyNew->setText((hasUnseen ? "* " : "") + label);
	if (_lstScroll > 0)
	{
		_lstResearch->scrollTo(_lstScroll);
		_lstScroll = 0;
	}
}

}
