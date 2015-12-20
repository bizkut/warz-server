#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"

#include "obj_ServerBarricade.h"
#include "ServerGameLogic.h"
#include "ObjectsCode/obj_ServerPlayer.h"
#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponArmory.h"
#include "../../GameEngine/ai/AutodeskNav/AutodeskNavMesh.h"
#include "Async_ServerObjects.h"

IMPLEMENT_CLASS(obj_ServerBarricade, "obj_ServerBarricade", "Object");
AUTOREGISTER_CLASS(obj_ServerBarricade);
IMPLEMENT_CLASS(obj_StrongholdServerBarricade, "obj_StrongholdServerBarricade", "Object");
AUTOREGISTER_CLASS(obj_StrongholdServerBarricade);
IMPLEMENT_CLASS(obj_ConstructorServerBarricade, "obj_ConstructorServerBarricade", "Object");
AUTOREGISTER_CLASS(obj_ConstructorServerBarricade);

std::vector<obj_ServerBarricade*> obj_ServerBarricade::allBarricades;

const static int CONSTRUCTION_EXPIRE_TIME = 7 * 24 * 60 * 60; // barricade will expire in 7 days
const static int BARRICADE_EXPIRE_TIME = 3 * 24 * 60 * 60; // barricade will expire in 3 days
const static int STRONGHOLD_EXPIRE_TIME = 30 * 24 * 60 * 60; // stronghold items will expire in 30 days
const static int DEV_EVENT_EXPIRE_TIME = 30 * 60; // dev event items will expire in 30 minutes

obj_StrongholdServerBarricade::obj_StrongholdServerBarricade() :
obj_ServerBarricade()
{
	float expireTime = r3dGetTime() + STRONGHOLD_EXPIRE_TIME;

#ifdef DISABLE_GI_ACCESS_FOR_DEV_EVENT_SERVER
	if (gServerLogic.ginfo_.gameServerId == 148353
		// for testing in dev environment
		//|| gServerLogic.ginfo_.gameServerId==11
		)
		expireTime = r3dGetTime() + DEV_EVENT_EXPIRE_TIME;
#endif

	srvObjParams_.ExpireTime = expireTime; //r3dGetTime() + STRONGHOLD_EXPIRE_TIME;
}

obj_StrongholdServerBarricade::~obj_StrongholdServerBarricade()
{
}

obj_ConstructorServerBarricade::obj_ConstructorServerBarricade() :
obj_ServerBarricade()
{
	float expireTime = r3dGetTime() + CONSTRUCTION_EXPIRE_TIME;

	srvObjParams_.ExpireTime = expireTime;
}

obj_ConstructorServerBarricade::~obj_ConstructorServerBarricade()
{
}

obj_ServerBarricade::obj_ServerBarricade()
{
	allBarricades.push_back(this);

	ObjTypeFlags |= OBJTYPE_GameplayItem | OBJTYPE_Barricade;
	ObjFlags |= OBJFLAG_SkipCastRay;
	
	m_ItemID = 0;
	m_Health = 1;
	m_ObstacleId = -1;
	m_ActivateTrap = 0.0f;
	
	float expireTime = r3dGetTime() + BARRICADE_EXPIRE_TIME;

#ifdef DISABLE_GI_ACCESS_FOR_DEV_EVENT_SERVER
	if (gServerLogic.ginfo_.gameServerId == 148353
		// for testing in dev environment
		//|| gServerLogic.ginfo_.gameServerId==11
		)
		expireTime = r3dGetTime() + DEV_EVENT_EXPIRE_TIME;
#endif

	if (m_ItemID == WeaponConfig::ITEMID_ConstructorBlockSmall ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorBlockBig ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorBlockCircle ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorColum1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorColum2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorColum3 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorFloor1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorFloor2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling3 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWallMetalic ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorSlope ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall3 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall4 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorBaseBunker ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall5)
	{
		expireTime = r3dGetTime() + CONSTRUCTION_EXPIRE_TIME;
	}

	srvObjParams_.ExpireTime = expireTime;
}

obj_ServerBarricade::~obj_ServerBarricade()
{
}

BOOL obj_ServerBarricade::OnCreate()
{
	r3dOutToLog("obj_ServerBarricade[%d] created. ItemID:%d Health:%.0f, %.0f mins left\n", srvObjParams_.ServerObjectID, m_ItemID, m_Health, (srvObjParams_.ExpireTime - r3dGetTime()) / 60.0f);
	
	// set FileName based on itemid for ReadPhysicsConfig() in OnCreate() 
	r3dPoint3D bsize(1, 1, 1);
	if(m_ItemID == WeaponConfig::ITEMID_BarbWireBarricade)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Item_Barricade_BarbWire_Built.sco";
		bsize    = r3dPoint3D(5.320016f, 1.842704f, 1.540970f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_WoodShieldBarricade || m_ItemID==WeaponConfig::ITEMID_WoodShieldBarricadeZB)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Item_Barricade_WoodShield_Built.sco";
		bsize    = r3dPoint3D(1.582400f, 2.083451f, 0.708091f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_RiotShieldBarricade || m_ItemID==WeaponConfig::ITEMID_RiotShieldBarricadeZB)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Item_Riot_Shield_01.sco";
		bsize    = r3dPoint3D(1.726829f, 2.136024f, 0.762201f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_SandbagBarricade)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\item_barricade_Sandbag_built.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_WoodenDoorBlock)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_Door_Wood_2M_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_MetalWallBlock)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_Wall_Metal_2M_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_TallBrickWallBlock)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_Wall_Brick_Tall_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_WoodenWallPiece)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_Wall_Wood_2M_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ShortBrickWallPiece)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_Wall_Brick_Short_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_PlaceableLight)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_Light_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_SmallPowerGenerator)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_PowerGen_01_Small.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_BigPowerGenerator)
	{
		FileName = "Data\\ObjectsDepot\\Weapons\\Block_PowerGen_01_Industrial.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorBlockSmall)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_ab_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorBlockBig)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_ab_02.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorBlockCircle)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_ab_03.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorColum1)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_col_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorColum2)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_col_02.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorColum3)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_col_03.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorBaseBunker)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\base_lm_infantrybunker_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorFloor1)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_flr_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorFloor2)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_flr_02.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling1)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_fnd_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling2)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_fnd_02.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling3)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_fnd_03.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorWallMetalic)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_metalicpole_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorSlope)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_str_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorWall1)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_wal_01.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorWall2)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_wal_02.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorWall3)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_wal_03.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorWall4)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_01_01x04m.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_ConstructorWall5)
	{
		FileName = "Data\\ObjectsDepot\\G3_BuildingBlocks\\g3_buildingblock_01_02x04m.sco";
		bsize    = r3dPoint3D(1.513974f, 1.057301f, 1.111396f);
	}

	else if(m_ItemID == WeaponConfig::ITEMID_Traps_Bear)
	{
		FileName = "Data\\ObjectsDepot\\WZ_Consumables\\INB_Traps_Bear_01_Armed_tps.sco";
		bsize    = r3dPoint3D(0.720f, 0.095f, 0.765f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_Traps_Spikes)
	{
		FileName = "Data\\ObjectsDepot\\WZ_Consumables\\INB_Traps_Spikes_01_Armed_tps.sco";
		bsize    = r3dPoint3D(0.858f, 0.146f, 0.858f);
	}
	else if(m_ItemID == WeaponConfig::ITEMID_Campfire)
	{
		FileName = "Data\\ObjectsDepot\\WZ_Consumables\\INB_prop_campfire_01.sco";
		bsize    = r3dPoint3D(0.929f, 0.580f, 0.929f);
	}
	else
		r3dError("unknown barricade item %d\n", m_ItemID);

	parent::OnCreate();
	
	// add navigational obstacle
	r3dBoundBox obb;
	obb.Size = bsize;
	obb.Org  = r3dPoint3D(GetPosition().x - obb.Size.x/2, GetPosition().y, GetPosition().z - obb.Size.z/2);
	m_ObstacleId = gAutodeskNavMesh.AddObstacle(this, obb, GetRotationVector().x);
	
	// calc 2d radius
	m_Radius = R3D_MAX(obb.Size.x, obb.Size.z) / 2;

	gServerLogic.NetRegisterObjectToPeers(this);
	return 1;
}

BOOL obj_ServerBarricade::OnDestroy()
{
	if(m_ObstacleId >= 0)
	{
		gAutodeskNavMesh.RemoveObstacle(m_ObstacleId);
	}

	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));

	for(std::vector<obj_ServerBarricade*>::iterator it = allBarricades.begin(); it != allBarricades.end(); ++it)
	{
		if(*it == this)
		{
			allBarricades.erase(it);
			break;
		}
	}
	
	return parent::OnDestroy();
}

BOOL obj_ServerBarricade::Update()
{
	if (srvObjParams_.ExpireTime < r3dGetTime())
		DestroyBarricade();

	// check for pending delete
	if(isActive() && m_Health <= 0.0f && srvObjParams_.ServerObjectID > 0)
	{
		g_AsyncApiMgr->AddJob(new CJobDeleteServerObject(this));

		setActiveFlag(0);
		return TRUE;
	}
	if (m_ItemID == WeaponConfig::ITEMID_Traps_Bear ||
		m_ItemID == WeaponConfig::ITEMID_Traps_Spikes)
		CheckTouch();

	if(m_ItemID == WeaponConfig::ITEMID_Campfire)
		SearchPlayers();

	return parent::Update();
}

DefaultPacket* obj_ServerBarricade::NetGetCreatePacket(int* out_size)
{
	static PKT_S2C_CreateNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	n.itemID  = m_ItemID;
	n.pos     = GetPosition();
	n.var1    = GetRotationVector().x;
	n.var3    = m_ActivateTrap;

	*out_size = sizeof(n);
	return &n;
}

void obj_ServerBarricade::DoDamage(float dmg)
{
	if (m_ItemID == WeaponConfig::ITEMID_ConstructorBlockSmall ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorBlockBig ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorBlockCircle ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorColum1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorColum2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorColum3 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorFloor1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorFloor2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorCeiling3 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWallMetalic ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorSlope ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall1 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall2 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall3 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall4 ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorBaseBunker ||
		m_ItemID == WeaponConfig::ITEMID_ConstructorWall5)
	{
		m_Health -=0.01f;
		return;
	}

	if(m_Health > 0 && m_ItemID != WeaponConfig::ITEMID_ConstructorWODBOX)
	{
		srvObjParams_.IsDirty = true;
		m_Health -= dmg;
		// do not delete object here, it may still be waiting for assigned ServerObjectID
	}
}

void obj_ServerBarricade::LoadServerObjectData()
{
	m_ItemID = srvObjParams_.ItemID;
	
	// deserialize from xml
	IServerObject::CSrvObjXmlReader xml(srvObjParams_.Var1);
	m_Health = xml.xmlObj.attribute("Health").as_float();
}

void obj_ServerBarricade::SearchPlayers()
{
		for( GameObject* obj = GameWorld().GetFirstObject(); obj; obj = GameWorld().GetNextObject(obj) )
		{
			if(obj->isObjType(OBJTYPE_Human))
			{
				float dist = (GetPosition() - obj->GetPosition()).LengthSq();
				if(dist < 2)
				{
					obj_ServerPlayer* Player = (obj_ServerPlayer*)obj;
					if (Player->loadout_->Alive > 0)
					{
						if (!Player->IsInVehicle())
						{
							if (Player->loadout_->Health<100)
								Player->loadout_->Health+=0.01f;
							if (Player->loadout_->MedBleeding!=0.0)
								Player->loadout_->MedBleeding = 0.0f;
							if (Player->loadout_->MedBloodInfection!=0.0)
								Player->loadout_->MedBloodInfection = 0.0f;
						}
					}
				}
			}
		}
}

void obj_ServerBarricade::CheckTouch()
{
	if (m_ActivateTrap>0.0)
	{
		for( GameObject* obj = GameWorld().GetFirstObject(); obj; obj = GameWorld().GetNextObject(obj) )
		{
			if(obj->isObjType(OBJTYPE_Human))
			{
				float dist = (GetPosition() - obj->GetPosition()).LengthSq();
				if(dist < 0.6)
				{
					obj_ServerPlayer* fromPlr = (obj_ServerPlayer*)obj;
					gServerLogic.ApplyDamage(fromPlr, obj, fromPlr->GetPosition()+r3dPoint3D(0,1,0), 10.0f, false, storecat_UsableItem, m_ItemID);
					m_ActivateTrap=0.0f;
					PKT_S2C_SetupTraps_s n;
					n.spawnID = toP2pNetId(GetNetworkID());
					n.m_Activate = m_ActivateTrap>0.0?1:0;
					gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));					
				}
			}
		}
	}
}

void obj_ServerBarricade::SaveServerObjectData()
{
	srvObjParams_.ItemID = m_ItemID;

	IServerObject::CSrvObjXmlWriter xml;
	xml.xmlObj.append_attribute("Health") = m_Health;
	xml.save(srvObjParams_.Var1);
}

#ifdef VEHICLES_ENABLED
int obj_ServerBarricade::GetDamageForVehicle()
{
	switch( m_ItemID )
	{
	default:
		return 0;
	case WeaponConfig::ITEMID_BarbWireBarricade:
		return 800;
	case WeaponConfig::ITEMID_WoodShieldBarricade:
	case WeaponConfig::ITEMID_WoodShieldBarricadeZB:
		return 100;
	case WeaponConfig::ITEMID_RiotShieldBarricade:
	case WeaponConfig::ITEMID_RiotShieldBarricadeZB:
		return 400;
	case WeaponConfig::ITEMID_SandbagBarricade:
		return 200;
	}
}
#endif

void obj_ServerBarricade::DestroyBarricade()
{
	setActiveFlag(0);
	g_AsyncApiMgr->AddJob(new CJobDeleteServerObject(this));
}
