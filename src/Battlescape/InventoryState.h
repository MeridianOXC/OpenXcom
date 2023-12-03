#pragma once
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
#include <map>
#include "../Engine/State.h"
#include "../Interface/TextButton.h"
#include "../Savegame/EquipmentLayoutItem.h"


namespace OpenXcom
{

class Surface;
class Text;
class TextEdit;
class NumberText;
class InteractiveSurface;
class Inventory;
class SavedBattleGame;
class BattlescapeState;
class BattleUnit;
class BattlescapeButton;
class Base;

/**
 * Screen which displays soldier's inventory.
 */
class InventoryState : public State
{
private:
	Surface *_bg, *_soldier;
	Text *_txtItem, *_txtAmmo, *_txtWeight, *_txtTus, *_txtStatLine1, *_txtStatLine2, *_txtStatLine3, *_txtStatLine4;
	TextEdit *_txtName;
	TextEdit *_btnQuickSearch;
	BattlescapeButton *_btnOk, *_btnPrev, *_btnNext, *_btnUnload, *_btnGround, *_btnRank, *_btnArmor;
	BattlescapeButton *_btnCreateTemplate, *_btnApplyTemplate;
	BattlescapeButton *_btnLinks;
	Surface *_selAmmo;
	Inventory *_inv;
	std::vector<EquipmentLayoutItem*> _curInventoryTemplate, _tempInventoryTemplate;
	SavedBattleGame *_battleGame;
	const bool _tu, _noCraft;
	BattlescapeState *_parent;
	Base *_base;
	std::map<Soldier*, Craft*> _backup;
	bool _resetCustomDeploymentBackup;
	std::string _currentTooltip;
	std::string _currentDamageTooltip;
	int _mouseHoverItemFrame = 0;
	BattleItem *_mouseHoverItem = nullptr;
	BattleItem *_currentDamageTooltipItem = nullptr;
	bool _reloadUnit;
	int _globalLayoutIndex;
	int _prev_key = 0, _key_repeats = 0;
	/// Helper method for Create Template button
	void _createInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate);
	/// Helper method for Apply Template button
	void _applyInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate);
public:
	/// Creates the Inventory state.
	InventoryState(bool tu, BattlescapeState *parent, Base *base, bool noCraft = false);
	/// Cleans up the Inventory state.
	~InventoryState();
	/// Updates all soldier info.
	void setGlobalLayoutIndex(int index, bool armorChanged);
	void init() override;
	/// Handler for pressing on the Name edit.
	void edtSoldierPress(Action *action);
	/// Handler for changing text on the Name edit.
	void edtSoldierChange(Action *action);
	/// Updates the soldier info (Weight, TU).
	void updateStats();
	/// Saves the soldiers' equipment-layout.
	void saveEquipmentLayout();
	/// Handler for clicking the Armor button.
	void btnArmorClick(Action *action);
	void btnArmorClickRight(Action *action);
	void btnArmorClickMiddle(Action *action);
	/// Methods for handling the global equipment layout save/load hotkeys.
	void saveGlobalLayout(int index, bool includingArmor);
	void loadGlobalLayout(int index);
	bool loadGlobalLayoutArmor(int index);
	bool tryArmorChange(const std::string& armorName);
	void btnGlobalEquipmentLayoutClick(Action *action);
	/// Handler for clicking the Load button.
	void btnInventoryLoadClick(Action *action);
	/// Handler for clicking the Save button.
	void btnInventorySaveClick(Action *action);
	/// Handler for clicking the Ufopaedia button.
	void btnUfopaediaClick(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action *action);
	void btnQuickSearchApply(Action *action);
	/// Handler for clicking the Previous button.
	void btnPrevClick(Action *action);
	/// Handler for clicking the Next button.
	void btnNextClick(Action *action);
	/// Handler for clicking the Unload button.
	void btnUnloadClick(Action *action);
	/// Handler for clicking on the Ground -> button.
	void btnGroundClickForward(Action *action);
	void btnGroundClickBackward(Action *action);
	/// Handler for clicking the Rank button.
	void btnRankClick(Action *action);
	/// Handler for clicking the Links button.
	void btnLinksClick(Action *action);
	/// Handler for clicking on the Create Template button.
	void btnCreateTemplateClick(Action *action);
	void btnCreatePersonalTemplateClick(Action *action);
	/// Handler for clicking the Apply Template button.
	void btnApplyTemplateClick(Action *action);
	void btnApplyPersonalTemplateClick(Action *action);
	void btnShowPersonalTemplateClick(Action *action);
	/// Handler for hitting the Clear Inventory hotkey.
	void onClearInventory(Action *action);
	/// Handler for hitting the Auto-equip hotkey.
	void onAutoequip(Action *action);
	/// Handler for hitting the Generate Backstory hotkey.
	void onGenerateBackstory(Action *action);
	/// Handler for clicking on the inventory.
	void invClick(Action *action);
	/// Handler for showing item info.
	void calculateCurrentDamageTooltip();
	void invMouseOver(Action *action);
	/// Handler for hiding item info.
	void invMouseOut(Action *action);
	/// Handler for hitting the [Move Ground Inventory To Base] hotkey.
	void onMoveGroundInventoryToBase(Action *action);
	/// Handles keypresses.
	void handle(Action *action) override;
	/// Runs state functionality every cycle.
	void think() override;
	/// Handler for showing tooltip.
	void txtTooltipIn(Action *action);
	/// Handler for hiding tooltip.
	void txtTooltipOut(Action *action);
	/// Handler for showing armor tooltip.
	void txtArmorTooltipIn(Action *action);
	/// Handler for hiding armor tooltip.
	void txtArmorTooltipOut(Action *action);

private:
	/// Update the visibility and icons for the template buttons.
	void updateTemplateButtons(bool isVisible);
	/// Refresh the hover status of the mouse.
	void refreshMouse();
};

}
