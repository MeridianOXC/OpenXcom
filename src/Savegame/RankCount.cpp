/*
 * Copyright 2010-2023 OpenXcom Developers.
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
#include "RankCount.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldier.h"

namespace OpenXcom
{

/**
 * Gets the count of promotions by SoldierRank.
 * @param rank to query for promotions
 * @return the count of this rank
 */
int &RankCountBase::operator[](const SoldierRank rank)
{
	return _rankCounts[rank];
}

/**
 * @brief Gets the total number of soldiers counted.
 * @return The total number of soldiers.
 */
int RankCountBase::getTotalSoldiers() const
{
	return _totalSoldiers;
}

/**
 * @brief Creates a new RankCount object containing rank
 * @param The list of soldiers to collect rank information about. Typically should be all soldiers.
 */
RankCount::RankCount(const std::vector<Soldier*> soldiers)
{
	_totalSoldiers = 0;
	for (auto* soldier : soldiers)
	{
		if (soldier->getRules()->getAllowPromotion())
		{
			_rankCounts[soldier->getRank()]++;
			_totalSoldiers++;
		}
	}
}

/**
 * Creates a new PromotionOpenings object, containing the count of promotion openings based on the mods promotion rules.
 * Ranks that are overfull for some reason will show as 0 openings, ranks that have unlimited openings will have -1 openings.
 * @param soldiers The set of soldiers to consider for calculation, typically should be all active soldiers.
 * @param mod The mod containing the rules for promotions.
 */
PromotionOpenings::PromotionOpenings(const std::vector<Soldier*> soldiers, const Mod* mod)
{
	RankCount currentRankCount = RankCount(soldiers);

	int totalSoldiers = currentRankCount.getTotalSoldiers();

	// special logic, there is only one Commander ever.
	if (currentRankCount[RANK_COMMANDER] == 0 && totalSoldiers >= mod->getSoldiersPerRank(RANK_COMMANDER))
	{
		_rankCounts[RANK_COMMANDER]++;
	}

	_rankCounts[RANK_COLONEL] += std::max(0, totalSoldiers / mod->getSoldiersPerRank(RANK_COLONEL) - currentRankCount[RANK_COLONEL]);
	_rankCounts[RANK_CAPTAIN] += std::max(0, totalSoldiers / mod->getSoldiersPerRank(RANK_CAPTAIN) - currentRankCount[RANK_CAPTAIN]);
	_rankCounts[RANK_SERGEANT] += std::max(0, totalSoldiers / mod->getSoldiersPerRank(RANK_SERGEANT) - currentRankCount[RANK_SERGEANT]);

	// promotions to Squaddie and Rookie are unlimited, indicate this with a -1.
	_rankCounts[RANK_SQUADDIE] = -1;
	_rankCounts[RANK_ROOKIE] = -1;
}

/**
 * @brief Tests if a soldier can be manually promoted to a new rank.
 * @param soldier The soldier to test for promotion.
 * @param newRank The new rank to promote the soldier to.
 * @return True if the soldier can be promoted to this rank, false otherwise.
 */
bool PromotionOpenings::isManualPromotionPossible(const Soldier* soldier, const SoldierRank newRank) const
{
	// check if the soldiers rules allow promotion.
	const auto* soldierRules = soldier->getRules();
	if (!soldierRules->getAllowPromotion())
	{
		return false;
	}

	const SoldierRank currentRank = soldier->getRank();

	// rookies cannot be promoted
	if (currentRank == RANK_ROOKIE /* || newRank == RANK_ROOKIE */)
	{
		return false;
	}

	// can't promote to same rank.
	if (currentRank == newRank)
	{
		return false;
	}

	// If the rankString for the soldier is not empty, check if the new rank is defined in the rank strings.
	// If not, it is not allowed. If no rankStrings are defined, we assume default behavior.
	const size_t rankStringsSize = soldierRules->getRankStrings().size();
	if (rankStringsSize != 0 && (size_t)newRank >= rankStringsSize)
	{
		return false;
	}

	// can always demote to rookie or squaddie
	if (newRank == RANK_ROOKIE || newRank == RANK_SQUADDIE)
	{
		return true;
	}

	// otherwise promotion or demotion depends on there being an opening.
	return _rankCounts[newRank] > 0;
}

}
