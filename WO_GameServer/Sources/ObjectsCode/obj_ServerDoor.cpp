//=========================================================================
//	Module: obj_Door.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================


#include "r3dPCH.h"
#include "r3d.h"

#include "obj_ServerDoor.h"
#include "obj_ServerPlayer.h"
#include "../../GameEngine/ai/AutodeskNav/AutodeskNavMesh.h"

IMPLEMENT_CLASS(obj_Door, "obj_Door", "Object");
AUTOREGISTER_CLASS(obj_Door);

obj_Door::obj_Door()
{
	m_Healt = 30.0f;
	DoorIsDestroy = 0;
	TimeToRespawnDoor = 0;
	m_OpenDoor = 0;
}

obj_Door::~obj_Door()
{
}

BOOL obj_Door::OnCreate()
{
	SetNetworkID(gServerLogic.GetFreeNetId());

	int Number = rand() % 100;
	m_OpenDoor = (BYTE)Number % 2 == 0?1:0;

	SetPosition(pos);
	SetRotationVector(rot);

	gServerLogic.NetRegisterObjectToPeers(this);

	return parent::OnCreate();
}

BOOL obj_Door::OnDestroy()
{
	return parent::OnDestroy();
}

BOOL obj_Door::Update()
{
	if (DoorIsDestroy == 1 && TimeToRespawnDoor!=0)
	{
		if ((r3dGetTime() - TimeToRespawnDoor) > (60*20)) // Respawn on 20 minutes
		{
			TimeToRespawnDoor = 0;
			RespawnDoor();
		}
	}
	return parent::Update();
}

DefaultPacket* obj_Door::NetGetCreatePacket(int* out_size)
{
	static PKT_S2C_SendDataDoor_s n;
	n.DoorID			= toP2pNetId(GetNetworkID());
	n.m_OpenDoor		= m_OpenDoor;
	n.DoorIsDestroy		= DoorIsDestroy;
	n.ExecuteParticle	= 0;
	//r3dOutToLog("###### entra 1 DoorID %i, OpenDoor %i, IsDestroy %i, ExcuteParticle %i\n",toP2pNetId(GetNetworkID()),n.m_OpenDoor,n.DoorIsDestroy,n.ExecuteParticle);
	*out_size = sizeof(n);
	return &n;
}

#undef DEFINE_GAMEOBJ_PACKET_HANDLER
#define DEFINE_GAMEOBJ_PACKET_HANDLER(xxx) \
	case xxx: { \
		const xxx##_s&n = *(xxx##_s*)packetData; \
		if(packetSize != sizeof(n)) { \
			r3dOutToLog("!!!!errror!!!! %s packetSize %d != %d\n", #xxx, packetSize, sizeof(n)); \
			return TRUE; \
		} \
		OnNetPacket(n); \
		return TRUE; \
	}
BOOL obj_Door::OnNetReceive(DWORD EventID, const void* packetData, int packetSize)
{
	switch(EventID)
	{
		DEFINE_GAMEOBJ_PACKET_HANDLER(PKT_S2C_SendDataDoor);
	}
	return TRUE;
}
#undef DEFINE_GAMEOBJ_PACKET_HANDLER

void obj_Door::ReadSerializedData(pugi::xml_node& node)
{
	GameObject::ReadSerializedData(node);
	pugi::xml_node cNode = node.child("Door");
	SelectDoor = cNode.attribute("Number").as_int();

	pugi::xml_node cNodePos = node.child("position");
	pos.x = cNodePos.attribute("x").as_float();
	pos.y = cNodePos.attribute("y").as_float();
	pos.z = cNodePos.attribute("z").as_float();

	pugi::xml_node cNodeRot = node.child("gameObject");
	rot.x = cNodeRot.child("rotation").attribute("x").as_float();
	rot.y = 0.0f;
	rot.z = 0.0f;
}

BOOL obj_Door::DamageDoor(float Damage)
{
	m_Healt-=Damage;
	if (m_Healt<0)
	{
		m_Healt=0;
		DoorIsDestroy=1;

		PKT_S2C_SendDataDoor_s n;
		n.DoorID			= toP2pNetId(GetNetworkID());
		n.m_OpenDoor		= m_OpenDoor;
		n.DoorIsDestroy		= DoorIsDestroy;
		n.ExecuteParticle   = 1;
		//r3dOutToLog("###### entra 4 DoorID %i, OpenDoor %i, IsDestroy %i, ExcuteParticle %i\n",toP2pNetId(GetNetworkID()),n.m_OpenDoor,n.DoorIsDestroy,n.ExecuteParticle);

		gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
		TimeToRespawnDoor = r3dGetTime();
		return TRUE;
	}
	return FALSE;
}

void obj_Door::RespawnDoor()
{
	m_Healt=10.0f;
	DoorIsDestroy = 0;
	int Number = rand() % 100;
	m_OpenDoor = (BYTE)Number % 2 == 0?1:0;

	PKT_S2C_SendDataDoor_s n;
	n.DoorID			= toP2pNetId(GetNetworkID());
	n.m_OpenDoor		= m_OpenDoor;
	n.DoorIsDestroy		= DoorIsDestroy;
	n.ExecuteParticle   = 0;
	//r3dOutToLog("###### entra 2 DoorID %i, OpenDoor %i, IsDestroy %i, ExcuteParticle %i\n",toP2pNetId(GetNetworkID()),n.m_OpenDoor,n.DoorIsDestroy,n.ExecuteParticle);

	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
}

void obj_Door::RelayPacket(const DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered)
{
	for(int i=0; i<gServerLogic.MAX_PEERS_COUNT; i++)
	{
		if(gServerLogic.peers_[i].status_ >= gServerLogic.PEER_VALIDATED1)
		{
			gServerLogic.RelayPacket(gServerLogic.peers_[i].CharID, this, packetData, packetSize, guaranteedAndOrdered);
		}
	}
}

void obj_Door::OnNetPacket(const PKT_S2C_SendDataDoor_s& n)
{
	m_OpenDoor = n.m_OpenDoor;

	PKT_S2C_SendDataDoor_s n2;
	n2.DoorID			= n.DoorID;
	n2.m_OpenDoor		= m_OpenDoor;
	n2.DoorIsDestroy	= DoorIsDestroy;
	n2.ExecuteParticle  = 0;
	//r3dOutToLog("###### entra 3 DoorID %i, OpenDoor %i, IsDestroy %i, ExcuteParticle %i\n",n.DoorID,n.m_OpenDoor,n.DoorIsDestroy,n.ExecuteParticle);

	gServerLogic.p2pBroadcastToActive(this, &n2, sizeof(n2));

	//RelayPacket(&n, sizeof(n), true);
}

void obj_Door::LoadServerObjectData()
{
	// Do Nothing!
}

void obj_Door::SaveServerObjectData()
{
	// Do Nothing!
}
