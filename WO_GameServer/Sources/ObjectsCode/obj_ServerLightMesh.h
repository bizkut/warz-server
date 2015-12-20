#pragma once

#include "GameCommon.h"
#include "NetworkHelper.h"

class obj_ServerLightMesh : public GameObject, INetworkHelper
{
	DECLARE_CLASS(obj_ServerLightMesh, GameObject)

public:
	obj_ServerLightMesh();
	~obj_ServerLightMesh();

	virtual	BOOL		Load(const char *name);

	virtual	BOOL		OnCreate();
	virtual	BOOL		OnDestroy();

	virtual	BOOL		Update();

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);
	void			SyncLightStatus(DWORD peerId);
	void		INetworkHelper::LoadServerObjectData() { r3dError("not implemented"); }
	void		INetworkHelper::SaveServerObjectData() { r3dError("not implemented"); }

	virtual BOOL		OnNetReceive(DWORD EventID, const void* packetData, int packetSize);

	bool				bLightOn;
};
