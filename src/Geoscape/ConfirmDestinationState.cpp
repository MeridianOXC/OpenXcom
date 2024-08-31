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
#include "CraftErrorState.h"
#include "CraftNotEnoughPilotsState.h"
#include "ConfirmDestinationState.h"
#include "../fmath.h"
#include "../Engine/Game.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/Mod.h"
#include "../Mod/AlienRace.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleStartingCondition.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/ArticleDefinition.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Target.h"
#include "../Savegame/Waypoint.h"
#include "../Savegame/Base.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/Soldier.h"
#include "../Engine/Options.h"
#include "../Engine/Sound.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Destination window.
 * @param game Pointer to the core game.
 * @param crafts Vector for the crafts part of a wing to retarget.
 * @param target Pointer to the selected target (NULL if it's just a point on the globe).
 */
ConfirmDestinationState::ConfirmDestinationState(std::vector<Craft*> crafts, Target *target) : _crafts(std::move(crafts)), _target(target)
{
	Waypoint *w = dynamic_cast<Waypoint*>(_target);
	_screen = false;

	Base *base = dynamic_cast<Base*>(_target);
	bool transferAvailable = false; // no transfer allowed for wings

	if (_crafts.size() == 1)
	{
		transferAvailable = (Options::canTransferCraftsWhileAirborne && base != 0 && base != _crafts.front()->getBase() && _crafts.front()->arePilotsOnboard());
	}

	int btnOkX = transferAvailable ? 29 : 68;
	int btnCancelX = transferAvailable ? 177 : 138;

	// Create objects
	_window = new Window(this, 244, 72, 6, 64);
	_btnOk = new TextButton(50, 12, btnOkX, 104);
	_btnTransfer = new TextButton(82, 12, 87, 104);
	_btnCancel = new TextButton(50, 12, btnCancelX, 104);
	_btnFollowWingLeader = new ToggleTextButton(170, 16, 43, 138);
	_txtTarget = new Text(232, 32, 12, 72);
	_txtETA = new Text(232, 9, 12, 120);

	// Set palette
	setInterface("confirmDestination", w != 0 && w->getId() == 0);

	add(_window, "window", "confirmDestination");
	add(_btnOk, "button", "confirmDestination");
	add(_btnCancel, "button", "confirmDestination");
	add(_btnTransfer, "button", "confirmDestination");
	add(_btnFollowWingLeader, "button", "confirmDestination");
	add(_txtTarget, "text", "confirmDestination");
	add(_txtETA, "text", "confirmDestination");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "confirmDestination");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ConfirmDestinationState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ConfirmDestinationState::btnOkClick, Options::keyOk);

	_btnTransfer->setText(tr("STR_TRANSFER_UC"));
	_btnTransfer->onMouseClick((ActionHandler)&ConfirmDestinationState::btnTransferClick);
	_btnTransfer->setVisible(transferAvailable);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&ConfirmDestinationState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&ConfirmDestinationState::btnCancelClick, Options::keyCancel);

	_btnFollowWingLeader->setText(tr("STR_FOLLOW_WING_LEADER_QUESTION"));
	_btnFollowWingLeader->setVisible(false);

	if (_crafts.size() > 1)
	{
		_btnFollowWingLeader->setVisible(true);

		Ufo* u = dynamic_cast<Ufo*>(_target);
		if (u && u->getStatus() == Ufo::FLYING)
		{
			_btnFollowWingLeader->setPressed(false); // everybody go for the UFO as quickly as possible
		}
		else
		{
			_btnFollowWingLeader->setPressed(true); // follow the wing leader please
		}
	}

	_txtTarget->setBig();
	_txtTarget->setAlign(ALIGN_CENTER);
	_txtTarget->setVerticalAlign(ALIGN_MIDDLE);
	_txtTarget->setWordWrap(true);
	if (w != 0 && w->getId() == 0)
	{
		_txtTarget->setText(tr("STR_TARGET").arg(tr("STR_WAY_POINT")));
	}
	else
	{
		_txtTarget->setText(tr("STR_TARGET").arg(_target->getName(_game->getLanguage())));
	}

	// ETA display
	if (Options::oxceShowETAMode > 0 && _target)
	{
		MovingTarget* mt = dynamic_cast<MovingTarget*>(_target);
		if (Options::oxceShowETAMode == 1 && mt && mt->getSpeed() > 0)
		{
			// don't show ETA for moving targets (i.e. UFOs and crafts)
		}
		else
		{
			int speed = _crafts.front()->getCraftStats().speedMax;
			int distance = XcomDistance(_crafts.front()->getDistance(_target));
			int etaInHoursHelper = (distance + (speed / 2)) / speed;
			int days = etaInHoursHelper / 24;
			int hours = etaInHoursHelper % 24;
			std::ostringstream ssStatus;
			if (days > 0) ssStatus << tr("STR_DAY_SHORT").arg(days);
			if (hours > 0 || days == 0)
			{
				if (days > 0) ssStatus << "/";
				ssStatus << tr("STR_HOUR_SHORT").arg(hours);
			}
			_txtETA->setAlign(ALIGN_CENTER);
			_txtETA->setText(tr("STR_ETA").arg(ssStatus.str()));
		}
	}
}

/**
 *
 */
ConfirmDestinationState::~ConfirmDestinationState()
{

}

/**
* Checks the starting condition.
*/
std::string ConfirmDestinationState::checkStartingCondition()
{
	Ufo* u = dynamic_cast<Ufo*>(_target);
	MissionSite* m = dynamic_cast<MissionSite*>(_target);
	AlienBase* b = dynamic_cast<AlienBase*>(_target);

	AlienDeployment *ruleDeploy = 0;
	if (u != 0)
	{
		ruleDeploy = _game->getMod()->getDeployment(u->getRules()->getType()); // no need to check for fake underwater UFOs here
	}
	else if (m != 0)
	{
		ruleDeploy = _game->getMod()->getDeployment(m->getDeployment()->getType());
	}
	else if (b != 0)
	{
		AlienRace *race = _game->getMod()->getAlienRace(b->getAlienRace());
		ruleDeploy = _game->getMod()->getDeployment(race->getBaseCustomMission());
		if (!ruleDeploy) ruleDeploy = _game->getMod()->getDeployment(b->getDeployment()->getType());
	}
	else
	{
		// for example just a waypoint
		return "";
	}

	if (ruleDeploy == 0)
	{
		// e.g. UFOs without alien deployment :(
		return "";
	}

	RuleStartingCondition *rule = _game->getMod()->getStartingCondition(ruleDeploy->getStartingCondition());
	if (rule == 0)
	{
		// rule doesn't exist (mod upgrades?)
		return "";
	}
	if (rule->requiresCommanderOnboard() && !_crafts.front()->isCommanderOnboard())
	{
		if (!u || u->getStatus() == Ufo::LANDED || u->getStatus() == Ufo::CRASHED)
		{
			return tr("STR_STARTING_CONDITION_COMMANDER");
		}
	}

	// Only check first selected craft
	// The other crafts will follow the first selected craft so they will not land at the mission site
	// This check is only performed once per wing
	// Checking if _crafts.size() != 1 will disallow the escorting scenario

	// check required item(s)
	auto& requiredItems = rule->getRequiredItems();
	if (!_crafts.front()->areRequiredItemsOnboard(requiredItems))
	{
		std::ostringstream ss2;
		int i2 = 0;
		for (auto& pair : requiredItems)
		{
			if (i2 > 0)
				ss2 << ", ";
			ss2 << tr(pair.first) << ": " << pair.second;
			i2++;
		}
		std::string argument2 = ss2.str();
		return tr("STR_STARTING_CONDITION_ITEM").arg(argument2);
	}

	// check permitted soldiers
	if (!_crafts.front()->areOnlyPermittedSoldierTypesOnboard(rule))
	{
		auto& list = rule->getForbiddenSoldierTypes().empty() ? rule->getAllowedSoldierTypes() : rule->getForbiddenSoldierTypes();
		const std::string messageCode = rule->getForbiddenSoldierTypes().empty() ? "STR_STARTING_CONDITION_SOLDIER_TYPE_ALLOWED" : "STR_STARTING_CONDITION_SOLDIER_TYPE_FORBIDDEN";

		std::ostringstream ss;
		int i = 0;
		for (auto& soldierType : list)
		{
			RuleSoldier* soldierTypeRule = _game->getMod()->getSoldier(soldierType, false);
			if (soldierTypeRule && _game->getSavedGame()->isResearched(soldierTypeRule->getRequirements()))
			{
				if (i > 0)
					ss << ", ";
				ss << tr(soldierType);
				i++;
			}
		}
		std::string argument = ss.str();
		if (argument.empty())
		{
			// no suitable soldier type yet?
			argument = tr("STR_UNKNOWN");
		}
		return tr(messageCode).arg(argument);
	}

	if (rule->isCraftPermitted(_crafts.front()->getRules()->getType()))
	{
		// craft is permitted
		return "";
	}

	// craft is not permitted (= either forbidden or not allowed)
	auto& list = rule->getForbiddenCraft().empty() ? rule->getAllowedCraft() : rule->getForbiddenCraft();
	const std::string messageCode = rule->getForbiddenCraft().empty() ? "STR_STARTING_CONDITION_CRAFT_ALLOWED" : "STR_STARTING_CONDITION_CRAFT_FORBIDDEN";

	std::ostringstream ss;
	int i = 0;
	for (auto& articleName : list)
	{
		ArticleDefinition *article = _game->getMod()->getUfopaediaArticle(articleName, false);
		if (article && Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
		{
			if (i > 0)
				ss << ", ";
			ss << tr(articleName);
			i++;
		}
	}
	std::string argument = ss.str();
	if (argument.empty())
	{
		// no suitable craft yet
		argument = tr("STR_UNKNOWN");
	}
	return tr(messageCode).arg(argument);
}

/**
 * Confirms the selected target for the craft.
 * @param action Pointer to an action.
 */
void ConfirmDestinationState::btnOkClick(Action *)
{
	std::string message = checkStartingCondition();
	if (!message.empty())
	{
		_game->popState();
		_game->popState();
		_game->pushState(new CraftErrorState(0, message));
		return;
	}

	for (auto* craft : _crafts)
	{
		if (!craft->arePilotsOnboard())
		{
			_game->popState();
			_game->popState();
			_game->pushState(new CraftNotEnoughPilotsState(craft));
			return;
		}
	}

	Waypoint *w = dynamic_cast<Waypoint*>(_target);
	if (w != 0 && w->getId() == 0)
	{
		w->setId(_game->getSavedGame()->getId("STR_WAY_POINT"));
		_game->getSavedGame()->getWaypoints()->push_back(w);
	}

	// first selected _craft (first shift-clicked craft) is wing leader; the other crafts follow the wing leader
	if (_crafts.front() == _target)
	{
		//setting itself as target works fine but it should be saying "patrolling" instead
		_crafts.front()->setDestination(0);
	}
	else
	{
		_crafts.front()->setDestination(_target);
		if (_crafts.front()->isTakingOff())
		{
			if (!_crafts.front()->getRules()->getTakeoffSoundRaw().empty())
			{
				_game->getMod()->getSound("GEO.CAT", _crafts.front()->getRules()->getTakeoffSound())->play();
			}
		}
	}

	for (auto* craft : _crafts)
	{
		if (craft != _crafts.front())
		{
			if (_btnFollowWingLeader->getPressed())
			{
				craft->setDestination(_crafts.front()); // follow the wing leader
			}
			else
			{
				craft->setDestination(_target); // go for the same target as the wing leader
			}
		}

		if (craft->getRules()->canAutoPatrol())
		{
			// cancel auto-patrol
			craft->setIsAutoPatrolling(false);
		}

		craft->setStatus("STR_OUT");
	}

	_game->popState();
	_game->popState();
}

/**
 * Handles clicking the transfer button
 * Performs a transfer of a craft to a targeted base if possible, otherwise pops an error message
 * @param action Pointer to an action.
 */
void ConfirmDestinationState::btnTransferClick(Action *)
{
	std::string errorMessage;
	
	Base *targetBase = dynamic_cast<Base*>(_target);
	if ((targetBase->getAvailableHangars() - targetBase->getUsedHangars()) <= 0) // don't know how you'd get less than 0 available hangars, but want to handle that just in case
	{
		errorMessage = tr("STR_NO_FREE_HANGARS_FOR_TRANSFER");
	}
	else if (_crafts.front()->getNumTotalSoldiers() > targetBase->getAvailableQuarters() - targetBase->getUsedQuarters())
	{
		errorMessage = tr("STR_NO_FREE_ACCOMODATION_CREW");
	}
	else if (Options::storageLimitsEnforced && targetBase->storesOverfull(_crafts.front()->getTotalItemStorageSize(_game->getMod())))
	{
		errorMessage = tr("STR_NOT_ENOUGH_STORE_SPACE_FOR_CRAFT");
	}
	else if (_crafts.front()->getFuel() < _crafts.front()->getFuelLimit(targetBase))
	{
		errorMessage = tr("STR_NOT_ENOUGH_FUEL_TO_REACH_TARGET");
	}

	// clicking transfer will start the craft moving or make us need to pick a new destination
	// either way, we need to get rid of this confirming the destination state
	_game->popState();
	if (errorMessage.empty())
	{
		// Transfer soldiers inside craft
		Base *currentBase = _crafts.front()->getBase();
		for (auto soldierIt = currentBase->getSoldiers()->begin(); soldierIt != currentBase->getSoldiers()->end();)
		{
			Soldier* soldier = (*soldierIt);
			if (soldier->getCraft() == _crafts.front())
			{
				soldier->setPsiTraining(false);
				if (soldier->isInTraining())
				{
					soldier->setReturnToTrainingWhenHealed(true);
				}
				soldier->setTraining(false);
				targetBase->getSoldiers()->push_back(soldier);
				soldierIt = currentBase->getSoldiers()->erase(soldierIt);
			}
			else
			{
				++soldierIt;
			}
		}

		// Transfer craft
		currentBase->removeCraft(_crafts.front(), false);
		targetBase->getCrafts()->push_back(_crafts.front());
		_crafts.front()->setBase(targetBase, false);
		_crafts.front()->returnToBase();
		_crafts.front()->setStatus("STR_OUT");
		if (_crafts.front()->getFuel() <= _crafts.front()->getFuelLimit(targetBase))
		{
			_crafts.front()->setLowFuel(true);
		}

		// pop the selecting the destination state
		_game->popState();
	}
	else
	{
		RuleInterface *menuInterface = _game->getMod()->getInterface("errorMessages");
		_game->pushState(new ErrorMessageState(errorMessage, _palette, menuInterface->getElement("geoscapeColor")->color, "BACK13.SCR", menuInterface->getElement("geoscapePalette")->color));
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ConfirmDestinationState::btnCancelClick(Action *)
{
	Waypoint *w = dynamic_cast<Waypoint*>(_target);
	if (w != 0 && w->getId() == 0)
	{
		delete w;
	}
	_game->popState();
}

}
