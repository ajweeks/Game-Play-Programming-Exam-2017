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
	//void ProcessEvents(const SDL_Event& e) override;

protected:
	void LogOnFail(bool succeeded, const std::string& message);

	void AddItemToInventory(int slotID, const EntityInfo& entityInfo, const ItemInfo& itemInfo);
	void RemoveItemFromInventory(int slotID);
	Item GetItemFromInventory(int slotID);
	void UseItemInInventory(int slotID);

	int FirstEmptyInventorySlotID() const;
	int EmptyInventorySlots() const;
	int InventoryItemCount(eItemType itemType);
	int InventoryFirstSlotWithItemType(eItemType itemType);

	bool PointInFOV(const b2Vec2& point, const AgentInfo& agentInfo);

	void RemoveFromKnownItems(const EntityInfo& entityInfo);
	void DetermineInHouseIndex(const b2Vec2& agentPos);

	// Read the appropriate metadata for each item type
	void ConstructPistol(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, Pistol& pistol);
	void ConstructHealthPack(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, HealthPack& healthPack);
	void ConstructFood(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, Food& food);
	void ConstructEnemy(const EntityInfo& entityInfo, b2Vec2 Position, Enemy& enemy);
	void ConstructHouse(const HouseInfo& houseInfo, House& house);

	float m_SecondsElapsed;
	float m_SecondsSinceNavMeshTargetUpdate;
	float m_SecondsBetweenNavMeshTargetUpdates = 0.1f;
	Enemy m_EmptyTargetEnemy;

	BehaviourTree* m_pBehaviourTree = nullptr;
	SteeringParams m_Goal = {};
	SteeringParams m_NextNavMeshGoal = {};
	bool m_GoalSet = false;

	std::vector<SteeringBehaviours::ISteeringBehaviour*> m_BehaviourVec = {};
	SteeringParams m_AverageNearbyEnemy;
	CombinedSB::BlendedSteering* m_pBlendedBehaviour = nullptr;
	size_t m_FleeBehaviourWeightPairIndex;
	float m_FleeWeightNearEnemies = 0.5f;
	float m_FleeWeightNotNearEnemies = 0.0f;

	std::vector<b2Vec2> m_SearchPoints;
	int m_SearchPointIndex;

	std::vector<Item> m_Inventory; // Store this ourselves because the engine yells at us when we ask if a certain slot is full

	float m_SecondsBetweenHouseRevisits = 90.0f; // How long to wait until visiting a house again
	float m_SecondsToEstimateEnemyPositionsFor = 4.0f;

	int m_NextHouseIndex = 0; // Keeps track of which house we're visiting next
	int m_InHouseIndex = -1; // -1 when not in any house

	float m_StartingStamina;
	float m_StartingEnergy;
	float m_StartingHealth;
	int m_BestPistolIndex = -1;
	float m_LongestPistolRange = 0.0f;
	int m_LongestPistolRangeInventoryIndex = -1;

	// Cached world vectors
	std::vector<HealthPack> m_KnownHealthPacks;
	std::vector<Food> m_KnownFoodItems;
	std::vector<Pistol> m_KnownPistols;
	std::vector<EntityInfo> m_KnownItems; // Stores items we've seen in our FOV but we haven't gotten close enough to see their type
	std::vector<Enemy> m_KnownEnemies;
	std::vector<House> m_KnownHouses;
};
