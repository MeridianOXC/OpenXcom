/*
 * Copyright 2010-2025 OpenXcom Developers.
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
#include "GlobalResearchDiaryState.h"
#include <sstream>
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Savegame/ResearchDiary.h"
#include "../Savegame/SavedGame.h"
#include "../Mod/RuleResearch.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "TechTreeViewerState.h"
#include <algorithm>
#include <locale>

namespace OpenXcom
{

struct compareItemName
{
	bool operator()(const TranslatedResearchDiaryItem* a, const TranslatedResearchDiaryItem* b) const
	{
		return Unicode::naturalCompare(a->name, b->name);
	}
};

struct compareItemSortOrder
{
	bool operator()(const TranslatedResearchDiaryItem* a, const TranslatedResearchDiaryItem* b) const
	{
		return a->sortOrder < b->sortOrder;
	}
};


/**
 * Initializes all the elements in the Research Diary window.
 */
GlobalResearchDiaryState::GlobalResearchDiaryState() : _doNotReset(false)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 19);
	_btnOk = new TextButton(304, 16, 8, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtName = new Text(142, 9, 10, 29);
	_txtType = new Text(48, 9, 193, 29);
	_txtDate = new Text(54, 9, 242, 29);
	_lstItems = new TextList(288, 96, 8, 38);
	_sortName = new ArrowButton(ARROW_NONE, 11, 8, 10, 29);
	_sortDate = new ArrowButton(ARROW_NONE, 11, 8, 242, 29);
	_txtTooltip = new Text(304, 40, 8, 135);

	// Set palette
	setInterface("globalResearchDiary");

	add(_window, "window", "globalResearchDiary");
	add(_btnQuickSearch, "button", "globalResearchDiary");
	add(_btnOk, "button", "globalResearchDiary");
	add(_txtTitle, "text", "globalResearchDiary");
	add(_txtName, "text", "globalResearchDiary");
	add(_txtType, "text", "globalResearchDiary");
	add(_txtDate, "text", "globalResearchDiary");
	add(_lstItems, "list", "globalResearchDiary");
	add(_sortName, "text", "globalResearchDiary");
	add(_sortDate, "text", "globalResearchDiary");
	add(_txtTooltip, "text", "globalResearchDiary");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "globalResearchDiary");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&GlobalResearchDiaryState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&GlobalResearchDiaryState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&GlobalResearchDiaryState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_RESEARCH_DIARY"));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtType->setText(tr("STR_TYPE"));
	_txtType->setAlign(ALIGN_CENTER);

	_txtDate->setText(tr("STR_DATE_UC"));

	_lstItems->setColumns(3, 204, 28, 54);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(2);
	_lstItems->onMouseClick((ActionHandler)&GlobalResearchDiaryState::lstItemLClick, SDL_BUTTON_LEFT);
	_lstItems->onMouseClick((ActionHandler)&GlobalResearchDiaryState::lstItemMClick, SDL_BUTTON_MIDDLE);
	_lstItems->onMouseOver((ActionHandler)&GlobalResearchDiaryState::lstItemMouseOver);
	_lstItems->onMouseOut((ActionHandler)&GlobalResearchDiaryState::lstItemMouseOut);

	_sortName->setX(_sortName->getX() + _txtName->getTextWidth() + 4);
	_sortName->onMouseClick((ActionHandler)&GlobalResearchDiaryState::sortNameClick);

	_sortDate->setX(_sortDate->getX() + _txtDate->getTextWidth() + 4);
	_sortDate->onMouseClick((ActionHandler)&GlobalResearchDiaryState::sortDateClick);

	_itemOrder = RESEARCH_DIARY_SORT_NONE;

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&GlobalResearchDiaryState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(Options::oxceQuickSearchButton);

	_btnOk->onKeyboardRelease((ActionHandler)&GlobalResearchDiaryState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	// translate only once
	auto& vec = _game->getSavedGame()->getResearchDiary();
	int sortOrder = vec.size();
	for (auto it = vec.rbegin(); it != vec.rend(); ++it)
	{
		auto* researchDiaryEntry = (*it);

		std::string translation = tr(researchDiaryEntry->research->getName());
		std::string upper = translation;
		Unicode::upperCase(upper);

		std::ostringstream ss3;
		ss3 << researchDiaryEntry->year << "-";
		if (researchDiaryEntry->month < 10) ss3 << "0";
		ss3 << researchDiaryEntry->month << "-";
		if (researchDiaryEntry->day < 10) ss3 << "0";
		ss3 << researchDiaryEntry->day;

		_itemList.push_back(TranslatedResearchDiaryItem(researchDiaryEntry, translation, upper, ss3.str(), sortOrder));
		sortOrder--;
	}

	_txtTooltip->setVerticalAlign(ALIGN_MIDDLE);
	_txtTooltip->setWordWrap(true);
}

/**
 *
 */
GlobalResearchDiaryState::~GlobalResearchDiaryState()
{

}

/**
 * Returns to the previous screen.
 */
void GlobalResearchDiaryState::btnOkClick(Action *)
{
	_game->popState();
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void GlobalResearchDiaryState::btnQuickSearchToggle(Action *action)
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
*/
void GlobalResearchDiaryState::btnQuickSearchApply(Action *)
{
	initList();
}

/**
 * Reloads the item list.
 */
void GlobalResearchDiaryState::initList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	// clear everything
	_filteredItemList.clear();

	// find relevant items
	for (auto& origItem : _itemList)
	{
		// quick search
		if (!searchString.empty())
		{
			if (origItem.upperName.find(searchString) == std::string::npos && origItem.date.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		_filteredItemList.push_back(&origItem);
	}

	sortList();
}

/**
 * Refreshes the item list.
 */
void GlobalResearchDiaryState::init()
{
	State::init();

	// coming back from TechTreeViewer/Ufopedia
	if (_doNotReset)
	{
		_doNotReset = false;
		return;
	}

	initList();
}

/**
 * Updates the sorting arrows based on the current setting.
 */
void GlobalResearchDiaryState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	_sortDate->setShape(ARROW_NONE);
	switch (_itemOrder)
	{
	case RESEARCH_DIARY_SORT_NONE:
		_sortDate->setShape(ARROW_SMALL_DOWN); // it is already sorted by date in the original vector
		break;
	case RESEARCH_DIARY_SORT_NAME_ASC:
		_sortName->setShape(ARROW_SMALL_UP);
		break;
	case RESEARCH_DIARY_SORT_NAME_DESC:
		_sortName->setShape(ARROW_SMALL_DOWN);
		break;
	case RESEARCH_DIARY_SORT_DATE_ASC:
		_sortDate->setShape(ARROW_SMALL_UP);
		break;
	case RESEARCH_DIARY_SORT_DATE_DESC:
		_sortDate->setShape(ARROW_SMALL_DOWN);
		break;
	default:
		break;
	}
}

/**
 * Sorts the list.
 */
void GlobalResearchDiaryState::sortList()
{
	updateArrows();

	switch (_itemOrder)
	{
	case RESEARCH_DIARY_SORT_NONE:
		break;
	case RESEARCH_DIARY_SORT_NAME_ASC:
		std::stable_sort(_filteredItemList.begin(), _filteredItemList.end(), compareItemName());
		break;
	case RESEARCH_DIARY_SORT_NAME_DESC:
		std::stable_sort(_filteredItemList.rbegin(), _filteredItemList.rend(), compareItemName());
		break;
	case RESEARCH_DIARY_SORT_DATE_ASC:
		std::stable_sort(_filteredItemList.begin(), _filteredItemList.end(), compareItemSortOrder());
		break;
	case RESEARCH_DIARY_SORT_DATE_DESC:
		std::stable_sort(_filteredItemList.rbegin(), _filteredItemList.rend(), compareItemSortOrder());
		break;
	}

	updateList();
}

/**
 * Updates the list with the current diary items.
 */
void GlobalResearchDiaryState::updateList()
{
	_lstItems->clearList();

	for (const auto* item : _filteredItemList)
	{
		std::ostringstream ss2;
		ss2 << (int)item->diaryEntry->source.type;
		_lstItems->addRow(3, item->name.c_str(), ss2.str().c_str(), item->date.c_str());
	}
}

/**
 * Sorts the items by name.
 */
void GlobalResearchDiaryState::sortNameClick(Action *)
{
	_itemOrder = _itemOrder == RESEARCH_DIARY_SORT_NAME_ASC ? RESEARCH_DIARY_SORT_NAME_DESC : RESEARCH_DIARY_SORT_NAME_ASC;
	sortList();
}

/**
 * Sorts the items by date.
 */
void GlobalResearchDiaryState::sortDateClick(Action *)
{
	_itemOrder = _itemOrder == RESEARCH_DIARY_SORT_DATE_ASC ? RESEARCH_DIARY_SORT_DATE_DESC : RESEARCH_DIARY_SORT_DATE_ASC;
	sortList();
}

/**
 * Handles mouse L-clicks.
 * @param action Pointer to an action.
 */
void GlobalResearchDiaryState::lstItemLClick(Action* action)
{
	auto* selectedTopic = _filteredItemList[_lstItems->getSelectedRow()]->diaryEntry->research;
	_doNotReset = true;
	_game->pushState(new TechTreeViewerState(selectedTopic, 0));
}

/**
 * Handles mouse M-clicks.
 * @param action Pointer to an action.
 */
void GlobalResearchDiaryState::lstItemMClick(Action* action)
{
	auto* selectedTopic = _filteredItemList[_lstItems->getSelectedRow()]->diaryEntry->research;
	_doNotReset = true;
	Ufopaedia::openArticle(_game, selectedTopic->getName());
}

void GlobalResearchDiaryState::lstItemMouseOver(Action *)
{
	size_t sel = _lstItems->getSelectedRow();
	if (sel < _filteredItemList.size())
	{
		auto item = _filteredItemList[sel];

		std::string sourceNameTranslated;
		if (item->diaryEntry->source.type != BASE) sourceNameTranslated = tr(item->diaryEntry->source.name);

		std::string descTemplate{ item->diaryEntry->source.getTypeString() };
		std::string desc = tr(descTemplate).arg(item->diaryEntry->research->getName()).arg(item->diaryEntry->source.name).arg(sourceNameTranslated);

		_txtTooltip->setText(desc);
	}
	else
	{
		_txtTooltip->setText("");
	}
}

void GlobalResearchDiaryState::lstItemMouseOut(Action *)
{
	_txtTooltip->setText("");
}

}
