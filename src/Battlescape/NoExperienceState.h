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

class Window;
class Text;
class TextButton;
class TextList;

/**
 * NoExperience window that displays a list of soldiers without any experience points gained in the current mission.
 */
class NoExperienceState : public State
{
private:
	Window *_window;
	Text *_txtTitle;
	TextButton *_btnCancel;
	TextList *_lstSoldiers;
public:
	/// Creates the NoExperience state.
	NoExperienceState();
	/// Cleans up the NoExperience state.
	~NoExperienceState() = default;
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
};

}
