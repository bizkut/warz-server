//=========================================================================
//	Module: obj_Door.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#include "NetworkHelper.h"

class obj_ServerPlayer;

class obj_Door : public GameObject, INetworkHelper
{
	obj_Door(const obj_Door& Door) { }
	const obj_Door& operator=(const obj_Door& Door) { }

	DECLARE_CLASS(obj_Door, GameObject)

public:
	// Enum must match obj_Gravestone::EKilledBy enums
	BYTE		m_OpenDoor;
	int			SelectDoor;
	r3dPoint3D  pos;
	r3dVector	rot;
	float		m_Healt;
	BYTE		DoorIsDestroy;
	float		TimeToRespawnDoor;

public:
	obj_Door();
	~obj_Door();

	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();
	virtual BOOL	DamageDoor(float Damage);
	void	RespawnDoor();

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);

	BOOL OnNetReceive(DWORD EventID, const void* packetData, int packetSize);
	void RelayPacket(const DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered = true);
	void        OnNetPacket(const PKT_S2C_SendDataDoor_s& n);

	virtual void ReadSerializedData(pugi::xml_node& node);

	void			INetworkHelper::LoadServerObjectData();
	void			INetworkHelper::SaveServerObjectData();
};