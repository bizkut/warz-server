#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"

#include "ObjectsCode/obj_ServerPlayerSpawnPoint.h"

#include "ServerGameLogic.h"

ControlPointsMgr	gCPMgr;

IMPLEMENT_CLASS(obj_ServerPlayerSpawnPoint, "obj_PlayerSpawnPoint", "Object");
AUTOREGISTER_CLASS(obj_ServerPlayerSpawnPoint);

obj_ServerPlayerSpawnPoint::obj_ServerPlayerSpawnPoint()
{
}

obj_ServerPlayerSpawnPoint::~obj_ServerPlayerSpawnPoint()
{
}

BOOL obj_ServerPlayerSpawnPoint::OnCreate()
{
	parent::OnCreate();
	
	gCPMgr.RegisterControlPoint(this);

	return 1;
}

BOOL obj_ServerPlayerSpawnPoint::Load(const char *fname)
{
	// skip mesh loading
	if(!GameObject::Load(fname)) 
		return FALSE;

	return TRUE;
}

