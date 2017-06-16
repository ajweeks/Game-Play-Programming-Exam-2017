#pragma once

#include "Blackboard.h"
#include "BehaviourTree.h"
#include "HelperStructs.h"
#include "SteeringBehaviours.h"

#include <Box2D\Box2D.h>

bool NearestEnemy(const std::vector<Enemy>& enemies, AgentInfo* pAgentInfo, Enemy& nearestEnemy, float& dist)
{
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

//GENERAL BEHAVIOURS
inline bool HasTarget(Blackboard* pBlackboard)
{
	//Get data
	auto hasTarget = false;
	auto dataAvailable = pBlackboard->GetData("TargetSet", hasTarget);
	if (!dataAvailable)
		return false;

	return hasTarget;
}

inline bool NotCloseToTarget(Blackboard* pBlackboard)
{
	//Get data
	AgentInfo* pAgentInfo = nullptr;
	SteeringParams target = {};
	auto dataAvailable =
		pBlackboard->GetData("AgentInfo", pAgentInfo) &&
		pBlackboard->GetData("Target", target);

	if (!dataAvailable || !pAgentInfo)
		return false;

	auto distance = b2Distance(target.Position, pAgentInfo->Position);
	if (distance > pAgentInfo->GrabRange)
		return true;

	return false;
}

inline bool CloseToEnemy(Blackboard* pBlackboard)
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
	if (NearestEnemy(knownEnemies, pAgentInfo, nearestEnemy, dist))
	{
		if (dist <= pAgentInfo->GrabRange)
		{
			return true;
		}
	}
	
	return false;
}

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

inline BehaviourState AimAtNearestEnemy(Blackboard* pBlackboard)
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

	if (NearestEnemy(knownEnemies, pAgentInfo, nearestEnemy, dist))
	{
		b2Vec2 v1 = nearestEnemy.Position;
		v1.Normalize();

		b2Vec2 v2 = pAgentInfo->Position;
		v2.Normalize();

		const float dAngle = acos(b2Dot(v1, v2));
		const float minAngle = 0.01f;

		if (abs(dAngle) > minAngle)
		{
			float angularVel = std::copysign(pAgentInfo->MaxAngularSpeed, dAngle);
			angularVel = std::min(dAngle, angularVel); // Don't overturn
			pAgentInfo->AngularVelocity = angularVel;

			return Running;
		}
		else
		{
			return Success;
		}
	}

	return Failure;
}

inline BehaviourState ShootPistol(Blackboard* pBlackboard)
{
	pBlackboard->ChangeData("ShootPistol", true);

	return Success;
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
