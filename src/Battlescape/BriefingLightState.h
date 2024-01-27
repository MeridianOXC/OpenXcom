#pragma once
/*
 * Copyright 2010-2016 OpenXcom Developers.
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
class ToggleTextButton;
class Window;
class Text;
class TextList;
class AlienDeployment;

/**
 * Briefing screen which displays basic info
 * about a mission site or an alien base.
 */
class BriefingLightState : public State
{
private:
	TextButton *_btnOk;
	ToggleTextButton *_btnArmors;
	Window *_window;
	Text *_txtTitle, *_txtBriefing, *_txtArmors;
	TextList* _lstArmors;
	std::vector<std::pair<std::string, std::string> > _armorNameList;
	// Checks the starting condition
	void checkStartingCondition(AlienDeployment *deployment);
public:
	/// Creates the BriefingLight state.
	BriefingLightState(AlienDeployment *deployment);
	/// Cleans up the BriefingLight state.
	~BriefingLightState();
	/// Handler for clicking the Ok button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Armors button.
	void btnArmorsClick(Action *action);
	/// Handler for clicking the Armors list.
	void lstArmorsClick(Action* action);
};

}
