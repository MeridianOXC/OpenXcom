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
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include <climits>
#include "BaseFacility.h"

namespace OpenXcom
{
Production::Production(const RuleManufacture * rules, int amount) : _rules(rules), _amount(amount), _infinite(false), _timeSpent(0), _engineers(0), _sell(false)
{
	_efficiency = 100;
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

int Production::getProgress(Base* b, SavedGame* g, const Mod* m, int loyaltyRating)
{
	if (!m->getIsFTAGame())
	{
		return _engineers;
	}
	else
	{
		int progress = 0;
		std::vector<Soldier*> assignedEngineers;
		for (auto s : *b->getSoldiers())
		{
			if (s->getProductionProject() == this)
			{
				assignedEngineers.push_back(s);
			}
		}

		if (assignedEngineers.size() > 0)
		{
			double effort = 0;
			auto projStats = _rules->getStats();
			int factor = m->getEngineerTrainingFactor();
			int summEfficiency = 0;
			for (auto s : assignedEngineers)
			{
				auto stats = s->getStatsWithAllBonuses();
				auto caps = s->getRules()->getStatCaps();
				unsigned int statsN = 0;
				Log(LOG_INFO) << "Soldier " << s->getName() << " is calculating his/her effort for the manufacturing project";

				if (projStats.weaponry > 0)
				{
					effort += (stats->weaponry * projStats.weaponry) / (10000);
					if (stats->weaponry < caps.weaponry
						&& RNG::generate(0, caps.weaponry) > stats->weaponry
						&& RNG::percent(factor * (projStats.weaponry / 100)))
					{
						s->getEngineerExperience()->weaponry++;
					}
					statsN++;
				}

				if (projStats.explosives > 0)
				{
					effort += (stats->explosives * projStats.explosives) / (10000);
					if (stats->explosives < caps.explosives
						&& RNG::generate(0, caps.explosives) > stats->explosives
						&& RNG::percent(factor * (projStats.explosives / 100)))
					{
						s->getEngineerExperience()->explosives++;
					}
					statsN++;
				}

				if (projStats.microelectronics > 0)
				{
					effort += (stats->microelectronics * projStats.microelectronics) / (10000);
					if (stats->microelectronics < caps.microelectronics
						&& RNG::generate(0, caps.microelectronics) > stats->microelectronics
						&& RNG::percent(factor * (projStats.microelectronics / 100)))
					{
						s->getEngineerExperience()->microelectronics++;
					}
					statsN++;
				}

				if (projStats.metallurgy > 0)
				{
					effort += (stats->metallurgy * projStats.metallurgy) / (10000);
					if (stats->metallurgy < caps.metallurgy
						&& RNG::generate(0, caps.metallurgy) > stats->metallurgy
						&& RNG::percent(factor * (projStats.metallurgy / 100)))
					{
						s->getEngineerExperience()->metallurgy++;
					}
					statsN++;
				}

				if (projStats.processing > 0)
				{
					effort += (stats->processing * projStats.processing) / (10000);
					if (stats->processing < caps.processing
						&& RNG::generate(0, caps.processing) > stats->processing
						&& RNG::percent(factor * (projStats.processing / 100)))
					{
						s->getEngineerExperience()->processing++;
					}
					statsN++;
				}

				if (projStats.robotics > 0)
				{
					effort += (stats->robotics * projStats.robotics) / (10000);
					if (stats->robotics < caps.robotics
						&& RNG::generate(0, caps.robotics) > stats->robotics
						&& RNG::percent(factor * (projStats.robotics / 100)))
					{
						s->getEngineerExperience()->robotics++;
					}
					statsN++;
				}

				if (projStats.hacking > 0)
				{
					effort += (stats->hacking * projStats.hacking) / (10000);
					if (stats->hacking < caps.hacking
						&& RNG::generate(0, caps.hacking) > stats->hacking
						&& RNG::percent(factor * (projStats.hacking / 100)))
					{
						s->getEngineerExperience()->hacking++;
					}
					statsN++;
				}

				if (projStats.construction > 0)
				{
					effort += (stats->construction * projStats.construction) / (10000);
					if (stats->construction < caps.construction
						&& RNG::generate(0, caps.construction) > stats->construction
						&& RNG::percent(factor * (projStats.construction / 100)))
					{
						s->getEngineerExperience()->construction++;
					}
					statsN++;
				}

				if (projStats.alienTech > 0)
				{
					effort += (stats->alienTech * projStats.alienTech) / (10000);
					if (stats->alienTech < caps.alienTech
						&& RNG::generate(0, caps.alienTech) > stats->alienTech
						&& RNG::percent(factor * (projStats.alienTech / 100)))
					{
						s->getEngineerExperience()->alienTech++;
					}
					statsN++;
				}

				if (projStats.reverseEngineering > 0)
				{
					effort += (stats->reverseEngineering * projStats.reverseEngineering) / (10000);
					if (stats->reverseEngineering < caps.reverseEngineering
						&& RNG::generate(0, caps.reverseEngineering) > stats->reverseEngineering
						&& RNG::percent(factor * (projStats.reverseEngineering / 100)))
					{
						s->getEngineerExperience()->reverseEngineering++;
					}
					statsN++;
				}

				Log(LOG_INFO) << "Raw effort equals: " << effort;

				int deliganceFactor = ceil((stats->diligence * 0.5) - 25);

				effort = (effort * deliganceFactor) / 100;

				Log(LOG_INFO) << "Effort with diligence bonus: " << effort;

				if (statsN > 0)
				{
					effort /= statsN;
				}

				Log(LOG_INFO) << "Final effort value: " << effort;

				summEfficiency += stats->efficiency;
			}

			_efficiency = summEfficiency / assignedEngineers.size();

			// If one woman can carry a baby in nine months, nine women can't do it in a month...
			effort *= 100 - (19 * log(assignedEngineers.size()));
			Log(LOG_INFO) << "Progress after correction for size: " << effort;
			effort = (effort * loyaltyRating) / 100;
			progress = static_cast<int>(ceil(effort));
			Log(LOG_INFO) << " >>> Total hourly progress for manufacturing project " << _rules->getName() << ": " << progress;
		}
		else
		{
			Log(LOG_INFO) << " >>> No assigned engineers for project: " << _rules->getName();
			_efficiency = 100;
		}

		return progress;
	}
}

productionProgress_e Production::step(Base * b, SavedGame * g, const Mod *m, Language *lang, int rating)
{
	int done = getAmountProduced();
	_timeSpent += getProgress(b, g, m, rating);

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
						Soldier *s = m->genSoldier(g, rule->getType());
						if (_rules->getSpawnedPersonName() != "")
						{
							s->setName(lang->getString(_rules->getSpawnedPersonName()));
						}
						s->load(_rules->getSpawnedSoldierTemplate(), m, g, m->getScriptGlobal(), true); // load from soldier template
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
