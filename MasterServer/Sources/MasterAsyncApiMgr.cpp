#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"

#include "MasterAsyncApiMgr.h"
#include "MasterGameServer.h"

#include "../../EclipseStudio/Sources/backend/WOBackendAPI.h"

char* g_ServerApiKey = "Fg5jaBgj3uy3ja";

CMSAsyncApiMgr*	g_MSAsyncApiMgr = NULL;

CMSAsyncApiJob::CMSAsyncApiJob()
{
	ResultCode   = 0;
}

CMSAsyncApiWorker::CMSAsyncApiWorker()
{
	InitializeCriticalSection(&csJobs_);

	idx_        = -1;
	hThread     = NULL;

	curJob_     = NULL;
	hStartEvt_  = CreateEvent(NULL, FALSE, FALSE, NULL);
	needToExit_ = false;
}

CMSAsyncApiWorker::~CMSAsyncApiWorker()
{
	r3d_assert(jobs_.size() == 0);
	r3d_assert(curJob_ == NULL);
	DeleteCriticalSection(&csJobs_);
}

static unsigned int WINAPI CMSAsyncApiWorker_WorkerThread(void* in_ptr)
{
	return ((CMSAsyncApiWorker*)in_ptr)->WorkerThread();
}

CMSAsyncApiJob* CMSAsyncApiWorker::GetNextJob()
{
	r3dCSHolder cs1(csJobs_);
	if(jobs_.size() == 0)
		return NULL;

	CMSAsyncApiJob* job = jobs_.front();
	jobs_.pop_front();
	
	return job;
}

void CMSAsyncApiWorker::ProcessJob(CMSAsyncApiJob* job)
{
	r3d_assert(curJob_ == NULL);
	curJob_ = job;

	// exec it
	#ifdef _DEBUG
	r3dOutToLog("CMSAsyncApiWorker %d executing %s\n", idx_, job->desc);
	#endif

	try
	{
		job->ResultCode = job->Exec();
	}
	catch(const char* msg)
	{
		r3dOutToLog("!!!! CMSAsyncApiWorker %d job crashed: %s\n", idx_, msg);
		job->ResultCode = 99;
	}

	#ifdef _DEBUG
	r3dOutToLog("CMSAsyncApiWorker %d finished %s\n", idx_, job->desc);
	#endif

	// place it to finish queue
	{
		r3dCSHolder cs2(csJobs_);

		curJob_ = NULL;
		finished_.push_back(job);
	}
}

int CMSAsyncApiWorker::WorkerThread()
{
	while(true)
	{
		::WaitForSingleObject(hStartEvt_, INFINITE);

		while(CMSAsyncApiJob* job = GetNextJob())
		{
			ProcessJob(job);
		}
		
		r3dCSHolder cs1(csJobs_);
		if(jobs_.size() == 0 && needToExit_)
		{
			r3dOutToLog("CMSAsyncApiWorker %d finished\n", idx_);
			return 0;
		}
	}
}

CMSAsyncApiMgr::CMSAsyncApiMgr()
{
	StartWorkers();
}

CMSAsyncApiMgr::~CMSAsyncApiMgr()
{
	r3dOutToLog("CMSAsyncApiMgr stopping\n"); CLOG_INDENT;
	
	HANDLE hs[NUM_WORKER_THREADS] = {0};
	
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		hs[i] = workers_[i].hThread;
	
		workers_[i].needToExit_ = true;
		::SetEvent(workers_[i].hStartEvt_);
	}
	
	// wait for thread finishing
	DWORD w0 = ::WaitForMultipleObjects(NUM_WORKER_THREADS, hs, TRUE, 10000);
	if(w0 == WAIT_TIMEOUT) 
	{
		r3dOutToLog("!!!! some API thread wasn't finished\n");
	}
}

void CMSAsyncApiMgr::StartWorkers()
{
	r3dOutToLog("CMSAsyncApiMgr starting\n");

	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CMSAsyncApiWorker& worker = workers_[i];
		
		worker.idx_    = i;
		worker.hThread = (HANDLE)_beginthreadex(NULL, 0, CMSAsyncApiWorker_WorkerThread, &worker, 0, NULL);
	}
}

void CMSAsyncApiMgr::TerminateAll()
{
	r3dOutToLog("CMSAsyncApiMgr terminating all threads\n");

	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		::TerminateThread(workers_[i].hThread, 2);
	}
}

void CMSAsyncApiMgr::GetStatus(char* text)
{
	*text = 0;
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		r3dCSHolder cs1(workers_[i].csJobs_);
		sprintf(text + strlen(text), "%d ", workers_[i].jobs_.size());
	}
}

void CMSAsyncApiMgr::AddJob(CMSAsyncApiJob* job)
{
	//r3dOutToLog("AddJob %s\n", job->desc); CLOG_INDENT;
	
	// search for less used worker
	int    workerIdx = 0;
	size_t minJobs   = 99;
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CMSAsyncApiWorker& worker = workers_[i];
		r3dCSHolder cs1(worker.csJobs_);
		
		if(worker.jobs_.size() < minJobs)
		{
			minJobs   = worker.jobs_.size();
			workerIdx = i;
		}
	}
	
	// add job to worker
	CMSAsyncApiWorker& worker = workers_[workerIdx];
	r3dCSHolder cs1(worker.csJobs_);
	worker.jobs_.push_back(job);

	::SetEvent(worker.hStartEvt_);
	
	return;
}

void CMSAsyncApiMgr::Tick()
{
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CMSAsyncApiWorker& worker = workers_[i];
		
		r3dCSHolder cs1(worker.csJobs_);
		
		for(std::list<CMSAsyncApiJob*>::iterator it = worker.finished_.begin(); it != worker.finished_.end(); ++it)
		{
			CMSAsyncApiJob* job = *it;

			// log that job is failed
			if(job->ResultCode != 0)
			{
				r3dOutToLog("CMSAsyncApiWorker %d job %s failed\n", worker.idx_, job->desc);
			}
			else
			{
				job->OnSuccess();
			}

			delete job;
			continue;
		}

		worker.finished_.clear();
	}
}

//
//
// api classes
//
//

int CMSJobGetServerList::Exec()
{
	CWOBackendReq req("api_ServersMgr.aspx");
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("func",   "srv_list");
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobGetServerList failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}

	pugi::xml_document xmlFile;
	req.ParseXML(xmlFile);
	
	pugi::xml_node xmlServerList = xmlFile.child("slist");
	pugi::xml_node xmlItem = xmlServerList.first_child();
	while(!xmlItem.empty())
	{
		CMasterServerConfig::rentGame_s srv;
		srv.ginfo.gameServerId = xmlItem.attribute("ID").as_uint();
		srv.ginfo.region       = xmlItem.attribute("ServerRegion").as_uint();
		srv.ginfo.flags        = xmlItem.attribute("ServerFlags").as_uint();
		srv.ginfo.mapId        = xmlItem.attribute("ServerMap").as_uint();
		srv.ginfo.maxPlayers   = xmlItem.attribute("ServerSlots").as_uint();
		srv.ginfo.EnableGass = true;
		srv.ginfo.isPVE		 = 0;
		if(srv.ginfo.IsGameworld())
			srv.ginfo.channel = 3; // private server
		else
			srv.ginfo.channel = 5; // stronghold
		r3dscpy(srv.ginfo.name,  xmlItem.attribute("ServerName").value());
		r3dscpy(srv.pwd,         xmlItem.attribute("ServerPwd").value());
		srv.OwnerCustomerID    = xmlItem.attribute("OwnerCustomerID").as_uint();
		srv.AdminKey           = xmlItem.attribute("AdminKey").as_uint();
		srv.RentHours          = xmlItem.attribute("RentHours").as_uint();
		srv.WorkHours          = xmlItem.attribute("WorkHours").as_uint();
		out_rentGames.push_back(srv);

		xmlItem = xmlItem.next_sibling();
	}
	
	return 0;
}

void CMSJobGetServerList::OnSuccess()
{
	r3dOutToLog("CMSJobGetServerList: got %d servers\n", out_rentGames.size());

	// replace current game config with our one
	gServerConfig->rentGames_ = out_rentGames;
	gServerConfig->OnGameListUpdated();
	
	gMasterGameServer.CloseExpiredGames();
	return;
}

int CMSJobTickAllServers::Exec()
{
	CWOBackendReq req("api_ServersMgr.aspx");
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("func",   "srv_tickall");
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CMSJobTickAllServers failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	return 0;
}
