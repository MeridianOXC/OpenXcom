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
#include "../Engine/Yaml.h"

namespace OpenXcom
{

class RuleManufacture;
class Base;
class SavedGame;
class Language;
class Mod;
enum productionProgress_e { PROGRESS_NOT_COMPLETE, PROGRESS_COMPLETE, PROGRESS_NOT_ENOUGH_MONEY, PROGRESS_NOT_ENOUGH_MATERIALS, PROGRESS_NOT_ENOUGH_LIVING_SPACE, PROGRESS_MAX, PROGRESS_CONSTRUCTION };

class Production
{
public:
	Production (const RuleManufacture * rules, int amount);
	int getAmountTotal() const;
	void setAmountTotal (int);
	bool getInfiniteAmount() const;
	void setInfiniteAmount (bool);
	int getTimeSpent() const;
	void setTimeSpent (int);
	bool isQueuedOnly() const;
	int getAmountProduced() const;
	int getAssignedEngineers() const;
	void setAssignedEngineers (int);
	bool getSellItems() const;
	void setSellItems (bool);
	bool isFallback() const { return _isFallback; }
	void setFallback(bool newValue) { _isFallback = newValue; }
	productionProgress_e step(Base * b, SavedGame * g, const Mod *m, Language *lang);
	const RuleManufacture * getRules() const;
	void startItem(Base * b, SavedGame * g, const Mod *m) const;
	void refundItem(Base * b, SavedGame * g, const Mod *m) const;
	void save(YAML::YamlNodeWriter writer) const;
	void load(const YAML::YamlNodeReader& reader);
	const std::map<std::string, int> &getRandomProductionInfo() const { return _randomProductionInfo; }
private:
	const RuleManufacture * _rules;
	int _amount;
	bool _infinite;
	int _timeSpent;
	int _engineers;
	bool _sell;
	bool _isFallback;
	std::map<std::string, int> _randomProductionInfo;
	bool haveEnoughMoneyForOneMoreUnit(SavedGame * g) const;
	bool haveEnoughLivingSpaceForOneMoreUnit(Base * b);
	bool haveEnoughMaterialsForOneMoreUnit(Base * b, const Mod *m) const;
};

}
