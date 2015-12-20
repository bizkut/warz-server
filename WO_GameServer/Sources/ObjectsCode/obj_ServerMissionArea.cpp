//=========================================================================
//	Module: obj_ServerMissionArea.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "obj_ServerMissionArea.h"
#include "GameCommon.h"

IMPLEMENT_CLASS(obj_MissionArea, "obj_MissionArea", "Object");
AUTOREGISTER_CLASS(obj_MissionArea);

obj_MissionArea::obj_MissionArea()
	: m_areaType( MissionAreaType::AABB )
	, m_extents( 1.0f, 1.0f, 1.0f )
{
	ObjTypeFlags |= OBJTYPE_GameplayItem;
	ObjFlags |= OBJFLAG_SkipCastRay | OBJFLAG_SkipDraw;
}

obj_MissionArea::~obj_MissionArea()
{
}

BOOL obj_MissionArea::OnCreate()
{
	return parent::OnCreate();
}

BOOL obj_MissionArea::Update()
{
	return parent::Update();
}

BOOL obj_MissionArea::OnDestroy()
{
	return parent::OnDestroy();
}

void obj_MissionArea::ReadSerializedData(pugi::xml_node& node)
{
	parent::ReadSerializedData( node );
	pugi::xml_node missionAreaNode = node.child("mission_area_parameters");
	int areaType = 0;
	while( areaType < MissionAreaType::MAX_AREA_TYPE - 1 &&
		   _tcsnicmp( missionAreaNode.attribute("type").value(),
					 MissionAreaType::MissionAreaTypeNames[ areaType ],
					 _tcslen( MissionAreaType::MissionAreaTypeNames[ areaType ] ) ) )
	{
					 ++areaType;
	}
	r3d_assert( areaType < (int)MissionAreaType::MAX_AREA_TYPE - 1 );
	m_areaType = (MissionAreaType::EMissionAreaType)(areaType + 1); // Enum starts at 1, not 0, so that the checkboxes will work properly in the editor.
	if( MissionAreaType::AABB == m_areaType )
	{
		m_extents.x = fabs(missionAreaNode.attribute("x").as_float());
		m_extents.y = fabs(missionAreaNode.attribute("y").as_float());
		m_extents.z = fabs(missionAreaNode.attribute("z").as_float());
	}
	else // ( MissionAreaType::Sphere == m_areaType )
	{
		m_extents.x = fabs(missionAreaNode.attribute("radius").as_float());
		m_extents.y = 0.0f;
		m_extents.z = 0.0f;
	}
}

BOOL obj_MissionArea::Contains( GameObject* obj )
{
	const r3dPoint3D& pos = GetPosition();
	const r3dPoint3D& objPos = obj->GetPosition();
	if( MissionAreaType::AABB == m_areaType )
	{
		return( objPos.x >= pos.x - m_extents.x && objPos.x <= pos.x + m_extents.x &&
				objPos.y >= pos.y - m_extents.y && objPos.y <= pos.y + m_extents.y &&
				objPos.z >= pos.z - m_extents.z && objPos.z <= pos.z + m_extents.z );
	}
	// else ( MissionAreaType::Sphere == m_areaType )
	// Sphere Mission Areas use only the x axis of the extent as the radius to check.
	r3dVector toObj = objPos - pos;
	return( toObj.LengthSq() <= ( m_extents.x * m_extents.x ) );
}

