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
#include <sstream>
#include "ExperienceOverviewState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Mod/Mod.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Experience Overview screen.
 */
ExperienceOverviewState::ExperienceOverviewState()
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 13);
	_txtName = new Text(100, 10, 10, 40);
	_btnOk = new TextButton(160, 16, 80, 174);
	_lstSoldiers = new TextList(286, 112, 8, 52);
	_txtBravery = new Text(18, 10, 120 - 3, 40);
	_txtReactions = new Text(18, 10, 141 - 3, 40);
	_txtFiring = new Text(18, 10, 162 - 3, 40);
	_txtThrowing = new Text(18, 10, 183 - 3, 40);
	_txtPsiSkill = new Text(18, 10, 204 - 3, 40);
	_txtPsiStrength = new Text(18, 10, 225 - 3, 40);
	_txtMelee = new Text(18, 10, 246 - 3, 40);
	_txtMana = new Text(18, 10, 267 - 3, 40);

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	add(_window, "messageWindowBorder", "battlescape");
	add(_btnOk, "messageWindowButtons", "battlescape");
	add(_txtName, "messageWindows", "battlescape");
	add(_txtTitle, "messageWindows", "battlescape");
	add(_lstSoldiers, "optionLists", "battlescape");
	add(_txtBravery, "messageWindows", "battlescape");
	add(_txtReactions, "messageWindows", "battlescape");
	add(_txtFiring, "messageWindows", "battlescape");
	add(_txtThrowing, "messageWindows", "battlescape");
	add(_txtPsiSkill, "messageWindows", "battlescape");
	add(_txtPsiStrength, "messageWindows", "battlescape");
	add(_txtMelee, "messageWindows", "battlescape");
	add(_txtMana, "messageWindows", "battlescape");

	centerAllSurfaces();

	// Set up objects
	_window->setHighContrast(true);
	_window->setBackground(_game->getMod()->getSurface("TAC00.SCR"));

	_btnOk->setHighContrast(true);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ExperienceOverviewState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ExperienceOverviewState::btnOkClick, Options::keyCancel);

	_txtTitle->setHighContrast(true);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_EXPERIENCE_OVERVIEW"));

	_txtName->setHighContrast(true);
	_txtBravery->setHighContrast(true);
	_txtReactions->setHighContrast(true);
	_txtFiring->setHighContrast(true);
	_txtThrowing->setHighContrast(true);
	_txtPsiSkill->setHighContrast(true);
	_txtPsiStrength->setHighContrast(true);
	_txtMelee->setHighContrast(true);
	_txtMana->setHighContrast(true);

	_txtName->setText(tr("STR_NAME"));
	_txtBravery->setText(tr("STR_BRAVERY_ABBREVIATION"));
	_txtReactions->setText(tr("STR_REACTIONS_ABBREVIATION"));
	_txtFiring->setText(tr("STR_FIRING_ACCURACY_ABBREVIATION"));
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY_ABBREVIATION"));
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_ABBREVIATION"));
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH_ABBREVIATION"));
	_txtMelee->setText(tr("STR_MELEE_ACCURACY_ABBREVIATION"));
	if (_game->getMod()->isManaFeatureEnabled() && _game->getMod()->isManaTrainingPrimary())
	{
		_txtMana->setText(tr("STR_MANA_ABBREVIATION"));
	}

	_lstSoldiers->setColumns(10, 110, 21, 21, 21, 21, 21, 21, 21, 21, 18);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setHighContrast(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(2);

	_lstSoldiers->clearList();
	int row = 0;
	for (auto* soldier : *_game->getSavedGame()->getSavedBattle()->getUnits())
	{
		if (!soldier->getGeoscapeSoldier())
		{
			continue;
		}
		if (soldier->getStatus() == STATUS_DEAD || soldier->getStatus() == STATUS_IGNORE_ME)
		{
			continue;
		}

		const UnitStats* stats = soldier->getExpStats();

		std::ostringstream bravery;
		bravery << stats->bravery;
		std::ostringstream reactions;
		reactions << stats->reactions;
		std::ostringstream firing;
		firing << stats->firing;
		std::ostringstream throwing;
		throwing << stats->throwing;
		std::ostringstream psiSkill;
		psiSkill << stats->psiSkill;
		std::ostringstream psiStrength;
		psiStrength << stats->psiStrength;
		std::ostringstream melee;
		melee << stats->melee;
		std::ostringstream mana;
		if (_game->getMod()->isManaFeatureEnabled() && _game->getMod()->isManaTrainingPrimary())
		{
			mana << stats->mana;
		}

		_lstSoldiers->addRow(10,
			soldier->getName(_game->getLanguage()).c_str(),
			bravery.str().c_str(),
			reactions.str().c_str(),
			firing.str().c_str(),
			throwing.str().c_str(),
			psiSkill.str().c_str(),
			psiStrength.str().c_str(),
			melee.str().c_str(),
			mana.str().c_str(),
			"");
		for (int col = 1; col < 9; ++col)
		{
			if (_lstSoldiers->getCellText(row, col) != "0")
			{
				_lstSoldiers->setCellColor(row, col, _lstSoldiers->getSecondaryColor());
			}
		}
		row++;
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ExperienceOverviewState::btnOkClick(Action*)
{
	_game->popState();
}

}
