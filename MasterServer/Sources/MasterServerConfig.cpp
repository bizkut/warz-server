#include "r3dPCH.h"
#include "r3d.h"

#include "MasterServerConfig.h"

	CMasterServerConfig* gServerConfig = NULL;

static const char* configFile = "MasterServer.cfg";

CMasterServerConfig::CMasterServerConfig()
{
  const char* group      = "MasterServer";

  if(_access(configFile, 0) != 0) {
    r3dError("can't open config file %s\n", configFile);
  }

  masterPort_  = r3dReadCFG_I(configFile, group, "masterPort", SBNET_MASTER_PORT);
  clientPort_  = r3dReadCFG_I(configFile, group, "clientPort", GBNET_CLIENT_PORT);
  masterCCU_   = r3dReadCFG_I(configFile, group, "masterCCU",  3000);

  #define CHECK_I(xx) if(xx == 0)  r3dError("missing %s value in %s", #xx, configFile);
  #define CHECK_S(xx) if(xx == "") r3dError("missing %s value in %s", #xx, configFile);
  CHECK_I(masterPort_);
  CHECK_I(clientPort_);
  #undef CHECK_I
  #undef CHECK_S

  serverId_    = r3dReadCFG_I(configFile, group, "serverId", 0);
  if(serverId_ == 0)
  {
	MessageBox(NULL, "you must define serverId in MasterServer.cfg", "", MB_OK);
	r3dError("no serverId");
  }
  if(serverId_ > 255 || serverId_ < 1)
  {
	MessageBox(NULL, "bad serverId", "", MB_OK);
	r3dError("bad serverId");
  }

  minSupersToStartGame_ = r3dReadCFG_I(configFile, group, "minServers", 10);
  
  LoadConfig();
  
  // give time to spawn our hosted games (except for dev server)
  nextRentGamesCheck_ = r3dGetTime() + 60.0f;
  if(serverId_ >= MASTERSERVER_DEV_ID) nextRentGamesCheck_ = r3dGetTime() + 5.0f;
  
  return;
}

void CMasterServerConfig::LoadConfig()
{
  r3dCloseCFG_Cur();
  
  numPermGames_ = 0;

  LoadPermGamesConfig();
  Temp_Load_WarZGames();
  
  OnGameListUpdated();
}

void CMasterServerConfig::Temp_Load_WarZGames()
{

  char group[128];
  sprintf(group, "WarZGames");

  int numGames    = r3dReadCFG_I(configFile, group, "numGames", 0);
  int numGamesV1    = r3dReadCFG_I(configFile, group, "numGamesV1", 0);
  int maxPlayers  = r3dReadCFG_I(configFile, group, "maxPlayers", 32);
  int numCliffGames = r3dReadCFG_I(configFile, group, "numCliffGames", 0);
  int numTrialGames = r3dReadCFG_I(configFile, group, "numTrialGames", 0);
  int numPremiumGames = r3dReadCFG_I(configFile, group, "numPremiumGames", 0);
  int numVeteranGames = r3dReadCFG_I(configFile, group, "numVeteranGames", 0);
  int numPTEGamesColorado = r3dReadCFG_I(configFile, group, "numPTEGames", 0);
  int numPTEGamesCali = r3dReadCFG_I(configFile, group, "numPTEGamesCali", 0);
  int numPTEGamesStrongholds = r3dReadCFG_I(configFile, group, "numPTEGamesStronghold", 0);

  int numPTEPlayersColorado = 200;
  int numPTEPlayersCali = 100;
  int numPTEPlayersStrongholds = 50;
  
  for(int i=0; i<numGames; i++)
  {
    GBGameInfo ginfo;
    ginfo.mapId      = GBGameInfo::MAPID_WZ_Colorado;
    ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
    ginfo.maxPlayers = maxPlayers;
    ginfo.channel    = 2; // official server
	ginfo.EnableGass = 1;
	ginfo.isPVE		 = 0;

    sprintf(ginfo.name, "US Server %03d", i + 1);
    AddPermanentGame(10000 + i, ginfo, GBNET_REGION_US_West);
  }

  for(int i=0; i<numGamesV1; i++)
  {
    GBGameInfo ginfo;
    ginfo.mapId      = GBGameInfo::MAPID_WZ_ColoradoV1;
    ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
    ginfo.maxPlayers = maxPlayers;
    ginfo.channel    = 2; // official server
	ginfo.EnableGass = 1;
	ginfo.isPVE		 = 0;

    sprintf(ginfo.name, "US Server %03d", i + 1);
    AddPermanentGame(10000 + i, ginfo, GBNET_REGION_US_West);
  }

  for(int i=0; i<numTrialGames; i++)
  {
	  GBGameInfo ginfo;
	  ginfo.mapId      = GBGameInfo::MAPID_WZ_Colorado;
	  ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers | GBGameInfo::SFLAGS_TrialsAllowed;
	  ginfo.maxPlayers = maxPlayers;
	  ginfo.channel	   = 1; // trial server
	  ginfo.EnableGass = 1;
	  ginfo.isPVE	   = 0;

	  sprintf(ginfo.name, "US Trial Server %03d", i + 1);
	  AddPermanentGame(12000 + i, ginfo, GBNET_REGION_US_West);
  }

  for(int i=0; i<numPremiumGames; i++)
  {
	  GBGameInfo ginfo;
	  ginfo.mapId      = GBGameInfo::MAPID_WZ_Colorado;
	  ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
	  ginfo.maxPlayers = maxPlayers;
	  ginfo.channel	   = 4; // premium server
	  ginfo.EnableGass = 1;
	  ginfo.isPVE	   = 0;

	  sprintf(ginfo.name, "US Premium Server %03d", i + 1);
	  AddPermanentGame(14000 + i, ginfo, GBNET_REGION_US_West);
  }

  for(int i=0; i<numPTEGamesColorado; i++)
  {
	  GBGameInfo ginfo;
	  ginfo.mapId      = GBGameInfo::MAPID_WZ_Colorado;
	  ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
	  ginfo.maxPlayers = numPTEPlayersColorado;
	  ginfo.channel	   = 6; 
	  ginfo.EnableGass = 1;
	  ginfo.isPVE	   = 0;

	  sprintf(ginfo.name, "US PTE Colorado %03d", i + 1);
	  AddPermanentGame(15000 + i, ginfo, GBNET_REGION_US_West);
  }


//  California isn't in release
  for(int i=0; i<numPTEGamesCali; i++)
  {
	  GBGameInfo ginfo;
	  ginfo.mapId      = GBGameInfo::MAPID_WZ_California;
	  ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
	  ginfo.maxPlayers = numPTEPlayersCali;
	  ginfo.channel    = 6; 
	  ginfo.EnableGass = 1;
	  ginfo.isPVE	   = 0;

	  sprintf(ginfo.name, "US PTE California %03d", i + 1);
	  AddPermanentGame(15100 + i, ginfo, GBNET_REGION_US_West);
  }
 

  for(int i=0; i<numPTEGamesStrongholds; i++)
  {
	  GBGameInfo ginfo;
	  ginfo.mapId      = GBGameInfo::MAPID_WZ_Cliffside;
	  ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
	  ginfo.maxPlayers = numPTEPlayersStrongholds;
	  ginfo.channel    = 6;
	  ginfo.EnableGass = 1;
	  ginfo.isPVE	   = 0;

	  sprintf(ginfo.name, "US PTE Stronghold %03d", i + 1);
	  AddPermanentGame(15200 + i, ginfo, GBNET_REGION_US_West);
  }

  for(int i=0; i<numVeteranGames; i++)
  {
	  GBGameInfo ginfo;
	  ginfo.mapId      = GBGameInfo::MAPID_WZ_Colorado;
	  ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
	  ginfo.maxPlayers = maxPlayers;
	  ginfo.gameTimeLimit = 50; // X hours limit
	  ginfo.channel    = 7; // veteran server
	  ginfo.EnableGass = 1;
	  ginfo.isPVE	   = 0;

	  sprintf(ginfo.name, "US Veteran Server %03d", i + 1);
	  AddPermanentGame(16000 + i, ginfo, GBNET_REGION_US_West);
  }

  // stronghold cliffside games
  for(int i=0; i<numCliffGames; ++i) 
  {
	  GBGameInfo ginfo;
	  ginfo.mapId      = GBGameInfo::MAPID_WZ_Cliffside;
	  ginfo.flags      = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
	  ginfo.maxPlayers = 20;
	  ginfo.channel	   = 5; // strongholds
	  ginfo.EnableGass = 1;
	  ginfo.isPVE	   = 0;

	  sprintf(ginfo.name, "US Cliffside %03d", i+1);
	  AddPermanentGame(18000+i, ginfo, GBNET_REGION_US_West);
  }

  return;
}

void CMasterServerConfig::LoadPermGamesConfig()
{
  numPermGames_ = 0;

//#ifdef _DEBUG
//  r3dOutToLog("Permanet games disabled in DEBUG");
//  return;
//#endif
  
  for(int i=0; i<250; i++)
  {
    char group[128];
    sprintf(group, "PermGame%d", i+1);

    char map[512] = "";
    char data[512] = "";
    char name[512];
	int DisableASR;
	int DisableSNP;
	int DisableWPN;
	int OnlyFPS;
    r3dscpy(map,  r3dReadCFG_S(configFile, group, "map", ""));
    r3dscpy(data, r3dReadCFG_S(configFile, group, "data", ""));
    r3dscpy(name, r3dReadCFG_S(configFile, group, "name", ""));

	DisableASR = r3dReadCFG_I(configFile, group, "DisableASR", 0);
	DisableSNP = r3dReadCFG_I(configFile, group, "DisableSNP", 0);
	DisableWPN = r3dReadCFG_I(configFile, group, "DisableWPN", 0);
	OnlyFPS = r3dReadCFG_I(configFile, group, "OnlyFPS", 0);

    if(name[0] == 0)
      sprintf(name, "PermGame%d", i+1);

    if(*map == 0)
      continue;

    ParsePermamentGame(i, name, map, data,DisableASR,DisableSNP,DisableWPN,OnlyFPS);
  }

  return;  
}

static int StringToGBMapID(char* str)
{
  if(stricmp(str, "MAPID_WZ_Colorado") == 0)
    return GBGameInfo::MAPID_WZ_Colorado;
  if(stricmp(str, "MAPID_WZ_Cliffside") == 0)
    return GBGameInfo::MAPID_WZ_Cliffside;
  if(stricmp(str, "MAPID_WZ_California") == 0)
    return GBGameInfo::MAPID_WZ_California;
  if(stricmp(str, "MAPID_WZ_Caliwood") == 0)
	  return GBGameInfo::MAPID_WZ_Caliwood;
  if(stricmp(str, "MAPID_WZ_ColoradoV1") == 0)
    return GBGameInfo::MAPID_WZ_ColoradoV1;
  if(stricmp(str, "MAPID_WZ_SanDiego") == 0)
    return GBGameInfo::MAPID_WZ_SanDiego;
  if(stricmp(str, "MAPID_WZ_Devmap") == 0)
    return GBGameInfo::MAPID_WZ_Devmap;
  if(stricmp(str, "MAPID_WZ_GameHard1") == 0)
    return GBGameInfo::MAPID_WZ_GameHard1;
  if(stricmp(str, "MAPID_Editor_Particles") == 0)
    return GBGameInfo::MAPID_Editor_Particles;
  if(stricmp(str, "MAPID_ServerTest") == 0)
    return GBGameInfo::MAPID_ServerTest;
    
  r3dError("bad GBMapID %s\n", str);
  return 0;
}

static EGBGameRegion StringToGBRegion(const char* str)
{
  if(stricmp(str, "GBNET_REGION_US_West") == 0)
    return GBNET_REGION_US_West;

  else if(stricmp(str, "GBNET_REGION_US_East") == 0)
    return GBNET_REGION_US_East;

   else if(stricmp(str, "GBNET_REGION_Europe") == 0)
    return GBNET_REGION_Europe;

   else if(stricmp(str, "GBNET_REGION_Russia") == 0)
    return GBNET_REGION_Russia;

   else if(stricmp(str, "GBNET_REGION_SouthAmerica") == 0)
    return GBNET_REGION_SouthAmerica;

  r3dError("bad GBGameRegion %s\n", str);
  return GBNET_REGION_Unknown;
}

void CMasterServerConfig::ParsePermamentGame(int gameServerId, const char* name, const char* map, const char* data, int DisableASR ,int DisableSNP, int DisableWPN, int OnlyFPS)
{
  char mapid[128];
  char maptype[128];
  char region[128];
  int minGames;
  int maxGames;
  if(5 != sscanf(map, "%s %s %s %d %d", mapid, maptype, region, &minGames, &maxGames)) {
    r3dError("bad map format: %s\n", map);
  }

  int maxPlayers;
  int minLevel = 0;
  int maxLevel = 0;
  int channel = 0;
  int GassOnMap = 1;
  int isPVETheMap = 0;
  int gameTimeLimit = 0;
  if(7 != sscanf(data, "%d %d %d %d %d %d %d", &maxPlayers, &minLevel, &maxLevel, &channel, &gameTimeLimit, &GassOnMap, &isPVETheMap)) {
    r3dError("bad data format: %s\n", data);
  }

  GBGameInfo ginfo;
  ginfo.mapId        = StringToGBMapID(mapid);
  ginfo.maxPlayers   = maxPlayers;
  ginfo.EnableGass   = GassOnMap;
  ginfo.isPVE		 = isPVETheMap; // for PVE maps
  ginfo.flags        = GBGameInfo::SFLAGS_Nameplates | GBGameInfo::SFLAGS_CrossHair | GBGameInfo::SFLAGS_Tracers;
  if(channel == 1)
	  ginfo.flags |= GBGameInfo::SFLAGS_TrialsAllowed;
  if ( DisableASR == 1)
		ginfo.flags |= GBGameInfo::SFLAGS_DisableASR;
  if ( DisableSNP == 1)
		ginfo.flags |= GBGameInfo::SFLAGS_DisableSNP;
  if ( DisableWPN == 1)
		ginfo.flags |= GBGameInfo::SFLAGS_DisableWPN;
  if ( OnlyFPS == 1)
		ginfo.flags |= GBGameInfo::SFLAGS_OnlyFPS;
  ginfo.channel		 = channel; 
  ginfo.gameTimeLimit = gameTimeLimit;
  r3dscpy(ginfo.name, name);

  r3dOutToLog("permgame: ID:%d, %s, %s\n", 
    gameServerId, name, mapid);
  
  EGBGameRegion eregion = StringToGBRegion(region);
  AddPermanentGame(gameServerId, ginfo, eregion);
}

void CMasterServerConfig::AddPermanentGame(int gameServerId, const GBGameInfo& ginfo, EGBGameRegion region)
{
  r3d_assert(numPermGames_ < R3D_ARRAYSIZE(permGames_));
  permGame_s& pg = permGames_[numPermGames_++];

  r3d_assert(gameServerId);
  pg.ginfo = ginfo;
  pg.ginfo.gameServerId = gameServerId;
  pg.ginfo.region       = region;
  
  return;
}

void CMasterServerConfig::OnGameListUpdated()
{
  memset(&numGamesHosted, 0, sizeof(numGamesHosted));
  memset(&numGamesRented, 0, sizeof(numGamesRented));
  memset(&numStrongholdsRented, 0, sizeof(numStrongholdsRented));

  for(int i=0; i<numPermGames_; i++)
  {
    const GBGameInfo& ginfo = permGames_[i].ginfo;

    int regIdx = 0;
    if(ginfo.region == GBNET_REGION_US_West) regIdx = 0;
    else if(ginfo.region == GBNET_REGION_Europe) regIdx = 1;
    else if(ginfo.region == GBNET_REGION_Russia) regIdx = 2;
	else if(ginfo.region == GBNET_REGION_SouthAmerica) regIdx = 3;
    
    numGamesHosted[regIdx]++;
  }
  
  for(size_t i=0; i<rentGames_.size(); i++)
  {
    const GBGameInfo& ginfo = rentGames_[i].ginfo;

    int regIdx = 0;
    if(ginfo.region == GBNET_REGION_US_West) regIdx = 0;
    else if(ginfo.region == GBNET_REGION_Europe) regIdx = 1;
    else if(ginfo.region == GBNET_REGION_Russia) regIdx = 2;
	else if(ginfo.region == GBNET_REGION_SouthAmerica) regIdx = 3;
    
    if(ginfo.IsGameworld())
      numGamesRented[regIdx]++;
    else
      numStrongholdsRented[regIdx]++;
  }
  
  // create gameinfo<->gameServerId map
  rentByGameServerId_.clear();
  for(size_t i=0; i<rentGames_.size(); i++)
  {
    CMasterServerConfig::rentGame_s* rg = &gServerConfig->rentGames_[i];
    rentByGameServerId_.insert(TRentedGamesList::value_type(rg->ginfo.gameServerId, rg));
  }
  
  return;
}

CMasterServerConfig::rentGame_s* CMasterServerConfig::GetRentedGameInfo(DWORD gameServerId)
{
  TRentedGamesList::iterator it = rentByGameServerId_.find(gameServerId);
  if(it == rentByGameServerId_.end())
    return NULL;

  return it->second;
}
