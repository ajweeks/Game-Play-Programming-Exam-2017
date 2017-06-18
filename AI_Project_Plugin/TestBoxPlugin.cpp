
#include "stdafx.h"

#include "TestBoxPlugin.h"
#include "BehaviourTree.h"
#include "SteeringBehaviours.h"
#include "Behaviours.h"
#include "CombinedSB.h"

TestBoxPlugin::TestBoxPlugin():
	IBehaviourPlugin(GameDebugParams(20, false, false, false, false, 3.0f))
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
		m_Inventory[i].ItemInfo = {};
		m_Inventory[i].Valid = false;
	}

	AgentInfo agentInfo = AGENT_GetInfo(); //Contains all Agent Parameters, retrieved by copy!
	WorldInfo worldInfo = WORLD_GetInfo(); //Contains the location of the center of the world and the dimensions

	const b2Vec2 minWorldCoords = worldInfo.Center - worldInfo.Dimensions / 2.0f;
	const b2Vec2 maxWorldCoords = worldInfo.Center + worldInfo.Dimensions / 2.0f;

	int xCount = 3;
	int yCount = 3;
	float edgeBuffer = 45.0f;
	float searchPointOffsetX = minWorldCoords.x + edgeBuffer;
	float searchPointOffsetY = minWorldCoords.y + edgeBuffer;
	float adjustedWorldWidth = (worldInfo.Dimensions.x - edgeBuffer * 2.0f) / (float)(xCount - 1);
	float adjustedWorldHeight = (worldInfo.Dimensions.y - edgeBuffer * 2.0f) / (float)(yCount - 1);
	// Generate initial search points
	for (int i = 0; i < xCount; i++)
	{
		for (int j = 0; j < yCount; j++)
		{
			float x = i * adjustedWorldWidth + searchPointOffsetX;
			float y = j * adjustedWorldHeight + searchPointOffsetY;
			m_SearchPoints.push_back(b2Vec2(x, y));
		}
	}
	m_SearchPointIndex = 0;
	SteeringParams firstGoal;
	firstGoal.Position = m_SearchPoints[0];
	m_Goal = firstGoal;
	m_GoalSet = true;
	m_NextNavMeshGoal = NAVMESH_GetClosestPathPoint(m_Goal.Position);

	//Create behaviours
	auto pSeekBehaviour = new SteeringBehaviours::Seek();
	pSeekBehaviour->SetTarget(&m_NextNavMeshGoal);
	m_BehaviourVec.push_back(pSeekBehaviour);
	auto pWanderBehaviour = new SteeringBehaviours::Wander();
	m_BehaviourVec.push_back(pWanderBehaviour);
	auto pFleeBehaviour = new SteeringBehaviours::Flee();
	pFleeBehaviour->SetTarget(&m_NearestEnemy);
	m_BehaviourVec.push_back(pFleeBehaviour);
	//auto pSeekOtherSideOfMapBehaviour = new SteeringBehaviours::Seek();
	//pSeekOtherSideOfMapBehaviour->SetTarget(&m_OtherSideOfMapGoal);
	//m_BehaviourVec.push_back(pSeekOtherSideOfMapBehaviour);

	// Search outside behaviour
	std::vector<CombinedSB::BehaviourAndWeight> behavioursAndWeights;
	//behavioursAndWeights.push_back(CombinedSB::BehaviourAndWeight(pWanderBehaviour, 0.0f));
	behavioursAndWeights.push_back(CombinedSB::BehaviourAndWeight(pSeekBehaviour, 1.0f));
	behavioursAndWeights.push_back(CombinedSB::BehaviourAndWeight(pFleeBehaviour, 0.25f));
	m_pBlendedBehaviour = new CombinedSB::BlendedSteering(behavioursAndWeights);
	
	m_BehaviourVec.push_back(m_pBlendedBehaviour);

	m_CurrentSteeringBehaviour = m_pBlendedBehaviour;

	m_EmptyTargetEnemy = {};
	m_EmptyTargetEnemy.enemyInfo.EnemyHash = -1;
	
	//Create blackboard
	auto pBlackboard = new Blackboard;
	pBlackboard->AddData("AgentInfo", &agentInfo);
	pBlackboard->AddData("Goal", m_Goal);
	pBlackboard->AddData("NextNavMeshGoal", m_NextNavMeshGoal);
	pBlackboard->AddData("GoalSet", m_GoalSet);
	//pBlackboard->AddData("SoftGoal", m_SoftGoal);
	//pBlackboard->AddData("SoftGoalSet", m_SoftGoalSet);
	//pBlackboard->AddData("OutsideBehaviour", static_cast<SteeringBehaviours::ISteeringBehaviour*>(pOutsideHouseSteeringBehaviour));
	//pBlackboard->AddData("SeekItemBehaviour", static_cast<SteeringBehaviours::ISteeringBehaviour*>(pSeekItemSteeringBehaviour));
	pBlackboard->AddData("CurrentBehaviour", static_cast<SteeringBehaviours::ISteeringBehaviour*>(m_CurrentSteeringBehaviour));
	pBlackboard->AddData("Inventory", &m_Inventory);
	pBlackboard->AddData("MaxHealth", agentInfo.Health);
	pBlackboard->AddData("MaxEnergy", agentInfo.Energy);
	pBlackboard->AddData("LongestPistolRange", 0.0f);
	pBlackboard->AddData("TargetEnemy", m_EmptyTargetEnemy);
	pBlackboard->AddData("NearestEnemyPosition", &m_NearestEnemy);
	pBlackboard->AddData("AverageNearbyEnemyPostion", m_AverageNearbyEnemyPosition);
	pBlackboard->AddData("KnownItems", &m_KnownItems);
	pBlackboard->AddData("KnownHealthPacks", &m_KnownHealthPacks);
	pBlackboard->AddData("KnownFoodItems", &m_KnownFoodItems);
	pBlackboard->AddData("KnownPistols", &m_KnownPistols);
	pBlackboard->AddData("KnownEnemies", &m_KnownEnemies);
	pBlackboard->AddData("KnownHouses", &m_KnownHouses);
	pBlackboard->AddData("NextHouseIndex", m_NextHouseIndex);
	pBlackboard->AddData("SecondsBetweenHouseRevisits", m_SecondsBetweenHouseRevisits);
	pBlackboard->AddData("InsideHouse", false);
	pBlackboard->AddData("WorldMinCoords", minWorldCoords);
	pBlackboard->AddData("WorldMaxCoords", maxWorldCoords);

	pBlackboard->AddData("SearchPoints", &m_SearchPoints);
	pBlackboard->AddData("SearchPointIndex", m_SearchPointIndex);

	// Flags that behaviours can set to send info back to this class
	pBlackboard->AddData("ShootPistol", false);
	pBlackboard->AddData("UseHealthItem", false);
	pBlackboard->AddData("UseFoodItem", false);

	m_pBehaviourTree = new BehaviourTree(pBlackboard,
	new BehaviourSelector
	({
		new BehaviourSequence // Set GOAL to false upon arrival
		({
			new BehaviourConditional(IsGoalSet),
			new BehaviourConditional(HasReachedGoal),
			new BehaviourAction(SetGoalSetFalse)
		}),
		new BehaviourSequence // Update SEARCH POINT INDEX
		({
			new BehaviourConditionalInverse(MapSearchedEntirely),
			new BehaviourConditional(ArrivedAtNextSearchPoint),
			new BehaviourAction(IncrementSearchPoint)
		}),
		new BehaviourSequence // Use FOOD
		({
			new BehaviourConditional(NotMaxEnergy),
			new BehaviourConditional(HasFoodItem),
			new BehaviourAction(UseFoodItem)
		}),
		new BehaviourSequence // Use HEALTH
		({
			new BehaviourConditional(NotMaxHealth),
			new BehaviourConditional(HasHealthItem),
			new BehaviourAction(UseHealthItem)
		}),
		new BehaviourSequence // Find ITEMS
		({
			new BehaviourConditional(HaveInventorySpace),
			new BehaviourConditional(KnowOfItemsOnGround),
			new BehaviourAction(SetNearestItemAsGoal)
		}),
		new BehaviourSequence // Find ITEMS (even with no inventory space, we can trade things out)
		({
			new BehaviourConditional(LowEnergyOrHealth),
			new BehaviourConditional(KnowOfItemsOnGround),
			new BehaviourAction(SetNearestItemAsGoal)
		}),
		new BehaviourSequence // Explore unexplored HOUSES
		({
			new BehaviourConditional(KnowOfUnexploredHouse),
			new BehaviourAction(SetGoalToNearestUnexploredHouse)
		}),
		new BehaviourSequence // Shoot ENEMIES
		({
			new BehaviourConditional(HasLoadedPistol),
			new BehaviourConditional(HasEnemyInRange),
			new BehaviourConditional(HasEnemyInFOV),
			new BehaviourAction(AimAtNearestEnemyInFOV)
		}),
		new BehaviourSequence // Search entire map
		({
			new BehaviourConditionalInverse(MapSearchedEntirely),
			new BehaviourAction(SetGoalToNextSearchPoint)
		}),
		new BehaviourConditionalInverse(MapSearchedEntirely), // Don't go any further if map hasn't been fully searched
		new BehaviourConditional(IsGoalSet), // Don't go any further if a goal is set
		new BehaviourAction(SetGoalToNextHouse)
	}));

	m_StartingHealth = agentInfo.Health;
	m_StartingEnergy = agentInfo.Energy;

	m_SecondsElapsed = 0.0f;
	m_SecondsSinceNavMeshTargetUpdate = 0.0f;
}

PluginOutput TestBoxPlugin::Update(float dt)
{
	m_SecondsElapsed += dt;

	WorldInfo worldInfo = WORLD_GetInfo(); //Contains the location of the center of the world and the dimensions
	AgentInfo agentInfo = AGENT_GetInfo(); //Contains all Agent Parameters, retrieved by copy!

	b2Vec2 minCoords = worldInfo.Center - worldInfo.Dimensions / 2.0f;
	b2Vec2 maxCoords = worldInfo.Center + worldInfo.Dimensions / 2.0f;

	auto iter = m_KnownEnemies.begin();
	while (iter != m_KnownEnemies.end())
	{
		iter->InFieldOfView = false;
		iter->SecondsSinceInsideFOV += dt;
		iter->PredictedPosition += iter->Velocity * dt;

		if (iter->SecondsSinceInsideFOV > m_SecondsToEstimateEnemyPositionsFor)
		{
			iter = m_KnownEnemies.erase(iter);
		}
		else
		{
			++iter;
		}
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
			bool addedToList = false; // If this isn't set to true, this item is added to our list of unknown items
			if (b2Distance(entityInfo.Position, agentInfo.Position) < (agentInfo.GrabRange * 0.75f)) // Use smaller grab range because sometimes it doesn't think you're in it
			{
				ItemInfo itemInfo = {};
				ITEM_Grab(entityInfo, itemInfo);

				switch (itemInfo.Type)
				{
				case eItemType::PISTOL:
				{
					Pistol pistol = {};
					ConstructPistol(entityInfo, itemInfo, entityInfo.Position, pistol);
					if (pistol.Ammo > 0 && pistol.DPS > 0 && pistol.Range > 0 && !Contains(foundPistols, pistol))
					{
						foundPistols.push_back(pistol);
						addedToList = true;
					}
				} break;
				case eItemType::HEALTH:
				{
					HealthPack healthPack = {};
					ConstructHealthPack(entityInfo, itemInfo, entityInfo.Position, healthPack);
					if (healthPack.HealingAmount > 0 && !Contains(foundHealthPacks, healthPack))
					{
						foundHealthPacks.push_back(healthPack);
						addedToList = true;
					}
				} break;
				case eItemType::FOOD:
				{
					Food food = {};
					ConstructFood(entityInfo, itemInfo, entityInfo.Position, food);

					if (food.EnergyAmount > 0 && !Contains(foundFood, food))
					{
						foundFood.push_back(food);
						addedToList = true;
					}
				} break;
				case eItemType::GARBAGE:
				{
					int firstFreeInventorySlot = FirstEmptyInventorySlotID();
					if (firstFreeInventorySlot != -1)
					{
						// If we have a space for it, pick up garbage then remvoe it
						// To get rid of the distraction
						AddItemToInventory(firstFreeInventorySlot, entityInfo, itemInfo);
						RemoveItemFromInventory(firstFreeInventorySlot);
						DEBUG_LogMessage("Disposing of garbage\n");
					}

					// We didn't actually add it to any list, but still
					// set this so we don't add it to our list of items
					addedToList = true;

					// We added this to our list before we knew it was garbage
					// Remove it
					RemoveFromKnownItems(entityInfo); 

				} break;
				default:
					DEBUG_LogMessage("Unhandled item type: " + std::to_string(itemInfo.Type) + "\n");
					break;
				}
			}

			if (!addedToList)
			{
				if (!Contains(m_KnownItems, entityInfo))
				{
					m_KnownItems.push_back(entityInfo);
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
				// We already know about this enemy, update our info on it
				Enemy& knownEnemy = *iter;
				b2Vec2 lastPos = knownEnemy.Position;
				knownEnemy.enemyInfo.Health = enemy.enemyInfo.Health;
				knownEnemy.Position = entityInfo.Position;
				knownEnemy.PredictedPosition = entityInfo.Position;
				knownEnemy.LastPosition = lastPos;
				knownEnemy.InFieldOfView = true;
				knownEnemy.SecondsSinceInsideFOV = 0.0f;

				b2Vec2 newVel = entityInfo.Position - lastPos;
				knownEnemy.Velocity = newVel;
			}
		} break;
		default:
		{
			DEBUG_LogMessage("Unhandled entity type: " + std::to_string(entityInfo.Type) + "\n");
		} break;
		}
	}

	// Check if any known items are not in our FOV but we still think they're there
	//if (!m_KnownItems.empty())
	//{
	//	auto iter = m_KnownItems.begin();
	//	while (iter != m_KnownItems.end())
	//	{
	//		EntityInfo& knownItem = *iter;
	//		if (!Contains(entitiesInFOV, knownItem) && PointInFOV(knownItem.Position, agentInfo))
	//		{
	//			iter = m_KnownItems.erase(iter);
	//		}
	//		else
	//		{
	//			++iter;
	//		}
	//	}
	//}

	// Add new found houses to cache
	std::vector<HouseInfo> housesInFOV = FOV_GetHouses();
	for (size_t i = 0; i < housesInFOV.size(); i++)
	{
		House house;
		ConstructHouse(housesInFOV[i], house);

		if (!Contains(m_KnownHouses, house))
		{
			m_KnownHouses.push_back(house);
		}
	}

	DetermineInHouseIndex(agentInfo.Position);
	if (m_InHouseIndex != -1) 
	{
		m_KnownHouses[m_InHouseIndex].SecondsSinceLastVisit = 0.0f;
	}

	for (size_t i = 0; i < m_KnownHouses.size(); i++)
	{
		if (m_InHouseIndex != i)
		{
			m_KnownHouses[i].SecondsSinceLastVisit += dt;
		}
	}

	if (!foundFood.empty())
	{
		auto iter = foundFood.begin();
		while (iter != foundFood.end())
		{
			Food food = *iter;
			if (food.Fresh)
			{
				int firstEmptyInventorySlot = FirstEmptyInventorySlotID();
				if (firstEmptyInventorySlot == -1)
				{
					const int foodSlotIndex = InventoryFirstSlotWithItemType(eItemType::FOOD);
					// Consider dropping something if we have not food and need energy
					if (foodSlotIndex == -1 && agentInfo.Energy < m_StartingEnergy)
					{
						int pistolSlotIndex = InventoryFirstSlotWithItemType(eItemType::PISTOL);
						if (pistolSlotIndex != -1)
						{
							DEBUG_LogMessage("Dropping pistol to make room for food");
							RemoveItemFromInventory(pistolSlotIndex);
							firstEmptyInventorySlot = pistolSlotIndex;
						}
						else
						{
							int healthSlotIndex = InventoryFirstSlotWithItemType(eItemType::HEALTH);
							if (healthSlotIndex != -1)
							{
								DEBUG_LogMessage("Dropping health pack to make room for food");
								RemoveItemFromInventory(healthSlotIndex);
								firstEmptyInventorySlot = healthSlotIndex;
							}
							else
							{
								assert(false); // If our inventory doesn't have any food, or pistols, it better have health
								++iter;
							}
						}
					}
					else
					{
						RemoveItemFromInventory(foodSlotIndex);
						firstEmptyInventorySlot = foodSlotIndex;
					}
				}

				if (firstEmptyInventorySlot != -1)
				{
					AddItemToInventory(firstEmptyInventorySlot, food.EntityInfo, food.ItemInfo);
					iter = foundFood.erase(iter);
				}
			}
			else
			{
				++iter;
			}
		}
	}

	// If still not empty, we aren't picking them all up. Cache for later
	for (size_t i = 0; i < foundFood.size(); i++)
	{
		if (Contains(m_KnownFoodItems, foundFood[i]))
		{
			m_KnownFoodItems[i].Fresh = true;
		}
		else
		{
			m_KnownFoodItems.push_back(foundFood[i]);
			RemoveFromKnownItems(foundHealthPacks[i].EntityInfo);
		}
	}

	if (!foundHealthPacks.empty())
	{
		auto iter = foundHealthPacks.begin();
		while (iter != foundHealthPacks.end())
		{
			HealthPack healthPack = *iter;
			if (healthPack.Fresh)
			{
				int firstEmptyInventorySlot = FirstEmptyInventorySlotID();
				if (firstEmptyInventorySlot == -1)
				{
					// Consider dropping something if we have no health but need some
					const int healthSlotIndex = InventoryFirstSlotWithItemType(eItemType::HEALTH);
					if (healthSlotIndex == -1 && agentInfo.Health < m_StartingHealth)
					{
						const int pistolSlotIndex = InventoryFirstSlotWithItemType(eItemType::PISTOL);
						if (pistolSlotIndex != -1)
						{
							DEBUG_LogMessage("Dropping pistol to make room for health");
							RemoveItemFromInventory(pistolSlotIndex);
							firstEmptyInventorySlot = pistolSlotIndex;
						}
						else
						{
							const int foodSlotIndex = InventoryFirstSlotWithItemType(eItemType::FOOD);
							if (foodSlotIndex != -1)
							{
								DEBUG_LogMessage("Dropping food to make room for health");
								RemoveItemFromInventory(foodSlotIndex);
								firstEmptyInventorySlot = foodSlotIndex;
							}
							else
							{
								assert(false); // If our inventory doesn't have any health, or pistols, it better have food
								++iter;
							}
						}
					}
					else
					{
						// Trade with the one we have already just to get it off the ground
						RemoveItemFromInventory(healthSlotIndex);
						firstEmptyInventorySlot = healthSlotIndex;
					}
				}

				if (firstEmptyInventorySlot != -1)
				{
					AddItemToInventory(firstEmptyInventorySlot, healthPack.EntityInfo, healthPack.ItemInfo);
					iter = foundHealthPacks.erase(iter);
				}
			}
			else
			{
				++iter;
			}
		}
	}

	// If still not empty, we aren't picking them all up. Cache for later
	for (size_t i = 0; i < foundHealthPacks.size(); i++)
	{
		if (Contains(m_KnownHealthPacks, foundHealthPacks[i]))
		{
			m_KnownHealthPacks[i].Fresh = true;
		}
		else
		{
			m_KnownHealthPacks.push_back(foundHealthPacks[i]);
			RemoveFromKnownItems(foundHealthPacks[i].EntityInfo);
		}
	}

	if (!foundPistols.empty())
	{
		Pistol bestPistol = {};

		Item currentBestPistol = GetItemFromInventory(m_BestPistolIndex);
		if (currentBestPistol.Valid)
		{
			// Compare new pistols to current one
			ConstructPistol(currentBestPistol.EntityInfo, currentBestPistol.ItemInfo, {}, bestPistol);
		}

		float bestValue = 0.0f;
		auto bestPistolIter = foundPistols.end();
		for (auto iter = foundPistols.begin(); iter != foundPistols.end(); ++iter)
		{
			Pistol pistol = *iter;
			const float value = pistol.GetValue();

			if (pistol.Fresh && value > bestValue)
			{
				bestValue = value;
				bestPistol = pistol;
				bestPistolIter = iter;
			}
		}

		if (bestPistolIter != foundPistols.end())
		{
			int previousBestPistolIndex = m_BestPistolIndex;
			if (previousBestPistolIndex != -1)
			{
				RemoveItemFromInventory(m_BestPistolIndex);
			}
			m_BestPistolIndex = FirstEmptyInventorySlotID();
			AddItemToInventory(m_BestPistolIndex, bestPistol.entityInfo, bestPistol.itemInfo);
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
			m_KnownPistols[i].Fresh = true;
		}
		else
		{
			m_KnownPistols.push_back(foundPistols[i]);
		}
	}


	//if (agentInfo.Stamina >= 9.9f)
	//{
	//	agentInfo.RunMode = true;
	//}
	//else if (agentInfo.Stamina <= 0.1f)
	//{
		// Start regenerating stamina
		agentInfo.RunMode = false;
	//}

	//if (b2Distance(m_NearestEnemy.Position, agentInfo.Position) < agentInfo.FOV_Range)
	{
		//DEBUG_LogMessage("flee weight: %.2f\n", m_FleeWeightNearEnemies);
		//m_pBlendedBehaviour->SetBehaviourWeight(m_FleeBehaviourWeightPairIndex, m_FleeWeightNearEnemies);
	}
	//else
	{
		//DEBUG_LogMessage("flee weight: %.2f\n", m_FleeWeightNotNearEnemies);
		//m_pBlendedBehaviour->SetBehaviourWeight(m_FleeBehaviourWeightPairIndex, m_FleeWeightNotNearEnemies);
	}

	Blackboard* pBlackboard = m_pBehaviourTree->GetBlackboard();
	pBlackboard->ChangeData("AgentInfo", &agentInfo);
	pBlackboard->ChangeData("Inventory", &m_Inventory);
	pBlackboard->ChangeData("LongestPistolRange", m_LongestPistolRange);
	pBlackboard->ChangeData("KnownItems", &m_KnownItems);
	pBlackboard->ChangeData("KnownHealthPacks", &m_KnownHealthPacks);
	pBlackboard->ChangeData("KnownFoodItems", &m_KnownFoodItems);
	pBlackboard->ChangeData("KnownPistols", &m_KnownPistols);
	pBlackboard->ChangeData("KnownEnemies", &m_KnownEnemies);
	pBlackboard->ChangeData("KnownHouses", &m_KnownHouses);
	pBlackboard->ChangeData("InsideHouse", m_InHouseIndex != -1);
	pBlackboard->ChangeData("TargetEnemy", m_EmptyTargetEnemy);

	m_pBehaviourTree->Update();

	bool goalWasSet = m_GoalSet;
	pBlackboard->GetData("GoalSet", m_GoalSet);


	m_SecondsSinceNavMeshTargetUpdate += dt;

	if (m_GoalSet)
	{
		pBlackboard->GetData("Goal", m_Goal);
		float distSqr = b2DistanceSquared(agentInfo.Position, m_Goal.Position);
		if (distSqr < 1.0f || 
			m_GoalSet != goalWasSet || 
			m_SecondsSinceNavMeshTargetUpdate > m_SecondsBetweenNavMeshTargetUpdates)
		{
			m_SecondsSinceNavMeshTargetUpdate = 0.0f;
			m_NextNavMeshGoal = NAVMESH_GetClosestPathPoint(m_Goal.Position);
			pBlackboard->ChangeData("NextNavMeshGoal", m_NextNavMeshGoal);
		}

		DEBUG_DrawCircle(agentInfo.Position, agentInfo.GrabRange, { 0.0f, 0.0f, 1.0f });
		DEBUG_DrawSolidCircle(m_Goal.Position, 0.4f, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });
		if (m_NextNavMeshGoal != m_Goal)
		{
			DEBUG_DrawSolidCircle(m_NextNavMeshGoal.Position, 0.4f, { 0.0f, 0.0f }, { 0.0f, 0.5f, 0.5f });
		}
	}

	// Draw debuging helpers
	for (size_t i = 0; i < m_KnownEnemies.size(); i++)
	{
		if (!m_KnownEnemies[i].InFieldOfView)
		{
			DEBUG_DrawCircle(m_KnownEnemies[i].PredictedPosition, 1.5f, { 1.0f, 0.1f, 0.1f , 0.1f });
		}
	}

	pBlackboard->GetData("NextHouseIndex", m_NextHouseIndex);
	pBlackboard->GetData("SearchPointIndex", m_SearchPointIndex);

	for (size_t i = 0; i < m_KnownHouses.size(); i++)
	{
		HouseInfo info = m_KnownHouses[i].Info;
		
		b2Vec2 points[4] = {
			b2Vec2(info.Center.x - (info.Size.x / 2.0f + 2.0f), info.Center.y - (info.Size.y / 2.0f + 2.0f)),
			b2Vec2(info.Center.x - (info.Size.x / 2.0f + 2.0f), info.Center.y + (info.Size.y / 2.0f + 2.0f)),
			b2Vec2(info.Center.x + (info.Size.x / 2.0f + 2.0f), info.Center.y + (info.Size.y / 2.0f + 2.0f)),
			b2Vec2(info.Center.x + (info.Size.x / 2.0f + 2.0f), info.Center.y - (info.Size.y / 2.0f + 2.0f))
		};

		b2Color color;
		if (i == m_NextHouseIndex && m_SearchPointIndex == m_SearchPoints.size())
		{
			color = fmod(m_SecondsElapsed, 1.0f) > 0.5f ? b2Color(0.5f, 0.34f, 0.3f) : b2Color(0.7f, 0.56f, 0.5f);
		}
		else
		{
			color = b2Color(0.12f, 0.10f, 0.08f);
		}
		DEBUG_DrawSolidPolygon(points, 4, color, 1.0f);
		// Broken:
		//DEBUG_DrawString(info.Center + info.Size / 2.0f + b2Vec2(2.0f, 2.0f), "%i", i);
	}

	for (int i = 0; i < (int)m_SearchPoints.size(); i++)
	{
		b2Color color;
		if (m_SearchPointIndex > i)
		{
			color = b2Color(0.9f, 0.9f, 0.85f);
		}
		else if (m_SearchPointIndex == i)
		{
			color = fmod(m_SecondsElapsed, 1.0f) > 0.5f ? b2Color(0.9f, 0.9f, 0.4f) : b2Color(0.6f, 0.6f, 0.1f);
		}
		else
		{
			color = b2Color(0.95f, 0.18f, 0.78f);
		}

		DEBUG_DrawCircle(m_SearchPoints[i], 3.0f, color);
		DEBUG_DrawCircle(m_SearchPoints[i], 2.0f, color);
		DEBUG_DrawCircle(m_SearchPoints[i], 1.0f, color);
	}

	for (size_t i = 0; i < m_KnownItems.size(); i++)
	{
		DEBUG_DrawCircle(m_KnownItems[i].Position, 1.0f, { 1.0f, 1.0f, 0.92f });
	}

	float angularSteering = 0.0f;
	bool overrideAutoOrient = true;

	Enemy targetEnemy;
	pBlackboard->GetData("TargetEnemy", targetEnemy);
	if (targetEnemy.enemyInfo.EnemyHash != m_EmptyTargetEnemy.enemyInfo.EnemyHash)
	{
		overrideAutoOrient = false;

		b2Vec2 dPos = targetEnemy.Position - agentInfo.Position;

		float currOrientation = agentInfo.Orientation;
		float targetOrientation = GetOrientationFromVelocity(dPos);
		angularSteering = targetEnemy.Orientation - currOrientation;

		bool shootPistol = false;

		float dist = b2Distance(agentInfo.Position, targetEnemy.Position);
		b2Vec2 forward = b2Vec2(cos(agentInfo.Orientation), sin(agentInfo.Orientation));
		b2Vec2 testPoint = agentInfo.Position + forward * dist; // Slightly fishy calculation, but works well

		if (shootPistol)
		{
			DEBUG_LogMessage("----Shooting pistol\n");
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
					if (enemyInFOV.enemyInfo.EnemyHash == targetEnemy.enemyInfo.EnemyHash)
					{
						enemyKilled = false;
						break;
					}
				}
			}

			if (enemyKilled)
			{
				DEBUG_LogMessage("----Killed enemy!\n");
				auto iter = m_KnownEnemies.begin();
				while (iter != m_KnownEnemies.end())
				{
					if (iter->enemyInfo.EnemyHash == targetEnemy.enemyInfo.EnemyHash)
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
			ConstructPistol(item.EntityInfo, item.ItemInfo, {}, pistol);
			if (pistol.Ammo == 0)
			{
				RemoveItemFromInventory(m_BestPistolIndex);

				m_BestPistolIndex = -1;
				float bestValue = 0.0f;
				int bestPistolIndex = -1;
				// Check for other pistols to put in this slot
				for (size_t i = 1; i < m_Inventory.size(); i++)
				{
					if (m_Inventory[i].ItemInfo.Type == eItemType::PISTOL)
					{
						Pistol pistol = {};
						ConstructPistol(m_Inventory[i].EntityInfo, m_Inventory[i].ItemInfo, {}, pistol);
						const float value = pistol.GetValue();
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
				if (m_Inventory[i].Valid && m_Inventory[i].ItemInfo.Type == eItemType::HEALTH)
				{
					HealthPack pack;
					ConstructHealthPack(m_Inventory[i].EntityInfo, m_Inventory[i].ItemInfo, {}, pack);

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
				DEBUG_LogMessage("----Using health pack\n");
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
				if (m_Inventory[i].Valid && m_Inventory[i].ItemInfo.Type == eItemType::FOOD)
				{
					Food food;
					ConstructFood(m_Inventory[i].EntityInfo, m_Inventory[i].ItemInfo, {}, food);

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
				DEBUG_LogMessage("----Eating food\n");
				UseItemInInventory(bestFoodIndex);
				RemoveItemFromInventory(bestFoodIndex); // One time use item
			}
		}
	}


	pBlackboard->GetData("CurrentBehaviour", m_CurrentSteeringBehaviour);
	SteeringOutput steeringOutput = m_CurrentSteeringBehaviour->CalculateSteering(dt, agentInfo);


	PluginOutput output = {};
	output.RunMode = agentInfo.RunMode;
	output.LinearVelocity = steeringOutput.LinearVelocity;
	output.AngularVelocity = overrideAutoOrient ? angularSteering : steeringOutput.AngularVelocity;
	output.AutoOrientate = overrideAutoOrient;

	for (size_t i = 0; i < m_KnownPistols.size(); i++)
	{
		m_KnownPistols[i].Fresh = false;
	}
	for (size_t i = 0; i < m_KnownHealthPacks.size(); i++)
	{
		m_KnownHealthPacks[i].Fresh = false;
	}
	for (size_t i = 0; i < m_KnownFoodItems.size(); i++)
	{
		m_KnownFoodItems[i].Fresh = false;
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

void TestBoxPlugin::AddItemToInventory(int slotID, const EntityInfo& entityInfo, const ItemInfo& itemInfo)
{
	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return;

	if (!m_Inventory[slotID].Valid)
	{
		INVENTORY_AddItem(slotID, itemInfo);
		m_Inventory[slotID].EntityInfo = entityInfo;
		m_Inventory[slotID].ItemInfo = itemInfo;
		m_Inventory[slotID].Valid = true;

		RemoveFromKnownItems(m_Inventory[slotID].EntityInfo);
	}
}

void TestBoxPlugin::RemoveItemFromInventory(int slotID)
{
	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return;

	if (m_Inventory[slotID].Valid)
	{
		RemoveFromKnownItems(m_Inventory[slotID].EntityInfo);

		INVENTORY_RemoveItem(slotID);
		m_Inventory[slotID].ItemInfo = {};
		m_Inventory[slotID].Valid = false;
	}
}

Item TestBoxPlugin::GetItemFromInventory(int slotID)
{
	Item invalidItem = {};
	invalidItem.Valid = false;

	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return invalidItem;

	if (m_Inventory[slotID].Valid)
	{
		ItemInfo newInfo;
		INVENTORY_GetItem(slotID, newInfo);

		m_Inventory[slotID].ItemInfo = newInfo;
	}

	return m_Inventory[slotID];
}

void TestBoxPlugin::UseItemInInventory(int slotID)
{
	if (slotID < 0 || slotID >= (int)m_Inventory.size()) return;

	if (m_Inventory[slotID].Valid)
	{
		INVENTORY_UseItem(slotID);
	}
}

int TestBoxPlugin::InventoryItemCount(eItemType itemType)
{
	int count = 0;

	for (size_t i = 0; i < m_Inventory.size(); i++)
	{
		if (m_Inventory[i].ItemInfo.Type == itemType)
		{
			++count;
		}
	}

	return count;
}

int TestBoxPlugin::InventoryFirstSlotWithItemType(eItemType itemType)
{
	for (int i = 0; i < (int)m_Inventory.size(); i++)
	{
		if (m_Inventory[i].ItemInfo.Type == itemType)
		{
			return i;
		}
	}

	return -1;
}

bool TestBoxPlugin::PointInFOV(const b2Vec2& point, const AgentInfo& agentInfo)
{
	float dist = b2Distance(point, agentInfo.Position);
	if (dist > agentInfo.FOV_Range)
	{
		return false;
	}

	b2Vec2 dPos = point - agentInfo.Position;
	b2Vec2 facingDir = b2Vec2(cos(agentInfo.Orientation), sin(agentInfo.Orientation));
	float dot = b2Dot(facingDir, dPos);
	dot = Clamp(dot, -1.0f, 1.0f);

	bool inFOV = acos(dot) < agentInfo.FOV_Angle;

	return inFOV;
}

void TestBoxPlugin::ConstructPistol(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, Pistol& pistol)
{
	pistol.entityInfo = entityInfo;
	pistol.itemInfo = itemInfo;
	LogOnFail(ITEM_GetMetadata(itemInfo, "ammo", pistol.Ammo), "ammo metadata not found on pistol!\n");
	LogOnFail(ITEM_GetMetadata(itemInfo, "dps", pistol.DPS), "dps metadata not found on pistol!\n");
	LogOnFail(ITEM_GetMetadata(itemInfo, "range", pistol.Range), "range metadata not found on pistol!\n");
	pistol.Position = Position;
	pistol.Fresh = true;
}

void TestBoxPlugin::ConstructHealthPack(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, HealthPack& healthPack)
{
	healthPack.EntityInfo = entityInfo;
	healthPack.ItemInfo = itemInfo;
	LogOnFail(ITEM_GetMetadata(itemInfo, "health", healthPack.HealingAmount), "health metadata not found on health!\n");
	healthPack.Position = Position;
	healthPack.Fresh = true;
}

void TestBoxPlugin::ConstructFood(const EntityInfo& entityInfo, const ItemInfo& itemInfo, b2Vec2 Position, Food& food)
{
	LogOnFail(ITEM_GetMetadata(itemInfo, "energy", food.EnergyAmount), "energy metadata not found on food!\n");
	food.EntityInfo = entityInfo;
	food.ItemInfo = itemInfo;
	food.Position = Position;
	food.Fresh = true;
}

void TestBoxPlugin::ConstructEnemy(const EntityInfo& entityInfo, b2Vec2 Position, Enemy& enemy)
{
	EnemyInfo enemyInfo = {};
	ENEMY_GetInfo(entityInfo, enemyInfo);

	enemy.enemyInfo = enemyInfo;
	enemy.Position = Position;
	enemy.LastPosition = Position;
	enemy.InFieldOfView = true;
}

void TestBoxPlugin::ConstructHouse(const HouseInfo& houseInfo, House& house)
{
	house.Info = houseInfo;
	house.SecondsSinceLastVisit = m_SecondsBetweenHouseRevisits + 1.0f; // Flag so we know to go in here
	house.Unexplored = true;
}

//int TestBoxPlugin::InventoryItemCount(eItemType itemType)
//{
//	int count = 0;
//
//	for (size_t i = 0; i < m_Inventory.size(); i++)
//	{
//		if (m_Inventory[i].itemInfo.Type == itemType)
//		{
//			++count;
//		}
//	}
//
//	return count;
//}

int TestBoxPlugin::FirstEmptyInventorySlotID() const
{
	for (size_t i = 0; i < m_Inventory.size(); i++)
	{
		if (!m_Inventory[i].Valid)
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
		if (!m_Inventory[i].Valid)
		{
			++emptySlots;
		}
	}

	return emptySlots;
}

void TestBoxPlugin::RemoveFromKnownItems(const EntityInfo& entityInfo)
{
	for (auto iter = m_KnownItems.begin(); iter != m_KnownItems.end(); ++iter)
	{
		if (iter->EntityHash == entityInfo.EntityHash)
		{
			iter = m_KnownItems.erase(iter);
			return;
		}
	}
}

void TestBoxPlugin::DetermineInHouseIndex(const b2Vec2& agentPos)
{
	for (size_t i = 0; i < m_KnownHouses.size(); i++)
	{
		if (PointInAABB(agentPos, m_KnownHouses[i].Info.Center, m_KnownHouses[i].Info.Size))
		{
			m_KnownHouses[i].Unexplored = false;
			m_InHouseIndex = i;
			return;
		}
	}
	m_InHouseIndex = -1;
}

// [Optional] For Debugging
void TestBoxPlugin::ProcessEvents(const SDL_Event& e)
{
}
