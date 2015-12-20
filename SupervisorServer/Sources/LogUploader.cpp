#include "r3dPCH.h"
#include "r3d.h"

#include "CkHttpRequest.h"
#include "CkHttpResponse.h"
#include "CkByteData.h"
#include "CkGZip.h"

#include "LogUploader.h"
#include "SupervisorServer.h"
#include "SupervisorConfig.h"

CLogUploader	gLogUploader;

static	const char*	gDomainBaseUrl= "/WarZ/api/";
// ptumik: switched to non SSL due to random connection issues on SSL port
static	int		gDomainPort   = 80;//443;
static	bool		gDomainUseSSL = false;//true;
// see CLogUploader::Start for dev mode override


CFileData::CFileData(const char* in_fname, bool compress)
{
	fname[0] = 0;
	size     = 0;
	
	char fullfname[MAX_PATH];
	sprintf(fullfname, "logss\\%s", in_fname);
	if(!data.loadFile(fullfname)) {
		r3dOutToLog("!!! can't open file %s\n", fullfname);
		return;
	}
	size = data.getSize();

	if(compress)
	{
		// compress them
		CkGzip gzip;
		if(gzip.UnlockComponent("ARKTOSZIP_cRvE6e7mpSqD") == false)
			r3dError("failed to unlock gzip");

		CkByteData cdata;
		if(!gzip.CompressMemory(data, cdata)) {
			r3dOutToLog("!!! can't compress file %s\n", in_fname);
			return;
		}
		
		data = cdata;
	}
	r3dscpy(fname, in_fname);
}

void CFileData::DeleteFile()
{
	if(fname[0] == 0)
		return;
		
	char fullfname[MAX_PATH];
	sprintf(fullfname, "logss\\%s", fname);
	
	::DeleteFile(fullfname);
}

CLogUploader::CLogUploader()
{
	uploadThread_ = NULL;

	bool success = http_.UnlockComponent("ARKTOSHttp_decCLPWFQXmU");
	if(!success || !http_.IsUnlocked()) {
		r3dError("unable to unlock http component", MB_OK);
		return;
	}
	http_.put_ConnectTimeout(30);
	http_.put_ReadTimeout(60);
}

CLogUploader::~CLogUploader()
{
	Stop(false);
}

void CLogUploader::Start()
{
	if(gSupervisorConfig->uploadLogs_ == 2) 
	{
		r3dOutToLog("LogUploader: Working in DEV mode\n");

		g_api_ip->SetString("localhost");
		gDomainBaseUrl= "/WarZ/api/";
		gDomainPort   = 80;
		gDomainUseSSL = false;
	}

	// create upload thread
	uploadThread_ = (HANDLE)_beginthreadex(NULL, 0,	&UploadThreadEntry, this, 0, NULL);

	return;
}

void CLogUploader::Stop(bool waitForUploadFinish)
{
	if(uploadThread_ == NULL)
		return;
		
	r3dOutToLog("LogUploader: stopping\n");
	
	//TODO: waitForUploadFinish

	TerminateThread(uploadThread_, 0);
	uploadThread_ = NULL;
}

int CLogUploader::ScanForNewLogs()
{
	WIN32_FIND_DATA ffblk;
	HANDLE h;

	// scan for pictures
	h = FindFirstFile("logss\\*.jpg", &ffblk);
	if(h != INVALID_HANDLE_VALUE) do 
	{
		r3dOutToLog("LogUploader: jpg %s\n", ffblk.cFileName);
		if(!UploadPicture(ffblk.cFileName))
		{
			return 0;
		}
	} while(FindNextFile(h, &ffblk) != 0);
	FindClose(h);

	// scan for dumps
	h = FindFirstFile("logss\\*.dmp", &ffblk);
	if(h != INVALID_HANDLE_VALUE) do 
	{
		r3dOutToLog("LogUploader: dump %s\n", ffblk.cFileName);
		if(!UploadDump(ffblk.cFileName))
		{
			return 0;
		}
	} while(FindNextFile(h, &ffblk) != 0);
	FindClose(h);
	
	return 1;
}

unsigned int __stdcall CLogUploader::UploadThreadEntry(LPVOID in)
{
	CLogUploader* This = (CLogUploader*)in;
	return This->UploadThread();
}

int CLogUploader::UploadDataToServer(CFileData& logdata, CFileData& dmpdata, CFileData& jpgdata)
{
	float t1 = r3dGetTime();

	char strSize[64];

	r3dOutToLog("LogUploader: %d bytes\n", logdata.data.getSize() + dmpdata.data.getSize() + jpgdata.data.getSize());
	
	CkHttpRequest req;
	char fullUrl[512];
	sprintf(fullUrl, "%s%s", gDomainBaseUrl, "api_SrvUploadLogFile.aspx");

	req.UseUpload();
	req.put_Path(fullUrl);
	req.AddParam("skey1", "Fg5jaBgj3uy3ja");

	// add 3 potential files
	if(logdata.IsValid())
	{
		req.AddBytesForUpload("logFile", logdata.fname, logdata.data);
		sprintf(strSize, "%d", logdata.size);
		req.AddParam("logSize", strSize);
	}

	if(dmpdata.IsValid())
	{
		req.AddBytesForUpload("dmpFile", dmpdata.fname, dmpdata.data);
		sprintf(strSize, "%d", dmpdata.size);
		req.AddParam("dmpSize", strSize);
	}

	if(jpgdata.IsValid())
	{
		req.AddBytesForUpload("jpgFile", jpgdata.fname, jpgdata.data);
		sprintf(strSize, "%d", jpgdata.size);
		req.AddParam("jpgSize", strSize);
	}

	// start upload
	CkHttpResponse* resp = NULL;
	resp = http_.SynchronousRequest(g_api_ip->GetString(), gDomainPort, gDomainUseSSL, req);
	if(resp == NULL) {
		r3dOutToLog("LogUploader: timeout %s\n", http_.lastErrorText());
		return 0;
	}
	
	if(resp->get_StatusCode() != 200) {
		r3dOutToLog("LogUploader: http%d\n", resp->get_StatusCode());
		//r3dOutToLog("%s\n", resp->bodyStr());
		SAFE_DELETE(resp);
		return 0;
	}
	
	const char* bodyStr = resp->bodyStr();
	if(stricmp(bodyStr, "WO_0") != 0) {
		r3dOutToLog("LogUploader: !!! failed %s\n", bodyStr);
		SAFE_DELETE(resp);
		return 0;
	}
	
	SAFE_DELETE(resp);

	float t2 = r3dGetTime();
	r3dOutToLog("LogUploader: finish, %f sec\n", t2 - t1);
	return 1;
}


int CLogUploader::UploadPicture(const char* jpgName)
{
	::Sleep(200); //hack: allow file to be writed and closed

	CFileData ldata;
	CFileData ddata;
	CFileData jdata(jpgName, false);
	if(!UploadDataToServer(ldata, ddata, jdata))
		return 0;

	jdata.DeleteFile();
	return 1;
}

int CLogUploader::UploadDump(const char* dmpName)
{
	::Sleep(500); //hack: allow file to be writed and closed
	
	// generate log file name - must have same name as .dmp
	char logName[MAX_PATH];
	r3dscpy(logName, dmpName);
	r3d_assert(strlen(logName) > 4);
	r3dscpy(logName + strlen(logName) - 3, "txt");

	CFileData ldata(logName, true);
	CFileData ddata(dmpName, true);
	CFileData jdata;

	if(!UploadDataToServer(ldata, ddata, jdata))
		return 0;

	// upload success - delete crash, keep log
	ddata.DeleteFile();
	return 1;
}

DWORD CLogUploader::UploadThread()
{
	bool finished_ = false;
	while(!finished_)
	{
		try
		{
			if(!ScanForNewLogs())
			{
				r3dOutToLog("LogUploader: fatal error, sleeping for 5 min\n");
				::Sleep(5 * 60 * 1000);
			}
			::Sleep(500);

			continue;
		}
		catch(const char* err)
		{
			r3dOutToLog("CLogUploader: failed %s\n", err);
		}
	}

	return 0;
}