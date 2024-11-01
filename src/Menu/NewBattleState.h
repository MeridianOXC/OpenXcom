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
#include "../Engine/State.h"
#include <map>
#include <vector>
#include <string>

namespace OpenXcom
{

enum class NewBattleSelectType { MISSION = 0, TERRAIN, ALIENRACE, GLOBETEXTURE };

class TextButton;
class TextEdit;
class TextList;
class ToggleTextButton;
class Window;
class Text;
class ComboBox;
class Slider;
class Frame;
class Craft;

/**
 * New Battle that displays a list
 * of options to configure a new
 * standalone mission.
 */
class NewBattleState : public State
{
private:
	Window *_window;
	Frame *_frameLeft, *_frameRight;
	Text *_txtTitle, *_txtMapOptions, *_txtAlienOptions;
	Text *_txtMission, *_txtCraft, *_txtDarkness, *_txtTerrain, *_txtDifficulty, *_txtAlienRace, *_txtAlienTech, *_txtDepth;
	ComboBox *_cbxMission, *_cbxCraft, *_cbxTerrain, *_cbxDifficulty, *_cbxAlienRace;
	Slider *_slrDarkness, *_slrAlienTech, *_slrDepth;
	TextButton *_btnOk, *_btnCancel, *_btnEquip, *_btnRandom;
	TextButton *_btnMission, *_btnTerrain, *_btnAlienRace;
	Text *_txtGlobeTexture;
	TextButton *_btnGlobeTexture;
	TextButton *_btnGlobeTextureToggle;
	ToggleTextButton *_btnUfoLanded;
	TextList *_lstSelect;
	TextEdit *_btnQuickSearch;
	std::map<Surface*, bool> _surfaceBackup;
	std::vector<std::string> _missionTypes, _terrainTypes, _alienRaces, _crafts;
	std::vector<std::string> _globeTextures;
	std::vector<int> _globeTextureIDs;
	Craft *_craft;
	NewBattleSelectType _selectType;
	bool _isRightClick;
	bool _depthVisible, _globeTextureVisible;
	size_t _selectedGlobeTexture;
	std::vector<size_t> _filtered;

	static const int TFTD_DEPLOYMENTS = 22;
	void fillList(NewBattleSelectType selectType, bool isRightClick);
	void cleanup();
public:
	/// Creates the New Battle state.
	NewBattleState();
	/// Cleans up the New Battle state.
	~NewBattleState();
	/// Handle keypresses.
	void handle(Action* action) override;
	/// Resets state.
	void init() override;
	/// Loads New Battle settings.
	void load(const std::string &filename = "battle");
	/// Saves New Battle settings.
	void save(const std::string &filename = "battle");
	/// Initializes a blank savegame.
	void initSave();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for clicking the Randomize button.
	void btnRandomClick(Action *action);
	/// Handler for clicking the Equip Craft button.
	void btnEquipClick(Action *action);
	/// Handler for changing the Mission combobox.
	void cbxMissionChange(Action *action);
	/// Handler for changing the Craft combobox.
	void cbxCraftChange(Action *action);
	/// Updates the depth slider accordingly when terrain selection changes.
	void cbxTerrainChange(Action *action);
	/// Handler for clicking the Mission... button.
	void btnMissionChange(Action *action);
	/// Handler for clicking the Terrain... button.
	void btnTerrainChange(Action *action);
	/// Handlers for clicking the Globe Texture buttons.
	void btnGlobeTextureChange(Action *action);
	void btnGlobeTextureToggle(Action *action);
	/// Handler for clicking the Alien Race... button.
	void btnAlienRaceChange(Action *action);
	/// Handler for clicking the Select list.
	void lstSelectClick(Action *action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action *action);
	void btnQuickSearchApply(Action *action);
};

}
