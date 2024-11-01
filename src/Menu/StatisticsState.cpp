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
#include "StatisticsState.h"
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Savegame/SavedGame.h"
#include "MainMenuState.h"
#include "../Savegame/MissionStatistics.h"
#include "../Savegame/Base.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/AlienBase.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Statistics window.
 * @param game Pointer to the core game.
 */
StatisticsState::StatisticsState()
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_btnOk = new TextButton(50, 12, 135, 180);
	_txtTitle = new Text(310, 25, 5, 8);
	_lstStats = new TextList(280, 136, 12, 36);

	// Set palette
	setInterface("endGameStatistics");

	add(_window, "window", "endGameStatistics");
	add(_btnOk, "button", "endGameStatistics");
	add(_txtTitle, "text", "endGameStatistics");
	add(_lstStats, "list", "endGameStatistics");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "endGameStatistics");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&StatisticsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&StatisticsState::btnOkClick, Options::keyOk);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_lstStats->setColumns(2, 200, 80);
	_lstStats->setDot(true);

	listStats();
}

/**
 *
 */
StatisticsState::~StatisticsState()
{

}

template <typename T>
T StatisticsState::sumVector(const std::vector<T> &vec) const
{
	T total = 0;
	for (typename std::vector<T>::const_iterator i = vec.begin(); i != vec.end(); ++i)
	{
		total += *i;
	}
	return total;
}

void StatisticsState::listStats()
{
	SavedGame *save = _game->getSavedGame();

	std::ostringstream ss;
	GameTime *time = save->getTime();
	if (save->getEnding() == END_WIN)
	{
		ss << tr("STR_VICTORY");
	}
	else if (save->getEnding() == END_LOSE)
	{
		ss << tr("STR_DEFEAT");
	}
	else
	{
		ss << tr("STR_STATISTICS");
	}
	ss << Unicode::TOK_NL_SMALL << time->getDayString(_game->getLanguage()) << " " << tr(time->getMonthString()) << " " << time->getYear();
	_txtTitle->setText(ss.str());

	int totalScore = sumVector(save->getResearchScores());
	for (auto* region : *save->getRegions())
	{
		totalScore += sumVector(region->getActivityXcom()) - sumVector(region->getActivityAlien());
	}

	int monthlyScore = totalScore / (int)save->getResearchScores().size();
	int64_t totalIncome = sumVector(save->getIncomes());
	int64_t totalExpenses = sumVector(save->getExpenditures());

	int alienBasesDestroyed = 0, xcomBasesLost = 0;
	int missionsWin = 0, missionsLoss = 0, nightMissions = 0;
	int bestScore = -9999, worstScore = 9999;
	for (const auto* ms : *save->getMissionStatistics())
	{
		if (ms->success)
		{
			missionsWin++;
		}
		else
		{
			missionsLoss++;
		}
		bestScore = std::max(bestScore, ms->score);
		worstScore = std::min(worstScore, ms->score);
		if (ms->isDarkness(_game->getMod()))
		{
			nightMissions++;
		}
		if (ms->isAlienBase() && ms->success)
		{
			alienBasesDestroyed++;
		}
		if (ms->isBaseDefense() && !ms->success)
		{
			xcomBasesLost++;
		}
	}
	// Make sure dummy values aren't left in
	bestScore = (bestScore == -9999) ? 0 : bestScore;
	worstScore = (worstScore == 9999) ? 0 : worstScore;

	std::vector<Soldier*> allSoldiers;
	for (auto* xbase : *save->getBases())
	{
		allSoldiers.insert(allSoldiers.end(), xbase->getSoldiers()->begin(), xbase->getSoldiers()->end());
	}
	allSoldiers.insert(allSoldiers.end(), save->getDeadSoldiers()->begin(), save->getDeadSoldiers()->end());
	int soldiersRecruited = allSoldiers.size();
	int soldiersLost = save->getDeadSoldiers()->size();
	int vehiclesLost = save->getVehiclesLost();

	int aliensKilled = 0, aliensCaptured = 0, friendlyKills = 0;
	int daysWounded = 0, longestMonths = 0;
	int shotsFired = 0, shotsLanded = 0;
	std::map<std::string, int> weaponKills, alienKills;
	for (const auto* soldier : allSoldiers)
	{
		const SoldierDiary *diary = soldier->getDiary();
		aliensKilled += diary->getKillTotal();
		aliensCaptured += diary->getStunTotal();
		daysWounded += diary->getDaysWoundedTotal();
		longestMonths = std::max(longestMonths, diary->getMonthsService());
		std::map<std::string, int> weaponTotal = diary->getWeaponTotal();
		shotsFired += diary->getShotsFiredTotal();
		shotsLanded += diary->getShotsLandedTotal();
		for (const auto& pair : weaponTotal)
		{
			if (weaponKills.find(pair.first) == weaponKills.end())
			{
				weaponKills[pair.first] = pair.second;
			}
			else
			{
				weaponKills[pair.first] += pair.second;
			}
		}

		if (soldier->getDeath() != 0 && soldier->getDeath()->getCause() != 0)
		{
			const BattleUnitKills *kills = soldier->getDeath()->getCause();
			if (kills->faction == FACTION_PLAYER)
			{
				friendlyKills++;
			}
			if (!kills->race.empty())
			{
				if (alienKills.find(kills->race) == alienKills.end())
				{
					alienKills[kills->race] = 1;
				}
				else
				{
					alienKills[kills->race] += 1;
				}
			}
		}
	}
	int accuracy = 0;
	if (shotsFired > 0)
	{
		accuracy = 100 * shotsLanded / shotsFired;
	}

	int maxWeapon = 0;
	std::string highestWeapon = "STR_NONE";
	for (const auto& pair : weaponKills)
	{
		if (pair.second > maxWeapon)
		{
			maxWeapon = pair.second;
			highestWeapon = pair.first;
		}
	}
	int maxAlien = 0;
	std::string highestAlien = "STR_NONE";
	for (const auto& pair : alienKills)
	{
		if (pair.second > maxAlien)
		{
			maxAlien = pair.second;
			highestAlien = pair.first;
		}
	}

	std::map<std::string, int> ids = save->getAllIds();
	int alienBases = alienBasesDestroyed;
	for (const auto* ab : *save->getAlienBases())
	{
		if (ab->isDiscovered())
		{
			alienBases++;
		}
	}
	int ufosDetected = std::max(0, ids["STR_UFO"] - 1);
	int terrorSites = std::max(0, ids["STR_TERROR_SITE"] - 1);
	int totalCrafts = 0;
	for (const auto& craftType : _game->getMod()->getCraftsList())
	{
		totalCrafts += std::max(0, ids[craftType] - 1);
	}

	int xcomBases = save->getBases()->size() + xcomBasesLost;
	int currentScientists = 0, currentEngineers = 0;
	for (const auto* xbase : *save->getBases())
	{
		currentScientists += xbase->getTotalScientists();
		currentEngineers += xbase->getTotalEngineers();
	}

	int countriesLost = 0;
	for (const auto* country : *save->getCountries())
	{
		if (country->getPact())
		{
			countriesLost++;
		}
	}

	int researchDone = save->getDiscoveredResearch().size();

	std::string difficulty[] = { "STR_1_BEGINNER", "STR_2_EXPERIENCED", "STR_3_VETERAN", "STR_4_GENIUS", "STR_5_SUPERHUMAN" };

	_lstStats->addRow(2, tr("STR_DIFFICULTY").c_str(), tr(difficulty[save->getDifficulty()]).c_str());
	_lstStats->addRow(2, tr("STR_AVERAGE_MONTHLY_RATING").c_str(), Unicode::formatNumber(monthlyScore).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_INCOME").c_str(), Unicode::formatFunding(totalIncome).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_EXPENDITURE").c_str(), Unicode::formatFunding(totalExpenses).c_str());
	if (Options::soldierDiaries)
	{
		_lstStats->addRow(2, tr("STR_MISSIONS_WON").c_str(), Unicode::formatNumber(missionsWin).c_str());
		_lstStats->addRow(2, tr("STR_MISSIONS_LOST").c_str(), Unicode::formatNumber(missionsLoss).c_str());
		_lstStats->addRow(2, tr("STR_NIGHT_MISSIONS").c_str(), Unicode::formatNumber(nightMissions).c_str());
		_lstStats->addRow(2, tr("STR_BEST_RATING").c_str(), Unicode::formatNumber(bestScore).c_str());
		_lstStats->addRow(2, tr("STR_WORST_RATING").c_str(), Unicode::formatNumber(worstScore).c_str());
	}
	_lstStats->addRow(2, tr("STR_SOLDIERS_RECRUITED").c_str(), Unicode::formatNumber(soldiersRecruited).c_str());
	_lstStats->addRow(2, tr("STR_SOLDIERS_LOST").c_str(), Unicode::formatNumber(soldiersLost).c_str());
	_lstStats->addRow(2, tr("STR_VEHICLES_LOST").c_str(), Unicode::formatNumber(vehiclesLost).c_str()); // "STR_TANKS_DESTROYED" from DebriefingState
	if (Options::soldierDiaries)
	{
		_lstStats->addRow(2, tr("STR_ALIEN_KILLS").c_str(), Unicode::formatNumber(aliensKilled).c_str());
		_lstStats->addRow(2, tr("STR_ALIEN_CAPTURES").c_str(), Unicode::formatNumber(aliensCaptured).c_str());
		_lstStats->addRow(2, tr("STR_FRIENDLY_KILLS").c_str(), Unicode::formatNumber(friendlyKills).c_str());
		_lstStats->addRow(2, tr("STR_AVERAGE_ACCURACY").c_str(), Unicode::formatPercentage(accuracy).c_str());
		_lstStats->addRow(2, tr("STR_WEAPON_MOST_KILLS").c_str(), tr(highestWeapon).c_str());
		_lstStats->addRow(2, tr("STR_ALIEN_MOST_KILLS").c_str(), tr(highestAlien).c_str());
		_lstStats->addRow(2, tr("STR_LONGEST_SERVICE").c_str(), Unicode::formatNumber(longestMonths).c_str());
		_lstStats->addRow(2, tr("STR_TOTAL_DAYS_WOUNDED").c_str(), Unicode::formatNumber(daysWounded).c_str());
	}
	_lstStats->addRow(2, tr("STR_TOTAL_UFOS").c_str(), Unicode::formatNumber(ufosDetected).c_str());
	if (Options::soldierDiaries)
	{
		_lstStats->addRow(2, tr("STR_TOTAL_ALIEN_BASES").c_str(), Unicode::formatNumber(alienBases).c_str());
	}
	_lstStats->addRow(2, tr("STR_COUNTRIES_LOST").c_str(), Unicode::formatNumber(countriesLost).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_TERROR_SITES").c_str(), Unicode::formatNumber(terrorSites).c_str());
	if (Options::soldierDiaries)
	{
		_lstStats->addRow(2, tr("STR_TOTAL_BASES").c_str(), Unicode::formatNumber(xcomBases).c_str());
	}
	_lstStats->addRow(2, tr("STR_TOTAL_CRAFT").c_str(), Unicode::formatNumber(totalCrafts).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_SCIENTISTS").c_str(), Unicode::formatNumber(currentScientists).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_ENGINEERS").c_str(), Unicode::formatNumber(currentEngineers).c_str());
	_lstStats->addRow(2, tr("STR_TOTAL_RESEARCH").c_str(), Unicode::formatNumber(researchDone).c_str());
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void StatisticsState::btnOkClick(Action *)
{
	if (_game->getSavedGame()->getEnding() == END_NONE)
	{
		_game->popState();
	}
	else
	{
		_game->setSavedGame(0);
		_game->setState(new GoToMainMenuState);
	}
}

}
