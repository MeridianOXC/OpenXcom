#pragma once
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
#include "../Engine/State.h"
#include <vector>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextEdit;
class TextList;
class ArrowButton;
struct ResearchDiaryEntry;

/// Research diary sorting modes.
enum ReserachDiarySort
{
	RESEARCH_DIARY_SORT_NONE,
	RESEARCH_DIARY_SORT_NAME_ASC,
	RESEARCH_DIARY_SORT_NAME_DESC,
	RESEARCH_DIARY_SORT_DATE_ASC,
	RESEARCH_DIARY_SORT_DATE_DESC
};

struct TranslatedResearchDiaryItem
{
	TranslatedResearchDiaryItem(ResearchDiaryEntry* _diaryEntry, const std::string& _name, const std::string& _upperName, const std::string& _date, int _sortOrder)
		: diaryEntry(_diaryEntry), name(_name), upperName(_upperName), date(_date), sortOrder(_sortOrder)
	{
	}
	ResearchDiaryEntry* diaryEntry;
	std::string name;
	std::string upperName;
	std::string date;
	int sortOrder;
};

/**
 * Research Diary window that displays all discovered research.
 */
class GlobalResearchDiaryState : public State
{
private:
	TextButton *_btnOk;
	TextEdit *_btnQuickSearch;
	Window *_window;
	Text *_txtTitle, *_txtName, *_txtType, *_txtDate;
	TextList *_lstItems;
	ArrowButton *_sortName, *_sortDate;
	Text *_txtTooltip;
	bool _doNotReset;

	std::vector<TranslatedResearchDiaryItem> _itemList;
	std::vector<TranslatedResearchDiaryItem*> _filteredItemList;
	void initList();
	ReserachDiarySort _itemOrder;
	void updateArrows();
public:
	/// Creates the GlobalResearchDiary state.
	GlobalResearchDiaryState();
	/// Cleans up the GlobalResearchDiary state.
	~GlobalResearchDiaryState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action* action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action* action);
	void btnQuickSearchApply(Action* action);
	/// Sets up the item list.
	void init() override;
	/// Sorts the item list.
	void sortList();
	/// Updates the item list.
	virtual void updateList();
	/// Handler for clicking the Name arrow.
	void sortNameClick(Action* action);
	/// Handler for clicking the Date arrow.
	void sortDateClick(Action* action);
	/// Handler for clicking the item list.
	void lstItemLClick(Action* action);
	void lstItemMClick(Action* action);
	/// Handler for moving the mouse over an item.
	void lstItemMouseOver(Action* action);
	/// Handler for moving the mouse outside the list.
	void lstItemMouseOut(Action* action);
};

}
