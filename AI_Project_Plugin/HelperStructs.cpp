#include "stdafx.h"

#include "HelperStructs.h"

bool operator==(const EntityInfo& lhs, const EntityInfo& rhs)
{
	return lhs.EntityHash == rhs.EntityHash;
}

bool operator==(const Enemy& lhs, const Enemy& rhs)
{
	return lhs.enemyInfo.EnemyHash == rhs.enemyInfo.EnemyHash;
}

bool operator==(const Item& lhs, const Item& rhs)
{
	return	lhs.ItemInfo.Type == rhs.ItemInfo.Type && 
			lhs.ItemInfo.ItemHash == rhs.ItemInfo.ItemHash;
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

bool operator==(const House& lhs, const House& rhs)
{
	return	lhs.Info.Center == rhs.Info.Center &&
			lhs.Info.Size == rhs.Info.Size;
}

float Pistol::GetValue()
{
	if (Ammo == 0 || DPS == 0 || Range == 0.0f)
	{
		// We're a dud!
		return 0.0f;
	}

	float value = 0.0f;
	value += DPS * 1.0f;
	value += Range * 0.5f;
	value += Ammo * 2.0f;

	return value;
}
