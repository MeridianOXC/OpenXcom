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
#include <string>
#include "../Engine/Yaml.h"
#include "RuleEvent.h"
#include "ModScript.h"
#include "RuleBaseFacilityFunctions.h"

namespace OpenXcom
{

class Mod;

/**
 * Represents a specific funding country.
 * Contains constant info like its location in the
 * world and starting funding range.
 */
class RuleCountry
{
private:
	std::string _type;
	std::string _signedPactEventName, _rejoinedXcomEventName;
	int _fundingBase, _fundingCap;
	double _labelLon, _labelLat;
	std::vector<double> _lonMin, _lonMax, _latMin, _latMax;
	int _labelColor, _zoomLevel;
	const RuleEvent* _signedPactEvent = nullptr;
	const RuleEvent* _rejoinedXcomEvent = nullptr;
	RuleBaseFacilityFunctions _provideBaseFunc = 0;
	RuleBaseFacilityFunctions _forbiddenBaseFunc = 0;

	ModScript::CountryScripts::Container _countryScripts;
	ScriptValues<RuleCountry> _scriptValues;
public:
	/// Name of class used in script.
	static constexpr const char* ScriptName = "RuleCountry";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Creates a blank country ruleset.
	RuleCountry(const std::string &type);
	/// Cleans up the country ruleset.
	~RuleCountry();
	/// Loads the country from YAML.
	void load(const YAML::YamlNodeReader& reader, const ModScript& parsers, Mod* mod);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);
	/// Gets the country's type.
	const std::string& getType() const;
	/// Gets the event triggered after the country leaves Xcom.
	const RuleEvent* getSignedPactEvent() const { return _signedPactEvent; }
	/// Gets the event triggered after the country rejoins Xcom.
	const RuleEvent* getRejoinedXcomEvent() const { return _rejoinedXcomEvent; }
	/// Generates the country's starting funding.
	int generateFunding() const;
	/// Gets the country's funding cap.
	int getFundingCap() const;
	/// Gets the country's label X position.
	double getLabelLongitude() const;
	/// Gets the country's label Y position.
	double getLabelLatitude() const;
	/// Checks if a point is inside the country.
	bool insideCountry(double lon, double lat) const;
	const std::vector<double> &getLonMax() const { return _lonMax; }
	const std::vector<double> &getLonMin() const { return _lonMin; }
	const std::vector<double> &getLatMax() const { return _latMax; }
	const std::vector<double> &getLatMin() const { return _latMin; }
	/// Gets the country's label color.
	int getLabelColor() const;
	/// Gets the minimum zoom level required to display the label (Note: works for extraGlobeLabels only, not for vanilla countries).
	int getZoomLevel() const;
	/// Gets the functions provided by the country.
	RuleBaseFacilityFunctions getProvidedBaseFunc() const { return _provideBaseFunc; }
	/// Gets the functions forbidden by the coutry.
	RuleBaseFacilityFunctions getForbiddenBaseFunc() const { return _forbiddenBaseFunc; }

	/// Gets script.
	template<typename Script>
	const typename Script::Container& getScript() const { return _countryScripts.get<Script>(); }
};

}
