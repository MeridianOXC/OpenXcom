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
#include "NextTurnState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleEnviroEffects.h"
#include "../Mod/RuleInterface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Palette.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Engine/Action.h"
#include "../Savegame/SavedBattleGame.h"
#include "BattlescapeState.h"
#include "Map.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Next Turn screen.
 * @param game Pointer to the core game.
 * @param battleGame Pointer to the saved game.
 * @param state Pointer to the Battlescape state.
 */
NextTurnState::NextTurnState(SavedBattleGame *battleGame, BattlescapeState *state) : _battleGame(battleGame), _state(state), _timer(0), _currentTurn(0)
{
	_currentTurn = _battleGame->getTurn() < 1 ? 1 : _battleGame->getTurn();

	// Create objects
	int y = state->getMap()->getMessageY();

	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(320, 17, 0, 68);
	_txtTurn = new Text(320, 17, 0, 92);
	_txtSide = new Text(320, 17, 0, 108);
	_txtMessage = new Text(320, 17, 0, 132);
	_txtMessage2 = new Text(320, 33, 0, 156);
	_txtMessage3 = new Text(320, 17, 0, 172);
	_bg = new Surface(_game->getScreen()->getWidth(), _game->getScreen()->getWidth(), 0, 0);

	// Set palette
	battleGame->setPaletteByDepth(this);

	add(_bg);
	add(_window);
	add(_txtTitle, "messageWindows", "battlescape");
	add(_txtTurn, "messageWindows", "battlescape");
	add(_txtSide, "messageWindows", "battlescape");
	add(_txtMessage, "messageWindows", "battlescape");
	add(_txtMessage2, "messageWindows", "battlescape");
	add(_txtMessage3, "messageWindows", "battlescape");

	centerAllSurfaces();

	_bg->setX(0);
	_bg->setY(0);
	SDL_Rect rect;
	rect.h = _bg->getHeight();
	rect.w = _bg->getWidth();
	rect.x = rect.y = 0;

	// Note: un-hardcoded the color from 15 to ruleset value, default 15
	int bgColor = 15;
	auto sc = _battleGame->getEnviroEffects();
	if (sc)
	{
		bgColor = sc->getMapBackgroundColor();
	}
	_bg->drawRect(&rect, Palette::blockOffset(0) + bgColor);
	// make this screen line up with the hidden movement screen
	_window->setY(y);
	_txtTitle->setY(y + 68);
	_txtTurn->setY(y + 92);
	_txtSide->setY(y + 108);
	_txtMessage->setY(y + 132);
	_txtMessage2->setY(y + 156);
	_txtMessage3->setY(y + 172);

	// Set up objects
	_window->setColor(Palette::blockOffset(0)-1);
	_window->setHighContrast(true);
	_window->setBackground(_game->getMod()->getSurface(_battleGame->getHiddenMovementBackground()));


	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setHighContrast(true);
	_txtTitle->setText(tr("STR_OPENXCOM"));


	_txtTurn->setBig();
	_txtTurn->setAlign(ALIGN_CENTER);
	_txtTurn->setHighContrast(true);
	std::stringstream ss;
	ss << tr("STR_TURN").arg(_currentTurn);
	if (battleGame->getTurnLimit() > 0)
	{
		ss << "/" << battleGame->getTurnLimit();
		if (battleGame->getTurnLimit() - _currentTurn <= 3)
		{
			// gonna borrow the inventory's "over weight" colour when we're down to the last three turns
			_txtTurn->setColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color2);
		}
	}
	_txtTurn->setText(ss.str());


	_txtSide->setBig();
	_txtSide->setAlign(ALIGN_CENTER);
	_txtSide->setHighContrast(true);
	_txtSide->setText(tr("STR_SIDE").arg(tr((_battleGame->getSide() == FACTION_PLAYER ? "STR_XCOM" : "STR_ALIENS"))));


	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setHighContrast(true);
	_txtMessage->setText(tr("STR_PRESS_BUTTON_TO_CONTINUE"));

	_txtMessage2->setBig();
	_txtMessage2->setAlign(ALIGN_CENTER);
	_txtMessage2->setHighContrast(true);
	_txtMessage2->setText("");

	_txtMessage3->setBig();
	_txtMessage3->setAlign(ALIGN_CENTER);
	_txtMessage3->setHighContrast(true);
	_txtMessage3->setText("");

	_state->clearMouseScrollingState();

	// environmental effects
	std::string message;

	if (sc)
	{
		if (_battleGame->getSide() == FACTION_PLAYER)
		{
			// beginning of player's turn
			EnvironmentalCondition friendly = sc->getEnvironmetalCondition("STR_FRIENDLY");

			bool showMessage = applyEnvironmentalConditionToFaction(FACTION_PLAYER, friendly);

			if (showMessage)
			{
				_txtMessage2->setColor(friendly.color);
				message = tr(friendly.message);
				_txtMessage2->setText(message);
			}
		}
		else
		{
			// beginning of alien's and neutral's turn
			bool anyoneStanding = false;
			for (std::vector<BattleUnit*>::iterator j = _battleGame->getUnits()->begin(); j != _battleGame->getUnits()->end(); ++j)
			{
				if ((*j)->getOriginalFaction() == FACTION_HOSTILE && !(*j)->isOut())
				{
					anyoneStanding = true;
				}
			}

			if (anyoneStanding)
			{
				EnvironmentalCondition hostile = sc->getEnvironmetalCondition("STR_HOSTILE");
				EnvironmentalCondition neutral = sc->getEnvironmetalCondition("STR_NEUTRAL");

				bool showMessage = false;
				if (applyEnvironmentalConditionToFaction(FACTION_HOSTILE, hostile))
				{
					_txtMessage2->setColor(hostile.color);
					_txtMessage2->setText(tr(hostile.message));
					showMessage = true;
				}
				if (applyEnvironmentalConditionToFaction(FACTION_NEUTRAL, neutral))
				{
					_txtMessage3->setColor(neutral.color);
					_txtMessage3->setText(tr(neutral.message));
					showMessage = true;
				}

				if (showMessage)
				{
					ss.clear();
					ss << tr(hostile.message) << tr(neutral.message);
					message = ss.str();
				}
			}
		}
	}

	if (message.empty())
	{
		_battleGame->appendToHitLog(HITLOG_NEW_TURN, _battleGame->getSide());
	}
	else
	{
		_battleGame->appendToHitLog(HITLOG_NEW_TURN_WITH_MESSAGE, _battleGame->getSide(), message);
	}

	if (_battleGame->getSide() == FACTION_PLAYER)
	{
		checkBugHuntMode();
		_state->bugHuntMessage();
	}

	if (Options::skipNextTurnScreen && message.empty())
	{
		_timer = new Timer(NEXT_TURN_DELAY);
		_timer->onTimer((StateHandler)&NextTurnState::close);
		_timer->start();
	}
}

/**
 *
 */
NextTurnState::~NextTurnState()
{
	delete _timer;
}

/**
* Checks if bug hunt mode should be activated or not.
*/
void NextTurnState::checkBugHuntMode()
{
	// too early for bug hunt
	if (_currentTurn < _battleGame->getBughuntMinTurn()) return;

	// bug hunt is already activated
	if (_battleGame->getBughuntMode()) return;

	int count = 0;
	for (std::vector<BattleUnit*>::iterator j = _battleGame->getUnits()->begin(); j != _battleGame->getUnits()->end(); ++j)
	{
		if (!(*j)->isOut())
		{
			if ((*j)->getOriginalFaction() == FACTION_HOSTILE)
			{
				// we can see them, duh!
				if ((*j)->getVisible()) return;

				// VIPs are still in the game
				if ((*j)->getRankInt() <= _game->getMod()->getBughuntRank()) return; // AR_COMMANDER = 0, AR_LEADER = 1, ...

				count++;
				// too many enemies are still in the game
				if (count > _game->getMod()->getBughuntMaxEnemies()) return;

				bool hasWeapon = (*j)->getLeftHandWeapon() || (*j)->getRightHandWeapon();
				bool hasLowMorale = (*j)->getMorale() < _game->getMod()->getBughuntLowMorale();
				bool hasTooManyTUsLeft = (*j)->getTimeUnits() > ((*j)->getUnitRules()->getStats()->tu * _game->getMod()->getBughuntTimeUnitsLeft() / 100);
				if (!hasWeapon || hasLowMorale || hasTooManyTUsLeft)
				{
					continue; // this unit is powerless, check next unit...
				}
				else
				{
					return; // this unit is OK, no bug hunt yet!
				}
			}
		}
	}

	_battleGame->setBughuntMode(true); // if we made it this far, turn on the bug hunt!
}

/**
* Applies given environmental condition effects to a given faction.
* @return True, if any unit was affected (even if damage was zero).
*/
bool NextTurnState::applyEnvironmentalConditionToFaction(UnitFaction faction, EnvironmentalCondition condition)
{
	if (!_battleGame->getEnvironmentalConditionsEnabled(faction))
	{
		return false;
	}

	// Killing people before battle starts causes a crash
	// Panicking people before battle starts causes endless loop
	// Let's just avoid this instead of reworking everything
	if (faction == FACTION_PLAYER && _currentTurn <= 1)
	{
		return false;
	}

	// Note: there are limitations, since we're not using a projectile and nobody is actually shooting
	// 1. no power bonus based on shooting unit's stats (nobody is shooting)
	// 2. no power range reduction (there is no projectile, range = 0)
	// 3. no AOE damage from explosions (targets are damaged directly without affecting anyone/anything)
	// 4. no terrain damage
	// 5. no self-destruct
	// 6. no vanilla target morale loss when hurt; vanilla morale loss for fatal wounds still applies though
	//
	// 7. no setting target on fire (...could be added if needed)
	// 8. no fire extinguisher

	bool showMessage = false;

	if (condition.chancePerTurn > 0 && condition.firstTurn <= _currentTurn && _currentTurn <= condition.lastTurn)
	{
		const RuleItem *weaponOrAmmo = _game->getMod()->getItem(condition.weaponOrAmmo);
		const RuleDamageType *type = weaponOrAmmo->getDamageType();
		const int power = weaponOrAmmo->getPower(); // no power bonus, no power range reduction

		UnitSide side = SIDE_FRONT;
		UnitBodyPart bodypart = BODYPART_TORSO;

		if (condition.side > -1)
		{
			side = (UnitSide)condition.side;
		}
		if (condition.bodyPart > -1)
		{
			bodypart = (UnitBodyPart)condition.bodyPart;
		}

		for (std::vector<BattleUnit*>::iterator j = _battleGame->getUnits()->begin(); j != _battleGame->getUnits()->end(); ++j)
		{
			if ((*j)->getOriginalFaction() == faction && (*j)->getStatus() != STATUS_DEAD)
			{
				if (RNG::percent(condition.chancePerTurn))
				{
					if (condition.side == -1)
					{
						side = (UnitSide)RNG::generate((int)SIDE_FRONT, (int)SIDE_UNDER);
					}
					if (condition.bodyPart == -1)
					{
						bodypart = (UnitBodyPart)RNG::generate(BODYPART_HEAD, BODYPART_LEFTLEG);
					}
					(*j)->damage(Position(0, 0, 0), type->getRandomDamage(power), type, _battleGame, { }, side, bodypart);
					showMessage = true;
				}
			}
		}
	}

	// now check for new casualties
	_battleGame->getBattleGame()->checkForCasualties(nullptr, BattleActionAttack{ }, true, false);
	// revive units if damage could give hp or reduce stun
	//_battleGame->reviveUnconsciousUnits(true);

	return showMessage;
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void NextTurnState::handle(Action *action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN || action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		close();
	}
}

/**
 * Keeps the timer running.
 */
void NextTurnState::think()
{
	if (_timer)
	{
		_timer->think(this, 0);
	}
}

/**
 * Closes the window.
 */
void NextTurnState::close()
{
	_battleGame->getBattleGame()->cleanupDeleted();
	_game->popState();

	int liveAliens = 0;
	int liveSoldiers = 0;
	_state->getBattleGame()->tallyUnits(liveAliens, liveSoldiers);

	if ((_battleGame->getObjectiveType() != MUST_DESTROY && liveAliens == 0) || liveSoldiers == 0)		// not the final mission and all aliens dead.
	{
		_state->finishBattle(false, liveSoldiers);
	}
	else
	{
		_state->btnCenterClick(0);

		// Autosave every set amount of turns
		if ((_currentTurn == 1 || _currentTurn % Options::autosaveFrequency == 0) && _battleGame->getSide() == FACTION_PLAYER)
		{
			_state->autosave();
		}
	}
}

void NextTurnState::resize(int &dX, int &dY)
{
	State::resize(dX, dY);
	_bg->setX(0);
	_bg->setY(0);
}

}
