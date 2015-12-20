#pragma once

#include "GameCommon.h"
#include "../EclipseStudio/Sources/ObjectsCode/Gameplay/BaseItemSpawnPoint.h"

class LootBoxConfig;
class obj_SpawnedItem;

class obj_ServerItemSpawnPoint : public BaseItemSpawnPoint
{
	DECLARE_CLASS(obj_ServerItemSpawnPoint, BaseItemSpawnPoint)

	float m_spawnAllItemsAtTime;
	float m_nextUpdateTime;
	LootBoxConfig*	m_LootBoxConfig;
protected:
	bool	m_isHackerDecoySpawnPoint;
	
public:
	static std::vector<obj_ServerItemSpawnPoint*> m_allSpawns;

public:
	obj_ServerItemSpawnPoint();
	virtual ~obj_ServerItemSpawnPoint();
	
	virtual BOOL	Load(const char* name);
	virtual BOOL	OnCreate();
	virtual BOOL	Update();
	
	void		SpawnItem(int spawnIndex);
	obj_SpawnedItem* CreateSpawnedItem(int spawnIndex, const wiInventoryItem& wi);

	void		PickUpObject(int spawnIndex, gobjid_t spawnedObjID);

	void		LoadServerObjectData(const pugi::xml_node xmlNode);
	void		SaveServerObjectData(pugi::xml_node& xmlNode);
};

class obj_ServerHackerItemSpawnPoint : public obj_ServerItemSpawnPoint
{
	DECLARE_CLASS(obj_ServerHackerItemSpawnPoint, obj_ServerItemSpawnPoint)

public:
	obj_ServerHackerItemSpawnPoint();
	virtual ~obj_ServerHackerItemSpawnPoint();
};
