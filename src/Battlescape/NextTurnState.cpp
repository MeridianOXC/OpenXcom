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
#include "../Mod/AlienRace.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleEnviroEffects.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleTerrain.h"
#include "../Mod/RuleUfo.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Palette.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Engine/Action.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/HitLog.h"
#include "AIModule.h"
#include "BattlescapeState.h"
#include "BattlescapeGame.h"
#include "BriefingState.h"
#include "Map.h"
#include "TileEngine.h"
#include "Pathfinding.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Next Turn screen.
 * @param game Pointer to the core game.
 * @param battleGame Pointer to the saved game.
 * @param state Pointer to the Battlescape state.
 */
NextTurnState::NextTurnState(SavedBattleGame *battleGame, BattlescapeState *state) : _battleGame(battleGame), _state(state), _timer(0), _currentTurn(0), _showBriefing(false)
{
	if (_battleGame->isPreview())
	{
		// skip everything, go straight to init()
		return;
	}

	_currentTurn = _battleGame->getTurn() < 1 ? 1 : _battleGame->getTurn();

	// Create objects
	int y = state->getMap()->getMessageY();

	_window = new Window(this, 320, 200, 0, 0);
	_txtMessageReinforcements = new Text(320, 33, 0, 8);
	_btnBriefingReinforcements = new TextButton(120, 16, 100, 45);
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
	add(_txtMessageReinforcements, "messageWindows", "battlescape");
	add(_btnBriefingReinforcements, "messageWindowButtons", "battlescape");
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
	_txtMessageReinforcements->setY(y + 8);
	_btnBriefingReinforcements->setY(y + 45);
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


	_txtMessageReinforcements->setBig();
	_txtMessageReinforcements->setAlign(ALIGN_CENTER);
	_txtMessageReinforcements->setVerticalAlign(ALIGN_BOTTOM);
	_txtMessageReinforcements->setWordWrap(true);
	_txtMessageReinforcements->setHighContrast(true);
	_txtMessageReinforcements->setColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color2); // red

	_btnBriefingReinforcements->setText(tr("STR_TELL_ME_MORE"));
	_btnBriefingReinforcements->setHighContrast(true);
	_btnBriefingReinforcements->onMouseClick((ActionHandler)&NextTurnState::btnBriefingReinforcementsClick);
	_btnBriefingReinforcements->setVisible(false);

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

	if (sc && !_battleGame->getBattleGame()->areAllEnemiesNeutralized())
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
				if (((*j)->getOriginalFaction() == FACTION_HOSTILE || (*j)->getOriginalFaction() == FACTION_ALIEN_PLAYER) && !(*j)->isOut())
				{
					anyoneStanding = true;
				}
			}

			if (anyoneStanding)
			{
				EnvironmentalCondition hostile = sc->getEnvironmetalCondition("STR_HOSTILE");
				EnvironmentalCondition neutral = sc->getEnvironmetalCondition("STR_NEUTRAL");

				bool showMessage = false;
				if ((applyEnvironmentalConditionToFaction(FACTION_HOSTILE, hostile) || applyEnvironmentalConditionToFaction(FACTION_ALIEN_PLAYER, hostile)))
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

	std::string messageReinforcements;
	bool allowReinforcements = (_battleGame->getSide() == FACTION_HOSTILE || _battleGame->getSide() == FACTION_ALIEN_PLAYER) && _battleGame->getTurn() > 0; //Bug double reinforcements
	if (_battleGame->getSide() == FACTION_PLAYER && _battleGame->getTurn() == 0)
	{
		allowReinforcements = true;
	}
	if (allowReinforcements && !_battleGame->getBattleGame()->areAllEnemiesNeutralized())
	{
		bool showAlert = determineReinforcements();
		if (showAlert && _battleGame->getTurn() > 0)
		{
			messageReinforcements = tr("STR_REINFORCEMENTS_ALERT");
			_txtMessageReinforcements->setText(messageReinforcements);
		}
	}

	if (Options::skipNextTurnScreen && message.empty() && messageReinforcements.empty())
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

void NextTurnState::init()
{
	State::init();

	if (_battleGame->isPreview())
	{
		_battleGame->getBattleGame()->cleanupDeleted();
		_game->popState();

		if (_battleGame->getSide() == FACTION_ALIEN_PLAYER)
		{
			// end preview
			_state->finishBattle(false, 42);
		}
		else
		{
			// start preview
			_state->btnCenterClick(0);

			// Try to reactivate the touch buttons at the start of the player's turn
			if ((_battleGame->getSide() == FACTION_PLAYER) || (_battleGame->getSide() == FACTION_ALIEN_PLAYER) || (_battleGame->getSide() == FACTION_NEUTRAL)) // bug neural terror skip turn abort
			{
				_state->toggleTouchButtons(false, true);
			}
			// Try to reactivate the touch buttons at the start of the player's turn THE CLIENT
			//if (_battleGame->getSide() == FACTION_ALIEN_PLAYER)
			//{
			//	_state->toggleTouchButtons(false, true);
			//}
		}
	}
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
			if ((*j)->getOriginalFaction() == faction && (*j)->getStatus() != STATUS_DEAD && !(*j)->isIgnored())
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

	if (_btnBriefingReinforcements->getVisible() && action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		double mx = action->getAbsoluteXMouse();
		double my = action->getAbsoluteYMouse();
		if (mx >= _btnBriefingReinforcements->getX() && mx < _btnBriefingReinforcements->getX() + _btnBriefingReinforcements->getWidth() &&
			my >= _btnBriefingReinforcements->getY() && my < _btnBriefingReinforcements->getY() + _btnBriefingReinforcements->getHeight())
		{
			// don't close on Briefing button click
			return;
		}
	}

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

	auto tally = _state->getBattleGame()->tallyUnits();

	if (_battleGame->getBattleGame()->areAllEnemiesNeutralized()) // DELETE THIS AND OTHER THING TO MIND CONTROL EVERYTHING
	{
		// we don't care if someone was revived in the meantime, the decision to end the battle was already made!
		tally.liveAliens = 0;

		// mind control anyone who was revived (needed for correct recovery in the debriefing)
		for (auto bu : *_battleGame->getUnits())
		{
			if ((bu->getOriginalFaction() == FACTION_HOSTILE || bu->getOriginalFaction() == FACTION_ALIEN_PLAYER) && !bu->isOut()) // ALSO DELETE THIS AND OTHER THI
			{
				bu->convertToFaction(FACTION_PLAYER);
			}
		}

		// reset needed because of the potential next stage in multi-stage missions
		_battleGame->getBattleGame()->resetAllEnemiesNeutralized();
	}
	 // DELETE THIS AND OTHER THING TO MIND CONTROL EVERYTHING
	{
		// we don't care if someone was revived in the meantime, the decision to end the battle was already made!
		//tally.liveAliens = 0;

		// mind control anyone who was revived (needed for correct recovery in the debriefing)
		//for (auto bu : *_battleGame->getUnits())
		//{
		//	if (bu->getOriginalFaction() == FACTION_HOSTILE) // ALSO DELETE THIS AND OTHER THI
		//	{
		//		bu->convertToFaction(FACTION_ALIEN_PLAYER);
		//	}
		//}

		// reset needed because of the potential next stage in multi-stage missions
		// _battleGame->getBattleGame()->resetAllEnemiesNeutralized();
	}
	// not "escort the VIPs" missions, not the final mission and all aliens dead.
	bool killingAllAliensIsNotEnough = _battleGame->getObjectiveType() == MUST_DESTROY || (_battleGame->getVIPSurvivalPercentage() > 0 && _battleGame->getVIPEscapeType() != ESCAPE_NONE);
	if ((!killingAllAliensIsNotEnough && tally.liveAliens == 0) || tally.liveSoldiers == 0)
	{
		_state->finishBattle(false, tally.liveSoldiers);
	}
	else
	{
		_state->btnCenterClick(0);
		// Try to reactivate the touch buttons at the start of the player's turn HOST
		if ((_battleGame->getSide() == FACTION_PLAYER) || (_battleGame->getSide() == FACTION_ALIEN_PLAYER) || (_battleGame->getSide() == FACTION_NEUTRAL)) // bug neural terror skip turn abort 
		{
			_state->toggleTouchButtons(false, true);
		}
		// Try to reactivate the touch buttons at the start of the player's turn HOST
		//if (_battleGame->getSide() == FACTION_PLAYER)
		//{
		//	_state->toggleTouchButtons(false, true);
		//}
		// Try to reactivate the touch buttons at the start of the player's turn THE CLIENT
		//if (_battleGame->getSide() == FACTION_ALIEN_PLAYER)
		//{
		//	_state->toggleTouchButtons(false, true);
		//}
		// Try to reactivate the touch buttons at the start of the player's turn NEUTRAL
		//if (_battleGame->getSide() == FACTION_NEUTRAL)
		//{
		//	_state->toggleTouchButtons(false, true);
		//}
		//
		// Autosave every set amount of turns
		if ((_currentTurn == 1 || _currentTurn % Options::autosaveFrequency == 0) && _battleGame->getSide() == FACTION_PLAYER)
		{
			_state->autosave(_currentTurn);
		}
	}
}

void NextTurnState::resize(int &dX, int &dY)
{
	State::resize(dX, dY);
	_bg->setX(0);
	_bg->setY(0);
}

/**
 * Shows the custom briefing for reinforcements.
 * @param action Pointer to an action.
 */
void NextTurnState::btnBriefingReinforcementsClick(Action*)
{
	if (_showBriefing)
	{
		_game->pushState(new BriefingState(0, 0, true, &_customBriefing));
	}
}

/**
 * Runs reinforcements logic.
 */
bool NextTurnState::determineReinforcements()
{
	const AlienDeployment* deployment = _game->getMod()->getDeployment(_battleGame->getReinforcementsDeployment(), true);

	int currentTurnReinforcements = _battleGame->getTurn();

	if (!deployment)
	{
		// for backwards-compatibility! this save does not contain the data needed for this functionality...
		return false;
	}

	bool showAlert = false;
	for (auto& wave : *deployment->getReinforcementsData())
	{
		// 1. check pre-requisites
		{
			if (_game->getSavedGame()->getDifficulty() < wave.minDifficulty || _game->getSavedGame()->getDifficulty() > wave.maxDifficulty)
			{
				continue;
			}
			if (wave.objectiveDestroyed)
			{
				if (!_battleGame->allObjectivesDestroyed())
				{
					continue;
				}
			}
			else if (!wave.turns.empty())
			{
				if (std::find(wave.turns.begin(), wave.turns.end(), currentTurnReinforcements) == wave.turns.end())
				{
					continue;
				}
			}
			else
			{
				if (currentTurnReinforcements < wave.minTurn || (wave.maxTurn != -1 && currentTurnReinforcements > wave.maxTurn))
				{
					continue;
				}
			}
			if (wave.executionOdds < 100 && !RNG::percent(wave.executionOdds))
			{
				continue;
			}
			if (wave.maxRuns != -1 && _battleGame->getReinforcementsMemory()[wave.type] >= wave.maxRuns)
			{
				continue;
			}
		}

		// 2a. calculate compliant blocks
		{
			_compliantBlocksMap.clear();
			_compliantBlocksList.clear();

			int sizeX = _battleGame->getMapSizeX() / 10;
			int sizeY = _battleGame->getMapSizeY() / 10;

			if (wave.mapBlockFilterType == MFT_BY_MAPSCRIPT)
			{
				_compliantBlocksMap = _battleGame->getReinforcementsBlocks(); // we're done
			}
			else if (wave.mapBlockFilterType == MFT_BY_REINFORCEMENTS || wave.mapBlockFilterType == MFT_BY_BOTH_UNION || wave.mapBlockFilterType == MFT_BY_BOTH_INTERSECTION)
			{
				if (wave.spawnBlocks.empty())
					_compliantBlocksMap.resize((sizeX), std::vector<int>((sizeY), 1)); // start with all true
				else
					_compliantBlocksMap.resize((sizeX), std::vector<int>((sizeY), 0)); // start with all false
			}
			else //if (wave.mapBlockFilterType == MFT_NONE)
			{
				_compliantBlocksMap.resize((sizeX), std::vector<int>((sizeY), 1)); // all true (and we're done)
			}

			if (wave.mapBlockFilterType == MFT_BY_REINFORCEMENTS || wave.mapBlockFilterType == MFT_BY_BOTH_UNION || wave.mapBlockFilterType == MFT_BY_BOTH_INTERSECTION)
			{
				if (!wave.spawnBlocks.empty())
				{
					for (auto& dir : wave.spawnBlocks)
					{
						if (dir == "EDGES")
						{
							for (int x = 0; x < sizeX; ++x) _compliantBlocksMap[x][0] = 1;
							for (int y = 0; y < sizeY; ++y) _compliantBlocksMap[0][y] = 1;
							for (int x = 0; x < sizeX; ++x) _compliantBlocksMap[x][sizeY - 1] = 1;
							for (int y = 0; y < sizeY; ++y) _compliantBlocksMap[sizeX - 1][y] = 1;
							break;
						}
						if (dir == "NORTH")
							for (int x = 0; x < sizeX; ++x) _compliantBlocksMap[x][0] = 1;
						else if (dir == "WEST")
							for (int y = 0; y < sizeY; ++y) _compliantBlocksMap[0][y] = 1;
						else if (dir == "SOUTH")
							for (int x = 0; x < sizeX; ++x) _compliantBlocksMap[x][sizeY - 1] = 1;
						else if (dir == "EAST")
							for (int y = 0; y < sizeY; ++y) _compliantBlocksMap[sizeX - 1][y] = 1;
						else if (dir == "NW")
							_compliantBlocksMap[0][0] = 1;
						else if (dir == "NE")
							_compliantBlocksMap[sizeX - 1][0] = 1;
						else if (dir == "SW")
							_compliantBlocksMap[0][sizeY - 1] = 1;
						else if (dir == "SE")
							_compliantBlocksMap[sizeX - 1][sizeY - 1] = 1;
					}
				}

				if (wave.mapBlockFilterType == MFT_BY_BOTH_UNION)
				{
					auto& toMerge = _battleGame->getReinforcementsBlocks();
					for (int x = 0; x < sizeX; ++x)
						for (int y = 0; y < sizeY; ++y)
							_compliantBlocksMap[x][y] = _compliantBlocksMap[x][y] + toMerge[x][y];
				}
				else if (wave.mapBlockFilterType == MFT_BY_BOTH_INTERSECTION)
				{
					auto& toMerge = _battleGame->getReinforcementsBlocks();
					for (int x = 0; x < sizeX; ++x)
						for (int y = 0; y < sizeY; ++y)
							_compliantBlocksMap[x][y] = _compliantBlocksMap[x][y] * toMerge[x][y];
				}
			}

			bool checkGroups = !wave.spawnBlockGroups.empty();

			_compliantBlocksList.reserve(sizeX * sizeY);

			for (int x = 0; x < sizeX; ++x)
			{
				for (int y = 0; y < sizeY; ++y)
				{
					if (_compliantBlocksMap[x][y] > 0)
					{
						if (checkGroups)
						{
							auto terrain = _game->getMod()->getTerrain(_battleGame->getFlattenedMapTerrainNames()[x][y], false);
							if (!terrain)
							{
								auto craft = _game->getMod()->getCraft(_battleGame->getFlattenedMapTerrainNames()[x][y], false);
								if (craft)
								{
									terrain = craft->getBattlescapeTerrainData();
								}
							}
							if (!terrain)
							{
								auto ufo = _game->getMod()->getUfo(_battleGame->getFlattenedMapTerrainNames()[x][y], false);
								if (ufo)
								{
									terrain = ufo->getBattlescapeTerrainData();
								}
							}
							if (terrain)
							{
								auto mapblock = terrain->getMapBlock(_battleGame->getFlattenedMapBlockNames()[x][y]);
								if (mapblock)
								{
									bool groupMatched = false;
									for (auto group : wave.spawnBlockGroups)
									{
										if (mapblock->isInGroup(group))
										{
											groupMatched = true;
											break;
										}
									}
									if (!groupMatched)
									{
										continue; // don't add this map block into _compliantBlocksList
									}
								}
							}
						}
						_compliantBlocksList.push_back(Position(x, y, 0));
					}
				}
			}
		}

		// synchronize _compliantBlocksList and _compliantBlocksMap
		{
			for (int x = 0; x < _battleGame->getMapSizeX() / 10; ++x)
				for (int y = 0; y < _battleGame->getMapSizeY() / 10; ++y)
					_compliantBlocksMap[x][y] = 0;

			for (auto& pos : _compliantBlocksList)
				_compliantBlocksMap[pos.x][pos.y] = 1;
		}

		// 2b. calculate compliant nodes
		_compliantNodesList.clear();
		if (wave.useSpawnNodes)
		{
			bool checkBlocks = wave.mapBlockFilterType != MFT_NONE;
			bool checkNodeRanks = !wave.spawnNodeRanks.empty();
			bool checkZLevels = !wave.spawnZLevels.empty();

			_compliantNodesList.reserve(_battleGame->getNodes()->size());

			for (auto node : *_battleGame->getNodes())
			{
				if (node->isDummy())
				{
					continue;
				}
				if (checkNodeRanks && std::find(wave.spawnNodeRanks.begin(), wave.spawnNodeRanks.end(), (int)node->getRank()) == wave.spawnNodeRanks.end())
				{
					continue;
				}
				if (checkZLevels && std::find(wave.spawnZLevels.begin(), wave.spawnZLevels.end(), node->getPosition().z) == wave.spawnZLevels.end())
				{
					continue;
				}
				if (checkBlocks && _compliantBlocksMap[node->getPosition().x / 10][node->getPosition().y / 10] == 0)
				{
					continue;
				}
				auto tileToCheck = _battleGame->getTile(node->getPosition());
				if (tileToCheck && tileToCheck->getUnit())
				{
					continue;
				}
				if (wave.minDistanceFromXcomUnits > 1)
				{
					bool foundXcomUnitNearby = false;
					for (auto xcomUnit : *_battleGame->getUnits())
					{
						if (xcomUnit->getOriginalFaction() == FACTION_PLAYER &&
							!xcomUnit->isOut() &&
							Position::distanceSq(xcomUnit->getPosition(), node->getPosition()) < wave.minDistanceFromXcomUnits * wave.minDistanceFromXcomUnits)
						{
							foundXcomUnitNearby = true;
							break;
						}
					}
					if (foundXcomUnitNearby) continue;
				}
				if (wave.maxDistanceFromBorders > 0 && wave.maxDistanceFromBorders < 10)
				{
					if (node->getPosition().x >= wave.maxDistanceFromBorders &&
						node->getPosition().x < _battleGame->getMapSizeX() - wave.maxDistanceFromBorders &&
						node->getPosition().y >= wave.maxDistanceFromBorders &&
						node->getPosition().y < _battleGame->getMapSizeY() - wave.maxDistanceFromBorders)
					{
						continue;
					}
				}
				_compliantNodesList.push_back(node);
			}

			// let's simulate selecting at random even when iterating sequentially
			RNG::shuffle(_compliantNodesList);
		}

		// 3. deploy units
		bool success = deployReinforcements(wave);

		// 4. post-processing
		if (success)
		{
			// remember success
			_battleGame->getReinforcementsMemory()[wave.type] += 1;

			if (!wave.briefing.title.empty())
			{
				_customBriefing = wave.briefing;
				_showBriefing = true;
			}
			showAlert = true;
		}
	}
	if (_showBriefing)
	{
		_btnBriefingReinforcements->setVisible(true);
	}

	return showAlert;
}

/**
 * Deploys the reinforcements, according to the alien reinforcements deployment rules.
 * @param wave Pointer to the reinforcements deployment rules.
 */
bool NextTurnState::deployReinforcements(const ReinforcementsData &wave)
{
	const AlienRace* race = _game->getMod()->getAlienRace(_battleGame->getReinforcementsRace(), true);
	const int month = _battleGame->getReinforcementsItemLevel();
	bool success = false;

	for (auto& d : wave.data)
	{
		int quantity;

		switch (_game->getSavedGame()->getDifficulty())
		{
		case DIFF_BEGINNER:
			quantity = d.lowQty;
			break;
		case DIFF_EXPERIENCED:
			quantity = d.medQty > 0 ? d.lowQty + ((d.medQty - d.lowQty) / 2) : d.lowQty;
			break;
		case DIFF_VETERAN:
			quantity = d.medQty > 0 ? d.medQty : d.lowQty + ((d.highQty - d.lowQty) / 2);
			break;
		case DIFF_GENIUS:
			quantity = d.medQty > 0 ? d.medQty + ((d.highQty - d.medQty) / 2) : d.lowQty + ((d.highQty - d.lowQty) / 2);
			break;
		case DIFF_SUPERHUMAN:
		default:
			quantity = d.highQty;
		}

		quantity += RNG::generate(0, d.dQty);
		quantity += RNG::generate(0, d.extraQty);

		for (int i = 0; i < quantity; ++i)
		{
			std::string alienName = d.customUnitType.empty() ? race->getMember(d.alienRank) : d.customUnitType;
			Unit* rule = _game->getMod()->getUnit(alienName, true);
			bool civilian = d.percentageOutsideUfo != 0; // small misuse of an unused attribute ;) pls don't kill me
			BattleUnit* unit = addReinforcement(wave, rule, d.alienRank, civilian);
			size_t itemLevel = (size_t)(_game->getMod()->getAlienItemLevels().at(month).at(RNG::generate(0, 9)));
			if (unit)
			{
				success = true;
				_battleGame->initUnit(unit, itemLevel);
				if (!rule->isLivingWeapon())
				{
					if (d.itemSets.empty())
					{
						throw Exception("Reinforcements generator encountered an error: item set not defined");
					}
					if (itemLevel >= d.itemSets.size())
					{
						itemLevel = d.itemSets.size() - 1;
					}
					for (auto& it : d.itemSets.at(itemLevel).items)
					{
						RuleItem* ruleItem = _game->getMod()->getItem(it);
						if (ruleItem)
						{
							_battleGame->createItemForUnit(ruleItem, unit);
						}
					}
					for (auto& iset : d.extraRandomItems)
					{
						if (iset.items.empty())
							continue;
						auto pick = RNG::generate(0, iset.items.size() - 1);
						RuleItem* ruleItem = _game->getMod()->getItem(iset.items[pick]);
						if (ruleItem)
						{
							_battleGame->createItemForUnit(ruleItem, unit);
						}
					}
				}
			}
		}
	}

	return success;
}

/**
 * Adds a reinforcements unit to the game and places him on a free spawnpoint.
 * @param wave Pointer to the reinforcements deployment rules.
 * @param rules Pointer to the Unit which holds info about the alien.
 * @param alienRank The rank of the alien, used for spawn point search.
 * @param civilian Spawn as a civilian?
 * @return Pointer to the created unit.
 */
BattleUnit* NextTurnState::addReinforcement(const ReinforcementsData &wave, Unit *rules, int alienRank, bool civilian)
{
	BattleUnit* unit = _battleGame->createTempUnit(rules, civilian ? FACTION_NEUTRAL : FACTION_ALIEN_PLAYER);

	// 1. try nodes first
	bool unitPlaced = false;
	if (wave.useSpawnNodes && !_compliantNodesList.empty())
	{
		for (auto node : _compliantNodesList)
		{
			if (_battleGame->setUnitPosition(unit, node->getPosition()))
			{
				unit->getAIModule()->setStartNode(node);
				unit->setRankInt(alienRank);
				unit->setDirection(RNG::generate(0, 7));
				_battleGame->getUnits()->push_back(unit);
				unitPlaced = true;
				break;
			}
		}
	}

	// 2. then try random positions on compliant blocks
	if (!unitPlaced && !_compliantBlocksList.empty())
	{
		auto tmpZList = wave.spawnZLevels;
		if (tmpZList.empty())
		{
			tmpZList.reserve(_battleGame->getMapSizeZ());
			for (int z = 0; z < _battleGame->getMapSizeZ(); ++z)
			{
				tmpZList.push_back(z);
			}
		}
		if (wave.randomizeZLevels)
		{
			RNG::shuffle(tmpZList);
		}
		int tries = 100;
		int randomX = 0;
		int randomY = 0;
		int edgeX = (_battleGame->getMapSizeX() / 10) - 1;
		int edgeY = (_battleGame->getMapSizeY() / 10) - 1;
		while (!unitPlaced && tries)
		{
			int randomBlockIndex = RNG::generate(0, _compliantBlocksList.size() - 1);
			if (wave.maxDistanceFromBorders > 0 && wave.maxDistanceFromBorders < 10)
			{
				// Note: we assume that the modder has done the necessary work and allowed only border blocks to come this far
				if (_compliantBlocksList[randomBlockIndex].x == 0)
				{
					randomX = RNG::generate(0, wave.maxDistanceFromBorders - 1);
				}
				else if (_compliantBlocksList[randomBlockIndex].x == edgeX)
				{
					randomX = RNG::generate(10 - wave.maxDistanceFromBorders, 9);
				}
				else
				{
					randomX = RNG::generate(0, 9); // modder fail
				}
				if (_compliantBlocksList[randomBlockIndex].y == 0)
				{
					randomY = RNG::generate(0, wave.maxDistanceFromBorders - 1);
				}
				else if (_compliantBlocksList[randomBlockIndex].y == edgeY)
				{
					randomY = RNG::generate(10 - wave.maxDistanceFromBorders, 9);
				}
				else
				{
					randomY = RNG::generate(0, 9); // modder fail
				}
			}
			else
			{
				randomX = RNG::generate(0, 9);
				randomY = RNG::generate(0, 9);
			}
			Position randomPos = Position(_compliantBlocksList[randomBlockIndex].x * 10 + randomX, _compliantBlocksList[randomBlockIndex].y * 10 + randomY, 0);

			bool foundXcomUnitNearby = false;
			if (wave.minDistanceFromXcomUnits > 1)
			{
				for (auto xcomUnit : *_battleGame->getUnits())
				{
					if (xcomUnit->getOriginalFaction() == FACTION_PLAYER &&
						!xcomUnit->isOut() &&
						Position::distanceSq(xcomUnit->getPosition(), randomPos) < wave.minDistanceFromXcomUnits * wave.minDistanceFromXcomUnits)
					{
						foundXcomUnitNearby = true;
						break;
					}
				}
			}
			if (!foundXcomUnitNearby)
			{
				for (auto tryZ : tmpZList)
				{
					Position finalPos = randomPos + Position(0, 0, tryZ);
					if (unit->getMovementType() != MT_FLY)
					{
						Tile* t = _battleGame->getTile(finalPos);
						if (t == 0 || t->getTUCost(O_FLOOR, unit->getMovementType()) == Pathfinding::INVALID_MOVE_COST)
						{
							// non-flying units cannot spawn on e.g. a water tile (e.g. in the POLAR terrain)
							continue;
						}
					}
					if (_battleGame->setUnitPosition(unit, finalPos))
					{
						unit->setRankInt(alienRank);
						unit->setDirection(RNG::generate(0, 7));
						_battleGame->getUnits()->push_back(unit);
						unitPlaced = true;
						break;
					}
				}
			}
			--tries;
		}
	}

	// 3. finally just place it somewhere
	if (!unitPlaced && wave.forceSpawnNearFriend && placeReinforcementNearFriend(unit))
	{
		unit->setRankInt(alienRank);
		unit->setDirection(RNG::generate(0, 7));
		_battleGame->getUnits()->push_back(unit);
		unitPlaced = true;
	}

	// 4. dude, seriously?
	if (!unitPlaced)
	{
		delete unit;
		unit = nullptr;
	}

	return unit;
}

/**
 * Places a unit near a friendly unit.
 * @param unit Pointer to the unit in question.
 * @return If we successfully placed the unit.
 */
bool NextTurnState::placeReinforcementNearFriend(BattleUnit *unit)
{
	if (_battleGame->getUnits()->empty())
	{
		return false;
	}
	for (int i = 0; i != 10; ++i)
	{
		Position entryPoint = TileEngine::invalid;
		int tries = 100;
		bool largeUnit = false;
		while (entryPoint == TileEngine::invalid && tries)
		{
			BattleUnit* k = _battleGame->getUnits()->at(RNG::generate(0, _battleGame->getUnits()->size() - 1));
			if (k->getFaction() == unit->getFaction() && k->getPosition() != TileEngine::invalid && k->getArmor()->getSize() >= unit->getArmor()->getSize())
			{
				entryPoint = k->getPosition();
				largeUnit = (k->getArmor()->getSize() != 1);
			}
			--tries;
		}
		if (tries && _battleGame->placeUnitNearPosition(unit, entryPoint, largeUnit))
		{
			return true;
		}
	}
	return false;
}

}
