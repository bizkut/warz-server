#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"
#include <shellapi.h>
#include "process.h"

#include "CkHttpRequest.h"
#include "CkHttp.h"
#include "CkHttpResponse.h"

#pragma warning(disable: 4065)	// switch statement contains 'default' but no 'case' labels

#include "MasterGameServer.h"
#include "MasterServer.h"
#include "MasterAsyncApiMgr.h"

#include "../../MasterServer/Sources/NetPacketsServerBrowser.h"
using namespace NetPacketsServerBrowser;

#include "ObjectsCode/weapons/WeaponConfig.h"
#include "ServerWeapons/MasterServerWeaponArmory.h"
#include "../../EclipseStudio/Sources/backend/WOBackendAPI.h"

#include "../EclipseStudio/Sources/GameLevel.h"
#include "../EclipseStudio/Sources/GameLevel.cpp"

	CMasterGameServer gMasterGameServer;
	r3dNetwork	serverNet;

extern int		gDomainPort;
extern bool		gDomainUseSSL;

CMasterGameServer::CMasterGameServer()
{
  curSuperId_ = 1;

  nextBillingHourUpdate_ = r3dGetTime() + (60 * 60);
  
  itemsDbUpdateThread_ = NULL;
  itemsDbUpdateFlag_   = ITEMSDBUPDATE_Unactive;
  itemsDbLastUpdate_   = 0;
  newWeaponArmory_     = NULL;
  
  blockedAttempts_     = 0;
  
  shuttingDown_        = false;
  
  return;
}

CMasterGameServer::~CMasterGameServer()
{
}

void CMasterGameServer::Start(int port, int in_serverId)
{
  SetConsoleTitle("WO::Master");
  
  masterServerId_  = in_serverId;
  r3d_assert(masterServerId_ > 0 && masterServerId_ < 255);
  
  // give time for supervisors to connect to us (except for dev servers)
  supersCooldown_  = r3dGetTime() + 30.0f; // controlled now by the number of supers connected
  if(masterServerId_ >= MASTERSERVER_DEV_ID) supersCooldown_ = -1;
  
#if ENABLED_SERVER_WEAPONARMORY
  DoFirstItemsDbUpdate();
#endif
  
  serverNet.Initialize(this, "serverNet");
  if(!serverNet.CreateHost(port, MAX_PEERS_COUNT)) {
    r3dError("CreateHost failed\n");
  }

  r3dOutToLog("MasterGameServer started at port %d\n", port);
  nextLogTime_ = r3dGetTime();
  
  return;
}

int CMasterGameServer::IsGameServerIdStarted(DWORD gameServerId, CServerG** out_game)
{
  const float curTime = r3dGetTime();

  // scan supervisors for active
  for(TSupersList::iterator it = supers_.begin(); it != supers_.end(); ++it) 
  {
    const CServerS* super = it->second;

    // for each game slot with correct permanent game index
    for(int gslot=0; gslot<super->maxGames_; gslot++)
    {
      if(super->games_[gslot].info.ginfo.gameServerId != gameServerId)
        continue;

      // closing games is not active and not free
      if(curTime < super->games_[gslot].closeTime)
        continue;

      // check if slot is not used
      if(super->games_[gslot].game == NULL &&
        (curTime > super->games_[gslot].createTime + CServerS::REGISTER_EXPIRE_TIME)) {
        continue;
      }

      CServerG* game = super->games_[gslot].game;
      if(game == NULL) {
        // game not started yet (within registering time) - consider it as active
        return 2;
      }

      // game is fully active
      if(out_game) *out_game = game;
      return 1;
    }
  }

  return 0;
}

void CMasterGameServer::UpdateAllGames()
{
  static float nextCheck = -1;
  const float curTime = r3dGetTime();
  if(curTime < nextCheck)
    return;
  nextCheck = curTime + 0.5f;
  
  // no new games if we shutting down
  if(shuttingDown_)
    return;
  
  // don't do anything if there is no supervisors registered
  if(supers_.size() == 0)
    return;

  if(IsServerStartingUp())
    return;

  if(UpdatePermanentGames()) {
    // spawn 50 games per sec to avoid all servers spawning at once
    // not needed now as we have internal cooldown specific for each supervisor
    // nextCheck = curTime + 0.02f;
    return;
  }
}

bool CMasterGameServer::UpdatePermanentGames()
{
  // scan each permanent game slot and spawn new game if there isn't enough of them
  for(int pgslot=0; pgslot<gServerConfig->numPermGames_; pgslot++)
  {
    const CMasterServerConfig::permGame_s& pg = gServerConfig->permGames_[pgslot];
    r3d_assert(!pg.ginfo.IsRentedGame());

    // there should be only ONE game server of that ID running
    if(IsGameServerIdStarted(pg.ginfo.gameServerId, NULL))
      continue;

    // spawn new game
    // note: we don't have to check supervisor availability here, it'll be checked in CreateNewGame()
    CMSNewGameData ngd(pg.ginfo, "", 0);

    DWORD ip;
    DWORD port;
    __int64 sessionId;
    if(!CreateNewGame(ngd, &ip, &port, &sessionId)) {
      continue;
    }
	

/*
    r3dOutToLog("permgame: %d(%d out of %d) created at %s:%d\n", 
      pgslot,
      pg.curGames,
      pg.maxGames,
      inet_ntoa(*(in_addr*)&ip), 
      port);
*/      
    //disabled: supervisors now have internal cooldown so they won't spawn many games at once
    //return true;	// spawn only one game per tick
  }
  
  return false;
}

void CMasterGameServer::CloseExpiredGames()
{
  if(shuttingDown_)
    return;
  
  // kill active games that do not have entries in server list anymore
  for(TGamesList::iterator it = games_.begin(); it != games_.end(); ++it) 
  {
    const CServerG* game = it->second;
    const GBGameInfo& ginfo = game->getGameInfo();
    if(!ginfo.IsRentedGame())
      continue;
      
    if(gServerConfig->GetRentedGameInfo(ginfo.gameServerId) == NULL)
    {
      r3dOutToLog("closing game %s '%s' because of rent expiration time\n", game->GetName(), ginfo.name);

      SBPKT_M2G_KillGame_s n1(game->id_);
      net_->SendToPeer(&n1, sizeof(n1), game->peer_);
      net_->DisconnectPeer(game->peer_);
    }
  }
    
  return;
}

void CMasterGameServer::RequestShutdown()
{
  if(shuttingDown_)
    return;

  r3dOutToLog("--------- SHUTDOWN requested\n");

  shuttingDown_ = true;
  shutdownLeft_ = 300.0f; // 5 min shutdown
    
  // notify each game about shutdown
  for(TGamesList::iterator it = games_.begin(); it != games_.end(); ++it) 
  {
    const CServerG* game = it->second;

    SBPKT_M2G_ShutdownNote_s n(game->id_);
    n.reason   = 1;
    n.timeLeft = shutdownLeft_ - 5.0f; // make that game server shutdown 5 sec before us

    net_->SendToPeer(&n, sizeof(n), game->peer_, true);
  }
}

void CMasterGameServer::Tick()
{
  const float curTime = r3dGetTime();
  
  net_->Update();
  g_MSAsyncApiMgr->Tick();
  
  DisconnectIdlePeers();

  if(shuttingDown_) {
    shutdownLeft_ -= r3dGetFrameTime();
    return;
  }
  
  // do not start games if supervisor cooldown is active
  if(r3dGetTime() > supersCooldown_)
  {
    UpdateAllGames();
  }
  
  // pereodically update rented servers list, every 5 min
  if(curTime > gServerConfig->nextRentGamesCheck_)
  {
    gServerConfig->nextRentGamesCheck_ = curTime + 5 * 60.0f;
    g_MSAsyncApiMgr->AddJob(new CMSJobGetServerList());
  }
  
  if(masterServerId_ < MASTERSERVER_DEV_ID && curTime > nextBillingHourUpdate_)
  {
    r3dOutToLog("Starting CMSJobTickAllServers\n");
    nextBillingHourUpdate_ = curTime + (60 * 60);
    g_MSAsyncApiMgr->AddJob(new CMSJobTickAllServers());
  }

#if ENABLED_SERVER_WEAPONARMORY
  UpdateItemsDb();
#endif

  return;
}

void CMasterGameServer::Stop()
{
  if(net_)
    net_->Deinitialize();
}

void CMasterGameServer::DisconnectIdlePeers()
{
  const float IDLE_PEERS_CHECK = 0.2f;	// do checks every N seconds
  const float IDLE_PEERS_DELAY = 30.0f;	// peer have this N seconds to validate itself

  static float nextCheck = -1;
  const float curTime = r3dGetTime();
  if(curTime < nextCheck)
    return;
  nextCheck = curTime + IDLE_PEERS_CHECK;
  
  for(int i=0; i<MAX_PEERS_COUNT; i++) 
  {
    if(peers_[i].status != PEER_Connected)
      continue;
  
    if(curTime < peers_[i].connectTime + IDLE_PEERS_DELAY)
      continue;
      
    DisconnectCheatPeer(i, "game peer validation time expired");
  }
  
  return;
}

void CMasterGameServer::DisconnectCheatPeer(DWORD peerId, const char* message)
{
  r3dOutToLog("cheat: master peer%d, reason: %s\n", peerId, message);
  net_->DisconnectPeer(peerId);
  
  // fire up disconnect event manually, enet might skip if if other peer disconnect as well
  OnNetPeerDisconnected(peerId);
}

void CMasterGameServer::OnNetPeerConnected(DWORD peerId)
{
#if 0
  // do not do that anymore, code was here to prevent other clients to DDOS us
  DWORD ip = net_->GetPeerIp(peerId);
  for(size_t i=0; i<blockedIps_.size(); i++) {
    if(blockedIps_[i] == ip) {
      blockedAttempts_++;
      net_->DisconnectPeer(peerId);
      return;
    }
  }
#endif

  peer_s& peer = peers_[peerId];
  r3d_assert(peer.status == PEER_Free);
  
  peer.status = PEER_Connected;
  peer.connectTime = r3dGetTime();

  // report our version
  CREATE_PACKET(SBPKT_ValidateConnectingPeer, n);
  n.version = SBNET_VERSION;
  n.key1    = 0;
  net_->SendToPeer(&n, sizeof(n), true);

  return;
}

void CMasterGameServer::OnNetPeerDisconnected(DWORD peerId)
{
  peer_s& peer = peers_[peerId];
  switch(peer.status)
  {
    default: 
      r3dOutToLog("!!!!!!! invalid peer type %d disconnected\n", peer.status);
      break;
      
    case PEER_Free:
    case PEER_Connected:
    case PEER_Validated:
      break;
    
    case PEER_SuperServer:
    {
      TSupersList::iterator it = supers_.find(peerId);
      if(it == supers_.end()) {
        break;
      }
      
      CServerS* super = it->second;

      r3dOutToLog("master: %ssuper disconnected '%s'[%d] \n", super->disabled_ ? "shut down " : "", super->GetName(), super->id_); CLOG_INDENT;

      // supervisor is disconnected, this might be network problem and other supervisors will be disconnecting as well
      // so we initiate cooldown time to wait for disconnected games, etc, etc
      if(super->disabled_ == false && r3dGetTime() > supersCooldown_) {
        r3dOutToLog("starting cooldown timer\n");
        supersCooldown_ = r3dGetTime() + 60.0f;
      }

      // delete all games that belong to this supervisor
      DeleteAllSupervisorGames(super->id_);
      delete super;
    
      supers_.erase(it);
      
      break;
    }
  }
  
  peer.status = PEER_Free;
  return;
}

bool CMasterGameServer::DoValidatePeer(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize)
{
  peer_s& peer = peers_[peerId];
  
  if(peer.status >= PEER_Validated)
    return true;

  // we still can receive packets after peer was force disconnected
  if(peer.status == PEER_Free)
    return false;

  r3d_assert(peer.status == PEER_Connected);
  if(PacketData->EventID != SBPKT_ValidateConnectingPeer) {
    DisconnectCheatPeer(peerId, "wrong validate packet id");
    return false;
  }

  // check if we have correct validation packet, otherwise - put that ip to black list
  const SBPKT_ValidateConnectingPeer_s& n = *(SBPKT_ValidateConnectingPeer_s*)PacketData;
  if(sizeof(n) != PacketSize || n.version != SBNET_VERSION || n.key1 != SBNET_KEY1 || n.key2 != SBNET_KEY2) {
    DWORD ip = net_->GetPeerIp(peerId);
    blockedIps_.push_back(ip);
    char buf[128];
    sprintf(buf, "!!!! wrong validate packet %s %d%d%d%d", inet_ntoa(*(in_addr*)&ip), sizeof(n) != PacketSize, n.version != SBNET_VERSION, n.key1 != SBNET_KEY1, n.key2 != SBNET_KEY2);
    
    DisconnectCheatPeer(peerId, buf);
    return false;
  }

  // set peer to validated
  peer.status = PEER_Validated;
  return false;
}

void CMasterGameServer::OnNetData(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize)
{
  // PunkBuster Packet
  if ( PacketSize > 7 && memcmp ( PacketData, "\xff\xff\xff\xffPB_", 7) == 0 )
	return;

  if(PacketSize < sizeof(r3dNetPacketHeader)) {
    DisconnectCheatPeer(peerId, "too small packet");
    return;
  }

  // wait for validating packet
  if(!DoValidatePeer(peerId, PacketData, PacketSize))
    return;

  peer_s& peer = peers_[peerId];
  r3d_assert(peer.status >= PEER_Validated);
  
  switch(PacketData->EventID) 
  {
    default:
      r3dOutToLog("CMasterGameServer: invalid packetId %d", PacketData->EventID);
      DisconnectCheatPeer(peerId, "wrong packet id");
      return;
      
    case SBPKT_S2M_RegisterMachine:
    {
      const SBPKT_S2M_RegisterMachine_s& n = *(SBPKT_S2M_RegisterMachine_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);

      r3d_assert(supers_.find(peerId) == supers_.end());

      DWORD ip = net_->GetPeerIp(peerId);
      DWORD id = curSuperId_++;
      
      // check if that super is already connected or wasn't disconnected yet
      for(TSupersList::iterator it = supers_.begin(); it != supers_.end(); ++it) 
      {
        CServerS* super = it->second;
        if((super->ip_ == ip || super->ip_ == n.externalIpAddr) && strcmp(super->GetName(), n.serverName) == 0)
        {
          r3dOutToLog("master: DUPLICATE super registered '%s' ip:%s\n", n.serverName, inet_ntoa(*(in_addr*)&ip));
          net_->DisconnectPeer(peerId);
          return;
        }
      }
      
      CServerS* super = new CServerS(id, ip, peerId);
      super->Init(n);
      supers_.insert(TSupersList::value_type(peerId, super));
      
      r3d_assert(peer.status == PEER_Validated);
      peer.status = PEER_SuperServer;
      
      // send registration answer
      {
        SBPKT_M2S_RegisterAns_s n;
        n.id = id;
        net_->SendToPeer(&n, sizeof(n), peerId);
      }
      
      break;
    }
    
    case SBPKT_S2M_ShutdownNote:
    {
      const SBPKT_S2M_ShutdownNote_s& n = *(SBPKT_S2M_ShutdownNote_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);
      r3d_assert(peer.status == PEER_SuperServer);

      TSupersList::iterator it = supers_.find(peerId);
      r3d_assert(it != supers_.end());
      CServerS* super = it->second;
      super->DisableSupervisor();
      break;
    }
    
    case SBPKT_S2M_DisableSlot:
    {
      const SBPKT_S2M_DisableSlot_s& n = *(SBPKT_S2M_DisableSlot_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);
      r3d_assert(peer.status == PEER_SuperServer);

      TSupersList::iterator it = supers_.find(peerId);
      r3d_assert(it != supers_.end());
      CServerS* super = it->second;
      super->DisableSlot(n.slot);
      break;
    }

    case SBPKT_S2M_GameDisconnect:
    {
      const SBPKT_S2M_GameDisconnect_s& n = *(SBPKT_S2M_GameDisconnect_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);

      TGamesList::iterator it = games_.find(n.gameId);
      if(it != games_.end())
      {
        CServerG* game = it->second;
        
        const CServerS* super = GetServerByGameId(game->id_);

        r3dOutToLog("!!! game %s closed unexpectedly, id:%d, super:%s, sess:%I64x\n", 
          game->GetName(), 
          game->info_.ginfo.gameServerId, 
          super ? super->GetName() : "UNKNOWN", 
          game->info_.sessionId);

        DeleteGame(game);
      }
      break;
    }
    
    case SBPKT_G2M_RegisterGame:
    {
      const SBPKT_G2M_RegisterGame_s& n = *(SBPKT_G2M_RegisterGame_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);
      r3d_assert(peer.status == PEER_SuperServer);

      // register this game in supervisor
      CServerS* super = GetServerByGameId(n.gameId);
      if(super == NULL) {
        // this might happen when supervisor crashed between game start & registration
        r3dOutToLog("!!! game 0x%x without supervisor\n", n.gameId);
        break;
      }
      
      // check if we have that gameId already connected
      if(games_.find(n.gameId) != games_.end())
      {
        r3dOutToLog("!!! game 0x%x already connected\n", n.gameId);
        SBPKT_M2G_KillGame_s n1(n.gameId);
        net_->SendToPeer(&n1, sizeof(n1), peerId);
        break;
      }
      
      CServerG* game = super->CreateGame(n.gameId, peerId);
      if(!game) {
        // game was not created
        SBPKT_M2G_KillGame_s n1(n.gameId);
        net_->SendToPeer(&n1, sizeof(n1), peerId);
        break;
      }

      // register game
      r3dOutToLog("game %s connected\n", game->GetName());

      std::pair<TGamesList::iterator, bool> result;
      result = games_.insert(TGamesList::value_type(n.gameId, game));
      r3d_assert(result.second);
      result = gamesByGameServerId_.insert(TGamesList::value_type(game->getGameInfo().gameServerId, game));
      r3d_assert(result.second);
      
      #if ENABLED_SERVER_WEAPONARMORY
      SendArmoryInfoToGame(game);
      #endif
      break;
    }

    case SBPKT_G2M_AddPlayer:
    {
      const SBPKT_G2M_AddPlayer_s& n = *(SBPKT_G2M_AddPlayer_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);

      TGamesList::iterator it = games_.find(n.gameId);
      r3d_assert(it != games_.end());
      CServerG* game = it->second;
      game->AddPlayer(n);
      break;
    }

    case SBPKT_G2M_RemovePlayer:
    {
      const SBPKT_G2M_RemovePlayer_s& n = *(SBPKT_G2M_RemovePlayer_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);

      TGamesList::iterator it = games_.find(n.gameId);
      r3d_assert(it != games_.end());
      CServerG* game = it->second;
      game->RemovePlayer(n.playerIdx);
      break;
    }

    case SBPKT_G2M_FinishGame:
    {
      const SBPKT_G2M_FinishGame_s& n = *(SBPKT_G2M_FinishGame_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);

      TGamesList::iterator it = games_.find(n.gameId);
      r3d_assert(it != games_.end());
      CServerG* game = it->second;

      //r3dOutToLog("game %s finished\n", game->GetName());
      game->finished_ = true;

      break;
    }

    case SBPKT_G2M_CloseGame:
    {
      const SBPKT_G2M_CloseGame_s& n = *(SBPKT_G2M_CloseGame_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);

      TGamesList::iterator it = games_.find(n.gameId);
      r3d_assert(it != games_.end());
      CServerG* game = it->second;

      r3dOutToLog("game %s closed\n", game->GetName());
      DeleteGame(game);
      break;
    }
    
    #ifdef ENABLE_ARMORY_UPDATE
    case SBPKT_G2M_DataUpdateReq:
    {
      const SBPKT_G2M_DataUpdateReq_s& n = *(SBPKT_G2M_DataUpdateReq_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);

      r3dOutToLog("got SBPKT_G2M_DataUpdateReq\n");
      #if ENABLED_SERVER_WEAPONARMORY
      StartItemsDbUpdate(true);
      #endif
      break;
    }
    #endif

  }

  return;
}

void CMasterGameServer::DeleteGame(CServerG* game)
{
  CServerS* super = GetServerByGameId(game->id_);

  if(super) {
    super->DeregisterGame(game);
  } else {
    r3dOutToLog("master: game %s from UNKNOWN[%d] supervisor disconnected\n", game->GetName(), game->id_ >> 16);
  }

  games_.erase(game->id_);
  gamesByGameServerId_.erase(game->getGameInfo().gameServerId);

  delete game;
}

void CMasterGameServer::DeleteAllSupervisorGames(DWORD superId)
{
  std::vector<CServerG*> gamesToKill;
  
  // collect games on that supervisor (just in case, do not use supervisor internal game list)
  for(TGamesList::iterator it = games_.begin(); it != games_.end(); ++it) 
  {
    CServerG* game = it->second;

    DWORD sid = game->id_ >> 16;
    if(sid == superId)
    {
      gamesToKill.push_back(game);
    }
  }
  
  // kill them
  for(size_t i=0; i<gamesToKill.size(); i++)
  {
    CServerG* game = gamesToKill[i];

    r3dOutToLog("deleting game %s\n", game->GetName());
    DeleteGame(game);
  }
}

bool CMasterGameServer::IsServerStartingUp()
{
  if(gServerConfig->serverId_ < MASTERSERVER_DEV_ID) // only on real server
    if(supers_.size() < gServerConfig->minSupersToStartGame_) // do not spawn any games until we have a minimum number of supervisors connected (helps with patch process)
      return true;
      
  return false;
}

CServerS* CMasterGameServer::GetLeastUsedServer(EGBGameRegion region, ESBServerType serverType, int mapId)
{
  const float curTime = r3dGetTime();

  CServerS* selected = NULL;
  int       minGames = 999;
  
  // search for server with maximum available users
  for(TSupersList::iterator it = supers_.begin(); it != supers_.end(); ++it) 
  {
    CServerS* super = it->second;
    if(super->disabled_)
      continue;
  
    // filter our supervisors if region is specified
	bool haveSupers = false;
	switch(region)
	{
	case GBNET_REGION_US_West:
		haveSupers = true;
		break;
	case GBNET_REGION_US_East:
		haveSupers = true;
		break;
	case GBNET_REGION_Europe:
		haveSupers = true;
		break;
	case GBNET_REGION_Russia:
		haveSupers = true;
		break;
	default:
		haveSupers = false;
	}
    // check if we have supervisors with correct region, if not - skip it

    // silently continue, no need to spam log about that
    if(!haveSupers) {
      continue;
    }
    // filter our supervisors if region is specified
    //if(region != GBNET_REGION_Unknown && super->region_ != region)
    //  continue;
	// DESACTIVADO MAMON
    //if(!(serverType & super->serverType_))
    //  continue;
   // if(super->mapId_ != 0xFF && super->mapId_ != mapId)
   //   continue;
      
    int expectedGames = super->GetExpectedGames();
    if(expectedGames >= super->maxGames_)
      continue;
      
    if(curTime < super->nextGameStart_)
      continue;
      
    if(expectedGames < minGames) {
      selected = super;
      minGames = expectedGames;
    }
  }
  
  return selected;
}


CServerS* CMasterGameServer::GetServerByGameId(DWORD gameId)
{
  DWORD sid = gameId >> 16;
    
  for(TSupersList::iterator it = supers_.begin(); it != supers_.end(); ++it) {
    CServerS* super = it->second;
    if(sid == super->id_)
      return super;
  }
  
  return NULL;
}

CServerG* CMasterGameServer::GetGameByGameServerId(DWORD gameServerId)
{
  TGamesList::iterator it = gamesByGameServerId_.find(gameServerId);
  if(it == gamesByGameServerId_.end())
    return NULL;

  return it->second;
}

CServerG* CMasterGameServer::GetGameBySessionId(__int64 sessionId)
{
  for(TGamesList::iterator it = games_.begin(); it != games_.end(); ++it) 
  {
    CServerG* game = it->second;
    if(sessionId == game->info_.sessionId)
      return game;
  }
  
  return NULL;
}

CServerG* CMasterGameServer::GetQuickJoinGame(int gameMap, EGBGameRegion region, int BrowseChannel, int timePlayed)
{
#ifdef _DEBUG
	r3dOutToLog("GetQuickJoinGame: %d region:%d, channel:%d\n", gameMap, region, BrowseChannel); CLOG_INDENT;
#endif  
  
  // find less populated game
  int       foundMin  = 999;
  CServerG* foundGame = NULL;
  
  for(TGamesList::iterator it = games_.begin(); it != games_.end(); ++it) 
  {
    CServerG* game = it->second;
    if(!game->canJoinGame())
      continue;
    if(game->isPassworded())
      continue;
      
    const GBGameInfo& gi = game->getGameInfo();

    // filter out region
    if(region != GBNET_REGION_Unknown && gi.region != region) {
      continue;
    }
      
    // filter out specified map/mode (0xFF mean any game/mode)
    if(gameMap < 0xFF && gameMap != gi.mapId)
      continue;
      
    // only gameworlds
    if(!gi.IsGameworld())
      continue;

    // filter out 100 player maps for now (sergey's request)
    //if(gi.maxPlayers == 100)
    //  continue;

	if(gi.gameTimeLimit > timePlayed)
		continue;

	if(!gi.isSameChannel(BrowseChannel))
		continue;

	/*if(BrowseChannel==2) // OFFICIAL SERVERS ONLY
		if(gi.IsRentedGame())
			continue;

	if(BrowseChannel==3) // PRIVATE SERVERS ONLY
		if(!gi.IsRentedGame())
			continue;*/
      
    int numPlayers = game->curPlayers_ + game->GetJoiningPlayers();
    if(numPlayers >= gi.maxPlayers)
      continue;
      
    if(numPlayers < foundMin) {
      foundMin  = numPlayers;
      foundGame = game;
    }
  }
  
  CServerG* game = foundGame;
  if(game == NULL) {
#ifdef _DEBUG  
    r3dOutToLog("no free games\n");
#endif    
    return NULL;
  }
  
#ifdef _DEBUG  
  r3dOutToLog("%s, %d(+%d) of %d players\n", 
    game->GetName(), 
    game->curPlayers_, 
    game->GetJoiningPlayers(), 
    game->getGameInfo().maxPlayers);
#endif    
    
  return game;
}

bool CMasterGameServer::CreateNewGame(const CMSNewGameData& ngd, DWORD* out_ip, DWORD* out_port, __int64* out_sessionId)
{
  // detect what server type we need to spawn game
  ESBServerType serverType = ngd.ginfo.IsGameworld() ? SBNET_SERVER_Gameworld : SBNET_SERVER_Stronghold;

  CServerS* super = GetLeastUsedServer((EGBGameRegion)ngd.ginfo.region, serverType, ngd.ginfo.mapId);
  if(super == NULL)
  {
    //r3dOutToLog("there is no free game servers at region:%d\n", ngd.ginfo.region);
    return false;
  }

  //r3dOutToLog("gameServerId %d - spawning new\n", ngd.ginfo.gameServerId); CLOG_INDENT;
  
  CREATE_PACKET(SBPKT_M2S_StartGameReq, n);
  if(super->RegisterNewGameSlot(ngd, n) == false)
  {
    r3dOutToLog("request for new game failed at %s\n", super->GetName());
    return false;
  }

#if 1
  //r3dOutToLog("request send to %s, creator:%d, id:0x%x, port:%d\n", super->GetName(), ngd.CustomerID, n.gameId, n.port);
  net_->SendToPeer(&n, sizeof(n), super->peer_, true);
#else
  char strginfo[256];
  ginfo.ToString(strginfo);

  char cmd[512];
  sprintf(cmd, "\"%u %u %u\" \"%s\"", n.gameId, n.port, ngd.CustomerID, strginfo);
  const char* exe = "WZ_GameServer.exe";
  int err;
  if(err = (int)ShellExecute(NULL, "open", exe, cmd, "", SW_SHOW) < 32) {
    r3dOutToLog("!!! unable to run %s: %d\n", exe, err);
  }
#endif
  
  *out_ip        = super->ip_;
  *out_port      = n.port;
  *out_sessionId = n.sessionId;

  return true;
}

#if ENABLED_SERVER_WEAPONARMORY
DWORD __stdcall CMasterGameServer::ItemsDbUpdateThread(LPVOID in)
{
  CMasterGameServer* This = (CMasterGameServer*)in;

  MSWeaponArmory* wa   = NULL;
  CkHttpResponse* resp = NULL;

  try
  {
    r3dOutToLog("ItemsDBUpdateThread: started\n");

    CkHttp http;
    int success = http.UnlockComponent("ARKTOSHttp_decCLPWFQXmU");
    if(success != 1) 
      r3dError("Internal error");

    CkHttpRequest req;
    req.UsePost();
    req.put_Path("/php/api_getItemsDB.php");
    req.AddParam("serverkey", "CfFkqQWjfgksYG56893GDhjfjZ20");

    resp = http.SynchronousRequest(g_api_ip->GetString(), gDomainPort, gDomainUseSSL, req);
    if(!resp)
      throw "no response";
      
    wa = new MSWeaponArmory();
    char* data = (char*)resp->bodyStr();
    if(!wa->loadItemsDB(data, strlen(data))) 
      throw "failed to load itemsdb";
    
    r3dOutToLog("ItemsDBUpdateThread: updated, %d weapons\n", wa->m_NumWeaponsLoaded);
    
    delete resp;

    This->newWeaponArmory_   = wa;
    This->itemsDbUpdateFlag_ = ITEMSDBUPDATE_Ok;
    return This->itemsDbUpdateFlag_;
  }
  catch(const char* msg)
  {
    r3dOutToLog("!!!! ItemsDBUpdateThread failed: %s\n", msg);
  }
  
  SAFE_DELETE(wa);
  SAFE_DELETE(resp);

  This->itemsDbUpdateFlag_ = ITEMSDBUPDATE_Error;
  return This->itemsDbUpdateFlag_;
}

void CMasterGameServer::StartItemsDbUpdate(bool forced)
{
  if(itemsDbUpdateFlag_ != ITEMSDBUPDATE_Unactive) 
  {
    r3dOutToLog("items db update already in process\n");
    return;
  }
  
  itemsDbUpdateFlag_   = ITEMSDBUPDATE_Processing;
  itemsDbUpdateForced_ = forced;

  // create items update thread thread
  itemsDbUpdateThread_ = CreateThread(NULL, 0, &ItemsDbUpdateThread, this, 0, NULL);
  itemsDbLastUpdate_   = r3dGetTime();
  
  return;
}

void CMasterGameServer::DoFirstItemsDbUpdate()
{
  // minor hack: if we're running in local test mode, skip items updating
  extern int gDomainPort;
  if(gDomainPort == 55016 || stricmp(g_api_ip->GetString(), "localhost") == 0)
  {
    itemsDbUpdateFlag_ = ITEMSDBUPDATE_Processing; // put it in permanent wait state
    return;
  }

  r3dOutToLog("reading new items db\n");
  StartItemsDbUpdate(false);
  DWORD rr = ::WaitForSingleObject(itemsDbUpdateThread_, 30000);
  if(rr != WAIT_OBJECT_0) {
    r3dError("failed to download items db - timeout\n");
    return;
  }
  if(itemsDbUpdateFlag_ != ITEMSDBUPDATE_Ok) {
    r3dError("failed to download items db - error\n");
    return;
  }

  // swap current weapon armory with new one
  r3d_assert(gMSWeaponArmory == NULL);
  gMSWeaponArmory    = newWeaponArmory_;
  newWeaponArmory_   = NULL;
  itemsDbUpdateFlag_ = ITEMSDBUPDATE_Unactive;
  return;
}

void CMasterGameServer::UpdateItemsDb()
{
  if(itemsDbUpdateFlag_ == ITEMSDBUPDATE_Processing)
    return;
    
  if(itemsDbUpdateFlag_ == ITEMSDBUPDATE_Unactive)
  {
    // do update every 10 min
    if(r3dGetTime() > itemsDbLastUpdate_ + 600.0f)
    {
      r3dOutToLog("Starting periodic itemsdb update\n");
      StartItemsDbUpdate(false);
    }
    return;
  }
    
  if(itemsDbUpdateFlag_ == ITEMSDBUPDATE_Error)
  {
    r3dOutToLog("failed to get items db update\n");
    itemsDbUpdateFlag_ = ITEMSDBUPDATE_Unactive;
    return;
  }
  
  r3d_assert(itemsDbUpdateFlag_ == ITEMSDBUPDATE_Ok);
  r3dOutToLog("got new weapon info, sending to %d games\n", games_.size());

  // swap current weapon armory with new one
  SAFE_DELETE(gMSWeaponArmory);
  r3d_assert(gMSWeaponArmory == NULL);
  gMSWeaponArmory    = newWeaponArmory_;
  newWeaponArmory_   = NULL;
  itemsDbUpdateFlag_ = ITEMSDBUPDATE_Unactive;
  
  // send new data to games
  if(itemsDbUpdateForced_)
  {
    for(TGamesList::iterator it = games_.begin(); it != games_.end(); ++it) 
    {
      const CServerG* game = it->second;
      SendArmoryInfoToGame(game);
    }
  }
  
  return;
}

void CMasterGameServer::SendArmoryInfoToGame(const CServerG* game)
{
  if(gMSWeaponArmory == NULL) {
    r3dOutToLog("gMSWeaponArmory isn't loaded\n");
    return;
  }

  r3d_assert(gMSWeaponArmory);
  
  // send all weapons to game server
  for(uint32_t i=0; i<gMSWeaponArmory->m_NumWeaponsLoaded; i++)
  {
    const WeaponConfig& wc = *gMSWeaponArmory->m_WeaponArray[i];
    
    SBPKT_M2G_UpdateWeaponData_s n;
    n.itemId = wc.m_itemID;
    wc.copyParametersTo(n.wi);
    
    net_->SendToPeer(&n, sizeof(n), game->peer_, true);
  }

  // send all gears to game server
  for(uint32_t i=0; i<gMSWeaponArmory->m_NumGearLoaded; i++)
  {
    const GearConfig& gc = *gMSWeaponArmory->m_GearArray[i];
    
    SBPKT_M2G_UpdateGearData_s n;
    n.itemId = gc.m_itemID;
    gc.copyParametersTo(n.gi);
    
    net_->SendToPeer(&n, sizeof(n), game->peer_, true);
  }
  
  // send lootboxes to game server
  for(uint32_t i=0; i<gMSWeaponArmory->m_NumItemLoaded; i++)
  {
    const ItemConfig& ic = *gMSWeaponArmory->m_ItemArray[i];
    if(ic.category != storecat_LootBox)
	continue;
    
    SBPKT_M2G_UpdateItemData_s n;
    n.itemId = ic.m_itemID;
    n.LevelRequired = ic.m_LevelRequired;
    
    net_->SendToPeer(&n, sizeof(n), game->peer_, true);
  }

  // send end update event
  SBPKT_M2G_UpdateDataEnd_s n;
  net_->SendToPeer(&n, sizeof(n), game->peer_, true);
  
  return;
}
#endif