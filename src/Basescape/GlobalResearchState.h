#pragma once
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
#include "../Engine/State.h"

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class Base;
class RuleResearch;

/**
 * Global Research screen that provides overview
 * of the ongoing research operations in all the bases.
 */
class GlobalResearchState : public State
{
private:
	TextButton *_btnOk;
	TextButton *_btnDiary;
	Window *_window;
	Text *_txtTitle, *_txtAvailable, *_txtAllocated, *_txtSpace, *_txtProject, *_txtScientists, *_txtProgress;
	TextList *_lstResearch;

	std::vector<Base*> _bases;
	std::vector<const RuleResearch*> _topics;
	bool _openedFromBasescape;
public:
	/// Creates the GlobalResearchState.
	GlobalResearchState(bool openedFromBasescape);
	/// Cleans up the GlobalResearchState.
	~GlobalResearchState();
	/// Handler for clicking the Diary button.
	void btnDiaryClick(Action* action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the ResearchProject list.
	void onSelectBase(Action *action);
	void onOpenTechTreeViewer(Action *action);
	/// Fills the list with ResearchProjects from all bases.
	void fillProjectList();
	/// Updates the research list.
	void init() override;
};

}
