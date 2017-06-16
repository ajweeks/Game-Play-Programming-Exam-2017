#include "stdafx.h"

#include "SteeringBehaviours.h"
#include "HelperStructs.h"
#include "../_Includes/IBehaviourPlugin.h"

namespace SteeringBehaviours
{
	//SEEK
	//****
	SteeringOutput Seek::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		SteeringOutput steering = {};

		auto targetVelocity = (*m_pTargetRef).Position - agentInfo.Position;
		targetVelocity.Normalize();
		targetVelocity *= agentInfo.MaxLinearSpeed;

		steering.LinearVelocity = targetVelocity - agentInfo.LinearVelocity;

		return steering;
	}

	//PERSUE
	//******
	SteeringOutput Persue::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		auto distance = (agentInfo.Position - m_pTargetRef->Position).Length();
		auto t = distance / agentInfo.MaxLinearSpeed;

		auto targetVelocity = (*m_pTargetRef).LinearVelocity;
		targetVelocity.Normalize();

		auto newTarget = SteeringParams(m_pTargetRef->Position + (targetVelocity * t));
		Seek::m_pTargetRef = &newTarget;

		return Seek::CalculateSteering(deltaT, agentInfo);
	}

	//FLEE
	//****
	SteeringOutput Flee::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		SteeringOutput steering = {};

		auto targetVelocity = agentInfo.Position - m_pTargetRef->Position;
		targetVelocity.Normalize();
		targetVelocity *= agentInfo.MaxLinearSpeed;

		steering.LinearVelocity = targetVelocity - agentInfo.LinearVelocity;

		return steering;
	}

	//EVADE
	//*****
	SteeringOutput Evade::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		auto distance = (agentInfo.Position - m_pTargetRef->Position).Length();
		auto t = distance / agentInfo.MaxLinearSpeed;

		auto targetVelocity = (*m_pTargetRef).LinearVelocity;
		targetVelocity.Normalize();

		auto newTarget = SteeringParams(m_pTargetRef->Position + (targetVelocity * t));
		Flee::m_pTargetRef = &newTarget;

		return Flee::CalculateSteering(deltaT, agentInfo);
	}

	//WANDER
	//******
	SteeringOutput Wander::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		auto offset = agentInfo.LinearVelocity;
		offset.Normalize();
		offset *= m_Offset;

		b2Vec2 circleOffset = { cos(m_WanderAngle) * m_Radius, sin(m_WanderAngle) * m_Radius };

		m_WanderAngle += randomFloat() * m_AngleChange - (m_AngleChange * .5f); //RAND[-angleChange/2,angleChange/2]

		auto newTarget = SteeringParams(agentInfo.Position + offset + circleOffset);
		Seek::m_pTargetRef = &newTarget;

		//DEBUG RENDERING
		//if (m_pContext)
		//{
		//	auto pos = agentInfo.Position;
		//	m_pContext->renderer->DrawSegment(pos, pos + offset, { 0,0,1 });
		//	m_pContext->renderer->DrawCircle(pos + offset, m_Radius, { 0,0,1 }, -0.7f);
		//	m_pContext->renderer->DrawSolidCircle(pos + offset + circleOffset, 0.1f, { 0,0 }, { 0, 1, 0 }, -0.75f);
		//}

		return Seek::CalculateSteering(deltaT, agentInfo);
	}

	//ARRIVE
	//******
	SteeringOutput Arrive::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		SteeringOutput steering = {};

		auto targetVelocity = (*m_pTargetRef).Position - agentInfo.Position;
		auto distance = targetVelocity.Normalize() - m_TargetRadius;

		if (distance < m_SlowRadius) //Inside SlowRadius
		{
			targetVelocity *= agentInfo.MaxLinearSpeed * (distance / (m_SlowRadius + m_TargetRadius));
		}
		else
		{
			targetVelocity *= agentInfo.MaxLinearSpeed;
		}

		//Calculate Steering
		steering.LinearVelocity = targetVelocity - agentInfo.LinearVelocity;

		return steering;
	}

	SteeringOutput AvoidObstacle::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
	{
		auto normVel = agentInfo.LinearVelocity;
		auto lengthVel = normVel.Normalize();

		auto dynamicLength = lengthVel / agentInfo.MaxLinearSpeed;
		auto ahead = agentInfo.Position + normVel * dynamicLength;
		auto ahead2 = ahead * 0.5f;

		bool set;
		auto mostThreatening = FindMostThreateningObstacle(ahead, ahead2, agentInfo.Position, set);

		b2Vec2 avoidance = {};

		if (set) 
		{

			avoidance.x = ahead.x - mostThreatening.center.x;
			avoidance.y = ahead.y - mostThreatening.center.y;

			avoidance.Normalize();
			avoidance *= m_MaxAvoidanceForce;
		}
		else 
		{
			avoidance.SetZero(); // nullify the avoidance force
		}

		SteeringOutput steering = {};
		steering.LinearVelocity = avoidance;

		return steering;
	}

	Obstacle AvoidObstacle::FindMostThreateningObstacle(const b2Vec2& ahead, const b2Vec2& ahead2, const b2Vec2& position, bool& set)
	{
		Obstacle mostThreatening = {};
		set = false;

		for (auto obstacle : m_Obstacles)
		{
			auto collision = LineIntersectsCircle(ahead, ahead2, obstacle);

			// "position" is the character's current position
			if (collision && (set || Distance(position, obstacle.center) < Distance(position, mostThreatening.center)))
			{
				mostThreatening = obstacle;
				set = true;
			}
		}

		return mostThreatening;
	}

	float AvoidObstacle::Distance(b2Vec2 a, b2Vec2 b) const
	{
		return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	}

	bool AvoidObstacle::LineIntersectsCircle(b2Vec2 ahead, b2Vec2 ahead2, Obstacle obstacle) const
	{
		// the property "center" of the obstacle is a Vector3D.
		return Distance(obstacle.center, ahead) <= obstacle.radius || Distance(obstacle.center, ahead2) <= obstacle.radius;
	}
}
