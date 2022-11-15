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
#include "BattleUnit.h"
#include "BattleItem.h"
#include <sstream>
#include <algorithm>
#include "../Engine/Collections.h"
#include "../Engine/Surface.h"
#include "../Engine/Script.h"
#include "../Engine/ScriptBind.h"
#include "../Engine/Language.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/AIModule.h"
#include "../Battlescape/Inventory.h"
#include "../Battlescape/TileEngine.h"
#include "../Battlescape/ExplosionBState.h"
#include "../Mod/Mod.h"
#include "../Mod/Armor.h"
#include "../Mod/Unit.h"
#include "../Mod/RuleEnviroEffects.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleSkill.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierBonus.h"
#include "../Mod/RuleStartingCondition.h"
#include "Soldier.h"
#include "Tile.h"
#include "SavedGame.h"
#include "SavedBattleGame.h"
#include "../Engine/ShaderDraw.h"
#include "BattleUnitStatistics.h"
#include "../fmath.h"
#include "../fallthrough.h"

namespace OpenXcom
{

/**
 * Initializes a BattleUnit from a Soldier
 * @param soldier Pointer to the Soldier.
 * @param depth the depth of the battlefield (used to determine movement type in case of MT_FLOAT).
 */
BattleUnit::BattleUnit(const Mod *mod, Soldier *soldier, int depth, const RuleStartingCondition* sc) :
	_faction(FACTION_PLAYER), _originalFaction(FACTION_PLAYER), _killedBy(FACTION_PLAYER), _id(0), _tile(0),
	_lastPos(Position()), _direction(0), _toDirection(0), _directionTurret(0), _toDirectionTurret(0),
	_verticalDirection(0), _status(STATUS_STANDING), _wantsToSurrender(false), _isSurrendering(false), _walkPhase(0), _fallPhase(0), _kneeled(false), _floating(false),
	_dontReselect(false), _fire(0), _currentAIState(0), _visible(false),
	_exp{ }, _expTmp{ },
	_motionPoints(0), _scannedTurn(-1), _kills(0), _hitByFire(false), _hitByAnything(false), _alreadyExploded(false), _fireMaxHit(0), _smokeMaxHit(0), _moraleRestored(0), _charging(0), _turnsSinceSpotted(255), _turnsLeftSpottedForSnipers(0),
	_statistics(), _murdererId(0), _mindControllerID(0), _fatalShotSide(SIDE_FRONT), _fatalShotBodyPart(BODYPART_HEAD), _armor(0),
	_geoscapeSoldier(soldier), _unitRules(0), _rankInt(0), _turretType(-1), _hidingForTurn(false), _floorAbove(false), _respawn(false), _alreadyRespawned(false),
	_isLeeroyJenkins(false), _summonedPlayerUnit(false), _resummonedFakeCivilian(false), _pickUpWeaponsMoreActively(false), _disableIndicators(false),
	_capturable(true), _vip(false), _bannedInNextStage(false)
{
	_name = soldier->getName(true);
	_id = soldier->getId();
	_type = "SOLDIER";
	_rank = soldier->getRankString();
	_stats = *soldier->getCurrentStats();
	_armor = soldier->getArmor();
	_standHeight = _armor->getStandHeight() == -1 ? soldier->getRules()->getStandHeight() : _armor->getStandHeight();
	_kneelHeight = _armor->getKneelHeight() == -1 ? soldier->getRules()->getKneelHeight() : _armor->getKneelHeight();
	_floatHeight = _armor->getFloatHeight() == -1 ? soldier->getRules()->getFloatHeight() : _armor->getFloatHeight();

	_intelligence = 2;
	_aggression = 1;
	_specab = (SpecialAbility)_armor->getSpecialAbility();
	_originalMovementType = _movementType = _armor->getMovementTypeByDepth(depth);
	_moveCostBase = _armor->getMoveCostBase();
	_moveCostBaseFly = _armor->getMoveCostBaseFly();
	_moveCostBaseNormal = _armor->getMoveCostBaseNormal();

	// armor and soldier bonuses may modify effective stats
	{
		soldier->prepareStatsWithBonuses(mod); // refresh all bonuses
		_stats = *soldier->getStatsWithAllBonuses();
	}
	int visibilityBonus = 0;
	for (auto bonusRule : *soldier->getBonuses(nullptr))
	{
		visibilityBonus += bonusRule->getVisibilityAtDark();
	}
	_maxViewDistanceAtDark = _armor->getVisibilityAtDark() ? _armor->getVisibilityAtDark() : 9;
	_maxViewDistanceAtDark += visibilityBonus;
	_maxViewDistanceAtDark = Clamp(_maxViewDistanceAtDark, 1, mod->getMaxViewDistance());
	_maxViewDistanceAtDarkSquared = _maxViewDistanceAtDark * _maxViewDistanceAtDark;
	_maxViewDistanceAtDay = _armor->getVisibilityAtDay() ? _armor->getVisibilityAtDay() : mod->getMaxViewDistance();
	_loftempsSet = _armor->getLoftempsSet();
	_gender = soldier->getGender();
	_faceDirection = -1;
	_breathFrame = -1;
	if (_armor->drawBubbles())
	{
		_breathFrame = 0;
	}
	_floorAbove = false;
	_breathing = false;

	int rankbonus = 0;

	switch (soldier->getRank())
	{
	case RANK_SERGEANT:  rankbonus =  1; break;
	case RANK_CAPTAIN:   rankbonus =  3; break;
	case RANK_COLONEL:   rankbonus =  6; break;
	case RANK_COMMANDER: rankbonus = 10; break;
	default:             rankbonus =  0; break;
	}

	_value = soldier->getRules()->getValue() + soldier->getMissions() + rankbonus;

	_tu = _stats.tu;
	_energy = _stats.stamina;
	_health = std::max(1, _stats.health - soldier->getHealthMissing());
	_mana = std::max(0, _stats.mana - soldier->getManaMissing());
	_morale = 100;
	// wounded soldiers (defending the base) start with lowered morale
	{
		if (soldier->isWounded())
		{
			_morale = 75;
			_health = std::max(1, _health - soldier->getWoundRecoveryInt());
		}
	}
	_stunlevel = 0;
	_maxArmor[SIDE_FRONT] = _armor->getFrontArmor();
	_maxArmor[SIDE_LEFT] = _armor->getLeftSideArmor();
	_maxArmor[SIDE_RIGHT] = _armor->getRightSideArmor();
	_maxArmor[SIDE_REAR] = _armor->getRearArmor();
	_maxArmor[SIDE_UNDER] = _armor->getUnderArmor();
	{
		for (auto bonusRule : *soldier->getBonuses(nullptr))
		{
			_maxArmor[SIDE_FRONT] += bonusRule->getFrontArmor();
			_maxArmor[SIDE_LEFT]  += bonusRule->getLeftSideArmor();
			_maxArmor[SIDE_RIGHT] += bonusRule->getRightSideArmor();
			_maxArmor[SIDE_REAR]  += bonusRule->getRearArmor();
			_maxArmor[SIDE_UNDER] += bonusRule->getUnderArmor();
		}
		_maxArmor[SIDE_FRONT] = std::max(0, _maxArmor[SIDE_FRONT]);
		_maxArmor[SIDE_LEFT]  = std::max(0, _maxArmor[SIDE_LEFT]);
		_maxArmor[SIDE_RIGHT] = std::max(0, _maxArmor[SIDE_RIGHT]);
		_maxArmor[SIDE_REAR]  = std::max(0, _maxArmor[SIDE_REAR]);
		_maxArmor[SIDE_UNDER] = std::max(0, _maxArmor[SIDE_UNDER]);
	}
	_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
	_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
	_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
	_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
	_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];
	for (int i = 0; i < BODYPART_MAX; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
		_specWeapon[i] = 0;

	_activeHand = "STR_RIGHT_HAND";
	_preferredHandForReactions = "";

	lastCover = TileEngine::invalid;

	_statistics = new BattleUnitStatistics();

	deriveRank();

	int look = soldier->getGender() + 2 * soldier->getLook() + 8 * soldier->getLookVariant();
	setRecolor(look, look, _rankInt);

	prepareUnitSounds();
	prepareUnitResponseSounds(mod);
	prepareBannedFlag(sc);
}

/**
 * Updates BattleUnit's armor and related attributes (after a change/transformation of armor).
 * @param soldier Pointer to the Geoscape Soldier.
 * @param ruleArmor Pointer to the new Armor ruleset.
 * @param depth The depth of the battlefield.
 */
void BattleUnit::updateArmorFromSoldier(const Mod *mod, Soldier *soldier, Armor *ruleArmor, int depth, bool inBattlescape, const RuleStartingCondition* sc)
{
	_stats = *soldier->getCurrentStats();
	_armor = ruleArmor;

	_standHeight = _armor->getStandHeight() == -1 ? soldier->getRules()->getStandHeight() : _armor->getStandHeight();
	_kneelHeight = _armor->getKneelHeight() == -1 ? soldier->getRules()->getKneelHeight() : _armor->getKneelHeight();
	_floatHeight = _armor->getFloatHeight() == -1 ? soldier->getRules()->getFloatHeight() : _armor->getFloatHeight();

	_specab = (SpecialAbility)_armor->getSpecialAbility();
	_originalMovementType = _movementType = _armor->getMovementTypeByDepth(depth);
	_moveCostBase = _armor->getMoveCostBase();
	_moveCostBaseFly = _armor->getMoveCostBaseFly();
	_moveCostBaseNormal = _armor->getMoveCostBaseNormal();

	// armor and soldier bonuses may modify effective stats
	{
		soldier->prepareStatsWithBonuses(mod); // refresh needed, because of armor stats
		_stats = *soldier->getStatsWithAllBonuses();
	}
	int visibilityBonus = 0;
	for (auto bonusRule : *soldier->getBonuses(nullptr))
	{
		visibilityBonus += bonusRule->getVisibilityAtDark();
	}
	_maxViewDistanceAtDark = _armor->getVisibilityAtDark() ? _armor->getVisibilityAtDark() : 9;
	_maxViewDistanceAtDark += visibilityBonus;
	_maxViewDistanceAtDark = Clamp(_maxViewDistanceAtDark, 1, mod->getMaxViewDistance());
	_maxViewDistanceAtDarkSquared = _maxViewDistanceAtDark * _maxViewDistanceAtDark;
	_maxViewDistanceAtDay = _armor->getVisibilityAtDay() ? _armor->getVisibilityAtDay() : mod->getMaxViewDistance();
	_loftempsSet = _armor->getLoftempsSet();

	_tu = _stats.tu;
	_energy = _stats.stamina;
	if (inBattlescape)
	{
		_health = std::min(_health, (int)_stats.health);
		_mana = std::min(_mana, (int)_stats.mana);
	}
	else
	{
		_health = std::max(1, _stats.health - soldier->getHealthMissing());
		_mana = std::max(0, _stats.mana - soldier->getManaMissing());
	}
	_maxArmor[SIDE_FRONT] = _armor->getFrontArmor();
	_maxArmor[SIDE_LEFT] = _armor->getLeftSideArmor();
	_maxArmor[SIDE_RIGHT] = _armor->getRightSideArmor();
	_maxArmor[SIDE_REAR] = _armor->getRearArmor();
	_maxArmor[SIDE_UNDER] = _armor->getUnderArmor();
	{
		for (auto bonusRule : *soldier->getBonuses(nullptr))
		{
			_maxArmor[SIDE_FRONT] += bonusRule->getFrontArmor();
			_maxArmor[SIDE_LEFT]  += bonusRule->getLeftSideArmor();
			_maxArmor[SIDE_RIGHT] += bonusRule->getRightSideArmor();
			_maxArmor[SIDE_REAR]  += bonusRule->getRearArmor();
			_maxArmor[SIDE_UNDER] += bonusRule->getUnderArmor();
		}
		_maxArmor[SIDE_FRONT] = std::max(0, _maxArmor[SIDE_FRONT]);
		_maxArmor[SIDE_LEFT]  = std::max(0, _maxArmor[SIDE_LEFT]);
		_maxArmor[SIDE_RIGHT] = std::max(0, _maxArmor[SIDE_RIGHT]);
		_maxArmor[SIDE_REAR]  = std::max(0, _maxArmor[SIDE_REAR]);
		_maxArmor[SIDE_UNDER] = std::max(0, _maxArmor[SIDE_UNDER]);
	}
	_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
	_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
	_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
	_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
	_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];

	int look = soldier->getGender() + 2 * soldier->getLook() + 8 * soldier->getLookVariant();
	setRecolor(look, look, _rankInt);

	prepareUnitSounds();
	prepareUnitResponseSounds(mod);
	prepareBannedFlag(sc);
}

/**
 * Helper function preparing unit sounds.
 */
void BattleUnit::prepareUnitSounds()
{
	_lastReloadSound = Mod::ITEM_RELOAD;

	if (_geoscapeSoldier)
	{
		Collections::removeAll(_aggroSound);
		_moveSound = _armor->getMoveSound() != Mod::NO_SOUND ? _armor->getMoveSound() : Mod::NO_SOUND; // there's no soldier move sound, thus hardcoded -1
	}
	else if (_unitRules)
	{
		_aggroSound = _unitRules->getAggroSounds();
		_moveSound = _armor->getMoveSound() != Mod::NO_SOUND ? _armor->getMoveSound() : _unitRules->getMoveSound();
	}

	// lower priority: soldier type / unit type
	if (_geoscapeSoldier)
	{
		auto soldierRules = _geoscapeSoldier->getRules();
		if (_gender == GENDER_MALE)
			_deathSound = soldierRules->getMaleDeathSounds();
		else
			_deathSound = soldierRules->getFemaleDeathSounds();
	}
	else if (_unitRules)
	{
		_deathSound = _unitRules->getDeathSounds();
	}

	// higher priority: armor
	if (_gender == GENDER_MALE)
	{
		if (!_armor->getMaleDeathSounds().empty())
			_deathSound = _armor->getMaleDeathSounds();
	}
	else
	{
		if (!_armor->getFemaleDeathSounds().empty())
			_deathSound = _armor->getFemaleDeathSounds();
	}
}

/**
 * Helper function preparing unit response sounds.
 */
void BattleUnit::prepareUnitResponseSounds(const Mod *mod)
{
	if (!mod->getEnableUnitResponseSounds())
		return;

	// custom sounds by soldier name
	bool custom = false;
	if (mod->getSelectUnitSounds().find(_name) != mod->getSelectUnitSounds().end())
	{
		custom = true;
		_selectUnitSound = mod->getSelectUnitSounds().find(_name)->second;
	}
	if (mod->getStartMovingSounds().find(_name) != mod->getStartMovingSounds().end())
	{
		custom = true;
		_startMovingSound = mod->getStartMovingSounds().find(_name)->second;
	}
	if (mod->getSelectWeaponSounds().find(_name) != mod->getSelectWeaponSounds().end())
	{
		custom = true;
		_selectWeaponSound = mod->getSelectWeaponSounds().find(_name)->second;
	}
	if (mod->getAnnoyedSounds().find(_name) != mod->getAnnoyedSounds().end())
	{
		custom = true;
		_annoyedSound = mod->getAnnoyedSounds().find(_name)->second;
	}

	if (custom)
		return;

	// lower priority: soldier type / unit type
	if (_geoscapeSoldier)
	{
		auto soldierRules = _geoscapeSoldier->getRules();
		if (_gender == GENDER_MALE)
		{
			_selectUnitSound = soldierRules->getMaleSelectUnitSounds();
			_startMovingSound = soldierRules->getMaleStartMovingSounds();
			_selectWeaponSound = soldierRules->getMaleSelectWeaponSounds();
			_annoyedSound = soldierRules->getMaleAnnoyedSounds();
		}
		else
		{
			_selectUnitSound = soldierRules->getFemaleSelectUnitSounds();
			_startMovingSound = soldierRules->getFemaleStartMovingSounds();
			_selectWeaponSound = soldierRules->getFemaleSelectWeaponSounds();
			_annoyedSound = soldierRules->getFemaleAnnoyedSounds();
		}
	}
	else if (_unitRules)
	{
		_selectUnitSound = _unitRules->getSelectUnitSounds();
		_startMovingSound = _unitRules->getStartMovingSounds();
		_selectWeaponSound = _unitRules->getSelectWeaponSounds();
		_annoyedSound = _unitRules->getAnnoyedSounds();
	}

	// higher priority: armor
	if (_gender == GENDER_MALE)
	{
		if (!_armor->getMaleSelectUnitSounds().empty())
			_selectUnitSound = _armor->getMaleSelectUnitSounds();
		if (!_armor->getMaleStartMovingSounds().empty())
			_startMovingSound = _armor->getMaleStartMovingSounds();
		if (!_armor->getMaleSelectWeaponSounds().empty())
			_selectWeaponSound = _armor->getMaleSelectWeaponSounds();
		if (!_armor->getMaleAnnoyedSounds().empty())
			_annoyedSound = _armor->getMaleAnnoyedSounds();
	}
	else
	{
		if (!_armor->getFemaleSelectUnitSounds().empty())
			_selectUnitSound = _armor->getFemaleSelectUnitSounds();
		if (!_armor->getFemaleStartMovingSounds().empty())
			_startMovingSound = _armor->getFemaleStartMovingSounds();
		if (!_armor->getFemaleSelectWeaponSounds().empty())
			_selectWeaponSound = _armor->getFemaleSelectWeaponSounds();
		if (!_armor->getFemaleAnnoyedSounds().empty())
			_annoyedSound = _armor->getFemaleAnnoyedSounds();
	}
}

/**
 * Helper function preparing the banned flag.
 */
void BattleUnit::prepareBannedFlag(const RuleStartingCondition* sc)
{
	_bannedInNextStage = false;
	if (sc && !sc->getForbiddenArmorsInNextStage().empty())
	{
		const auto& bannedList = sc->getForbiddenArmorsInNextStage();
		if (std::find(bannedList.begin(), bannedList.end(), _armor) != bannedList.end())
		{
			_bannedInNextStage = true;
		}
	}
}

/**
 * Initializes a BattleUnit from a Unit (non-player) object.
 * @param unit Pointer to Unit object.
 * @param faction Which faction the units belongs to.
 * @param id Unique unit ID.
 * @param enviro Pointer to battle enviro effects.
 * @param armor Pointer to unit Armor.
 * @param diff difficulty level (for stat adjustment).
 * @param depth the depth of the battlefield (used to determine movement type in case of MT_FLOAT).
 */
BattleUnit::BattleUnit(const Mod *mod, Unit *unit, UnitFaction faction, int id, const RuleEnviroEffects* enviro, Armor *armor, StatAdjustment *adjustment, int depth, const RuleStartingCondition* sc) :
	_faction(faction), _originalFaction(faction), _killedBy(faction), _id(id),
	_tile(0), _lastPos(Position()), _direction(0), _toDirection(0), _directionTurret(0),
	_toDirectionTurret(0), _verticalDirection(0), _status(STATUS_STANDING), _wantsToSurrender(false), _isSurrendering(false), _walkPhase(0),
	_fallPhase(0), _kneeled(false), _floating(false), _dontReselect(false), _fire(0), _currentAIState(0),
	_visible(false), _exp{ }, _expTmp{ },
	_motionPoints(0), _scannedTurn(-1), _kills(0), _hitByFire(false), _hitByAnything(false), _alreadyExploded(false), _fireMaxHit(0), _smokeMaxHit(0),
	_moraleRestored(0), _charging(0), _turnsSinceSpotted(255), _turnsLeftSpottedForSnipers(0),
	_statistics(), _murdererId(0), _mindControllerID(0), _fatalShotSide(SIDE_FRONT),
	_fatalShotBodyPart(BODYPART_HEAD), _armor(armor), _geoscapeSoldier(0),  _unitRules(unit),
	_rankInt(0), _turretType(-1), _hidingForTurn(false), _respawn(false), _alreadyRespawned(false),
	_isLeeroyJenkins(false), _summonedPlayerUnit(false), _resummonedFakeCivilian(false), _pickUpWeaponsMoreActively(false), _disableIndicators(false),
	_vip(false), _bannedInNextStage(false)
{
	if (enviro)
	{
		auto newArmor = enviro->getArmorTransformation(_armor);
		if (newArmor)
		{
			_armor = newArmor;
		}
	}
	_type = unit->getType();
	_rank = unit->getRank();
	_race = unit->getRace();
	_stats = *unit->getStats();
	_standHeight = _armor->getStandHeight() == -1 ? unit->getStandHeight() : _armor->getStandHeight();
	_kneelHeight = _armor->getKneelHeight() == -1 ? unit->getKneelHeight() : _armor->getKneelHeight();
	_floatHeight = _armor->getFloatHeight() == -1 ? unit->getFloatHeight() : _armor->getFloatHeight();
	_loftempsSet = _armor->getLoftempsSet();
	_intelligence = unit->getIntelligence();
	_aggression = unit->getAggression();
	_specab = (SpecialAbility) unit->getSpecialAbility();
	_spawnUnit = unit->getSpawnUnit();
	_value = unit->getValue();
	_faceDirection = -1;
	_capturable = unit->getCapturable();
	_isLeeroyJenkins = unit->isLeeroyJenkins();
	if (unit->getPickUpWeaponsMoreActively() != -1)
	{
		_pickUpWeaponsMoreActively = (unit->getPickUpWeaponsMoreActively() != 0);
	}
	else
	{
		if (faction == FACTION_HOSTILE)
			_pickUpWeaponsMoreActively = mod->getAIPickUpWeaponsMoreActively();
		else
			_pickUpWeaponsMoreActively = mod->getAIPickUpWeaponsMoreActivelyCiv();
	}
	if (_unitRules && _unitRules->isVIP())
	{
		_vip = true;
	}

	_originalMovementType = _movementType = _armor->getMovementTypeByDepth(depth);
	_moveCostBase = _armor->getMoveCostBase();
	_moveCostBaseFly = _armor->getMoveCostBaseFly();
	_moveCostBaseNormal = _armor->getMoveCostBaseNormal();

	_stats += *_armor->getStats();	// armors may modify effective stats
	_stats = UnitStats::obeyFixedMinimum(_stats); // don't allow to go into minus!
	_maxViewDistanceAtDark = _armor->getVisibilityAtDark() ? _armor->getVisibilityAtDark() : faction==FACTION_HOSTILE ? mod->getMaxViewDistance() : 9;
	_maxViewDistanceAtDarkSquared = _maxViewDistanceAtDark * _maxViewDistanceAtDark;
	_maxViewDistanceAtDay = _armor->getVisibilityAtDay() ? _armor->getVisibilityAtDay() : mod->getMaxViewDistance();

	_breathFrame = -1; // most aliens don't breathe per-se, that's exclusive to humanoids
	if (_armor->drawBubbles())
	{
		_breathFrame = 0;
	}
	_floorAbove = false;
	_breathing = false;

	_maxArmor[SIDE_FRONT] = _armor->getFrontArmor();
	_maxArmor[SIDE_LEFT] = _armor->getLeftSideArmor();
	_maxArmor[SIDE_RIGHT] = _armor->getRightSideArmor();
	_maxArmor[SIDE_REAR] = _armor->getRearArmor();
	_maxArmor[SIDE_UNDER] = _armor->getUnderArmor();

	if (faction == FACTION_HOSTILE)
	{
		adjustStats(*adjustment);
	}

	_tu = _stats.tu;
	_energy = _stats.stamina;
	_health = _stats.health;
	_mana = _stats.mana;
	_morale = 100;
	_stunlevel = 0;
	_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
	_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
	_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
	_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
	_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];
	for (int i = 0; i < BODYPART_MAX; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
		_specWeapon[i] = 0;

	_activeHand = "STR_RIGHT_HAND";
	_preferredHandForReactions = "";
	_gender = GENDER_MALE;

	lastCover = TileEngine::invalid;

	_statistics = new BattleUnitStatistics();

	int generalRank = 0;
	if (faction == FACTION_HOSTILE)
	{
		const int max = 7;
		const char* rankList[max] =
		{
			"STR_LIVE_SOLDIER",
			"STR_LIVE_ENGINEER",
			"STR_LIVE_MEDIC",
			"STR_LIVE_NAVIGATOR",
			"STR_LIVE_LEADER",
			"STR_LIVE_COMMANDER",
			"STR_LIVE_TERRORIST",
		};
		for (int i = 0; i < max; ++i)
		{
			if (_rank.compare(rankList[i]) == 0)
			{
				generalRank = i;
				break;
			}
		}
	}
	else if (faction == FACTION_NEUTRAL)
	{
		generalRank = RNG::seedless(0, 7);
	}

	setRecolor(RNG::seedless(0, 127), RNG::seedless(0, 127), generalRank);

	prepareUnitSounds();
	prepareUnitResponseSounds(mod);
	prepareBannedFlag(sc);
}

/**
 * Updates BattleUnit's armor and related attributes (after a change/transformation of armor).
 */
void BattleUnit::updateArmorFromNonSoldier(const Mod* mod, Armor* newArmor, int depth, const RuleStartingCondition* sc)
{
	if (_originalFaction != FACTION_PLAYER)
	{
		// armor updates for enemies and civilians is only allowed in the constructor (they don't travel between mission stages)
		return;
	}
	if (newArmor)
	{
		_armor = newArmor;
	}
	_standHeight = _armor->getStandHeight() == -1 ? _unitRules->getStandHeight() : _armor->getStandHeight();
	_kneelHeight = _armor->getKneelHeight() == -1 ? _unitRules->getKneelHeight() : _armor->getKneelHeight();
	_floatHeight = _armor->getFloatHeight() == -1 ? _unitRules->getFloatHeight() : _armor->getFloatHeight();
	_loftempsSet = _armor->getLoftempsSet();

	_originalMovementType = _movementType = _armor->getMovementTypeByDepth(depth);
	_moveCostBase = _armor->getMoveCostBase();
	_moveCostBaseFly = _armor->getMoveCostBaseFly();
	_moveCostBaseNormal = _armor->getMoveCostBaseNormal();

	_stats = *_unitRules->getStats();
	_stats += *_armor->getStats();	// armors may modify effective stats
	_stats = UnitStats::obeyFixedMinimum(_stats); // don't allow to go into minus!

	_maxViewDistanceAtDark = _armor->getVisibilityAtDark() ? _armor->getVisibilityAtDark() : 9;
	_maxViewDistanceAtDarkSquared = _maxViewDistanceAtDark * _maxViewDistanceAtDark;
	_maxViewDistanceAtDay = _armor->getVisibilityAtDay() ? _armor->getVisibilityAtDay() : mod->getMaxViewDistance();

	_maxArmor[SIDE_FRONT] = _armor->getFrontArmor();
	_maxArmor[SIDE_LEFT] = _armor->getLeftSideArmor();
	_maxArmor[SIDE_RIGHT] = _armor->getRightSideArmor();
	_maxArmor[SIDE_REAR] = _armor->getRearArmor();
	_maxArmor[SIDE_UNDER] = _armor->getUnderArmor();

	_tu = _stats.tu;
	_energy = _stats.stamina;
	_health = std::min(_health, (int)_stats.health);
	_mana = std::min(_mana, (int)_stats.mana);

	_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
	_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
	_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
	_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
	_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];

	setRecolor(RNG::seedless(0, 127), RNG::seedless(0, 127), 0);

	prepareUnitSounds();
	prepareUnitResponseSounds(mod);
	prepareBannedFlag(sc);
}


/**
 *
 */
BattleUnit::~BattleUnit()
{
	for (std::vector<BattleUnitKills*>::const_iterator i = _statistics->kills.begin(); i != _statistics->kills.end(); ++i)
	{
		delete *i;
	}
	delete _statistics;
	delete _currentAIState;
}

/**
 * Loads the unit from a YAML file.
 * @param node YAML node.
 */
void BattleUnit::load(const YAML::Node &node, const Mod *mod, const ScriptGlobal *shared)
{
	_id = node["id"].as<int>(_id);
	_faction = (UnitFaction)node["faction"].as<int>(_faction);
	_status = (UnitStatus)node["status"].as<int>(_status);
	_wantsToSurrender = node["wantsToSurrender"].as<bool>(_wantsToSurrender);
	_isSurrendering = node["isSurrendering"].as<bool>(_isSurrendering);
	_pos = node["position"].as<Position>(_pos);
	_direction = _toDirection = node["direction"].as<int>(_direction);
	_directionTurret = _toDirectionTurret = node["directionTurret"].as<int>(_directionTurret);
	_tu = node["tu"].as<int>(_tu);
	_health = node["health"].as<int>(_health);
	_mana = node["mana"].as<int>(_mana);
	_stunlevel = node["stunlevel"].as<int>(_stunlevel);
	_energy = node["energy"].as<int>(_energy);
	_morale = node["morale"].as<int>(_morale);
	_kneeled = node["kneeled"].as<bool>(_kneeled);
	_floating = node["floating"].as<bool>(_floating);
	for (int i=0; i < SIDE_MAX; i++)
		_currentArmor[i] = node["armor"][i].as<int>(_currentArmor[i]);
	for (int i=0; i < BODYPART_MAX; i++)
		_fatalWounds[i] = node["fatalWounds"][i].as<int>(_fatalWounds[i]);
	_fire = node["fire"].as<int>(_fire);
	_exp.bravery = node["expBravery"].as<int>(_exp.bravery);
	_exp.reactions = node["expReactions"].as<int>(_exp.reactions);
	_exp.firing = node["expFiring"].as<int>(_exp.firing);
	_exp.throwing = node["expThrowing"].as<int>(_exp.throwing);
	_exp.psiSkill = node["expPsiSkill"].as<int>(_exp.psiSkill);
	_exp.psiStrength = node["expPsiStrength"].as<int>(_exp.psiStrength);
	_exp.mana = node["expMana"].as<int>(_exp.mana);
	_exp.melee = node["expMelee"].as<int>(_exp.melee);
	_stats = node["currStats"].as<UnitStats>(_stats);
	_turretType = node["turretType"].as<int>(_turretType);
	_visible = node["visible"].as<bool>(_visible);
	_turnsSinceSpotted = node["turnsSinceSpotted"].as<int>(_turnsSinceSpotted);
	_turnsLeftSpottedForSnipers = node["turnsLeftSpottedForSnipers"].as<int>(_turnsLeftSpottedForSnipers);
	_turnsSinceStunned = node["turnsSinceStunned"].as<int>(_turnsSinceStunned);
	_killedBy = (UnitFaction)node["killedBy"].as<int>(_killedBy);
	_moraleRestored = node["moraleRestored"].as<int>(_moraleRestored);
	_rankInt = node["rankInt"].as<int>(_rankInt);
	_kills = node["kills"].as<int>(_kills);
	_dontReselect = node["dontReselect"].as<bool>(_dontReselect);
	_charging = 0;

	if (const YAML::Node& spawn = node["spawnUnit"])
	{
		_spawnUnit = mod->getUnit(spawn.as<std::string>(), false); //ignored bugged types
		if (_spawnUnit)
		{
			_respawn = node["respawn"].as<bool>(_respawn);
			_spawnUnitFaction = (UnitFaction)node["spawnUnitFaction"].as<int>(_spawnUnitFaction);
		}
	}

	_motionPoints = node["motionPoints"].as<int>(0);
	_alreadyRespawned = node["alreadyRespawned"].as<bool>(_alreadyRespawned);
	_activeHand = node["activeHand"].as<std::string>(_activeHand);
	_preferredHandForReactions = node["preferredHandForReactions"].as<std::string>(_preferredHandForReactions);
	if (node["tempUnitStatistics"])
	{
		_statistics->load(node["tempUnitStatistics"]);
	}
	_murdererId = node["murdererId"].as<int>(_murdererId);
	_fatalShotSide = (UnitSide)node["fatalShotSide"].as<int>(_fatalShotSide);
	_fatalShotBodyPart = (UnitBodyPart)node["fatalShotBodyPart"].as<int>(_fatalShotBodyPart);
	_murdererWeapon = node["murdererWeapon"].as<std::string>(_murdererWeapon);
	_murdererWeaponAmmo = node["murdererWeaponAmmo"].as<std::string>(_murdererWeaponAmmo);

	if (const YAML::Node& p = node["recolor"])
	{
		_recolor.clear();
		for (size_t i = 0; i < p.size(); ++i)
		{
			_recolor.push_back(std::make_pair(p[i][0].as<int>(), p[i][1].as<int>()));
		}
	}
	_mindControllerID = node["mindControllerID"].as<int>(_mindControllerID);
	_summonedPlayerUnit = node["summonedPlayerUnit"].as<bool>(_summonedPlayerUnit);
	_resummonedFakeCivilian = node["resummonedFakeCivilian"].as<bool>(_resummonedFakeCivilian);
	_pickUpWeaponsMoreActively = node["pickUpWeaponsMoreActively"].as<bool>(_pickUpWeaponsMoreActively);
	_disableIndicators = node["disableIndicators"].as<bool>(_disableIndicators);
	_movementType = (MovementType)node["movementType"].as<int>(_movementType);
	if (const YAML::Node& p = node["moveCost"])
	{
		_moveCostBase.load(p["basePercent"]);
		_moveCostBaseFly.load(p["baseFlyPercent"]);
		_moveCostBaseNormal.load(p["baseNormalPercent"]);
	}
	_vip = node["vip"].as<bool>(_vip);
	_bannedInNextStage = node["bannedInNextStage"].as<bool>(_bannedInNextStage);
	_meleeAttackedBy = node["meleeAttackedBy"].as<std::vector<int> >(_meleeAttackedBy);

	_scriptValues.load(node, shared);
}

/**
 * Saves the soldier to a YAML file.
 * @return YAML node.
 */
YAML::Node BattleUnit::save(const ScriptGlobal *shared) const
{
	YAML::Node node;

	node["id"] = _id;
	node["genUnitType"] = _type;
	node["genUnitArmor"] = _armor->getType();
	node["faction"] = (int)_faction;
	node["status"] = (int)_status;
	if (_wantsToSurrender)
		node["wantsToSurrender"] = _wantsToSurrender;
	if (_isSurrendering)
		node["isSurrendering"] = _isSurrendering;
	node["position"] = _pos;
	node["direction"] = _direction;
	node["directionTurret"] = _directionTurret;
	node["tu"] = _tu;
	node["health"] = _health;
	node["mana"] = _mana;
	node["stunlevel"] = _stunlevel;
	node["energy"] = _energy;
	node["morale"] = _morale;
	if (_kneeled)
		node["kneeled"] = _kneeled;
	if (_floating)
		node["floating"] = _floating;
	node["armor"].SetStyle(YAML::EmitterStyle::Flow); for (int i=0; i < SIDE_MAX; i++) node["armor"].push_back(_currentArmor[i]);
	node["fatalWounds"].SetStyle(YAML::EmitterStyle::Flow); for (int i=0; i < BODYPART_MAX; i++) node["fatalWounds"].push_back(_fatalWounds[i]);
	node["fire"] = _fire;
	node["expBravery"] = _exp.bravery;
	node["expReactions"] = _exp.reactions;
	node["expFiring"] = _exp.firing;
	node["expThrowing"] = _exp.throwing;
	node["expPsiSkill"] = _exp.psiSkill;
	node["expPsiStrength"] = _exp.psiStrength;
	node["expMana"] = _exp.mana;
	node["expMelee"] = _exp.melee;
	node["currStats"] = _stats;
	if (_turretType > -1)
		node["turretType"] = _turretType;
	if (_visible)
		node["visible"] = _visible;
	node["turnsSinceSpotted"] = _turnsSinceSpotted;
	node["turnsLeftSpottedForSnipers"] = _turnsLeftSpottedForSnipers;
	node["turnsSinceStunned"] = _turnsSinceStunned;
	node["rankInt"] = _rankInt;
	node["moraleRestored"] = _moraleRestored;
	if (getAIModule())
	{
		node["AI"] = getAIModule()->save();
	}
	node["killedBy"] = (int)_killedBy; // does not have a default value, must always be saved
	if (_originalFaction != _faction)
		node["originalFaction"] = (int)_originalFaction;
	if (_kills)
		node["kills"] = _kills;
	if (_faction == FACTION_PLAYER && _dontReselect)
		node["dontReselect"] = _dontReselect;

	if (_spawnUnit)
	{
		node["spawnUnit"] = _spawnUnit->getType();
		node["respawn"] = _respawn;
		node["spawnUnitFaction"] = (int)_spawnUnitFaction;
	}

	node["motionPoints"] = _motionPoints;
	if (_alreadyRespawned)
		node["alreadyRespawned"] = _alreadyRespawned;
	node["activeHand"] = _activeHand;
	if (!_preferredHandForReactions.empty())
		node["preferredHandForReactions"] = _preferredHandForReactions;
	node["tempUnitStatistics"] = _statistics->save();
	if (_murdererId)
		node["murdererId"] = _murdererId;
	if (_fatalShotSide)
		node["fatalShotSide"] = (int)_fatalShotSide;
	if (_fatalShotBodyPart)
		node["fatalShotBodyPart"] = (int)_fatalShotBodyPart;
	if (!_murdererWeapon.empty())
		node["murdererWeapon"] = _murdererWeapon;
	if (!_murdererWeaponAmmo.empty())
		node["murdererWeaponAmmo"] = _murdererWeaponAmmo;

	for (size_t i = 0; i < _recolor.size(); ++i)
	{
		YAML::Node p;
		p.SetStyle(YAML::EmitterStyle::Flow);
		p.push_back((int)_recolor[i].first);
		p.push_back((int)_recolor[i].second);
		node["recolor"].push_back(p);
	}
	if (_mindControllerID)
		node["mindControllerID"] = _mindControllerID;
	if (_summonedPlayerUnit)
		node["summonedPlayerUnit"] = _summonedPlayerUnit;
	if (_resummonedFakeCivilian)
		node["resummonedFakeCivilian"] = _resummonedFakeCivilian;
	if (_pickUpWeaponsMoreActively)
		node["pickUpWeaponsMoreActively"] = _pickUpWeaponsMoreActively;
	if (_disableIndicators)
		node["disableIndicators"] = _disableIndicators;

	if (_originalMovementType != _movementType)
		node["movementType"] = (int)_movementType;

	{
		YAML::Node p;
		if (_moveCostBase != _armor->getMoveCostBase())
		{
			_moveCostBase.save(p, "basePercent");
		}
		if (_moveCostBaseFly != _armor->getMoveCostBaseFly())
		{
			_moveCostBaseFly.save(p, "baseFlyPercent");
		}
		if (_moveCostBaseNormal != _armor->getMoveCostBaseNormal())
		{
			_moveCostBaseNormal.save(p, "baseNormalPercent");
		}
		if (!p.IsNull())
		{
			p.SetStyle(YAML::EmitterStyle::Flow);
			node["moveCost"] = p;
		}
	}
	if (_vip)
		node["vip"] = _vip;
	if (_bannedInNextStage)
		node["bannedInNextStage"] = _bannedInNextStage;
	if (!_meleeAttackedBy.empty())
	{
		node["meleeAttackedBy"] = _meleeAttackedBy;
	}

	_scriptValues.save(node, shared);

	return node;
}

/**
 * Prepare vector values for recolor.
 * @param basicLook select index for hair and face color.
 * @param utileLook select index for utile color.
 * @param rankLook select index for rank color.
 */
void BattleUnit::setRecolor(int basicLook, int utileLook, int rankLook)
{
	_recolor.clear(); // reset in case of OXCE on-the-fly armor changes/transformations
	const int colorsMax = 4;
	std::pair<int, int> colors[colorsMax] =
	{
		std::make_pair(_armor->getFaceColorGroup(), _armor->getFaceColor(basicLook)),
		std::make_pair(_armor->getHairColorGroup(), _armor->getHairColor(basicLook)),
		std::make_pair(_armor->getUtileColorGroup(), _armor->getUtileColor(utileLook)),
		std::make_pair(_armor->getRankColorGroup(), _armor->getRankColor(rankLook)),
	};

	for (int i = 0; i < colorsMax; ++i)
	{
		if (colors[i].first > 0 && colors[i].second > 0)
		{
			_recolor.push_back(std::make_pair(colors[i].first << 4, colors[i].second));
		}
	}
}

/**
 * Returns the BattleUnit's unique ID.
 * @return Unique ID.
 */
int BattleUnit::getId() const
{
	return _id;
}

/**
 * Calculates the distance squared between the unit and a given position.
 * @param pos The position.
 * @return Distance squared.
 */
int BattleUnit::distance3dToPositionSq(const Position& pos) const
{
	int x = _pos.x - pos.x;
	int y = _pos.y - pos.y;
	int z = _pos.z - pos.z;
	if (isBigUnit())
	{
		if (_pos.x < pos.x)
			x++;
		if (_pos.y < pos.y)
			y++;
	}
	return x*x + y*y + z*z;
}

/**
 * Calculates the distance squared between the unit and a given other unit.
 * @param otherUnit The other unit.
 * @param considerZ Whether to consider the z coordinate.
 * @return Distance squared.
 */
int BattleUnit::distance3dToUnitSq(BattleUnit* otherUnit) const
{
	// TODO?: distance calculation isn't precise for 2x2 units here
	// and even though improving just the distance calculation would be easy
	// it would also require changes on other places:
	// - we would need to improve AI targeting involving 2x2 units
	// - we would need to improve reaction fire targeting involving 2x2 units
	// and that is NOT trivial and currently not worth the effort
	// PS: targeting involving a player-controlled 2x2 unit/actor was fixed recently, OXC PR #1307 commit dd7f938

	return Position::distanceSq(_pos, otherUnit->getPosition());
}

/**
 * Changes the BattleUnit's position.
 * @param pos position
 * @param updateLastPos refresh last stored position
 */
void BattleUnit::setPosition(Position pos, bool updateLastPos)
{
	if (updateLastPos) { _lastPos = _pos; }
	_pos = pos;
}

/**
 * Gets the BattleUnit's position.
 * @return position
 */
Position BattleUnit::getPosition() const
{
	return _pos;
}

/**
 * Gets the BattleUnit's position.
 * @return position
 */
Position BattleUnit::getLastPosition() const
{
	return _lastPos;
}

/**
 * Gets position of unit center in voxels.
 * @return position in voxels
 */
Position BattleUnit::getPositionVexels() const
{
	Position center = _pos.toVoxel();
	center += Position(8, 8, 0) * _armor->getSize();
	return center;
}

/**
 * Gets the BattleUnit's destination.
 * @return destination
 */
Position BattleUnit::getDestination() const
{
	return _destination;
}

/**
 * Changes the BattleUnit's (horizontal) direction.
 * Only used for initial unit placement.
 * @param direction new horizontal direction
 */
void BattleUnit::setDirection(int direction)
{
	_direction = direction;
	_toDirection = direction;
	_directionTurret = direction;
	_toDirectionTurret = direction;
}

/**
 * Changes the BattleUnit's (horizontal) face direction.
 * Only used for strafing moves.
 * @param direction new face direction
 */
void BattleUnit::setFaceDirection(int direction)
{
	_faceDirection = direction;
}

/**
 * Gets the BattleUnit's (horizontal) direction.
 * @return horizontal direction
 */
int BattleUnit::getDirection() const
{
	return _direction;
}

/**
 * Gets the BattleUnit's (horizontal) face direction.
 * Used only during strafing moves.
 * @return face direction
 */
int BattleUnit::getFaceDirection() const
{
	return _faceDirection;
}

/**
 * Gets the BattleUnit's turret direction.
 * @return direction
 */
int BattleUnit::getTurretDirection() const
{
	return _directionTurret;
}

/**
 * Gets the BattleUnit's turret To direction.
 * @return toDirectionTurret
 */
int BattleUnit::getTurretToDirection() const
{
	return _toDirectionTurret;
}

/**
 * Gets the BattleUnit's vertical direction. This is when going up or down.
 * @return direction
 */
int BattleUnit::getVerticalDirection() const
{
	return _verticalDirection;
}

/**
 * Gets the unit's status.
 * @return the unit's status
 */
UnitStatus BattleUnit::getStatus() const
{
	return _status;
}

/**
* Does the unit want to surrender?
* @return True if the unit wants to surrender
*/
bool BattleUnit::wantsToSurrender() const
{
	return _wantsToSurrender;
}

/**
 * Is the unit surrendering this turn?
 * @return True if the unit is surrendering (on this turn)
 */
bool BattleUnit::isSurrendering() const
{
	return _isSurrendering;
}

/**
 * Mark the unit as surrendering this turn.
 * @param isSurrendering
 */
void BattleUnit::setSurrendering(bool isSurrendering)
{
	_isSurrendering = isSurrendering;
}

/**
 * Initialises variables to start walking.
 * @param direction Which way to walk.
 * @param destination The position we should end up on.
 * @param savedBattleGame Which is used to get tile is currently below the unit.
 */
void BattleUnit::startWalking(int direction, Position destination, SavedBattleGame *savedBattleGame)
{
	if (direction >= Pathfinding::DIR_UP)
	{
		_verticalDirection = direction;
		_status = STATUS_FLYING;
	}
	else
	{
		_direction = direction;
		_status = STATUS_WALKING;
	}
	if (_haveNoFloorBelow || direction >= Pathfinding::DIR_UP)
	{
		_status = STATUS_FLYING;
		_floating = true;
	}
	else
	{
		_floating = false;
	}

	_walkPhase = 0;
	_destination = destination;
	_lastPos = _pos;
	_kneeled = false;
	if (_breathFrame >= 0)
	{
		_breathing = false;
		_breathFrame = 0;
	}
}

/**
 * This will increment the walking phase.
 * @param savedBattleGame Pointer to save to get tile currently below the unit.
 * @param fullWalkCycle Do full walk cycle or short one when unit is off screen.
 */
void BattleUnit::keepWalking(SavedBattleGame *savedBattleGame, bool fullWalkCycle)
{
	int middle, end;
	if (_verticalDirection)
	{
		middle = 4;
		end = 8;
	}
	else
	{
		// diagonal walking takes double the steps
		middle = 4 + 4 * (_direction % 2);
		end = 8 + 8 * (_direction % 2);
		if (isBigUnit())
		{
			if (_direction < 1 || _direction > 5)
				middle = end;
			else if (_direction == 5)
				middle = 12;
			else if (_direction == 1)
				middle = 5;
			else
				middle = 1;
		}
	}

	if (!fullWalkCycle)
	{
		_pos = _destination;
		end = 2;
	}

	_walkPhase++;

	if (_walkPhase == middle)
	{
		// we assume we reached our destination tile
		// this is actually a drawing hack, so soldiers are not overlapped by floor tiles
		_pos = _destination;
	}

	if (!fullWalkCycle || (_walkPhase == middle))
	{
		setTile(savedBattleGame->getTile(_destination), savedBattleGame);
	}

	if (_walkPhase >= end)
	{
		if (_floating && !_haveNoFloorBelow)
		{
			_floating = false;
		}
		// we officially reached our destination tile
		_status = STATUS_STANDING;
		_walkPhase = 0;
		_verticalDirection = 0;
		if (_faceDirection >= 0) {
			// Finish strafing move facing the correct way.
			_direction = _faceDirection;
			_faceDirection = -1;
		}

		// motion points calculation for the motion scanner blips
		if (isBigUnit())
		{
			_motionPoints += 30;
		}
		else
		{
			// sectoids actually have less motion points
			// but instead of create yet another variable,
			// I used the height of the unit instead (logical)
			if (getStandHeight() > 16)
				_motionPoints += 4;
			else
				_motionPoints += 3;
		}
	}
}

/**
 * Gets the walking phase for animation and sound.
 * @return phase will always go from 0-7
 */
int BattleUnit::getWalkingPhase() const
{
	return _walkPhase % 8;
}

/**
 * Gets the walking phase for diagonal walking.
 * @return phase this will be 0 or 8
 */
int BattleUnit::getDiagonalWalkingPhase() const
{
	return (_walkPhase / 8) * 8;
}

/**
 * Look at a point.
 * @param point Position to look at.
 * @param turret True to turn the turret, false to turn the unit.
 */
void BattleUnit::lookAt(Position point, bool turret)
{
	int dir = directionTo (point);

	if (turret)
	{
		_toDirectionTurret = dir;
		if (_toDirectionTurret != _directionTurret)
		{
			_status = STATUS_TURNING;
		}
	}
	else
	{
		_toDirection = dir;
		if (_toDirection != _direction
			&& _toDirection < 8
			&& _toDirection > -1)
		{
			_status = STATUS_TURNING;
		}
	}
}

/**
 * Look at a direction.
 * @param direction Direction to look at.
 * @param force True to reset the direction, false to animate to it.
 */
void BattleUnit::lookAt(int direction, bool force)
{
	if (!force)
	{
		if (direction < 0 || direction >= 8) return;
		_toDirection = direction;
		if (_toDirection != _direction)
		{
			_status = STATUS_TURNING;
		}
	}
	else
	{
		_toDirection = direction;
		_direction = direction;
	}
}

/**
 * Advances the turning towards the target direction.
 * @param turret True to turn the turret, false to turn the unit.
 */
void BattleUnit::turn(bool turret)
{
	int a = 0;

	if (turret)
	{
		if (_directionTurret == _toDirectionTurret)
		{
			abortTurn();
			return;
		}
		a = _toDirectionTurret - _directionTurret;
	}
	else
	{
		if (_direction == _toDirection)
		{
			abortTurn();
			return;
		}
		a = _toDirection - _direction;
	}

	if (a != 0) {
		if (a > 0) {
			if (a <= 4) {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret++;
					_direction++;
				} else _directionTurret++;
			} else {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret--;
					_direction--;
				} else _directionTurret--;
			}
		} else {
			if (a > -4) {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret--;
					_direction--;
				} else _directionTurret--;
			} else {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret++;
					_direction++;
				} else _directionTurret++;
			}
		}
		if (_direction < 0) _direction = 7;
		if (_direction > 7) _direction = 0;
		if (_directionTurret < 0) _directionTurret = 7;
		if (_directionTurret > 7) _directionTurret = 0;
	}

	if (turret)
	{
		 if (_toDirectionTurret == _directionTurret)
		 {
			// we officially reached our destination
			_status = STATUS_STANDING;
		 }
	}
	else if (_toDirection == _direction || _status == STATUS_UNCONSCIOUS)
	{
		// we officially reached our destination
		_status = STATUS_STANDING;
	}
}

/**
 * Stops the turning towards the target direction.
 */
void BattleUnit::abortTurn()
{
	_status = STATUS_STANDING;
}


/**
 * Gets the soldier's gender.
 */
SoldierGender BattleUnit::getGender() const
{
	return _gender;
}

/**
 * Returns the unit's faction.
 * @return Faction. (player, hostile or neutral)
 */
UnitFaction BattleUnit::getFaction() const
{
	return _faction;
}

/**
 * Gets values used for recoloring sprites.
 * @param i what value choose.
 * @return Pairs of value, where first is color group to replace and second is new color group with shade.
 */
const std::vector<std::pair<Uint8, Uint8> > &BattleUnit::getRecolor() const
{
	return _recolor;
}

/**
 * Kneel down.
 * @param kneeled to kneel or to stand up
 */
void BattleUnit::kneel(bool kneeled)
{
	_kneeled = kneeled;
}

/**
 * Is kneeled down?
 * @return true/false
 */
bool BattleUnit::isKneeled() const
{
	return _kneeled;
}

/**
 * Is floating? A unit is floating when there is no ground under him/her.
 * @return true/false
 */
bool BattleUnit::isFloating() const
{
	return _floating;
}

/**
 * Aim. (shows the right hand sprite and weapon holding)
 * @param aiming true/false
 */
void BattleUnit::aim(bool aiming)
{
	if (aiming)
		_status = STATUS_AIMING;
	else
		_status = STATUS_STANDING;
}

/**
 * Returns the direction from this unit to a given point.
 * 0 <-> y = -1, x = 0
 * 1 <-> y = -1, x = 1
 * 3 <-> y = 1, x = 1
 * 5 <-> y = 1, x = -1
 * 7 <-> y = -1, x = -1
 * @param point given position.
 * @return direction.
 */
int BattleUnit::directionTo(Position point) const
{
	double ox = point.x - _pos.x;
	double oy = point.y - _pos.y;
	double angle = atan2(ox, -oy);
	// divide the pie in 4 angles each at 1/8th before each quarter
	double pie[4] = {(M_PI_4 * 4.0) - M_PI_4 / 2.0, (M_PI_4 * 3.0) - M_PI_4 / 2.0, (M_PI_4 * 2.0) - M_PI_4 / 2.0, (M_PI_4 * 1.0) - M_PI_4 / 2.0};
	int dir = 0;

	if (angle > pie[0] || angle < -pie[0])
	{
		dir = 4;
	}
	else if (angle > pie[1])
	{
		dir = 3;
	}
	else if (angle > pie[2])
	{
		dir = 2;
	}
	else if (angle > pie[3])
	{
		dir = 1;
	}
	else if (angle < -pie[1])
	{
		dir = 5;
	}
	else if (angle < -pie[2])
	{
		dir = 6;
	}
	else if (angle < -pie[3])
	{
		dir = 7;
	}
	else if (angle < pie[0])
	{
		dir = 0;
	}
	return dir;
}

/**
 * Returns the soldier's amount of time units.
 * @return Time units.
 */
int BattleUnit::getTimeUnits() const
{
	return _tu;
}

/**
 * Returns the soldier's amount of energy.
 * @return Energy.
 */
int BattleUnit::getEnergy() const
{
	return _energy;
}

/**
 * Returns the soldier's amount of health.
 * @return Health.
 */
int BattleUnit::getHealth() const
{
	return _health;
}

/**
 * Returns the soldier's amount of mana.
 * @return Mana.
 */
int BattleUnit::getMana() const
{
	return _mana;
}

/**
 * Returns the soldier's amount of morale.
 * @return Morale.
 */
int BattleUnit::getMorale() const
{
	return _morale;
}

/**
 * Get overkill damage to unit.
 * @return Damage over normal health.
 */
int BattleUnit::getOverKillDamage() const
{
	return std::max(-_health - (int)(_stats.health * _armor->getOverKill()), 0);
}

/**
 * Helper function for setting value with max bound.
 */
static inline void setValueMax(int& value, int diff, int min, int max)
{
	value = Clamp(value + diff, min, max);
}

/**
 * Do an amount of damage.
 * @param relative The relative position of which part of armor and/or bodypart is hit.
 * @param damage The amount of damage to inflict.
 * @param type The type of damage being inflicted.
 * @return damage done after adjustment
 */
int BattleUnit::damage(Position relative, int damage, const RuleDamageType *type, SavedBattleGame *save, BattleActionAttack attack, UnitSide sideOverride, UnitBodyPart bodypartOverride)
{
	if (save->isPreview())
	{
		return 0;
	}
	UnitSide side = SIDE_FRONT;
	UnitBodyPart bodypart = BODYPART_TORSO;

	_hitByAnything = true;
	if (_health <= 0)
	{
		return 0;
	}

	RNG::RandomState rand = RNG::globalRandomState().subSequence();
	damage = reduceByResistance(damage, type->ResistType);

	if (!type->IgnoreDirection)
	{
		if (relative == Position(0, 0, 0))
		{
			side = SIDE_UNDER;
		}
		else
		{
			int relativeDirection;
			const int abs_x = abs(relative.x);
			const int abs_y = abs(relative.y);
			if (abs_y > abs_x * 2)
				relativeDirection = 8 + 4 * (relative.y > 0);
			else if (abs_x > abs_y * 2)
				relativeDirection = 10 + 4 * (relative.x < 0);
			else
			{
				if (relative.x < 0)
				{
					if (relative.y > 0)
						relativeDirection = 13;
					else
						relativeDirection = 15;
				}
				else
				{
					if (relative.y > 0)
						relativeDirection = 11;
					else
						relativeDirection = 9;
				}
			}

			switch((relativeDirection - _direction) % 8)
			{
			case 0:	side = SIDE_FRONT; 										break;
			case 1:	side = RNG::generate(0,2) < 2 ? SIDE_FRONT:SIDE_RIGHT; 	break;
			case 2:	side = SIDE_RIGHT; 										break;
			case 3:	side = RNG::generate(0,2) < 2 ? SIDE_REAR:SIDE_RIGHT; 	break;
			case 4:	side = SIDE_REAR; 										break;
			case 5:	side = RNG::generate(0,2) < 2 ? SIDE_REAR:SIDE_LEFT; 	break;
			case 6:	side = SIDE_LEFT; 										break;
			case 7:	side = RNG::generate(0,2) < 2 ? SIDE_FRONT:SIDE_LEFT; 	break;
			}
			if (relative.z >= getHeight())
			{
				bodypart = BODYPART_HEAD;
			}
			else if (relative.z > 4)
			{
				switch(side)
				{
				case SIDE_LEFT:		bodypart = BODYPART_LEFTARM; break;
				case SIDE_RIGHT:	bodypart = BODYPART_RIGHTARM; break;
				default:			bodypart = BODYPART_TORSO;
				}
			}
			else
			{
				switch(side)
				{
				case SIDE_LEFT: 	bodypart = BODYPART_LEFTLEG; 	break;
				case SIDE_RIGHT:	bodypart = BODYPART_RIGHTLEG; 	break;
				default:
					bodypart = (UnitBodyPart) rand.generate(BODYPART_RIGHTLEG,BODYPART_LEFTLEG);
				}
			}
		}
	}

	const int orgDamage = damage;
	const int overKillMinimum = type->IgnoreOverKill ? 0 : -UnitStats::OverkillMultipler * _stats.health;


	{
		ModScript::HitUnit::Output args { damage, bodypart, side, };
		ModScript::HitUnit::Worker work { this, attack.damage_item, attack.weapon_item, attack.attacker, save, attack.skill_rules, orgDamage, type->ResistType, attack.type };

		if (attack.damage_item)
		{
			work.execute(attack.damage_item->getRules()->getScript<ModScript::HitUnitAmmo>(), args);
		}

		work.execute(this->getArmor()->getScript<ModScript::HitUnit>(), args);

		damage = args.getFirst();
		bodypart = (UnitBodyPart)args.getSecond();
		side = (UnitSide)args.getThird();
		if (bodypart >= BODYPART_MAX)
		{
			bodypart = {};
		}
		if (side >= SIDE_MAX)
		{
			side = {};
		}
	}

	// side and bodypart overrides (used by environmental conditions only)
	if (sideOverride != SIDE_MAX)
	{
		side = sideOverride;
	}
	if (bodypartOverride != BODYPART_MAX)
	{
		bodypart = bodypartOverride;
	}


	const RuleItem *specialDamageTransform = attack.damage_item ? attack.damage_item->getRules() : nullptr;
	int specialDamageTransformChance = 0;

	if (specialDamageTransform
		&& !specialDamageTransform->getZombieUnit(this).empty()
		&& getArmor()->getZombiImmune() == false)
	{
		specialDamageTransformChance = getOriginalFaction() != FACTION_HOSTILE ? specialDamageTransform->getSpecialChance() : 0;
	}
	else
	{
		specialDamageTransform = nullptr;
	}


	// update state of unit stats
	if (damage > 0)
	{
		constexpr int toHealth = 0;
		constexpr int toArmor = 1;
		constexpr int toStun = 2;
		constexpr int toTime = 3;
		constexpr int toEnergy = 4;
		constexpr int toMorale = 5;
		constexpr int toWound = 6;
		constexpr int toTransform = 7;
		constexpr int toMana = 8;

		ModScript::DamageUnit::Output args { };

		std::get<toTransform>(args.data) += specialDamageTransformChance;
		std::get<toArmor>(args.data) += type->getArmorPreFinalDamage(damage);

		if (type->ArmorEffectiveness > 0.0f)
		{
			damage -= getArmor(side) * type->ArmorEffectiveness;
		}

		if (damage > 0)
		{
			// stun level change
			std::get<toStun>(args.data) += type->getStunFinalDamage(damage);

			// morale change
			std::get<toMorale>(args.data) += type->getMoraleFinalDamage(damage);

			// time units change
			std::get<toTime>(args.data) += type->getTimeFinalDamage(damage);

			// health change
			std::get<toHealth>(args.data) += type->getHealthFinalDamage(damage);

			// mana change
			std::get<toMana>(args.data) += type->getManaFinalDamage(damage);

			// energy change
			std::get<toEnergy>(args.data) += type->getEnergyFinalDamage(damage);

			// fatal wounds change
			std::get<toWound>(args.data) += type->getWoundFinalDamage(damage);

			// armor value change
			std::get<toArmor>(args.data) += type->getArmorFinalDamage(damage);
		}

		ModScript::DamageUnit::Worker work { this, attack.damage_item, attack.weapon_item, attack.attacker, save, attack.skill_rules, damage, orgDamage, bodypart, side, type->ResistType, attack.type, };

		if (attack.damage_item)
		{
			work.execute(attack.damage_item->getRules()->getScript<ModScript::DamageUnitAmmo>(), args);
		}

		work.execute(this->getArmor()->getScript<ModScript::DamageUnit>(), args);

		if (!_armor->getPainImmune() || type->IgnorePainImmunity)
		{
			setValueMax(_stunlevel, std::get<toStun>(args.data), 0, UnitStats::StunMultipler * _stats.health);
		}

		moraleChange(- reduceByBravery(std::get<toMorale>(args.data)));

		setValueMax(_tu, - std::get<toTime>(args.data), 0, _stats.tu);

		setValueMax(_health, - std::get<toHealth>(args.data), std::min(overKillMinimum, _health), _stats.health); // `std::min` required because of script that could set `_health = -100`, if we do not have "overkill" `-100` become min value allowed by this line, if "overkill" then this line can go lower than this.

		setValueMax(_mana, - std::get<toMana>(args.data), 0, _stats.mana);

		setValueMax(_energy, - std::get<toEnergy>(args.data), 0, _stats.stamina);

		if (isWoundable())
		{
			setValueMax(_fatalWounds[bodypart], std::get<toWound>(args.data), 0, 100);
			moraleChange(-std::get<toWound>(args.data));
		}

		setValueMax(_currentArmor[side], - std::get<toArmor>(args.data), 0, _maxArmor[side]);


		setFatalShotInfo(side, bodypart);


		damage = std::get<toHealth>(args.data);
		specialDamageTransformChance = std::get<toTransform>(args.data);
	}

	// special effects
	if (save->getBattleState())
	{
		constexpr int arg_specialDamageTransform = 0;
		constexpr int arg_specialDamageTransformChance = 1;
		constexpr int arg_selfDestruct = 2;
		constexpr int arg_selfDestructChance = 3;
		constexpr int arg_moraleLoss = 4;
		constexpr int arg_fire = 5;

		ModScript::DamageSpecialUnit::Output args { };

		// chance to tranform
		std::get<arg_specialDamageTransform>(args.data) = specialDamageTransform != nullptr;
		std::get<arg_specialDamageTransformChance>(args.data) = specialDamageTransformChance;

		// morale loss based on final damage to health
		if (type->IgnoreNormalMoraleLose == false)
		{
			const int bravery = reduceByBravery(10);
			const int modifier = getFaction() == FACTION_PLAYER ? save->getFactionMoraleModifier(true) : 100;

			std::get<arg_moraleLoss>(args.data) = 100 * (damage * bravery / 10) / modifier;
		}

		// self destruction
		std::get<arg_selfDestruct>(args.data) = (getSpecialAbility() == SPECAB_EXPLODEONDEATH || getSpecialAbility() == SPECAB_BURN_AND_EXPLODE);
		if (std::get<arg_selfDestruct>(args.data) && !isOut() && isOutThresholdExceed())
		{
			if (type->IgnoreSelfDestruct == false)
			{
				std::get<arg_selfDestructChance>(args.data) = 100;
			}
		}

		// normal fire
		std::get<arg_fire>(args.data) = getFire();
		if (damage >= type->FireThreshold)
		{
			float resistance = getArmor()->getDamageModifier(type->ResistType);
			if (resistance > 0.0)
			{
				int burnTime = rand.generate(0, int(5.0f * resistance));
				if (std::get<arg_fire>(args.data) < burnTime)
				{
					std::get<arg_fire>(args.data) = burnTime; // catch fire and burn
				}
			}
		}
		// fire extinguisher
		if (std::get<arg_fire>(args.data) > 0)
		{
			if (attack.weapon_item && attack.weapon_item->getRules()->isFireExtinguisher())
			{
				// firearm, melee weapon, or even a grenade...
				std::get<arg_fire>(args.data) = 0;
			}
			else if (attack.damage_item && attack.damage_item->getRules()->isFireExtinguisher())
			{
				// bullet/ammo
				std::get<arg_fire>(args.data) = 0;
			}
		}


		// script call

		ModScript::DamageSpecialUnit::Worker work { this, attack.damage_item, attack.weapon_item, attack.attacker, save, attack.skill_rules, damage, orgDamage, bodypart, side, type->ResistType, attack.type, };

		work.execute(this->getArmor()->getScript<ModScript::DamageSpecialUnit>(), args);


		// update state
		moraleChange(-std::get<arg_moraleLoss>(args.data));
		setFire(std::get<arg_fire>(args.data));

		// check if this unit turns others into zombies
		if (rand.percent(std::get<arg_specialDamageTransformChance>(args.data)) && specialDamageTransform
			&& !getSpawnUnit())
		{
			auto typeName = specialDamageTransform->getZombieUnit(this);
			auto type = save->getMod()->getUnit(typeName);
			if (type->getArmor()->getSize() <= getArmor()->getSize())
			{
				// converts the victim to a zombie on death
				setRespawn(true);
				setSpawnUnitFaction(FACTION_HOSTILE);
				setSpawnUnit(type);
			}
			else
			{
				Log(LOG_ERROR) << "Transforming armor type '" << this->getArmor()->getType() << "' to unit type '" << typeName << "' is not allowed because of bigger armor size";
			}
		}

		if (rand.percent(std::get<arg_selfDestructChance>(args.data))
			&& !hasAlreadyExploded())
		{
			setAlreadyExploded(true);
			Position p = getPosition().toVoxel();
			save->getBattleGame()->statePushNext(new ExplosionBState(save->getBattleGame(), p, BattleActionAttack{ BA_SELF_DESTRUCT, this, }, 0));
		}
	}

	return damage;
}

/**
 * Do an amount of stun recovery.
 * @param power
 */
void BattleUnit::healStun(int power)
{
	_stunlevel -= power;
	if (_stunlevel < 0) _stunlevel = 0;
}

int BattleUnit::getStunlevel() const
{
	return _stunlevel;
}

bool BattleUnit::hasNegativeHealthRegen() const
{
	if (_health > 0)
	{
		int HPRecovery = 0;

		// apply soldier bonuses
		if (_geoscapeSoldier)
		{
			for (auto bonusRule : *_geoscapeSoldier->getBonuses(nullptr))
			{
				HPRecovery += bonusRule->getHealthRecovery(this);
			}
		}

		return _armor->getHealthRecovery(this, HPRecovery) < 0;
	}
	return false;
}

/**
 * Raises a unit's stun level sufficiently so that the unit is ready to become unconscious.
 * Used when another unit falls on top of this unit.
 * Zombified units first convert to their spawn unit.
 * @param battle Pointer to the battlescape game.
 */
void BattleUnit::knockOut(BattlescapeGame *battle)
{
	if (_spawnUnit)
	{
		setRespawn(false);
		BattleUnit *newUnit = battle->convertUnit(this);

		if (newUnit)
		{
			if (newUnit->getSpawnUnit())
			{
				//scripts or rulesets could make new chryssalid from chryssalid, this means we could have infinite loop there
				//setting null will break it
				newUnit->clearSpawnUnit();
			}

			newUnit->knockOut(battle);
		}
	}
	else
	{
		_stunlevel = std::max(_health, 1);
	}
}

/**
 * Initialises the falling sequence. Occurs after death or stunned.
 */
void BattleUnit::startFalling()
{
	_status = STATUS_COLLAPSING;
	_fallPhase = 0;
	_turnsSinceStunned = 0;
}

/**
 * Advances the phase of falling sequence.
 */
void BattleUnit::keepFalling()
{
	_fallPhase++;
	if (_fallPhase == _armor->getDeathFrames())
	{
		_fallPhase--;
		if (_health <= 0)
		{
			_status = STATUS_DEAD;
		}
		else
			_status = STATUS_UNCONSCIOUS;
	}
}

/**
 * Set final falling state. Skipping animation.
 */
void BattleUnit::instaFalling()
{
	startFalling();
	_fallPhase =  _armor->getDeathFrames() - 1;
	if (_health <= 0)
	{
		_status = STATUS_DEAD;
	}
	else
	{
		_status = STATUS_UNCONSCIOUS;
	}
}


/**
 * Returns the phase of the falling sequence.
 * @return phase
 */
int BattleUnit::getFallingPhase() const
{
	return _fallPhase;
}

/**
 * Returns whether the soldier is out of combat, dead or unconscious.
 * A soldier that is out, cannot perform any actions, cannot be selected, but it's still a unit.
 * @return flag if out or not.
 */
bool BattleUnit::isOut() const
{
	return _status == STATUS_DEAD || _status == STATUS_UNCONSCIOUS || isIgnored();
}

/**
 * Return true when unit stun level is greater that current health or unit have no health.
 * @return true if unit should be knockout.
 */
bool BattleUnit::isOutThresholdExceed() const
{
	return getHealth() <= 0 || getHealth() <= getStunlevel();
}

/**
 * Unit is removed from game.
 */
bool BattleUnit::isIgnored() const
{
	return _status == STATUS_IGNORE_ME;
}

/**
 * Get the number of time units a certain action takes.
 * @param actionType
 * @param item
 * @return TUs
 */
RuleItemUseCost BattleUnit::getActionTUs(BattleActionType actionType, const BattleItem *item) const
{
	if (item == 0)
	{
		return 0;
	}
	return getActionTUs(actionType, item->getRules());
}

/**
 * Get the number of time units a certain skill action takes.
 * @param actionType
 * @param skillRules
 * @return TUs
 */
RuleItemUseCost BattleUnit::getActionTUs(BattleActionType actionType, const RuleSkill *skillRules) const
{
	if (skillRules == 0)
	{
		return 0;
	}
	RuleItemUseCost cost(skillRules->getCost());
	applyPercentages(cost, skillRules->getFlat());

	return cost;
}

/**
 * Get the number of time units a certain action takes.
 * @param actionType
 * @param item
 * @return TUs
 */
RuleItemUseCost BattleUnit::getActionTUs(BattleActionType actionType, const RuleItem *item) const
{
	RuleItemUseCost cost;
	if (item != 0)
	{
		RuleItemUseCost flat = item->getFlatUse();
		switch (actionType)
		{
			case BA_PRIME:
				flat = item->getFlatPrime();
				cost = item->getCostPrime();
				break;
			case BA_UNPRIME:
				flat = item->getFlatUnprime();
				cost = item->getCostUnprime();
				break;
			case BA_THROW:
				flat = item->getFlatThrow();
				cost = item->getCostThrow();
				break;
			case BA_AUTOSHOT:
				flat = item->getFlatAuto();
				cost = item->getCostAuto();
				break;
			case BA_SNAPSHOT:
				flat = item->getFlatSnap();
				cost = item->getCostSnap();
				break;
			case BA_HIT:
				flat = item->getFlatMelee();
				cost = item->getCostMelee();
				break;
			case BA_LAUNCH:
			case BA_AIMEDSHOT:
				flat = item->getFlatAimed();
				cost = item->getCostAimed();
				break;
			case BA_USE:
				cost = item->getCostUse();
				break;
			case BA_MINDCONTROL:
				cost = item->getCostMind();
				break;
			case BA_PANIC:
				cost = item->getCostPanic();
				break;
			default:
				break;
		}

		applyPercentages(cost, flat);
	}
	return cost;
}

void BattleUnit::applyPercentages(RuleItemUseCost &cost, const RuleItemUseCost &flat) const
{
	{
		// if it's a percentage, apply it to unit TUs
		if (!flat.Time && cost.Time)
		{
			cost.Time = std::max(1, (int)floor(getBaseStats()->tu * cost.Time / 100.0f));
		}
		// if it's a percentage, apply it to unit Energy
		if (!flat.Energy && cost.Energy)
		{
			cost.Energy = std::max(1, (int)floor(getBaseStats()->stamina * cost.Energy / 100.0f));
		}
		// if it's a percentage, apply it to unit Morale
		if (!flat.Morale && cost.Morale)
		{
			cost.Morale = std::max(1, (int)floor((110 - getBaseStats()->bravery) * cost.Morale / 100.0f));
		}
		// if it's a percentage, apply it to unit Health
		if (!flat.Health && cost.Health)
		{
			cost.Health = std::max(1, (int)floor(getBaseStats()->health * cost.Health / 100.0f));
		}
		// if it's a percentage, apply it to unit Health
		if (!flat.Stun && cost.Stun)
		{
			cost.Stun = std::max(1, (int)floor(getBaseStats()->health * cost.Stun / 100.0f));
		}
		// if it's a percentage, apply it to unit Mana
		if (!flat.Mana && cost.Mana)
		{
			cost.Mana = std::max(1, (int)floor(getBaseStats()->mana * cost.Mana / 100.0f));
		}
	}
}

/**
 * Spend time units if it can. Return false if it can't.
 * @param tu
 * @return flag if it could spend the time units or not.
 */
bool BattleUnit::spendTimeUnits(int tu)
{
	if (tu <= _tu)
	{
		_tu -= tu;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Spend energy  if it can. Return false if it can't.
 * @param energy
 * @return flag if it could spend the time units or not.
 */
bool BattleUnit::spendEnergy(int energy)
{
	if (energy <= _energy)
	{
		_energy -= energy;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Spend resources cost without checking.
 * @param cost
 */
void BattleUnit::spendCost(const RuleItemUseCost& cost)
{
	_tu -= cost.Time;
	_energy -= cost.Energy;
	_morale -= cost.Morale;
	_health -= cost.Health;
	_stunlevel += cost.Stun;
	_mana -= cost.Mana;
}

/**
 * Clear number of time units.
 */
void BattleUnit::clearTimeUnits()
{
	_tu = 0;
}

/**
 * Reset time units and energy.
 */
void BattleUnit::resetTimeUnitsAndEnergy()
{
	_tu = _stats.tu;
	_energy = _stats.stamina;
}

/**
 * Add this unit to the list of visible units. Returns true if this is a new one.
 * @param unit
 * @return
 */
bool BattleUnit::addToVisibleUnits(BattleUnit *unit)
{
	bool add = true;
	for (std::vector<BattleUnit*>::iterator i = _unitsSpottedThisTurn.begin(); i != _unitsSpottedThisTurn.end();++i)
	{
		if ((BattleUnit*)(*i) == unit)
		{
			add = false;
			break;
		}
	}
	if (add)
	{
		_unitsSpottedThisTurn.push_back(unit);
	}
	for (std::vector<BattleUnit*>::iterator i = _visibleUnits.begin(); i != _visibleUnits.end(); ++i)
	{
		if ((BattleUnit*)(*i) == unit)
		{
			return false;
		}
	}
	_visibleUnits.push_back(unit);
	return true;
}

/**
* Removes the given unit from the list of visible units.
* @param unit The unit to add to our visibility cache.
* @return true if unit found and removed
*/
bool BattleUnit::removeFromVisibleUnits(BattleUnit *unit)
{
	if (!_visibleUnits.size()) {
		return false;
	}
	std::vector<BattleUnit*>::iterator i = std::find(_visibleUnits.begin(), _visibleUnits.end(), unit);
	if (i == _visibleUnits.end())
	{
		return false;
	}
	//Slow to remove stuff from vector as it shuffles all the following items. Swap in rearmost element before removal.
	(*i) = *(_visibleUnits.end() - 1);
	_visibleUnits.pop_back();
	return true;
}

/**
* Checks if the given unit is on the list of visible units.
* @param unit The unit to check whether we have in our visibility cache.
* @return true if on the visible list or of the same faction
*/
bool BattleUnit::hasVisibleUnit(BattleUnit *unit)
{
	if (getFaction() == unit->getFaction())
	{
		//Units of same faction are always visible, but not stored in the visible unit list
		return true;
	}
	return std::find(_visibleUnits.begin(), _visibleUnits.end(), unit) != _visibleUnits.end();
}

/**
 * Get the pointer to the vector of visible units.
 * @return pointer to vector.
 */
std::vector<BattleUnit*> *BattleUnit::getVisibleUnits()
{
	return &_visibleUnits;
}

/**
 * Clear visible units.
 */
void BattleUnit::clearVisibleUnits()
{
	_visibleUnits.clear();
}

/**
 * Add this unit to the list of visible tiles.
 * @param tile that we're now able to see.
 * @return true if a new tile.
 */
bool BattleUnit::addToVisibleTiles(Tile *tile)
{
	//Only add once, otherwise we're going to mess up the visibility value and make trouble for the AI (if sneaky).
	if (_visibleTilesLookup.insert(tile).second)
	{
		tile->setVisible(1);
		_visibleTiles.push_back(tile);
		return true;
	}
	return false;
}

/**
 * Get the pointer to the vector of visible tiles.
 * @return pointer to vector.
 */
const std::vector<Tile*> *BattleUnit::getVisibleTiles()
{
	return &_visibleTiles;
}

/**
 * Clears visible tiles. Also reduces the associated visibility counter used by the AI.
 */
void BattleUnit::clearVisibleTiles()
{
	for (std::vector<Tile*>::iterator j = _visibleTiles.begin(); j != _visibleTiles.end(); ++j)
	{
		(*j)->setVisible(-1);
	}
	_visibleTilesLookup.clear();
	_visibleTiles.clear();
}

/**
 * Get accuracy of different types of psi attack.
 * @param actionType Psi attack type.
 * @param item Psi-Amp.
 * @return Attack bonus.
 */
int BattleUnit::getPsiAccuracy(BattleActionAttack::ReadOnly attack)
{
	auto actionType = attack.type;
	auto item = attack.weapon_item;

	int psiAcc = 0;
	if (actionType == BA_MINDCONTROL)
	{
		psiAcc = item->getRules()->getAccuracyMind();
	}
	else if (actionType == BA_PANIC)
	{
		psiAcc = item->getRules()->getAccuracyPanic();
	}
	else if (actionType == BA_USE)
	{
		psiAcc = item->getRules()->getAccuracyUse();
	}

	psiAcc += item->getRules()->getAccuracyMultiplier(attack);

	return psiAcc;
}

/**
 * Calculate firing accuracy.
 * Formula = accuracyStat * weaponAccuracy * kneeling bonus(1.15) * one-handPenalty(0.8) * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param actionType
 * @param item
 * @return firing Accuracy
 */
int BattleUnit::getFiringAccuracy(BattleActionAttack::ReadOnly attack, Mod *mod)
{
	auto actionType = attack.type;
	auto item = attack.weapon_item;
	const int modifier = attack.attacker->getAccuracyModifier(item);
	int result = 0;
	bool kneeled = attack.attacker->_kneeled;

	if (actionType == BA_SNAPSHOT)
	{
		result = item->getRules()->getAccuracyMultiplier(attack) * item->getRules()->getAccuracySnap() / 100;
	}
	else if (actionType == BA_AIMEDSHOT || actionType == BA_LAUNCH)
	{
		result = item->getRules()->getAccuracyMultiplier(attack) * item->getRules()->getAccuracyAimed() / 100;
	}
	else if (actionType == BA_AUTOSHOT)
	{
		result = item->getRules()->getAccuracyMultiplier(attack) * item->getRules()->getAccuracyAuto() / 100;
	}
	else if (actionType == BA_HIT)
	{
		kneeled = false;
		result = item->getRules()->getMeleeMultiplier(attack) * item->getRules()->getAccuracyMelee() / 100;
	}
	else if (actionType == BA_THROW)
	{
		kneeled = false;
		result = item->getRules()->getThrowMultiplier(attack) * item->getRules()->getAccuracyThrow() / 100;
	}
	else if (actionType == BA_CQB)
	{
		kneeled = false;
		result = item->getRules()->getCloseQuartersMultiplier(attack) * item->getRules()->getAccuracyCloseQuarters(mod) / 100;
	}

	if (kneeled)
	{
		result = result * item->getRules()->getKneelBonus(mod) / 100;
	}

	if (item->getRules()->isTwoHanded() && actionType != BA_THROW)
	{
		// two handed weapon, means one hand should be empty
		if (attack.attacker->getRightHandWeapon() != 0 && attack.attacker->getLeftHandWeapon() != 0)
		{
			result = result * item->getRules()->getOneHandedPenalty(mod) / 100;
		}
		else if (item->getRules()->isSpecialUsingEmptyHand())
		{
			// for special weapons that use an empty hand... already one hand with an item is enough for the penalty to apply
			if (attack.attacker->getRightHandWeapon() != 0 || attack.attacker->getLeftHandWeapon() != 0)
			{
				result = result * item->getRules()->getOneHandedPenalty(mod) / 100;
			}
		}
	}

	return result * modifier / 100;
}

/**
 * To calculate firing accuracy. Takes health and fatal wounds into account.
 * Formula = accuracyStat * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param item the item we are shooting right now.
 * @return modifier
 */
int BattleUnit::getAccuracyModifier(const BattleItem *item) const
{
	int wounds = _fatalWounds[BODYPART_HEAD];

	if (item)
	{
		if (item->getRules()->isTwoHanded())
		{
			wounds += _fatalWounds[BODYPART_RIGHTARM] + _fatalWounds[BODYPART_LEFTARM];
		}
		else
		{
			auto slot = item->getSlot();
			if (slot)
			{
				// why broken hands should affect your aim if you shoot not using them?
				if (slot->isRightHand())
				{
					wounds += _fatalWounds[BODYPART_RIGHTARM];
				}
				if (slot->isLeftHand())
				{
					wounds += _fatalWounds[BODYPART_LEFTARM];
				}
			}
		}
	}
	return std::max(10, 25 * _health / getBaseStats()->health + 75 + -10 * wounds);
}

/**
 * Set the armor value of a certain armor side.
 * @param armor Amount of armor.
 * @param side The side of the armor.
 */
void BattleUnit::setArmor(int armor, UnitSide side)
{
	_currentArmor[side] = Clamp(armor, 0, _maxArmor[side]);
}

/**
 * Get the armor value of a certain armor side.
 * @param side The side of the armor.
 * @return Amount of armor.
 */
int BattleUnit::getArmor(UnitSide side) const
{
	return _currentArmor[side];
}

/**
 * Get the max armor value of a certain armor side.
 * @param side The side of the armor.
 * @return Amount of armor.
 */
int BattleUnit::getMaxArmor(UnitSide side) const
{
	return _maxArmor[side];
}

/**
 * Get total amount of fatal wounds this unit has.
 * @return Number of fatal wounds.
 */
int BattleUnit::getFatalWounds() const
{
	int sum = 0;
	for (int i = 0; i < BODYPART_MAX; ++i)
		sum += _fatalWounds[i];
	return sum;
}


/**
 * Little formula to calculate reaction score.
 * @return Reaction score.
 */
double BattleUnit::getReactionScore() const
{
	//(Reactions Stat) x (Current Time Units / Max TUs)
	double score = ((double)getBaseStats()->reactions * (double)getTimeUnits()) / (double)getBaseStats()->tu;
	return score;
}

/**
 * Helper function preparing Time Units recovery at beginning of turn.
 * @param tu New time units for this turn.
 */
void BattleUnit::prepareTimeUnits(int tu)
{
	if (!isOut())
	{
		// Add to previous turn TU, if regen is less than normal unit need couple of turns to regen full bar
		setValueMax(_tu, tu, 0, getBaseStats()->tu);

		// Apply reductions, if new TU == 0 then it could make not spend TU decay
		float encumbrance = (float)getBaseStats()->strength / (float)getCarriedWeight();
		if (encumbrance < 1)
		{
		  _tu = int(encumbrance * _tu);
		}
		// Each fatal wound to the left or right leg reduces the soldier's TUs by 10%.
		_tu -= (_tu * ((_fatalWounds[BODYPART_LEFTLEG]+_fatalWounds[BODYPART_RIGHTLEG]) * 10))/100;

		setValueMax(_tu, 0, 0, getBaseStats()->tu);
	}
}

/**
 * Helper function preparing Energy recovery at beginning of turn.
 * @param energy Energy grain this turn.
 */
void BattleUnit::prepareEnergy(int energy)
{
	if (!isOut())
	{
		// Each fatal wound to the body reduces the soldier's energy recovery by 10%.
		energy -= (_energy * (_fatalWounds[BODYPART_TORSO] * 10))/100;

		setValueMax(_energy, energy, 0, getBaseStats()->stamina);
	}
}

/**
 * Helper function preparing Health recovery at beginning of turn.
 * @param health Health grain this turn.
 */
void BattleUnit::prepareHealth(int health)
{
	// suffer from fatal wounds
	health -= getFatalWounds();

	// suffer from fire
	if (!_hitByFire && _fire > 0)
	{
		health -= reduceByResistance(RNG::generate(Mod::FIRE_DAMAGE_RANGE[0], Mod::FIRE_DAMAGE_RANGE[1]), DT_IN);
		_fire--;
	}

	setValueMax(_health, health, -UnitStats::OverkillMultipler * _stats.health, _stats.health);

	// if unit is dead, AI state should be gone
	if (_health <= 0 && _currentAIState)
	{
		delete _currentAIState;
		_currentAIState = 0;
	}
}

/**
 * Helper function preparing Mana recovery at beginning of turn.
 * @param mana Mana gain this turn.
 */
void BattleUnit::prepareMana(int mana)
{
	if (!isOut())
	{
		setValueMax(_mana, mana, 0, getBaseStats()->mana);
	}
}

/**
 * Helper function preparing Stun recovery at beginning of turn.
 * @param stun Stun damage reduction this turn.
 */
void BattleUnit::prepareStun(int stun)
{
	if (isSmallUnit() || !isOut())
	{
		healStun(stun);
	}
}

/**
 * Helper function preparing Morale recovery at beginning of turn.
 * @param morale Morale grain this turn.
 */
void BattleUnit::prepareMorale(int morale)
{
	if (!isOut())
	{
		moraleChange(morale);
		int chance = 100 - (2 * getMorale());
		if (RNG::generate(1,100) <= chance)
		{
			int type = RNG::generate(0,100);
			int berserkChance = _unitRules ? _unitRules->getBerserkChance() : 33;
			_status = (type <= berserkChance ? STATUS_BERSERK : STATUS_PANICKING); // 33% chance of berserk, panic can mean freeze or flee, but that is determined later
			_wantsToSurrender = true;
		}
		else
		{
			// successfully avoided panic
			// increase bravery experience counter
			if (chance > 1)
				addBraveryExp();
		}
	}
	else
	{
		// knocked out units are willing to surrender if they wake up
		if (_status == STATUS_UNCONSCIOUS)
			_wantsToSurrender = true;
	}
}
/**
 * Prepare for a new turn.
 */
void BattleUnit::prepareNewTurn(bool fullProcess)
{
	if (isIgnored())
	{
		return;
	}

	_isSurrendering = false;
	_unitsSpottedThisTurn.clear();
	_meleeAttackedBy.clear();

	_hitByFire = false;
	_dontReselect = false;
	_motionPoints = 0;

	if (!isOut())
	{
		incTurnsSinceStunned();
	}

	// don't give it back its TUs or anything this round
	// because it's no longer a unit of the team getting TUs back
	if (_faction != _originalFaction)
	{
		_faction = _originalFaction;
		if (_faction == FACTION_PLAYER && _currentAIState)
		{
			delete _currentAIState;
			_currentAIState = 0;
		}
		return;
	}
	else
	{
		updateUnitStats(true, false);
	}

	// transition between stages, don't do damage or panic
	if (!fullProcess)
	{
		if (_kneeled)
		{
			// stand up if kneeling
			_kneeled = false;
		}
		return;
	}

	updateUnitStats(false, true);
}

/**
 * Update stats of unit.
 * @param tuAndEnergy
 * @param rest
 */
void BattleUnit::updateUnitStats(bool tuAndEnergy, bool rest)
{
	// snapshot of current stats
	int TURecovery = 0;
	int ENRecovery = 0;

	if (tuAndEnergy)
	{
		// apply soldier bonuses
		if (_geoscapeSoldier)
		{
			for (auto bonusRule : *_geoscapeSoldier->getBonuses(nullptr))
			{
				TURecovery += bonusRule->getTimeRecovery(this);
				ENRecovery += bonusRule->getEnergyRecovery(this);
			}
		}

		//unit update will be done after other stats are calculated and updated
	}

	if (rest)
	{
		// snapshot of current stats
		int HPRecovery = 0;
		int MNRecovery = 0;
		int MRRecovery = 0;
		int STRecovery = 0;

		// apply soldier bonuses
		if (_geoscapeSoldier)
		{
			for (auto bonusRule : *_geoscapeSoldier->getBonuses(nullptr))
			{
				HPRecovery += bonusRule->getHealthRecovery(this);
				MNRecovery += bonusRule->getManaRecovery(this);
				MRRecovery += bonusRule->getMoraleRecovery(this);
				STRecovery += bonusRule->getStunRegeneration(this);
			}
		}

		// update stats
		prepareHealth(_armor->getHealthRecovery(this, HPRecovery));
		prepareMana(_armor->getManaRecovery(this, MNRecovery));
		prepareMorale(_armor->getMoraleRecovery(this, MRRecovery));
		prepareStun(_armor->getStunRegeneration(this, STRecovery));
	}

	if (tuAndEnergy)
	{
		// update stats
		prepareTimeUnits(_armor->getTimeRecovery(this, TURecovery));
		prepareEnergy(_armor->getEnergyRecovery(this, ENRecovery));
	}
}

/**
 * Morale change with bounds check.
 * @param change can be positive or negative
 */
void BattleUnit::moraleChange(int change)
{
	if (!isFearable()) return;

	_morale += change;
	if (_morale > 100)
		_morale = 100;
	if (_morale < 0)
		_morale = 0;
}

/**
 * Get reduced morale change value by bravery.
 */
int BattleUnit::reduceByBravery(int moraleChange) const
{
	return (110 - _stats.bravery) * moraleChange / 100;
}

/**
 * Calculate power reduction by resistances.
 */
int BattleUnit::reduceByResistance(int power, ItemDamageType resistType) const
{
	return (int)floor(power * _armor->getDamageModifier(resistType));
}

/**
 * Mark this unit as not reselectable.
 */
void BattleUnit::dontReselect()
{
	_dontReselect = true;
}

/**
 * Mark this unit as reselectable.
 */
void BattleUnit::allowReselect()
{
	_dontReselect = false;
}


/**
 * Check whether reselecting this unit is allowed.
 * @return bool
 */
bool BattleUnit::reselectAllowed() const
{
	return !_dontReselect;
}

/**
 * Set the amount of turns this unit is on fire. 0 = no fire.
 * @param fire : amount of turns this tile is on fire.
 */
void BattleUnit::setFire(int fire)
{
	if (_specab != SPECAB_BURNFLOOR && _specab != SPECAB_BURN_AND_EXPLODE)
		_fire = fire;
}

/**
 * Get the amount of turns this unit is on fire. 0 = no fire.
 * @return fire : amount of turns this tile is on fire.
 */
int BattleUnit::getFire() const
{
	return _fire;
}

/**
 * Get the pointer to the vector of inventory items.
 * @return pointer to vector.
 */
std::vector<BattleItem*> *BattleUnit::getInventory()
{
	return &_inventory;
}

/**
 * Fit item into inventory slot.
 * @param slot Slot to fit.
 * @param item Item to fit.
 * @return True if succeeded, false otherwise.
 */
bool BattleUnit::fitItemToInventory(RuleInventory *slot, BattleItem *item)
{
	auto rule = item->getRules();
	if (rule->canBePlacedIntoInventorySection(slot) == false)
	{
		return false;
	}
	if (slot->getType() == INV_HAND)
	{
		if (!Inventory::overlapItems(this, item, slot))
		{
			item->moveToOwner(this);
			item->setSlot(slot);
			return true;
		}
	}
	else if (slot->getType() == INV_SLOT)
	{
		for (const RuleSlot &rs : *slot->getSlots())
		{
			if (!Inventory::overlapItems(this, item, slot, rs.x, rs.y) && slot->fitItemInSlot(rule, rs.x, rs.y))
			{
				item->moveToOwner(this);
				item->setSlot(slot);
				item->setSlotX(rs.x);
				item->setSlotY(rs.y);
				return true;
			}
		}
	}
	return false;
}

/**
 * Adds an item to an XCom soldier (auto-equip).
 * @param item Pointer to the Item.
 * @param mod Pointer to the Mod.
 * @param save Pointer to the saved battle game for storing items.
 * @param allowSecondClip allow the unit to take a second clip or not. (only applies to xcom soldiers, aliens are allowed regardless of this flag)
 * @param allowAutoLoadout allow auto equip of weapons for solders.
 * @param allowUnloadedWeapons allow equip of weapons without ammo.
 * @return if the item was placed or not.
 */
bool BattleUnit::addItem(BattleItem *item, const Mod *mod, bool allowSecondClip, bool allowAutoLoadout, bool allowUnloadedWeapons)
{
	RuleInventory *rightHand = mod->getInventoryRightHand();
	RuleInventory *leftHand = mod->getInventoryLeftHand();
	bool placed = false;
	bool loaded = false;
	const RuleItem *rule = item->getRules();
	int weight = 0;

	// tanks and aliens don't care about weight or multiple items,
	// their loadouts are defined in the rulesets and more or less set in stone.
	if (getFaction() == FACTION_PLAYER && hasInventory() && !isSummonedPlayerUnit())
	{
		weight = getCarriedWeight() + item->getTotalWeight();
		// allow all weapons to be loaded by avoiding this check,
		// they'll return false later anyway if the unit has something in his hand.
		if (rule->getBattleType() != BT_FIREARM && rule->getBattleType() != BT_MELEE)
		{
			int tally = 0;
			for (BattleItem *i : *getInventory())
			{
				if (rule->getType() == i->getRules()->getType())
				{
					if (allowSecondClip && rule->getBattleType() == BT_AMMO)
					{
						tally++;
						if (tally == 2)
						{
							return false;
						}
					}
					else
					{
						// we already have one, thanks.
						return false;
					}
				}
			}
		}
	}

	// place fixed weapon
	if (rule->isFixed())
	{
		// either in the default slot provided in the ruleset
		if (rule->getDefaultInventorySlot())
		{
			RuleInventory *defaultSlot = const_cast<RuleInventory *>(rule->getDefaultInventorySlot());
			BattleItem *defaultSlotWeapon = getItem(defaultSlot);
			if (!defaultSlotWeapon)
			{
				item->moveToOwner(this);
				item->setSlot(defaultSlot);
				item->setSlotX(rule->getDefaultInventorySlotX());
				item->setSlotY(rule->getDefaultInventorySlotY());
				placed = true;
				item->setXCOMProperty(getFaction() == FACTION_PLAYER && !isSummonedPlayerUnit());
				if (item->getRules()->getTurretType() > -1)
				{
					setTurretType(item->getRules()->getTurretType());
				}
			}
		}
		// or in the left/right hand
		if (!placed && (fitItemToInventory(rightHand, item) || fitItemToInventory(leftHand, item)))
		{
			placed = true;
			item->setXCOMProperty(getFaction() == FACTION_PLAYER && !isSummonedPlayerUnit());
			if (item->getRules()->getTurretType() > -1)
			{
				setTurretType(item->getRules()->getTurretType());
			}
		}
		return placed;
	}

	// we equip item only if we have skill to use it.
	if (getBaseStats()->psiSkill <= 0 && rule->isPsiRequired())
	{
		return false;
	}

	if (rule->isManaRequired() && getOriginalFaction() == FACTION_PLAYER)
	{
		// don't auto-equip items that require mana for now, maybe reconsider in the future
		return false;
	}

	bool keep = true;
	switch (rule->getBattleType())
	{
	case BT_FIREARM:
	case BT_MELEE:
		if (item->haveAnyAmmo() || getFaction() != FACTION_PLAYER || !hasInventory() || allowUnloadedWeapons)
		{
			loaded = true;
		}

		if (loaded && (getGeoscapeSoldier() == 0 || allowAutoLoadout))
		{
			if (getBaseStats()->strength * 0.66 >= weight) // weight is always considered 0 for aliens
			{
				if (fitItemToInventory(rightHand, item))
				{
					placed = true;
				}
				bool allowTwoMainWeapons = (getFaction() != FACTION_PLAYER) || _armor->getAllowTwoMainWeapons();
				if (!placed && allowTwoMainWeapons && fitItemToInventory(leftHand, item))
				{
					placed = true;
				}
			}
		}
		break;
	case BT_AMMO:
		{
			BattleItem *rightWeapon = getRightHandWeapon();
			BattleItem *leftWeapon = getLeftHandWeapon();
			// xcom weapons will already be loaded, aliens and tanks, however, get their ammo added afterwards.
			// so let's try to load them here.
			if (rightWeapon && (rightWeapon->getRules()->isFixed() || getFaction() != FACTION_PLAYER || allowUnloadedWeapons) &&
				rightWeapon->isWeaponWithAmmo() && rightWeapon->setAmmoPreMission(item))
			{
				placed = true;
				break;
			}
			if (leftWeapon && (leftWeapon->getRules()->isFixed() || getFaction() != FACTION_PLAYER || allowUnloadedWeapons) &&
				leftWeapon->isWeaponWithAmmo() && leftWeapon->setAmmoPreMission(item))
			{
				placed = true;
				break;
			}
			// don't take ammo for weapons we don't have.
			keep = (getFaction() != FACTION_PLAYER);
			if (rightWeapon)
			{
				if (rightWeapon->getRules()->getSlotForAmmo(rule) != -1)
				{
					keep = true;
				}
			}
			if (leftWeapon)
			{
				if (leftWeapon->getRules()->getSlotForAmmo(rule) != -1)
				{
					keep = true;
				}
			}
			if (!keep)
			{
				break;
			}
			FALLTHROUGH;
		}
	default:
		if (rule->getBattleType() == BT_PSIAMP && getFaction() == FACTION_HOSTILE)
		{
			if (fitItemToInventory(rightHand, item) || fitItemToInventory(leftHand, item))
			{
				placed = true;
			}
		}
		else if ((getGeoscapeSoldier() == 0 || allowAutoLoadout))
		{
			if (getBaseStats()->strength >= weight) // weight is always considered 0 for aliens
			{
				// this is `n*(log(n) + log(n))` code, it could be `n` but we would lose predefined order, as `RuleItem` have them in effective in random order (depending on global memory allocations)
				for (const std::string &s : mod->getInvsList())
				{
					RuleInventory *slot = mod->getInventory(s);
					if (slot->getType() == INV_SLOT)
					{
						placed = fitItemToInventory(slot, item);
						if (placed)
						{
							break;
						}
					}
				}
			}
		}
		break;
	}

	item->setXCOMProperty(getFaction() == FACTION_PLAYER && !isSummonedPlayerUnit());

	return placed;
}

/**
 * Let AI do their thing.
 * @param action AI action.
 */
void BattleUnit::think(BattleAction *action)
{
	reloadAmmo();
	_currentAIState->think(action);
}

/**
 * Changes the current AI state.
 * @param aiState Pointer to AI state.
 */
void BattleUnit::setAIModule(AIModule *ai)
{
	if (_currentAIState)
	{
		delete _currentAIState;
	}
	_currentAIState = ai;
}

/**
 * Returns the current AI state.
 * @return Pointer to AI state.
 */
AIModule *BattleUnit::getAIModule() const
{
	return _currentAIState;
}

/**
 * Set whether this unit is visible.
 * @param flag
 */
void BattleUnit::setVisible(bool flag)
{
	_visible = flag;
}


/**
 * Get whether this unit is visible.
 * @return flag
 */
bool BattleUnit::getVisible() const
{
	if (getFaction() == FACTION_PLAYER || _armor->isAlwaysVisible())
	{
		return true;
	}
	else
	{
		return _visible;
	}
}

/**
 * Check if unit can fall down.
 * @param saveBattleGame
 */
void BattleUnit::updateTileFloorState(SavedBattleGame *saveBattleGame)
{
	if (_tile)
	{
		auto armorSize = _armor->getSize() - 1;
		auto newPos = _tile->getPosition();
		_haveNoFloorBelow = true;
		for (int x = armorSize; x >= 0; --x)
		{
			for (int y = armorSize; y >= 0; --y)
			{
				auto t = saveBattleGame->getTile(newPos + Position(x, y, 0));
				if (t)
				{
					if (!t->hasNoFloor(saveBattleGame))
					{
						_haveNoFloorBelow = false;
						return;
					}
				}
			}
		}
	}
	else
	{
		_haveNoFloorBelow = false;
	}
}
/**
 * Sets the unit's tile it's standing on
 * @param tile Pointer to tile.
 * @param saveBattleGame Pointer to save to get tile below.
 */
void BattleUnit::setTile(Tile *tile, SavedBattleGame *saveBattleGame)
{
	if (_tile == tile)
	{
		return;
	}

	auto armorSize = _armor->getSize() - 1;
	// Reset tiles moved from.
	if (_tile)
	{
		auto prevPos = _tile->getPosition();
		for (int x = armorSize; x >= 0; --x)
		{
			for (int y = armorSize; y >= 0; --y)
			{
				auto t = saveBattleGame->getTile(prevPos + Position(x,y, 0));
				if (t && t->getUnit() == this)
				{
					t->setUnit(nullptr);
				}
			}
		}
	}

	_tile = tile;
	if (!_tile)
	{
		_floating = false;
		_haveNoFloorBelow = false;
		return;
	}

	// Update tiles moved to.
	auto newPos = _tile->getPosition();
	_haveNoFloorBelow = true;
	for (int x = armorSize; x >= 0; --x)
	{
		for (int y = armorSize; y >= 0; --y)
		{
			auto t = saveBattleGame->getTile(newPos + Position(x, y, 0));
			if (t)
			{
				_haveNoFloorBelow &= t->hasNoFloor(saveBattleGame);
				t->setUnit(this);
			}
		}
	}

	// unit could have changed from flying to walking or vice versa
	if (_status == STATUS_WALKING && _haveNoFloorBelow && _movementType == MT_FLY)
	{
		_status = STATUS_FLYING;
		_floating = true;
	}
	else if (_status == STATUS_FLYING && !_haveNoFloorBelow && _verticalDirection == 0)
	{
		_status = STATUS_WALKING;
		_floating = false;
	}
	else if (_status == STATUS_UNCONSCIOUS)
	{
		_floating = _movementType == MT_FLY && _haveNoFloorBelow;
	}
}

/**
 * Set only unit tile without any additional logic.
 * Used only in before battle, other wise will break game.
 * Need call setTile after to fix links
 * @param tile
 */
void BattleUnit::setInventoryTile(Tile *tile)
{
	_tile = tile;
}

/**
 * Gets the unit's tile.
 * @return Tile
 */
Tile *BattleUnit::getTile() const
{
	return _tile;
}

/**
 * Checks if there's an inventory item in
 * the specified inventory position.
 * @param slot Inventory slot.
 * @param x X position in slot.
 * @param y Y position in slot.
 * @return Item in the slot, or NULL if none.
 */
BattleItem *BattleUnit::getItem(RuleInventory *slot, int x, int y) const
{
	// Soldier items
	if (slot->getType() != INV_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator i = _inventory.begin(); i != _inventory.end(); ++i)
		{
			if ((*i)->getSlot() == slot && (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	// Ground items
	else if (_tile != 0)
	{
		for (std::vector<BattleItem*>::const_iterator i = _tile->getInventory()->begin(); i != _tile->getInventory()->end(); ++i)
		{
			if ((*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	return 0;
}

/**
 * Get the "main hand weapon" from the unit.
 * @param quickest Whether to get the quickest weapon, default true
 * @return Pointer to item.
 */
BattleItem *BattleUnit::getMainHandWeapon(bool quickest) const
{
	BattleItem *weaponRightHand = getRightHandWeapon();
	BattleItem *weaponLeftHand = getLeftHandWeapon();

	// ignore weapons without ammo (rules out grenades)
	if (!weaponRightHand || !weaponRightHand->haveAnyAmmo())
		weaponRightHand = 0;
	if (!weaponLeftHand || !weaponLeftHand->haveAnyAmmo())
		weaponLeftHand = 0;

	// if there is only one weapon, it's easy:
	if (weaponRightHand && !weaponLeftHand)
		return weaponRightHand;
	else if (!weaponRightHand && weaponLeftHand)
		return weaponLeftHand;
	else if (!weaponRightHand && !weaponLeftHand)
	{
		// Allow *AI* to use also a special weapon, but only when both hands are empty
		// Only need to check for firearms since melee/psi is handled elsewhere
		BattleItem* specialWeapon = getSpecialWeapon(BT_FIREARM);
		if (specialWeapon)
		{
			return specialWeapon;
		}

		return 0;
	}

	// otherwise pick the one with the least snapshot TUs
	int tuRightHand = getActionTUs(BA_SNAPSHOT, weaponRightHand).Time;
	int tuLeftHand = getActionTUs(BA_SNAPSHOT, weaponLeftHand).Time;
	BattleItem *weaponCurrentHand = const_cast<BattleItem*>(getActiveHand(weaponLeftHand, weaponRightHand));
	//prioritize blaster
	if (!quickest && _faction != FACTION_PLAYER)
	{
		if (weaponRightHand->getCurrentWaypoints() != 0)
		{
			return weaponRightHand;
		}
		if (weaponLeftHand->getCurrentWaypoints() != 0)
		{
			return weaponLeftHand;
		}
	}
	// if only one weapon has snapshot, pick that one
	if (tuLeftHand <= 0 && tuRightHand > 0)
		return weaponRightHand;
	else if (tuRightHand <= 0 && tuLeftHand > 0)
		return weaponLeftHand;
	// else pick the better one
	else
	{
		if (tuLeftHand >= tuRightHand)
		{
			if (quickest)
			{
				return weaponRightHand;
			}
			else if (_faction == FACTION_PLAYER)
			{
				return weaponCurrentHand;
			}
			else
			{
				return weaponLeftHand;
			}
		}
		else
		{
			if (quickest)
			{
				return weaponLeftHand;
			}
			else if (_faction == FACTION_PLAYER)
			{
				return weaponCurrentHand;
			}
			else
			{
				return weaponRightHand;
			}
		}
	}
}

/**
 * Get a grenade from the belt (used for AI)
 * @return Pointer to item.
 */
BattleItem *BattleUnit::getGrenadeFromBelt() const
{
	for (std::vector<BattleItem*>::const_iterator i = _inventory.begin(); i != _inventory.end(); ++i)
	{
		if ((*i)->getRules()->getBattleType() == BT_GRENADE)
		{
			return *i;
		}
	}
	return 0;
}

/**
 * Gets the item from right hand.
 * @return Item in right hand.
 */
BattleItem *BattleUnit::getRightHandWeapon() const
{
	for (auto i : _inventory)
	{
		auto slot = i->getSlot();
		if (slot && slot->isRightHand())
		{
			return i;
		}
	}
	return nullptr;
}

/**
 *  Gets the item from left hand.
 * @return Item in left hand.
 */
BattleItem *BattleUnit::getLeftHandWeapon() const
{
	for (auto i : _inventory)
	{
		auto slot = i->getSlot();
		if (slot && slot->isLeftHand())
		{
			return i;
		}
	}
	return nullptr;
}

/**
 * Set the right hand as main active hand.
 */
void BattleUnit::setActiveRightHand()
{
	_activeHand = "STR_RIGHT_HAND";
}

/**
 * Set the left hand as main active hand.
 */
void BattleUnit::setActiveLeftHand()
{
	_activeHand = "STR_LEFT_HAND";
}

/**
 * Choose what weapon was last use by unit.
 */
const BattleItem *BattleUnit::getActiveHand(const BattleItem *left, const BattleItem *right) const
{
	if (_activeHand == "STR_RIGHT_HAND" && right) return right;
	if (_activeHand == "STR_LEFT_HAND" && left) return left;
	return left ? left : right;
}

/**
 * Check if we have ammo and reload if needed (used for AI).
 * @return Do we have ammo?
 */
bool BattleUnit::reloadAmmo()
{
	BattleItem *list[2] =
	{
		getRightHandWeapon(),
		getLeftHandWeapon(),
	};

	for (int i = 0; i < 2; ++i)
	{
		BattleItem *weapon = list[i];
		if (!weapon || !weapon->isWeaponWithAmmo() || weapon->haveAllAmmo())
		{
			continue;
		}

		// we have a non-melee weapon with no ammo and 15 or more TUs - we might need to look for ammo then
		BattleItem *ammo = 0;
		auto ruleWeapon = weapon->getRules();
		auto tuCost = getTimeUnits() + 1;
		auto slotAmmo = 0;

		for (BattleItem* bi : *getInventory())
		{
			int slot = ruleWeapon->getSlotForAmmo(bi->getRules());
			if (slot != -1 && !weapon->getAmmoForSlot(slot))
			{
				int tuTemp = (Mod::EXTENDED_ITEM_RELOAD_COST && bi->getSlot()->getType() != INV_HAND) ? bi->getMoveToCost(weapon->getSlot()) : 0;
				tuTemp += ruleWeapon->getTULoad(slot);
				if (tuTemp < tuCost)
				{
					tuCost = tuTemp;
					ammo = bi;
					slotAmmo = slot;
				}
			}
		}

		if (ammo && spendTimeUnits(tuCost))
		{
			weapon->setAmmoForSlot(slotAmmo, ammo);

			auto sound = ammo->getRules()->getReloadSound();
			if (sound == Mod::NO_SOUND)
			{
				sound = ruleWeapon->getReloadSound();
			}
			if (sound == Mod::NO_SOUND)
			{
				sound = Mod::ITEM_RELOAD;
			}

			_lastReloadSound = sound;
			return true;
		}
	}
	return false;
}

/**
 * Toggle the right hand as main hand for reactions.
 */
void BattleUnit::toggleRightHandForReactions()
{
	if (isRightHandPreferredForReactions())
		_preferredHandForReactions = "";
	else
		_preferredHandForReactions = "STR_RIGHT_HAND";
}

/**
 * Toggle the left hand as main hand for reactions.
 */
void BattleUnit::toggleLeftHandForReactions()
{
	if (isLeftHandPreferredForReactions())
		_preferredHandForReactions = "";
	else
		_preferredHandForReactions = "STR_LEFT_HAND";
}

/**
 * Is right hand preferred for reactions?
 */
bool BattleUnit::isRightHandPreferredForReactions() const
{
	return _preferredHandForReactions == "STR_RIGHT_HAND";
}

/**
 * Is left hand preferred for reactions?
 */
bool BattleUnit::isLeftHandPreferredForReactions() const
{
	return _preferredHandForReactions == "STR_LEFT_HAND";
}

/**
 * Get preferred weapon for reactions, if applicable.
 */
BattleItem *BattleUnit::getWeaponForReactions(bool meleeOnly) const
{
	if (_preferredHandForReactions.empty())
		return nullptr;

	BattleItem* weapon = nullptr;
	if (isRightHandPreferredForReactions())
		weapon = getRightHandWeapon();
	else
		weapon = getLeftHandWeapon();

	if (!weapon && meleeOnly)
	{
		// try also empty hands melee
		weapon = getSpecialWeapon(BT_MELEE);
		if (weapon && !weapon->getRules()->isSpecialUsingEmptyHand())
		{
			weapon = nullptr;
		}
	}

	if (!weapon)
		return nullptr;

	if (meleeOnly)
	{
		if (weapon->getRules()->getBattleType() == BT_MELEE)
			return weapon;
	}
	else
	{
		// ignore weapons without ammo (rules out grenades)
		if (!weapon->haveAnyAmmo())
			return nullptr;

		int tu = getActionTUs(BA_SNAPSHOT, weapon).Time;
		if (tu > 0)
			return weapon;

	}

	return nullptr;
}

/**
 * Check if this unit is in the exit area.
 * @param stt Type of exit tile to check for.
 * @return Is in the exit area?
 */
bool BattleUnit::isInExitArea(SpecialTileType stt) const
{
	return liesInExitArea(_tile, stt);
}

/**
 * Check if this unit lies (e.g. unconscious) in the exit area.
 * @param tile Unit's location.
 * @param stt Type of exit tile to check for.
 * @return Is in the exit area?
 */
bool BattleUnit::liesInExitArea(Tile *tile, SpecialTileType stt) const
{
	return tile && tile->getFloorSpecialTileType() == stt;
}

/**
 * Gets the unit height taking into account kneeling/standing.
 * @return Unit's height.
 */
int BattleUnit::getHeight() const
{
	return isKneeled()?getKneelHeight():getStandHeight();
}

/**
 * Adds one to the bravery exp counter.
 */
void BattleUnit::addBraveryExp()
{
	_exp.bravery++;
}

/**
 * Adds one to the reaction exp counter.
 */
void BattleUnit::addReactionExp()
{
	_exp.reactions++;
}

/**
 * Adds one to the firing exp counter.
 */
void BattleUnit::addFiringExp()
{
	_exp.firing++;
}

/**
 * Adds one to the throwing exp counter.
 */
void BattleUnit::addThrowingExp()
{
	_exp.throwing++;
}

/**
 * Adds one to the psi skill exp counter.
 */
void BattleUnit::addPsiSkillExp()
{
	_exp.psiSkill++;
}

/**
 * Adds one to the psi strength exp counter.
 */
void BattleUnit::addPsiStrengthExp()
{
	_exp.psiStrength++;
}

/**
 * Adds to the mana exp counter.
 */
void BattleUnit::addManaExp(int weaponStat)
{
	if (weaponStat > 0)
	{
		_exp.mana += weaponStat / 100;
		if (RNG::percent(weaponStat % 100))
		{
			_exp.mana++;
		}
	}
}

/**
 * Adds one to the melee exp counter.
 */
void BattleUnit::addMeleeExp()
{
	_exp.melee++;
}

/**
 * Did the unit gain any experience yet?
 */
bool BattleUnit::hasGainedAnyExperience()
{
	return _exp.bravery || _exp.reactions || _exp.firing || _exp.psiSkill || _exp.psiStrength || _exp.melee || _exp.throwing || _exp.mana;
}

void BattleUnit::updateGeoscapeStats(Soldier *soldier) const
{
	soldier->addMissionCount();
	soldier->addKillCount(_kills);
}

/**
 * Check if unit eligible for squaddie promotion. If yes, promote the unit.
 * Increase the mission counter. Calculate the experience increases.
 * @param geoscape Pointer to geoscape save.
 * @param statsDiff (out) The passed UnitStats struct will be filled with the stats differences.
 * @return True if the soldier was eligible for squaddie promotion.
 */
bool BattleUnit::postMissionProcedures(const Mod *mod, SavedGame *geoscape, SavedBattleGame *battle, StatAdjustment &statsDiff)
{
	Soldier *s = geoscape->getSoldier(_id);
	if (s == 0)
	{
		return false;
	}

	updateGeoscapeStats(s);

	UnitStats *stats = s->getCurrentStats();
	StatAdjustment statsOld = { };
	statsOld.statGrowth = (*stats);
	statsDiff.statGrowth = -(*stats);        // subtract old stat
	const UnitStats caps = s->getRules()->getStatCaps();
	int manaLossOriginal = _stats.mana - _mana;
	int healthLossOriginal = _stats.health - _health;
	int manaLoss = mod->getReplenishManaAfterMission() ? 0 : manaLossOriginal;
	int healthLoss = mod->getReplenishHealthAfterMission() ? 0 : healthLossOriginal;

	auto recovery = (int)RNG::generate((healthLossOriginal*0.5),(healthLossOriginal*1.5));

	if (_exp.bravery && stats->bravery < caps.bravery)
	{
		if (_exp.bravery > RNG::generate(0,10)) stats->bravery += 10;
	}
	if (_exp.reactions && stats->reactions < caps.reactions)
	{
		stats->reactions += improveStat(_exp.reactions);
	}
	if (_exp.firing && stats->firing < caps.firing)
	{
		stats->firing += improveStat(_exp.firing);
	}
	if (_exp.melee && stats->melee < caps.melee)
	{
		stats->melee += improveStat(_exp.melee);
	}
	if (_exp.throwing && stats->throwing < caps.throwing)
	{
		stats->throwing += improveStat(_exp.throwing);
	}
	if (_exp.psiSkill && stats->psiSkill < caps.psiSkill)
	{
		stats->psiSkill += improveStat(_exp.psiSkill);
	}
	if (_exp.psiStrength && stats->psiStrength < caps.psiStrength)
	{
		stats->psiStrength += improveStat(_exp.psiStrength);
	}
	if (mod->isManaTrainingPrimary())
	{
		if (_exp.mana && stats->mana < caps.mana)
		{
			stats->mana += improveStat(_exp.mana);
		}
	}

	bool hasImproved = false;
	if (hasGainedAnyExperience())
	{
		hasImproved = true;
		if (s->getRank() == RANK_ROOKIE)
			s->promoteRank();
		int v;
		v = caps.tu - stats->tu;
		if (v > 0) stats->tu += RNG::generate(0, v/10 + 2);
		v = caps.health - stats->health;
		if (v > 0) stats->health += RNG::generate(0, v/10 + 2);
		if (mod->isManaTrainingSecondary())
		{
			v = caps.mana - stats->mana;
			if (v > 0) stats->mana += RNG::generate(0, v/10 + 2);
		}
		v = caps.strength - stats->strength;
		if (v > 0) stats->strength += RNG::generate(0, v/10 + 2);
		v = caps.stamina - stats->stamina;
		if (v > 0) stats->stamina += RNG::generate(0, v/10 + 2);
	}

	statsDiff.statGrowth += *stats; // add new stat

	if (_armor->getInstantWoundRecovery())
	{
		recovery = 0;
	}

	{
		ModScript::ReturnFromMissionUnit::Output arg { };
		ModScript::ReturnFromMissionUnit::Worker work{ this, battle, s, &statsDiff, &statsOld };

		auto ref = std::tie(recovery, manaLossOriginal, healthLossOriginal, manaLoss, healthLoss);

		arg.data = ref;

		work.execute(getArmor()->getScript<ModScript::ReturnFromMissionUnit>(), arg);

		ref = arg.data;
	}

	//after mod execution this value could change
	statsDiff.statGrowth = *stats - statsOld.statGrowth;

	s->setWoundRecovery(recovery);
	s->setManaMissing(manaLoss);
	s->setHealthMissing(healthLoss);

	if (s->isWounded())
	{
		// remove from craft
		//s->setCraft(nullptr); // Note to self: we need to do this much later (as late as possible), so that we can correctly remove the items too (without side effects)

		// remove from training, but remember to return to training when healed
		{
			if (s->isInTraining())
			{
				s->setReturnToTrainingWhenHealed(true);
			}
			s->setTraining(false);
		}
	}

	return hasImproved;
}

/**
 * Converts the number of experience to the stat increase.
 * @param Experience counter.
 * @return Stat increase.
 */
int BattleUnit::improveStat(int exp) const
{
	if      (exp > 10) return RNG::generate(2, 6);
	else if (exp > 5)  return RNG::generate(1, 4);
	else if (exp > 2)  return RNG::generate(1, 3);
	else if (exp > 0)  return RNG::generate(0, 1);
	else               return 0;
}

/**
 * Get the unit's minimap sprite index. Used to display the unit on the minimap
 * @return the unit minimap index
 */
int BattleUnit::getMiniMapSpriteIndex() const
{
	//minimap sprite index:
	// * 0-2   : Xcom soldier
	// * 3-5   : Alien
	// * 6-8   : Civilian
	// * 9-11  : Item
	// * 12-23 : Xcom HWP
	// * 24-35 : Alien big terror unit(cyberdisk, ...)
	if (isOut())
	{
		return 9;
	}
	switch (getFaction())
	{
	case FACTION_HOSTILE:
		if (isSmallUnit())
			return 3;
		else
			return 24;
	case FACTION_NEUTRAL:
		if (isSmallUnit())
			return 6;
		else
			return 12;
	default:
		if (isSmallUnit())
			return 0;
		else
			return 12;
	}
}

/**
  * Set the turret type. -1 is no turret.
  * @param turretType
  */
void BattleUnit::setTurretType(int turretType)
{
	_turretType = turretType;
}

/**
  * Get the turret type. -1 is no turret.
  * @return type
  */
int BattleUnit::getTurretType() const
{
	return _turretType;
}

/**
 * Get the amount of fatal wound for a body part
 * @param part The body part (in the range 0-5)
 * @return The amount of fatal wound of a body part
 */
int BattleUnit::getFatalWound(UnitBodyPart part) const
{
	if (part < 0 || part >= BODYPART_MAX)
		return 0;
	return _fatalWounds[part];
}
/**
 * Set fatal wound amount of a body part
 * @param wound The amount of fatal wound of a body part shoud have
 * @param part The body part (in the range 0-5)
 */
void BattleUnit::setFatalWound(int wound, UnitBodyPart part)
{
	if (part < 0 || part >= BODYPART_MAX)
		return;
	_fatalWounds[part] = Clamp(wound, 0, 100);
}

/**
 * Heal a fatal wound of the soldier
 * @param part the body part to heal
 * @param woundAmount the amount of fatal wound healed
 * @param healthAmount The amount of health to add to soldier health
 */
void BattleUnit::heal(UnitBodyPart part, int woundAmount, int healthAmount)
{
	if (part < 0 || part >= BODYPART_MAX || !_fatalWounds[part])
	{
		return;
	}

	setValueMax(_fatalWounds[part], -woundAmount, 0, 100);
	setValueMax(_health, healthAmount, std::min(_health, 1), getBaseStats()->health); //Hippocratic Oath: First do no harm

}

/**
 * Restore soldier morale
 * @param moraleAmount additional morale boost.
 * @param painKillersStrength how much of damage convert to morale.
 */
void BattleUnit::painKillers(int moraleAmount, float painKillersStrength)
{
	int lostHealth = (getBaseStats()->health - _health) * painKillersStrength;
	if (lostHealth > _moraleRestored)
	{
		_morale = std::min(100, (lostHealth - _moraleRestored + _morale));
		_moraleRestored = lostHealth;
	}
	moraleChange(moraleAmount);
}

/**
 * Restore soldier energy and reduce stun level, can restore mana too
 * @param energy The amount of energy to add
 * @param stun The amount of stun level to reduce
 * @param mana The amount of mana to add
 */
void BattleUnit::stimulant(int energy, int stun, int mana)
{
	_energy += energy;
	if (_energy > getBaseStats()->stamina)
		_energy = getBaseStats()->stamina;
	healStun(stun);
	setValueMax(_mana, mana, 0, getBaseStats()->mana);
}


/**
 * Get motion points for the motion scanner. More points
 * is a larger blip on the scanner.
 * @return points.
 */
int BattleUnit::getMotionPoints() const
{
	return _motionPoints;
}

/**
 * Gets the unit's armor.
 * @return Pointer to armor.
 */
const Armor *BattleUnit::getArmor() const
{
	return _armor;
}

/**
 * Set the unit's name.
 * @param name Name
 */
void BattleUnit::setName(const std::string &name)
{
	_name = name;
}

/**
 * Get unit's name.
 * An aliens name is the translation of it's race and rank.
 * hence the language pointer needed.
 * @param lang Pointer to language.
 * @param debugAppendId Append unit ID to name for debug purposes.
 * @return name String of the unit's name.
 */
std::string BattleUnit::getName(Language *lang, bool debugAppendId) const
{
	if (_type != "SOLDIER" && lang != 0)
	{
		std::string ret;

		if (_type.find("STR_") != std::string::npos)
			ret = lang->getString(_type);
		else
			ret = lang->getString(_race);

		if (debugAppendId)
		{
			std::ostringstream ss;
			ss << ret << " " << _id;
			ret = ss.str();
		}
		return ret;
	}

	return _name;
}

/**
  * Gets pointer to the unit's stats.
  * @return stats Pointer to the unit's stats.
  */
UnitStats *BattleUnit::getBaseStats()
{
	return &_stats;
}

/**
  * Gets pointer to the unit's stats.
  * @return stats Pointer to the unit's stats.
  */
const UnitStats *BattleUnit::getBaseStats() const
{
	return &_stats;
}

/**
  * Get the unit's stand height.
  * @return The unit's height in voxels, when standing up.
  */
int BattleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
  * Get the unit's kneel height.
  * @return The unit's height in voxels, when kneeling.
  */
int BattleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
  * Get the unit's floating elevation.
  * @return The unit's elevation over the ground in voxels, when flying.
  */
int BattleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
  * Get the unit's loft ID, one per unit tile.
  * Each tile only has one loft, as it is repeated over the entire height of the unit.
  * @param entry Unit tile
  * @return The unit's line of fire template ID.
  */
int BattleUnit::getLoftemps(int entry) const
{
	return _loftempsSet.at(entry);
}

/**
  * Get the unit's value. Used for score at debriefing.
  * @return value score
  */
int BattleUnit::getValue() const
{
	return _value;
}

/**
 * Get the unit's death sounds.
 * @return List of sound IDs.
 */
const std::vector<int> &BattleUnit::getDeathSounds() const
{
	return _deathSound;
}

/**
 * Get the unit's move sound.
 * @return id.
 */
int BattleUnit::getMoveSound() const
{
	return _moveSound;
}


/**
 * Get whether the unit is affected by fatal wounds.
 * Normally only soldiers are affected by fatal wounds.
 * @return Is the unit affected by wounds?
 */
bool BattleUnit::isWoundable() const
{
	return !_armor->getBleedImmune(!(_type=="SOLDIER" || (Options::alienBleeding && _originalFaction != FACTION_PLAYER)));
}

/**
 * Get whether the unit is affected by morale loss.
 * Normally only small units are affected by morale loss.
 * @return Is the unit affected by morale?
 */
bool BattleUnit::isFearable() const
{
	return !_armor->getFearImmune();
}

/**
 * Is this unit capable of shooting beyond max. visual range?
 * @return True, if unit is capable of shooting beyond max. visual range.
 */
bool BattleUnit::isSniper() const
{
	if (_unitRules && _unitRules->getSniperPercentage() > 0)
	{
		return true;
	}
	return false;
}

/**
 * Gets true when unit is 1x1 sized unit.
 */
bool BattleUnit::isSmallUnit() const
{
	return _armor->getSize() == 1;
}

/**
 * Gets true when unit is 2x2 sized unit.
 */
bool BattleUnit::isBigUnit() const
{
	return _armor->getSize() > 1;
}

/**
 * Get the number of turns an AI unit remembers a soldier's position.
 * @return intelligence.
 */
int BattleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Get the unit's aggression.
 * @return aggression.
 */
int BattleUnit::getAggression() const
{
	return _aggression;
}

int BattleUnit::getMaxViewDistance(int baseVisibility, int nerf, int buff) const
{
	int result = baseVisibility;
	if (nerf > 0)
	{
		result = nerf; // fixed distance nerf
	}
	else
	{
		result += nerf; // relative distance nerf
	}
	if (result < 1)
	{
		result = 1;  // can't go under melee distance
	}
	result += buff; // relative distance buff
	if (result > baseVisibility)
	{
		result = baseVisibility; // don't overbuff (buff is only supposed to counter the nerf)
	}
	return result;
}

int BattleUnit::getMaxViewDistanceAtDark(const Armor *otherUnitArmor) const
{
	if (otherUnitArmor)
	{
		return getMaxViewDistance(_maxViewDistanceAtDark, otherUnitArmor->getCamouflageAtDark(), _armor->getAntiCamouflageAtDark());
	}
	else
	{
		return _maxViewDistanceAtDark;
	}
}

int BattleUnit::getMaxViewDistanceAtDarkSquared() const
{
	return _maxViewDistanceAtDarkSquared;
}

int BattleUnit::getMaxViewDistanceAtDay(const Armor *otherUnitArmor) const
{
	if (otherUnitArmor)
	{
		return getMaxViewDistance(_maxViewDistanceAtDay, otherUnitArmor->getCamouflageAtDay(), _armor->getAntiCamouflageAtDay());
	}
	else
	{
		return _maxViewDistanceAtDay;
	}
}

/**
 * Returns the unit's special ability.
 * @return special ability.
 */
int BattleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Sets this unit to respawn (or not).
 * @param respawn whether it should respawn.
 */
void BattleUnit::setRespawn(bool respawn)
{
	_respawn = respawn;
}

/**
 * Gets this unit's respawn flag.
 */
bool BattleUnit::getRespawn() const
{
	return _respawn;
}

/**
 * Marks this unit as already respawned (or not).
 * @param alreadyRespawned whether it already respawned.
 */
void BattleUnit::setAlreadyRespawned(bool alreadyRespawned)
{
	_alreadyRespawned = alreadyRespawned;
}

/**
 * Gets this unit's alreadyRespawned flag.
 */
bool BattleUnit::getAlreadyRespawned() const
{
	return _alreadyRespawned;
}

/**
 * Get the unit that is spawned when this one dies.
 * @return unit.
 */
const Unit *BattleUnit::getSpawnUnit() const
{
	return _spawnUnit;
}

/**
 * Set the unit that is spawned when this one dies.
 * @param spawnUnit unit.
 */
void BattleUnit::setSpawnUnit(const Unit *spawnUnit)
{
	_spawnUnit = spawnUnit;
}

/**
 * Clear all information for spawn unit.
 */
void BattleUnit::clearSpawnUnit()
{
	setSpawnUnit(nullptr);
	setSpawnUnitFaction(FACTION_HOSTILE);
	setRespawn(false);
}



/**
 * Get the units's rank string.
 * @return rank.
 */
const std::string& BattleUnit::getRankString() const
{
	return _rank;
}

/**
 * Get the geoscape-soldier object.
 * @return soldier.
 */
Soldier *BattleUnit::getGeoscapeSoldier() const
{
	return _geoscapeSoldier;
}

/**
 * Add a kill to the counter.
 */
void BattleUnit::addKillCount()
{
	_kills++;
}

/**
 * Get unit type.
 * @return unit type.
 */
const std::string& BattleUnit::getType() const
{
	return _type;
}

/**
 * Converts unit to another faction (original faction is still stored).
 * @param f faction.
 */
void BattleUnit::convertToFaction(UnitFaction f)
{
	_faction = f;
}

/**
* Set health to 0 - used when getting killed unconscious.
*/
void BattleUnit::kill()
{
	_health = 0;
}

/**
 * Set health to 0 and set status dead - used when getting zombified.
 */
void BattleUnit::instaKill()
{
	_health = 0;
	_status = STATUS_DEAD;
	_turnsSinceStunned = 0;
}

/**
 * Gets whether the unit has any aggro sounds.
 * @return True, if the unit has any aggro sounds.
 */
bool BattleUnit::hasAggroSound() const
{
	return !_aggroSound.empty();
}

/**
 * Gets a unit's random aggro sound.
 * @return The sound id.
 */
int BattleUnit::getRandomAggroSound() const
{
	if (hasAggroSound())
	{
		return _aggroSound[RNG::generate(0, _aggroSound.size() - 1)];
	}
	return -1;
}

/**
 * Set a specific amount of time units.
 * @param tu time units.
 */
void BattleUnit::setTimeUnits(int tu)
{
	_tu = Clamp(tu, 0, (int)_stats.tu);
}

/**
 * Get the faction the unit was killed by.
 * @return faction
 */
UnitFaction BattleUnit::killedBy() const
{
	return _killedBy;
}

/**
 * Set the faction the unit was killed by.
 * @param f faction
 */
void BattleUnit::killedBy(UnitFaction f)
{
	_killedBy = f;
}

/**
 * Set the units we are charging towards.
 * @param chargeTarget Charge Target
 */
void BattleUnit::setCharging(BattleUnit *chargeTarget)
{
	_charging = chargeTarget;
}

/**
 * Get the units we are charging towards.
 * @return Charge Target
 */
BattleUnit *BattleUnit::getCharging()
{
	return _charging;
}

/**
 * Get the units carried weight in strength units.
 * @param draggingItem item to ignore
 * @return weight
 */
int BattleUnit::getCarriedWeight(BattleItem *draggingItem) const
{
	int weight = _armor->getWeight();
	for (std::vector<BattleItem*>::const_iterator i = _inventory.begin(); i != _inventory.end(); ++i)
	{
		if ((*i) == draggingItem) continue;
		weight += (*i)->getTotalWeight();
	}
	return std::max(0,weight);
}

/**
 * Set how long since this unit was last exposed.
 * @param turns number of turns
 */
void BattleUnit::setTurnsSinceSpotted (int turns)
{
	_turnsSinceSpotted = turns;
}

/**
 * Get how long since this unit was exposed.
 * @return number of turns
 */
int BattleUnit::getTurnsSinceSpotted() const
{
	return _turnsSinceSpotted;
}

/**
 * Set how many turns left snipers will know about this unit.
 * @param turns number of turns
 */
void BattleUnit::setTurnsLeftSpottedForSnipers (int turns)
{
	_turnsLeftSpottedForSnipers = turns;
}

/**
 * Get how many turns left snipers can fire on this unit.
 * @return number of turns
 */
int BattleUnit::getTurnsLeftSpottedForSnipers() const
{
	return _turnsLeftSpottedForSnipers;
}

/**
 * Get this unit's original Faction.
 * @return original faction
 */
UnitFaction BattleUnit::getOriginalFaction() const
{
	return _originalFaction;
}

/**
 * Get the list of units spotted this turn.
 * @return List of units.
 */
std::vector<BattleUnit *> &BattleUnit::getUnitsSpottedThisTurn()
{
	return _unitsSpottedThisTurn;
}

/**
 * Get the list of units spotted this turn.
 * @return List of units.
 */
const std::vector<BattleUnit *> &BattleUnit::getUnitsSpottedThisTurn() const
{
	return _unitsSpottedThisTurn;
}

/**
 * Change the numeric version of the unit's rank.
 * @param rank unit rank, 0 = lowest
 */
void BattleUnit::setRankInt(int rank)
{
	_rankInt = rank;
}

/**
 * Return the numeric version of the unit's rank.
 * @return unit rank, 0 = lowest
 */
int BattleUnit::getRankInt() const
{
	return _rankInt;
}

/**
 * Derive the numeric unit rank from the string rank
 * (for soldier units).
 */
void BattleUnit::deriveRank()
{
	if (_geoscapeSoldier)
	{
		switch (_geoscapeSoldier->getRank())
		{
		case RANK_ROOKIE:    _rankInt = 0; break;
		case RANK_SQUADDIE:  _rankInt = 1; break;
		case RANK_SERGEANT:  _rankInt = 2; break;
		case RANK_CAPTAIN:   _rankInt = 3; break;
		case RANK_COLONEL:   _rankInt = 4; break;
		case RANK_COMMANDER: _rankInt = 5; break;
		default:             _rankInt = 0; break;
		}
	}
}

/**
* this function checks if a tile is visible from either of the unit's tiles, using maths.
* @param pos the position to check against
* @param useTurretDirection use turret facing (true) or body facing (false) for sector calculation
* @return what the maths decide
*/
bool BattleUnit::checkViewSector (Position pos, bool useTurretDirection /* = false */) const
{
	int unitSize = getArmor()->getSize();
	//Check view cone from each of the unit's tiles
	for (int x = 0; x < unitSize; ++x)
	{
		for (int y = 0; y < unitSize; ++y)
		{
			int deltaX = pos.x - (_pos.x + x);
			int deltaY = (_pos.y + y) - pos.y;
			switch (useTurretDirection ? _directionTurret : _direction)
			{
			case 0:
				if ((deltaX + deltaY >= 0) && (deltaY - deltaX >= 0))
					return true;
				break;
			case 1:
				if ((deltaX >= 0) && (deltaY >= 0))
					return true;
				break;
			case 2:
				if ((deltaX + deltaY >= 0) && (deltaY - deltaX <= 0))
					return true;
				break;
			case 3:
				if ((deltaY <= 0) && (deltaX >= 0))
					return true;
				break;
			case 4:
				if ((deltaX + deltaY <= 0) && (deltaY - deltaX <= 0))
					return true;
				break;
			case 5:
				if ((deltaX <= 0) && (deltaY <= 0))
					return true;
				break;
			case 6:
				if ((deltaX + deltaY <= 0) && (deltaY - deltaX >= 0))
					return true;
				break;
			case 7:
				if ((deltaY >= 0) && (deltaX <= 0))
					return true;
				break;
			default:
				break;
			}
		}
	}
	return false;
}

/**
 * common function to adjust a unit's stats according to difficulty setting.
 * @param statAdjustment the stat adjustment variables coefficient value.
 */
void BattleUnit::adjustStats(const StatAdjustment &adjustment)
{
	_stats += UnitStats::percent(_stats, adjustment.statGrowth, adjustment.growthMultiplier);

	_stats.firing *= adjustment.aimMultiplier;
	_maxArmor[0] *= adjustment.armorMultiplier;
	_maxArmor[1] *= adjustment.armorMultiplier;
	_maxArmor[2] *= adjustment.armorMultiplier;
	_maxArmor[3] *= adjustment.armorMultiplier;
	_maxArmor[4] *= adjustment.armorMultiplier;
}

/**
 * did this unit already take fire damage this turn?
 * (used to avoid damaging large units multiple times.)
 * @return ow it burns
 */
bool BattleUnit::tookFireDamage() const
{
	return _hitByFire;
}

/**
 * toggle the state of the fire damage tracking boolean.
 */
void BattleUnit::toggleFireDamage()
{
	_hitByFire = !_hitByFire;
}

/**
 * Checks if this unit can be selected. Only alive units
 * belonging to the faction can be selected.
 * @param faction The faction to compare with.
 * @param checkReselect Check if the unit is reselectable.
 * @param checkInventory Check if the unit has an inventory.
 * @return True if the unit can be selected, false otherwise.
 */
bool BattleUnit::isSelectable(UnitFaction faction, bool checkReselect, bool checkInventory) const
{
	return (_faction == faction && !isOut() && (!checkReselect || reselectAllowed()) && (!checkInventory || hasInventory()));
}

/**
 * Checks if this unit has an inventory. Large units and/or
 * terror units generally don't have inventories.
 * @return True if an inventory is available, false otherwise.
 */
bool BattleUnit::hasInventory() const
{
	return (_armor->hasInventory());
}

/**
 * If this unit is breathing, what frame should be displayed?
 * @return frame number.
 */
int BattleUnit::getBreathExhaleFrame() const
{
	if (_breathing)
	{
		auto frame = _breathFrame - BUBBLES_FIRST_FRAME;
		if (frame >= 0)
		{
			return frame;
		}
	}

	return -1;
}

/**
 * Count frames to next start of breath animation.
 */
int BattleUnit::getBreathInhaleFrame() const
{
	if (_breathing)
	{
		auto frame = BUBBLES_FIRST_FRAME - _breathFrame;
		if (frame >= 0)
		{
			return frame;
		}
	}

	return -1;
}

/**
 * Decides if we should start producing bubbles, and/or updates which bubble frame we are on.
 */
void BattleUnit::breathe()
{
	// _breathFrame of -1 means this unit doesn't produce bubbles
	if (_breathFrame < 0)
	{
		_breathing = false;
		return;
	}

	// moving or knock out do not breathe, even when still alive :)
	if (isOut() || _status == STATUS_WALKING)
	{
		_breathing = false;
		_breathFrame = 0;
		return;
	}

	if (!_breathing)
	{
		// deviation from original: TFTD used a static 10% chance for every animation frame,
		// instead let's use 5%, but allow morale to affect it.
		_breathing = RNG::seedless(0, 99) < (105 - _morale);
		_breathFrame = 0;
	}

	if (_breathing)
	{
		// advance the bubble frame
		_breathFrame++;

		// we've reached the end of the cycle, get rid of the bubbles
		if (_breathFrame > BUBBLES_LAST_FRAME)
		{
			_breathFrame = 0;
			_breathing = false;
		}
	}
}

/**
 * Sets the flag for "this unit is under cover" meaning don't draw bubbles.
 * @param floor is there a floor.
 */
void BattleUnit::setFloorAbove(bool floor)
{
	_floorAbove = floor;
}

/**
 * Checks if the floor above flag has been set.
 * @return if we're under cover.
 */
bool BattleUnit::getFloorAbove() const
{
	return _floorAbove;
}

/**
 * Get the name of any utility weapon we may be carrying, or a built in one.
 * @return the name .
 */
BattleItem *BattleUnit::getUtilityWeapon(BattleType type)
{
	BattleItem *melee = getRightHandWeapon();
	if (melee && melee->getRules()->getBattleType() == type)
	{
		return melee;
	}
	melee = getLeftHandWeapon();
	if (melee && melee->getRules()->getBattleType() == type)
	{
		return melee;
	}
	melee = getSpecialWeapon(type);
	if (melee)
	{
		return melee;
	}
	return 0;
}

/**
 * Set fire damage from environment.
 * @param damage
 */
void BattleUnit::setEnviFire(int damage)
{
	if (_fireMaxHit < damage) _fireMaxHit = damage;
}

/**
 * Set smoke damage from environment.
 * @param damage
 */
void BattleUnit::setEnviSmoke(int damage)
{
	if (_smokeMaxHit < damage) _smokeMaxHit = damage;
}

/**
 * Calculate smoke and fire damage from environment.
 */
void BattleUnit::calculateEnviDamage(Mod *mod, SavedBattleGame *save)
{
	if (_fireMaxHit)
	{
		_hitByFire = true;
		damage(Position(0, 0, 0), _fireMaxHit, mod->getDamageType(DT_IN), save, { });
		// try to set the unit on fire.
		if (RNG::percent(40 * getArmor()->getDamageModifier(DT_IN)))
		{
			int burnTime = RNG::generate(0, int(5.0f * getArmor()->getDamageModifier(DT_IN)));
			if (getFire() < burnTime)
			{
				setFire(burnTime);
			}
		}
	}

	if (_smokeMaxHit)
	{
		damage(Position(0,0,0), _smokeMaxHit, mod->getDamageType(DT_SMOKE), save, { });
	}

	_fireMaxHit = 0;
	_smokeMaxHit = 0;
}

/**
 * Gets the turn cost.
 */
int BattleUnit::getTurnCost() const
{
	return _armor->getTurnCost();
}

/**
 * Elevates the unit to grand galactic inquisitor status,
 * meaning they will NOT take part in the current battle.
 */
void BattleUnit::goToTimeOut()
{
	_status = STATUS_IGNORE_ME;

	// 1. Problem:
	// Take 2 rookies to an alien colony, leave 1 behind, and teleport the other to the exit and abort.
	// Then let the aliens kill the rookie in the second stage.
	// The mission will be a success, alien colony destroyed and everything recovered! (which is unquestionably wrong)
	// ------------
	// 2. Solution:
	// Proper solution would be to fix this in the Debriefing, but (as far as I can say)
	// that would require a lot of changes, Debriefing simply is not prepared for this scenario.
	// ------------
	// 3. Workaround:
	// Knock out all the player units left behind in the earlier stages
	// so that they don't count as survivors when all player units in the later stage are killed.
	if (_originalFaction == FACTION_PLAYER)
	{
		_stunlevel = std::max(_health, 1);
	}
}

/**
 * Set special weapon that is handled outside inventory.
 * @param save
 */
void BattleUnit::setSpecialWeapon(SavedBattleGame *save, bool updateFromSave)
{
	const Mod *mod = save->getMod();
	int i = 0;

	if (_specWeapon[0] && updateFromSave)
	{
		// new saves already contain special built-in weapons, we can stop here
		return;
		// old saves still need the below functionality to work properly
	}

	auto addItem = [&](const RuleItem *item)
	{
		if (item && i < SPEC_WEAPON_MAX)
		{
			//TODO: move this check to load of ruleset
			if ((item->getBattleType() == BT_FIREARM || item->getBattleType() == BT_MELEE) && !item->getClipSize())
			{
				throw Exception("Weapon " + item->getType() + " is used as a special built-in weapon on unit " + getUnitRules()->getType() + " but doesn't have it's own ammo - give it a clipSize!");
			}

			// we already have an item of this type, skip it
			for (auto* w : _specWeapon)
			{
				if (w && w->getRules() == item)
				{
					return;
				}
			}

			_specWeapon[i++] = save->createItemForUnitSpecialBuiltin(item, this);
		}
	};

	if (getUnitRules())
	{
		addItem(mod->getItem(getUnitRules()->getMeleeWeapon()));
	}

	addItem(getArmor()->getSpecialWeapon());

	if (getBaseStats()->psiSkill > 0 && getOriginalFaction() == FACTION_HOSTILE)
	{
		addItem(mod->getItem(getUnitRules()->getPsiWeapon()));
	}
	if (getGeoscapeSoldier())
	{
		addItem(getGeoscapeSoldier()->getRules()->getSpecialWeapon());
	}
}

/**
 * Add/assign a special weapon loaded from a save.
 */
void BattleUnit::addLoadedSpecialWeapon(BattleItem* item)
{
	for (auto*& s : _specWeapon)
	{
		if (s == nullptr)
		{
			s = item;
			return;
		}
	}
	Log(LOG_ERROR) << "Failed to add special built-in item '" << item->getRules()->getType() << "' (id " << item->getId() << ") to unit '" << getType() << "' (id " << getId() << ")";
}

/**
 * Remove all special weapons.
 */
void BattleUnit::removeSpecialWeapons(SavedBattleGame *save)
{
	for (auto*& s : _specWeapon)
	{
		if (s)
		{
			s->setOwner(nullptr); // stops being a special weapon, so that `removeItem` can remove it
			save->removeItem(s);
			s = nullptr;
		}
	}
}

/**
 * Get special weapon by battletype.
 */
BattleItem *BattleUnit::getSpecialWeapon(BattleType type) const
{
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
	{
		if (!_specWeapon[i])
		{
			break;
		}
		if (_specWeapon[i]->getRules()->getBattleType() == type)
		{
			return _specWeapon[i];
		}
	}
	return 0;
}


/**
 * Get special weapon by name.
 */
BattleItem *BattleUnit::getSpecialWeapon(const RuleItem *weaponRule) const
{
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
	{
		if (!_specWeapon[i])
		{
			break;
		}
		if (_specWeapon[i]->getRules() == weaponRule)
		{
			return _specWeapon[i];
		}
	}
	return 0;
}

/**
 * Gets the special weapon that uses an icon
 * @param type Parameter passed to get back the type of the weapon
 * @return Pointer the the weapon, null if not found
 */
BattleItem *BattleUnit::getSpecialIconWeapon(BattleType &type) const
{
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
	{
		if (!_specWeapon[i])
		{
			break;
		}

		if (_specWeapon[i]->getRules()->getSpecialIconSprite() != -1)
		{
			type = _specWeapon[i]->getRules()->getBattleType();
			return _specWeapon[i];
		}
	}
	return 0;
}

/**
 * Recovers a unit's TUs and energy, taking a number of factors into consideration.
 */
void BattleUnit::recoverTimeUnits()
{
	updateUnitStats(true, false);
}

/**
 * Get the unit's statistics.
 * @return BattleUnitStatistics statistics.
 */
BattleUnitStatistics* BattleUnit::getStatistics()
{
	return _statistics;
}

/**
 * Sets the unit murderer's id.
 * @param int murderer id.
 */
void BattleUnit::setMurdererId(int id)
{
	_murdererId = id;
}

/**
 * Gets the unit murderer's id.
 * @return int murderer id.
 */
int BattleUnit::getMurdererId() const
{
	return _murdererId;
}

/**
 * Set information on the unit's fatal blow.
 * @param UnitSide unit's side that was shot.
 * @param UnitBodyPart unit's body part that was shot.
 */
void BattleUnit::setFatalShotInfo(UnitSide side, UnitBodyPart bodypart)
{
	_fatalShotSide = side;
	_fatalShotBodyPart = bodypart;
}

/**
 * Get information on the unit's fatal shot's side.
 * @return UnitSide fatal shot's side.
 */
UnitSide BattleUnit::getFatalShotSide() const
{
	return _fatalShotSide;
}

/**
 * Get information on the unit's fatal shot's body part.
 * @return UnitBodyPart fatal shot's body part.
 */
UnitBodyPart BattleUnit::getFatalShotBodyPart() const
{
	return _fatalShotBodyPart;
}

/**
 * Gets the unit murderer's weapon.
 * @return int murderer weapon.
 */
std::string BattleUnit::getMurdererWeapon() const
{
	return _murdererWeapon;
}

/**
 * Set the murderer's weapon.
 * @param string murderer's weapon.
 */
void BattleUnit::setMurdererWeapon(const std::string& weapon)
{
	_murdererWeapon = weapon;
}

/**
 * Gets the unit murderer's weapon's ammo.
 * @return int murderer weapon ammo.
 */
std::string BattleUnit::getMurdererWeaponAmmo() const
{
	return _murdererWeaponAmmo;
}

/**
 * Set the murderer's weapon's ammo.
 * @param string murderer weapon ammo.
 */
void BattleUnit::setMurdererWeaponAmmo(const std::string& weaponAmmo)
{
	_murdererWeaponAmmo = weaponAmmo;
}

/**
 * Sets the unit mind controller's id.
 * @param int mind controller id.
 */
void BattleUnit::setMindControllerId(int id)
{
	_mindControllerID = id;
}

/**
 * Gets the unit mind controller's id.
 * @return int mind controller id.
 */
int BattleUnit::getMindControllerId() const
{
	return _mindControllerID;
}

/**
 * Gets the spotter score. Determines how many turns sniper AI units can act on this unit seeing your troops.
 * @return The unit's spotter value.
 */
int BattleUnit::getSpotterDuration() const
{
	if (_unitRules)
	{
		return _unitRules->getSpotterDuration();
	}
	return 0;
}

/**
 * Remembers the unit's XP (used for shotguns).
 */
void BattleUnit::rememberXP()
{
	_expTmp = _exp;
}

/**
 * Artificially alter a unit's XP (used for shotguns).
 */
void BattleUnit::nerfXP()
{
	_exp = UnitStats::min(_exp, _expTmp + UnitStats::scalar(1));
}

/**
 * Was this unit just hit?
 */
bool BattleUnit::getHitState()
{
	return _hitByAnything;
}

/**
 * reset the unit hit state.
 */
void BattleUnit::resetHitState()
{
	_hitByAnything = false;
}

/**
 * Was this unit melee attacked by a given attacker this turn (both hit and miss count)?
 */
bool BattleUnit::wasMeleeAttackedBy(int attackerId) const
{
	return std::find(_meleeAttackedBy.begin(), _meleeAttackedBy.end(), attackerId) != _meleeAttackedBy.end();
}

/**
 * Set the "melee attacked by" flag.
 */
void BattleUnit::setMeleeAttackedBy(int attackerId)
{
	if (!wasMeleeAttackedBy(attackerId))
	{
		_meleeAttackedBy.push_back(attackerId);
	}
}

/**
 * Gets whether this unit can be captured alive (applies to aliens).
 */
bool BattleUnit::getCapturable() const
{
	return _capturable;
}

void BattleUnit::freePatrolTarget()
{
	if (_currentAIState)
	{
		_currentAIState->freePatrolTarget();
	}
}

/**
 * Marks this unit as summoned by an item or not.
 * @param summonedPlayerUnit summoned?
 */
void BattleUnit::setSummonedPlayerUnit(bool summonedPlayerUnit)
{
	_summonedPlayerUnit = summonedPlayerUnit;
}

/**
 * Was this unit summoned by an item?
 * @return True, if this unit was summoned by an item and therefore won't count for recovery or total player units left.
 */
bool BattleUnit::isSummonedPlayerUnit() const
{
	return _summonedPlayerUnit;
}

/**
 * Should this unit (player, alien or civilian) be ignored for various things related to soldier diaries and commendations?
 */
bool BattleUnit::isCosmetic() const
{
	return _unitRules && _unitRules->isCosmetic();
}

/**
 * Should this AI unit (alien or civilian) be ignored by other AI units?
 */
bool BattleUnit::isIgnoredByAI() const
{
	return _unitRules && _unitRules->isIgnoredByAI();
}

/**
 * Disable showing indicators for this unit.
 */
void BattleUnit::disableIndicators()
{
	_disableIndicators = true;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

void setArmorValueScript(BattleUnit *bu, int side, int value)
{
	if (bu && 0 <= side && side < SIDE_MAX)
	{
		bu->setArmor(value, (UnitSide)side);
	}
}
void addArmorValueScript(BattleUnit *bu, int side, int value)
{
	if (bu && 0 <= side && side < SIDE_MAX)
	{
		//limit range to prevent overflow
		value = Clamp(value, -UnitStats::BaseStatLimit, UnitStats::BaseStatLimit);
		bu->setArmor(value + bu->getArmor((UnitSide)side), (UnitSide)side);
	}
}
void getArmorValueScript(const BattleUnit *bu, int &ret, int side)
{
	if (bu && 0 <= side && side < SIDE_MAX)
	{
		ret = bu->getArmor((UnitSide)side);
		return;
	}
	ret = 0;
}
void getArmorValueMaxScript(const BattleUnit *bu, int &ret, int side)
{
	if (bu && 0 <= side && side < SIDE_MAX)
	{
		ret = bu->getMaxArmor((UnitSide)side);
		return;
	}
	ret = 0;
}

void setFatalWoundScript(BattleUnit *bu, int part, int val)
{
	if (bu && 0 <= part && part < BODYPART_MAX)
	{
		bu->setFatalWound(val, (UnitBodyPart)part);
	}
}
void addFatalWoundScript(BattleUnit *bu, int part, int val)
{
	if (bu && 0 <= part && part < BODYPART_MAX)
	{
		//limit range to prevent overflow
		val = Clamp(val, -UnitStats::BaseStatLimit, UnitStats::BaseStatLimit);
		bu->setFatalWound(val + bu->getFatalWound((UnitBodyPart)part), (UnitBodyPart)part);
	}
}
void getFatalWoundScript(const BattleUnit *bu, int &ret, int part)
{
	if (bu && 0 <= part && part < BODYPART_MAX)
	{
		ret = bu->getFatalWound((UnitBodyPart)part);
		return;
	}
	ret = 0;
}
void getFatalWoundMaxScript(const BattleUnit *bu, int &ret, int part)
{
	if (bu && 0 <= part && part < BODYPART_MAX)
	{
		ret = 100;
		return;
	}
	ret = 0;
}



void getMovmentTypeScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = (int)bu->getMovementType();
		return;
	}
	ret = 0;
}
void getOriginalMovmentTypeScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = (int)bu->getOriginalMovementType();
		return;
	}
	ret = 0;
}

void setMovmentTypeScript(BattleUnit *bu, int type)
{
	if (bu && 0 <= type && type <= MT_SLIDE)
	{
		bu->setMovementType((MovementType)type);
		return;
	}
}



void getGenderScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getGender();
		return;
	}
	ret = 0;
}
void getLookScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		auto g = bu->getGeoscapeSoldier();
		if (g)
		{
			ret = g->getLook();
			return;
		}
	}
	ret = 0;
}
void getLookVariantScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		auto g = bu->getGeoscapeSoldier();
		if (g)
		{
			ret = g->getLookVariant();
			return;
		}
	}
	ret = 0;
}

struct getRuleSoldierScript
{
	static RetEnum func(const BattleUnit *bu, const RuleSoldier* &ret)
	{
		if (bu)
		{
			auto g = bu->getGeoscapeSoldier();
			if (g)
			{
				ret = g->getRules();
			}
			else
			{
				ret = nullptr;
			}
		}
		else
		{
			ret = nullptr;
		}
		return RetContinue;
	}
};
struct getGeoscapeSoldierScript
{
	static RetEnum func(BattleUnit *bu, Soldier* &ret)
	{
		if (bu)
		{
			ret = bu->getGeoscapeSoldier();
		}
		else
		{
			ret = nullptr;
		}
		return RetContinue;
	}
};
struct getGeoscapeSoldierConstScript
{
	static RetEnum func(const BattleUnit *bu, const Soldier* &ret)
	{
		if (bu)
		{
			ret = bu->getGeoscapeSoldier();
		}
		else
		{
			ret = nullptr;
		}
		return RetContinue;
	}
};

void getReactionScoreScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = (int)bu->getReactionScore();
		return;
	}
	ret = 0;
}
void getRecolorScript(const BattleUnit *bu, int &pixel)
{
	if (bu)
	{
		const auto& vec = bu->getRecolor();
		const int g = pixel & helper::ColorGroup;
		const int s = pixel & helper::ColorShade;
		for(auto& p : vec)
		{
			if (g == p.first)
			{
				pixel = s + p.second;
				return;
			}
		}
	}
}
void getTileShade(const BattleUnit *bu, int &shade)
{
	if (bu)
	{
		auto tile = bu->getTile();
		if (tile)
		{
			shade = tile->getShade();
			return;
		}
	}
	shade = 0;
}

void getStunMaxScript(const BattleUnit *bu, int &maxStun)
{
	if (bu)
	{
		maxStun = bu->getBaseStats()->health * UnitStats::StunMultipler;
		return;
	}
	maxStun = 0;
}

struct getRightHandWeaponScript
{
	static RetEnum func(BattleUnit *bu, BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getRightHandWeapon();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};
struct getRightHandWeaponConstScript
{
	static RetEnum func(const BattleUnit *bu, const BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getRightHandWeapon();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};
struct getLeftHandWeaponScript
{
	static RetEnum func(BattleUnit *bu, BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getLeftHandWeapon();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};
struct getLeftHandWeaponConstScript
{
	static RetEnum func(const BattleUnit *bu, const BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getLeftHandWeapon();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};

struct reduceByBraveryScript
{
	static RetEnum func(const BattleUnit *bu, int &ret)
	{
		if (bu)
		{
			ret = bu->reduceByBravery(ret);
		}
		return RetContinue;
	}
};

struct reduceByResistanceScript
{
	static RetEnum func(const BattleUnit *bu, int &ret, int resistType)
	{
		if (bu)
		{
			if (resistType >= 0 && resistType < DAMAGE_TYPES)
			{
				ret = bu->reduceByResistance(ret, (ItemDamageType)resistType);
			}
		}
		return RetContinue;
	}
};

void isWalkingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_WALKING;
		return;
	}
	ret = 0;
}
void isFlyingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_FLYING;
		return;
	}
	ret = 0;
}
void isCollapsingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_COLLAPSING;
		return;
	}
	ret = 0;
}
void isStandingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_STANDING;
		return;
	}
	ret = 0;
}
void isAimingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_AIMING;
		return;
	}
	ret = 0;
}

void makeVisibleScript(BattleUnit *bu)
{
	if (bu)
	{
		bu->setVisible(true);
		return;
	}
}

struct burnShadeScript
{
	static RetEnum func(int &curr, int burn, int shade)
	{
		Uint8 d = curr;
		Uint8 s = curr;
		helper::BurnShade::func(d, s, burn, shade);
		curr = d;
		return RetContinue;
	}
};

template<int BattleUnit::*StatCurr, UnitStats::Ptr StatMax, int NegativeLimitMult = 0>
void setBaseStatScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		(bu->*StatCurr) = Clamp(val, - NegativeLimitMult * (bu->getBaseStats()->*StatMax), +(bu->getBaseStats()->*StatMax));
	}
}
template<int BattleUnit::*StatCurr, UnitStats::Ptr StatMax, int NegativeLimitMult = 0>
void addBaseStatScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		//limit range to prevent overflow
		val = Clamp(val, -UnitStats::BaseStatLimit, UnitStats::BaseStatLimit);
		setBaseStatScript<StatCurr, StatMax, NegativeLimitMult>(bu, val + (bu->*StatCurr));
	}
}

template<int BattleUnit::*StatCurr>
void setStunScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		(bu->*StatCurr) = Clamp(val, 0, (bu->getBaseStats()->health) * UnitStats::StunMultipler);
	}
}

template<int BattleUnit::*StatCurr>
void addStunScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		//limit range to prevent overflow, 4 time bigger than normal as stun can be 4 time bigger than health
		val = Clamp(val, -UnitStats::StunStatLimit, UnitStats::StunStatLimit);
		setStunScript<StatCurr>(bu, val + (bu->*StatCurr));
	}
}

template<int BattleUnit::*StatCurr, int Min, int Max>
void setBaseStatRangeScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		(bu->*StatCurr) = Clamp(val, Min, Max);
	}
}

template<int BattleUnit::*StatCurr, int Min, int Max>
void addBaseStatRangeScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		//limit range to prevent overflow
		val = Clamp(val, -UnitStats::BaseStatLimit, UnitStats::BaseStatLimit);
		setBaseStatRangeScript<StatCurr, Min, Max>(bu, val + (bu->*StatCurr));
	}
}

void setFireScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		val = Clamp(val, 0, UnitStats::BaseStatLimit);
		bu->setFire(val);
	}
}


void getVisibleUnitsCountScript(BattleUnit *bu, int &ret)
{
	if (bu)
	{

		auto visibleUnits = bu->getVisibleUnits();
		ret = visibleUnits->size();
	}
}

/**
 * Get the X part of the tile coordinate of this unit.
 * @return X Position.
 */
void getPositionXScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getPosition().x;
		return;
	}
	ret = 0;
}

/**
 * Get the Y part of the tile coordinate of this unit.
 * @return Y Position.
 */
void getPositionYScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getPosition().y;
		return;
	}
	ret = 0;
}
/**
 * Get the Z part of the tile coordinate of this unit.
 * @return Z Position.
 */
void getPositionZScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getPosition().z;
		return;
	}
	ret = 0;
}

void getFactionScript(const BattleUnit *bu, int &faction)
{
	if (bu)
	{
		faction = (int)bu->getFaction();
		return;
	}
	faction = 0;
}

void getOriginalFactionScript(const BattleUnit *bu, int &faction)
{
	if (bu)
	{
		faction = (int)bu->getOriginalFaction();
		return;
	}
	faction = 0;
}


void setSpawnUnitScript(BattleUnit *bu, const Unit* unitType)
{
	if (bu && unitType && bu->getArmor()->getSize() >= unitType->getArmor()->getSize())
	{
		bu->setSpawnUnit(unitType);
		bu->setRespawn(true);
		bu->setSpawnUnitFaction(FACTION_HOSTILE);
	}
	else if (bu)
	{
		bu->clearSpawnUnit();
	}
}

void getSpawnUnitScript(BattleUnit *bu, const Unit*& unitType)
{
	unitType = bu ? bu->getSpawnUnit() : nullptr;
}

void setSpawnUnitInstantRespawnScript(BattleUnit *bu, int respawn)
{
	if (bu && bu->getSpawnUnit())
	{
		bu->setRespawn(respawn);
	}
}

void getSpawnUnitInstantRespawnScript(BattleUnit *bu, int& respawn)
{
	respawn = bu ? bu->getRespawn() : 0;
}

void setSpawnUnitFactionScript(BattleUnit *bu, int faction)
{
	if (bu && bu->getSpawnUnit())
	{
		if (faction >= FACTION_PLAYER && faction <= FACTION_NEUTRAL)
		{
			bu->setSpawnUnitFaction((UnitFaction)faction);
		}
	}
}

void getSpawnUnitFactionScript(BattleUnit *bu, int& faction)
{
	faction = bu ? bu->getSpawnUnitFaction() : 0;
}


void getInventoryItemScript(BattleUnit* bu, BattleItem *&foundItem, const RuleItem *itemRules)
{
	foundItem = nullptr;
	if (bu)
	{
		for (auto* i : *bu->getInventory())
		{
			if (i->getRules() == itemRules)
			{
				foundItem = i;
				break;
			}
		}
	}
}

void getInventoryItemScript1(BattleUnit* bu, BattleItem *&foundItem, const RuleInventory *inv, const RuleItem *itemRules)
{
	foundItem = nullptr;
	if (bu)
	{
		for (auto* i : *bu->getInventory())
		{
			if (i->getSlot() == inv && i->getRules() == itemRules)
			{
				foundItem = i;
				break;
			}
		}
	}
}

void getInventoryItemScript2(BattleUnit* bu, BattleItem *&foundItem, const RuleInventory *inv)
{
	foundItem = nullptr;
	if (bu)
	{
		for (auto* i : *bu->getInventory())
		{
			if (i->getSlot() == inv)
			{
				foundItem = i;
				break;
			}
		}
	}
}

//TODO: move it to script bindings
template<auto Member>
void getListScript(BattleUnit* bu, BattleItem *&foundItem, int i)
{
	foundItem = nullptr;
	if (bu)
	{
		auto& ptr = (bu->*Member);
		if ((size_t)i < std::size(ptr))
		{
			foundItem = ptr[i];
		}
	}
}

//TODO: move it to script bindings
template<auto Member>
void getListSizeScript(BattleUnit* bu, int& i)
{
	i = 0;
	if (bu)
	{
		auto& ptr = (bu->*Member);
		i = (int)std::size(ptr);
	}
}

template<auto Member>
void getListSizeHackScript(BattleUnit* bu, int& i)
{
	i = 0;
	if (bu)
	{
		auto& ptr = (bu->*Member);
		//count number of elements until null, and interpret this as size of array
		i = std::distance(
			std::begin(ptr),
			std::find(std::begin(ptr), std::end(ptr), nullptr)
		);
	}
}


std::string debugDisplayScript(const BattleUnit* bu)
{
	if (bu)
	{
		std::string s;
		s += BattleUnit::ScriptName;
		s += "(type: \"";
		s += bu->getType();
		auto unit = bu->getUnitRules();
		if (unit)
		{
			s += "\" race: \"";
			s += unit->getRace();
		}
		auto soldier = bu->getGeoscapeSoldier();
		if (soldier)
		{
			s += "\" name: \"";
			s += soldier->getName();
		}
		s += "\" id: ";
		s += std::to_string(bu->getId());
		s += " faction: ";
		switch (bu->getFaction())
		{
		case FACTION_HOSTILE: s += "Hostile"; break;
		case FACTION_NEUTRAL: s += "Neutral"; break;
		case FACTION_PLAYER: s += "Player"; break;
		}
		s += " hp: ";
		s += std::to_string(bu->getHealth());
		s += "/";
		s += std::to_string(bu->getBaseStats()->health);
		s += ")";
		return s;
	}
	else
	{
		return "null";
	}
}

} // namespace

/**
 * Register BattleUnit in script parser.
 * @param parser Script parser.
 */
void BattleUnit::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<Mod>();
	parser->registerPointerType<Armor>();
	parser->registerPointerType<RuleSoldier>();
	parser->registerPointerType<BattleItem>();
	parser->registerPointerType<Soldier>();
	parser->registerPointerType<RuleSkill>();
	parser->registerPointerType<Unit>();
	parser->registerPointerType<RuleInventory>();

	Bind<BattleUnit> bu = { parser };

	bu.addField<&BattleUnit::_id>("getId");
	bu.addField<&BattleUnit::_rankInt>("getRank");
	bu.add<&getGenderScript>("getGender");
	bu.add<&getLookScript>("getLook");
	bu.add<&getLookVariantScript>("getLookVariant");
	bu.add<&getRecolorScript>("getRecolor");
	bu.add<&BattleUnit::isFloating>("isFloating");
	bu.add<&BattleUnit::isKneeled>("isKneeled");
	bu.add<&isStandingScript>("isStanding");
	bu.add<&isWalkingScript>("isWalking");
	bu.add<&isFlyingScript>("isFlying");
	bu.add<&isCollapsingScript>("isCollapsing");
	bu.add<&isAimingScript>("isAiming");
	bu.add<&BattleUnit::isFearable>("isFearable");
	bu.add<&BattleUnit::isWoundable>("isWoundable");
	bu.add<&getReactionScoreScript>("getReactionScore");
	bu.add<&BattleUnit::getDirection>("getDirection");
	bu.add<&BattleUnit::getIntelligence>("getIntelligence");
	bu.add<&BattleUnit::getAggression>("getAggression");
	bu.add<&BattleUnit::getTurretDirection>("getTurretDirection");
	bu.add<&BattleUnit::getWalkingPhase>("getWalkingPhase");
	bu.add<&BattleUnit::disableIndicators>("disableIndicators");

	bu.add<&BattleUnit::getVisible>("isVisible");
	bu.add<&makeVisibleScript>("makeVisible");


	bu.add<&setSpawnUnitScript>("setSpawnUnit", "set type of zombie will be spawn from current unit, it will reset everything to default (hostile & instant)");
	bu.add<&getSpawnUnitScript>("getSpawnUnit", "get type of zombie will be spawn from current unit");
	bu.add<&setSpawnUnitInstantRespawnScript>("setSpawnUnitInstantRespawn", "set 1 to make unit instantly change to spawn zombie unit, other wise it will transform on death");
	bu.add<&getSpawnUnitInstantRespawnScript>("getSpawnUnitInstantRespawn", "get state of instant respawn");
	bu.add<&setSpawnUnitFactionScript>("setSpawnUnitFaction", "set faction of unit that will spawn");
	bu.add<&getSpawnUnitFactionScript>("getSpawnUnitFaction", "get faction of unit that will spawn");


	bu.addField<&BattleUnit::_tu>("getTimeUnits");
	bu.add<&UnitStats::getMaxStatScript<BattleUnit, &BattleUnit::_stats, &UnitStats::tu>>("getTimeUnitsMax");
	bu.add<&setBaseStatScript<&BattleUnit::_tu, &UnitStats::tu>>("setTimeUnits");
	bu.add<&addBaseStatScript<&BattleUnit::_tu, &UnitStats::tu>>("addTimeUnits");

	bu.addField<&BattleUnit::_health>("getHealth");
	bu.add<UnitStats::getMaxStatScript<BattleUnit, &BattleUnit::_stats, &UnitStats::health>>("getHealthMax");
	bu.add<&setBaseStatScript<&BattleUnit::_health, &UnitStats::health>>("setHealth");
	bu.add<&addBaseStatScript<&BattleUnit::_health, &UnitStats::health>>("addHealth");
	bu.add<&setBaseStatScript<&BattleUnit::_health, &UnitStats::health, UnitStats::OverkillMultipler>>("setHealthWithOverkill", "same as setHealth but allow negative health values like with Overkill");
	bu.add<&addBaseStatScript<&BattleUnit::_health, &UnitStats::health, UnitStats::OverkillMultipler>>("addHealthWithOverkill", "same as addHealth but allow negative health values like with Overkill");

	bu.addField<&BattleUnit::_mana>("getMana");
	bu.add<&UnitStats::getMaxStatScript<BattleUnit, &BattleUnit::_stats, &UnitStats::mana>>("getManaMax");
	bu.add<&setBaseStatScript<&BattleUnit::_mana, &UnitStats::mana>>("setMana");
	bu.add<&addBaseStatScript<&BattleUnit::_mana, &UnitStats::mana>>("addMana");

	bu.addField<&BattleUnit::_energy>("getEnergy");
	bu.add<&UnitStats::getMaxStatScript<BattleUnit, &BattleUnit::_stats, &UnitStats::stamina>>("getEnergyMax");
	bu.add<&setBaseStatScript<&BattleUnit::_energy, &UnitStats::stamina>>("setEnergy");
	bu.add<&addBaseStatScript<&BattleUnit::_energy, &UnitStats::stamina>>("addEnergy");

	bu.addField<&BattleUnit::_stunlevel>("getStun");
	bu.add<&getStunMaxScript>("getStunMax");
	bu.add<&setStunScript<&BattleUnit::_stunlevel>>("setStun");
	bu.add<&addStunScript<&BattleUnit::_stunlevel>>("addStun");

	bu.addField<&BattleUnit::_morale>("getMorale");
	bu.addFake<100>("getMoraleMax");
	bu.add<&setBaseStatRangeScript<&BattleUnit::_morale, 0, 100>>("setMorale");
	bu.add<&addBaseStatRangeScript<&BattleUnit::_morale, 0, 100>>("addMorale");


	bu.add<&BattleUnit::getFire>("getFire");
	bu.add<&setFireScript>("setFire");


	bu.add<&setArmorValueScript>("setArmor", "first arg is side, second one is new value of armor");
	bu.add<&addArmorValueScript>("addArmor", "first arg is side, second one is value to add to armor");
	bu.add<&getArmorValueScript>("getArmor", "first arg return armor value, second arg is side");
	bu.add<&getArmorValueMaxScript>("getArmorMax", "first arg return max armor value, second arg is side");

	bu.add<&BattleUnit::getFatalWounds>("getFatalwoundsTotal", "sum for every body part");
	bu.add<&setFatalWoundScript>("setFatalwounds", "first arg is body part, second one is new value of wounds");
	bu.add<&addFatalWoundScript>("addFatalwounds", "first arg is body part, second one is value to add to wounds");
	bu.add<&getFatalWoundScript>("getFatalwounds", "first arg return wounds number, second arg is body part");
	bu.add<&getFatalWoundMaxScript>("getFatalwoundsMax", "first arg return max wounds number, second arg is body part");

	UnitStats::addGetStatsScript<&BattleUnit::_stats>(bu, "Stats.");
	UnitStats::addSetStatsWithCurrScript<&BattleUnit::_stats, &BattleUnit::_tu, &BattleUnit::_energy, &BattleUnit::_health, &BattleUnit::_mana>(bu, "Stats.");

	UnitStats::addGetStatsScript<&BattleUnit::_exp>(bu, "Exp.", true);


	bu.add<&getMovmentTypeScript>("getMovmentType", BindBase::functionInvisible); //old bugged name
	bu.add<&getMovmentTypeScript>("getMovementType", "get move type of unit");
	bu.add<&getOriginalMovmentTypeScript>("getOriginalMovementType", "get original move type of unit");
	bu.add<&setMovmentTypeScript>("setMovementType", "set move type of unit");

	bu.addField<&BattleUnit::_moveCostBase, &ArmorMoveCost::TimePercent>("MoveCost.getBaseTimePercent", "MoveCost.setBaseTimePercent");
	bu.addField<&BattleUnit::_moveCostBase, &ArmorMoveCost::EnergyPercent>("MoveCost.getBaseEnergyPercent", "MoveCost.setBaseEnergyPercent");
	bu.addField<&BattleUnit::_moveCostBaseFly, &ArmorMoveCost::TimePercent>("MoveCost.getBaseFlyTimePercent", "MoveCost.setBaseFlyTimePercent");
	bu.addField<&BattleUnit::_moveCostBaseFly, &ArmorMoveCost::EnergyPercent>("MoveCost.getBaseFlyEnergyPercent", "MoveCost.setBaseFlyEnergyPercent");
	bu.addField<&BattleUnit::_moveCostBaseNormal, &ArmorMoveCost::TimePercent>("MoveCost.getBaseNormalTimePercent", "MoveCost.setBaseNormalTimePercent");
	bu.addField<&BattleUnit::_moveCostBaseNormal, &ArmorMoveCost::EnergyPercent>("MoveCost.getBaseNormalEnergyPercent", "MoveCost.setBaseNormalEnergyPercent");

	bu.add<&getVisibleUnitsCountScript>("getVisibleUnitsCount");
	bu.add<&getFactionScript>("getFaction", "get current faction of unit");
	bu.add<&getOriginalFactionScript>("getOriginalFaction", "get original faction of unit");

	bu.add<&BattleUnit::getOverKillDamage>("getOverKillDamage");
	bu.addRules<Armor, &BattleUnit::getArmor>("getRuleArmor");
	bu.addFunc<getRuleSoldierScript>("getRuleSoldier");
	bu.addFunc<getGeoscapeSoldierScript>("getGeoscapeSoldier");
	bu.addFunc<getGeoscapeSoldierConstScript>("getGeoscapeSoldier");
	bu.addFunc<reduceByBraveryScript>("reduceByBravery", "change first arg1 to `(110 - bravery) * arg1 / 100`");
	bu.addFunc<reduceByResistanceScript>("reduceByResistance", "change first arg1 to `arg1 * resist[arg2]`");

	bu.addFunc<getRightHandWeaponScript>("getRightHandWeapon");
	bu.addFunc<getRightHandWeaponConstScript>("getRightHandWeapon");
	bu.addFunc<getLeftHandWeaponScript>("getLeftHandWeapon");
	bu.addFunc<getLeftHandWeaponConstScript>("getLeftHandWeapon");
	bu.add<&getInventoryItemScript>("getInventoryItem");
	bu.add<&getInventoryItemScript1>("getInventoryItem");
	bu.add<&getInventoryItemScript2>("getInventoryItem");
	bu.add<&getListSizeScript<&BattleUnit::_inventory>>("getInventoryItem.size");
	bu.add<&getListScript<&BattleUnit::_inventory>>("getInventoryItem");
	bu.add<&getListSizeHackScript<&BattleUnit::_specWeapon>>("getSpecialItem.size");
	bu.add<&getListScript<&BattleUnit::_specWeapon>>("getSpecialItem");

	bu.add<&getPositionXScript>("getPosition.getX");
	bu.add<&getPositionYScript>("getPosition.getY");
	bu.add<&getPositionZScript>("getPosition.getZ");
	bu.add<&BattleUnit::getTurnsSinceSpotted>("getTurnsSinceSpotted");
	bu.add<&setBaseStatRangeScript<&BattleUnit::_turnsSinceSpotted, 0, 255>>("setTurnsSinceSpotted");
	bu.addField<&BattleUnit::_turnsSinceStunned>("getTurnsSinceStunned");
	bu.add<&setBaseStatRangeScript<&BattleUnit::_turnsSinceStunned, 0, 255>>("setTurnsSinceStunned");

	bu.addScriptValue<BindBase::OnlyGet, &BattleUnit::_armor, &Armor::getScriptValuesRaw>();
	bu.addScriptValue<&BattleUnit::_scriptValues>();
	bu.addDebugDisplay<&debugDisplayScript>();


	bu.add<&getTileShade>("getTileShade");


	bu.addCustomConst("BODYPART_HEAD", BODYPART_HEAD);
	bu.addCustomConst("BODYPART_TORSO", BODYPART_TORSO);
	bu.addCustomConst("BODYPART_LEFTARM", BODYPART_LEFTARM);
	bu.addCustomConst("BODYPART_RIGHTARM", BODYPART_RIGHTARM);
	bu.addCustomConst("BODYPART_LEFTLEG", BODYPART_LEFTLEG);
	bu.addCustomConst("BODYPART_RIGHTLEG", BODYPART_RIGHTLEG);

	bu.addCustomConst("UNIT_RANK_ROOKIE", 0);
	bu.addCustomConst("UNIT_RANK_SQUADDIE", 1);
	bu.addCustomConst("UNIT_RANK_SERGEANT", 2);
	bu.addCustomConst("UNIT_RANK_CAPTAIN", 3);
	bu.addCustomConst("UNIT_RANK_COLONEL", 4);
	bu.addCustomConst("UNIT_RANK_COMMANDER", 5);

	bu.addCustomConst("COLOR_X1_HAIR", 6);
	bu.addCustomConst("COLOR_X1_FACE", 9);

	bu.addCustomConst("COLOR_X1_NULL", 0);
	bu.addCustomConst("COLOR_X1_YELLOW", 1);
	bu.addCustomConst("COLOR_X1_RED", 2);
	bu.addCustomConst("COLOR_X1_GREEN0", 3);
	bu.addCustomConst("COLOR_X1_GREEN1", 4);
	bu.addCustomConst("COLOR_X1_GRAY", 5);
	bu.addCustomConst("COLOR_X1_BROWN0", 6);
	bu.addCustomConst("COLOR_X1_BLUE0", 7);
	bu.addCustomConst("COLOR_X1_BLUE1", 8);
	bu.addCustomConst("COLOR_X1_BROWN1", 9);
	bu.addCustomConst("COLOR_X1_BROWN2", 10);
	bu.addCustomConst("COLOR_X1_PURPLE0", 11);
	bu.addCustomConst("COLOR_X1_PURPLE1", 12);
	bu.addCustomConst("COLOR_X1_BLUE2", 13);
	bu.addCustomConst("COLOR_X1_SILVER", 14);
	bu.addCustomConst("COLOR_X1_SPECIAL", 15);


	bu.addCustomConst("LOOK_BLONDE", LOOK_BLONDE);
	bu.addCustomConst("LOOK_BROWNHAIR", LOOK_BROWNHAIR);
	bu.addCustomConst("LOOK_ORIENTAL", LOOK_ORIENTAL);
	bu.addCustomConst("LOOK_AFRICAN", LOOK_AFRICAN);

	bu.addCustomConst("GENDER_MALE", GENDER_MALE);
	bu.addCustomConst("GENDER_FEMALE", GENDER_FEMALE);

	bu.addCustomConst("movement_type_walk", MT_WALK);
	bu.addCustomConst("movement_type_fly", MT_FLY);
	bu.addCustomConst("movement_type_slide", MT_SLIDE);
}

/**
 * Register BattleUnitVisibility in script parser.
 * @param parser Script parser.
 */
void BattleUnitVisibility::ScriptRegister(ScriptParserBase* parser)
{
	Bind<BattleUnitVisibility> uv = { parser };

	uv.addScriptTag();
}


namespace
{

void commonImpl(BindBase& b, Mod* mod)
{
	b.addCustomPtr<const Mod>("rules", mod);

	b.addCustomConst("blit_torso", BODYPART_TORSO);
	b.addCustomConst("blit_leftarm", BODYPART_LEFTARM);
	b.addCustomConst("blit_rightarm", BODYPART_RIGHTARM);
	b.addCustomConst("blit_legs", BODYPART_LEGS);
	b.addCustomConst("blit_collapse", BODYPART_COLLAPSING);

	b.addCustomConst("blit_large_torso_0", BODYPART_LARGE_TORSO + 0);
	b.addCustomConst("blit_large_torso_1", BODYPART_LARGE_TORSO + 1);
	b.addCustomConst("blit_large_torso_2", BODYPART_LARGE_TORSO + 2);
	b.addCustomConst("blit_large_torso_3", BODYPART_LARGE_TORSO + 3);
	b.addCustomConst("blit_large_propulsion_0", BODYPART_LARGE_PROPULSION + 0);
	b.addCustomConst("blit_large_propulsion_1", BODYPART_LARGE_PROPULSION + 1);
	b.addCustomConst("blit_large_propulsion_2", BODYPART_LARGE_PROPULSION + 2);
	b.addCustomConst("blit_large_propulsion_3", BODYPART_LARGE_PROPULSION + 3);
	b.addCustomConst("blit_large_turret", BODYPART_LARGE_TURRET);
}

void battleActionImpl(BindBase& b)
{
	b.addCustomConst("battle_action_aimshoot", BA_AIMEDSHOT); //TODO: fix name, it require some new logic in script to allow old typo for backward compatiblity
	b.addCustomConst("battle_action_autoshoot", BA_AUTOSHOT);
	b.addCustomConst("battle_action_snapshot", BA_SNAPSHOT);
	b.addCustomConst("battle_action_walk", BA_WALK);
	b.addCustomConst("battle_action_hit", BA_HIT);
	b.addCustomConst("battle_action_throw", BA_THROW);
	b.addCustomConst("battle_action_use", BA_USE);
	b.addCustomConst("battle_action_mindcontrol", BA_MINDCONTROL);
	b.addCustomConst("battle_action_panic", BA_PANIC);
	b.addCustomConst("battle_action_cqb", BA_CQB);
}

void moveTypesImpl(BindBase& b)
{
	b.addCustomConst("move_normal", BAM_NORMAL);
	b.addCustomConst("move_run", BAM_RUN);
	b.addCustomConst("move_strafe", BAM_STRAFE);
	b.addCustomConst("move_sneak", BAM_SNEAK);
}

void medikitBattleActionImpl(BindBase& b)
{
	b.addCustomConst("medikit_action_heal", BMA_HEAL);
	b.addCustomConst("medikit_action_stimulant", BMA_STIMULANT);
	b.addCustomConst("medikit_action_painkiller", BMA_PAINKILLER);
}

void commonBattleUnitAnimations(ScriptParserBase* parser)
{
	Bind<BattleUnit> bu = { parser, BindBase::ExtensionBinding{} };

	bu.add<&BattleUnit::getFloorAbove>("isFloorAbove", "check if floor is shown above unit");
	bu.add<&BattleUnit::getBreathExhaleFrame>("getBreathExhaleFrame", "return animation frame of breath bubbles, -1 means no animation");
	bu.add<&BattleUnit::getBreathInhaleFrame>("getBreathInhaleFrame", "return number of frames to next breath animation start, 0 means animation started, -1 no animation");
}


}

/**
 * Constructor of recolor script parser.
 */
ModScript::RecolorUnitParser::RecolorUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"new_pixel",
	"old_pixel",

	"unit", "battle_game", "blit_part", "anim_frame", "shade", "burn" }
{
	BindBase b { this };

	b.addCustomFunc<burnShadeScript>("add_burn_shade");

	commonImpl(b, mod);
	commonBattleUnitAnimations(this);

	b.addCustomConst("blit_item_righthand", BODYPART_ITEM_RIGHTHAND);
	b.addCustomConst("blit_item_lefthand", BODYPART_ITEM_LEFTHAND);
	b.addCustomConst("blit_item_floor", BODYPART_ITEM_FLOOR);
	b.addCustomConst("blit_item_big", BODYPART_ITEM_INVENTORY);

	setDefault("unit.getRecolor new_pixel; add_burn_shade new_pixel burn shade; return new_pixel;");
}

/**
 * Constructor of select sprite script parser.
 */
ModScript::SelectUnitParser::SelectUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"sprite_index",
	"sprite_offset",

	"unit", "battle_game", "blit_part", "anim_frame", "shade" }
{
	BindBase b { this };

	commonImpl(b, mod);
	commonBattleUnitAnimations(this);

	setDefault("add sprite_index sprite_offset; return sprite_index;");
}

/**
 * Constructor of select sound script parser.
 */
ModScript::SelectMoveSoundUnitParser::SelectMoveSoundUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"sound_index",
	"unit", "walking_phase", "unit_sound_index", "tile_sound_index",
	"base_tile_sound_index", "base_tile_sound_offset", "base_fly_sound_index",
	"move", }
{
	BindBase b { this };

	commonImpl(b, mod);
	commonBattleUnitAnimations(this);

	moveTypesImpl(b);
}

/**
 * Constructor of reaction chance script parser.
 */
ModScript::ReactionUnitParser::ReactionUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"reaction_chance",
	"distance",

	"action_unit",
	"reaction_unit", "reaction_weapon", "reaction_battle_action", "reaction_count",
	"weapon", "skill", "battle_action", "action_target",
	"move", "arc_to_action_unit", "battle_game" }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);

	moveTypesImpl(b);
}

/**
 * Constructor of visibility script parser.
 */
ModScript::VisibilityUnitParser::VisibilityUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "current_visibility", "default_visibility", "visibility_mode", "observer_unit", "target_unit", "distance", "distance_max", "smoke_density", "fire_density", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

/**
 * Init all required data in script using object data.
 */
void BattleUnit::ScriptFill(ScriptWorkerBlit* w, const BattleUnit* unit, const SavedBattleGame* save, int body_part, int anim_frame, int shade, int burn)
{
	w->clear();
	if(unit)
	{
		w->update(unit->getArmor()->getScript<ModScript::RecolorUnitSprite>(), unit, save, body_part, anim_frame, shade, burn);
	}
}

ModScript::DamageUnitParser::DamageUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"to_health",
	"to_armor",
	"to_stun",
	"to_time",
	"to_energy",
	"to_morale",
	"to_wound",
	"to_transform",
	"to_mana",

	"unit", "damaging_item", "weapon_item", "attacker",
	"battle_game", "skill", "currPower", "orig_power", "part", "side", "damaging_type", "battle_action", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);

	setEmptyReturn();
}

ModScript::DamageSpecialUnitParser::DamageSpecialUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"transform",
	"transform_chance",
	"self_destruct",
	"self_destruct_chance",
	"morale_loss",
	"fire",

	"unit", "damaging_item", "weapon_item", "attacker",
	"battle_game", "skill", "health_damage", "orig_power", "part", "side", "damaging_type", "battle_action", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);

	setEmptyReturn();
}

ModScript::TryPsiAttackUnitParser::TryPsiAttackUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"psi_attack_success",

	"item",
	"attacker",
	"victim",
	"skill",
	"attack_strength",
	"defense_strength",
	"battle_action",
	"battle_game",
}
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);
}

ModScript::TryMeleeAttackUnitParser::TryMeleeAttackUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"melee_attack_success",

	"item",
	"attacker",
	"victim",
	"skill",
	"attack_strength",
	"defense_strength",
	"battle_action",
	"battle_game",
}
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);
}

ModScript::HitUnitParser::HitUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"power",
	"part",
	"side",
	"unit", "damaging_item", "weapon_item", "attacker",
	"battle_game", "skill", "orig_power", "damaging_type", "battle_action" }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);
}

ModScript::SkillUseUnitParser::SkillUseUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents { shared, name,
	"continue_action",
	"spend_tu",
	"actor",
	"item",
	"battle_game",
	"skill",
	"battle_action",
	"have_tu"
}
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);

	setEmptyReturn();
}

ModScript::HealUnitParser::HealUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"medikit_action_type",
	"body_part",
	"wound_recovery",
	"health_recovery",
	"energy_recovery",
	"stun_recovery",
	"mana_recovery",
	"morale_recovery",
	"painkiller_recovery",
	"actor",
	"item",
	"battle_game",
	"target", "battle_action"}
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);

	medikitBattleActionImpl(b);

	setEmptyReturn();
}

ModScript::CreateUnitParser::CreateUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "unit", "battle_game", "turn", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);
}

ModScript::NewTurnUnitParser::NewTurnUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "unit", "battle_game", "turn", "side", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::ReturnFromMissionUnitParser::ReturnFromMissionUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"recovery_time",
	"mana_loss",
	"health_loss",
	"final_mana_loss",
	"final_health_loss",
	"unit", "battle_game", "soldier", "statChange", "statPrevious" }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	setEmptyReturn();
}

ModScript::AwardExperienceParser::AwardExperienceParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
	"experience_multipler",
	"experience_type",
	"attacker", "unit", "weapon", "battle_action", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);

	battleActionImpl(b);
}


} //namespace OpenXcom
