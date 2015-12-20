//=========================================================================
//	Module: obj_ServerGravestone.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#include "NetworkHelper.h"

class obj_ServerPlayer;

class obj_ServerGravestone : public GameObject, INetworkHelper
{
	obj_ServerGravestone(const obj_ServerGravestone& gravestone) { }
	const obj_ServerGravestone& operator=(const obj_ServerGravestone& gravestone) { }

	DECLARE_CLASS(obj_ServerGravestone, GameObject)

public:
	// Enum must match obj_Gravestone::EKilledBy enums
	enum EKilledBy
	{
		 KilledBy_Self = 0
		,KilledBy_Zombie
		,KilledBy_Player

		,KilledBy_Unknown = 15
	};

	// Keep the gravestone params in INetworkHelper::srvObjParams_
	// CreateTime	- When was the gravestone created?
	// ExpireTime	- When will the gravestone disappear?
	// CharID		- (2 Lower Bits) Which version of the gravestone to use, (Next 4 Bits) Type who killed the player
	// Var1			- Who was the aggressor?
	// Var2			- Who was the victim?

public:
	obj_ServerGravestone();
	~obj_ServerGravestone();

	BOOL			Init(obj_ServerPlayer* targetPlr);

	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);

	void			INetworkHelper::LoadServerObjectData();
	void			INetworkHelper::SaveServerObjectData();
};