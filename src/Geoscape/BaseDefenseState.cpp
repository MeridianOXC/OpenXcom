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
#include "BaseDefenseState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Region.h"
#include "../Mod/RuleRegion.h"
#include "../Savegame/BaseFacility.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Savegame/Ufo.h"
#include "../Interface/TextList.h"
#include "GeoscapeState.h"
#include "../Engine/Action.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Base Defense screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base being attacked.
 * @param ufo Pointer to the attacking ufo.
 * @param state Pointer to the Geoscape.
 */
BaseDefenseState::BaseDefenseState(Base *base, Ufo *ufo, GeoscapeState *state, bool instaHyper) : _state(state)
{
	bool showUfo = _game->getMod()->showUfoPreviewInBaseDefense();

	_base = base;
	_action = BDA_NONE;
	_row = -1;
	_passes = 0;
	_attacks = 0;
	_thinkcycles = 0;
	_ufo = ufo;
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 16, 6);
	_txtInit = new Text(300, 10, 16, 24);
	_lstDefenses = new TextList(300, 128, 16, 40);
	_btnOk = new TextButton(120, 18, 100, 170);
	_btnStart = new TextButton(showUfo ? 148-13 : 148, 16, 8, 176);
	_btnAbort = new TextButton(showUfo ? 148+13 : 148, 16, showUfo ? 164-13 : 164, 176);
	_preview = new InteractiveSurface(160, 96, 151, 75);

	// Set palette
	std::string baseDefenseInterface = showUfo ? "baseDefense_geo" : "baseDefense";
	setInterface(baseDefenseInterface);

	add(_window, "window", baseDefenseInterface);
	add(_btnOk, "button", baseDefenseInterface);
	add(_btnStart, "button", baseDefenseInterface);
	add(_btnAbort, "button", baseDefenseInterface);
	add(_txtTitle, "text", baseDefenseInterface);
	add(_txtInit, "text", baseDefenseInterface);
	add(_lstDefenses, "text", baseDefenseInterface);
	add(_preview);

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, baseDefenseInterface);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BaseDefenseState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&BaseDefenseState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&BaseDefenseState::btnOkClick, Options::keyCancel);
	_btnOk->setVisible(false);

	_btnStart->setText(tr("STR_START_FIRING"));
	_btnStart->onMouseClick((ActionHandler)&BaseDefenseState::btnStartClick);

	_btnAbort->setText(tr("STR_SKIP_FIRING"));
	_btnAbort->onMouseClick((ActionHandler)&BaseDefenseState::btnOkClick);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_BASE_UNDER_ATTACK").arg(_base->getName()));
	_txtInit->setVisible(false);

	_txtInit->setText(tr("STR_BASE_DEFENSES_INITIATED"));

	_lstDefenses->setColumns(3, 134, 70, 50);
	_lstDefenses->setFlooding(true);

	if (showUfo)
	{
		RuleInterface* dogfightInterface = _game->getMod()->getInterface("dogfight");

		SurfaceCrop crop = _game->getMod()->getSurface("INTERWIN.DAT")->getCrop();
		crop.setX(0);
		crop.setY(0);
		crop.getCrop()->x = 0;
		crop.getCrop()->y = 0;
		crop.getCrop()->w = _preview->getWidth();
		crop.getCrop()->h = _preview->getHeight();
		//_preview->drawRect(crop.getCrop(), 15);
		//crop.blit(_preview);

		_preview->drawRect(crop.getCrop(), 15);
		crop.getCrop()->y = dogfightInterface->getElement("previewTop")->y;
		crop.getCrop()->h = dogfightInterface->getElement("previewTop")->h;
		crop.blit(_preview);

		crop.setY(_preview->getHeight() - dogfightInterface->getElement("previewBot")->h);
		crop.getCrop()->y = dogfightInterface->getElement("previewBot")->y;
		crop.getCrop()->h = dogfightInterface->getElement("previewBot")->h;
		crop.blit(_preview);

		if (ufo->getRules()->getModSprite().empty())
		{
			crop.getCrop()->y = dogfightInterface->getElement("previewMid")->y + dogfightInterface->getElement("previewMid")->h * _ufo->getRules()->getSprite();
			crop.getCrop()->h = dogfightInterface->getElement("previewMid")->h;
		}
		else
		{
			crop = _game->getMod()->getSurface(ufo->getRules()->getModSprite())->getCrop();
		}
		crop.setX(dogfightInterface->getElement("previewTop")->x);
		crop.setY(dogfightInterface->getElement("previewTop")->h);
		crop.blit(_preview);

		// extra info
		bool extraInfo = ufo->getHyperDetected();
		if (!extraInfo && instaHyper)
		{
			for (auto* fac : *_base->getFacilities())
			{
				if (fac->getBuildTime() == 0 && fac->getRules()->isHyperwave())
				{
					extraInfo = true;
					break;
				}
			}
		}
		if (extraInfo)
		{
			std::ostringstream ss;
			ss << Unicode::TOK_COLOR_FLIP << tr(_ufo->getRules()->getType());
			_lstDefenses->addRow(3, tr("STR_CRAFT_TYPE").c_str(), ss.str().c_str(), "");

			ss.str("");
			ss << Unicode::TOK_COLOR_FLIP << tr(_ufo->getAlienRace());
			_lstDefenses->addRow(3, tr("STR_RACE").c_str(), ss.str().c_str(), "");

			ss.str("");
			ss << Unicode::TOK_COLOR_FLIP << tr(_ufo->getMissionType());
			_lstDefenses->addRow(3, tr("STR_MISSION").c_str(), ss.str().c_str(), "");

		}
	}
	else
	{
		_preview->setVisible(false);
	}

	_gravShields = _base->getGravShields();
	_defenses = _base->getDefenses()->size();
	_timer = new Timer(250);
	_timer->onTimer((StateHandler)&BaseDefenseState::nextStep);

	_explosionCount = 0;

	if (_ufo->getRules()->getMissilePower() != 0)
	{
		if (showUfo)
		{
			_btnAbort->setVisible(false);
		}
		else
		{
			btnStartClick(0);
		}
	}
}

/**
 *
 */
BaseDefenseState::~BaseDefenseState()
{
	delete _timer;
}

void BaseDefenseState::think()
{
	_timer->think(this, 0);
}

bool BaseDefenseState::applyDamage(const RuleBaseFacility* rule)
{
	bool shieldDown = false;

	int power = rule->getDefenseValue();

	if (rule->unifiedDamageFormula() && rule->getAmmoItem())
	{
		// unified damage formula
		int damage = rule->getAmmoItem()->getDamageType()->getRandomDamage(power);

		if (_ufo->getShield() != 0)
		{
			int shieldDamage = damage * rule->getShieldDamageModifier() / 100;
			if (rule->getShieldDamageModifier() == 0)
			{
				damage = 0;
			}
			else
			{
				// scale down by bleed-through factor and scale up by shield-effectiveness factor
				damage = std::max(0, shieldDamage - _ufo->getShield()) * _ufo->getCraftStats().shieldBleedThrough / rule->getShieldDamageModifier();
			}
			_ufo->setShield(_ufo->getShield() - shieldDamage);

			if (_ufo->getShield() == 0)
			{
				shieldDown = true;
			}
		}

		damage = std::max(0, damage - _ufo->getCraftStats().armor);
		_ufo->setDamage(_ufo->getDamage() + damage, _game->getMod());
	}
	else
	{
		// vanilla dmg formula: 50-150%
		int dmg = power / 2 + RNG::generate(0, power);

		if (_ufo->getShield() > 0)
		{
			int shieldDmg = dmg * rule->getShieldDamageModifier() / 100;
			if (rule->getShieldDamageModifier() == 0)
			{
				dmg = 0;
			}
			else
			{
				// scale down by bleed-through factor NOT performed for backwards-compatibility
				// only scale up by shield-effectiveness factor
				dmg = std::max(0, shieldDmg - _ufo->getShield()) * 100 / rule->getShieldDamageModifier();
			}
			_ufo->setShield(_ufo->getShield() - shieldDmg);

			if (_ufo->getShield() == 0)
			{
				shieldDown = true;
			}
		}
		_ufo->setDamage(_ufo->getDamage() + dmg, _game->getMod());
	}

	return shieldDown;
}

void BaseDefenseState::nextStep()
{
	if (_thinkcycles == -1)
		return;

	++_thinkcycles;

	if (_thinkcycles == 1)
	{
		_txtInit->setVisible(true);
		return;
	}

	if (_thinkcycles > 1)
	{
		switch (_action)
		{
		case BDA_DESTROY:
			if (!_explosionCount)
			{
				_lstDefenses->addRow(2, tr("STR_UFO_DESTROYED").c_str()," "," ");
				++_row;
				if (_row > 14)
				{
					_lstDefenses->scrollDown(true);
				}
			}
			_game->getMod()->getSound("GEO.CAT", Mod::UFO_EXPLODE)->play();
			if (++_explosionCount == 3)
			{
				_action = BDA_END;
			}
			return;
		case BDA_END:
			_btnOk->setVisible(true);
			_thinkcycles = -1;
			return;
		default:
			break;
		}
		if (_attacks == _defenses && _passes == _gravShields)
		{
			_action = BDA_END;
			return;
		}
		else if (_attacks == _defenses && _passes < _gravShields)
		{
			_lstDefenses->addRow(3, tr("STR_GRAV_SHIELD_REPELS_UFO").c_str()," "," ");
			if (_row > 14)
			{
				_lstDefenses->scrollDown(true);
			}
			++_row;
			++_passes;
			_attacks = 0;
			return;
		}



		BaseFacility* def = _base->getDefenses()->at(_attacks);
		const RuleItem* ammo = (def)->getRules()->getAmmoItem();
		int ammoNeeded = (def)->getRules()->getAmmoNeeded();
		bool hasOwnAmmo = def->getRules()->getAmmoMax() > 0;
		bool spendAmmo = false;

		int chanceToHit = def->getRules()->getHitRatio(); // vanilla xcom
		chanceToHit -= _ufo->getCraftStats().avoidBonus2;

		switch (_action)
		{
		case  BDA_NONE:
			_lstDefenses->addRow(3, tr((def)->getRules()->getType()).c_str()," "," ");
			++_row;
			_action = BDA_FIRE;
			if (_row > 14)
			{
				_lstDefenses->scrollDown(true);
			}
			return;
		case BDA_FIRE:
			if (hasOwnAmmo && def->getAmmo() < ammoNeeded)
			{
				_lstDefenses->setCellText(_row, 1, tr("STR_NO_AMMO"));
			}
			else if (!hasOwnAmmo && ammo && _base->getStorageItems()->getItem(ammo) < ammoNeeded)
			{
				_lstDefenses->setCellText(_row, 1, tr("STR_NO_AMMO"));
			}
			else
			{
				_lstDefenses->setCellText(_row, 1, tr("STR_FIRING"));
				_game->getMod()->getSound("GEO.CAT", (def)->getRules()->getFireSound())->play();
			}
			_timer->setInterval(333);
			_action = BDA_RESOLVE;
			return;
		case BDA_RESOLVE:
			if (hasOwnAmmo && def->getAmmo() < ammoNeeded)
			{
				//_lstDefenses->setCellText(_row, 2, tr("STR_NO_AMMO"));
			}
			else if (!hasOwnAmmo && ammo && _base->getStorageItems()->getItem(ammo) < ammoNeeded)
			{
				//_lstDefenses->setCellText(_row, 2, tr("STR_NO_AMMO"));
			}
			else if (!RNG::percent(chanceToHit))
			{
				spendAmmo = true;
				_lstDefenses->setCellText(_row, 2, tr("STR_MISSED"));
			}
			else
			{
				spendAmmo = true;
				_lstDefenses->setCellText(_row, 2, tr("STR_HIT"));
				_game->getMod()->getSound("GEO.CAT", (def)->getRules()->getHitSound())->play();

				bool shieldDown = applyDamage(def->getRules());

				if (shieldDown)
				{
					_lstDefenses->addRow(3, tr("STR_UFO_SHIELD_DOWN").c_str(), " ", " ");
					++_row;
					if (_row > 14)
					{
						_lstDefenses->scrollDown(true);
					}
				}
			}
			if (spendAmmo && ammoNeeded > 0)
			{
				if (hasOwnAmmo)
				{
					def->setAmmo(def->getAmmo() - ammoNeeded);
					def->resetAmmoMissingReported();
				}
				else if (!hasOwnAmmo && ammo)
				{
					_base->getStorageItems()->removeItem(ammo, ammoNeeded);
				}
			}
			if (_ufo->getStatus() == Ufo::DESTROYED)
				_action = BDA_DESTROY;
			else
				_action = BDA_NONE;
			++_attacks;
			_timer->setInterval(250);
			return;
		default:
			break;
		}
	}
}

/**
* Starts base defense
* @param action Pointer to an action.
*/
void BaseDefenseState::btnStartClick(Action *)
{
	_preview->setVisible(false);
	_lstDefenses->clearList();

	_btnStart->setVisible(false);
	_btnAbort->setVisible(false);
	_timer->start();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BaseDefenseState::btnOkClick(Action *)
{
	_timer->stop();
	_game->popState();
	if (_ufo->getStatus() != Ufo::DESTROYED)
	{
		_state->handleBaseDefense(_base, _ufo);
	}
	else
	{
		_base->cleanupDefenses(true);

		// instant retaliation mission only spawns one UFO and then ends
		if (_ufo->getMission()->getRules().getObjective() == OBJECTIVE_INSTANT_RETALIATION)
		{
			_ufo->getMission()->setInterrupted(true);
		}

		// aliens are not stupid and should stop trying eventually
		if (_ufo->getMission()->getRules().getObjective() == OBJECTIVE_RETALIATION && RNG::percent(_game->getMod()->getChanceToStopRetaliation()))
		{
			// unmark base...
			_base->setRetaliationTarget(false);

			AlienMission* am = _base->getRetaliationMission();
			if (!am)
			{
				// backwards-compatibility
				RuleRegion* regionRule = _game->getSavedGame()->getRegions()->front()->getRules(); // wrong, but that's how it is in OXC
				for (const auto* region : *_game->getSavedGame()->getRegions())
				{
					if (region->getRules()->insideRegion(_base->getLongitude(), _base->getLatitude()))
					{
						regionRule = region->getRules();
						break;
					}
				}
				am = _game->getSavedGame()->findAlienMission(regionRule->getType(), OBJECTIVE_RETALIATION);
			}

			if (am && am->getRules().isMultiUfoRetaliation())
			{
				// Remember that more UFOs may be coming
				am->setMultiUfoRetaliationInProgress(true);
			}
			else
			{
				// Delete the mission and any live UFOs
				_game->getSavedGame()->deleteRetaliationMission(am, _base);
			}
		}
	}
}

}
