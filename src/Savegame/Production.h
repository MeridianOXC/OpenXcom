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
#include <yaml-cpp/yaml.h>

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
private:
	const RuleManufacture * _rules;
	int _amount;
	bool _infinite;
	int _timeSpent;
	int _engineers;
	int _efficiency;
	bool _sell;
	std::map<std::string, int> _randomProductionInfo;
	bool haveEnoughMoneyForOneMoreUnit(SavedGame * g) const;
	bool haveEnoughLivingSpaceForOneMoreUnit(Base * b);
	bool haveEnoughMaterialsForOneMoreUnit(Base * b, const Mod *m) const;
public:
	Production (const RuleManufacture * rules, int amount);
	int getAmountTotal() const { return _amount; }
	void setAmountTotal (int amount) { _amount = amount; }
	bool getInfiniteAmount() const { return _infinite; }
	void setInfiniteAmount (bool inf) { _infinite = inf; }
	int getTimeSpent() const { return _timeSpent; }
	void setTimeSpent (int done) { _timeSpent = done; }
	int getAmountProduced() const;
	int getAssignedEngineers() const { return _engineers; };
	void setAssignedEngineers (int engineers) { _engineers = engineers; }
	int getEfficiency() const { return _efficiency; }
	void setEfficiency(int efficiency) { _efficiency = efficiency; }
	bool getSellItems() const { return _sell; }
	void setSellItems (bool sell) { _sell = sell; }
	/// Gets the hourly progress with assigned engineers and other conditions
	int getProgress(Base* b, SavedGame* g, const Mod* m, int loyalty);
	productionProgress_e step(Base * b, SavedGame * g, const Mod *m, Language *lang, int rating);
	const RuleManufacture * getRules() const;
	void startItem(Base * b, SavedGame * g, const Mod *m) const;
	void refundItem(Base * b, SavedGame * g, const Mod *m) const;
	YAML::Node save() const;
	void load(const YAML::Node &node);
	const std::map<std::string, int> &getRandomProductionInfo() const { return _randomProductionInfo; }
};

}
