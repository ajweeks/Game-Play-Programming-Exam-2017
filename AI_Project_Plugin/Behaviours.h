#pragma once

#include "Blackboard.h"
#include "BehaviourTree.h"
#include "HelperStructs.h"
#include "SteeringBehaviours.h"

#include <Box2D\Box2D.h>


// Misc
inline bool NearestEnemy(const std::vector<Enemy>& enemies, AgentInfo* pAgentInfo, Enemy& nearestEnemy, float& dist)
{
	if (enemies.empty()) return false;

	float smallestDistance = FLT_MAX;
	auto closestIter = enemies.end();
	for (auto iter = enemies.begin(); iter != enemies.end(); ++iter)
	{
		float dist = b2Distance(iter->Position, pAgentInfo->Position);
		if (dist < smallestDistance)
		{
			smallestDistance = dist;
			closestIter = iter;
		}
	}

	dist = smallestDistance;

	if (closestIter != enemies.end())
	{
		nearestEnemy = *closestIter;
		return true;
	}

	return false;
}

inline bool NearestEnemyInFOV(const std::vector<Enemy>& enemies, AgentInfo* pAgentInfo, Enemy& nearestEnemy, float& dist)
{
	if (enemies.empty()) return false;

	float smallestDistance = FLT_MAX;
	auto closestIter = enemies.end();
	for (auto iter = enemies.begin(); iter != enemies.end(); ++iter)
	{
		float dist = b2Distance(iter->Position, pAgentInfo->Position);
		if (iter->InFieldOfView && dist < smallestDistance)
		{
			smallestDistance = dist;
			closestIter = iter;
		}
	}

	dist = smallestDistance;

	if (closestIter != enemies.end())
	{
		nearestEnemy = *closestIter;
		return true;
	}

	return false;
}

inline bool HaveInventorySpace(Blackboard* pBlackboard)
{
	std::vector<Item> inventory;
	float maxHealth = 0;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable)
		return false;

	for (size_t i = 0; i < inventory.size(); i++)
	{
		if (!inventory[i].valid)
		{
			return true;
		}
	}

	return false;
}

inline bool KnownHouseNotRecentlyVisited(Blackboard* pBlackboard)
{
	float secondsBetweenRevisits;
	std::vector<House> knownHouses;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("SecondsBetweenHouseRevisits", secondsBetweenRevisits) &&
		pBlackboard->GetData("KnownHouses", knownHouses);

	if (!dataAvailable || knownHouses.empty())
		return false;

	for (size_t i = 0; i < knownHouses.size(); i++)
	{
		if (knownHouses[i].secondsSinceLastVisit >= secondsBetweenRevisits)
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
	std::vector<House> knownHouses;
	float maxHealth = 0;
	bool dataAvailable =
		pBlackboard->GetData("SecondsBetweenHouseRevisits", secondsBetweenRevisits) &&
		pBlackboard->GetData("KnownHouses", knownHouses);

	if (!dataAvailable)
		return Failure;

	House closestHouse;
	int closestHouseIndex = -1;
	for (size_t i = 0; i < knownHouses.size(); i++)
	{
		if (knownHouses[i].secondsSinceLastVisit >= secondsBetweenRevisits)
		{
			closestHouse = knownHouses[i];
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
	std::vector<HealthPack> knownHealthPacks;
	float maxHealth = 0;
	bool dataAvailable = pBlackboard->GetData("KnownHealthPacks", knownHealthPacks);

	if (!dataAvailable)
		return false;

	return !knownHealthPacks.empty();
}

inline BehaviourState SetClosestKnownHealthPackAsTarget(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<HealthPack> knownHealthPacks;
	float maxHealth = 0;
	bool dataAvailable = 
		pBlackboard->GetData("KnownHealthPacks", knownHealthPacks) && 
		pBlackboard->GetData("AgentInfo", pAgentInfo);

	if (!dataAvailable)
		return Failure;

	float closestPackDist = FLT_MAX;
	int closestPackIndex = -1;
	for (size_t i = 0; i < knownHealthPacks.size(); i++)
	{
		float dist = b2Distance(knownHealthPacks[i].Position, pAgentInfo->Position);
		if (dist < closestPackDist)
		{
			closestPackDist = dist;
			closestPackIndex = i;
		}
	}

	if (closestPackIndex != -1)
	{
		SteeringParams goal = {};
		goal.Position = knownHealthPacks[closestPackIndex].Position;
		pBlackboard->ChangeData("Goal", goal);
		pBlackboard->ChangeData("GoalSet", true);
		return Success;
	}

	return Failure;
}

inline bool HasHealthItem(Blackboard* pBlackboard)
{
	std::vector<Item> inventory;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable || inventory.empty())
		return false;

	for (size_t i = 0; i < inventory.size(); i++)
	{
		if (inventory[i].valid && inventory[i].info.Type == eItemType::HEALTH)
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
	std::vector<Item> inventory;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable || inventory.empty())
		return false;

	for (size_t i = 0; i < inventory.size(); i++)
	{
		if (inventory[i].valid && inventory[i].info.Type == eItemType::FOOD)
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
	std::vector<Item> inventory;
	bool dataAvailable = pBlackboard->GetData("Inventory", inventory);

	if (!dataAvailable)
		return false;

	for (size_t i = 0; i < inventory.size(); i++)
	{
		if (inventory[i].valid && inventory[i].info.Type == eItemType::PISTOL)
		{
			return true;
		}
	}

	return false;
}

inline bool HasEnemyInFOV(Blackboard* pBlackboard)
{
	AgentInfo* pAgentInfo = nullptr;
	std::vector<Enemy> knownEnemies;
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
	std::vector<Enemy> knownEnemies;
	float longestPistolRange;
	bool dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("KnownEnemies", knownEnemies) &&
		pBlackboard->GetData("LongestPistolRange", longestPistolRange);

	if (!dataAvailable || !pAgentInfo)
		return false;

	assert(longestPistolRange > 0.0f);

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
	std::vector<Enemy> knownEnemies;
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
			pBlackboard->ChangeData("TargetEnemyHash", nearestEnemy.info.EnemyHash);
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
