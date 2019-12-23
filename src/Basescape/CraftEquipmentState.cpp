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
CraftEquipmentState::CraftEquipmentState(Base *base, size_t craft) : _lstScroll(0), _sel(0), _craft(craft), _base(base), _totalItems(0), _ammoColor(0), _reload(true)
{
	Craft *c = _base->getCrafts()->at(_craft);
	bool craftHasACrew = c->getNumSoldiers() > 0;
	bool isNewBattle = _game->getSavedGame()->getMonthsPassed() == -1;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 264, 12);
	_btnOk = new TextButton((craftHasACrew || isNewBattle)?30:140, 16, (craftHasACrew || isNewBattle)?274:164, 176);
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
	_btnClear->setVisible(isNewBattle);

	_btnInventory->setText(tr("STR_INVENTORY"));
	_btnInventory->onMouseClick((ActionHandler)&CraftEquipmentState::btnInventoryClick);
	_btnInventory->setVisible(craftHasACrew && !isNewBattle);
	_btnInventory->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnInventoryClick, Options::keyBattleInventory);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtStores->setText(tr("STR_STORES"));

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));

	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));

	std::ostringstream ss3;
	ss3 << tr("STR_SOLDIERS_UC") << ">" << Unicode::TOK_COLOR_FLIP << c->getNumSoldiers();
	_txtCrew->setText(ss3.str());

	// populate sort options
	_categoryStrings.push_back("STR_ALL");
	_categoryStrings.push_back("STR_EQUIPPED");
	bool hasUnassigned = false;
	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		RuleItem *rule = _game->getMod()->getItem(*i);
		auto isVehicle = rule->getVehicleUnit();
		int cQty = isVehicle ? c->getVehicleCount(*i) : c->getItems()->getItem(*i);

		if ((isVehicle || rule->isInventoryItem()) && rule->canBeEquippedToCraftInventory() &&
			_game->getSavedGame()->isResearched(rule->getRequirements()) &&
			(_base->getStorageItems()->getItem(*i) > 0 || cQty > 0))
		{
			if (rule->getCategories().empty())
			{
				hasUnassigned = true;
			}
			else
			{
				for (std::vector<std::string>::const_iterator j = rule->getCategories().begin(); j != rule->getCategories().end(); ++j)
				{
					_usedCategoryStrings[(*j)] = true;
				}
			}
		}
	}
	const std::vector<std::string> &itemCategories = _game->getMod()->getItemCategoriesList();
	for (std::vector<std::string>::const_iterator i = itemCategories.begin(); i != itemCategories.end(); ++i)
	{
		if (_usedCategoryStrings[(*i)])
		{
			if (!_game->getMod()->getItemCategory((*i))->isHidden())
			{
				_categoryStrings.push_back((*i));
			}
		}
	}
	if (hasUnassigned)
	{
		_categoryStrings.push_back("STR_UNASSIGNED");
	}

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
		initList();
	}
	_reload = true;
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
	bool shareAmmoCategories = _game->getMod()->getShareAmmoCategories();

	Craft *c = _base->getCrafts()->at(_craft);

	// reset
	_totalItems = 0;
	_items.clear();
	_lstEquipment->clearList();

	int row = 0;
	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		RuleItem *rule = _game->getMod()->getItem(*i);

		auto isVehicle = rule->getVehicleUnit();
		int cQty = 0;
		if (isVehicle)
		{
			cQty = c->getVehicleCount(*i);
		}
		else
		{
			cQty = c->getItems()->getItem(*i);
			_totalItems += cQty;
		}

		if ((isVehicle || rule->isInventoryItem()) && rule->canBeEquippedToCraftInventory() &&
			(_base->getStorageItems()->getItem(*i) > 0 || cQty > 0))
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
				else
				{
					bool isOK = rule->belongsToCategory(selectedCategory);
					if (shareAmmoCategories && !isOK && rule->getBattleType() == BT_FIREARM)
					{
						for (auto& compatibleAmmoName : *rule->getPrimaryCompatibleAmmo())
						{
							if (_base->getStorageItems()->getItem(compatibleAmmoName) > 0 || c->getItems()->getItem(compatibleAmmoName) > 0)
							{
								RuleItem *ammoRule = _game->getMod()->getItem(compatibleAmmoName);
								if (ammoRule && ammoRule->isInventoryItem() && ammoRule->canBeEquippedToCraftInventory() && _game->getSavedGame()->isResearched(ammoRule->getRequirements()))
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
				std::string projectName = tr((*i));
				Unicode::upperCase(projectName);
				if (projectName.find(searchString) == std::string::npos)
				{
					continue;
				}
			}

			_items.push_back(*i);
			std::ostringstream ss, ss2;
			if (_game->getSavedGame()->getMonthsPassed() > -1)
			{
				ss << _base->getStorageItems()->getItem(*i);
			}
			else
			{
				ss << "-";
			}
			ss2 << cQty;

			std::string s = tr(*i);
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
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentMousePress(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerRight->stop();
		_timerLeft->stop();
		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveRightByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerRight->stop();
		_timerLeft->stop();
		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveLeftByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		_lstScroll = _lstEquipment->getScroll();
		RuleItem *rule = _game->getMod()->getItem(_items[_sel]);
		std::string articleId = rule->getType();
		Ufopaedia::openArticle(_game, articleId);
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
		cQty = c->getItems()->getItem(_items[_sel]);
	}
	std::ostringstream ss, ss2;
	if (_game->getSavedGame()->getMonthsPassed() > -1)
	{
		ss << _base->getStorageItems()->getItem(_items[_sel]);
	}
	else
	{
		ss << "-";
	}
	ss2 << cQty;

	Uint8 color;
	if (cQty == 0)
	{
		RuleItem *rule = _game->getMod()->getItem(_items[_sel], true);
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
	RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int cQty = 0;
	if (item->getVehicleUnit()) cQty = c->getVehicleCount(_items[_sel]);
	else cQty = c->getItems()->getItem(_items[_sel]);
	if (change <= 0 || cQty <= 0) return;
	change = std::min(cQty, change);
	// Convert vehicle to item
	if (item->getVehicleUnit())
	{
		if (!item->getPrimaryCompatibleAmmo()->empty())
		{
			// Calculate how much ammo needs to be added to the base.
			RuleItem *ammo = _game->getMod()->getItem(item->getPrimaryCompatibleAmmo()->front(), true);
			int ammoPerVehicle;
			if (ammo->getClipSize() > 0 && item->getClipSize() > 0)
			{
				ammoPerVehicle = item->getClipSize() / ammo->getClipSize();
			}
			else
			{
				ammoPerVehicle = ammo->getClipSize();
			}
			// Put the vehicles and their ammo back as separate items.
			if (_game->getSavedGame()->getMonthsPassed() != -1)
			{
				_base->getStorageItems()->addItem(_items[_sel], change);
				_base->getStorageItems()->addItem(ammo->getType(), ammoPerVehicle * change);
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
			if (_game->getSavedGame()->getMonthsPassed() != -1)
			{
				_base->getStorageItems()->addItem(_items[_sel], change);
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
		c->getItems()->removeItem(_items[_sel], change);
		_totalItems -= change;
		if (_game->getSavedGame()->getMonthsPassed() > -1)
		{
			_base->getStorageItems()->addItem(_items[_sel], change);
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
	RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int bqty = _base->getStorageItems()->getItem(_items[_sel]);
	if (_game->getSavedGame()->getMonthsPassed() == -1)
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
		int room = std::min(c->getRules()->getVehicles() - c->getNumVehicles(), c->getSpaceAvailable() / size);
		if (room > 0)
		{
			change = std::min(room, change);
			if (!item->getPrimaryCompatibleAmmo()->empty())
			{
				// And now let's see if we can add the total number of vehicles.
				RuleItem *ammo = _game->getMod()->getItem(item->getPrimaryCompatibleAmmo()->front(), true);
				int ammoPerVehicle, clipSize;
				if (ammo->getClipSize() > 0 && item->getClipSize() > 0)
				{
					clipSize = item->getClipSize();
					ammoPerVehicle = clipSize / ammo->getClipSize();
				}
				else
				{
					clipSize = ammo->getClipSize();
					ammoPerVehicle = clipSize;
				}

				int baseQty = _base->getStorageItems()->getItem(ammo->getType()) / ammoPerVehicle;
				if (_game->getSavedGame()->getMonthsPassed() == -1)
					baseQty = change;
				int canBeAdded = std::min(change, baseQty);
				if (canBeAdded > 0)
				{
					for (int i = 0; i < canBeAdded; ++i)
					{
						if (_game->getSavedGame()->getMonthsPassed() != -1)
						{
							_base->getStorageItems()->removeItem(ammo->getType(), ammoPerVehicle);
							_base->getStorageItems()->removeItem(_items[_sel]);
						}
						c->getVehicles()->push_back(new Vehicle(item, clipSize, size));
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
					c->getVehicles()->push_back(new Vehicle(item, item->getClipSize(), size));
					if (_game->getSavedGame()->getMonthsPassed() != -1)
					{
						_base->getStorageItems()->removeItem(_items[_sel]);
					}
				}
		}
	}
	else
	{
		if (c->getRules()->getMaxItems() > 0 && _totalItems + change > c->getRules()->getMaxItems())
		{
			if (!suppressErrors)
			{
				_timerRight->stop();
				LocalizedText msg(tr("STR_NO_MORE_EQUIPMENT_ALLOWED", c->getRules()->getMaxItems()));
				_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
				_reload = false;
			}
			change = c->getRules()->getMaxItems() - _totalItems;
		}
		c->getItems()->addItem(_items[_sel],change);
		_totalItems += change;
		if (_game->getSavedGame()->getMonthsPassed() > -1)
		{
			_base->getStorageItems()->removeItem(_items[_sel],change);
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
}

/**
 * Displays the inventory screen for the soldiers
 * inside the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnInventoryClick(Action *)
{
	Craft *craft = _base->getCrafts()->at(_craft);
	if (craft->getNumSoldiers() != 0)
	{
		SavedBattleGame *bgame = new SavedBattleGame(_game->getMod(), _game->getLanguage());
		_game->getSavedGame()->setBattleGame(bgame);

		if ((SDL_GetModState() & KMOD_CTRL) && (SDL_GetModState() & KMOD_ALT))
		{
			_game->getSavedGame()->setDisableSoldierEquipment(true);
		}
		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.runInventory(craft);

		_game->getScreen()->clear();
		_game->pushState(new InventoryState(false, 0, _base));
	}
}

void CraftEquipmentState::saveGlobalLoadout(int index)
{
	// clear the template
	ItemContainer *tmpl = _game->getSavedGame()->getGlobalCraftLoadout(index);
	tmpl->getContents()->clear();

	Craft *c = _base->getCrafts()->at(_craft);
	// save only what is visible on the screen (can be DIFFERENT than what's really in the craft for various reasons)
	for (auto& itemRule : _items)
	{
		RuleItem *item = _game->getMod()->getItem(itemRule, true);
		int cQty = 0;
		if (item->getVehicleUnit())
		{
			cQty = c->getVehicleCount(itemRule);
		}
		else
		{
			cQty = c->getItems()->getItem(itemRule);
		}
		if (cQty > 0)
		{
			tmpl->addItem(itemRule, cQty);
		}
	}
}

void CraftEquipmentState::loadGlobalLoadout(int index)
{
	// reset filters and reload the full equipment list
	_btnQuickSearch->setText("");
	_cbxFilterBy->setSelected(0);
	initList();

	// first move everything visible back to base
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		moveLeftByValue(INT_MAX);
	}

	// now start applying the template (consider ONLY items visible on the GUI)
	ItemContainer *tmpl = _game->getSavedGame()->getGlobalCraftLoadout(index);
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
		int tQty = tmpl->getItem(item->getName());
		moveRightByValue(tQty, true);
	}

	// lastly check and report what's missing
	Craft *c = _base->getCrafts()->at(_craft);
	std::string craftName = c->getName(_game->getLanguage());
	std::vector<ReequipStat> _missingItems;
	for (auto& templateItem : *tmpl->getContents())
	{
		RuleItem *item = _game->getMod()->getItem(templateItem.first, false);
		if (item)
		{
			int tQty = templateItem.second;
			int cQty = 0;
			if (item->getVehicleUnit())
			{
				// Note: we will also report HWPs as missing:
				// - if there is not enough ammo to arm them
				// - if there is not enough cargo space in the craft
				cQty = c->getVehicleCount(item->getName());
			}
			else
			{
				cQty = c->getItems()->getItem(item->getName());
			}
			int missing = tQty - cQty;
			if (missing > 0)
			{
				ReequipStat stat = { item->getName(), missing, craftName, item->getListOrder() };
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
		_reload = false;
	}
}

/**
* Opens the CraftEquipmentLoadState screen.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnLoadClick(Action *)
{
	if (_game->getSavedGame()->getMonthsPassed() > -1)
	{
		_game->pushState(new CraftEquipmentLoadState(this));
	}
}

/**
* Opens the CraftEquipmentSaveState screen.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnSaveClick(Action *)
{
	if (_game->getSavedGame()->getMonthsPassed() > -1)
	{
		_game->pushState(new CraftEquipmentSaveState(this));
	}
}

}
