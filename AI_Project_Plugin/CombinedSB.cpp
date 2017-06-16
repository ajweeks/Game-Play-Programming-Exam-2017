#include "stdafx.h"

#include "CombinedSB.h"

namespace CombinedSB
{
	//BLENDED STEERING
	//****************
	SteeringOutput BlendedSteering::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		SteeringOutput steering = {};
		auto totalWeight = 0.0f;

		for (auto pair : m_WeightedBehavioursVec)
		{
			auto retSteering = pair.pBehaviour->CalculateSteering(deltaT, agentInfo);
			steering.LinearVelocity += pair.Weight * retSteering.LinearVelocity;
			steering.AngularVelocity += pair.Weight * retSteering.AngularVelocity;

			totalWeight += pair.Weight;
		}

		if (totalWeight > 0.0f)
		{
			float scale = 1.0f / totalWeight;
			steering /= scale;
		}

		return steering;
	}

	//PRIORITY STEERING
	//*****************
	SteeringOutput PrioritySteering::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		SteeringOutput steering = {};

		for (auto pBehaviour : m_Behaviours)
		{
			steering = pBehaviour->CalculateSteering(deltaT, agentInfo);

			if (!steering.IsEmpty(m_Epsilon))
				break;
		}

		//If none of the behaviours return a valid output, last behaviour is returned
		return steering;
	}

	//BLENDED-PRIORITY STEERING
	//*************************
	SteeringOutput BlendedPrioritySteering::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		SteeringOutput steering = {};

		for (auto pBlendedBehaviour : m_PriorityGroups)
		{
			steering = pBlendedBehaviour->CalculateSteering(deltaT, agentInfo);

			if (!steering.IsEmpty(m_Epsilon))
				break;
		}

		//If none of the behaviours return a valid output, last behaviour is returned
		return steering;
	}
}