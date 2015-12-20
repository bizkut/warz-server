//=========================================================================
//	Module: MissionTrigger.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

#include "GameCommon.h"
#include "NetworkHelper.h"

//-------------------------------------------------------------------------

namespace Mission
{

class obj_MissionTrigger : public GameObject, public INetworkHelper
{
	DECLARE_CLASS(obj_MissionTrigger, GameObject)
public:
	// we keep as much mission trigger data as possible inside INetworkHelper::srvObjParams_
	//  missionTitleStringID - Var1
	//  missionNameStringID  - Var2
	//  missionDescStringID  - Var3
	uint32_t	m_missionID;
	bool		m_isRegistered;

public:
	obj_MissionTrigger();
	~obj_MissionTrigger();
	
	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();

	const char* 	GetMissionTitleStringID();
	const char* 	GetMissionNameStringID();
	const char* 	GetMissionDescStringID();
	void			SetMissionTitleStringID( const char* stringID );
	void			SetMissionNameStringID( const char* stringID );
	void			SetMissionDescStringID( const char* stringID );

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	virtual DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);
	void			NetShowMissionTrigger(DWORD peerId);

	virtual void	ReadSerializedData(pugi::xml_node& node);

	int			GetServerObjectSerializationType() { return 1; } // static object
	void		INetworkHelper::LoadServerObjectData();
	void		INetworkHelper::SaveServerObjectData();
};

} // namespace Mission
#endif
