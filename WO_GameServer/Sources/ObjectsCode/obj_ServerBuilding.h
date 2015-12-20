#pragma once

#include "GameCommon.h"

class obj_ServerBuilding : public GameObject
{
	DECLARE_CLASS(obj_ServerBuilding, GameObject)

public:
	obj_ServerBuilding();
	~obj_ServerBuilding();

	virtual int		IsStatic() { return true; }

	virtual BOOL		Load(const char* name);
	virtual BOOL		OnCreate();
};
