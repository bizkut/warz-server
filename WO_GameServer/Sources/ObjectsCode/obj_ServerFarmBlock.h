#pragma once

#include "GameCommon.h"
#include "NetworkHelper.h"

class obj_ServerFarmBlock: public GameObject, INetworkHelper
{
	DECLARE_CLASS(obj_ServerFarmBlock, GameObject)
public:
	uint32_t	m_ItemID; // itemID of farm
	int		m_ObstacleId;
	float		m_Radius;

	float		m_TimeUntilRipe;
	float		m_ActivateTrap;

public:
	obj_ServerFarmBlock();
	~obj_ServerFarmBlock();

	virtual	BOOL	OnCreate();
	virtual	BOOL	OnDestroy();

	virtual	BOOL	Update();

	void		TryToHarvest(obj_ServerPlayer* plr);

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);

	int		GetServerObjectSerializationType() { return 1; } // static object
	void		INetworkHelper::LoadServerObjectData();
	void		INetworkHelper::SaveServerObjectData();
};
