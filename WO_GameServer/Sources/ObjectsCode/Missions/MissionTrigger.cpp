//=========================================================================
//	Module: MissionTrigger.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"

#include "MissionTrigger.h"
#include "MissionManager.h"

#if defined(MISSIONS) && defined(MISSION_TRIGGERS)
namespace Mission
{

IMPLEMENT_CLASS(obj_MissionTrigger, "obj_MissionTrigger", "Object");
AUTOREGISTER_CLASS(obj_MissionTrigger);

obj_MissionTrigger::obj_MissionTrigger()
	: m_missionID( 0 )
	, m_isRegistered( false )
{
	serializeFile = SF_ServerData;
}

obj_MissionTrigger::~obj_MissionTrigger()
{
}

BOOL obj_MissionTrigger::OnCreate()
{
	return parent::OnCreate();
}

BOOL obj_MissionTrigger::OnDestroy()
{
	//r3dOutToLog("obj_MissionTrigger %p destroyed\n", this);

	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	
	return parent::OnDestroy();
}

BOOL obj_MissionTrigger::Update()
{
	if(gServerLogic.net_mapLoaded_LastNetID == 0) // do not register until server has fully loaded
		return TRUE;

	if( !m_isRegistered )
	{
		SetNetworkID(gServerLogic.GetFreeNetId());
		NetworkLocal = true;
		
		// raycast down to earth in case world was changed
		r3dPoint3D pos = gServerLogic.AdjustPositionToFloor(GetPosition());
		SetPosition(pos);

		r3dOutToLog("obj_MissionTrigger[%d] created. netID: %d  missionID: %d\n", srvObjParams_.ServerObjectID, GetNetworkID(), m_missionID);

		gServerLogic.NetRegisterObjectToPeers(this);
		m_isRegistered = true;
	}

	return parent::Update();
}

const char* obj_MissionTrigger::GetMissionTitleStringID()
{
	return srvObjParams_.Var1.c_str();
}

const char* obj_MissionTrigger::GetMissionNameStringID()
{
	return srvObjParams_.Var2.c_str();
}

const char* obj_MissionTrigger::GetMissionDescStringID()
{
	return srvObjParams_.Var3.c_str();
}

void obj_MissionTrigger::SetMissionTitleStringID( const char* stringID )
{
	srvObjParams_.Var1 = stringID;
}

void obj_MissionTrigger::SetMissionNameStringID( const char* stringID )
{
	srvObjParams_.Var2 = stringID;
}

void obj_MissionTrigger::SetMissionDescStringID( const char* stringID )
{
	srvObjParams_.Var3 = stringID;
}

DefaultPacket* obj_MissionTrigger::NetGetCreatePacket(int* out_size)
{
	r3d_assert( false && "Mission Triggers are disabled, since no UI exists for them atm." );
	return NULL;
	static PKT_S2C_CreateMissionTrigger_s n;
	n.spawnID	= toP2pNetId(GetNetworkID());
	n.pos		= GetPosition();
	n.missionID	= m_missionID;
	r3dscpy( n.missionTitleStringID, GetMissionTitleStringID() );
	r3dscpy( n.missionNameStringID, GetMissionNameStringID() );
	r3dscpy( n.missionDescStringID, GetMissionDescStringID() );
	
	*out_size = sizeof(n);
	return &n;
}

void obj_MissionTrigger::NetShowMissionTrigger(DWORD peerId)
{
	r3d_assert( false && "Mission Triggers are disabled, since no UI exists for them atm." );
	if( g_pMissionMgr->IsMissionActive( m_missionID ) )
	{
		PKT_S2C_ShowMissionTrigger_s n;
		
		gServerLogic.p2pSendToPeer(peerId, this, &n, sizeof(n));
	}
}

void obj_MissionTrigger::ReadSerializedData(pugi::xml_node& node)
{
	parent::ReadSerializedData( node );
	m_missionID = node.attribute("missionID").as_uint();
	SetMissionTitleStringID( node.attribute("titleID").value() );
	SetMissionNameStringID( node.attribute("nameID").value() );
	SetMissionDescStringID( node.attribute("descID").value() );

	r3d_assert(m_missionID > 0);
}

// we keep obj_MissionTrigger data inside srvObjParams_
void obj_MissionTrigger::LoadServerObjectData() {}
void obj_MissionTrigger::SaveServerObjectData() {}

} // namespace Mission
#endif