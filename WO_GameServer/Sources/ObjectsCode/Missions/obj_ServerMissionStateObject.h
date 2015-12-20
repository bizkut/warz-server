//=========================================================================
//	Module: obj_ServerMissionStateObject.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

#include <unordered_set>
#include "NetworkHelper.h"

namespace Mission
{

class obj_ServerMissionStateObject : public GameObject
{
	obj_ServerMissionStateObject(const obj_ServerMissionStateObject& stateObject) { }
	const obj_ServerMissionStateObject& operator=(const obj_ServerMissionStateObject& stateObject) { }

	DECLARE_CLASS(obj_ServerMissionStateObject, GameObject)

public:
	std::tr1::unordered_set<uint32_t>	m_missionIDs;

public:
	obj_ServerMissionStateObject();
	~obj_ServerMissionStateObject();

	BOOL			Init();

	virtual int		IsStatic() { return true; }
	virtual BOOL	Load(const char* name);
	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();

	virtual	void	ReadSerializedData( pugi::xml_node& node );
};

} // namespace Mission
#endif