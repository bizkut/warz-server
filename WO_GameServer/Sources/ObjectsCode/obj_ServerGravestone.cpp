//=========================================================================
//	Module: obj_ServerGravestone.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================


#include "r3dPCH.h"
#include "r3d.h"

#include "obj_ServerGravestone.h"
#include "obj_ServerPlayer.h"

const static float	Lifetime = 3 * 60 * 60;  // 3 hours of realtime
const static float  DEV_EVENT_LIFETIME = 15 * 60;

IMPLEMENT_CLASS(obj_ServerGravestone, "obj_ServerGravestone", "Object");
AUTOREGISTER_CLASS(obj_ServerGravestone);

obj_ServerGravestone::obj_ServerGravestone()
{
}

obj_ServerGravestone::~obj_ServerGravestone()
{
}

BOOL obj_ServerGravestone::Init(obj_ServerPlayer* targetPlr)
{
	SetNetworkID(gServerLogic.GetFreeNetId());
	NetworkLocal = true;
	srvObjParams_.CharID	= targetPlr->killedBy << 2 | u_random( 3 );
	if( targetPlr->killedBy == KilledBy_Player )
		srvObjParams_.Var1 = targetPlr->aggressor;
	else
		srvObjParams_.Var1 = "";
	srvObjParams_.Var2		= targetPlr->userName;

	// raycast down to earth in case world was changed
	r3dPoint3D pos = gServerLogic.AdjustPositionToFloor(targetPlr->GetPosition());
	SetPosition(pos);

	return OnCreate();
}

BOOL obj_ServerGravestone::OnCreate()
{
	r3d_assert(NetworkLocal);
	r3d_assert(GetNetworkID());

	srvObjParams_.CreateTime	= r3dGetTime();

	float expireTime = r3dGetTime() + Lifetime;

#ifdef DISABLE_GI_ACCESS_FOR_DEV_EVENT_SERVER
	if (gServerLogic.ginfo_.gameServerId == 148353
		// for testing in dev environment
		//|| gServerLogic.ginfo_.gameServerId==11
		)
		expireTime = r3dGetTime() + DEV_EVENT_LIFETIME;
#endif

	srvObjParams_.ExpireTime	= expireTime;

	gServerLogic.NetRegisterObjectToPeers(this);

	return parent::OnCreate();
}

BOOL obj_ServerGravestone::OnDestroy()
{
	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	
	return parent::OnDestroy();
}

BOOL obj_ServerGravestone::Update()
{
	// Has the lifetime expired?
	float curTime = r3dGetTime();
	if( srvObjParams_.ExpireTime < curTime )
		setActiveFlag(0);

	return parent::Update();
}

DefaultPacket* obj_ServerGravestone::NetGetCreatePacket(int* out_size)
{
	static PKT_S2C_CreateGravestone_s n;
	n.spawnID			= toP2pNetId(GetNetworkID());
	n.pos				= GetPosition();
	n.GravestoneInfo	= (BYTE)srvObjParams_.CharID;
	r3dscpy(n.Aggressor, srvObjParams_.Var1.c_str());
	r3dscpy(n.Victim, srvObjParams_.Var2.c_str());
	
	*out_size = sizeof(n);
	return &n;
}

void obj_ServerGravestone::LoadServerObjectData()
{
	// Do Nothing!
}

void obj_ServerGravestone::SaveServerObjectData()
{
	// Do Nothing!
}
