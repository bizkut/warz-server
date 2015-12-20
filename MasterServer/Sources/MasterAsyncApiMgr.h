#pragma once

#include "MasterServerConfig.h"

class CMSAsyncApiJob
{
  public:
	char		desc[156];
  
	int		ResultCode;
	
  public:
	CMSAsyncApiJob();
virtual	~CMSAsyncApiJob() { }
	
virtual	int		Exec() = NULL;
virtual	void		OnSuccess() = NULL;
};

class CMSAsyncApiWorker
{
  public:
	int		idx_;
	HANDLE		hThread;
	
	HANDLE		hStartEvt_;
	bool		needToExit_;

	CRITICAL_SECTION csJobs_;
	std::list<CMSAsyncApiJob*> jobs_;
	std::list<CMSAsyncApiJob*> finished_;
	CMSAsyncApiJob*	curJob_;

	int		WorkerThread();
	CMSAsyncApiJob*	 GetNextJob();
	void		 ProcessJob(CMSAsyncApiJob* job);
  
  public:
	CMSAsyncApiWorker();
	~CMSAsyncApiWorker();
};

class CMSAsyncApiMgr
{
  public:
	enum { NUM_WORKER_THREADS = 4, };
	CMSAsyncApiWorker	workers_[NUM_WORKER_THREADS];

	void		AddJob(CMSAsyncApiJob* job);
	void		StartWorkers();
	
  public:
	CMSAsyncApiMgr();
	~CMSAsyncApiMgr();
	
	void		Tick();
	void		TerminateAll();
	void		GetStatus(char* text);
};
extern CMSAsyncApiMgr*	g_MSAsyncApiMgr;


//
//
// specific API calls
//
//

class CMSJobGetServerList : public CMSAsyncApiJob
{
public:
	std::vector<CMasterServerConfig::rentGame_s> out_rentGames;

public:
	CMSJobGetServerList() : CMSAsyncApiJob()
	{
		sprintf(desc, "CMSJobGetServerList %p", this);
	}

	int		Exec();
	void		OnSuccess();
};

class CMSJobTickAllServers : public CMSAsyncApiJob
{
public:

public:
	CMSJobTickAllServers() : CMSAsyncApiJob()
	{
		sprintf(desc, "CMSJobTickAllServers %p", this);
	}

	int		Exec();
	void		OnSuccess() {}
};
