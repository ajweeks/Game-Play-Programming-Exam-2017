#pragma once

#include "IBehaviourPlugin.h"
#include "SteeringBehaviours.h"

#include <vector>

namespace CombinedSB
{
	class BlendedSteering;
}

class BehaviourTree;

class TestBoxPlugin : public IBehaviourPlugin
{
public:
	TestBoxPlugin();
	~TestBoxPlugin();

	void Start() override;
	PluginOutput Update(float dt) override;
	void ExtendUI_ImGui() override;
	void End() override;
	void ProcessEvents(const SDL_Event& e) override;

protected:
	void LogOnFail(bool succeeded, const std::string& message);

	void AddItemToInventory(int slotID, const EntityInfo& entityInfo, const ItemInfo& itemInfo);
	void RemoveItemFromInventory(int slotID);
	Item GetItemFromInventory(int slotID);
	void UseItemInInventory(int slotID);

	int InventoryItemCount(eItemType itemType);
	int InventoryFirstSlotWithItemType(eItemType itemType);

	bool PointInFOV(const b2Vec2& point, const AgentInfo& agentInfo);

	int FirstEmptyInventorySlotID() const;
	int EmptyInventorySlots() const;

	void RemoveFromKnownItems(const EntityInfo& entityInfo);
	void DetermineInHouseIndex(const b2Vec2& agentPos);

	// Read the appropriate metadata for each item type
	void ConstructPistol(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, Pistol& pistol);
	void ConstructHealthPack(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, HealthPack& healthPack);
	void ConstructFood(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, Food& food);
	void ConstructEnemy(const EntityInfo& entityInfo, b2Vec2 Position, Enemy& enemy);
	void ConstructHouse(const HouseInfo& houseInfo, House& house);

	SteeringBehaviours::ISteeringBehaviour* m_CurrentSteeringBehaviour = nullptr;

	float m_SecondsElapsed;
	float m_SecondsSinceNavMeshTargetUpdate;
	float m_SecondsBetweenNavMeshTargetUpdates = 0.1f;
	Enemy m_EmptyTargetEnemy;

	BehaviourTree* m_pBehaviourTree = nullptr;
	SteeringParams m_Goal = {};
	SteeringParams m_NextNavMeshGoal = {};
	bool m_GoalSet = false;
	//SteeringParams m_SoftGoal = {}; // A general area to go to, if something interesting is spotted on the way here, look at it
	//bool m_SoftGoalSet = false;
	SteeringParams m_NearestEnemy = {};
	std::vector<SteeringBehaviours::ISteeringBehaviour*> m_BehaviourVec = {};
	CombinedSB::BlendedSteering* m_pBlendedBehaviour = nullptr;
	size_t m_FleeBehaviourWeightPairIndex = 2;
	float m_FleeWeightNearEnemies = 1.0f;
	float m_FleeWeightNotNearEnemies = 0.0f;

	std::vector<b2Vec2> m_SearchPoints;
	int m_SearchPointIndex;

	b2Vec2 m_AverageNearbyEnemyPosition;

	std::vector<Item> m_Inventory; // Store this ourselves because the engine yells at us when we ask if a certain slot is full

	float m_SecondsBetweenHouseRevisits = 90.0f; // How long to wait until visiting a house again
	float m_SecondsToEstimateEnemyPositionsFor = 5.0f;

	int m_NextHouseIndex = 0; // Keeps track of which house we're visiting next
	int m_InHouseIndex = -1; // -1 when not in any house

	float m_StartingHealth;
	float m_StartingEnergy;
	int m_BestPistolIndex = -1;
	float m_LongestPistolRange = 0.0f;
	int m_LongestPistolRangeIndex = -1;

	// Keep track of items that we find but don't pick up to potentially come back to later
	std::vector<EntityInfo> m_KnownItems; // Stores items we've seen in our FOV but we haven't gotten close enough to see their type
	std::vector<Pistol> m_KnownPistols;
	std::vector<Food> m_KnownFoodItems;
	std::vector<HealthPack> m_KnownHealthPacks;
	std::vector<Enemy> m_KnownEnemies;

	std::vector<House> m_KnownHouses;
};

