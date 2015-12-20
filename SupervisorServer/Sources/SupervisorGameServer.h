#pragma once

#include "r3dNetwork.h"
#include <list>
#include <hash_map>

#include "../../ServerNetPackets/NetPacketsGameInfo.h"
#include "../../MasterServer/Sources/NetPacketsServerBrowser.h"

class CSupervisorGameServer : public r3dNetCallback
{
  protected:	
	r3dNetwork	g_net;
	void		OnNetPeerConnected(DWORD peerId);
	void		OnNetPeerDisconnected(DWORD peerId);
	void		OnNetData(DWORD peerId, const r3dNetPacketHeader* packetData, int packetSize);
	
  public:
	enum { MAX_PEERS_COUNT = 1024, };
	enum EPeerStatus
	{
	  PEER_Free,
	  PEER_Connected,
	  PEER_Validated,
	};
	struct peer_s 
	{
	  int		status;
	  float		connectTime;
	  DWORD		gameId;
	  
	  peer_s() 
	  {
	    status = PEER_Free;
	    gameId = 0;
	  }
	};
	peer_s		peers_[MAX_PEERS_COUNT];

	typedef stdext::hash_map<DWORD, DWORD> TGamesList;
	TGamesList	games_;			// gameId<->peer map
	
	void		DisconnectIdlePeers();
	void		DisconnectCheatPeer(DWORD peerId, const char* message);
	bool		DoValidatePeer(DWORD peerId, const r3dNetPacketHeader* PacketData, int PacketSize);
	
	void		RelayToGame(const r3dNetPacketHeader* PacketData, int PacketSize);

	void		OnGameProcessClosed(DWORD gameId);
	
	void		RequestGamesShutdown(float timeLeft);

  public:
	CSupervisorGameServer();
	~CSupervisorGameServer();

	void		Start(int port);
	void		Tick();
	void		Stop();
};

extern	CSupervisorGameServer gSupervisorGameServer;
