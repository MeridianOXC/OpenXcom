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
#include "Unit.h"

namespace OpenXcom
{

class Soldier;

// from X-Com Util.
static constexpr int STATSTRING_CONDITION_ABSOLUTE_DEFAULT_MIN = 0;
static constexpr int STATSTRING_CONDITION_ABSOLUTE_DEFAULT_MAX = 255;

static constexpr float STATSTRING_CONDITION_PERCENT_DEFAULT_MIN = 0;
static constexpr float STATSTRING_CONDITION_PERCENT_DEFAULT_MAX = 255;

static constexpr bool STAT_CONDITION_TRAINING_INTRAINING_DEFAULT = true;

/**
 * @brief Abstract base class for a condition.
 * All conditions are either psi related conditions or not, and can test if a soldier meets them.
 */
class StatStringCondition
{
private:
	bool _psiCondition;

public:
	/// Create a new StatString Condition
	StatStringCondition(bool psi) : _psiCondition(psi) {}

	/// none of our chihldren should need special destruction, but just in case.
	virtual ~StatStringCondition() = default;

	// because this is an abstract class, we don't want it to be copied, slicing will result.
	StatStringCondition(const StatStringCondition&) = delete;
	StatStringCondition &operator=(const StatStringCondition &) = delete;

	/// Gets if this condition is a psi or not.
	virtual bool isPsiCondition() const { return _psiCondition; }
	/// Checks if a soldier meets this condition.
	virtual bool isMet(const Soldier &soldier) const = 0;
};

/**
 * @brief Condition to test if a soldiers attribute is in a given range.
 * Range is treated as [minVal, maxVal]. Is a PsiCondition if the stat is a psiStat.
 */
class StatStringConditionAbsolulte : public StatStringCondition
{
private:
	int _minVal, _maxVal;
	UnitStats::Type UnitStats::* _statRef;

public:
	/// Creates a StandardStatStringCondition, for testing if a stat is in an attribute range.
	StatStringConditionAbsolulte(UnitStats::Type UnitStats::* stat, int minVal, int maxVal)
	  : StatStringCondition(stat == &UnitStats::psiStrength || stat == &UnitStats::psiSkill),
			_statRef(stat), _minVal(minVal), _maxVal(maxVal) {}
	
	/// Checks if a soldier meets this condition.
	bool isMet(const Soldier &soldier) const override;
};

/**
 * @brief Condition to test if a soldiers attributes fall in a given percent range of their cap.
 * Note that while the min/max are floats, they are stored on a 0-100 basis, not 0-1.
 * Range is treated as [minVal, maxVal). Is a PsiCondition if the stat is a psiStat.
 */
class StatStringConditionPercent : public StatStringCondition
{
private:
	float _minVal, _maxVal;
	UnitStats::Type UnitStats::*_statRef;

public:
	/// Creates a condition, for testing if a stat is an percentage range.
	StatStringConditionPercent(UnitStats::Type UnitStats::* stat, float minVal, float maxVal)
		: StatStringCondition(stat == &UnitStats::psiStrength || stat == &UnitStats::psiSkill),
		  _statRef(stat), _minVal(minVal), _maxVal(maxVal) {}

	bool isMet(const Soldier &soldier) const override;
};

enum class TrainingType { PSI_TRAINING, PHYS_TRAINING };

/**
 * @brief Condition to test if a soldier is in Training. Always a psi condition. 
 */
class StatStringConditionTraining : public StatStringCondition
{
private:
	bool _inTraining = STAT_CONDITION_TRAINING_INTRAINING_DEFAULT;
	TrainingType _trainingType;

public:
	/// Creates a conditio for checking if a soldier is in PsiTraining.
	StatStringConditionTraining(TrainingType trainingType, bool inTraining = STAT_CONDITION_TRAINING_INTRAINING_DEFAULT)
		: StatStringCondition(trainingType == TrainingType::PSI_TRAINING), _trainingType(trainingType), _inTraining(inTraining) {}

	bool isMet(const Soldier &soldier) const override;
};

}
