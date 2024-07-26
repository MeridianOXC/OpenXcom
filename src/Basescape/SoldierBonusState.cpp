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
#include "SoldierBonusState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/HelperMeta.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldierBonus.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Ufopaedia/StatsForNerdsState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the SoldierBonus window.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierBonusState::SoldierBonusState(Base *base, size_t soldier) : _base(base), _soldier(soldier)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_btnSummary = new ToggleTextButton(84, 16, 73, 156);
	_btnCancel = new TextButton(84, 16, 164, 156);
	_txtTitle = new Text(182, 16, 69, 28);
	_txtType = new Text(90, 9, 80, 52);
	_lstBonuses = new TextList(158, 80, 73, 68);
	_lstSummary = new TextList(158, 80, 73, 68);

	// Set palette
	setInterface("soldierBonus");

	add(_window, "window", "soldierBonus");
	add(_btnSummary, "button", "soldierBonus");
	add(_btnCancel, "button", "soldierBonus");
	add(_txtTitle, "text", "soldierBonus");
	add(_txtType, "text", "soldierBonus");
	add(_lstBonuses, "list", "soldierBonus");
	add(_lstSummary, "list", "soldierBonus");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierBonus");

	_btnSummary->setText(tr("STR_SUMMARY"));
	_btnSummary->onMouseClick((ActionHandler)&SoldierBonusState::btnSummaryClick);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierBonusState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierBonusState::btnCancelClick, Options::keyCancel);

	Soldier *s = _base ? _base->getSoldiers()->at(_soldier) : _game->getSavedGame()->getDeadSoldiers()->at(_soldier);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SOLDIER_BONUSES_FOR").arg(s->getName()));

	_txtType->setText(tr("STR_TYPE"));

	_lstBonuses->setColumns(1, 150);
	_lstBonuses->setSelectable(true);
	_lstBonuses->setBackground(_window);
	_lstBonuses->setMargin(8);

	_bonuses.clear();
	_lstBonuses->clearList();
	for (auto* bonusRule : *s->getBonuses(nullptr))
	{
		_bonuses.push_back(bonusRule->getName());
		_lstBonuses->addRow(1, tr(bonusRule->getName()).c_str());
	}

	_lstBonuses->onMouseClick((ActionHandler)& SoldierBonusState::lstBonusesClick);

	_lstSummary->setColumns(2, 122, 20);
	_lstSummary->setAlign(ALIGN_RIGHT, 1);
	_lstSummary->setSelectable(true);
	_lstSummary->setBackground(_window);
	_lstSummary->setMargin(8);
	_lstSummary->setVisible(false);

	int visibilityAtDark = 0;
	int visibilityAtDay = 0;
	int psiVision = 0;
	int heatVision = 0;

	int frontArmor = 0, leftArmor = 0, rightArmor = 0, rearArmor = 0, underArmor = 0;
	UnitStats stats;
	bool timeRecovery = false, energyRecovery = false, moraleRecovery = false, healthRecovery = false, stunRecovery = false, manaRecovery = false;
	for (auto* bonusRule : *s->getBonuses(nullptr))
	{
		frontArmor += bonusRule->getFrontArmor();
		leftArmor  += bonusRule->getLeftSideArmor();
		rightArmor += bonusRule->getRightSideArmor();
		rearArmor  += bonusRule->getRearArmor();
		underArmor += bonusRule->getUnderArmor();

		visibilityAtDark += bonusRule->getVisibilityAtDark();
		visibilityAtDay += bonusRule->getVisibilityAtDay();
		psiVision += bonusRule->getPsiVision();
		heatVision += bonusRule->getHeatVision();

		stats += *bonusRule->getStats();
		timeRecovery = timeRecovery || bonusRule->getTimeRecoveryRaw()->isModded();
		energyRecovery = energyRecovery || bonusRule->getEnergyRecoveryRaw()->isModded();
		moraleRecovery = moraleRecovery || bonusRule->getMoraleRecoveryRaw()->isModded();
		healthRecovery = healthRecovery || bonusRule->getHealthRecoveryRaw()->isModded();
		stunRecovery = stunRecovery || bonusRule->getStunRegenerationRaw()->isModded();
		manaRecovery = manaRecovery || bonusRule->getManaRecoveryRaw()->isModded();
	}
	if (stats.tu != 0)
		_lstSummary->addRow(2, tr("STR_TIME_UNITS").c_str(), std::to_string(stats.tu).c_str());
	if (stats.stamina != 0)
		_lstSummary->addRow(2, tr("STR_STAMINA").c_str(), std::to_string(stats.stamina).c_str());
	if (stats.health != 0)
		_lstSummary->addRow(2, tr("STR_HEALTH").c_str(), std::to_string(stats.health).c_str());
	if (stats.bravery != 0)
		_lstSummary->addRow(2, tr("STR_BRAVERY").c_str(), std::to_string(stats.bravery).c_str());
	if (stats.reactions != 0)
		_lstSummary->addRow(2, tr("STR_REACTIONS").c_str(), std::to_string(stats.reactions).c_str());
	if (stats.firing != 0)
		_lstSummary->addRow(2, tr("STR_FIRING_ACCURACY").c_str(), std::to_string(stats.firing).c_str());
	if (stats.throwing != 0)
		_lstSummary->addRow(2, tr("STR_THROWING_ACCURACY").c_str(), std::to_string(stats.throwing).c_str());
	if (stats.melee != 0)
		_lstSummary->addRow(2, tr("STR_MELEE_ACCURACY").c_str(), std::to_string(stats.melee).c_str());
	if (stats.strength != 0)
		_lstSummary->addRow(2, tr("STR_STRENGTH").c_str(), std::to_string(stats.strength).c_str());
	if (stats.mana != 0)
		_lstSummary->addRow(2, tr("STR_MANA_POOL").c_str(), std::to_string(stats.mana).c_str());
	if (stats.psiStrength != 0)
		_lstSummary->addRow(2, tr("STR_PSIONIC_STRENGTH").c_str(), std::to_string(stats.psiStrength).c_str());
	if (stats.psiSkill != 0)
		_lstSummary->addRow(2, tr("STR_PSIONIC_SKILL").c_str(), std::to_string(stats.psiSkill).c_str());


	helper::SingleRun gap;


	gap.reset();
	if (frontArmor != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("STR_FRONT_ARMOR_UC").c_str(), std::to_string(frontArmor).c_str());
	}
	if (leftArmor != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("STR_LEFT_ARMOR_UC").c_str(), std::to_string(leftArmor).c_str());
	}
	if (rightArmor != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("STR_RIGHT_ARMOR_UC").c_str(), std::to_string(rightArmor).c_str());
	}
	if (rearArmor != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("STR_REAR_ARMOR_UC").c_str(), std::to_string(rearArmor).c_str());
	}
	if (underArmor != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("STR_UNDER_ARMOR_UC").c_str(), std::to_string(underArmor).c_str());
	}


	gap.reset();
	if (visibilityAtDark != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("visibilityAtDark").c_str(), std::to_string(visibilityAtDark).c_str());
	}
	if (visibilityAtDay != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("visibilityAtDay").c_str(), std::to_string(visibilityAtDay).c_str());
	}
	if (psiVision != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("psiVision").c_str(), std::to_string(psiVision).c_str());
	}
	if (heatVision != 0)
	{
		if (gap.tryRun()) _lstSummary->addRow(1, "");
		_lstSummary->addRow(2, tr("heatVision").c_str(), std::to_string(heatVision).c_str());
	}


	if (timeRecovery || energyRecovery || moraleRecovery || healthRecovery || stunRecovery || manaRecovery)
	{
		_lstSummary->addRow(1, "");
		_lstSummary->addRow(1, tr("recovery").c_str());
		if (timeRecovery)
			_lstSummary->addRow(1, tr("time").c_str());
		if (energyRecovery)
			_lstSummary->addRow(1, tr("energy").c_str());
		if (moraleRecovery)
			_lstSummary->addRow(1, tr("morale").c_str());
		if (healthRecovery)
			_lstSummary->addRow(1, tr("health").c_str());
		if (stunRecovery)
			_lstSummary->addRow(1, tr("stun").c_str());
		if (manaRecovery)
			_lstSummary->addRow(1, tr("mana").c_str());
	}
}

/**
 *
 */
SoldierBonusState::~SoldierBonusState()
{

}

/**
 * Toggles Summary view.
 * @param action Pointer to an action.
 */
void SoldierBonusState::btnSummaryClick(Action*)
{
	_lstBonuses->setVisible(!_btnSummary->getPressed());
	_lstSummary->setVisible(_btnSummary->getPressed());
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierBonusState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Shows corresponding Stats for Nerds article.
 * @param action Pointer to an action.
 */
void SoldierBonusState::lstBonusesClick(Action *)
{
	std::string articleId = _bonuses[_lstBonuses->getSelectedRow()];
	_game->pushState(new StatsForNerdsState(UFOPAEDIA_TYPE_UNKNOWN, articleId, false, false, false));
}

}
