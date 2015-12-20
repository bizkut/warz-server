#pragma once

#include "GameCommon.h"

class obj_ServerPostBox : public GameObject
{
	DECLARE_CLASS(obj_ServerPostBox, GameObject)

public:
	float		useRadius;

public:
	obj_ServerPostBox();
	virtual ~obj_ServerPostBox();

	virtual BOOL	OnCreate();
	virtual	void	ReadSerializedData(pugi::xml_node& node);
};

class PostBoxesMgr
{
public:
	enum { MAX_POST_BOXES = 32 }; // 32 should be more than enough, if not, will redo into vector
	obj_ServerPostBox* postBoxes_[MAX_POST_BOXES];
	int		numPostBoxes_;

	void RegisterPostBox(obj_ServerPostBox* pbox) 
	{
		r3d_assert(numPostBoxes_ < MAX_POST_BOXES);
		postBoxes_[numPostBoxes_++] = pbox;
	}

public:
	PostBoxesMgr() { numPostBoxes_ = 0; }
	~PostBoxesMgr() {}
};

extern	PostBoxesMgr gPostBoxesMngr;
