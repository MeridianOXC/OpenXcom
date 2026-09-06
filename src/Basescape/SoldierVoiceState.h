#pragma once
/*
 * Copyright 2010-2026 OpenXcom Developers.
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

enum SoldierVoiceOrigin
{
	SV_GEOSCAPE,
	SV_BATTLESCAPE
};

class BattleUnit;
class Soldier;
class RuleVoiceSet;
class TextButton;
class Window;
class Text;
class TextEdit;
class TextList;
class ArrowButton;

/// Voice sorting modes.
enum VoiceSort
{
	VOICE_SORT_NONE,
	VOICE_SORT_NAME_ASC,
	VOICE_SORT_NAME_DESC,
};

struct VoiceSetItem
{
	VoiceSetItem(const std::string &_type, const std::string &_name, const RuleVoiceSet* _vs) : type(_type), name(_name), vs(_vs)
	{
	}
	std::string type;
	std::string name;
	const RuleVoiceSet* vs = nullptr;
};

/**
 * Select Voice window that allows changing the soldier's voice.
 */
class SoldierVoiceState : public State
{
private:
	BattleUnit* _bu;
	Soldier* _soldier;

	SoldierVoiceOrigin _origin;
	TextButton *_btnCancel;
	TextEdit *_btnQuickSearch;
	Window *_window;
	Text *_txtTitle, *_txtType;
	TextList *_lstVoice;
	ArrowButton *_sortName;
	std::vector<VoiceSetItem> _voices;
	std::vector<int> _indices;
	VoiceSort _voiceOrder;
	void updateArrows();
public:
	/// Creates the Soldier Voice state.
	SoldierVoiceState(BattleUnit* bu, Soldier* soldier, SoldierVoiceOrigin origin);
	/// Cleans up the Soldier Voice state.
	~SoldierVoiceState();
	/// Sorts the voice list.
	void sortList();
	/// Updates the voice list.
	void updateList();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action* action);
	void btnQuickSearchApply(Action* action);
	/// Handler for clicking the voice list.
	void lstVoiceClick(Action *action);
	/// Handler for clicking the voice list.
	void lstVoiceClickRight(Action *action);
	/// Handler for clicking the Name arrow.
	void sortNameClick(Action *action);
};

}
