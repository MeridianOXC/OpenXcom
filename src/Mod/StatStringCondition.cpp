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
#include "StatStringCondition.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleSoldier.h"
#include "../Engine/Logger.h"

namespace OpenXcom
{
/**
 * Checks if this condition is valid for the given soldier.
 * @param soldier The soldier to test.
 * @return If the condition is met.
 */
bool StatStringConditionAbsolulte::isMet(const Soldier &soldier) const
{
	const int stat = soldier.getCurrentStats()[_statType];
	return stat >= _minVal && stat <= _maxVal;
}

/**
 * Checks if this condition is valid for the given soldier.
 * @param soldier The soldier to test.
 * @return If the condition is met.
 */
bool StatStringConditionPercent::isMet(const Soldier &soldier) const
{
	const int stat = soldier.getCurrentStats()[_statType];
	const int cap = soldier.getRules()->getStatCaps()[_statType];

	// cap should probably never be 0, but we'll handle that just in case to avoid DBZ.
	if (cap == 0)
	{
		return _minVal <= 0;
	}

	// we store our percents as values from 0-100, so multiply to get them in range.
	float percent = (static_cast<float>(stat) / cap) * 100;

	// Since these are not integers we make do a range that is [minVal, maxVal)
	return percent >= _minVal && percent < _maxVal;
}

/**
 * Checks if this condition is valid for the given soldier.
 * @param soldier The soldier to test.
 * @return If the condition is met.
 */
bool StatStringConditionTraining::isMet(const Soldier &soldier) const
{
	switch (_trainingType)
	{
	case TrainingType::PSI_TRAINING:
		return soldier.isInPsiTraining() == _inTraining;
	case TrainingType::PHYS_TRAINING:
		return soldier.isInTraining() == _inTraining;
	default:
		Log(LOG_ERROR) << "Invalid Training Type Enum.";
		return false;
	}
}
}
