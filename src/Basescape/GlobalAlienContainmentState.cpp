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
#include "GlobalAlienContainmentState.h"
#include <sstream>
#include <set>
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
#include "ManageAlienContainmentState.h"
#include "../Savegame/ResearchProject.h"
#include "../Mod/RuleResearch.h"
#include "TechTreeViewerState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the GlobalResearch screen.
 */
GlobalAlienContainmentState::GlobalAlienContainmentState(bool openedFromBasescape) : _openedFromBasescape(openedFromBasescape)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(304, 16, 8, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtAvailable = new Text(150, 9, 10, 24);
	_txtAllocated = new Text(150, 9, 160, 24);
	_txtSpace = new Text(300, 9, 10, 34);
	_txtProject = new Text(110, 17, 10, 44);
	_txtScientists = new Text(106, 17, 120, 44);
	_txtProgress = new Text(84, 9, 226, 44);
	_lstAliens = new TextList(288, 112, 8, 62);

	// Set palette
	setInterface("globalContainmentMenu");

	add(_window, "window", "globalContainmentMenu");
	add(_btnOk, "button", "globalContainmentMenu");
	add(_txtTitle, "text", "globalContainmentMenu");
	add(_txtAvailable, "text", "globalContainmentMenu");
	add(_txtAllocated, "text", "globalContainmentMenu");
	add(_txtSpace, "text", "globalContainmentMenu");
	add(_txtProject, "text", "globalContainmentMenu");
	add(_txtScientists, "text", "globalContainmentMenu");
	add(_txtProgress, "text", "globalContainmentMenu");
	add(_lstAliens, "list", "globalContainmentMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "globalContainmentMenu");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&GlobalAlienContainmentState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&GlobalAlienContainmentState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PRISONER_OVERVIEW"));

	_txtProject->setWordWrap(true);
	_txtProject->setText(tr("STR_RESEARCH_PROJECT"));

	_txtScientists->setWordWrap(true);
	_txtScientists->setText(tr("STR_SCIENTISTS_ALLOCATED_UC"));

	_txtProgress->setText(tr("STR_PROGRESS"));

	_lstAliens->setColumns(3, 158, 58, 70);
	_lstAliens->setSelectable(true);
	_lstAliens->setBackground(_window);
	_lstAliens->setMargin(2);
	_lstAliens->setWordWrap(true);
	_lstAliens->onMouseClick((ActionHandler)&GlobalAlienContainmentState::onSelectBase, SDL_BUTTON_LEFT);
	_lstAliens->onMouseClick((ActionHandler)&GlobalAlienContainmentState::onOpenTechTreeViewer, SDL_BUTTON_MIDDLE);
}

/**
 *
 */
GlobalAlienContainmentState::~GlobalAlienContainmentState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void GlobalAlienContainmentState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Goes to the base's research screen when clicking its project
 * @param action Pointer to an action.
 */
void GlobalAlienContainmentState::onSelectBase(Action *)
{
	auto base = _bases[_lstAliens->getSelectedRow()];

	if (base.first())
	{
		// close this window
		_game->popState();

		// close Containment UI (goes back to BaseView)
		if (_openedFromBasescape)
		{
			_game->popState();
		}

		// open new window 
		_game->pushState(new ManageAlienContainmentState(base.first(), base.second(), OPT_GEOSCAPE));
	}
}

/**
 * Opens the TechTreeViewer for the corresponding topic.
 * @param action Pointer to an action.
 */
void GlobalAlienContainmentState::onOpenTechTreeViewer(Action *)
{
}

/**
 * Updates the prisoner list
 * after going to other screens.
 */
void GlobalAlienContainmentState::init()
{
	State::init();
	fillAlienList();
}

/**
 * Fills the list with prisoners from all bases. Also updates total count of available containment space over all bases
 */
void GlobalAlienContainmentState::fillAlienList()
{
	_bases.clear();
	_topics.clear();
	_lstAliens->clearList();

	int availableScientists = 0;
	int allocatedScientists = 0;
	int freeLaboratories = 0;

	std::set<int> prisonTypes;
    //determine prison types in game
	for(auto& itemType : _game->getMod()->getItemList())
	{
		if(itemType->isAlien())
		{
			prisonTypes.insert(itemType->getPrisonType())
		}
	}
	//start gathering base information
	for (Base *xbase : *_game->getSavedGame()->getBases())
	{
		//find the prisoners under interrogation at the base (i.e. under research)
		std::vector<std::string> researchList;
		for (const auto* proj : _base->getResearch())
		{
			const RuleResearch *research = proj->getRules();
			RuleItem *item = _game->getMod()->getItem(research->getName());
			if (research->needItem() && research->destroyItem() && item && item->isAlien()))
			{
				researchList.push_back(research->getName());
			}
		}
		//walk the prison types
		for(auto prisonType : prisonTypes)
		{	
			//extract the prisoners in holding
			for (auto& itemType : _game->getMod()->getItemsList())
			{
				int qty = _base->getStorageItems()->getItem(itemType);
				RuleItem *rule = _game->getMod()->getItem(itemType, true);
				if (qty > 0 && rule->isAlien() && rule->getPrisonType() == prisonType)
				{
					_qtys.push_back(0);
					_aliens.push_back(itemType);
					std::ostringstream ss;
					ss << qty;
					std::string rqty;
					auto researchIt = std::find(researchList.begin(), researchList.end(), itemType);
					if (researchIt != researchList.end())
					{
						rqty = "1";
						researchList.erase(researchIt);
					}
					else
					{
						rqty = "0";
					}

					_lstAliens->addRow(5, tr(itemType).c_str(), formattedCost.c_str(), ss.str().c_str(), "0", rqty.c_str());
				}
			}

			//old research overview code for reference, will be deleted soon enough
			auto& baseProjects = xbase->getResearch();
			if (!baseProjects.empty())
			{
				std::string baseName = xbase->getName(_game->getLanguage());
				_lstAliens->addRow(3, baseName.c_str(), "", "");
				_lstAliens->setRowColor(_lstAliens->getLastRowIndex(), _lstAliens->getSecondaryColor());

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
				_lstAliens->addRow(3, wstr.c_str(), sstr.str().c_str(), tr(proj->getResearchProgress()).c_str());

				_bases.push_back(xbase);
				_topics.push_back(_game->getMod()->getResearch(r->getName()));
			}
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
