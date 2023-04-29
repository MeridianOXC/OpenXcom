#pragma once
/*
 * Copyright 2010-2023 OpenXcom Developers.
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

class Base;
class Text;
class TextButton;
class TextList;
class Window;

/**
 * Global Alien Containment screen that provides overview
 * of all the prisoners in all the bases.
 */
class GlobalAlienContainmentState : public State
{
private:
	TextButton *_btnOk;
	Window *_window;
	Text *_txtTitle, *_txtTotalUsed, *_txtTotalInterrogated, *_txtPrisoner, *_txtPrisonerAmount, *_txtPrisonersInterrogated;
	TextList *_lstPrisoners;

	std::vector<std::tuple<std::string, Base*, int> > _topics;
	bool _openedFromBasescape;
public:
	/// Creates the GlobalAlienContainmentState.
	GlobalAlienContainmentState(bool openedFromBasescape);
	/// Cleans up the GlobalAlienContainmentState.
	~GlobalAlienContainmentState();
	/// Updates the prisoner list.
	void init() override;
	/// Fills the list with all prisoners from all bases.
	void fillPrisonerList();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the prisoners list.
	void onSelectBase(Action *action);
	void onOpenTechTreeViewer(Action *action);
};

}
