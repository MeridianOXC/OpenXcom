#pragma once
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
#include <vector>
#include "../Engine/State.h"

namespace OpenXcom
{

class Soldier;
class TextButton;
class Window;
class Text;
class TextList;

/**
 * Soldier Diary Light window that allows viewing basic data from the soldier's diary.
 */
class SoldierDiaryLightState : public State
{
private:
	Soldier* _soldier;

	TextButton *_btnOk;
	Window *_window;
	Text *_txtTitle;
	TextList *_lstStats;
public:
	/// Creates the Soldier Diary Light state.
	SoldierDiaryLightState(Soldier* soldier);
	/// Cleans up the Soldier Diary Light state.
	~SoldierDiaryLightState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
};

}
