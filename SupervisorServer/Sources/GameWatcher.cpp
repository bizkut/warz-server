#include "r3dPCH.h"
#include "r3d.h"

#include "GameWatcher.h"
#include "SupervisorGameServer.h"

CGameWatcher::CGameWatcher()
{
  gameId    = 0;
  hProcess  = NULL;

  hMapFile  = NULL;
  info      = NULL;

  isStarted = 0;
}

CGameWatcher::~CGameWatcher()
{
  Reset();
}

void CGameWatcher::Reset()
{
  if(info) 
  {
    if(UnmapViewOfFile(info) == 0) {
      r3dOutToLog("!!!! failed to UnmapViewOfFile, err: %d\n", GetLastError());
    }
    info = NULL;
  }

  if(hMapFile) 
  {
    if(CloseHandle(hMapFile) == 0) {
      r3dOutToLog("!!!! failed to CloseHandle, err: %d\n", GetLastError());
    }
    hMapFile = NULL;
  }
  
  if(hProcess)
  {
    gSupervisorGameServer.OnGameProcessClosed(gameId);
  }
  
  gameId    = 0;
  hProcess  = 0;
  isStarted = 0;
}

void CGameWatcher::Init(DWORD in_GameId, __int64 in_sessionId)
{
  r3d_assert(in_GameId);
  r3d_assert(hProcess == NULL);
  r3d_assert(info == NULL);

  Reset();
	
  gameId    = in_GameId;
  sessionId = in_sessionId;
  lastTick  = r3dGetTime();
  
  char mmname[128];
  sprintf(mmname, "WZ_GServer_%08x", in_GameId);
	
  // Create the file mapping
  hMapFile = CreateFileMapping(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    sizeof(MMapGameInfo_s),
    mmname);
  if(hMapFile == NULL) {
    r3dError("unable to CreateFileMapping: %x\n", GetLastError());
  }

  // Map to the file
  info = (MMapGameInfo_s*)MapViewOfFile(
    hMapFile, // handle to map object
    FILE_MAP_ALL_ACCESS,	// read/write permission
    0,
    0,
    sizeof(MMapGameInfo_s)); 
  if(info == NULL) {
    r3dError("unable to MapViewOfFile: %x\n", GetLastError());
  }
  
  info->Reset(in_GameId, in_sessionId);
  return;
}

void CGameWatcher::CheckProcess()
{
#ifdef _DEBUG
  // don't run game watcher in debug
  return;
#endif

  if(hProcess == 0) 
  {
    r3d_assert(info == NULL);
    r3d_assert(gameId == 0);
    return;
  }
    
  r3d_assert(info);

  // if game expicitly reported that we crashed
  if(info->isCrashed)  {
    OnGameCrash(true);
    return;
  }

  // see if game terminated
  DWORD w0 = WaitForSingleObject(hProcess, 0);
  if(w0 != WAIT_TIMEOUT) 
  {
    CloseHandle(hProcess);
    Reset();
    return;
  }
  
  // wait for game start
  if(!info->isStarted)
    return;
    
  // init counter
  if(!isStarted) {
    r3dOutToLog("game 0x%x activated\n", gameId);
    isStarted = true;
    tickCount = info->tickCount;
    lastTick  = r3dGetTime();
  }
  
  // check for crash
  if(tickCount != info->tickCount) {
    tickCount = info->tickCount;
    lastTick  = r3dGetTime();
    return;
  }
  
  const float WAIT_CRASH_TIME = 5.0f;
  if(r3dGetTime() > lastTick + WAIT_CRASH_TIME) {
    OnGameCrash(false);
    return;
  }
  
  return;
}

void CGameWatcher::OnGameCrash(bool crash)
{
  if(crash)
    r3dOutToLog("!!! game 0x%x GS_%I64x CRASHED\n", gameId, sessionId);
  else
    r3dOutToLog("!!! game 0x%x GS_%I64x TIMEOUT\n", gameId, sessionId);
  
  Terminate();
}


void CGameWatcher::Terminate()
{
  if(hProcess == 0)
    return;
    
  r3dOutToLog("terminating game 0x%x\n", gameId);
  if(::TerminateProcess(hProcess, 0) == 0) {
    r3dOutToLog("!!!! unable to terminate game, err: %d\n", GetLastError());
  }

  Reset();
}
