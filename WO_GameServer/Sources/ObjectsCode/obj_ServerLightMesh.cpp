#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"

#include "ObjectsCode/obj_ServerLightMesh.h"
#include "ObjectsCode/obj_ServerPlayer.h"

#include "ServerGameLogic.h"

IMPLEMENT_CLASS(obj_ServerLightMesh, "obj_LightMesh", "Object");
AUTOREGISTER_CLASS(obj_ServerLightMesh);

obj_ServerLightMesh::obj_ServerLightMesh()
{
	bLightOn = true;
}

obj_ServerLightMesh::~obj_ServerLightMesh()
{
}

BOOL obj_ServerLightMesh::Load(const char *fname)
{
	return parent::Load(fname);
}

BOOL obj_ServerLightMesh::OnCreate()
{
	parent::OnCreate();
	ObjFlags |= OBJFLAG_SkipCastRay;

	SetNetworkID(gServerLogic.GetFreeNetId());
	NetworkLocal = true;

	return 1;
}

BOOL obj_ServerLightMesh::OnDestroy()
{
	return parent::OnDestroy();
}

BOOL obj_ServerLightMesh::Update()
{
	parent::Update();

	return TRUE;
}

BOOL obj_ServerLightMesh::OnNetReceive(DWORD EventID, const void* packetData, int packetSize)
{
	// no packets to process
	return FALSE;
}

void obj_ServerLightMesh::SyncLightStatus(DWORD peerId)
{
	if(bLightOn)
		return;

	PKT_S2C_LightMeshStatus_s n;

	if(peerId == -1)
		gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	else
		gServerLogic.p2pSendToPeer(peerId, this, &n, sizeof(n));
}

DefaultPacket* obj_ServerLightMesh::NetGetCreatePacket(int* out_size)
{
	// only send this if light is off
	if(!bLightOn)
	{
		static PKT_S2C_LightMeshStatus_s n;
		*out_size = sizeof(n);
		return &n;
	}
	else
	{
		*out_size = 0;
		return NULL;
	}
}