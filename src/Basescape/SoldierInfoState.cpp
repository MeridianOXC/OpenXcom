/*
 * Copyright 2010-2017 OpenXcom Developers.
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
#include "SoldierInfoState.h"
#include "SoldierDiaryOverviewState.h"
#include <algorithm>
#include <sstream>
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Bar.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/ComboBox.h"
#include "../Engine/Surface.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CovertOperation.h"
#include "../Savegame/Soldier.h"
#include "../Engine/SurfaceSet.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleSoldier.h"
#include "../Menu/ErrorMessageState.h"
#include "SellState.h"
#include "SoldierArmorState.h"
#include "SoldierBonusState.h"
#include "SackSoldierState.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierBonus.h"
#include "../Savegame/SoldierDeath.h"

namespace OpenXcom
{
/**
 * Initializes all the elements in the Soldier Info screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from. NULL to use the dead soldiers list.
 * @param soldierId ID of the selected soldier.
 */
SoldierInfoState::SoldierInfoState(Base *base, size_t soldierId) : _base(base), _soldierId(soldierId), _soldier(0)
{
	if (_base == 0)
	{
		_list = _game->getSavedGame()->getDeadSoldiers();
		if (_soldierId >= _list->size())
		{
			_soldierId = 0;
		}
		else
		{
			_soldierId = _list->size() - (1 + _soldierId);
		}
	}
	else
	{
		_list = _base->getSoldiers();
	}

	_ftaUI = _game->getMod()->getIsFTAGame();
	_localChange = false;

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_rank = new Surface(26, 23, 4, 4);
	_flag = new InteractiveSurface(40, 20, 275, 6);
	_btnPrev = new TextButton(28, 14, 0, 33);
	_btnOk = new TextButton(48, 14, 30, 33);
	_btnNext = new TextButton(28, 14, 80, 33);
	_btnArmor = new TextButton(110, 14, 130, 33);
	_btnBonuses = new TextButton(16, 14, 242, 33);
	_edtSoldier = new TextEdit(this, 210, 16, 40, 9);
	_btnSack = new TextButton(60, 14, 260, 33);
	if (_ftaUI)
	{
		_btnDiary = new TextButton(60, 14, 260, 33);
	}
	else
	{
		_btnDiary = new TextButton(60, 14, 260, 48);
	}
	_cbxRoles = new ComboBox(this, 60, 14, 260, 48, false);
	_txtRank = new Text(130, 9, 0, 48);
	_txtMissions = new Text(100, 9, 130, 48);
	_txtKills = new Text(100, 9, 200, 48);
	_txtStuns = new Text(60, 9, 260, 48);
	_txtCraft = new Text(130, 9, 0, 56);
	_txtRecovery = new Text(180, 9, 130, 56);
	_txtOperation = new Text(180, 9, 0, 64);
	_txtPsionic = new Text(150, 9, 0, 66);
	_txtDead = new Text(150, 9, 130, 33);

	defineStatLines();

	// Set palette
	setInterface("soldierInfo");

	add(_bg);
	add(_rank);
	add(_flag);
	add(_btnOk, "button", "soldierInfo");
	add(_btnPrev, "button", "soldierInfo");
	add(_btnNext, "button", "soldierInfo");
	add(_btnArmor, "button", "soldierInfo");
	add(_btnBonuses, "button", "soldierInfo");
	add(_edtSoldier, "text1", "soldierInfo");
	add(_btnSack, "button", "soldierInfo");
	add(_btnDiary, "button", "soldierInfo");
	add(_cbxRoles, "button", "soldierInfo");
	add(_txtRank, "text1", "soldierInfo");
	add(_txtMissions, "text1", "soldierInfo");
	add(_txtKills, "text1", "soldierInfo");
	add(_txtStuns, "text1", "soldierInfo");
	add(_txtCraft, "text1", "soldierInfo");
	add(_txtOperation, "text1", "soldierInfo");
	add(_txtRecovery, "text1", "soldierInfo");
	add(_txtPsionic, "text2", "soldierInfo");
	add(_txtDead, "text2", "soldierInfo");

	addStatLines();

	centerAllSurfaces();

	// Set up objects
	_game->getMod()->getSurface("BACK06.SCR")->blitNShade(_bg, 0, 0);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&SoldierInfoState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SoldierInfoState::btnOkClick, Options::keyCancel);

	_btnPrev->setText("<<");
	if (_base == 0)
	{
		_btnPrev->onMouseClick((ActionHandler)&SoldierInfoState::btnNextClick);
		_btnPrev->onKeyboardPress((ActionHandler)&SoldierInfoState::btnNextClick, Options::keyBattlePrevUnit);
	}
	else
	{
		_btnPrev->onMouseClick((ActionHandler)&SoldierInfoState::btnPrevClick);
		_btnPrev->onKeyboardPress((ActionHandler)&SoldierInfoState::btnPrevClick, Options::keyBattlePrevUnit);
	}

	_btnNext->setText(">>");
	if (_base == 0)
	{
		_btnNext->onMouseClick((ActionHandler)&SoldierInfoState::btnPrevClick);
		_btnNext->onKeyboardPress((ActionHandler)&SoldierInfoState::btnPrevClick, Options::keyBattleNextUnit);
	}
	else
	{
		_btnNext->onMouseClick((ActionHandler)&SoldierInfoState::btnNextClick);
		_btnNext->onKeyboardPress((ActionHandler)&SoldierInfoState::btnNextClick, Options::keyBattleNextUnit);
	}

	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)&SoldierInfoState::btnArmorClick);

	_btnBonuses->setText(tr("STR_BONUSES_BUTTON")); // tiny button, default translation is " "
	_btnBonuses->onMouseClick((ActionHandler)&SoldierInfoState::btnBonusesClick);

	_edtSoldier->setBig();
	_edtSoldier->onChange((ActionHandler)&SoldierInfoState::edtSoldierChange);
	_edtSoldier->onMousePress((ActionHandler)&SoldierInfoState::edtSoldierPress);

	// Can't change nationality of dead soldiers
	if (_base != 0)
	{
		// Ignore also if flags are used to indicate number of kills
		if (_game->getMod()->getFlagByKills().empty())
		{
			_flag->onMouseClick((ActionHandler)&SoldierInfoState::btnFlagClick, SDL_BUTTON_LEFT);
			_flag->onMouseClick((ActionHandler)&SoldierInfoState::btnFlagClick, SDL_BUTTON_RIGHT);
		}
	}

	_btnSack->setText(tr("STR_SACK"));
	_btnSack->onMouseClick((ActionHandler)&SoldierInfoState::btnSackClick);
	if (_ftaUI)
	{
		_btnSack->setVisible(false);
		_btnSack->setX(0); //go away!
		_btnSack->setY(0);
		_btnSack->setWidth(0);
		_btnSack->setHeight(0);
	}

	_btnDiary->setText(tr("STR_DIARY"));
	_btnDiary->onMouseClick((ActionHandler)&SoldierInfoState::btnDiaryClick);
	_btnDiary->setVisible(Options::soldierDiaries);

	_rolesList.push_back("STR_SOLDIER");
	_rolesList.push_back("STR_PILOT");
	_rolesList.push_back("STR_AGENT");
	_rolesList.push_back("STR_SCIENTIST");
	_rolesList.push_back("STR_ENGINEER");
	_cbxRoles->setOptions(_rolesList, true);
	_cbxRoles->setSelected(0);
	_cbxRoles->onChange((ActionHandler)&SoldierInfoState::cbxRolesChange);
	_cbxRoles->setVisible(_ftaUI);

	_txtPsionic->setText(tr("STR_IN_PSIONIC_TRAINING"));

	nameBars();
}

/**
 *
 */
SoldierInfoState::~SoldierInfoState()
{

}

/**
 * Updates soldier stats when
 * the soldier changes.
 */
void SoldierInfoState::init()
{
	State::init();
	if (_list->empty())
	{
		_game->popState();
		return;
	}
	if (_soldierId >= _list->size())
	{
		_soldierId = 0;
	}
	_soldier = _list->at(_soldierId);
	_edtSoldier->setBig();
	_edtSoldier->setText(_soldier->getName());
	UnitStats *initial = _soldier->getInitStats();
	UnitStats *current = _soldier->getCurrentStats();

	bool hasBonus = _soldier->prepareStatsWithBonuses(_game->getMod()); // refresh all bonuses
	UnitStats withArmor = *_soldier->getStatsWithAllBonuses();
	_btnBonuses->setVisible(hasBonus);

	SoldierRole role = _soldier->getBestRole();
	// here we set default display, do not do this if rendering after combobox change!
	if (!_localChange) 
	{
		switch (role)
		{
		case OpenXcom::ROLE_SOLDIER:
			_cbxRoles->setSelected(0);
			break;
		case OpenXcom::ROLE_PILOT:
			_cbxRoles->setSelected(1);
			break;
		case OpenXcom::ROLE_AGENT:
			_cbxRoles->setSelected(2);
			break;
		case OpenXcom::ROLE_SCIENTIST:
			_cbxRoles->setSelected(3);
			break;
		case OpenXcom::ROLE_ENGINEER:
			_cbxRoles->setSelected(4);
			break;
		default:
			_cbxRoles->setSelected(0);
			break;
		}
	}

	SurfaceSet *texture = _game->getMod()->getSurfaceSet("BASEBITS.PCK");
	auto frame = texture->getFrame(_soldier->getRankSprite());
	if (_ftaUI)
	{
		frame = texture->getFrame(_soldier->getRoleRankSprite(role));
	}
	if (frame)
	{
		frame->blitNShade(_rank, 0, 0);
	}

	std::ostringstream flagId;
	flagId << "Flag";
	const std::vector<int> mapping = _game->getMod()->getFlagByKills();
	if (mapping.empty())
	{
		flagId << _soldier->getNationality() + _soldier->getRules()->getFlagOffset();
	}
	else
	{
		int index = 0;
		for (auto item : mapping)
		{
			if (_soldier->getKills() <= item)
			{
				break;
			}
			index++;
		}
		flagId << index + _soldier->getRules()->getFlagOffset();
	}
	Surface *flagTexture = _game->getMod()->getSurface(flagId.str().c_str(), false);
	_flag->clear();
	if (flagTexture != 0)
	{
		flagTexture->blitNShade(_flag, _flag->getWidth() - flagTexture->getWidth(), 0); // align right
	}

	fillNumbers();

	updateVisibility();

	std::string wsArmor;
	if (_soldier->getArmor() == _soldier->getRules()->getDefaultArmor())
	{
		wsArmor= tr("STR_ARMOR_").arg(tr(_soldier->getArmor()->getType()));
	}
	else
	{
		wsArmor = tr(_soldier->getArmor()->getType());
	}

	_btnArmor->setText(wsArmor);

	_btnSack->setVisible(_game->getSavedGame()->getMonthsPassed() > -1 && !(_soldier->getCraft() && _soldier->getCraft()->getStatus() == "STR_OUT"));
	if (_soldier->getCovertOperation() != 0)
	{
		_btnSack->setVisible(false);
	}
	_txtRank->setText(tr("STR_RANK_").arg(tr(_soldier->getRankString(_ftaUI))));

	_txtMissions->setText(tr("STR_MISSIONS").arg(_soldier->getMissions()));

	_txtKills->setText(tr("STR_KILLS").arg(_soldier->getKills()));

	_txtStuns->setText(tr("STR_STUNS").arg(_soldier->getStuns()));
	_txtStuns->setVisible(!Options::soldierDiaries);

	std::string craft;
	if (_soldier->getCraft() == 0)
	{
		craft = tr("STR_NONE_UC");
	}
	else
	{
		craft = _soldier->getCraft()->getName(_game->getLanguage());
	}
	_txtCraft->setText(tr("STR_CRAFT_").arg(craft));

	if (_soldier->getCovertOperation() != 0)
	{
		_txtOperation->setVisible(true);
		_txtOperation->setText(tr("STR_OPERATION_").arg(tr(_soldier->getCovertOperation()->getOperationName())));
	}
	else
	{
		_txtOperation->setVisible(false);
	}

	auto recovery = _base ? _base->getSumRecoveryPerDay() : BaseSumDailyRecovery();
	auto getDaysOrInfinity = [&](int days)
	{
		if (days < 0)
		{
			return std::string{ "∞" };
		}
		else
		{
			return std::string{tr("STR_DAY", days)};
		}
	};
	if (_soldier->isWounded())
	{
		int recoveryTime = _soldier->getNeededRecoveryTime(recovery);
		_txtRecovery->setText(tr("STR_WOUND_RECOVERY").arg(getDaysOrInfinity(recoveryTime)));
	}
	else
	{
		_txtRecovery->setText("");
		if (_soldier->getManaMissing() > 0)
		{
			int manaRecoveryTime = _soldier->getManaRecovery(recovery.ManaRecovery);
			_txtRecovery->setText(tr("STR_MANA_RECOVERY").arg(getDaysOrInfinity(manaRecoveryTime)));
		}
		if (_soldier->getHealthMissing() > 0)
		{
			int healthRecoveryTime = _soldier->getHealthRecovery(recovery.HealthRecovery);
			_txtRecovery->setText(tr("STR_HEALTH_RECOVERY").arg(getDaysOrInfinity(healthRecoveryTime)));
		}
	}

	_txtPsionic->setVisible(_soldier->isInPsiTraining());

	//we want to apply psionic visibility at the very end to overwrite preview settings

	// Dead can't talk
	if (_base == 0)
	{
		_btnArmor->setVisible(false);
		_btnSack->setVisible(false);
		_txtCraft->setVisible(false);
		_txtDead->setVisible(true);
		std::string status = "STR_MISSING_IN_ACTION";
		if (_soldier->getDeath() && _soldier->getDeath()->getCause())
		{
			status = "STR_KILLED_IN_ACTION";
		}
		_txtDead->setText(tr(status, _soldier->getGender()));
	}
	else
	{
		_txtDead->setVisible(false);
	}
	_localChange = false; // become ready for general screen update again.
}

/**
 * Disables the soldier input.
 * @param action Pointer to an action.
 */
void SoldierInfoState::edtSoldierPress(Action *)
{
	if (_base == 0)
	{
		_edtSoldier->setFocus(false);
	}
}

/**
 * Set the soldier Id.
 */
void SoldierInfoState::setSoldierId(size_t soldier)
{
	_soldierId = soldier;
}

/**
 * Changes the soldier's name.
 * @param action Pointer to an action.
 */
void SoldierInfoState::edtSoldierChange(Action *)
{
	_soldier->setName(_edtSoldier->getText());
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnOkClick(Action *)
{

	_game->popState();
	if (_game->getSavedGame()->getMonthsPassed() > -1 && Options::storageLimitsEnforced && _base != 0 && _base->storesOverfull())
	{
		_game->pushState(new SellState(_base, 0));
		_game->pushState(new ErrorMessageState(tr("STR_STORAGE_EXCEEDED").arg(_base->getName()), _palette, _game->getMod()->getInterface("soldierInfo")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("soldierInfo")->getElement("errorPalette")->color));
	}
}

/**
 * Goes to the previous soldier.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnPrevClick(Action *)
{
	if (_soldierId == 0)
		_soldierId = _list->size() - 1;
	else
		_soldierId--;
	init();
}

/**
 * Goes to the next soldier.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnNextClick(Action *)
{
	_soldierId++;
	if (_soldierId >= _list->size())
		_soldierId = 0;
	init();
}

/**
 * Shows the Select Armor window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnArmorClick(Action *)
{
	if (!_soldier->getCraft() || (_soldier->getCraft() && _soldier->getCraft()->getStatus() != "STR_OUT"))
	{
		if (_soldier->getCovertOperation() != 0)
		{
			return;
		}
		else
		{
			_game->pushState(new SoldierArmorState(_base, _soldierId, SA_GEOSCAPE));
		}
	}
}

/**
 * Shows the SoldierBonus window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnBonusesClick(Action *)
{
	_game->pushState(new SoldierBonusState(_base, _soldierId));
}

/**
 * Shows the Sack Soldier window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnSackClick(Action *)
{
	if (_soldier->getCovertOperation() != 0)
	{
		return;
	}
	else
	{
		_game->pushState(new SackSoldierState(_base, _soldierId));
	}
}

/**
 * Shows the Diary Soldier window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnDiaryClick(Action *)
{
	_game->pushState(new SoldierDiaryOverviewState(_base, _soldierId, this));
}

void SoldierInfoState::cbxRolesChange(Action *action)
{
	_localChange = true;
	init();
}

/**
* Changes soldier's nationality.
* @param action Pointer to an action.
*/
void SoldierInfoState::btnFlagClick(Action *action)
{
	int temp = _soldier->getNationality();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		temp += 1;
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		temp += -1;
	}

	const std::vector<SoldierNamePool*> &names = _soldier->getRules()->getNames();
	if (!names.empty())
	{
		const int max = names.size();
		if (temp > max - 1)
		{
			temp = 0;
		}
		else if (temp < 0)
		{
			temp = max - 1;
		}
	}
	else
	{
		temp = 0;
	}

	_soldier->setNationality(temp);
	init();
}

void OpenXcom::SoldierInfoState::defineStatLines()
{
	int yPos = 80;
	int step = 11;
	if (_game->getMod()->isManaFeatureEnabled())
	{
		yPos = 81;
		step = 10;
	}

	_txtTimeUnits = new Text(120, 9, 6, yPos);
	_numTimeUnits = new Text(18, 9, 131, yPos);
	_barTimeUnits = new Bar(170, 7, 150, yPos);
	_txtManeuvering = new Text(120, 9, 6, yPos);
	_numManeuvering = new Text(18, 9, 131, yPos);
	_barManeuvering = new Bar(170, 7, 150, yPos);
	_txtPhysics = new Text(120, 9, 6, yPos);
	_numPhysics = new Text(18, 9, 131, yPos);
	_barPhysics = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtStamina = new Text(120, 9, 6, yPos);
	_numStamina = new Text(18, 9, 131, yPos);
	_barStamina = new Bar(170, 7, 150, yPos);
	_txtMissiles = new Text(120, 9, 6, yPos);
	_numMissiles = new Text(18, 9, 131, yPos);
	_bartMissiles = new Bar(170, 7, 150, yPos);
	_txtChemistry = new Text(120, 9, 6, yPos);
	_numChemistry = new Text(18, 9, 131, yPos);
	_barChemistry = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtHealth = new Text(120, 9, 6, yPos);
	_numHealth = new Text(18, 9, 131, yPos);
	_barHealth = new Bar(170, 7, 150, yPos);
	_txtDogfight = new Text(120, 9, 6, yPos);
	_numDogfight = new Text(18, 9, 131, yPos);
	_barDogfight = new Bar(170, 7, 150, yPos);
	_txtBiology = new Text(120, 9, 6, yPos);
	_numBiology = new Text(18, 9, 131, yPos);
	_barBiology = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtBravery = new Text(120, 9, 6, yPos);
	_numBravery = new Text(18, 9, 131, yPos);
	_barBravery = new Bar(170, 7, 150, yPos);
	_txtInsight = new Text(120, 9, 6, yPos);
	_numInsight = new Text(18, 9, 131, yPos);
	_barInsight = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtReactions = new Text(120, 9, 6, yPos);
	_numReactions = new Text(18, 9, 131, yPos);
	_barReactions = new Bar(170, 7, 150, yPos);
	_txtTracking = new Text(120, 9, 6, yPos);
	_numTracking = new Text(18, 9, 131, yPos);
	_barTracking = new Bar(170, 7, 150, yPos);
	_txtData = new Text(120, 9, 6, yPos);
	_numData = new Text(18, 9, 131, yPos);
	_barData = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtFiring = new Text(120, 9, 6, yPos);
	_numFiring = new Text(18, 9, 131, yPos);
	_barFiring = new Bar(170, 7, 150, yPos);
	_txtCooperation = new Text(120, 9, 6, yPos);
	_numCooperation = new Text(18, 9, 131, yPos);
	_barCooperation = new Bar(170, 7, 150, yPos);
	_txtComputers = new Text(120, 9, 6, yPos);
	_numComputers = new Text(18, 9, 131, yPos);
	_barComputers = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtThrowing = new Text(120, 9, 6, yPos);
	_numThrowing = new Text(18, 9, 131, yPos);
	_barThrowing = new Bar(170, 7, 150, yPos);
	_txtBeams = new Text(120, 9, 6, yPos);
	_numBeams = new Text(18, 9, 131, yPos);
	_barBeams = new Bar(170, 7, 150, yPos);
	_txtTactics = new Text(120, 9, 6, yPos);
	_numTactics = new Text(18, 9, 131, yPos);
	_barTactics = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtMelee = new Text(120, 9, 6, yPos);
	_numMelee = new Text(18, 9, 131, yPos);
	_barMelee = new Bar(170, 7, 150, yPos);
	_txtSynaptic = new Text(120, 9, 6, yPos);
	_numSynaptic = new Text(18, 9, 131, yPos);
	_barSynaptic = new Bar(170, 7, 150, yPos);
	_txtMaterials = new Text(120, 9, 6, yPos);
	_numMaterials = new Text(18, 9, 131, yPos);
	_barMaterials = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtStrength = new Text(120, 9, 6, yPos);
	_numStrength = new Text(18, 9, 131, yPos);
	_barStrength = new Bar(170, 7, 150, yPos);
	_txtGravity = new Text(120, 9, 6, yPos);
	_numGravity = new Text(18, 9, 131, yPos);
	_barGravity = new Bar(170, 7, 150, yPos);
	_txtPsychology = new Text(120, 9, 6, yPos);
	_numPsychology = new Text(18, 9, 131, yPos);
	_barPsychology = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtMana = new Text(120, 9, 6, yPos);
	_numMana = new Text(18, 9, 131, yPos);
	_barMana = new Bar(170, 7, 150, yPos);
	_txtDesigning = new Text(120, 9, 6, yPos);
	_numDesigning = new Text(18, 9, 131, yPos);
	_barDesigning = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtPsiStrength = new Text(120, 9, 6, yPos);
	_numPsiStrength = new Text(18, 9, 131, yPos);
	_barPsiStrength = new Bar(170, 7, 150, yPos);
	_txtPsionics = new Text(120, 9, 6, yPos);
	_numPsionics = new Text(18, 9, 131, yPos);
	_barPsionics = new Bar(170, 7, 150, yPos);
	yPos += step;

	_txtPsiSkill = new Text(120, 9, 6, yPos);
	_numPsiSkill = new Text(18, 9, 131, yPos);
	_barPsiSkill = new Bar(170, 7, 150, yPos);
	_txtXenolinguistics = new Text(120, 9, 6, yPos);
	_numXenolinguistics = new Text(18, 9, 131, yPos);
	_barXenolinguistics = new Bar(170, 7, 150, yPos);
}

void OpenXcom::SoldierInfoState::addStatLines()
{
	add(_txtTimeUnits, "text2", "soldierInfo");
	add(_numTimeUnits, "numbers", "soldierInfo");
	add(_barTimeUnits, "barTUs", "soldierInfo");
	add(_txtManeuvering, "text2", "soldierInfo");
	add(_numManeuvering, "numbers", "soldierInfo");
	add(_barManeuvering, "barManeuvering", "soldierInfo");
	add(_txtPhysics, "text2", "soldierInfo");
	add(_numPhysics, "numbers", "soldierInfo");
	add(_barPhysics, "barPhysics", "soldierInfo");

	add(_txtStamina, "text2", "soldierInfo");
	add(_numStamina, "numbers", "soldierInfo");
	add(_barStamina, "barEnergy", "soldierInfo");
	add(_txtMissiles, "text2", "soldierInfo");
	add(_numMissiles, "numbers", "soldierInfo");
	add(_bartMissiles, "bartMissiles", "soldierInfo");
	add(_txtChemistry, "text2", "soldierInfo");
	add(_numChemistry, "numbers", "soldierInfo");
	add(_barChemistry, "barChemistry", "soldierInfo");

	add(_txtHealth, "text2", "soldierInfo");
	add(_numHealth, "numbers", "soldierInfo");
	add(_barHealth, "barHealth", "soldierInfo");
	add(_txtDogfight, "text2", "soldierInfo");
	add(_numDogfight, "numbers", "soldierInfo");
	add(_barDogfight, "barDogfight", "soldierInfo");
	add(_txtBiology, "text2", "soldierInfo");
	add(_numBiology, "numbers", "soldierInfo");
	add(_barBiology, "barBiology", "soldierInfo");

	add(_txtBravery, "text2", "soldierInfo");
	add(_numBravery, "numbers", "soldierInfo");
	add(_barBravery, "barBravery", "soldierInfo");
	add(_txtInsight, "text2", "soldierInfo");
	add(_numInsight, "numbers", "soldierInfo");
	add(_barInsight, "barInsight", "soldierInfo");

	add(_txtReactions, "text2", "soldierInfo");
	add(_numReactions, "numbers", "soldierInfo");
	add(_barReactions, "barReactions", "soldierInfo");
	add(_txtTracking, "text2", "soldierInfo");
	add(_numTracking, "numbers", "soldierInfo");
	add(_barTracking, "barTracking", "soldierInfo");
	add(_txtData, "text2", "soldierInfo");
	add(_numData, "numbers", "soldierInfo");
	add(_barData, "barData", "soldierInfo");

	add(_txtFiring, "text2", "soldierInfo");
	add(_numFiring, "numbers", "soldierInfo");
	add(_barFiring, "barFiring", "soldierInfo");
	add(_txtCooperation, "text2", "soldierInfo");
	add(_numCooperation, "numbers", "soldierInfo");
	add(_barCooperation, "barCooperation", "soldierInfo");
	add(_txtComputers, "text2", "soldierInfo");
	add(_numComputers, "numbers", "soldierInfo");
	add(_barComputers, "barComputers", "soldierInfo");

	add(_txtThrowing, "text2", "soldierInfo");
	add(_numThrowing, "numbers", "soldierInfo");
	add(_barThrowing, "barThrowing", "soldierInfo");
	add(_txtBeams, "text2", "soldierInfo");
	add(_numBeams, "numbers", "soldierInfo");
	add(_barBeams, "barBeams", "soldierInfo");
	add(_txtTactics, "text2", "soldierInfo");
	add(_numTactics, "numbers", "soldierInfo");
	add(_barTactics, "barTactics", "soldierInfo");

	add(_txtMelee, "text2", "soldierInfo");
	add(_numMelee, "numbers", "soldierInfo");
	add(_barMelee, "barMelee", "soldierInfo");
	add(_txtSynaptic, "text2", "soldierInfo");
	add(_numSynaptic, "numbers", "soldierInfo");
	add(_barSynaptic, "barSynaptic", "soldierInfo");
	add(_txtMaterials, "text2", "soldierInfo");
	add(_numMaterials, "numbers", "soldierInfo");
	add(_barMaterials, "barMaterials", "soldierInfo");

	add(_txtStrength, "text2", "soldierInfo");
	add(_numStrength, "numbers", "soldierInfo");
	add(_barStrength, "barStrength", "soldierInfo");
	add(_txtGravity, "text2", "soldierInfo");
	add(_numGravity, "numbers", "soldierInfo");
	add(_barGravity, "barGravity", "soldierInfo");
	add(_txtPsychology, "text2", "soldierInfo");
	add(_numPsychology, "numbers", "soldierInfo");
	add(_barPsychology, "barPsychology", "soldierInfo");

	add(_txtMana, "text2", "soldierInfo");
	add(_numMana, "numbers", "soldierInfo");
	add(_barMana, "barMana", "soldierInfo");
	add(_txtDesigning, "text2", "soldierInfo");
	add(_numDesigning, "numbers", "soldierInfo");
	add(_barDesigning, "barDesigning", "soldierInfo");

	add(_txtPsiStrength, "text2", "soldierInfo");
	add(_numPsiStrength, "numbers", "soldierInfo");
	add(_barPsiStrength, "barPsiStrength", "soldierInfo");
	add(_txtPsionics, "text2", "soldierInfo");
	add(_numPsionics, "numbers", "soldierInfo");
	add(_barPsionics, "barPsionics", "soldierInfo");

	add(_txtPsiSkill, "text2", "soldierInfo");
	add(_numPsiSkill, "numbers", "soldierInfo");
	add(_barPsiSkill, "barPsiSkill", "soldierInfo");
	add(_txtXenolinguistics, "text2", "soldierInfo");
	add(_numXenolinguistics, "numbers", "soldierInfo");
	add(_barXenolinguistics, "barXenolinguistics", "soldierInfo");
}

void OpenXcom::SoldierInfoState::nameBars()
{
	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));
	_barTimeUnits->setScale(1.0);
	_txtManeuvering->setText(tr("STR_MANEUVERING"));
	_barManeuvering->setScale(1.0);
	_txtPhysics->setText(tr("STR_PHYSICS_UC"));
	_barPhysics->setScale(1.0);

	_txtStamina->setText(tr("STR_STAMINA"));
	_barStamina->setScale(1.0);
	_txtMissiles->setText(tr("STR_MISSILE_OPERATION"));
	_bartMissiles->setScale(1.0);
	_txtChemistry->setText(tr("STR_CHEMISTRY_UC"));
	_barChemistry->setScale(1.0);

	_txtHealth->setText(tr("STR_HEALTH"));
	_barHealth->setScale(1.0);
	_txtDogfight->setText(tr("STR_DOGFIGHT"));
	_barDogfight->setScale(1.0);
	_txtBiology->setText(tr("STR_BIOLOGY_UC"));
	_barBiology->setScale(1.0);

	_txtBravery->setText(tr("STR_BRAVERY"));
	_barBravery->setScale(1.0);
	_txtInsight->setText(tr("STR_INSIGHT_UC"));
	_barInsight->setScale(1.0);

	_txtReactions->setText(tr("STR_REACTIONS"));
	_barReactions->setScale(1.0);
	_txtTracking->setText(tr("STR_TRACKING"));
	_barTracking->setScale(1.0);
	_txtData->setText(tr("STR_DATA_ANALISIS_UC"));
	_barData->setScale(1.0);

	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));
	_barFiring->setScale(1.0);
	_txtCooperation->setText(tr("STR_COOPERATION"));
	_barCooperation->setScale(1.0);
	_txtComputers->setText(tr("STR_COMPUTER_SCIENCE_UC"));
	_barComputers->setScale(1.0);

	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));
	_barThrowing->setScale(1.0);
	_txtBeams->setText(tr("STR_BEAMS_OPERATION"));
	_barBeams->setScale(1.0);
	_txtTactics->setText(tr("STR_TACTICS_UC"));
	_barTactics->setScale(1.0);

	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));
	_barMelee->setScale(1.0);
	_txtSynaptic->setText(tr("STR_SYNAPTIC_CONNECTIVITY"));
	_barSynaptic->setScale(1.0);
	_txtMaterials->setText(tr("STR_MATERIAL_SCIENCE_UC"));
	_barMaterials->setScale(1.0);

	_txtStrength->setText(tr("STR_STRENGTH"));
	_barStrength->setScale(1.0);
	_txtGravity->setText(tr("STR_GRAVITY_MANIPULATION"));
	_barGravity->setScale(1.0);
	_txtPsychology->setText(tr("STR_PSYCHOLOGY_UC"));
	_barPsychology->setScale(1.0);

	_txtMana->setText(tr("STR_MANA_POOL"));
	_barMana->setScale(1.0);
	_txtDesigning->setText(tr("STR_DESIGNING_UC"));
	_barDesigning->setScale(1.0);

	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));
	_barPsiStrength->setScale(1.0);
	_txtPsionics->setText(tr("STR_PSIONICS_UC"));
	_barPsionics->setScale(1.0);

	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));
	_barPsiSkill->setScale(1.0);
	_txtXenolinguistics->setText(tr("STR_XENOLINGUISTICS_UC"));
	_barXenolinguistics->setScale(1.0);
}

void OpenXcom::SoldierInfoState::fillNumbers()
{
	UnitStats withArmor = *_soldier->getStatsWithAllBonuses();
	UnitStats *current = _soldier->getCurrentStats();
	UnitStats *initial = _soldier->getInitStats();

	std::ostringstream ss1;
	ss1 << withArmor.tu;
	_numTimeUnits->setText(ss1.str());
	_barTimeUnits->setMax(current->tu);
	_barTimeUnits->setValue(withArmor.tu);
	_barTimeUnits->setValue2(std::min(withArmor.tu, initial->tu));
	std::ostringstream ss21;
	ss21 << withArmor.maneuvering;
	_numManeuvering->setText(ss21.str());
	_barManeuvering->setMax(current->maneuvering);
	_barManeuvering->setValue(withArmor.maneuvering);
	_barManeuvering->setValue2(std::min(withArmor.maneuvering, initial->maneuvering));
	std::ostringstream ss31;
	ss31 << withArmor.physics;
	_numPhysics->setText(ss31.str());
	_barPhysics->setMax(current->physics);
	_barPhysics->setValue(withArmor.physics);
	_barPhysics->setValue2(std::min(withArmor.physics, initial->physics));

	std::ostringstream ss2;
	ss2 << withArmor.stamina;
	_numStamina->setText(ss2.str());
	_barStamina->setMax(current->stamina);
	_barStamina->setValue(withArmor.stamina);
	_barStamina->setValue2(std::min(withArmor.stamina, initial->stamina));
	std::ostringstream ss22;
	ss22 << withArmor.missiles;
	_numMissiles->setText(ss22.str());
	_bartMissiles->setMax(current->missiles);
	_bartMissiles->setValue(withArmor.missiles);
	_bartMissiles->setValue2(std::min(withArmor.missiles, initial->missiles));
	std::ostringstream ss32;
	ss32 << withArmor.chemistry;
	_numChemistry->setText(ss32.str());
	_barChemistry->setMax(current->chemistry);
	_barChemistry->setValue(withArmor.chemistry);
	_barChemistry->setValue2(std::min(withArmor.chemistry, initial->chemistry));

	std::ostringstream ss3;
	ss3 << withArmor.health;
	_numHealth->setText(ss3.str());
	_barHealth->setMax(current->health);
	_barHealth->setValue(withArmor.health);
	_barHealth->setValue2(std::min(withArmor.health, initial->health));
	std::ostringstream ss23;
	ss23 << withArmor.dogfight;
	_numDogfight->setText(ss23.str());
	_barDogfight->setMax(current->dogfight);
	_barDogfight->setValue(withArmor.dogfight);
	_barDogfight->setValue2(std::min(withArmor.dogfight, initial->dogfight));
	std::ostringstream ss33;
	ss33 << withArmor.biology;
	_numBiology->setText(ss33.str());
	_barBiology->setMax(current->biology);
	_barBiology->setValue(withArmor.biology);
	_barBiology->setValue2(std::min(withArmor.biology, initial->biology));

	std::ostringstream ss4;
	ss4 << withArmor.bravery;
	_numBravery->setText(ss4.str());
	_barBravery->setMax(current->bravery);
	_barBravery->setValue(withArmor.bravery);
	_barBravery->setValue2(std::min(withArmor.bravery, initial->bravery));
	std::ostringstream ss34;
	ss34 << withArmor.insight;
	_numInsight->setText(ss34.str());
	_barInsight->setMax(current->insight);
	_barInsight->setValue(withArmor.insight);
	_barInsight->setValue2(std::min(withArmor.insight, initial->insight));

	std::ostringstream ss5;
	ss5 << withArmor.reactions;
	_numReactions->setText(ss5.str());
	_barReactions->setMax(current->reactions);
	_barReactions->setValue(withArmor.reactions);
	_barReactions->setValue2(std::min(withArmor.reactions, initial->reactions));
	std::ostringstream ss25;
	ss25 << withArmor.tracking;
	_numTracking->setText(ss25.str());
	_barTracking->setMax(current->tracking);
	_barTracking->setValue(withArmor.tracking);
	_barTracking->setValue2(std::min(withArmor.tracking, initial->tracking));
	std::ostringstream ss35;
	ss35 << withArmor.data;
	_numData->setText(ss35.str());
	_barData->setMax(current->data);
	_barData->setValue(withArmor.data);
	_barData->setValue2(std::min(withArmor.data, initial->data));

	std::ostringstream ss6;
	ss6 << withArmor.firing;
	_numFiring->setText(ss6.str());
	_barFiring->setMax(current->firing);
	_barFiring->setValue(withArmor.firing);
	_barFiring->setValue2(std::min(withArmor.firing, initial->firing));
	std::ostringstream ss26;
	ss26 << withArmor.cooperation;
	_numCooperation->setText(ss26.str());
	_barCooperation->setMax(current->cooperation);
	_barCooperation->setValue(withArmor.cooperation);
	_barCooperation->setValue2(std::min(withArmor.cooperation, initial->cooperation));
	std::ostringstream ss36;
	ss36 << withArmor.computers;
	_numComputers->setText(ss36.str());
	_barComputers->setMax(current->computers);
	_barComputers->setValue(withArmor.computers);
	_barComputers->setValue2(std::min(withArmor.computers, initial->computers));

	std::ostringstream ss7;
	ss7 << withArmor.throwing;
	_numThrowing->setText(ss7.str());
	_barThrowing->setMax(current->throwing);
	_barThrowing->setValue(withArmor.throwing);
	_barThrowing->setValue2(std::min(withArmor.throwing, initial->throwing));
	std::ostringstream ss27;
	ss27 << withArmor.beams;
	_numBeams->setText(ss27.str());
	_barBeams->setMax(current->beams);
	_barBeams->setValue(withArmor.beams);
	_barBeams->setValue2(std::min(withArmor.beams, initial->beams));
	std::ostringstream ss37;
	ss37 << withArmor.tactics;
	_numTactics->setText(ss37.str());
	_barTactics->setMax(current->tactics);
	_barTactics->setValue(withArmor.tactics);
	_barTactics->setValue2(std::min(withArmor.tactics, initial->tactics));

	std::ostringstream ss8;
	ss8 << withArmor.melee;
	_numMelee->setText(ss8.str());
	_barMelee->setMax(current->melee);
	_barMelee->setValue(withArmor.melee);
	_barMelee->setValue2(std::min(withArmor.melee, initial->melee));
	std::ostringstream ss28;
	ss28 << withArmor.synaptic;
	_numSynaptic->setText(ss28.str());
	_barSynaptic->setMax(current->synaptic);
	_barSynaptic->setValue(withArmor.synaptic);
	_barSynaptic->setValue2(std::min(withArmor.synaptic, initial->synaptic));
	std::ostringstream ss38;
	ss38 << withArmor.materials;
	_numMaterials->setText(ss37.str());
	_barMaterials->setMax(current->materials);
	_barMaterials->setValue(withArmor.materials);
	_barMaterials->setValue2(std::min(withArmor.materials, initial->materials));
	

	std::ostringstream ss9;
	ss9 << withArmor.strength;
	_numStrength->setText(ss9.str());
	_barStrength->setMax(current->strength);
	_barStrength->setValue(withArmor.strength);
	_barStrength->setValue2(std::min(withArmor.strength, initial->strength));
	std::ostringstream ss29;
	ss29 << withArmor.gravity;
	_numGravity->setText(ss29.str());
	_barGravity->setMax(current->gravity);
	_barGravity->setValue(withArmor.gravity);
	_barGravity->setValue2(std::min(withArmor.gravity, initial->gravity));
	std::ostringstream ss39;
	ss39 << withArmor.psychology;
	_numPsychology->setText(ss38.str());
	_barPsychology->setMax(current->psychology);
	_barPsychology->setValue(withArmor.psychology);
	_barPsychology->setValue2(std::min(withArmor.physics, initial->psychology));


	std::ostringstream ss10;
	ss10 << withArmor.mana;
	_numMana->setText(ss10.str());
	_barMana->setMax(current->mana);
	_barMana->setValue(withArmor.mana);
	_barMana->setValue2(std::min(withArmor.mana, initial->mana));
	std::ostringstream ss310;
	ss310 << withArmor.designing;
	_numDesigning->setText(ss39.str());
	_barDesigning->setMax(current->designing);
	_barDesigning->setValue(withArmor.designing);
	_barDesigning->setValue2(std::min(withArmor.designing, initial->designing));
	

	std::ostringstream ss11;
	ss11 << withArmor.psiStrength;
	_numPsiStrength->setText(ss11.str());
	_barPsiStrength->setMax(current->psiStrength);
	_barPsiStrength->setValue(withArmor.psiStrength);
	_barPsiStrength->setValue2(std::min(withArmor.psiStrength, initial->psiStrength));
	std::ostringstream ss311;
	ss311 << withArmor.xenolinguistics;
	_numPsionics->setText(ss311.str());
	_barPsionics->setMax(current->psionics);
	_barPsionics->setValue(withArmor.psionics);
	_barPsionics->setValue2(std::min(withArmor.psionics, initial->psionics));

	std::ostringstream ss12;
	ss12 << withArmor.psiSkill;
	_numPsiSkill->setText(ss12.str());
	_barPsiSkill->setMax(current->psiSkill);
	_barPsiSkill->setValue(withArmor.psiSkill);
	_barPsiSkill->setValue2(std::min(withArmor.psiSkill, initial->psiSkill));
	std::ostringstream ss312;
	ss312 << withArmor.xenolinguistics;
	_numXenolinguistics->setText(ss312.str());
	_barXenolinguistics->setMax(current->xenolinguistics);
	_barXenolinguistics->setValue(withArmor.xenolinguistics);
	_barXenolinguistics->setValue2(std::min(withArmor.xenolinguistics, initial->xenolinguistics));
}

void SoldierInfoState::updateVisibility()
{
	hideAllStats();
	SoldierRole selected = static_cast<SoldierRole>(_cbxRoles->getSelected());
	displayPsionic(selected);

	if (selected == ROLE_SOLDIER)
	{
		_txtTimeUnits->setVisible(true);
		_numTimeUnits->setVisible(true);
		_barTimeUnits->setVisible(true);

		_txtStamina->setVisible(true);
		_numStamina->setVisible(true);
		_barStamina->setVisible(true);

		_txtHealth->setVisible(true);
		_numHealth->setVisible(true);
		_barHealth->setVisible(true);

		_txtBravery->setVisible(true);
		_numBravery->setVisible(true);
		_barBravery->setVisible(true);

		_txtReactions->setVisible(true);
		_numReactions->setVisible(true);
		_barReactions->setVisible(true);

		_txtFiring->setVisible(true);
		_numFiring->setVisible(true);
		_barFiring->setVisible(true);

		_txtThrowing->setVisible(true);
		_numThrowing->setVisible(true);
		_barThrowing->setVisible(true);

		_txtMelee->setVisible(true);
		_numMelee->setVisible(true);
		_barMelee->setVisible(true);

		_txtStrength->setVisible(true);
		_numStrength->setVisible(true);
		_barStrength->setVisible(true);
	}
	else if (selected == ROLE_PILOT)
	{
		_txtManeuvering->setVisible(true);
		_numManeuvering->setVisible(true);
		_barManeuvering->setVisible(true);

		_txtMissiles->setVisible(true);
		_numMissiles->setVisible(true);
		_bartMissiles->setVisible(true);

		_txtDogfight->setVisible(true);
		_numDogfight->setVisible(true);
		_barDogfight->setVisible(true);

		_txtBravery->setVisible(true);
		_numBravery->setVisible(true);
		_barBravery->setVisible(true);

		_txtTracking->setVisible(true);
		_numTracking->setVisible(true);
		_barTracking->setVisible(true);

		_txtCooperation->setVisible(true);
		_numCooperation->setVisible(true);
		_barCooperation->setVisible(true);

		if (_game->getSavedGame()->isResearched(_game->getMod()->getBeamOperationsUnlockResearch()))
		{
			_txtBeams->setVisible(true);
			_numBeams->setVisible(true);
			_barBeams->setVisible(true);
		}

		if (_game->getSavedGame()->isResearched(_game->getMod()->getCraftSynapseUnlockResearch()))
		{
			_txtSynaptic->setVisible(true);
			_numSynaptic->setVisible(true);
			_barSynaptic->setVisible(true);
		}
		
		if (_game->getSavedGame()->isResearched(_game->getMod()->getGravControlUnlockResearch()))
		{
			_txtGravity->setVisible(true);
			_numGravity->setVisible(true);
			_barGravity->setVisible(true);
		}
	}
	else if (selected == ROLE_SCIENTIST)
	{
		_txtPhysics->setVisible(true);
		_numPhysics->setVisible(true);
		_barPhysics->setVisible(true);

		_txtChemistry->setVisible(true);
		_numChemistry->setVisible(true);
		_barChemistry->setVisible(true);

		_txtBiology->setVisible(true);
		_numBiology->setVisible(true);
		_barBiology->setVisible(true);

		_txtInsight->setVisible(true);
		_numInsight->setVisible(true);
		_barInsight->setVisible(true);

		_txtData->setVisible(true);
		_numData->setVisible(true);
		_barData->setVisible(true);

		_txtComputers->setVisible(true);
		_numComputers->setVisible(true);
		_barComputers->setVisible(true);

		_txtTactics->setVisible(true);
		_numTactics->setVisible(true);
		_barTactics->setVisible(true);

		_txtMaterials->setVisible(true);
		_numMaterials->setVisible(true);
		_barMaterials->setVisible(true);

		_txtPsychology->setVisible(true);
		_numPsychology->setVisible(true);
		_barPsychology->setVisible(true);

		_txtDesigning->setVisible(true);
		_numDesigning->setVisible(true);
		_barDesigning->setVisible(true);

		if (_game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()))
		{
			_txtPsionics->setVisible(true);
			_numPsionics->setVisible(true);
			_barPsionics->setVisible(true);
		}
		
		if (_game->getSavedGame()->isResearched(_game->getMod()->getXenologyUnlockResearch()))
		{
			_txtXenolinguistics->setVisible(true);
			_numXenolinguistics->setVisible(true);
			_barXenolinguistics->setVisible(true);
		}
	}
}

void OpenXcom::SoldierInfoState::displayPsionic(SoldierRole selected)
{
	if (_game->getMod()->isManaFeatureEnabled()
		&& _game->getSavedGame()->isManaUnlocked(_game->getMod())
		&& (selected == ROLE_SOLDIER || selected == ROLE_AGENT)) 
	{
		_txtMana->setVisible(true);
		_numMana->setVisible(true);
		_barMana->setVisible(true);
	}

	bool psi = _soldier->getStatsWithSoldierBonusesOnly()->psiSkill > 0
		&& (selected == ROLE_SOLDIER || selected == ROLE_AGENT || selected == ROLE_PILOT);
	if (psi || ((Options::psiStrengthEval && !_ftaUI) && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements())))
	{
		_txtPsiStrength->setVisible(true);
		_numPsiStrength->setVisible(true);
		_barPsiStrength->setVisible(true);
	}
	if (psi)
	{
		_txtPsiSkill->setVisible(true);
		_numPsiSkill->setVisible(true);
		_barPsiSkill->setVisible(true);
	}
}

void OpenXcom::SoldierInfoState::hideAllStats()
{
	_txtTimeUnits->setVisible(false);
	_numTimeUnits->setVisible(false);
	_barTimeUnits->setVisible(false);

	_txtStamina->setVisible(false);
	_numStamina->setVisible(false);
	_barStamina->setVisible(false);

	_txtHealth->setVisible(false);
	_numHealth->setVisible(false);
	_barHealth->setVisible(false);

	_txtBravery->setVisible(false);
	_numBravery->setVisible(false);
	_barBravery->setVisible(false);

	_txtReactions->setVisible(false);
	_numReactions->setVisible(false);
	_barReactions->setVisible(false);

	_txtFiring->setVisible(false);
	_numFiring->setVisible(false);
	_barFiring->setVisible(false);

	_txtThrowing->setVisible(false);
	_numThrowing->setVisible(false);
	_barThrowing->setVisible(false);

	_txtMelee->setVisible(false);
	_numMelee->setVisible(false);
	_barMelee->setVisible(false);

	_txtStrength->setVisible(false);
	_numStrength->setVisible(false);
	_barStrength->setVisible(false);

	_txtMana->setVisible(false);
	_numMana->setVisible(false);
	_barMana->setVisible(false);

	_txtPsiSkill->setVisible(false);
	_numPsiSkill->setVisible(false);
	_barPsiSkill->setVisible(false);

	_txtPsiStrength->setVisible(false);
	_numPsiStrength->setVisible(false);
	_barPsiStrength->setVisible(false);

	// hide pilot stats
	_txtManeuvering->setVisible(false);
	_numManeuvering->setVisible(false);
	_barManeuvering->setVisible(false);

	_txtMissiles->setVisible(false);
	_numMissiles->setVisible(false);
	_bartMissiles->setVisible(false);

	_txtDogfight->setVisible(false);
	_numDogfight->setVisible(false);
	_barDogfight->setVisible(false);

	_txtTracking->setVisible(false);
	_numTracking->setVisible(false);
	_barTracking->setVisible(false);

	_txtCooperation->setVisible(false);
	_numCooperation->setVisible(false);
	_barCooperation->setVisible(false);

	_txtBeams->setVisible(false);
	_numBeams->setVisible(false);
	_barBeams->setVisible(false);

	_txtSynaptic->setVisible(false);
	_numSynaptic->setVisible(false);
	_barSynaptic->setVisible(false);

	_txtGravity->setVisible(false);
	_numGravity->setVisible(false);
	_barGravity->setVisible(false);

	// hide science
	_txtPhysics->setVisible(false);
	_numPhysics->setVisible(false);
	_barPhysics->setVisible(false);

	_txtChemistry->setVisible(false);
	_numChemistry->setVisible(false);
	_barChemistry->setVisible(false);

	_txtBiology->setVisible(false);
	_numBiology->setVisible(false);
	_barBiology->setVisible(false);

	_txtInsight->setVisible(false);
	_numInsight->setVisible(false);
	_barInsight->setVisible(false);

	_txtData->setVisible(false);
	_numData->setVisible(false);
	_barData->setVisible(false);

	_txtComputers->setVisible(false);
	_numComputers->setVisible(false);
	_barComputers->setVisible(false);

	_txtTactics->setVisible(false);
	_numTactics->setVisible(false);
	_barTactics->setVisible(false);

	_txtMaterials->setVisible(false);
	_numMaterials->setVisible(false);
	_barMaterials->setVisible(false);

	_txtPsychology->setVisible(false);
	_numPsychology->setVisible(false);
	_barPsychology->setVisible(false);

	_txtDesigning->setVisible(false);
	_numDesigning->setVisible(false);
	_barDesigning->setVisible(false);

	_txtPsionics->setVisible(false);
	_numPsionics->setVisible(false);
	_barPsionics->setVisible(false);

	_txtXenolinguistics->setVisible(false);
	_numXenolinguistics->setVisible(false);
	_barXenolinguistics->setVisible(false);
}

}
