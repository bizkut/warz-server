#pragma once

#include "AsyncFuncs.h"
#include "NetworkHelper.h"

class CJobGetServerObjects : public CAsyncApiJob
{
  public:
	std::vector<IServerObject> AllObjects;
	void		parseObjects(pugi::xml_node xmlNote);
	
  public:
	CJobGetServerObjects() : CAsyncApiJob()
	{
		sprintf(desc, "CJobGetServerObjects %p", this);
	}

	int		Exec();
	void		OnSuccess();
};

class CJobAddServerObject : public CAsyncApiJob
{
  public:
	gobjid_t	GameObjID;
	IServerObject	prm;

	int		out_ObjectID;
  
  public:
	CJobAddServerObject(GameObject* obj);

	int		Exec();
	void		OnSuccess();
};

class CJobUpdateServerObject : public CAsyncApiJob
{
  public:
	IServerObject	prm;
  
  public:
	CJobUpdateServerObject(GameObject* obj);

	int		Exec();
	void		OnSuccess() {}
};

class CJobDeleteServerObject : public CAsyncApiJob
{
  public:
	DWORD		ServerObjectID;
  
  public:
	CJobDeleteServerObject(GameObject* obj);

	int		Exec();
	void		OnSuccess() {}
};

class CJobHibernate : public CAsyncApiJob
{
  public:
	std::vector<CAsyncApiJob*> jobs;
	int		ApiStartHibernate();

  public:
	CJobHibernate()
	{
		sprintf(desc, "CJobHibernate %p", this);
	}
	~CJobHibernate();

	int		Exec();
	void		OnSuccess();
	void		OnFailure();
};
