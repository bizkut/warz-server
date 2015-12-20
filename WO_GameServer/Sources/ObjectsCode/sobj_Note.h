#pragma once

#include "GameCommon.h"
#include "NetworkHelper.h"

class obj_Note : public GameObject, public INetworkHelper
{
	DECLARE_CLASS(obj_Note, GameObject)
public:
	// we keep all notes data inside INetworkHelper::srvObjParams_
	//  TextFrom - Var1
	//  TextSubj - Var2

public:
	obj_Note();
	~obj_Note();
	
	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();
	
	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);
	void		NetSendNoteData(DWORD peerId);

	int		GetServerObjectSerializationType() { return 1; } // static object
	void		INetworkHelper::LoadServerObjectData();
	void		INetworkHelper::SaveServerObjectData();
};
