#pragma once
/*
 * Copyright 2010-2019 OpenXcom Developers.
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
class RuleManufacture;

/**
 * Global Manufacture screen that provides overview
 * of the ongoing manufacturing operations in all the bases.
 */
class GlobalManufactureState : public State
{
private:
	TextButton *_btnOk;
	Window *_window;
	Text *_txtTitle, *_txtAvailable, *_txtAllocated, *_txtSpace, *_txtFunds, *_txtItem, *_txtEngineers, *_txtProduced, *_txtCost, *_txtTimeLeft;
	TextList *_lstManufacture;

	std::vector<Base*> _bases;
	std::vector<const RuleManufacture*> _topics;
	bool _openedFromBasescape;
public:
	/// Creates the GlobalManufacture state.
	GlobalManufactureState(bool openedFromBasescape);
	/// Cleans up the GlobalManufacture state.
	~GlobalManufactureState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Production list.
	void onSelectBase(Action *action);
	void onOpenTechTreeViewer(Action *action);
	/// Updates the production list.
	void init() override;
	/// Fills the list with Productions from all bases.
	void fillProductionList();
};

}
