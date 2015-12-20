#pragma once

#include "GameCommon.h"
#include "NetworkHelper.h"

class obj_ServerLockbox: public GameObject, INetworkHelper
{
	DECLARE_CLASS(obj_ServerLockbox, GameObject)
public:
	uint32_t	m_ItemID; // itemID of lockbox
	int		m_ObstacleId;
	float		m_Radius;
	DWORD		lockboxOwnerId;
	char		m_AccessCodeS[32];
	int		m_IsLocked;

	std::vector<wiInventoryItem> items; // all items in this lockbox
	uint32_t	maxItems; // max items this lockbox can hold

	int		nextInventoryID;

	void		setAccessCode(const char* newCodeS);
	
	// security lockdown list. per user.
	struct lock_s
	{
	  DWORD		CustomerID;
	  float		lockEndTime;
	  int		tries;
	};
	std::vector<lock_s> m_lockdowns;
	std::vector<lock_s> m_uses;		// used for tracking usage-per-seconds for lockboxes, lockEndTime used as lockbox opening time
	float		m_nextLockdownClear;

public:
	obj_ServerLockbox();
	~obj_ServerLockbox();

	virtual	BOOL	OnCreate();
	virtual	BOOL	OnDestroy();

	virtual	BOOL	Update();

	void		SendContentToPlayer(obj_ServerPlayer* plr);
	bool		IsLockdownActive(const obj_ServerPlayer* plr);
	void		SetLockdown(DWORD CustomerID);
	bool		IsLockboxAbused(const obj_ServerPlayer* plr);
	void		DestroyLockbox();
	void		UpdateServerData();

	wiInventoryItem* FindItemWithInvID(__int64 invID);
	bool		AddItemToLockbox(const wiInventoryItem& itm, int quantity);
	void		RemoveItemFromLockbox(__int64 invID, int amount);

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);

	int		GetServerObjectSerializationType() { return 1; } // static object
	void		INetworkHelper::LoadServerObjectData();
	void		INetworkHelper::SaveServerObjectData();
};
