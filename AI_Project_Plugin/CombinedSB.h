#pragma once

#include "SteeringBehaviours.h"

namespace CombinedSB
{
	struct BehaviourAndWeight
	{
		SteeringBehaviours::ISteeringBehaviour* pBehaviour;
		float Weight;

		BehaviourAndWeight(SteeringBehaviours::ISteeringBehaviour* pBeh, float weight) :
			pBehaviour(pBeh),
			Weight(weight)
		{};
	};

	class BlendedSteering : public SteeringBehaviours::ISteeringBehaviour
	{
	public:
		BlendedSteering(std::vector<BehaviourAndWeight> weightedBehaviours) :
			m_WeightedBehavioursVec(weightedBehaviours) 
		{}
		virtual ~BlendedSteering() {};

		void SetBehaviourWeight(size_t behaviourIndex, float newWeight);
		void AddBehaviour(BehaviourAndWeight pair) { m_WeightedBehavioursVec.push_back(pair); }

		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

	private:
		std::vector<BehaviourAndWeight> m_WeightedBehavioursVec = {};
	};


	class PrioritySteering : public SteeringBehaviours::ISteeringBehaviour
	{
	public:
		PrioritySteering(std::vector<SteeringBehaviours::ISteeringBehaviour*> behaviours) :
			m_Behaviours(behaviours) 
		{}
		virtual ~PrioritySteering() {}

		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

	private:
		std::vector<SteeringBehaviours::ISteeringBehaviour*> m_Behaviours = {};
		float m_Epsilon = 0.0001f;
	};


	class BlendedPrioritySteering : public SteeringBehaviours::ISteeringBehaviour
	{
	public:
		BlendedPrioritySteering(std::vector<BlendedSteering*> priorityGroups) :
			m_PriorityGroups(priorityGroups) 
		{}
		virtual ~BlendedPrioritySteering() {}

		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

	private:
		std::vector<BlendedSteering*> m_PriorityGroups = {};
		float m_Epsilon = 0.0001f;
	};
}
