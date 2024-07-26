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
#include "CraftInfoState.h"
#include <cmath>
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Action.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Vehicle.h"
#include "CraftSoldiersState.h"
#include "CraftWeaponsState.h"
#include "CraftEquipmentState.h"
#include "CraftArmorState.h"
#include "CraftPilotsState.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Info screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craftId ID of the selected craft.
 */
CraftInfoState::CraftInfoState(Base *base, size_t craftId) : _base(base), _craftId(craftId), _craft(0)
{
	// Create objects
	if (_game->getSavedGame()->getMonthsPassed() != -1)
	{
		_window = new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	}
	else
	{
		_window = new Window(this, 320, 200, 0, 0, POPUP_NONE);
	}

	_craft = _base->getCrafts()->at(_craftId);
	_weaponNum = _craft->getRules()->getWeapons();
	if (_weaponNum > RuleCraft::WeaponMax)
		_weaponNum = RuleCraft::WeaponMax;

	int showNewBattle = 0;
	if (_game->getSavedGame()->getDebugMode() && _game->getSavedGame()->getMonthsPassed() != -1)
	{
		// only the first craft can be used
		if (_craftId == 0 && _craft->getRules()->isForNewBattle())
		{
			showNewBattle = 1;
		}
	}

	const int top = _weaponNum > 2 ? 42 : 64;
	const int top_row = 41;
	const int bottom = 125;
	const int bottom_row = 17;
	bool pilots = _craft->getRules()->getPilots() > 0;
	_btnOk = new TextButton((pilots ? 218 : 288) - showNewBattle * 98, 16, pilots ? 86 : 16, 176);
	_btnNewBattle = new TextButton(92, 16, 212, 176);
	for (int i = 0; i < _weaponNum; ++i)
	{
		const int x = i % 2 ? 282 : 14;
		const int y = top + (i / 2) * top_row;
		_btnW[i] = new TextButton(24, 32, x, y);
	}
	_btnCrew = new TextButton(64, 16, 16, bottom);
	_btnEquip = new TextButton(64, 16, 16, bottom + bottom_row);
	_btnArmor = new TextButton(64, 16, 16, bottom + 2 * bottom_row);
	_btnPilots = new TextButton(64, 16, 16, bottom + 3 * bottom_row);
	_edtCraft = new TextEdit(this, 140, 16, 80, 8);
	_txtDamage = new Text(100, 17, 14, 24);
	_txtShield = new Text(100, 17, 120, 24);
	_txtFuel = new Text(82, 17, 228, 24);
	_txtSkin = new Text(32, 9, 144, 46);
	for (int i = 0; i < _weaponNum; ++i)
	{
		const int x = i % 2 ? 204 : 46;
		const int y = top + (i / 2) * top_row;
		const int d = i % 2 ? 20 : 0;
		_txtWName[i] = new Text(95, 16, x - d, y);
		_txtWAmmo[i] = new Text(75, 24, x, y + 16);
	}
	_sprite = new InteractiveSurface(32, 40, 144, 56);
	for (int i = 0; i < _weaponNum; ++i)
	{
		const int x = i % 2 ? 184 : 121;
		const int y = top + 16 + (i / 2) * top_row;
		_weapon[i] = new InteractiveSurface(15, 17, x, y);
	}
	_crew = new Surface(220, 18, 85, bottom - 1);
	_equip = new Surface(220, 18, 85, bottom + bottom_row);

	// Set palette
	setInterface("craftInfo");

	add(_window, "window", "craftInfo");
	add(_btnOk, "button", "craftInfo");
	add(_btnNewBattle, "button", "craftInfo");
	for (int i = 0; i < _weaponNum; ++i)
	{
		add(_btnW[i], "button", "craftInfo");
	}
	add(_btnCrew, "button", "craftInfo");
	add(_btnEquip, "button", "craftInfo");
	add(_btnArmor, "button", "craftInfo");
	add(_btnPilots, "button", "craftInfo");
	add(_edtCraft, "text1", "craftInfo");
	add(_txtDamage, "text1", "craftInfo");
	add(_txtShield, "text1", "craftInfo");
	add(_txtFuel, "text1", "craftInfo");
	add(_txtSkin, "text1", "craftInfo");
	for (int i = 0; i < _weaponNum; ++i)
	{
		add(_txtWName[i], "text2", "craftInfo");
		add(_txtWAmmo[i], "text3", "craftInfo");
		add(_weapon[i]);
	}
	add(_sprite);
	add(_crew);
	add(_equip);

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftInfo");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftInfoState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftInfoState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&CraftInfoState::btnUfopediaClick, Options::keyGeoUfopedia);

	_btnNewBattle->setText(tr("STR_NEW_BATTLE"));
	_btnNewBattle->onMouseClick((ActionHandler)&CraftInfoState::btnNewBattleClick);
	_btnNewBattle->setVisible(showNewBattle > 0);

	for (int i = 0; i < _weaponNum; ++i)
	{
		const char num[] = { char('1' + i), 0 };
		_btnW[i]->setText(num);
		_btnW[i]->onMouseClick((ActionHandler)&CraftInfoState::btnWClick);
		_weapon[i]->onMouseClick((ActionHandler)&CraftInfoState::btnWIconClick);
	}

	_sprite->onMouseClick((ActionHandler)&CraftInfoState::btnCraftIconClick);

	_btnCrew->setText(tr("STR_CREW"));
	_btnCrew->onMouseClick((ActionHandler)&CraftInfoState::btnCrewClick);

	_btnEquip->setText(tr("STR_EQUIPMENT_UC"));
	_btnEquip->onMouseClick((ActionHandler)&CraftInfoState::btnEquipClick);

	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)&CraftInfoState::btnArmorClick);

	_btnPilots->setText(tr("STR_PILOTS"));
	_btnPilots->onMouseClick((ActionHandler)&CraftInfoState::btnPilotsClick);
	_btnPilots->setVisible(pilots);

	_edtCraft->setBig();
	_edtCraft->setAlign(ALIGN_CENTER);
	_edtCraft->onChange((ActionHandler)&CraftInfoState::edtCraftChange);

	_txtSkin->setAlign(ALIGN_CENTER);
	if (_craft->getRules()->getMaxSkinIndex() > 0)
	{
		_txtSkin->setText(tr("STR_CRAFT_SKIN_ID").arg(_craft->getSkinIndex()));
	}

	for (int i =0; i < _weaponNum; ++i)
	{
		_txtWName[i]->setWordWrap(true);
	}
}

/**
 *
 */
CraftInfoState::~CraftInfoState()
{

}

/**
 * The craft info can change
 * after going into other screens.
 */
void CraftInfoState::init()
{
	State::init();

	_edtCraft->setText(_craft->getName(_game->getLanguage()));

	_sprite->clear();
	SurfaceSet *texture = _game->getMod()->getSurfaceSet("BASEBITS.PCK");
	texture->getFrame(_craft->getSkinSprite() + 33)->blitNShade(_sprite, 0, 0);

	std::ostringstream firlsLine;
	firlsLine << tr("STR_DAMAGE_UC_").arg(Unicode::formatPercentage(_craft->getDamagePercentage()));
	if (_craft->getStatus() == "STR_REPAIRS" && _craft->getDamage() > 0)
	{
		int damageHours = (int)ceil((double)_craft->getDamage() / _craft->getRules()->getRepairRate());
		firlsLine << formatTime(damageHours);
	}
	_txtDamage->setText(firlsLine.str());

	std::ostringstream secondLine;
	secondLine << tr("STR_FUEL").arg(Unicode::formatPercentage(_craft->getFuelPercentage()));
	if (_craft->getStatus() == "STR_REFUELLING" && _craft->getFuelMax() - _craft->getFuel() > 0)
	{
		int fuelHours = (int)ceil((double)(_craft->getFuelMax() - _craft->getFuel()) / _craft->getRules()->getRefuelRate() / 2.0);
		secondLine << formatTime(fuelHours);
	}
	_txtFuel->setText(secondLine.str());

	std::ostringstream thirdLine;
	if (_craft->getShieldCapacity() != 0)
	{
		thirdLine << tr("STR_SHIELD").arg(Unicode::formatPercentage(_craft->getShieldPercentage()));
		if (_craft->getShield() < _craft->getShieldCapacity())
		{
			if (_craft->getRules()->getShieldRechargeAtBase() != 0)
			{
				int shieldHours = (int)ceil((double)(_craft->getShieldCapacity() - _craft->getShield()) / _craft->getRules()->getShieldRechargeAtBase());
				thirdLine << formatTime(shieldHours);
			}
		}
	}
	else
	{
		thirdLine << "";
	}
	_txtShield->setText(thirdLine.str());

	if (_craft->getRules()->getMaxUnitsLimit() > 0)
	{
		_crew->clear();
		_equip->clear();

		Surface *frame1 = texture->getFrame(38);

		SurfaceSet *customArmorPreviews = _game->getMod()->getSurfaceSet("CustomArmorPreviews");
		int x = 0;
		for (const auto* soldier : *_base->getSoldiers())
		{
			if (soldier->getCraft() == _craft)
			{
				for (int index : soldier->getArmor()->getCustomArmorPreviewIndex())
				{
					Surface *customFrame1 = customArmorPreviews->getFrame(index);
					if (customFrame1)
					{
						// modded armor previews
						customFrame1->blitNShade(_crew, x, 0);
					}
					else
					{
						// vanilla
						frame1->blitNShade(_crew, x, 0);
					}
					x += 10;
				}
			}
		}

		Surface *frame2 = texture->getFrame(40);

		SurfaceSet *customItemPreviews = _game->getMod()->getSurfaceSet("CustomItemPreviews");
		x = 0;
		for (const auto* vehicle : *_craft->getVehicles())
		{
			for (int index : vehicle->getRules()->getCustomItemPreviewIndex())
			{
				Surface *customFrame2 = customItemPreviews->getFrame(index);
				if (customFrame2)
				{
					// modded HWP/auxiliary previews
					customFrame2->blitNShade(_equip, x, 0);
				}
				else
				{
					// vanilla
					frame2->blitNShade(_equip, x, 0);
				}
				x += 10;
			}
		}

		Surface *frame3 = texture->getFrame(39);

		using ArrayIndexes = std::array<int, 3>;
		using ArraySurfaces = std::array<const Surface *, 3>;
		std::map<ArrayIndexes, std::tuple<ArraySurfaces, size_t>, std::greater<>> itemsBySprite;

		for (auto& item : *_craft->getItems()->getContents())
		{
			ArrayIndexes ind = { };

			// fill default values
			for (auto& arr : ind)
			{
				arr = -1;
			}

			// load values from config, zip will clip range to min length of one of arguments
			for (auto [arr, prev] : Collections::zipTie(Collections::range(ind), Collections::range(item.first->getCustomItemPreviewIndex())))
			{
				arr = prev;
			}

			auto& pos = itemsBySprite[ind];

			// update surfaces if not set yet
			for (auto [surf, arr] : Collections::zipTie(Collections::range(std::get<ArraySurfaces>(pos)), Collections::range(ind)))
			{
				if (surf != nullptr || arr < 0)
				{
					break;
				}

				surf = customItemPreviews->getFrame(arr);
			}

			std::get<size_t>(pos) += item.second;
		}

		for (const auto& pair : itemsBySprite)
		{
			const auto& pos = pair.second;
			if (std::get<ArraySurfaces>(pos)[0])
			{
				// new logic for items grouped by sprite
				size_t i = 4, next = 8;

				// draw icons for next "fibonacci" item count
				do
				{
					for (auto& s : std::get<ArraySurfaces>(pos))
					{
						if (s)
						{
							s->blitNShade(_equip, x, 0);
							x += 10;
							i = std::exchange(next, next + i); // calling this here make multi part sprites occupy similar size to single part ones
						}
						else
						{
							break;
						}
					}
				}
				while (i <= std::get<size_t>(pos));
			}
			else
			{
				// classic behavior
				for (size_t i = 0; i < std::get<size_t>(pos); i += 4)
				{
					frame3->blitNShade(_equip, x, 0);
					x += 10;
				}
			}
		}
	}
	else
	{
		_crew->setVisible(false);
		_equip->setVisible(false);
		_btnCrew->setVisible(false);
		_btnEquip->setVisible(false);
		_btnArmor->setVisible(false);
		_btnPilots->setVisible(false);
	}

	for (int i = 0; i < _weaponNum; ++i)
	{
		CraftWeapon *w1 = _craft->getWeapons()->at(i);

		_weapon[i]->clear();
		if (w1 != 0)
		{
			Surface *frame = texture->getFrame(w1->getRules()->getSprite() + 48);
			frame->blitNShade(_weapon[i], 0, 0);

			std::ostringstream weaponLine;
			if (w1->isDisabled()) weaponLine << "*";
			weaponLine << Unicode::TOK_COLOR_FLIP << tr(w1->getRules()->getType());
			_txtWName[i]->setText(weaponLine.str());
			weaponLine.str("");
			if (w1->getRules()->getAmmoMax())
			{
				weaponLine << tr("STR_AMMO_").arg(w1->getAmmo()) << "\n" << Unicode::TOK_COLOR_FLIP;
				weaponLine << tr("STR_MAX").arg(w1->getRules()->getAmmoMax());
				if (_craft->getStatus() == "STR_REARMING" && w1->getAmmo() < w1->getRules()->getAmmoMax() && !w1->isDisabled())
				{
					int rearmHours = (int)ceil((double)(w1->getRules()->getAmmoMax() - w1->getAmmo()) / w1->getRules()->getRearmRate());
					weaponLine << formatTime(rearmHours);
				}
			}
			_txtWAmmo[i]->setText(weaponLine.str());
			if (!_craft->getRules()->getFixedWeaponInSlot(i).empty())
			{
				_btnW[i]->setVisible(false); // cannot remove or change a fixed weapon
			}
		}
		else
		{
			_txtWName[i]->setText("");
			_txtWAmmo[i]->setText("");
		}
	}

}

/**
 * Turns an amount of time into a
 * day/hour string.
 * @param total Amount in hours.
 */
std::string CraftInfoState::formatTime(int total)
{
	std::ostringstream ss;
	int days = total / 24;
	int hours = total % 24;
	ss << "\n(";
	if (days > 0)
	{
		ss << tr("STR_DAY", days) << "/";
	}
	if (hours > 0)
	{
		ss << tr("STR_HOUR", hours);
	}
	ss << ")";
	return ss.str();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens the corresponding Ufopaedia craft article.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnUfopediaClick(Action *)
{
	if (_craft)
	{
		std::string articleId = _craft->getRules()->getType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

/**
 * Updates the New Battle file (battle.cfg) and returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnNewBattleClick(Action *)
{
	_game->popState();

	size_t mission = 0;
	size_t craft = 0;
	size_t darkness = 0;
	size_t terrain = 0;
	size_t alienRace = 0;
	size_t difficulty = 0;
	size_t alienTech = 0;

	std::string s = Options::getMasterUserFolder() + "battle.cfg";
	if (!CrossPlatform::fileExists(s))
	{
		// nothing
	}
	else
	{
		try
		{
			YAML::Node doc = YAML::Load(*CrossPlatform::readFile(s));
			mission = doc["mission"].as<size_t>(0);
			//craft = doc["craft"].as<size_t>(0);
			darkness = doc["darkness"].as<size_t>(0);
			terrain = doc["terrain"].as<size_t>(0);
			alienRace = doc["alienRace"].as<size_t>(0);
			//difficulty = doc["difficulty"].as<size_t>(0);
			alienTech = doc["alienTech"].as<size_t>(0);
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_WARNING) << e.what();
		}
	}

	// index of the craft type in the New Battle combobox
	size_t idx = 0;
	for (auto& craftType : _game->getMod()->getCraftsList())
	{
		const RuleCraft* rule = _game->getMod()->getCraft(craftType);
		if (rule->isForNewBattle())
		{
			if (rule == _craft->getRules())
			{
				craft = idx;
				break;
			}
			++idx;
		}
		else
		{
			// don't increase the index, these crafts are not in the New Battle combobox
		}
	}

	// transfer also the difficulty
	difficulty = _game->getSavedGame()->getDifficulty();

	YAML::Emitter out;
	YAML::Node node;
	node["mission"] = mission;
	node["craft"] = craft;
	node["darkness"] = darkness;
	node["terrain"] = terrain;
	node["alienRace"] = alienRace;
	node["difficulty"] = difficulty;
	node["alienTech"] = alienTech;
	node["base"] = _base->save();
	out << node;

	if (!CrossPlatform::writeFile(s, out.c_str()))
	{
		Log(LOG_WARNING) << "Failed to save " << s;
	}
}

/**
 * Goes to the Select Armament window
 * for the weapons.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnWClick(Action * act)
{
	for (int i = 0; i < _weaponNum; ++i)
	{
		if (act->getSender() == _btnW[i])
		{
			_game->pushState(new CraftWeaponsState(_base, _craftId, i));
			return;
		}
	}
}

/**
 * Toggles the enabled/disabled status of the weapon.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnWIconClick(Action *action)
{
	for (int i = 0; i < _weaponNum; ++i)
	{
		if (action->getSender() == _weapon[i])
		{
			CraftWeapon *w1 = _craft->getWeapons()->at(i);
			if (w1)
			{
				// Toggle the weapon status
				w1->setDisabled(!w1->isDisabled());

				// If we just enabled the weapon, we should begin rearming immediately.
				if (!w1->isDisabled())
				{
					_craft->checkup();
				}

				// Update the onscreen info.
				// Note: This method is overkill, since we only need to update a few things. But at least this ensures we haven't missed anything.
				init();
			}
		}
	}
}

/**
 * Toggles the craft skin.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnCraftIconClick(Action *action)
{
	if (_craft->getRules()->getMaxSkinIndex() > 0)
	{
		int newIndex = _craft->getSkinIndex() + 1;
		if (newIndex > _craft->getRules()->getMaxSkinIndex())
		{
			newIndex = 0;
		}
		_craft->setSkinIndex(newIndex);

		_txtSkin->setText(tr("STR_CRAFT_SKIN_ID").arg(_craft->getSkinIndex()));

		_sprite->clear();
		SurfaceSet* texture = _game->getMod()->getSurfaceSet("BASEBITS.PCK");
		texture->getFrame(_craft->getSkinSprite() + 33)->blitNShade(_sprite, 0, 0);
	}
}

/**
 * Goes to the Select Squad screen.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnCrewClick(Action *)
{
	_game->pushState(new CraftSoldiersState(_base, _craftId));
}

/**
 * Goes to the Select Equipment screen.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnEquipClick(Action *)
{
	_game->pushState(new CraftEquipmentState(_base, _craftId));
}

/**
 * Goes to the Select Armor screen.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnArmorClick(Action *)
{
	_game->pushState(new CraftArmorState(_base, _craftId));
}

/**
 * Goes to the Pilots Info screen.
 * @param action Pointer to an action.
 */
void CraftInfoState::btnPilotsClick(Action *)
{
	_game->pushState(new CraftPilotsState(_base, _craftId));
}

/**
 * Changes the Craft name.
 * @param action Pointer to an action.
 */
void CraftInfoState::edtCraftChange(Action *action)
{
	if (_edtCraft->getText() == _craft->getDefaultName(_game->getLanguage()))
	{
		_craft->setName("");
	}
	else
	{
		_craft->setName(_edtCraft->getText());
	}
	if (action->getDetails()->key.keysym.sym == SDLK_RETURN ||
		action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		_edtCraft->setText(_craft->getName(_game->getLanguage()));
	}
}

}
