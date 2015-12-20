#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"

#include "obj_ServerBuilding.h"

IMPLEMENT_CLASS(obj_ServerBuilding, "obj_Building", "Object");
AUTOREGISTER_CLASS(obj_ServerBuilding);

obj_ServerBuilding::obj_ServerBuilding()
{
}

obj_ServerBuilding::~obj_ServerBuilding()
{
}

BOOL obj_ServerBuilding::Load(const char* name)
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
	 
BOOL obj_ServerBuilding::OnCreate()
{
	parent::OnCreate();

	return 1;
}

