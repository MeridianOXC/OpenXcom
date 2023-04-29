#pragma once
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
#include "Soldier.h"

namespace OpenXcom
{

/**
 * Base class for the rank count classes.
 */
class RankCountBase
{
  protected:
	/// count of soldiers in each rank.
	std::array<int, RANK_COMMANDER + 1> _rankCounts;
	int _totalSoldiers;

	RankCountBase() : _rankCounts(), _totalSoldiers(0) {}

  public:
	int& operator[](const SoldierRank rank);

	int getTotalSoldiers() const;
};

/**
 * @brief Container for counts of soldier ranks.
 */
class RankCount : public RankCountBase
{
  public:
	RankCount(const std::vector<Soldier*> soldiers);
};

/**
 * @brief Container for counts of promotion openings.
 */
class PromotionOpenings : public RankCountBase
{
  public:
	PromotionOpenings(const std::vector<Soldier*> soldiers, const Mod* mod);

	// check if a soldier can be promoted to a given rank.
	bool isManualPromotionPossible(const Soldier* soldier, const SoldierRank newRank) const;
};

}
