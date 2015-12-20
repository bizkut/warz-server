#include "r3dPCH.h"
#include "r3d.h"

#include "XMLHelpers.h"
#include "ServerGameLogic.h"

#include "obj_VehicleSpawnPoint.h"
#include "obj_Vehicle.h"

#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponArmory.h"

#ifdef VEHICLES_ENABLED

IMPLEMENT_CLASS(obj_VehicleSpawnPoint, "obj_VehicleSpawnPoint", "Object");
AUTOREGISTER_CLASS(obj_VehicleSpawnPoint);

obj_VehicleSpawnPoint::obj_VehicleSpawnPoint()
	: isInitialSpawn(true)
	, numberOfSpawnedVehicles(0)
	, timeSinceLastSpawn(0)
	, nextUpdateTime(0)
	, maxSpawns(0)
	, lootBoxConfig(NULL)
{
	serializeFile = SF_ServerData;

	ObjTypeFlags |= OBJTYPE_VehicleSpawnPoint;
}

obj_VehicleSpawnPoint::~obj_VehicleSpawnPoint()
{

}

BOOL obj_VehicleSpawnPoint::Load(const char *fname)
{
	// skip mesh loading
	if(!GameObject::Load(fname)) 
		return FALSE;

	return TRUE;
}

BOOL obj_VehicleSpawnPoint::OnCreate()
{
	if (m_LootBoxID == 0)
		return FALSE;

	lootBoxConfig = const_cast<LootBoxConfig*>(g_pWeaponArmory->getLootBoxConfig(m_LootBoxID));
	if(lootBoxConfig == NULL)
	{
		r3dOutToLog("LootBox: !!!! Failed to get lootbox with ID: %d. Server/Client desync??\n", m_LootBoxID);
		return FALSE;
	}

	if(lootBoxConfig->entries.size() == 0)
	{
		r3dOutToLog("Vehicle LootBox: !!!! m_LootBoxID %d does NOT have items inside\n", m_LootBoxID);
	}

	r3dOutToLog("Vehicle LootBox: %p with ItemID %d, %d items, tick: %.0f\n", this, m_LootBoxID, lootBoxConfig->entries.size(), m_TickPeriod);

	nextUpdateTime = r3dGetTime() + m_TickPeriod;

	return parent::OnCreate();
}

BOOL obj_VehicleSpawnPoint::OnDestroy()
{
	return parent::OnDestroy();
}

BOOL obj_VehicleSpawnPoint::Update()
{
	if (gServerLogic.net_mapLoaded_LastNetID > 0)
	{
		if (r3dGetTime() > nextUpdateTime)
		{
			nextUpdateTime = r3dGetTime() + m_TickPeriod;

			if (lootBoxConfig->entries.size() == 0)
				return parent::Update();

			int randomIndex = (int)u_GetRandom(0.0f, float(m_SpawnPointsV.size() - 1));
			
			ItemSpawn& spawn = m_SpawnPointsV[randomIndex];

			if (spawn.itemID == 0 && spawn.cooldown < r3dGetTime() && numberOfSpawnedVehicles < maxSpawns)
			{
				SpawnVehicle((int)randomIndex);
			}
		}
	}

	parent::Update();

	return TRUE;
}

bool obj_VehicleSpawnPoint::SpawnVehicle(int spawnIndex)
{
	ItemSpawn& spawn = m_SpawnPointsV[spawnIndex];
	wiInventoryItem wi;

	wi = RollItem(lootBoxConfig, 0);
	if (wi.itemID == 0 || wi.itemID == -1)
		return false;

	CreateSpawnedVehicle(spawnIndex, wi);

	return true;
}

void obj_VehicleSpawnPoint::CreateSpawnedVehicle(int spawnIndex, const wiInventoryItem& wi)
{
	ItemSpawn& spawn = m_SpawnPointsV[spawnIndex];

	char name[28];
	sprintf(name, "Vehicle_%d_%p", numberOfSpawnedVehicles++, this);

	obj_Vehicle* vehicle = (obj_Vehicle*)srv_CreateGameObject("obj_Vehicle", name, spawn.pos);
	vehicle->SetNetworkID(gServerLogic.GetFreeNetId());
	vehicle->NetworkLocal = true;
	vehicle->spawnObject = this;
	vehicle->spawnIndex = spawnIndex;

	obj_Vehicle::VehicleTypes vehicleType;
	
	if (wi.itemID == 101412)
		vehicleType = obj_Vehicle::VEHICLETYPE_BUGGY;
	else if (wi.itemID == 101413)
		vehicleType = obj_Vehicle::VEHICLETYPE_STRYKER;	
	if (wi.itemID == 101414)
		vehicleType = obj_Vehicle::VEHICLETYPE_ZOMBIEKILLER;
	if (wi.itemID == 101418)
		vehicleType = obj_Vehicle::VEHICLETYPE_HUMMER;
	if (wi.itemID == 101419)
		vehicleType = obj_Vehicle::VEHICLETYPE_POLICE;
	if (wi.itemID == 101420)
		vehicleType = obj_Vehicle::VEHICLETYPE_ABANDONEDSUV;
	if (wi.itemID == 101421)
		vehicleType = obj_Vehicle::VEHICLETYPE_BONECRUSHER;
	if (wi.itemID == 101422)
		vehicleType = obj_Vehicle::VEHICLETYPE_COPCAR;
	if (wi.itemID == 101423)
		vehicleType = obj_Vehicle::VEHICLETYPE_DUNEBUGGY;
	if (wi.itemID == 101424)
		vehicleType = obj_Vehicle::VEHICLETYPE_ECONOLINE;
	if (wi.itemID == 101425)
		vehicleType = obj_Vehicle::VEHICLETYPE_LARGETRUCK;
	if (wi.itemID == 101426)
		vehicleType = obj_Vehicle::VEHICLETYPE_MILITARYAPC;
	if (wi.itemID == 101427)
		vehicleType = obj_Vehicle::VEHICLETYPE_PARAMEDIC;
	if (wi.itemID == 101428)
		vehicleType = obj_Vehicle::VEHICLETYPE_SUV;
	if (wi.itemID == 101429)
		vehicleType = obj_Vehicle::VEHICLETYPE_HELICOPTER;
	if (wi.itemID == 101415)
		vehicleType = obj_Vehicle::VEHICLETYPE_TANK_T80;
	if (wi.itemID == 101430)
		vehicleType = obj_Vehicle::VEHICLETYPE_JEEP;

	vehicle->SetVehicleType(vehicleType);
	vehicle->SetRotationVector(r3dVector(u_GetRandom(0, 360), 0, 0));

	vehicle->OnCreate();

	spawn.itemID = vehicle->vehicleId;

	vehicles.push_back(vehicle);
}

void obj_VehicleSpawnPoint::ReadSerializedData(pugi::xml_node& node)
{
	parent::ReadSerializedData(node);

	pugi::xml_node spawnNode = node.child("spawn_parameters");

	GetXMLVal("MaxSpawns", spawnNode, &maxSpawns);

	// Private and Premium servers have double the amount of vehicle spawns
	if (gServerLogic.ginfo_.channel == 3 || gServerLogic.ginfo_.channel == 4)
		maxSpawns *= 2;
}

void obj_VehicleSpawnPoint::OnVehicleDestroyed(const obj_Vehicle* vehicle)
{
	// NOTE:
	// only clean up properly spawned vehicles from actual spawn points.
	// developers can spawn vehicles that are not assigned to a spawn point.
	if (vehicle->spawnIndex == -1)
		return;

	ItemSpawn& spawn = m_SpawnPointsV[vehicle->spawnIndex];
	spawn.itemID = 0;
	spawn.cooldown = m_Cooldown + r3dGetTime() + u_GetRandom(15.0f, 60.0f); // add a little bit of randomness to help spread out vehicle spawns.

	for (std::vector<obj_Vehicle*>::iterator it = vehicles.begin(); it != vehicles.end(); ++it)
	{
		if (*it == vehicle)
		{
			vehicles.erase(it);
			break;
		}
	}

	numberOfSpawnedVehicles = (int)vehicles.size();
}

#endif
