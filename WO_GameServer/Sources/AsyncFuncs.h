#pragma once

#include "Backend/ServerUserProfile.h"

class obj_ServerPlayer;

class CAsyncApiJob
{
  public:
	char		desc[156];
	int		JobNumTries;
	int		JobMaxTries;
  
	int		peerId;		// or -1 if this is peerless operation
	DWORD		CustomerID;
	DWORD		SessionID;
	DWORD		CharID;
	
	int		ResultCode;
	
  public:
	CAsyncApiJob();
	CAsyncApiJob(const obj_ServerPlayer* plr);
	CAsyncApiJob(DWORD in_peerId);
virtual	~CAsyncApiJob() { }
	
virtual	int		Exec() = NULL;
virtual	void		OnSuccess() = NULL;
virtual void		OnFailure() {};
};

class CAsyncApiWorker
{
  public:
	int		idx_;
	HANDLE		hThread;
	
	HANDLE		hStartEvt_;
	bool		needToExit_;

	CRITICAL_SECTION csJobs_;
	std::list<CAsyncApiJob*> jobs_;
	std::list<CAsyncApiJob*> finished_;
	CAsyncApiJob*	curJob_;

	int		WorkerThread();
	CAsyncApiJob*	 GetNextJob();
	void		 ProcessJob(CAsyncApiJob* job);

  private:	
	// make copy constructor and assignment operator inaccessible
	CAsyncApiWorker(const CAsyncApiWorker& rhs);
	CAsyncApiWorker& operator=(const CAsyncApiWorker& rhs);

  public:
	CAsyncApiWorker();
	~CAsyncApiWorker();
};

class CAsyncApiMgr
{
  public:
	enum { NUM_WORKER_THREADS = 4, };
	CAsyncApiWorker	workers_[NUM_WORKER_THREADS];

	void		AddJob(CAsyncApiJob* job);
	void		AddFrontJob(CAsyncApiJob* job);
	void		StartWorkers();
	
  public:
	CAsyncApiMgr();
	~CAsyncApiMgr();
	
	void		Tick();
	void		TerminateAll();
	void		GetStatus(char* text);
	int		GetActiveJobs();
	void		DumpJobs();
};
extern CAsyncApiMgr*	g_AsyncApiMgr;


//
//
// specific API calls
//
//

class CJobBanUser : public CAsyncApiJob
{
public:
	int		isPermaBan;
	char		BanReason[256];
	bool		sendGameblocksEvent;

public:
	CJobBanUser(const obj_ServerPlayer* plr) : CAsyncApiJob(plr)
	{
		sprintf(desc, "CJobBanUser[%d] %p", CustomerID, this);
		isPermaBan = 1; // always perma ban
		BanReason[0] = 0;
		sendGameblocksEvent = false;
	}

	int		Exec();
	void		OnSuccess();
};

class CJobProcessUserJoin : public CAsyncApiJob
{
	CServerUserProfile prof;
	int		GameJoinResult;
	bool		isServerHop;
	
	void		DetectServerHop();

  public:
	CJobProcessUserJoin(DWORD in_peerId) : CAsyncApiJob(in_peerId)
	{
		sprintf(desc, "CJobProcessUserJoin[%d] %p", CustomerID, this);
		JobMaxTries    = 1;	// no need to retry that job

		GameJoinResult = -1;
		isServerHop    = false;
	}

	int		Exec();
	void		OnSuccess();
};

class CJobUserPingGame : public CAsyncApiJob
{
  public:
  
  public:
	CJobUserPingGame(DWORD in_peerId) : CAsyncApiJob(in_peerId)
	{
		sprintf(desc, "CJobPingGame[%d] %p", CustomerID, this);
		JobMaxTries = 3;	// no need to retry that job
	}

	int		Exec();
	void		OnSuccess() {}
};

class CJobUserLeftGame : public CAsyncApiJob
{
  public:
	int		TimePlayed;
	int		GameMapId;
	DWORD		GameServerId;
  
  public:
	CJobUserLeftGame(DWORD in_peerId) : CAsyncApiJob(in_peerId)
	{
		sprintf(desc, "CJobUserLeftGame[%d] %p", CustomerID, this);
		JobMaxTries  = 3;	// no need to retry that job

		GameMapId    = 0;
		GameServerId = 0;
		TimePlayed   = 0;
	}

	int		Exec();
	void		OnSuccess() {}
};

class CJobUpdateChar : public CAsyncApiJob
{
  public:
	wiCharDataFull	CharData;
	wiCharDataFull	OldData;
	int		GD_Diff;	// Game Dollars difference from previous update
	int		ResWood;
	int		ResStone;
	int		ResMetal;

	const static int DISCONNECT_WAIT_TIME = 10; // time player must wait before actual disconnect (to prevent exiting from battlefield while fighting)
	bool		Disconnect;	// disconnect after update
  
  public:
	CJobUpdateChar(const obj_ServerPlayer* plr) : CAsyncApiJob(plr)
	{
		sprintf(desc, "CJobUpdateChar[%d] %p", CustomerID, this);
		JobMaxTries = 5;	// try to update for 50 sec
	}

	int		Exec();
	void		OnSuccess();
};

class CJobChangeBackpack : public CAsyncApiJob
{
  public:
	DWORD		BackpackID;
	int		BackpackSize;
	std::vector<wiInventoryItem> DroppedItems;
  
  public:
	CJobChangeBackpack(const obj_ServerPlayer* plr) : CAsyncApiJob(plr)
	{
		sprintf(desc, "CJobChangeBackpack[%d] %p", CustomerID, this);
	}

	int		Exec();
	void		OnSuccess();
};


class CJobAddLogInfo : public CAsyncApiJob
{
  public:
	int		CheatID;	// or 0 for normal logging messages
	DWORD		IP;

	char		Gamertag[128];
	char		Msg[2048];
	char		Data[4096];
  
  public:
	CJobAddLogInfo() : CAsyncApiJob()
	{
		sprintf(desc, "CJobAddLogInfo"); // do not modify - it's used for reentrancy checks
		JobMaxTries = 3;	// no need to retry that job
		Gamertag[0] = 0;
	}

	int		Exec();
	void		OnSuccess() {}
};

extern void AsyncJobAddServerLogInfo(int CheatID, DWORD CustomerID, DWORD CharID, const char* msg, const char* data, ...);

class CJobBuyItem : public CAsyncApiJob
{
  public:
	DWORD		ItemID;
	int		BuyIdx;
	int		SlotTo;		// backpack slot where item should be moved after buying
	
	int		out_OpAns;
	int		out_Balance;
	__int64		out_InventoryID;

	CJobBuyItem(const obj_ServerPlayer* plr) : CAsyncApiJob(plr)
	{
		sprintf(desc, "CJobBuyItem[%d] %p", CustomerID, this);
		JobMaxTries = 1;	// no need to retry that job

		ItemID = 0;
		BuyIdx = 0;
		SlotTo = -1;
	}

	int		Exec();
	void		OnSuccess();
};

class CJobBackpackFromInventory : public CAsyncApiJob
{
  public:
	__int64		InventoryID;
	int		SlotTo;
	int		Amount;
	
	int		out_OpAns;
	__int64		out_InventoryID;

	CJobBackpackFromInventory(const obj_ServerPlayer* plr) : CAsyncApiJob(plr)
	{
		sprintf(desc, "CJobBackpackFromInventory[%d] %p", CustomerID, this);
		InventoryID = 0;
		SlotTo      = -1;
	}

	int		Exec();
	void		OnSuccess();
};

class CJobBackpackToInventory : public CAsyncApiJob
{
  public:
	__int64		InventoryID;
	int		SlotFrom;
	int		Amount;
	
	int		out_OpAns;
	__int64		out_InventoryID;

	CJobBackpackToInventory(const obj_ServerPlayer* plr) : CAsyncApiJob(plr)
	{
		sprintf(desc, "CJobBackpackToInventory[%d] %p", CustomerID, this);
		SlotFrom = -1;
	}

	int		Exec();
	void		OnSuccess();
};

class CJobGetLootboxData : public CAsyncApiJob
{
  public:
	pugi::xml_document out_xmlFile;

  public:
	CJobGetLootboxData() : CAsyncApiJob()
	{
		sprintf(desc, "CJobGetLootboxData[%d] %p", CustomerID, this);
		JobMaxTries = 1;	// no need to retry that job, we'll get lootbox data on next update
	}

	int		Exec();
	void		OnSuccess();
};

class CJobUpdateMissionsData : public CAsyncApiJob
{
  public:
	std::string	MissionsData;
  
  public:
	CJobUpdateMissionsData(const obj_ServerPlayer* plr);

	int		Exec();
	void		OnSuccess() {}
};

class CJobUpdateCharData : public CAsyncApiJob
{
  public:
	std::string	XmlCharData;
  
  public:
	CJobUpdateCharData(const obj_ServerPlayer* plr);

	int		Exec();
	void		OnSuccess() {}
};

