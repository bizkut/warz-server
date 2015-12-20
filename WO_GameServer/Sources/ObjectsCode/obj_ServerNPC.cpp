#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "XMLHelpers.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"

#include "obj_ServerNPC.h"

IMPLEMENT_CLASS(obj_ServerStoreNPC, "obj_StoreNPC", "Object");
AUTOREGISTER_CLASS(obj_ServerStoreNPC);
IMPLEMENT_CLASS(obj_ServerVaultNPC, "obj_VaultNPC", "Object");
AUTOREGISTER_CLASS(obj_ServerVaultNPC);

ServerNPCMgr gServerNPCMngr;

bool ServerNPCMgr::isCloseToNPC(const r3dPoint3D& pos, const char* className)
{
	float minDist = 9999999.0f;
	for(int i=0; i<numNPC_; ++i)
	{
		float d = (pos-NPCs_[i]->GetPosition()).Length();
		if(d < minDist && NPCs_[i]->Class->Name == className)
			minDist = d;
	}

	return minDist <= 5.0f;
}

obj_ServerStoreNPC::obj_ServerStoreNPC() 
{
}

obj_ServerStoreNPC::~obj_ServerStoreNPC()
{
}

BOOL obj_ServerStoreNPC::OnCreate()
{
	parent::OnCreate();

	gServerNPCMngr.RegisterNPC(this);
	return 1;
}

obj_ServerVaultNPC::obj_ServerVaultNPC() 
{
}

obj_ServerVaultNPC::~obj_ServerVaultNPC()
{
}

BOOL obj_ServerVaultNPC::OnCreate()
{
	parent::OnCreate();

	gServerNPCMngr.RegisterNPC(this);
	return 1;
}
