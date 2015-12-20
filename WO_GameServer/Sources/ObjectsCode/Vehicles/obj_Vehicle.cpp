#include "r3dpch.h"
#include "r3d.h"

#include "multiplayer/P2PMessages.h"

#include "obj_Vehicle.h"
#include "obj_VehicleSpawnPoint.h"
#include "GameObjects/VehicleDescriptor.h"
#include "../obj_ServerPlayer.h"
#include "../obj_ServerBarricade.h"
#include "../../GameEngine/ai/AutodeskNav/AutodeskNavMesh.h"

#ifdef VEHICLES_ENABLED

IMPLEMENT_CLASS(obj_Vehicle, "obj_Vehicle", "Object");
AUTOREGISTER_CLASS(obj_Vehicle);

CVAR_COMMENT("_vehicles_", "Vehicles");

const float _NetLagSpeedCompensation	= 3.0f;

float _vehicles_MaxSpawnDelay			= 60.0f;
ushort	_vehicles_NumVehicles			= 0;
float _vehicles_VehicleSpawnProtect		= 15.0f;

extern float getWaterDepthAtPos(const r3dPoint3D& pos);

std::vector<obj_Vehicle*> obj_Vehicle::s_ListOfAllActiveVehicles;

obj_Vehicle* obj_Vehicle::GetVehicleById(DWORD vehicleId)
{
	for (std::vector<obj_Vehicle*>::iterator it = obj_Vehicle::s_ListOfAllActiveVehicles.begin(); it != obj_Vehicle::s_ListOfAllActiveVehicles.end(); ++it)
	{
		if (!*it)
			continue;

		if ((*it)->vehicleId == vehicleId)
		{
			return *it;
		}
	}
	
	return NULL;
}

obj_Vehicle::obj_Vehicle()
	: spawnObject(NULL)
	, netMover(this, 0.2f, (float)PKT_C2C_MoveSetCell_s::VEHICLE_CELL_RADIUS)
	, isPlayerDriving(false)
	, vehicleType(VEHICLETYPE_INVALID)
	, vd(NULL)
	, accumDistance(0)
	, curDurability(2000)
	, maxDurability(2000)
	, lastExitTime(0)
	, explosionTime(0)
	, despawnAfterExplosionTime(30)
	, forceExplosionTime(60 * 30)
	, hasExploded(false)
	, destructionRate(1)
	, despawnStarted(false)
	, lastDestructionTick(0)
	, armorExteriorValue(0.0f)
	, armorInteriorValue(0.0f)
	, speedCheckWaitTime(1.0f)
	, movementSpeed(0)
	, lastFuelCheckTime(0.0f)
	, fuelCheckWaitTime(1.0f)
	, maxFuel(0)
	, curFuel(0)
	, camangle(0)
	, cameraUpDown(0)
	, hasRunOutOfFuel(false)
	, isRemovingPlayers(false)
	, isEntryAllowed(false)
	, lastDamageTime(0.0f)
	, damageWaitTime(2.0f)
	, isHeadlightsEnabled(false)
	, seatOccupancy(0)
	, lastDriverId(0)
	, lastFuelSendTime(0.0f)
	, fuelSendTimeWait(3.0f)
	, flyingStartTime(0.0f)
	, flyingAllowedTime(15.0f)
	, isFlying(false)
	, lastUnstuckAttemptTime(0.0f)
	, unstuckAttemptTimeWait(2.0f)
	, isReady(false) // this is needed in order for flying check to not false positive on load
	, hasFlyingBeenHandled(false)
	, obstacleId(-1)
	, spawnIndex(-1)
	, isHelicopter(false)
{
	s_ListOfAllActiveVehicles.push_back(this);

	_vehicles_NumVehicles++;
}

obj_Vehicle::~obj_Vehicle()
{
	for (std::vector<obj_Vehicle*>::iterator it = s_ListOfAllActiveVehicles.begin(); it < s_ListOfAllActiveVehicles.end(); ++it)
	{
		if ((*it)->GetNetworkID() == this->GetNetworkID())
		{
			s_ListOfAllActiveVehicles.erase(it);
			break;
		}
	}

	if (vd)
		delete vd;

	memset( playersInVehicle, 0, sizeof( obj_ServerPlayer* ) * MAX_SEATS );
}

BOOL obj_Vehicle::OnCreate()
{
	ObjTypeFlags |= OBJTYPE_Vehicle;

	r3d_assert(NetworkLocal);
	r3d_assert(GetNetworkID());

	vehicleId = _vehicles_NumVehicles;
	NetworkLocal = false;	
	lastExitTime = r3dGetTime();

	if (vehicleType == VEHICLETYPE_STRYKER)
	{
		maxPlayerCount = 9;
		vic = g_pWeaponArmory->getVehicleConfig(101413);
	}
	else if (vehicleType == VEHICLETYPE_BUGGY)
	{
		maxPlayerCount = 3;
		vic = g_pWeaponArmory->getVehicleConfig(101412);
	}
	else if (vehicleType == VEHICLETYPE_ZOMBIEKILLER)
	{
		maxPlayerCount = 2;
		vic = g_pWeaponArmory->getVehicleConfig(101414);
	}
	else if (vehicleType == VEHICLETYPE_HUMMER)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101418);
	}
	else if (vehicleType == VEHICLETYPE_POLICE)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101419);
	}
	else if (vehicleType == VEHICLETYPE_ABANDONEDSUV)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101420);
	}
	else if (vehicleType == VEHICLETYPE_BONECRUSHER)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101421);
	}
	else if (vehicleType == VEHICLETYPE_COPCAR)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101422);
	}
	else if (vehicleType == VEHICLETYPE_DUNEBUGGY)
	{
		maxPlayerCount = 2;
		vic = g_pWeaponArmory->getVehicleConfig(101423);
	}
	else if (vehicleType == VEHICLETYPE_ECONOLINE)
	{
		maxPlayerCount = 2;
		vic = g_pWeaponArmory->getVehicleConfig(101424);
	}
	else if (vehicleType == VEHICLETYPE_LARGETRUCK)
	{
		maxPlayerCount = 2;
		vic = g_pWeaponArmory->getVehicleConfig(101425);
	}
	else if (vehicleType == VEHICLETYPE_MILITARYAPC)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101426);
	}
	else if (vehicleType == VEHICLETYPE_PARAMEDIC)
	{
		maxPlayerCount = 2;
		vic = g_pWeaponArmory->getVehicleConfig(101427);
	}
	else if (vehicleType == VEHICLETYPE_SUV)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101428);
	}
	else if (vehicleType == VEHICLETYPE_TANK_T80)
	{
		maxPlayerCount = 3;
		vic = g_pWeaponArmory->getVehicleConfig(101415);
	}
	else if (vehicleType == VEHICLETYPE_HELICOPTER)
	{
		maxPlayerCount = 6;
		vic = g_pWeaponArmory->getVehicleConfig(101429);
		isHelicopter = true;
	}
	else if (vehicleType == VEHICLETYPE_JEEP)
	{
		maxPlayerCount = 4;
		vic = g_pWeaponArmory->getVehicleConfig(101430);
	}

	memset( playersInVehicle, 0, sizeof( obj_ServerPlayer* ) * MAX_SEATS );

	vd = game_new VehicleDescriptor;
	if( vd )
	{
		vd->owner = this;
		LoadXML();
		//char ratios[128];
		//int offset = 0;
		//for(uint32_t i = 0; i < vd->gearsData.mNumRatios; ++i)
		//	offset += sprintf( ratios + offset, "%.3f ", vd->gearsData.mRatios[ i ] );
		//r3dOutToLog("!!! Vehicle Descriptor:\nNum Ratios:%d, Ratios:%s, Final Ratio:%.3f, Peak Torque:%.3f, Max Omega:%.3f, Wheel Radius:%.4f, MaxSpeed:%.3f\n", vd->gearsData.mNumRatios, ratios, vd->gearsData.mFinalRatio, vd->engineData.mPeakTorque, vd->engineData.mMaxOmega, vd->wheelsData[0].wheelData.mRadius, vd->GetMaxSpeed());
		armorExteriorValue = (100.0f - (float)vd->armorExterior) / 100.0f;
		armorInteriorValue = (100.0f - (float)vd->armorExterior) / 100.0f;

		maxDurability = vd->durability;
		curDurability = maxDurability; //(int)u_GetRandom((float)maxDurability / 2.0f, (float)maxDurability);

		maxFuel = vd->maxFuel;
		curFuel = (int)u_GetRandom((float)(maxFuel * 0.75f), (float)maxFuel);
	}
	else
	{
		r3dOutToLog("!!! Failed to allocate Vehicle Descriptor!\n");
	}

	netMover.SetStartCell(GetPosition());
	lastMovementPos = GetPosition();

	CreateNavMeshObstacle();

	gServerLogic.NetRegisterObjectToPeers(this);

	isReady = true;

	return parent::OnCreate();
}

BOOL obj_Vehicle::OnDestroy()
{
	PKT_S2C_DestroyNetObject_s packet;
	packet.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &packet, sizeof(packet));

	DeleteNavMeshObstacle();

	return parent::OnDestroy();
}

DefaultPacket* obj_Vehicle::NetGetCreatePacket(int* out_size)
{
	//r3dOutToLog("!!! Vehicle Info: PeakTorque:%.4f, MaxOmega:%.4f, Armor:%d, Durability:%d, Max Fuel:%d, Weight:%.4f\n", vd->engineData.mPeakTorque, vd->engineData.mMaxOmega, vd->armor, vd->durability, vd->maxFuel, vd->weight);

	static PKT_S2C_VehicleSpawn_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	n.position = GetPosition();
	n.rotationX = GetRotationVector().x;
	n.rotationY = GetRotationVector().y;
	n.rotationZ = GetRotationVector().z;
	n.moveCell = netMover.SrvGetCell();
	n.vehicleType = vehicleType;
	n.maxSpeed = 35;
	n.vehicleId = vehicleId;
	n.isCarDrivable = (BYTE)1;

	n.camangle = camangle;
	n.cameraUpDown = cameraUpDown;

	n.maxDurability = maxDurability;
	n.curDurability = curDurability;
	n.armor	= (int)vd->armorExterior;
	n.peakTorque = (int)vd->engineData.mPeakTorque;
	n.maxOmega = (int)vd->engineData.mMaxOmega;
	n.curFuel = curFuel;
	n.maxFuel = maxFuel;
	n.weight = (int)vd->weight;

	n.isHeadlightsEnabled = isHeadlightsEnabled;

	uint32_t playerCount = 0;
	for (uint32_t i = 0; i < maxPlayerCount; ++i)
	{
		obj_ServerPlayer* player = playersInVehicle[i];
		if (!player)
			continue;

		++playerCount;
		n.playersInVehicle[i] = toP2pNetId(player->GetNetworkID());
	}
	n.hasPlayers = playerCount > 0;
	n.playerCount = (BYTE)playerCount;

	*out_size = sizeof(n);

	return &n;
}

BOOL obj_Vehicle::Update()
{
	parent::Update();

	if (!isActive() || !isReady)
		return TRUE;

	if (!isHelicopter)
		CheckFlying();

	ApplyAutoDamage();

	CheckSpeed();

	if (!HasPlayers())
		return TRUE;

	if (IsInWater())
		return TRUE;

	if (!CheckFuel())
		return TRUE;

	if (!hasExploded && movementSpeed > 0.0f)
		ScanForGeometry();

	return TRUE;
}

bool obj_Vehicle::IsInWater()
{
	float allowedDepth = 1.5f; 
	float waterDepth = getWaterDepthAtPos(GetPosition());
	if (waterDepth > allowedDepth)
	{
		StartDespawn();
		RemovePlayers();
		return true;
	}

	return false;
}

void obj_Vehicle::RemovePlayers()
{
	isRemovingPlayers = true;

	for (uint32_t i = 0; i < maxPlayerCount; ++i) 
	{
		if( !playersInVehicle[i] )
			continue;

		if (!playersInVehicle[i]->IsInVehicle())
		{
			playersInVehicle[i] = 0;
		}
		else
		{
			playersInVehicle[i]->ExitVehicle(true);
		}
	}

	if (!HasPlayers())
		CreateNavMeshObstacle();

	isRemovingPlayers = false;
}

bool obj_Vehicle::CheckFuel()
{
	// let's keep the fuel guage for the players more in sync with the server
	// this is sent every 3 seconds, it's only going to tick down if players are in it
	if (r3dGetTime() > lastFuelSendTime + fuelSendTimeWait)
	{
		lastFuelSendTime = r3dGetTime();
		SendFuelToClients();
	}

	if (r3dGetTime() > lastFuelCheckTime + fuelCheckWaitTime)
	{
		lastFuelCheckTime = r3dGetTime();

		if (curFuel > 0)
		{
			if (movementSpeed > 0)
				curFuel -= 2;
			else
				curFuel -= 1;

			if (curFuel <= 0)
			{
				curFuel = 0;
				OnRunOutOfFuel();
			}
		}
	}

	return curFuel > 0;
}

void obj_Vehicle::CheckSpeed()
{
	if (r3dGetTime() > lastSpeedCheckTime + speedCheckWaitTime)
	{
		lastSpeedCheckTime = r3dGetTime();

		movementSpeed = accumDistance;
		accumDistance = 0.0f;

		if (movementSpeed > vd->GetMaxSpeed() * 1.5f)
		{
			obj_ServerPlayer* player = playersInVehicle[0];
			if (player)
				gServerLogic.LogCheat(player->peerId_, PKT_S2C_CheatWarning_s::CHEAT_FastMove, true, "vehicle_speedcheat", "%f", movementSpeed);
		}
	}
}

void obj_Vehicle::CheckFlying()
{
	float distanceFromGround = CheckDistanceFromGround(GetPosition());
	if (distanceFromGround > 3.0f || distanceFromGround == -1.0f) // if our distance is -1 our distance check went over 2000 units, meaning vehicle is definitely flying, no vehicle is flush on the ground at 0.
	{
		if (!isFlying)
		{
			isFlying = true;
			flyingStartTime = r3dGetTime();
		}
	}
	else 
	{
		isFlying = false;
	}

	if (isFlying && !hasFlyingBeenHandled)
	{
		if (r3dGetTime() > flyingStartTime + flyingAllowedTime)
		{
			hasFlyingBeenHandled = true;

			ForceExplosion();

			obj_ServerPlayer* player = playersInVehicle[0];
			if (!player)
				return;

			gServerLogic.LogCheat(player->peerId_, PKT_S2C_CheatWarning_s::CHEAT_Flying, true, "VehicleFlying",
				"%f %f %f", GetPosition().x, GetPosition().y, GetPosition().z);
		}
	}
}

float obj_Vehicle::CheckDistanceFromGround(r3dPoint3D position)
{
	PxVec3 pos(position.x, position.y + 0.5f, position.z);
	PxSceneQueryFilterData filter(PxFilterData(COLLIDABLE_PLAYER_COLLIDABLE_MASK, 0, 0, 0), PxSceneQueryFilterFlag::eSTATIC|PxSceneQueryFilterFlag::eDYNAMIC);

	float MAX_CASTING_DISTANCE = 2000.0f;

	PxRaycastHit hit;
	if (g_pPhysicsWorld->PhysXScene->raycastSingle(pos, PxVec3(0, -1, 0), MAX_CASTING_DISTANCE, PxSceneQueryFlag::eDISTANCE|PxSceneQueryFlag::eINITIAL_OVERLAP|PxSceneQueryFlag::eINITIAL_OVERLAP_KEEP, hit, filter))
		return hit.distance;

	// this should never be the case with a max distance of 2000... 
	// if so, our check flying method kicks for cheating or our unstuck method fails for being under terrain.
	return -1.0f; 
}

void obj_Vehicle::ApplyAutoDamage()
{
	if (!hasExploded)
	{
		// if vehicle has not been exited in N time, start despawn process
		if (!despawnStarted && r3dGetTime() > lastExitTime + forceExplosionTime && !HasPlayers())
		{
			StartDespawn();
		}

		if (despawnStarted && curDurability >= 300)
			curDurability = 300;

		int dura = GetDurabilityAsPercent();

		// when vehicle starts smoking, continue damaging the vehicle
		if (r3dGetTime() > lastDestructionTick + destructionRate && !hasExploded)
		{
			lastDestructionTick = r3dGetTime();

			if (dura <= 10)
			{
				SetDurability(-15);
			}
			else if (dura <= 20)
			{
				SetDurability(-10);
			}
			else if (dura <= 30 && !HasPlayers())
			{
				SetDurability(-5);
			}
		}
	}

	if (hasExploded && r3dGetTime() > explosionTime + despawnAfterExplosionTime)
	{
		if (spawnObject)
			spawnObject->OnVehicleDestroyed(this);

		setActiveFlag(0);
	}
}

void obj_Vehicle::ScanForGeometry()
{
	IsHittingGeomtry(GetvForw());
}

void obj_Vehicle::IsHittingGeomtry(r3dPoint3D dir)
{	
	PxBoxGeometry boxTest(1.1f, 0.2f, 2.0f);
	PxTransform boxPose(PxVec3(GetPosition().x, GetPosition().y + 0.7f, GetPosition().z), PxQuat(0, 0, 0, 1));
	PxSceneQueryFilterData filter(PxFilterData(COLLIDABLE_PLAYER_COLLIDABLE_MASK, 0, 0, 0), PxSceneQueryFilterFlag::eSTATIC|PxSceneQueryFilterFlag::eDYNAMIC);

	r3dPoint3D testDir = dir.Normalize();
	float MAX_CASTING_DISTANCE = R3D_MIN(3.5f, 3.5f * GetSpeed() / 35.0f);
	if (MAX_CASTING_DISTANCE == 0) MAX_CASTING_DISTANCE = 0.1f;

	bool isBlockHit;
	PxSweepHit sweepHits[32];
	int results = g_pPhysicsWorld->PhysXScene->sweepMultiple(boxTest, boxPose, PxVec3(testDir.x, testDir.y, testDir.z), MAX_CASTING_DISTANCE, PxSceneQueryFlag::eINITIAL_OVERLAP|PxSceneQueryFlag::eINITIAL_OVERLAP_KEEP|PxSceneQueryFlag::eNORMAL, sweepHits, 32, isBlockHit, filter);

	for (int i = 0; i < results; ++i)
	{
		PhysicsCallbackObject* target = NULL;

		if (sweepHits[i].shape && (target = static_cast<PhysicsCallbackObject*>(sweepHits[i].shape->getActor().userData)))
		{
			GameObject* gameObj = target->isGameObject();
			if (!gameObj)
				continue;

			if ( (movementSpeed + _NetLagSpeedCompensation) > 10.0f )
			{
				if( gameObj->isObjType(OBJTYPE_Barricade) )
				{
					obj_ServerBarricade* barricade = (obj_ServerBarricade*)gameObj;
					if (barricade->m_Health > 0.0f)
					{
						SetDurability(-barricade->GetDamageForVehicle(), false);
						barricade->DoDamage( barricade->m_Health + 1 );
					}
				}
				else if (gameObj->isObjType(OBJTYPE_Terrain) && sweepHits[i].distance > 0.1f)
					continue;

				const float damageMultiplier = 100.0f;
				float damage = ((movementSpeed / vd->GetMaxSpeed()) * damageMultiplier) * 0.7f;
				SetDurability(-(int)damage, false);
			}
		}
	}
}

obj_ServerPlayer* obj_Vehicle::GetPlayerById(DWORD networkId)
{
	return IsServerPlayer(GameWorld().GetNetworkObject(networkId));
}

bool obj_Vehicle::HasPlayers() const
{
	uint32_t playerCount = 0;
	for(uint32_t i = 0; i < maxPlayerCount; ++i)
	{
		if( playersInVehicle[ i ] )
		{
			r3d_assert(playersInVehicle[i]->IsInVehicle());

			++playerCount;
		}
	}
	return playerCount > 0;
}

void obj_Vehicle::SetVehicleType(VehicleTypes vt)
{
	vehicleType = vt;
}

bool obj_Vehicle::IsVehicleSeatsFull() const
{	
	uint32_t playerCount = 0;
	for(uint32_t i = 0; i < maxPlayerCount; ++i)
	{
		if( playersInVehicle[ i ] )
		{
			r3d_assert(playersInVehicle[i]->IsInVehicle());			
			++playerCount;
		}
	}
	return playerCount == maxPlayerCount;
}

int obj_Vehicle::ReserveFirstAvailableSeat(obj_ServerPlayer* player)
{
	if(!player)
		return -1;

	for(uint32_t i = 0; i < maxPlayerCount; ++i)
	{
		if( !playersInVehicle[ i ] )
		{
			playersInVehicle[ i ] = player;
			return i;
		}
	}

	return -1;
}

void obj_Vehicle::ReleaseSeat(int seatPosition)
{
	r3d_assert(seatPosition>=0);
	if( seatPosition < maxPlayerCount )
		playersInVehicle[ seatPosition ] = 0;
}

bool obj_Vehicle::HasSafeExit(int startAtSeat)
{
	int checks = 0;
	int currentSeat = startAtSeat;
	while (checks < maxPlayerCount)
	{
		if (currentSeat >= maxPlayerCount)
			currentSeat = 0;

		if (IsExitSafe(currentSeat, safeExitPosition))
		{
			return true;
		}

		++currentSeat;
		++checks;
	}

	if (IsExitSafe(4, safeExitPosition))
	{
		safeExitPosition = r3dVector(GetPosition().x, GetPosition().y + 3.0f, GetPosition().z);
		return true;
	}
	return false;
}

bool obj_Vehicle::IsExitSafe(int seatPosition, r3dPoint3D& outPosition)
{
	outPosition = GetExitPosition(seatPosition);

	{
		r3dVector testDirection;
		if (seatPosition == 0)
			testDirection = -GetvRight();
		else if (seatPosition == 1)
			testDirection = GetvRight();
		else
			testDirection = -GetvForw();

		PxVec3 dir(testDirection.x, testDirection.y, testDirection.z);

		PxSweepHit hit;

		r3dVector rootPos = GetPosition();
		PxTransform rootPose(PxVec3(rootPos.x, rootPos.y + 0.5f, rootPos.z));

		r3dVector pos = outPosition;
		PxTransform pose(PxVec3(pos.x, pos.y+1.8f, pos.z));
		PxBoxGeometry boxg(0.5f, 0.1f, 0.5f);

		PxSceneQueryFilterData filter(PxFilterData(COLLIDABLE_PLAYER_COLLIDABLE_MASK, 0, 0, 0), PxSceneQueryFilterFlag::eSTATIC);

		if(g_pPhysicsWorld->PhysXScene->sweepSingle(boxg, rootPose, dir, 3.0f, PxSceneQueryFlag::eDISTANCE|PxSceneQueryFlag::eINITIAL_OVERLAP|PxSceneQueryFlag::eINITIAL_OVERLAP_KEEP, hit, filter))
		{
			PhysicsCallbackObject* target = NULL;

			if (hit.shape && (target = static_cast<PhysicsCallbackObject*>(hit.shape->getActor().userData)))
			{
				GameObject* gameObj = target->isGameObject();
				if (!gameObj || !gameObj->isObjType(OBJTYPE_Terrain))
					return false;
			}
			else						
				return false;
		}
		if(!g_pPhysicsWorld->PhysXScene->sweepSingle(boxg, pose, PxVec3(0,-1,0), 50.0f, PxSceneQueryFlag::eDISTANCE|PxSceneQueryFlag::eINITIAL_OVERLAP|PxSceneQueryFlag::eINITIAL_OVERLAP_KEEP, hit, filter))
		{
			return false;
		}
		else
		{
			outPosition = r3dPoint3D(hit.impact.x, hit.impact.y + 0.25f, hit.impact.z);
		}
	}

	return true;
}

void obj_Vehicle::TryUnstuck()
{
	if (r3dGetTime() < lastUnstuckAttemptTime + unstuckAttemptTimeWait || HasPlayers())
		return;

	lastUnstuckAttemptTime = r3dGetTime();

	PxBoxGeometry box(3.0f, 3.0f, 3.0f); 
	PxTransform pose(PxVec3(GetPosition().x, GetPosition().y - 0.5f, GetPosition().z), PxQuat(0,0,0,1));
	PxTransform testPose = pose;
	PxSceneQueryFilterData filter(PxFilterData(COLLIDABLE_STATIC_MASK, 0, 0, 0), PxSceneQueryFilterFlag::eSTATIC|PxSceneQueryFilterFlag::eDYNAMIC);

	// do initial test to ensure we are actually stuck.
	if (!DoesPoseOverlapGeometry(box, pose, filter))
		return;

	const float RADIUS = 25.0f;
	const int MAX_TESTS = 10;
	r3dPoint3D pos;
	bool hasTestFailed = false;
	float distanceFromGround;

	// it's possible the vehicle is stuck, attempt to unstuck.
	for (int i = 0; i < MAX_TESTS; ++i)
	{
		hasTestFailed = false;

		float theta = R3D_DEG2RAD(float(i) / float(MAX_TESTS)) * 360.0f;
		pos.Assign(cosf(theta) * RADIUS, 0, sinf(theta) * RADIUS);
		pos += r3dPoint3D(pose.p.x, pose.p.y, pose.p.z);

		testPose.p.x = pos.x;
		testPose.p.y = pos.y;
		testPose.p.z = pos.z;

		hasTestFailed = DoesPoseOverlapGeometry(box, testPose, filter);

		if (!hasTestFailed)
		{
			distanceFromGround = CheckDistanceFromGround(pos);
			if (distanceFromGround == -1.0f)
				hasTestFailed = true;
		}

		if (!hasTestFailed)
			break;
	}

	// we have a valid spot to move to, move it.
	if (!hasTestFailed)
	{
		SetPosition(r3dPoint3D(testPose.p.x, testPose.p.y - distanceFromGround + 0.2f, testPose.p.z));
		netMover.SrvSetCell(GetPosition());

		PKT_S2C_VehicleUnstuck_s n;
		n.isSuccess = true;
		n.position = GetPosition();
		gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	}
	else
	{
		PKT_S2C_VehicleUnstuck_s n;
		n.isSuccess = false;
		gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	}
}

bool obj_Vehicle::DoesPoseOverlapGeometry(PxBoxGeometry box, PxTransform pose, PxSceneQueryFilterData filter)
{
	const PxU32 maxHits = 64;
	PxShape* hits[maxHits];

	PxI32 results = g_pPhysicsWorld->PhysXScene->overlapMultiple(box, pose, hits, maxHits, filter);
	if (results == 0)
		return false;
	
	PhysicsCallbackObject* target = NULL;

	for (PxI32 i = 0; i < results; ++i)
	{
		if (hits[i]->userData && (target = static_cast<PhysicsCallbackObject*>(hits[i]->userData)))
		{
			GameObject* gameObj = target->isGameObject();
			// we don't care if our box overlaps with the terrain or the ground plane on devmap
			// todo: this could be cleaned up a bit in order to set the rotation of the vehicle to the current slope angle of the terrain
			if (gameObj && (gameObj->isObjType(OBJTYPE_Terrain) || gameObj->FileName == "data/objectsdepot/editor/groundplane_100x100.sco"))
				continue;
			
			return true;
		}
	}

	return false;
}

int obj_Vehicle::AddPlayerToVehicle(obj_ServerPlayer* player)
{
	r3d_assert(!IsVehicleSeatsFull());

	gServerLogic.LogInfo(player->peerId_, "AddPlayerToVehicle", ""); CLOG_INDENT;
		
	int seatPosition = ReserveFirstAvailableSeat(player);
	if (seatPosition >= 0)
	{
		//r3dOutToLog("!!! Adding player(%u:%s) to seat '%d'\n", player->GetNetworkID(), player->Name.c_str(), seatPosition);
	}

	// we save this information in the event of car bombing. (jumping out of car and letting it explode to kill zombies or players).
	if (seatPosition == 0)
		lastDriverId = player->GetNetworkID();

	despawnStarted = false;
		
	DeleteNavMeshObstacle();

	return seatPosition; // driver will be seat position 0
}

void obj_Vehicle::RemovePlayerFromVehicle(obj_ServerPlayer* player)
{
	r3dOutToLog("!!! Removing player(%u:%s) from seat '%d'\n", player->GetNetworkID(), player->Name.c_str(), player->seatPosition);
	r3d_assert( player );
	r3d_assert( player->IsInVehicle() );
	r3d_assert( player->seatPosition < maxPlayerCount );
	r3d_assert( playersInVehicle[player->seatPosition] == player );

	ReleaseSeat( player->seatPosition );

	lastExitTime = r3dGetTime();
	despawnStarted = false;

	if (!HasPlayers())
		CreateNavMeshObstacle();
}

#undef DEFINE_GAMEOBJ_PACKET_HANDLER
#define DEFINE_GAMEOBJ_PACKET_HANDLER(xxx) \
	case xxx: { \
	const xxx##_s&n = *(xxx##_s*)packetData; \
	if(packetSize != sizeof(n)) { \
	r3dOutToLog("!!!!errror!!!! %s packetSize %d != %d\n", #xxx, packetSize, sizeof(n)); \
	return TRUE; \
	} \
	OnNetPacket(n); \
	return TRUE; \
}

#undef DEFINE_GAMEOBJ_PACKET_HANDLER_NOCONST
#define DEFINE_GAMEOBJ_PACKET_HANDLER_NOCONST(xxx) \
	case xxx: { \
	xxx##_s&n = *(xxx##_s*)packetData; \
	if(packetSize != sizeof(n)) { \
	r3dOutToLog("!!!!errror!!!! %s packetSize %d != %d\n", #xxx, packetSize, sizeof(n)); \
	return TRUE; \
	} \
	OnNetPacket(n); \
	return TRUE; \
}



BOOL obj_Vehicle::OnNetReceive(DWORD EventId, const void* packetData, int packetSize)
{
	r3d_assert(!(ObjFlags & OBJFLAG_JustCreated));

	switch(EventId)
	{
		DEFINE_GAMEOBJ_PACKET_HANDLER(PKT_C2C_VehicleMoveSetCell);
		DEFINE_GAMEOBJ_PACKET_HANDLER_NOCONST(PKT_C2C_VehicleMoveRel);
		DEFINE_GAMEOBJ_PACKET_HANDLER(PKT_C2C_VehicleAction);
		DEFINE_GAMEOBJ_PACKET_HANDLER(PKT_C2C_VehicleHeadlights);
		DEFINE_GAMEOBJ_PACKET_HANDLER(PKT_C2S_TurrerAngles);
	}

	return FALSE;
}

int obj_Vehicle::GetRandomVehicleTypeForSpawn()
{
	return obj_Vehicle::VEHICLETYPE_BUGGY;
}

void obj_Vehicle::OnNetPacket(const PKT_C2C_VehicleMoveSetCell_s& n)
{
	if(gServerLogic.ginfo_.mapId != GBGameInfo::MAPID_ServerTest && n.pos.Length() < 10)
	{
		obj_ServerPlayer* player = playersInVehicle[0];
		if (player)
		{			
			gServerLogic.LogCheat(player->peerId_, PKT_S2C_CheatWarning_s::CHEAT_Data, true, "ZeroTeleport",
				"%f %f %f", 
				n.pos.x, n.pos.y, n.pos.z);
		}
		return;
	}
	
	lastMovementPos = netMover.SrvGetCell();

	// for now we will check ONLY ZX, because somehow players is able to fall down
	r3dPoint3D p1 = netMover.SrvGetCell();
	r3dPoint3D p2 = n.pos;
	p1.y = 0;
	p2.y = 0;
	float dist = (p1 - p2).Length();
	
	if (dist > vd->GetMaxSpeed() * 3.0f) // temporary value, this number does need to be better balanced throughout testing.
	{
		obj_ServerPlayer* player = playersInVehicle[0];
		if (player && player->loadout_->Alive)
		{	
			gServerLogic.LogCheat(player->peerId_, PKT_S2C_CheatWarning_s::CHEAT_FastMove, true, (dist > 500.0f ? "huge_vehicle_teleport" : "vehicle_teleport"),
				"%f, srvGetCell: %.2f, %.2f, %.2f; n.pos: %.2f, %.2f, %.2f", 
				dist, 
				netMover.SrvGetCell().x, netMover.SrvGetCell().y, netMover.SrvGetCell().z, 
				n.pos.x, n.pos.y, n.pos.z
			);
		}
	}
	
	netMover.SetCell(n);

	SetPosition(n.pos);
	UpdatePlayers(n.pos);

	// keep them guaranteed
	RelayPacket(&n, sizeof(n), true);
}

void obj_Vehicle::OnNetPacket(PKT_C2C_VehicleMoveRel_s& n)
{
	const CVehicleNetCellMover::moveData_s& md = netMover.DecodeMove(n);
	
	lastMovementPos = md.pos;

	r3dPoint3D p1 = GetPosition();
	r3dPoint3D p2 = md.pos;
	p1.y = 0;
	p2.y = 0;
	float dist = (p1 - p2).Length();

	n.speed = (int)movementSpeed;

	accumDistance += dist;
		
	SetPosition(md.pos);
	UpdatePlayers(md.pos);
	SetRotationVector(r3dVector(md.rot.x, md.rot.y, md.rot.z));
	
	RelayPacket(&n, sizeof(n), false);
}

void obj_Vehicle::OnNetPacket(const PKT_C2C_VehicleAction_s& n)
{
	if (n.action & 1)
	{
		TryUnstuck();
	}
	else if (n.action & 2)
	{
		PKT_C2C_VehicleAction_s n2;
		n2.action = n.action;
		gServerLogic.p2pBroadcastToActive(this, &n2, sizeof(n2));

		obj_ServerPlayer* player = playersInVehicle[0];
		gServerLogic.InformZombiesAboutSoundItemID(player, WeaponConfig::ITEMID_AirHorn);
	}
	else
	{
		RelayPacket(&n, sizeof(n), false);
	}
}

void obj_Vehicle::OnNetPacket(const PKT_C2S_TurrerAngles_s& n)
{
	camangle = n.camangle;
	cameraUpDown = n.cameraUpDown;

	RelayPacket(&n, sizeof(n), false);
}

void obj_Vehicle::OnNetPacket(const PKT_C2C_VehicleHeadlights_s& n)
{
	isHeadlightsEnabled = n.isHeadlightsEnabled;

	//RelayPacket(&n, sizeof(n), false);
	PKT_C2C_VehicleHeadlights_s n2;
	n2.isHeadlightsEnabled = n.isHeadlightsEnabled;
	gServerLogic.p2pBroadcastToActive(this, &n2, sizeof(n2));
}

void obj_Vehicle::RelayPacket(const DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered)
{
	DWORD peerId = -1;
	obj_ServerPlayer* player = playersInVehicle[0];
	if (player)
		peerId = player->peerId_;
	gServerLogic.RelayPacket(peerId, this, packetData, packetSize, guaranteedAndOrdered);
}


void obj_Vehicle::UpdatePlayers(r3dPoint3D pos)
{
	for (uint32_t i = 0; i < maxPlayerCount; ++i)
	{
		if (playersInVehicle[i])
		{
			r3d_assert(playersInVehicle[i]->IsInVehicle());
			playersInVehicle[i]->UpdatePosition(pos);
			playersInVehicle[i]->netMover.SrvSetCell(netMover.SrvGetCell());
		}
	}
}

r3dPoint3D obj_Vehicle::GetExitPosition(int seat)
{
	r3dVector position;
	
	if (seat == 0)
		position = GetPosition() + -(GetvRight() * 2.7f);
	else if (seat == 1)
		position = GetPosition() + (GetvRight() * 2.7f);
	else
		position = GetPosition() + -(GetvForw() * 3.4f);

	return position;
}

int obj_Vehicle::GetDurability()
{
	return curDurability;
}

int obj_Vehicle::GetDurabilityAsPercent()
{
	return (int)((float)curDurability / (float)maxDurability * 100.0f);
}

void obj_Vehicle::SetDurability(int val, bool isForced)
{
	if (!isForced && val < 0)
	{
		if(r3dGetTime() > lastDamageTime + damageWaitTime)
			lastDamageTime = r3dGetTime();
		else
			return;
	}

	curDurability += val;

	if (curDurability > maxDurability)
		curDurability = maxDurability;

	if (curDurability <= 0)
	{
		curDurability = 0;
		OnExplode();
	}

	PKT_S2C_VehicleDurability_s n;
	n.durability = curDurability;

	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
}


void obj_Vehicle::StartDespawn()
{
	if (despawnStarted)
		return;

	despawnStarted = true;

	if (curDurability > 300)
		curDurability = 300;

	PKT_S2C_VehicleDurability_s n;
	n.durability = curDurability;

	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
}

void obj_Vehicle::ForceExplosion()
{
	if (despawnStarted)
		return;

	despawnStarted = true;

	curDurability = 5;

	PKT_S2C_VehicleDurability_s n;
	n.durability = curDurability;

	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
}

void obj_Vehicle::OnExplode()
{
	hasExploded = true;
	explosionTime = r3dGetTime();

	if (isHeadlightsEnabled == true)
	{
		isHeadlightsEnabled  = false;
		PKT_C2C_VehicleHeadlights_s n2;
		n2.isHeadlightsEnabled = isHeadlightsEnabled ;
		gServerLogic.p2pBroadcastToActive(this, &n2, sizeof(n2));
	}

	for (uint32_t i = 0; i < maxPlayerCount; ++i)
	{
		obj_ServerPlayer* player = playersInVehicle[i];
		if (player)
		{
			gServerLogic.ApplyDamage(this, player, GetPosition(), 1000.0f, true, storecat_Vehicle, vehicleId);
			player->ExitVehicle(true, true);
		}
	}

	gServerLogic.DoExplosion(this, this, R3D_ZERO_VECTOR, R3D_ZERO_VECTOR, 360.0f, 10.0f, 250.0f, storecat_Vehicle, vehicleId, true);
	gServerLogic.InformZombiesAboutVehicleExplosion(this);
}

bool obj_Vehicle::ApplyDamage(GameObject* fromObj, float damage, STORE_CATEGORIES damageSource)
{
	if (damageSource == storecat_GRENADE) // double damage for grenades against vehicles
	{
		float modifier = 20.0f; 
		if (vehicleType == VEHICLETYPE_STRYKER)
			modifier = 10.0f;
		SetDurability(-(int)(damage * modifier));
	}
	else
	{
		SetDurability(-(int)(damage * armorExteriorValue));
	}

	if (GetDurability() <= 0)
		return true; // return true if destroyed

	// do not cause any damage to players inside a stryker, or on vehicles that have health above their ignore threshold
	if (vehicleType == VEHICLETYPE_STRYKER || 
		vehicleType == VEHICLETYPE_BONECRUSHER || 
		vehicleType == VEHICLETYPE_MILITARYAPC ||
		GetDurabilityAsPercent() > vd->thresholdIgnoreMelee || 
		GetDurabilityAsPercent() > vd->thresholdIgnoreMelee)
		return false;

	for (uint32_t i = 0; i < maxPlayerCount; ++i)
	{
		if (!playersInVehicle[i])
			continue;

		// players should take more damage from other players, but less from zombies
		obj_ServerPlayer* player = playersInVehicle[i];	
		if (player)
		{
			r3d_assert(player->currentVehicleId == vehicleId);
			r3d_assert(player->IsInVehicle());

			float appliedDamage = 0;
			if (fromObj->isObjType(OBJTYPE_Zombie))
				appliedDamage = u_GetRandom(0.0f, 2.0f);
			else if (fromObj->isObjType(OBJTYPE_Human))
				appliedDamage = u_GetRandom(0.0f, 4.0f);
			
			gServerLogic.ApplyDamage(fromObj, player, GetPosition(), appliedDamage, true, damageSource, vehicleId, false);
		}
	}

	return false;
}

int obj_Vehicle::GetVehicleType()
{
	return vehicleType;
}

void obj_Vehicle::OnRunOutOfFuel()
{
	if (hasRunOutOfFuel)
		return;

	hasRunOutOfFuel = true;

	SendFuelToClients();
}

void obj_Vehicle::AddFuel(int amount)
{
	curFuel += amount;

	if (curFuel > maxFuel)
		curFuel	 = maxFuel;

	if (curFuel > 0)
		hasRunOutOfFuel = false;

	SendFuelToClients();
}

void obj_Vehicle::SendFuelToClients()
{
	PKT_S2C_VehicleFuel_s n;
	n.fuel = curFuel;
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
}

int obj_Vehicle::GetFuel()
{
	return curFuel;
}

int obj_Vehicle::GetSpeed()
{
	return (int)movementSpeed;
}

void obj_Vehicle::LoadXML()
{
	if( !vic )
	{
		r3dOutToLog("!!! Failed to load vehicle definition file!  Vehicle Info Config is un-initialized or not found!\n");
		return;
	}
	if( !vd )
	{
		r3dOutToLog("!!! Failed to load vehicle definition file!  Vehicle Descriptor is un-initialized!\n");
		return;
	}

	vd->driveFileDefinitionPath = "Data/ObjectsDepot/Vehicles/";
	vd->driveFileDefinitionPath += vic->m_FNAME;
	vd->driveFileDefinitionPath += "_DriveData.xml";

	bool checkG3_Vehicles = false;
	r3dFile* f = r3d_open(vd->driveFileDefinitionPath.c_str(), "rb");
	if ( !f )
	{
		r3dOutToLog("Failed to open vehicle definition file: %s\n", vd->driveFileDefinitionPath.c_str());
		vd->driveFileDefinitionPath = "Data/ObjectsDepot/G3_Vehicles/";
		vd->driveFileDefinitionPath += vic->m_FNAME;
		vd->driveFileDefinitionPath += "_DriveData.xml";
		r3dOutToLog("Redirecting to: %s\n", vd->driveFileDefinitionPath.c_str());
		checkG3_Vehicles = true;
	}

	if (checkG3_Vehicles == true)
		f = r3d_open(vd->driveFileDefinitionPath.c_str(), "rb");
	if ( !f && checkG3_Vehicles == true)
	{
		r3dOutToLog("Failed to open vehicle definition file: %s\n", vd->driveFileDefinitionPath.c_str());
		return;
	}

	char* fileBuffer = game_new char[f->size + 1];
	fread(fileBuffer, f->size, 1, f);
	fileBuffer[f->size] = 0;

	pugi::xml_document xmlDoc;
	pugi::xml_parse_result parseResult = xmlDoc.load_buffer_inplace(fileBuffer, f->size);
	fclose(f);
	if (!parseResult)
		r3dError("Failed to parse XML, error: %s", parseResult.description());
	vd->ReadSerializedData( xmlDoc );
	delete [] fileBuffer;
}

void obj_Vehicle::CreateNavMeshObstacle()
{
	if (obstacleId >= 0)
		return;

	r3dBoundBox obb;
	obb.Size = r3dPoint3D(3.5f, 2.0f, 3.5f); // this is enough space to allow a player to at least have a few extra seconds to get into a "parked" vehicle.
	obb.Org  = r3dPoint3D(GetPosition().x - obb.Size.x/2, GetPosition().y, GetPosition().z - obb.Size.z/2);
	obstacleId = gAutodeskNavMesh.AddObstacle(this, obb, GetRotationVector().y);

	obstacleRadius = R3D_MAX(obb.Size.x, obb.Size.z) / 2;
}

void obj_Vehicle::DeleteNavMeshObstacle()
{
	if(obstacleId >= 0)
	{
		gAutodeskNavMesh.RemoveObstacle(obstacleId);
		obstacleId = -1;
	}
}
#endif
