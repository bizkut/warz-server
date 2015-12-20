#pragma once

#include "AsyncFuncs.h"

class CJobGetSavedServerState : public CAsyncApiJob
{
  public:
	pugi::xml_document state;
	
	void		LoadItemSpawns(const pugi::xml_node& xmlNode);
	
  public:
	CJobGetSavedServerState()
	{
		sprintf(desc, "CJobGetSavedServerState %p", this);
	}

	int		Exec();
	void		OnSuccess();
};

class CJobSetSavedServerState : public CAsyncApiJob
{
  public:
	std::string	state;
	
	void		SaveItemSpawns(pugi::xml_node& xmlNode);
	
  public:
	CJobSetSavedServerState();

	int		Exec();
	void		OnSuccess() {}
};