#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"
#include <ShellAPI.h>
#include "CkString.h"

//#pragma warning(disable: 4065)	// switch statement contains 'default' but no 'case' labels

#include "SupervisorServer.h"
#include "SupervisorGameServer.h"
#include "../../MasterServer/Sources/SrvCrashHandler.h"

#pragma comment(lib, "pdh.lib")

using namespace NetPacketsServerBrowser;

static	r3dNetwork	serverNet;
	CSupervisorServer gSupervisorServer;


CSupervisorServer::CSupervisorServer()
{
  disconnected_ = false;
  superId_      = 0;
  games_        = NULL;
  InitializeCriticalSection(&csGames_);

  pc_query = NULL;
  PdhOpenQuery(NULL, (DWORD_PTR)NULL, &pc_query);
  PdhAddCounter(pc_query, "\\Processor(_Total)\\% Processor Time", 0, &pc_hCPU);

  nextMonitorUpdate_ = 0;
  
  shuttingDown_  = false;
}

CSupervisorServer::~CSupervisorServer()
{
  if(games_) 
    delete[] games_;
    
  DeleteCriticalSection(&csGames_);
  
  if(pc_query)
    PdhCloseQuery(pc_query);
}

static bool IsPortInUse_Check(unsigned short port, const char *hostAddress)
{
	SOCKET listenSocket;
	sockaddr_in listenerSocketAddress;
	memset(&listenerSocketAddress,0,sizeof(sockaddr_in));
	// Listen on our designated Port#
	listenerSocketAddress.sin_port = htons( port );
	listenSocket = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( listenSocket == (SOCKET) -1 )
		return true;
	// bind our name to the socket
	// Fill in the rest of the address structure
	listenerSocketAddress.sin_family = AF_INET;
	if ( hostAddress && hostAddress[0] )
		listenerSocketAddress.sin_addr.s_addr = inet_addr( hostAddress );
	else
		listenerSocketAddress.sin_addr.s_addr = INADDR_ANY;

	int ret = bind( listenSocket, ( struct sockaddr * ) & listenerSocketAddress, sizeof( listenerSocketAddress ) );
	closesocket(listenSocket);

	return ret <= -1;
}

static bool IsPortInUse(int port)
{
  // get all computer available ip addresses
  char ac[256];
  if(gethostname(ac, sizeof(ac)) == -1)
    r3dError("gethostname failed\n");
  struct hostent* phe = gethostbyname(ac);
  if(phe == NULL) 
    r3dError("gethostbyname %s failed\n", ac);

  int numBinds = 0;
  for(numBinds = 0; numBinds < 128; ++numBinds) {
    if(phe->h_addr_list[numBinds] == NULL)
      break;
      
    in_addr ipaddr;
    memcpy(&ipaddr, phe->h_addr_list[numBinds], sizeof(in_addr));

    char ip[128];
    r3dscpy(ip, inet_ntoa(ipaddr));
    if(IsPortInUse_Check(port, ip) == true) {
      return true;
    }
  }
  
  return false;
}

void CSupervisorServer::StartGame(const SBPKT_M2S_StartGameReq_s& n)
{
  r3dCSHolder cs1(csGames_);

  const int slot = n.port - gSupervisorConfig->portStart_;
  if(slot < 0 || slot >= gSupervisorConfig->maxGames_) {
    r3dOutToLog("!!!warning!!! invalid StartGame request port %d", n.port);
    return;
  }
  
  if(shuttingDown_)
    return;
  
  if(games_[slot].hProcess != 0) {
    r3dOutToLog("!!!warning!!! invalid StartGame request, slot %d already used\n", slot);

    // this may be because of previous instance crash, so terminate it
    TerminateProcess(games_[slot].hProcess, 0);
    games_[slot].Reset();
  }
  
  r3dOutToLog("StartGame id:0x%x, sess:%I64x, slot:%d, creatorID:%u\n", n.gameId, n.sessionId, slot, n.creatorID); CLOG_INDENT;
  
  // check if port can be used
  if(IsPortInUse(n.port))
  {
    r3dOutToLog("!!! port %d is already in use - unable to start game\n", n.port);

    SBPKT_S2M_DisableSlot_s n2;
    n2.slot = (WORD)slot;
    net_->SendToHost(&n2, sizeof(n2));

    return;
  }

  games_[slot].Init(n.gameId, n.sessionId);
  
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  char arg1[128];
  char arg2[128];
  char arg3[128];
  sprintf(arg1, "%u %u %u", n.gameId, n.port, n.creatorID);
  n.ginfo.ToString(arg2);
  r3dscpy(arg3, gSupervisorConfig->masterIp_.c_str());
  
  CkString ckGameName;
  ckGameName.setString(n.ginfo.name);
  ckGameName.base64Encode("utf-8");
  
  char params[512];
  sprintf(params, "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%d\"", 
    gSupervisorConfig->gameServerExe_.c_str(), 
    arg1, 
    arg2,
    arg3,
    ckGameName.getString(),
    gSupervisorConfig->uploadLogs_
    );
  
  //r3dOutToLog("CreateProcess: %s\n", params);
  BOOL res = CreateProcess(
    NULL,
    params,
    NULL,
    NULL,
    FALSE,
    DETACHED_PROCESS, // no console by default, if needed game server will alloc it
    NULL,
    NULL,
    &si,
    &pi);
    
  if(res == 0) {
    r3dOutToLog("!!!warning!!! unable to spawn game servers, hr:%d\n", GetLastError());
    games_[slot].Reset();
    return;
  }
  
  // set process handle
  games_[slot].hProcess = pi.hProcess;
  
  return;
}

void CSupervisorServer::OnNetPeerConnected(DWORD peerId)
{
  r3dOutToLog("Supervisor: Connected to master as peer %d\n", peerId);
  return;
}

void CSupervisorServer::OnNetPeerDisconnected(DWORD peerId)
{
  r3dOutToLog("!!!!!!!! Supervisor: MasterServer disconnected\n");
  disconnected_ = true;
  return;
}

void CSupervisorServer::OnNetData(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize)
{

  // PunkBuster Packet
  if ( PacketSize > 7 && memcmp ( PacketData, "\xff\xff\xff\xffPB_", 7) == 0 )
	return;

  switch(PacketData->EventID) 
  {
    default:
      r3dError("CSupervisorServer: invalid packetId %d", PacketData->EventID);
      break;

    case SBPKT_ValidateConnectingPeer:
    {
      const SBPKT_ValidateConnectingPeer_s& n = *(SBPKT_ValidateConnectingPeer_s*)PacketData;
      if(n.version != SBNET_VERSION) {
        r3dError("master server version is different (%d vs %d)", n.version, SBNET_VERSION);
        break;
      }
      break;
    }
    
    case SBPKT_M2S_RegisterAns:
    {
      const SBPKT_M2S_RegisterAns_s& n = *(SBPKT_M2S_RegisterAns_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);
      
      superId_ = n.id;
      break;
    }
    
    case SBPKT_M2S_StartGameReq:
    {
      const SBPKT_M2S_StartGameReq_s& n = *(SBPKT_M2S_StartGameReq_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);
      
      StartGame(n);
      break;
    }

	case SBPKT_M2G_SendIsPVE: // for PVE maps
    {
      const SBPKT_M2G_SendIsPVE_s& n = *(SBPKT_M2G_SendIsPVE_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);
	  gSupervisorGameServer.RelayToGame(PacketData, PacketSize);
      break;
    }

    // master<->game relayed packets
    case SBPKT_M2G_ShutdownNote:
    case SBPKT_M2G_KillGame:
    case SBPKT_M2G_KickPlayer:
    case SBPKT_M2G_SetGameFlags:
      gSupervisorGameServer.RelayToGame(PacketData, PacketSize);
      break;
  }

  return;
}

int CSupervisorServer::WaitFunc(fn_wait fn, float timeout, const char* msg)
{
  const float endWait = r3dGetTime() + timeout;

  r3dOutToLog("waiting: %s, %.1f sec left\n", msg, endWait - r3dGetTime());
  
  while(1) 
  {
    r3dEndFrame();
    r3dStartFrame();
    
    net_->Update();
    
    if((this->*fn)())
      break;
      
    if(disconnected_)
      return 0;

    if(r3dGetTime() > endWait) {
      return 0;
    }
  }
  
  return 1;
}

int CSupervisorServer::ConnectToMasterServer()
{
  const char* hostaddr = gSupervisorConfig->masterIp_.c_str();
  int         hostport = gSupervisorConfig->masterPort_;
  r3dOutToLog("Connecting to master server at %s:%d\n", hostaddr, hostport); CLOG_INDENT;
  
  disconnected_ = false;
  superId_      = 0;
  net_->Connect(hostaddr, hostport);
  if(!WaitFunc(&CSupervisorServer::wait_IsConnected, 20.0f, "connecting"))
    return 2;

  // send validation packet immediately
  CREATE_PACKET(SBPKT_ValidateConnectingPeer, n);
  n.version = SBNET_VERSION;
  n.key1    = SBNET_KEY1;
  n.key2    = SBNET_KEY2;
  net_->SendToHost(&n, sizeof(n), true);
  
  // report caps
  {
    CREATE_PACKET(SBPKT_S2M_RegisterMachine, n);
    n.region         = (BYTE)gSupervisorConfig->serverGroup_;
    n.serverType     = (BYTE)gSupervisorConfig->serverType_;
    r3dscpy(n.serverName, gSupervisorConfig->serverName_.c_str());
    n.mapId          = (BYTE)gSupervisorConfig->mapId_;
    n.maxGames       = gSupervisorConfig->maxGames_;
    n.maxPlayers     = gSupervisorConfig->maxPlayers_;
    n.portStart      = gSupervisorConfig->portStart_;
    n.externalIpAddr = gSupervisorConfig->externalIpAddr_;
    net_->SendToHost(&n, sizeof(n), true);
  }

  if(!WaitFunc(&CSupervisorServer::wait_IsRegistered, 10.0f, "wait for register")) 
  {
    if(disconnected_)
      r3dOutToLog("Supervisor was disconnected from master server\n");
    else
      r3dOutToLog("Supervisor was unable to register at master server\n");
    return 3;
  }
  
  r3dOutToLog("registered, id: %d\n", superId_);
  return 1;
}

bool CSupervisorServer::Start()
{
  SetConsoleTitle("WO::Supervisor");

  _mkdir("logsv");
  time_t t1;
  time(&t1);
  char fname[MAX_PATH];
  sprintf(fname, "logsv\\SV_%x.txt", (DWORD)t1);
  extern void r3dChangeLogFile(const char* fname);
  r3dChangeLogFile(fname);

  sprintf(fname, "logsv\\SV_%x.dmp", (DWORD)t1);
  SrvSetCrashHandler(fname);

  games_ = new CGameWatcher[gSupervisorConfig->maxGames_];

  serverNet.Initialize(this, "serverNet");
  serverNet.CreateClient(SBNET_SUPER_MASTER_PORT);

  // try to connect for max 60sec, then exit.
  const float maxConnectTime = r3dGetTime() + 60.0f;
  while(1)
  {
    r3dStartFrame();
    r3dEndFrame();

    switch(ConnectToMasterServer())
    {
      default: 
        r3d_assert(0);
        break;
        
      case 1: // ok
        return true;
        
      case 2: // timeout
        r3dOutToLog("retrying in 2 sec\n");
        ::Sleep(2000);
        break;
      
      case 3: // disconnect
        r3dOutToLog("failed to connect to master server\n");
        return false;
    }
       
    if(r3dGetTime() > maxConnectTime) {
      r3dOutToLog("Timeout while connecting to master server\n");
      return false;
    }
  }
  
  r3d_assert(0);
}

void CSupervisorServer::MonitorProcesses()
{
  if(r3dGetTime() < nextMonitorUpdate_)
    return;
  nextMonitorUpdate_ = r3dGetTime() + 0.1f;

  r3dCSHolder cs1(csGames_);
  for(int slot=0; slot<gSupervisorConfig->maxGames_; slot++) {
    games_[slot].CheckProcess();
  }

  return;
}

void CSupervisorServer::TerminateAllGames()
{
  r3dCSHolder cs1(csGames_);

  if(games_ == NULL)
    return;

  r3dOutToLog("Stopping all games\n"); CLOG_INDENT;    
  for(int slot=0; slot<gSupervisorConfig->maxGames_; slot++) {
    // do not call .Terminate() here as it will spam that game crashed
    if(games_[slot].hProcess) {
      ::TerminateProcess(games_[slot].hProcess, 1);
    }
  }
}

void CSupervisorServer::RequestShutdown()
{
  if(shuttingDown_) {
    r3dOutToLog("--------- SHUTDOWN in progress, waiting for games finish\n");
    return;
  }

  r3dOutToLog("--------- SHUTDOWN requested\n");

  shuttingDown_ = true;
  shutdownLeft_ = 300.0f; // 5 min shutdown

  // report to games
  gSupervisorGameServer.RequestGamesShutdown(shutdownLeft_);
  
  // report to master
  SBPKT_S2M_ShutdownNote_s n;
  net_->SendToHost(&n, sizeof(n));

  return;
}

bool CSupervisorServer::IsActiveSession(__int64 gameSessionId)
{
  r3dCSHolder cs1(csGames_);

  if(games_ == NULL)
    return false;

  for(int slot=0; slot<gSupervisorConfig->maxGames_; slot++) {
    if(games_[slot].info && games_[slot].info->sessionId == gameSessionId)
      return true;
  }
  
  return false;
}

void CSupervisorServer::Tick()
{
  net_->Update();
  
  MonitorProcesses();
  
  if(shuttingDown_ && gSupervisorGameServer.games_.size() == 0)
    disconnected_ = true;

  // output some info
  static float nextLog = 0;
  if(r3dGetTime() > nextLog) 
  {
    nextLog = r3dGetTime() + 2.0f;
    
    int numGames = 0;
    for(int slot=0; slot<gSupervisorConfig->maxGames_; slot++) {
      if(games_[slot].hProcess != NULL) 
        numGames++;
    }

    PdhCollectQueryData(pc_query);
        
    DWORD ret;
    PDH_FMT_COUNTERVALUE value;
    PdhGetFormattedCounterValue(pc_hCPU, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, &ret, &value);
    //r3dOutToLog("CPU Total usage: %2.2f\n",value.doubleValue);

    char buf[1024];
    sprintf(buf, "WO::Supervisor, %d games, %2.0f%% CPU %s", numGames, value.doubleValue, shuttingDown_ ? "(SHUTTING DOWN)" : "");
    SetConsoleTitle(buf);
  }
  
  return;
}

void CSupervisorServer::Stop()
{
  if(net_)
    net_->Deinitialize();
}
