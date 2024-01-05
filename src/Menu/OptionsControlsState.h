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
#include <string>
#include <vector>
#include "../Engine/OptionInfo.h"
#include "OptionsBaseState.h"

namespace OpenXcom
{

class TextList;

/**
 * Controls screen which allows the user to
 * customize the various key shortcuts in the game.
 */
class OptionsControlsState : public OptionsBaseState
{
private:
	TextButton *_btnOXC, *_btnOXCE, *_btnOTHER;
	TextButton *_owner;
	TextList *_lstControls;
	std::vector<OptionInfo> _controlsGeneral[OPTION_OWNER_MAX];
	std::vector<OptionInfo> _controlsGeo[OPTION_OWNER_MAX];
	std::vector<OptionInfo> _controlsBase[OPTION_OWNER_MAX];
	std::vector<OptionInfo> _controlsBattle[OPTION_OWNER_MAX];
	int _offsetGeneralMin = -1;
	int _offsetGeneralMax = -1;
	int _offsetGeoMin = -1;
	int _offsetGeoMax = -1;
	int _offsetBaseMin = -1;
	int _offsetBaseMax = -1;
	int _offsetBattleMin = -1;
	int _offsetBattleMax = -1;

	int _selected;
	OptionInfo *_selKey;
	Uint8 _colorGroup, _colorSel, _colorNormal;

	void addControls(const std::vector<OptionInfo> &keys);
	OptionInfo *getControl(size_t sel);
	std::string ucWords(std::string str);
public:
	/// Creates the Controls state.
	OptionsControlsState(OptionsOrigin origin);
	/// Cleans up the Controls state.
	~OptionsControlsState();
	/// Refreshes the UI.
	void init() override;
	/// Fills controls list.
	void updateList();
	/// Handler for clicking the Controls list.
	void lstControlsClick(Action *action);
	/// Handler for pressing a key in the Controls list.
	void lstControlsKeyPress(Action *action);
	/// Handler for clicking buttons.
	void btnGroupPress(Action* action);
};

}
