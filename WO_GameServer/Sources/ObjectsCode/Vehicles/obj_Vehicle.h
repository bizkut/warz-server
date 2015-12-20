#pragma once

#include "GameCommon.h"
#include "multiplayer/NetCellMover.h"
#include "NetworkHelper.h"
#include "obj_Vehicle.h"
#include "ObjectsCode/WEAPONS/WeaponArmory.h"

#ifdef VEHICLES_ENABLED

class obj_VehicleSpawnPoint;
class obj_ServerPlayer;
struct VehicleDescriptor;

const uint32_t MAX_SEATS = 9;

class VehicleSeat
{
public:
	VehicleSeat();
	~VehicleSeat();

	DWORD playerId;
	int id;
	r3dVector offset;
};

class obj_Vehicle : public GameObject, INetworkHelper
{
	DECLARE_CLASS(obj_Vehicle, GameObject)

	const VehicleInfoConfig* vic;

	float wheel0Rotation[2];
	float wheel1Rotation[2];
	float wheel2Rotation[2];
	float wheel3Rotation[2];

	float wheelPosition[4];

	float camangle;
	float cameraUpDown;

	bool hasExploded;
	float forceExplosionTime;
	float lastExitTime;
	float explosionTime;
	float despawnAfterExplosionTime;

	float lastSpeedCheckTime;
	float speedCheckWaitTime;
	float movementSpeed;

	r3dVector lastMovementPos;

	float lastFuelCheckTime;
	float fuelCheckWaitTime;
	int maxFuel;
	int curFuel;
	bool hasRunOutOfFuel;

	bool isRemovingPlayers;
	bool isEntryAllowed;

	float lastDamageTime;
	float damageWaitTime;

	bool isHeadlightsEnabled;
	bool isHelicopter;

	uint32_t seatOccupancy;

	float lastFuelSendTime;
	float fuelSendTimeWait;

	bool isReady;
	bool hasFlyingBeenHandled;

	float lastUnstuckAttemptTime;
	float unstuckAttemptTimeWait;
	bool DoesPoseOverlapGeometry(PxBoxGeometry box, PxTransform pose, PxSceneQueryFilterData filter);
	void ForceExplosion();

	void CreateNavMeshObstacle();
	void DeleteNavMeshObstacle();
public:
	// This must match client for loading of vehicles
	enum VehicleTypes
	{
		VEHICLETYPE_INVALID = -1,
		VEHICLETYPE_BUGGY = 0,
		VEHICLETYPE_STRYKER,
		VEHICLETYPE_ZOMBIEKILLER,
		VEHICLETYPE_HUMMER,
		VEHICLETYPE_POLICE,
		VEHICLETYPE_ABANDONEDSUV,
		VEHICLETYPE_BONECRUSHER,
		VEHICLETYPE_COPCAR,
		VEHICLETYPE_DUNEBUGGY,
		VEHICLETYPE_ECONOLINE,
		VEHICLETYPE_LARGETRUCK,
		VEHICLETYPE_MILITARYAPC,
		VEHICLETYPE_PARAMEDIC,
		VEHICLETYPE_SUV,
		VEHICLETYPE_JEEP,
		VEHICLETYPE_TANK_T80,
		VEHICLETYPE_HELICOPTER
	};

	static r3dgameVector(obj_Vehicle*) s_ListOfAllActiveVehicles;	
	obj_ServerPlayer* playersInVehicle[MAX_SEATS];	

	obj_VehicleSpawnPoint* spawnObject;
	int spawnIndex;

	CVehicleNetCellMover netMover;
	
	int obstacleId;
	float obstacleRadius;
	uint32_t vehicleIdForFF;
	DWORD lastDriverId;
	DWORD vehicleId;
	uint16_t maxPlayerCount;
	bool isPlayerDriving;
	
	VehicleTypes vehicleType;

	VehicleDescriptor* vd;

	float maxVelocity;
	float accumDistance;

	void SetVehicleType(VehicleTypes vt);
	int GetVehicleType();

	bool HasPlayers() const;
	bool IsVehicleSeatsFull() const;
	int ReserveFirstAvailableSeat(obj_ServerPlayer* player);
	void ReleaseSeat(int seatPosition);
	r3dVector safeExitPosition;
	bool HasSafeExit(int seatPosition);
	bool IsExitSafe(int seatPosition, r3dPoint3D& outPosition);
	
	int GetSpeed();

	int AddPlayerToVehicle(obj_ServerPlayer* player);
	void RemovePlayerFromVehicle(obj_ServerPlayer* player);
	void RemovePlayers();

	void StartEngine();
	void StopEngine();

	void StartMoving();
	void StopMoving();
	void StartBreaking();
	void StopBreaking();

	void CheckSpeed();
	bool ApplyDamage(GameObject* fromObj, float damage, STORE_CATEGORIES damageSource);

	int GetRandomVehicleTypeForSpawn();

	obj_ServerPlayer* GetPlayerById(DWORD playerId);

	r3dPoint3D GetExitPosition(int seat);

	int maxDurability;
	int curDurability;
	int GetDurability();
	int GetDurabilityAsPercent();
	void SetDurability(int val, bool isForced = true);

	float destructionRate;
	float lastDestructionTick;
	bool despawnStarted;
	void StartDespawn();

	float armorExteriorValue;
	float armorInteriorValue;

	bool CheckFuel();
	void AddFuel(int amount);
	int GetFuel();
	void SendFuelToClients();

	r3dVector GetvForw() const 
	{ 
		D3DXMATRIX m = GetRotationMatrix();
		return r3dVector(m._31, m._32, m._33); 
	}
	r3dVector GetvRight() const 
	{ 
		D3DXMATRIX m = GetRotationMatrix();
		return r3dVector(m._11, m._12, m._13); 
	}
	
	void OnDamage();
	void OnExplode();
	void OnRepair();
	void OnRunOutOfFuel();

	void ApplyAutoDamage();
	void ScanForGeometry();

	void IsHittingGeomtry(r3dPoint3D dir);

	bool IsInWater();

	bool isFlying;
	float flyingStartTime;
	float flyingAllowedTime;
	void CheckFlying();
	float CheckDistanceFromGround(r3dPoint3D position);

	void UpdatePlayers(r3dPoint3D pos);

	void TryUnstuck();
public:
	
	static obj_Vehicle* GetVehicleById(DWORD vehicleId);

	obj_Vehicle();
	~obj_Vehicle();

	virtual BOOL OnCreate();
	virtual BOOL OnDestroy();
	virtual BOOL Update();

	INetworkHelper* GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket* INetworkHelper::NetGetCreatePacket(int* out_size);
	void INetworkHelper::LoadServerObjectData() { r3dError("not implemented."); }
	void INetworkHelper::SaveServerObjectData() { r3dError("not implemented."); }

	virtual BOOL OnNetReceive(DWORD EventId, const void* packetData, int packetSize);
	void		OnNetPacket(const PKT_C2C_VehicleMoveSetCell_s& n);
	void		OnNetPacket(PKT_C2C_VehicleMoveRel_s& n);
	void		OnNetPacket(const PKT_C2C_VehicleAction_s& n);
	void		OnNetPacket(const PKT_C2S_TurrerAngles_s& n);
	void		OnNetPacket(const PKT_C2C_VehicleHeadlights_s& n);

	void		RelayPacket(const DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered = true);

	void LoadXML();
};

#endif
