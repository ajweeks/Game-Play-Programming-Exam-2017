#pragma once

#include "IBehaviourPlugin.h"
#include "SteeringBehaviours.h"

#include <vector>

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
	bool DecideToPickUpItem(const ItemInfo& itemInfo);

	void AddItemToInventory(int slotID, const ItemInfo& itemInfo);
	void RemoveItemFromInventory(int slotID);
	Item GetItemFromInventory(int slotID);
	void UseItemInInventory(int slotID);

	int FirstEmptyInventorySlotID() const;
	int EmptyInventorySlots() const;

	// Read the appropriate metadata for each item type
	void ConstructPistol(const ItemInfo& itemInfo, b2Vec2 Position, Pistol& pistol);
	void ConstructHealthPack(const ItemInfo& itemInfo, b2Vec2 Position, HealthPack& healthPack);
	void ConstructFood(const ItemInfo& itemInfo, b2Vec2 Position, Food& food);

	bool m_GrabAction = false;
	bool m_UseItemAction = false;
	bool m_RemoveItemAction = false;
	bool m_RunAction = false;
	int m_SelectedInventorySlot = 0;

	SteeringBehaviours::ISteeringBehaviour* m_CurrentSteeringBehaviour = nullptr;

	bool m_TargetSet = false;

	BehaviourTree* m_pBehaviourTree = nullptr;
	b2Vec2 m_ClickGoal; // Used for debugging
	SteeringParams m_Target = {};
	std::vector<SteeringBehaviours::ISteeringBehaviour*> m_BehaviourVec = {};

	std::vector<Item> m_Inventory; // Store this ourselves because the engine yells at us when we ask if a certain slot is full

	// Keep track of items that we find but don't pick up to potentially come back to later
	std::vector<Pistol> m_KnownPistols;
	std::vector<Food> m_KnownFood;
	std::vector<HealthPack> m_KnownHealthPacks;
	std::vector<Enemy> m_KnownEnemies;

	std::vector<HouseInfo> m_KnownHouses;
};
