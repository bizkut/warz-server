#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"

#include "MasterServer.h"
#include "MasterServerConfig.h"
using namespace NetPacketsServerBrowser;

// Eternity compile fix
DWORD DriverUpdater(HWND hParentWnd, DWORD VendorId, DWORD v1, DWORD v2, DWORD v3, DWORD v4, DWORD hash) { return 0; }
void r3dScaleformBeginFrame() {}
void r3dScaleformEndFrame() {}
void SetNewSimpleFogParams() {}
void SetAdvancedFogParams() {}
void SetVolumeFogParams() {}
r3dCamera gCam ;
class r3dSun * Sun;
r3dScreenBuffer * DepthBuffer;
r3dLightSystem WorldLightSystem;
bool g_bEditMode=false;

CServerG::CServerG(DWORD id, DWORD ip, DWORD peer)
{
  id_          = id;
  ip_          = ip;
  peer_        = peer;

  curPlayers_  = 0;
  finished_    = false;
  AdminKey_    = 0;
  
  for(int i=0; i<R3D_ARRAYSIZE(joiners_); i++)
  {
    joiners_[i].CustomerID   = 0;
    joiners_[i].expectedTime = -1;
  }
}

void CServerG::Init(const CMSGameData& in_info)
{
  info_       = in_info;

  curPlayers_ = 0;
  startTime_  = r3dGetTime();

  if(info_.ginfo.IsRentedGame()) {
    const CMasterServerConfig::rentGame_s* rg = gServerConfig->GetRentedGameInfo(info_.ginfo.gameServerId);
    AdminKey_ = rg ? rg->AdminKey : 0;
  }
}

const char* CServerG::GetName() const
{
  static char name[64];
  sprintf(name, "%d, id:0x%x", info_.ginfo.gameServerId, id_);
  return name;
}

void CServerG::AddPlayer(const SBPKT_G2M_AddPlayer_s& n)
{
  r3d_assert(n.playerIdx < MAX_NUM_PLAYERS_MS);
  curPlayers_++;
  
  // add to player list
  memcpy(&playerList_[n.playerIdx].plr, &n, sizeof(n));

  // clear user from joiners
  float curTime = r3dGetTime();
  for(int i=0; i<R3D_ARRAYSIZE(joiners_); i++)
  {
    if(joiners_[i].CustomerID == n.CustomerID)
    {
      joiners_[i].expectedTime = -1;
    }
  }
  
  return;
}

void CServerG::RemovePlayer(int playerIdx)
{
  r3d_assert(playerIdx < MAX_NUM_PLAYERS_MS);

  // remove from player list
  playerList_[playerIdx].plr.CharID = 0;

  curPlayers_--;
  if(curPlayers_ < 0)
  {
	r3dOutToLog("!!!!!!!!!!!!!!! curPlayers_ %d\n", curPlayers_);
	curPlayers_ = 0;
  }
  
  return;
}

void CServerG::AddJoiningPlayer(DWORD CustomerID)
{
  float curTime = r3dGetTime();
  for(int i=0; i<R3D_ARRAYSIZE(joiners_); i++)
  {
    if(curTime >= joiners_[i].expectedTime)
    {
      joiners_[i].CustomerID   = CustomerID;
      joiners_[i].expectedTime = curTime + 60.0f; // give 60 sec to connect?
      break;
    }
  }
  
  return;
}

int CServerG::GetJoiningPlayers() const
{
  int num = 0;
  
  float curTime = r3dGetTime();
  for(int i=0; i<R3D_ARRAYSIZE(joiners_); i++)
  {
    if(curTime < joiners_[i].expectedTime)
    {
      num++;
    }
  }
  
  return num;
}

void CServerS::Init(const SBPKT_S2M_RegisterMachine_s& n)
{
  r3d_assert(games_ == NULL);
  
  // if server specified external ip - replace it
  if(n.externalIpAddr) 
    ip_ = n.externalIpAddr;

  disabled_    = false;
  region_      = n.region;
  serverType_  = (ESBServerType)n.serverType;
  maxPlayers_  = n.maxPlayers;
  maxGames_    = n.maxGames;
  portStart_   = n.portStart;
  serverName_  = n.serverName;
  mapId_       = n.mapId;
  games_       = new games_s[maxGames_];
  for(int i=0; i<maxGames_; i++) {
    games_[i].isDisabled = false;
    games_[i].game       = NULL;
    games_[i].createTime = -99;
    games_[i].closeTime  = -99;
  }

  r3dOutToLog("master: supervisor registered '%s'[%d] ip:%s. %d sessions, type:%d, map:%d\n", 
    GetName(),
    id_, 
    inet_ntoa(*(in_addr*)&ip_),
    maxGames_,
    serverType_,
    mapId_);
    
  return;
}


void CServerS::DisableSupervisor()
{
  r3dOutToLog("master: supervisor %s is shutting down\n", GetName());

  disabled_ = true;

  for(int i=0; i<maxGames_; i++) 
  {
    if(games_[i].game)
      games_[i].game->finished_ = true;
  }
  
  return;
}

void CServerS::DisableSlot(int slot)
{
  r3d_assert(slot < maxGames_);
  games_[slot].isDisabled = true;
  games_[slot].createTime = -99; // reset creation time, so we'll try to spawn new game in other slot
  disabledSlots_++;

  r3dOutToLog("!!! supervisor %s disabled game slot %d\n", GetName(), slot);
  return;
}

bool CServerS::RegisterNewGameSlot(const CMSNewGameData& ngd, SBPKT_M2S_StartGameReq_s& out_n)
{
  r3d_assert(games_ != NULL);

  const float curTime = r3dGetTime();
  int gameSlot = -1;
  
  for(int i=0; i<maxGames_; i++) 
  {
    if(games_[i].isDisabled)
      continue;
      
    if(games_[i].game != NULL)
      continue;
      
    // filter out pending games
    if(curTime < games_[i].createTime + REGISTER_EXPIRE_TIME) 
      continue;
    
    // filter out closing games
    if(curTime < games_[i].closeTime)
      continue;
      
    gameSlot = i;
    break;
  }
  
  if(gameSlot == -1)
  {
    r3dOutToLog("CServerS::RegisterNewGameSlot: no free slots\n");
    return false;
  }
  
  // set game params
  DWORD gameId      = CreateGameId(gameSlot);
  __int64 sessionId = CreateSessionId(ngd.ginfo.gameServerId);
  WORD gamePort     = portStart_ + gameSlot;

  // create answering packet
  out_n.gameId    = gameId;
  out_n.sessionId = sessionId;
  out_n.ginfo     = ngd.ginfo;
  out_n.port      = gamePort;
  out_n.creatorID = ngd.CustomerID;
  
  // reserved this gameslot
  games_s& slot = games_[gameSlot];
  r3d_assert(slot.game == NULL);
  slot.createTime     = curTime;
  slot.info           = ngd;
  slot.info.gameId    = gameId;
  slot.info.port      = gamePort;
  slot.info.sessionId = sessionId;

  r3dOutToLog("starting game %d at %s[%d], id:0x%x, name '%s', pwd '%s', owner:%d\n",
	slot.info.ginfo.gameServerId, 
	serverName_.c_str(),
	gameSlot,
	gameId,
	slot.info.ginfo.name, 
	slot.info.pwd, 
	slot.info.CustomerID
	);
	
  nextGameStart_ = r3dGetTime() + 1.0f;
  
  return true;
}

__int64 CServerS::CreateSessionId(int gameServerId)
{
  r3d_assert(gameServerId);
  
  // all sessions should be started after 1st jan 2012, cutoff time to reduce amount of seconds
  __int64 jan2012 = 1325376000;

  // get current time and calc number of seconds after jan2012
  __int64 curTime;
  _time64(&curTime);
  __int64 seconds = (curTime - jan2012) & 0x00FFFFFFFFFFFF;

  // high 16bit of sessionId is game server id, lower 48 bits is starting second
  __int64 sessionId = ((__int64)gameServerId << 48 | seconds);
  
  // because we should NOT start sessions within same seconds this ID should be unique
  return sessionId;
}

int CServerS::GetExpectedGames() const
{
  r3d_assert(games_ != NULL);

  const float curTime = r3dGetTime();
  int num = 0;

  for(int i=0; i<maxGames_; i++) 
  {
    // game is active, registering up, or waiting for close. disabled games considered expected as well.
    if(games_[i].game || (curTime < games_[i].createTime + REGISTER_EXPIRE_TIME) || (curTime < games_[i].closeTime) || games_[i].isDisabled) {
      num++;
    }
  }
  
  return num;
}

int CServerS::GetExpectedPlayers() const
{
  r3d_assert(games_ != NULL);

  int users = 0;
  for(int i=0; i<maxGames_; i++) 
  {
    const CServerG* game = games_[i].game;
    if(!game) 
      continue;

    users += game->curPlayers_ + game->GetJoiningPlayers();
  }
  
  return users;
}

CServerG* CServerS::CreateGame(DWORD gameId, DWORD peerId)
{
  int gameSlot;
  DWORD serverId;
  ParseGameId(gameId, &serverId, &gameSlot);
  r3d_assert(serverId == id_);

  r3d_assert(gameSlot < maxGames_);
  games_s& slot = games_[gameSlot];
  
  // not sure why, but this can happen somehow
  if(slot.info.gameId != gameId)
  {
    r3dOutToLog("!!!! gameId mismatch %x vs %x, time:%f\n", slot.info.gameId, gameId, r3dGetTime() - slot.createTime);
    return NULL;
  }

  r3d_assert(slot.info.gameId == gameId);
  r3d_assert(slot.game == NULL);

  CServerG* game = new CServerG(gameId, this->ip_, peerId);
  game->Init(slot.info);

  slot.game = game;

  //r3dOutToLog("game %x registered in slot %d in %s\n", gameId, gameSlot, GetName());
  return game;
}

void CServerS::DeregisterGame(const CServerG* game)
{
  r3d_assert(games_ != NULL);

  int gameSlot;
  DWORD serverId;
  ParseGameId(game->id_, &serverId, &gameSlot);
  r3d_assert(serverId == id_);

  r3d_assert(gameSlot < maxGames_);
  games_s& slot = games_[gameSlot];
  
  r3d_assert(slot.game == game);
  slot.game       = NULL;
  slot.createTime = -99;
  slot.closeTime  = r3dGetTime() + 2.0f; // give few sec to allow game server executable to finish

  //r3dOutToLog("game stopped at slot %d in %s\n", gameSlot, GetName());

  return;
}
