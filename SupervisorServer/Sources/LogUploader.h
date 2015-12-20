#pragma once

#include "CkByteData.h"
#include "CkHttp.h"

class CFileData
{
  public:
	char		fname[MAX_PATH];
	CkByteData	data;
	int		size;
	
  public:
	CFileData() { fname[0] = 0; size = 0;}
	CFileData(const char* in_fname, bool compress);
	bool		IsValid() const { return fname[0] > 0; }
	void		DeleteFile();
};

class CLogUploader
{
  private:
	CkHttp		http_;
  
	static unsigned int __stdcall UploadThreadEntry(LPVOID in);
	DWORD		UploadThread();
	HANDLE		uploadThread_;

	int		ScanForNewLogs();

	int		UploadPicture(const char* jpgName);
	int		UploadDump(const char* dmpName);
	int		UploadDataToServer(CFileData& logdata, CFileData& dmpdata, CFileData& jpgdata);
  
  public:
	CLogUploader();
	~CLogUploader();
	
	void		Start();
	void		Stop(bool waitForUploadFinish);
};

extern CLogUploader	gLogUploader;
