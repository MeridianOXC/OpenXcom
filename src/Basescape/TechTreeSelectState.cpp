/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "TechTreeSelectState.h"
#include "TechTreeViewerState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleArcScript.h"
#include "../Mod/RuleEventScript.h"
#include "../Mod/RuleMissionScript.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"

namespace OpenXcom
{

/**
 * Initializes all the elements on the UI.
 * @param parent Pointer to parent state.
 */
TechTreeSelectState::TechTreeSelectState(TechTreeViewerState *parent) : _parent(parent)
{
	_screen = false;

	_window = new Window(this, 230, 140, 45, 32);
	_btnQuickSearch = new TextEdit(this, 198, 9, 53, 52);
	_txtTitle = new Text(182, 9, 53, 42);
	_lstTopics = new TextList(198, 80, 53, 62);
	_btnOk = new TextButton(206, 16, 57, 145);

	// Set palette
	setInterface("techTreeSelect");

	add(_window, "window", "techTreeSelect");
	add(_btnQuickSearch, "button", "techTreeSelect");
	add(_txtTitle, "text", "techTreeSelect");
	add(_btnOk, "button", "techTreeSelect");
	add(_lstTopics, "list", "techTreeSelect");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "techTreeSelect");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_AVAILABLE_TOPICS"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&TechTreeSelectState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&TechTreeSelectState::btnOkClick, Options::keyCancel);

	_lstTopics->setColumns(1, 198);
	_lstTopics->setSelectable(true);
	_lstTopics->setBackground(_window);
	_lstTopics->setMargin(0);
	_lstTopics->setAlign(ALIGN_CENTER);
	_lstTopics->onMouseClick((ActionHandler)&TechTreeSelectState::onSelectTopic);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->setDrawBackground(false);
	_btnQuickSearch->onEnter((ActionHandler)&TechTreeSelectState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(true);
	_btnQuickSearch->setFocus(true);

	_btnOk->onKeyboardRelease((ActionHandler)&TechTreeSelectState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
}

TechTreeSelectState::~TechTreeSelectState()
{
}

/**
* Initializes the screen (fills the lists).
*/
void TechTreeSelectState::init()
{
	State::init();
	initLists();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void TechTreeSelectState::btnOkClick(Action *)
{
	_game->popState();
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void TechTreeSelectState::btnQuickSearchToggle(Action *action)
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
void TechTreeSelectState::btnQuickSearchApply(Action *)
{
	initLists();
}

/**
* Shows the filtered topics.
*/
void TechTreeSelectState::initLists()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_firstManufacturingTopicIndex = 0;
	_firstFacilitiesTopicIndex = 0;
	_firstItemTopicIndex = 0;
	_firstCraftTopicIndex = 0;

	_availableTopics.clear();
	_lstTopics->clearList();

	if (searchString.length() < 3)
	{
		_lstTopics->addRow(1, tr("STR_QS_THREE_LETTERS_A").c_str());
		_lstTopics->addRow(1, tr("STR_QS_THREE_LETTERS_B").c_str());
		return;
	}

	int row = 0;
	std::unordered_set<std::string> tmpList;
	if (searchString == "ASCRIPT")
	{
		for (auto& arcScriptId : *_game->getMod()->getArcScriptList())
		{
			auto* arcScript = _game->getMod()->getArcScript(arcScriptId, false);
			if (arcScript)
			{
				for (auto& trigger : arcScript->getResearchTriggers())
				{
					tmpList.insert(trigger.first);
				}
			}
		}
	}
	else if (searchString == "ESCRIPT")
	{
		for (auto& eventScriptId : *_game->getMod()->getEventScriptList())
		{
			auto* eventScript = _game->getMod()->getEventScript(eventScriptId, false);
			if (eventScript)
			{
				for (auto& trigger : eventScript->getResearchTriggers())
				{
					tmpList.insert(trigger.first);
				}
			}
		}
	}
	else if (searchString == "MSCRIPT")
	{
		for (auto& missionScriptId : *_game->getMod()->getMissionScriptList())
		{
			auto* missionScript = _game->getMod()->getMissionScript(missionScriptId, false);
			if (missionScript)
			{
				for (auto& trigger : missionScript->getResearchTriggers())
				{
					tmpList.insert(trigger.first);
				}
			}
		}
	}
	if (!tmpList.empty())
	{
		for (auto& tmp : tmpList)
		{
			_availableTopics.push_back(tmp);
			_lstTopics->addRow(1, tr(tmp).c_str());
			_lstTopics->setRowColor(row, _parent->getResearchColor(tmp));
			++row;
		}
		_firstManufacturingTopicIndex = row;
		_firstFacilitiesTopicIndex = row;
		_firstItemTopicIndex = row;
		_firstCraftTopicIndex = row;
		return;
	}

	for (auto& res : _game->getMod()->getResearchList())
	{
		std::string projectName = tr(res);
		Unicode::upperCase(projectName);
		if (searchString == "SHAZAM")
		{
			if (_parent->isDiscoveredResearch(res))
			{
				continue;
			}
		}
		else if (projectName.find(searchString) == std::string::npos)
		{
			continue;
		}

		_availableTopics.push_back(res);
		_lstTopics->addRow(1, tr(res).c_str());
		_lstTopics->setRowColor(row, _parent->getResearchColor(res));
		++row;
	}

	_firstManufacturingTopicIndex = row;

	for (auto& manuf : _game->getMod()->getManufactureList())
	{
		std::string projectName = tr(manuf);
		Unicode::upperCase(projectName);
		if (searchString == "SHAZAM")
		{
			if (_parent->isDiscoveredManufacture(manuf))
			{
				continue;
			}
		}
		else if (projectName.find(searchString) == std::string::npos)
		{
			continue;
		}

		_availableTopics.push_back(manuf);
		std::ostringstream ss;
		ss << tr(manuf);
		ss << tr("STR_M_FLAG");
		_lstTopics->addRow(1, ss.str().c_str());
		if (!_parent->isDiscoveredManufacture(manuf))
		{
			_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		}
		++row;
	}

	_firstFacilitiesTopicIndex = row;

	for (auto& facType : _game->getMod()->getBaseFacilitiesList())
	{
		std::string facilityName = tr(facType);
		Unicode::upperCase(facilityName);
		if (searchString == "SHAZAM")
		{
			if (_parent->isDiscoveredFacility(facType))
			{
				continue;
			}
		}
		else if (facilityName.find(searchString) == std::string::npos)
		{
			continue;
		}

		_availableTopics.push_back(facType);
		std::ostringstream ss;
		ss << tr(facType);
		ss << tr("STR_F_FLAG");
		_lstTopics->addRow(1, ss.str().c_str());
		if (!_parent->isDiscoveredFacility(facType))
		{
			_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		}
		++row;
	}

	_firstItemTopicIndex = row;

	for (auto& itemType : _game->getMod()->getItemsList())
	{
		if (!_parent->isProtectedItem(itemType))
		{
			// items that are not protected at all are irrelevant for the Tech Tree Viewer!
			continue;
		}
		std::string itemName = tr(itemType);
		Unicode::upperCase(itemName);
		if (searchString == "SHAZAM")
		{
			if (_parent->isProtectedAndDiscoveredItem(itemType))
			{
				continue;
			}
		}
		else if (itemName.find(searchString) == std::string::npos)
		{
			continue;
		}

		_availableTopics.push_back(itemType);
		std::ostringstream ss;
		ss << tr(itemType);
		ss << tr("STR_I_FLAG");
		_lstTopics->addRow(1, ss.str().c_str());
		if (!_parent->isProtectedAndDiscoveredItem(itemType))
		{
			_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		}
		++row;
	}

	_firstCraftTopicIndex = row;

	for (auto& craftType : _game->getMod()->getCraftsList())
	{
		std::string craftName = tr(craftType);
		Unicode::upperCase(craftName);
		if (searchString == "SHAZAM")
		{
			if (_parent->isDiscoveredCraft(craftType))
			{
				continue;
			}
		}
		else if (craftName.find(searchString) == std::string::npos)
		{
			continue;
		}

		_availableTopics.push_back(craftType);
		std::ostringstream ss;
		ss << tr(craftType);
		ss << tr("STR_C_FLAG");
		_lstTopics->addRow(1, ss.str().c_str());
		if (!_parent->isDiscoveredCraft(craftType))
		{
			_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		}
		++row;
	}
}

/**
* Selects the topic.
* @param action Pointer to an action.
*/
void TechTreeSelectState::onSelectTopic(Action *)
{
	size_t index = _lstTopics->getSelectedRow();
	if (index >= _availableTopics.size())
		return;

	const std::string selectedTopic = _availableTopics[index];

	TTVMode topicType = TTV_RESEARCH;
	if (index >= _firstCraftTopicIndex)
	{
		topicType = TTV_CRAFTS;
	}
	else if (index >= _firstItemTopicIndex)
	{
		topicType = TTV_ITEMS;
	}
	else if (index >= _firstFacilitiesTopicIndex)
	{
		topicType = TTV_FACILITIES;
	}
	else if (index >= _firstManufacturingTopicIndex)
	{
		topicType = TTV_MANUFACTURING;
	}

	_parent->setSelectedTopic(selectedTopic, topicType);

	_game->popState();
}

}
