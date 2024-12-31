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
#include <vector>
#include "../Engine/Yaml.h"
#include "../Engine/Script.h"

namespace OpenXcom
{

class RuleCountry;
class SavedGame;

/**
 * Represents a country that funds the player.
 * Contains variable info about a country like
 * monthly funding and various activities.
 */
class Country
{
public:
	enum class Satisfaction : int { ALIEN_PACT, UNHAPPY, SATISFIED, HAPPY };

	/// Name of class used in script.
	static constexpr const char* ScriptName = "Country";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

private:
	RuleCountry *_rules;
	bool _pact, _newPact, _cancelPact;
	std::vector<int> _funding, _activityXcom, _activityAlien;
	Satisfaction _satisfaction;
	ScriptValues<Country> _scriptValues;

public:
	/// Creates a new country of the specified type.
	Country(RuleCountry *rules, bool gen = true);
	/// Cleans up the country.
	~Country();
	/// Loads the country from YAML.
	void load(const YAML::YamlNodeReader& reader, const ScriptGlobal* shared);
	/// Saves the country to YAML.
	void save(YAML::YamlNodeWriter writer, const ScriptGlobal* shared) const;
	/// Gets the country's ruleset.
	const RuleCountry *getRules() const;
	/// Gets the country's funding.
	std::vector<int> &getFunding();
	/// Sets the country's funding.
	void setFunding(int funding);
	/// get the country's satisfaction level
	Satisfaction getSatisfaction() const;
	/// add xcom activity in this country
	void addActivityXcom(int activity);
	/// add alien activity in this country
	void addActivityAlien(int activity);
	/// get xcom activity to this country
	std::vector<int> &getActivityXcom();
	/// get xcom activity to this country
	std::vector<int> &getActivityAlien();
	/// store last month's counters, start new counters, set this month's change.
	void newMonth(int xcomTotal, int alienTotal, int pactScore, int averageFunding, const SavedGame* save);
	/// are we signing a new pact?
	bool getNewPact() const;
	/// sign a pact at the end of this month.
	void setNewPact();
	/// are we cancelling an existing pact?
	bool getCancelPact() const;
	/// cancel or prevent a pact.
	void setCancelPact();
	/// have we signed a pact?
	bool getPact() const;
	/// sign a pact immediately
	void setPact();
	/// can be (re)infiltrated?
	bool canBeInfiltrated();

private:
	int getCurrentFunding() const { return _funding.back(); }
	int getCurrentActivityAlien()  const { return _activityAlien.back(); }
	int getCurrentActivityXcom() const { return _activityXcom.back(); }
};

}
