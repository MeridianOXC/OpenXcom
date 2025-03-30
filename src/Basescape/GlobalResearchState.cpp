/*
 * Copyright 2010-2018 OpenXcom Developers.
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
#include "GlobalResearchState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "ResearchState.h"
#include "../Savegame/ResearchProject.h"
#include "../Mod/RuleResearch.h"
#include "TechTreeViewerState.h"
#include "GlobalResearchDiaryState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the GlobalResearch screen.
 */
GlobalResearchState::GlobalResearchState(bool openedFromBasescape) : _openedFromBasescape(openedFromBasescape)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnDiary = new TextButton(148, 16, 8, 176);
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
	setInterface("globalResearchMenu");

	add(_window, "window", "globalResearchMenu");
	add(_btnDiary, "button", "globalResearchMenu");
	add(_btnOk, "button", "globalResearchMenu");
	add(_txtTitle, "text", "globalResearchMenu");
	add(_txtAvailable, "text", "globalResearchMenu");
	add(_txtAllocated, "text", "globalResearchMenu");
	add(_txtSpace, "text", "globalResearchMenu");
	add(_txtProject, "text", "globalResearchMenu");
	add(_txtScientists, "text", "globalResearchMenu");
	add(_txtProgress, "text", "globalResearchMenu");
	add(_lstResearch, "list", "globalResearchMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "globalResearchMenu");

	_btnDiary->setText(tr("STR_RESEARCH_DIARY"));
	_btnDiary->onMouseClick((ActionHandler)&GlobalResearchState::btnDiaryClick);
	_btnDiary->onKeyboardPress((ActionHandler)&GlobalResearchState::btnDiaryClick, Options::keyGeoGlobalResearch);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&GlobalResearchState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&GlobalResearchState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_RESEARCH_OVERVIEW"));

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
	_lstResearch->onMouseClick((ActionHandler)&GlobalResearchState::onSelectBase, SDL_BUTTON_LEFT);
	_lstResearch->onMouseClick((ActionHandler)&GlobalResearchState::onOpenTechTreeViewer, SDL_BUTTON_MIDDLE);
}

/**
 *
 */
GlobalResearchState::~GlobalResearchState()
{
}

/**
 * Displays the Research Diary UI.
 * @param action Pointer to an action.
 */
void GlobalResearchState::btnDiaryClick(Action *)
{
	_game->pushState(new GlobalResearchDiaryState());
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void GlobalResearchState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Goes to the base's research screen when clicking its project
 * @param action Pointer to an action.
 */
void GlobalResearchState::onSelectBase(Action *)
{
	Base *base = _bases[_lstResearch->getSelectedRow()];

	if (base)
	{
		// close this window
		_game->popState();

		// close Research UI (goes back to BaseView)
		if (_openedFromBasescape)
		{
			_game->popState();
		}

		// open new window
		_game->pushState(new ResearchState(base));
	}
}

/**
 * Opens the TechTreeViewer for the corresponding topic.
 * @param action Pointer to an action.
 */
void GlobalResearchState::onOpenTechTreeViewer(Action *)
{
	const RuleResearch *selectedTopic = _topics[_lstResearch->getSelectedRow()];

	if (selectedTopic)
	{
		_game->pushState(new TechTreeViewerState(selectedTopic, 0));
	}
}

/**
 * Updates the research list
 * after going to other screens.
 */
void GlobalResearchState::init()
{
	State::init();
	fillProjectList();
}

/**
 * Fills the list with ResearchProjects from all bases. Also updates total count of available lab space and available/allocated scientists.
 */
void GlobalResearchState::fillProjectList()
{
	_bases.clear();
	_topics.clear();
	_lstResearch->clearList();

	int availableScientists = 0;
	int allocatedScientists = 0;
	int freeLaboratories = 0;

	for (Base *xbase : *_game->getSavedGame()->getBases())
	{
		auto& baseProjects = xbase->getResearch();
		if (!baseProjects.empty() || xbase->getScientists() > 0)
		{
			std::string baseName = xbase->getName(_game->getLanguage());
			_lstResearch->addRow(3, baseName.c_str(), "", "");
			_lstResearch->setRowColor(_lstResearch->getLastRowIndex(), _lstResearch->getSecondaryColor());

			// dummy
			_bases.push_back(0);
			_topics.push_back(0);
		}
		for (const auto* proj : baseProjects)
		{
			std::ostringstream sstr;
			sstr << proj->getAssigned();
			const RuleResearch *r = proj->getRules();

			std::string wstr = tr(r->getName());
			_lstResearch->addRow(3, wstr.c_str(), sstr.str().c_str(), tr(proj->getResearchProgress()).c_str());

			_bases.push_back(xbase);
			_topics.push_back(r);
		}
		if (baseProjects.empty() && xbase->getScientists() > 0)
		{
			_lstResearch->addRow(3, tr("STR_NONE").c_str(), "", "");

			_bases.push_back(xbase);
			_topics.push_back(0);
		}

		availableScientists += xbase->getAvailableScientists();
		allocatedScientists += xbase->getAllocatedScientists();
		freeLaboratories += xbase->getFreeLaboratories();
	}

	_txtAvailable->setText(tr("STR_SCIENTISTS_AVAILABLE").arg(availableScientists));
	_txtAllocated->setText(tr("STR_SCIENTISTS_ALLOCATED").arg(allocatedScientists));
	_txtSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE").arg(freeLaboratories));
}

}
