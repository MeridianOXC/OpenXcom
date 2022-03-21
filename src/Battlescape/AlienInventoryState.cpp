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
#include "AlienInventoryState.h"
#include "AlienInventory.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"
#include "../Mod/Mod.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleSoldier.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Battlescape/TileEngine.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the AlienInventory screen.
 * @param unit Pointer to the alien unit.
 */
AlienInventoryState::AlienInventoryState(BattleUnit *unit)
{
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	int offsetX = _game->getMod()->getAlienInventoryOffsetX();
	_soldier = new Surface(320 - offsetX, 200, offsetX, 0);
	_txtName = new Text(308, 17, 6, 6);
	_txtLeftHand = new Text(308, 17, 6, 160);
	_txtRightHand = new Text(308, 17, 6, 180);
	_btnArmor = new BattlescapeButton(40, 70, 140, 65);
	_inv = new AlienInventory(_game, 320, 200, 0, 0);

	// Set palette
	setStandardPalette("PAL_BATTLESCAPE");

	add(_bg);
	add(_soldier);
	add(_txtName, "textName", "inventory", _bg);
	add(_txtLeftHand, "textName", "inventory", _bg);
	add(_txtRightHand, "textName", "inventory", _bg);
	add(_btnArmor, "buttonOK", "inventory", _bg);
	add(_inv);

	centerAllSurfaces();

	// Set up objects
	Surface *tmp = _game->getMod()->getSurface("AlienInventory2", false);
	if (!tmp || !unit->getGeoscapeSoldier())
	{
		tmp = _game->getMod()->getSurface("AlienInventory", false);
	}
	if (tmp)
	{
		tmp->blitNShade(_bg, 0, 0);
	}

	_txtName->setBig();
	_txtName->setHighContrast(true);
	_txtName->setAlign(ALIGN_CENTER);

	if (Options::oxceDisableAlienInventory)
	{
		_txtName->setHeight(_txtName->getHeight() * 9);
		_txtName->setWordWrap(true);
		_txtName->setText(tr("STR_THIS_FEATURE_IS_DISABLED_5"));
		_soldier->setVisible(false);
		_txtLeftHand->setVisible(false);
		_txtRightHand->setVisible(false);
		_btnArmor->onKeyboardPress((ActionHandler)&AlienInventoryState::btnOkClick, Options::keyCancel);
		_inv->setVisible(false);
		return;
	}

	if (unit->getOriginalFaction() == FACTION_NEUTRAL)
	{
		_txtName->setText(tr(unit->getType()));
	}
	else
	{
		if (unit->getUnitRules())
		{
			if (unit->getUnitRules()->getShowFullNameInAlienInventory(_game->getMod()))
			{
				// e.g. Sectoid Leader
				_txtName->setText(unit->getName(_game->getLanguage()));
			}
			else
			{
				// e.g. Sectoid
				_txtName->setText(tr(unit->getUnitRules()->getRace()));
			}
		}
		else
		{
			// Soldier names
			_txtName->setText(unit->getName(_game->getLanguage()));
		}
	}

	_txtLeftHand->setBig();
	_txtLeftHand->setHighContrast(true);
	_txtLeftHand->setAlign(ALIGN_CENTER);
	_txtLeftHand->setVisible(false);

	_txtRightHand->setBig();
	_txtRightHand->setHighContrast(true);
	_txtRightHand->setAlign(ALIGN_CENTER);
	_txtRightHand->setVisible(false);

	_btnArmor->onKeyboardPress((ActionHandler)&AlienInventoryState::btnToggleClick, SDLK_F1);
	_btnArmor->onKeyboardPress((ActionHandler)&AlienInventoryState::btnOkClick, Options::keyCancel);
	_btnArmor->onMouseClick((ActionHandler)&AlienInventoryState::btnArmorClickMiddle, SDL_BUTTON_MIDDLE);

	_soldier->clear();

	Soldier *s = unit->getGeoscapeSoldier();
	if (s)
	{
		if (s->getArmor()->hasLayersDefinition())
		{
			for (const auto& layer : s->getArmorLayers())
			{
				auto surf = _game->getMod()->getSurface(layer, true);
				surf->blitNShade(_soldier->getSurface(), 0, 0);
			}
		}
		else
		{
			const std::string look = s->getArmor()->getSpriteInventory();
			const std::string gender = s->getGender() == GENDER_MALE ? "M" : "F";
			std::stringstream ss;
			Surface *surf = 0;

			for (int i = 0; i <= RuleSoldier::LookVariantBits; ++i)
			{
				ss.str("");
				ss << look;
				ss << gender;
				ss << (int)s->getLook() + (s->getLookVariant() & (RuleSoldier::LookVariantMask >> i)) * 4;
				ss << ".SPK";
				surf = _game->getMod()->getSurface(ss.str(), false);
				if (surf)
				{
					break;
				}
			}
			if (!surf)
			{
				ss.str("");
				ss << look;
				ss << ".SPK";
				surf = _game->getMod()->getSurface(ss.str(), false);
			}
			if (!surf)
			{
				surf = _game->getMod()->getSurface(look, true);
			}
			surf->blitNShade(_soldier, 0, 0);
		}
	}
	else
	{
		Surface *armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory(), false);
		if (!armorSurface)
		{
			armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory() + ".SPK", false);
		}
		if (!armorSurface)
		{
			armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory() + "M0.SPK", false);
		}
		if (armorSurface)
		{
			armorSurface->blitNShade(_soldier, 0, 0);
		}
	}

	_inv->setSelectedUnit(unit);
	_inv->draw();

	// Bleeding indicator
	tmp = _game->getMod()->getSurface("BigWoundIndicator", false);
	if (tmp && unit->getFatalWounds() > 0 && unit->indicatorsAreEnabled())
	{
		tmp->blitNShade(_soldier, 32, 32);
	}

	// Burning indicator
	tmp = _game->getMod()->getSurface("BigBurnIndicator", false);
	if (tmp && unit->getFire() > 0 && unit->indicatorsAreEnabled())
	{
		tmp->blitNShade(_soldier, 112, 32);
	}

	// --------------------- DEBUG INDICATORS ---------------------
	if (!Options::debug)
		return;

	auto weaponL = unit->getLeftHandWeapon();
	if (!weaponL)
	{
		auto typesToCheck = { BT_MELEE, BT_FIREARM };
		for (auto& type : typesToCheck)
		{
			weaponL = unit->getSpecialWeapon(type);
			if (weaponL && weaponL->getRules()->isSpecialUsingEmptyHand())
			{
				break;
			}
			weaponL = nullptr;
		}
	}
	if (weaponL)
	{
		if (weaponL->getRules()->getBattleType() == BT_FIREARM)
		{
			calculateRangedWeapon(unit, weaponL, _txtLeftHand);
		}
		else if (weaponL->getRules()->getBattleType() == BT_MELEE)
		{
			calculateMeleeWeapon(unit, weaponL, _txtLeftHand);
		}
	}

	auto weaponR = unit->getRightHandWeapon();
	if (!weaponR)
	{
		auto typesToCheck = { BT_MELEE, BT_FIREARM };
		for (auto& type : typesToCheck)
		{
			weaponR = unit->getSpecialWeapon(type);
			if (weaponR && weaponR->getRules()->isSpecialUsingEmptyHand())
			{
				break;
			}
			weaponR = nullptr;
		}
	}
	if (weaponR)
	{
		if (weaponR->getRules()->getBattleType() == BT_FIREARM)
		{
			calculateRangedWeapon(unit, weaponR, _txtRightHand);
		}
		else if (weaponR->getRules()->getBattleType() == BT_MELEE)
		{
			calculateMeleeWeapon(unit, weaponR, _txtRightHand);
		}
	}
}

/**
 *
 */
AlienInventoryState::~AlienInventoryState()
{
	if (Options::maximizeInfoScreens)
	{
		Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
		_game->getScreen()->resetDisplay(false);
	}
}

void AlienInventoryState::calculateMeleeWeapon(BattleUnit* unit, BattleItem* weapon, Text* label)
{
	std::ostringstream ss;

	auto tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();

	// Start by finding the target for the check
	int surroundingTilePositions[8][2] = {
		{0, -1}, // north (-y direction)
		{1, -1}, // northeast
		{1, 0}, // east (+ x direction)
		{1, 1}, // southeast
		{0, 1}, // south (+y direction)
		{-1, 1}, // southwest
		{-1, 0}, // west (-x direction)
		{-1, -1} }; // northwest

	Position tileToCheck = unit->getPosition();
	auto dir = unit->getDirection();
	tileToCheck.x += surroundingTilePositions[dir][0];
	tileToCheck.y += surroundingTilePositions[dir][1];
	BattleUnit* meleeDodgeTarget = nullptr;
	if (_game->getSavedGame()->getSavedBattle()->getTile(tileToCheck)) // Make sure the tile is in bounds
	{
		meleeDodgeTarget = _game->getSavedGame()->getSavedBattle()->selectUnit(tileToCheck);
	}

	ss << tr(weapon->getRules()->getType()) << " > ";
	if (meleeDodgeTarget)
	{
		BattleActionAttack attack;
		attack.type = BA_HIT;
		attack.attacker = unit;
		attack.weapon_item = weapon;
		attack.damage_item = weapon;
		attack.skill_rules = nullptr;
		int hitChance = BattleUnit::getFiringAccuracy(attack, _game->getMod());

		auto victim = meleeDodgeTarget;
		if (victim)
		{
			int arc = tileEngine->getArcDirection(tileEngine->getDirectionTo(victim->getPositionVexels(), unit->getPositionVexels()), victim->getDirection());
			float penalty = 1.0f - arc * victim->getArmor()->getMeleeDodgeBackPenalty() / 4.0f;
			if (penalty > 0)
			{
				hitChance -= victim->getArmor()->getMeleeDodge(victim) * penalty;
			}
		}
		ss << "hitChance " << hitChance << "% ";
	}
	else
	{
		ss << "not facing target ";
	}

	label->setText(ss.str());
}

void AlienInventoryState::calculateRangedWeapon(BattleUnit* unit, BattleItem* weapon, Text* label)
{
	std::ostringstream ss;

	auto tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();

	// Start by finding 'targets' for the check
	std::vector<BattleUnit*> closeQuartersTargetList;
	int surroundingTilePositions[8][2] = {
		{0, -1}, // north (-y direction)
		{1, -1}, // northeast
		{1, 0}, // east (+ x direction)
		{1, 1}, // southeast
		{0, 1}, // south (+y direction)
		{-1, 1}, // southwest
		{-1, 0}, // west (-x direction)
		{-1, -1} }; // northwest

	for (int dir = 0; dir < 8; dir++)
	{
		Position tileToCheck = unit->getPosition();
		tileToCheck.x += surroundingTilePositions[dir][0];
		tileToCheck.y += surroundingTilePositions[dir][1];

		if (_game->getSavedGame()->getSavedBattle()->getTile(tileToCheck)) // Make sure the tile is in bounds
		{
			BattleUnit* closeQuartersTarget = _game->getSavedGame()->getSavedBattle()->selectUnit(tileToCheck);
			// Variable for LOS check
			int checkDirection = tileEngine->getDirectionTo(tileToCheck, unit->getPosition());
			if (closeQuartersTarget && unit->getFaction() != closeQuartersTarget->getFaction() // Unit must exist and not be same faction
				&& closeQuartersTarget->getArmor()->getCreatesMeleeThreat() // Unit must be valid defender, 2x2 default false here
				&& closeQuartersTarget->getTimeUnits() >= _game->getMod()->getCloseQuartersTuCostGlobal() // Unit must have enough TUs
				&& closeQuartersTarget->getEnergy() >= _game->getMod()->getCloseQuartersEnergyCostGlobal() // Unit must have enough Energy
				&& tileEngine->validMeleeRange(closeQuartersTarget, unit, checkDirection) // Unit must be able to see the unit attempting to fire
				&& !(unit->getFaction() == FACTION_PLAYER && closeQuartersTarget->getFaction() == FACTION_NEUTRAL) // Civilians don't inhibit player
				&& !(unit->getFaction() == FACTION_NEUTRAL && closeQuartersTarget->getFaction() == FACTION_PLAYER) // Player doesn't inhibit civilians
				&& !(unit->getFaction() == FACTION_ALIEN_PLAYER && closeQuartersTarget->getFaction() == FACTION_NEUTRAL) // Civilians don't inhibit ALIEN player
				&& !(unit->getFaction() == FACTION_NEUTRAL && closeQuartersTarget->getFaction() == FACTION_ALIEN_PLAYER)) // ALIEN Player doesn't inhibit civilians
			{
				closeQuartersTargetList.push_back(closeQuartersTarget);
			}
		}
	}

	ss << tr(weapon->getRules()->getType()) << " > ";
	if (!closeQuartersTargetList.empty())
	{
		for (std::vector<BattleUnit*>::iterator bu = closeQuartersTargetList.begin(); bu != closeQuartersTargetList.end(); ++bu)
		{
			{
				BattleActionAttack attack;
				attack.type = BA_CQB;
				attack.attacker = unit;
				attack.weapon_item = weapon;
				attack.damage_item = weapon;
				attack.skill_rules = nullptr;
				int hitChance = BattleUnit::getFiringAccuracy(attack, _game->getMod());

				auto victim = (*bu);
				if (victim)
				{
					int arc = tileEngine->getArcDirection(tileEngine->getDirectionTo(victim->getPositionVexels(), unit->getPositionVexels()), victim->getDirection());
					float penalty = 1.0f - arc * victim->getArmor()->getMeleeDodgeBackPenalty() / 4.0f;
					if (penalty > 0)
					{
						hitChance -= victim->getArmor()->getMeleeDodge(victim) * penalty;
					}
				}
				ss << "cqcChance " << hitChance << "% ";
			}
		}
	}

	label->setText(ss.str());
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void AlienInventoryState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Toggles debug indicators.
 * @param action Pointer to an action.
 */
void AlienInventoryState::btnToggleClick(Action *)
{
	_txtLeftHand->setVisible(!_txtLeftHand->getVisible());
	_txtRightHand->setVisible(!_txtRightHand->getVisible());
}

/**
 * Opens Ufopaedia entry for the corresponding armor.
 * @param action Pointer to an action.
 */
void AlienInventoryState::btnArmorClickMiddle(Action *action)
{
	BattleUnit *unit = _inv->getSelectedUnit();
	if (unit != 0)
	{
		std::string articleId = unit->getArmor()->getUfopediaType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

}
