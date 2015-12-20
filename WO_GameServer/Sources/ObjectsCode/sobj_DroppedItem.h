#pragma once

#include "GameCommon.h"
#include "NetworkHelper.h"

class obj_DroppedItem : public GameObject, INetworkHelper
{
	DECLARE_CLASS(obj_DroppedItem, GameObject)

public:
	wiInventoryItem	m_Item;
	float expireAirDrop;

public:
	obj_DroppedItem();
	~obj_DroppedItem();
	
	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();

	int GetItemDefault(int i);
	r3dPoint3D	AirDropPos;
	bool	m_IsOnTerrain;
	BYTE	m_FirstTime;
	float	ExpireFirstTime;
	uint32_t		m_LootBoxID1;
	uint32_t		m_LootBoxID2;
	uint32_t		m_LootBoxID3;
	uint32_t		m_LootBoxID4;
	uint32_t		m_LootBoxID5;
	uint32_t		m_LootBoxID6;
	int			m_DefaultItems;
	
	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);

	// we no longer save dropped items. int		GetServerObjectSerializationType() { return 2; } // hibernating object
	void		INetworkHelper::LoadServerObjectData();
	void		INetworkHelper::SaveServerObjectData();
};
