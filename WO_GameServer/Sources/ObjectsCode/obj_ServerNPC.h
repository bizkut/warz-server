#pragma once

#include "GameCommon.h"

class obj_ServerStoreNPC : public GameObject
{
	DECLARE_CLASS(obj_ServerStoreNPC, GameObject)

public:
	obj_ServerStoreNPC();
	virtual ~obj_ServerStoreNPC();

	virtual BOOL	OnCreate();
};


class obj_ServerVaultNPC : public GameObject
{
	DECLARE_CLASS(obj_ServerVaultNPC, GameObject)

public:
	obj_ServerVaultNPC();
	virtual ~obj_ServerVaultNPC();

	virtual BOOL	OnCreate();
};

class ServerNPCMgr
{
public:
	enum { MAX_NPC = 64 }; // 64 should be more than enough, if not, will redo into vector
	GameObject* NPCs_[MAX_NPC];
	int		numNPC_;

	void RegisterNPC(GameObject* npc) 
	{
		r3d_assert(numNPC_ < MAX_NPC);
		NPCs_[numNPC_++] = npc;
	}

	bool isCloseToNPC(const r3dPoint3D& pos, const char* className);

public:
	ServerNPCMgr() { numNPC_ = 0; }
	~ServerNPCMgr() {}
};

extern	ServerNPCMgr gServerNPCMngr;
