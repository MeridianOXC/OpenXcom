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
#include "InventoryState.h"
#include "../Basescape/SoldierArmorState.h"
#include "../Basescape/SoldierAvatarState.h"
#include "../Engine/Action.h"
#include "../Engine/Collections.h"
#include "../Engine/FileMap.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Menu/SaveGameState.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleSoldier.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "BattlescapeGenerator.h"
#include "BattlescapeState.h"
#include "ExtendedInventoryLinksState.h"
#include "Inventory.h"
#include "InventoryLoadState.h"
#include "InventoryPersonalState.h"
#include "InventorySaveState.h"
#include "TileEngine.h"
#include "UnitInfoState.h"
#include <algorithm>

#include <string>
#include <fstream>
#include <iostream>

namespace OpenXcom
{

static const int _templateBtnX = 288;
static const int _createTemplateBtnY = 90;
static const int _applyTemplateBtnY  = 113;

/**
 * Initializes all the elements in the Inventory screen.
 * @param game Pointer to the core game.
 * @param tu Does Inventory use up Time Units?
 * @param parent Pointer to parent Battlescape.
 */
InventoryState::InventoryState(bool tu, BattlescapeState *parent, Base *base, bool noCraft) :
	_tu(tu), _noCraft(noCraft), _parent(parent), _base(base),
	_resetCustomDeploymentBackup(false), _reloadUnit(false), _globalLayoutIndex(-1)
{
	_battleGame = _game->getSavedGame()->getSavedBattle();

	if (Options::oxceAlternateCraftEquipmentManagement && !_tu && _base && _noCraft)
	{
		// deassign all soldiers
		for (auto* soldier : *_base->getSoldiers())
		{
			_backup[soldier] = soldier->getCraft();
			if (soldier->getCraft() && soldier->getCraft()->getStatus() != "STR_OUT")
			{
				soldier->setCraftAndMoveEquipment(0, _base, _game->getSavedGame()->getMonthsPassed() == -1);
			}
		}
	}

	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}
	else if (_battleGame->isBaseCraftInventory())
	{
		Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
		_game->getScreen()->resetDisplay(false);
	}

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_soldier = new Surface(320, 200, 0, 0);
	_txtName = new TextEdit(this, 210, 17, 28, 6);
	_txtTus = new Text(40, 9, 245, 24);
	_txtWeight = new Text(70, 9, 245, 24);
	_txtStatLine1 = new Text(70, 9, 245, 32);
	_txtStatLine2 = new Text(70, 9, 245, 40);
	_txtStatLine3 = new Text(70, 9, 245, 48);
	_txtStatLine4 = new Text(70, 9, 245, 56);
	_txtItem = new Text(160, 9, 128, 140);
	_txtAmmo = new Text(66, 24, 254, 64);
	_btnOk = new BattlescapeButton(35, 22, 237, 1);
	_btnPrev = new BattlescapeButton(23, 22, 273, 1);
	_btnNext = new BattlescapeButton(23, 22, 297, 1);
	_btnUnload = new BattlescapeButton(32, 25, 288, 32);
	_btnGround = new BattlescapeButton(32, 15, 289, 137);
	_btnRank = new BattlescapeButton(26, 23, 0, 0);
	_btnArmor = new BattlescapeButton(RuleInventory::PAPERDOLL_W, RuleInventory::PAPERDOLL_H, RuleInventory::PAPERDOLL_X, RuleInventory::PAPERDOLL_Y);
	_btnCreateTemplate = new BattlescapeButton(32, 22, _templateBtnX, _createTemplateBtnY);
	_btnApplyTemplate = new BattlescapeButton(32, 22, _templateBtnX, _applyTemplateBtnY);
	Element* pixelShift = _game->getMod()->getInterface("inventory")->getElement("buttonLinks");
	if (pixelShift && pixelShift->TFTDMode)
	{
		_btnLinks = new BattlescapeButton(23, 22, 213, 0);
	}
	else
	{
		_btnLinks = new BattlescapeButton(23, 22, 213, 1);
	}
	_selAmmo = new Surface(RuleInventory::HAND_W * RuleInventory::SLOT_W, RuleInventory::HAND_H * RuleInventory::SLOT_H, 272, 88);
	_inv = new Inventory(_game, 320, 200, 0, 0, _parent == 0);
	_btnQuickSearch = new TextEdit(this, 40, 9, 244, 140);

	// Set palette
	setStandardPalette("PAL_BATTLESCAPE");

	add(_bg);

	// Set up objects
	_game->getMod()->getSurface("TAC01.SCR")->blitNShade(_bg, 0, 0);
	add(_btnArmor, "buttonOK", "inventory", _bg);

	add(_soldier);
	add(_btnQuickSearch, "textItem", "inventory");
	add(_txtName, "textName", "inventory", _bg);
	add(_txtTus, "textTUs", "inventory", _bg);
	add(_txtWeight, "textWeight", "inventory", _bg);
	add(_txtStatLine1, "textStatLine1", "inventory", _bg);
	add(_txtStatLine2, "textStatLine2", "inventory", _bg);
	add(_txtStatLine3, "textStatLine3", "inventory", _bg);
	add(_txtStatLine4, "textStatLine4", "inventory", _bg);
	add(_txtItem, "textItem", "inventory", _bg);
	add(_txtAmmo, "textAmmo", "inventory", _bg);
	add(_btnOk, "buttonOK", "inventory", _bg);
	add(_btnPrev, "buttonPrev", "inventory", _bg);
	add(_btnNext, "buttonNext", "inventory", _bg);
	add(_btnUnload, "buttonUnload", "inventory", _bg);
	add(_btnGround, "buttonGround", "inventory", _bg);
	add(_btnRank, "rank", "inventory", _bg);
	add(_btnCreateTemplate, "buttonCreate", "inventory", _bg);
	add(_btnApplyTemplate, "buttonApply", "inventory", _bg);
	add(_btnLinks, "buttonLinks", "inventory", _bg);
	add(_selAmmo);
	add(_inv);

	// move the TU display down to make room for the weight display
	if (Options::showMoreStatsInInventoryView)
	{
		_txtTus->setY(_txtTus->getY() + 8);
	}

	centerAllSurfaces();



	_txtName->setBig();
	_txtName->setHighContrast(true);
	_txtName->onChange((ActionHandler)&InventoryState::edtSoldierChange);
	_txtName->onMousePress((ActionHandler)&InventoryState::edtSoldierPress);

	_txtTus->setHighContrast(true);

	_txtWeight->setHighContrast(true);

	_txtStatLine1->setHighContrast(true);

	_txtStatLine2->setHighContrast(true);

	_txtStatLine3->setHighContrast(true);

	_txtStatLine4->setHighContrast(true);

	_txtItem->setHighContrast(true);

	_txtAmmo->setAlign(ALIGN_CENTER);
	_txtAmmo->setHighContrast(true);

	_btnOk->onMouseClick((ActionHandler)&InventoryState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyBattleInventory);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnUfopaediaClick, Options::keyGeoUfopedia);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnArmorClick, Options::keyInventoryArmor);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnArmorClickRight, Options::keyInventoryAvatar);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnInventoryLoadClick, Options::keyInventoryLoad);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnInventorySaveClick, Options::keyInventorySave);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnCreatePersonalTemplateClick, Options::keyInvSavePersonalEquipment);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnApplyPersonalTemplateClick, Options::keyInvLoadPersonalEquipment);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnShowPersonalTemplateClick, Options::keyInvShowPersonalEquipment);
	_btnOk->setTooltip("STR_OK");
	_btnOk->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnOk->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::invMouseOver, SDLK_LALT);
	_btnOk->onKeyboardRelease((ActionHandler)&InventoryState::invMouseOver, SDLK_LALT);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::invMouseOver, SDLK_RALT);
	_btnOk->onKeyboardRelease((ActionHandler)&InventoryState::invMouseOver, SDLK_RALT);

	_btnPrev->onMouseClick((ActionHandler)&InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&InventoryState::btnPrevClick, Options::keyBattlePrevUnit);
	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");
	_btnPrev->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnPrev->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnNext->onMouseClick((ActionHandler)&InventoryState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&InventoryState::btnNextClick, Options::keyBattleNextUnit);
	_btnNext->setTooltip("STR_NEXT_UNIT");
	_btnNext->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnNext->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnUnload->onMouseClick((ActionHandler)&InventoryState::btnUnloadClick);
	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");
	_btnUnload->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnUnload->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnGround->onMouseClick((ActionHandler)&InventoryState::btnGroundClickForward, SDL_BUTTON_LEFT);
	_btnGround->onMouseClick((ActionHandler)&InventoryState::btnGroundClickBackward, SDL_BUTTON_RIGHT);
	_btnGround->setTooltip("STR_SCROLL_RIGHT");
	_btnGround->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnGround->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);
	_btnGround->onKeyboardPress((ActionHandler)&InventoryState::btnGroundClickBackward, Options::keyBattleLeft);
	_btnGround->onKeyboardPress((ActionHandler)&InventoryState::btnGroundClickForward, Options::keyBattleRight);

	_btnRank->onMouseClick((ActionHandler)&InventoryState::btnRankClick);
	_btnRank->setTooltip("STR_UNIT_STATS");
	_btnRank->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnRank->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	if (!_game->getMod()->getInventoryOverlapsPaperdoll())
	{
		_btnArmor->onMouseClick((ActionHandler)&InventoryState::btnArmorClick);
		_btnArmor->onMouseClick((ActionHandler)&InventoryState::btnArmorClickRight, SDL_BUTTON_RIGHT);
		_btnArmor->onMouseClick((ActionHandler)&InventoryState::btnArmorClickMiddle, SDL_BUTTON_MIDDLE);
		_btnArmor->onMouseIn((ActionHandler)&InventoryState::txtArmorTooltipIn);
		_btnArmor->onMouseOut((ActionHandler)&InventoryState::txtArmorTooltipOut);
	}

	_btnCreateTemplate->onMouseClick((ActionHandler)&InventoryState::btnCreateTemplateClick);
	_btnCreateTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnCreateTemplateClick, Options::keyInvCreateTemplate);
	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");
	_btnCreateTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnCreateTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnApplyTemplate->onMouseClick((ActionHandler)&InventoryState::btnApplyTemplateClick);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnApplyTemplateClick, Options::keyInvApplyTemplate);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onClearInventory, Options::keyInvClear);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onAutoequip, Options::keyInvAutoEquip);
	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
	_btnApplyTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnApplyTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnLinks->onMouseClick((ActionHandler)&InventoryState::btnLinksClick);
	_btnLinks->setTooltip("STR_EXTENDED_LINKS");
	_btnLinks->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnLinks->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnQuickSearch->setHighContrast(true);
	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&InventoryState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&InventoryState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	_game->getMod()->getSurface("oxceLinksInv")->blitNShade(_btnLinks, 0, 0);
	_btnLinks->initSurfaces();
	_btnLinks->setVisible(Options::oxceLinks && !_tu);

	// only use copy/paste buttons in setup (i.e. non-tu) mode
	if (_tu)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
	}
	else
	{
		updateTemplateButtons(true);
	}

	_inv->draw();
	_inv->setTuMode(_tu);
	_inv->setSelectedUnit(_game->getSavedGame()->getSavedBattle()->getSelectedUnit(), true);
	_inv->onMouseClick((ActionHandler)&InventoryState::invClick, 0);
	_inv->onMouseOver((ActionHandler)&InventoryState::invMouseOver);
	_inv->onMouseOut((ActionHandler)&InventoryState::invMouseOut);

	if (_battleGame->getDebugMode() && _game->isShiftPressed())
	{
		// replenish TUs
		auto* unit = _inv->getSelectedUnit();
		if (unit)
		{
			int missingTUs = unit->getBaseStats()->tu - unit->getTimeUnits();
			unit->spendTimeUnits(-missingTUs);
		}
	}

	_txtTus->setVisible(_tu);
	_txtWeight->setVisible(Options::showMoreStatsInInventoryView);
	_txtStatLine1->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtStatLine2->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtStatLine3->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtStatLine4->setVisible(Options::showMoreStatsInInventoryView && !_tu);
}

static void _clearInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	Collections::deleteAll(inventoryTemplate);
}

/**
 *
 */
InventoryState::~InventoryState()
{
	_clearInventoryTemplate(_curInventoryTemplate);
	_clearInventoryTemplate(_tempInventoryTemplate);

	if (!_battleGame->isBaseCraftInventory())
	{
		if (Options::maximizeInfoScreens)
		{
			Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
			_game->getScreen()->resetDisplay(false);
		}

		//fix case when scripts could kill unit before inventory is closed
		if (BattleUnit* unit =_battleGame->getSelectedUnit())
		{
			Tile *inventoryTile = unit->getTile();
			_battleGame->getTileEngine()->applyGravity(inventoryTile);
		}
		_battleGame->getTileEngine()->calculateLighting(LL_ITEMS); // dropping/picking up flares
		_battleGame->getTileEngine()->recalculateFOV();
	}
	else
	{
		Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, true);
		_game->getScreen()->resetDisplay(false);
	}
}

void InventoryState::setGlobalLayoutIndex(int index, bool armorChanged)
{
	_globalLayoutIndex = index;
	if (armorChanged)
	{
		_reloadUnit = true;
	}
}

/**
 * Updates all soldier stats when the soldier changes.
 */
void InventoryState::init()
{
	State::init();
	BattleUnit *unit = _battleGame->getSelectedUnit();

	// no selected unit, close inventory
	if (unit == 0)
	{
		btnOkClick(0);
		return;
	}
	// skip to the first unit with inventory
	if (!unit->hasInventory())
	{
		if (_parent)
		{
			_parent->selectNextPlayerUnit(false, false, true, _tu);
		}
		else
		{
			_battleGame->selectNextPlayerUnit(false, false, true);
		}
		// no available unit, close inventory
		if (_battleGame->getSelectedUnit() == 0 || !_battleGame->getSelectedUnit()->hasInventory())
		{
			// starting a mission with just vehicles
			btnOkClick(0);
			return;
		}
		else
		{
			unit = _battleGame->getSelectedUnit();
		}
	}

	_soldier->clear();
	_btnRank->clear();

	_txtName->setBig();
	_txtName->setText(unit->getName(_game->getLanguage()));

	_btnLinks->setVisible(Options::oxceLinks && !_tu);

	bool resetGroundOffset = _tu;
	if (unit->isSummonedPlayerUnit())
	{
		resetGroundOffset = true; // this unit is likely not standing on the shared inventory tile, just re-arrange it every time
	}
	_inv->setSelectedUnit(unit, resetGroundOffset);
	Soldier *s = unit->getGeoscapeSoldier();
	if (s)
	{
		// reload necessary after the change of armor
		if (_reloadUnit)
		{
			if (Options::oxceAlternateCraftEquipmentManagement && s->getArmor() && unit->getArmor() && s->getArmor()->getSize() > unit->getArmor()->getSize())
			{
				_resetCustomDeploymentBackup = true;
			}

			// Step 0: update unit's armor
			unit->updateArmorFromSoldier(_game->getMod(), s, s->getArmor(), _battleGame->getDepth(), false, nullptr);

			// Step 1: remember the unit's equipment (incl. loaded fixed items)
			_clearInventoryTemplate(_tempInventoryTemplate);
			_createInventoryTemplate(_tempInventoryTemplate);

			// Step 2: drop all items (and delete fixed items!!)
			Tile *groundTile = unit->getTile();
			_battleGame->getTileEngine()->itemDropInventory(groundTile, unit, true, true);

			// Step 3: equip fixed items // Note: the inventory must be *completely* empty before this step
			_battleGame->initUnit(unit);

			// Step 4: re-equip original items (unless slots taken by fixed items)
			_applyInventoryTemplate(_tempInventoryTemplate);

			// refresh ui
			_inv->arrangeGround(); // calls drawItems() too

			// reset armor tooltip
			_currentTooltip = "";
			_txtItem->setText("");

			// reload done
			_reloadUnit = false;
		}

		SurfaceSet *texture = _game->getMod()->getSurfaceSet("SMOKE.PCK");
		auto* frame = texture->getFrame(s->getRankSpriteBattlescape());
		if (frame)
		{
			frame->blitNShade(_btnRank, 0, 0);
		}

		if (s->getArmor()->hasLayersDefinition())
		{
			for (const auto& layer : s->getArmorLayers())
			{
				_game->getMod()->getSurface(layer, true)->blitNShade(_soldier, 0, 0);
			}
		}
		else
		{
			const std::string look = s->getArmor()->getSpriteInventory();
			const std::string gender = s->getGender() == GENDER_MALE ? "M" : "F";
			Surface *surf = 0;
			std::stringstream ss;

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

	// coming from InventoryLoad window...
	if (_globalLayoutIndex > -1)
	{
		loadGlobalLayout((_globalLayoutIndex));
		_globalLayoutIndex = -1;

		// refresh ui
		_inv->arrangeGround();
	}

	updateStats();
	refreshMouse();
}

/**
 * Disables the input, if not a soldier. Sets the name without a statstring otherwise.
 * @param action Pointer to an action.
 */
void InventoryState::edtSoldierPress(Action *action)
{
	if (_btnLinks->getVisible())
	{
		double mx = action->getAbsoluteXMouse();
		if (mx >= _btnLinks->getX())
		{
			_txtName->setFocus(false);
			return;
		}
		else
		{
			_btnLinks->setVisible(false);
		}
	}

	{
		BattleUnit *unit = _inv->getSelectedUnit();
		if (unit != 0)
		{
			Soldier *s = unit->getGeoscapeSoldier();
			if (s)
			{
				// set the soldier's name without a statstring
				_txtName->setText(s->getName());
			}

		}
	}
}

/**
 * Changes the soldier's name.
 * @param action Pointer to an action.
 */
void InventoryState::edtSoldierChange(Action *)
{
	BattleUnit *unit = _inv->getSelectedUnit();
	if (unit != 0)
	{
		Soldier *s = unit->getGeoscapeSoldier();
		if (s)
		{
			// set the soldier's name
			s->setName(_txtName->getText());
			// also set the unit's name (with a statstring)
			unit->setName(s->getName(true));
		}
	}
}

/**
 * Updates the soldier stats (Weight, TU).
 */
void InventoryState::updateStats()
{
	BattleUnit *unit = _battleGame->getSelectedUnit();

	_txtTus->setText(tr("STR_TIME_UNITS_SHORT").arg(unit->getTimeUnits()));

	int weight = unit->getCarriedWeight(_inv->getSelectedItem());
	_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(unit->getBaseStats()->strength));
	if (weight > unit->getBaseStats()->strength)
	{
		_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color2);
	}
	else
	{
		_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color);
	}

	auto psiSkillWithoutAnyBonuses = unit->getBaseStats()->psiSkill;
	if (unit->getGeoscapeSoldier())
	{
		psiSkillWithoutAnyBonuses = unit->getGeoscapeSoldier()->getCurrentStats()->psiSkill;
	}
	bool showPsiStrength = (psiSkillWithoutAnyBonuses > 0 || (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements())));

	auto updateStatLine = [&](Text* txtField, const std::string& elementId)
	{
		Element *element = _game->getMod()->getInterface("inventory")->getElement(elementId);
		if (element)
		{
			switch (element->custom)
			{
				case 1:
					txtField->setText(tr("STR_ACCURACY_SHORT").arg(unit->getBaseStats()->firing));
					break;
				case 2:
					txtField->setText(tr("STR_REACTIONS_SHORT").arg(unit->getBaseStats()->reactions));
					break;
				case 3:
					if (psiSkillWithoutAnyBonuses > 0)
						txtField->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(unit->getBaseStats()->psiSkill));
					else
						txtField->setText("");
					break;
				case 4:
					if (showPsiStrength)
						txtField->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(unit->getBaseStats()->psiStrength));
					else
						txtField->setText("");
					break;
				case 11:
					txtField->setText(tr("STR_FIRING_SHORT").arg(unit->getBaseStats()->firing));
					break;
				case 12:
					txtField->setText(tr("STR_THROWING_SHORT").arg(unit->getBaseStats()->throwing));
					break;
				case 13:
					txtField->setText(tr("STR_MELEE_SHORT").arg(unit->getBaseStats()->melee));
					break;
				case 14:
					if (showPsiStrength)
					{
						txtField->setText(tr("STR_PSI_SHORT")
							.arg(unit->getBaseStats()->psiStrength)
							.arg(unit->getBaseStats()->psiSkill > 0 ? unit->getBaseStats()->psiSkill : 0));
					}
					else
					{
						txtField->setText("");
					}
					break;
				default:
					txtField->setText("");
					break;
			}
		}
	};

	updateStatLine(_txtStatLine1, "textStatLine1");
	updateStatLine(_txtStatLine2, "textStatLine2");
	updateStatLine(_txtStatLine3, "textStatLine3");
	updateStatLine(_txtStatLine4, "textStatLine4");
}

/**
 * Saves the soldiers' equipment-layout.
 */
void InventoryState::saveEquipmentLayout()
{
	for (auto* bu : *_battleGame->getUnits())
	{
		// we need X-Com soldiers only
		if (bu->getGeoscapeSoldier() == 0) continue;

		std::vector<EquipmentLayoutItem*> *layoutItems = bu->getGeoscapeSoldier()->getEquipmentLayout();

		// clear the previous save
		if (!layoutItems->empty())
		{
			for (auto* equipmentLayoutItem : *layoutItems)
			{
				delete equipmentLayoutItem;
			}
			layoutItems->clear();
		}

		// save the soldier's items
		// note: with using getInventory() we are skipping the ammos loaded, (they're not owned) because we handle the loaded-ammos separately (inside)
		for (auto* bi : *bu->getInventory())
		{
			// skip fixed items
			if (bi->getRules()->isFixed())
			{
				bool loaded = bi->needsAmmoForSlot(0) && bi->getAmmoForSlot(0);
				if (!loaded) continue;
			}
			else if (!bi->getRules()->canBeEquippedToCraftInventory())
			{
				// skip other not-allowed items, which somehow made it here (e.g. were spawned instead of equipped)
				continue;
			}

			layoutItems->push_back(new EquipmentLayoutItem(bi));
		}
	}
}

/**
 * Opens the Armor Selection GUI
 * @param action Pointer to an action.
 */
void InventoryState::btnArmorClick(Action *action)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// only allowed during base equipment
	if (_base == 0)
	{
		return;
	}

	// equipment in the base
	BattleUnit *unit = _battleGame->getSelectedUnit();
	Soldier *s = unit->getGeoscapeSoldier();

	if (!(s->getCraft() && s->getCraft()->getStatus() == "STR_OUT"))
	{
		size_t soldierIndex = 0;
		for (auto soldierIt = _base->getSoldiers()->begin(); soldierIt != _base->getSoldiers()->end(); ++soldierIt)
		{
			if ((*soldierIt)->getId() == s->getId())
			{
				soldierIndex = soldierIt - _base->getSoldiers()->begin();
			}
		}

		_reloadUnit = true;
		_game->pushState(new SoldierArmorState(_base, soldierIndex, SA_BATTLESCAPE));
	}
}

/**
 * Opens the Avatar Selection GUI
 * @param action Pointer to an action.
 */
void InventoryState::btnArmorClickRight(Action *action)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// only allowed during base equipment
	if (_base == 0)
	{
		return;
	}

	// equipment in the base
	BattleUnit *unit = _battleGame->getSelectedUnit();
	Soldier *s = unit->getGeoscapeSoldier();

	if (!(s->getCraft() && s->getCraft()->getStatus() == "STR_OUT"))
	{
		size_t soldierIndex = 0;
		for (auto soldierIt = _base->getSoldiers()->begin(); soldierIt != _base->getSoldiers()->end(); ++soldierIt)
		{
			if ((*soldierIt)->getId() == s->getId())
			{
				soldierIndex = soldierIt - _base->getSoldiers()->begin();
			}
		}

		_game->pushState(new SoldierAvatarState(_base, soldierIndex));
	}
}

/**
* Opens Ufopaedia entry for the corresponding armor.
* @param action Pointer to an action.
*/
void InventoryState::btnArmorClickMiddle(Action *action)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit *unit = _inv->getSelectedUnit();
	if (unit != 0)
	{
		std::string articleId = unit->getArmor()->getUfopediaType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

void InventoryState::saveGlobalLayout(int index, bool includingArmor)
{
	std::vector<EquipmentLayoutItem*> *tmpl = _game->getSavedGame()->getGlobalEquipmentLayout(index);

	// clear current template
	_clearInventoryTemplate(*tmpl);

	// create new template
	_createInventoryTemplate(*tmpl);

	// optionally save armor info too
	if (includingArmor && _battleGame->getSelectedUnit()->getGeoscapeSoldier())
	{
		_game->getSavedGame()->setGlobalEquipmentLayoutArmor(index, _battleGame->getSelectedUnit()->getArmor()->getType());
	}
	else
	{
		_game->getSavedGame()->setGlobalEquipmentLayoutArmor(index, std::string());
	}
}

void InventoryState::loadGlobalLayout(int index)
{
	std::vector<EquipmentLayoutItem*> *tmpl = _game->getSavedGame()->getGlobalEquipmentLayout(index);

	_applyInventoryTemplate(*tmpl);
}

bool InventoryState::loadGlobalLayoutArmor(int index)
{
	auto& armorName = _game->getSavedGame()->getGlobalEquipmentLayoutArmor(index);
	return tryArmorChange(armorName);
}

bool InventoryState::tryArmorChange(const std::string& armorName)
{
	Armor* prev = nullptr;
	Soldier* soldier = nullptr;
	BattleUnit* unit = _inv->getSelectedUnit();
	if (unit)
	{
		soldier = unit->getGeoscapeSoldier();
		if (soldier)
		{
			prev = soldier->getArmor();
		}
	}

	Armor* next = nullptr;
	{
		next = _game->getMod()->getArmor(armorName, false);
	}

	// check armor availability
	bool armorAvailable = false;
	if (prev && next && next != prev && soldier && _base)
	{
		armorAvailable = true;
		if (_game->getSavedGame()->getMonthsPassed() != -1)
		{
			// is the armor physically available?
			if (next->getStoreItem() && prev->getStoreItem() != next->getStoreItem())
			{
				if (_base->getStorageItems()->getItem(next->getStoreItem()) <= 0)
				{
					armorAvailable = false;
				}
			}
			// is the armor unlocked?
			if (next->getRequiredResearch() && !_game->getSavedGame()->isResearched(next->getRequiredResearch()))
			{
				armorAvailable = false;
			}
		}
		// does the armor fit on the current unit?
		if (!next->getCanBeUsedBy(soldier->getRules()))
		{
			armorAvailable = false;
		}
	}

	// change armor
	bool armorChanged = false;
	if (armorAvailable)
	{
		Craft* craft = soldier->getCraft();
		if (craft)
		{
			if (!craft->validateArmorChange(prev->getSize(), next->getSize()))
			{
				// STR_NOT_ENOUGH_CRAFT_SPACE
				return false;
			}
		}
		if (_game->getSavedGame()->getMonthsPassed() != -1)
		{
			if (prev->getStoreItem())
			{
				_base->getStorageItems()->addItem(prev->getStoreItem());
			}
			if (next->getStoreItem())
			{
				_base->getStorageItems()->removeItem(next->getStoreItem());
			}
		}
		if (Options::oxceAlternateCraftEquipmentManagement && next->getSize() > prev->getSize())
		{
			_resetCustomDeploymentBackup = true;
		}
		soldier->setArmor(next, true);
		armorChanged = true;
	}

	return armorChanged;
}

/**
* Handles global equipment layout actions.
* @param action Pointer to an action.
*/
void InventoryState::btnGlobalEquipmentLayoutClick(Action *action)
{
	// cannot use this feature during the mission!
	if (_tu)
	{
		return;
	}

	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// SDLK_0 = 48, SDLK_1 = 49, SDLK_9 = 57
	// SDLK_0 selects the 10-th inventory layout
	// by repeating a key you can load a layout from the next decade
	const int sym = action->getDetails()->key.keysym.sym;
	const int layout_no = (sym == 48) ? 10 : sym - 48;

	if (sym == _prev_key && !_game->isCtrlPressed())
	{
		_key_repeats++;
	}
	else
	{
		_key_repeats = 0;
	}
	_prev_key = sym;

	const int index = 10 * _key_repeats + layout_no - 1;

	if (index < 0 || index >= Options::oxceMaxEquipmentLayoutTemplates)
	{
		// do nothing if the layout index is out of bounds
		return;
	}

	if (_game->isCtrlPressed())
	{
		// can't save layout >10 this way
		_prev_key = 0, _key_repeats = 0;
		saveGlobalLayout(index, false);

		// give audio feedback
		_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
		refreshMouse();
	}
	else
	{
		// simulate what happens when loading via the InventoryLoadState dialog
		bool armorChanged = loadGlobalLayoutArmor(index);
		setGlobalLayoutIndex(index, armorChanged);
		init();

		// give audio feedback
		_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
	}
}

/**
* Opens the InventoryLoad screen.
* @param action Pointer to an action.
*/
void InventoryState::btnInventoryLoadClick(Action *)
{
	// cannot use this feature during the mission!
	if (_tu)
	{
		return;
	}

	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	_game->pushState(new InventoryLoadState(this));
}

/**
* Opens the InventorySave screen.
* @param action Pointer to an action.
*/
void InventoryState::btnInventorySaveClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	_game->pushState(new InventorySaveState(this));
}

/**
 * Opens the Ufopaedia.
 * @param action Pointer to an action.
 */
void InventoryState::btnUfopaediaClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	Ufopaedia::open(_game);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnOkClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
		return;
	_game->popState();
	if (!_tu)
	{
		if (_base || !Options::oxceAlternateCraftEquipmentManagement)
		{
			saveEquipmentLayout();
		}
		if (Options::oxceAlternateCraftEquipmentManagement && !_tu && _base && _noCraft)
		{
			// assign all soldiers back, if possible
			for (auto* soldier : *_base->getSoldiers())
			{
				Craft* c = _backup[soldier];
				if (!soldier->getCraft() && c && c->getStatus() != "STR_OUT")
				{
					int space = c->getSpaceAvailable();
					if (c->validateAddingSoldier(space, soldier))
					{
						soldier->setCraftAndMoveEquipment(c, _base, _game->getSavedGame()->getMonthsPassed() == -1, _resetCustomDeploymentBackup);
					}
				}
			}
		}
		if (_parent)
		{
			_battleGame->startFirstTurn();
		}
	}
}

/**
 * Selects the previous soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnPrevClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	if (_parent)
	{
		_parent->selectPreviousPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectPreviousPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnNextClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}
	if (_parent)
	{
		_parent->selectNextPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectNextPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Unloads the selected weapon.
 * @param action Pointer to an action.
 */
void InventoryState::btnUnloadClick(Action *)
{
	if (_inv->unload(false))
	{
		_txtItem->setText("");
		_txtAmmo->setText("");
		_selAmmo->clear();
		updateStats();
		_game->getMod()->getSoundByDepth(0, Mod::ITEM_DROP)->play();
	}
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void InventoryState::btnQuickSearchToggle(Action *action)
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
void InventoryState::btnQuickSearchApply(Action *)
{
	_inv->setSearchString(_btnQuickSearch->getText());
}

/**
 * Shows more ground items / rearranges them.
 * @param action Pointer to an action.
 */
void InventoryState::btnGroundClickForward(Action *action)
{
	if (_game->isShiftPressed())
	{
		// scroll backwards
		_inv->arrangeGround(-1);
	}
	else
	{
		// scroll forward
		_inv->arrangeGround(1);
	}
}

/**
 * Shows more ground items / rearranges them.
 * @param action Pointer to an action.
 */
void InventoryState::btnGroundClickBackward(Action *action)
{
	// scroll backwards
	_inv->arrangeGround(-1);
}

/**
 * Shows the unit info screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnRankClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	_game->pushState(new UnitInfoState(_battleGame->getSelectedUnit(), _parent, true, false));
}

void InventoryState::_createInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	// copy inventory instead of just keeping a pointer to it.  that way
	// create/apply can be used as an undo button for a single unit and will
	// also work as expected if inventory is modified after 'create' is clicked
	for (const auto* bi : *_battleGame->getSelectedUnit()->getInventory())
	{
		// skip fixed items
		if (bi->getRules()->isFixed())
		{
			bool loaded = bi->needsAmmoForSlot(0) && bi->getAmmoForSlot(0);
			if (!loaded) continue;
		}
		else if (!bi->getRules()->canBeEquippedToCraftInventory())
		{
			// skip other not-allowed items, which somehow made it here (e.g. were spawned instead of equipped)
			continue;
		}

		inventoryTemplate.push_back(new EquipmentLayoutItem(bi));
	}
}

void InventoryState::btnLinksClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	_game->pushState(new ExtendedInventoryLinksState(this, _battleGame, _base, !_tu));
}

void InventoryState::btnCreateTemplateClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// clear current template
	_clearInventoryTemplate(_curInventoryTemplate);

	// create new template
	_createInventoryTemplate(_curInventoryTemplate);

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
	refreshMouse();
}

void InventoryState::btnCreatePersonalTemplateClick(Action *)
{
	// cannot use this feature during the mission!
	if (_tu)
	{
		return;
	}

	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	auto* unit = _battleGame->getSelectedUnit();
	if (unit && unit->getGeoscapeSoldier())
	{
		auto& personalTemplate = *unit->getGeoscapeSoldier()->getPersonalEquipmentLayout();

		// clear current personal template
		_clearInventoryTemplate(personalTemplate);

		// create new personal template
		_createInventoryTemplate(personalTemplate);

		// optionally save armor info too
		if (Options::oxcePersonalLayoutIncludingArmor)
		{
			unit->getGeoscapeSoldier()->setPersonalEquipmentArmor(_battleGame->getSelectedUnit()->getArmor());
		}
		else
		{
			unit->getGeoscapeSoldier()->setPersonalEquipmentArmor(nullptr);
		}

		// give visual feedback
		_inv->showWarning(tr("STR_PERSONAL_EQUIPMENT_SAVED"));

		// give audio feedback
		_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
		refreshMouse();
	}
}

void InventoryState::_applyInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*> *groundInv     = groundTile->getInventory();

	_battleGame->getTileEngine()->itemDropInventory(groundTile, unit, true, false);

	// attempt to replicate inventory template by grabbing corresponding items
	// from the ground.  if any item is not found on the ground, display warning
	// message, but continue attempting to fulfill the template as best we can
	bool itemMissing = false;
	for (const auto* equipmentLayoutItem : inventoryTemplate)
	{
		// search for template item in ground inventory
		bool found = false;

		bool needsAmmo[RuleItem::AmmoSlotMax] = { };
		std::string targetAmmo[RuleItem::AmmoSlotMax] = { };
		BattleItem *matchedWeapon = nullptr;
		BattleItem *matchedAmmo[RuleItem::AmmoSlotMax] = { };

		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			targetAmmo[slot] = equipmentLayoutItem->getAmmoItemForSlot(slot);
			needsAmmo[slot] = (targetAmmo[slot] != "NONE");
			matchedAmmo[slot] = nullptr;
		}

		for (BattleItem* groundItem : *groundInv)
		{
			// if we find the appropriate ammo, remember it for later for if we find
			// the right weapon but with the wrong ammo
			const std::string groundItemName = groundItem->getRules()->getType();

			bool skipAmmo = false;
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				if (needsAmmo[slot] && !matchedAmmo[slot] && targetAmmo[slot] == groundItemName)
				{
					matchedAmmo[slot] = groundItem;
					skipAmmo = true;
				}
			}
			if (skipAmmo)
			{
				continue;
			}

			if (equipmentLayoutItem->isFixed() == false && equipmentLayoutItem->getItemType() == groundItemName)
			{
				// if the loaded ammo doesn't match the template item's,
				// remember the weapon for later and continue scanning
				bool skipWeapon = false;
				for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
				{
					if (!groundItem->needsAmmoForSlot(slot))
					{
						continue;
					}
					BattleItem *loadedAmmo = groundItem->getAmmoForSlot(slot);
					if ((needsAmmo[slot] && (!loadedAmmo || targetAmmo[slot] != loadedAmmo->getRules()->getType()))
						|| (!needsAmmo[slot] && loadedAmmo))
					{
						// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
						if (!matchedWeapon || matchedWeapon->getAmmoForSlot(slot))
						{
							matchedWeapon = groundItem;
						}
						skipWeapon = true;
					}
				}
				if (!skipWeapon)
				{
					matchedWeapon = groundItem;
					found = true; // found = true, even if not equipped
					break;
				}
			}
		}

		if (equipmentLayoutItem->isFixed())
		{
			for (BattleItem* fixedItem : *unit->getInventory())
			{
				if (fixedItem->getRules()->isFixed() == false)
				{
					// this is not a fixed item, continue searching...
					continue;
				}
				if (fixedItem->getSlot()->getId() == equipmentLayoutItem->getSlot() &&
					fixedItem->getSlotX() == equipmentLayoutItem->getSlotX() &&
					fixedItem->getSlotY() == equipmentLayoutItem->getSlotY() &&
					fixedItem->getRules()->getType() == equipmentLayoutItem->getItemType())
				{
					// if the loaded ammo doesn't match the template item's,
					// remember the weapon for later and continue scanning
					bool skipWeapon = false;
					for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
					{
						if (!fixedItem->needsAmmoForSlot(slot))
						{
							continue;
						}
						BattleItem* loadedAmmo = fixedItem->getAmmoForSlot(slot);
						if ((needsAmmo[slot] && (!loadedAmmo || targetAmmo[slot] != loadedAmmo->getRules()->getType()))
							|| (!needsAmmo[slot] && loadedAmmo))
						{
							// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
							if (!matchedWeapon || matchedWeapon->getAmmoForSlot(slot))
							{
								matchedWeapon = fixedItem;
							}
							skipWeapon = true;
						}
					}
					if (!skipWeapon)
					{
						matchedWeapon = fixedItem;
						found = true; // found = true, even if not equipped
						break;
					}
				}
			}
		}

		// if we failed to find an exact match, but found unloaded ammo and
		// the right weapon, unload the target weapon, load the right ammo, and use it
		if (!found && matchedWeapon)
		{
			found = true;
			bool allMatch = true;
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				allMatch &= (needsAmmo[slot] && matchedAmmo[slot]) || (!needsAmmo[slot]);
			}
			if (allMatch)
			{
				for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
				{
					if (matchedWeapon->needsAmmoForSlot(slot) && (!needsAmmo[slot] || matchedAmmo[slot]))
					{
						// unload the existing ammo (if any) from the weapon
						BattleItem *loadedAmmo = matchedWeapon->setAmmoForSlot(slot, matchedAmmo[slot]);
						if (loadedAmmo)
						{
							_battleGame->getTileEngine()->itemDrop(groundTile, loadedAmmo, false);
						}
					}
				}
			}
			else
			{
				// nope we can't do it.
				found = false;
				matchedWeapon = nullptr;
			}
		}

		if (!found)
		{
			itemMissing = true;
		}

		if (equipmentLayoutItem->isFixed())
		{
			// we have loaded the fixed weapon (if possible) and we don't need to do anything else, it's already in the correct slot
			continue;
		}

		// check if the slot is not occupied already (e.g. by a fixed weapon)
		if (matchedWeapon && !_inv->overlapItems(
			unit,
			matchedWeapon,
			_game->getMod()->getInventory(equipmentLayoutItem->getSlot(), true),
			equipmentLayoutItem->getSlotX(),
			equipmentLayoutItem->getSlotY()))
		{
			// move matched item from ground to the appropriate inventory slot
			matchedWeapon->moveToOwner(unit);
			matchedWeapon->setSlot(_game->getMod()->getInventory(equipmentLayoutItem->getSlot()));
			matchedWeapon->setSlotX(equipmentLayoutItem->getSlotX());
			matchedWeapon->setSlotY(equipmentLayoutItem->getSlotY());
			matchedWeapon->setFuseTimer(equipmentLayoutItem->getFuseTimer());
		}
		else
		{
			// let the user know or not? probably not... should be obvious why
		}
	}

	if (itemMissing)
	{
		_inv->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));
	}
}

void InventoryState::btnApplyTemplateClick(Action *)
{
	// don't accept clicks when moving items
	// it's ok if the template is empty -- it will just result in clearing the
	// unit's inventory
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	_applyInventoryTemplate(_curInventoryTemplate);

	// refresh ui
	_inv->arrangeGround();
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::btnApplyPersonalTemplateClick(Action *)
{
	// cannot use this feature during the mission!
	if (_tu)
	{
		return;
	}

	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	auto* unit = _battleGame->getSelectedUnit();
	if (unit && unit->getGeoscapeSoldier())
	{
		// optionally load armor too
		if (Options::oxcePersonalLayoutIncludingArmor)
		{
			auto* newArmor = unit->getGeoscapeSoldier()->getPersonalEquipmentArmor();
			if (newArmor && newArmor != unit->getArmor())
			{
				bool success = tryArmorChange(newArmor->getType());

				if (success)
				{
					_reloadUnit = true;
					init();
				}
				else
				{
					// FIXME: a better message? or no message?
					//_inv->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));
				}
			}
		}

		auto& personalTemplate = *unit->getGeoscapeSoldier()->getPersonalEquipmentLayout();

		_applyInventoryTemplate(personalTemplate);

		// refresh ui
		_inv->arrangeGround();
		updateStats();
		refreshMouse();

		// give audio feedback
		_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
	}
}

void InventoryState::btnShowPersonalTemplateClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	auto* unit = _battleGame->getSelectedUnit();
	if (unit && unit->getGeoscapeSoldier())
	{
		_game->pushState(new InventoryPersonalState(unit->getGeoscapeSoldier()));
	}
}

void InventoryState::refreshMouse()
{
	// send a mouse motion event to refresh any hover actions
	int x, y;
	SDL_GetMouseState(&x, &y);
	SDL_WarpMouse(x+1, y);

	// move the mouse back to avoid cursor creep
	SDL_WarpMouse(x, y);
}

void InventoryState::onClearInventory(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit       = _battleGame->getSelectedUnit();
	Tile                     *groundTile = unit->getTile();

	_battleGame->getTileEngine()->itemDropInventory(groundTile, unit, true, false);

	// refresh ui
	_inv->arrangeGround();
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::onAutoequip(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*>  groundInv     = *groundTile->getInventory();
	Mod                      *mod           = _game->getMod();
	RuleInventory            *groundRuleInv = mod->getInventoryGround();
	int                       worldShade    = _battleGame->getGlobalShade();

	std::vector<BattleUnit*> units;
	units.push_back(unit);
	BattlescapeGenerator::autoEquip(units, mod, &groundInv, groundRuleInv, worldShade, true, true);

	// refresh ui
	_inv->arrangeGround();
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::onGenerateBackstory(Action *)
{
	YAML::Node payloadNode = _battleGame->getSelectedUnit()->getGeoscapeSoldier()->asYaml();
	std::string command = "generateCharacterBackstory";
	std::string payloadPath = Options::getMasterUserFolder() + "payload.yaml";

	// Payload
	std::ofstream outFile(payloadPath);
	outFile << payloadNode;
	outFile.close();

	// Command
	std::string pythonCommand = "python3 " + Options::getMasterUserFolder() + "main.py \"" + command + "\" \"" + payloadPath + "\"";
	system(pythonCommand.c_str());

	// refresh ui
	_inv->arrangeGround();
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

/**
 * Updates item info.
 * @param action Pointer to an action.
 */
void InventoryState::invClick(Action *act)
{
	updateStats();
	_prev_key = 0, _key_repeats = 0;
}

/**
 * Calculates item damage info.
 */
void InventoryState::calculateCurrentDamageTooltip()
{
	// Differences against battlescape indicator:
	// 1. doesn't consider which action (auto/snap/aim/melee) is used... just takes ammo from primary slot
	// 2. doesn't show psi success chance (distance is unknown)
	// 3. doesn't consider range power reduction (distance is unknown)

	const BattleUnit *currentUnit = _inv->getSelectedUnit();

	if (!currentUnit)
		return;

	if (!_currentDamageTooltipItem)
		return;

	const BattleItem *damageItem = _currentDamageTooltipItem;
	const RuleItem *weaponRule = _currentDamageTooltipItem->getRules();
	const int PRIMARY_SLOT = 0;

	// step 1: determine rule
	const RuleItem *rule;
	if (weaponRule->getBattleType() == BT_PSIAMP)
	{
		rule = weaponRule;
	}
	else if (_currentDamageTooltipItem->needsAmmoForSlot(PRIMARY_SLOT))
	{
		auto* ammo = _currentDamageTooltipItem->getAmmoForSlot(PRIMARY_SLOT);
		if (ammo != nullptr)
		{
			damageItem = ammo;
			rule = ammo->getRules();
		}
		else
		{
			rule = 0; // empty weapon = no rule
		}
	}
	else
	{
		rule = weaponRule;
	}

	// step 2: check if unlocked
	if (_game->getSavedGame()->getMonthsPassed() == -1)
	{
		// new battle mode
	}
	else if (rule)
	{
		// instead of checking the weapon/ammo itself... we're checking their ufopedia articles here
		// same as for the battlescape indicator
		// it's arguable if this is the correct approach, but so far this is what we have
		ArticleDefinition *article = _game->getMod()->getUfopaediaArticle(rule->getType(), false);
		if (article && !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
		{
			// ammo/weapon locked
			rule = 0;
		}
		if (rule && rule->getType() != weaponRule->getType())
		{
			article = _game->getMod()->getUfopaediaArticle(weaponRule->getType(), false);
			if (article && !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
			{
				// weapon locked
				rule = 0;
			}
		}
	}

	// step 3: calculate and remember
	if (rule)
	{
		if (rule->getBattleType() != BT_CORPSE)
		{
			int totalDamage = 0;
			totalDamage += rule->getPowerBonus({ BA_NONE, currentUnit, _currentDamageTooltipItem, damageItem }); //TODO: find what exactly attack we can do
			//totalDamage -= rule->getPowerRangeReduction(distance * 16);
			if (totalDamage < 0) totalDamage = 0;
			std::ostringstream ss;
			ss << rule->getDamageType()->getRandomDamage(totalDamage, 1);
			ss << "-";
			ss << rule->getDamageType()->getRandomDamage(totalDamage, 2);
			if (rule->getDamageType()->RandomType == DRT_UFO_WITH_TWO_DICE)
				ss << "*";
			_currentDamageTooltip = tr("STR_DAMAGE_UC_").arg(ss.str());
		}
	}
	else
	{
		_currentDamageTooltip = tr("STR_DAMAGE_UC_").arg(tr("STR_UNKNOWN"));
	}
}
/**
 * Shows item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOver(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	bool altPressed = _game->isAltPressed();
	bool currentDamageTooltipItemChanged = false;

	BattleItem *item = _inv->getMouseOverItem();
	if (item != _mouseHoverItem)
	{
		_mouseHoverItemFrame = _inv->getAnimFrame();
		_mouseHoverItem = item;
	}
	if (altPressed)
	{
		if (item != _currentDamageTooltipItem)
		{
			currentDamageTooltipItemChanged = true;
			_currentDamageTooltipItem = item;
			_currentDamageTooltip = "";
		}
	}
	else
	{
		_currentDamageTooltipItem = nullptr;
		_currentDamageTooltip = "";
	}
	if (item != 0)
	{
		std::string itemName;
		if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
		{
			itemName = item->getUnit()->getName(_game->getLanguage());
		}
		else
		{
			auto* save = _game->getSavedGame();
			if (save->isResearched(item->getRules()->getRequirements()))
			{
				std::string text = tr(item->getRules()->getName());
				for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
				{
					if (!item->needsAmmoForSlot(slot))
					{
						continue;
					}

					auto* ammo = item->getAmmoForSlot(slot);
					if (!ammo || !save->isResearched(ammo->getRules()->getRequirements()))
					{
						continue;
					}

					const auto& ammoName = ammo->getRules()->getNameAsAmmo();
					if (!ammoName.empty())
					{
						text += " ";
						text += tr(ammoName);
					}
				}
				if (altPressed)
				{
					if (currentDamageTooltipItemChanged)
					{
						calculateCurrentDamageTooltip();
					}
					itemName = _currentDamageTooltip;
				}
				else
				{
					itemName = text;
				}
			}
			else
			{
				itemName = tr("STR_ALIEN_ARTIFACT");
			}
		}

		if (!altPressed)
		{
			std::ostringstream ss;
			ss << itemName;
			ss << " [";
			ss << item->getTotalWeight();
			ss << "]";
			_txtItem->setText(ss.str().c_str());
		}
		else
		{
			_txtItem->setText(itemName);
		}

		_selAmmo->clear();
		bool hasSelfAmmo = item->getRules()->getBattleType() != BT_AMMO && item->getRules()->getClipSize() > 0;
		if ((item->isWeaponWithAmmo() || hasSelfAmmo) && item->haveAnyAmmo())
		{
			updateTemplateButtons(false);
			_txtAmmo->setText("");
		}
		else
		{
			_mouseHoverItem = nullptr;
			updateTemplateButtons(!_tu);
			std::string s;
			if (item->getAmmoQuantity() != 0 && item->getRules()->getBattleType() == BT_AMMO)
			{
				s = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
			}
			else if (item->getRules()->getBattleType() == BT_MEDIKIT)
			{
				s = tr("STR_MEDI_KIT_QUANTITIES_LEFT").arg(item->getPainKillerQuantity()).arg(item->getStimulantQuantity()).arg(item->getHealQuantity());
			}
			_txtAmmo->setText(s);
		}
	}
	else
	{
		if (_currentTooltip.empty())
		{
			_txtItem->setText("");
		}
		_txtAmmo->setText("");
		_selAmmo->clear();
		updateTemplateButtons(!_tu);
	}
}

/**
 * Hides item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOut(Action *)
{
	_txtItem->setText("");
	_txtAmmo->setText("");
	_selAmmo->clear();
	_inv->setMouseOverItem(0);
	_mouseHoverItem = nullptr;
	_currentDamageTooltipItem = nullptr;
	_currentDamageTooltip = "";
	updateTemplateButtons(!_tu);
}

void InventoryState::onMoveGroundInventoryToBase(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	if (_base == 0)
	{
		// equipment just before the mission (=after briefing) or during the mission
		return;
	}

	if (_noCraft)
	{
		// pre-equipping in the base, but *without* a craft
		return;
	}

	// ok, which craft?
	BattleUnit *unit = _battleGame->getSelectedUnit();
	Soldier *s = unit->getGeoscapeSoldier();
	Craft *c = s->getCraft();

	if (c == 0 || c->getStatus() == "STR_OUT")
	{
		// we're either not in a craft or not in a hangar (should not happen, but just in case)
		return;
	}

	Tile                     *groundTile = unit->getTile();
	std::vector<BattleItem*> *groundInv = groundTile->getInventory();

	// step 1: move stuff from craft to base
	for (auto* bi : *groundInv)
	{
		const auto& weaponType = bi->getRules()->getType();
		// check all ammo slots first
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			if (bi->getAmmoForSlot(slot))
			{
				const auto& ammoType = bi->getAmmoForSlot(slot)->getRules()->getType();
				// only real ammo
				if (weaponType != ammoType)
				{
					c->getItems()->removeItem(ammoType);
					_base->getStorageItems()->addItem(ammoType);
				}
			}
		}
		// and the weapon as last
		c->getItems()->removeItem(weaponType);
		_base->getStorageItems()->addItem(weaponType);
	}

	// step 2: clear ground
	for (auto itemIt = groundInv->begin(); itemIt != groundInv->end(); )
	{
		BattleItem* item = (*itemIt);
		item->setOwner(NULL);
		itemIt = groundInv->erase(itemIt);
		_game->getSavedGame()->getSavedBattle()->removeItem(item);
	}

	// refresh ui
	_inv->arrangeGround();
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
void InventoryState::handle(Action *action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN && !_btnQuickSearch->isFocused() && !_txtName->isFocused())
	{
		// "ctrl+1..9" - save equipment
		// "1..9" - load equipment
		if (action->getDetails()->key.keysym.sym >= SDLK_0 && action->getDetails()->key.keysym.sym <= SDLK_9)
		{
			{
				btnGlobalEquipmentLayoutClick(action);
			}
		}
		else
		{
			_prev_key = 0, _key_repeats = 0;
		}

		if (action->getDetails()->key.keysym.sym == Options::keyInvClear)
		{
			if (_game->isCtrlPressed() && _game->isAltPressed())
			{
				onMoveGroundInventoryToBase(action);
			}
		}
	}

#ifndef __MORPHOS__
	if (Options::oxceThumbButtons && action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_X1)
		{
			btnNextClick(action);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_X2)
		{
			btnPrevClick(action);
		}
	}
#endif
}

/**
 * Cycle through loaded ammo in hover over item.
 */
void InventoryState::think()
{
	if (_mouseHoverItem)
	{
		int anim = _inv->getAnimFrame();
		int seq = std::max(((anim - _mouseHoverItemFrame) / 10) - 1, 0); // `-1` cause that first item will be show bit more longer
		int modulo = 0;
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			bool showSelfAmmo = slot == 0 && _mouseHoverItem->getRules()->getClipSize() > 0;
			if ((_mouseHoverItem->needsAmmoForSlot(slot) || showSelfAmmo) && _mouseHoverItem->getAmmoForSlot(slot))
			{
				++modulo;
			}
		}
		if (modulo)
		{
			seq %= modulo;
		}

		BattleItem* firstAmmo = nullptr;
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			bool showSelfAmmo = slot == 0 && _mouseHoverItem->getRules()->getClipSize() > 0;
			if ((_mouseHoverItem->needsAmmoForSlot(slot) || showSelfAmmo) && _mouseHoverItem->getAmmoForSlot(slot))
			{
				firstAmmo = _mouseHoverItem->getAmmoForSlot(slot);
				if (slot >= seq)
				{
					break;
				}
			}
			else
			{
				// this will skip empty slot
				++seq;
			}
		}
		if (firstAmmo)
		{
			_txtAmmo->setText(tr("STR_AMMO_ROUNDS_LEFT").arg(firstAmmo->getAmmoQuantity()));
			SDL_Rect r;
			r.x = 0;
			r.y = 0;
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_selAmmo->drawRect(&r, _game->getMod()->getInterface("inventory")->getElement("grid")->color);
			r.x++;
			r.y++;
			r.w -= 2;
			r.h -= 2;
			_selAmmo->drawRect(&r, Palette::blockOffset(0)+15);
			firstAmmo->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selAmmo, firstAmmo, _game->getSavedGame()->getSavedBattle(), anim);
		}
		else
		{
			_selAmmo->clear();
		}
	}
	State::think();
}

/**
 * Shows a tooltip for the appropriate button.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipIn(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtItem->setText(tr(_currentTooltip));
	}
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipOut(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_txtItem->setText("");
		}
	}
}

/**
* Shows a tooltip for the paperdoll's armor.
* @param action Pointer to an action.
*/
void InventoryState::txtArmorTooltipIn(Action *action)
{
	if (_inv->getSelectedItem() == 0)
	{
		BattleUnit *unit = _inv->getSelectedUnit();
		if (unit != 0)
		{
			action->getSender()->setTooltip(unit->getArmor()->getType());
			_currentTooltip = action->getSender()->getTooltip();
			{
				std::ostringstream ss;

				if (unit->getGeoscapeSoldier())
				{
					auto* soldierRules = unit->getGeoscapeSoldier()->getRules();
					if (soldierRules->getShowTypeInInventory())
					{
						ss << tr(soldierRules->getType());
						ss << ": ";
					}
				}

				ss << tr(_currentTooltip);
				if (unit->getArmor()->getWeight() != 0)
				{
					ss << " [";
					ss << unit->getArmor()->getWeight();
					ss << "]";
				}
				_txtItem->setText(ss.str().c_str());
			}
		}
	}
}

/**
* Clears the armor tooltip text.
* @param action Pointer to an action.
*/
void InventoryState::txtArmorTooltipOut(Action *action)
{
	if (_inv->getSelectedItem() == 0)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_txtItem->setText("");
		}
	}
}

void InventoryState::updateTemplateButtons(bool isVisible)
{
	if (isVisible)
	{
		if (_curInventoryTemplate.empty())
		{
			// use "empty template" icons
			_game->getMod()->getSurface("InvCopy")->blitNShade(_btnCreateTemplate, 0, 0);
			_game->getMod()->getSurface("InvPasteEmpty")->blitNShade(_btnApplyTemplate, 0, 0);
			_btnApplyTemplate->setTooltip("STR_CLEAR_INVENTORY");
		}
		else
		{
			// use "active template" icons
			_game->getMod()->getSurface("InvCopyActive")->blitNShade(_btnCreateTemplate, 0, 0);
			_game->getMod()->getSurface("InvPaste")->blitNShade(_btnApplyTemplate, 0, 0);
			_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
		}
		_btnCreateTemplate->initSurfaces();
		_btnApplyTemplate->initSurfaces();
	}
	else
	{
		_btnCreateTemplate->clear();
		_btnApplyTemplate->clear();
	}
}

}
