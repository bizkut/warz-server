#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"

#include "sobj_Note.h"

IMPLEMENT_CLASS(obj_Note, "obj_Note", "Object");
AUTOREGISTER_CLASS(obj_Note);

static const int PLAYER_NOTE_EXPIRE_TIME = 24*60*60; // in minutes (24 hours of real time (not in-game) expire time)

obj_Note::obj_Note()
{
	srvObjParams_.ExpireTime = r3dGetTime() + PLAYER_NOTE_EXPIRE_TIME;
}

obj_Note::~obj_Note()
{
}

BOOL obj_Note::OnCreate()
{
	r3dOutToLog("obj_Note[%d] created. %.0f mins left, %s\n", srvObjParams_.ServerObjectID, (srvObjParams_.ExpireTime - r3dGetTime()) / 60.0f, srvObjParams_.Var1.c_str());
	
	r3d_assert(NetworkLocal);
	r3d_assert(GetNetworkID());
	//NoteID can be 0 for newly created notes
	
	// raycast down to earth in case world was changed
	r3dPoint3D pos = gServerLogic.AdjustPositionToFloor(GetPosition());
	SetPosition(pos);

	gServerLogic.NetRegisterObjectToPeers(this);
	
	return parent::OnCreate();
}

BOOL obj_Note::OnDestroy()
{
	//r3dOutToLog("obj_Note %p destroyed\n", this);

	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	
	return parent::OnDestroy();
}

BOOL obj_Note::Update()
{
	const float curTime = r3dGetTime();
	if(curTime > srvObjParams_.ExpireTime)
	{
		setActiveFlag(0);
	}

	return parent::Update();
}

DefaultPacket* obj_Note::NetGetCreatePacket(int* out_size)
{
	static PKT_S2C_CreateNote_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	n.pos     = GetPosition();
	
	*out_size = sizeof(n);
	return &n;
}

void obj_Note::NetSendNoteData(DWORD peerId)
{
	PKT_S2C_SetNoteData_s n;
	r3dscpy(n.TextFrom, srvObjParams_.Var1.c_str());
	r3dscpy(n.TextSubj, srvObjParams_.Var2.c_str());
	
	gServerLogic.p2pSendToPeer(peerId, this, &n, sizeof(n));
}

// we keep notes data inside srvObjParams_
void obj_Note::LoadServerObjectData() {}
void obj_Note::SaveServerObjectData() {}
