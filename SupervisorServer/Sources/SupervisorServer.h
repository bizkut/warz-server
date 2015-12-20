#pragma once

#include "r3dNetwork.h"
#include "../../MasterServer/Sources/NetPacketsServerBrowser.h"
#include "SupervisorConfig.h"
#include "GameWatcher.h"

#include "Pdh.h"

class CSupervisorServer : public r3dNetCallback
{
  private:
	volatile bool	disconnected_;
	volatile DWORD	superId_;
	
	bool		shuttingDown_;
	float		shutdownLeft_;

	typedef bool (CSupervisorServer::*fn_wait)();
	int		WaitFunc(fn_wait fn, float timeout, const char* msg);

	// wait functions
	bool		wait_IsConnected() {
	  return net_->IsConnected();
	}
	bool		wait_IsRegistered() {
	  return superId_ > 0;
	}

	int		ConnectToMasterServer();
	
	void		StartGame(const NetPacketsServerBrowser::SBPKT_M2S_StartGameReq_s& n);
	
	CGameWatcher*	games_;
	CRITICAL_SECTION csGames_;
	float		nextMonitorUpdate_;
	void		MonitorProcesses();
	
	// performance counters
	PDH_HQUERY	pc_query;
	HCOUNTER	pc_hCPU;

  public:
	bool		IsMasterDisconnected() const {
	  return disconnected_;
	}

  private:
	// callbacks from r3dNetwork
	void		OnNetPeerConnected(DWORD peerId);
	void		OnNetPeerDisconnected(DWORD peerId);
	void		OnNetData(DWORD peerId, const r3dNetPacketHeader* packetData, int packetSize);
	
  public:
	CSupervisorServer();
	~CSupervisorServer();

	bool		Start();
	void		Stop();

	void		Tick();
	void		TerminateAllGames();
	void		RequestShutdown();

	bool		IsActiveSession(__int64 gameSessionId);
};

extern	CSupervisorServer	gSupervisorServer;
