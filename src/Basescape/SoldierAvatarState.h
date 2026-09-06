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
#include "../Savegame/SoldierAvatar.h"

namespace OpenXcom
{

class BattleUnit;
class TextButton;
class Window;
class Text;
class TextList;
class Soldier;
class SoldierAvatar;

/**
 * Select Avatar window that allows changing
 * of the soldier's avatar.
 */
class SoldierAvatarState : public State
{
private:
	BattleUnit* _bu;

	TextButton *_btnCancel, *_btnOk;
	Window *_window;
	Text *_txtTitle, *_txtType;
	Surface *_soldierSurface;
	TextList *_lstAvatar;
	std::vector<SoldierAvatar> _avatars;
	SoldierAvatar _origAvatar;
	/// Creates the avatar preview
	void initPreview(Soldier *s);
public:
	/// Creates the Soldier Avatar state.
	SoldierAvatarState(BattleUnit* bu);
	/// Cleans up the Soldier Avatar state.
	~SoldierAvatarState();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Avatar list.
	void lstAvatarClick(Action *action);
};

}
