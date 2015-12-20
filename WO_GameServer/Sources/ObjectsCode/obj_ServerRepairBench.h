#pragma once

#include "GameCommon.h"

class obj_ServerRepairBench : public GameObject
{
	DECLARE_CLASS(obj_ServerRepairBench, GameObject)

public:
	obj_ServerRepairBench();
	virtual ~obj_ServerRepairBench();

	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();

	static bool isCloseToRepairBench(const r3dPoint3D& pos);
};
