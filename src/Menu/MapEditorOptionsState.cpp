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
#include "MapEditorOptionsState.h"
#include "../Battlescape/MapEditor.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Mod/Mod.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "AbandonGameState.h"
#include "MapEditorInfoState.h"
#include "MapEditorSaveAsState.h"
#include "OptionsVideoState.h"
#include "OptionsGeoscapeState.h"
#include "OptionsBattlescapeState.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the map editor options window
 * @param game Pointer to the core game.
 * @param parent Pointer to the map editor menu
 */
MapEditorOptionsState::MapEditorOptionsState(OptionsOrigin origin) : _origin(origin)
{
    _screen = false;

    _window = new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_txtTitle = new Text(206, 17, 57, 8);

	_btnInfo = new TextButton(80, 16, 8, 28);
	_btnLoad = new TextButton(80, 16, 8, 48);
	_btnSave = new TextButton(80, 16, 8, 68);
	_btnAbandon = new TextButton(80, 16, 8, 88);
	_btnOptions = new TextButton(80, 16, 8, 108);
	_btnCancel = new TextButton(80, 16, 8, 128);

	_lstOptions = new TextList(200, 116, 94, 28);
	_txtTooltip = new Text(219, 25, 94, 148);

	//_isTFTD = false;
	//for (std::vector< std::pair<std::string, bool> >::const_iterator i = Options::mods.begin(); i != Options::mods.end(); ++i)
	//{
	//	if (i->second)
	//	{
	//		if (i->first == "xcom2")
	//		{
	//			_isTFTD = true;
	//			break;
	//		}
	//	}
	//}

	// Set palette
	setInterface("optionsMenu", false, _game->getSavedGame()->getSavedBattle());

    add(_window, "window", "optionsMenu");
    add(_txtTitle, "text", "optionsMenu");
	add(_btnInfo, "text", "optionsMenu");
	add(_btnLoad, "button", "optionsMenu");
	add(_btnSave, "button", "optionsMenu");
	add(_btnAbandon, "button", "optionsMenu");
	add(_btnOptions, "button", "optionsMenu");
	add(_btnCancel, "button", "optionsMenu");
	add(_lstOptions, "optionLists", "battlescape");
    add(_txtTooltip, "text", "optionsMenu");

    centerAllSurfaces();

    // Set up objects
	applyBattlescapeTheme();
	setWindowBackground(_window, "mainMenu");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_OPTIONS_UC"));

	_btnInfo->setText(tr("STR_MAP_INFO"));
	_btnInfo->onMouseClick((ActionHandler)&MapEditorOptionsState::btnInfoClick);

	_btnLoad->setText(tr("STR_LOAD_MAP"));
	_btnLoad->onMouseClick((ActionHandler)&MapEditorOptionsState::btnLoadClick);

	_btnSave->setText(tr("STR_SAVE_MAP"));
	_btnSave->onMouseClick((ActionHandler)&MapEditorOptionsState::btnSaveClick);

	_btnAbandon->setText(tr("STR_ABANDON_GAME"));
	_btnAbandon->onMouseClick((ActionHandler)&MapEditorOptionsState::btnAbandonClick);

	_btnOptions->setText(tr("STR_GAME_OPTIONS"));
	_btnOptions->onMouseClick((ActionHandler)&MapEditorOptionsState::btnOptionsClick);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&MapEditorOptionsState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorOptionsState::btnCancelClick, Options::keyCancel);
	_btnCancel->onKeyboardPress((ActionHandler)&MapEditorOptionsState::btnCancelClick, Options::keyBattleOptions);

	// Set up the list of user options for the editor
	// how much room do we need for YES/NO
	Text text = Text(100, 9, 0, 0);
	text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	text.setText(tr("STR_YES"));
	int yes = text.getTextWidth();
	text.setText(tr("STR_NO"));
	int no = text.getTextWidth();

	int rightcol = std::max(yes, no) + 2;
	int leftcol = _lstOptions->getWidth() - rightcol;

	// Set up objects
	_lstOptions->setAlign(ALIGN_RIGHT, 1);
	_lstOptions->setColumns(2, leftcol, rightcol);
	_lstOptions->setWordWrap(true);
	_lstOptions->setSelectable(true);
	_lstOptions->setBackground(_window);
	_lstOptions->onMouseClick((ActionHandler)&MapEditorOptionsState::lstOptionsClick, 0);
	_lstOptions->onMouseOver((ActionHandler)&MapEditorOptionsState::lstOptionsMouseOver);
	_lstOptions->onMouseOut((ActionHandler)&MapEditorOptionsState::lstOptionsMouseOut);

	const std::vector<OptionInfo> &options = Options::getOptionInfo();
	for (std::vector<OptionInfo>::const_iterator i = options.begin(); i != options.end(); ++i)
	{
		if (i->type() != OPTION_KEY && !i->description().empty())
		{
			if (i->category() == "STR_MAPEDITOR")
			{
				_settings.push_back(*i);
			}
		}
	}
}

/**
 * Cleans up the state
 */
MapEditorOptionsState::~MapEditorOptionsState()
{

}

/**
 * Fills the settings list.
 */
void MapEditorOptionsState::init()
{
	_lstOptions->clearList();
	for (std::vector<OptionInfo>::const_iterator i = _settings.begin(); i != _settings.end(); ++i)
	{
		std::string name = tr(i->description());
		std::string value;
		if (i->type() == OPTION_BOOL)
		{
			value = *i->asBool() ? tr("STR_YES") : tr("STR_NO");
		}
		else if (i->type() == OPTION_INT)
		{
			std::ostringstream ss;
			ss << *i->asInt();
			value = ss.str();
		}
		_lstOptions->addRow(2, name.c_str(), value.c_str());
	}
}

/**
 * Opens the info screen
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnInfoClick(Action *)
{
	_game->pushState(new MapEditorInfoState());
}

/**
 * Opens the load map screen
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnLoadClick(Action *)
{

}

/**
 * Saves the current map
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnSaveClick(Action *)
{
	if (_game->getMapEditor()->getMapName().size() == 0)
	{
		_game->pushState(new MapEditorSaveAsState());
	}
	else
	{
    	_game->getMapEditor()->saveMapFile(_game->getMapEditor()->getMapName());
	}
}

/**
 * Opens the Game Options screen.
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnOptionsClick(Action *)
{
	Options::backupDisplay();
	_game->pushState(new OptionsVideoState(_origin));
}

/**
 * Opens the Abandon Game window.
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnAbandonClick(Action *)
{
	_game->pushState(new AbandonGameState(_origin));
}

/**
 * Returns to the map editor state
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::btnCancelClick(Action *)
{
    _game->popState();
}

/**
 * Changes the clicked setting.
 * @param action Pointer to an action.
 */
void MapEditorOptionsState::lstOptionsClick(Action *action)
{
	Uint8 button = action->getDetails()->button.button;
	if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT)
	{
		return;
	}
	size_t sel = _lstOptions->getSelectedRow();
	OptionInfo *setting;
	if (sel < _settings.size())
	{
		setting = &_settings.at(sel);
	}
	else
	{
		return;
	}

	std::string settingText;
	if (setting->type() == OPTION_BOOL)
	{
		bool *b = setting->asBool();
		*b = !*b;
		settingText = *b ? tr("STR_YES") : tr("STR_NO");
	}
	else if (setting->type() == OPTION_INT) // integer variables will need special handling
	{
		int *i = setting->asInt();

		int increment = (button == SDL_BUTTON_LEFT) ? 1 : -1; // left-click increases, right-click decreases
		//if (i == &Options::changeValueByMouseWheel || i == &Options::FPS || i == &Options::FPSInactive || i == &Options::oxceWoundedDefendBaseIf)
		//{
		//	increment *= 10;
		//}
		*i += increment;

		int min = 0, max = 0;
		// set specific values for options if we need them, e.g.
		// if (i == &Options::something)
		// {min = x; max = y;}

		if (*i < min)
		{
			*i = max;
		}
		else if (*i > max)
		{
			*i = min;
		}

		std::ostringstream ss;
		ss << *i;
		settingText = ss.str();
	}
	_lstOptions->setCellText(sel, 1, settingText);
}

void MapEditorOptionsState::lstOptionsMouseOver(Action *)
{
	size_t sel = _lstOptions->getSelectedRow();
	OptionInfo *setting = 0;
	if (sel < _settings.size())
	{
		setting = &_settings.at(sel);
	}
	std::string desc;
	if (setting)
	{
		desc = tr(setting->description() + "_DESC");
	}
	_txtTooltip->setText(desc);
}

void MapEditorOptionsState::lstOptionsMouseOut(Action *)
{
	_txtTooltip->setText("");
}

}