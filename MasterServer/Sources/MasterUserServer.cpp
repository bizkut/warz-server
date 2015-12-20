#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"
#include <shellapi.h>

#pragma warning(disable: 4065)	// switch statement contains 'default' but no 'case' labels

#include "MasterUserServer.h"
#include "MasterGameServer.h"
#include "NetPacketsServerBrowser.h"

using namespace NetPacketsGameBrowser;

static	r3dNetwork	clientNet;
	CMasterUserServer gMasterUserServer;

static bool IsNullTerminated(const char* data, int size)
{
  for(int i=0; i<size; i++) {
    if(data[i] == 0)
      return true;
  }

  return false;
}

CMasterUserServer::CMasterUserServer()
{
  return;
}

CMasterUserServer::~CMasterUserServer()
{
  SAFE_DELETE_ARRAY(peers_);
}

void CMasterUserServer::Start(int port, int in_maxPeerCount)
{
  r3d_assert(in_maxPeerCount);
  MAX_PEERS_COUNT = in_maxPeerCount;
  peers_          = new peer_s[MAX_PEERS_COUNT];

  numConnectedPeers_ = 0;
  maxConnectedPeers_ = 0;
  curPeerUniqueId_   = 0;

  clientNet.Initialize(this, "clientNet");
  if(!clientNet.CreateHost(port, MAX_PEERS_COUNT)) {
    r3dError("CreateHost failed\n");
  }
  
  r3dOutToLog("MasterUserServer started at port %d, %d CCU, need %d servers to start\n", port, MAX_PEERS_COUNT, gServerConfig->minSupersToStartGame_);
  
  return;
}

static bool SupervisorsSortByName(const CServerS* d1, const CServerS* d2)
{
  return strcmp(d1->GetName(), d2->GetName()) < 0;
}

void CMasterUserServer::PrintStats()
{
  const float curTime = r3dGetTime();
  
  static float nextShutLog_ = 0;
  if(gMasterGameServer.shuttingDown_ && curTime > nextShutLog_) {
    nextShutLog_ = curTime + 1.0f;
    r3dOutToLog("SHUTDOWN in %.0f\n", gMasterGameServer.shutdownLeft_);
  }
  
  // dump some useful statistics
  static float nextDebugLog_ = 0;
  static int   outToLogCnt   = 0;
  if(curTime < nextDebugLog_) 
    return;

  nextDebugLog_ = curTime + 10.0f;
  bool bToLog = (++outToLogCnt % 12) == 0; // output to r3dLog every 120 sec

  // calc number of games and CCU
  int numGames     = 0;
  int numGamesH[4] = {0}; // hosted games per region
  int numGamesR[4] = {0}; // renged games per region
  int numHoldsR[4] = {0}; // strongholds per region
  int numCCU       = 0;
  int maxCCU       = 0;
  for(CMasterGameServer::TGamesList::const_iterator it = gMasterGameServer.games_.begin(); 
      it != gMasterGameServer.games_.end();
      ++it)
  {
    const CServerG* game = it->second;
    const GBGameInfo& ginfo = game->getGameInfo();

    numCCU += game->curPlayers_;
    maxCCU += ginfo.maxPlayers;
    numGames++;
    
    int regIdx = 0;
    if(ginfo.region == GBNET_REGION_US_West) regIdx = 0;
    else if(ginfo.region == GBNET_REGION_Europe) regIdx = 1;
    else if(ginfo.region == GBNET_REGION_Russia) regIdx = 2;
	else if(ginfo.region == GBNET_REGION_SouthAmerica) regIdx = 3;
    
    if(!game->getGameInfo().IsRentedGame())
      numGamesH[regIdx]++;
    else if(game->getGameInfo().IsGameworld())
      numGamesR[regIdx]++;
    else
      numHoldsR[regIdx]++;
  }

  // find games capacity per region
  int maxGames[4] = {0};
  int maxHolds[4] = {0};
  for(CMasterGameServer::TSupersList::const_iterator it = gMasterGameServer.supers_.begin(); it != gMasterGameServer.supers_.end(); ++it) 
  {
    const CServerS* super = it->second;

    int regIdx = 0;
    if(super->region_ == GBNET_REGION_US_West) regIdx = 0;
    else if(super->region_ == GBNET_REGION_Europe) regIdx = 1;
    else if(super->region_ == GBNET_REGION_Russia) regIdx = 2;
	else if(super->region_ == GBNET_REGION_SouthAmerica) regIdx = 3;
    
    if(super->IsStrongholdServer())
      maxHolds[regIdx] += super->maxGames_;
    else
      maxGames[regIdx] += super->maxGames_;
  }

    
  static int peakCCU = 0;
  if(numCCU > peakCCU) peakCCU = numCCU;

  FILE* f = fopen("MasterServer_ccu.txt", "wt");

  char buf[1024];    
  sprintf(buf, "MSINFO: %d (%d max) peers, %d CCU in %d games, PeakCCU: %d, MaxCCU:%d\n",
    numConnectedPeers_,
    maxConnectedPeers_,
    numCCU,
    numGames,
    peakCCU,
    maxCCU
    ); 
  if(bToLog) r3dOutToLog(buf);
  if(f) fprintf(f, buf);

  // list of spawned/required games per region
  for(int i=0; i<4; i++)
  {
    const static char* regName[4] = { "US", "EU", "RU", "SA" };
    sprintf(buf, " [%s] Hosted:%d/%d\tRented:%d/%d\tCapacity:%d\tBalance:%+d\n",
      regName[i],
      numGamesH[i], gServerConfig->numGamesHosted[i],
      gServerConfig->rentGames_.size(), gServerConfig->numGamesRented[i],
      maxGames[i], 
      maxGames[i] - (gServerConfig->numGamesHosted[i] + gServerConfig->numGamesRented[i])
    );
    if(bToLog) r3dOutToLog(buf);
    if(f) fprintf(f, buf);
  }

  // list of spawned/required games per region
  for(int i=0; i<4; i++)
  {
    const static char* regName[4] = { "US", "EU", "RU", "SA" };
    sprintf(buf, " [%s] Strongholds\tRented:%d/%d\tCapacity:%d\tBalance:%+d\n",
      regName[i],
      numHoldsR[i], gServerConfig->numStrongholdsRented[i],
      maxHolds[i], 
      maxHolds[i] - (gServerConfig->numStrongholdsRented[i])
    );
    if(bToLog) r3dOutToLog(buf);
    if(f) fprintf(f, buf);
  }

  // list of supervisors
  sprintf(buf, "Supervisors: %d%s, Games:%d|%d\n", 
    gMasterGameServer.supers_.size(),
    gMasterGameServer.supers_.size() < gServerConfig->minSupersToStartGame_ ? "(need more to start)" : "",
    gMasterGameServer.games_.size(), 
    gMasterGameServer.gamesByGameServerId_.size());
  r3dOutToLog(buf);
  if(f) fprintf(f, buf);
  CLOG_INDENT;
  
  static std::vector<const CServerS*> supers;
  supers.clear();
  for(CMasterGameServer::TSupersList::const_iterator it = gMasterGameServer.supers_.begin(); it != gMasterGameServer.supers_.end(); ++it)
    supers.push_back(it->second);
    
  std::sort(supers.begin(), supers.end(), SupervisorsSortByName);
  
  // log supervisors status by region
  int regionsToLog[4] = {GBNET_REGION_US_West, GBNET_REGION_Europe, GBNET_REGION_Russia, GBNET_REGION_SouthAmerica};
  const char* regionsName[4] = {"US", "EU", "RU", "SA"};
  for(int curRegion=0; curRegion<4; ++curRegion)
  {
	  sprintf(buf, "REGION: %s\n", regionsName[curRegion]);
	  if(bToLog) r3dOutToLog(buf);
	  if(f) fprintf(f, buf);

	  for(size_t i=0; i<supers.size(); ++i)
	  {
		  const CServerS* super = supers[i];
		  if(super->region_ == regionsToLog[curRegion])
		  {
			  sprintf(buf, "%s(%s), games:%d/%d, players:%d/%d %s %s\n", 
				  super->GetName(),
				  inet_ntoa(*(in_addr*)&super->ip_),
				  super->GetExpectedGames(), super->maxGames_,
				  super->GetExpectedPlayers(), super->maxPlayers_,
				  super->IsStrongholdServer() ? "(STRONGHOLD)" : "",
				  super->disabledSlots_ > 0 ? "(HAVE DISABLED SLOTS)" : "");

			  if(bToLog) r3dOutToLog(buf);
			  if(f) fprintf(f, buf);
		  }
	  }
  }

  if(f) fclose(f);
  
  return;
}

void CMasterUserServer::Temp_Debug1()
{
  r3dOutToLog("Supervisors_Debug:\n");

  // log supervisors status
  for(CMasterGameServer::TSupersList::const_iterator it = gMasterGameServer.supers_.begin();
      it != gMasterGameServer.supers_.end();
      ++it)
  {
    const CServerS* super = it->second;
    r3dOutToLog("%s, games:%d/%d, players:%d/%d\n", 
      super->GetName(),
      super->GetExpectedGames(), super->maxGames_,
      super->GetExpectedPlayers(), super->maxPlayers_);
    CLOG_INDENT;

    int users = 0;
    for(int i=0; i<super->maxGames_; i++) 
    {
      const CServerG* game = super->games_[i].game;
      if(!game) 
        continue;

      r3dOutToLog("game %s, %d/%d players, %d joiners, name:%s, ip:%s\n", 
        game->GetName(), 
        game->curPlayers_, game->getGameInfo().maxPlayers, game->GetJoiningPlayers(), 
        game->getGameInfo().name,
        inet_ntoa(*(in_addr*)&game->ip_)
        );
    }
    
  }
  
  return;
}

void CMasterUserServer::Tick()
{
  const float curTime = r3dGetTime();
  
  net_->Update();
  
  DisconnectIdlePeers();
  
  PrintStats();

  return;
}

void CMasterUserServer::Stop()
{
  if(net_)
    net_->Deinitialize();
}

void CMasterUserServer::DisconnectIdlePeers()
{
  const float IDLE_PEERS_CHECK = 0.2f;	// do checks every N seconds
  const float VALIDATE_DELAY   = 5.0f;	// peer have this N seconds to validate itself

  static float nextCheck = -1;
  const float curTime = r3dGetTime();
  if(curTime < nextCheck)
    return;
  nextCheck = curTime + IDLE_PEERS_CHECK;
  
  for(int i=0; i<MAX_PEERS_COUNT; i++) 
  {
    if(peers_[i].status == PEER_Connected && curTime > peers_[i].connectTime + VALIDATE_DELAY) {
      DisconnectCheatPeer(i, "validation time expired");
      continue;
    }
  }
  
  return;
}

void CMasterUserServer::DisconnectCheatPeer(DWORD peerId, const char* message, ...)
{
	char buf[2048] = {0};

  if(message)
  {
	  va_list ap;
	  va_start(ap, message);
	  StringCbVPrintfA(buf, sizeof(buf), message, ap);
	  va_end(ap);
  }

  DWORD ip = net_->GetPeerIp(peerId);
  if(message)
  {
    r3dOutToLog("!!! cheat: peer%d[%s], reason: %s\n", 
      peerId, 
      inet_ntoa(*(in_addr*)&ip),
      buf);
  }

  net_->DisconnectPeer(peerId);
  
  // fire up disconnect event manually, enet might skip if if other peer disconnect as well
  OnNetPeerDisconnected(peerId);
}

bool CMasterUserServer::DisconnectIfShutdown(DWORD peerId)
{
  if(!gMasterGameServer.shuttingDown_)
    return false;

  GBPKT_M2C_ShutdownNote_s n;
  n.reason = 0;
  net_->SendToPeer(&n, sizeof(n), peerId);
  
  DisconnectCheatPeer(peerId, NULL);
  return true;
}

bool CMasterUserServer::Validate(const GBPKT_C2M_JoinGameReq_s& n)
{
  if(!IsNullTerminated(n.pwd, sizeof(n.pwd)))
    return false;

  return true;    
}

bool CMasterUserServer::Validate(const GBPKT_C2M_MyServerSetParams_s& n)
{
  if(!IsNullTerminated(n.pwd, sizeof(n.pwd)))
    return false;
  if(n.gameTimeLimit < 0)
	  return false;

  return true;    
}


void CMasterUserServer::OnNetPeerConnected(DWORD peerId)
{
  peer_s& peer = peers_[peerId];
  r3d_assert(peer.status == PEER_Free);
  
  curPeerUniqueId_++;
  peer.peerUniqueId = (peerId << 16) | (curPeerUniqueId_ & 0xFFFF);
  peer.status       = PEER_Connected;
  peer.connectTime  = r3dGetTime();
  peer.lastReqTime  = r3dGetTime() - 1.0f; // minor hack to avoid check for 'too many requests'
  
  // send validate packet, so client can check version right now
  GBPKT_ValidateConnectingPeer_s n;
  n.version = GBNET_VERSION;
  n.key1    = 0;
  net_->SendToPeer(&n, sizeof(n), peerId, true);
  
  numConnectedPeers_++;
  maxConnectedPeers_ = R3D_MAX(maxConnectedPeers_, numConnectedPeers_);
  
  return;
}

void CMasterUserServer::OnNetPeerDisconnected(DWORD peerId)
{
  peer_s& peer = peers_[peerId];
  
  if(peer.status != PEER_Free)
    numConnectedPeers_--;
  
  //r3dOutToLog("master: client disconnected\n");
  peer.status       = PEER_Free;
  peer.peerUniqueId = 0;
  return;
}

bool CMasterUserServer::DoValidatePeer(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize)
{
  peer_s& peer = peers_[peerId];
  
  if(peer.status >= PEER_Validated)
    return true;

  // we still can receive packets after peer was force disconnected
  if(peer.status == PEER_Free)
    return false;

  r3d_assert(peer.status == PEER_Connected);
  if(PacketData->EventID != GBPKT_ValidateConnectingPeer) {
    DisconnectCheatPeer(peerId, "wrong validate packet id");
    return false;
  }

  const GBPKT_ValidateConnectingPeer_s& n = *(GBPKT_ValidateConnectingPeer_s*)PacketData;
  if(sizeof(n) != PacketSize) {
    DisconnectCheatPeer(peerId, "wrong validate packet size");
    return false;
  }
  if(n.version != GBNET_VERSION) {
    DisconnectCheatPeer(peerId, "wrong validate version");
    return false;
  }
  if(n.key1 != GBNET_KEY1) {
    DisconnectCheatPeer(peerId, "wrong client validation key");
    return false;
  }
  
  // send server info to client
  GBPKT_M2C_MasterInfo_s n1;
  n1.serverId = BYTE(gMasterGameServer.masterServerId_);
  net_->SendToPeer(&n1, sizeof(n1), peerId, true);
  
  // set peer to validated
  peer.status = PEER_Validated;
  return false;
}

#define DEFINE_PACKET_HANDLER_MUS(xxx) \
    case xxx: \
    { \
      const xxx##_s& n = *(xxx##_s*)PacketData; \
      if(sizeof(n) != PacketSize) { \
        DisconnectCheatPeer(peerId, "wrong %s size %d vs %d", #xxx, sizeof(n), PacketSize); \
        break; \
      } \
      if(!Validate(n)) { \
        DisconnectCheatPeer(peerId, "invalid %s", #xxx); \
        break; \
      } \
      On##xxx(peerId, n); \
      break; \
    }

void CMasterUserServer::OnNetData(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize)
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
    
  if(DisconnectIfShutdown(peerId))
    return;

  peer_s& peer = peers_[peerId];
  r3d_assert(peer.status >= PEER_Validated);
  
  const float curTime = r3dGetTime();
  
  // check for valid request overflows
  if(curTime < peer.lastReqTime + 0.2f) {
    DisconnectCheatPeer(peerId, "too many requests per sec");
    return;
  }
  peer.lastReqTime = curTime;
      
  switch(PacketData->EventID) 
  {
    default:
      DisconnectCheatPeer(peerId, "invalid packet id");
      return;

    DEFINE_PACKET_HANDLER_MUS(GBPKT_C2M_RefreshList)
    DEFINE_PACKET_HANDLER_MUS(GBPKT_C2M_JoinGameReq);
    DEFINE_PACKET_HANDLER_MUS(GBPKT_C2M_QuickGameReq);
    DEFINE_PACKET_HANDLER_MUS(GBPKT_C2M_MyServerInfoReq);
    DEFINE_PACKET_HANDLER_MUS(GBPKT_C2M_MyServerKickPlayer);
    DEFINE_PACKET_HANDLER_MUS(GBPKT_C2M_MyServerSetParams);
  }
  
  return;
}

bool CMasterUserServer::IsGameFiltered(const GBPKT_C2M_RefreshList_s& n, const GBGameInfo& ginfo, const char* pwd, int curPlayers)
{
	if(!ginfo.isSameChannel(n.browseChannel))
		return true;

	if(n.enable_options)
	{
		if(!((n.tracers2 && (ginfo.flags & GBGameInfo::SFLAGS_Tracers)) ||
			(!n.tracers2 && !(ginfo.flags & GBGameInfo::SFLAGS_Tracers))))
			return true;

		if(!((n.crosshair2 && (ginfo.flags & GBGameInfo::SFLAGS_CrossHair)) ||
			(!n.crosshair2 && !(ginfo.flags & GBGameInfo::SFLAGS_CrossHair))))
			return true;

		if(!((n.nameplates2 && (ginfo.flags & GBGameInfo::SFLAGS_Nameplates)) ||
			(!n.nameplates2 && !(ginfo.flags & GBGameInfo::SFLAGS_Nameplates))))
			return true;

		if(!((n.password && (pwd[0])) ||
			(!n.password && !(pwd[0]))))
			return true;
		if(ginfo.gameTimeLimit > n.timeLimit)
			return true;
	}
	
	if(n.hideempty && curPlayers == 0)
		return true;

	if(n.hidefull && curPlayers >= ginfo.maxPlayers)
		return true;
		
	return false;
}

void CMasterUserServer::OnGBPKT_C2M_RefreshList(DWORD peerId, const GBPKT_C2M_RefreshList_s& n)
{
  //r3dOutToLog("sending session list to client%d\n", peerId);

  { // start list
    CREATE_PACKET(GBPKT_M2C_StartGamesList, n);
    net_->SendToPeer(&n, sizeof(n), peerId);
  }
  
  // send supervisors data
  for(CMasterGameServer::TSupersList::iterator it = gMasterGameServer.supers_.begin(); it != gMasterGameServer.supers_.end(); ++it)
  {
    const CServerS* super = it->second;
    
    GBPKT_M2C_SupervisorData_s n;
    n.ID     = WORD(super->id_);
    n.ip     = super->ip_;
    n.region = super->region_;
    net_->SendToPeer(&n, sizeof(n), peerId);
  }
  
  DWORD numFiltered = 0;

  // send games
  for(CMasterGameServer::TGamesList::iterator it = gMasterGameServer.games_.begin(); it != gMasterGameServer.games_.end(); ++it) 
  {
    const CServerG* game = it->second;
    if(game->isValid() == false)
      continue;
      
    if(game->isFinished())
      continue;
      
    int curPlayers = game->curPlayers_ + game->GetJoiningPlayers();
      
    // filter our region and games based on filters
    if(n.region != game->info_.ginfo.region) {
      continue;
    }
    if(IsGameFiltered(n, game->info_.ginfo, game->info_.pwd, curPlayers)) {
      numFiltered++;
      continue;
    }
      
    CREATE_PACKET(GBPKT_M2C_GameData, n);
    n.superId    = (game->id_ >> 16);
    n.info       = game->info_.ginfo;
    n.status     = 0;
    n.curPlayers = (WORD)curPlayers;
    // override passworded flag, as it can be changed right now for every type of servers
    n.info.flags &= ~GBGameInfo::SFLAGS_Passworded;
    if(game->info_.pwd[0])
      n.info.flags |= GBGameInfo::SFLAGS_Passworded;

    net_->SendToPeer(&n, sizeof(n), peerId);
  }
  
  // send not started rented games
  for(size_t i=0; i<gServerConfig->rentGames_.size(); i++)
  {
    const CMasterServerConfig::rentGame_s* rg = &gServerConfig->rentGames_[i];
    
    // filter games for dev servers
    if(gMasterGameServer.masterServerId_ == MASTERSERVER_DEV_ID) {
      break;
    }

    if(gMasterGameServer.masterServerId_ == MASTERSERVER_DEV_ID + 1) {
      if(rg->ginfo.gameServerId != 102386 && rg->ginfo.gameServerId != 133613)	// "azzy server", owner 1000003
        continue;
    }

    // skip started games
    if(gMasterGameServer.GetGameByGameServerId(rg->ginfo.gameServerId))
      continue;

    // filter our region and games based on filters
    if(n.region != rg->ginfo.region) {
      continue;
    }

    if(IsGameFiltered(n, rg->ginfo, rg->pwd, 0)) {
      numFiltered++;
      continue;
    }

    CREATE_PACKET(GBPKT_M2C_GameData, n);
    n.superId    = 0;
    n.info       = rg->ginfo;
    n.status     = 0;
    n.curPlayers = 0;
    // override passworded flag, as it can be changed right now for every type of servers
    n.info.flags &= ~GBGameInfo::SFLAGS_Passworded;
    if(rg->pwd[0])
      n.info.flags |= GBGameInfo::SFLAGS_Passworded;

    net_->SendToPeer(&n, sizeof(n), peerId);
  }

  { // end list
    CREATE_PACKET(GBPKT_M2C_EndGamesList, n);
    n.numFiltered = numFiltered;
    net_->SendToPeer(&n, sizeof(n), peerId);
  }
  
  
  return;
}

void CMasterUserServer::OnGBPKT_C2M_JoinGameReq(DWORD peerId, const GBPKT_C2M_JoinGameReq_s& n)
{
  GBPKT_M2C_JoinGameAns_s ans;
  do 
  {
    if(gMasterGameServer.IsServerStartingUp())
    {
      ans.result = GBPKT_M2C_JoinGameAns_s::rMasterStarting;
      break;
    }

    CServerG* game = NULL;
    int gameStatus = gMasterGameServer.IsGameServerIdStarted(n.gameServerId, &game);
    if(game) {
      DoJoinGame(game, n.CustomerID, n.pwd, ans);
      break;
    }
    
    // check if this is rented game
    const CMasterServerConfig::rentGame_s* rg = gServerConfig->GetRentedGameInfo(n.gameServerId);
    if(rg == 0) {
      ans.result = GBPKT_M2C_JoinGameAns_s::rGameNotFound;
      break;
    }

    // check if user can enter passworded game to prevent game starting for user with invalid password
    // do not check password for GM, we allow GMs to enter any game
    bool isAdmin = IsAdmin(n.CustomerID);
    if(rg->pwd[0] && !isAdmin) {
      if(strcmp(rg->pwd, n.pwd) != 0) {
        ans.result = GBPKT_M2C_JoinGameAns_s::rWrongPassword;
        break;
      }
    }
    
    // check if we that game in starting status
    if(gameStatus == 2) {
      ans.result = GBPKT_M2C_JoinGameAns_s::rGameStarting;
      break;
    }

    // spawn new game by user request
    CMSNewGameData ngd(rg->ginfo, rg->pwd, rg->OwnerCustomerID);

    DWORD ip;
    DWORD port;
    __int64 sessionId;
    if(!gMasterGameServer.CreateNewGame(ngd, &ip, &port, &sessionId)) {
      r3dOutToLog("!!! unable to spawn user requested map %d - no free slots at regioin %d\n", ngd.ginfo.mapId, ngd.ginfo.region);
      ans.result = GBPKT_M2C_JoinGameAns_s::rNoGames;
      break;
    }

    ans.result = GBPKT_M2C_JoinGameAns_s::rGameStarting;
    break;
  } while (0);

  r3d_assert(ans.result != GBPKT_M2C_JoinGameAns_s::rUnknown);
  net_->SendToPeer(&ans, sizeof(ans), peerId, true);
  return;
}

void CMasterUserServer::OnGBPKT_C2M_QuickGameReq(DWORD peerId, const GBPKT_C2M_QuickGameReq_s& n)
{
  GBPKT_M2C_JoinGameAns_s ans;

  if(gMasterGameServer.IsServerStartingUp())
  {
    ans.result = GBPKT_M2C_JoinGameAns_s::rMasterStarting;
    net_->SendToPeer(&ans, sizeof(ans), peerId, true);
    return;
  }
  
  CServerG* game = gMasterGameServer.GetQuickJoinGame(n.gameMap, (EGBGameRegion)n.region, n.browseChannel, n.playerGameTime);
  // in case some region game wasn't available, repeat search without specifying filter
  if(game == NULL && n.region != GBNET_REGION_Unknown)
  {
    CLOG_INDENT;
    game = gMasterGameServer.GetQuickJoinGame(n.gameMap, GBNET_REGION_Unknown, n.browseChannel, n.playerGameTime);
  }
  
  if(!game) {
    ans.result = GBPKT_M2C_JoinGameAns_s::rGameNotFound;
    net_->SendToPeer(&ans, sizeof(ans), peerId, true);
    return;
  }

  game->AddJoiningPlayer(n.CustomerID);
  
  ans.result    = GBPKT_M2C_JoinGameAns_s::rOk;
  ans.ip        = game->ip_;
  ans.port      = game->info_.port;
  ans.sessionId = game->info_.sessionId;
  net_->SendToPeer(&ans, sizeof(ans), peerId, true);
  return;
}

// sync with ServerGameLogic::IsAdmin(DWORD CustomerID)
bool CMasterUserServer::IsAdmin(DWORD CustomerID)
{
/*
	//@TH 127.0.0.1
	switch(CustomerID)
	{
		case 1651761:
		case 1651758:
		case 1000015:
		case 1000006:
		case 1241667:
		case 1000003:
		case 1000012:
		case 1000004:
		case 1000014:
		case 1000011:
		case 1288630:
		case 1125258:
		case 1731195:
		case 1320420:
		case 1731117:
		case 1731140:
		case 1182980:
		case 2280615: // askidmore@thewarz.com
		case 3099993: // eric koch
			return true;
	}
*/	
	return false;
}

void CMasterUserServer::DoJoinGame(CServerG* game, DWORD CustomerID, const char* pwd, GBPKT_M2C_JoinGameAns_s& ans)
{
  r3d_assert(game);
  
  if(game->isFull() && !IsAdmin(CustomerID)) {
    ans.result = GBPKT_M2C_JoinGameAns_s::rGameFull;
    return;
  }

  if(game->isFinished()) {
    ans.result = GBPKT_M2C_JoinGameAns_s::rGameFinished;
    return;
  }

  // do not check password for GM, we allow GMs to enter any game
  bool isAdmin = IsAdmin(CustomerID);
  if(game->isPassworded() && !isAdmin) {
    if(strcmp(game->info_.pwd, pwd) != 0) {
      ans.result = GBPKT_M2C_JoinGameAns_s::rWrongPassword;
      return;
    }
  }
  
  game->AddJoiningPlayer(CustomerID);

  ans.result    = GBPKT_M2C_JoinGameAns_s::rOk;
  ans.ip        = game->ip_;
  ans.port      = game->info_.port;
  ans.sessionId = game->info_.sessionId;

  NetPacketsServerBrowser::SBPKT_M2G_SendIsPVE_s n(game->id_); // for PVE maps
  n.isPVE = game->getGameInfo().isPVE;
  gMasterGameServer.net_->SendToPeer(&n, sizeof(n), game->peer_);

  return;
}

void CMasterUserServer::OnGBPKT_C2M_MyServerInfoReq(DWORD peerId, const GBPKT_C2M_MyServerInfoReq_s& n)
{
  // check if this is rented game
  const CMasterServerConfig::rentGame_s* rg = gServerConfig->GetRentedGameInfo(n.gameServerId);
  if(rg == NULL)
  {
    // not setup yet
    CREATE_PACKET(GBPKT_M2C_MyServerInfoAns, n);
    n.status = 1;
    net_->SendToPeer(&n, sizeof(n), peerId);
    return;
  }

  if(rg->AdminKey != n.AdminKey)
  {
    r3dOutToLog("!!!! info bad AdminKey: %d vs %d\n", rg->AdminKey, n.AdminKey);
    CREATE_PACKET(GBPKT_M2C_MyServerInfoAns, n);
    n.status = 0;
    net_->SendToPeer(&n, sizeof(n), peerId);
    return;
  }

  // we can have game as NULL here. it only mean that game isn't spawned. we still need to process this request
  const CServerG* game = gMasterGameServer.GetGameByGameServerId(n.gameServerId);
  if(!game) {
    CREATE_PACKET(GBPKT_M2C_MyServerInfoAns, n);
    n.status = gMasterGameServer.IsServerStartingUp() ? 4 : 2; // starting : offline
    net_->SendToPeer(&n, sizeof(n), peerId);
    return;
  }
  
  // players
  for(int i=0; i<CServerG::MAX_NUM_PLAYERS_MS; i++)
  {
    const SBPKT_G2M_AddPlayer_s& plr = game->playerList_[i].plr;
    if(plr.CharID == 0) 
      continue;

    CREATE_PACKET(GBPKT_M2C_MyServerAddPlayer, n);
    n.CharID     = plr.CharID;
    r3dscpy(n.gamertag, plr.gamertag);
    n.reputation = plr.reputation;
    n.XP         = plr.XP;

    net_->SendToPeer(&n, sizeof(n), peerId);
  }

  // end list 
  {
    CREATE_PACKET(GBPKT_M2C_MyServerInfoAns, n);
    n.status = 3; // online
    net_->SendToPeer(&n, sizeof(n), peerId);
  }

  return;
}

void CMasterUserServer::OnGBPKT_C2M_MyServerKickPlayer(DWORD peerId, const GBPKT_C2M_MyServerKickPlayer_s& n)
{
  const CServerG* game = gMasterGameServer.GetGameByGameServerId(n.gameServerId);
  if(!game)
    return;
  
  if(game->AdminKey_ != n.AdminKey || game->AdminKey_ == 0)
  {
    r3dOutToLog("!!!! kick bad AdminKey: %d vs %d\n", game->AdminKey_, n.AdminKey);
    return;
  }
  
  // pass kick request to game
  NetPacketsServerBrowser::SBPKT_M2G_KickPlayer_s n2(game->id_);
  n2.CharID = n.CharID;
  gMasterGameServer.net_->SendToPeer(&n2, sizeof(n2), game->peer_);
}

void CMasterUserServer::OnGBPKT_C2M_MyServerSetParams(DWORD peerId, const GBPKT_C2M_MyServerSetParams_s& n)
{
  CMasterServerConfig::rentGame_s* rg = gServerConfig->GetRentedGameInfo(n.gameServerId);
  if(rg == NULL)
    return;

  if(rg->AdminKey != n.AdminKey)
  {
    r3dOutToLog("!!!! setpwd AdminKey: %d vs %d\n", rg->AdminKey, n.AdminKey);
    return;
  }
    
  // set password in game info
  strcpy(rg->pwd, n.pwd);
  rg->ginfo.flags = n.flags;
  rg->ginfo.gameTimeLimit = n.gameTimeLimit;

  // and in actual game if it started
  CServerG* game = gMasterGameServer.GetGameByGameServerId(n.gameServerId);
  if(game)
  {
    r3dscpy(game->info_.pwd, n.pwd);
    game->info_.ginfo.flags = n.flags;
    game->info_.ginfo.gameTimeLimit = n.gameTimeLimit;

    // pass request to game
    NetPacketsServerBrowser::SBPKT_M2G_SetGameFlags_s n2(game->id_);
    n2.flags = n.flags;
    n2.gametimeLimit = n.gameTimeLimit;
    gMasterGameServer.net_->SendToPeer(&n2, sizeof(n2), game->peer_);
  }
}

