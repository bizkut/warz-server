#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"
#include "ObjectsCode/Weapons/WeaponArmory.h"
#include "ObjectsCode/Weapons/HeroConfig.h"

#include "sobj_Zombie.h"
#include "sobj_ZombieSpawn.h"
#include "ObjectsCode/obj_ServerGrenade.h"
#include "ObjectsCode/obj_ServerPlayer.h"
#include "ObjectsCode/sobj_DroppedItem.h"
#include "ObjectsCode/obj_ServerBarricade.h"
#include "ServerWeapons/ServerWeapon.h"
#include "r3dInterpolator.h"
#include "../obj_ServerDoor.h"

#include "../../EclipseStudio/Sources/ObjectsCode/Gameplay/ZombieStates.h"

#include "../../GameEngine/ai/AutodeskNav/AutodeskNavMesh.h"
#include "../../GameEngine/ai/AI_Brain.h"

#ifdef VEHICLES_ENABLED
#include "../Vehicles/obj_VehicleSpawnPoint.h"
#include "../Vehicles/obj_Vehicle.h"
#endif

#ifdef ENABLE_GAMEBLOCKS
#include "GBClient/Inc/GBClient.h"
#include "GBClient/Inc/GBReservedEvents.h"

extern GameBlocks::GBClient* g_GameBlocks_Client;
extern GameBlocks::GBPublicSourceId g_GameBlocks_ServerID;
#endif //ENABLE_GAMEBLOCKS

extern const uint32_t	g_SuperZombieItemID		= 20207;
const uint32_t			g_DogZombieItemID		= 20219;

IMPLEMENT_CLASS(obj_Zombie, "obj_Zombie", "Object");
AUTOREGISTER_CLASS(obj_Zombie);

#if 0 //@ 
	//DEBUG VARIABLES
	int		_zai_DisableDetect       = 1;
	float		_zai_IdleStatePatrolPerc = 1.0f;
	float		_zai_SuperZombieIdleStatePatrolPerc	= 1.0f;
	float		_zai_NoPatrolPlayerDist  = -1.0f;
	float		_zai_SafeSuicideDist     = 150.0f;	// distance for players check for stucked (ourside of spawn area) zombies
	float		_zai_PlayerSpawnProtect  = 0.0f;
	float		_zai_MaxSpawnDelay       = 0.1f;
	float		_zai_AttackDamage        = 0.0f;
	float		_zai_SuperZombieSuperAttackDamage	= 0.0f;
	int		_zai_DebugAI             = 1;
#else
	int		_zai_DisableDetect						= 0;
	float		_zai_IdleStatePatrolPerc			= 0.4f;
	float		_zai_SuperZombieIdleStatePatrolPerc	= 0.6f;
	float		_zai_NoPatrolPlayerDist				= 500.0;
	float		_zai_SafeSuicideDist				= 150.0f;	// distance for players check for stucked (ourside of spawn area) zombies
	float		_zai_PlayerSpawnProtect				= 35.0f;	// radius where zombie won't be spawned because of player presense
	float		_zai_MaxSpawnDelay					= 9999.0f;
	float		_zai_AttackDamage					= 23.0f;
	float		_zai_SuperZombieSuperAttackDamage	= 28.75f;	// 1.25 * _zai_AttackDamage
	int		_zai_DebugAI							= 0;
#endif
	float		_zai_MaxPatrolDistance				= 30.0f;	// distance to search for navpoints when switchint to patrol
	float		_zai_MaxPursueDistance				= 300.0f;
	float		_zai_AttackRadius					= 1.2f;
	float		_zai_SZAttackRadius					= 2.4f;		// super zombie attack radius
	float		_zai_AttackVerticalLimit			= 3.0f;		// maximum distance above or below a zombie can reach the player (works for Super Zombie only right now)
	float		_zai_AttackTimer					= 1.2f;
	float		_zai_SuperZombieAttackTimer			= 1.033f;
	float		_zai_SuperZombieSuperAttackTimer	= 1.2f;
	float		_zai_DistToRecalcPath				= 0.8f;		// _zai_AttackRadius / 2
	float		_zai_VisionConeCos					= cosf(50.0f); // have 100 degree vision
	float		_zai_ChanceOfInfecting				= 3.0f;		// 3% chance
	float		_zai_InfectStrength					= 4.0f;		// add X to toxicity of blood on each successful infection

	float		_zai_CallForHelpTime				= 2.5f;		// amount of time to run call for help state before starting attack (This must match the length of the animation)
	float		_zai_SZCallForHelpTime				= 3.04f;	// amount of time to run call for help state before starting attack (This must match the length of the animation)
	float		_zai_SZChestBeatTime				= 5.57f;	// amount of time to run Super Zombie Chest Beat (This must match the length of the animation)
	float		_zai_SZAlert						= 3.37f;	// amount of time to run Super Zombie Alert (This must match the length of the animation)
	float		_zai_SZAlertSniff					= 4.03f;	// amount of time to run Super Zombie Alert Sniff (This must match the length of the animation)
	float		_zai_SZHealTimeMin					= 4.0f;		// minimum time between Super Zombie's passive heals.
	float		_zai_SZHealTimeMax					= 5.0f;		// maximum time between Super Zombie's passive heals;
	float		_zai_SZHealRate						= 0.05f;	// percent of max health the Super Zombie will heal each time the passive heal takes effect.

	int		_zstat_NumZombies = 0;
	int		_zstat_NavFails   = 0;
	int		_zstat_Disabled   = 0;

	const float	_zstat_MaxHealth = 100;
	const float _zstat_MaxHealthSuperZombie = 3000;
	const float _zstat_MaxHealthDogZombie = 1000;

#ifdef ZOMBIE_PERFORMANCE_TESTING
	uint32_t	g_ZombieFrameStartCalcPathCount;
	float		g_ZombieFrameStartCalcTime;
	uint32_t	g_ZombieFrameCalculatingPathCount;
	float		g_ZombieFrameCalculatingTime;
	uint32_t	g_ZombieFrameBrainsCalcCount;
	float		g_ZombieFrameBrainsCalculatingTime;
	uint32_t	g_ZombieFrameGetClosestTargetCount;
	float		g_ZombieFrameGetClosestTargetTime;
	uint32_t	g_ZombieFrameGetClosestPlayerCount;
	float		g_ZombieFrameGetClosestPlayerTime;
	uint32_t	g_ZombieFrameCheckViewToPlayerCount;
	float		g_ZombieFrameCheckViewToPlayerTime;
#endif

static uint32_t nextZombieIDForFF = 1;

std::list<obj_Zombie*> obj_Zombie::s_ListOfAllActiveZombies;

extern ObjectGroupings zombieGroups;

obj_Zombie::obj_Zombie()
	: spawnObject( NULL )
	, netMover(this, 1.0f / 10.0f, (float)PKT_C2C_MoveSetCell_s::PLAYER_CELL_RADIUS)
	, HalloweenZombie( false )
	, HeroItemID( -1 )
	, HeadIdx( -1 )
	, BodyIdx( -1 )
	, LegsIdx( -1 )
	, ZombieUniqueIDForFF( nextZombieIDForFF++ )
	, ZombieDisabled( 0 )
	, ZombieState( EZombieStates::ZState_Idle )
	, StateStartTime( r3dGetTime() )
	, StateTimer( -1.0f )
	, HealTimer( -1.0f )
	, AvoidanceIdleStartTime( -1.0f )
	, ZombieHealth( _zstat_MaxHealth )
	, FastZombie( false )
	, WalkSpeed( -1.0f )
	, RunSpeed( -1.0f )
	, CanSprint( false )
	, SprintSpeed( -1.0f )
	, SprintTimer( -1.0f )
	, SprintFalloffTimer( -1.0f )
	, SprintCooldownTimer( -1.0f )
	, navAgent( NULL )
	, patrolPntIdx( -1 )
	, moveFrameCount( 0 )
	, staggerTime( -1.0f )
	, animState( 0 )
	, DetectRadius( -1.0f )
	, hardObjLock( invalidGameObjectID )
	, prevObjLock( invalidGameObjectID )
	, nextDetectTime( r3dGetTime() + 2.0f )
	, attackCounter( 0 )
	, superAttackDir( 0 )
	, isSuperZombieForced(false)
	, idleFidget( EZombieStates::ZFidget_None )
{
	_zstat_NumZombies++;

	distToCreateSq = 200 * 200;
	distToDeleteSq = 300 * 300;

	ZombieGroup = zombieGroups.InsertAtNextAvailableIndex();

	s_ListOfAllActiveZombies.push_back(this);
}

obj_Zombie::~obj_Zombie()
{
	s_ListOfAllActiveZombies.erase(std::find(s_ListOfAllActiveZombies.begin(), s_ListOfAllActiveZombies.end(), this));

	_zstat_NumZombies--;
	if(ZombieDisabled)
		_zstat_Disabled--;
}

BOOL obj_Zombie::OnCreate()
{
	//r3dOutToLog("obj_Zombie %p %s created\n", this, Name.c_str()); CLOG_INDENT;
	
	ObjTypeFlags |= OBJTYPE_Zombie;
	
	r3d_assert(NetworkLocal);
	r3d_assert(GetNetworkID());
	r3d_assert(spawnObject);
	
	if(u_GetRandom() < spawnObject->sleepersRate)
		ZombieState = EZombieStates::ZState_Sleep;

	if(spawnObject->ZombieSpawnSelection.size() == 0)
		HeroItemID = u_GetRandom() >= 0.5f ? 20170 : 20183; // old behaviour. TODO: remove
	else
	{
		if (isSuperZombieForced)
		{
			HeroItemID = 20207;
		}
		else
		{
			uint32_t idx1 = u_random(spawnObject->ZombieSpawnSelection.size());
			r3d_assert(idx1 < spawnObject->ZombieSpawnSelection.size());
			HeroItemID = spawnObject->ZombieSpawnSelection[idx1];
		}
	}

	const HeroConfig* heroConfig = g_pWeaponArmory->getHeroConfig(HeroItemID);
	if(!heroConfig) { 
		r3dOutToLog("!!!! unable to spawn zombie - there is no hero config %d\n", HeroItemID);
		return FALSE;
	}
	
	if( IsSuperZombie() )
	{
// 		if(gServerLogic.ginfo_.channel!=6)
// 		{
// 			return FALSE; // spawn super zombie ONLY on PTE maps
// 		}
		ZombieHealth	= _zstat_MaxHealthSuperZombie;
		FastZombie		= true;
		WalkSpeed		= 1.9f;
		RunSpeed		= 5.4f;
		//WalkSpeed		+= WalkSpeed * u_GetRandom(-spawnObject->speedVariation, +spawnObject->speedVariation);
		//RunSpeed		+= RunSpeed  * u_GetRandom(-spawnObject->speedVariation, +spawnObject->speedVariation);
	}
	else if ( IsDogZombie() )
	{
		ZombieHealth	= _zstat_MaxHealthDogZombie;
		FastZombie		= true;
		WalkSpeed		= 1.9f;
		RunSpeed		= 5.4f;
	}
	else
	{
		FastZombie = u_GetRandom() > spawnObject->fastZombieChance;

		//HalloweenZombie = u_GetRandom() < (15.0f / 100.0f) ? true : false; // every Xth zombie is special
		//if(HalloweenZombie) FastZombie = 1;

		WalkSpeed   = FastZombie ? 1.8f : 1.0f;
		RunSpeed    = FastZombie ? 3.2f : 2.9f;
		
		WalkSpeed += WalkSpeed * u_GetRandom(-spawnObject->speedVariation, +spawnObject->speedVariation);
		RunSpeed  += RunSpeed  * u_GetRandom(-spawnObject->speedVariation, +spawnObject->speedVariation);
		
		r3d_assert(WalkSpeed > 0);
		r3d_assert(RunSpeed > 0);
		r3d_assert(DetectRadius >= 0);
		
	}

	CanSprint = IsSuperZombie() ? false : ( IsDogZombie()?false:spawnObject->m_SprintersSpawnPerc >= u_GetRandom(0.0f, 1.0f) );

	HeadIdx = u_random(heroConfig->getNumHeads());
	BodyIdx = u_random(heroConfig->getNumBodys());
	LegsIdx = u_random(heroConfig->getNumLegs());
	turnUserData.nextState = EZombieStates::ZState_Idle;
	turnUserData.targetID = invalidGameObjectID;

	// need to create nav agent so it will be placed to navmesh position
	CreateNavAgent();
	
#ifdef NEW_AI
	// Assign a random brain configuration, from the ones selected for the spawner
	uint32_t numBrains = spawnObject->ZombieBrainSelection.size();
	if( numBrains > 0 )
		navAgent->m_brain->m_BrainConfigID = spawnObject->ZombieBrainSelection[ u_random( numBrains ) ];
#endif // NEW_AI

	gServerLogic.NetRegisterObjectToPeers(this);

	return parent::OnCreate();
}

BOOL obj_Zombie::OnDestroy()
{
	//r3dOutToLog("obj_Zombie %p destroyed\n", this);

	zombieGroups.RemoveFromIndex( ZombieGroup );

	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	
	DeleteZombieNavAgent(navAgent);
	navAgent = NULL;
	
	return parent::OnDestroy();
}

void obj_Zombie::DisableZombie()
{
	AILog(0, "DisableZombie\n");
	StopNavAgent();

	ZombieDisabled = true;

	//DeleteZombieNavAgent(navAgent);
	//navAgent = NULL;

	_zstat_Disabled++;
	_zstat_NavFails++;
}

void obj_Zombie::AILog(int level, const char* fmt, ...)
{
	if(level > _zai_DebugAI)
		return;
		
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	StringCbVPrintfA(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	
	r3dOutToLog("AIZombie%p[%d] %s", this, navAgent->m_navBot->GetVisualDebugId(), buf);
}

void obj_Zombie::CreateNavAgent()
{
	r3d_assert(!navAgent);

	// there is no checks here, they should be done in ZombieSpawn, so pos is navmesh position
	r3dPoint3D pos = GetPosition();
	navAgent = CreateZombieNavAgent(this);

	//AILog(3, "created at %f %f %f\n", GetPosition().x, GetPosition().y, GetPosition().z);	
	
	return;
}

void obj_Zombie::StopNavAgent()
{
	if(!navAgent) return;
		
	if(patrolPntIdx >= 0)
	{
		spawnObject->ReleaseNavPoint(patrolPntIdx);
		patrolPntIdx = -1;
	}
	
	navAgent->StopMove();
}


void obj_Zombie::SetNavAgentSpeed(float speed)
{
	if(!navAgent) return;

	navAgent->SetTargetSpeed(speed);
}

bool obj_Zombie::MoveNavAgent(const r3dPoint3D& pos, float maxAstarRange)
{
	if(!navAgent)
		return false;

	AILog(4, "navigating to %f %f %f from %f %f %f\n", pos.x, pos.y, pos.z, GetPosition().x, GetPosition().y, GetPosition().z);
	
#ifdef ZOMBIE_PERFORMANCE_TESTING
	float startCalcPathTime = r3dGetTime();
#endif
	if(!navAgent->StartMove(pos, maxAstarRange))
		return false;

#ifdef ZOMBIE_PERFORMANCE_TESTING
	g_ZombieFrameStartCalcTime += r3dGetTime() - startCalcPathTime;
	++g_ZombieFrameStartCalcPathCount;
#endif

	moveFrameCount = 0;

	moveWatchTime  = 0;
	moveWatchPos   = GetPosition();
	moveStartPos   = GetPosition();
	moveTargetPos  = pos;
	moveStartTime  = r3dGetTime();
	moveAvoidTime  = 0;
	moveAvoidPos   = GetPosition();

	SendAIStateToNet();

	return true;
}

int obj_Zombie::CheckMoveWatchdog()
{
	if(navAgent->m_status == AutodeskNavAgent::ComputingPath)
		return 1;
	if(navAgent->m_status != AutodeskNavAgent::Moving)
		return 2;
	if(!IsSuperZombie() && staggerTime > 0)
		return 1;
		
	// check every 5 sec that we moved somewhere
	moveWatchTime += r3dGetFrameTime();
	if(moveWatchTime < 5)
		return 1;
	
	if((GetPosition() - moveWatchPos).Length() < 0.5f)
	{
		AILog(1, "!!! move watchdog %d %d %d at %f %f %f\n", //@ was 1
			navAgent->m_navBot->GetProgressOnLivePathStatus(), 
			navAgent->m_navBot->GetLivePath().GetPathValidityStatus(),
			navAgent->m_navBot->GetTrajectory() ? navAgent->m_navBot->GetTrajectory()->m_avoidanceResult : -1,
			GetPosition().x, GetPosition().y, GetPosition().z);
		//DisableZombie();
		return 0;
	}

	moveWatchTime = 0;
	moveWatchPos  = GetPosition();
	return 1;
}

int obj_Zombie::CheckMoveStatus()
{
	float curTime = r3dGetTime();
	
	switch(navAgent->m_status)
	{
		case AutodeskNavAgent::Idle:
			return 2;
			
		case AutodeskNavAgent::ComputingPath:
#ifndef NEW_AI
			if(curTime > moveStartTime + 5.0f)
			{
				AILog(0, "!!! ComputingPath for %f\n", curTime - moveStartTime);
				_zstat_NavFails++;
				StopNavAgent();
				return 2;
			}
#endif // NEW_AI
#ifdef ZOMBIE_PERFORMANCE_TESTING
			++g_ZombieFrameCalculatingPathCount;
#endif
			return 1;
		
		case AutodeskNavAgent::PathNotFound:
		{
			Kaim::BaseAStarQuery* baseAStarQuery = (Kaim::BaseAStarQuery*)navAgent->m_navBot->GetPathFinderQuery();
			AILog(5, "PATH_NOT_FOUND %d to %f,%f,%f from %f,%f,%f\n", 
				baseAStarQuery->GetResult(),
				baseAStarQuery->GetDestPos().x, baseAStarQuery->GetDestPos().z, baseAStarQuery->GetDestPos().y,
				moveStartPos.x, moveStartPos.y, moveStartPos.z);

			StopNavAgent();
			return 2;
		}
			
		case AutodeskNavAgent::Moving:
			if( navAgent->IsAvoidanceResultChanged() )
			{
				if( ZombieState == EZombieStates::ZState_Walk ||
					ZombieState == EZombieStates::ZState_Pursue ||
					ZombieState == EZombieStates::ZState_PursueSprint )
				{
					AutodeskNavAgentEnums::EAvoidanceResult avoidResult = navAgent->GetAvoidanceResult();
					switch( avoidResult )
					{
					// Resume walking or running; Sprints have been cancelled.
					case AutodeskNavAgentEnums::NoAvoidance:
						AvoidanceIdleStartTime = -1.0f;
						switch( ZombieState )
						{
						case EZombieStates::ZState_Walk:
							SetNavAgentSpeed(WalkSpeed);
							break;
						case EZombieStates::ZState_Pursue:
						case EZombieStates::ZState_PursueSprint:
							SetNavAgentSpeed(RunSpeed);
							break;
						}
						break;

					// TODO: The avoidance system switches between SlowDown
					//		 and NoAvoidance a lot, causing the animations
					//		 to look real jittery, so this is commented out
					//		 here and in the client obj_Zombie until a solution
					//		 can be worked out.
					//// Slow down zombies that have been told to slow.
					//case AutodeskNavAgentEnums::SlowDown:
					//	// Can't go any slower for Walking zombies
					//	// right now, so this will have no effect
					//	// on them, but for Running or Sprinting
					//	// zombies, it will slow them down.
					//	switch( ZombieState )
					//	{
					//	case EZombieStates::ZState_Pursue:
					//	case EZombieStates::ZState_PursueSprint:
					//		SetNavAgentSpeed(WalkSpeed);
					//		CancelSprint();
					//		break;
					//	}
					//	break;

					// Stop zombies that are continuing to play walk, run
					// and sprint animations, even though they are not moving.
					case AutodeskNavAgentEnums::Stop:
						AvoidanceIdleStartTime = r3dGetTime();
						SetNavAgentSpeed( 0.0f );
						CancelSprint();
						break;

					default:
						break;
					}
				}
			}
			// Prevent an indefinite avoidance induced idle.
			if( 0.0f < AvoidanceIdleStartTime &&
				r3dGetTime() - AvoidanceIdleStartTime > 3.0f )
			{
				AvoidanceIdleStartTime = -1.0f;
				return 2;
			}

			// break to next check for path status
			break;
			
		case AutodeskNavAgent::Arrived:
			return 0;

		case AutodeskNavAgent::Failed:
			AILog(5, "!!! Failed with %d\n", navAgent->m_navBot->GetProgressOnLivePathStatus());
			return 2;
	}
	
	//Kaim::ProgressOnPathStatus status = navAgent->m_navBot->GetProgressOnLivePathStatus();
	//in 2014.0.5 this status became totally pointless

	return 1;
}

GameObject* obj_Zombie::FindDoor()
{
	for(GameObject* targetObj=GameWorld().GetFirstObject(); targetObj; targetObj=GameWorld().GetNextObject(targetObj))
	{
		if (targetObj->Class->Name == "obj_Door")
		{
			obj_Door* shield = (obj_Door*)targetObj;
			
			// fast discard by radius

			if((GetPosition() - shield->GetPosition()).Length() < 1.0)
			{
				//r3dOutToLog("###### PRUEBA %f\n",(GetPosition() - shield->GetPosition()).Length());
				if (shield->m_Healt<=0)
					return NULL;
				else
					return shield;
			}
		}
	}
	return NULL;
}

GameObject* obj_Zombie::FindBarricade()
{
	for(std::vector<obj_ServerBarricade*>::iterator it = obj_ServerBarricade::allBarricades.begin(); it != obj_ServerBarricade::allBarricades.end(); ++it)
	{
		obj_ServerBarricade* shield = *it;

		// fast discard by radius
		if((GetPosition() - shield->GetPosition()).Length() > shield->m_Radius + _zai_AttackRadius)
			continue;

		// get obstacle, TODO: rework to point vs OBB logic
		Kaim::WorldElement* e = gAutodeskNavMesh.obstacles[shield->m_ObstacleId];
		r3d_assert(e);
		if(e->GetType() != Kaim::TypeBoxObstacle)
			continue;
		Kaim::BoxObstacle* boxObstacle = static_cast<Kaim::BoxObstacle*>(e);
		
		// search for every spatial cylinder there
		for(KyUInt32 cidx = 0; cidx < boxObstacle->GetSpatializedCylinderCount(); cidx++)
		{
			const Kaim::SpatializedCylinder& cyl = boxObstacle->GetSpatializedCylinder(cidx);
			r3dPoint3D p1 = r3dPoint3D(GetPosition().x, 0, GetPosition().z);
			r3dPoint3D p2 = r3dPoint3D(cyl.GetPosition().x, 0, cyl.GetPosition().y); // KY_R3D
			float dist = (p1 - p2).Length() - cyl.GetRadius();
			if(dist < _zai_AttackRadius * 0.7f)
			{
				return shield;
			}
		}
	}
	return NULL;
}

bool obj_Zombie::CheckForBarricadeBlock()
{
	if(navAgent->m_navBot->GetTrajectory() == NULL)
		return false;
		
	if( IsSuperZombie() )
	{
		// Don't even try to avoid the barricade, just destroy it!
		float dist = (GetPosition() - moveAvoidPos).Length();
		moveAvoidTime = 0;
		moveAvoidPos  = GetPosition();

		float speed = GetVelocity().Length();

		float minDistToMove  = ((speed >= RunSpeed) ? RunSpeed : WalkSpeed) * 5.5f;
		if(dist >= minDistToMove)
			return false;
		
		GameObject* shield = FindBarricade();
		if(!shield)
			return false;

		// found barricade, wreck it!
		if( hardObjLock != invalidGameObjectID )
		{
			GameObject* trg = GameWorld().GetObject(hardObjLock);
			if( trg && trg->isObjType( OBJTYPE_Human 
#if VEHICLES_ENABLED
				|| OBJTYPE_Vehicle
#endif
				) )
				prevObjLock = hardObjLock;
		}
		SetTarget( shield->GetSafeID() );
		attackTimer = _zai_AttackTimer / 2;
		r3dOutToLog("!!! Moving Attack Start: CurSpeed(%f) TargetSpeed(%f)\n", speed, RunSpeed);
		navAgent->SetTargetSpeed( RunSpeed );
		SwitchToState(EZombieStates::ZState_MovingAttack);
		return true;
	}
	else
	{
		// we detect barricade by checking for them every sec if we're in avoidance mode.
		Kaim::IAvoidanceComputer::AvoidanceResult ares = navAgent->m_navBot->GetTrajectory()->GetAvoidanceResult();
		if(ares == Kaim::IAvoidanceComputer::NoAvoidance)
		{
			moveAvoidTime = 0;
			moveAvoidPos  = GetPosition();
			return false;
		}

		moveAvoidTime += r3dGetFrameTime();
		
		float avoidTimeCheck = 1.0f;
		float minDistToMove  = WalkSpeed * avoidTimeCheck * 0.8f;
		//r3dOutToLog("%f %f vs %f\n", moveAvoidTime, (GetPosition() - moveAvoidPos).Length(), minDistToMove);
		if(moveAvoidTime >= avoidTimeCheck)
		{
			float dist = (GetPosition() - moveAvoidPos).Length();
			moveAvoidTime = 0;
			moveAvoidPos  = GetPosition();

			if(dist >= minDistToMove)
				return false;
			
			GameObject* shield = FindBarricade();
			if(!shield)
				return false;
				
			StopNavAgent();
			
			// found barricade, wreck it!
			SetTarget( shield->GetSafeID() );
			attackTimer = _zai_AttackTimer / 2;
			SwitchToState(EZombieStates::ZState_BarricadeAttack);
			return true;
		}
	}

	return false;
}


void obj_Zombie::FaceVector(const r3dPoint3D& v)
{
	float angle = atan2f(-v.z, v.x);
	angle = R3D_RAD2DEG(angle);
	SetRotationVector(r3dVector(angle - 90, 0, 0));
}

r3dVector obj_Zombie::GetFacingVector() const
{
	r3dVector fwd( 0, 0, -1 );
	fwd.RotateAroundY( GetRotationVector().x );
	return fwd;
}

r3dVector obj_Zombie::GetRightVector() const
{
	r3dVector fwd( 0, 0, -1 );
	fwd.RotateAroundY( GetRotationVector().x + 90.0f );
	return fwd;
}

GameObject* obj_Zombie::ScanForTarget(bool immediate)
{
	if(ZombieDisabled || _zai_DisableDetect)
		return NULL;
		
	const float curTime = r3dGetTime();

	// Is this zombie allowed to Update this turn?
	if( !immediate && ZombieGroup != zombieGroups.activeGroupNdx )
		return NULL;

	return GetClosestTargetBySenses();
}

void obj_Zombie::SetTarget(const gobjid_t& objSafeID)
{
	// Has the target changed?
	if( objSafeID != hardObjLock )
	{
		hardObjLock = objSafeID;

		// If the target is valid, then get the target's network safe id.
		GameObject* targetObj = NULL;
		gp2pnetid_t target = invalidP2pNetID;
		if( objSafeID != invalidGameObjectID )
			targetObj = GameWorld().GetObject( objSafeID );
		if( targetObj )
		{
#ifdef VEHICLES_ENABLED
			if (targetObj->isObjType(OBJTYPE_Human) && ((obj_ServerPlayer*)targetObj)->IsInVehicle())
				targetObj = GameWorld().GetNetworkObject(((obj_ServerPlayer*)targetObj)->GetNetworkID());
#endif
			target = toP2pNetId( targetObj->GetSafeNetworkID() );
		}

		// Update the network.
		PKT_S2C_ZombieTarget_s n;
		n.targetId = target;
		gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	}
}

bool obj_Zombie::IsPlayerDetectable(const obj_ServerPlayer* plr, float distSq)
{
	if(plr->loadout_->Alive == 0)
		return false;
	//if(plr->profile_.ProfileData.isDevAccount & wiUserProfile::DAA_INVISIBLE)
	//	return false;
	if (plr->m_DevPlayerHide == true)
		return false;

	if(plr->m_isAdmin_GodMode)
		return false;
    
	if (plr->GodMode == 2)
		return false;  

	if (plr->m_ZombieRepelentTime > r3dGetTime())
		return false;

	if(r3dGetTime() < plr->m_SpawnProtectedUntil)
		return false;

	// Lowers detection radius when near zombies
	if(plr->loadout_->Skills[CUserSkills::SKILL_Survival12] && (plr->m_PlayerState != PLAYER_MOVE_SPRINT
		&& plr->m_PlayerState != PLAYER_SWIM_FAST
		))
		distSq *= 2.0f; // this skill lowers detection radius of zombies by 50%, so just double dist to player to simulate then (one check, instead of two)

	// detect by smell
	float detectRadiusSq = DetectRadius * DetectRadius; 
	if(distSq < detectRadiusSq) {
		return true;
	}

	// vision is disabled in sleep mode
	if(ZombieState == EZombieStates::ZState_Sleep)
		return false;

	// vision check: range
	const static float VisionDetectRangesByState[] = {
		 4.0f, //PLAYER_IDLE = 0,
		 4.0f, //PLAYER_IDLEAIM,
		 7.0f, //PLAYER_MOVE_CROUCH,
		 7.0f, //PLAYER_MOVE_CROUCH_AIM,
		10.0f, //PLAYER_MOVE_WALK_AIM,
		15.0f, //PLAYER_MOVE_RUN,
		30.0f, //PLAYER_MOVE_SPRINT,
		 4.0f, //PLAYER_MOVE_PRONE,
		 4.0f, //PLAYER_MOVE_PRONE_AIM,
		 1.8f, //PLAYER_PRONE_UP,
		 1.8f, //PLAYER_PRONE_DOWN,
		 1.8f, //PLAYER_PRONE_IDLE,
		 4.0f, //PLAYER_SWIM_IDLE,
		 7.0f, //PLAYER_SWIM_SLOW,
		15.0f, //PLAYER_SWIM,
		30.0f, //PLAYER_SWIM_FAST,
#ifdef VEHICLES_ENABLED
		100.0f, // VEHICLE DRIVER
		100.0f, // VEHICLE PASSENGER
#endif
		 0.0f, //PLAYER_DIE,
	};
	// vision check: range
	const static float SZVisionDetectRangesByState[] = {
		 8.0f, //PLAYER_IDLE = 0,
		 8.0f, //PLAYER_IDLEAIM,
		 14.0f, //PLAYER_MOVE_CROUCH,
		 14.0f, //PLAYER_MOVE_CROUCH_AIM,
		20.0f, //PLAYER_MOVE_WALK_AIM,
		30.0f, //PLAYER_MOVE_RUN,
		60.0f, //PLAYER_MOVE_SPRINT,
		 7.0f, //PLAYER_MOVE_PRONE,
		 7.0f, //PLAYER_MOVE_PRONE_AIM,
		 3.0f, //PLAYER_PRONE_UP,
		 3.0f, //PLAYER_PRONE_DOWN,
		 3.0f, //PLAYER_PRONE_IDLE,
		 8.0f, //PLAYER_SWIM_IDLE,
		 14.0f, //PLAYER_SWIM_SLOW,
		30.0f, //PLAYER_SWIM,
		60.0f, //PLAYER_SWIM_FAST,
#ifdef VEHICLES_ENABLED
		100.0f, // VEHICLE DRIVER
		100.0f, // VEHICLE PASSENGER
#endif
		 0.0f, //PLAYER_DIE,
	};
	COMPILE_ASSERT( R3D_ARRAYSIZE(VisionDetectRangesByState) == PLAYER_NUM_STATES);
	COMPILE_ASSERT( R3D_ARRAYSIZE(SZVisionDetectRangesByState) == PLAYER_NUM_STATES);
	
	if(plr->m_PlayerState < 0 || plr->m_PlayerState >= PLAYER_NUM_STATES) {
		r3dOutToLog("!!! bad state\n");
		return false;
	}
	if( IsSuperZombie() )
	{
		if(distSq > SZVisionDetectRangesByState[plr->m_PlayerState] * SZVisionDetectRangesByState[plr->m_PlayerState]) {
			return false;
		}
	}
	else
	{
		if(distSq > VisionDetectRangesByState[plr->m_PlayerState] * VisionDetectRangesByState[plr->m_PlayerState]) {
			return false;
		}
	}
	
	// vision check: cone of view
	/*{
		r3dPoint3D v1(0, 0, -1);
		v1.RotateAroundY(GetRotationVector().x);
		r3dPoint3D v2 = (plr->GetPosition() - GetPosition()).NormalizeTo();
		
		float dot = v1.Dot(v2);
		//float deg = R3D_RAD2DEG(acosf(dot));
		if(dot < _zai_VisionConeCos)
			return false;
	}*/
	
	// vision check: obstacles
	if(!CheckViewToPlayer(plr))
		return false;
		
	return true;
}

#ifdef VEHICLES_ENABLED
bool obj_Zombie::IsVehicleDetectable(obj_Vehicle* vehicle, float distance)
{
	if (vehicle->GetDurability() <= 0 || !vehicle->HasPlayers() || ZombieState == EZombieStates::ZState_Sleep)
		return false;

	const static float VisionDetectRangesByVehicleType[] = 
	{
		0.0f,	// invalid vehicle
		100.0f, // BUGGY
		50.0f,	// STRYKER
	};

	if(distance > VisionDetectRangesByVehicleType[1]) {
		return false;
	}
	
	if(!CheckViewToVehicle(vehicle))
		return false;
		
	return true;
}

bool obj_Zombie::CheckViewToVehicle(obj_Vehicle* vehicle)
{
	return CheckViewToPlayer((GameObject*)vehicle);
}

GameObject* obj_Zombie::GetClosestTargetBySenses()
{
	GameObject* found   = NULL;
	float       minDistSq = 9999999;

#ifdef ZOMBIE_PERFORMANCE_TESTING
	float startTime = r3dGetTime();
#endif

	for(int i=0; i < gServerLogic.curPlayers_; i++)
	{
		obj_ServerPlayer* plr = gServerLogic.plrList_[i];
		if(!plr)
			continue;

#ifdef ZOMBIE_PERFORMANCE_TESTING
		// Done here to get the total number of player checks being
		// done by all zombies, not just how many zombies checked
		// for the closest player.
		++g_ZombieFrameGetClosestTargetCount;
#endif
		float distSq = (GetPosition() - plr->GetPosition()).LengthSq();
		if(!IsPlayerDetectable(plr, distSq))
			continue;

		if(distSq < minDistSq)
		{
			minDistSq = distSq;
#ifdef VEHICLES_ENABLED
			if (plr->IsInVehicle())
				found = obj_Vehicle::GetVehicleById(plr->currentVehicleId);
			else
#endif
				found   = plr;
		}
	}

#ifdef ZOMBIE_PERFORMANCE_TESTING
	g_ZombieFrameGetClosestTargetTime += r3dGetTime() - startTime;
#endif

	//if(found) r3dOutToLog("zombie%p GetClosestPlayerBySenses %s\n", this, found->userName);
	return found;
}
#endif

bool obj_Zombie::CheckViewToPlayer(const GameObject* obj)
{
	const float eyeHeight = ( IsSuperZombie() || IsDogZombie() ) ? 2.4f : 1.6f;
#ifdef ZOMBIE_PERFORMANCE_TESTING
	float startTime = r3dGetTime();
	
	++g_ZombieFrameCheckViewToPlayerCount;
#endif

	// Issue raycast query to check visibility occluders
	r3dPoint3D dir = (obj->GetPosition() - GetPosition());
	float dist = dir.Length() - 1.0f;
	if( dist < 0 )
		return true;
	dir.Normalize();
		
	PxVec3 porigin(GetPosition().x, GetPosition().y + eyeHeight, GetPosition().z);
	PxVec3 pdir(dir.x, dir.y, dir.z);
	PxSceneQueryFlags flags = PxSceneQueryFlag::eDISTANCE;
	PxRaycastHit hit;
	PxSceneQueryFilterData filter(PxFilterData(COLLIDABLE_STATIC_MASK, 0, 0, 0), PxSceneQueryFilterFlags(PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC));
	if(g_pPhysicsWorld->PhysXScene->raycastSingle(porigin, pdir, dist, flags, hit, filter))
	{
	/*
		AILog(20, "view obstructed\n");

		PhysicsCallbackObject* target = NULL;
		if(hit.shape && (target = static_cast<PhysicsCallbackObject*>(hit.shape->getActor().userData)))
		{
			GameObject* obj = target->isGameObject();
			if(obj)
				r3dOutToLog("obj: %s\n", obj->Name.c_str());
		}*/

#ifdef ZOMBIE_PERFORMANCE_TESTING
		g_ZombieFrameCheckViewToPlayerTime += r3dGetTime() - startTime;
#endif
		return false;
	}
	
#ifdef ZOMBIE_PERFORMANCE_TESTING
	g_ZombieFrameCheckViewToPlayerTime += r3dGetTime() - startTime;
#endif

	return true;
}
bool obj_Zombie::SenseItemSound(const obj_ServerPlayer* plr, int ItemID)
{
	if(ZombieDisabled)
		return false;
	if(ZombieState == EZombieStates::ZState_Dead)
		return false; // no attack while dead.
	if(ZombieState == EZombieStates::ZState_Waking)
		return false; // give him time to wake!

	float range = 0;

	if (ItemID == 101323)
		range = 150;

	float dist = (GetPosition() - plr->GetPosition()).Length();
	if(dist > range)
		return false;
	
	//r3dOutToLog("zombie%p sensed weapon fire from %s\n", this, plr->userName); CLOG_INDENT;

	// check if new target is closer that current one
	const GameObject* trg = GameWorld().GetObject(hardObjLock);
	if((trg == NULL) || (trg && dist < (trg->GetPosition() - GetPosition()).Length()))
	{
		StartAttack(plr);
		return true;
	}
	
	// if this is same target, recalculate path if he was moved
	if((ZombieState == EZombieStates::ZState_Pursue || ZombieState == EZombieStates::ZState_PursueSprint) && trg == plr && (trg->GetPosition() - lastTargetPos).Length() > _zai_DistToRecalcPath)
	{
		lastTargetPos = trg->GetPosition();
		MoveNavAgent(trg->GetPosition(), _zai_MaxPursueDistance);
	}
	
	return true;
}

bool obj_Zombie::SenseWeaponFire(const obj_ServerPlayer* plr, const ServerWeapon* wpn)
{
	if (plr->GodMode == 2)
		return false;  

	if(ZombieDisabled)
		return false;

	if (plr->m_DevPlayerHide == true)
		return false;

	if(ZombieState == EZombieStates::ZState_Dead)
		return false; // no attack while dead.
	if(ZombieState == EZombieStates::ZState_Waking)
		return false; // give him time to wake!

	if(r3dGetTime() < plr->m_SpawnProtectedUntil)
		return false;

	if(!wpn)
		return false;

	float range = 0;
	switch(wpn->getCategory())
	{
		default:
		case storecat_SNP:
			range = 75;
			break;

		case storecat_ASR:
		case storecat_SHTG:
		case storecat_MG:
			range = 50;
			break;

		case storecat_HG:
		case storecat_SMG:
			range = 30;
			break;
			
		case storecat_MELEE:
			range = 15; // sergey's design.
			break;
	}

	// silencer halves range
	if(wpn->m_Attachments[WPN_ATTM_MUZZLE] && (wpn->m_Attachments[WPN_ATTM_MUZZLE]->m_itemID == 400013 || wpn->m_Attachments[WPN_ATTM_MUZZLE]->m_itemID == 400156 || wpn->m_Attachments[WPN_ATTM_MUZZLE]->m_itemID == 400159))
	{
		range *= 0.5f;
	}
	
	// override for .50 cal - big range, no silencer
	if(wpn->getConfig()->m_itemID == 101088)
		range = 75;
	// override for VSS 
	if(wpn->getConfig()->m_itemID == 101084)
		range *= 0.35f;
	// override for crossbow
	if(wpn->getConfig()->m_itemID == 101322)
		range = 15;
	// override for nailgun
	if(wpn->getConfig()->m_itemID == 101392)
		range = 15;

	float dist = (GetPosition() - plr->GetPosition()).Length();
	if(dist > range)
		return false;
	
	// if player can't be seen, check agains halved radius
	if(!CheckViewToPlayer(plr))
	{
		if(dist > range * 0.5f)
			return false;
	}

	//r3dOutToLog("zombie%p sensed weapon fire from %s\n", this, plr->userName); CLOG_INDENT;

	// check if new target is closer that current one
	const GameObject* trg = GameWorld().GetObject(hardObjLock);
	if((trg == NULL) || (trg && dist < (trg->GetPosition() - GetPosition()).Length()))
	{
		StartAttack(plr);
		return true;
	}
	
	// if this is same target, recalculate path if he was moved
	if((ZombieState == EZombieStates::ZState_Pursue || ZombieState == EZombieStates::ZState_PursueSprint) && trg == plr && (trg->GetPosition() - lastTargetPos).Length() > _zai_DistToRecalcPath)
	{
		lastTargetPos = trg->GetPosition();
		MoveNavAgent(trg->GetPosition(), _zai_MaxPursueDistance);
	}
	
	return true;
}

bool obj_Zombie::SenseGrenadeSound(const obj_ServerGrenade* grenade, bool isExplosion)
{
	if(ZombieState == EZombieStates::ZState_Dead)
		return false; // no attack while dead.
	if(ZombieState == EZombieStates::ZState_Waking)
		return false; // give him time to wake!
		
	if(!grenade)
		return false;

	float playerRange = 0;  // if the grenade lands this close to the zombie, they will absolutely go after the player instead.
	float range = 0;		// if the grenade is at least this close the zombie, and he cannot see the player, he will investigate the sound.
	float dist = (GetPosition() - grenade->GetPosition()).Length();

	// Can the zombie hear the explosion?
	if( isExplosion )
	{
		switch(grenade->m_ItemID)
		{
			case WeaponConfig::ITEMID_FragGrenade:
			case WeaponConfig::ITEMID_SharpnelBomb:
			case WeaponConfig::ITEMID_SharpnelTripWireBomb:
			range = 60;
				break;
		}
		if(dist > range)
			return false;

		// check if new target is closer than current one
		const GameObject* trg = GameWorld().GetObject(hardObjLock);
		if((trg == NULL) || (!trg->isObjType(OBJTYPE_Human) && dist < (trg->GetPosition() - GetPosition()).Length()))
		{
			// Go investigate the explosion...
			StartAttack(grenade);
			return true;
		}
		return false;
	}

	// Can the zombie hear the clink of the grenade hitting the ground/wall/etc?
	switch(grenade->m_ItemID)
	{
		case WeaponConfig::ITEMID_FragGrenade:
		case WeaponConfig::ITEMID_SharpnelBomb:
		case WeaponConfig::ITEMID_SharpnelTripWireBomb:
			playerRange = 1;
			range = 20;
			break;
		case WeaponConfig::ITEMID_Flare:
			playerRange = 1;
			range = 30;
			break;
		default:
		case WeaponConfig::ITEMID_ChemLight:
		case WeaponConfig::ITEMID_ChemLightBlue:
		case WeaponConfig::ITEMID_ChemLightGreen:
		case WeaponConfig::ITEMID_ChemLightOrange:
		case WeaponConfig::ITEMID_ChemLightRed:
		case WeaponConfig::ITEMID_ChemLightYellow:
			playerRange = 1;
			range = 25;
			break;
	}

	if(dist > range)
		return false;

	// check if the player is within sight, and attack if he is.
	obj_ServerPlayer* plr = (obj_ServerPlayer*)GameWorld().GetObject(grenade->ownerID);
	if(!plr)
		return false;
	bool bCanSeePlayer = IsPlayerDetectable( plr, ( plr->GetPosition() - GetPosition() ).LengthSq() );
	if(bCanSeePlayer)
	{
		StartAttack(plr);
		return true;
	}

	// check if new target is closer than current one
	const GameObject* trg = GameWorld().GetObject(hardObjLock);
	if((trg == NULL) || (trg && dist < (trg->GetPosition() - GetPosition()).Length()))
	{
		if(plr)
		{
			// did the grenade land close enough the zombie should be able to
			// trace it back to the player
			if(dist < playerRange)
			{
				StartAttack(plr);
				return true;
			}
		}

		// Go investigate the sound... hehe...
		if(trg == NULL || !trg->isObjType(OBJTYPE_Human))
		{
			StartAttack(grenade);
			return true;
		}
		return false;
	}
	
	// if this is same player/target, recalculate path if he was moved
	if((ZombieState == EZombieStates::ZState_Pursue || ZombieState == EZombieStates::ZState_PursueSprint) && plr != NULL && trg == plr && (trg->GetPosition() - lastTargetPos).Length() > _zai_DistToRecalcPath)
	{
		lastTargetPos = trg->GetPosition();
		MoveNavAgent(trg->GetPosition(), _zai_MaxPursueDistance);
	}
	
	return true;
}

#ifdef VEHICLES_ENABLED
void obj_Zombie::SenseVehicleExplosion(const obj_Vehicle* vehicle)
{
	if(ZombieState == EZombieStates::ZState_Dead ||
		ZombieState == EZombieStates::ZState_Waking)
		return;

	if(!vehicle)
		return;

	float dist = (GetPosition() - vehicle->GetPosition()).Length();	
	if (dist > 60.0f)
		return;

	StartAttack(vehicle);

	lastTargetPos = vehicle->GetPosition();
	MoveNavAgent(vehicle->GetPosition(), _zai_MaxPursueDistance);
}
#endif

bool obj_Zombie::CallForHelp(const GameObject* trg)
{
	if( CanCallForHelp && spawnObject->m_CFHPerc >= u_GetRandom(0.0, 1.0) )
	{
		// Are there enough zombies around to make the call worthwhile?
		uint32_t numZombies = 0;
		std::tr1::unordered_set<uint32_t> PoiTypesSet;
		PoiTypesSet.insert(AutodeskNavAgent::PoiZombie);
		PoiTypesSet.insert(AutodeskNavAgent::PoiSuperZombie);
		AutodeskNavAgent** pZombies = gAutodeskNavMesh.GetNavAgentsInAABB( GetPosition(), spawnObject->m_CFHExtents, PoiTypesSet, numZombies );
		if( spawnObject->m_CFHMinAdjZombies > numZombies )
		{
			delete[] pZombies;
			return false;
		}
		// Are any other zombies in the group already calling for help?
		for(uint32_t i = 0; i < numZombies; ++i)
		{
			if( EZombieStates::ZState_CallForHelp == ((ZombieNavAgent*)pZombies[i])->m_pOwner->ZombieState )
			{
				delete[] pZombies;
				return false;
			}
		}

		SetNavAgentSpeed( 0.0f );
		SetTarget( trg->GetSafeID() );
		SwitchToState(EZombieStates::ZState_CallForHelp);
		return true;
	}
	return false;
}

bool obj_Zombie::CycleIdleFidget()
{
	float curTime = r3dGetTime();

	if( curTime >= StateTimer )
	{
		// Reset fidget timer.
		if( idleFidget == EZombieStates::ZFidget_SZIdleChestBeat ||
			idleFidget == EZombieStates::ZFidget_SZIdleAlert ||
			idleFidget == EZombieStates::ZFidget_SZIdleAlertSniff )
			idleFidget = EZombieStates::ZFidget_None;
	}

	static float delay = u_GetRandom(8.0f, 15.0f);
	static float idleTime = 0;
	if( IsSuperZombie() )
	{
		if( delay <= idleTime )
		{
			if( u_random(8) == 0 )
			{
				idleFidget = EZombieStates::ZFidget_SZIdleChestBeat;
				StateTimer = curTime + _zai_SZChestBeatTime;

				PKT_S2C_ZombieCycleFidget_s n;
				n.Fidget = (BYTE)idleFidget;
				gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
			}
			else if( u_random(10) == 0 )
			{
				switch(u_random(2))
				{
				default:
				case 0:
					idleFidget = EZombieStates::ZFidget_SZIdleAlert;
					StateTimer = curTime + _zai_SZAlert;
					break;
				case 1:
					idleFidget = EZombieStates::ZFidget_SZIdleAlertSniff;
					StateTimer = curTime + _zai_SZAlertSniff;
					break;
				}

				PKT_S2C_ZombieCycleFidget_s n;
				n.Fidget = (BYTE)idleFidget;
				gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
			}

			// Prepare for the next cycle.
			delay = u_GetRandom(8.0f, 15.0f);
			idleTime = 0;

			return true;
		}
	}
	else if( delay <= idleTime )
	{
		bool bFidgeting = false;

		if( idleFidget != EZombieStates::ZFidget_IdleCrouched )
		{
			if( u_random(10) == 0 )
			{
				bFidgeting = true;
				idleFidget = EZombieStates::ZFidget_IdleCrouched;

				PKT_S2C_ZombieCycleFidget_s n;
				n.Fidget = (BYTE)idleFidget;
				gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
			}
		}
		else if( idleFidget != EZombieStates::ZFidget_IdleEating )
		{
			if( u_random(10) == 0 )
			{
				bFidgeting = true;
				idleFidget = EZombieStates::ZFidget_IdleEating;

				PKT_S2C_ZombieCycleFidget_s n;
				n.Fidget = (BYTE)idleFidget;
				gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
			}
			else if( u_random(10) == 0 )
			{
				bFidgeting = false;
				idleFidget = EZombieStates::ZFidget_None;

				PKT_S2C_ZombieCycleFidget_s n;
				n.Fidget = (BYTE)idleFidget;
				gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
			}
		}
		else if( u_random(10) == 0 )
		{
			bFidgeting = false;
			idleFidget = EZombieStates::ZFidget_None;

			PKT_S2C_ZombieCycleFidget_s n;
			n.Fidget = (BYTE)idleFidget;
			gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
		}

		// Prepare for the next cycle.
		delay = u_GetRandom(5.0f, 10.0f);
		idleTime = 0;

		return bFidgeting;
	}

	idleTime += r3dGetFrameTime();
	return false;
}

bool obj_Zombie::StartAttack(const GameObject* trg)
{
	r3d_assert(ZombieState != EZombieStates::ZState_Dead);

	if(ZombieDisabled)
		return false;
	if(hardObjLock == trg->GetSafeID())
		return true;

	if(ZombieState == EZombieStates::ZState_Sleep)
	{
		// wake up sleeper
		SwitchToState(EZombieStates::ZState_Waking);
		return true;
	}

	if( ZombieState != EZombieStates::ZState_MovingAttack )
		StopNavAgent(); // to release current nav point from Walk state

	AILog(2, "attacking %s\n", trg->Name.c_str()); CLOG_INDENT;

	float dist = (trg->GetPosition() - GetPosition()).Length();
	if(dist > _zai_MaxPursueDistance)
		return false;
		
	// check if we can switch to melee immediately
	if( DoAttack( trg ) )
	{
		return true;
	}

	// check if zombie can get to the player within 2 radius of attack
	r3dPoint3D trgPos = trg->GetPosition();
	if(!gAutodeskNavMesh.AdjustNavPointHeight(trgPos, 1.0f))
	{
		if(!gAutodeskNavMesh.GetClosestNavMeshPoint(trgPos, 2.0f, _zai_AttackRadius * 2))
		{
			AILog(5, "player offmesh at %f %f %f\n", trgPos.x, trgPos.y, trgPos.z);
			return false;
		}
		
		if((trgPos - GetPosition()).Length() < 1.0f)
		{
			AILog(5, "player offmesh and we can't reach him\n");
			return false;
		}

		// ok, we'll reach him in end of our path
		AILog(5, "going to offmesh player to %f %f %f\n", trgPos.x, trgPos.y, trgPos.z);
	}

	// Should the Zombie turn before pursuing.
	if(	ZombieState == EZombieStates::ZState_Attack ||
		ZombieState == EZombieStates::ZState_BarricadeAttack ||
		ZombieState == EZombieStates::ZState_Idle ||
		ZombieState == EZombieStates::ZState_CallForHelp )
	{
		r3dVector vecToTarget = (trg->GetPosition() - GetPosition()).NormalizeTo();
		r3dVector vecFacing = GetFacingVector();
		float dot = R3D_CLAMP( vecFacing.Dot( vecToTarget ), -1.0f, 1.0f );
		if( dot < 0.95f )
		{
			// The time for the client-side anim to turn 180 degrees is
			// about 1.68s, and 90 degrees is about 0.84s
			float timeToFinish = fabs( 1.68f - dot * 1.68f ) * 0.5f;
			turnInterp.Reset( vecFacing, vecToTarget, timeToFinish );
			
			// Save what to do after the turn.
			turnUserData.targetID = trg->GetSafeID();
			turnUserData.nextState = EZombieStates::ZState_Pursue;
			
			r3dVector vecRight = r3dVector( 0, 1, 0 ).Cross( vecFacing ).NormalizeTo();
			float dotRight = vecToTarget.Dot( vecRight );
			if ( dotRight < 0.0f )
			{
				if( EZombieStates::ZState_TurnLeft != ZombieState )
					SwitchToState( EZombieStates::ZState_TurnLeft );
			}
			else
			{
				if( EZombieStates::ZState_TurnRight != ZombieState )
					SwitchToState( EZombieStates::ZState_TurnRight );
			}
			return false;
		}
	}

	// start pursuing immediately
	SetNavAgentSpeed((IsSuperZombie() || IsDogZombie() || staggerTime < 0) ? RunSpeed : 0.0f);
	if(!MoveNavAgent(trgPos, _zai_MaxPursueDistance))
		return false;
	
	lastTargetPos = trg->GetPosition();
	SetTarget( trg->GetSafeID() );

	// switch to pursue mode
	if(ZombieState != EZombieStates::ZState_Pursue) 
	{
		SwitchToState(EZombieStates::ZState_Pursue);
	}
	
	return true;
}

void obj_Zombie::StopAttack()
{
	r3d_assert(	ZombieState == EZombieStates::ZState_Attack ||
				ZombieState == EZombieStates::ZState_BarricadeAttack ||
				ZombieState == EZombieStates::ZState_MovingAttack );
	
	SetTarget( invalidGameObjectID );

	// scan for new target immediately
	if(GameObject* trg = ScanForTarget(true))
	{
		if(StartAttack(trg))
			return;
	}
	
	// When the SuperZombie attacks a barricade on his way to a player, it
	// should resume chasing the player (or vehicle).
	if( IsSuperZombie() &&
		//ZombieState == EZombieStates::ZState_MovingAttack &&
		prevObjLock != invalidGameObjectID &&
		GameWorld().GetObject( prevObjLock ) )
		SetTarget( prevObjLock );

	// start attack failed or no target to attack, switch to idle
	SwitchToState(EZombieStates::ZState_Idle);
}

bool obj_Zombie::CanAttack(const GameObject* trg)
{
	if(trg)
	{
		const r3dVector dirToTrg = trg->GetPosition() - GetPosition();
		float attackRadius = IsSuperZombie()?_zai_SZAttackRadius:_zai_AttackRadius;

#ifdef VEHICLES_ENABLED
		if (trg->isObjType(OBJTYPE_Vehicle))
		{
			obj_Vehicle* vehicle = (obj_Vehicle*)trg;
			if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_BUGGY)
				attackRadius *= IsDogZombie()?3.5f:2.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_ZOMBIEKILLER)
				attackRadius *= IsDogZombie()?4.5f:3.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_HUMMER)
				attackRadius *= IsDogZombie()?4.5f:3.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_POLICE)
				attackRadius *= IsDogZombie()?3.5f:2.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_ABANDONEDSUV)
				attackRadius *= IsDogZombie()?3.5f:2.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_BONECRUSHER)
				attackRadius *= IsDogZombie()?4.5f:3.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_COPCAR)
				attackRadius *= IsDogZombie()?3.5f:2.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_DUNEBUGGY)
				attackRadius *= IsDogZombie()?2.5f:1.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_ECONOLINE)
				attackRadius *= IsDogZombie()?3.5f:2.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_LARGETRUCK)
				attackRadius *= IsDogZombie()?5.5f:4.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_MILITARYAPC)
				attackRadius *= IsDogZombie()?4.5f:3.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_PARAMEDIC)
				attackRadius *= IsDogZombie()?4.5f:3.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_SUV)
				attackRadius *= IsDogZombie()?3.5f:2.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_STRYKER)
				attackRadius *= IsDogZombie()?5.5f:4.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_HELICOPTER)
				attackRadius *= IsDogZombie()?5.5f:4.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_TANK_T80)
				attackRadius *= IsDogZombie()?6.5f:5.5f;
			else if (vehicle->GetVehicleType() == obj_Vehicle::VEHICLETYPE_JEEP)
				attackRadius *= IsDogZombie()?3.5f:2.5f;
		}
#endif
		float attackRadiusSq = attackRadius*attackRadius;

		if(IsSuperZombie())
		{
			float XZdist = r3dPoint2D(dirToTrg.x, dirToTrg.z).LengthSq();
			float Ydist = R3D_ABS(dirToTrg.y);
			if(XZdist < attackRadiusSq && Ydist < _zai_AttackVerticalLimit)
				return true;
		}
		else
		{
			float dist = dirToTrg.LengthSq();
			if( dist < attackRadiusSq && staggerTime < 0)
				return true;
		}
	}

	return false;
}

bool obj_Zombie::DoAttack(const GameObject* trg)
{
	if( trg )
	{
		if(CanAttack(trg))
		{
			//r3dOutToLog("!!!zombie%p ATTACK Dist: %0.4f\n", this, dist);

			SetTarget( trg->GetSafeID() );

			if(ZombieState != EZombieStates::ZState_Attack)
				SwitchToState(EZombieStates::ZState_Attack);

			if( IsSuperZombie() )
				attackTimer	= _zai_SuperZombieAttackTimer / 2;
			else
				attackTimer	= _zai_AttackTimer / 2;
			attackCounter	= 0;
			superAttackDir	= 0;
			isFirstAttack	= true;
			return true;
		}
	}
	return false;
}

void obj_Zombie::CancelSprint()
{
	if( g_enable_zombie_sprint->GetBool() )
	{
		// Reset the sprint timers and enter cooldown if necessary.
		if( 0.0f < SprintTimer || 0.0f < SprintFalloffTimer )
		{
			r3d_assert(spawnObject);
			SprintCooldownTimer = spawnObject->m_SprintCooldownTime;
			//r3dOutToLog("!!!zombie%p Sprint Cooldown: %0.2f\n", this, SprintCooldownTimer);
		}
		SprintTimer = -1.0;
		SprintFalloffTimer = -1.0f;
	}
}
void obj_Zombie::UpdateSprint(const GameObject& target, const float& distToTarget)
{
	if( g_enable_zombie_sprint->GetBool() && 
		CanSprint &&
		navAgent->m_velocity.GetSquareLength() > ( RunSpeed * RunSpeed - 0.1f ) )
	{
		// NOTE: Currently only called from Update, after
		//		 the state has been assured, so the extra
		//		 check is not strictly necessary, but is
		//		 shown here for clarity.
		//if( EZombieStates::ZState_Pursue == ZombieState || EZombieStates::ZState_PursueSprint == ZombieState )
		{
			r3d_assert(spawnObject);

			// check if we're sprinting/falling off, cooling down, or should sprint
			if( 0.0f < SprintTimer )
			{
				SprintTimer -= r3dGetFrameTime();
				if( 0.0f >= SprintTimer )
				{
					SprintTimer = -1.0f;
					SprintFalloffTimer = spawnObject->m_SprintMaxTime * 0.5f;
					//r3dOutToLog("!!!zombie%p Sprint Falloff: %0.2f\n", this, SprintFalloffTimer);
				}
			}
			else if( 0.0f < SprintFalloffTimer )
			{
				SprintFalloffTimer -= r3dGetFrameTime();
				if( 0.0f >= SprintFalloffTimer )
				{
					SprintFalloffTimer = -1.0f;
					SprintCooldownTimer = spawnObject->m_SprintCooldownTime;
					//r3dOutToLog("!!!zombie%p Sprint Cooldown: %0.2f\n", this, SprintCooldownTimer);
				}
			}
			else if( 0.0f >= SprintCooldownTimer &&
					 target.isObjType( OBJTYPE_Human ) > 0 &&
					 ( 0.0f < spawnObject->m_SprintPerc &&
					   distToTarget <= spawnObject->m_SprintRadius &&
					   u_GetRandom( 0.0f, 1.0f ) <= spawnObject->m_SprintPerc ) ||
					 ( 0.0f < spawnObject->m_SprintFromFarPerc &&
					   distToTarget >= spawnObject->m_SprintRadius &&
					   u_GetRandom( 0.0f, 1.0f ) <= spawnObject->m_SprintFromFarPerc ) )
			{
				PKT_S2C_ZombieSprint_s n;
				gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));

				SprintTimer = spawnObject->m_SprintMaxTime;
				if( EZombieStates::ZState_PursueSprint != ZombieState )
					SwitchToState(EZombieStates::ZState_PursueSprint);
				//r3dOutToLog("!!!zombie%p SPRINTING - Timer: %0.2f\n", this, SprintTimer);
			}

			UpdateSprintSpeed();
		}
	}
}

void obj_Zombie::UpdateSprintSpeed()
{
	// NOTE: Currently only called from UpdateSprint, after
	//		 the state has been assured, so the extra
	//		 checks are not strictly necessary, but are
	//		 shown here for clarity.
	//if( g_enable_zombie_sprint->GetBool() && 
	//	CanSprint &&
	//	navAgent->m_velocity.GetSquareLength() > ( RunSpeed * RunSpeed - 0.1f ) )
	{
		//if( EZombieStates::ZState_Pursue == ZombieState || EZombieStates::ZState_PursueSprint == ZombieState )
		{
			r3d_assert(spawnObject);

			//static float DBG_PrevReport = r3dGetTime();
			if( 0.0f < SprintTimer )
			{
				// increase the sprint speed toward the maximum
				double xAxis = ( spawnObject->m_SprintSlope - 1 ) * spawnObject->m_SprintMaxTime;
				double halfTime = ( spawnObject->m_SprintMaxTime * 0.5 );
				if( xAxis < halfTime )
					xAxis = halfTime;
				double exp = -spawnObject->m_SprintSlope * SprintTimer + xAxis;
				SprintSpeed = -spawnObject->m_SprintMaxSpeed /( 1 + (float)pow( M_E, exp ) ) + spawnObject->m_SprintMaxSpeed;
				//if( 1.0f <= (r3dGetTime() - DBG_PrevReport) )
				//{
				//	DBG_PrevReport = r3dGetTime();
				//	r3dOutToLog("!!!zombie%p Sprint Speed: %0.4f[%0.2f] Current Speed: %0.4f\n", this, SprintSpeed, SprintTimer, 	GetVelocity().Length());
				//}
			}
			else if( 0.0f < SprintFalloffTimer )
			{
				// make the sprint speed falloff, back to normal running speed
				SprintSpeed = spawnObject->m_SprintMaxSpeed * (float)pow( M_E, ( ( SprintFalloffTimer - spawnObject->m_SprintMaxSpeed ) * 0.25 ) );
				//if( 1.0f <= (r3dGetTime() - DBG_PrevReport) )
				//{
				//	DBG_PrevReport = r3dGetTime();
				//	r3dOutToLog("!!!zombie%p Sprint Speed: %0.4f[%0.2f] Current Speed: %0.4f\n", this, SprintSpeed, SprintFalloffTimer, 	GetVelocity().Length());
				//}
			}

			// clamp the sprinting speed to [RunSpeed, Max]
			if( SprintSpeed > spawnObject->m_SprintMaxSpeed )
				SprintSpeed = spawnObject->m_SprintMaxSpeed;
			if( SprintSpeed < RunSpeed )
				SprintSpeed = RunSpeed;
		}
	}
}

BOOL obj_Zombie::Update()
{
	parent::Update();
	
	if(!isActive())
		return TRUE;

	const float curTime = r3dGetTime();
	
	if(ZombieState == EZombieStates::ZState_Dead)
	{
		// deactivate zombie after some time
		if(curTime > StateStartTime + 60.0f)
			setActiveFlag(0);
		return TRUE;
	}
	
	PassiveHeal();

	DebugSingleZombie();

	// Propagate AI agent position to zombie position
	if(navAgent)
	{
		SetPosition(navAgent->GetPosition());
		
		if(navAgent->m_status == AutodeskNavAgent::Moving)
		{
			Kaim::Vec3f rot  = navAgent->m_velocity;
			if(rot.GetSquareLength2d() > 0.001f)
			{
				r3dVector heading = r3dPoint3D(rot[0], rot[2], rot[1]);
				if( EZombieStates::ZState_TurnLeft != ZombieState &&
					EZombieStates::ZState_TurnRight != ZombieState )
				{
					// Perform Interpolation from current heading to final heading.
					float timeToFinish = 0.1f;
					r3dVector fwd = GetFacingVector();
					turnInterp.Reset( fwd, heading, timeToFinish );
				}
			}
		}
	}
	moveFrameCount++;

	// check for stagger
	if(staggerTime > 0)
	{
		animState = 1;
		staggerTime -= r3dGetFrameTime();
		if(staggerTime <= 0.001f)
		{
			animState = 0;
			staggerTime = -1;
		}
	}

	if( !turnInterp.IsFinished() )
	{
		turnInterp.Update();
		FaceVector( turnInterp.GetCurrent() );
	}

	// send network position update
	{
		CNetCellMover::moveData_s md;
		md.pos       = GetPosition();
		md.turnAngle = ( turnInterp.IsFinished() ) ? GetRotationVector().x : turnInterp.GetFinishAngle();
		md.bendAngle = 0;
		md.state     = ( (USHORT)(navAgent->GetAvoidanceResult()) << 8 ) | (USHORT)animState;

		PKT_C2C_MoveSetCell_s n1;
		PKT_C2C_MoveRel_s     n2;
		DWORD pktFlags = netMover.SendPosUpdate(md, &n1, &n2);
		if(pktFlags & 0x1)
			gServerLogic.p2pBroadcastToActive(this, &n1, sizeof(n1));
		if(pktFlags & 0x2)
			gServerLogic.p2pBroadcastToActive(this, &n2, sizeof(n2));
	}
	
	if( g_enable_zombie_sprint->GetBool() )
	{
		if( 0.0f < SprintCooldownTimer )
			SprintCooldownTimer -= r3dGetFrameTime();
		else
			SprintCooldownTimer = -1.0f;
	}

	switch(ZombieState)
	{
		default:
			break;
			
		case EZombieStates::ZState_Sleep:
		{
			// if detected, wake zombie up, but do not set target
			if(GameObject* trg = ScanForTarget())
			{
				SwitchToState(EZombieStates::ZState_Waking);
			}
			break;
		}
			
		case EZombieStates::ZState_Waking:
		{
			if(curTime < StateStartTime + 3.0f) // wait for client "wake up" animation finish
				break;

			// perform immidiate surrounding check if we don't have target yet
			if(hardObjLock == invalidGameObjectID)
			{
				if(GameObject* trg = ScanForTarget(true))
				{
					if( !CallForHelp( trg ) )
						StartAttack(trg);
					break;
				}
			}

			GameObject* trg = GameWorld().GetObject(hardObjLock);
			if(!trg)
			{
				// no target, switch to idle
				SwitchToState(EZombieStates::ZState_Idle);
				break;
			}
			else
			{
				StartAttack(trg);
			}
			break;
		}

		case EZombieStates::ZState_CallForHelp:
		{
			ResetPassiveHealTimer();

			// Is it time to switch to attacking yet?
			if( ( IsSuperZombie() && _zai_SZCallForHelpTime < curTime - StateStartTime ) ||
				( !IsSuperZombie() && _zai_CallForHelpTime < curTime - StateStartTime ) )
			{
				GameObject* trg = GameWorld().GetObject( hardObjLock );
				SetTarget( invalidGameObjectID );
				// Make sure the target is still valid.
				if( trg )
				{
					// Alert the nearby Zombies
					uint32_t numZombies = 0;
					std::tr1::unordered_set<uint32_t> PoiTypesSet;
					PoiTypesSet.insert(AutodeskNavAgent::PoiZombie);
					PoiTypesSet.insert(AutodeskNavAgent::PoiSuperZombie);
					AutodeskNavAgent** pZombies = gAutodeskNavMesh.GetNavAgentsInAABB( GetPosition(), spawnObject->m_CFHExtents, PoiTypesSet, numZombies );
					for(uint32_t i = 0; i < numZombies; ++i)
						((ZombieNavAgent*)pZombies[i])->m_pOwner->StartAttack( trg );
					delete[] pZombies;

					StartAttack( trg );
				}
				else
				{
					SwitchToState( EZombieStates::ZState_Idle );
				}
			}
			break;
		}
			
		case EZombieStates::ZState_Idle:
		{
			// try to find someone to attack, do not switch to patrol if we have anyone
			if(GameObject* trg = ScanForTarget())
			{
				if( idleFidget != EZombieStates::ZFidget_SZIdleChestBeat && !CallForHelp( trg ) )
					StartAttack(trg);
				break;
			}
			
			// Perform fidget.
			if( CycleIdleFidget() )
				break;

			// check for idle finish	
			if(curTime < StateTimer || staggerTime > 0) 
				break;
			
			// do not switch to patrol if there is no players around
			bool doPatrol = !ZombieDisabled && u_GetRandom() < (IsSuperZombie() ? _zai_SuperZombieIdleStatePatrolPerc : _zai_IdleStatePatrolPerc);
			if(doPatrol && _zai_NoPatrolPlayerDist > 0 && gServerLogic.CheckForPlayersAround(GetPosition(), _zai_NoPatrolPlayerDist) == false)
			{
				StateTimer = curTime + u_GetRandom(3, 5);
				break;
			}
			
			if(doPatrol)
			{
				r3dPoint3D out_pos;
				r3dPoint3D cur_pos = GetPosition();
				patrolPntIdx = spawnObject->GetFreeNavPointIdx(&out_pos, true, _zai_MaxPatrolDistance, &cur_pos);
				if(patrolPntIdx >= 0)
				{
					SetNavAgentSpeed(WalkSpeed);
					MoveNavAgent(out_pos, _zai_MaxPatrolDistance * 2);
					SwitchToState(EZombieStates::ZState_Walk);
					break;
				}
				else
				{
					// navigation failed, keep idle time
					StateTimer = r3dGetTime() + u_GetRandom(5, 20);
					AILog(5, "patrol navigation failed around pos %f %f %f\n", spawnObject->GetPosition().x, spawnObject->GetPosition().y, spawnObject->GetPosition().z);

					// if there is no players around, suicide - we do not have any navigation points close to us
					if(gServerLogic.CheckForPlayersAround(GetPosition(), _zai_SafeSuicideDist) == false)
					{
						DoDeath(true);
						// make new zombie spawn in 1 sec.
						spawnObject->timeSinceLastSpawn = r3dGetTime() - spawnObject->zombieSpawnDelay - 1;
					}
				}
			}
			else
			{
				// continuing idle time for some more
				StateTimer = curTime + u_GetRandom(5, 60);
			}
			break;
		}

		case EZombieStates::ZState_TurnLeft:
		case EZombieStates::ZState_TurnRight:
		{
			if( turnInterp.IsFinished() )
			{
				GameObject* trg = GameWorld().GetObject( turnUserData.targetID );
				if( !trg )
				{
					if( EZombieStates::ZState_Idle != ZombieState )
						SwitchToState( EZombieStates::ZState_Idle );
					break;
				}

				if( StartAttack( trg ) )
				{
					if( ZombieState != turnUserData.nextState )
						SwitchToState( turnUserData.nextState );

					// Clean Up Interpolator and saved data.
					turnUserData.nextState = EZombieStates::ZState_Idle;
					turnUserData.targetID = invalidGameObjectID;
				}
				else
				{
					if( EZombieStates::ZState_Idle != ZombieState )
						SwitchToState( EZombieStates::ZState_Idle );
				}
			}
			break;
		}

		case EZombieStates::ZState_Walk:
		{
			r3d_assert(!ZombieDisabled && navAgent);

			// try to find someone to attack
			if(GameObject* trg = ScanForTarget())
			{
				if( !CallForHelp( trg ) )
					StartAttack(trg);
				break;
			}
			
			if(IsSuperZombie() || IsDogZombie() || staggerTime < 0)
				SetNavAgentSpeed(WalkSpeed);
			
			GameObject* DoorF = FindDoor();
			if(DoorF)
			{
				obj_Door* Door = (obj_Door*)DoorF;
				if (Door->m_OpenDoor == 0)
				{
					hardObjLock = DoorF->GetSafeID();
					StopNavAgent();
					SwitchToState(EZombieStates::ZState_Attack);
				}
					break;
			}
			// check if we have barricade around
			if(CheckForBarricadeBlock())
			{
				break;
			}

			switch(CheckMoveStatus())
			{
				default: r3d_assert(false);
				case 0: // completed
				case 2: // failed
					SwitchToState(EZombieStates::ZState_Idle);
					break;
				case 1: // in progress
					if(!CheckMoveWatchdog())
					{
						SwitchToState(EZombieStates::ZState_Idle);
					}
					break;
			}
			
			break;
		}

		case EZombieStates::ZState_Pursue:
		case EZombieStates::ZState_PursueSprint:
		{
			r3d_assert(!ZombieDisabled && navAgent);

			ResetPassiveHealTimer();

			GameObject* trg = GameWorld().GetObject(hardObjLock);
			if(trg)
			{
				// check if we're within melee range
				if( DoAttack( trg ) )
				{
					StopNavAgent();
					break;
				}
				else
				{
					float dist = (trg->GetPosition() - GetPosition()).Length();
					UpdateSprint( *trg, dist );
				}
			}

			if(IsSuperZombie() || IsDogZombie() || staggerTime < 0)
			{
				if( g_enable_zombie_sprint->GetBool() &&
					( 0.0f < SprintTimer || 0.0f < SprintFalloffTimer ) )
					SetNavAgentSpeed(SprintSpeed);
				else
					SetNavAgentSpeed(RunSpeed);
			}

			GameObject* DoorF = FindDoor();
			if(DoorF)
			{
				obj_Door* Door = (obj_Door*)DoorF;
				if (Door->m_OpenDoor == 0)
				{
					hardObjLock = DoorF->GetSafeID();
					StopNavAgent();
					SwitchToState(EZombieStates::ZState_Attack);
				}
					break;
			}
			// check if we still have target in our visibility
			if(GameObject* trg = ScanForTarget())
			{
				if(trg->GetSafeID() != hardObjLock)
				{
					// new target, switch to him
					StartAttack(trg);
					break;
				}
				
				// if player went off mesh - do nothing, continue what we was doing
				if(!gAutodeskNavMesh.IsNavPointValid(trg->GetPosition()))
				{
					break;
				}

				// recalculate paths sometime
				if((trg->GetPosition() - lastTargetPos).Length() > _zai_DistToRecalcPath)
				{
					lastTargetPos = trg->GetPosition();
					MoveNavAgent(trg->GetPosition(), _zai_MaxPursueDistance);
				}
			}

			// check if we have barricade around
			if(CheckForBarricadeBlock())
			{
				break;
			}
			
			switch(CheckMoveStatus())
			{
				default: r3d_assert(false);
				case 0: // completed
				case 2: // failed
					SetTarget( invalidGameObjectID );
					SwitchToState(EZombieStates::ZState_Idle);
					break;
				case 1: // in progress
					break;
			}

			break;
		}
		
		case EZombieStates::ZState_Attack:
		{
#ifdef VEHICLES_ENABLED
			// this is temporary until vehicles are a part of final build
			GameObject* gameObj = GameWorld().GetObject(hardObjLock);
			if (!gameObj)
			{
				StopAttack();
				break;
			}
			if (gameObj->Class->Name == "obj_Door")
			{
				obj_Door* Door = (obj_Door*)gameObj;
				if (Door->m_Healt >= 0)
				{
					if (Door->m_OpenDoor == 1)
					{
						StopAttack();
						break;
					}
					else {
							if(Door->DamageDoor(0.01f))
							{
								StopAttack();
								break;
							}
					}
				}
			}
			else if (gameObj->isObjType(OBJTYPE_Vehicle))
			{
				obj_Vehicle* vehicle = (obj_Vehicle*)gameObj;
				if (vehicle->GetDurability() <= 0 || !vehicle->HasPlayers())
				{
					StopAttack();
					break;
				}

				FaceVector(vehicle->GetPosition() - GetPosition());

				if (staggerTime > 0)
				{
					attackTimer = 0;
					isFirstAttack = false;
					break;
				}

				ResetPassiveHealTimer();

				attackTimer += r3dGetFrameTime();

				if( ( IsSuperZombie() && attackCounter != 2 && attackTimer >= _zai_SuperZombieAttackTimer ) ||
					( IsSuperZombie() && attackCounter == 2 && attackTimer >= _zai_SuperZombieSuperAttackTimer ) ||
					( !IsSuperZombie() && attackTimer >= _zai_AttackTimer ) )
				{
					attackTimer = 0;

					// first attack always land
					bool canAttack = true;
					if(!isFirstAttack)
					{
						float dist = (vehicle->GetPosition() - GetPosition()).Length();
						if(!CanAttack(vehicle) || !CheckViewToVehicle(vehicle))
							canAttack = false;
					}

					if(!canAttack)
					{
						StopAttack();
						break;
					}
					else
					{
						isFirstAttack = false;

						float damage = IsSuperZombie() ? _zai_SuperZombieSuperAttackDamage : _zai_AttackDamage;
						
						vehicle->ApplyDamage(this, damage, storecat_MELEE);

						if( IsSuperZombie() && attackCounter == 2 )
						{
							superAttackDir++;
							superAttackDir %= 2;
						}

						attackCounter++;
						attackCounter %= 3;

						// Make sure we are still in Attack state (could get here if the navAgent fails
						// to create its query, after the vehicle blows up and informs all zombies,
						// including this one)
						if (ZombieState == EZombieStates::ZState_Attack && vehicle->GetDurability() <= 0)
						{
							StopAttack();
							break;
						}
					}
				}
			}
			else
#endif
			{
				obj_ServerPlayer* trg = IsServerPlayer(GameWorld().GetObject(hardObjLock));
				if(!trg || trg->loadout_->Alive == 0 || trg->GodMode == 2)
				{
					StopAttack();
					break;
				}

				FaceVector(trg->GetPosition() - GetPosition());

				if(!IsSuperZombie() && staggerTime > 0)
				{
					attackTimer   = 0;
					isFirstAttack = false;
					break;
				}

				ResetPassiveHealTimer();

				attackTimer += r3dGetFrameTime();
				if( ( IsSuperZombie() && attackCounter != 2 && attackTimer >= _zai_SuperZombieAttackTimer ) ||
					( IsSuperZombie() && attackCounter == 2 && attackTimer >= _zai_SuperZombieSuperAttackTimer ) ||
					( !IsSuperZombie() && attackTimer >= _zai_AttackTimer ) )
				{
					attackTimer = 0;

					// first attack always land
					bool canAttack = true;
					if(!isFirstAttack)
					{
						float dist = (trg->GetPosition() - GetPosition()).Length();
						if(!CanAttack(trg) || !CheckViewToPlayer(trg))
							canAttack = false;
					}

					if(!canAttack)
					{
						StopAttack();
						break;
					}
					else
					{
						isFirstAttack = false;

#ifdef VEHICLES_ENABLED
						if (trg->IsInVehicle())
						{
							GameObject* obj = obj_Vehicle::GetVehicleById( trg->currentVehicleId );
							if( obj )
								hardObjLock = obj->GetSafeID();
							break;
						}
#endif
						if( IsSuperZombie() && attackCounter == 2 )
							gServerLogic.ApplyDamageToPlayer(this, trg, GetPosition(), _zai_SuperZombieSuperAttackDamage, 0, 0, false, storecat_MELEE, 0);
						else
							gServerLogic.ApplyDamageToPlayer(this, trg, GetPosition(), _zai_AttackDamage, 0, 0, false, storecat_MELEE, 0);

						if( IsSuperZombie() && attackCounter == 2 )
						{
							superAttackDir++;
							superAttackDir %= 2;
						}

						attackCounter++;
						attackCounter %= 3;

						if(trg->loadout_->Health <= 0)
						{
							StopAttack();
							break;
						}
					}
				}
			}
			break;
		}
	
		case EZombieStates::ZState_BarricadeAttack:
		case EZombieStates::ZState_MovingAttack:
		{
			obj_ServerBarricade* shield = (obj_ServerBarricade*)GameWorld().GetObject(hardObjLock);
			if(!shield)
			{
				StopAttack();
				break;
			}

			ResetPassiveHealTimer();

			// check if we have player in melee range
			if(GameObject* trg = ScanForTarget())
			{
				if(CanAttack(trg))
				{
					StartAttack(trg);
					break;
				}
			}
			
			FaceVector(shield->GetPosition() - GetPosition());
			if( !IsSuperZombie() || ZombieState ==  EZombieStates::ZState_BarricadeAttack )
			{
				if(staggerTime > 0)
				{
					attackTimer   = 0;
					break;
				}
			}

			attackTimer += r3dGetFrameTime();
			if(attackTimer >= _zai_AttackTimer)
			{
				attackTimer = 0;
				if( IsSuperZombie() )
					shield->DoDamage( shield->m_Health + 1.0f );
				else
					shield->DoDamage(_zai_AttackDamage);
			}
			break;
		}
			
		case EZombieStates::ZState_Dead:
			ResetPassiveHealTimer();
			break;
	}
	
	return TRUE;
}

DefaultPacket* obj_Zombie::NetGetCreatePacket(int* out_size)
{
	static PKT_S2C_CreateZombie_s n;
	n.spawnID    = toP2pNetId(GetNetworkID());
	n.spawnPos   = GetPosition();
	n.spawnDir   = GetRotationVector().x;
	n.moveCell   = netMover.SrvGetCell();
	n.HeroItemID = HeroItemID;
	n.HeadIdx    = (BYTE)HeadIdx;
	n.BodyIdx    = (BYTE)BodyIdx;
	n.LegsIdx    = (BYTE)LegsIdx;
	n.State      = (BYTE)ZombieState;
	n.FastZombie = (BYTE)FastZombie;
	n.WalkSpeed  = WalkSpeed;
	n.RunSpeed   = RunSpeed;
	if(HalloweenZombie) n.HeroItemID += 1000000;
	n.SprintMaxSpeed		= spawnObject->m_SprintMaxSpeed;
	n.SprintMaxTime			= spawnObject->m_SprintMaxTime;
	n.SprintSlope			= spawnObject->m_SprintSlope;
	n.SprintCooldownTime	= spawnObject->m_SprintCooldownTime;

	
	*out_size = sizeof(n);
	return &n;
}

void obj_Zombie::SendAIStateToNet()
{
	if(!_zai_DebugAI)
		return;
		
	if(navAgent->m_status == AutodeskNavAgent::Moving)
	{
		PKT_S2C_Zombie_DBG_AIInfo_s n;
		n.from = moveStartPos;
		n.to   = moveTargetPos;
		gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	}
}

void obj_Zombie::DebugSingleZombie()
{
	if(!_zai_DebugAI)
		return;

	static KyUInt32 debugVisualId = 0;
	FILE* f = fopen("zdebug.txt", "rt");
	if(!f) return;
	fscanf(f, "%d", &debugVisualId);
	fclose(f);
	
	if(navAgent->m_navBot->GetVisualDebugId() != debugVisualId)
		return;
		
	if(ZombieDisabled)
	{
		AILog(0, "zombie disabled\n");
		return;
	}

/*
	Kaim::Bot* m_navBot = navAgent->m_navBot;
	Kaim::AStarQuery<Kaim::AStarCustomizer_Default>* m_pathFinderQuery = navAgent->m_pathFinderQuery;
	AILog(0, "state: %d, time: %f\n", ZombieState, r3dGetTime() - StateStartTime);
	AILog(0, "GetTargetOnLivePathStatus(): %d\n", m_navBot->GetTargetOnLivePathStatus());
	AILog(0, "GetPathValidityStatus(): %d\n", m_navBot->GetLivePath().GetPathValidityStatus());
	AILog(0, "GetPathFinderResult(): %d %d\n", m_pathFinderQuery->GetPathFinderResult(), m_pathFinderQuery->GetResult());
	if(m_navBot->GetPathFinderQuery())
		AILog(0, "m_processStatus: %d\n", m_navBot->GetPathFinderQuery()->m_processStatus);
*/		
}

BOOL obj_Zombie::OnNetReceive(DWORD EventID, const void* packetData, int packetSize)
{
	switch(EventID)
	{
		case PKT_C2S_Zombie_DBG_AIReq:
			SendAIStateToNet();
			break;
	}
	
	return TRUE;
}

void obj_Zombie::SwitchToState(int in_state)
{
	r3d_assert(ZombieState != EZombieStates::ZState_Dead); // death should be final
	r3d_assert(ZombieState != in_state);
	ZombieState    = in_state;
	StateStartTime = r3dGetTime();
	
	// duration of idle state
	if(in_state == EZombieStates::ZState_Idle)
	{
		StopNavAgent(); // to release current nav point from Walk state
		SetTarget( invalidGameObjectID );
		StateTimer  = r3dGetTime() + u_GetRandom(3, 10);
	}
	
	//r3dOutToLog("zombie%p SwitchToState %d\n", this, ZombieState); CLOG_INDENT;

	if( EZombieStates::ZState_PursueSprint != in_state )
		CancelSprint();

	if( EZombieStates::ZState_TurnLeft == in_state ||
		EZombieStates::ZState_TurnRight == in_state )
	{
		PKT_S2C_ZombieSetTurnState_s n;
		n.State			= (BYTE)ZombieState;
		n.TargetAngle	= turnInterp.GetFinishAngle();
		gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
		return;
	}

	PKT_S2C_ZombieSetState_s n;
	n.State    = (BYTE)ZombieState;
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
}

void obj_Zombie::DoDeath(bool fakeDeath)
{
	extern wiInventoryItem RollItem(const LootBoxConfig* lootCfg, int depth);
	
	// drop loot
	if(!fakeDeath && spawnObject->lootBoxCfg)
	{
		wiInventoryItem wi = RollItem(spawnObject->lootBoxCfg, 0);
		if(wi.itemID > 0)
		{
			// create random position around zombie
			r3dPoint3D pos = GetPosition();
			pos.y += 0.4f;
			pos.x += u_GetRandom(-1, 1);
			pos.z += u_GetRandom(-1, 1);

			// create network object
			obj_DroppedItem* obj = (obj_DroppedItem*)srv_CreateGameObject("obj_DroppedItem", "obj_DroppedItem", pos);
			obj->SetNetworkID(gServerLogic.GetFreeNetId());
			obj->NetworkLocal = true;
			// vars
			obj->m_Item       = wi;
		}
	}
	
	if(!fakeDeath && HalloweenZombie && u_GetRandom() < 0.3f) // 30% to drop that helmet
	{
			// create random position around zombie
			r3dPoint3D pos = GetPosition();
			pos.y += 0.4f;
			pos.x += u_GetRandom(-1, 1);
			pos.z += u_GetRandom(-1, 1);

			// create network object
			obj_DroppedItem* obj = (obj_DroppedItem*)srv_CreateGameObject("obj_DroppedItem", "obj_DroppedItem", pos);
			obj->SetNetworkID(gServerLogic.GetFreeNetId());
			obj->NetworkLocal = true;
			// vars
			obj->m_Item.itemID   = 20197; // Christmas special
			obj->m_Item.quantity = 1;
	}

	// remove from active zombies, but keep object - so it'll be visible for some time on all clients
	StopNavAgent();
	DeleteZombieNavAgent(navAgent);
	navAgent = NULL;

	SetTarget( invalidGameObjectID );
	spawnObject->OnZombieKilled(this);
		
	SwitchToState(EZombieStates::ZState_Dead);
}

bool obj_Zombie::ApplyDamage(GameObject* fromObj, float damage, int bodyPart, STORE_CATEGORIES damageSource)
{
	if(ZombieState == EZombieStates::ZState_Dead)
		return false;
	
	if( !IsSuperZombie())
	{
		if (damageSource != storecat_Vehicle)
		{
			if(damageSource != storecat_GRENADE 
				&& bodyPart != 1) // only hitting head will lower zombie's health (except grenades)
			{
				// Sprinting Zombies can take damage when they are shot in the body.
				if( g_enable_zombie_sprint->GetBool() && 
					( EZombieStates::ZState_PursueSprint == ZombieState && 0.0f < SprintTimer ) || IsDogZombie() )
					damage = 1.0f + _zstat_MaxHealth * 0.1f;  // Using MaxHeath to get a consistent number of shots to kill the zombie; this should be changed to deal with different types of weapons, or just default back to part of the damage (once it's confirmed there's no issues using that value).
				else
					damage = 0;
			}

			if(damageSource != storecat_MELEE && bodyPart == 1) // everything except for melee: one shot in head = kill
			{
					damage = 1000; 
			}
		}
	}

	if(IsSuperZombie() && damageSource==storecat_MELEE)
		damage = 0;

	//r3dOutToLog("zombie%p takes %0.2f damage to body part %d, starting with %0.2f health\n", this, dmg, bodyPart, ZombieHealth);
	ZombieHealth -= damage;

	if(ZombieHealth <= 0.0f)
	{
		DoDeath();

		if(fromObj->Class->Name == "obj_ServerPlayer")
		{
			obj_ServerPlayer* plr = (obj_ServerPlayer*)fromObj;
			if(!IsSuperZombie())
				gServerLogic.AddPlayerReward(plr, RWD_ZombieKill);
			else
				gServerLogic.AddPlayerReward(plr, RWD_SuperZombieKill);

#ifdef ENABLE_GAMEBLOCKS
			if(g_GameBlocks_Client && g_GameBlocks_Client->Connected())
			{
				g_GameBlocks_Client->PrepareEventForSending("ZombieKill", g_GameBlocks_ServerID, GameBlocks::GBPublicPlayerId(uint32_t(plr->profile_.CustomerID)));
				g_GameBlocks_Client->AddKeyValueInt("Weapon", damageSource);
				g_GameBlocks_Client->SendEvent();
			}
#endif
		}

		return true;
	}

	// stagger code
	if(staggerTime < 0)
	{
		if( g_enable_zombie_sprint->GetBool() )
		{
			// Sprinting zombies do not stagger.
			if( EZombieStates::ZState_Sleep != ZombieState  &&
				EZombieStates::ZState_Waking != ZombieState &&
				( !g_enable_zombie_sprint->GetBool() ||
				  ( g_enable_zombie_sprint->GetBool() && !( EZombieStates::ZState_PursueSprint == ZombieState && 0.0f < SprintTimer ) ) ) )
			{
				// Running Super Zombies do not stagger, but they can slow down to RunSpeed.
				if( IsSuperZombie() &&
					(ZombieState == EZombieStates::ZState_Pursue ||
					 ZombieState == EZombieStates::ZState_PursueSprint ||
					 ZombieState == EZombieStates::ZState_MovingAttack) )
				{
					float vel = GetVelocity().Length();
					vel = (( vel > RunSpeed ) ? vel : RunSpeed) * 0.75f;
					SetNavAgentSpeed( vel );
					staggerTime = 1.0f;
				}
				else if( IsDogZombie() &&
					(ZombieState == EZombieStates::ZState_Pursue ||
					 ZombieState == EZombieStates::ZState_PursueSprint ||
					 ZombieState == EZombieStates::ZState_MovingAttack) )
				{
					float vel = GetVelocity().Length();
					vel = (( vel > RunSpeed ) ? vel : RunSpeed) * 0.75f;
					SetNavAgentSpeed( vel );
					staggerTime = 1.0f;
				}
				else
				{
					SetNavAgentSpeed(0.0);
					staggerTime = 1.0f;
				}
			}
		}
	}

	// waking zombies can't be switch to attack
	if(ZombieState == EZombieStates::ZState_Waking)
		return false;
		
	// direct hit, switch to that player anyway if it is new or closer than current one
	float distSq = (fromObj->GetPosition() - GetPosition()).LengthSq();
	const GameObject* trg = GameWorld().GetObject(hardObjLock);
	if((trg == NULL) || (trg && distSq < (trg->GetPosition() - GetPosition()).LengthSq()))
	{
		StartAttack(fromObj);
	}
	
	return false; // false as zombie wasn't killed
}

bool obj_Zombie::IsSuperZombie()
{
	return HeroItemID == g_SuperZombieItemID;
}

bool obj_Zombie::IsDogZombie()
{
	return HeroItemID == g_DogZombieItemID;
}

void obj_Zombie::PassiveHeal()
{
	if( ZombieHealth >= (IsSuperZombie() ? _zstat_MaxHealthSuperZombie : _zstat_MaxHealth) )
		return;

	float curTime = r3dGetTime();
	if( curTime > HealTimer )
	{
		if( IsSuperZombie() )
		{
			ZombieHealth = R3D_CLAMP( ZombieHealth + _zstat_MaxHealthSuperZombie * _zai_SZHealRate , ZombieHealth, _zstat_MaxHealthSuperZombie );
			ResetPassiveHealTimer();
		}
	}
}

void obj_Zombie::ResetPassiveHealTimer()
{
	//  Reset the Passive Heal Timer.
	float curTime = r3dGetTime();
	if( IsSuperZombie() )
		HealTimer = curTime + u_GetRandom(_zai_SZHealTimeMin, _zai_SZHealTimeMax);
}