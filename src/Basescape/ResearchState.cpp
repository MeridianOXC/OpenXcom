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
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Basescape/ScientistsState.h"
#include "NewResearchListState.h"
#include "GlobalResearchState.h"
#include "../Savegame/ResearchProject.h"
#include "../Mod/RuleResearch.h"
#include "ResearchInfoState.h"
#include "ResearchInfoStateFtA.h"
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
	_ftaUi = _game->getMod()->getIsFTAGame();
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	if (_ftaUi)
	{
		_btnOk = new TextButton(96, 16, 216, 176);
		_btnNew = new TextButton(96, 16, 8, 176);
	}
	else
	{
		_btnOk = new TextButton(148, 16, 164, 176);
		_btnNew = new TextButton(148, 16, 8, 176);
	}
	_btnScientists = new TextButton(96, 16, 112, 176);
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
	add(_btnScientists, "button", "researchMenu");
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

	_btnScientists->setText(tr("STR_SCIENTISTS_LC"));
	_btnScientists->onMouseClick((ActionHandler)&ResearchState::btnScientistsClick);
	_btnScientists->setVisible(_ftaUi);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_RESEARCH"));

	_txtProject->setWordWrap(true);
	_txtProject->setText(tr("STR_RESEARCH_PROJECT"));

	_txtScientists->setWordWrap(true);
	_txtScientists->setText(tr("STR_SCIENTISTS_ALLOCATED_UC"));

	_txtProgress->setText(tr("STR_PROGRESS"));

	_lstResearch->setColumns(3, 158, 58, 70);
	_lstResearch->setSelectable(true);
	_lstResearch->setBackground(_window);
	_lstResearch->setMargin(2);
	_lstResearch->setWordWrap(true);
	_lstResearch->onMouseClick((ActionHandler)&ResearchState::onSelectProject, SDL_BUTTON_LEFT);
	_lstResearch->onMouseClick((ActionHandler)&ResearchState::onOpenTechTreeViewer, SDL_BUTTON_MIDDLE);
	_lstResearch->onMousePress((ActionHandler)&ResearchState::lstResearchMousePress);
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
 * Displays the list of possible ResearchProjects.
 * @param action Pointer to an action.
 */
void ResearchState::btnNewClick(Action *)
{
	bool sortByCost = _game->isCtrlPressed() && _game->isAltPressed();
	_game->pushState(new NewResearchListState(_base, sortByCost));
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ResearchState::btnScientistsClick(Action *action)
{
	_game->pushState(new ScientistsState(_base));
}

/**
 * Opens state with selected Research Project
 * @param action Pointer to an action.
 */
void ResearchState::onSelectProject(Action *)
{
	const std::vector<ResearchProject *> & baseProjects(_base->getResearch());
	auto project = baseProjects[_lstResearch->getSelectedRow()];
	if (_ftaUi)
	{
		_game->pushState(new ResearchInfoStateFtA(_base, project));
	}
	else
	{
		_game->pushState(new ResearchInfoState(_base, project));
	}
}

/**
* Opens the TechTreeViewer for the corresponding topic.
* @param action Pointer to an action.
*/
void ResearchState::onOpenTechTreeViewer(Action *)
{
	if (_game->getMod()->getIsResearchTreeDisabled() && !_game->getSavedGame()->getDebugMode())
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
		change = std::min(change, _base->getFreeLaboratories(_ftaUi));
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
	const std::vector<ResearchProject *> & baseProjects(_base->getResearch());
	_lstResearch->clearList();
	for (std::vector<ResearchProject *>::const_iterator iter = baseProjects.begin(); iter != baseProjects.end(); ++iter)
	{
		std::ostringstream sstr, sspr;
		if (_ftaUi)
		{
			unsigned int n = 0;
			for (auto s : *_base->getSoldiers())
			{
				if (s->getResearchProject() == (*iter))
				{
					n++;
				}
			}
			sstr << n;

			float progress = static_cast<float>((*iter)->getSpent()) / (*iter)->getRules()->getCost();
			if (n == 0)
			{
				sspr << tr("STR_NONE");
			}
			else if (progress <= 0.25f)
			{
				sspr << tr("STR_UNKNOWN");
			}
			else if (progress <= 0.40f)
			{
				sspr << tr("STR_POOR");
			}
			else if (progress <= 0.65f)
			{
				sspr << tr("STR_AVERAGE");
			}
			else if (progress <= 0.85f)
			{
				sspr << tr("STR_GOOD");
			}
			else
			{
				sspr << tr("STR_EXCELLENT");
			}
		}
		else
		{
			sstr << (*iter)->getAssigned();
			sspr << tr((*iter)->getResearchProgress());
		}
		const RuleResearch *r = (*iter)->getRules();
		std::string wstr = tr(r->getName());
		_lstResearch->addRow(3, wstr.c_str(), sstr.str().c_str(), sspr.str().c_str());
	}

	if (_ftaUi)
	{
		auto recovery = _base->getSumRecoveryPerDay();
		unsigned int freeScientists = 0, busyScientists = 0;
		bool isBusy = false, isFree = false;
		for (auto s : _base->getPersonnel(ROLE_SCIENTIST))
		{
			s->getCurrentDuty(_game->getLanguage(), recovery, isBusy, isFree, LAB);
			if (!isBusy || isFree)
			{
				freeScientists++;
			}
			if (s->getResearchProject())
			{
				busyScientists++;
			}
		}
		_txtAvailable->setText(tr("STR_SCIENTISTS_AVAILABLE").arg(freeScientists));
		_txtAllocated->setText(tr("STR_SCIENTISTS_ALLOCATED").arg(busyScientists));
	}
	else
	{
		_txtAvailable->setText(tr("STR_SCIENTISTS_AVAILABLE").arg(_base->getAvailableScientists()));
		_txtAllocated->setText(tr("STR_SCIENTISTS_ALLOCATED").arg(_base->getAllocatedScientists()));
	}

	_txtSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE").arg(_base->getFreeLaboratories(_ftaUi)));

	if (scrl)
		_lstResearch->scrollTo(scrl);
}

}
