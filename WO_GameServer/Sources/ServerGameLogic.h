#pragma once

#include "r3dNetwork.h"
#include "multiplayer/P2PMessages.h"

#include "../../ServerNetPackets/NetPacketsGameInfo.h"
#include "Backend/ServerUserProfile.h"

#define ENABLE_GAMEBLOCKS


class GameObject;
class obj_ServerPlayer;
class obj_ServerGrenade;
class obj_ServerLockbox;
class ServerWeapon;
class WeaponConfig;

#ifdef VEHICLES_ENABLED
class obj_Vehicle;
#endif

__forceinline obj_ServerPlayer* IsServerPlayer(const GameObject* obj)
{
	if(obj == NULL) return NULL;
	if(obj->isObjType(OBJTYPE_Human)) return (obj_ServerPlayer*)obj;
	else return NULL;
}

class ServerGameLogic : public r3dNetCallback
{
  protected:
	r3dNetwork	g_net;
	// r3dNetCallback virtuals
virtual	void		OnNetPeerConnected(DWORD peerId);
virtual	void		OnNetPeerDisconnected(DWORD peerId);
virtual	void		OnNetData(DWORD peerId, const r3dNetPacketHeader* packetData, int packetSize);

private:
	wiInventoryItem devEventItems[72];
	int				devEventItemsCount;
	int				devEventBackpackId;

	void GiveDevEventLoadout(DWORD peerId);
public:
	void PreloadDevEventLoadout();

  public:
	GBGameInfo	ginfo_;	// game info
	uint32_t	creatorID_; // customerID of player who created game, 0 - if permanent game
	void		SetNewGameInfo(DWORD flags, int gameTimeLimit);
	void		SetPVEinfo(DWORD isPVE); // for PVE maps

	float		AirDropSpawnTime;
	
	// peer-to-player array
	enum peerStatus_e 
	{
	  PEER_FREE,
	  PEER_CONNECTED,
	  PEER_VALIDATED1,	// version validated
	  PEER_LOADING,
	  PEER_PLAYING,		// 
	};
	
	struct peerInfo_s 
	{
	  peerStatus_e	status_;
	  float		startTime;	// status start time
	  void		SetStatus(peerStatus_e status) {
	    status_   = status;
	    startTime = r3dGetTime();
	  }

	  int		playerIdx;
	  obj_ServerPlayer* player;
	  
	  // user id and it profile
	  DWORD		CustomerID;
	  DWORD		SessionID;
	  DWORD		CharID;
	  DWORD		startGameAns; // PKT_S2C_StartGameAns_s::RES_ codes
	  float		nextPingOnLoad; // time for next ping while player is loading
	  CServerUserProfile temp_profile;
	  bool		isServerHop;
	  DWORD		voicePeerId;	// assigned unique voice id for peer
	  uint16_t	voiceClientId;	// assigned clientID from TS

	  // security report stuff (active when player is created)
	  float		secRepRecvTime;	// last time security report was received
	  float		secRepGameTime;	// last value of reported game time
	  float		secRepRecvAccum;
	  float		lastPacketTime;
	  int		lastPacketId;
	  
	  void Clear()
	  {
	    status_     = PEER_FREE;
	    startTime   = r3dGetTime();
	    
	    playerIdx   = -1;
	    player      = NULL;

	    startGameAns= 0;
	    CustomerID  = 0;
	    CharID      = 0;
	    
	    voicePeerId   = 0;
	    voiceClientId = 0;
	  }
	};

	enum { MAX_PEERS_COUNT = MAX_NUM_PLAYERS + 16 }; // (add some additional slots to include temporary connects)
	peerInfo_s	peers_[MAX_PEERS_COUNT];	// peer to player table
	peerInfo_s&	GetPeer(DWORD peerId)
	{
          r3d_assert(peerId < MAX_PEERS_COUNT);
          return peers_[peerId];
	}

	void		DisconnectPeer(DWORD peerId, bool cheat, const char* message, ...);

	// actual player data - based on it ID
	peerInfo_s*	plrToPeer_[MAX_NUM_PLAYERS];	// index to players table
	obj_ServerPlayer* plrList_[MAX_NUM_PLAYERS];	// flat players list [0..curPlayers_]
	int		curPlayers_;
	int		maxLoggedPlayers_;

	float m_StartGameTime; //gamehardcore
	float m_StartGameTimeR; //gamehardcore
	int countTimeFinish; //gamehardcore
	bool		ForceStarGame;//gamehardcore

	volatile int	curPeersConnected;
	obj_ServerPlayer* CreateNewPlayer(DWORD peerId, const r3dPoint3D& spawnPos, float spawnDir, bool needsSpawnProtection);
	void		DeletePlayer(int playerIdx, obj_ServerPlayer* plr);
	obj_ServerPlayer* GetPlayerByIdx(int playerIdx)
	{
	  r3d_assert(playerIdx < MAX_NUM_PLAYERS);
	  if(plrToPeer_[playerIdx] == NULL)
	    return NULL;

	  return plrToPeer_[playerIdx]->player;
	}

	bool		IsAdmin(DWORD CustomerID);

	typedef stdext::hash_map<DWORD, float>  TKickedPlayers;
	TKickedPlayers	kickedPlayers_;
	obj_ServerPlayer* GetPlayer(int playerIdx)
	{
	  r3d_assert(playerIdx < MAX_NUM_PLAYERS);
	  if(plrToPeer_[playerIdx] == NULL)
	    return NULL;

	  return plrToPeer_[playerIdx]->player;
	}
	DWORD		GetExternalIP()
	{
	    return g_net.firstBindIP_;
	}

	USHORT		GetPort()
	{
		return g_net.GetHostPort();
	}

	void		SendSystemChatMessageToPeer(DWORD peerId, const obj_ServerPlayer* fromPlr, const char* msg);
	
	bool		CheckForPlayersAround(const r3dPoint3D& pos, float dist);
	void		GetStartSpawnPosition(const wiCharDataFull& loadout, r3dPoint3D* pos, float* dir, bool& needsSpawnProtection);
	void		  GetSpawnPositionNewPlayer(const r3dPoint3D& GamePos, r3dPoint3D* pos, float* dir);
	void		  GetSpawnPositionAfterDeath(const r3dPoint3D& GamePos, r3dPoint3D* pos, float* dir);
	void		  GetSpawnPositionAfterDeath2(const r3dPoint3D& GamePos, r3dPoint3D* pos, float* dir);

	r3dPoint3D	AdjustPositionToFloor(const r3dPoint3D& pos);
	obj_ServerLockbox* TryToOpenLockbox(obj_ServerPlayer* fromPlr, gp2pnetid_t lockboxID, const char* AccessCodeS);
	
	void		NetRegisterObjectToPeers(GameObject* netObj);		// register any net object visibility to current players
	void		UpdateNetObjVisData(const obj_ServerPlayer* plr);	// update all network object visibility of passed player
	void		  UpdateNetObjVisData(DWORD peerId, GameObject* netObj);
	void		ResetNetObjVisData(const obj_ServerPlayer* plr);
	
	#define DEFINE_PACKET_FUNC(XX) \
	  void On##XX(const XX##_s& n, GameObject* fromObj, DWORD peerId, bool& needPassThru);
	#define IMPL_PACKET_FUNC(CLASS, XX) \
	  void CLASS::On##XX(const XX##_s& n, GameObject* fromObj, DWORD peerId, bool& needPassThru)

	int		ProcessWorldEvent(GameObject* fromObj, DWORD eventId, DWORD peerId, const void* packetData, int packetSize);
	 DEFINE_PACKET_FUNC(PKT_C2S_ValidateConnectingPeer);
	 DEFINE_PACKET_FUNC(PKT_C2S_JoinGameReq);
	 DEFINE_PACKET_FUNC(PKT_C2S_StartGameReq);
	 DEFINE_PACKET_FUNC(PKT_C2C_ChatMessage);
	 DEFINE_PACKET_FUNC(PKT_C2S_DataUpdateReq);
	 DEFINE_PACKET_FUNC(PKT_S2C_CamuDataS);
	 DEFINE_PACKET_FUNC(PKT_C2S_SecurityRep);
	 DEFINE_PACKET_FUNC(PKT_C2S_CameraPos);
	 DEFINE_PACKET_FUNC(PKT_C2S_UseNetObject);
	 DEFINE_PACKET_FUNC(PKT_C2S_PreparingUseNetObject);
	 DEFINE_PACKET_FUNC(PKT_C2S_LockboxOpAns);
	 DEFINE_PACKET_FUNC(PKT_C2S_LockboxKeyReset);
	 DEFINE_PACKET_FUNC(PKT_C2S_CreateNote);
	 DEFINE_PACKET_FUNC(PKT_C2S_TEST_SpawnDummyReq);
	 DEFINE_PACKET_FUNC(PKT_C2S_Admin_GiveItem);
	 DEFINE_PACKET_FUNC(PKT_S2C_SetupTraps);
	 DEFINE_PACKET_FUNC(PKT_C2S_GetUAV);
#ifdef MISSIONS
	 DEFINE_PACKET_FUNC(PKT_C2S_AcceptMission);
	 DEFINE_PACKET_FUNC(PKT_C2S_AbandonMission);
	 DEFINE_PACKET_FUNC(PKT_C2S_MissionStateObjectUse);
#endif
	 DEFINE_PACKET_FUNC(PKT_C2S_DBG_LogMessage);
#ifdef VEHICLES_ENABLED
	 DEFINE_PACKET_FUNC(PKT_C2S_VehicleEnter);
	 DEFINE_PACKET_FUNC(PKT_C2S_VehicleExit);
	 DEFINE_PACKET_FUNC(PKT_C2S_VehicleHitTarget);
	 DEFINE_PACKET_FUNC(PKT_C2S_VehicleStopZombie);
#endif
	 void		OnPKT_C2S_ScreenshotData(DWORD peerId, const int size, const char* data,const char* FoundPlayer);
	 void		OnPKT_C2S_PunkBuster(DWORD peerId, const int size, const char* data);
	 
	void		ValidateMove(GameObject* fromObj, const void* packetData, int packetSize);
	
	int		ProcessChatCommand(obj_ServerPlayer* plr, const char* cmd);
	int       Cmd_GodMode(obj_ServerPlayer* plr, const char* cmd);  
	int		  Cmd_Teleport(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_TeleportToPlayer(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_TeleportPlayerToDev(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_GiveItem(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_SetVitals(obj_ServerPlayer* plr, const char* cmd);
	int       Cmd_DevKit(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_Kick(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_Ban(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_Loc(obj_ServerPlayer* plr, const char* cmd); // locate player
	int		  Cmd_AirDrop(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_ForceGame(obj_ServerPlayer* plr, const char* cmd);
	int		  Cmd_ZombieSpawn(obj_ServerPlayer* plr, const char* cmd, bool isSuperZombie);
#ifdef MISSIONS
	int		  Cmd_ResetMissionData(obj_ServerPlayer* plr, const char* cmd);
#endif
#ifdef VEHICLES_ENABLED
	int		  Cmd_VehicleSpawn(obj_ServerPlayer* plr, const char* cmd);
#endif

	int		  admin_TeleportPlayer(obj_ServerPlayer* plr, float x, float z);
	int		  admin_TeleportPlayer(obj_ServerPlayer* plr, float x, float y, float z); // to exact location

	void		RelayPacket(DWORD peerId, const GameObject* from, const DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered);
	void		p2pBroadcastToActive(const GameObject* from, DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered = true);
	void		p2pBroadcastToAll(DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered = true);
	obj_ServerPlayer* FindPlayer(DWORD peerId);
	void		p2pSendToPeer(DWORD peerId, const GameObject* from, DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered = true);
	void		p2pSendRawToPeer(DWORD peerId, DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered = true);

	bool		CanDamageThisObject(const GameObject* targetObj);
	void		ApplyDamage(GameObject* sourceObj, GameObject* targetObj, const r3dPoint3D& dmgPos, float damage, bool force_damage, STORE_CATEGORIES damageSource, uint32_t dmgItemID, bool canApplyBleeding = true);
	bool		ApplyDamageToPlayer(GameObject* sourceObj, obj_ServerPlayer* targetPlr, const r3dPoint3D& dmgPos, float damage, int bodyBone, int bodyPart, bool force_damage, STORE_CATEGORIES damageSource, uint32_t dmgItemID, const int airState = 0, bool canApplyBleeding = true);  
	bool		ApplyDamageToZombie(GameObject* sourceObj, GameObject* targetZombie, const r3dPoint3D& dmgPos, float damage, int bodyBone, int bodyPart, bool force_damage, STORE_CATEGORIES damageSource, uint32_t dmgItemID);  
#ifdef VEHICLES_ENABLED
	bool		ApplyDamageToVehicle(GameObject* fromObj, GameObject* targetVehicle, const r3dPoint3D& dmgPos, float damage, bool forceDamage, STORE_CATEGORIES damageSource, uint32_t damageItemId);
#endif
	void		DoKillPlayer(GameObject* sourceObj, obj_ServerPlayer* targetPlr, STORE_CATEGORIES weaponCat, bool forced_by_server=false, bool fromPlayerInAir = false, bool targetPlayerInAir = false );
	void		DoExplosion(GameObject* fromObj, GameObject* sourceObj, r3dVector& forwVector, r3dVector& lastCollisionNormal, float direction, float damageArea, float damageAmount, STORE_CATEGORIES damageCategory, uint32_t damageItemId, bool isFromVehicle = false); // const WeaponConfig* wpnConfig)

	int			getReputationKillEffect(int repFromPlr, int repKilledPlr);
	
	float		getInGameTime();

	void		InformZombiesAboutSound(const obj_ServerPlayer* plr, const ServerWeapon* wpn);
	void		InformZombiesAboutSoundItemID(const obj_ServerPlayer* plr, int ItemID);
	void		InformZombiesAboutGrenadeSound(const obj_ServerGrenade* grenade, bool isExplosion);
#ifdef VEHICLES_ENABLED
	void		InformZombiesAboutVehicleExplosion(const obj_Vehicle* vehicle);
#endif

	void		AddPlayerReward(obj_ServerPlayer* plr, EPlayerRewardID rewardID);
	wiStatsTracking	  GetRewardData(obj_ServerPlayer* plr, EPlayerRewardID rewardID);
	void		AddDirectPlayerReward(obj_ServerPlayer* plr, const wiStatsTracking& in_rwd, const char* rewardName);

	//
	// async api funcs.
	//
	void		ApiPlayerUpdateChar(obj_ServerPlayer* plr, bool disconnectAfter = false);
	void		ApiPlayerLeftGame(DWORD peerId);

	// security stuff
	void		CheckPeerValidity();
	void		LogInfo(DWORD peerId, const char* msg, const char* fmt = "", ...);
	void		LogCheat(DWORD peerId, int LogID, int disconnect, const char* msg, const char* fmt = "", ...);
	
	bool		gameFinished_;
	int		weaponDataUpdates_;	// number of times weapon data was updated

	float		gameStartTime_;
	__int64		gameStartTimeAdminOffset; // to change time on server
	__int64		GetUtcGameTime();
	void		SendWeaponsInfoToPlayer(DWORD peerId);
	
	// hibernate logic
	float		secsWithoutPlayers_;
	int		hibernateStarted_;
	void		UpdateHibernate();
	void		  StartHibernate();
	
	float		nextLootboxUpdate_;

	struct AirDropPositions
	{
		r3dPoint3D	m_location;
		float		m_radius;
		uint32_t		m_LootBoxID1;
		uint32_t		m_LootBoxID2;
		uint32_t		m_LootBoxID3;
		uint32_t		m_LootBoxID4;
		uint32_t		m_LootBoxID5;
		uint32_t		m_LootBoxID6;
		int			m_DefaultItems;
	};
	std::map<uint32_t, AirDropPositions> AirDropsPos;

	// groups
	uint32_t	nextGroupID;
	uint32_t	getNextGroupID() { return nextGroupID++;}
	uint32_t	getNumPlayersInGroup(uint32_t groupID);
	void		joinPlayerToGroup(obj_ServerPlayer* plr, uint32_t groupID);
	void		createNewPlayerGroup(obj_ServerPlayer* leader, obj_ServerPlayer* plr, uint32_t groupID);
	bool		SetAirDrop( const AirDropPositions& AirDrop );
	void		leavePlayerFromGroup(obj_ServerPlayer* plr);
	void		cleanPlayerGroupInvites(obj_ServerPlayer* plr);
	
	// data size for each logical packet ids
	__int64		netRecvPktSize[256];
	__int64		netSentPktSize[256];
	void		DumpPacketStatistics();

	DWORD		net_lastFreeId;
	DWORD		net_mapLoaded_LastNetID; // to make sure that client has identit map as server
	DWORD		GetFreeNetId();
	
  public:
	ServerGameLogic();
	virtual ~ServerGameLogic();

	void		Init(const GBGameInfo& ginfo, uint32_t creatorID);
	void		CreateHost(int port);
	void		OnGameStart();

	void		Disconnect();

	void		Tick();

	void		SendGameClose();
	
	struct WeaponStats_s
	{
	  uint32_t ItemID;
	  int	ShotsFired;
	  int	ShotsHits;
	  int	Kills;
	  
	  WeaponStats_s()
	  {
		ItemID = 0;
		ShotsFired = 0;
		ShotsHits = 0;
		Kills = 0;
	  }
	};
	std::vector<WeaponStats_s> weaponStats_;
	void		TrackWeaponUsage(uint32_t ItemID, int ShotsFired, int ShotsHits, int Kills);
};

extern	ServerGameLogic	gServerLogic;
