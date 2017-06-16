#pragma once

#include "HelperStructs.h"

#include <vector>

namespace SteeringBehaviours
{
	class ISteeringBehaviour
	{
	public:
		ISteeringBehaviour() {}
		virtual ~ISteeringBehaviour() {}

		virtual SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) = 0;

		template<class T, typename std::enable_if<std::is_base_of<ISteeringBehaviour, T>::value>::type* = nullptr>
		T* As()
		{
			return static_cast<T*>(this);
		}
	};

	//SEEK
	//****
	class Seek : public ISteeringBehaviour
	{
	public:
		Seek() {};
		virtual ~Seek() {};

		//Seek Behaviour
		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

		//Seek Functions
		virtual void SetTarget(const SteeringParams* pTarget) { m_pTargetRef = pTarget; }

	protected:
		const SteeringParams* m_pTargetRef = nullptr;
	};

	//PERSUE
	//******
	class Persue :public Seek
	{
	public:
		Persue() {};
		virtual ~Persue() {};

		//Persue Behaviour
		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

		//Persue Functions
		virtual void SetTarget(const SteeringParams* pTarget) override { m_pTargetRef = pTarget; }

	protected:
		const SteeringParams* m_pTargetRef = nullptr;
	};

	//FLEE
	//****
	class Flee : public Seek
	{
	public:
		Flee() {};
		virtual ~Flee() {};

		//Seek Behaviour
		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;
	};

	//EVADE
	//*****
	class Evade : public Flee
	{
	public:
		Evade() {};
		virtual ~Evade() {};

		//Evade Behaviour
		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

		//Evade Functions
		virtual void SetTarget(const SteeringParams* pTarget) override { m_pTargetRef = pTarget; }

	protected:
		const SteeringParams* m_pTargetRef = nullptr;
	};

	//WANDER
	//******
	class Wander : public Seek
	{
	public:
		Wander() {};
		virtual ~Wander() {};

		//Wander Behaviour
		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

		void SetWanderOffset(float offset) { m_Offset = offset; }
		void SetWanderRadius(float radius) { m_Radius = radius; }
		void SetMaxAngleChange(float rad) { m_AngleChange = rad; }

	protected:

		float m_Offset = 6.0f; //Offset (Agent Direction)
		float m_Radius = 4.0f; //WanderRadius
		float m_AngleChange = b2_pidiv4; //Max WanderAngle change per frame
		float m_WanderAngle = 0.0f; //Internal

	private:
		void SetTarget(const SteeringParams* pTarget) override {} //Hide SetTarget, No Target needed for Wander
	};

	//ARRIVE
	//******
	class Arrive : public ISteeringBehaviour
	{
	public:
		Arrive() {};
		virtual ~Arrive() {};

		//Seek Behaviour
		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

		//Seek Functions
		virtual void SetTarget(const SteeringParams* pTarget) { m_pTargetRef = pTarget; }
		void SetSlowRadius(float radius) { m_SlowRadius = radius; }

	protected:

		const SteeringParams* m_pTargetRef = nullptr;
		float m_SlowRadius = 10.0f;
		float m_TargetRadius = 2.0f;
	};

	//ALIGN
	//*****

	//AVOID-OBSTACLE
	struct Obstacle
	{
		b2Vec2 center;
		float radius;
	};

	class AvoidObstacle : public ISteeringBehaviour
	{
	public:
		AvoidObstacle(std::vector<Obstacle> obstacles) :m_Obstacles(obstacles) {};
		virtual ~AvoidObstacle() {};

		//AvoidObstacle Behaviour
		SteeringOutput CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

		//AvoidObstacle Functions
		void SetMaxAvoidanceForce(float max) { m_MaxAvoidanceForce = max; }

	protected:
		std::vector<Obstacle> m_Obstacles = {};
		float m_MaxAvoidanceForce = 10.0f;

	private:
		Obstacle FindMostThreateningObstacle(const b2Vec2& ahead, const b2Vec2& ahead2, const b2Vec2& position, bool& set);
		float Distance(b2Vec2 a, b2Vec2 b) const;
		bool LineIntersectsCircle(b2Vec2 ahead, b2Vec2 ahead2, Obstacle obstacle) const;

	};
}

