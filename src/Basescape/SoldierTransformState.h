#pragma once
/*
 * Copyright 2010-2024 OpenXcom Developers.
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
#include <vector>
#include "../Engine/State.h"

namespace OpenXcom
{

class ArrowButton;
class Base;
class Text;
class TextButton;
class TextEdit;
class TextList;
class Window;

/// Transformation sorting modes.
enum TransformationSort
{
	TRANSFORM_SORT_NONE,
	TRANSFORM_SORT_NAME_ASC,
	TRANSFORM_SORT_NAME_DESC,
};

struct TransformationItem
{
	TransformationItem(const std::string &_type, const std::string &_name) : type(_type), name(_name)
	{
	}
	std::string type;
	std::string name;
};

/**
 * Soldier Transform window that allows performing a transformation on a soldier.
 */
class SoldierTransformState : public State
{
private:
	Base *_base;
	size_t _soldier;

	TextButton *_btnCancel;
	TextEdit *_btnQuickSearch;
	Window *_window;
	Text *_txtTitle, *_txtType;
	TextList *_lstTransformations;
	ArrowButton *_sortName;
	std::vector<TransformationItem> _transformations;
	std::vector<int> _indices;
	TransformationSort _transformationOrder;
	void updateArrows();
public:
	/// Creates the Soldier Transformation state.
	SoldierTransformState(Base* base, size_t soldier);
	/// Cleans up the Soldier Transformation state.
	~SoldierTransformState();
	/// Sorts the transformations list.
	void sortList();
	/// Updates the transformations list.
	void updateList();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action* action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action* action);
	void btnQuickSearchApply(Action* action);
	/// Handler for clicking the transformations list.
	void lstTransformationClick(Action* action);
	/// Handler for clicking the transformations list.
	void lstTransformationClickMiddle(Action* action);
	/// Handler for clicking the Name arrow.
	void sortNameClick(Action* action);
};

}
