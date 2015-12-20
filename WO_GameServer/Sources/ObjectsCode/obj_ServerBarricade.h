#pragma once

#include "GameCommon.h"
#include "NetworkHelper.h"

class obj_ServerBarricade: public GameObject, protected INetworkHelper
{
	DECLARE_CLASS(obj_ServerBarricade, GameObject)
public:
	uint32_t	m_ItemID;
	int		m_ObstacleId;
	float		m_Radius;
	float		m_Health;
	float		m_ActivateTrap;
	
	static std::vector<obj_ServerBarricade*> allBarricades;

public:
	obj_ServerBarricade();
	virtual ~obj_ServerBarricade();

	virtual	BOOL		OnCreate();
	virtual	BOOL		OnDestroy();

	virtual	BOOL		Update();

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);

	int		GetServerObjectSerializationType() { return 1; } // static object
	void		INetworkHelper::LoadServerObjectData();
	void		INetworkHelper::SaveServerObjectData();

	void	DoDamage(float dmg);
	int		GetDamageForVehicle();
	void	DestroyBarricade();
	void	SearchPlayers();
	void	CheckTouch();
};

class obj_StrongholdServerBarricade : public obj_ServerBarricade
{
	DECLARE_CLASS(obj_StrongholdServerBarricade, obj_ServerBarricade)
public:
	obj_StrongholdServerBarricade();
	virtual ~obj_StrongholdServerBarricade();
};

class obj_ConstructorServerBarricade : public obj_ServerBarricade
{
	DECLARE_CLASS(obj_ConstructorServerBarricade, obj_ServerBarricade)
public:
	obj_ConstructorServerBarricade();
	virtual ~obj_ConstructorServerBarricade();
};