#include "r3dPCH.h"
#include "r3d.h"

#include "MasterServerLogic.h"

	MasterServerLogic gMasterServerLogic;

#include "../../MasterServer/Sources/NetPacketsServerBrowser.h"
using namespace NetPacketsServerBrowser;

#include "ObjectsCode/weapons/WeaponArmory.h"

#include "ServerGameLogic.h"

MasterServerLogic::MasterServerLogic()
{
  disconnected_    = false;
  gotWeaponUpdate_ = false;
  shuttingDown_    = 0;
}

MasterServerLogic::~MasterServerLogic()
{
}

void MasterServerLogic::OnNetPeerConnected(DWORD peerId)
{
  return;
}

void MasterServerLogic::OnNetPeerDisconnected(DWORD peerId)
{
  r3dOutToLog("!!! master server disconnected\n");
  disconnected_ = true;
  return;
}

void MasterServerLogic::OnNetData(DWORD peerId, const r3dNetPacketHeader* packetData, int packetSize)
{

  // PunkBuster Packet
  if ( packetSize > 7 && memcmp ( packetData, "\xff\xff\xff\xffPB_", 7) == 0 )
	return;

  switch(packetData->EventID) 
  {
    default:
      r3dError("MasterServerLogic: unknown packetId %d", packetData->EventID);
      return;

    case SBPKT_ValidateConnectingPeer:
    {
      const SBPKT_ValidateConnectingPeer_s& n = *(SBPKT_ValidateConnectingPeer_s*)packetData;
      if(n.version != SBNET_VERSION) {
        r3dError("master server version is different (%d vs %d)", n.version, SBNET_VERSION);
        break;
      }
      break;
    }
    
    case SBPKT_M2G_KillGame:
    {
      r3dOutToLog("Master server requested game kill\n");
      net_->DisconnectPeer(peerId);
      break;
    }
    
    case SBPKT_M2G_KickPlayer:
    {
      const SBPKT_M2G_KickPlayer_s& n = *(SBPKT_M2G_KickPlayer_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);

      r3dOutToLog("Master server requested player kick %d\n", n.CharID);
      kickReqCharID_ = n.CharID;
      break;
    }

    case SBPKT_M2G_SetGameFlags:
    {
      const SBPKT_M2G_SetGameFlags_s& n = *(SBPKT_M2G_SetGameFlags_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);

      r3dOutToLog("Master server set new game flags %d\n", n.flags);
      gServerLogic.SetNewGameInfo(n.flags, n.gametimeLimit);

      break;
    }
    case SBPKT_M2G_SendIsPVE: // for PVE maps
    {
      const SBPKT_M2G_SendIsPVE_s& n = *(SBPKT_M2G_SendIsPVE_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);

      gServerLogic.SetPVEinfo(n.isPVE);

      break;
    }
    case SBPKT_M2G_ShutdownNote:
    {
      const SBPKT_M2G_ShutdownNote_s& n = *(SBPKT_M2G_ShutdownNote_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);
      
      if(n.reason == 2)
        r3dOutToLog("---- Supervisor server is shutting down now\n");
      else
        r3dOutToLog("---- Master server is shutting down now\n");
      shuttingDown_ = n.reason;
      shutdownLeft_ = n.timeLeft;
      break;
    }
    
    #ifdef ENABLE_ARMORY_UPDATE
    case SBPKT_M2G_UpdateWeaponData:
    {
      const SBPKT_M2G_UpdateWeaponData_s& n = *(SBPKT_M2G_UpdateWeaponData_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);

      /*
      WeaponConfig* wc = const_cast<WeaponConfig*>(g_pWeaponArmory->getWeaponConfig(n.itemId));
      if(wc == NULL) {
        r3dOutToLog("!!! got update for not existing weapon %d\n", n.itemId);
        return;
      }

      wc->copyParametersFrom(n.wi);
      //r3dOutToLog("got update for weapon %s\n", wc->m_StoreName);
      */
      break;
    }

    case SBPKT_M2G_UpdateGearData:
    {
      const SBPKT_M2G_UpdateGearData_s& n = *(SBPKT_M2G_UpdateGearData_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);

      /*
      GearConfig* gc = const_cast<GearConfig*>(g_pWeaponArmory->getGearConfig(n.itemId));
      if(gc == NULL) {
        r3dOutToLog("!!! got update for not existing gear %d\n", n.itemId);
        return;
      }

      gc->copyParametersFrom(n.gi);
      //r3dOutToLog("got update for gear %s\n", gc->m_StoreName);
      */
      break;
    }

    case SBPKT_M2G_UpdateItemData:
    {
      const SBPKT_M2G_UpdateItemData_s& n = *(SBPKT_M2G_UpdateItemData_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);

      break;
    }

    case SBPKT_M2G_UpdateDataEnd:
    {
      const SBPKT_M2G_UpdateDataEnd_s& n = *(SBPKT_M2G_UpdateDataEnd_s*)packetData;
      r3d_assert(sizeof(n) == packetSize);

      r3dOutToLog("got SBPKT_M2G_UpdateDataEnd\n");
      gotWeaponUpdate_ = true;
      
      break;
    }
    #endif
    
  }

  return;
}

int MasterServerLogic::WaitFunc(fn_wait fn, float timeout, const char* msg)
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

    if(r3dGetTime() > endWait) {
      return 0;
    }
  }
  
  return 1;
}

void MasterServerLogic::Init(DWORD gameId)
{
  gameId_ = gameId;
}

int MasterServerLogic::Connect(const char* host, int port, int listen_port)
{
  r3dOutToLog("Connecting to master server at %s:%d\n", host, port); CLOG_INDENT;
  
  g_net.Initialize(this, "master server");
  g_net.CreateClient(listen_port);
  g_net.Connect(host, port);

  if(!WaitFunc(&MasterServerLogic::wait_IsConnected, 30, "connecting"))
    throw "can't connect to master server";

  // send validation packet immediately
  CREATE_PACKET(SBPKT_ValidateConnectingPeer, n);
  n.version = SBNET_VERSION;
  n.key1    = SBNET_KEY1;
  n.key2    = SBNET_KEY2;
  net_->SendToHost(&n, sizeof(n), true);
    
  return 1;
}

void MasterServerLogic::Disconnect()
{
  g_net.Deinitialize();
}

void MasterServerLogic::Tick()
{
  if(net_ == NULL)
    return;

  net_->Update();
  
  if(shuttingDown_)
    shutdownLeft_ -= r3dGetFrameTime();
}

void MasterServerLogic::RegisterGame()
{
  if(net_ == NULL)
    return;
    
  SBPKT_G2M_RegisterGame_s n(gameId_);
  net_->SendToHost(&n, sizeof(n), true);

  return;
}

void MasterServerLogic::FinishGame()
{
  if(net_ == NULL)
    return;

  SBPKT_G2M_FinishGame_s n(gameId_);
  net_->SendToHost(&n, sizeof(n), true);
  
  return;
}

void MasterServerLogic::CloseGame()
{
  if(net_ == NULL || IsMasterDisconnected())
    return;

  SBPKT_G2M_CloseGame_s n(gameId_);
  net_->SendToHost(&n, sizeof(n), true);
  
  return;
}

void MasterServerLogic::AddPlayer(int playerIdx, DWORD CustomerID, const wiCharDataFull* loadout)
{
  SBPKT_G2M_AddPlayer_s n(gameId_);
  n.playerIdx  = (WORD)playerIdx;
  n.CustomerID = CustomerID;
  n.CharID     = loadout->LoadoutID;
  r3dscpy(n.gamertag, loadout->Gamertag);
  n.reputation = loadout->Stats.Reputation;
  n.XP         = loadout->Stats.XP;
  net_->SendToHost(&n, sizeof(n), true);

  return;  
}

void MasterServerLogic::RemovePlayer(int playerIdx)
{
  SBPKT_G2M_RemovePlayer_s n(gameId_);
  n.playerIdx = (WORD)playerIdx;
  net_->SendToHost(&n, sizeof(n), true);

  return;  
}

#ifdef ENABLE_ARMORY_UPDATE
void MasterServerLogic::RequestDataUpdate()
{
  SBPKT_G2M_DataUpdateReq_s n;
  net_->SendToHost(&n, sizeof(n), true);
}
#endif