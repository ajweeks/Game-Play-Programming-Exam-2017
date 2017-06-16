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

inline bool HaveInventorySpace(Blackboard* pBlackboard)
{
	std::vector<Item>* inventory = nullptr;
	float maxHealth = 0;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable)
		return false;

	for (size_t i = 0; i < inventory->size(); i++)
	{
		if (!inventory->at(i).valid)
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

inline BehaviourState SetNearestItemAsTarget(Blackboard* pBlackboard)
{
	std::vector<EntityInfo>* knownItems = nullptr;
	std::vector<HealthPack>* knownHealthPacks = nullptr;
	std::vector<Food>* knownFoodItems = nullptr;
	std::vector<Pistol>* knownPistols = nullptr;
	AgentInfo* pAgentInfo = nullptr;
	float maxHealth = 0;
	float maxEnergy = 0;
	bool dataAvailable =
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

	// Prioritize healing
	if (nearestHealthPackIndex != -1)
	{
		HealthPack nearestHealthPack = knownHealthPacks->at(nearestHealthPackIndex);
		if (neededHealth >= nearestHealthPack.HealingAmount || 
			(nearestFoodItemIndex == -1 && nearestPistolIndex == -1))
		{
			SteeringParams goal = {};
			goal.Position = nearestHealthPack.Position;
			pBlackboard->ChangeData("Goal", goal);
			pBlackboard->ChangeData("GoalSet", true);
			return Success;
		}
	}
	// Then energy
	if (nearestFoodItemIndex != -1)
	{
		Food nearestFoodItem = knownFoodItems->at(nearestFoodItemIndex);
		if (nearestFoodItem.EnergyAmount >= neededEnergy ||
			nearestPistolIndex == -1)
		{
			SteeringParams goal = {};
			goal.Position = nearestFoodItem.Position;
			pBlackboard->ChangeData("Goal", goal);
			pBlackboard->ChangeData("GoalSet", true);
			return Success;
		}
	}
	// Then weapons
	if (nearestPistolIndex != -1)
	{
		Pistol nearestPistol = knownPistols->at(nearestFoodItemIndex);
		SteeringParams goal = {};
		goal.Position = nearestPistol.Position;
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	// Items we haven't inspected yet
	if (nearestItemIndex != -1)
	{
		EntityInfo nearestItem = knownItems->at(nearestItemIndex);
		SteeringParams goal = {};
		goal.Position = nearestItem.Position;
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Success;
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
		if (knownHouses->at(i).secondsSinceLastVisit >= secondsBetweenRevisits)
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

inline BehaviourState SetTargetToClosestHouseNotRecentlyVisited(Blackboard* pBlackboard)
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
		if (knownHouses->at(i).secondsSinceLastVisit >= secondsBetweenRevisits)
		{
			closestHouse = knownHouses->at(i);
			closestHouseIndex = i;
		}
	}

	if (closestHouseIndex != -1)
	{
		SteeringParams goal = {};
		goal.Position = closestHouse.info.Center;
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Failure;
}

// Health
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
	if (neededHealing > 0)
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

inline BehaviourState SetClosestKnownHealthPackAsTarget(Blackboard* pBlackboard)
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
		if (inventory->at(i).valid && inventory->at(i).itemInfo.Type == eItemType::HEALTH)
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

	return Success;
}

// Food
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
	if (neededEnergy > 0)
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
		if (inventory->at(i).valid && inventory->at(i).itemInfo.Type == eItemType::FOOD)
		{
			return true;
		}
	}

	return false;
}

inline BehaviourState UseFoodItem(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	bool dataAvailable = pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable || !pAgentInfo)
		return Failure;

	pBlackboard->ChangeData("UseFoodItem", true);

	return Failure; // Keep running other cases even though this one succeeded
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
		if (inventory->at(i).valid && 
			inventory->at(i).itemInfo.Type == eItemType::PISTOL)
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
		b2Vec2 v1 = nearestEnemy.Position - pAgentInfo->Position;
		v1.Normalize();

		b2Vec2 v2 = b2Vec2(cos(pAgentInfo->Orientation), sin(pAgentInfo->Orientation));
		v2.Normalize();

		const float dAngle = acos(b2Dot(v1, v2));
		const float minAngle = 0.08f; // TODO: Find better way of figuring out if enemies are able to be shot

		if (abs(dAngle) > minAngle)
		{
			float angularVel = std::copysign(pAgentInfo->MaxAngularSpeed, dAngle);
			// Prevent turning too far
			angularVel = dAngle > 0 ? std::min(dAngle, angularVel) : std::max(dAngle, angularVel);
			pAgentInfo->AngularVelocity = angularVel;

			return Running;
		}
		else
		{
			// They're in our sights! Fire!
			pBlackboard->ChangeData("TargetEnemyHash", nearestEnemy.enemyInfo.EnemyHash);
			return Success;
		}
	}

	return Failure;
}

inline BehaviourState ShootPistol(Blackboard* pBlackboard)
{
	pBlackboard->ChangeData("ShootPistol", true);

	return Failure; // Keep executing other behaviours
}

inline BehaviourState ChangeToSeek(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	SteeringBehaviours::ISteeringBehaviour* pSeekBehaviour = nullptr;
	SteeringBehaviours::ISteeringBehaviour* pCurrentBehaviour = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("SeekBehaviour", pSeekBehaviour) &&
		pBlackboard->GetData("CurrentBehaviour", pCurrentBehaviour);

	if (!dataAvailable || !pAgentInfo || !pSeekBehaviour)
		return Failure;

	if (pCurrentBehaviour != pSeekBehaviour)
	{
		printf("Come over here \n");
		pBlackboard->ChangeData("CurrentBehaviour", pSeekBehaviour);
	}

	return Success;
}

inline BehaviourState ChangeToWander(Blackboard* pBlackboard)
{
	//Get data
	AgentInfo* pAgentInfo = nullptr;
	SteeringBehaviours::ISteeringBehaviour* pWanderBehaviour = nullptr;
	SteeringBehaviours::ISteeringBehaviour* pCurrentBehaviour = nullptr;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("WanderBehaviour", pWanderBehaviour) &&
		pBlackboard->GetData("CurrentBehaviour", pCurrentBehaviour);

	if (!dataAvailable || !pAgentInfo || !pWanderBehaviour)
		return Failure;

	if (pCurrentBehaviour != pWanderBehaviour)
	{
		pBlackboard->ChangeData("CurrentBehaviour", pWanderBehaviour);
		printf("Just minding my own business \n");
	}

	return Success;
}
