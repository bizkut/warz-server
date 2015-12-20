#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"

#include "AsyncFuncs.h"
#include "ServerGameLogic.h"
#include "ObjectsCode/obj_ServerPlayer.h"
#include "ObjectsCode/WEAPONS/WeaponArmory.h"
#include "ObjectsCode/WEAPONS/WeaponConfig.h"
#include "ObjectsCode/Missions/MissionProgress.h"
#include "ObjectsCode/Missions/MissionActionsData.h"

#ifdef ENABLE_GAMEBLOCKS
#include "GBClient/Inc/GBClient.h"
#include "GBClient/Inc/GBReservedEvents.h"

extern GameBlocks::GBClient* g_GameBlocks_Client;
extern GameBlocks::GBPublicSourceId g_GameBlocks_ServerID;
#endif //ENABLE_GAMEBLOCKS


#include "../../EclipseStudio/Sources/backend/WOBackendAPI.h"

char* g_ServerApiKey = "Fg5jaBgj3uy3ja";

CAsyncApiMgr*	g_AsyncApiMgr = NULL;

const int DEFAULT_JOB_RETRIES = 18;	// our http timeout is 10sec, so max trying time per job will be 3min

CAsyncApiJob::CAsyncApiJob()
{
	peerId       = -1;
	CustomerID   = 0;
	SessionID    = 0;
	CharID       = 0;

	JobNumTries  = 0;
	JobMaxTries  = DEFAULT_JOB_RETRIES;
	ResultCode   = 0;
}

CAsyncApiJob::CAsyncApiJob(const obj_ServerPlayer* plr)
{
	peerId       = plr->peerId_;
	CustomerID   = plr->profile_.CustomerID;
	SessionID    = plr->profile_.SessionID;
	CharID       = plr->loadout_->LoadoutID;

	r3d_assert(CustomerID);
	r3d_assert(CharID);
	
	JobNumTries  = 0;
	JobMaxTries  = DEFAULT_JOB_RETRIES;
	ResultCode   = 0;
}

CAsyncApiJob::CAsyncApiJob(DWORD in_peerId)
{
	ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(in_peerId);

	peerId       = in_peerId;
	CustomerID   = peer.CustomerID;
	SessionID    = peer.SessionID;
	CharID       = peer.CharID;

	r3d_assert(CustomerID);
	r3d_assert(CharID);
	
	JobNumTries  = 0;
	JobMaxTries  = DEFAULT_JOB_RETRIES;
	ResultCode   = 0;
}

CAsyncApiWorker::CAsyncApiWorker()
{
	InitializeCriticalSection(&csJobs_);

	idx_        = -1;
	hThread     = NULL;

	curJob_     = NULL;
	hStartEvt_  = CreateEvent(NULL, FALSE, FALSE, NULL);
	needToExit_ = false;
}

CAsyncApiWorker::~CAsyncApiWorker()
{
	r3d_assert(jobs_.size() == 0);
	r3d_assert(curJob_ == NULL);
	DeleteCriticalSection(&csJobs_);
}

static unsigned int WINAPI CAsyncApiWorker_WorkerThread(void* in_ptr)
{
	return ((CAsyncApiWorker*)in_ptr)->WorkerThread();
}

CAsyncApiJob* CAsyncApiWorker::GetNextJob()
{
	r3dCSHolder cs1(csJobs_);
	if(jobs_.size() == 0)
		return NULL;

	CAsyncApiJob* job = jobs_.front();
	jobs_.pop_front();
	
	return job;
}

void CAsyncApiWorker::ProcessJob(CAsyncApiJob* job)
{
	r3d_assert(curJob_ == NULL);
	curJob_ = job;

	// exec it
	#ifdef _DEBUG
	r3dOutToLog("CAsyncApiWorker %d executing %s\n", idx_, job->desc);
	#endif

	try
	{
		for(job->JobNumTries=0; job->JobNumTries < job->JobMaxTries; job->JobNumTries++)
		{
			if(job->JobNumTries > 0) 
				r3dOutToLog("CAsyncApiWorker %d job %s failed, retrying %d/%d\n", idx_, job->desc, job->JobNumTries, job->JobMaxTries);

			// exec job and retry only on timeout (code 8)
			job->ResultCode = job->Exec();
			if(job->ResultCode != 8)
				break;
		}
	}
	catch(const char* msg)
	{
		r3dOutToLog("!!!! CAsyncApiWorker %d job crashed: %s\n", idx_, msg);
		job->ResultCode = 99;
	}

	#ifdef _DEBUG
	r3dOutToLog("CAsyncApiWorker %d finished %s\n", idx_, job->desc);
	#endif

	// if job is failed, remove all future jobs for that customerid
	if(job->ResultCode != 0 && job->CustomerID)
	{
		r3dOutToLog("!!!! CAsyncApiWorker %d job %s failed\n", idx_, job->desc);

		r3dCSHolder cs3(csJobs_);
		for(std::list<CAsyncApiJob*>::iterator it = jobs_.begin(); it != jobs_.end(); )
		{
			if((*it)->CustomerID == job->CustomerID)
			{
				r3dOutToLog("!!! CAsyncApiWorker %d removed future job %s\n", idx_, (*it)->desc);
				it = jobs_.erase(it);
				continue;
			}
				
			++it;
		}
	}
		
	// place it to finish queue
	{
		r3dCSHolder cs2(csJobs_);

		curJob_ = NULL;
		finished_.push_back(job);
	}
}

int CAsyncApiWorker::WorkerThread()
{
	r3dRandInitInTread rand_in_thread;
	
	while(true)
	{
		::WaitForSingleObject(hStartEvt_, INFINITE);

		while(CAsyncApiJob* job = GetNextJob())
		{
			ProcessJob(job);
		}
		
		r3dCSHolder cs1(csJobs_);
		if(jobs_.size() == 0 && needToExit_)
		{
			r3dOutToLog("CAsyncApiWorker %d finished\n", idx_);
			return 0;
		}
	}
}

CAsyncApiMgr::CAsyncApiMgr()
{
	StartWorkers();
}

CAsyncApiMgr::~CAsyncApiMgr()
{
	r3dOutToLog("CAsyncApiMgr stopping\n"); CLOG_INDENT;
	
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

void CAsyncApiMgr::StartWorkers()
{
	r3dOutToLog("CAsyncApiMgr starting\n");

	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CAsyncApiWorker& worker = workers_[i];
		
		worker.idx_    = i;
		worker.hThread = (HANDLE)_beginthreadex(NULL, 0, CAsyncApiWorker_WorkerThread, &worker, 0, NULL);
	}
}

void CAsyncApiMgr::TerminateAll()
{
	r3dOutToLog("CAsyncApiMgr terminating all threads\n");

	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		::TerminateThread(workers_[i].hThread, 2);
	}
}

void CAsyncApiMgr::GetStatus(char* text)
{
	*text = 0;
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		r3dCSHolder cs1(workers_[i].csJobs_);
		sprintf(text + strlen(text), "%d ", workers_[i].jobs_.size());
	}
}

int CAsyncApiMgr::GetActiveJobs()
{
	size_t n = 0;
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CAsyncApiWorker& wrk = workers_[i];
		r3dCSHolder cs1(wrk.csJobs_);

		n += wrk.jobs_.size();
		if(wrk.curJob_)
			n += 1;
		n += wrk.finished_.size();
	}

	return (size2int_t)n;
}

void CAsyncApiMgr::DumpJobs()
{
	r3dOutToLog("DumpJobs:\n"); CLOG_INDENT;
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CAsyncApiWorker& wrk = workers_[i];
		r3dCSHolder cs1(wrk.csJobs_);

		r3dOutToLog("Worker%d:\n", i); CLOG_INDENT;
		if(wrk.curJob_)
		{
			r3dOutToLog("CID:%d %s (ACTIVE)\n", wrk.curJob_->CustomerID, wrk.curJob_->desc);
		}
		for(std::list<CAsyncApiJob*>::iterator it = wrk.jobs_.begin(); it != wrk.jobs_.end(); ++it)
		{
			r3dOutToLog("CID:%d %s\n", (*it)->CustomerID, (*it)->desc);
		}
	}
}

void CAsyncApiMgr::AddJob(CAsyncApiJob* job)
{
	//r3dOutToLog("AddJob CID:%d %s\n", job->CustomerID, job->desc); CLOG_INDENT;
	
	// search for less used worker
	int    workerIdx = 0;
	size_t minJobs   = 9999;
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CAsyncApiWorker& worker = workers_[i];
		r3dCSHolder cs1(worker.csJobs_);
		
		// check if we already have job with that customerid
		if(job->CustomerID)
		{
			if(worker.curJob_ && worker.curJob_->CustomerID == job->CustomerID)
			{
				workerIdx = i;
				break;
			}
			bool found = false;
			for(std::list<CAsyncApiJob*>::iterator it = worker.jobs_.begin(); it != worker.jobs_.end(); ++it)
			{
				if((*it)->CustomerID == job->CustomerID)
				{
					workerIdx = i;
					found     = true;
					break;
				}
			}
			if(found)
				break;
		}
		
		size_t curJobs = worker.jobs_.size() + (worker.curJob_ != NULL) ? 1 : 0;
		if(curJobs < minJobs)
		{
			minJobs   = curJobs;
			workerIdx = i;
		}
	}
	
	// add job to worker
	CAsyncApiWorker& worker = workers_[workerIdx];
	r3dCSHolder cs1(worker.csJobs_);
	worker.jobs_.push_back(job);

	::SetEvent(worker.hStartEvt_);
	
	return;
}

void CAsyncApiMgr::AddFrontJob(CAsyncApiJob* job)
{
	r3dOutToLog("AddForcedJob CID:%d %s\n", job->CustomerID, job->desc); CLOG_INDENT;
	
	// add job to first worker and add it to the front
	CAsyncApiWorker& worker = workers_[0];
	r3dCSHolder cs1(worker.csJobs_);
	worker.jobs_.push_front(job);

	::SetEvent(worker.hStartEvt_);
	
	return;
}

void CAsyncApiMgr::Tick()
{
	for(int i=0; i<NUM_WORKER_THREADS; i++)
	{
		CAsyncApiWorker& worker = workers_[i];
		
		r3dCSHolder cs1(worker.csJobs_);
		
		for(std::list<CAsyncApiJob*>::iterator it = worker.finished_.begin(); it != worker.finished_.end(); ++it)
		{
			CAsyncApiJob* job = *it;

			// log that job is failed
			if(job->ResultCode != 0)
			{
				r3dOutToLog("CAsyncApiWorker %d job %s failed\n", worker.idx_, job->desc);
				
				if(strcmp(job->desc, "CJobAddLogInfo") != 0) // do NOT log it, or we can get into infinite loop
				{
					AsyncJobAddServerLogInfo(PKT_S2C_CheatWarning_s::CHEAT_Api, job->CustomerID, job->CharID,
						"Api", "job: %d %s", 
						job->ResultCode, job->desc);
				}
			}
			
			// peerless job
			if(job->peerId == -1)
			{
				if(job->ResultCode != 0)
					job->OnFailure();
				else
					job->OnSuccess();

				delete job;
				continue;
			}
			
			// player job, see if peer still have valid player
			ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(job->peerId);
			if(peer.CustomerID != job->CustomerID || peer.SessionID != job->SessionID)
			{
				//r3dOutToLog("CAsyncApiWorker %d job %s peer already disconnected\n", worker.idx_, job->desc);
			}
			else if(job->ResultCode != 0)
			{
				job->OnFailure();
				
				// prevent player from updating it's loadout after error
				if(peer.player) 
					peer.player->wasDisconnected_ = true;

				// player job failed, disconnect him
				gServerLogic.DisconnectPeer(job->peerId, false, "API FAILED");
			}
			else
			{
				job->OnSuccess();
			}

			delete job;
		}

		worker.finished_.clear();
	}
}

//
//
// api classes
//
//

int CJobProcessUserJoin::Exec()
{
	// step 1 - get profile (login credentials is checked here as well)
	prof.CustomerID = CustomerID;
	prof.SessionID  = SessionID;
	GameJoinResult = prof.GetProfile(CharID);
	if(GameJoinResult != 0)
	{
		r3dOutToLog("!!!! prof.GetProfile failed, code: %d\n", GameJoinResult);
		return 0; // return success so we can pass GameJoinResult to main server loop
	}

	// before joining server we need to detect if user server hopped.
	 DetectServerHop();

	// step 2, join the game
	CWOBackendReq req("api_SrvUserGame.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("func",   "join");
	req.AddParam("CharID", CharID);
	req.AddParam("g1",     gServerLogic.ginfo_.mapId);
	req.AddParam("g2",     gServerLogic.ginfo_.gameServerId);
	if(isServerHop)
	{
		// send his new position to fix issue when user disconnect before joining actual game.
		const wiCharDataFull& slot = prof.ProfileData.ArmorySlots[0];
		char strGamePos[256];
		sprintf(strGamePos, "%.3f %.3f %.3f %.0f", slot.GamePos.x, slot.GamePos.y, slot.GamePos.z, slot.GameDir);
		req.AddParam("gSH", strGamePos);
	}

	req.Issue();
	GameJoinResult = req.resultCode_;
	if(GameJoinResult != 0)
	{
		r3dOutToLog("!!!! api_SrvUserJoinedGame failed, code: %d\n", req.resultCode_);
		return 0; // return success so we can pass GameJoinResult to main server loop
	}
	
	return 0;
}

void CJobProcessUserJoin::DetectServerHop()
{
	// make sure we got a valid profile
	if(prof.ProfileData.NumSlots != 1) return;
	if(prof.ProfileData.ArmorySlots[0].LoadoutID != CharID) return;
	if(prof.ProfileData.ArmorySlots[0].Alive == 0) return;

	wiCharDataFull& loadout = prof.ProfileData.ArmorySlots[0];
	const GBGameInfo& ginfo = gServerLogic.ginfo_;

	// detect if player was server hopped
	const int SEC_TO_CONSIDER_SERVERHOP = 4*60*60;
	if(loadout.Alive == 1 &&
	   (loadout.GameFlags & wiCharDataFull::GAMEFLAG_NearPostBox) == 0 &&
	   loadout.GameMapId == ginfo.mapId &&
	   loadout.GameServerId != ginfo.gameServerId &&
	   loadout.SecFromLastGame > 0 && loadout.SecFromLastGame < SEC_TO_CONSIDER_SERVERHOP)
	{
		isServerHop = true;
		
		// get new hopped position
		r3dPoint3D spawnPos;
		float      spawnDir;
		gServerLogic.GetSpawnPositionAfterDeath(loadout.GamePos, &spawnPos, &spawnDir);

		// and put into profile
		loadout.GamePos = spawnPos;
		loadout.GameDir = spawnDir;
	}
}

void CJobProcessUserJoin::OnSuccess()
{
	// check if we still have correct peer
	ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(peerId);
	r3d_assert(peer.CustomerID == CustomerID && peer.SessionID == SessionID);
  
	// set profile and map API error code to haveprofile error
	peer.temp_profile = prof;
	peer.isServerHop  = isServerHop;
	switch(GameJoinResult)
	{
		case 0: peer.startGameAns = PKT_S2C_StartGameAns_s::RES_Ok; break;
		case 1: peer.startGameAns = PKT_S2C_StartGameAns_s::RES_InvalidLogin; break; // invalid login session
		case 7: peer.startGameAns = PKT_S2C_StartGameAns_s::RES_StillInGame;  break; // user already in game (ResultCode from [WZ_SRV_UserJoinedGame2])
		default:peer.startGameAns = PKT_S2C_StartGameAns_s::RES_Failed; break; // some other generic error
	}
	return;
}

int CJobUserPingGame::Exec()
{
	CWOBackendReq req("api_SrvUserGame.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("func",   "ping");
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobPingGame failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	return 0;
}

int CJobBanUser::Exec()
{
	r3d_assert(strlen(BanReason)>3);

	CWOBackendReq req("api_SrvBanUser.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("BanReason", BanReason);
	req.AddParam("IsPermaBan", isPermaBan);

	if(!req.Issue())
	{
		r3dOutToLog("!!!! api_SrvBanUser failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}

	return 0;
}

void CJobBanUser::OnSuccess()
{
#ifdef ENABLE_GAMEBLOCKS
	if(g_GameBlocks_Client && g_GameBlocks_Client->Connected() && sendGameblocksEvent)
	{
		g_GameBlocks_Client->PrepareEventForSending("BanConfirm", g_GameBlocks_ServerID, GameBlocks::GBPublicPlayerId(uint32_t(CustomerID)));
		g_GameBlocks_Client->SendEvent();
	}
#endif

	gServerLogic.DisconnectPeer(peerId, false, "banned by server, reason is in log");
}

int CJobUserLeftGame::Exec()
{
	r3d_assert(GameMapId);
	r3d_assert(GameServerId);

	CWOBackendReq req("api_SrvUserGame.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("func",   "leave");
	req.AddParam("CharID", CharID);

	req.AddParam("g1", GameMapId);
	req.AddParam("g2", GameServerId);
	req.AddParam("s1", TimePlayed);

	if(!req.Issue())
	{
		r3dOutToLog("!!!! api_SrvUserLeftGame failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}

	return 0;
}

static void UpdateChar_SetAttachments(CWOBackendReq& req, const wiCharDataFull& w)
{
	char attm[2][512];
	for(int i=0; i<2; i++) 
	{
		// should match arguments of parseCharAttachments
		sprintf(attm[i], "%d %d %d %d %d %d %d %d", 
			w.Attachment[i].attachments[0],
			w.Attachment[i].attachments[1],
			w.Attachment[i].attachments[2],
			w.Attachment[i].attachments[3],
			w.Attachment[i].attachments[4],
			w.Attachment[i].attachments[5],
			w.Attachment[i].attachments[6],
			w.Attachment[i].attachments[7]);
	}

	req.AddParam("attm1",  attm[0]);
	req.AddParam("attm2",  attm[1]);
}

static void UpdateChar_SetBackpack(CWOBackendReq& req, const wiCharDataFull& cur, const wiCharDataFull& old)
{
	int updIdx = 0;
	for(int i=0; i<wiCharDataFull::CHAR_MAX_BACKPACK_SIZE; i++) 
	{
		const wiInventoryItem& w1 = cur.Items[i];
		const wiInventoryItem& w2 = old.Items[i];
		if(w1 == w2)
			continue;
			
		//r3dOutToLog("backpack slot %d changed %d -> %d\n", i, w2.itemID, w1.itemID);
	
		char value[128];
		int op;
		if(w1.itemID && w2.itemID == 0)
			op = 0; // add
		else if(w1.itemID && w2.itemID)
			op = 1; // alter
		else if(w1.itemID == 0 && w2.itemID)
			op = 2; // remove
		else if(w1.itemID == 0 && w2.itemID == 0)
			continue; // slot was changed, but no items was modified
		sprintf(value, "%d %d %d %d %d %d %d", i, op, w1.itemID, w1.quantity, w1.Var1, w1.Var2, w1.Var3);
		
		char name[128];
		sprintf(name, "bp%d", updIdx++);
		req.AddParam(name, value);
	}
}


int CJobUpdateChar::Exec()
{
	float startTime = r3dGetTime();

	bool updateCharEnable=true;

#ifdef DISABLE_GI_ACCESS_ON_PTE_MAP
	if(gServerLogic.ginfo_.channel==6) // public test environment server, do not save any info about player, as those maps might have game breaking loot\changes, so let's not propagate them to the rest of the game
		updateCharEnable=false;
#endif
#ifdef DISABLE_GI_ACCESS_ON_PTE_STRONGHOLD_MAP
	if(gServerLogic.ginfo_.channel==6 && gServerLogic.ginfo_.mapId==GBGameInfo::MAPID_WZ_Cliffside) // public test environment server, do not save any info about player, as those maps might have game breaking loot\changes, so let's not propagate them to the rest of the game
		updateCharEnable=false;
#endif
#ifdef DISABLE_GI_ACCESS_FOR_CALI_SERVER
	if(gServerLogic.ginfo_.mapId==GBGameInfo::MAPID_WZ_California)
		updateCharEnable=false;
#endif
	
	if(updateCharEnable)
	{
		const wiCharDataFull& slot = CharData;

		pugi::xml_document xmlFile;
		CUserProfile::SaveCharData(slot, xmlFile);
		xml_string_writer xmlBuf;
		xmlFile.save(xmlBuf, "", pugi::format_raw);
		std::string XmlCharData = xmlBuf.out_;

		CWOBackendReq req("api_SrvCharUpdate.aspx");
		req.AddSessionInfo(CustomerID, SessionID);
		req.AddParam("skey1",  g_ServerApiKey);
		req.AddParam("CharID", slot.LoadoutID);

		// character status
		char strGamePos[256];
		sprintf(strGamePos, "%.3f %.3f %.3f %.0f", slot.GamePos.x, slot.GamePos.y, slot.GamePos.z, slot.GameDir);
		req.AddParam("s1",          slot.Alive);
		req.AddParam("s2",          strGamePos);
		req.AddParam("s3",          (int)slot.Health);
		req.AddParam("s4",          (int)slot.Hunger);
		req.AddParam("s5",          (int)slot.Thirst);
		req.AddParam("s6",          (int)slot.Toxic);
		req.AddParam("s7",          slot.Stats.TimePlayed);
		req.AddParam("s8",          slot.Stats.XP);
		req.AddParam("s9",          slot.Stats.Reputation);
		req.AddParam("sA",          slot.GameFlags);
		req.AddParam("sB",          GD_Diff);
		req.AddParam("sC",          XmlCharData.c_str());
		req.AddParam("map",         slot.GameMapId);
		req.AddParam("r1",          ResWood);
		req.AddParam("r2",          ResStone);
		req.AddParam("r3",          ResMetal);

		//r3dOutToLog("!!! Saved Pos: %s\n", strGamePos);

		UpdateChar_SetAttachments(req, slot);
		UpdateChar_SetBackpack(req, CharData, OldData);

		//trackable stats
		req.AddParam("ts00",        slot.Stats.KilledZombies);
		req.AddParam("ts01",        slot.Stats.KilledSurvivors);
		req.AddParam("ts02",        slot.Stats.KilledBandits);
		req.AddParam("ts03",        slot.Stats.VictorysHardGames);//gamehardcore
		req.AddParam("ts04",        0);
		req.AddParam("ts05",        0);

		if(!req.Issue())
		{
			r3dOutToLog("!!!! UpdateCharThread failed, code: %d, ans: %s\n", req.resultCode_, req.bodyStr_);
			return req.resultCode_;
		}
	}

	if(Disconnect)
	{
		float sleepTime = r3dGetTime() - startTime + DISCONNECT_WAIT_TIME;
		if(sleepTime > 0) 
			::Sleep((int)(sleepTime * 1000));
	}

	return 0;
}

void CJobUpdateChar::OnSuccess()
{
	//r3dOutToLog("@@ UpdateCharThread: finished %d\n", CustomerID);

	// check if we still have correct peer - player can be already disconnected at the moment
	ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(peerId);
	if(peer.CustomerID != CustomerID || peer.SessionID != peer.SessionID || !peer.player)
	{
		r3dOutToLog("!! CJobUpdateChar %d/%d %d/%d %d\n", peer.CustomerID, CustomerID, peer.SessionID, peer.SessionID, peer.status_);
		return;
	}

	// confirm player if needed disconnect
	if(Disconnect)
	{
		peer.player->wasDisconnected_ = true;
	
		PKT_C2S_DisconnectReq_s n;
		gServerLogic.p2pSendToPeer(peerId, peer.player, &n, sizeof(n));

		gServerLogic.DisconnectPeer(peerId, false, "finished disconnect");
	}
}

int CJobChangeBackpack::Exec()
{
#ifdef DISABLE_GI_ACCESS_ON_PTE_MAP
	if(gServerLogic.ginfo_.channel==6) // public test environment server, do not save any info about player, as those maps might have game breaking loot\changes, so let's not propagate them to the rest of the game
		return 0;
#endif
#ifdef DISABLE_GI_ACCESS_ON_PTE_STRONGHOLD_MAP
	if(gServerLogic.ginfo_.channel==6 && gServerLogic.ginfo_.mapId==GBGameInfo::MAPID_WZ_Cliffside) // public test environment server, do not save any info about player, as those maps might have game breaking loot\changes, so let's not propagate them to the rest of the game
		return 0;
#endif
#ifdef DISABLE_GI_ACCESS_FOR_CALI_SERVER
	if(gServerLogic.ginfo_.mapId==GBGameInfo::MAPID_WZ_California)
		return 0;
#endif

	CWOBackendReq req("api_CharBackpack.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("CharID", CharID);
	req.AddParam("skey1",  g_ServerApiKey);

	req.AddParam("op",  56);
	req.AddParam("v1",  BackpackID);
	req.AddParam("v2",  BackpackSize);
	req.AddParam("v3",  0);

	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! BackpackChangeThread failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	return 0;
}

void CJobChangeBackpack::OnSuccess()
{
	ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(peerId);
	if(peer.CustomerID != CustomerID || peer.SessionID != SessionID || !peer.player)
		return;
		
	peer.player->OnChangeBackpackSuccess(DroppedItems);
}

int CJobAddLogInfo::Exec()
{
	CWOBackendReq req("api_SrvAddLogInfo.aspx");
	req.AddParam("skey1", g_ServerApiKey);
	
	req.AddParam("s_id",    CustomerID);	// note - it CAN be ZERO for not-connected users
	req.AddParam("CharID",  CharID);
	req.AddParam("Gamertag",Gamertag);
	req.AddParam("GameSessionID", gServerLogic.ginfo_.gameServerId);
	req.AddParam("CheatID", CheatID);
	req.AddParam("Msg",     Msg);
	req.AddParam("Data",    Data);
	
	char ipstr[128];
	sprintf(ipstr, "%u", 	IP);
	req.AddParam("IP",      ipstr);

	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! AddLogInfoThread failed, code: %d\n", req.resultCode_);
		return 0;
	}

	return 0;
}

void AsyncJobAddServerLogInfo(int CheatID, DWORD CustomerID, DWORD CharID, const char* msg, const char* data, ...)
{
	va_list ap;
	va_start(ap, data);

	CJobAddLogInfo* job = new CJobAddLogInfo();
	job->CheatID    = CheatID;
	job->CustomerID = CustomerID;
	job->CharID     = CharID;
	job->IP         = gServerLogic.GetExternalIP();
	sprintf(job->Msg,  msg);
	StringCbVPrintfA(job->Data, sizeof(job->Data), data, ap);
	va_end(ap);

	g_AsyncApiMgr->AddFrontJob(job);
}


int CJobBuyItem::Exec()
{
	CWOBackendReq req("api_BuyItem3.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("ItemID", ItemID);
	req.AddParam("BuyIdx", BuyIdx);
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobBuyItem failed, code: %d\n", req.resultCode_);
		out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Fail;
		return 0;
	}

	int nargs = sscanf(req.bodyStr_, "%d %I64d", &out_Balance, &out_InventoryID);
	if(nargs != 2 || out_InventoryID == 0)
	{
		r3dOutToLog("!!!! CJobBuyItem failed, bad ans: %s\n", req.bodyStr_);
		out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Fail;
		return 0;
	}

	out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Success;
	return 0;
}

void CJobBuyItem::OnSuccess()
{
	// check if we still have correct peer - player can be already disconnected at the moment
	ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(peerId);
	if(peer.CustomerID != CustomerID || peer.SessionID != SessionID || !peer.player)
		return;
	
	peer.player->OnBuyItemCallback(this);
}

int CJobBackpackFromInventory::Exec()
{
	char strInventoryID[128];
	sprintf(strInventoryID, "%I64d", InventoryID);

	CWOBackendReq req("api_CharBackpack.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("CharID", CharID);
	req.AddParam("op",     53);		// inventory operation code - ApiBackpackFromInventory
	req.AddParam("v1",     strInventoryID);	// value 1
	req.AddParam("v2",     SlotTo);		// value 2
	req.AddParam("v3",     Amount);		// value 3
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobBackpackFromInventory failed, code: %d\n", req.resultCode_);
		out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Fail;
		return 0;
	}

	int nargs = sscanf(req.bodyStr_, "%I64d", &out_InventoryID);
	if(nargs != 1 || out_InventoryID == 0)
	{
		r3dOutToLog("!!!! CJobBackpackFromInventory failed, bad ans: %s\n", req.bodyStr_);
		out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Fail;
		return 0;
	}

	out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Success;
	return 0;
}

void CJobBackpackFromInventory::OnSuccess()
{
	// check if we still have correct peer - player can be already disconnected at the moment
	ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(peerId);
	if(peer.CustomerID != CustomerID || peer.SessionID != SessionID || !peer.player)
		return;
	
	peer.player->OnFromInventoryCallback(this);
}

int CJobBackpackToInventory::Exec()
{
	char strInventoryID[128];
	sprintf(strInventoryID, "%I64d", InventoryID);

	CWOBackendReq req("api_CharBackpack.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("CharID", CharID);
	req.AddParam("op",     52);		// inventory operation code - ApiBackpackToInventory
	req.AddParam("v1",     strInventoryID);	// value 1
	req.AddParam("v2",     SlotFrom);	// value 2
	req.AddParam("v3",     Amount);		// value 3
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobBackpackToInventory failed, code: %d\n", req.resultCode_);
		if(req.resultCode_ == 9)
			out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_NoInventorySpace;
		else
			out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Fail;
		return 0;
	}

	int nargs = sscanf(req.bodyStr_, "%I64d", &out_InventoryID);
	if(nargs != 1 || out_InventoryID == 0)
	{
		r3dOutToLog("!!!! CJobBackpackToInventory failed, bad ans: %s\n", req.bodyStr_);
		out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Fail;
		return 0;
	}

	out_OpAns = PKT_S2C_InventoryOpAns_s::ANS_Success;
	return 0;
}

void CJobBackpackToInventory::OnSuccess()
{
	// check if we still have correct peer - player can be already disconnected at the moment
	ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(peerId);
	if(peer.CustomerID != CustomerID || peer.SessionID != peer.SessionID || !peer.player)
		return;
	
	peer.player->OnToInventoryCallback(this);
}

int CJobGetLootboxData::Exec()
{
	CkHttp http;
	int success = http.UnlockComponent("ARKTOSHttp_decCLPWFQXmU");
	if (success != 1) 
		r3dError("Internal error!!!");

	CkHttpRequest req;
	req.UsePost();
	req.put_Path("/WarZ/api/php/api_GetLootBoxConfig.php");
	req.AddParam("serverkey", "Fg5jaBgj3uy3ja");

	// if api on localhost need to grab lootboxes from actual server
	const char* api_ip = g_api_ip->GetString();
	if(strcmp(api_ip, "localhost") == 0) api_ip = "127.0.0.1";
	CkHttpResponse* resp = http.SynchronousRequest(api_ip, 80, false, req);
	if(!resp)
	{
		r3dOutToLog("!!! timeout getting lootbox db\n");
		return 8;
	}
			
	// we can't use getBosyStr() because it'll fuckup characters inside UTF-8 xml
	CkByteData bodyData;
	resp->get_Body(bodyData);
	delete resp;
		
	// note, not _inplace function, fmlFile should have it own buffer
	pugi::xml_parse_result parseResult = out_xmlFile.load_buffer(
		(void*)bodyData.getBytes(), 
		bodyData.getSize(), 
		pugi::parse_default, 
		pugi::encoding_utf8);
	if(!parseResult)
	{
		r3dOutToLog("!!! Failed to parse lootbox db, error: %s", parseResult.description());
		return 9;
	}

	return 0;
}

void CJobGetLootboxData::OnSuccess()
{
	r3dOutToLog("Loot boxes data updated\n"); CLOG_INDENT;

	// detect loot type modifier from channel	
	int srvLootType = 0;
	if(gServerLogic.ginfo_.channel == 1) srvLootType = 1; // trial
	else if(gServerLogic.ginfo_.channel == 4) srvLootType = 2; // premium
	g_pWeaponArmory->updateLootBoxContent(out_xmlFile.child("LootBoxDB"), srvLootType);
}

CJobUpdateMissionsData::CJobUpdateMissionsData(const obj_ServerPlayer* plr)
  : CAsyncApiJob(plr)
{
	sprintf(desc, "CJobUpdateMissionsData[%d] %p", CustomerID, this);

	pugi::xml_document xmlFile;
	pugi::xml_node xmlMissions = xmlFile; // no need to add another level of indirection, this xml will be embedded into GetProfile()

#ifdef MISSIONS
	if( plr->loadout_ )
		CUserProfile::SaveMissionData( *plr->loadout_, xmlMissions );
#endif

	xml_string_writer xmlBuf;
	xmlFile.save(xmlBuf, "", pugi::format_raw);
	MissionsData = xmlBuf.out_;
}

int CJobUpdateMissionsData::Exec()
{
	CWOBackendReq req("api_SrvUpdateMissionsData.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("CharID", CharID);
	req.AddParam("md",     MissionsData.c_str());
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobUpdateMissionsData failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}

	//r3dOutToLog("%s\n", MissionsData.c_str());
	return 0;
}

CJobUpdateCharData::CJobUpdateCharData(const obj_ServerPlayer* plr)
  : CAsyncApiJob(plr)
{
	sprintf(desc, "CJobUpdateCharData[%d] %p", CustomerID, this);

	pugi::xml_document xmlFile;
	CUserProfile::SaveCharData(*plr->loadout_, xmlFile);
	
	xml_string_writer xmlBuf;
	xmlFile.save(xmlBuf, "", pugi::format_raw);
	XmlCharData = xmlBuf.out_;
}

int CJobUpdateCharData::Exec()
{
	CWOBackendReq req("api_SrvUpdateCharData.aspx");
	req.AddSessionInfo(CustomerID, SessionID);
	req.AddParam("skey1",  g_ServerApiKey);
	req.AddParam("CharID", CharID);
	req.AddParam("cd",     XmlCharData.c_str());
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobUpdateCharData failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}

	return 0;
}
