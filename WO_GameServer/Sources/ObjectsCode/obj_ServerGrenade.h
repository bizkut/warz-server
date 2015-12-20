#pragma once

#include "GameCommon.h"
#include "multiplayer/NetCellMover.h"
#include "NetworkHelper.h"

class WeaponConfig;

class obj_ServerGrenade: public GameObject, INetworkHelper
{
	DECLARE_CLASS(obj_ServerGrenade, GameObject)
public:
	const WeaponConfig*	m_Weapon;

	uint32_t			m_ItemID;
	float				m_CreationTime;
	float				m_HangTime;
	float				m_LastTimeUpdateCalled;
	float				m_AddedDelay;
	r3dPoint3D			m_CreationPos;
	r3dPoint3D			m_LastCollisionNormal;
	r3dPoint3D			m_AppliedVelocity;
	r3dVector			m_FireDirection;
	CNetCellMover		m_NetMover;
	gobjid_t			m_TrackedID;
	bool				m_bHadCollided;
	bool				m_bIgnoreNetMover;

	static std::vector<obj_ServerGrenade*> allGrenades;

public:
	obj_ServerGrenade();
	~obj_ServerGrenade();

	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual void	OnCollide(PhysicsCallbackObject *tobj, CollisionInfo &trace);
	virtual BOOL	Update();

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);

	void		INetworkHelper::LoadServerObjectData();
	void		INetworkHelper::SaveServerObjectData();

private:
	void			OnExplode();
};