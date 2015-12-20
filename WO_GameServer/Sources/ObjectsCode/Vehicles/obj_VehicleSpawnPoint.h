#ifdef VEHICLES_ENABLED

#pragma once

#include "GameCommon.h"
#include "ObjectsCode/Gameplay/BaseItemSpawnPoint.h"

class obj_Vehicle;

class obj_VehicleSpawnPoint : public BaseItemSpawnPoint
{
	DECLARE_CLASS(obj_VehicleSpawnPoint, BaseItemSpawnPoint)

public:
	std::vector<obj_Vehicle*> vehicles;

	bool isInitialSpawn;

	int numberOfSpawnedVehicles;

	float timeSinceLastSpawn;
	float nextUpdateTime;
	int maxSpawns;
	LootBoxConfig* lootBoxConfig;
	
	std::vector<uint32_t> vehicleSpawnSelection;

	struct NavigationPoint
	{
		r3dPoint3D position;
		int marked;
	};

	bool SpawnVehicle(int spawnIndex);
	void CreateSpawnedVehicle(int spawnIndex, const wiInventoryItem& wi);
	
	obj_VehicleSpawnPoint();
	~obj_VehicleSpawnPoint();

	virtual BOOL Load(const char* name);
	virtual BOOL OnCreate();
	virtual BOOL OnDestroy();
	virtual BOOL Update();
	virtual void ReadSerializedData(pugi::xml_node& node);

	void OnVehicleDestroyed(const obj_Vehicle* vehicle);
};

#endif
