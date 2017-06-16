#include "stdafx.h"

#include "IBehaviourPlugin.h"

bool operator==(const Pistol& lhs, const Pistol& rhs)
{
	return (lhs.Ammo == rhs.Ammo &&
		lhs.DPS == rhs.DPS &&
		lhs.Range == rhs.Range);
}

bool operator==(const HealthPack& lhs, const HealthPack& rhs)
{
	return (lhs.HealingAmount == rhs.HealingAmount);
}

bool operator==(const Food& lhs, const Food& rhs)
{
	return (lhs.EnergyAmount == rhs.EnergyAmount);
}

bool operator==(const HouseInfo& lhs, const HouseInfo& rhs)
{
	return	lhs.Center == rhs.Center &&
		lhs.Size == rhs.Size;
}