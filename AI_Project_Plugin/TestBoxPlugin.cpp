
#include "stdafx.h"

#include "TestBoxPlugin.h"
#include "BehaviourTree.h"
#include "SteeringBehaviours.h"
#include "Behaviours.h"

TestBoxPlugin::TestBoxPlugin():IBehaviourPlugin(GameDebugParams(20, true, false, false, false, 2.0f))
{
}

TestBoxPlugin::~TestBoxPlugin()
{
	for (auto pBehaviour : m_BehaviourVec)
	{
		SafeDelete(pBehaviour);
	}
	m_BehaviourVec.clear();

	SafeDelete(m_pBehaviourTree);
}

void TestBoxPlugin::Start()
{
	m_Inventory.resize(INVENTORY_GetCapacity());
	for (size_t i = 0; i < m_Inventory.size(); i++)
	{
		m_Inventory[i].info = {};
		m_Inventory[i].valid = false;
	}

	AgentInfo agentInfo = AGENT_GetInfo(); //Contains all Agent Parameters, retrieved by copy!

	//Create behaviours
	auto pSeekBehaviour = new SteeringBehaviours::Seek();
	pSeekBehaviour->SetTarget(&m_Target);
	m_BehaviourVec.push_back(pSeekBehaviour);
	auto pWanderBehaviour = new SteeringBehaviours::Wander();
	m_BehaviourVec.push_back(pWanderBehaviour);
	auto pFleeBehaviour = new SteeringBehaviours::Flee();
	m_BehaviourVec.push_back(pFleeBehaviour);

	m_CurrentSteeringBehaviour = pWanderBehaviour;

	//m_pSeparationBehaviour = new Separation(this);
	//m_pSeparationBehaviour->SetNeighbourhoodRadius(8.0f);
	//m_pSeparationBehaviour->SetNeighbourhoodMinDP(-1.0f);
	//
	//m_pCohesionBehaviour = new Cohesion(this);
	//m_pCohesionBehaviour->SetNeighbourhoodRadius(30.0f);
	//m_pCohesionBehaviour->SetNeighbourhoodMinDP(0.0f);
	//
	//m_pVelMatchBehaviour = new VelocityMatch(this);
	//m_pVelMatchBehaviour->SetNeighbourhoodRadius(15.0f);
	//m_pVelMatchBehaviour->SetNeighbourhoodMinDP(0.0f);
	//
	//
	//m_pBlendedSteering = new CombinedSB::BlendedSteering(
	//{
	//	{ m_pSeparationBehaviour, 1.0f },
	//	{ m_pCohesionBehaviour, 1.0f },
	//	{ m_pVelMatchBehaviour, 2.0f }
	//});

	//Create blackboard
	auto pBlackboard = new Blackboard;
	pBlackboard->AddData("AgentInfo", &agentInfo);
	pBlackboard->AddData("Target", m_Target);
	pBlackboard->AddData("TargetSet", false);
	pBlackboard->AddData("SeekBehaviour", static_cast<SteeringBehaviours::ISteeringBehaviour*>(pSeekBehaviour));
	pBlackboard->AddData("WanderBehaviour", static_cast<SteeringBehaviours::ISteeringBehaviour*>(pWanderBehaviour));
	pBlackboard->AddData("FleeBehaviour", static_cast<SteeringBehaviours::ISteeringBehaviour*>(pFleeBehaviour));
	pBlackboard->AddData("CurrentBehaviour", static_cast<SteeringBehaviours::ISteeringBehaviour*>(m_CurrentSteeringBehaviour));
	pBlackboard->AddData("Inventory", m_Inventory);
	pBlackboard->AddData("KnownEnemies", m_KnownEnemies);
	pBlackboard->AddData("MaxHealth", agentInfo.Health);
	pBlackboard->AddData("MaxEnergy", agentInfo.Energy);
	pBlackboard->AddData("LongestPistolRange", 0.0f);
	pBlackboard->AddData("TargetEnemyHash", 0);
	pBlackboard->AddData("KnownHealthPacks", m_KnownHealthPacks);
	pBlackboard->AddData("KnownFoodItems", m_KnownFoodItems);

	// Flags behaviours can set to send info back to this class
	pBlackboard->AddData("ShootPistol", false);
	pBlackboard->AddData("UseHealthItem", false);
	pBlackboard->AddData("UseFoodItem", false);

	m_pBehaviourTree = new BehaviourTree(pBlackboard,
	new BehaviourSelector
	({
		new BehaviourSelector // Health
		({
			new BehaviourSequence
			({
				new BehaviourConditional(LowHealth),
				new BehaviourConditional(HasHealthItem),
				new BehaviourAction(UseHealthItem)
			}),
			new BehaviourSequence // Search for resources
			({
				new BehaviourConditional(LowHealth),
				new BehaviourConditional(KnowLocationOfHealthPacks),
				new BehaviourAction(FindKnownHealthPacks)
			}),
			new BehaviourSequence // Search for resources
			({
				new BehaviourConditional(LowHealth),
				new BehaviourAction(ChangeToWander) // TODO: Put something more smart here that looks for health packs
			}),
		}),
		new BehaviourSequence // Check energy
		({
			new BehaviourConditional(LowEnergy),
			new BehaviourConditional(HasFoodItem),
			new BehaviourAction(UseFoodItem)
		}),
		new BehaviourSequence // Try to shoot enemies
		({
			new BehaviourConditional(HasLoadedPistol),
			new BehaviourConditional(HasEnemyInFOV),
			new BehaviourConditional(HasEnemyInRange),
			new BehaviourAction(AimAtNearestEnemyInFOV),
			new BehaviourAction(ShootPistol)
		}),
		new BehaviourSequence // ~~~~~~~~~~~~~~~DEBUG: Go to clicked point TODO: REMOVE ~~~~~~~~~~~~~~~~~~~~~
		({
			new BehaviourConditional(HasTarget),
			new BehaviourConditional(NotCloseToTarget),
			new BehaviourAction(ChangeToSeek)
		}),
		new BehaviourAction(ChangeToWander) // Look for things randomly
	}));

	m_StartingHealth = agentInfo.Health;
	m_StartingEnergy = agentInfo.Energy;
}

PluginOutput TestBoxPlugin::Update(float dt)
{
	WorldInfo worldInfo = WORLD_GetInfo(); //Contains the location of the center of the world and the dimensions
	AgentInfo agentInfo = AGENT_GetInfo(); //Contains all Agent Parameters, retrieved by copy!

	b2Vec2 minCoords = worldInfo.Center - worldInfo.Dimensions / 2.0f;
	b2Vec2 maxCoords = worldInfo.Center + worldInfo.Dimensions / 2.0f;

	for (size_t i = 0; i < m_KnownEnemies.size(); i++)
	{
		m_KnownEnemies[i].InFieldOfView = false;
		m_KnownEnemies[i].PredictedPosition += m_KnownEnemies[i].Velocity * dt;
	}

	// Forget about enemies seen before, they've probably moved now
	std::vector<Enemy> enemiesInFOV;

	std::vector<Pistol> foundPistols;
	std::vector<HealthPack> foundHealthPacks;
	std::vector<Food> foundFood;

	// Add new found entities to cache
	std::vector<EntityInfo> entitiesInFOV = FOV_GetEntities();
	for (auto iter = entitiesInFOV.begin(); iter != entitiesInFOV.end(); ++iter)
	{
		EntityInfo& entityInfo = *iter;

		switch (entityInfo.Type)
		{
		case eEntityType::ITEM:
		{
			if (b2DistanceSquared(entityInfo.Position, agentInfo.Position) < agentInfo.GrabRange * agentInfo.GrabRange)
			{
				ItemInfo itemInfo = {};
				ITEM_Grab(entityInfo, itemInfo);

				switch (itemInfo.Type)
				{
				case eItemType::PISTOL:
				{
					Pistol pistol = {};
					ConstructPistol(itemInfo, entityInfo.Position, pistol);
					if (!Contains(foundPistols, pistol))
					{
						foundPistols.push_back(pistol);
					}
				} break;
				case eItemType::HEALTH:
				{
					HealthPack healthPack = {};
					ConstructHealthPack(itemInfo, entityInfo.Position, healthPack);
					if (!Contains(foundHealthPacks, healthPack))
					{
						foundHealthPacks.push_back(healthPack);
					}
				} break;
				case eItemType::FOOD:
				{
					Food food = {};
					ConstructFood(itemInfo, entityInfo.Position, food);

					if (food.EnergyAmount > 0)
					{
						if (!Contains(foundFood, food))
						{
							foundFood.push_back(food);
						}
					}
				} break;
				case eItemType::GARBAGE:
				{
					// Ignore
				} break;
				default:
					DEBUG_LogMessage("Unhandled item type: " + std::to_string(itemInfo.Type) + "\n");
					break;
				}
			}
		} break;
		case eEntityType::ENEMY:
		{
			Enemy enemy = {};
			ConstructEnemy(entityInfo, entityInfo.Position, enemy);
			enemiesInFOV.push_back(enemy);

			std::vector<Enemy>::iterator iter = IndexOf(m_KnownEnemies, enemy);
			if (iter == m_KnownEnemies.end())
			{
				m_KnownEnemies.push_back(enemy);
			}
			else
			{
				// TODO: Display predicted position

				// We already know about this enemy, update our info on it
				b2Vec2 lastPos = iter->Position;
				iter->info.Health = enemy.info.Health;
				iter->Position = entityInfo.Position;
				iter->PredictedPosition = entityInfo.Position;
				iter->LastPosition = lastPos;
				iter->InFieldOfView = true;

				b2Vec2 newVel = entityInfo.Position - lastPos;
				iter->Velocity = newVel;
			}
		} break;
		default:
		{
			DEBUG_LogMessage("Unhandled entity type: " + std::to_string(entityInfo.Type) + "\n");
		} break;
		}
	}

	// Add new found houses to cache
	std::vector<HouseInfo> housesInFOV = FOV_GetHouses();
	for (size_t i = 0; i < housesInFOV.size(); i++)
	{
		if (!Contains(m_KnownHouses, housesInFOV[i]))
		{
			m_KnownHouses.push_back(housesInFOV[i]);
		}
	}

	if (!foundPistols.empty())
	{
		Pistol bestPistol = {};

		Item pistolSlot = GetItemFromInventory(m_BestPistolIndex);
		if (pistolSlot.valid)
		{
			// Compare new pistols to current one
			ConstructPistol(pistolSlot.info, {}, bestPistol);
		}

		float bestValue = 0.0f;
		auto bestPistolIter = foundPistols.end();
		for(auto iter = foundPistols.begin(); iter != foundPistols.end(); ++iter)
		{
			Pistol pistol = *iter;
			const float value = GetPistolValue(pistol);

			if (pistol.fresh && value > bestValue)
			{
				bestValue = value;
				bestPistol = pistol;
				bestPistolIter = iter;
			}
		}

		if (bestPistolIter != foundPistols.end())
		{
			m_BestPistolIndex = FirstEmptyInventorySlotID();
			AddItemToInventory(m_BestPistolIndex, bestPistol.itemInfo);
			foundPistols.erase(bestPistolIter);

			if (bestPistol.Range > m_LongestPistolRange)
			{
				m_LongestPistolRange = bestPistol.Range;
				m_LongestPistolRangeIndex = m_BestPistolIndex;
			}
		}
	}

	// If still not empty, we aren't picking them all up. Cache for later
	for (size_t i = 0; i < foundPistols.size(); i++)
	{
		if (Contains(m_KnownPistols, foundPistols[i]))
		{
			m_KnownPistols[i].fresh = true;
		}
		else
		{
			m_KnownPistols.push_back(foundPistols[i]);
		}
	}

	if (!foundHealthPacks.empty())
	{
		auto iter = foundHealthPacks.begin();
		while (iter != foundHealthPacks.end())
		{
			HealthPack healthPack = *iter;
			if (healthPack.fresh)
			{
				int firstEmptyInventorySlot = FirstEmptyInventorySlotID();
				if (firstEmptyInventorySlot == -1)
				{
					// TODO: Consider dropping something to make room for this
					++iter;
				}
				else
				{
					AddItemToInventory(firstEmptyInventorySlot, healthPack.itemInfo);
					iter = foundHealthPacks.erase(iter);
				}
			}
			else
			{
				// TODO: Consider going after health pack
			}
		}
	}

	// If still not empty, we aren't picking them all up. Cache for later
	for (size_t i = 0; i < foundHealthPacks.size(); i++)
	{
		if (Contains(m_KnownHealthPacks, foundHealthPacks[i]))
		{
			m_KnownHealthPacks[i].fresh = true;
		}
		else
		{
			m_KnownHealthPacks.push_back(foundHealthPacks[i]);
		}
	}

	if (!foundFood.empty())
	{
		auto iter = foundFood.begin();
		while (iter != foundFood.end())
		{
			Food food = *iter;
			if (food.fresh)
			{
				int firstEmptyInventorySlot = FirstEmptyInventorySlotID();
				if (firstEmptyInventorySlot == -1)
				{
					// TODO: Consider dropping something to make room for this
					++iter;
				}
				else
				{
					AddItemToInventory(firstEmptyInventorySlot, food.itemInfo);
					iter = foundFood.erase(iter);
				}
			}
			else
			{
				// TODO: Consider going after food
			}
		}
	}

	// If still not empty, we aren't picking them all up. Cache for later
	for (size_t i = 0; i < foundFood.size(); i++)
	{
		if (Contains(m_KnownFoodItems, foundFood[i]))
		{
			m_KnownFoodItems[i].fresh = true;
		}
		else
		{
			m_KnownFoodItems.push_back(foundFood[i]);
		}
	}


	if (agentInfo.Stamina == 10.0f)
	{
		agentInfo.RunMode = true;
	}
	else if (agentInfo.Stamina == 0.0f)
	{
		// Start regenerating stamina
		agentInfo.RunMode = false;
	}

	DEBUG_DrawCircle(agentInfo.Position, agentInfo.GrabRange, { 0, 0, 1 });
	DEBUG_DrawSolidCircle(m_Target.Position, 0.3f, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });

	Blackboard* pBlackboard = m_pBehaviourTree->GetBlackboard();
	pBlackboard->ChangeData("AgentInfo", &agentInfo);
	pBlackboard->ChangeData("KnownEnemies", m_KnownEnemies);
	pBlackboard->ChangeData("Target", m_Target);
	pBlackboard->ChangeData("Inventory", m_Inventory);
	pBlackboard->ChangeData("TargetSet", m_TargetSet);
	pBlackboard->ChangeData("LongestPistolRange", m_LongestPistolRange);


	m_pBehaviourTree->Update();

	
	pBlackboard->GetData("TargetSet", m_TargetSet);
	pBlackboard->GetData("CurrentBehaviour", m_CurrentSteeringBehaviour);
	bool shootPistol = false;
	pBlackboard->GetData("ShootPistol", shootPistol);
	if (shootPistol)
	{
		int targetEnemyHash;
		pBlackboard->GetData("TargetEnemyHash", targetEnemyHash);

		DEBUG_LogMessage("Shooting\n");
		UseItemInInventory(m_BestPistolIndex);
		pBlackboard->ChangeData("ShootPistol", false);

		// Check if you killed em (true unless they are still in front of us)
		bool enemyKilled = true;
		auto entitiesInFOVUpdated = FOV_GetEntities();
		for (size_t i = 0; i < entitiesInFOVUpdated.size(); i++)
		{
			if (entitiesInFOVUpdated[i].Type == eEntityType::ENEMY)
			{
				Enemy enemyInFOV;
				ConstructEnemy(entitiesInFOVUpdated[i], {}, enemyInFOV);
				if (enemyInFOV.info.EnemyHash == targetEnemyHash)
				{
					enemyKilled = false;
					break;
				}
			}
		}

		if (enemyKilled)
		{
			auto iter = m_KnownEnemies.begin();
			while (iter != m_KnownEnemies.end())
			{
				if (iter->info.EnemyHash == targetEnemyHash)
				{
					m_KnownEnemies.erase(iter);
					break;
				}
				else
				{
					++iter;
				}
			}
		}

		Item item = GetItemFromInventory(m_BestPistolIndex);
		Pistol pistol;
		ConstructPistol(item.info, {}, pistol);
		if (pistol.Ammo == 0)
		{
			RemoveItemFromInventory(m_BestPistolIndex);

			m_BestPistolIndex = -1;
			float bestValue = 0.0f;
			int bestPistolIndex = -1;
			// Check for other pistols to put in this slot
			for (size_t i = 1; i < m_Inventory.size(); i++)
			{
				if (m_Inventory[i].info.Type == eItemType::PISTOL)
				{
					Pistol pistol = {};
					ConstructPistol(m_Inventory[i].info, {}, pistol);
					const float value = GetPistolValue(pistol);
					if (value > bestValue)
					{
						bestValue = value;
						bestPistolIndex = i;
					}
				}
			}

			// Set to best pistol index, or -1 if no pistols remain
			m_BestPistolIndex = bestPistolIndex;

			if (m_BestPistolIndex == -1)
			{
				m_LongestPistolRangeIndex = -1;
				m_LongestPistolRange = 0.0f;
				pBlackboard->ChangeData("LongestPistolRange", 0.0f);
			}
		}
	}

	bool useHealthItem = false;
	pBlackboard->GetData("UseHealthItem", useHealthItem);
	if (useHealthItem)
	{
		pBlackboard->ChangeData("UseHealthItem", false);
		int healingNeeded = (int)(m_StartingHealth - agentInfo.Health);
		if (healingNeeded > 0)
		{
			HealthPack bestHealthPack = {};
			int bestHealthPackIndex = -1;

			for (size_t i = 0; i < m_Inventory.size(); i++)
			{
				if (m_Inventory[i].valid && m_Inventory[i].info.Type == eItemType::HEALTH)
				{
					HealthPack pack;
					ConstructHealthPack(m_Inventory[i].info, {}, pack);

					// Don't use packs that have more healing than we need
					if (pack.HealingAmount <= healingNeeded &&
						pack.HealingAmount > bestHealthPack.HealingAmount)
					{
						bestHealthPack = pack;
						bestHealthPackIndex = i;
					}
				}
			}

			if (bestHealthPackIndex != -1)
			{
				UseItemInInventory(bestHealthPackIndex);
				RemoveItemFromInventory(bestHealthPackIndex); // One time use item
			}
		}
	}

	bool useFoodItem = false;
	pBlackboard->GetData("UseFoodItem", useFoodItem);
	if (useFoodItem)
	{
		pBlackboard->ChangeData("UseFoodItem", false);
		int energyNeeded = (int)(m_StartingEnergy - agentInfo.Energy);
		if (energyNeeded > 0.0f)
		{
			Food bestFood = {};
			int bestFoodIndex = -1;

			for (size_t i = 0; i < m_Inventory.size(); i++)
			{
				if (m_Inventory[i].valid && m_Inventory[i].info.Type == eItemType::FOOD)
				{
					Food food;
					ConstructFood(m_Inventory[i].info, {}, food);

					// Don't use packs that have more healing than we need
					if (food.EnergyAmount <= energyNeeded &&
						food.EnergyAmount > bestFood.EnergyAmount)
					{
						bestFood = food;
						bestFoodIndex = i;
					}
				}
			}

			if (bestFoodIndex != -1)
			{
				UseItemInInventory(bestFoodIndex);
				RemoveItemFromInventory(bestFoodIndex); // One time use item
			}
		}
	}


	SteeringOutput steeringOutput = m_CurrentSteeringBehaviour->CalculateSteering(dt, agentInfo);

	PluginOutput output = {};
	output.RunMode = agentInfo.RunMode;
	output.LinearVelocity = steeringOutput.LinearVelocity;
	output.AngularVelocity = steeringOutput.AngularVelocity;

	for (size_t i = 0; i < m_KnownPistols.size(); i++)
	{
		m_KnownPistols[i].fresh = false;
	}
	for (size_t i = 0; i < m_KnownHealthPacks.size(); i++)
	{
		m_KnownHealthPacks[i].fresh = false;
	}
	for (size_t i = 0; i < m_KnownFoodItems.size(); i++)
	{
		m_KnownFoodItems[i].fresh = false;
	}

	return output;
}

//Extend the UI [ImGui call only!]
void TestBoxPlugin::ExtendUI_ImGui()
{
	if (!m_KnownEnemies.empty())
	{
		ImGui::Text("Known enemies:");
		for (size_t i = 0; i < m_KnownEnemies.size(); i++)
		{
			ImGui::Text("- Position: (%.2f, %.2f)\n  In FOV: %s",
				m_KnownEnemies[i].Position.x, m_KnownEnemies[i].Position.y, m_KnownEnemies[i].InFieldOfView ? "true" : "false");
		}
	}
	if (!m_KnownPistols.empty())
	{
		ImGui::Text("Known pistols:");
		for (size_t i = 0; i < m_KnownPistols.size(); i++)
		{
			ImGui::Text("- Position: (%.2f, %.2f)\n  Ammo: %i\n  DPS: %.2f\n  Range: %.2f",
				m_KnownPistols[i].Position.x, m_KnownPistols[i].Position.y,
				m_KnownPistols[i].Ammo, m_KnownPistols[i].DPS, m_KnownPistols[i].Range);
		}
	}
	if (!m_KnownHealthPacks.empty())
	{
		ImGui::Text("Known health:");
		for (size_t i = 0; i < m_KnownHealthPacks.size(); i++)
		{
			ImGui::Text("- Position: (%.2f, %.2f)\n  Healing: %i",
				m_KnownHealthPacks[i].Position.x, m_KnownHealthPacks[i].Position.y,
				m_KnownHealthPacks[i].HealingAmount);
		}
	}
	if (!m_KnownFoodItems.empty())
	{
		ImGui::Text("Known food:");
		for (size_t i = 0; i < m_KnownFoodItems.size(); i++)
		{
			ImGui::Text("- Position: (%.2f, %.2f)\n  Energy: %i",
				m_KnownFoodItems[i].Position.x, m_KnownFoodItems[i].Position.y,
				m_KnownFoodItems[i].EnergyAmount);
		}
	}
}

void TestBoxPlugin::End()
{
}

void TestBoxPlugin::LogOnFail(bool succeeded, const std::string& message)
{
	if (!succeeded)
	{
		DEBUG_LogMessage(message);
	}
}

bool TestBoxPlugin::DecideToPickUpItem(const ItemInfo& itemInfo)
{
	return false;
}

void TestBoxPlugin::AddItemToInventory(int slotID, const ItemInfo& itemInfo)
{
	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return;

	if (!m_Inventory[slotID].valid)
	{
		INVENTORY_AddItem(slotID, itemInfo);
		m_Inventory[slotID].info = itemInfo;
		m_Inventory[slotID].valid = true;
	}
}

void TestBoxPlugin::RemoveItemFromInventory(int slotID)
{
	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return;

	if (m_Inventory[slotID].valid)
	{
		INVENTORY_RemoveItem(slotID);
		m_Inventory[slotID].info = {};
		m_Inventory[slotID].valid = false;
	}
}

Item TestBoxPlugin::GetItemFromInventory(int slotID)
{
	Item invalidItem = {};
	invalidItem.valid = false;

	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return invalidItem;

	if (m_Inventory[slotID].valid)
	{
		ItemInfo newInfo;
		INVENTORY_GetItem(slotID, newInfo);

		m_Inventory[slotID].info.ItemHash = newInfo.ItemHash;
	}

	return m_Inventory[slotID];
}

void TestBoxPlugin::UseItemInInventory(int slotID)
{
	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return;

	if (m_Inventory[slotID].valid)
	{
		INVENTORY_UseItem(slotID);
	}
}

void TestBoxPlugin::ConstructPistol(const ItemInfo& itemInfo, b2Vec2 Position, Pistol& pistol)
{
	pistol.itemInfo = itemInfo;
	LogOnFail(ITEM_GetMetadata(itemInfo, "ammo", pistol.Ammo), "ammo metadata not found on pistol!\n");
	LogOnFail(ITEM_GetMetadata(itemInfo, "dps", pistol.DPS), "dps metadata not found on pistol!\n");
	LogOnFail(ITEM_GetMetadata(itemInfo, "range", pistol.Range), "range metadata not found on pistol!\n");
	pistol.Position = Position;
	pistol.fresh = true;
}

void TestBoxPlugin::ConstructHealthPack(const ItemInfo& itemInfo, b2Vec2 Position, HealthPack& healthPack)
{
	healthPack.itemInfo = itemInfo;
	LogOnFail(ITEM_GetMetadata(itemInfo, "health", healthPack.HealingAmount), "health metadata not found on health!\n");
	healthPack.Position = Position;
	healthPack.fresh = true;
}

void TestBoxPlugin::ConstructFood(const ItemInfo& itemInfo, b2Vec2 Position, Food& food)
{
	LogOnFail(ITEM_GetMetadata(itemInfo, "energy", food.EnergyAmount), "energy metadata not found on food!\n");
	food.itemInfo = itemInfo;
	food.Position = Position;
	food.fresh = true;
}

void TestBoxPlugin::ConstructEnemy(const EntityInfo& entityInfo, b2Vec2 Position, Enemy& enemy)
{
	EnemyInfo enemyInfo = {};
	ENEMY_GetInfo(entityInfo, enemyInfo);

	enemy.info = enemyInfo;
	enemy.Position = Position;
	enemy.LastPosition = Position;
	enemy.InFieldOfView = true;
}

float TestBoxPlugin::GetPistolValue(const Pistol& pistol)
{
	if (pistol.Ammo == 0 || pistol.DPS == 0 || pistol.Range == 0.0f)
	{
		// Don't pick up duds
		return 0.0f;
	}

	float value = 0.0f;
	value += pistol.DPS * 1.0f;
	value += pistol.Range * 0.5f;
	value += pistol.Ammo * 2.0f;

	return value;
}

int TestBoxPlugin::FirstEmptyInventorySlotID() const
{
	for (size_t i = 0; i < m_Inventory.size(); i++)
	{
		if (!m_Inventory[i].valid)
		{
			return i;
		}
	}

	return -1;
}

int TestBoxPlugin::EmptyInventorySlots() const
{
	int emptySlots = 0;

	for (size_t i = 0; i < m_Inventory.size(); i++)
	{
		if (!m_Inventory[i].valid)
		{
			++emptySlots;
		}
	}

	return emptySlots;
}

// [Optional] For Debugging
void TestBoxPlugin::ProcessEvents(const SDL_Event& e)
{
	switch (e.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	{
		if (e.button.button == SDL_BUTTON_LEFT)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			auto pos = b2Vec2(static_cast<float>(x), static_cast<float>(y));
			m_ClickGoal = DEBUG_ConvertScreenPosToWorldPos(pos);
			m_Target.Position = NAVMESH_GetClosestPathPoint(m_ClickGoal);
			m_TargetSet = true;
		}
	}
	break;
	case SDL_KEYDOWN:
	{
	}
	break;
	}
}
