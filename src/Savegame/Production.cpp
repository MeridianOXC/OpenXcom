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
#include "Production.h"
#include <algorithm>
#include "../Engine/Collections.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleSoldier.h"
#include "Base.h"
#include "SavedGame.h"
#include "Transfer.h"
#include "ItemContainer.h"
#include "Soldier.h"
#include "Craft.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleCraft.h"
#include "../Engine/Language.h"
#include "../Engine/RNG.h"
#include <climits>
#include "BaseFacility.h"

namespace OpenXcom
{
Production::Production(const RuleManufacture * rules, int amount) : _rules(rules), _amount(amount), _infinite(false), _timeSpent(0), _engineers(0), _sell(false)
{
}

int Production::getAmountTotal() const
{
	return _amount;
}

void Production::setAmountTotal (int amount)
{
	_amount = amount;
}

bool Production::getInfiniteAmount() const
{
	return _infinite;
}

void Production::setInfiniteAmount (bool inf)
{
	_infinite = inf;
}

int Production::getTimeSpent() const
{
	return _timeSpent;
}

void Production::setTimeSpent (int done)
{
	_timeSpent = done;
}

int Production::getAssignedEngineers() const
{
	return _engineers;
}

void Production::setAssignedEngineers (int engineers)
{
	_engineers = engineers;
}

bool Production::getSellItems() const
{
	return _sell;
}

void Production::setSellItems (bool sell)
{
	_sell = sell;
}

bool Production::haveEnoughMoneyForOneMoreUnit(SavedGame * g) const
{
	return _rules->haveEnoughMoneyForOneMoreUnit(g->getFunds());
}

bool Production::haveEnoughLivingSpaceForOneMoreUnit(Base * b)
{
	if (_rules->getSpawnedPersonType() != "")
	{
		// Note: if the production is running then the space we need is already counted by getUsedQuarters
		if (b->getAvailableQuarters() < b->getUsedQuarters())
		{
			return false;
		}
	}
	return true;
}

bool Production::haveEnoughMaterialsForOneMoreUnit(Base * b, const Mod *m) const
{
	for (auto& i : _rules->getRequiredItems())
	{
		if (b->getStorageItems()->getItem(i.first) < i.second)
			return false;
	}
	for (auto& i : _rules->getRequiredCrafts())
	{
		if (b->getCraftCountForProduction(i.first) < i.second)
			return false;
	}
	return true;
}

productionProgress_e Production::step(Base * b, SavedGame * g, const Mod *m, Language *lang)
{
	int done = getAmountProduced();
	_timeSpent += _engineers;

	if (done < getAmountProduced())
	{
		int produced;
		if (!getInfiniteAmount())
		{
			produced = std::min(getAmountProduced(), _amount) - done; // std::min is required because we don't want to overproduce
		}
		else
		{
			produced = getAmountProduced() - done;
		}
		int count = 0;
		do
		{
			auto ruleCraft = _rules->getProducedCraft();
			if (ruleCraft)
			{
				Craft *craft = new Craft(ruleCraft, b, g->getId(ruleCraft->getType()));
				craft->initFixedWeapons(m);
				craft->setStatus("STR_REFUELLING");
				b->getCrafts()->push_back(craft);
			}
			else
			{
				for (auto& i : _rules->getProducedItems())
				{
					if (getSellItems())
					{
						int64_t adjustedSellValue = i.first->getSellCost();
						adjustedSellValue = adjustedSellValue * i.second * g->getSellPriceCoefficient() / 100;
						g->setFunds(g->getFunds() + adjustedSellValue);
					}
					else
					{
						b->getStorageItems()->addItem(i.first->getType(), i.second);
						if (!_rules->getRandomProducedItems().empty())
						{
							_randomProductionInfo[i.first->getType()] += i.second;
						}
						if (i.first->getBattleType() == BT_NONE)
						{
							for (std::vector<Craft*>::iterator c = b->getCrafts()->begin(); c != b->getCrafts()->end(); ++c)
							{
								(*c)->reuseItem(i.first);
							}
						}
					}
				}
			}
			// Random manufacture
			if (!_rules->getRandomProducedItems().empty())
			{
				int totalWeight = 0;
				for (auto& itemSet : _rules->getRandomProducedItems())
				{
					totalWeight += itemSet.first;
				}
				// RNG
				int roll = RNG::generate(1, totalWeight);
				int runningTotal = 0;
				for (auto& itemSet : _rules->getRandomProducedItems())
				{
					runningTotal += itemSet.first;
					if (runningTotal >= roll)
					{
						for (auto& i : itemSet.second)
						{
							b->getStorageItems()->addItem(i.first->getType(), i.second);
							_randomProductionInfo[i.first->getType()] += i.second;
							if (i.first->getBattleType() == BT_NONE)
							{
								for (std::vector<Craft*>::iterator c = b->getCrafts()->begin(); c != b->getCrafts()->end(); ++c)
								{
									(*c)->reuseItem(i.first);
								}
							}
						}
						// break outer loop
						break;
					}
				}
			}
			// Spawn persons (soldiers, engineers, scientists, ...)
			const std::string &spawnedPersonType = _rules->getSpawnedPersonType();
			if (spawnedPersonType != "")
			{
				if (spawnedPersonType == "STR_SCIENTIST")
				{
					Transfer *t = new Transfer(24);
					t->setScientists(1);
					b->getTransfers()->push_back(t);
				}
				else if (spawnedPersonType == "STR_ENGINEER")
				{
					Transfer *t = new Transfer(24);
					t->setEngineers(1);
					b->getTransfers()->push_back(t);
				}
				else
				{
					RuleSoldier *rule = m->getSoldier(spawnedPersonType);
					if (rule != 0)
					{
						Transfer *t = new Transfer(24);
						int nationality = g->selectSoldierNationalityByLocation(m, rule, b);
						Soldier *s = m->genSoldier(g, rule, nationality);
						s->load(_rules->getSpawnedSoldierTemplate(), m, g, m->getScriptGlobal(), true); // load from soldier template
						if (_rules->getSpawnedPersonName() != "")
						{
							s->setName(lang->getString(_rules->getSpawnedPersonName()));
						}
						else
						{
							s->genName();
						}
						t->setSoldier(s);
						b->getTransfers()->push_back(t);
					}
				}
			}
			count++;
			if (count < produced)
			{
				// We need to ensure that player has enough cash/item to produce a new unit
				if (!haveEnoughMoneyForOneMoreUnit(g)) return PROGRESS_NOT_ENOUGH_MONEY;
				if (!haveEnoughMaterialsForOneMoreUnit(b, m)) return PROGRESS_NOT_ENOUGH_MATERIALS;
				startItem(b, g, m);
			}
		}
		while (count < produced);
	}
	if (getAmountProduced() >= _amount && !getInfiniteAmount()) return PROGRESS_COMPLETE;
	if (done < getAmountProduced())
	{
		// We need to ensure that player has enough cash/item to produce a new unit
		if (!haveEnoughMoneyForOneMoreUnit(g)) return PROGRESS_NOT_ENOUGH_MONEY;
		if (!haveEnoughLivingSpaceForOneMoreUnit(b)) return PROGRESS_NOT_ENOUGH_LIVING_SPACE;
		if (!haveEnoughMaterialsForOneMoreUnit(b, m)) return PROGRESS_NOT_ENOUGH_MATERIALS;
		startItem(b, g, m);
	}
	return PROGRESS_NOT_COMPLETE;
}

int Production::getAmountProduced() const
{
	if (_rules->getManufactureTime() > 0)
		return _timeSpent / _rules->getManufactureTime();
	else
		return _amount;
}

const RuleManufacture * Production::getRules() const
{
	return _rules;
}

void Production::startItem(Base * b, SavedGame * g, const Mod *m) const
{
	g->setFunds(g->getFunds() - _rules->getManufactureCost());
	for (auto& i : _rules->getRequiredItems())
	{
		b->getStorageItems()->removeItem(i.first, i.second);
	}
	for (auto& i : _rules->getRequiredCrafts())
	{
		// Find suitable craft
		for (std::vector<Craft*>::iterator c = b->getCrafts()->begin(); c != b->getCrafts()->end(); ++c)
		{
			if ((*c)->getRules() == i.first)
			{
				Craft *craft = *c;
				b->removeCraft(craft, true);
				delete craft;
				break;
			}
		}
	}
}

void Production::refundItem(Base * b, SavedGame * g, const Mod *m) const
{
	g->setFunds(g->getFunds() + _rules->getManufactureCost());
	for (auto& iter : _rules->getRequiredItems())
	{
		b->getStorageItems()->addItem(iter.first->getType(), iter.second);
	}
	//for (auto& it : _rules->getRequiredCrafts())
	//{
	//	// not supported
	//}
}

YAML::Node Production::save() const
{
	YAML::Node node;
	node["item"] = getRules()->getName();
	node["assigned"] = getAssignedEngineers();
	node["spent"] = getTimeSpent();
	node["amount"] = getAmountTotal();
	node["infinite"] = getInfiniteAmount();
	if (getSellItems())
		node["sell"] = getSellItems();
	if (!_rules->getRandomProducedItems().empty())
	{
		node["randomProductionInfo"] = _randomProductionInfo;
	}
	return node;
}

void Production::load(const YAML::Node &node)
{
	setAssignedEngineers(node["assigned"].as<int>(getAssignedEngineers()));
	setTimeSpent(node["spent"].as<int>(getTimeSpent()));
	setAmountTotal(node["amount"].as<int>(getAmountTotal()));
	setInfiniteAmount(node["infinite"].as<bool>(getInfiniteAmount()));
	setSellItems(node["sell"].as<bool>(getSellItems()));
	if (!_rules->getRandomProducedItems().empty())
	{
		_randomProductionInfo = node["randomProductionInfo"].as< std::map<std::string, int> >(_randomProductionInfo);
	}
	// backwards compatibility
	if (getAmountTotal() == INT_MAX)
	{
		setAmountTotal(999);
		setInfiniteAmount(true);
		setSellItems(true);
	}
}

}
