#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "XMLHelpers.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"

#include "obj_ServerPostBox.h"

IMPLEMENT_CLASS(obj_ServerPostBox, "obj_PostBox", "Object");
AUTOREGISTER_CLASS(obj_ServerPostBox);

PostBoxesMgr gPostBoxesMngr;

obj_ServerPostBox::obj_ServerPostBox()
{
	useRadius = 2.0f;
}

obj_ServerPostBox::~obj_ServerPostBox()
{
}

BOOL obj_ServerPostBox::OnCreate()
{
	parent::OnCreate();

	gPostBoxesMngr.RegisterPostBox(this);
	return 1;
}

// copy from client version
void obj_ServerPostBox::ReadSerializedData(pugi::xml_node& node)
{
	parent::ReadSerializedData(node);
	pugi::xml_node objNode = node.child("post_box");
	GetXMLVal("useRadius", objNode, &useRadius);
}
