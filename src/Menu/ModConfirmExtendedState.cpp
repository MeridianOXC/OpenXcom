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
#include "ModConfirmExtendedState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/ModInfo.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../version.h"
#include "ModListState.h"

namespace OpenXcom
{

	/**
	 * Initializes all the elements in the Confirm OXCE screen.
	 * @param state Pointer to the Options|Mod state.
	 * @param modInfo What exactly mod caused this question?
	 */
	ModConfirmExtendedState::ModConfirmExtendedState(ModListState *state, const ModInfo *modInfo, const ModInfo *masterInfo) : _state(state), _isMaster(modInfo->isMaster())
	{
		_screen = false;

		// Create objects
		_window = new Window(this, 256, 100, 32, 50, POPUP_BOTH);
		_btnYes = new TextButton(60, 18, 60, 122);
		_btnNo = new TextButton(60, 18, 200, 122);
		_txtTitle = new Text(246, 50, 37, 64);

		// Set palette
		setInterface("optionsMenu");

		add(_window, "confirmDefaults", "optionsMenu");
		add(_btnYes, "confirmDefaults", "optionsMenu");
		add(_btnNo, "confirmDefaults", "optionsMenu");
		add(_txtTitle, "confirmDefaults", "optionsMenu");

		centerAllSurfaces();

		// Set up objects
		setWindowBackground(_window, "optionsMenu");

		_btnYes->setText(tr("STR_YES"));
		_btnYes->onMouseClick((ActionHandler)&ModConfirmExtendedState::btnYesClick);
		if (!modInfo->isEngineOk())
		{
			_btnYes->setVisible(false);
		}

		_btnNo->setText(tr("STR_CANCEL"));
		_btnNo->onMouseClick((ActionHandler)&ModConfirmExtendedState::btnNoClick);

		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		if (masterInfo && !modInfo->isParentMasterOk(masterInfo))
		{
			_txtTitle->setText(tr("STR_MASTER_MOD_VERSION_REQUIRED_QUESTION").arg(modInfo->getRequiredMasterVersion()).arg(masterInfo->getVersion()));
		}
		else if (modInfo->getRequiredExtendedEngine() != OPENXCOM_VERSION_ENGINE)
		{
			_txtTitle->setText(tr("STR_OXCE_REQUIRED_QUESTION").arg(modInfo->getRequiredExtendedEngine()));
		}
		else
		{
			_txtTitle->setText(tr("STR_VERSION_REQUIRED_QUESTION").arg(modInfo->getRequiredExtendedVersion()));
		}
	}

	/**
	 *
	 */
	ModConfirmExtendedState::~ModConfirmExtendedState()
	{

	}

	/**
	 * Closes the window. Enables the mod.
	 * @param action Pointer to an action.
	 */
	void ModConfirmExtendedState::btnYesClick(Action *)
	{
		_game->popState();

		if (_isMaster)
		{
			_state->changeMasterMod();
		}
		else
		{
			_state->toggleMod();
		}
	}

	/**
	 * Closes the window. Does not enable the mod.
	 * @param action Pointer to an action.
	 */
	void ModConfirmExtendedState::btnNoClick(Action *)
	{
		_game->popState();

		if (_isMaster)
		{
			_state->revertMasterMod();
		}
	}

	/**
	 * Check if master mod is not valid.
	 */
	bool ModConfirmExtendedState::isMasterNotValid(const ModInfo *masterInfo)
	{
		return !masterInfo->isEngineOk();
	}

	/**
	 * Check if mod is not valid.
	 */
	bool ModConfirmExtendedState::isModNotValid(const ModInfo *modInfo, const ModInfo *masterInfo)
	{
		return !modInfo->isMaster() && // skip checking master mod
			(!modInfo->isEngineOk() || !modInfo->isParentMasterOk(masterInfo));
	}


	bool ModConfirmExtendedState::tryShowMasterNotValidConfirmationState(ModListState *state, const ModInfo *masterInfo)
	{
		if (isMasterNotValid(masterInfo))
		{
			_game->pushState(new ModConfirmExtendedState(state, masterInfo));
			return true;
		}

		return false;
	}

	bool ModConfirmExtendedState::tryShowModNotValidConfirmationState(ModListState *state, const ModInfo *modInfo, const ModInfo *masterInfo)
	{
		if (isModNotValid(modInfo, masterInfo))
		{
			_game->pushState(new ModConfirmExtendedState(state, modInfo, masterInfo));
			return true;
		}

		return false;
	}
}
