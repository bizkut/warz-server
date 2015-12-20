#include "r3dPCH.h"
#include "r3d.h"
#include "r3dLight.h"
#include "d3dfont.h"

#include "GameObjects/ObjManag.h"

#include "../EclipseStudio/Sources/GameLevel.h"
#include "../EclipseStudio/Sources/rendering/Deffered/RenderDeffered.h"
#include "../../GameEngine/TrueNature2/Terrain3.h"
#include "../../GameEngine/TrueNature2/Terrain2.h"
#include "../../GameEngine/gameobjects/PhysXWorld.h"
#include "../../GameEngine/ai/AutodeskNav/AutodeskNavMesh.h"
#include "../EclipseStudio/Sources/Editors/CollectionsManager.h"
#include "ObjectsCode/Missions/MissionManager.h"
#include "ObjectsCode/Zombies/sobj_Zombie.h"
#include "../EclipseStudio/Sources/ObjectsCode/Gameplay/obj_NoClipBox.h"

#include "ServerGameLogic.h"
#include "MasterServerLogic.h"
#include "KeepAliveReporter.h"
#include "TeamSpeakServer.h"

#ifdef ZOMBIE_PERFORMANCE_TESTING
extern uint32_t	g_ZombieFrameStartCalcPathCount;
extern float	g_ZombieFrameStartCalcTime;
extern uint32_t	g_ZombieFrameCalculatingPathCount;
extern float	g_ZombieFrameCalculatingTime;
extern uint32_t	g_ZombieFrameBrainsCalcCount;
extern float	g_ZombieFrameBrainsCalculatingTime;
extern uint32_t	g_ZombieFrameGetClosestTargetCount;
extern float	g_ZombieFrameGetClosestTargetTime;
extern uint32_t	g_ZombieFrameGetClosestPlayerCount;
extern float	g_ZombieFrameGetClosestPlayerTime;
extern uint32_t	g_ZombieFrameCheckViewToPlayerCount;
extern float	g_ZombieFrameCheckViewToPlayerTime;
#endif

// PunkBuster SDK
#ifdef __WITH_PB__
#include "PunkBuster/pbcommon.h"
#endif

// just some crap to get server to link
	CD3DFont* 		Font_Label;
	r3dLightSystem WorldLightSystem;
	r3dCamera gCam ;
	float SunVectorXZProj = 0;
	class r3dSun * Sun;
	int CurHUDID = -1;

	r3dScreenBuffer* gBuffer_Depth;
	r3dScreenBuffer* ScreenBuffer;
	r3dScreenBuffer* gBuffer_Color;
	r3dScreenBuffer *gScreenSmall;
	r3dScreenBuffer * DepthBuffer;
	float* ShadowSplitDistances;
	DWORD DriverUpdater(HWND hParentWnd, DWORD VendorId, DWORD v1, DWORD v2, DWORD v3, DWORD v4, DWORD hash) { return hash; }
	void r3dScaleformBeginFrame() {}
	void r3dScaleformEndFrame() {}
	void SetNewSimpleFogParams() {}
	int g_bForceQualitySelectionInEditor = 0;
	void AdvanceLoadingProgress(float) {}
	void SetAdvancedFogParams() {}
	void SetVolumeFogParams() {}
	float ShadowSplitDistancesOpaqueHigh[NumShadowSlices+1] = {0};
	float *ShadowSplitDistancesOpaque = &ShadowSplitDistancesOpaqueHigh[0];
	ShadowMapOptimizationData gShadowMapOptimizationDataOpaque [ NumShadowSlices ];
	ShadowMapOptimizationData gShadowMapOptimizationData [ NumShadowSlices ];
	r3dCameraAccelerometer gCameraAccelerometer;
	#include "MeshPropertyLib.h"
	MeshPropertyLib * g_MeshPropertyLib = NULL;
	MeshPropertyLib::Entry* MeshPropertyLib::GetEntry( const string& key ) { return NULL; }
	MeshPropertyLib::Entry*	MeshPropertyLib::GetGlobalEntry( const string& key ) { return NULL; };
	void MeshPropertyLib::AddEntry( const string& key ) { r3dError("not implemented"); }
	void MeshPropertyLib::AddEntry( const string& category, const string& meshName ) { r3dError("not implemented"); }
	void MeshPropertyLib::AddGlobalEntry( const string& key )  { r3dError("not implemented"); }

	const float MAX_DIR_SHADOW_LENGTH = 2500.f;
	float ShadowSunOffset = 1;
// end of temp variables


void PlayGameServer()
{
  r3d_assert(gServerLogic.ginfo_.IsValid());
  switch(gServerLogic.ginfo_.mapId) 
  {
    default: 
      r3dError("invalid map id\n");
      break;
    case GBGameInfo::MAPID_Editor_Particles: 
      r3dGameLevel::SetHomeDir("WorkInProgress\\Editor_Particles"); 
      break;
    case GBGameInfo::MAPID_ServerTest:
      r3dGameLevel::SetHomeDir("WorkInProgress\\ServerTest");
      break;
    case GBGameInfo::MAPID_WZ_Colorado: 
      r3dGameLevel::SetHomeDir("Colorado_V2"); 
      break;
    case GBGameInfo::MAPID_WZ_California: 
      r3dGameLevel::SetHomeDir("California_V2"); 
      break;
    case GBGameInfo::MAPID_WZ_Cliffside: 
      r3dGameLevel::SetHomeDir("WZ_Cliffside"); 
      break;
    case GBGameInfo::MAPID_WZ_Caliwood: 
      r3dGameLevel::SetHomeDir("Caliwood"); 
      break;
    case GBGameInfo::MAPID_WZ_ColoradoV1: 
      r3dGameLevel::SetHomeDir("Colorado_V1"); 
      break;
    case GBGameInfo::MAPID_WZ_SanDiego: 
      r3dGameLevel::SetHomeDir("WZ_San_Diego"); 
      break;
    case GBGameInfo::MAPID_WZ_Devmap: 
      r3dGameLevel::SetHomeDir("WZ_Devmap"); 
      break;
	case GBGameInfo::MAPID_WZ_GameHard1:
      r3dGameLevel::SetHomeDir("WZ_GameHard1"); 
      break;
  }

  r3dResetFrameTime();
  
  GameWorld_Create();

  u_srand(timeGetTime());
  GameWorld().Init(OBJECTMANAGER_MAXOBJECTS, OBJECTMANAGER_MAXSTATICOBJECTS);
  
  g_pPhysicsWorld = new PhysXWorld;
  g_pPhysicsWorld->Init();

  LoadLevel_Objects( 1.f );

  gCollectionsManager.Init( 0, 1 );

  r3dOutToLog( "NavMesh.Load...\n" );
 #ifndef WO_SERVER
  gAutodeskNavMesh.Init();
#else
  gAutodeskNavMesh.Init(gServerLogic.GetPort() + 14879); // default server logic port is 34010, we add 14879 to get to 48889, which is one above the default Navigation Lab port.
#endif // WO_SERVER

#ifdef MISSIONS
  r3dOutToLog( "Loading Mission Data\n" );
  r3d_assert( Mission::g_pMissionMgr == NULL );
  Mission::g_pMissionMgr = new Mission::MissionManager();
  if( !Mission::g_pMissionMgr || !Mission::g_pMissionMgr->Init() )
  {
	  r3dOutToLog( "!!! Missions have been turned OFF!\n" );
	  Mission::g_pMissionMgr = NULL;
  }
#endif

#ifdef DISABLE_GI_ACCESS_FOR_DEV_EVENT_SERVER
  if (gServerLogic.ginfo_.gameServerId==148353 || gServerLogic.ginfo_.gameServerId==150340 || gServerLogic.ginfo_.gameServerId==150341|| gServerLogic.ginfo_.gameServerId==151732 || gServerLogic.ginfo_.gameServerId==151733 || gServerLogic.ginfo_.gameServerId==151734 || gServerLogic.ginfo_.gameServerId==151736
	  // for testing in dev environment
	  //|| gServerLogic.ginfo_.gameServerId==11
	  )
	  gServerLogic.PreloadDevEventLoadout();
#endif

  r3dResetFrameTime();
  GameWorld().Update();

  // init terrain3
  if( Terrain3 )
	  Terrain3->Update( gCam ) ;

  if( Terrain2 )
  Terrain2->UpdateAtlas( gCam ) ;

  r3dGameLevel::SetStartGameTime(r3dGetTime());

  r3dOutToLog("WorldObjects: %d + %d static\n", GameWorld().GetNumObjects(), GameWorld().GetStaticObjectCount());

  // TODO: Uncomment the code below when we get the FairFight API
  //if( obj_NoClipBox::s_vNoClipData.size() > 0 )
  //{
  //	// TODO: Call FairFight API and give them the NoClipBox data.
  //	obj_NoClipBox::s_vNoClipData.clear();
  //}

  r3dOutToLog("server main loop started\n");
  r3dResetFrameTime();

  gServerLogic.OnGameStart();

  gKeepAliveReporter.SetStarted(true);

  while(1) 
  {
	  ::Sleep(10);		// limit to 100 FPS

	  r3dEndFrame();

	  //r3dOutToLog("!!! Frame time: %.4f, FPS:%d\n", r3dGetFrameTime(), int(1.0f/r3dGetFrameTime()));

	  r3dStartFrame();

#ifdef ZOMBIE_PERFORMANCE_TESTING
	  g_ZombieFrameStartCalcPathCount = 0;
	  g_ZombieFrameStartCalcTime = 0.0f;
	  g_ZombieFrameCalculatingPathCount = 0;
	  g_ZombieFrameCalculatingTime = 0.0f;
	  g_ZombieFrameBrainsCalcCount = 0;
	  g_ZombieFrameBrainsCalculatingTime = 0.0f;
	  g_ZombieFrameGetClosestTargetCount = 0;
	  g_ZombieFrameGetClosestTargetTime = 0.0f;
	  g_ZombieFrameGetClosestPlayerCount = 0;
	  g_ZombieFrameGetClosestPlayerTime = 0.0f;
	  g_ZombieFrameCheckViewToPlayerCount = 0;
	  g_ZombieFrameCheckViewToPlayerTime = 0.0f;
#endif

	  //if(GetAsyncKeyState(VK_F1)&0x8000) r3dError("r3dError test");
	  //if(GetAsyncKeyState(VK_F2)&0x8000) r3d_assert(false && "r3d_Assert test");
	  gKeepAliveReporter.Tick(gServerLogic.curPlayers_);

	  float timeServerTick=0.0f, timeGameworldUpdate=0.0f, timeTerrain3Update=0.0f, timePhysXUpdate=0.0f, timeNavmeshUpdate=0.0f;
	  {
		  const float t1 = r3dGetTime();
		  gServerLogic.Tick();
		  gMasterServerLogic.Tick();
		  const float t2 = r3dGetTime() - t1;
		  timeServerTick = t2;
		  //r3dOutToLog("!!! Server tick took %f sec\n", t2);
	  }

	  if(gMasterServerLogic.IsMasterDisconnected()) {
		  r3dOutToLog("Master Server disconnected, exiting\n");
		  gKeepAliveReporter.SetStarted(false);
		  return;
	  }

	  {
		  const float t1 = r3dGetTime();
		  GameWorld().StartFrame();
		  GameWorld().Update();
		  const float t2 = r3dGetTime() - t1;
		  timeGameworldUpdate = t2;
		  //r3dOutToLog("!!! gameworld update took %f sec\n", t2);
	  }

	  {
		  const float t1 = r3dGetTime();
		  if( Terrain3 )
			  Terrain3->Update( gCam ) ;
		  const float t2 = r3dGetTime() - t1;
		  timeTerrain3Update = t2;
		  //r3dOutToLog("!!! terrain3 update took %f sec\n", t2);
	  }

	  // start physics after game world update right now, as gameworld will move some objects around if necessary
	  {
		  const float t1 = r3dGetTime();
		  g_pPhysicsWorld->StartSimulation();
		  g_pPhysicsWorld->EndSimulation();
		  const float t2 = r3dGetTime() - t1;
		  timePhysXUpdate = t2;
		  //r3dOutToLog("!!! physX update took %f sec\n", t2);
	  }

#if ENABLE_AUTODESK_NAVIGATION
	  const float t1 = r3dGetTime();
	  gAutodeskNavMesh.Update();
	  const float t2 = r3dGetTime() - t1;
	  timeNavmeshUpdate = t2;

	  bool showSpd = (GetAsyncKeyState('Q') & 0x8000) && (GetAsyncKeyState('E') & 0x8000);
	  bool showKys = false; //(GetAsyncKeyState('Q') & 0x8000) && (GetAsyncKeyState('R') & 0x8000);

	  extern int _zstat_NumZombies;
	  extern int _zstat_NavFails;
	  extern int _zstat_Disabled;
	  if(showKys) {
		  gAutodeskNavMesh.perfBridge.Dump();
		  r3dOutToLog("NavMeshUpdate %f sec, z:%d, bad:%d, f:%d\n", t2, _zstat_NumZombies, _zstat_Disabled, _zstat_NavFails);
	  }
	  if(showSpd) {
		  r3dOutToLog("NavMeshUpdate %f sec, z:%d, bad:%d, f:%d\n", t2, _zstat_NumZombies, _zstat_Disabled, _zstat_NavFails);
	  }
#ifndef _DEBUG    
	  if((t2 > (1.0f / 25.0f))) {
		  r3dOutToLog("!!!! NavMeshUpdate %f sec, z:%d, bad:%d, f:%d\n", t2, _zstat_NumZombies, _zstat_Disabled, _zstat_NavFails);
		  //gAutodeskNavMesh.perfBridge.Dump();
		  //r3dOutToLog("!!!! NavMeshUpdate %f sec, z:%d, bad:%d, f:%d, navPend:%d\n", t2, _zstat_NumZombies, _zstat_Disabled, _zstat_NavFails, _zstat_UnderProcess);
	  }
#endif
#endif // ENABLE_AUTODESK_NAVIGATION

	  GameWorld().EndFrame();

	  if(timeServerTick+timeGameworldUpdate+timeTerrain3Update+timePhysXUpdate+timeNavmeshUpdate>0.09f)
	  {
		  r3dOutToLog("!!!PERFORMANCE REPORT:\ntick:%.2f\ngameworld:%.2f\nterrain3:%.2f\nPhysX:%.2f\nnavmesh:%.2f\nEND\n",
			  timeServerTick,timeGameworldUpdate,timeTerrain3Update,timePhysXUpdate,timeNavmeshUpdate);
	  }

#ifdef ZOMBIE_PERFORMANCE_TESTING
//#ifndef FINAL_BUILD
//	  r3d_assert(g_ZombieFrameStartCalcPathCount >= 0);
//	  r3d_assert(g_ZombieFrameStartCalcPathCount <= obj_Zombie::s_ListOfAllActiveZombies.size());
//	  r3d_assert(g_ZombieFrameCalculatingPathCount >= 0);
//	  r3d_assert(g_ZombieFrameCalculatingPathCount <= obj_Zombie::s_ListOfAllActiveZombies.size());
//	  r3d_assert(g_ZombieFrameBrainsCalcCount >= 0);
//	  r3d_assert(g_ZombieFrameBrainsCalcCount <= obj_Zombie::s_ListOfAllActiveZombies.size());
//#endif
	  uint32_t	zombieReportingThreshold = obj_Zombie::s_ListOfAllActiveZombies.size() / 4;
	  float		timeReportingThreshold = 0.003f;
	  if( g_ZombieFrameStartCalcPathCount > zombieReportingThreshold )
		  r3dOutToLog("!!! Navigation Performance Warning: More than %d zombies STARTED CALCULATING a path this frame! %d/%d\nAnd took %.4f seconds doing it.\n", zombieReportingThreshold, g_ZombieFrameStartCalcPathCount, obj_Zombie::s_ListOfAllActiveZombies.size(), g_ZombieFrameStartCalcTime );
	  else if( g_ZombieFrameStartCalcTime > timeReportingThreshold )
		  r3dOutToLog("!!! Navigation Performance Warning: Took longer than %.4f seconds to START CALCULATING zombie paths this frame! (%.4f seconds)\nAnd was executed on %d zombies.\n", timeReportingThreshold, g_ZombieFrameStartCalcTime, g_ZombieFrameStartCalcPathCount );
	  if( g_ZombieFrameCalculatingPathCount > zombieReportingThreshold )
		  r3dOutToLog("!!! Navigation Performance Warning: More than %d zombies ARE CALCULATING a path this frame! %d/%d\nAnd took %.4f seconds doing it.\n", zombieReportingThreshold, g_ZombieFrameCalculatingPathCount, obj_Zombie::s_ListOfAllActiveZombies.size(), g_ZombieFrameCalculatingTime );
	  else if( g_ZombieFrameCalculatingTime > timeReportingThreshold )
		  r3dOutToLog("!!! Navigation Performance Warning: Took longer than %.4f seconds to CALCULATE zombie paths this frame! (%.4f seconds)\nAnd was executed on %d zombies.\n", timeReportingThreshold, g_ZombieFrameCalculatingTime, g_ZombieFrameCalculatingPathCount );
	  if( g_ZombieFrameBrainsCalcCount > zombieReportingThreshold )
		  r3dOutToLog("!!! AI Performance Warning: More than %d zombie brains ARE EVALUATING priorities this frame! %d/%d\nAnd took %.4f seconds doing it.\n", zombieReportingThreshold, g_ZombieFrameBrainsCalcCount, obj_Zombie::s_ListOfAllActiveZombies.size(), g_ZombieFrameBrainsCalculatingTime );
	  else if( g_ZombieFrameBrainsCalculatingTime > timeReportingThreshold )
		  r3dOutToLog("!!! AI Performance Warning: Took longer than %.4f seconds to EVALUATE zombie priorities this frame! (%.4f seconds)\nAnd was executed on %d zombies.\n", timeReportingThreshold, g_ZombieFrameBrainsCalculatingTime, g_ZombieFrameBrainsCalcCount );
	  if( g_ZombieFrameGetClosestTargetCount > zombieReportingThreshold * gServerLogic.MAX_PEERS_COUNT )
		  r3dOutToLog("!!! AI Performance Warning: More than %d closest target DISTANCE CHECKS have been made this frame! %d/%d\n And took %.4f seconds doing it.\n", zombieReportingThreshold * gServerLogic.MAX_PEERS_COUNT, g_ZombieFrameGetClosestTargetCount, obj_Zombie::s_ListOfAllActiveZombies.size() * gServerLogic.MAX_PEERS_COUNT, g_ZombieFrameGetClosestTargetTime );
	  else if( g_ZombieFrameGetClosestTargetTime > timeReportingThreshold )
		  r3dOutToLog("!!! AI Performance Warning: Took longer than %.4f seconds to perform target DISTANCE CHECKS this frame! (%.4f seconds)\nAnd was executed %d times (Max = numTargets * numZombies).\n", timeReportingThreshold, g_ZombieFrameGetClosestTargetTime, g_ZombieFrameGetClosestTargetCount );
	  if( g_ZombieFrameGetClosestPlayerCount > zombieReportingThreshold * gServerLogic.MAX_PEERS_COUNT )
		  r3dOutToLog("!!! AI Performance Warning: More than %d closest player DISTANCE CHECKS have been made this frame! %d/%d\n And took %.4f seconds doing it.\n", zombieReportingThreshold * gServerLogic.MAX_PEERS_COUNT, g_ZombieFrameGetClosestPlayerCount, obj_Zombie::s_ListOfAllActiveZombies.size() * gServerLogic.MAX_PEERS_COUNT, g_ZombieFrameGetClosestPlayerTime );
	  else if( g_ZombieFrameGetClosestPlayerTime > timeReportingThreshold )
		  r3dOutToLog("!!! AI Performance Warning: Took longer than %.4f seconds to perform player DISTANCE CHECKS this frame! (%.4f seconds)\nAnd was executed %d times (Max = numPlayers * numZombies).\n", timeReportingThreshold, g_ZombieFrameGetClosestPlayerTime, g_ZombieFrameGetClosestPlayerCount );
	  if( g_ZombieFrameCheckViewToPlayerCount > zombieReportingThreshold * gServerLogic.MAX_PEERS_COUNT )
		  r3dOutToLog("!!! AI Performance Warning: More than %d player VISIBILITY CHECKS have been made this frame! %d/%d\n And took %.4f seconds doing it.\n", zombieReportingThreshold * gServerLogic.MAX_PEERS_COUNT, g_ZombieFrameCheckViewToPlayerCount, obj_Zombie::s_ListOfAllActiveZombies.size() * gServerLogic.MAX_PEERS_COUNT, g_ZombieFrameCheckViewToPlayerTime );
	  else if( g_ZombieFrameCheckViewToPlayerTime > timeReportingThreshold )
		  r3dOutToLog("!!! AI Performance Warning: Took longer than %.4f seconds to perform player VISIBILITY CHECKS this frame! (%.4f seconds)\nAnd was executed %d times (Max = numPlayers * numZombies).\n", timeReportingThreshold, g_ZombieFrameCheckViewToPlayerTime, g_ZombieFrameCheckViewToPlayerCount );
	  uint32_t allChecks = g_ZombieFrameStartCalcPathCount + g_ZombieFrameCalculatingPathCount + g_ZombieFrameBrainsCalcCount + g_ZombieFrameGetClosestTargetCount + g_ZombieFrameGetClosestPlayerCount + g_ZombieFrameCheckViewToPlayerCount;
	  float allTime = g_ZombieFrameStartCalcTime + g_ZombieFrameCalculatingTime + g_ZombieFrameBrainsCalculatingTime + g_ZombieFrameGetClosestTargetTime + g_ZombieFrameGetClosestPlayerTime + g_ZombieFrameCheckViewToPlayerTime;
	  if( allTime > timeReportingThreshold )
		  r3dOutToLog("!!! Navigation/AI Performance Warning: All checks %d being made took longer than %.4f seconds to perform! (%.4f seconds)\nStart Calc(%.4f): %d\nFrame Calc(%.4f): %d\nBrains Calc(%.4f): %d\nTarget Dist(%.4f): %d\nPlayer Dist(%.4f): %d\nVisibility(%.4f): %d\n", allChecks, timeReportingThreshold, allTime, g_ZombieFrameStartCalcTime, g_ZombieFrameStartCalcPathCount, g_ZombieFrameCalculatingTime, g_ZombieFrameCalculatingPathCount, g_ZombieFrameBrainsCalculatingTime, g_ZombieFrameBrainsCalcCount, g_ZombieFrameGetClosestTargetTime, g_ZombieFrameGetClosestTargetCount, g_ZombieFrameGetClosestPlayerTime, g_ZombieFrameGetClosestPlayerCount, g_ZombieFrameCheckViewToPlayerTime, g_ZombieFrameCheckViewToPlayerCount );
#endif

	  // Process PunkBuster Server Events
#ifdef __WITH_PB__
	  PbServerProcessEvents() ;
#endif

	  if(gServerLogic.gameFinished_)
		  break;
  }

  // set that we're finished
  gKeepAliveReporter.SetStarted(false);

  gMasterServerLogic.FinishGame();
  
  gServerLogic.DumpPacketStatistics();
  
  gCollectionsManager.Destroy();

  GameWorld_Destroy();
  
  return;
}
