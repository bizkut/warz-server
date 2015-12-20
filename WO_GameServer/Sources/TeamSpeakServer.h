#pragma once

typedef unsigned __int64 uint64;
class CTeamSpeakServer
{
  public:
	uint64		serverID;
	DWORD		m_serverPwd;
	int		m_serverPort;
	HANDLE		m_hThread;
	volatile int	m_stopRequest;
	
	struct client_s
	{
	  int		peerID;
	  DWORD		CustomerID;
	};
	
	stdext::hash_map<int, client_s> m_clients;
	int		m_curClients;
	int		m_serverSlots;
	void		AddClient(int clientID, int peerId);
	void		RemoveClient(int clientID);
	DWORD		GetCustomerID(int clientID);
	
  public:
	CTeamSpeakServer();
	~CTeamSpeakServer();
	
	int		Startup();
	int		Shutdown();
	void		Tick();
	
	uint64		CreateVirtualServer(const char* name, int port, int maxClients);
	uint64		CreateChannel(const char* name);
};

extern	void		TSServer_Start(int port);
extern	void		TSServer_Stop();
extern	DWORD		TSServer_GetPassword();
