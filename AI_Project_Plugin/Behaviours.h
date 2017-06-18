#pragma once

#include "Blackboard.h"
#include "BehaviourTree.h"
#include "HelperStructs.h"
#include "SteeringBehaviours.h"

#include <Box2D\Box2D.h>

// Misc
inline bool NearestEnemyInFOV(std::vector<Enemy>* enemies, AgentInfo* pAgentInfo, Enemy& nearestEnemy, float& dist)
{
	if (enemies->empty()) return false;

	float smallestDistance = FLT_MAX;
	auto closestIter = enemies->end();
	for (auto iter = enemies->begin(); iter != enemies->end(); ++iter)
	{
		float dist = b2Distance(iter->Position, pAgentInfo->Position);
		if (iter->InFieldOfView && dist < smallestDistance)
		{
			smallestDistance = dist;
			closestIter = iter;
		}
	}

	dist = smallestDistance;

	if (closestIter != enemies->end())
	{
		nearestEnemy = *closestIter;
		return true;
	}

	return false;
}

inline bool MapSearchedEntirely(Blackboard* pBlackboard)
{
	std::vector<b2Vec2>* searchPoints;
	int searchPointIndex;
	bool dataAvailable =
		pBlackboard->GetData("SearchPoints", searchPoints) &&
		pBlackboard->GetData("SearchPointIndex", searchPointIndex);

	if (!dataAvailable)
		return false;

	return (searchPointIndex == searchPoints->size());
}

inline bool ArrivedAtNextSearchPoint(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<b2Vec2>* searchPoints;
	int searchPointIndex;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("SearchPoints", searchPoints) &&
		pBlackboard->GetData("SearchPointIndex", searchPointIndex);

	if (!dataAvailable || !pAgentInfo)
		return false;

	float dist = b2Distance(pAgentInfo->Position, searchPoints->at(searchPointIndex));

	if (dist < pAgentInfo->GrabRange * 4.0f) // Agent just has to get somewhat close 
	{
		return true;
	}

	return false;
}

inline BehaviourState SetGoalToNextSearchPoint(Blackboard* pBlackboard)
{
	SteeringParams previousGoal;
	std::vector<b2Vec2>* searchPoints;
	int searchPointIndex;
	bool dataAvailable =
		pBlackboard->GetData("Goal", previousGoal) &&
		pBlackboard->GetData("SearchPoints", searchPoints) &&
		pBlackboard->GetData("SearchPointIndex", searchPointIndex);

	if (!dataAvailable || searchPointIndex == searchPoints->size())
		return Failure;

	SteeringParams goal;
	goal.Position = searchPoints->at(searchPointIndex);
	if (previousGoal.Position != goal.Position)
	{
		printf("Set goal back to search point index: %i/%i\n", searchPointIndex, searchPoints->size());
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Failure;
}

inline BehaviourState IncrementSearchPoint(Blackboard* pBlackboard)
{
	std::vector<b2Vec2>* searchPoints;
	int searchPointIndex;
	bool dataAvailable =
		pBlackboard->GetData("SearchPoints", searchPoints) &&
		pBlackboard->GetData("SearchPointIndex", searchPointIndex);

	if (!dataAvailable)
		return Failure;

	int newSearchPointIndex = (searchPointIndex + 1);
	pBlackboard->ChangeData("SearchPointIndex", newSearchPointIndex);

	if (newSearchPointIndex < (int)searchPoints->size())
	{
		printf("Set new search point index: %i/%i\n", newSearchPointIndex, searchPoints->size());
		SteeringParams goal;
		goal.Position = searchPoints->at(newSearchPointIndex);
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Failure;
}

inline bool IsGoalSet(Blackboard* pBlackboard)
{
	bool goalSet;
	bool dataAvailable = pBlackboard->GetData("GoalSet", goalSet);

	if (!dataAvailable)
		return false;

	return goalSet;
}

inline bool HasReachedGoal(Blackboard* pBlackboard)
{
	SteeringParams goal;
	AgentInfo* pAgentInfo = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("Goal", goal) &&
		pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable || !pAgentInfo)
		return false;

	float dist = b2Distance(goal.Position, pAgentInfo->Position);

	return dist < pAgentInfo->GrabRange * 0.75f;
}

inline BehaviourState SetGoalSetFalse(Blackboard* pBlackboard)
{
	pBlackboard->ChangeData("GoalSet", false);
	return Failure; // Continue doing other behaviours
}

inline bool HaveInventorySpace(Blackboard* pBlackboard)
{
	std::vector<Item>* inventory = nullptr;
	float maxHealth = 0;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable)
		return false;

	for (size_t i = 0; i < inventory->size(); i++)
	{
		if (!inventory->at(i).Valid)
		{
			return true;
		}
	}

	return false;
}

inline bool KnowOfItemsOnGround(Blackboard* pBlackboard)
{
	std::vector<EntityInfo>* knownItems = nullptr;
	std::vector<HealthPack>* knownHealthPacks = nullptr;
	std::vector<Food>* knownFoodItems = nullptr;
	std::vector<Pistol>* knownPistols = nullptr;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("KnownItems", knownItems) &&
		pBlackboard->GetData("KnownHealthPacks", knownHealthPacks) &&
		pBlackboard->GetData("KnownFoodItems", knownFoodItems) &&
		pBlackboard->GetData("KnownPistols", knownPistols);

	if (!dataAvailable)
		return false;

	return (!knownItems->empty() ||
			!knownHealthPacks->empty() || 
			!knownFoodItems->empty() ||
			!knownPistols->empty());
}

inline BehaviourState SetNearestItemAsGoal(Blackboard* pBlackboard)
{
	SteeringParams previousGoal;
	std::vector<Item>* inventory;
	std::vector<EntityInfo>* knownItems = nullptr;
	std::vector<HealthPack>* knownHealthPacks = nullptr;
	std::vector<Food>* knownFoodItems = nullptr;
	std::vector<Pistol>* knownPistols = nullptr;
	AgentInfo* pAgentInfo = nullptr;
	float maxHealth = 0;
	float maxEnergy = 0;
	bool dataAvailable =
		pBlackboard->GetData("Goal", previousGoal) &&
		pBlackboard->GetData("Inventory", inventory) &&
		pBlackboard->GetData("KnownItems", knownItems) &&
		pBlackboard->GetData("KnownHealthPacks", knownHealthPacks) &&
		pBlackboard->GetData("KnownFoodItems", knownFoodItems) &&
		pBlackboard->GetData("KnownPistols", knownPistols) &&
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("MaxHealth", maxHealth) &&
		pBlackboard->GetData("MaxEnergy", maxEnergy);


	if (!dataAvailable || !pAgentInfo)
		return Failure;

	float neededHealth = maxHealth - pAgentInfo->Health;
	float neededEnergy = maxEnergy - pAgentInfo->Energy;

	float nearestHealthPackDist = FLT_MAX;
	int nearestHealthPackIndex = -1;
	for (size_t i = 0; i < knownHealthPacks->size(); i++)
	{
		const float dist = b2Distance(knownHealthPacks->at(i).Position, pAgentInfo->Position);
		if (dist < nearestHealthPackDist)
		{
			nearestHealthPackDist = dist;
			nearestHealthPackIndex = i;
		}
	}

	float nearestFoodItemDist = FLT_MAX;
	int nearestFoodItemIndex = -1;
	for (size_t i = 0; i < knownFoodItems->size(); i++)
	{
		const float dist = b2Distance(knownFoodItems->at(i).Position, pAgentInfo->Position);
		if (dist < nearestFoodItemDist)
		{
			nearestFoodItemDist = dist;
			nearestFoodItemIndex = i;
		}
	}

	float nearestPistolDist = FLT_MAX;
	int nearestPistolIndex = -1;
	for (size_t i = 0; i < knownPistols->size(); i++)
	{
		const float dist = b2Distance(knownPistols->at(i).Position, pAgentInfo->Position);
		if (dist < nearestPistolDist)
		{
			nearestPistolDist = dist;
			nearestPistolIndex = i;
		}
	}

	// The items we haven't gotten close enough to to find out what they are
	float nearestItemDist = FLT_MAX;
	int nearestItemIndex = -1;
	for (size_t i = 0; i < knownItems->size(); i++)
	{
		const float dist = b2Distance(knownItems->at(i).Position, pAgentInfo->Position);
		if (dist < nearestItemDist)
		{
			nearestItemDist = dist;
			nearestItemIndex = i;
		}
	}

	// Prioritize FOOD
	if (nearestFoodItemIndex != -1)
	{
		Food nearestFoodItem = knownFoodItems->at(nearestFoodItemIndex);
		if (nearestFoodItem.EnergyAmount >= neededEnergy ||
			nearestPistolIndex == -1)
		{
			SteeringParams goal = {};
			goal.Position = nearestFoodItem.Position;
			if (previousGoal.Position != goal.Position)
			{
				printf("Set goal of food\n");
				pBlackboard->ChangeData("Goal", goal);
				pBlackboard->ChangeData("GoalSet", true);
			}
			return Success;
		}
	}
	// Then HEALTH
	if (nearestHealthPackIndex != -1)
	{
		HealthPack nearestHealthPack = knownHealthPacks->at(nearestHealthPackIndex);
		if (neededHealth >= nearestHealthPack.HealingAmount || 
			(nearestFoodItemIndex == -1 && nearestPistolIndex == -1))
		{
			SteeringParams goal = {};
			goal.Position = nearestHealthPack.Position;
			if (previousGoal.Position != goal.Position)
			{
				printf("Set goal of health\n");
				pBlackboard->ChangeData("Goal", goal);
				pBlackboard->ChangeData("GoalSet", true);
			}
			return Success;
		}
	}

	// Then PISTOLS
	if (nearestPistolIndex != -1)
	{
		Pistol nearestPistol = knownPistols->at(nearestFoodItemIndex);
		SteeringParams goal = {};
		goal.Position = nearestPistol.Position;
		if (previousGoal.Position != goal.Position)
		{
			printf("Set goal of pistol\n");
			pBlackboard->ChangeData("Goal", goal);
			pBlackboard->ChangeData("GoalSet", true);
		}
		return Success;
	}

	// Then ITEMS we haven't inspected yet (could be garbage)
	if (nearestItemIndex != -1)
	{
		EntityInfo nearestItem = knownItems->at(nearestItemIndex);
		SteeringParams goal = {};
		goal.Position = nearestItem.Position;
		if (previousGoal.Position != goal.Position)
		{
			printf("Set goal of nearest item!\n");
			pBlackboard->ChangeData("Goal", goal);
			pBlackboard->ChangeData("GoalSet", true);
		}
		return Success;
	}

	return Failure;
}

inline BehaviourState SetGoalToNearestHouse(Blackboard* pBlackboard)
{
	SteeringParams previousGoal;
	AgentInfo* pAgentInfo = nullptr;
	float secondsBetweenVisits;
	std::vector<House>* pKnownHouses = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("Goal", previousGoal) &&
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("SecondsBetweenHouseRevisits", secondsBetweenVisits) &&
		pBlackboard->GetData("KnownHouses", pKnownHouses);

	if (!dataAvailable || !pAgentInfo || !pKnownHouses)
		return Failure;

	int closestHouseIndex = -1;
	float closestHouseDistSqr = FLT_MAX;
	size_t numKnownHouses = pKnownHouses->size();
	for (size_t i = 0; i < numKnownHouses; i++)
	{
		House house = pKnownHouses->at(i);

		if (house.SecondsSinceLastVisit > secondsBetweenVisits)
		{
			float distSqr = b2DistanceSquared(house.Info.Center, pAgentInfo->Position);
			if (distSqr < closestHouseDistSqr)
			{
				closestHouseDistSqr = distSqr;
				closestHouseIndex = i;
			}
		}
	}

	if (closestHouseIndex == -1 && pKnownHouses->size() > 1)
	{
		// No houses that we haven't visited recently
		// Just keep searching new houses
		return SetGoalToNextSearchPoint(pBlackboard);
	}

	if (closestHouseIndex != -1)
	{
		SteeringParams goal;
		goal.Position = pKnownHouses->at(closestHouseIndex).Info.Center;
		if (goal.Position != previousGoal.Position)
		{
			printf("Set goal of nearest house!\n");
			pBlackboard->ChangeData("Goal", goal);
			pBlackboard->ChangeData("GoalSet", true);
		}
		return Success;
	}


	return Failure;
}

inline BehaviourState SetGoalToNearestUnexploredHouse(Blackboard* pBlackboard)
{
	SteeringParams previousGoal;
	AgentInfo* pAgentInfo = nullptr;
	std::vector<House>* pKnownHouses = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("Goal", previousGoal) &&
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("KnownHouses", pKnownHouses);

	if (!dataAvailable || !pAgentInfo || !pKnownHouses)
		return Failure;

	int closestHouseIndex = -1;
	float closestHouseDistSqr = FLT_MAX;
	size_t numKnownHouses = pKnownHouses->size();
	for (size_t i = 0; i < numKnownHouses; i++)
	{
		House house = pKnownHouses->at(i);

		if (house.Unexplored)
		{
			float distSqr = b2DistanceSquared(house.Info.Center, pAgentInfo->Position);
			if (distSqr < closestHouseDistSqr)
			{
				closestHouseDistSqr = distSqr;
				closestHouseIndex = i;
			}
		}
	}

	if (closestHouseIndex != -1)
	{
		SteeringParams goal;
		goal.Position = pKnownHouses->at(closestHouseIndex).Info.Center;
		if (goal.Position != previousGoal.Position)
		{
			printf("Set goal of nearest unexplored house!\n");
			pBlackboard->ChangeData("Goal", goal);
			pBlackboard->ChangeData("GoalSet", true);
		}
		return Success;
	}

	return Failure;
}

inline bool KnowOfUnexploredHouse(Blackboard* pBlackboard)
{
	std::vector<House>* knownHouses = nullptr;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("KnownHouses", knownHouses);

	if (!dataAvailable || knownHouses->empty())
		return false;

	for (size_t i = 0; i < knownHouses->size(); i++)
	{
		if (knownHouses->at(i).Unexplored)
		{
			return true;
		}
	}

	return false;
}

inline bool KnownHouseNotRecentlyVisited(Blackboard* pBlackboard)
{
	float secondsBetweenRevisits;
	std::vector<House>* knownHouses = nullptr;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("SecondsBetweenHouseRevisits", secondsBetweenRevisits) &&
		pBlackboard->GetData("KnownHouses", knownHouses);

	if (!dataAvailable || knownHouses->empty())
		return false;

	for (size_t i = 0; i < knownHouses->size(); i++)
	{
		if (knownHouses->at(i).SecondsSinceLastVisit >= secondsBetweenRevisits)
		{
			return true;
		}
	}

	return false;
}

inline bool CurrentlyInsideHouse(Blackboard* pBlackboard)
{
	bool insideHouse;
	bool dataAvailable = pBlackboard->GetData("InsideHouse", insideHouse);

	if (!dataAvailable)
		return false;

	return insideHouse;
}

inline BehaviourState SetGoalToClosestHouseNotRecentlyVisited(Blackboard* pBlackboard)
{
	float secondsBetweenRevisits;
	std::vector<House>* knownHouses = nullptr;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("SecondsBetweenHouseRevisits", secondsBetweenRevisits) &&
		pBlackboard->GetData("KnownHouses", knownHouses);

	if (!dataAvailable)
		return Failure;

	House closestHouse;
	int closestHouseIndex = -1;
	for (size_t i = 0; i < knownHouses->size(); i++)
	{
		if (knownHouses->at(i).SecondsSinceLastVisit >= secondsBetweenRevisits)
		{
			closestHouse = knownHouses->at(i);
			closestHouseIndex = i;
		}
	}

	if (closestHouseIndex != -1)
	{
		printf("Set goal of house\n");
		SteeringParams goal = {};
		goal.Position = closestHouse.Info.Center;
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Failure;
}

inline BehaviourState SetGoalToNextHouse(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<House>* knownHouses;
	int nextHouseIndex;
	bool dataAvailable =
		pBlackboard->GetData("KnownHouses", knownHouses) &&
		pBlackboard->GetData("NextHouseIndex", nextHouseIndex) &&
		pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable || knownHouses->empty())
		return Failure;

	if (knownHouses->size() == 1 && pAgentInfo->IsInHouse)
	{
		return Failure;
	}

	int newNextHouseIndex = (nextHouseIndex + 1) % knownHouses->size();

	pBlackboard->ChangeData("NextHouseIndex", newNextHouseIndex);

	printf("Set goal of next house, index %i/%i\n", newNextHouseIndex, knownHouses->size());
	SteeringParams goal;
	goal.Position = knownHouses->at(newNextHouseIndex).Info.Center;
	pBlackboard->ChangeData("Goal", goal);
	pBlackboard->ChangeData("GoalSet", true);

	return Success;
}

// Health
inline bool NotMaxHealth(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("MaxHealth", maxHealth);

	if (!dataAvailable || !pAgentInfo)
		return false;

	const int neededHealing = (int)(maxHealth - pAgentInfo->Health);
	if (neededHealing > 0)
	{
		return true;
	}

	return false;
}

inline bool LowHealth(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("MaxHealth", maxHealth);

	if (!dataAvailable || !pAgentInfo)
		return false;

	const int neededHealing = (int)(maxHealth - pAgentInfo->Health);
	if (neededHealing > maxHealth / 2)
	{
		return true;
	}

	return false;
}

inline bool KnowLocationOfHealthPacks(Blackboard* pBlackboard)
{
	std::vector<HealthPack>* knownHealthPacks = nullptr;
	float maxHealth = 0;
	bool dataAvailable = pBlackboard->GetData("KnownHealthPacks", knownHealthPacks);

	if (!dataAvailable)
		return false;

	return !knownHealthPacks->empty();
}

inline BehaviourState SetClosestKnownHealthPackAsGoal(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<HealthPack>* knownHealthPacks = nullptr;
	float maxHealth = 0;
	bool dataAvailable = 
		pBlackboard->GetData("KnownHealthPacks", knownHealthPacks) && 
		pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable)
		return Failure;

	float closestPackDist = FLT_MAX;
	int closestPackIndex = -1;
	for (size_t i = 0; i < knownHealthPacks->size(); i++)
	{
		float dist = b2Distance(knownHealthPacks->at(i).Position, pAgentInfo->Position);
		if (dist < closestPackDist)
		{
			closestPackDist = dist;
			closestPackIndex = i;
		}
	}

	if (closestPackIndex != -1)
	{
		printf("Set goal of closest health pack!\n");
		SteeringParams goal = {};
		goal.Position = knownHealthPacks->at(closestPackIndex).Position;
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Failure;
}

inline bool HasHealthItem(Blackboard* pBlackboard)
{
	std::vector<Item>* inventory = nullptr;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable || inventory->empty())
		return false;

	for (size_t i = 0; i < inventory->size(); i++)
	{
		if (inventory->at(i).Valid && inventory->at(i).ItemInfo.Type == eItemType::HEALTH)
		{
			return true;
		}
	}

	return false;
}

inline BehaviourState UseHealthItem(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	bool dataAvailable = pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable || !pAgentInfo)
		return Failure;

	pBlackboard->ChangeData("UseHealthItem", true);

	return Failure; // Continue with other sequences (we might not be hurt enough to use health yet)
}

// Food
inline bool NotMaxEnergy(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	float maxEnergy = 0;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("MaxEnergy", maxEnergy);

	if (!dataAvailable || !pAgentInfo)
		return false;

	const int neededEnergy = (int)(maxEnergy - pAgentInfo->Energy);
	if (neededEnergy > 0)
	{
		return true;
	}

	return false;
}

inline bool LowEnergy(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	float maxEnergy = 0;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("MaxEnergy", maxEnergy);

	if (!dataAvailable || !pAgentInfo)
		return false;

	const int neededEnergy = (int)(maxEnergy - pAgentInfo->Energy);
	if (neededEnergy > maxEnergy / 2)
	{
		return true;
	}

	return false;
}

inline bool HasFoodItem(Blackboard* pBlackboard)
{
	std::vector<Item>* inventory = nullptr;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable || inventory->empty())
		return false;

	for (size_t i = 0; i < inventory->size(); i++)
	{
		if (inventory->at(i).Valid && inventory->at(i).ItemInfo.Type == eItemType::FOOD)
		{
			return true;
		}
	}

	return false;
}

inline bool KnowLocationOfFoodItems(Blackboard* pBlackboard)
{
	std::vector<Food>* knownFoodItems = nullptr;
	float maxHealth = 0;
	bool dataAvailable = pBlackboard->GetData("KnownFoodItems", knownFoodItems);

	if (!dataAvailable)
		return false;

	return !knownFoodItems->empty();
}

inline BehaviourState SetClosestKnownFoodItemAsGoal(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<Food>* knownFoodItems = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("KnownFoodItems", knownFoodItems) &&
		pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable)
		return Failure;

	float closestFoodItemDist = FLT_MAX;
	int closestFoodItemIndex = -1;
	for (size_t i = 0; i < knownFoodItems->size(); i++)
	{
		float dist = b2Distance(knownFoodItems->at(i).Position, pAgentInfo->Position);
		if (dist < closestFoodItemDist)
		{
			closestFoodItemDist = dist;
			closestFoodItemIndex = i;
		}
	}

	if (closestFoodItemIndex != -1)
	{
		printf("Set goal of closest food item!\n");
		SteeringParams goal = {};
		goal.Position = knownFoodItems->at(closestFoodItemIndex).Position;
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Failure;
}

inline BehaviourState UseFoodItem(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	bool dataAvailable = pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable || !pAgentInfo)
		return Failure;

	pBlackboard->ChangeData("UseFoodItem", true);

	return Failure; // Continue with other sequences (we might not be hungry enough to eat yet)
}

inline bool LowEnergyOrHealth(Blackboard* pBlackboard)
{
	return LowEnergy(pBlackboard) || LowHealth(pBlackboard);
}

// Shooting
inline bool HasLoadedPistol(Blackboard* pBlackboard)
{
	std::vector<Item>* inventory = nullptr;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable)
		return false;

	for (size_t i = 0; i < inventory->size(); i++)
	{
		if (inventory->at(i).Valid && 
			inventory->at(i).ItemInfo.Type == eItemType::PISTOL)
		{
			return true;
		}
	}

	return false;
}

inline bool HasEnemyInFOV(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<Enemy>* knownEnemies = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("KnownEnemies", knownEnemies);

	if (!dataAvailable || !pAgentInfo)
		return false;

	float dist;
	Enemy nearestEnemy;
	if (NearestEnemyInFOV(knownEnemies, pAgentInfo, nearestEnemy, dist))
	{
		return true;
	}

	return false;
}

// Returns true if any enemy is within range of the agent's longest range pistol
inline bool HasEnemyInRange(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<Enemy>* knownEnemies = nullptr;
	float longestPistolRange;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("KnownEnemies", knownEnemies) &&
		pBlackboard->GetData("LongestPistolRange", longestPistolRange);

	if (!dataAvailable || !pAgentInfo || longestPistolRange == 0.0f)
		return false;

	float dist;
	Enemy nearestEnemy;
	if (NearestEnemyInFOV(knownEnemies, pAgentInfo, nearestEnemy, dist))
	{
		if (dist < longestPistolRange)
		{
			return true;
		}
	}

	return false;
}

inline BehaviourState AimAtNearestEnemyInFOV(Blackboard* pBlackboard)
{
	std::vector<Enemy>* knownEnemies = nullptr;
	AgentInfo* pAgentInfo = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("KnownEnemies", knownEnemies);

	if (!dataAvailable || !pAgentInfo)
		return Failure;

	float dist;
	Enemy nearestEnemy;
	if (NearestEnemyInFOV(knownEnemies, pAgentInfo, nearestEnemy, dist))
	{
		printf("Aiming at nearest enemy! (enemy hash %i)\n", nearestEnemy.enemyInfo.EnemyHash);
		pBlackboard->ChangeData("TargetEnemy", nearestEnemy);
		return Failure;
	}

	return Failure;
}

//inline BehaviourState ShootPistol(Blackboard* pBlackboard)
//{
//	pBlackboard->ChangeData("ShootPistol", true);
//
//	return Failure; // Keep executing other behaviours
//}
