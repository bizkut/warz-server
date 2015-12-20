#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"

#include "SupervisorGameServer.h"
#include "SupervisorServer.h"

//#pragma warning(disable: 4065)	// switch statement contains 'default' but no 'case' labels

#include "../../MasterServer/Sources/NetPacketsServerBrowser.h"
using namespace NetPacketsServerBrowser;

	CSupervisorGameServer gSupervisorGameServer;

CSupervisorGameServer::CSupervisorGameServer()
{
  return;
}

CSupervisorGameServer::~CSupervisorGameServer()
{
}

void CSupervisorGameServer::Start(int port)
{
  g_net.Initialize(this, "supervisorNet");
  if(!g_net.CreateHost(port, MAX_PEERS_COUNT)) {
    r3dError("CreateHost failed\n");
  }

  r3dOutToLog("SupervisorGameServer started at port %d\n", port);
  
  return;
}

void CSupervisorGameServer::Tick()
{
  const float curTime = r3dGetTime();
  
  net_->Update();
  
  DisconnectIdlePeers();

  return;
}

void CSupervisorGameServer::Stop()
{
  if(net_)
    net_->Deinitialize();
}

void CSupervisorGameServer::RequestGamesShutdown(float timeLeft)
{
  for(TGamesList::iterator it = games_.begin(); it != games_.end(); ++it)
  {
    DWORD peerId = it->second;

    SBPKT_M2G_ShutdownNote_s n(it->first);
    n.reason   = 2;
    n.timeLeft = timeLeft;
    net_->SendToPeer(&n, sizeof(n), peerId);
  }
}

void CSupervisorGameServer::DisconnectIdlePeers()
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

void CSupervisorGameServer::DisconnectCheatPeer(DWORD peerId, const char* message)
{
  r3dOutToLog("cheat: master peer%d, reason: %s\n", peerId, message);
  net_->DisconnectPeer(peerId);
  
  // fire up disconnect event manually, enet might skip if if other peer disconnect as well
  OnNetPeerDisconnected(peerId);
}

void CSupervisorGameServer::OnNetPeerConnected(DWORD peerId)
{
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

void CSupervisorGameServer::OnNetPeerDisconnected(DWORD peerId)
{
  peer_s& peer = peers_[peerId];
  switch(peer.status)
  {
    case PEER_Free:
      break;
    case PEER_Connected:
      break;

    case PEER_Validated:
      r3dOutToLog("game 0x%x disconnected from peer%d\n", peer.gameId, peerId);
    
      // report to master
      SBPKT_S2M_GameDisconnect_s n2(peer.gameId);
      gSupervisorServer.net_->SendToHost(&n2, sizeof(n2));

      games_.erase(peer.gameId);
      break;
  }
  
  peer.status = PEER_Free;
  peer.gameId = 0;
  return;
}

bool CSupervisorGameServer::DoValidatePeer(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize)
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
    DisconnectCheatPeer(peerId, "!!!! wrong validate packet data");
    return false;
  }

  // set peer to validated
  peer.status = PEER_Validated;
  return false;
}

void CSupervisorGameServer::OnNetData(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize)
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
      r3dOutToLog("CSupervisorGameServer: invalid packetId %d", PacketData->EventID);
      DisconnectCheatPeer(peerId, "wrong packet id");
      return;

    // game<->master relayed packets
    case SBPKT_G2M_RegisterGame:
    {
      const SBPKT_G2M_RegisterGame_s& n = *(const SBPKT_G2M_RegisterGame_s*)PacketData;
      r3d_assert(sizeof(n) == PacketSize);
      
      peer.gameId = n.gameId;
      r3dOutToLog("game 0x%x connected at peer%d\n", peer.gameId, peerId);
      
      games_.insert(TGamesList::value_type(n.gameId, peerId));
      // and fall thru
    }
    case SBPKT_G2M_AddPlayer:
    case SBPKT_G2M_RemovePlayer:
    case SBPKT_G2M_FinishGame:
    case SBPKT_G2M_CloseGame:
      gSupervisorServer.net_->SendToHost(PacketData, PacketSize);
      break;
  }

  return;
}

void CSupervisorGameServer::RelayToGame(const r3dNetPacketHeader* PacketData, int PacketSize)
{
  // MAKE VERY SURE that first DWORD of relaying packet is gameId
  DWORD gameId = *(DWORD*)(((BYTE*)PacketData) + sizeof(r3dNetPacketHeader));	// skip EventID
	
  TGamesList::iterator it = games_.find(gameId);
  if(it == games_.end())
  {
    r3dOutToLog("!!!!!! relaying %d to game 0x%x\n", PacketData->EventID, gameId);
    return;
  }

  DWORD peerId = it->second;
  net_->SendToPeer(PacketData, PacketSize, peerId);
}

void CSupervisorGameServer::OnGameProcessClosed(DWORD gameId)
{
  TGamesList::iterator it = games_.find(gameId);
  if(it == games_.end())
  {
    r3dOutToLog("game 0x%x finished\n", gameId);
    return;
  }

  // we don't have process anymore, but we still have connected peer. probably game is crashed
  r3dOutToLog("!!!! game 0x%x probably CRASHED\n", gameId);

  DWORD peerId = it->second;
  net_->DisconnectPeer(peerId, true); // must do immidiate disconnect as process is no longer active and we must free associated peer
  OnNetPeerDisconnected(peerId);
}