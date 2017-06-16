#include "stdafx.h"

#include "HelperStructs.h"

bool operator==(const Enemy& lhs, const Enemy& rhs)
{
	return  lhs.Position == rhs.Position &&
			lhs.info.EnemyHash == rhs.info.EnemyHash &&
			lhs.info.Health == rhs.info.Health;
}

bool operator==(const Item& lhs, const Item& rhs)
{
	return	lhs.info.Type == rhs.info.Type && 
			lhs.info.ItemHash == rhs.info.ItemHash;
}

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
