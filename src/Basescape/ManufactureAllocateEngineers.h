#pragma once
/*
 * Copyright 2010-2022 OpenXcom Developers.
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
#include "SoldierSortUtil.h"
#include <vector>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class ComboBox;
class Base;
class Soldier;
class RuleResearch;
class ManufactureInfoStateFtA;
struct SortFunctor;

/**
* Allocate Engineers screen that lets the player
* pick the personnel to assign to the project.
*/
class ManufactureAllocateEngineers : public State
{
private:
	TextButton* _btnOk;
	Window* _window;
	Text* _txtTitle, * _txtName, * _txtRank, * _txtCraft, * _txtUsed;
	ComboBox* _cbxSortBy;
	TextList* _lstEngineers;
	Base* _base;
	ManufactureInfoStateFtA* _planningProject;
	Uint8 _otherCraftColor;
	std::vector<Soldier*> _origSoldierOrder;
	std::vector<SortFunctor*> _sortFunctors;
	std::vector<int> _scientistsNumbers;
	getStatFn_t _dynGetter;
	/// initializes the display list based on the project soldier's list and the position to display
	void initList(size_t scrl);

public:
	/// Creates the Craft Soldiers state.
	ManufactureAllocateEngineers(Base* base, ManufactureInfoStateFtA* planningProject);
	/// Cleans up the Craft Soldiers state.
	~ManufactureAllocateEngineers();
	/// Handler for changing the sort by combo-box.
	void cbxSortByChange(Action* action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action* action);
	/// Updates the soldiers list.
	void init() override;
	/// Handler for clicking the Soldiers list.
	void lstEngineersClick(Action* action);
	/// Handler for clicking the De-assign All Soldiers button.
	void btnDeassignProjectEngineersClick(Action* action);
};

}
