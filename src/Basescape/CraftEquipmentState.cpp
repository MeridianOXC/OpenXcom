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
#include "CraftEquipmentState.h"
#include "CraftEquipmentLoadState.h"
#include "CraftEquipmentSaveState.h"
#include <climits>
#include <sstream>
#include <algorithm>
#include <locale>
#include "../Engine/CrossPlatform.h"
#include "../Engine/Screen.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Timer.h"
#include "../Engine/Collections.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Mod/Armor.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleItemCategory.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/Vehicle.h"
#include "../Savegame/SavedGame.h"
#include "../Menu/ErrorMessageState.h"
#include "../Battlescape/CannotReequipState.h"
#include "../Battlescape/DebriefingState.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/RuleInterface.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Equipment screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftEquipmentState::CraftEquipmentState(Base *base, size_t craft) :
	_lstScroll(0), _sel(0), _craft(craft), _base(base), _totalItems(0), _totalItemStorageSize(0.0), _ammoColor(0),
	_reload(true), _returningFromGlobalTemplates(false), _returningFromInventory(false), _firstInit(true), _isNewBattle(false)
{
	Craft *c = _base->getCrafts()->at(_craft);
	bool craftHasACrew = c->getNumTotalSoldiers() > 0;
	_isNewBattle = _game->getSavedGame()->getMonthsPassed() == -1;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 264, 12);
	_btnOk = new TextButton((craftHasACrew || _isNewBattle)?30:140, 16, (craftHasACrew || _isNewBattle)?274:164, 176);
	_btnClear = new TextButton(102, 16, 164, 176);
	_btnInventory = new TextButton(102, 16, 164, 176);
	_txtTitle = new Text(300, 17, 16, 7);
	_txtItem = new Text(144, 9, 16, 32);
	_txtStores = new Text(150, 9, 160, 32);
	_txtAvailable = new Text(110, 9, 16, 24);
	_txtUsed = new Text(110, 9, 130, 24);
	_txtCrew = new Text(71, 9, 244, 24);
	_lstEquipment = new TextList(288, 128, 8, 40);
	_cbxFilterBy = new ComboBox(this, 140, 16, 16, 176, true);

	// Set palette
	setInterface("craftEquipment");

	_ammoColor = _game->getMod()->getInterface("craftEquipment")->getElement("ammoColor")->color;

	add(_window, "window", "craftEquipment");
	add(_btnQuickSearch, "button", "craftEquipment");
	add(_btnOk, "button", "craftEquipment");
	add(_btnClear, "button", "craftEquipment");
	add(_btnInventory, "button", "craftEquipment");
	add(_txtTitle, "text", "craftEquipment");
	add(_txtItem, "text", "craftEquipment");
	add(_txtStores, "text", "craftEquipment");
	add(_txtAvailable, "text", "craftEquipment");
	add(_txtUsed, "text", "craftEquipment");
	add(_txtCrew, "text", "craftEquipment");
	add(_lstEquipment, "list", "craftEquipment");
	add(_cbxFilterBy, "button", "craftEquipment");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftEquipment");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftEquipmentState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnClearClick, Options::keyRemoveEquipmentFromCraft);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnLoadClick, Options::keyCraftLoadoutLoad);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnSaveClick, Options::keyCraftLoadoutSave);

	_btnClear->setText(tr("STR_UNLOAD_CRAFT"));
	_btnClear->onMouseClick((ActionHandler)&CraftEquipmentState::btnClearClick);
	_btnClear->setVisible(_isNewBattle);

	_btnInventory->setText(tr("STR_INVENTORY"));
	_btnInventory->onMouseClick((ActionHandler)&CraftEquipmentState::btnInventoryClick);
	_btnInventory->setVisible(craftHasACrew && !_isNewBattle);
	_btnInventory->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnInventoryClick, Options::keyBattleInventory);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtStores->setText(tr("STR_STORES"));

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));

	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));

	std::ostringstream ss3;
	ss3 << tr("STR_SOLDIERS_UC") << ">" << Unicode::TOK_COLOR_FLIP << c->getNumTotalSoldiers();
	_txtCrew->setText(ss3.str());

	// populate sort options
	_categoryStrings.push_back("STR_ALL");
	_categoryStrings.push_back("STR_EQUIPPED");
	bool hasUnassigned = false;
	for (auto& itemType : _game->getMod()->getItemsList())
	{
		RuleItem *rule = _game->getMod()->getItem(itemType);
		Unit* isVehicle = rule->getVehicleUnit();
		int cQty = isVehicle ? c->getVehicleCount(itemType) : c->getItems()->getItem(rule);

		if ((isVehicle || rule->isInventoryItem()) && rule->canBeEquippedToCraftInventory() &&
			_game->getSavedGame()->isResearched(rule->getRequirements()) &&
			(_base->getStorageItems()->getItem(rule) > 0 || cQty > 0))
		{
			if (rule->getCategories().empty())
			{
				hasUnassigned = true;
			}
			else
			{
				for (auto& itemCategoryName : rule->getCategories())
				{
					_usedCategoryStrings[itemCategoryName] = true;
				}
			}
		}
	}
	auto& itemCategories = _game->getMod()->getItemCategoriesList();
	for (auto& categoryName : itemCategories)
	{
		if (_usedCategoryStrings[categoryName])
		{
			if (!_game->getMod()->getItemCategory(categoryName)->isHidden())
			{
				_categoryStrings.push_back(categoryName);
			}
		}
	}
	if (hasUnassigned && !itemCategories.empty())
	{
		_categoryStrings.push_back("STR_UNASSIGNED");
	}
	_categoryStrings.push_back("STR_NOT_EQUIPPED");

	_cbxFilterBy->setOptions(_categoryStrings, true);
	_cbxFilterBy->setSelected(0);
	_cbxFilterBy->onChange((ActionHandler)&CraftEquipmentState::cbxFilterByChange);

	_lstEquipment->setArrowColumn(203, ARROW_HORIZONTAL);
	_lstEquipment->setColumns(3, 156, 83, 41);
	_lstEquipment->setSelectable(true);
	_lstEquipment->setBackground(_window);
	_lstEquipment->setMargin(8);
	_lstEquipment->onLeftArrowPress((ActionHandler)&CraftEquipmentState::lstEquipmentLeftArrowPress);
	_lstEquipment->onLeftArrowRelease((ActionHandler)&CraftEquipmentState::lstEquipmentLeftArrowRelease);
	_lstEquipment->onLeftArrowClick((ActionHandler)&CraftEquipmentState::lstEquipmentLeftArrowClick);
	_lstEquipment->onRightArrowPress((ActionHandler)&CraftEquipmentState::lstEquipmentRightArrowPress);
	_lstEquipment->onRightArrowRelease((ActionHandler)&CraftEquipmentState::lstEquipmentRightArrowRelease);
	_lstEquipment->onRightArrowClick((ActionHandler)&CraftEquipmentState::lstEquipmentRightArrowClick);
	_lstEquipment->onMousePress((ActionHandler)&CraftEquipmentState::lstEquipmentMousePress);

	_lstEquipment->onMouseWheel((ActionHandler)&CraftEquipmentState::lstEquipmentMouseWheel);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&CraftEquipmentState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&CraftEquipmentState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	_timerLeft = new Timer(250);
	_timerLeft->onTimer((StateHandler)&CraftEquipmentState::moveLeft);
	_timerRight = new Timer(250);
	_timerRight->onTimer((StateHandler)&CraftEquipmentState::moveRight);
}

/**
 *
 */
CraftEquipmentState::~CraftEquipmentState()
{
	delete _timerLeft;
	delete _timerRight;
}

/**
 * Filters the equipment list by the selected criterion
 * @param action Pointer to an action.
 */
void CraftEquipmentState::cbxFilterByChange(Action *action)
{
	initList();
}

/**
* Resets the savegame when coming back from the inventory.
*/
void CraftEquipmentState::init()
{
	State::init();

	_game->getSavedGame()->setBattleGame(0);

	Craft *c = _base->getCrafts()->at(_craft);
	c->setInBattlescape(false);

	// don't reload after closing error popups
	if (_reload)
	{
		if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
		{
			// skip when returning from craft equipment template load/save
			if (!_returningFromGlobalTemplates)
			{
				c->calculateTotalSoldierEquipment();
			}
			if (_returningFromInventory)
			{
				// now that we're back from the inventory screen, we need to remove all the excess base gear
				for (_sel = 0; _sel != _items.size(); ++_sel)
				{
					int excessQty = c->getItems()->getItem(_items[_sel]) - (c->getExtraItems()->getItem(_items[_sel]) + c->getSoldierItems()->getItem(_items[_sel]));
					moveLeftByValue(excessQty);
				}
			}
		}
		initList();
	}
	_reload = true;
	_returningFromGlobalTemplates = false;
	_returningFromInventory = false;
	_firstInit = false;
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnQuickSearchToggle(Action *action)
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
void CraftEquipmentState::btnQuickSearchApply(Action *)
{
	initList();
}

/**
 * Shows the equipment in a list filtered by selected criterion.
 */
void CraftEquipmentState::initList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	size_t selIdx = _cbxFilterBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}
	const std::string selectedCategory = _categoryStrings[selIdx];
	bool categoryFilterEnabled = (selectedCategory != "STR_ALL");
	bool categoryUnassigned = (selectedCategory == "STR_UNASSIGNED");
	bool categoryEquipped = (selectedCategory == "STR_EQUIPPED");
	bool categoryNotEquipped = (selectedCategory == "STR_NOT_EQUIPPED");
	bool shareAmmoCategories = _game->getMod()->getShareAmmoCategories();

	Craft *c = _base->getCrafts()->at(_craft);

	// reset
	_totalItems = 0;
	_totalItemStorageSize = 0.0;
	_items.clear();
	_lstEquipment->clearList();

	int row = 0;
	for (auto& itemType : _game->getMod()->getItemsList())
	{
		const RuleItem *rule = _game->getMod()->getItem(itemType);

		Unit* isVehicle = rule->getVehicleUnit();
		int cQty = 0;
		if (isVehicle)
		{
			cQty = c->getVehicleCount(itemType);
		}
		else
		{
			cQty = c->getItems()->getItem(rule);
			_totalItems += cQty;
			_totalItemStorageSize += cQty * rule->getSize();
		}

		int bQty = _base->getStorageItems()->getItem(rule);
		int reserved = 0;
		if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
		{
			reserved = c->getSoldierItems()->getItem(rule);
		}
		if ((isVehicle || rule->isInventoryItem()) && rule->canBeEquippedToCraftInventory() &&
			(bQty > 0 || cQty > 0 || reserved > 0))
		{
			// check research requirements
			if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			{
				continue;
			}

			// filter by category
			if (categoryFilterEnabled)
			{
				if (categoryUnassigned)
				{
					if (!rule->getCategories().empty())
					{
						continue;
					}
				}
				else if (categoryEquipped)
				{
					if (!(cQty > 0))
					{
						continue;
					}
				}
				else if (categoryNotEquipped)
				{
					if (cQty > 0)
					{
						continue;
					}
				}
				else
				{
					bool isOK = rule->belongsToCategory(selectedCategory);
					if (shareAmmoCategories && !isOK && rule->getBattleType() == BT_FIREARM)
					{
						for (auto* ammoRule : *rule->getPrimaryCompatibleAmmo())
						{
							if (_base->getStorageItems()->getItem(ammoRule) > 0 || c->getItems()->getItem(ammoRule) > 0)
							{
								if (ammoRule->isInventoryItem() && ammoRule->canBeEquippedToCraftInventory() && _game->getSavedGame()->isResearched(ammoRule->getRequirements()))
								{
									isOK = ammoRule->belongsToCategory(selectedCategory);
									if (isOK) break;
								}
							}
						}
					}
					if (!isOK) continue;
				}
			}

			// quick search
			if (!searchString.empty())
			{
				std::string projectName = tr(itemType);
				Unicode::upperCase(projectName);
				if (projectName.find(searchString) == std::string::npos)
				{
					continue;
				}
			}

			_items.push_back(itemType);
			std::ostringstream ss, ss2;
			if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
			{
				// doing this once (on opening the screen) is enough
				// and just to make sure, we must skip this when returning from craft equipment template load/save
				if (_firstInit && !isVehicle && cQty < reserved && !_returningFromGlobalTemplates)
				{
					// try to automatically add more items (if possible)
					int itemsToAdd = std::min(bQty, reserved - cQty);
					if (itemsToAdd > 0)
					{
						_base->getStorageItems()->removeItem(rule, itemsToAdd);
						bQty -= itemsToAdd;
						c->getItems()->addItem(rule, itemsToAdd);
						cQty += itemsToAdd;
						_totalItems += itemsToAdd;
						_totalItemStorageSize += itemsToAdd * rule->getSize();
					}
				}
				if (isVehicle)
					ss2 << cQty;
				else if (cQty - reserved > 0)
					ss2 << reserved << "/+" << cQty - reserved;
				else if (cQty - reserved == 0)
					ss2 << cQty;
				else
					ss2 << cQty << "/" << cQty - reserved;
			}
			else
			{
				ss2 << cQty;
			}
			if (!_isNewBattle)
			{
				ss << bQty;
			}
			else
			{
				ss << "-";
			}

			std::string s = tr(itemType);
			if (rule->getBattleType() == BT_AMMO)
			{
				s.insert(0, "  ");
			}
			_lstEquipment->addRow(3, s.c_str(), ss.str().c_str(), ss2.str().c_str());

			Uint8 color;
			if (cQty == 0)
			{
				if (rule->getBattleType() == BT_AMMO)
				{
					color = _ammoColor;
				}
				else
				{
					color = _lstEquipment->getColor();
				}
			}
			else
			{
					color = _lstEquipment->getSecondaryColor();
			}
			_lstEquipment->setRowColor(row, color);

			++row;
		}
	}

	_lstEquipment->draw();
	if (_lstScroll > 0)
	{
		_lstEquipment->scrollTo(_lstScroll);
		_lstScroll = 0;
	}
}

/**
 * Runs the arrow timers.
 */
void CraftEquipmentState::think()
{
	State::think();

	_timerLeft->think(this, 0);
	_timerRight->think(this, 0);
}


/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Starts moving the item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowPress(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerLeft->isRunning()) _timerLeft->start();
}

/**
 * Stops moving the item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLeft->stop();
	}
}

/**
 * Moves all the items to the base on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) moveLeftByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveLeftByValue(1);
		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
}

/**
 * Starts moving the item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowPress(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerRight->isRunning()) _timerRight->start();
}

/**
 * Stops moving the item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerRight->stop();
	}
}

/**
 * Moves all the items (as much as possible) to the craft on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) moveRightByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveRightByValue(1);
		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
}

/**
 * Handles middle-click
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentMousePress(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		_lstScroll = _lstEquipment->getScroll();
		RuleItem *rule = _game->getMod()->getItem(_items[_sel]);
		std::string articleId = rule->getUfopediaType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentMouseWheel(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	const SDL_Event &ev(*action->getDetails());
	if (ev.type == SDL_MOUSEWHEEL)
	{
		_timerRight->stop();
		_timerLeft->stop();
		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			if (ev.wheel.y > 0)
				moveRightByValue(Options::changeValueByMouseWheel);
			else
				moveLeftByValue(Options::changeValueByMouseWheel);
		}
	}
}

/**
 * Updates the displayed quantities of the
 * selected item on the list.
 */
void CraftEquipmentState::updateQuantity()
{
	Craft *c = _base->getCrafts()->at(_craft);
	RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int cQty = 0;
	if (item->getVehicleUnit())
	{
		cQty = c->getVehicleCount(_items[_sel]);
	}
	else
	{
		cQty = c->getItems()->getItem(item);
	}
	std::ostringstream ss, ss2;
	if (!_isNewBattle)
	{
		ss << _base->getStorageItems()->getItem(item);
	}
	else
	{
		ss << "-";
	}
	if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
	{
		int reserved = c->getSoldierItems()->getItem(item);
		if (item->getVehicleUnit())
			ss2 << cQty;
		else if (cQty - reserved > 0)
			ss2 << reserved << "/+" << cQty - reserved;
		else if (cQty - reserved == 0)
			ss2 << cQty;
		else
			ss2 << cQty << "/" << cQty - reserved;
	}
	else
	{
		ss2 << cQty;
	}

	Uint8 color;
	if (cQty == 0)
	{
		if (item->getBattleType() == BT_AMMO)
		{
			color = _ammoColor;
		}
		else
		{
			color = _lstEquipment->getColor();
		}
	}
	else
	{
		color = _lstEquipment->getSecondaryColor();
	}
	_lstEquipment->setRowColor(_sel, color);
	_lstEquipment->setCellText(_sel, 1, ss.str());
	_lstEquipment->setCellText(_sel, 2, ss2.str());

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

/**
 * Moves the selected item to the base.
 */
void CraftEquipmentState::moveLeft()
{
	_timerLeft->setInterval(50);
	_timerRight->setInterval(50);
	moveLeftByValue(1);
}

/**
 * Moves the given number of items (selected) to the base.
 * @param change Item difference.
 */
void CraftEquipmentState::moveLeftByValue(int change)
{
	Craft *c = _base->getCrafts()->at(_craft);
	const RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int cQty = 0;
	if (item->getVehicleUnit()) cQty = c->getVehicleCount(_items[_sel]);
	else cQty = c->getItems()->getItem(item);
	if (change <= 0 || cQty <= 0) return;
	if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
	{
		int reserved = c->getSoldierItems()->getItem(item);
		if (cQty - reserved > 0)
		{
			change = std::min(cQty - reserved, change);
		}
		else
		{
			//change = 0;
			return;
		}
	}
	else
	{
		change = std::min(cQty, change);
	}
	// Convert vehicle to item
	if (item->getVehicleUnit())
	{
		if (item->getVehicleClipAmmo())
		{
			// Calculate how much ammo needs to be added to the base.
			const RuleItem *ammo = item->getVehicleClipAmmo();
			int ammoPerVehicle = item->getVehicleClipsLoaded();

			// Put the vehicles and their ammo back as separate items.
			if (!_isNewBattle)
			{
				_base->getStorageItems()->addItem(item, change);
				_base->getStorageItems()->addItem(ammo, ammoPerVehicle * change);
			}
			// now delete the vehicles from the craft.
			Collections::deleteIf(*c->getVehicles(), change,
				[&](Vehicle* v)
				{
					return v->getRules() == item;
				}
			);
		}
		else
		{
			if (!_isNewBattle)
			{
				_base->getStorageItems()->addItem(item, change);
			}
			Collections::deleteIf(*c->getVehicles(), change,
				[&](Vehicle* v)
				{
					return v->getRules() == item;
				}
			);
		}
	}
	else
	{
		c->getItems()->removeItem(item, change);
		_totalItems -= change;
		_totalItemStorageSize -= change * item->getSize();
		if (!_isNewBattle)
		{
			_base->getStorageItems()->addItem(item, change);
		}
	}
	updateQuantity();
}

/**
 * Moves the selected item to the craft.
 */
void CraftEquipmentState::moveRight()
{
	_timerLeft->setInterval(50);
	_timerRight->setInterval(50);
	moveRightByValue(1);
}

/**
 * Moves the given number of items (selected) to the craft.
 * @param change Item difference.
 * @param suppressErrors Suppress error messages?
 */
void CraftEquipmentState::moveRightByValue(int change, bool suppressErrors)
{
	Craft *c = _base->getCrafts()->at(_craft);
	const RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int bqty = _base->getStorageItems()->getItem(item);
	if (_isNewBattle)
	{
		if (change == INT_MAX)
		{
			change = 10;
		}
		bqty = change;
	}
	if (0 >= change || 0 >= bqty) return;
	change = std::min(bqty, change);
	// Do we need to convert item to vehicle?
	if (item->getVehicleUnit())
	{
		int size = item->getVehicleUnit()->getArmor()->getTotalSize();
		// Check if there's enough room
		int room = c->validateAddingVehicles(size);
		if (room > 0)
		{
			change = std::min(room, change);
			if (item->getVehicleClipAmmo())
			{
				// And now let's see if we can add the total number of vehicles.
				const RuleItem *ammo = item->getVehicleClipAmmo();
				int ammoPerVehicle = item->getVehicleClipsLoaded();

				int baseQty = _base->getStorageItems()->getItem(ammo) / ammoPerVehicle;
				if (_isNewBattle)
					baseQty = change;
				int canBeAdded = std::min(change, baseQty);
				if (canBeAdded > 0)
				{
					for (int i = 0; i < canBeAdded; ++i)
					{
						if (!_isNewBattle)
						{
							_base->getStorageItems()->removeItem(ammo, ammoPerVehicle);
							_base->getStorageItems()->removeItem(item);
						}
						c->getVehicles()->push_back(new Vehicle(item, item->getVehicleClipSize(), size));
						c->resetCustomDeployment(); // adding a vehicle into a craft invalidates a custom craft deployment
					}
				}
				else
				{
					if (!suppressErrors)
					{
						// So we haven't managed to increase the count of vehicles because of the ammo
						_timerRight->stop();
						LocalizedText msg(tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP").arg(ammoPerVehicle).arg(tr(ammo->getType())));
						_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
						_reload = false;
					}
				}
			}
			else
				for (int i = 0; i < change; ++i)
				{
					c->getVehicles()->push_back(new Vehicle(item, item->getVehicleClipSize(), size));
					c->resetCustomDeployment(); // adding a vehicle into a craft invalidates a custom craft deployment
					if (!_isNewBattle)
					{
						_base->getStorageItems()->removeItem(item);
					}
				}
		}
	}
	else
	{
		if (_totalItems + change > c->getMaxItemsClamped())
		{
			if (!suppressErrors)
			{
				_timerRight->stop();
				LocalizedText msg(tr("STR_NO_MORE_EQUIPMENT_ALLOWED", c->getMaxItemsClamped()));
				_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
				_reload = false;
			}
			change = c->getMaxItemsClamped() - _totalItems;
			if (change < 0)
			{
				// if the player is already over the maximum (e.g. after a mod update), don't go into some ridiculous minus values
				change = 0;
			}
		}
		if (_totalItemStorageSize + (change * item->getSize()) > c->getMaxStorageSpaceClamped() + 0.05)
		{
			if (item->getSize() > 0.0)
			{
				change = (int)floor((c->getMaxStorageSpaceClamped() + 0.05 - _totalItemStorageSize) / item->getSize());
			}
			if (change < 0)
			{
				// if the player is already over the maximum (e.g. after a mod update), don't go into some ridiculous minus values
				change = 0;
			}
			if (!suppressErrors)
			{
				_timerRight->stop();
				LocalizedText msg(tr("STR_NO_MORE_EQUIPMENT_ALLOWED_BY_SIZE").arg(c->getMaxStorageSpaceClamped()));
				_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
				_reload = false;
			}
		}
		c->getItems()->addItem(item, change);
		_totalItems += change;
		_totalItemStorageSize += change * item->getSize();
		if (!_isNewBattle)
		{
			_base->getStorageItems()->removeItem(item, change);
		}
	}
	updateQuantity();
}

/**
 * Empties the contents of the craft, moving all of the items back to the base.
 */
void CraftEquipmentState::btnClearClick(Action *)
{
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		moveLeftByValue(INT_MAX);
	}

	// in New Battle, clear also stuff that is not displayed on the GUI (for whatever reason)
	if (_isNewBattle)
	{
		Craft* c = _base->getCrafts()->at(_craft);
		c->getItems()->clear();
	}
}

/**
 * Displays the inventory screen for the soldiers
 * inside the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnInventoryClick(Action *)
{
	Craft *craft = _base->getCrafts()->at(_craft);
	if (craft->getNumTotalSoldiers() > 0)
	{
		if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
		{
			// This is a bit tricky... here's what we're doing:
			// * Remember the extra craft items (i.e. items that are on the craft, but not equipped by soldiers)
			// * Move all equipment from the base into the craft.
			// * Run the inventory screen.
			// * Remove excess items from the craft when CraftEquipmentState::init() is called after leaving the inventory screen.
			// (After this, the craft should have all the updated soldier equipment, and the same extra items as before.)

			// Note: the current implementation assumes no limit to the number or size of items a craft can hold.
			//       If the craft has limited space, then we just won't have all the base items available on the inventory screen.

			auto& extras = *craft->getExtraItems();
			extras.clear();
			for (_sel = 0; _sel != _items.size(); ++_sel)
			{
				RuleItem* rule = _game->getMod()->getItem(_items[_sel], true);
				if (craft->getItems()->getItem(rule) > 0)
				{
					extras.addItem(rule, craft->getItems()->getItem(rule) - craft->getSoldierItems()->getItem(rule));
				}
				if (!rule->getVehicleUnit() && rule->canBeEquippedBeforeBaseDefense())
				{
					moveRightByValue(INT_MAX, true);
				}
			}
		}

		SavedBattleGame *bgame = new SavedBattleGame(_game->getMod(), _game->getLanguage());
		_game->getSavedGame()->setBattleGame(bgame);

		if (_game->isCtrlPressed() && _game->isAltPressed())
		{
			_game->getSavedGame()->setDisableSoldierEquipment(true);
		}
		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.runInventory(craft);

		_game->getScreen()->clear();
		_game->pushState(new InventoryState(false, 0, _base));
		_returningFromInventory = true;
	}
}

void CraftEquipmentState::saveGlobalLoadout(int index)
{
	// clear the template
	ItemContainer *tmpl = _game->getSavedGame()->getGlobalCraftLoadout(index);
	tmpl->clear();

	Craft *c = _base->getCrafts()->at(_craft);
	// save only what is visible on the screen (can be DIFFERENT than what's really in the craft for various reasons)
	for (const auto& itemType : _items)
	{
		const RuleItem *item = _game->getMod()->getItem(itemType, true);
		int cQty = 0;
		if (item->getVehicleUnit())
		{
			cQty = c->getVehicleCount(itemType);
		}
		else
		{
			cQty = c->getItems()->getItem(item);
		}
		if (cQty > 0)
		{
			tmpl->addItem(item, cQty);
		}
	}
}

void CraftEquipmentState::loadGlobalLoadout(int index, bool onlyAddItems)
{
	// temporarily turn off alternate craft equipment management to allow removing all items from the craft
	bool backup = Options::oxceAlternateCraftEquipmentManagement;
	Options::oxceAlternateCraftEquipmentManagement = false;

	// reset filters and reload the full equipment list
	_btnQuickSearch->setText("");
	_cbxFilterBy->setSelected(0);
	initList();

	Craft* c = _base->getCrafts()->at(_craft);

	ItemContainer craftItemsBackup;
	std::vector<Vehicle*> craftVehiclesBackup;
	if (onlyAddItems)
	{
		// remember for later, make copies
		craftItemsBackup = *c->getItems();
		craftVehiclesBackup = *c->getVehicles();
	}
	else
	{
		// first move everything visible back to base
		for (_sel = 0; _sel != _items.size(); ++_sel)
		{
			moveLeftByValue(INT_MAX);
		}
	}

	// now start applying the template (consider ONLY items visible on the GUI)
	ItemContainer *tmpl = _game->getSavedGame()->getGlobalCraftLoadout(index);
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
		int tQty = tmpl->getItem(item);
		moveRightByValue(tQty, true);
	}

	// lastly check and report what's missing
	std::string craftName = c->getName(_game->getLanguage());
	std::vector<ReequipStat> _missingItems;
	for (const auto& templateItem : *tmpl->getContents())
	{
		const RuleItem *item = templateItem.first;
		if (item)
		{
			int tQty = templateItem.second;
			int cQty = 0;
			if (item->getVehicleUnit())
			{
				// Note: we will also report HWPs as missing:
				// - if there is not enough ammo to arm them
				// - if there is not enough cargo space in the craft
				cQty = c->getVehicleCount(item->getType());
				if (onlyAddItems)
				{
					int total = 0;
					for (const auto* vehicle : craftVehiclesBackup)
					{
						if (vehicle->getRules() == item)
						{
							total++;
						}
					}
					cQty -= total; // i.e. only count newly added vehicles
				}
			}
			else
			{
				cQty = c->getItems()->getItem(item);
				if (onlyAddItems)
				{
					cQty -= craftItemsBackup.getItem(item); // i.e. only count newly added items
				}
			}
			int missing = tQty - cQty;
			if (missing > 0)
			{
				ReequipStat stat = { item->getType(), missing, craftName, item->getListOrder() };
				_missingItems.push_back(stat);
			}
		}
	}

	if (!_missingItems.empty())
	{
		std::sort(_missingItems.begin(), _missingItems.end(), [](const ReequipStat &a, const ReequipStat &b)
			{
				return a.listOrder < b.listOrder;
			}
		);
		_game->pushState(new CannotReequipState(_missingItems, _base));
	}

	// turn back the original setting
	Options::oxceAlternateCraftEquipmentManagement = backup;
}

/**
* Opens the CraftEquipmentLoadState screen.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnLoadClick(Action *)
{
	if (!_isNewBattle)
	{
		_game->pushState(new CraftEquipmentLoadState(this));
		_returningFromGlobalTemplates = true;
	}
}

/**
* Opens the CraftEquipmentSaveState screen.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnSaveClick(Action *)
{
	if (!_isNewBattle)
	{
		_game->pushState(new CraftEquipmentSaveState(this));
		_returningFromGlobalTemplates = true;
	}
}

}
