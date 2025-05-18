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
#include "ResearchState.h"
#include <sstream>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "NewResearchListState.h"
#include "GlobalResearchState.h"
#include "../Savegame/ResearchProject.h"
#include "../Mod/RuleResearch.h"
#include "ResearchInfoState.h"
#include "TechTreeViewerState.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Initializes all the elements in the Research screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
ResearchState::ResearchState(Base *base) : _base(base)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnNew = new TextButton(148, 16, 8, 176);
	_btnOk = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtAvailable = new Text(150, 9, 10, 24);
	_txtAllocated = new Text(150, 9, 160, 24);
	_txtSpace = new Text(300, 9, 10, 34);
	_txtProject = new Text(110, 17, 10, 44);
	_txtScientists = new Text(106, 17, 120, 44);
	_txtProgress = new Text(84, 9, 226, 44);
	_lstResearch = new TextList(288, 112, 8, 62);

	// Set palette
	setInterface("researchMenu");

	add(_window, "window", "researchMenu");
	add(_btnNew, "button", "researchMenu");
	add(_btnOk, "button", "researchMenu");
	add(_txtTitle, "text", "researchMenu");
	add(_txtAvailable, "text", "researchMenu");
	add(_txtAllocated, "text", "researchMenu");
	add(_txtSpace, "text", "researchMenu");
	add(_txtProject, "text", "researchMenu");
	add(_txtScientists, "text", "researchMenu");
	add(_txtProgress, "text", "researchMenu");
	add(_lstResearch, "list", "researchMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "researchMenu");

	_btnNew->setText(tr("STR_NEW_PROJECT"));
	_btnNew->onMouseClick((ActionHandler)&ResearchState::btnNewClick);
	_btnNew->onKeyboardPress((ActionHandler)&ResearchState::btnNewClick, Options::keyToggleQuickSearch);
	_btnNew->onKeyboardPress((ActionHandler)&ResearchState::onCurrentGlobalResearchClick, Options::keyGeoGlobalResearch);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ResearchState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ResearchState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_RESEARCH"));

	_txtProject->setWordWrap(true);
	_txtProject->setText(tr("STR_RESEARCH_PROJECT"));

	_txtScientists->setWordWrap(true);
	_txtScientists->setText(tr("STR_SCIENTISTS_ALLOCATED_UC"));

	_txtProgress->setText(tr("STR_PROGRESS"));

	_lstResearch->setArrowColumn(192, ARROW_VERTICAL);
	_lstResearch->setColumns(3, 158, 58, 70);
	_lstResearch->setSelectable(true);
	_lstResearch->setBackground(_window);
	_lstResearch->setMargin(2);
	_lstResearch->setWordWrap(true);
	_lstResearch->onLeftArrowClick((ActionHandler)&ResearchState::lstResearchLeftArrowClick);
	_lstResearch->onRightArrowClick((ActionHandler)&ResearchState::lstResearchRightArrowClick);
	_lstResearch->onMouseClick((ActionHandler)&ResearchState::onSelectProject, SDL_BUTTON_LEFT);
	_lstResearch->onMouseClick((ActionHandler)&ResearchState::onOpenTechTreeViewer, SDL_BUTTON_MIDDLE);
	//_lstResearch->onMousePress((ActionHandler)&ResearchState::lstResearchMousePress);
}

/**
 *
 */
ResearchState::~ResearchState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ResearchState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ResearchState::btnNewClick(Action *)
{
	bool sortByCost = _game->isCtrlPressed() && _game->isAltPressed();
	_game->pushState(new NewResearchListState(_base, sortByCost));
}

/**
 * Displays the list of possible ResearchProjects.
 * @param action Pointer to an action.
 */
void ResearchState::onSelectProject(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstResearch->getArrowsLeftEdge() && mx < _lstResearch->getArrowsRightEdge())
	{
		return;
	}

	const std::vector<ResearchProject *> & baseProjects(_base->getResearch());
	_game->pushState(new ResearchInfoState(_base, baseProjects[_lstResearch->getSelectedRow()]));
}

/**
* Opens the TechTreeViewer for the corresponding topic.
* @param action Pointer to an action.
*/
void ResearchState::onOpenTechTreeViewer(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstResearch->getArrowsLeftEdge() && mx < _lstResearch->getArrowsRightEdge())
	{
		return;
	}

	const std::vector<ResearchProject *> & baseProjects(_base->getResearch());
	const RuleResearch *selectedTopic = baseProjects[_lstResearch->getSelectedRow()]->getRules();
	_game->pushState(new TechTreeViewerState(selectedTopic, 0));
}

/**
 * Handles the mouse-wheels.
 * @param action Pointer to an action.
 */
void ResearchState::lstResearchMousePress(Action *action)
{
	/*
	if (!_lstResearch->isInsideNoScrollArea(action->getAbsoluteXMouse()))
	{
		return;
	}

	int change = Options::oxceResearchScrollSpeed;
	if (_game->isCtrlPressed())
		change = Options::oxceResearchScrollSpeedWithCtrl;

	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		change = std::min(change, _base->getAvailableScientists());
		change = std::min(change, _base->getFreeLaboratories());
		if (change > 0)
		{
			ResearchProject *selectedProject = _base->getResearch()[_lstResearch->getSelectedRow()];
			selectedProject->setAssigned(selectedProject->getAssigned() + change);
			_base->setScientists(_base->getScientists() - change);
			fillProjectList(_lstResearch->getScroll());
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		ResearchProject *selectedProject = _base->getResearch()[_lstResearch->getSelectedRow()];
		change = std::min(change, selectedProject->getAssigned());
		if (change > 0)
		{
			selectedProject->setAssigned(selectedProject->getAssigned() - change);
			_base->setScientists(_base->getScientists() + change);
			fillProjectList(_lstResearch->getScroll());
		}
	}
	*/
}

/**
 * Opens the Current Global Research UI.
 * @param action Pointer to an action.
 */
void ResearchState::onCurrentGlobalResearchClick(Action *)
{
	_game->pushState(new GlobalResearchState(true));
}
/**
 * Updates the research list
 * after going to other screens.
 */
void ResearchState::init()
{
	State::init();
	fillProjectList(0);

	if (Options::oxceResearchScrollSpeed > 0 || Options::oxceResearchScrollSpeedWithCtrl > 0)
	{
		// 175 +/- 20
		_lstResearch->setNoScrollArea(_txtAllocated->getX() - 5, _txtAllocated->getX() + 35);
	}
	else
	{
		_lstResearch->setNoScrollArea(0, 0);
	}
}

/**
 * Fills the list with Base ResearchProjects. Also updates count of available lab space and available/allocated scientists.
 */
void ResearchState::fillProjectList(size_t scrl)
{
	_lstResearch->clearList();
	for (const auto* proj : _base->getResearch())
	{
		std::ostringstream sstr;
		sstr << proj->getAssigned();
		const RuleResearch *r = proj->getRules();

		std::string wstr = tr(r->getName());
		_lstResearch->addRow(3, wstr.c_str(), sstr.str().c_str(), tr(proj->getResearchProgress()).c_str());
	}
	_txtAvailable->setText(tr("STR_SCIENTISTS_AVAILABLE").arg(_base->getAvailableScientists()));
	_txtAllocated->setText(tr("STR_SCIENTISTS_ALLOCATED").arg(_base->getAllocatedScientists()));
	_txtSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE").arg(_base->getFreeLaboratories()));

	if (scrl)
		_lstResearch->scrollTo(scrl);
}

/**
 * Reorders a research topic up.
 * @param action Pointer to an action.
 */
void ResearchState::lstResearchLeftArrowClick(Action* action)
{
	unsigned int row = _lstResearch->getSelectedRow();
	if (row > 0)
	{
		if (_game->isLeftClick(action, true))
		{
			moveTopicUp(action, row);
		}
		else if (_game->isRightClick(action, true))
		{
			moveTopicUp(action, row, true);
		}
	}
}

/**
 * Moves a research topic up on the list.
 * @param action Pointer to an action.
 * @param row Selected research topic row.
 * @param max Move the research topic to the top?
 */
void ResearchState::moveTopicUp(Action* action, unsigned int row, bool max)
{
	auto& topics = _base->getResearch();
	if (max)
	{
		auto* r = topics.at(row);
		topics.erase(topics.begin() + row);
		topics.insert(topics.begin(), r);
	}
	else
	{
		std::swap(topics[row], topics[row - 1]);
		if (row != _lstResearch->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() - static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstResearch->scrollUp(false);
		}
	}
	fillProjectList(_lstResearch->getScroll());
}

/**
 * Reorders a research topic down.
 * @param action Pointer to an action.
 */
void ResearchState::lstResearchRightArrowClick(Action* action)
{
	unsigned int row = _lstResearch->getSelectedRow();
	size_t numTopics = _base->getResearch().size();
	if (0 < numTopics && INT_MAX >= numTopics && row < numTopics - 1)
	{
		if (_game->isLeftClick(action, true))
		{
			moveTopicDown(action, row);
		}
		else if (_game->isRightClick(action, true))
		{
			moveTopicDown(action, row, true);
		}
	}
}

/**
 * Moves a research topic down on the list.
 * @param action Pointer to an action.
 * @param row Selected research topic row.
 * @param max Move the research topic to the bottom?
 */
void ResearchState::moveTopicDown(Action* action, unsigned int row, bool max)
{
	auto& topics = _base->getResearch();
	if (max)
	{
		auto* r = topics.at(row);
		topics.erase(topics.begin() + row);
		topics.insert(topics.end(), r);
	}
	else
	{
		std::swap(topics[row], topics[row + 1]);
		if (row != _lstResearch->getVisibleRows() - 1 + _lstResearch->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() + static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstResearch->scrollDown(false);
		}
	}
	fillProjectList(_lstResearch->getScroll());
}

}
