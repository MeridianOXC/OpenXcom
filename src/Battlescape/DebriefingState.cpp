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
#include <algorithm>
#include <climits>
#include "TileEngine.h"
#include "DebriefingState.h"
#include "CannotReequipState.h"
#include "../Geoscape/GeoscapeEventState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "PromotionsState.h"
#include "CommendationState.h"
#include "CommendationLateState.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleRegion.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleUfo.h"
#include "../Mod/Armor.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Vehicle.h"
#include "../Savegame/BaseFacility.h"
#include <sstream>
#include "../Menu/ErrorMessageState.h"
#include "../Menu/MainMenuState.h"
#include "../Interface/Cursor.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Basescape/ManageAlienContainmentState.h"
#include "../Basescape/TransferBaseState.h"
#include "../Engine/Screen.h"
#include "../Basescape/SellState.h"
#include "../Menu/SaveGameState.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/RuleInterface.h"
#include "../Savegame/MissionStatistics.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../fallthrough.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Debriefing screen.
 * @param game Pointer to the core game.
 */
DebriefingState::DebriefingState() : _eventToSpawn(nullptr), _region(0), _country(0), _positiveScore(true), _destroyBase(false), _showSellButton(true), _initDone(false), _pageNumber(0)
{
	_missionStatistics = new MissionStatistics();

	Options::baseXResolution = Options::baseXGeoscape;
	Options::baseYResolution = Options::baseYGeoscape;
	_game->getScreen()->resetDisplay(false);

	// Restore the cursor in case something weird happened
	_game->getCursor()->setVisible(true);
	_limitsEnforced = Options::storageLimitsEnforced ? 1 : 0;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(40, 12, 16, 180);
	_btnStats = new TextButton(60, 12, 244, 180);
	_btnSell = new TextButton(60, 12, 176, 180);
	_btnTransfer = new TextButton(80, 12, 88, 180);
	_txtTitle = new Text(300, 17, 16, 8);
	_txtItem = new Text(180, 9, 16, 24);
	_txtQuantity = new Text(60, 9, 200, 24);
	_txtScore = new Text(55, 9, 270, 24);
	_txtRecovery = new Text(180, 9, 16, 60);
	_txtRating = new Text(200, 9, 64, 180);
	_lstStats = new TextList(290, 80, 16, 32);
	_lstRecovery = new TextList(290, 80, 16, 32);
	_lstTotal = new TextList(290, 9, 16, 12);

	// Second page (soldier stats)
	_txtSoldier     = new Text(90, 9,  16, 24); //16..106 = 90
	_txtTU          = new Text(18, 9, 106, 24); //106
	_txtStamina     = new Text(18, 9, 124, 24); //124
	_txtHealth      = new Text(18, 9, 142, 24); //142
	_txtBravery     = new Text(18, 9, 160, 24); //160
	_txtReactions   = new Text(18, 9, 178, 24); //178
	_txtFiring      = new Text(18, 9, 196, 24); //196
	_txtThrowing    = new Text(18, 9, 214, 24); //214
	_txtMelee       = new Text(18, 9, 232, 24); //232
	_txtStrength    = new Text(18, 9, 250, 24); //250
	_txtPsiStrength = new Text(18, 9, 268, 24); //268
	_txtPsiSkill    = new Text(18, 9, 286, 24); //286..304 = 18

	_lstSoldierStats = new TextList(288, 144, 16, 32); // 18 rows

	_txtTooltip = new Text(200, 9, 64, 180);

	// Third page (recovered items)
	_lstRecoveredItems = new TextList(272, 144, 16, 32); // 18 rows

	applyVisibility();

	// Set palette
	setInterface("debriefing");

	_ammoColor = _game->getMod()->getInterface("debriefing")->getElement("totals")->color;

	add(_window, "window", "debriefing");
	add(_btnOk, "button", "debriefing");
	add(_btnStats, "button", "debriefing");
	add(_btnSell, "button", "debriefing");
	add(_btnTransfer, "button", "debriefing");
	add(_txtTitle, "heading", "debriefing");
	add(_txtItem, "text", "debriefing");
	add(_txtQuantity, "text", "debriefing");
	add(_txtScore, "text", "debriefing");
	add(_txtRecovery, "text", "debriefing");
	add(_txtRating, "text", "debriefing");
	add(_lstStats, "list", "debriefing");
	add(_lstRecovery, "list", "debriefing");
	add(_lstTotal, "totals", "debriefing");

	add(_txtSoldier, "text", "debriefing");
	add(_txtTU, "text", "debriefing");
	add(_txtStamina, "text", "debriefing");
	add(_txtHealth, "text", "debriefing");
	add(_txtBravery, "text", "debriefing");
	add(_txtReactions, "text", "debriefing");
	add(_txtFiring, "text", "debriefing");
	add(_txtThrowing, "text", "debriefing");
	add(_txtMelee, "text", "debriefing");
	add(_txtStrength, "text", "debriefing");
	add(_txtPsiStrength, "text", "debriefing");
	add(_txtPsiSkill, "text", "debriefing");
	add(_lstSoldierStats, "list", "debriefing");
	add(_txtTooltip, "text", "debriefing");

	add(_lstRecoveredItems, "list", "debriefing");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "debriefing");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&DebriefingState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&DebriefingState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&DebriefingState::btnOkClick, Options::keyCancel);

	_btnStats->onMouseClick((ActionHandler)&DebriefingState::btnStatsClick);

	_btnSell->setText(tr("STR_SELL"));
	_btnSell->onMouseClick((ActionHandler)&DebriefingState::btnSellClick);
	_btnTransfer->setText(tr("STR_TRANSFER_UC"));
	_btnTransfer->onMouseClick((ActionHandler)&DebriefingState::btnTransferClick);

	_txtTitle->setBig();

	_txtItem->setText(tr("STR_LIST_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtQuantity->setAlign(ALIGN_RIGHT);

	_txtScore->setText(tr("STR_SCORE"));

	_lstStats->setColumns(3, 224, 30, 64);
	_lstStats->setDot(true);

	_lstRecovery->setColumns(3, 224, 30, 64);
	_lstRecovery->setDot(true);

	_lstTotal->setColumns(2, 254, 64);
	_lstTotal->setDot(true);

	// Second page
	_txtSoldier->setText(tr("STR_NAME_UC"));

	_txtTU->setAlign(ALIGN_CENTER);
	_txtTU->setText(tr("STR_TIME_UNITS_ABBREVIATION"));
	_txtTU->setTooltip("STR_TIME_UNITS");
	_txtTU->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtTU->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtStamina->setAlign(ALIGN_CENTER);
	_txtStamina->setText(tr("STR_STAMINA_ABBREVIATION"));
	_txtStamina->setTooltip("STR_STAMINA");
	_txtStamina->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtStamina->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtHealth->setAlign(ALIGN_CENTER);
	_txtHealth->setText(tr("STR_HEALTH_ABBREVIATION"));
	_txtHealth->setTooltip("STR_HEALTH");
	_txtHealth->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtHealth->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtBravery->setAlign(ALIGN_CENTER);
	_txtBravery->setText(tr("STR_BRAVERY_ABBREVIATION"));
	_txtBravery->setTooltip("STR_BRAVERY");
	_txtBravery->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtBravery->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtReactions->setAlign(ALIGN_CENTER);
	_txtReactions->setText(tr("STR_REACTIONS_ABBREVIATION"));
	_txtReactions->setTooltip("STR_REACTIONS");
	_txtReactions->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtReactions->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtFiring->setAlign(ALIGN_CENTER);
	_txtFiring->setText(tr("STR_FIRING_ACCURACY_ABBREVIATION"));
	_txtFiring->setTooltip("STR_FIRING_ACCURACY");
	_txtFiring->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtFiring->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtThrowing->setAlign(ALIGN_CENTER);
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY_ABBREVIATION"));
	_txtThrowing->setTooltip("STR_THROWING_ACCURACY");
	_txtThrowing->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtThrowing->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtMelee->setAlign(ALIGN_CENTER);
	_txtMelee->setText(tr("STR_MELEE_ACCURACY_ABBREVIATION"));
	_txtMelee->setTooltip("STR_MELEE_ACCURACY");
	_txtMelee->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtMelee->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtStrength->setAlign(ALIGN_CENTER);
	_txtStrength->setText(tr("STR_STRENGTH_ABBREVIATION"));
	_txtStrength->setTooltip("STR_STRENGTH");
	_txtStrength->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtStrength->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtPsiStrength->setAlign(ALIGN_CENTER);
	if (_game->getMod()->isManaFeatureEnabled())
	{
		_txtPsiStrength->setText(tr("STR_MANA_ABBREVIATION"));
		_txtPsiStrength->setTooltip("STR_MANA_POOL");
	}
	else
	{
		_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH_ABBREVIATION"));
		_txtPsiStrength->setTooltip("STR_PSIONIC_STRENGTH");
	}
	_txtPsiStrength->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtPsiStrength->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_txtPsiSkill->setAlign(ALIGN_CENTER);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_ABBREVIATION"));
	_txtPsiSkill->setTooltip("STR_PSIONIC_SKILL");
	_txtPsiSkill->onMouseIn((ActionHandler)&DebriefingState::txtTooltipIn);
	_txtPsiSkill->onMouseOut((ActionHandler)&DebriefingState::txtTooltipOut);

	_lstSoldierStats->setColumns(13, 90, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 0);
	_lstSoldierStats->setAlign(ALIGN_CENTER);
	_lstSoldierStats->setAlign(ALIGN_LEFT, 0);
	_lstSoldierStats->setDot(true);

	// Third page
	_lstRecoveredItems->setColumns(2, 254, 18);
	_lstRecoveredItems->setAlign(ALIGN_LEFT);
	_lstRecoveredItems->setDot(true);
}

/**
 *
 */
DebriefingState::~DebriefingState()
{
	for (std::vector<DebriefingStat*>::iterator i = _stats.begin(); i != _stats.end(); ++i)
	{
		delete *i;
	}
	for (std::map<int, RecoveryItem*>::iterator i = _recoveryStats.begin(); i != _recoveryStats.end(); ++i)
	{
		delete i->second;
	}
	_recoveryStats.clear();
	_rounds.clear();
	_roundsPainKiller.clear();
	_roundsStimulant.clear();
	_roundsHeal.clear();
	_recoveredItems.clear();
}

std::string DebriefingState::makeSoldierString(int stat)
{
	if (stat == 0) return "";

	std::ostringstream ss;
	ss << Unicode::TOK_COLOR_FLIP << '+' << stat << Unicode::TOK_COLOR_FLIP;
	return ss.str();
}

void DebriefingState::applyVisibility()
{
	bool showScore = _pageNumber == 0;
	bool showStats = _pageNumber == 1;
	bool showItems = _pageNumber == 2;

	// First page (scores)
	_txtItem->setVisible(showScore || showItems);
	_txtQuantity->setVisible(showScore);
	_txtScore->setVisible(showScore);
	_txtRecovery->setVisible(showScore);
	_txtRating->setVisible(showScore);
	_lstStats->setVisible(showScore);
	_lstRecovery->setVisible(showScore);
	_lstTotal->setVisible(showScore);

	// Second page (soldier stats)
	_txtSoldier->setVisible(showStats);
	_txtTU->setVisible(showStats);
	_txtStamina->setVisible(showStats);
	_txtHealth->setVisible(showStats);
	_txtBravery->setVisible(showStats);
	_txtReactions->setVisible(showStats);
	_txtFiring->setVisible(showStats);
	_txtThrowing->setVisible(showStats);
	_txtMelee->setVisible(showStats);
	_txtStrength->setVisible(showStats);
	_txtPsiStrength->setVisible(showStats);
	_txtPsiSkill->setVisible(showStats);
	_lstSoldierStats->setVisible(showStats);
	_txtTooltip->setVisible(showStats);

	// Third page (recovered items)
	_lstRecoveredItems->setVisible(showItems);

	// Set text on toggle button accordingly
	_btnSell->setVisible(showItems && _showSellButton);
	_btnTransfer->setVisible(showItems && _showSellButton && _game->getSavedGame()->getBases()->size() > 1);
	if (showScore)
	{
		_btnStats->setText(tr("STR_STATS"));
	}
	else if (showStats)
	{
		_btnStats->setText(tr("STR_LOOT"));
	}
	else if (showItems)
	{
		_btnStats->setText(tr("STR_SCORE"));
	}
}

void DebriefingState::init()
{
	State::init();

	if (_initDone)
	{
		return;
	}
	_initDone = true;

	prepareDebriefing();

	for (std::vector<SoldierStatsEntry>::iterator i = _soldierStats.begin(); i != _soldierStats.end(); ++i)
	{
		auto tmp = (*i).second.psiStrength;
		if (_game->getMod()->isManaFeatureEnabled())
		{
			tmp = (*i).second.mana;
		}
		_lstSoldierStats->addRow(13, (*i).first.c_str(),
				makeSoldierString((*i).second.tu).c_str(),
				makeSoldierString((*i).second.stamina).c_str(),
				makeSoldierString((*i).second.health).c_str(),
				makeSoldierString((*i).second.bravery).c_str(),
				makeSoldierString((*i).second.reactions).c_str(),
				makeSoldierString((*i).second.firing).c_str(),
				makeSoldierString((*i).second.throwing).c_str(),
				makeSoldierString((*i).second.melee).c_str(),
				makeSoldierString((*i).second.strength).c_str(),
				makeSoldierString(tmp).c_str(),
				makeSoldierString((*i).second.psiSkill).c_str(),
				"");
		// note: final dummy element to cause dot filling until the end of the line
	}

	// compare stuff from after and before recovery
	if (_base && _showSellButton)
	{
		int row = 0;
		ItemContainer *origBaseItems = _game->getSavedGame()->getSavedBattle()->getBaseStorageItems();
		const std::vector<std::string> &items = _game->getMod()->getItemsList();
		for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
		{
			int qty = _base->getStorageItems()->getItem(*i);
			if (qty > 0 && (Options::canSellLiveAliens || !_game->getMod()->getItem(*i)->isAlien()))
			{
				RuleItem *rule = _game->getMod()->getItem(*i);

				// IGNORE vehicles and their ammo
				// Note: because their number in base has been messed up by Base::setupDefenses() already in geoscape :(
				if (rule->getVehicleUnit())
				{
					// if this vehicle requires ammo, remember to ignore it later too
					if (rule->getVehicleClipAmmo())
					{
						origBaseItems->addItem(rule->getVehicleClipAmmo(), 1000000);
					}
					continue;
				}

				qty -= origBaseItems->getItem(*i);
				if (qty > 0)
				{
					_recoveredItems[rule] = qty;

					std::ostringstream ss;
					ss << Unicode::TOK_COLOR_FLIP << qty << Unicode::TOK_COLOR_FLIP;
					std::string item = tr(*i);
					if (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0))
					{
						item.insert(0, "  ");
						_lstRecoveredItems->addRow(2, item.c_str(), ss.str().c_str());
						_lstRecoveredItems->setRowColor(row, _ammoColor);
					}
					else
					{
						_lstRecoveredItems->addRow(2, item.c_str(), ss.str().c_str());
					}
					++row;
				}
			}
		}
	}

	int total = 0, statsY = 0, recoveryY = 0;
	int civiliansSaved = 0, civiliansDead = 0;
	int aliensKilled = 0, aliensStunned = 0;
	for (std::vector<DebriefingStat*>::iterator i = _stats.begin(); i != _stats.end(); ++i)
	{
		if ((*i)->qty == 0)
			continue;

		std::ostringstream ss, ss2;
		ss << Unicode::TOK_COLOR_FLIP << (*i)->qty << Unicode::TOK_COLOR_FLIP;
		ss2 << Unicode::TOK_COLOR_FLIP << (*i)->score;
		total += (*i)->score;
		if ((*i)->recovery)
		{
			_lstRecovery->addRow(3, tr((*i)->item).c_str(), ss.str().c_str(), ss2.str().c_str());
			recoveryY += 8;
		}
		else
		{
			_lstStats->addRow(3, tr((*i)->item).c_str(), ss.str().c_str(), ss2.str().c_str());
			statsY += 8;
		}
		if ((*i)->item == "STR_CIVILIANS_SAVED")
		{
			civiliansSaved = (*i)->qty;
		}
		if ((*i)->item == "STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES" || (*i)->item == "STR_CIVILIANS_KILLED_BY_ALIENS")
		{
			civiliansDead += (*i)->qty;
		}
		if ((*i)->item == "STR_ALIENS_KILLED")
		{
			aliensKilled += (*i)->qty;
		}
		if ((*i)->item == "STR_LIVE_ALIENS_RECOVERED")
		{
			aliensStunned += (*i)->qty;
		}
		}
		if (civiliansSaved && !civiliansDead && _missionStatistics->success == true)
		{
			_missionStatistics->valiantCrux = true;
		}

	std::ostringstream ss3;
	ss3 << total;
	_lstTotal->addRow(2, tr("STR_TOTAL_UC").c_str(), ss3.str().c_str());

	// add the points to our activity score
	if (_region)
	{
		_region->addActivityXcom(total);
	}
	if (_country)
	{
		_country->addActivityXcom(total);
	}

	// Resize (if needed)
	if (statsY > 80) statsY = 80;
	if (recoveryY > 80) recoveryY = 80;
	if (statsY + recoveryY > 120)
	{
		recoveryY = 120 - statsY;
		if (recoveryY < 80) _lstRecovery->setHeight(recoveryY);
		if (recoveryY > 80) recoveryY = 80;
	}

	// Reposition to fit the screen
	if (recoveryY > 0)
	{
		if (_txtRecovery->getText().empty())
		{
			_txtRecovery->setText(tr("STR_BOUNTY"));
		}
		_txtRecovery->setY(_lstStats->getY() + statsY + 5);
		_lstRecovery->setY(_txtRecovery->getY() + 8);
		_lstTotal->setY(_lstRecovery->getY() + recoveryY + 5);
	}
	else
	{
		_txtRecovery->setText("");
		_lstTotal->setY(_lstStats->getY() + statsY + 5);
	}

	// Calculate rating
	std::string rating;
	if (total <= -200)
	{
		rating = "STR_RATING_TERRIBLE";
	}
	else if (total <= 0)
	{
		rating = "STR_RATING_POOR";
	}
	else if (total <= 200)
	{
		rating = "STR_RATING_OK";
	}
	else if (total <= 500)
	{
		rating = "STR_RATING_GOOD";
	}
	else
	{
		rating = "STR_RATING_EXCELLENT";
	}

	if (!_game->getMod()->getMissionRatings()->empty())
	{
		rating = "";
		int temp = INT_MIN;
		const std::map<int, std::string> *missionRatings = _game->getMod()->getMissionRatings();
		for (std::map<int, std::string>::const_iterator i = missionRatings->begin(); i != missionRatings->end(); ++i)
		{
			if (i->first > temp && i->first <= total)
			{
				temp = i->first;
				rating = i->second;
			}
		}
	}

	_missionStatistics->rating = rating;
	_missionStatistics->score = total;
	_txtRating->setText(tr("STR_RATING").arg(tr(rating)));

	SavedGame *save = _game->getSavedGame();
	SavedBattleGame *battle = save->getSavedBattle();

	_missionStatistics->daylight = save->getSavedBattle()->getGlobalShade();
	_missionStatistics->id = _game->getSavedGame()->getMissionStatistics()->size();
	_game->getSavedGame()->getMissionStatistics()->push_back(_missionStatistics);

	// Award Best-of commendations.
	int bestScoreID[7] = {0, 0, 0, 0, 0, 0, 0};
	int bestScore[7] = {0, 0, 0, 0, 0, 0, 0};
	int bestOverallScorersID = 0;
	int bestOverallScore = 0;

	// Check to see if any of the dead soldiers were exceptional.
	for (std::vector<BattleUnit*>::iterator deadUnit = battle->getUnits()->begin(); deadUnit != battle->getUnits()->end(); ++deadUnit)
	{
		if (!(*deadUnit)->getGeoscapeSoldier() || (*deadUnit)->getStatus() != STATUS_DEAD)
		{
			continue;
		}

		/// Post-mortem kill award
		int killTurn = -1;
		for (std::vector<BattleUnit*>::iterator killerUnit = battle->getUnits()->begin(); killerUnit != battle->getUnits()->end(); ++killerUnit)
		{
			for(std::vector<BattleUnitKills*>::iterator kill = (*killerUnit)->getStatistics()->kills.begin(); kill != (*killerUnit)->getStatistics()->kills.end(); ++kill)
			{
				if ((*kill)->id == (*deadUnit)->getId())
				{
					killTurn = (*kill)->turn;
					break;
				}
			}
			if (killTurn != -1)
			{
				break;
			}
		}
		int postMortemKills = 0;
		if (killTurn != -1)
		{
			for(std::vector<BattleUnitKills*>::iterator deadUnitKill = (*deadUnit)->getStatistics()->kills.begin(); deadUnitKill != (*deadUnit)->getStatistics()->kills.end(); ++deadUnitKill)
			{
				if ((*deadUnitKill)->turn > killTurn && (*deadUnitKill)->faction == FACTION_HOSTILE || (*deadUnitKill)->faction == FACTION_ALIEN_PLAYER)
				{
					postMortemKills++;
				}
			}
		}
		(*deadUnit)->getGeoscapeSoldier()->getDiary()->awardPostMortemKill(postMortemKills);

		SoldierRank rank = (*deadUnit)->getGeoscapeSoldier()->getRank();
		// Rookies don't get this next award. No one likes them.
		if (rank == RANK_ROOKIE)
		{
			continue;
		}

		/// Best-of awards
		// Find the best soldier per rank by comparing score.
		for (std::vector<Soldier*>::iterator j = _game->getSavedGame()->getDeadSoldiers()->begin(); j != _game->getSavedGame()->getDeadSoldiers()->end(); ++j)
		{
			int score = (*j)->getDiary()->getScoreTotal(_game->getSavedGame()->getMissionStatistics());

			// Don't forget this mission's score!
			if ((*j)->getId() == (*deadUnit)->getId())
			{
				score += _missionStatistics->score;
			}

			if (score > bestScore[rank])
			{
				bestScoreID[rank] = (*deadUnit)->getId();
				bestScore[rank] = score;
				if (score > bestOverallScore)
				{
					bestOverallScorersID = (*deadUnit)->getId();
					bestOverallScore = score;
				}
			}
		}
	}
	// Now award those soldiers commendations!
	for (std::vector<BattleUnit*>::iterator deadUnit = battle->getUnits()->begin(); deadUnit != battle->getUnits()->end(); ++deadUnit)
	{
		if (!(*deadUnit)->getGeoscapeSoldier() || (*deadUnit)->getStatus() != STATUS_DEAD)
		{
			continue;
		}
		if ((*deadUnit)->getId() == bestScoreID[(*deadUnit)->getGeoscapeSoldier()->getRank()])
		{
			(*deadUnit)->getGeoscapeSoldier()->getDiary()->awardBestOfRank(bestScore[(*deadUnit)->getGeoscapeSoldier()->getRank()]);
		}
		if ((*deadUnit)->getId() == bestOverallScorersID)
		{
			(*deadUnit)->getGeoscapeSoldier()->getDiary()->awardBestOverall(bestOverallScore);
		}
	}

	for (std::vector<BattleUnit*>::iterator j = battle->getUnits()->begin(); j != battle->getUnits()->end(); ++j)
	{
		if ((*j)->getGeoscapeSoldier())
		{
			int soldierAlienKills = 0;
			int soldierAlienStuns = 0;
			for (std::vector<BattleUnitKills*>::const_iterator k = (*j)->getStatistics()->kills.begin(); k != (*j)->getStatistics()->kills.end(); ++k)
			{
				if (((*k)->faction == FACTION_HOSTILE || (*k)->faction == FACTION_ALIEN_PLAYER) && (*k)->status == STATUS_DEAD)
				{
					soldierAlienKills++;
				}
				if (((*k)->faction == FACTION_HOSTILE || (*k)->faction == FACTION_ALIEN_PLAYER) && (*k)->status == STATUS_UNCONSCIOUS)
				{
					soldierAlienStuns++;
				}
			}
			(*j)->getGeoscapeSoldier()->addStunCount(soldierAlienStuns);

			if (aliensKilled != 0 && aliensKilled == soldierAlienKills && _missionStatistics->success == true && aliensStunned == soldierAlienStuns)
			{
				(*j)->getStatistics()->nikeCross = true;
			}
			if (aliensStunned != 0 && aliensStunned == soldierAlienStuns && _missionStatistics->success == true && aliensKilled == 0)
			{
				(*j)->getStatistics()->mercyCross = true;
			}
			int daysWoundedTmp = (*j)->getGeoscapeSoldier()->getWoundRecovery(0.0f, 0.0f);
			(*j)->getStatistics()->daysWounded = daysWoundedTmp;
			if (daysWoundedTmp != 0)
			{
				_missionStatistics->injuryList[(*j)->getGeoscapeSoldier()->getId()] = daysWoundedTmp;
			}

			// Award Martyr Medal
			if ((*j)->getMurdererId() == (*j)->getId() && (*j)->getStatistics()->kills.size() != 0)
			{
				int martyrKills = 0; // How many aliens were killed on the same turn?
				int martyrTurn = -1;
				for (std::vector<BattleUnitKills*>::iterator unitKill = (*j)->getStatistics()->kills.begin(); unitKill != (*j)->getStatistics()->kills.end(); ++unitKill)
				{
					if ( (*unitKill)->id == (*j)->getId() )
					{
						martyrTurn = (*unitKill)->turn;
						break;
					}
				}
				for (std::vector<BattleUnitKills*>::iterator unitKill = (*j)->getStatistics()->kills.begin(); unitKill != (*j)->getStatistics()->kills.end(); ++unitKill)
				{
					if ((*unitKill)->turn == martyrTurn && (((*unitKill)->faction == FACTION_HOSTILE) || (*unitKill)->faction == FACTION_ALIEN_PLAYER))
					{
						martyrKills++;
					}
				}
				if (martyrKills > 0)
				{
					if (martyrKills > 10)
					{
						martyrKills = 10;
					}
					(*j)->getStatistics()->martyr = martyrKills;
				}
			}

			// Set the UnitStats delta
			(*j)->getStatistics()->delta = *(*j)->getGeoscapeSoldier()->getCurrentStats() - *(*j)->getGeoscapeSoldier()->getInitStats();

			(*j)->getGeoscapeSoldier()->getDiary()->updateDiary((*j)->getStatistics(), _game->getSavedGame()->getMissionStatistics(), _game->getMod());
			if (!(*j)->getStatistics()->MIA && !(*j)->getStatistics()->KIA && (*j)->getGeoscapeSoldier()->getDiary()->manageCommendations(_game->getMod(), _game->getSavedGame()->getMissionStatistics()))
			{
				_soldiersCommended.push_back((*j)->getGeoscapeSoldier());
			}
			else if ((*j)->getStatistics()->MIA || (*j)->getStatistics()->KIA)
			{
				(*j)->getGeoscapeSoldier()->getDiary()->manageCommendations(_game->getMod(), _game->getSavedGame()->getMissionStatistics());
				_deadSoldiersCommended.push_back((*j)->getGeoscapeSoldier());
			}
		}
	}

	_positiveScore = (total > 0);

	std::vector<Soldier*> participants;
	for (std::vector<BattleUnit*>::const_iterator i = _game->getSavedGame()->getSavedBattle()->getUnits()->begin();
		i != _game->getSavedGame()->getSavedBattle()->getUnits()->end(); ++i)
	{
		if ((*i)->getGeoscapeSoldier())
		{
			if (Options::fieldPromotions && !(*i)->hasGainedAnyExperience())
			{
				// Note: difference from OXC, soldier needs to actually have done something during the mission
				continue;
			}
			participants.push_back((*i)->getGeoscapeSoldier());
		}
	}
	_promotions = _game->getSavedGame()->handlePromotions(participants, _game->getMod());

	_game->getSavedGame()->setBattleGame(0);

	if (_positiveScore)
	{
		_game->getMod()->playMusic(Mod::DEBRIEF_MUSIC_GOOD);
	}
	else
	{
		_game->getMod()->playMusic(Mod::DEBRIEF_MUSIC_BAD);
	}
}

/**
* Shows a tooltip for the appropriate text.
* @param action Pointer to an action.
*/
void DebriefingState::txtTooltipIn(Action *action)
{
	_currentTooltip = action->getSender()->getTooltip();
	_txtTooltip->setText(tr(_currentTooltip));
}

/**
* Clears the tooltip text.
* @param action Pointer to an action.
*/
void DebriefingState::txtTooltipOut(Action *action)
{
	if (_currentTooltip == action->getSender()->getTooltip())
	{
		_txtTooltip->setText("");
	}
}

/**
 * Displays soldiers' stat increases.
 * @param action Pointer to an action.
 */
void DebriefingState::btnStatsClick(Action *)
{
	_pageNumber = (_pageNumber + 1) % 3;
	applyVisibility();
}

/**
* Opens the Sell/Sack UI (for recovered items ONLY).
* @param action Pointer to an action.
*/
void DebriefingState::btnSellClick(Action *)
{
	if (!_destroyBase)
	{
		_game->pushState(new SellState(_base, this, OPT_BATTLESCAPE));
	}
}

/**
 * Opens the Transfer UI (for recovered items ONLY).
 * @param action Pointer to an action.
 */
void DebriefingState::btnTransferClick(Action *)
{
	if (!_destroyBase)
	{
		_game->pushState(new TransferBaseState(_base, this));
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void DebriefingState::btnOkClick(Action *)
{
	_game->popState();
	if (_game->getSavedGame()->getMonthsPassed() == -1)
	{
		_game->setState(new MainMenuState);
	}
	else
	{
		if (_eventToSpawn)
		{
			bool canSpawn = _game->getSavedGame()->canSpawnInstantEvent(_eventToSpawn);
			if (canSpawn)
			{
				_game->pushState(new GeoscapeEventState(*_eventToSpawn));
			}
		}
		if (!_deadSoldiersCommended.empty())
		{
			_game->pushState(new CommendationLateState(_deadSoldiersCommended));
		}
		if (!_soldiersCommended.empty())
		{
			_game->pushState(new CommendationState(_soldiersCommended));
		}
		if (!_destroyBase)
		{
			if (_promotions)
			{
				_game->pushState(new PromotionsState);
			}
			if (!_missingItems.empty())
			{
				_game->pushState(new CannotReequipState(_missingItems, _base));
			}

			// refresh! (we may have sold some prisoners in the meantime; directly from Debriefing)
			for (std::map<int, int>::iterator i = _containmentStateInfo.begin(); i != _containmentStateInfo.end(); ++i)
			{
				if (i->second == 2)
				{
					int availableContainment = _base->getAvailableContainment(i->first);
					int usedContainment = _base->getUsedContainment(i->first);
					int freeContainment = availableContainment - (usedContainment * _limitsEnforced);
					if (availableContainment > 0 && freeContainment >= 0)
					{
						_containmentStateInfo[i->first] = 0; // 0 = OK
					}
					else if (usedContainment == 0)
					{
						_containmentStateInfo[i->first] = 0; // 0 = OK
					}
				}
			}

			for (std::map<int, int>::const_iterator i = _containmentStateInfo.begin(); i != _containmentStateInfo.end(); ++i)
			{
				if (i->second == 2)
				{
					_game->pushState(new ManageAlienContainmentState(_base, i->first, OPT_BATTLESCAPE));
					_game->pushState(new ErrorMessageState(trAlt("STR_CONTAINMENT_EXCEEDED", i->first).arg(_base->getName()), _palette, _game->getMod()->getInterface("debriefing")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("debriefing")->getElement("errorPalette")->color));
				}
				else if (i->second == 1)
				{
					_game->pushState(new ErrorMessageState(
						trAlt("STR_ALIEN_DIES_NO_ALIEN_CONTAINMENT_FACILITY", i->first),
						_palette,
						_game->getMod()->getInterface("debriefing")->getElement("errorMessage")->color,
						"BACK01.SCR",
						_game->getMod()->getInterface("debriefing")->getElement("errorPalette")->color));
				}
			}

			if (Options::storageLimitsEnforced && _base->storesOverfull())
			{
				_game->pushState(new SellState(_base, 0, OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(tr("STR_STORAGE_EXCEEDED").arg(_base->getName()), _palette, _game->getMod()->getInterface("debriefing")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("debriefing")->getElement("errorPalette")->color));
			}
		}

		// Autosave after mission
		if (_game->getSavedGame()->isIronman())
		{
			_game->pushState(new SaveGameState(OPT_GEOSCAPE, SAVE_IRONMAN, _palette));
		}
		else if (Options::autosave)
		{
			_game->pushState(new SaveGameState(OPT_GEOSCAPE, SAVE_AUTO_GEOSCAPE, _palette));
		}
	}
}

/**
 * Adds to the debriefing stats.
 * @param name The untranslated name of the stat.
 * @param quantity The quantity to add.
 * @param score The score to add.
 */
void DebriefingState::addStat(const std::string &name, int quantity, int score)
{
	for (std::vector<DebriefingStat*>::iterator i = _stats.begin(); i != _stats.end(); ++i)
	{
		if ((*i)->item == name)
		{
			(*i)->qty = (*i)->qty + quantity;
			(*i)->score = (*i)->score + score;
			break;
		}
	}
}

/**
 * Prepares debriefing: gathers Aliens, Corpses, Artefacts, UFO Components.
 * Adds the items to the craft.
 * Also calculates the soldiers experience, and possible promotions.
 * If aborted, only the things on the exit area are recovered.
 */
void DebriefingState::prepareDebriefing()
{
	for (std::vector<std::string>::const_iterator i = _game->getMod()->getItemsList().begin(); i != _game->getMod()->getItemsList().end(); ++i)
	{
		RuleItem *rule = _game->getMod()->getItem(*i);
		if (rule->getSpecialType() > 1 && rule->getSpecialType() < DEATH_TRAPS)
		{
			RecoveryItem *item = new RecoveryItem();
			item->name = *i;
			item->value = rule->getRecoveryPoints();
			_recoveryStats[rule->getSpecialType()] = item;
			_missionStatistics->lootValue = item->value;
		}
	}

	SavedGame *save = _game->getSavedGame();
	SavedBattleGame *battle = save->getSavedBattle();

	AlienDeployment *ruleDeploy = _game->getMod()->getDeployment(battle->getMissionType());
	// OXCE: Don't forget custom mission overrides
	auto alienCustomMission = _game->getMod()->getDeployment(battle->getAlienCustomMission());
	if (alienCustomMission)
	{
		ruleDeploy = alienCustomMission;
	}
	// OXCE: Don't forget about UFO landings/crash sites
	if (!ruleDeploy)
	{
		for (std::vector<Ufo*>::iterator ufo = _game->getSavedGame()->getUfos()->begin(); ufo != _game->getSavedGame()->getUfos()->end(); ++ufo)
		{
			if ((*ufo)->isInBattlescape())
			{
				// Note: fake underwater UFO deployment was already considered above (via alienCustomMission)
				ruleDeploy = _game->getMod()->getDeployment((*ufo)->getRules()->getType());
				break;
			}
		}
	}

	bool aborted = battle->isAborted();
	bool success = !aborted || battle->allObjectivesDestroyed();
	Craft *craft = 0;
	Base *base = 0;
	std::string target;

	int playersInExitArea = 0; // if this stays 0 the craft is lost...
	int playersSurvived = 0; // if this stays 0 the craft is lost...
	int playersUnconscious = 0;
	int playersInEntryArea = 0;
	int playersMIA = 0;

	_stats.push_back(new DebriefingStat("STR_ALIENS_KILLED", false));
	_stats.push_back(new DebriefingStat("STR_ALIEN_CORPSES_RECOVERED", false));
	_stats.push_back(new DebriefingStat("STR_LIVE_ALIENS_RECOVERED", false));
	_stats.push_back(new DebriefingStat("STR_LIVE_ALIENS_SURRENDERED", false));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ARTIFACTS_RECOVERED", false));

	std::string missionCompleteText, missionFailedText;
	std::string objectiveCompleteText, objectiveFailedText;
	int objectiveCompleteScore = 0, objectiveFailedScore = 0;
	if (ruleDeploy)
	{
		if (ruleDeploy->getObjectiveCompleteInfo(objectiveCompleteText, objectiveCompleteScore, missionCompleteText))
		{
			_stats.push_back(new DebriefingStat(objectiveCompleteText, false));
		}
		if (ruleDeploy->getObjectiveFailedInfo(objectiveFailedText, objectiveFailedScore, missionFailedText))
		{
			_stats.push_back(new DebriefingStat(objectiveFailedText, false));
		}
		if (aborted && ruleDeploy->getAbortPenalty() != 0)
		{
			_stats.push_back(new DebriefingStat("STR_MISSION_ABORTED", false));
			addStat("STR_MISSION_ABORTED", 1, -ruleDeploy->getAbortPenalty());
		}
	}
	if (battle->getVIPSurvivalPercentage() > 0)
	{
		_stats.push_back(new DebriefingStat("STR_VIPS_LOST", false));
		_stats.push_back(new DebriefingStat("STR_VIPS_SAVED", false));
	}

	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_ALIENS", false));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES", false));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_SAVED", false));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_KILLED", false));
	//_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_RETIRED_THROUGH_INJURY", false));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_MISSING_IN_ACTION", false));
	_stats.push_back(new DebriefingStat("STR_TANKS_DESTROYED", false));
	_stats.push_back(new DebriefingStat("STR_XCOM_CRAFT_LOST", false));

	for (std::map<int, RecoveryItem*>::const_iterator i = _recoveryStats.begin(); i != _recoveryStats.end(); ++i)
	{
		_stats.push_back(new DebriefingStat((*i).second->name, true));
	}

	_missionStatistics->time = *save->getTime();
	_missionStatistics->type = battle->getMissionType();
	_stats.push_back(new DebriefingStat(_game->getMod()->getAlienFuelName(), true));

	for (std::vector<Base*>::iterator i = save->getBases()->begin(); i != save->getBases()->end(); ++i)
	{
		// in case we have a craft - check which craft it is about
		for (std::vector<Craft*>::iterator j = (*i)->getCrafts()->begin(); j != (*i)->getCrafts()->end(); ++j)
		{
			if ((*j)->isInBattlescape())
			{
				for (std::vector<Region*>::iterator k = _game->getSavedGame()->getRegions()->begin(); k != _game->getSavedGame()->getRegions()->end(); ++k)
				{
					if ((*k)->getRules()->insideRegion((*j)->getLongitude(), (*j)->getLatitude()))
					{
						_region = (*k);
						_missionStatistics->region = _region->getRules()->getType();
						break;
					}
				}
				for (std::vector<Country*>::iterator k = _game->getSavedGame()->getCountries()->begin(); k != _game->getSavedGame()->getCountries()->end(); ++k)
				{
					if ((*k)->getRules()->insideCountry((*j)->getLongitude(), (*j)->getLatitude()))
					{
						_country = (*k);
						_missionStatistics->country = _country->getRules()->getType();
						break;
					}
				}
				craft = (*j);
				base = (*i);
				if (craft->getDestination() != 0)
				{
					_missionStatistics->markerName = craft->getDestination()->getMarkerName();
					_missionStatistics->markerId = craft->getDestination()->getMarkerId();
					target = craft->getDestination()->getType();
					// Ignore custom mission names
					if (dynamic_cast<AlienBase*>(craft->getDestination()))
					{
						target = "STR_ALIEN_BASE";
					}
					else if (dynamic_cast<MissionSite*>(craft->getDestination()))
					{
						target = "STR_MISSION_SITE";
					}
				}
				craft->returnToBase();
				craft->setMissionComplete(true);
				craft->setInBattlescape(false);
			}
			else if ((*j)->getDestination() != 0)
			{
				Ufo* u = dynamic_cast<Ufo*>((*j)->getDestination());
				if (u != 0 && u->isInBattlescape())
				{
					(*j)->returnToBase();
				}
				MissionSite* ms = dynamic_cast<MissionSite*>((*j)->getDestination());
				if (ms != 0 && ms->isInBattlescape())
				{
					(*j)->returnToBase();
				}
			}
		}
		// in case we DON'T have a craft (base defense)
		if ((*i)->isInBattlescape())
		{
			base = (*i);
			target = base->getType();
			base->setInBattlescape(false);
			base->cleanupDefenses(false);
			for (std::vector<Region*>::iterator k = _game->getSavedGame()->getRegions()->begin(); k != _game->getSavedGame()->getRegions()->end(); ++k)
			{
				if ((*k)->getRules()->insideRegion(base->getLongitude(), base->getLatitude()))
				{
					_region = (*k);
					_missionStatistics->region = _region->getRules()->getType();
					break;
				}
			}
			for (std::vector<Country*>::iterator k = _game->getSavedGame()->getCountries()->begin(); k != _game->getSavedGame()->getCountries()->end(); ++k)
			{
				if ((*k)->getRules()->insideCountry(base->getLongitude(), base->getLatitude()))
				{
					_country = (*k);
					_missionStatistics->country= _country->getRules()->getType();
					break;
				}
			}
			// Loop through the UFOs and see which one is sitting on top of the base... that is probably the one attacking you.
			for (std::vector<Ufo*>::iterator k = save->getUfos()->begin(); k != save->getUfos()->end(); ++k)
			{
				if (AreSame((*k)->getLongitude(), base->getLongitude()) && AreSame((*k)->getLatitude(), base->getLatitude()))
				{
					_missionStatistics->ufo = (*k)->getRules()->getType(); // no need to check for fake underwater UFOs here
					_missionStatistics->alienRace = (*k)->getAlienRace();
					break;
				}
			}
			if (aborted)
			{
				_destroyBase = true;
			}

			// This is an overkill, since we may not lose any hangar/craft, but doing it properly requires tons of changes
			_game->getSavedGame()->stopHuntingXcomCrafts(base);

			std::vector<BaseFacility*> toBeDamaged;
			for (std::vector<BaseFacility*>::iterator k = base->getFacilities()->begin(); k != base->getFacilities()->end(); ++k)
			{
				// this facility was demolished
				if (battle->getModuleMap()[(*k)->getX()][(*k)->getY()].second == 0)
				{
					toBeDamaged.push_back((*k));
				}
			}
			for (auto fac : toBeDamaged)
			{
				base->damageFacility(fac);
			}
			// this may cause the base to become disjointed, destroy the disconnected parts.
			base->destroyDisconnectedFacilities();
		}
	}

	if (!base && _game->getSavedGame()->isIronman())
	{
		throw Exception("Your save is corrupted. Try asking someone on the Openxcom forum/discord to fix it for you.");
	}

	// mission site disappears (even when you abort)
	for (std::vector<MissionSite*>::iterator i = save->getMissionSites()->begin(); i != save->getMissionSites()->end(); ++i)
	{
		if ((*i)->isInBattlescape())
		{
			_missionStatistics->alienRace = (*i)->getAlienRace();
			delete *i;
			save->getMissionSites()->erase(i);
			break;
		}
	}

	// lets see what happens with units

	// manual update state of all units
	for (auto unit : *battle->getUnits())
	{
		// scripts (or some bugs in the game) could make aliens or soldiers that have "unresolved" stun or death state.
		// Note: resolves the "last bleeding alien" too
		if (!unit->isOut() && unit->isOutThresholdExceed())
		{
			unit->instaFalling();
			if (unit->getTile())
			{
				battle->getTileEngine()->itemDropInventory(unit->getTile(), unit);
			}

			//spawn corpse/body for unit to recover
			for (int i = unit->getArmor()->getTotalSize() - 1; i >= 0; --i)
			{
				auto corpse = battle->createItemForTile(unit->getArmor()->getCorpseBattlescape()[i], nullptr);
				corpse->setUnit(unit);
				battle->getTileEngine()->itemDrop(unit->getTile(), corpse, false);
			}
		}
	}

	// first, we evaluate how many surviving XCom units there are, and how many are conscious
	// and how many have died (to use for commendations)
	int deadSoldiers = 0;
	for (std::vector<BattleUnit*>::iterator j = battle->getUnits()->begin(); j != battle->getUnits()->end(); ++j)
	{
		if ((*j)->getOriginalFaction() == FACTION_PLAYER && (*j)->getStatus() != STATUS_DEAD)
		{
			if ((*j)->getStatus() == STATUS_UNCONSCIOUS || (*j)->getFaction() == FACTION_HOSTILE || (*j)->getFaction() == FACTION_ALIEN_PLAYER)
			{
				playersUnconscious++;
			}
			else if ((*j)->isIgnored() && (*j)->getStunlevel() >= (*j)->getHealth())
			{
				// even for ignored xcom units, we need to know if they're conscious or unconscious
				playersUnconscious++;
			}
			else if ((*j)->isInExitArea(END_POINT))
			{
				playersInExitArea++;
			}
			else if ((*j)->isInExitArea(START_POINT))
			{
				playersInEntryArea++;
			}
			else if (aborted)
			{
				// if aborted, conscious xcom unit that is not on start/end point counts as MIA
				playersMIA++;
			}
			playersSurvived++;
		}
		else if ((*j)->getOriginalFaction() == FACTION_PLAYER && (*j)->getStatus() == STATUS_DEAD)
			deadSoldiers++;
	}
	// if all our men are unconscious, the aliens get to have their way with them.
	if (playersUnconscious + playersMIA == playersSurvived)
	{
		playersSurvived = playersMIA;
		for (std::vector<BattleUnit*>::iterator j = battle->getUnits()->begin(); j != battle->getUnits()->end(); ++j)
		{
			if ((*j)->getOriginalFaction() == FACTION_PLAYER && (*j)->getStatus() != STATUS_DEAD)
			{
				if ((*j)->getStatus() == STATUS_UNCONSCIOUS || (*j)->getFaction() == FACTION_HOSTILE || (*j)->getFaction() == FACTION_ALIEN_PLAYER)
				{
					(*j)->instaKill();
				}
				else if ((*j)->isIgnored() && (*j)->getStunlevel() >= (*j)->getHealth())
				{
					(*j)->instaKill();
				}
				else
				{
					// do nothing, units will be marked MIA later
				}
			}
		}
	}

	// if it's a UFO, let's see what happens to it
	for (std::vector<Ufo*>::iterator i = save->getUfos()->begin(); i != save->getUfos()->end(); ++i)
	{
		if ((*i)->isInBattlescape())
		{
			_missionStatistics->ufo = (*i)->getRules()->getType(); // no need to check for fake underwater UFOs here
			if (save->getMonthsPassed() != -1)
			{
				_missionStatistics->alienRace = (*i)->getAlienRace();
			}
			_txtRecovery->setText(tr("STR_UFO_RECOVERY"));
			(*i)->setInBattlescape(false);
			// if XCom failed to secure the landing zone, the UFO
			// takes off immediately and proceeds according to its mission directive
			if ((*i)->getStatus() == Ufo::LANDED && (aborted || playersSurvived == 0))
			{
				 (*i)->setSecondsRemaining(5);
			}
			// if XCom succeeds, or it's a crash site, the UFO disappears
			else
			{
				// Note: just before removing a landed UFO, check for mission interruption (by setting the UFO damage to max)
				if (save->getMonthsPassed() > -1)
				{
					if ((*i)->getStatus() == Ufo::LANDED)
					{
						(*i)->setDamage((*i)->getCraftStats().damageMax, _game->getMod());
					}
				}
				delete *i;
				save->getUfos()->erase(i);
			}
			break;
		}
	}

	if (ruleDeploy && ruleDeploy->getEscapeType() != ESCAPE_NONE)
	{
		if (ruleDeploy->getEscapeType() != ESCAPE_EXIT)
		{
			success = playersInEntryArea > 0;
		}

		if (ruleDeploy->getEscapeType() != ESCAPE_ENTRY)
		{
			success = success || playersInExitArea > 0;
		}
	}

	playersInExitArea = 0;

	if (playersSurvived == 1)
	{
		for (std::vector<BattleUnit*>::iterator j = battle->getUnits()->begin(); j != battle->getUnits()->end(); ++j)
		{
			// if only one soldier survived, give him a medal! (unless he killed all the others...)
			if ((*j)->getStatus() != STATUS_DEAD && (*j)->getOriginalFaction() == FACTION_PLAYER && !(*j)->getStatistics()->hasFriendlyFired() && deadSoldiers != 0)
			{
				(*j)->getStatistics()->loneSurvivor = true;
				break;
			}
			// if only one soldier survived AND none have died, means only one soldier went on the mission...
			if ((*j)->getStatus() != STATUS_DEAD && (*j)->getOriginalFaction() == FACTION_PLAYER && deadSoldiers == 0)
			{
				(*j)->getStatistics()->ironMan = true;
			}
		}
	}
	// alien base disappears (if you didn't abort)
	for (std::vector<AlienBase*>::iterator i = save->getAlienBases()->begin(); i != save->getAlienBases()->end(); ++i)
	{
		if ((*i)->isInBattlescape())
		{
			_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));
			bool destroyAlienBase = true;

			if (aborted || playersSurvived == 0)
			{
				if (!battle->allObjectivesDestroyed())
					destroyAlienBase = false;
			}

			if (ruleDeploy && !ruleDeploy->getNextStage().empty())
			{
				_missionStatistics->alienRace = (*i)->getAlienRace();
				destroyAlienBase = false;
			}

			success = destroyAlienBase;
			if (destroyAlienBase)
			{
				if (!objectiveCompleteText.empty())
				{
					addStat(objectiveCompleteText, 1, objectiveCompleteScore);
				}
				save->clearLinksForAlienBase((*i), _game->getMod());
				delete *i;
				save->getAlienBases()->erase(i);
				break;
			}
			else
			{
				(*i)->setInBattlescape(false);
				break;
			}
		}
	}

	// transform all zombie-like units to spawned ones
	std::vector<BattleUnit*> waitingTransformations;
	for (auto* u : *battle->getUnits())
	{
		if (u->getSpawnUnit() && (!u->isOut() || u->isIgnored()))
		{
			if (u->getOriginalFaction() == FACTION_HOSTILE || u->getOriginalFaction() == FACTION_ALIEN_PLAYER)
			{
				waitingTransformations.push_back(u);
			}
			else
			{
				//if unit belong to XCOM or CIVILIANS we leave it as-is
			}
		}
	}
	for (auto* u : waitingTransformations)
	{
		auto ignore = u->isIgnored();
		auto faction = u->getFaction();
		// convert it, and mind control the resulting unit.
		// reason: zombies don't create unconscious bodies... ever.
		// the only way we can get into this situation is if psi-capture is enabled.
		// we can use that knowledge to our advantage to save having to make it unconscious and spawn a body item for it.
		if (ignore)
		{
			//simplified handling for unit from previous stage
			BattleUnit *newUnit = battle->createTempUnit(u->getSpawnUnit(), u->getSpawnUnitFaction());
			battle->getUnits()->push_back(newUnit);
			newUnit->convertToFaction(faction);
		}
		else
		{
			BattleUnit *newUnit = battle->convertUnit(u);
			newUnit->convertToFaction(faction);
		}
		// original line
		//u->killedBy(FACTION_HOSTILE); //skip counting as kill
		u->killedBy(FACTION_ALIEN_PLAYER); //skip counting as kill
	}

	// time to care for units.
	bool psiStrengthEval = (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()));
	for (std::vector<BattleUnit*>::iterator j = battle->getUnits()->begin(); j != battle->getUnits()->end(); ++j)
	{
		UnitStatus status = (*j)->getStatus();
		UnitFaction faction = (*j)->getFaction();
		UnitFaction oldFaction = (*j)->getOriginalFaction();
		int value = (*j)->getValue();
		Soldier *soldier = save->getSoldier((*j)->getId());

		if (!(*j)->getTile())
		{
			Position pos = (*j)->getPosition();
			if (pos == TileEngine::invalid)
			{
				for (std::vector<BattleItem*>::iterator k = battle->getItems()->begin(); k != battle->getItems()->end(); ++k)
				{
					if ((*k)->getUnit() && (*k)->getUnit() == *j)
					{
						if ((*k)->getOwner())
						{
							pos = (*k)->getOwner()->getPosition();
						}
						else if ((*k)->getTile())
						{
							pos = (*k)->getTile()->getPosition();
						}
					}
				}
			}
			(*j)->setInventoryTile(battle->getTile(pos));
		}

		if (status == STATUS_DEAD)
		{ // so this is a dead unit
			if ((oldFaction == FACTION_HOSTILE || oldFaction == FACTION_ALIEN_PLAYER) && (*j)->killedBy() == FACTION_PLAYER)
			{
				addStat("STR_ALIENS_KILLED", 1, value);
			}
			else if (oldFaction == FACTION_PLAYER)
			{
				if (soldier != 0)
				{
					addStat("STR_XCOM_OPERATIVES_KILLED", 1, -value);
					(*j)->updateGeoscapeStats(soldier);

					// starting conditions: recover armor backup
					if (soldier->getReplacedArmor())
					{
						if (soldier->getReplacedArmor()->getStoreItem())
						{
							addItemsToBaseStores(soldier->getReplacedArmor()->getStoreItem()->getType(), base, 1, false);
						}
						soldier->setReplacedArmor(0);
					}
					// transformed armor doesn't get recovered
					soldier->setTransformedArmor(0);

					(*j)->getStatistics()->KIA = true;
					save->killSoldier(true, soldier); // in case we missed the soldier death on battlescape
				}
				else
				{ // non soldier player = tank
					addStat("STR_TANKS_DESTROYED", 1, -value);
				}
			}
			else if (oldFaction == FACTION_NEUTRAL)
			{
				if ((*j)->killedBy() == FACTION_PLAYER)
				{
					if (!(*j)->isCosmetic())
					{
						addStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES", 1, -(*j)->getValue() - (2 * ((*j)->getValue() / 3)));
					}
				}
				else // if civilians happen to kill themselves XCOM shouldn't get penalty for it
				{
					if (!(*j)->isCosmetic())
					{
						addStat("STR_CIVILIANS_KILLED_BY_ALIENS", 1, -(*j)->getValue());
					}
				}
			}
		}
		else
		{ // so this unit is not dead...
			if (oldFaction == FACTION_PLAYER)
			{
				if (
					(((*j)->isInExitArea(START_POINT) || (*j)->isIgnored()) && (battle->getMissionType() != "STR_BASE_DEFENSE" || success))
					|| !aborted
					|| (aborted && (*j)->isInExitArea(END_POINT)))
				{ // so game is not aborted or aborted and unit is on exit area
					StatAdjustment statIncrease;
					(*j)->postMissionProcedures(_game->getMod(), save, battle, statIncrease);
					if ((*j)->getGeoscapeSoldier())
						_soldierStats.push_back(std::pair<std::string, UnitStats>((*j)->getGeoscapeSoldier()->getName(), statIncrease.statGrowth));
					playersInExitArea++;

					recoverItems((*j)->getInventory(), base);

					if (soldier != 0)
					{
						// calculate new statString
						soldier->calcStatString(_game->getMod()->getStatStrings(), psiStrengthEval);
					}
					else
					{ // non soldier player = tank
						addItemsToBaseStores((*j)->getType(), base, 1, false);

						auto unloadWeapon = [&](BattleItem *weapon)
						{
							if (weapon)
							{
								const RuleItem *primaryRule = weapon->getRules();
								const BattleItem *ammoItem = weapon->getAmmoForSlot(0);
								const auto *compatible = primaryRule->getVehicleClipAmmo();
								if (primaryRule->getVehicleUnit() && compatible && ammoItem != 0 && ammoItem->getAmmoQuantity() > 0)
								{
									int total = ammoItem->getAmmoQuantity();

									if (primaryRule->getClipSize()) // meaning this tank can store multiple clips
									{
										total /= ammoItem->getRules()->getClipSize();
									}

									addItemsToBaseStores(compatible, base, total, false);
								}
							}
						};

						unloadWeapon((*j)->getRightHandWeapon());
						unloadWeapon((*j)->getLeftHandWeapon());
					}
				}
				else
				{ // so game is aborted and unit is not on exit area
					addStat("STR_XCOM_OPERATIVES_MISSING_IN_ACTION", 1, -value);
					playersSurvived--;
					if (soldier != 0)
					{
						(*j)->updateGeoscapeStats(soldier);

						// starting conditions: recover armor backup
						if (soldier->getReplacedArmor())
						{
							if (soldier->getReplacedArmor()->getStoreItem())
							{
								addItemsToBaseStores(soldier->getReplacedArmor()->getStoreItem()->getType(), base, 1, false);
							}
							soldier->setReplacedArmor(0);
						}
						// transformed armor doesn't get recovered
						soldier->setTransformedArmor(0);

						(*j)->getStatistics()->MIA = true;
						save->killSoldier(true, soldier);
					}
				}
			}
			else if ((oldFaction == FACTION_HOSTILE || oldFaction == FACTION_ALIEN_PLAYER) && (!aborted || (*j)->isInExitArea(START_POINT)) && !_destroyBase
				// mind controlled units may as well count as unconscious
				&& faction == FACTION_PLAYER && (!(*j)->isOut() || (*j)->isIgnored()))
			{
				if ((*j)->getTile())
				{
					battle->getTileEngine()->itemDropInventory((*j)->getTile(), (*j));
				}
				if (!(*j)->getArmor()->getCorpseBattlescape().empty())
				{
					auto corpseRule = (*j)->getArmor()->getCorpseBattlescape().front();
					if (corpseRule && corpseRule->isRecoverable())
					{
						recoverAlien(*j, base);
					}
				}
			}
			else if ((oldFaction == FACTION_HOSTILE || oldFaction == FACTION_ALIEN_PLAYER) && !aborted && !_destroyBase
				// surrendered units may as well count as unconscious too
				&& playersSurvived > 0 && faction != FACTION_PLAYER && (!(*j)->isOut() || (*j)->isIgnored())
				&& ((*j)->isSurrendering() || battle->getChronoTrigger() == FORCE_WIN_SURRENDER))
			{
				if ((*j)->getTile())
				{
					battle->getTileEngine()->itemDropInventory((*j)->getTile(), (*j));
				}
				if (!(*j)->getArmor()->getCorpseBattlescape().empty())
				{
					auto corpseRule = (*j)->getArmor()->getCorpseBattlescape().front();
					if (corpseRule && corpseRule->isRecoverable())
					{
						recoverAlien(*j, base);
					}
				}
			}
			else if (oldFaction == FACTION_NEUTRAL)
			{
				// if mission fails, all civilians die
				if ((aborted && !success) || playersSurvived == 0)
				{
					if (!(*j)->isResummonedFakeCivilian() && !(*j)->isCosmetic())
					{
						addStat("STR_CIVILIANS_KILLED_BY_ALIENS", 1, -(*j)->getValue());
					}
				}
				else
				{
					if (!(*j)->isResummonedFakeCivilian() && !(*j)->isCosmetic())
					{
						addStat("STR_CIVILIANS_SAVED", 1, (*j)->getValue());
					}
					recoverCivilian(*j, base);
				}
			}
		}
	}
	bool lostCraft = false;
	if (craft != 0 && ((playersInExitArea == 0 && aborted) || (playersSurvived == 0)))
	{
		if (craft->getRules()->keepCraftAfterFailedMission())
		{
			// craft was not even on the battlescape (e.g. paratroopers)
		}
		else if (ruleDeploy->keepCraftAfterFailedMission())
		{
			// craft didn't wait for you (e.g. escape/extraction missions)
		}
		else
		{
			addStat("STR_XCOM_CRAFT_LOST", 1, -craft->getRules()->getScore());
			// Since this is not a base defense mission, we can safely erase the craft,
			// without worrying it's vehicles' destructor calling double (on base defense missions
			// all vehicle object in the craft is also referenced by base->getVehicles() !!)
			_game->getSavedGame()->stopHuntingXcomCraft(craft); // lost during ground mission
			_game->getSavedGame()->removeAllSoldiersFromXcomCraft(craft); // needed in case some soldiers couldn't spawn
			base->removeCraft(craft, false);
			delete craft;
			craft = 0; // To avoid a crash down there!!
			lostCraft = true;
		}
		playersSurvived = 0; // assuming you aborted and left everyone behind
		success = false;
	}
	if ((aborted || playersSurvived == 0) && target == "STR_BASE")
	{
		for (std::vector<Craft*>::iterator i = base->getCrafts()->begin(); i != base->getCrafts()->end(); ++i)
		{
			addStat("STR_XCOM_CRAFT_LOST", 1, -(*i)->getRules()->getScore());
		}
		playersSurvived = 0; // assuming you aborted and left everyone behind
		success = false;
	}

	bool savedEnoughVIPs = true;
	if (battle->getVIPSurvivalPercentage() > 0)
	{
		bool retreated = aborted && (playersSurvived > 0);

		// 1. correct our initial assessment if necessary
		battle->correctVIPStats(success, retreated);
		int vipSubtotal = battle->getSavedVIPs() + battle->getLostVIPs();

		// 2. add non-fake civilian VIPs, no scoring
		for (auto unit : *battle->getUnits())
		{
			if (unit->isVIP() && unit->getOriginalFaction() == FACTION_NEUTRAL && !unit->isResummonedFakeCivilian())
			{
				if (unit->getStatus() == STATUS_DEAD)
					battle->addLostVIP(0);
				else if (success)
					battle->addSavedVIP(0);
				else
					battle->addLostVIP(0);
			}
		}

		// 3. check if we saved enough VIPs
		int vipTotal = battle->getSavedVIPs() + battle->getLostVIPs();
		if (vipTotal > 0)
		{
			int ratio = battle->getSavedVIPs() * 100 / vipTotal;
			if (ratio < battle->getVIPSurvivalPercentage())
			{
				savedEnoughVIPs = false; // didn't save enough VIPs
				success = false;
			}
		}
		else
		{
			savedEnoughVIPs = false; // nobody to save?
			success = false;
		}

		// 4. add stats
		if (vipSubtotal > 0 || (vipTotal > 0 && !savedEnoughVIPs))
		{
			addStat("STR_VIPS_LOST", battle->getLostVIPs(), battle->getLostVIPsScore());
			addStat("STR_VIPS_SAVED", battle->getSavedVIPs(), battle->getSavedVIPsScore());
		}
	}

	if ((!aborted || success) && playersSurvived > 0) 	// RECOVER UFO : run through all tiles to recover UFO components and items
	{
		if (target == "STR_BASE")
		{
			_txtTitle->setText(tr("STR_BASE_IS_SAVED"));
		}
		else if (target == "STR_UFO")
		{
			_txtTitle->setText(tr("STR_UFO_IS_RECOVERED"));
		}
		else if (target == "STR_ALIEN_BASE")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_DESTROYED"));
		}
		else
		{
			_txtTitle->setText(tr("STR_ALIENS_DEFEATED"));
			if (!aborted && !savedEnoughVIPs)
			{
				// Special case: mission was NOT aborted, all enemies were neutralized, but we couldn't save enough VIPs...
				if (!objectiveFailedText.empty())
				{
					addStat(objectiveFailedText, 1, objectiveFailedScore);
				}
			}
			else if (!objectiveCompleteText.empty())
			{
				int victoryStat = 0;
				if (ruleDeploy->getEscapeType() != ESCAPE_NONE)
				{
					if (ruleDeploy->getEscapeType() != ESCAPE_EXIT)
					{
						victoryStat += playersInEntryArea;
					}
					if (ruleDeploy->getEscapeType() != ESCAPE_ENTRY)
					{
						victoryStat += playersInExitArea;
					}
				}
				else
				{
					victoryStat = 1;
				}
				if (battle->getVIPSurvivalPercentage() > 0)
				{
					victoryStat = 1; // TODO: maybe show battle->getSavedVIPs() instead? need feedback...
				}

				addStat(objectiveCompleteText, victoryStat, objectiveCompleteScore);
			}
		}
		if (!aborted && !savedEnoughVIPs)
		{
			// Special case: mission was NOT aborted, all enemies were neutralized, but we couldn't save enough VIPs...
			if (!missionFailedText.empty())
			{
				_txtTitle->setText(tr(missionFailedText));
			}
			else
			{
				_txtTitle->setText(tr("STR_TERROR_CONTINUES"));
			}
		}
		else if (!missionCompleteText.empty())
		{
			_txtTitle->setText(tr(missionCompleteText));
		}

		if (!aborted)
		{
			// if this was a 2-stage mission, and we didn't abort (ie: we have time to clean up)
			// we can recover items from the earlier stages as well
			recoverItems(battle->getConditionalRecoveredItems(), base);
			size_t nonRecoverType = 0;
			if (ruleDeploy && ruleDeploy->getObjectiveType() && !ruleDeploy->allowObjectiveRecovery())
			{
				nonRecoverType = ruleDeploy->getObjectiveType();
			}
			for (int i = 0; i < battle->getMapSizeXYZ(); ++i)
			{
				// get recoverable map data objects from the battlescape map
				for (int part = O_FLOOR; part < O_MAX; ++part)
				{
					TilePart tp = (TilePart)part;
					if (battle->getTile(i)->getMapData(tp))
					{
						size_t specialType = battle->getTile(i)->getMapData(tp)->getSpecialType();
						if (specialType != nonRecoverType && specialType < (size_t)DEATH_TRAPS && _recoveryStats.find(specialType) != _recoveryStats.end())
						{
							addStat(_recoveryStats[specialType]->name, 1, _recoveryStats[specialType]->value);
						}
					}
				}
				// recover items from the floor
				recoverItems(battle->getTile(i)->getInventory(), base);
			}
		}
		else
		{
			for (int i = 0; i < battle->getMapSizeXYZ(); ++i)
			{
				if (battle->getTile(i)->getFloorSpecialTileType() == START_POINT)
					recoverItems(battle->getTile(i)->getInventory(), base);
			}
		}
	}
	else
	{
		if (lostCraft)
		{
			_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));
		}
		else if (target == "STR_BASE")
		{
			_txtTitle->setText(tr("STR_BASE_IS_LOST"));
			_destroyBase = true;
		}
		else if (target == "STR_UFO")
		{
			_txtTitle->setText(tr("STR_UFO_IS_NOT_RECOVERED"));
		}
		else if (target == "STR_ALIEN_BASE")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_STILL_INTACT"));
		}
		else
		{
			_txtTitle->setText(tr("STR_TERROR_CONTINUES"));
			if (!objectiveFailedText.empty())
			{
				addStat(objectiveFailedText, 1, objectiveFailedScore);
			}
		}
		if (!missionFailedText.empty())
		{
			_txtTitle->setText(tr(missionFailedText));
		}

		if (playersSurvived > 0 && !_destroyBase)
		{
			// recover items from the craft floor
			for (int i = 0; i < battle->getMapSizeXYZ(); ++i)
			{
				if (battle->getTile(i)->getMapData(O_FLOOR) && (battle->getTile(i)->getMapData(O_FLOOR)->getSpecialType() == START_POINT))
					recoverItems(battle->getTile(i)->getInventory(), base);
			}
		}
	}

	// recover all our goodies
	if (playersSurvived > 0)
	{
		bool alienAlloysExist = (_recoveryStats.find(ALIEN_ALLOYS) != _recoveryStats.end());
		for (std::vector<DebriefingStat*>::iterator i = _stats.begin(); i != _stats.end(); ++i)
		{
			// alien alloys recovery values are divided by 10 or divided by 150 in case of an alien base
			int aadivider = 1;
			if (alienAlloysExist && (*i)->item == _recoveryStats[ALIEN_ALLOYS]->name)
			{
				// hardcoded vanilla defaults, in case modders or players fail to install OXCE properly
				aadivider = (target == "STR_UFO") ? 10 : 150;
			}

			const RuleItem *itemRule = _game->getMod()->getItem((*i)->item, false);
			if (itemRule)
			{
				const std::map<std::string, int> recoveryDividers = itemRule->getRecoveryDividers();
				if (!recoveryDividers.empty())
				{
					bool done = false;
					if (ruleDeploy)
					{
						// step 1: check deployment
						if (recoveryDividers.find(ruleDeploy->getType()) != recoveryDividers.end())
						{
							aadivider = recoveryDividers.at(ruleDeploy->getType());
							done = true;
						}
					}
					if (!done)
					{
						// step 2: check mission type
						if (recoveryDividers.find(target) != recoveryDividers.end())
						{
							aadivider = recoveryDividers.at(target);
							done = true;
						}
					}
					if (!done)
					{
						// step 3: check global default
						if (recoveryDividers.find("STR_OTHER") != recoveryDividers.end())
						{
							aadivider = recoveryDividers.at("STR_OTHER");
						}
					}
				}
			}

			if (aadivider > 1)
			{
				(*i)->qty = (*i)->qty / aadivider;
				(*i)->score = (*i)->score / aadivider;
			}
			else if (aadivider < -1)
			{
				(*i)->qty = (*i)->qty * (-1) * aadivider;
				(*i)->score = (*i)->score * (-1) * aadivider;
			}

			// recoverable battlescape tiles are now converted to items and put in base inventory
			if ((*i)->recovery && (*i)->qty > 0)
			{
				addItemsToBaseStores((*i)->item, base, (*i)->qty, false);
			}
		}

		// assuming this was a multi-stage mission,
		// recover everything that was in the craft in the previous stage
		recoverItems(battle->getGuaranteedRecoveredItems(), base);
	}

	// calculate the clips for each type based on the recovered rounds.
	for (std::map<const RuleItem*, int>::const_iterator i = _rounds.begin(); i != _rounds.end(); ++i)
	{
		int total_clips = 0;
		if (_game->getMod()->getStatisticalBulletConservation())
		{
			total_clips = (i->second + RNG::generate(0, (i->first->getClipSize() - 1))) / i->first->getClipSize();
		}
		else
		{
			total_clips = i->second / i->first->getClipSize();
		}
		if (total_clips > 0)
		{
			addItemsToBaseStores(i->first, base, total_clips, true);
		}
	}

	// calculate the "remaining medikit items" for each type based on the recovered "clips".
	for (std::map<const RuleItem*, int>::const_iterator i = _roundsPainKiller.begin(); i != _roundsPainKiller.end(); ++i)
	{
		int totalRecovered = INT_MAX;
		if (_game->getMod()->getStatisticalBulletConservation())
		{
			if (i->first->getPainKillerQuantity() > 0)
				totalRecovered = std::min(totalRecovered, (i->second + RNG::generate(0, (i->first->getPainKillerQuantity() - 1))) / i->first->getPainKillerQuantity());
			if (i->first->getStimulantQuantity() > 0)
				totalRecovered = std::min(totalRecovered, (_roundsStimulant[i->first] + RNG::generate(0, (i->first->getStimulantQuantity() - 1))) / i->first->getStimulantQuantity());
			if (i->first->getHealQuantity() > 0)
				totalRecovered = std::min(totalRecovered, (_roundsHeal[i->first] + RNG::generate(0, (i->first->getHealQuantity() - 1))) / i->first->getHealQuantity());
		}
		else
		{
			if (i->first->getPainKillerQuantity() > 0)
				totalRecovered = std::min(totalRecovered, i->second / i->first->getPainKillerQuantity());
			if (i->first->getStimulantQuantity() > 0)
				totalRecovered = std::min(totalRecovered, _roundsStimulant[i->first] / i->first->getStimulantQuantity());
			if (i->first->getHealQuantity() > 0)
				totalRecovered = std::min(totalRecovered, _roundsHeal[i->first] / i->first->getHealQuantity());
		}

		if (totalRecovered > 0)
		{
			addItemsToBaseStores(i->first, base, totalRecovered, true);
		}
	}

	// reequip craft after a non-base-defense mission (of course only if it's not lost already (that case craft=0))
	if (craft)
	{
		reequipCraft(base, craft, true);
	}
	else
	{
		if (target != "STR_BASE" || _destroyBase)
		{
			hideSellTransferButtons();
		}
	}

	if (base && target == "STR_BASE")
	{
		AlienMission* am = base->getRetaliationMission();
		if (!am && _region)
		{
			// backwards-compatibility
			am = _game->getSavedGame()->findAlienMission(_region->getRules()->getType(), OBJECTIVE_RETALIATION);
		}
		if (!_destroyBase && am && am->getRules().isMultiUfoRetaliation())
		{
			// Remember that more UFOs may be coming (again, just in case)
			am->setMultiUfoRetaliationInProgress(true);
		}
		else
		{
			// Delete the mission and any live UFOs
			_game->getSavedGame()->deleteRetaliationMission(am, base);
		}

		if (!_destroyBase)
		{
			// reequip crafts (only those on the base) after a base defense mission
			for (std::vector<Craft*>::iterator c = base->getCrafts()->begin(); c != base->getCrafts()->end(); ++c)
			{
				if ((*c)->getStatus() != "STR_OUT")
					reequipCraft(base, *c, false);
			}
		}
		else if (_game->getSavedGame()->getMonthsPassed() != -1)
		{
			for (std::vector<Base*>::iterator i = _game->getSavedGame()->getBases()->begin(); i != _game->getSavedGame()->getBases()->end(); ++i)
			{
				if ((*i) == base)
				{
					_game->getSavedGame()->stopHuntingXcomCrafts((*i)); // destroyed together with the base
					delete (*i);
					base = 0; // To avoid similar (potential) problems as with the deleted craft
					_game->getSavedGame()->getBases()->erase(i);
					break;
				}
			}
		}
	}

	if (!_destroyBase)
	{
		// clean up remaining armor backups
		// Note: KIA and MIA soldiers have been handled already, only survivors can have non-empty values
		for (std::vector<Soldier*>::iterator i = base->getSoldiers()->begin(); i != base->getSoldiers()->end(); ++i)
		{
			if ((*i)->getReplacedArmor())
			{
				(*i)->setArmor((*i)->getReplacedArmor());
			}
			else if ((*i)->getTransformedArmor())
			{
				(*i)->setArmor((*i)->getTransformedArmor());

			}
			(*i)->setReplacedArmor(0);
			(*i)->setTransformedArmor(0);
		}
	}

	_missionStatistics->success = success;

	if (success && ruleDeploy && base)
	{
		// Unlock research defined in alien deployment, if the mission was a success
		const RuleResearch *research = _game->getMod()->getResearch(ruleDeploy->getUnlockedResearchOnSuccess());
		save->handleResearchUnlockedByMissions(research, _game->getMod());

		// Give bounty item defined in alien deployment, if the mission was a success
		const RuleItem *bountyItem = _game->getMod()->getItem(ruleDeploy->getMissionBountyItem());
		if (bountyItem)
		{
			int bountyQty = std::max(1, ruleDeploy->getMissionBountyItemCount());
			addItemsToBaseStores(bountyItem, base, bountyQty, true);
			auto specialType = bountyItem->getSpecialType();
			if (specialType > 1)
			{
				if (_recoveryStats.find(specialType) != _recoveryStats.end())
				{
					addStat(_recoveryStats[specialType]->name, bountyQty, bountyQty * _recoveryStats[specialType]->value);
				}
			}
		}

		// Increase counters
		save->increaseCustomCounter(ruleDeploy->getCounterSuccess());
		save->increaseCustomCounter(ruleDeploy->getCounterAll());
		// Decrease counters
		save->decreaseCustomCounter(ruleDeploy->getDecreaseCounterSuccess());
		save->decreaseCustomCounter(ruleDeploy->getDecreaseCounterAll());

		// Generate a success event
		_eventToSpawn = _game->getMod()->getEvent(ruleDeploy->chooseSuccessEvent());
	}
	else if (!success && ruleDeploy)
	{
		// Unlock research defined in alien deployment, if the mission was a failure
		const RuleResearch* research = _game->getMod()->getResearch(ruleDeploy->getUnlockedResearchOnFailure());
		save->handleResearchUnlockedByMissions(research, _game->getMod());

		// Increase counters
		save->increaseCustomCounter(ruleDeploy->getCounterFailure());
		save->increaseCustomCounter(ruleDeploy->getCounterAll());
		// Decrease counters
		save->decreaseCustomCounter(ruleDeploy->getDecreaseCounterFailure());
		save->decreaseCustomCounter(ruleDeploy->getDecreaseCounterAll());

		// Generate a failure event
		_eventToSpawn = _game->getMod()->getEvent(ruleDeploy->chooseFailureEvent());
	}

	// remember the base for later use (of course only if it's not lost already (in that case base=0))
	_base = base;
}

/**
 * Reequips a craft after a mission.
 * @param base Base to reequip from.
 * @param craft Craft to reequip.
 * @param vehicleItemsCanBeDestroyed Whether we can destroy the vehicles on the craft.
 */
void DebriefingState::reequipCraft(Base *base, Craft *craft, bool vehicleItemsCanBeDestroyed)
{
	std::map<std::string, int> craftItems = *craft->getItems()->getContents();
	for (std::map<std::string, int>::iterator i = craftItems.begin(); i != craftItems.end(); ++i)
	{
		int qty = base->getStorageItems()->getItem(i->first);
		if (qty >= i->second)
		{
			base->getStorageItems()->removeItem(i->first, i->second);
		}
		else
		{
			int missing = i->second - qty;
			base->getStorageItems()->removeItem(i->first, qty);
			craft->getItems()->removeItem(i->first, missing);
			ReequipStat stat = {i->first, missing, craft->getName(_game->getLanguage()), 0};
			_missingItems.push_back(stat);
		}
	}

	// Now let's see the vehicles
	ItemContainer craftVehicles;
	for (std::vector<Vehicle*>::iterator i = craft->getVehicles()->begin(); i != craft->getVehicles()->end(); ++i)
		craftVehicles.addItem((*i)->getRules());
	// Now we know how many vehicles (separated by types) we have to read
	// Erase the current vehicles, because we have to reAdd them (cause we want to redistribute their ammo)
	if (vehicleItemsCanBeDestroyed)
		for (std::vector<Vehicle*>::iterator i = craft->getVehicles()->begin(); i != craft->getVehicles()->end(); ++i)
			delete (*i);
	craft->getVehicles()->clear();
	// Ok, now read those vehicles
	for (std::map<std::string, int>::iterator i = craftVehicles.getContents()->begin(); i != craftVehicles.getContents()->end(); ++i)
	{
		int qty = base->getStorageItems()->getItem(i->first);
		RuleItem *tankRule = _game->getMod()->getItem(i->first, true);
		int size = tankRule->getVehicleUnit()->getArmor()->getTotalSize();
		int canBeAdded = std::min(qty, i->second);
		if (qty < i->second)
		{ // missing tanks
			int missing = i->second - qty;
			ReequipStat stat = {i->first, missing, craft->getName(_game->getLanguage()), 0};
			_missingItems.push_back(stat);
		}
		if (tankRule->getVehicleClipAmmo() == nullptr)
		{ // so this tank does NOT require ammo
			for (int j = 0; j < canBeAdded; ++j)
				craft->getVehicles()->push_back(new Vehicle(tankRule, tankRule->getVehicleClipSize(), size));
			base->getStorageItems()->removeItem(i->first, canBeAdded);
		}
		else
		{ // so this tank requires ammo
			const RuleItem *ammo = tankRule->getVehicleClipAmmo();
			int ammoPerVehicle = tankRule->getVehicleClipsLoaded();

			int baqty = base->getStorageItems()->getItem(ammo); // Ammo Quantity for this vehicle-type on the base
			if (baqty < i->second * ammoPerVehicle)
			{ // missing ammo
				int missing = (i->second * ammoPerVehicle) - baqty;
				ReequipStat stat = {ammo->getType(), missing, craft->getName(_game->getLanguage()), 0};
				_missingItems.push_back(stat);
			}
			canBeAdded = std::min(canBeAdded, baqty / ammoPerVehicle);
			if (canBeAdded > 0)
			{
				for (int j = 0; j < canBeAdded; ++j)
				{
					craft->getVehicles()->push_back(new Vehicle(tankRule, tankRule->getVehicleClipSize(), size));
					base->getStorageItems()->removeItem(ammo, ammoPerVehicle);
				}
				base->getStorageItems()->removeItem(i->first, canBeAdded);
			}
		}
	}
}

/**
 * Adds item(s) to base stores.
 * @param ruleItem Rule of the item(s) to be recovered.
 * @param base Base to add items to.
 * @param quantity How many items to recover.
 * @param considerTransformations Should the items be transformed before recovery?
 */
void DebriefingState::addItemsToBaseStores(const RuleItem *ruleItem, Base *base, int quantity, bool considerTransformations)
{
	if (!considerTransformations)
	{
		base->getStorageItems()->addItem(ruleItem->getType(), quantity);
	}
	else
	{
		auto recoveryTransformations = ruleItem->getRecoveryTransformations();
		if (!recoveryTransformations.empty())
		{
			for (auto& pair : recoveryTransformations)
			{
				if (pair.second.size() > 1)
				{
					int totalWeight = 0;
					for (auto it = pair.second.begin(); it != pair.second.end(); ++it)
					{
						totalWeight += (*it);
					}
					// roll each item separately
					for (int i = 0; i < quantity; ++i)
					{
						int roll = RNG::generate(1, totalWeight);
						int runningTotal = 0;
						int position = 0;
						for (auto it = pair.second.begin(); it != pair.second.end(); ++it)
						{
							runningTotal += (*it);
							if (runningTotal >= roll)
							{
								base->getStorageItems()->addItem(pair.first->getType(), position);
								break;
							}
							++position;
						}
					}
				}
				else
				{
					// no RNG
					base->getStorageItems()->addItem(pair.first->getType(), quantity * pair.second.front());
				}
			}
		}
		else
		{
			base->getStorageItems()->addItem(ruleItem->getType(), quantity);
		}
	}
}

/**
 * Adds item(s) to base stores.
 * @param ruleItem Rule of the item(s) to be recovered.
 * @param base Base to add items to.
 * @param quantity How many items to recover.
 * @param considerTransformations Should the items be transformed before recovery?
 */
void DebriefingState::addItemsToBaseStores(const std::string &itemType, Base *base, int quantity, bool considerTransformations)
{
	if (!considerTransformations)
	{
		base->getStorageItems()->addItem(itemType, quantity);
	}
	else
	{
		const RuleItem *ruleItem = _game->getMod()->getItem(itemType, false);
		if (ruleItem)
		{
			addItemsToBaseStores(ruleItem, base, quantity, considerTransformations);
		}
		else
		{
			// unknown item?
			base->getStorageItems()->addItem(itemType, quantity);
		}
	}
}

/**
 * Recovers items from the battlescape.
 *
 * Converts the battlescape inventory into a geoscape item container.
 * @param from Items recovered from the battlescape.
 * @param base Base to add items to.
 */
void DebriefingState::recoverItems(std::vector<BattleItem*> *from, Base *base)
{
	auto checkForRecovery = [&](BattleItem* item, const RuleItem *rule)
	{
		return !rule->isFixed() && rule->isRecoverable() && (!rule->isConsumable() || item->getFuseTimer() < 0);
	};

	auto recoveryAmmo = [&](BattleItem* clip, const RuleItem *rule)
	{
		if (rule->getBattleType() == BT_AMMO && rule->getClipSize() > 0)
		{
			// It's a clip, count any rounds left.
			_rounds[rule] += clip->getAmmoQuantity();
		}
		else
		{
			addItemsToBaseStores(rule, base, 1, true);
		}
	};

	auto recoveryAmmoInWeapon = [&](BattleItem* weapon)
	{
		// Don't need case of built-in ammo, since this is a fixed weapon
		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			BattleItem *clip = weapon->getAmmoForSlot(slot);
			if (clip && clip != weapon)
			{
				const RuleItem *rule = clip->getRules();
				if (checkForRecovery(clip, rule))
				{
					recoveryAmmo(clip, rule);
				}
			}
		}
	};

	for (std::vector<BattleItem*>::iterator it = from->begin(); it != from->end(); ++it)
	{
		const RuleItem *rule = (*it)->getRules();
		if (rule->getName() == _game->getMod()->getAlienFuelName())
		{
			// special case of an item counted as a stat
			addStat(_game->getMod()->getAlienFuelName(), _game->getMod()->getAlienFuelQuantity(), rule->getRecoveryPoints());
		}
		else
		{
			if (rule->isRecoverable() && !(*it)->getXCOMProperty())
			{
				if (rule->getBattleType() == BT_CORPSE)
				{
					BattleUnit *corpseUnit = (*it)->getUnit();
					if (corpseUnit->getStatus() == STATUS_DEAD)
					{
						if (rule->isCorpseRecoverable())
						{
							addItemsToBaseStores(corpseUnit->getArmor()->getCorpseGeoscape(), base, 1, true);
							addStat("STR_ALIEN_CORPSES_RECOVERED", 1, (*it)->getRules()->getRecoveryPoints());
						}
					}
					else if (corpseUnit->getStatus() == STATUS_UNCONSCIOUS ||
							// or it's in timeout because it's unconscious from the previous stage
							// units can be in timeout and alive, and we assume they flee.
							(corpseUnit->isIgnored() &&
							corpseUnit->getHealth() > 0 &&
							corpseUnit->getHealth() < corpseUnit->getStunlevel()))
					{
						if (corpseUnit->getOriginalFaction() == FACTION_HOSTILE || corpseUnit->getOriginalFaction() == FACTION_ALIEN_PLAYER)
						{
							recoverAlien(corpseUnit, base);
						}
					}
				}
				// only add recovery points for unresearched items
				else if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
				{
					addStat("STR_ALIEN_ARTIFACTS_RECOVERED", 1, rule->getRecoveryPoints());
				}
				else if (_game->getMod()->getGiveScoreAlsoForResearchedArtifacts())
				{
					addStat("STR_ALIEN_ARTIFACTS_RECOVERED", 1, rule->getRecoveryPoints());
				}
			}

			// Check if the bodies of our dead soldiers were left, even if we don't recover them
			// This is so we can give them a proper burial... or raise the dead!
			if ((*it)->getUnit() && (*it)->getUnit()->getStatus() == STATUS_DEAD && (*it)->getUnit()->getGeoscapeSoldier())
			{
				(*it)->getUnit()->getGeoscapeSoldier()->setCorpseRecovered(true);
			}

			// put items back in the base
			if (checkForRecovery(*it, rule))
			{
				bool recoverWeapon = true;
				switch (rule->getBattleType())
				{
					case BT_CORPSE: // corpses are handled above, do not process them here.
						break;
					case BT_MEDIKIT:
						if (rule->isConsumable())
						{
							// Need to remember all three!
							_roundsPainKiller[rule] += (*it)->getPainKillerQuantity();
							_roundsStimulant[rule] += (*it)->getStimulantQuantity();
							_roundsHeal[rule] += (*it)->getHealQuantity();
						}
						else
						{
							// Vanilla behaviour (recover a full medikit).
							addItemsToBaseStores(rule, base, 1, true);
						}
						break;
					case BT_AMMO:
						recoveryAmmo(*it, rule);
						break;
					case BT_FIREARM:
					case BT_MELEE:
						// Special case: built-in ammo (e.g. throwing knives or bamboo stick)
						if (!(*it)->needsAmmoForSlot(0) && rule->getClipSize() > 0)
						{
							_rounds[rule] += (*it)->getAmmoQuantity();
							recoverWeapon = false;
						}
						// It's a weapon, count any rounds left in the clip.
						recoveryAmmoInWeapon(*it);
						// Fall-through, to recover the weapon itself.
						FALLTHROUGH;
					default:
						if (recoverWeapon)
						{
							addItemsToBaseStores(rule, base, 1, true);
						}
				}
				if (rule->getBattleType() == BT_NONE)
				{
					for (std::vector<Craft*>::iterator c = base->getCrafts()->begin(); c != base->getCrafts()->end(); ++c)
					{
						(*c)->reuseItem(rule);
					}
				}
			}
			// special case of fixed weapons on a soldier's armor, but not HWPs
			// makes sure we recover the ammunition from this weapon
			else if (rule->isFixed() && (*it)->getOwner()->getOriginalFaction() == FACTION_PLAYER && (*it)->getOwner()->getGeoscapeSoldier())
			{
				switch (rule->getBattleType())
				{
					case BT_FIREARM:
					case BT_MELEE:
						// It's a weapon, count any rounds left in the clip.
						recoveryAmmoInWeapon(*it);
						break;
					default:
						break;
				}
			}
		}
	}
}

/**
* Recovers a live civilian from the battlescape.
* @param from Battle unit to recover.
* @param base Base to add items to.
*/
void DebriefingState::recoverCivilian(BattleUnit *from, Base *base)
{
	std::string type = from->getUnitRules()->getCivilianRecoveryType();
	if (type.empty())
	{
		return;
	}
	if (type == "STR_SCIENTIST")
	{
		Transfer *t = new Transfer(24);
		t->setScientists(1);
		base->getTransfers()->push_back(t);
	}
	else if (type == "STR_ENGINEER")
	{
		Transfer *t = new Transfer(24);
		t->setEngineers(1);
		base->getTransfers()->push_back(t);
	}
	else
	{
		RuleSoldier *ruleSoldier = _game->getMod()->getSoldier(type);
		if (ruleSoldier != 0)
		{
			Transfer *t = new Transfer(24);
			Soldier *s = _game->getMod()->genSoldier(_game->getSavedGame(), ruleSoldier->getType());
			if (!from->getUnitRules()->getSpawnedPersonName().empty())
			{
				s->setName(tr(from->getUnitRules()->getSpawnedPersonName()));
			}
			s->load(from->getUnitRules()->getSpawnedSoldierTemplate(), _game->getMod(), _game->getSavedGame(), _game->getMod()->getScriptGlobal(), true); // load from soldier template
			t->setSoldier(s);
			base->getTransfers()->push_back(t);
		}
		else
		{
			RuleItem *ruleItem = _game->getMod()->getItem(type);
			if (ruleItem != 0)
			{
				if (!ruleItem->isAlien())
				{
					addItemsToBaseStores(ruleItem, base, 1, true);
				}
				else
				{
					RuleItem *ruleLiveAlienItem = ruleItem;
					bool killPrisonersAutomatically = base->getAvailableContainment(ruleLiveAlienItem->getPrisonType()) == 0;
					if (killPrisonersAutomatically)
					{
						// check also other bases, maybe we can transfer/redirect prisoners there
						for (std::vector<Base*>::iterator i = _game->getSavedGame()->getBases()->begin(); i != _game->getSavedGame()->getBases()->end(); ++i)
						{
							if ((*i)->getAvailableContainment(ruleLiveAlienItem->getPrisonType()) > 0)
							{
								killPrisonersAutomatically = false;
								break;
							}
						}
					}
					if (killPrisonersAutomatically)
					{
						_containmentStateInfo[ruleLiveAlienItem->getPrisonType()] = 1; // 1 = not available in any base
					}
					else
					{
						addItemsToBaseStores(ruleLiveAlienItem, base, 1, false);
						int availableContainment = base->getAvailableContainment(ruleLiveAlienItem->getPrisonType());
						int usedContainment = base->getUsedContainment(ruleLiveAlienItem->getPrisonType());
						int freeContainment = availableContainment - (usedContainment * _limitsEnforced);
						// no capacity, or not enough capacity
						if (availableContainment == 0 || freeContainment < 0)
						{
							_containmentStateInfo[ruleLiveAlienItem->getPrisonType()] = 2; // 2 = overfull
						}
					}
				}
			}
		}
	}
}

/**
 * Recovers a live alien from the battlescape.
 * @param from Battle unit to recover.
 * @param base Base to add items to.
 */
void DebriefingState::recoverAlien(BattleUnit *from, Base *base)
{
	// Transform a live alien into one or more recovered items?
	auto* ruleLiveAlienItem = from->getUnitRules()->getLiveAlienGeoscape();
	if (ruleLiveAlienItem && !ruleLiveAlienItem->getRecoveryTransformations().empty())
	{
		addItemsToBaseStores(ruleLiveAlienItem, base, 1, true);

		// Ignore everything else, e.g. no points for live/dead aliens (since you did NOT recover them)
		// Also no points or anything else for the recovered items
		return;
	}

	// This ain't good! Let's display at least some useful info before we crash...
	if (!ruleLiveAlienItem)
	{
		std::ostringstream ss;
		ss << "Live alien item definition is missing. Unit ID = " << from->getId();
		ss << "; Type = " << from->getType();
		ss << "; Status = " << from->getStatus();
		ss << "; Faction = " << from->getFaction();
		ss << "; Orig. faction = " << from->getOriginalFaction();
		if (from->getSpawnUnit())
		{
			ss << "; Spawn unit = [" << from->getSpawnUnit()->getType() << "]";
		}
		ss << "; isSurrendering = " << from->isSurrendering();
		throw Exception(ss.str());
	}

	bool killPrisonersAutomatically = base->getAvailableContainment(ruleLiveAlienItem->getPrisonType()) == 0;
	if (killPrisonersAutomatically)
	{
		// check also other bases, maybe we can transfer/redirect prisoners there
		for (std::vector<Base*>::iterator i = _game->getSavedGame()->getBases()->begin(); i != _game->getSavedGame()->getBases()->end(); ++i)
		{
			if ((*i)->getAvailableContainment(ruleLiveAlienItem->getPrisonType()) > 0)
			{
				killPrisonersAutomatically = false;
				break;
			}
		}
	}
	if (killPrisonersAutomatically)
	{
		_containmentStateInfo[ruleLiveAlienItem->getPrisonType()] = 1; // 1 = not available in any base

		if (!from->getArmor()->getCorpseBattlescape().empty())
		{
			auto corpseRule = from->getArmor()->getCorpseBattlescape().front();
			if (corpseRule && corpseRule->isRecoverable())
			{
				if (corpseRule->isCorpseRecoverable())
				{
					addStat("STR_ALIEN_CORPSES_RECOVERED", 1, corpseRule->getRecoveryPoints());
					auto corpseItem = from->getArmor()->getCorpseGeoscape();
					addItemsToBaseStores(corpseItem, base, 1, true);
				}
			}
		}
	}
	else
	{
		RuleResearch *research = _game->getMod()->getResearch(from->getUnitRules()->getType());
		bool surrendered = (!from->isOut() || from->isIgnored())
			&& (from->isSurrendering() || _game->getSavedGame()->getSavedBattle()->getChronoTrigger() == FORCE_WIN_SURRENDER);
		if (research != 0 && !_game->getSavedGame()->isResearched(research))
		{
			// more points if it's not researched
			addStat(surrendered ? "STR_LIVE_ALIENS_SURRENDERED" : "STR_LIVE_ALIENS_RECOVERED", 1, from->getValue() * 2);
		}
		else if (_game->getMod()->getGiveScoreAlsoForResearchedArtifacts())
		{
			addStat(surrendered ? "STR_LIVE_ALIENS_SURRENDERED" : "STR_LIVE_ALIENS_RECOVERED", 1, from->getValue() * 2);
		}
		else
		{
			// 10 points for recovery
			addStat(surrendered ? "STR_LIVE_ALIENS_SURRENDERED" : "STR_LIVE_ALIENS_RECOVERED", 1, 10);
		}

		addItemsToBaseStores(ruleLiveAlienItem, base, 1, false);
		int availableContainment = base->getAvailableContainment(ruleLiveAlienItem->getPrisonType());
		int usedContainment = base->getUsedContainment(ruleLiveAlienItem->getPrisonType());
		int freeContainment = availableContainment - (usedContainment * _limitsEnforced);
		// no capacity, or not enough capacity
		if (availableContainment == 0 || freeContainment < 0)
		{
			_containmentStateInfo[ruleLiveAlienItem->getPrisonType()] = 2; // 2 = overfull
		}
	}
}

/**
 * Gets the number of recovered items of certain type.
 * @param rule Type of item.
 */
int DebriefingState::getRecoveredItemCount(const RuleItem *rule)
{
	auto it = _recoveredItems.find(rule);
	if (it != _recoveredItems.end())
		return it->second;

	return 0;
}

/**
 * Gets the total number of recovered items.
 */
int DebriefingState::getTotalRecoveredItemCount()
{
	int total = 0;
	for (auto item : _recoveredItems)
	{
		total += item.second;
	}
	return total;
}

/**
 * Decreases the number of recovered items by the sold/transferred amount.
 * @param rule Type of item.
 * @param amount Number of items sold or transferred.
 */
void DebriefingState::decreaseRecoveredItemCount(const RuleItem *rule, int amount)
{
	auto it = _recoveredItems.find(rule);
	if (it != _recoveredItems.end())
	{
		it->second = std::max(0, it->second - amount);
	}
}

/**
 * Hides the SELL and TRANSFER buttons.
 */
void DebriefingState::hideSellTransferButtons()
{
	_showSellButton = false;
	_btnSell->setVisible(_showSellButton);
	_btnTransfer->setVisible(_showSellButton);
}

}
