//=========================================================================
//	Module: obj_ServerMissionStateObject.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================


#include "r3dPCH.h"
#include "r3d.h"

#include "obj_ServerMissionStateObject.h"
#include "../obj_ServerPlayer.h"

#ifdef MISSIONS
namespace Mission
{

IMPLEMENT_CLASS(obj_ServerMissionStateObject, "obj_MissionStateObject", "Object");
AUTOREGISTER_CLASS(obj_ServerMissionStateObject);


obj_ServerMissionStateObject::obj_ServerMissionStateObject()
{
}

obj_ServerMissionStateObject::~obj_ServerMissionStateObject()
{
}

BOOL obj_ServerMissionStateObject::Init()
{
	return TRUE;
}

BOOL obj_ServerMissionStateObject::Load(const char* name)
{
	if(parent::Load(name))
	{
		// set temp bbox
		r3dBoundBox bboxLocal ;
		bboxLocal.Org	= r3dPoint3D( -0.5, -0.5, -0.5 );
		bboxLocal.Size	= r3dPoint3D( +0.5, +0.5, +0.5 );
		SetBBoxLocal( bboxLocal );

		return TRUE;
	}

	return FALSE;
}

BOOL obj_ServerMissionStateObject::OnCreate()
{
	return parent::OnCreate();
}

BOOL obj_ServerMissionStateObject::OnDestroy()
{
	return parent::OnDestroy();
}

BOOL obj_ServerMissionStateObject::Update()
{
	return parent::Update();
}

void obj_ServerMissionStateObject::ReadSerializedData(pugi::xml_node& node)
{
	GameObject::ReadSerializedData(node);

	pugi::xml_node objNode = node.child("StateParams");
	uint32_t numMissions = objNode.attribute("numMissionsSelected").as_uint();
	for(uint32_t i=0; i<numMissions; ++i)
	{
		char tempStr[32];
		sprintf(tempStr, "m%d", i);
		pugi::xml_node mNode = objNode.child(tempStr);
		r3d_assert(!mNode.empty());
		m_missionIDs.insert(mNode.attribute("id").as_uint());
	}
}

} // namespace Mission
#endif