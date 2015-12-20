#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "XMLHelpers.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"

#include "obj_ServerRepairBench.h"

IMPLEMENT_CLASS(obj_ServerRepairBench, "obj_RepairBench", "Object");
AUTOREGISTER_CLASS(obj_ServerRepairBench);

static std::vector<obj_ServerRepairBench*> s_RepairBenches;

bool obj_ServerRepairBench::isCloseToRepairBench(const r3dPoint3D& pos)
{
	float minDist = 9999999.0f;
	for(size_t i=0; i<s_RepairBenches.size(); ++i)
	{
		float d = (pos-s_RepairBenches[i]->GetPosition()).Length();
		if(d < minDist)
			minDist = d;
	}

	return minDist <= 5.0f;
}

obj_ServerRepairBench::obj_ServerRepairBench() 
{
}

obj_ServerRepairBench::~obj_ServerRepairBench()
{
}

BOOL obj_ServerRepairBench::OnCreate()
{
	s_RepairBenches.push_back(this);
	return parent::OnCreate();
}

BOOL obj_ServerRepairBench::OnDestroy()
{
	s_RepairBenches.erase(std::find(s_RepairBenches.begin(), s_RepairBenches.end(), this));
	return parent::OnDestroy();
}