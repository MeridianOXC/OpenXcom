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
#include "OptionsControlsState.h"
#include <SDL.h>
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Engine/Action.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Controls screen.
 * @param game Pointer to the core game.
 * @param origin Game section that originated this state.
 */
OptionsControlsState::OptionsControlsState(OptionsOrigin origin) : OptionsBaseState(origin), _selected(-1), _selKey(0)
{
#if !defined(__MOBILE__) && !defined(__PSEUDO_ANDROID__)
	setCategory(_btnControls);
#endif

	// Create objects
	_btnOXC = new TextButton(70, 16, 94, 8);
	_btnOXCE = new TextButton(70, 16, 168, 8);
	_btnOTHER = new TextButton(70, 16, 242, 8);
	_lstControls = new TextList(200, 120, 94, 26);

	_owner = _btnOXC;

	add(_btnOXC, "button", "controlsMenu");
	add(_btnOXCE, "button", "controlsMenu");
	add(_btnOTHER, "button", "controlsMenu");

	if (origin != OPT_BATTLESCAPE)
	{
		add(_lstControls, "optionLists", "controlsMenu");
	}
	else
	{
		add(_lstControls, "optionLists", "battlescape");
	}

	centerAllSurfaces();

	_btnOXC->setText(tr("STR_ENGINE_OXC"));
	_btnOXC->setGroup(&_owner);
	_btnOXC->onMousePress((ActionHandler)&OptionsControlsState::btnGroupPress, SDL_BUTTON_LEFT);

	_btnOXCE->setText(tr("STR_ENGINE_OXCE"));
	_btnOXCE->setGroup(&_owner);
	_btnOXCE->onMousePress((ActionHandler)&OptionsControlsState::btnGroupPress, SDL_BUTTON_LEFT);

	_btnOTHER->setText(tr("STR_ENGINE_OTHER")); // rename in your fork
	_btnOTHER->setGroup(&_owner);
	_btnOTHER->onMousePress((ActionHandler)&OptionsControlsState::btnGroupPress, SDL_BUTTON_LEFT);
	_btnOTHER->setVisible(false); // enable in your fork

	// Set up objects
	_lstControls->setColumns(2, 152, 48);
	_lstControls->setWordWrap(true);
	_lstControls->setSelectable(true);
	_lstControls->setBackground(_window);
	_lstControls->onMouseClick((ActionHandler)&OptionsControlsState::lstControlsClick, 0);
	_lstControls->onKeyboardPress((ActionHandler)&OptionsControlsState::lstControlsKeyPress);
	_lstControls->setFocus(true);
	_lstControls->setTooltip("STR_CONTROLS_DESC");
	_lstControls->onMouseIn((ActionHandler)&OptionsControlsState::txtTooltipIn);
	_lstControls->onMouseOut((ActionHandler)&OptionsControlsState::txtTooltipOut);

	_colorGroup = _lstControls->getSecondaryColor();
	_colorSel = _lstControls->getScrollbarColor();
	_colorNormal = _lstControls->getColor();

	for (const auto& optionInfo : Options::getOptionInfo())
	{
		if (optionInfo.type() == OPTION_KEY && !optionInfo.description().empty())
		{
			if (optionInfo.category() == "STR_GENERAL")
			{
				_controlsGeneral[optionInfo.owner()].push_back(optionInfo);
			}
			else if (optionInfo.category() == "STR_GEOSCAPE")
			{
				_controlsGeo[optionInfo.owner()].push_back(optionInfo);
			}
			else if (optionInfo.category() == "STR_BASESCAPE")
			{
				_controlsBase[optionInfo.owner()].push_back(optionInfo);
			}
			else if (optionInfo.category() == "STR_BATTLESCAPE")
			{
				_controlsBattle[optionInfo.owner()].push_back(optionInfo);
			}
		}
	}
}

/**
 *
 */
OptionsControlsState::~OptionsControlsState()
{
}

/**
 * Refreshes the UI.
 */
void OptionsControlsState::init()
{
	OptionsBaseState::init();

	updateList();
}

/**
 * Fills the controls list based on category.
 */
void OptionsControlsState::updateList()
{
	OptionOwner idx = _owner == _btnOXC ? OPTION_OXC : _owner == _btnOXCE ? OPTION_OXCE : OPTION_OTHER;

	_offsetGeneralMin = -1;
	_offsetGeneralMax = -1;
	_offsetGeoMin = -1;
	_offsetGeoMax = -1;
	_offsetBaseMin = -1;
	_offsetBaseMax = -1;
	_offsetBattleMin = -1;
	_offsetBattleMax = -1;

	_lstControls->clearList();

	int row = -1;

	if (_controlsGeneral[idx].size() > 0)
	{
		_lstControls->addRow(2, tr("STR_GENERAL").c_str(), "");
		row++;
		_offsetGeneralMin = row;
		_lstControls->setCellColor(_offsetGeneralMin, 0, _colorGroup);
		addControls(_controlsGeneral[idx]);
		row += _controlsGeneral[idx].size();
		_offsetGeneralMax = row;
	}
	if (_controlsGeo[idx].size() > 0)
	{
		if (row > -1) { _lstControls->addRow(2, "", ""); row++; }
		_lstControls->addRow(2, tr("STR_GEOSCAPE").c_str(), "");
		row++;
		_offsetGeoMin = row;
		_lstControls->setCellColor(_offsetGeoMin, 0, _colorGroup);
		addControls(_controlsGeo[idx]);
		row += _controlsGeo[idx].size();
		_offsetGeoMax = row;
	}
	if (_controlsBase[idx].size() > 0)
	{
		if (row > -1) { _lstControls->addRow(2, "", ""); row++; }
		_lstControls->addRow(2, tr("STR_BASESCAPE").c_str(), "");
		row++;
		_offsetBaseMin = row;
		_lstControls->setCellColor(_offsetBaseMin, 0, _colorGroup);
		addControls(_controlsBase[idx]);
		row += _controlsBase[idx].size();
		_offsetBaseMax = row;
	}
	if (_controlsBattle[idx].size() > 0)
	{
		if (row > -1) { _lstControls->addRow(2, "", ""); row++; }
		_lstControls->addRow(2, tr("STR_BATTLESCAPE").c_str(), "");
		row++;
		_offsetBattleMin = row;
		_lstControls->setCellColor(_offsetBattleMin, 0, _colorGroup);
		addControls(_controlsBattle[idx]);
		row += _controlsBattle[idx].size();
		_offsetBattleMax = row;
	}
}

/**
 * Uppercases all the words in a string.
 * @param str Source string.
 * @return Destination string.
 */
std::string OptionsControlsState::ucWords(std::string str)
{
	if (!str.empty())
	{
		str[0] = toupper(str[0]);
	}
	for (size_t i = str.find_first_of(' '); i != std::string::npos; i = str.find_first_of(' ', i+1))
	{
		if (str.length() > i+1)
			str[i+1] = toupper(str[i+1]);
		else
			break;
	}
	return str;
}

/**
 * Adds a bunch of controls to the list.
 * @param keys List of controls.
 */
void OptionsControlsState::addControls(const std::vector<OptionInfo> &keys)
{
	for (const auto& optionInfo : keys)
	{
		std::string name = tr(optionInfo.description());
		SDL_Keycode *key = optionInfo.asKey();
		std::string keyName = ucWords(SDL_GetKeyName(*key));
		if (*key == SDLK_UNKNOWN)
			keyName = "";
		_lstControls->addRow(2, name.c_str(), keyName.c_str());
	}
}

/**
 * Gets the currently selected control.
 * @param sel Selected row.
 * @return Pointer to option, NULL if none selected.
 */
OptionInfo *OptionsControlsState::getControl(size_t sel)
{
	int selInt = sel;
	OptionOwner idx = _owner == _btnOXC ? OPTION_OXC : _owner == _btnOXCE ? OPTION_OXCE : OPTION_OTHER;

	if (selInt > _offsetGeneralMin && selInt <= _offsetGeneralMax)
	{
		return &_controlsGeneral[idx][selInt - 1 - _offsetGeneralMin];
	}
	else if (selInt > _offsetGeoMin && selInt <= _offsetGeoMax)
	{
		return &_controlsGeo[idx][selInt - 1 - _offsetGeoMin];
	}
	else if (selInt > _offsetBaseMin && selInt <= _offsetBaseMax)
	{
		return &_controlsBase[idx][selInt - 1 - _offsetBaseMin];
	}
	else if (selInt > _offsetBattleMin && selInt <= _offsetBattleMax)
	{
		return &_controlsBattle[idx][selInt - 1 - _offsetBattleMin];
	}
	else
	{
		return 0;
	}
}

/**
 * Select a control for changing.
 * @param action Pointer to an action.
 */
void OptionsControlsState::lstControlsClick(Action *action)
{
	if (action->getDetails()->button.button != SDL_BUTTON_LEFT && action->getDetails()->button.button != SDL_BUTTON_RIGHT)
	{
		return;
	}
	if (_selected != -1)
	{
		unsigned int selected = _selected;
		_lstControls->setCellColor(_selected, 0, _colorNormal);
		_lstControls->setCellColor(_selected, 1, _colorNormal);
		_selected = -1;
		_selKey = 0;
		if (selected == _lstControls->getSelectedRow())
			return;
	}
	_selected = _lstControls->getSelectedRow();
	_selKey = getControl(_selected);
	if (!_selKey)
	{
		_selected = -1;
		return;
	}

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_lstControls->setCellColor(_selected, 0, _colorSel);
		_lstControls->setCellColor(_selected, 1, _colorSel);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_lstControls->setCellText(_selected, 1, "");
		*_selKey->asKey() = SDLK_UNKNOWN;
		_selected = -1;
		_selKey = 0;
	}
}

/**
 * Change selected control.
 * @param action Pointer to an action.
 */
void OptionsControlsState::lstControlsKeyPress(Action *action)
{
	if (_selected != -1)
	{
		SDL_Keycode key = action->getDetails()->key.keysym.sym;
		if (key != 0 &&
			key != SDLK_LSHIFT && key != SDLK_LALT && key != SDLK_LCTRL &&
			key != SDLK_RSHIFT && key != SDLK_RALT && key != SDLK_RCTRL)
		{
			*_selKey->asKey() = key;
			std::string name = ucWords(SDL_GetKeyName(*_selKey->asKey()));
			_lstControls->setCellText(_selected, 1, name);
		}
		_lstControls->setCellColor(_selected, 0, _colorNormal);
		_lstControls->setCellColor(_selected, 1, _colorNormal);
		_selected = -1;
		_selKey = 0;
	}
}

void OptionsControlsState::btnGroupPress(Action*)
{
	updateList();
}

}
