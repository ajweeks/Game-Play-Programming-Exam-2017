#pragma once

#include "../AI_Project_Plugin/HelperStructs.h"

#include <string>
#include <vector>

class IBehaviourPlugin
{
public:
	IBehaviourPlugin(GameDebugParams params);
	virtual ~IBehaviourPlugin();

	void UpdateInternal(float dt);
	void RenderInternal(float dt);

	virtual void Start() = 0;
	virtual PluginOutput Update(float dt) = 0;
	virtual void End() = 0;
	virtual void ExtendUI_ImGui() {};
	virtual void ProcessEvents(const SDL_Event& e){} //Debugging Only! [Deactivated in release build ;) ]

	//Callable Functions
	//******************

	//INVENTORY
	bool INVENTORY_AddItem(int slotId, ItemInfo item);
	bool INVENTORY_UseItem(int slotId);
	bool INVENTORY_RemoveItem(int slotId);
	bool INVENTORY_GetItem(int slotId, ItemInfo& item);
	int INVENTORY_GetCapacity() const;

	//WORLD INFO
	WorldInfo WORLD_GetInfo() const;

	//FOV (Field Of View)
	std::vector<EntityInfo> FOV_GetEntities() const;
	//Get all the houses you can see. If you are in a house, you only see the house you are standing in!
	//You need to exit the current house entirely to regain full vision (see all houses outside)
	std::vector<HouseInfo> FOV_GetHouses() const;

	//ITEM
	template<typename T>
	bool ITEM_GetMetadata(ItemInfo item, std::string category, T& val)
	{
		CheapVariant varVal;
		if(GetItemMeta(item, category, varVal))
		{
			val = (T)varVal;
			return true;
		}

		return false;
	}

	bool ITEM_Grab(EntityInfo entity, ItemInfo& item);

	//ENEMY
	bool ENEMY_GetInfo(EntityInfo entity, EnemyInfo& enemy);
	
	//MISC
	b2Vec2 NAVMESH_GetClosestPathPoint(b2Vec2 goal) const;
	AgentInfo AGENT_GetInfo() const;

	//DEBUG HELPERS (Only work during Debug Build ;) )
	b2Vec2 DEBUG_ConvertScreenPosToWorldPos(b2Vec2 screenPos);
	void DEBUG_LogMessage(std::string message, ...);
	void DEBUG_DrawPoint(const b2Vec2& p, float size, const b2Color& color);
	void DEBUG_DrawCircle(const b2Vec2& center, float radius, const b2Color& color);
	void DEBUG_DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
	void DEBUG_DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
	void DEBUG_DrawSolidPolygon(const b2Vec2* points, int count, const b2Color& color, float depth, bool triangulate = false);
	void DEBUG_DrawString(const b2Vec2& pw, const char* string, ...);

private:
	//Private/Internal Implementation
	class Impl;
	std::unique_ptr<Impl> _impl;

	bool GetItemMeta(ItemInfo item, std::string category, CheapVariant& val) const;
};

