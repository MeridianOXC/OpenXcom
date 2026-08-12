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
#include "SoldierVoiceState.h"
#include <algorithm>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleVoiceSet.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{

struct compareVoiceName
{
	bool operator()(const VoiceSetItem &a, const VoiceSetItem &b) const
	{
		return Unicode::naturalCompare(a.name, b.name);
	}
};


/**
 * Initializes all the elements in the Soldier Voice window.
 * @param bu Pointer to the battle unit to get info from.
 */
SoldierVoiceState::SoldierVoiceState(BattleUnit* bu, Soldier* soldier, SoldierVoiceOrigin origin) : _bu(bu), _soldier(soldier), _origin(origin)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_btnQuickSearch = new TextEdit(this, 48, 9, 80, 43);
	_btnCancel = new TextButton(140, 16, 90, 156);
	_txtTitle = new Text(182, 16, 69, 28);
	_txtType = new Text(90, 9, 80, 52);
	_lstVoice = new TextList(160, 80, 73, 68);
	_sortName = new ArrowButton(ARROW_NONE, 11, 8, 80, 52);

	// Set palette
	if (_origin == SV_BATTLESCAPE)
	{
		setStandardPalette("PAL_BATTLESCAPE");
	}
	else
	{
		setInterface("soldierVoice");
	}

	add(_window, "window", "soldierVoice");
	add(_btnQuickSearch, "button", "soldierVoice");
	add(_btnCancel, "button", "soldierVoice");
	add(_txtTitle, "text", "soldierVoice");
	add(_txtType, "text", "soldierVoice");
	add(_lstVoice, "list", "soldierVoice");
	add(_sortName, "text", "soldierVoice");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierVoice");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierVoiceState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierVoiceState::btnCancelClick, Options::keyCancel);

	_txtTitle->setAlign(ALIGN_CENTER);
	if (_bu)
		_txtTitle->setText(tr("STR_SELECT_VOICE_SET_FOR").arg(_bu->getName(_game->getLanguage())));
	else if (_soldier)
		_txtTitle->setText(tr("STR_SELECT_VOICE_SET_FOR").arg(_soldier->getName()));

	_txtType->setText(tr("STR_TYPE"));

	_lstVoice->setColumns(1, 153);
	_lstVoice->setSelectable(true);
	_lstVoice->setBackground(_window);
	_lstVoice->setMargin(8);
	_sortName->setX(_sortName->getX() + _txtType->getTextWidth() + 4);
	_sortName->onMouseClick((ActionHandler)&SoldierVoiceState::sortNameClick);

	for (auto& voiceSetName : _game->getMod()->getVoiceSetsList())
	{
		auto* vs = _game->getMod()->getVoiceSet(voiceSetName);
		if (vs)
		{
			_voices.push_back(VoiceSetItem(vs->getType(), tr(vs->getType()), vs));
		}
	}

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SoldierVoiceState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(Options::oxceQuickSearchButton);

	_btnCancel->onKeyboardRelease((ActionHandler)&SoldierVoiceState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	// switch to battlescape theme if called from inventory
	if (_origin == SV_BATTLESCAPE)
	{
		applyBattlescapeTheme("soldierVoice");

		const Element* element = _game->getMod()->getInterface("battlescape")->getElement("optionLists");
		_lstVoice->setSecondaryColor(element->color2);
	}

	_voiceOrder = VOICE_SORT_NONE;
	sortList();

	_lstVoice->onMouseClick((ActionHandler)&SoldierVoiceState::lstVoiceClick);
	_lstVoice->onMouseClick((ActionHandler)&SoldierVoiceState::lstVoiceClickRight, SDL_BUTTON_RIGHT);
}

/**
 *
 */
SoldierVoiceState::~SoldierVoiceState()
{

}

/**
* Updates the sorting arrows based
* on the current setting.
*/
void SoldierVoiceState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	switch (_voiceOrder)
	{
	case VOICE_SORT_NAME_ASC:
		_sortName->setShape(ARROW_SMALL_UP);
		break;
	case VOICE_SORT_NAME_DESC:
		_sortName->setShape(ARROW_SMALL_DOWN);
		break;
	default:
		break;
	}
}

/**
* Sorts the voice list.
*/
void SoldierVoiceState::sortList()
{
	updateArrows();

	switch (_voiceOrder)
	{
	case VOICE_SORT_NAME_ASC:
		std::stable_sort(_voices.begin(), _voices.end(), compareVoiceName());
		break;
	case VOICE_SORT_NAME_DESC:
		std::stable_sort(_voices.rbegin(), _voices.rend(), compareVoiceName());
		break;
	default:
		break;
	}

	updateList();
}

/**
* Updates the voice list with the current list
* of available voices.
*/
void SoldierVoiceState::updateList()
{
	std::string currentVoiceType = _bu ? (_bu->getUnitVoiceSet() ? _bu->getUnitVoiceSet()->getType() : "") : (_soldier ? _soldier->getVoiceSetType() : "");

	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_lstVoice->clearList();
	_indices.clear();

	int index = -1;
	for (const auto& vsItem : _voices)
	{
		++index;

		if (!vsItem.vs->getAllowedGenders().empty())
		{
			const int match = _bu ? (_bu->getGeoscapeSoldier() ? _bu->getGeoscapeSoldier()->getGender() : -1) : (_soldier ? _soldier->getGender() : -1);
			if (std::find(vsItem.vs->getAllowedGenders().begin(), vsItem.vs->getAllowedGenders().end(), match) == vsItem.vs->getAllowedGenders().end())
			{
				continue;
			}
		}

		if (!vsItem.vs->getAllowedSoldiers().empty())
		{
			const RuleSoldier* match = _bu ? (_bu->getGeoscapeSoldier() ? _bu->getGeoscapeSoldier()->getRules() : nullptr) : (_soldier ? _soldier->getRules() : nullptr);
			if (std::find(vsItem.vs->getAllowedSoldiers().begin(), vsItem.vs->getAllowedSoldiers().end(), match) == vsItem.vs->getAllowedSoldiers().end())
			{
				continue;
			}
		}

		if (!vsItem.vs->getAllowedUnits().empty())
		{
			const Unit* match = _bu ? _bu->getUnitRules() : nullptr;
			if (std::find(vsItem.vs->getAllowedUnits().begin(), vsItem.vs->getAllowedUnits().end(), match) == vsItem.vs->getAllowedUnits().end())
			{
				continue;
			}
		}

		if (!vsItem.vs->getAllowedArmors().empty())
		{
			const Armor* match = _bu ? _bu->getArmor() : (_soldier ? _soldier->getArmor() : nullptr);
			if (std::find(vsItem.vs->getAllowedArmors().begin(), vsItem.vs->getAllowedArmors().end(), match) == vsItem.vs->getAllowedArmors().end())
			{
				continue;
			}
		}

		// quick search
		if (!searchString.empty())
		{
			std::string voiceName = vsItem.name;
			Unicode::upperCase(voiceName);
			if (voiceName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		_lstVoice->addRow(1, vsItem.name.c_str());
		if (vsItem.type == currentVoiceType)
		{
			_lstVoice->setRowColor(_lstVoice->getLastRowIndex(), _lstVoice->getSecondaryColor());
		}
		_indices.push_back(index);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierVoiceState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Quick search toggle.
 * @param action Pointer to an action.
 */
void SoldierVoiceState::btnQuickSearchToggle(Action* action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText("");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
 * Quick search.
 * @param action Pointer to an action.
 */
void SoldierVoiceState::btnQuickSearchApply(Action*)
{
	updateList();
}

/**
 * Sets the voice on the unit and returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierVoiceState::lstVoiceClick(Action *)
{
	auto* vs = _game->getMod()->getVoiceSet(_voices[_indices[_lstVoice->getSelectedRow()]].type);
	if (vs)
	{
		if (_bu)
			_bu->setUnitAndSoldierVoiceSet(vs);
		else if (_soldier)
			_soldier->setVoiceSetType(vs->getType());
	}

	_game->popState();
}

/**
 * Play a voice set sample.
 * @param action Pointer to an action.
 */
void SoldierVoiceState::lstVoiceClickRight(Action *action)
{
	auto* vs = _game->getMod()->getVoiceSet(_voices[_indices[_lstVoice->getSelectedRow()]].type);
	if (vs)
	{
		std::vector<int> sounds;
		sounds.reserve(vs->getSelectUnitSounds().size() + vs->getStartMovingSounds().size() + vs->getSelectWeaponSounds().size() + vs->getAnnoyedSounds().size());
		sounds.insert(sounds.end(), vs->getSelectUnitSounds().begin(), vs->getSelectUnitSounds().end());
		sounds.insert(sounds.end(), vs->getStartMovingSounds().begin(), vs->getStartMovingSounds().end());
		sounds.insert(sounds.end(), vs->getSelectWeaponSounds().begin(), vs->getSelectWeaponSounds().end());
		sounds.insert(sounds.end(), vs->getAnnoyedSounds().begin(), vs->getAnnoyedSounds().end());
		if (!sounds.empty())
		{
			int sound = sounds[RNG::seedless(0, sounds.size() - 1)];
			if (sound != Mod::NO_SOUND)
			{
				if (!Mix_Playing(4))
				{
					// use fixed channel, so that we can check if the unit isn't already/still talking
					_game->getMod()->getSoundByDepth(0 /* depth */, sound)->play(4);
				}
			}
		}
	}
}

/**
 * Sorts the voices by name.
 * @param action Pointer to an action.
 */
void SoldierVoiceState::sortNameClick(Action *)
{
	_voiceOrder = _voiceOrder == VOICE_SORT_NAME_ASC ? VOICE_SORT_NAME_DESC : VOICE_SORT_NAME_ASC;
	sortList();
}

}
