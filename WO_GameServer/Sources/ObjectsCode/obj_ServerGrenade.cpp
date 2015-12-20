#include "r3dpch.h"
#include "r3d.h"

#include "obj_ServerGrenade.h"
#include "obj_ServerPlayer.h"
#include "ServerGameLogic.h"

#include "../ServerWeapons/ServerWeapon.h"

#include "ObjectsCode/Weapons/WeaponArmory.h"

#include "Async_ServerObjects.h"

#define FULL_AREA_EXPLOSION  360.0f

IMPLEMENT_CLASS(obj_ServerGrenade, "obj_ServerGrenade", "Object");
AUTOREGISTER_CLASS(obj_ServerGrenade);

std::vector<obj_ServerGrenade*> obj_ServerGrenade::allGrenades;

obj_ServerGrenade::obj_ServerGrenade()
	: m_ItemID( 0 )
	, m_CreationTime( 0.0f )
	, m_HangTime( 0.0f )
	, m_LastTimeUpdateCalled( 0.0f )
	, m_AddedDelay( 0.0f )
	, m_CreationPos( 0, 0, 0 )
	, m_LastCollisionNormal( 0, 1, 0 )
	, m_AppliedVelocity( 0, 0, 0 )
	, m_FireDirection( 0, 0, 0 )
	, m_NetMover(this, 1.0f / 10.0f, (float)PKT_C2C_MoveSetCell_s::PLAYER_CELL_RADIUS)
	, m_bHadCollided( false )
	, m_bIgnoreNetMover( false )
{
	m_isSerializable = false;

	allGrenades.push_back(this);

	ObjTypeFlags |= OBJTYPE_GameplayItem;
	ObjFlags |= OBJFLAG_SkipCastRay;
}

obj_ServerGrenade::~obj_ServerGrenade()
{
}

BOOL obj_ServerGrenade::OnCreate()
{
	const GameObject* owner = GameWorld().GetObject(ownerID);
	if(!owner)
		return FALSE;

	r3dOutToLog("obj_ServerGrenade[%d] created. ItemID:%d \n", srvObjParams_.ServerObjectID, m_ItemID);

	m_Weapon = g_pWeaponArmory->getWeaponConfig(m_ItemID);
	r3d_assert(m_Weapon);

	// set FileName based on itemid for ReadPhysicsConfig() in OnCreate() 
	r3dPoint3D bsize(1, 1, 1);

	switch (m_ItemID)
	{
		case WeaponConfig::ITEMID_FragGrenade:
		case WeaponConfig::ITEMID_SharpnelBomb:
		case WeaponConfig::ITEMID_SharpnelTripWireBomb:
			FileName	= "Data\\ObjectsDepot\\Weapons\\EXP_M26.sco";
			bsize		= r3dPoint3D(0.03f, 0.03f, 0.03f);
			break;
		case WeaponConfig::ITEMID_ChemLight:
			FileName	= "Data\\ObjectsDepot\\Weapons\\Item_Chemlight.sco";
			bsize		= r3dPoint3D(0.02f, 0.01f, 0.02f);
			break;
		case WeaponConfig::ITEMID_Flare:
		case WeaponConfig::ITEMID_FlareGun:
			FileName	= "Data\\ObjectsDepot\\Weapons\\EXP_Flare.sco";
			bsize		= r3dPoint3D(0.02f, 0.01f, 0.02f);
			break;
		case WeaponConfig::ITEMID_ChemLightBlue:
			FileName	= "Data\\ObjectsDepot\\Weapons\\Item_Chemlight_blue.sco";
			bsize		= r3dPoint3D(0.02f, 0.01f, 0.02f);
			break;
		case WeaponConfig::ITEMID_ChemLightGreen:
			FileName	= "Data\\ObjectsDepot\\Weapons\\Item_Chemlight_green.sco";
			bsize		= r3dPoint3D(0.02f, 0.01f, 0.02f);
			break;
		case WeaponConfig::ITEMID_ChemLightOrange:
			FileName	= "Data\\ObjectsDepot\\Weapons\\Item_Chemlight_orange.sco";
			bsize		= r3dPoint3D(0.02f, 0.01f, 0.02f);
			break;
		case WeaponConfig::ITEMID_ChemLightRed:
			FileName	= "Data\\ObjectsDepot\\Weapons\\Item_Chemlight_red.sco";
			bsize		= r3dPoint3D(0.02f, 0.01f, 0.02f);
			break;
		case WeaponConfig::ITEMID_ChemLightYellow:
			FileName	= "Data\\ObjectsDepot\\Weapons\\Item_Chemlight_yellow.sco";
			bsize		= r3dPoint3D(0.02f, 0.01f, 0.02f);
			break;
		default:
			r3dError("unknown grenade item %d\n", m_ItemID);
			break;
	}

	// we don't use the mesh on the server, so we estimate the bounding box size
	r3dBoundBox obb;
	obb.Size = bsize;
	obb.Org  = r3dPoint3D(GetPosition().x - obb.Size.x/2, GetPosition().y, GetPosition().z - obb.Size.z/2);

	ReadPhysicsConfig();
	PhysicsConfig.group = PHYSCOLL_PROJECTILES;
	PhysicsConfig.isFastMoving = true;

	parent::OnCreate();

	m_CreationTime = r3dGetTime() - m_AddedDelay;
	m_CreationPos = GetPosition();

	SetBBoxLocal( obb );
	UpdateTransform();

	if (WeaponConfig::ITEMID_FlareGun != m_ItemID)
		m_FireDirection.y += 0.1f; // to make grenade fly where you point
	m_AppliedVelocity = m_FireDirection * m_Weapon->m_AmmoSpeed;
	SetVelocity( m_AppliedVelocity );

//	r3dOutToLog("!!! Creating Grenade with Velocity: <%0.4f, %0.4f, %0.4f>\tLength: %0.4f\n", m_AppliedVelocity.x, m_AppliedVelocity.y, m_AppliedVelocity.z, m_AppliedVelocity.Length());
//	r3dOutToLog("!!! Creating Grenade with FireDirection: <%0.4f, %0.4f, %0.4f>\t AmmoSpeed: %0.4f\n", m_FireDirection.x, m_FireDirection.y, m_FireDirection.z, m_Weapon->m_AmmoSpeed);
	gServerLogic.NetRegisterObjectToPeers(this);

	return TRUE;
}

BOOL obj_ServerGrenade::OnDestroy()
{
	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));

	std::vector<obj_ServerGrenade*>::iterator it = std::find(allGrenades.begin(), allGrenades.end(), this);
	if(*it == this)
	{
		allGrenades.erase(it);
	}

	return parent::OnDestroy();
}

void obj_ServerGrenade::OnCollide(PhysicsCallbackObject *tobj, CollisionInfo &trace)
{
	m_LastCollisionNormal = trace.Normal;
	m_bHadCollided = true;
	gServerLogic.InformZombiesAboutGrenadeSound(this, false);
}

BOOL obj_ServerGrenade::Update()
{
	parent::Update();

	if(!isActive())
		return TRUE;

	if((ObjFlags & OBJFLAG_JustCreated) > 0)
		return TRUE;

	if (WeaponConfig::ITEMID_FlareGun == m_ItemID && PhysicsObject)
	{
		const r3dPoint3D& vel = GetVelocity();
		if( vel.y < 0.0f )
		{
			m_bIgnoreNetMover = true;

			float curTime = r3dGetTime();
			float diffTime = curTime - m_LastTimeUpdateCalled;
			m_LastTimeUpdateCalled = curTime;
			m_HangTime += diffTime;

			//r3dOutToLog("!!! [%0.6f (+%0.6f)] - Grenade(%d) Vel: <%0.2f, %0.2f, %0.2f> Len:%0.6f\n", m_HangTime, diffTime, GetNetworkID(), vel.x, vel.y, vel.z, vel.Length());
			float coeff = (float)(0.9 - pow(M_E, double(-m_HangTime)));
			//r3dOutToLog("Coeff: %0.2f\n", coeff);
			r3dPoint3D imp = r3dPoint3D(-vel.x * coeff, -vel.y * coeff, -vel.z * coeff);
			PhysicsObject->addImpulse( imp );
			//r3dOutToLog("Impuse: <%0.2f, %0.2f, %0.2f>\n", imp.x, imp.y, imp.z);
		}
		else
		{
			m_LastTimeUpdateCalled = r3dGetTime();
		}
	}

	if(m_CreationTime + m_Weapon->m_AmmoDelay < r3dGetTime())
	{
		OnExplode();
		return FALSE;
	}

//	r3dPoint3D pos = GetPosition();
//	r3dVector vel = GetVelocity();
//	r3dOutToLog("!!! Grenade Pos: <%0.2f, %0.2f, %0.2f> Vel: <%0.2f, %0.2f, %0.2f> Length: %0.2f\n", pos.x, pos.y, pos.z, vel.x, vel.y, m_AppliedVelocity.z, vel.Length());

	// send network position update
	if( !m_bIgnoreNetMover )
	{
		CNetCellMover::moveData_s md;
		md.pos       = GetPosition();
		md.turnAngle = GetRotationVector().x;
		md.bendAngle = 0;
		md.state     = (BYTE)0;

//		if( md.pos.Length() < 1.0f )
//			r3dOutToLog("!!! %s grenade being thrown from origin: <%0.2f, %0.2f, %0.2f>\n", Name.c_str(), md.pos.x, md.pos.y, md.pos.z);

		PKT_C2C_MoveSetCell_s n1;
		PKT_C2C_MoveRel_s     n2;
		DWORD pktFlags = m_NetMover.SendPosUpdate(md, &n1, &n2);
		if(pktFlags & 0x1)
			gServerLogic.p2pBroadcastToActive(this, &n1, sizeof(n1));
		if(pktFlags & 0x2)
			gServerLogic.p2pBroadcastToActive(this, &n2, sizeof(n2));
	}

	return TRUE;
}

DefaultPacket* obj_ServerGrenade::NetGetCreatePacket(int* out_size)
{
	obj_ServerPlayer* plr = (obj_ServerPlayer*)GameWorld().GetObject(ownerID);

	static PKT_S2C_CreateNetObject_s n;
	n.spawnID	= toP2pNetId(GetNetworkID());
	n.itemID	= m_ItemID;
	n.pos		= GetPosition();
	n.var1		= GetRotationVector().x;
	n.var2		= GetRotationVector().y;
	n.var3		= GetRotationVector().z;
	n.var4		= (!plr) ? 0 : toP2pNetId(plr->GetSafeNetworkID());
	n.var5		= m_TrackedID.get();

	*out_size = sizeof(n);
	return &n;
}

void obj_ServerGrenade::OnExplode()
{
	// if owner disappeared, do nothing.
	GameObject* owner = GameWorld().GetObject(ownerID);
	if(!owner)
	{
		setActiveFlag(0);
		return;
	}

	gServerLogic.DoExplosion(owner, this, R3D_ZERO_VECTOR, m_LastCollisionNormal, FULL_AREA_EXPLOSION, m_Weapon->m_AmmoArea, m_Weapon->m_AmmoDamage, m_Weapon->category, m_Weapon->m_itemID);
	gServerLogic.InformZombiesAboutGrenadeSound(this, true);

	setActiveFlag(0);
}

void obj_ServerGrenade::LoadServerObjectData()
{
	// Do Nothing!
}

void obj_ServerGrenade::SaveServerObjectData()
{
	// Do Nothing!
}
