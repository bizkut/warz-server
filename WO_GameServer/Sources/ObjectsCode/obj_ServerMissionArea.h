//=========================================================================
//	Module: obj_ServerMissionArea.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================
#pragma once

#include "GameCommon.h"

namespace MissionAreaType
{
	enum EMissionAreaType
	{
		 AABB	= 1
		,Sphere

		,MAX_AREA_TYPE // This Must Be Last
	};
	static const char* MissionAreaTypeNames[] = {
		 "AABB"
		,"Sphere"
	};
};

class obj_MissionArea : public GameObject
{
	DECLARE_CLASS(obj_MissionArea, GameObject)
public:
	obj_MissionArea();
	~obj_MissionArea();

	virtual	BOOL		OnCreate();
	virtual	BOOL		Update();
	virtual	BOOL		OnDestroy();
	virtual	void		ReadSerializedData(pugi::xml_node& node);

	BOOL				Contains( GameObject* obj );

public:
	MissionAreaType::EMissionAreaType	m_areaType;
	r3dVector	m_extents;
};