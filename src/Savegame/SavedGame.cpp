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
#include "SavedGame.h"
#include <sstream>
#include <set>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <yaml-cpp/yaml.h>
#include "../version.h"
#include "../Engine/Logger.h"
#include "../Mod/Mod.h"
#include "../Engine/RNG.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/ScriptBind.h"
#include "SavedBattleGame.h"
#include "SerializationHelper.h"
#include "GameTime.h"
#include "Country.h"
#include "Base.h"
#include "Craft.h"
#include "EquipmentLayoutItem.h"
#include "Region.h"
#include "Ufo.h"
#include "Waypoint.h"
#include "../Mod/RuleResearch.h"
#include "ResearchProject.h"
#include "ItemContainer.h"
#include "Soldier.h"
#include "Transfer.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "Production.h"
#include "MissionSite.h"
#include "AlienBase.h"
#include "AlienStrategy.h"
#include "AlienMission.h"
#include "GeoscapeEvent.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleRegion.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/SoldierNamePool.h"
#include "BaseFacility.h"
#include "MissionStatistics.h"
#include "SoldierDeath.h"
#include "SoldierDiary.h"
#include "../Mod/AlienRace.h"

namespace OpenXcom
{

const std::string SavedGame::AUTOSAVE_GEOSCAPE = "_autogeo_.asav",
				  SavedGame::AUTOSAVE_BATTLESCAPE = "_autobattle_.asav",
				  SavedGame::QUICKSAVE = "_quick_.asav";

namespace
{

struct findRuleResearch
{
	typedef ResearchProject* argument_type;
	typedef bool result_type;

	RuleResearch * _toFind;
	findRuleResearch(RuleResearch * toFind);
	bool operator()(const ResearchProject *r) const;
};

findRuleResearch::findRuleResearch(RuleResearch * toFind) : _toFind(toFind)
{
}

bool findRuleResearch::operator()(const ResearchProject *r) const
{
	return _toFind == r->getRules();
}

struct equalProduction
{
	typedef Production* argument_type;
	typedef bool result_type;

	RuleManufacture * _item;
	equalProduction(RuleManufacture * item);
	bool operator()(const Production * p) const;
};

equalProduction::equalProduction(RuleManufacture * item) : _item(item)
{
}

bool equalProduction::operator()(const Production * p) const
{
	return p->getRules() == _item;
}

bool researchLess(const RuleResearch *a, const RuleResearch *b)
{
	return std::less<const RuleResearch *>{}(a, b);
}

template<typename T>
void sortReserchVector(std::vector<T> &vec)
{
	std::sort(vec.begin(), vec.end(), researchLess);
}

bool haveReserchVector(const std::vector<const RuleResearch*> &vec, const RuleResearch *res)
{
	auto find = std::lower_bound(vec.begin(), vec.end(), res, researchLess);
	return find != vec.end() && *find == res;
}

bool haveReserchVector(const std::vector<const RuleResearch*> &vec,  const std::string &res)
{
	auto find = std::find_if(vec.begin(), vec.end(), [&](const RuleResearch* r){ return r->getName() == res; });
	return find != vec.end();
}

}

/**
 * Initializes a brand new saved game according to the specified difficulty.
 */
SavedGame::SavedGame() :
	_difficulty(DIFF_BEGINNER), _end(END_NONE), _ironman(false), _globeLon(0.0), _globeLat(0.0), _globeZoom(0),
	_battleGame(0), _previewBase(nullptr), _debug(false), _warned(false),
	_togglePersonalLight(true), _toggleNightVision(false), _toggleBrightness(0),
	_monthsPassed(-1), _selectedBase(0), _autosales(), _disableSoldierEquipment(false), _alienContainmentChecked(false)
{
	_time = new GameTime(6, 1, 1, 1999, 12, 0, 0);
	_alienStrategy = new AlienStrategy();
	_funds.push_back(0);
	_maintenance.push_back(0);
	_researchScores.push_back(0);
	_incomes.push_back(0);
	_expenditures.push_back(0);
	_lastselectedArmor="STR_NONE_UC";

	for (int j = 0; j < MAX_CRAFT_LOADOUT_TEMPLATES; ++j)
	{
		_globalCraftLoadout[j] = new ItemContainer();
	}
}

/**
 * Deletes the game content from memory.
 */
SavedGame::~SavedGame()
{
	delete _time;
	for (std::vector<Country*>::iterator i = _countries.begin(); i != _countries.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Region*>::iterator i = _regions.begin(); i != _regions.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Base*>::iterator i = _bases.begin(); i != _bases.end(); ++i)
	{
		delete *i;
	}
	delete _previewBase;
	for (std::vector<Ufo*>::iterator i = _ufos.begin(); i != _ufos.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Waypoint*>::iterator i = _waypoints.begin(); i != _waypoints.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<MissionSite*>::iterator i = _missionSites.begin(); i != _missionSites.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<AlienBase*>::iterator i = _alienBases.begin(); i != _alienBases.end(); ++i)
	{
		delete *i;
	}
	delete _alienStrategy;
	for (std::vector<AlienMission*>::iterator i = _activeMissions.begin(); i != _activeMissions.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<GeoscapeEvent*>::iterator i = _geoscapeEvents.begin(); i != _geoscapeEvents.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<Soldier*>::iterator i = _deadSoldiers.begin(); i != _deadSoldiers.end(); ++i)
	{
		delete *i;
	}
	for (int j = 0; j < Options::oxceMaxEquipmentLayoutTemplates; ++j)
	{
		for (std::vector<EquipmentLayoutItem*>::iterator i = _globalEquipmentLayout[j].begin(); i != _globalEquipmentLayout[j].end(); ++i)
		{
			delete *i;
		}
	}
	for (int j = 0; j < MAX_CRAFT_LOADOUT_TEMPLATES; ++j)
	{
		delete _globalCraftLoadout[j];
	}
	for (std::vector<MissionStatistics*>::iterator i = _missionStatistics.begin(); i != _missionStatistics.end(); ++i)
	{
		delete *i;
	}

	delete _battleGame;
}

/**
 * Removes version number from a mod name, if any.
 * @param name Mod id from a savegame.
 * @return Sanitized mod name.
 */
std::string SavedGame::sanitizeModName(const std::string &name)
{
	size_t versionInfoBreakPoint = name.find(" ver: ");
	if (versionInfoBreakPoint == std::string::npos)
	{
		return name;
	}
	else
	{
		return name.substr(0, versionInfoBreakPoint);
	}
}

static bool _isCurrentGameType(const SaveInfo &saveInfo, const std::string &curMaster)
{
	bool matchMasterMod = false;
	if (saveInfo.mods.empty())
	{
		// if no mods listed in the savegame, this is an old-style
		// savegame.  assume "xcom1" as the game type.
		matchMasterMod = (curMaster == "xcom1");
	}
	else
	{
		for (std::vector<std::string>::const_iterator i = saveInfo.mods.begin(); i != saveInfo.mods.end(); ++i)
		{
			std::string name = SavedGame::sanitizeModName(*i);
			if (name == curMaster)
			{
				matchMasterMod = true;
				break;
			}
		}
	}

	if (!matchMasterMod)
	{
		Log(LOG_DEBUG) << "skipping save from inactive master: " << saveInfo.fileName;
	}

	return matchMasterMod;
}

/**
 * Gets all the info of the saves found in the user folder.
 * @param lang Loaded language.
 * @param autoquick Include autosaves and quicksaves.
 * @return List of saves info.
 */
std::vector<SaveInfo> SavedGame::getList(Language *lang, bool autoquick)
{
	std::vector<SaveInfo> info;
	std::string curMaster = Options::getActiveMaster();
	auto saves = CrossPlatform::getFolderContents(Options::getMasterUserFolder(), "sav");

	if (autoquick)
	{
		auto asaves = CrossPlatform::getFolderContents(Options::getMasterUserFolder(), "asav");
		saves.insert(saves.begin(), asaves.begin(), asaves.end());
	}
	for (auto i = saves.begin(); i != saves.end(); ++i)
	{
		auto filename = std::get<0>(*i);
		try
		{
			SaveInfo saveInfo = getSaveInfo(filename, lang);
			if (!_isCurrentGameType(saveInfo, curMaster))
			{
				continue;
			}
			info.push_back(saveInfo);
		}
		catch (Exception &e)
		{
			Log(LOG_ERROR) << filename << ": " << e.what();
			continue;
		}
		catch (YAML::Exception &e)
		{
			Log(LOG_ERROR) << filename << ": " << e.what();
			continue;
		}
	}

	return info;
}

/**
 * Gets the info of a specific save file.
 * @param file Save filename.
 * @param lang Loaded language.
 */
SaveInfo SavedGame::getSaveInfo(const std::string &file, Language *lang)
{
	std::string fullname = Options::getMasterUserFolder() + file;
	YAML::Node doc = YAML::Load(*CrossPlatform::getYamlSaveHeader(fullname));
	SaveInfo save;

	save.fileName = file;

	if (save.fileName == QUICKSAVE)
	{
		save.displayName = lang->getString("STR_QUICK_SAVE_SLOT");
		save.reserved = true;
	}
	else if (save.fileName == AUTOSAVE_GEOSCAPE)
	{
		save.displayName = lang->getString("STR_AUTO_SAVE_GEOSCAPE_SLOT");
		save.reserved = true;
	}
	else if (save.fileName == AUTOSAVE_BATTLESCAPE)
	{
		save.displayName = lang->getString("STR_AUTO_SAVE_BATTLESCAPE_SLOT");
		save.reserved = true;
	}
	else if (save.fileName.find(AUTOSAVE_BATTLESCAPE) != std::string::npos)
	{
		int turn = 0;
		if (doc["turn"])
		{
			turn = doc["turn"].as<int>(turn);
		}
		save.displayName = lang->getString("STR_AUTO_SAVE_BATTLESCAPE_SLOT_WITH_NUMBER").arg(turn);
		save.reserved = true;
	}
	else
	{
		if (doc["name"])
		{
			save.displayName = doc["name"].as<std::string>();
		}
		else
		{
			save.displayName = CrossPlatform::noExt(file);
		}
		save.reserved = false;
	}

	save.timestamp = CrossPlatform::getDateModified(fullname);
	std::pair<std::string, std::string> str = CrossPlatform::timeToString(save.timestamp);
	save.isoDate = str.first;
	save.isoTime = str.second;
	save.mods = doc["mods"].as<std::vector< std::string> >(std::vector<std::string>());

	std::ostringstream details;
	if (doc["turn"])
	{
		details << lang->getString("STR_BATTLESCAPE") << ": " << lang->getString(doc["mission"].as<std::string>()) << ", ";
		details << lang->getString("STR_TURN").arg(doc["turn"].as<int>());
	}
	else
	{
		GameTime time = GameTime(6, 1, 1, 1999, 12, 0, 0);
		time.load(doc["time"]);
		details << lang->getString("STR_GEOSCAPE") << ": ";
		details << time.getDayString(lang) << " " << lang->getString(time.getMonthString()) << " " << time.getYear() << ", ";
		details << time.getHour() << ":" << std::setfill('0') << std::setw(2) << time.getMinute();
	}
	if (doc["ironman"].as<bool>(false))
	{
		details << " (" << lang->getString("STR_IRONMAN") << ")";
	}
	save.details = details.str();

	return save;
}

/**
 * Loads a saved game's contents from a YAML file.
 * @note Assumes the saved game is blank.
 * @param filename YAML filename.
 * @param mod Mod for the saved game.
 * @param lang Loaded language.
 */
void SavedGame::load(const std::string &filename, Mod *mod, Language *lang)
{
	std::string filepath = Options::getMasterUserFolder() + filename;
	std::vector<YAML::Node> file = YAML::LoadAll(*CrossPlatform::readFile(filepath));
	// Get brief save info
	YAML::Node brief = file[0];
	_time->load(brief["time"]);
	if (brief["name"])
	{
		_name = brief["name"].as<std::string>();
	}
	else
	{
		_name = filename;
	}
	_ironman = brief["ironman"].as<bool>(_ironman);

	// Get full save data
	YAML::Node doc = file[1];
	_difficulty = (GameDifficulty)doc["difficulty"].as<int>(_difficulty);
	_end = (GameEnding)doc["end"].as<int>(_end);
	if (doc["rng"] && (_ironman || !Options::newSeedOnLoad))
		RNG::setSeed(doc["rng"].as<uint64_t>());
	_monthsPassed = doc["monthsPassed"].as<int>(_monthsPassed);
	_graphRegionToggles = doc["graphRegionToggles"].as<std::string>(_graphRegionToggles);
	_graphCountryToggles = doc["graphCountryToggles"].as<std::string>(_graphCountryToggles);
	_graphFinanceToggles = doc["graphFinanceToggles"].as<std::string>(_graphFinanceToggles);
	_funds = doc["funds"].as< std::vector<int64_t> >(_funds);
	_maintenance = doc["maintenance"].as< std::vector<int64_t> >(_maintenance);
	_userNotes = doc["userNotes"].as< std::vector<std::string> >(_userNotes);
	_researchScores = doc["researchScores"].as< std::vector<int> >(_researchScores);
	_incomes = doc["incomes"].as< std::vector<int64_t> >(_incomes);
	_expenditures = doc["expenditures"].as< std::vector<int64_t> >(_expenditures);
	_warned = doc["warned"].as<bool>(_warned);
	_togglePersonalLight = doc["togglePersonalLight"].as<bool>(_togglePersonalLight);
	_toggleNightVision = doc["toggleNightVision"].as<bool>(_toggleNightVision);
	_toggleBrightness = doc["toggleBrightness"].as<int>(_toggleBrightness);
	_globeLon = doc["globeLon"].as<double>(_globeLon);
	_globeLat = doc["globeLat"].as<double>(_globeLat);
	_globeZoom = doc["globeZoom"].as<int>(_globeZoom);
	_ids = doc["ids"].as< std::map<std::string, int> >(_ids);

	for (YAML::const_iterator i = doc["countries"].begin(); i != doc["countries"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (mod->getCountry(type))
		{
			Country *c = new Country(mod->getCountry(type), false);
			c->load(*i);
			_countries.push_back(c);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load country " << type;
		}
	}

	for (YAML::const_iterator i = doc["regions"].begin(); i != doc["regions"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (mod->getRegion(type))
		{
			Region *r = new Region(mod->getRegion(type));
			r->load(*i);
			_regions.push_back(r);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load region " << type;
		}
	}

	// Alien bases must be loaded before alien missions
	for (YAML::const_iterator i = doc["alienBases"].begin(); i != doc["alienBases"].end(); ++i)
	{
		std::string deployment = (*i)["deployment"].as<std::string>("STR_ALIEN_BASE_ASSAULT");
		if (mod->getDeployment(deployment))
		{
			AlienBase *b = new AlienBase(mod->getDeployment(deployment), 0);
			b->load(*i);
			_alienBases.push_back(b);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load deployment for alien base " << deployment;
		}
	}

	// Missions must be loaded before UFOs.
	const YAML::Node &missions = doc["alienMissions"];
	for (YAML::const_iterator it = missions.begin(); it != missions.end(); ++it)
	{
		std::string missionType = (*it)["type"].as<std::string>();
		if (mod->getAlienMission(missionType))
		{
			const RuleAlienMission &mRule = *mod->getAlienMission(missionType);
			AlienMission *mission = new AlienMission(mRule);
			mission->load(*it, *this, mod);
			_activeMissions.push_back(mission);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load mission " << missionType;
		}
	}

	for (YAML::const_iterator i = doc["ufos"].begin(); i != doc["ufos"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (mod->getUfo(type))
		{
			Ufo *u = new Ufo(mod->getUfo(type), 0);
			u->load(*i, mod->getScriptGlobal(), *mod, *this);
			_ufos.push_back(u);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load UFO " << type;
		}
	}

	const YAML::Node &geoEvents = doc["geoscapeEvents"];
	for (YAML::const_iterator it = geoEvents.begin(); it != geoEvents.end(); ++it)
	{
		std::string eventName = (*it)["name"].as<std::string>();
		if (mod->getEvent(eventName))
		{
			const RuleEvent &eventRule = *mod->getEvent(eventName);
			GeoscapeEvent *event = new GeoscapeEvent(eventRule);
			event->load(*it);
			_geoscapeEvents.push_back(event);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load geoscape event " << eventName;
		}
	}

	for (YAML::const_iterator i = doc["waypoints"].begin(); i != doc["waypoints"].end(); ++i)
	{
		Waypoint *w = new Waypoint();
		w->load(*i);
		_waypoints.push_back(w);
	}

	// Backwards compatibility
	for (YAML::const_iterator i = doc["terrorSites"].begin(); i != doc["terrorSites"].end(); ++i)
	{
		std::string type = "STR_ALIEN_TERROR";
		std::string deployment = "STR_TERROR_MISSION";
		if (mod->getAlienMission(type) && mod->getDeployment(deployment))
		{
			MissionSite *m = new MissionSite(mod->getAlienMission(type), mod->getDeployment(deployment), nullptr);
			m->load(*i);
			_missionSites.push_back(m);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load mission " << type << " deployment " << deployment;
		}
	}

	for (YAML::const_iterator i = doc["missionSites"].begin(); i != doc["missionSites"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		std::string deployment = (*i)["deployment"].as<std::string>("STR_TERROR_MISSION");
		std::string alienWeaponDeploy = (*i)["missionCustomDeploy"].as<std::string>("");
		if (mod->getAlienMission(type) && mod->getDeployment(deployment))
		{
			MissionSite *m = new MissionSite(mod->getAlienMission(type), mod->getDeployment(deployment), mod->getDeployment(alienWeaponDeploy));
			m->load(*i);
			_missionSites.push_back(m);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load mission " << type << " deployment " << deployment;
		}
	}

	// Discovered Techs Should be loaded before Bases (e.g. for PSI evaluation)
	for (YAML::const_iterator it = doc["discovered"].begin(); it != doc["discovered"].end(); ++it)
	{
		std::string research = it->as<std::string>();
		if (mod->getResearch(research))
		{
			_discovered.push_back(mod->getResearch(research));
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load research " << research;
		}
	}
	sortReserchVector(_discovered);

	_generatedEvents = doc["generatedEvents"].as< std::map<std::string, int> >(_generatedEvents);
	_ufopediaRuleStatus = doc["ufopediaRuleStatus"].as< std::map<std::string, int> >(_ufopediaRuleStatus);
	_manufactureRuleStatus = doc["manufactureRuleStatus"].as< std::map<std::string, int> >(_manufactureRuleStatus);
	_researchRuleStatus = doc["researchRuleStatus"].as< std::map<std::string, int> >(_researchRuleStatus);
	_monthlyPurchaseLimitLog = doc["monthlyPurchaseLimitLog"].as< std::map<std::string, int> >(_monthlyPurchaseLimitLog);
	_hiddenPurchaseItemsMap = doc["hiddenPurchaseItems"].as< std::map<std::string, bool> >(_hiddenPurchaseItemsMap);
	_customRuleCraftDeployments = doc["customRuleCraftDeployments"].as< std::map<std::string, RuleCraftDeployment > >(_customRuleCraftDeployments);

	for (YAML::const_iterator i = doc["bases"].begin(); i != doc["bases"].end(); ++i)
	{
		Base *b = new Base(mod);
		b->load(*i, this, false);
		_bases.push_back(b);
	}

	// Finish loading crafts after bases (more specifically after all crafts) are loaded, because of references between crafts (i.e. friendly escorts)
	{
		for (YAML::const_iterator i = doc["bases"].begin(); i != doc["bases"].end(); ++i)
		{
			// Bases don't have IDs and names are not unique, so need to consider lon/lat too
			double lon = (*i)["lon"].as<double>(0.0);
			double lat = (*i)["lat"].as<double>(0.0);
			std::string baseName = "";
			if (const YAML::Node &name = (*i)["name"])
			{
				baseName = name.as<std::string>();
			}

			Base *base = 0;
			for (std::vector<Base*>::iterator b = _bases.begin(); b != _bases.end(); ++b)
			{
				if (AreSame(lon, (*b)->getLongitude()) && AreSame(lat, (*b)->getLatitude()) && (*b)->getName() == baseName)
				{
					base = (*b);
					break;
				}
			}
			if (base)
			{
				base->finishLoading(*i, this);
			}
		}
	}

	// Finish loading UFOs after all craft and all other UFOs are loaded
	for (YAML::const_iterator i = doc["ufos"].begin(); i != doc["ufos"].end(); ++i)
	{
		int uniqueUfoId = (*i)["uniqueId"].as<int>(0);
		if (uniqueUfoId > 0)
		{
			Ufo *ufo = 0;
			for (std::vector<Ufo*>::iterator u = _ufos.begin(); u != _ufos.end(); ++u)
			{
				if ((*u)->getUniqueId() == uniqueUfoId)
				{
					ufo = (*u);
					break;
				}
			}
			if (ufo)
			{
				ufo->finishLoading(*i, *this);
			}
		}
	}

	const YAML::Node &research = doc["poppedResearch"];
	for (YAML::const_iterator it = research.begin(); it != research.end(); ++it)
	{
		std::string id = it->as<std::string>();
		if (mod->getResearch(id))
		{
			_poppedResearch.push_back(mod->getResearch(id));
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load research " << id;
		}
	}
	_alienStrategy->load(doc["alienStrategy"], mod);

	for (YAML::const_iterator i = doc["deadSoldiers"].begin(); i != doc["deadSoldiers"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>(mod->getSoldiersList().front());
		if (mod->getSoldier(type))
		{
			Soldier *soldier = new Soldier(mod->getSoldier(type), nullptr, 0 /*nationality*/);
			soldier->load(*i, mod, this, mod->getScriptGlobal());
			_deadSoldiers.push_back(soldier);
		}
		else
		{
			Log(LOG_ERROR) << "Failed to load soldier " << type;
		}
	}

	for (int j = 0; j < Options::oxceMaxEquipmentLayoutTemplates; ++j)
	{
		std::ostringstream oss;
		oss << "globalEquipmentLayout" << j;
		std::string key = oss.str();
		if (const YAML::Node &layout = doc[key])
		{
			for (YAML::const_iterator i = layout.begin(); i != layout.end(); ++i)
			{
				EquipmentLayoutItem *layoutItem = new EquipmentLayoutItem(*i);

				// check if everything still exists (in case of mod upgrades)
				bool error = false;
				if (!mod->getInventory(layoutItem->getSlot()))
					error = true;
				if (!mod->getItem(layoutItem->getItemType()))
					error = true;
				for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
				{
					if (layoutItem->getAmmoItemForSlot(slot) == "NONE" || mod->getItem(layoutItem->getAmmoItemForSlot(slot)))
					{
						// ok
					}
					else
					{
						error = true;
						break;
					}
				}
				if (!error)
				{
					_globalEquipmentLayout[j].push_back(layoutItem);
				}
				else
				{
					delete layoutItem;
				}
			}
		}
		std::ostringstream oss2;
		oss2 << "globalEquipmentLayoutName" << j;
		std::string key2 = oss2.str();
		if (doc[key2])
		{
			_globalEquipmentLayoutName[j] = doc[key2].as<std::string>();
		}
		std::ostringstream oss3;
		oss3 << "globalEquipmentLayoutArmor" << j;
		std::string key3 = oss3.str();
		if (doc[key3])
		{
			_globalEquipmentLayoutArmor[j] = doc[key3].as<std::string>();
		}
	}

	for (int j = 0; j < MAX_CRAFT_LOADOUT_TEMPLATES; ++j)
	{
		std::ostringstream oss;
		oss << "globalCraftLoadout" << j;
		std::string key = oss.str();
		if (const YAML::Node &loadout = doc[key])
		{
			_globalCraftLoadout[j]->load(loadout);
		}
		std::ostringstream oss2;
		oss2 << "globalCraftLoadoutName" << j;
		std::string key2 = oss2.str();
		if (doc[key2])
		{
			_globalCraftLoadoutName[j] = doc[key2].as<std::string>();
		}
	}

	for (YAML::const_iterator i = doc["missionStatistics"].begin(); i != doc["missionStatistics"].end(); ++i)
	{
		MissionStatistics *ms = new MissionStatistics();
		ms->load(*i);
		_missionStatistics.push_back(ms);
	}

	for (YAML::const_iterator it = doc["autoSales"].begin(); it != doc["autoSales"].end(); ++it)
	{
		std::string itype = it->as<std::string>();
		if (mod->getItem(itype))
		{
			_autosales.insert(mod->getItem(itype));
		}
	}

	if (const YAML::Node &battle = doc["battleGame"])
	{
		_battleGame = new SavedBattleGame(mod, lang);
		_battleGame->load(battle, mod, this);
	}

	_scriptValues.load(doc, mod->getScriptGlobal());
}

/**
 * Saves a saved game's contents to a YAML file.
 * @param filename YAML filename.
 */
void SavedGame::save(const std::string &filename, Mod *mod) const
{
	YAML::Emitter out;

	// Saves the brief game info used in the saves list
	YAML::Node brief;
	brief["name"] = _name;
	brief["version"] = OPENXCOM_VERSION_SHORT;
	brief["engine"] = OPENXCOM_VERSION_ENGINE;
	std::string git_sha = OPENXCOM_VERSION_GIT;
	if (!git_sha.empty() && git_sha[0] ==  '.')
	{
		git_sha.erase(0,1);
	}
	brief["build"] = git_sha;
	brief["time"] = _time->save();
	if (_battleGame != 0)
	{
		brief["mission"] = _battleGame->getMissionType();
		brief["target"] = _battleGame->getMissionTarget();
		brief["craftOrBase"] = _battleGame->getMissionCraftOrBase();
		brief["turn"] = _battleGame->getTurn();
	}

	// only save mods that work with the current master
	std::vector<const ModInfo*> activeMods = Options::getActiveMods();
	std::vector<std::string> modsList;
	for (std::vector<const ModInfo*>::const_iterator i = activeMods.begin(); i != activeMods.end(); ++i)
	{
		modsList.push_back((*i)->getId() + " ver: " + (*i)->getVersion());
	}
	brief["mods"] = modsList;
	if (_ironman)
		brief["ironman"] = _ironman;
	out << brief;
	// Saves the full game data to the save
	out << YAML::BeginDoc;
	YAML::Node node;
	node["difficulty"] = (int)_difficulty;
	node["end"] = (int)_end;
	node["monthsPassed"] = _monthsPassed;
	node["graphRegionToggles"] = _graphRegionToggles;
	node["graphCountryToggles"] = _graphCountryToggles;
	node["graphFinanceToggles"] = _graphFinanceToggles;
	node["rng"] = RNG::getSeed();
	node["funds"] = _funds;
	node["maintenance"] = _maintenance;
	node["userNotes"] = _userNotes;
	node["researchScores"] = _researchScores;
	node["incomes"] = _incomes;
	node["expenditures"] = _expenditures;
	node["warned"] = _warned;
	node["togglePersonalLight"] = _togglePersonalLight;
	node["toggleNightVision"] = _toggleNightVision;
	node["toggleBrightness"] = _toggleBrightness;
	node["globeLon"] = serializeDouble(_globeLon);
	node["globeLat"] = serializeDouble(_globeLat);
	node["globeZoom"] = _globeZoom;
	node["ids"] = _ids;
	for (std::vector<Country*>::const_iterator i = _countries.begin(); i != _countries.end(); ++i)
	{
		node["countries"].push_back((*i)->save());
	}
	for (std::vector<Region*>::const_iterator i = _regions.begin(); i != _regions.end(); ++i)
	{
		node["regions"].push_back((*i)->save());
	}
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); ++i)
	{
		node["bases"].push_back((*i)->save());
	}
	for (std::vector<Waypoint*>::const_iterator i = _waypoints.begin(); i != _waypoints.end(); ++i)
	{
		node["waypoints"].push_back((*i)->save());
	}
	for (std::vector<MissionSite*>::const_iterator i = _missionSites.begin(); i != _missionSites.end(); ++i)
	{
		node["missionSites"].push_back((*i)->save());
	}
	// Alien bases must be saved before alien missions.
	for (std::vector<AlienBase*>::const_iterator i = _alienBases.begin(); i != _alienBases.end(); ++i)
	{
		node["alienBases"].push_back((*i)->save());
	}
	// Missions must be saved before UFOs, but after alien bases.
	for (std::vector<AlienMission *>::const_iterator i = _activeMissions.begin(); i != _activeMissions.end(); ++i)
	{
		node["alienMissions"].push_back((*i)->save());
	}
	// UFOs must be after missions
	for (std::vector<Ufo*>::const_iterator i = _ufos.begin(); i != _ufos.end(); ++i)
	{
		node["ufos"].push_back((*i)->save(mod->getScriptGlobal(), getMonthsPassed() == -1));
	}
	for (std::vector<GeoscapeEvent *>::const_iterator i = _geoscapeEvents.begin(); i != _geoscapeEvents.end(); ++i)
	{
		node["geoscapeEvents"].push_back((*i)->save());
	}
	for (std::vector<const RuleResearch *>::const_iterator i = _discovered.begin(); i != _discovered.end(); ++i)
	{
		node["discovered"].push_back((*i)->getName());
	}
	for (std::vector<const RuleResearch *>::const_iterator i = _poppedResearch.begin(); i != _poppedResearch.end(); ++i)
	{
		node["poppedResearch"].push_back((*i)->getName());
	}
	node["generatedEvents"] = _generatedEvents;
	node["ufopediaRuleStatus"] = _ufopediaRuleStatus;
	node["manufactureRuleStatus"] = _manufactureRuleStatus;
	node["researchRuleStatus"] = _researchRuleStatus;
	node["monthlyPurchaseLimitLog"] = _monthlyPurchaseLimitLog;
	node["hiddenPurchaseItems"] = _hiddenPurchaseItemsMap;
	node["customRuleCraftDeployments"] = _customRuleCraftDeployments;
	node["alienStrategy"] = _alienStrategy->save();
	for (std::vector<Soldier*>::const_iterator i = _deadSoldiers.begin(); i != _deadSoldiers.end(); ++i)
	{
		node["deadSoldiers"].push_back((*i)->save(mod->getScriptGlobal()));
	}
	for (int j = 0; j < Options::oxceMaxEquipmentLayoutTemplates; ++j)
	{
		std::ostringstream oss;
		oss << "globalEquipmentLayout" << j;
		std::string key = oss.str();
		if (!_globalEquipmentLayout[j].empty())
		{
			for (std::vector<EquipmentLayoutItem*>::const_iterator i = _globalEquipmentLayout[j].begin(); i != _globalEquipmentLayout[j].end(); ++i)
				node[key].push_back((*i)->save());
		}
		std::ostringstream oss2;
		oss2 << "globalEquipmentLayoutName" << j;
		std::string key2 = oss2.str();
		if (!_globalEquipmentLayoutName[j].empty())
		{
			node[key2] = _globalEquipmentLayoutName[j];
		}
		std::ostringstream oss3;
		oss3 << "globalEquipmentLayoutArmor" << j;
		std::string key3 = oss3.str();
		if (!_globalEquipmentLayoutArmor[j].empty())
		{
			node[key3] = _globalEquipmentLayoutArmor[j];
		}
	}
	for (int j = 0; j < MAX_CRAFT_LOADOUT_TEMPLATES; ++j)
	{
		std::ostringstream oss;
		oss << "globalCraftLoadout" << j;
		std::string key = oss.str();
		if (!_globalCraftLoadout[j]->getContents()->empty())
		{
			node[key] = _globalCraftLoadout[j]->save();
		}
		std::ostringstream oss2;
		oss2 << "globalCraftLoadoutName" << j;
		std::string key2 = oss2.str();
		if (!_globalCraftLoadoutName[j].empty())
		{
			node[key2] = _globalCraftLoadoutName[j];
		}
	}
	if (Options::soldierDiaries)
	{
		for (std::vector<MissionStatistics*>::const_iterator i = _missionStatistics.begin(); i != _missionStatistics.end(); ++i)
		{
			node["missionStatistics"].push_back((*i)->save());
		}
	}
	for (std::set<const RuleItem*>::const_iterator i = _autosales.begin(); i != _autosales.end(); ++i)
	{
		node["autoSales"].push_back((*i)->getName());
	}
	// snapshot of the user options (just for debugging purposes)
	{
		YAML::Node tmpNode;
		for (auto& info : Options::getOptionInfo())
		{
			info.save(tmpNode);
		}
		node["options"] = tmpNode;
	}
	if (_battleGame != 0)
	{
		node["battleGame"] = _battleGame->save();
	}
	_scriptValues.save(node, mod->getScriptGlobal());

	out << node;


	std::string filepath = Options::getMasterUserFolder() + filename;
	if (!CrossPlatform::writeFile(filepath, out.c_str()))
	{
		throw Exception("Failed to save " + filepath);
	}
}

/**
 * Returns the game's name shown in Save screens.
 * @return Save name.
 */
std::string SavedGame::getName() const
{
	return _name;
}

/**
 * Changes the game's name shown in Save screens.
 * @param name New name.
 */
void SavedGame::setName(const std::string &name)
{
	_name = name;
}

/**
 * Returns the game's difficulty level.
 * @return Difficulty level.
 */
GameDifficulty SavedGame::getDifficulty() const
{
	return _difficulty;
}

/**
 * Changes the game's difficulty to a new level.
 * @param difficulty New difficulty.
 */
void SavedGame::setDifficulty(GameDifficulty difficulty)
{
	_difficulty = difficulty;
}

/**
 * Returns the game's difficulty coefficient based
 * on the current level.
 * @return Difficulty coefficient.
 */
int SavedGame::getDifficultyCoefficient() const
{
	return Mod::DIFFICULTY_COEFFICIENT[std::min((int)_difficulty, 4)];
}

/**
 * Returns the game's sell price coefficient based
 * on the current difficulty level.
 * @return Sell price coefficient.
 */
int SavedGame::getSellPriceCoefficient() const
{
	return Mod::SELL_PRICE_COEFFICIENT[std::min((int)_difficulty, 4)];
}

/**
 * Returns the game's current ending.
 * @return Ending state.
 */
GameEnding SavedGame::getEnding() const
{
	return _end;
}

/**
 * Changes the game's current ending.
 * @param end New ending.
 */
void SavedGame::setEnding(GameEnding end)
{
	_end = end;
}

/**
 * Returns if the game is set to ironman mode.
 * Ironman games cannot be manually saved.
 * @return Tony Stark
 */
bool SavedGame::isIronman() const
{
	return _ironman;
}

/**
 * Changes if the game is set to ironman mode.
 * Ironman games cannot be manually saved.
 * @param ironman Tony Stark
 */
void SavedGame::setIronman(bool ironman)
{
	_ironman = ironman;
}

/**
 * Returns the player's current funds.
 * @return Current funds.
 */
int64_t SavedGame::getFunds() const
{
	return _funds.back();
}

/**
 * Returns the player's funds for the last 12 months.
 * @return funds.
 */
std::vector<int64_t> &SavedGame::getFundsList()
{
	return _funds;
}

/**
 * Changes the player's funds to a new value.
 * @param funds New funds.
 */
void SavedGame::setFunds(int64_t funds)
{
	if (_funds.back() > funds)
	{
		_expenditures.back() += _funds.back() - funds;
	}
	else
	{
		_incomes.back() += funds - _funds.back();
	}
	_funds.back() = funds;
}

/**
 * Returns the current longitude of the Geoscape globe.
 * @return Longitude.
 */
double SavedGame::getGlobeLongitude() const
{
	return _globeLon;
}

/**
 * Changes the current longitude of the Geoscape globe.
 * @param lon Longitude.
 */
void SavedGame::setGlobeLongitude(double lon)
{
	_globeLon = lon;
}

/**
 * Returns the current latitude of the Geoscape globe.
 * @return Latitude.
 */
double SavedGame::getGlobeLatitude() const
{
	return _globeLat;
}

/**
 * Changes the current latitude of the Geoscape globe.
 * @param lat Latitude.
 */
void SavedGame::setGlobeLatitude(double lat)
{
	_globeLat = lat;
}

/**
 * Returns the current zoom level of the Geoscape globe.
 * @return Zoom level.
 */
int SavedGame::getGlobeZoom() const
{
	return _globeZoom;
}

/**
 * Changes the current zoom level of the Geoscape globe.
 * @param zoom Zoom level.
 */
void SavedGame::setGlobeZoom(int zoom)
{
	_globeZoom = zoom;
}

/**
 * Gives the player his monthly funds, taking in account
 * all maintenance and profit costs.
 */
void SavedGame::monthlyFunding()
{
	auto countryFunding = getCountryFunding();
	auto baseMaintenance = getBaseMaintenance();
	_funds.back() += (countryFunding - baseMaintenance);
	_funds.push_back(_funds.back());
	_maintenance.back() = baseMaintenance;
	_maintenance.push_back(0);
	_incomes.push_back(countryFunding);
	_expenditures.push_back(baseMaintenance);
	_researchScores.push_back(0);

	if (_incomes.size() > 12)
		_incomes.erase(_incomes.begin());
	if (_expenditures.size() > 12)
		_expenditures.erase(_expenditures.begin());
	if (_researchScores.size() > 12)
		_researchScores.erase(_researchScores.begin());
	if (_funds.size() > 12)
		_funds.erase(_funds.begin());
	if (_maintenance.size() > 12)
		_maintenance.erase(_maintenance.begin());
}

/**
 * Returns the current time of the game.
 * @return Pointer to the game time.
 */
GameTime *SavedGame::getTime() const
{
	return _time;
}

/**
 * Changes the current time of the game.
 * @param time Game time.
 */
void SavedGame::setTime(const GameTime& time)
{
	_time = new GameTime(time);
}

/**
 * Returns the latest ID for the specified object
 * and increases it.
 * @param name Object name.
 * @return Latest ID number.
 */
int SavedGame::getId(const std::string &name)
{
	std::map<std::string, int>::iterator i = _ids.find(name);
	if (i != _ids.end())
	{
		return i->second++;
	}
	else
	{
		_ids[name] = 1;
		return _ids[name]++;
	}
}

/**
 * Returns the last used ID for the specified object.
 * @param name Object name.
 * @return Last used ID number.
 */
int SavedGame::getLastId(const std::string& name)
{
	std::map<std::string, int>::iterator i = _ids.find(name);
	if (i != _ids.end())
	{
		return std::max(1, i->second - 1);
	}
	else
	{
		return 0;
	}
}

/**
 * Increase a custom counter.
 * @param name Counter name.
 */
void SavedGame::increaseCustomCounter(const std::string& name)
{
	if (!name.empty())
	{
		std::map<std::string, int>::iterator i = _ids.find(name);
		if (i != _ids.end())
		{
			i->second++;
		}
		else
		{
			_ids[name] = 2; // not a typo
		}
	}
}

/**
 * Decrease a custom counter.
 * @param name Counter name.
 */
void SavedGame::decreaseCustomCounter(const std::string& name)
{
	if (!name.empty())
	{
		std::map<std::string, int>::iterator i = _ids.find(name);
		if (i != _ids.end())
		{
			// don't go below "zero" (which is saved as one)
			if (i->second > 1)
			{
				i->second--;
			}
		}
		else
		{
			_ids[name] = 1; // not a typo
		}
	}
}

/**
* Resets the list of unique object IDs.
* @param ids New ID list.
*/
const std::map<std::string, int> &SavedGame::getAllIds() const
{
	return _ids;
}

/**
 * Resets the list of unique object IDs.
 * @param ids New ID list.
 */
void SavedGame::setAllIds(const std::map<std::string, int> &ids)
{
	_ids = ids;
}

/**
 * Returns the list of countries in the game world.
 * @return Pointer to country list.
 */
std::vector<Country*> *SavedGame::getCountries()
{
	return &_countries;
}

/**
 * Adds up the monthly funding of all the countries.
 * @return Total funding.
 */
int SavedGame::getCountryFunding() const
{
	int total = 0;
	for (std::vector<Country*>::const_iterator i = _countries.begin(); i != _countries.end(); ++i)
	{
		total += (*i)->getFunding().back();
	}
	return total;
}

/**
 * Returns the list of world regions.
 * @return Pointer to region list.
 */
std::vector<Region*> *SavedGame::getRegions()
{
	return &_regions;
}

/**
 * Returns the list of player bases.
 * @return Pointer to base list.
 */
std::vector<Base*> *SavedGame::getBases()
{
	return &_bases;
}

/**
 * Returns the last selected player base.
 * @return Pointer to base.
 */
Base *SavedGame::getSelectedBase()
{
	// in case a base was destroyed or something...
	if (_selectedBase < _bases.size())
	{
		return _bases.at(_selectedBase);
	}
	else
	{
		return _bases.front();
	}
}

/**
 * Sets the last selected player base.
 * @param base number of the base.
 */
void SavedGame::setSelectedBase(size_t base)
{
	_selectedBase = base;
}

/**
 * Returns an immutable list of player bases.
 * @return Pointer to base list.
 */
const std::vector<Base*> *SavedGame::getBases() const
{
	return &_bases;
}

/**
 * Adds up the monthly maintenance of all the bases.
 * @return Total maintenance.
 */
int SavedGame::getBaseMaintenance() const
{
	int total = 0;
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); ++i)
	{
		total += (*i)->getMonthlyMaintenace();
	}
	return total;
}

/**
 * Returns the list of alien UFOs.
 * @return Pointer to UFO list.
 */
std::vector<Ufo*> *SavedGame::getUfos()
{
	return &_ufos;
}

/**
 * Returns the list of alien UFOs.
 * @return Pointer to UFO list.
 */
const std::vector<Ufo*> *SavedGame::getUfos() const
{
	return &_ufos;
}

/**
 * Returns the list of craft waypoints.
 * @return Pointer to waypoint list.
 */
std::vector<Waypoint*> *SavedGame::getWaypoints()
{
	return &_waypoints;
}

/**
 * Returns the list of mission sites.
 * @return Pointer to mission site list.
 */
std::vector<MissionSite*> *SavedGame::getMissionSites()
{
	return &_missionSites;
}

/**
 * Get pointer to the battleGame object.
 * @return Pointer to the battleGame object.
 */
SavedBattleGame *SavedGame::getSavedBattle()
{
	return _battleGame;
}

/**
 * Set battleGame object.
 * @param battleGame Pointer to the battleGame object.
 */
void SavedGame::setBattleGame(SavedBattleGame *battleGame)
{
	delete _battleGame;
	_battleGame = battleGame;
}

/**
 * Sets the status of a ufopedia rule
 * @param ufopediaRule The rule ID
 * @param newStatus Status to be set
 */
void SavedGame::setUfopediaRuleStatus(const std::string &ufopediaRule, int newStatus)
{
	_ufopediaRuleStatus[ufopediaRule] = newStatus;
}

/**
 * Sets the status of a manufacture rule
 * @param manufactureRule The rule ID
 * @param newStatus Status to be set
 */
void SavedGame::setManufactureRuleStatus(const std::string &manufactureRule, int newStatus)
{
	_manufactureRuleStatus[manufactureRule] = newStatus;
}

/**
* Sets the status of a research rule
* @param researchRule The rule ID
* @param newStatus Status to be set
*/
void SavedGame::setResearchRuleStatus(const std::string &researchRule, int newStatus)
{
	_researchRuleStatus[researchRule] = newStatus;
}

/**
 * Sets the hidden status of a purchase item
 * @param purchase item name
 * @param hidden
 */
void SavedGame::setHiddenPurchaseItemsStatus(const std::string &itemName, bool hidden)
{
	_hiddenPurchaseItemsMap[itemName] = hidden;
}

/**
 * Get the map of hidden items
 * @return map
 */
const std::map<std::string, bool> &SavedGame::getHiddenPurchaseItems()
{
	return _hiddenPurchaseItemsMap;
}

/*
 * Selects a "getOneFree" topic for the given research rule.
 * @param research Pointer to the given research rule.
 * @return Pointer to the selected getOneFree topic. Nullptr, if nothing was selected.
 */
const RuleResearch* SavedGame::selectGetOneFree(const RuleResearch* research)
{
	if (!research->getGetOneFree().empty() || !research->getGetOneFreeProtected().empty())
	{
		std::vector<const RuleResearch*> possibilities;
		for (auto& free : research->getGetOneFree())
		{
			if (isResearchRuleStatusDisabled(free->getName()))
			{
				continue; // skip disabled topics
			}
			if (!isResearched(free, false))
			{
				possibilities.push_back(free);
			}
		}
		for (auto& itMap : research->getGetOneFreeProtected())
		{
			if (isResearched(itMap.first, false))
			{
				for (auto& itVector : itMap.second)
				{
					if (isResearchRuleStatusDisabled(itVector->getName()))
					{
						continue; // skip disabled topics
					}
					if (!isResearched(itVector, false))
					{
						possibilities.push_back(itVector);
					}
				}
			}
		}
		if (!possibilities.empty())
		{
			size_t pick = 0;
			if (!research->sequentialGetOneFree())
			{
				pick = RNG::generate(0, possibilities.size() - 1);
			}
			auto ret = possibilities.at(pick);
			return ret;
		}
	}
	return nullptr;
}

/*
 * Checks for and removes a research project from the "already discovered" list
 * @param research is the project we are checking for and removing, if necessary.
 */
void SavedGame::removeDiscoveredResearch(const RuleResearch * research)
{
	std::vector<const RuleResearch*>::iterator r = std::find(_discovered.begin(), _discovered.end(), research);
	if (r != _discovered.end())
	{
		_discovered.erase(r);
	}
}

/**
 * Add a ResearchProject to the list of already discovered ResearchProject
 * @param research The newly found ResearchProject
 */
void SavedGame::addFinishedResearchSimple(const RuleResearch * research)
{
	_discovered.push_back(research);
	sortReserchVector(_discovered);
}

/**
 * Add a ResearchProject to the list of already discovered ResearchProject
 * @param research The newly found ResearchProject
 * @param mod the game Mod
 * @param base the base, in which the project was finished
 * @param score should the score be awarded or not?
 */
void SavedGame::addFinishedResearch(const RuleResearch * research, const Mod * mod, Base * base, bool score)
{
	// process "re-enables"
	for (auto& ree : research->getReenabled())
	{
		if (isResearchRuleStatusDisabled(ree->getName()))
		{
			setResearchRuleStatus(ree->getName(), RuleResearch::RESEARCH_STATUS_NEW); // reset status
		}
	}

	if (isResearchRuleStatusDisabled(research->getName()))
	{
		return;
	}

	// Not really a queue in C++ terminology (we don't need or want pop_front())
	std::vector<const RuleResearch *> queue;
	queue.push_back(research);

	size_t currentQueueIndex = 0;
	while (queue.size() > currentQueueIndex)
	{
		const RuleResearch *currentQueueItem = queue.at(currentQueueIndex);

		// 1. Find out and remember if the currentQueueItem has any undiscovered non-disabled "protected unlocks" or "getOneFree"
		bool hasUndiscoveredProtectedUnlocks = hasUndiscoveredProtectedUnlock(currentQueueItem, mod);
		bool hasAnyUndiscoveredGetOneFrees = hasUndiscoveredGetOneFree(currentQueueItem, false);

		// 2. If the currentQueueItem was *not* already discovered before, add it to discovered research
		bool checkRelatedZeroCostTopics = true;
		if (!isResearched(currentQueueItem, false))
		{
			_discovered.push_back(currentQueueItem);
			sortReserchVector(_discovered);
			if (!hasUndiscoveredProtectedUnlocks && !hasAnyUndiscoveredGetOneFrees)
			{
				// If the currentQueueItem can't tell you anything anymore, remove it from popped research
				// Note: this is for optimisation purposes only, functionally it is *not* required...
				// ... removing it prematurely leads to bugs, maybe we should not do it at all?
				removePoppedResearch(currentQueueItem);
			}
			if (score)
			{
				addResearchScore(currentQueueItem->getPoints());
			}
			// process "disables"
			for (auto& dis : currentQueueItem->getDisabled())
			{
				removeDiscoveredResearch(dis); // unresearch
				setResearchRuleStatus(dis->getName(), RuleResearch::RESEARCH_STATUS_DISABLED); // mark as permanently disabled
			}
		}
		else
		{
			// If the currentQueueItem *was* already discovered before, check if it has any undiscovered "protected unlocks".
			// If not, all zero-cost topics have already been processed before (during the first discovery)
			// and we can basically terminate here (i.e. skip step 3.).
			if (!hasUndiscoveredProtectedUnlocks)
			{
				checkRelatedZeroCostTopics = false;
			}
		}

		// 3. If currentQueueItem is completed for the *first* time, or if it has any undiscovered "protected unlocks",
		// process all related zero-cost topics
		if (checkRelatedZeroCostTopics)
		{
			// 3a. Gather all available research projects
			std::vector<RuleResearch *> availableResearch;
			if (base)
			{
				// Note: even if two different but related projects are finished in two different bases at the same time,
				// the algorithm is robust enough to treat them *sequentially* (i.e. as if one was researched first and the other second),
				// thus calling this method for *one* base only is enough
				getAvailableResearchProjects(availableResearch, mod, base);
			}
			else
			{
				// Used in vanilla save converter only
				getAvailableResearchProjects(availableResearch, mod, 0);
			}

			// 3b. Iterate through all available projects and add zero-cost projects to the processing queue
			for (const RuleResearch *itProjectToTest : availableResearch)
			{
				// We are only interested in zero-cost projects!
				if (itProjectToTest->getCost() == 0)
				{
					// We are only interested in *new* projects (i.e. not processed or scheduled for processing yet)
					bool isAlreadyInTheQueue = false;
					for (const RuleResearch *itQueue : queue)
					{
						if (itQueue->getName() == itProjectToTest->getName())
						{
							isAlreadyInTheQueue = true;
							break;
						}
					}

					if (!isAlreadyInTheQueue)
					{
						if (itProjectToTest->getRequirements().empty())
						{
							// no additional checks for "unprotected" topics
							queue.push_back(itProjectToTest);
						}
						else
						{
							// for "protected" topics, we need to check if the currentQueueItem can unlock it or not
							for (auto& itUnlocks : currentQueueItem->getUnlocked())
							{
								if (itProjectToTest == itUnlocks)
								{
									queue.push_back(itProjectToTest);
									break;
								}
							}
						}
					}
				}
			}
		}

		// 4. process remaining items in the queue
		++currentQueueIndex;
	}
}

/**
 *  Returns the list of already discovered ResearchProject
 * @return the list of already discovered ResearchProject
 */
const std::vector<const RuleResearch *> & SavedGame::getDiscoveredResearch() const
{
	return _discovered;
}

/**
 * Get the list of RuleResearch which can be researched in a Base.
 * @param projects the list of ResearchProject which are available.
 * @param mod the game Mod
 * @param base a pointer to a Base
 * @param considerDebugMode Should debug mode be considered or not.
 */
void SavedGame::getAvailableResearchProjects(std::vector<RuleResearch *> &projects, const Mod *mod, Base *base, bool considerDebugMode) const
{
	// This list is used for topics that can be researched even if *not all* dependencies have been discovered yet (e.g. STR_ALIEN_ORIGINS)
	// Note: all requirements of such topics *have to* be discovered though! This will be handled elsewhere.
	std::vector<const RuleResearch *> unlocked;
	for (const RuleResearch *research : _discovered)
	{
		for (auto& itUnlocked : research->getUnlocked())
		{
			unlocked.push_back(itUnlocked);
		}
		sortReserchVector(unlocked);
	}

	// Create a list of research topics available for research in the given base
	for (auto& pair : mod->getResearchMap())
	{
		// This research topic is permanently disabled, ignore it!
		if (isResearchRuleStatusDisabled(pair.first))
		{
			continue;
		}

		RuleResearch *research = pair.second;

		if ((considerDebugMode && _debug) || haveReserchVector(unlocked, research))
		{
			// Empty, these research topics are on the "unlocked list", *don't* check the dependencies!
		}
		else
		{
			// These items are not on the "unlocked list", we must check if "dependencies" are satisfied!
			if (!isResearched(research->getDependencies(), considerDebugMode))
			{
				continue;
			}
		}

		// Check if "requires" are satisfied
		// IMPORTANT: research topics with "requires" will NEVER be directly visible to the player anyway
		//   - there is an additional filter in NewResearchListState::fillProjectList(), see comments there for more info
		//   - there is an additional filter in NewPossibleResearchState::NewPossibleResearchState()
		//   - we do this check for other functionality using this method, namely SavedGame::addFinishedResearch()
		//     - Note: when called from there, parameter considerDebugMode = false
		if (!isResearched(research->getRequirements(), considerDebugMode))
		{
			continue;
		}

		// Remove the already researched topics from the list *UNLESS* they can still give you something more
		if (isResearched(research->getName(), false))
		{
			if (hasUndiscoveredGetOneFree(research, true))
			{
				// This research topic still has some more undiscovered non-disabled and *AVAILABLE* "getOneFree" topics, keep it!
			}
			else if (hasUndiscoveredProtectedUnlock(research, mod))
			{
				// This research topic still has one or more undiscovered non-disabled "protected unlocks", keep it!
			}
			else
			{
				// This topic can't give you anything else anymore, ignore it!
				continue;
			}
		}

		if (base)
		{
			// Check if this topic is already being researched in the given base
			const std::vector<ResearchProject *> & baseResearchProjects = base->getResearch();
			if (std::find_if(baseResearchProjects.begin(), baseResearchProjects.end(), findRuleResearch(research)) != baseResearchProjects.end())
			{
				continue;
			}

			// Check for needed item in the given base
			if (research->needItem() && base->getStorageItems()->getItem(research->getName()) == 0)
			{
				continue;
			}

			// Check for required buildings/functions in the given base
			if ((~base->getProvidedBaseFunc({}) & research->getRequireBaseFunc()).any())
			{
				continue;
			}
		}
		else
		{
			// Used in vanilla save converter only
			if (research->needItem() && research->getCost() == 0)
			{
				continue;
			}
		}

		// Hallelujah, all checks passed, add the research topic to the list
		projects.push_back(research);
	}
}

/**
 * Get the list of newly available research projects once a ResearchProject has been completed.
 * @param before the list of available RuleResearch before completing new research.
 * @param after the list of available RuleResearch after completing new research.
 * @param diff the list of newly available RuleResearch after completing new research (after - before).
 */
void SavedGame::getNewlyAvailableResearchProjects(std::vector<RuleResearch *> & before, std::vector<RuleResearch *> & after, std::vector<RuleResearch *> & diff) const
{
	// History lesson:
	// Completely rewritten the original recursive algorithm, because it was inefficient, unreadable and wrong
	// a/ inefficient: it could call SavedGame::getAvailableResearchProjects() way too many times
	// b/ unreadable: because of recursion
	// c/ wrong: could end in an endless loop! in two different ways! (not in vanilla, but in mods)

	// Note:
	// We could move the sorting of "before" vector right after its creation to optimize a little bit more.
	// But sorting a short list is negligible compared to other operations we had to do to get to this point.
	// So I decided to leave it here, so that it's 100% clear what's going on.
	sortReserchVector(before);
	sortReserchVector(after);
	std::set_difference(after.begin(), after.end(), before.begin(), before.end(), std::inserter(diff, diff.begin()), researchLess);
}

/**
 * Get the list of RuleManufacture which can be manufacture in a Base.
 * @param productions the list of Productions which are available.
 * @param mod the Game Mod
 * @param base a pointer to a Base
 */
void SavedGame::getAvailableProductions (std::vector<RuleManufacture *> & productions, const Mod * mod, Base * base, ManufacturingFilterType filter) const
{
	const std::vector<std::string> &items = mod->getManufactureList();
	const std::vector<Production *> &baseProductions = base->getProductions();
	auto baseFunc = base->getProvidedBaseFunc({});

	for (std::vector<std::string>::const_iterator iter = items.begin();
		iter != items.end();
		++iter)
	{
		RuleManufacture *m = mod->getManufacture(*iter);
		if (!isResearched(m->getRequirements()))
		{
			continue;
		}
		if (std::find_if (baseProductions.begin(), baseProductions.end(), equalProduction(m)) != baseProductions.end())
		{
			continue;
		}
		if ((~baseFunc & m->getRequireBaseFunc()).any())
		{
			if (filter != MANU_FILTER_FACILITY_REQUIRED)
				continue;
		}
		else
		{
			if (filter == MANU_FILTER_FACILITY_REQUIRED)
				continue;
		}

		productions.push_back(m);
	}
}

/**
 * Get the list of newly available manufacture projects once a ResearchProject has been completed. This function check for fake ResearchProject.
 * @param dependables the list of RuleManufacture which are now available.
 * @param research The RuleResearch which has just been discovered
 * @param mod the Game Mod
 * @param base a pointer to a Base
 */
void SavedGame::getDependableManufacture (std::vector<RuleManufacture *> & dependables, const RuleResearch *research, const Mod * mod, Base *) const
{
	const std::vector<std::string> &mans = mod->getManufactureList();
	for (std::vector<std::string>::const_iterator iter = mans.begin(); iter != mans.end(); ++iter)
	{
		// don't show previously unlocked (and seen!) manufacturing topics
		std::map<std::string, int>::const_iterator i = _manufactureRuleStatus.find(*iter);
		if (i != _manufactureRuleStatus.end())
		{
			if (i->second != RuleManufacture::MANU_STATUS_NEW)
				continue;
		}

		RuleManufacture *m = mod->getManufacture(*iter);
		const auto &reqs = m->getRequirements();
		if (isResearched(reqs) && std::find(reqs.begin(), reqs.end(), research) != reqs.end())
		{
			dependables.push_back(m);
		}
	}
}

/**
 * Get the list of RuleSoldierTransformation which can occur at a base.
 * @param transformations the list of Transformations which are available.
 * @param mod the Game Mod
 * @param base a pointer to a Base
 */
void SavedGame::getAvailableTransformations (std::vector<RuleSoldierTransformation *> & transformations, const Mod * mod, Base * base) const
{
	const std::vector<std::string> &items = mod->getSoldierTransformationList();
	auto baseFunc = base->getProvidedBaseFunc({});

	for (std::vector<std::string>::const_iterator iter = items.begin(); iter != items.end(); ++iter)
	{
		RuleSoldierTransformation *m = mod->getSoldierTransformation(*iter);
		if (!isResearched(m->getRequiredResearch()))
		{
			continue;
		}
		if ((~baseFunc & m->getRequiredBaseFuncs()).any())
		{
			continue;
		}

		transformations.push_back(m);
	}
}

/**
 * Get the list of newly available items to purchase once a ResearchProject has been completed.
 * @param dependables the list of RuleItem which are now available.
 * @param research The RuleResearch which has just been discovered
 * @param mod the Game Mod
 */
void SavedGame::getDependablePurchase(std::vector<RuleItem *> & dependables, const RuleResearch *research, const Mod * mod) const
{
	const std::vector<std::string> &itemlist = mod->getItemsList();
	for (std::vector<std::string>::const_iterator iter = itemlist.begin(); iter != itemlist.end(); ++iter)
	{
		RuleItem *item = mod->getItem(*iter);
		if (item->getBuyCost() != 0)
		{
			const std::vector<const RuleResearch *> &reqs = item->getRequirements();
			bool found = std::find(reqs.begin(), reqs.end(), research) != reqs.end();
			const std::vector<const RuleResearch *> &reqsBuy = item->getBuyRequirements();
			bool foundBuy = std::find(reqsBuy.begin(), reqsBuy.end(), research) != reqsBuy.end();
			if (found || foundBuy)
			{
				if (isResearched(item->getBuyRequirements()) && isResearched(item->getRequirements()))
				{
					dependables.push_back(item);
				}
			}
		}
	}
}

/**
 * Get the list of newly available craft to purchase/rent once a ResearchProject has been completed.
 * @param dependables the list of RuleCraft which are now available.
 * @param research The RuleResearch which has just been discovered
 * @param mod the Game Mod
 */
void SavedGame::getDependableCraft(std::vector<RuleCraft *> & dependables, const RuleResearch *research, const Mod * mod) const
{
	const std::vector<std::string> &craftlist = mod->getCraftsList();
	for (std::vector<std::string>::const_iterator iter = craftlist.begin(); iter != craftlist.end(); ++iter)
	{
		RuleCraft *craftItem = mod->getCraft(*iter);
		if (craftItem->getBuyCost() != 0)
		{
			const std::vector<std::string> &reqs = craftItem->getRequirements();
			if (std::find(reqs.begin(), reqs.end(), research->getName()) != reqs.end())
			{
				if (isResearched(craftItem->getRequirements()))
				{
					dependables.push_back(craftItem);
				}
			}
		}
	}
}

/**
 * Get the list of newly available facilities to build once a ResearchProject has been completed.
 * @param dependables the list of RuleBaseFacility which are now available.
 * @param research The RuleResearch which has just been discovered
 * @param mod the Game Mod
 */
void SavedGame::getDependableFacilities(std::vector<RuleBaseFacility *> & dependables, const RuleResearch *research, const Mod * mod) const
{
	const std::vector<std::string> &facilitylist = mod->getBaseFacilitiesList();
	for (std::vector<std::string>::const_iterator iter = facilitylist.begin(); iter != facilitylist.end(); ++iter)
	{
		RuleBaseFacility *facilityItem = mod->getBaseFacility(*iter);
		const std::vector<std::string> &reqs = facilityItem->getRequirements();
		if (std::find(reqs.begin(), reqs.end(), research->getName()) != reqs.end())
		{
			if (isResearched(facilityItem->getRequirements()))
			{
				dependables.push_back(facilityItem);
			}
		}
	}
}

/**
 * Gets the status of a ufopedia rule.
 * @param ufopediaRule Ufopedia rule ID.
 * @return Status (0=new, 1=normal).
 */
int SavedGame::getUfopediaRuleStatus(const std::string &ufopediaRule)
{
	return _ufopediaRuleStatus[ufopediaRule];
}

/**
 * Gets the status of a manufacture rule.
 * @param manufactureRule Manufacture rule ID.
 * @return Status (0=new, 1=normal, 2=hidden).
 */
int SavedGame::getManufactureRuleStatus(const std::string &manufactureRule)
{
	return _manufactureRuleStatus[manufactureRule];
}

/**
 * Is the research new?
 * @param researchRule Research rule ID.
 * @return True, if the research rule status is new.
 */
bool SavedGame::isResearchRuleStatusNew(const std::string &researchRule) const
{
	auto it = _researchRuleStatus.find(researchRule);
	if (it != _researchRuleStatus.end())
	{
		if (it->second != RuleResearch::RESEARCH_STATUS_NEW)
		{
			return false;
		}
	}
	return true; // no status = new
}

/**
 * Is the research permanently disabled?
 * @param researchRule Research rule ID.
 * @return True, if the research rule status is disabled.
 */
bool SavedGame::isResearchRuleStatusDisabled(const std::string &researchRule) const
{
	auto it = _researchRuleStatus.find(researchRule);
	if (it != _researchRuleStatus.end())
	{
		if (it->second == RuleResearch::RESEARCH_STATUS_DISABLED)
		{
			return true;
		}
	}
	return false;
}

/**
 * Returns if a research still has undiscovered non-disabled "getOneFree".
 * @param r Research to check.
 * @param checkOnlyAvailableTopics Check only available topics (=topics with discovered prerequisite) or all topics?
 * @return Whether it has any undiscovered non-disabled "getOneFree" or not.
 */
bool SavedGame::hasUndiscoveredGetOneFree(const RuleResearch * r, bool checkOnlyAvailableTopics) const
{
	if (!isResearched(r->getGetOneFree(), false, true))
	{
		return true; // found something undiscovered (and NOT disabled) already, no need to search further
	}
	else
	{
		// search through getOneFreeProtected topics too
		for (auto& itMap : r->getGetOneFreeProtected())
		{
			if (checkOnlyAvailableTopics && !isResearched(itMap.first, false))
			{
				// skip this group, its prerequisite has not been discovered yet
			}
			else
			{
				if (!isResearched(itMap.second, false, true))
				{
					return true; // found something undiscovered (and NOT disabled) already, no need to search further
				}
			}
		}
	}
	return false;
}

/**
 * Returns if a research still has undiscovered non-disabled "protected unlocks".
 * @param r Research to check.
 * @param mod the Game Mod
 * @return Whether it has any undiscovered non-disabled "protected unlocks" or not.
 */
bool SavedGame::hasUndiscoveredProtectedUnlock(const RuleResearch * r, const Mod * mod) const
{
	// Note: checking for not yet discovered unlocks protected by "requires" (which also implies cost = 0)
	for (auto& unlock : r->getUnlocked())
	{
		if (isResearchRuleStatusDisabled(unlock->getName()))
		{
			// ignore all disabled topics (as if they didn't exist)
			continue;
		}
		if (!unlock->getRequirements().empty())
		{
			if (!isResearched(unlock, false))
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * Returns if a certain research topic has been completed.
 * @param research Research ID.
 * @param considerDebugMode Should debug mode be considered or not.
 * @return Whether it's researched or not.
 */
bool SavedGame::isResearched(const std::string &research, bool considerDebugMode) const
{
	//if (research.empty())
	//	return true;
	if (considerDebugMode && _debug)
		return true;

	return haveReserchVector(_discovered, research);
}

bool SavedGame::isResearched(const RuleResearch *research, bool considerDebugMode) const
{
	//if (research.empty())
	//	return true;
	if (considerDebugMode && _debug)
		return true;

	return haveReserchVector(_discovered, research);
}

bool SavedGame::isResearched(const std::vector<std::string> &research, bool considerDebugMode) const
{
	if (research.empty())
		return true;
	if (considerDebugMode && _debug)
		return true;

	for (const std::string &r : research)
	{
		if (!haveReserchVector(_discovered, r))
		{
			return false;
		}
	}

	return true;
}

/**
 * Returns if a certain list of research topics has been completed.
 * @param research List of research IDs.
 * @param considerDebugMode Should debug mode be considered or not.
 * @param skipDisabled Should permanently disabled topics be considered or not.
 * @return Whether it's researched or not.
 */
bool SavedGame::isResearched(const std::vector<const RuleResearch *> &research, bool considerDebugMode, bool skipDisabled) const
{
	if (research.empty())
		return true;
	if (considerDebugMode && _debug)
		return true;
	std::vector<const RuleResearch *> matches = research;
	if (skipDisabled)
	{
		// ignore all disabled topics (as if they didn't exist)
		for (std::vector<const RuleResearch *>::iterator j = matches.begin(); j != matches.end();)
		{
			if (isResearchRuleStatusDisabled((*j)->getName()))
			{
				j = matches.erase(j);
			}
			else
			{
				++j;
			}
		}
	}

	for (auto& r : matches)
	{
		if (!haveReserchVector(_discovered, r))
		{
			return false;
		}
	}

	return true;
}

/**
 * Returns if a certain item has been obtained, i.e. is present directly in the base stores.
 * Items in and on craft, in transfer, worn by soldiers, etc. are ignored!!
 * @param itemType Item ID.
 * @return Whether it's obtained or not.
 */
bool SavedGame::isItemObtained(const std::string &itemType) const
{
	for (auto base : _bases)
	{
		if (base->getStorageItems()->getItem(itemType) > 0)
			return true;
	}
	return false;
}
/**
 * Returns if a certain facility has been built in any base.
 * @param facilityType facility ID.
 * @return Whether it's been built or not. If false, the facility has not been built in any base.
 */
bool SavedGame::isFacilityBuilt(const std::string &facilityType) const
{
	for (auto base : _bases)
	{
		for (auto fac : *base->getFacilities())
		{
			if (fac->getBuildTime() == 0 && fac->getRules()->getType() == facilityType)
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * Returns if a certain soldier type has been hired in any base.
 * @param soldierType soldier type ID.
 * @return Whether it's been hired (and arrived already) or not.
 */
bool SavedGame::isSoldierTypeHired(const std::string& soldierType) const
{
	for (auto* base : _bases)
	{
		for (auto* soldier : *base->getSoldiers())
		{
			if (soldier->getRules()->getType() == soldierType)
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * Returns pointer to the Soldier given it's unique ID.
 * @param id A soldier's unique id.
 * @return Pointer to Soldier.
 */
Soldier *SavedGame::getSoldier(int id) const
{
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); ++i)
	{
		for (std::vector<Soldier*>::const_iterator j = (*i)->getSoldiers()->begin(); j != (*i)->getSoldiers()->end(); ++j)
		{
			if ((*j)->getId() == id)
			{
				return (*j);
			}
		}
	}
	for (std::vector<Soldier*>::const_iterator j = _deadSoldiers.begin(); j != _deadSoldiers.end(); ++j)
	{
		if ((*j)->getId() == id)
		{
			return (*j);
		}
	}
	return 0;
}

/**
 * Handles the higher promotions (not the rookie-squaddie ones).
 * @param participants a list of soldiers that were actually present at the battle.
 * @param mod the Game Mod
 * @return Whether or not some promotions happened - to show the promotions screen.
 */
bool SavedGame::handlePromotions(std::vector<Soldier*> &participants, const Mod *mod)
{
	int soldiersPromoted = 0;
	Soldier *highestRanked = 0;
	PromotionInfo soldierData;
	std::vector<Soldier*> soldiers;
	for (std::vector<Base*>::iterator i = _bases.begin(); i != _bases.end(); ++i)
	{
		for (std::vector<Soldier*>::iterator j = (*i)->getSoldiers()->begin(); j != (*i)->getSoldiers()->end(); ++j)
		{
			soldiers.push_back(*j);
			processSoldier(*j, soldierData);
		}
		for (std::vector<Transfer*>::iterator j = (*i)->getTransfers()->begin(); j != (*i)->getTransfers()->end(); ++j)
		{
			if ((*j)->getType() == TRANSFER_SOLDIER)
			{
				soldiers.push_back((*j)->getSoldier());
				processSoldier((*j)->getSoldier(), soldierData);
			}
		}
	}

	int totalSoldiers = soldierData.totalSoldiers;

	if (soldierData.totalCommanders == 0)
	{
		if (totalSoldiers >= mod->getSoldiersPerCommander())
		{
			highestRanked = inspectSoldiers(soldiers, participants, RANK_COLONEL);
			if (highestRanked)
			{
				// only promote one colonel to commander
				highestRanked->promoteRank();
				soldiersPromoted++;
				soldierData.totalCommanders++;
				soldierData.totalColonels--;
			}
		}
	}

	if ((totalSoldiers / mod->getSoldiersPerColonel()) > soldierData.totalColonels)
	{
		while ((totalSoldiers / mod->getSoldiersPerColonel()) > soldierData.totalColonels)
		{
			highestRanked = inspectSoldiers(soldiers, participants, RANK_CAPTAIN);
			if (highestRanked)
			{
				highestRanked->promoteRank();
				soldiersPromoted++;
				soldierData.totalColonels++;
				soldierData.totalCaptains--;
			}
			else
			{
				break;
			}
		}
	}

	if ((totalSoldiers / mod->getSoldiersPerCaptain()) > soldierData.totalCaptains)
	{
		while ((totalSoldiers / mod->getSoldiersPerCaptain()) > soldierData.totalCaptains)
		{
			highestRanked = inspectSoldiers(soldiers, participants, RANK_SERGEANT);
			if (highestRanked)
			{
				highestRanked->promoteRank();
				soldiersPromoted++;
				soldierData.totalCaptains++;
				soldierData.totalSergeants--;
			}
			else
			{
				break;
			}
		}
	}

	if ((totalSoldiers / mod->getSoldiersPerSergeant()) > soldierData.totalSergeants)
	{
		while ((totalSoldiers / mod->getSoldiersPerSergeant()) > soldierData.totalSergeants)
		{
			highestRanked = inspectSoldiers(soldiers, participants, RANK_SQUADDIE);
			if (highestRanked)
			{
				highestRanked->promoteRank();
				soldiersPromoted++;
				soldierData.totalSergeants++;
			}
			else
			{
				break;
			}
		}
	}

	return (soldiersPromoted > 0);
}

/**
 * Processes a soldier, and adds their rank to the promotions data array.
 * @param soldier the soldier to process.
 * @param soldierData the data array to put their info into.
 */
void SavedGame::processSoldier(Soldier *soldier, PromotionInfo &soldierData)
{
	if (soldier->getRules()->getAllowPromotion())
	{
		soldierData.totalSoldiers++;
	}
	else
	{
		return;
	}

	switch (soldier->getRank())
	{
	case RANK_COMMANDER:
		soldierData.totalCommanders++;
		break;
	case RANK_COLONEL:
		soldierData.totalColonels++;
		break;
	case RANK_CAPTAIN:
		soldierData.totalCaptains++;
		break;
	case RANK_SERGEANT:
		soldierData.totalSergeants++;
		break;
	default:
		break;
	}
}

/**
 * Checks how many soldiers of a rank exist and which one has the highest score.
 * @param soldiers full list of live soldiers.
 * @param participants list of participants on this mission.
 * @param rank Rank to inspect.
 * @return the highest ranked soldier
 */
Soldier *SavedGame::inspectSoldiers(std::vector<Soldier*> &soldiers, std::vector<Soldier*> &participants, int rank)
{
	int highestScore = 0;
	Soldier *highestRanked = 0;
	for (std::vector<Soldier*>::iterator i = soldiers.begin(); i != soldiers.end(); ++i)
	{
		const std::vector<std::string> &rankStrings = (*i)->getRules()->getRankStrings();
		bool rankIsMatching = ((*i)->getRank() == rank);
		if (!rankStrings.empty())
		{
			// if rank is matching, but there are no more higher ranks defined for this soldier type, skip this soldier
			if (rankIsMatching && (rank >= (int)rankStrings.size() - 1))
			{
				rankIsMatching = false;
			}
		}
		if (rankIsMatching)
		{
			int score = getSoldierScore(*i);
			if (score > highestScore && (!Options::fieldPromotions || std::find(participants.begin(), participants.end(), *i) != participants.end()))
			{
				highestScore = score;
				highestRanked = (*i);
			}
		}
	}
	return highestRanked;
}

/**
 * Gets the (approximate) number of idle days since the soldier's last mission.
 */
int SavedGame::getSoldierIdleDays(Soldier *soldier)
{
	int lastMissionId = -1;
	int idleDays = 999;

	if (!soldier->getDiary()->getMissionIdList().empty())
	{
		lastMissionId = soldier->getDiary()->getMissionIdList().back();
	}

	if (lastMissionId == -1)
		return idleDays;

	for (auto missionInfo : _missionStatistics)
	{
		if (missionInfo->id == lastMissionId)
		{
			idleDays = 0;
			idleDays += (_time->getYear() - missionInfo->time.getYear()) * 365;
			idleDays += (_time->getMonth() - missionInfo->time.getMonth()) * 30;
			idleDays += (_time->getDay() - missionInfo->time.getDay()) * 1;
			break;
		}
	}

	if (idleDays > 999)
		idleDays = 999;

	return idleDays;
}

/**
 * Evaluate the score of a soldier based on all of his stats, missions and kills.
 * @param soldier the soldier to get a score for.
 * @return this soldier's score.
 */
int SavedGame::getSoldierScore(Soldier *soldier)
{
	UnitStats *s = soldier->getCurrentStats();
	int v1 = 2 * s->health + 2 * s->stamina + 4 * s->reactions + 4 * s->bravery;
	int v2 = v1 + 3*( s->tu + 2*( s->firing ) );
	int v3 = v2 + s->melee + s->throwing + s->strength;
	if (s->psiSkill > 0) v3 += s->psiStrength + 2 * s->psiSkill;
	return v3 + 10 * ( soldier->getMissions() + soldier->getKills() );
}

/**
  * Returns the list of alien bases.
  * @return Pointer to alien base list.
  */
std::vector<AlienBase*> *SavedGame::getAlienBases()
{
	return &_alienBases;
}

/**
 * Toggles debug mode.
 */
void SavedGame::setDebugMode()
{
	_debug = !_debug;
}

/**
 * Gets the current debug mode.
 * @return Debug mode.
 */
bool SavedGame::getDebugMode() const
{
	return _debug;
}

/**
 * Find a mission type in the active alien missions.
 * @param region The region string ID.
 * @param objective The active mission objective.
 * @param race The alien race used for more specific search (by type instead of objective). Optional.
 * @return A pointer to the mission, or 0 if no mission matched.
 */
AlienMission *SavedGame::findAlienMission(const std::string &region, MissionObjective objective, AlienRace* race) const
{
	if (race)
	{
		auto* retalWeights = race->retaliationMissionWeights(_monthsPassed);
		if (retalWeights)
		{
			auto retalNames = retalWeights->getNames();
			if (!retalNames.empty())
			{
				// if we made it this far, search by type and region
				for (auto& missionType : retalNames)
				{
					for (auto* mission : _activeMissions)
					{
						if (mission->getRules().getType() == missionType && mission->getRegion() == region)
						{
							return mission;
						}
					}
				}
				return 0;
			}
		}
	}

	for (auto* mission : _activeMissions)
	{
		if (mission->getRules().getObjective() == objective && mission->getRegion() == region)
		{
			return mission;
		}
	}
	return 0;
}

/**
 * return the list of monthly maintenance costs
 * @return list of maintenances.
 */
std::vector<int64_t> &SavedGame::getMaintenances()
{
	return _maintenance;
}

/**
 * adds to this month's research score
 * @param score the amount to add.
 */
void SavedGame::addResearchScore(int score)
{
	_researchScores.back() += score;
}

/**
 * return the list of research scores
 * @return list of research scores.
 */
std::vector<int> &SavedGame::getResearchScores()
{
	return _researchScores;
}

/**
 * return the list of income scores
 * @return list of income scores.
 */
std::vector<int64_t> &SavedGame::getIncomes()
{
	return _incomes;
}

/**
 * return the list of expenditures scores
 * @return list of expenditures scores.
 */
std::vector<int64_t> &SavedGame::getExpenditures()
{
	return _expenditures;
}

/**
 * return if the player has been
 * warned about poor performance.
 * @return true or false.
 */
bool SavedGame::getWarned() const
{
	return _warned;
}

/**
 * sets the player's "warned" status.
 * @param warned set "warned" to this.
 */
void SavedGame::setWarned(bool warned)
{
	_warned = warned;
}

/** @brief Check if a point is contained in a region.
 * This function object checks if a point is contained inside a region.
 */
class ContainsPoint
{
	typedef const Region* argument_type;
	typedef bool result_type;

public:
	/// Remember the coordinates.
	ContainsPoint(double lon, double lat) : _lon(lon), _lat(lat) { /* Empty by design. */ }
	/// Check is the region contains the stored point.
	bool operator()(const Region *region) const { return region->getRules()->insideRegion(_lon, _lat); }
private:
	double _lon, _lat;
};

/**
 * Find the region containing this location.
 * @param lon The longitude.
 * @param lat The latitude.
 * @return Pointer to the region, or 0.
 */
Region *SavedGame::locateRegion(double lon, double lat) const
{
	std::vector<Region *>::const_iterator found = std::find_if (_regions.begin(), _regions.end(), ContainsPoint(lon, lat));
	if (found != _regions.end())
	{
		return *found;
	}
	Log(LOG_ERROR) << "Failed to find a region at location [" << lon << ", " << lat << "].";
	return 0;
}

/**
 * Find the region containing this target.
 * @param target The target to locate.
 * @return Pointer to the region, or 0.
 */
Region *SavedGame::locateRegion(const Target &target) const
{
	return locateRegion(target.getLongitude(), target.getLatitude());
}

/** @brief Check if a point is contained in a country.
 * This function object checks if a point is contained inside a country.
 */
class CountryContainsPoint
{
	typedef const Country* argument_type;
	typedef bool result_type;

public:
	/// Remember the coordinates.
	CountryContainsPoint(double lon, double lat) : _lon(lon), _lat(lat) { /* Empty by design. */ }
	/// Check if the country contains the stored point.
	bool operator()(const Country* country) const { return country->getRules()->insideCountry(_lon, _lat); }
private:
	double _lon, _lat;
};

/**
 * Find the country containing this location.
 * @param lon The longitude.
 * @param lat The latitude.
 * @return Pointer to the country, or 0.
 */
Country* SavedGame::locateCountry(double lon, double lat) const
{
	std::vector<Country*>::const_iterator found = std::find_if(_countries.begin(), _countries.end(), CountryContainsPoint(lon, lat));
	if (found != _countries.end())
	{
		return *found;
	}
	//Log(LOG_DEBUG) << "Failed to find a country at location [" << lon << ", " << lat << "].";
	return 0;
}

/**
 * Find the country containing this target.
 * @param target The target to locate.
 * @return Pointer to the country, or 0.
 */
Country* SavedGame::locateCountry(const Target& target) const
{
	return locateCountry(target.getLongitude(), target.getLatitude());
}

/**
 * Select a soldier nationality based on mod rules and location on the globe.
 */
int SavedGame::selectSoldierNationalityByLocation(const Mod* mod, const RuleSoldier* rule, const Target* target) const
{
	if (!target)
	{
		return -1;
	}

	if (mod->getHireByCountryOdds() > 0 && RNG::percent(mod->getHireByCountryOdds()))
	{
		Country* country = locateCountry(*target);
		if (country)
		{
			int nationality = 0;
			for (auto* namepool : rule->getNames())
			{
				// we assume there is only one such name pool (or none), thus we stop searching on the first hit
				if (country->getRules()->getType() == namepool->getCountry())
				{
					return nationality;
				}
				++nationality;
			}
		}
	}

	if (mod->getHireByRegionOdds() > 0 && RNG::percent(mod->getHireByRegionOdds()))
	{
		Region* region = locateRegion(*target);
		if (region)
		{
			// build a new name pool collection, filtered by the region
			std::vector<std::pair<SoldierNamePool*, int> > filteredNames;
			int totalFilteredNamePoolWeight = 0;
			int nationality = 0;
			for (auto* namepool : rule->getNames())
			{
				if (region->getRules()->getType() == namepool->getRegion())
				{
					filteredNames.push_back(std::make_pair(namepool, nationality));
					totalFilteredNamePoolWeight += namepool->getGlobalWeight();
				}
				++nationality;
			}

			// select the nationality from the filtered pool, by weight
			int tmp = RNG::generate(0, totalFilteredNamePoolWeight);
			for (auto& namepoolPair : filteredNames)
			{
				if (tmp <= namepoolPair.first->getGlobalWeight())
				{
					return namepoolPair.second;
				}
				tmp -= namepoolPair.first->getGlobalWeight();
			}
		}
	}

	return -1;
}

/*
 * @return the month counter.
 */
int SavedGame::getMonthsPassed() const
{
	return _monthsPassed;
}

/*
 * @return the GraphRegionToggles.
 */
const std::string &SavedGame::getGraphRegionToggles() const
{
	return _graphRegionToggles;
}

/*
 * @return the GraphCountryToggles.
 */
const std::string &SavedGame::getGraphCountryToggles() const
{
	return _graphCountryToggles;
}

/*
 * @return the GraphFinanceToggles.
 */
const std::string &SavedGame::getGraphFinanceToggles() const
{
	return _graphFinanceToggles;
}

/**
 * Sets the GraphRegionToggles.
 * @param value The new value for GraphRegionToggles.
 */
void SavedGame::setGraphRegionToggles(const std::string &value)
{
	_graphRegionToggles = value;
}

/**
 * Sets the GraphCountryToggles.
 * @param value The new value for GraphCountryToggles.
 */
void SavedGame::setGraphCountryToggles(const std::string &value)
{
	_graphCountryToggles = value;
}

/**
 * Sets the GraphFinanceToggles.
 * @param value The new value for GraphFinanceToggles.
 */
void SavedGame::setGraphFinanceToggles(const std::string &value)
{
	_graphFinanceToggles = value;
}

/*
 * Increment the month counter.
 */
void SavedGame::addMonth()
{
	++_monthsPassed;

	_monthlyPurchaseLimitLog.clear();
}

/*
 * marks a research topic as having already come up as "we can now research"
 * @param research is the project we want to add to the vector
 */
void SavedGame::addPoppedResearch(const RuleResearch* research)
{
	if (!wasResearchPopped(research))
		_poppedResearch.push_back(research);
}

/*
 * checks if an unresearched topic has previously been popped up.
 * @param research is the project we are checking for
 * @return whether or not it has been popped up.
 */
bool SavedGame::wasResearchPopped(const RuleResearch* research)
{
	return (std::find(_poppedResearch.begin(), _poppedResearch.end(), research) != _poppedResearch.end());
}

/*
 * checks for and removes a research project from the "has been popped up" array
 * @param research is the project we are checking for and removing, if necessary.
 */
void SavedGame::removePoppedResearch(const RuleResearch* research)
{
	std::vector<const RuleResearch*>::iterator r = std::find(_poppedResearch.begin(), _poppedResearch.end(), research);
	if (r != _poppedResearch.end())
	{
		_poppedResearch.erase(r);
	}
}

/*
 * remembers that this event has been generated
 * @param event is the event we want to remember
 */
void SavedGame::addGeneratedEvent(const RuleEvent* event)
{
	_generatedEvents[event->getName()] += 1;
}

/*
 * checks if an event has been generated previously
 * @param eventName is the event we are checking for
 * @return whether or not it has been generated previously
 */
bool SavedGame::wasEventGenerated(const std::string& eventName)
{
	return (_generatedEvents.find(eventName) != _generatedEvents.end());
}

/**
 * Returns the list of dead soldiers.
 * @return Pointer to soldier list.
 */
std::vector<Soldier*> *SavedGame::getDeadSoldiers()
{
	return &_deadSoldiers;
}

/**
 * Sets the last selected armor.
 * @param value The new value for last selected armor - Armor type string.
 */

void SavedGame::setLastSelectedArmor(const std::string &value)
{
	_lastselectedArmor = value;
}

/**
 * Gets the last selected armor
 * @return last used armor type string
 */
std::string SavedGame::getLastSelectedArmor() const
{
	return _lastselectedArmor;
}

/**
* Returns the global equipment layout at specified index.
* @return Pointer to the EquipmentLayoutItem list.
*/
std::vector<EquipmentLayoutItem*> *SavedGame::getGlobalEquipmentLayout(int index)
{
	return &_globalEquipmentLayout[index];
}

/**
* Returns the name of a global equipment layout at specified index.
* @return A name.
*/
const std::string &SavedGame::getGlobalEquipmentLayoutName(int index) const
{
	return _globalEquipmentLayoutName[index];
}

/**
* Sets the name of a global equipment layout at specified index.
* @param index Array index.
* @param name New name.
*/
void SavedGame::setGlobalEquipmentLayoutName(int index, const std::string &name)
{
	_globalEquipmentLayoutName[index] = name;
}

/**
 * Returns the armor type of a global equipment layout at specified index.
 * @return Armor type.
 */
const std::string& SavedGame::getGlobalEquipmentLayoutArmor(int index) const
{
	return _globalEquipmentLayoutArmor[index];
}

/**
 * Sets the armor type of a global equipment layout at specified index.
 * @param index Array index.
 * @param armorType New armor type.
 */
void SavedGame::setGlobalEquipmentLayoutArmor(int index, const std::string& armorType)
{
	_globalEquipmentLayoutArmor[index] = armorType;
}

/**
* Returns the global craft loadout at specified index.
* @return Pointer to the ItemContainer list.
*/
ItemContainer *SavedGame::getGlobalCraftLoadout(int index)
{
	return _globalCraftLoadout[index];
}

/**
* Returns the name of a global craft loadout at specified index.
* @return A name.
*/
const std::string &SavedGame::getGlobalCraftLoadoutName(int index) const
{
	return _globalCraftLoadoutName[index];
}

/**
* Sets the name of a global craft loadout at specified index.
* @param index Array index.
* @param name New name.
*/
void SavedGame::setGlobalCraftLoadoutName(int index, const std::string &name)
{
	_globalCraftLoadoutName[index] = name;
}

/**
 * Returns the list of mission statistics.
 * @return Pointer to statistics list.
 */
std::vector<MissionStatistics*> *SavedGame::getMissionStatistics()
{
	return &_missionStatistics;
}

/**
* Adds a UFO to the ignore list.
* @param ufoId Ufo ID.
*/
void SavedGame::addUfoToIgnoreList(int ufoId)
{
	if (ufoId != 0)
	{
		_ignoredUfos.insert(ufoId);
	}
}

/**
* Checks if a UFO is on the ignore list.
* @param ufoId Ufo ID.
*/
bool SavedGame::isUfoOnIgnoreList(int ufoId)
{
	return _ignoredUfos.find(ufoId) != _ignoredUfos.end();
}

/**
 * Registers a soldier's death in the memorial.
 * @param soldier Pointer to dead soldier.
 * @param cause Pointer to cause of death, NULL if missing in action.
 */
std::vector<Soldier*>::iterator SavedGame::killSoldier(bool resetArmor, Soldier *soldier, BattleUnitKills *cause)
{
	if (resetArmor)
	{
		// OXCE: soldiers are buried in their default armor (...nicer stats in the Memorial GUI; no free armor if resurrected)
		soldier->setArmor(soldier->getRules()->getDefaultArmor());
		soldier->setReplacedArmor(0);
		soldier->setTransformedArmor(0);
	}
	else
	{
		// IMPORTANT: don't change the geoscape armor during the ongoing battle!
		// battlescape armor would reset to geoscape armor after save and reload
	}

	std::vector<Soldier*>::iterator j;
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); ++i)
	{
		for (j = (*i)->getSoldiers()->begin(); j != (*i)->getSoldiers()->end(); ++j)
		{
			if ((*j) == soldier)
			{
				soldier->die(new SoldierDeath(*_time, cause));
				_deadSoldiers.push_back(soldier);
				return (*i)->getSoldiers()->erase(j);
			}
		}
	}
	return j;
}

/**
 * enables/disables autosell for an item type
 */
void SavedGame::setAutosell(const RuleItem *itype, const bool enabled)
{
	if (enabled)
	{
		_autosales.insert(itype);
	}
	else
	{
		_autosales.erase(itype);
	}
}
/**
 * get autosell state for an item type
 */
bool SavedGame::getAutosell(const RuleItem *itype) const
{
	if (!Options::oxceAutoSell)
	{
		return false;
	}
	return _autosales.find(itype) != _autosales.end();
}

/**
 * Removes all soldiers from a given craft.
 */
void SavedGame::removeAllSoldiersFromXcomCraft(Craft *craft)
{
	for (auto base : _bases)
	{
		for (auto soldier : *base->getSoldiers())
		{
			if (soldier->getCraft() == craft)
			{
				soldier->setCraft(0);
			}
		}
	}
}

/**
 * Stop hunting the given xcom craft.
 */
void SavedGame::stopHuntingXcomCraft(Craft *target)
{
	for (std::vector<Ufo*>::iterator u = _ufos.begin(); u != _ufos.end(); ++u)
	{
		(*u)->resetOriginalDestination(target);
	}
}

/**
 * Stop hunting all xcom craft from a given xcom base.
 */
void SavedGame::stopHuntingXcomCrafts(Base *base)
{
	for (std::vector<Craft*>::iterator c = base->getCrafts()->begin(); c != base->getCrafts()->end(); ++c)
	{
		for (std::vector<Ufo*>::iterator u = _ufos.begin(); u != _ufos.end(); ++u)
		{
			(*u)->resetOriginalDestination((*c));
		}
	}
}

/**
 * Should all xcom soldiers have completely empty starting inventory when doing base equipment?
 */
bool SavedGame::getDisableSoldierEquipment() const
{
	return _disableSoldierEquipment;
}

/**
 * Sets the corresponding flag.
 */
void SavedGame::setDisableSoldierEquipment(bool disableSoldierEquipment)
{
	_disableSoldierEquipment = disableSoldierEquipment;
}

/**
 * Is the mana feature already unlocked?
 */
bool SavedGame::isManaUnlocked(Mod *mod) const
{
	auto researchName = mod->getManaUnlockResearch();
	if (Mod::isEmptyRuleName(researchName) || isResearched(researchName))
	{
		return true;
	}
	return false;
}

/**
 * Gets the current score based on research score and xcom/alien activity in regions.
 */
int SavedGame::getCurrentScore(int monthsPassed) const
{
	size_t invertedEntry = _funds.size() - 1;
	int scoreTotal = _researchScores.at(invertedEntry);
	if (monthsPassed > 1)
		scoreTotal += 400;
	for (auto region : _regions)
	{
		scoreTotal += region->getActivityXcom().at(invertedEntry) - region->getActivityAlien().at(invertedEntry);
	}
	return scoreTotal;
}

/**
 * Clear links for the given alien base. Use this before deleting the alien base.
 */
void SavedGame::clearLinksForAlienBase(AlienBase* alienBase, const Mod* mod)
{
	// Take care to remove supply missions for this base.
	for (auto am : _activeMissions)
	{
		if (am->getAlienBase() == alienBase)
		{
			am->setAlienBase(0);

			// if this is an Earth-based operation, losing the base means mission cannot continue anymore
			if (am->getRules().getOperationType() != AMOT_SPACE)
			{
				am->setInterrupted(true);
			}
		}
	}
	// If there was a pact with this base, cancel it?
	if (mod->getAllowCountriesToCancelAlienPact() && !alienBase->getPactCountry().empty())
	{
		for (auto cntr : _countries)
		{
			if (cntr->getRules()->getType() == alienBase->getPactCountry())
			{
				cntr->setCancelPact();
				break;
			}
		}
	}
}

/**
 * Delete the given retaliation mission.
 */
void SavedGame::deleteRetaliationMission(AlienMission* am, Base* base)
{
	for (std::vector<Ufo*>::iterator i = _ufos.begin(); i != _ufos.end();)
	{
		if ((*i)->getMission() == am)
		{
			delete (*i);
			i = _ufos.erase(i);
		}
		else
		{
			++i;
		}
	}
	for (std::vector<AlienMission*>::iterator i = _activeMissions.begin(); i != _activeMissions.end(); ++i)
	{
		if ((*i) == am)
		{
			delete (*i);
			_activeMissions.erase(i);
			break;
		}
	}
	if (base)
	{
		base->setRetaliationMission(nullptr);
	}
}

/**
 * Spawn a Geoscape event from the event rules.
 * @return True if successful.
 */
bool SavedGame::spawnEvent(const RuleEvent* eventRules)
{
	if (!eventRules)
	{
		return false;
	}

	GeoscapeEvent* newEvent = new GeoscapeEvent(*eventRules);
	int minutes = (eventRules->getTimer() + (RNG::generate(0, eventRules->getTimerRandom()))) / 30 * 30;
	if (minutes < 60) minutes = 60; // just in case
	newEvent->setSpawnCountdown(minutes);
	_geoscapeEvents.push_back(newEvent);

	// remember that it has been generated
	addGeneratedEvent(eventRules);

	return true;
}

/**
 * Checks if an instant Geoscape event can be spawned.
 */
bool SavedGame::canSpawnInstantEvent(const RuleEvent* eventRules)
{
	if (!eventRules)
	{
		return false;
	}

	bool interrupted = false;
	if (!eventRules->getInterruptResearch().empty())
	{
		if (isResearched(eventRules->getInterruptResearch(), false))
		{
			interrupted = true;
		}
	}

	if (!interrupted)
	{
		addGeneratedEvent(eventRules);
		return true;
	}

	return false;
}

/**
 * Handles research unlocked by successful/failed missions and despawned mission sites.
 * 1. Adds the research topic to finished research list. Silently.
 * 2. Adds also getOneFree bonus and possible lookup(s). Also silently.
 * 3. Handles alien mission interruption.
 */
bool SavedGame::handleResearchUnlockedByMissions(const RuleResearch* research, const Mod* mod)
{
	if (!research)
	{
		return false;
	}
	if (_bases.empty())
	{
		return false; // all bases lost, game over
	}
	Base* base = _bases.front();

	std::vector<const RuleResearch*> researchVec;
	researchVec.push_back(research);
	addFinishedResearch(research, mod, base, true);
	if (!research->getLookup().empty())
	{
		researchVec.push_back(mod->getResearch(research->getLookup(), true));
		addFinishedResearch(researchVec.back(), mod, base, true);
	}

	if (auto bonus = selectGetOneFree(research))
	{
		researchVec.push_back(bonus);
		addFinishedResearch(bonus, mod, base, true);
		if (!bonus->getLookup().empty())
		{
			researchVec.push_back(mod->getResearch(bonus->getLookup(), true));
			addFinishedResearch(researchVec.back(), mod, base, true);
		}
	}

	// check and interrupt alien missions if necessary (based on unlocked research)
	for (auto* am : _activeMissions)
	{
		auto& interruptResearchName = am->getRules().getInterruptResearch();
		if (!interruptResearchName.empty())
		{
			auto* interruptResearch = mod->getResearch(interruptResearchName, true);
			if (std::find(researchVec.begin(), researchVec.end(), interruptResearch) != researchVec.end())
			{
				am->setInterrupted(true);
			}
		}
	}

	return true;
}

/**
 * Handles research side effects for primary research sources.
 */
void SavedGame::handlePrimaryResearchSideEffects(const std::vector<const RuleResearch*> &topicsToCheck, const Mod* mod, Base* base)
{
	for (auto* myResearchRule : topicsToCheck)
	{
		// 3j. now iterate through all the bases and remove this project from their labs (unless it can still yield more stuff!)
		for (Base* otherBase : _bases)
		{
			for (ResearchProject* otherProject : otherBase->getResearch())
			{
				if (myResearchRule == otherProject->getRules())
				{
					if (hasUndiscoveredGetOneFree(myResearchRule, true))
					{
						// This research topic still has some more undiscovered non-disabled and *AVAILABLE* "getOneFree" topics, keep it!
					}
					else if (hasUndiscoveredProtectedUnlock(myResearchRule, mod))
					{
						// This research topic still has one or more undiscovered non-disabled "protected unlocks", keep it!
					}
					else
					{
						// This topic can't give you anything else anymore, remove it!
						otherBase->removeResearch(otherProject);
						break;
					}
				}
			}
		}
		// 3k. handle spawned items
		RuleItem* spawnedItem = mod->getItem(myResearchRule->getSpawnedItem());
		if (spawnedItem)
		{
			Transfer* t = new Transfer(1);
			t->setItems(myResearchRule->getSpawnedItem(), std::max(1, myResearchRule->getSpawnedItemCount()));
			base->getTransfers()->push_back(t);
		}
		for (auto& spawnedItemName2 : myResearchRule->getSpawnedItemList())
		{
			RuleItem* spawnedItem2 = mod->getItem(spawnedItemName2);
			if (spawnedItem2)
			{
				Transfer* t = new Transfer(1);
				t->setItems(spawnedItemName2);
				base->getTransfers()->push_back(t);
			}
		}
		// 3l. handle spawned events
		RuleEvent* spawnedEventRule = mod->getEvent(myResearchRule->getSpawnedEvent());
		spawnEvent(spawnedEventRule);
		// 3m. handle counters
		for (auto& inc : myResearchRule->getIncreaseCounter())
		{
			increaseCustomCounter(inc);
		}
		for (auto& dec : myResearchRule->getDecreaseCounter())
		{
			decreaseCustomCounter(dec);
		}
	}
}

////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

void getRandomScript(SavedGame* sg, RNG::RandomState*& r)
{
	if (sg)
	{
		r = &RNG::globalRandomState();
	}
	else
	{
		r = nullptr;
	}
}

void getTimeScript(const SavedGame* sg, const GameTime*& r)
{
	if (sg)
	{
		r = sg->getTime();
	}
	else
	{
		r = nullptr;
	}
}

void randomChanceScript(RNG::RandomState* rs, int& val)
{
	if (rs)
	{
		val = rs->generate(0, 99) < val;
	}
	else
	{
		val = 0;
	}
}

void randomRangeScript(RNG::RandomState* rs, int& val, int min, int max)
{
	if (rs && max >= min)
	{
		val = rs->generate(min, max);
	}
	else
	{
		val = 0;
	}
}

void getDaysPastEpochScript(const GameTime* p, int& val)
{
	if (p)
	{
		std::tm time = {  };
		time.tm_year = p->getYear() - 1900;
		time.tm_mon = p->getMonth() - 1;
		time.tm_mday = p->getDay();
		time.tm_hour = p->getHour();
		time.tm_min = p->getMinute();
		time.tm_sec = p->getSecond();
		val = (int)(std::mktime(&time) / (60 * 60 * 24));
	}
	else
	{
		val = 0;
	}
}

void getSecondsPastMidnightScript(const GameTime* p, int& val)
{
	if (p)
	{
		val = p->getSecond() +
			60 * p->getMinute() +
			60 * 60 * p->getHour();
	}
	else
	{
		val = 0;
	}
}

std::string debugDisplayScript(const RNG::RandomState* p)
{
	if (p)
	{
		std::string s;
		s += "RandomState";
		s += "(seed: \"";
		s += std::to_string(p->getSeed());
		s += "\")";
		return s;
	}
	else
	{
		return "null";
	}
}

std::string debugDisplayScript(const GameTime* p)
{
	if (p)
	{
		auto zeroPrefix = [](int i)
		{
			if (i < 10)
			{
				return "0" + std::to_string(i);
			}
			else
			{
				return std::to_string(i);
			}
		};

		std::string s;
		s += "Time";
		s += "(\"";
		s += std::to_string(p->getYear());
		s += "-";
		s += zeroPrefix(p->getMonth());
		s += "-";
		s += zeroPrefix(p->getDay());
		s += " ";
		s += zeroPrefix(p->getHour());
		s += ":";
		s += zeroPrefix(p->getMinute());
		s += ":";
		s += zeroPrefix(p->getSecond());
		s += "\")";
		return s;
	}
	else
	{
		return "null";
	}
}

void isResearchedScript(const SavedGame* sg, int& val, const RuleResearch* name)
{
	if (sg)
	{
		if (sg->isResearched(name, false))
		{
			val = 1;
			return;
		}
	}
	val = 0;
}

std::string debugDisplayScript(const SavedGame* p)
{
	if (p)
	{
		std::string s;
		s += SavedGame::ScriptName;
		s += "(fileName: \"";
		s += p->getName();
		s += "\" time: ";
		s += debugDisplayScript(p->getTime());
		s += ")";
		return s;
	}
	else
	{
		return "null";
	}
}

}

/**
 * Register SavedGame in script parser.
 * @param parser Script parser.
 */
void SavedGame::ScriptRegister(ScriptParserBase* parser)
{

	{
		const auto name = std::string{ "RandomState" };
		parser->registerRawPointerType<RNG::RandomState>(name);
		Bind<RNG::RandomState> rs = { parser, name };

		rs.add<&randomChanceScript>("randomChance", "Change value from range 0-100 to 0-1 based on probability");
		rs.add<&randomRangeScript>("randomRange", "Return random value from defined range");

		rs.addDebugDisplay<&debugDisplayScript>();
	}

	{
		const auto name = std::string{ "Time" };
		parser->registerRawPointerType<GameTime>(name);
		Bind<GameTime> t = { parser, name };

		t.add<&GameTime::getSecond>("getSecond");
		t.add<&GameTime::getMinute>("getMinute");
		t.add<&GameTime::getHour>("getHour");
		t.add<&GameTime::getDay>("getDay");
		t.add<&GameTime::getMonth>("getMonth");
		t.add<&GameTime::getYear>("getYear");
		t.add<&getDaysPastEpochScript>("getDaysPastEpoch", "Days past 1970-01-01");
		t.add<&getSecondsPastMidnightScript>("getSecondsPastMidnight", "Seconds past 00:00");

		t.addDebugDisplay<&debugDisplayScript>();
	}

	Bind<SavedGame> sgg = { parser };

	sgg.add<&getTimeScript>("getTime", "Get global time that is Greenwich Mean Time");
	sgg.add<&getRandomScript>("getRandomState");

	sgg.add<&isResearchedScript>("isResearched");

	sgg.addScriptValue<&SavedGame::_scriptValues>();

	sgg.addDebugDisplay<&debugDisplayScript>();
}

}
