#pragma once

#include "GameCommon.h"

class obj_AirDropSpawn : public GameObject
{
	DECLARE_CLASS(obj_AirDropSpawn, GameObject)

public:
	float		spawnRadius;
	uint32_t		m_LootBoxID1;
	uint32_t		m_LootBoxID2;
	uint32_t		m_LootBoxID3;
	uint32_t		m_LootBoxID4;
	uint32_t		m_LootBoxID5;
	uint32_t		m_LootBoxID6;
	int			m_DefaultItems;

public:
	obj_AirDropSpawn();
	~obj_AirDropSpawn();

	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();
	virtual	void	ReadSerializedData(pugi::xml_node& node);

};
