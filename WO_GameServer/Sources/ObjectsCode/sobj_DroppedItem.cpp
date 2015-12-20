#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"
#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponArmory.h"

#include "sobj_DroppedItem.h"

#ifdef ENABLE_GAMEBLOCKS
#include "GBClient/Inc/GBClient.h"
#include "GBClient/Inc/GBReservedEvents.h"

extern GameBlocks::GBClient* g_GameBlocks_Client;
extern GameBlocks::GBPublicSourceId g_GameBlocks_ServerID;
#endif //ENABLE_GAMEBLOCKS

const float DROPPED_ITEM_EXPIRE_TIME = 10.0f * 60.0f; // 10 min

IMPLEMENT_CLASS(obj_DroppedItem, "obj_DroppedItem", "Object");
AUTOREGISTER_CLASS(obj_DroppedItem);

extern wiInventoryItem RollItem(const LootBoxConfig* lootCfg, int depth);

obj_DroppedItem::obj_DroppedItem()
{
	srvObjParams_.ExpireTime = r3dGetTime() + DROPPED_ITEM_EXPIRE_TIME;	// setup here, as it can be overwritten
	AirDropPos = r3dPoint3D(0,0,0);
	m_IsOnTerrain = false;
	m_FirstTime = 0;
	m_LootBoxID1 = 0;
	m_LootBoxID2 = 0;
	m_LootBoxID3 = 0;
	m_LootBoxID4 = 0;
	m_LootBoxID5 = 0;
	m_LootBoxID6 = 0;
	m_DefaultItems = 1;
	ExpireFirstTime = r3dGetTime();
}

obj_DroppedItem::~obj_DroppedItem()
{
}

BOOL obj_DroppedItem::OnCreate()
{
	r3dOutToLog("obj_DroppedItem %p created. %d, %f sec left\n", this, m_Item.itemID, srvObjParams_.ExpireTime - r3dGetTime());

	r3d_assert(NetworkLocal);
	r3d_assert(GetNetworkID());
	r3d_assert(m_Item.itemID);

	m_Item.ResetClipIfFull();

	//distToCreateSq = 130 * 130;
	//distToDeleteSq = 150 * 150;
	
	if (m_Item.itemID == 'ARDR')
	{
		expireAirDrop = r3dGetTime() + (10.0f * 60.0f);
		distToCreateSq = FLT_MAX;
		distToDeleteSq = FLT_MAX;
		// raycast down to earth in case world was changed or trying to spawn item in the air (player killed during jump)
		r3dPoint3D pos = gServerLogic.AdjustPositionToFloor(AirDropPos);
		SetPosition(AirDropPos);
	}
	else {
	// overwrite object network visibility
		if (m_Item.itemID == 'FLPS')
		{
			distToCreateSq = FLT_MAX;
			distToDeleteSq = FLT_MAX;
		}
		else
		{
			distToCreateSq = 130 * 130;
			distToDeleteSq = 150 * 150;
		}
		// raycast down to earth in case world was changed or trying to spawn item in the air (player killed during jump)
		r3dPoint3D pos = gServerLogic.AdjustPositionToFloor(GetPosition());
		SetPosition(pos);
	}

	gServerLogic.NetRegisterObjectToPeers(this);

#ifdef ENABLE_GAMEBLOCKS
	if(g_GameBlocks_Client && g_GameBlocks_Client->Connected() && srvObjParams_.CustomerID != 0)
	{
		g_GameBlocks_Client->PrepareEventForSending("DropItem", g_GameBlocks_ServerID, GameBlocks::GBPublicPlayerId(uint32_t(srvObjParams_.CustomerID)));
		g_GameBlocks_Client->AddKeyValueInt("ItemID", m_Item.itemID);
		g_GameBlocks_Client->SendEvent();
	}
#endif

	return parent::OnCreate();
}

BOOL obj_DroppedItem::OnDestroy()
{
	//r3dOutToLog("obj_DroppedItem %p destroyed\n", this);

	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));
	
	return parent::OnDestroy();
}

BOOL obj_DroppedItem::Update()
{
	if(r3dGetTime() > srvObjParams_.ExpireTime)
	{
		setActiveFlag(0);
	}

	if (m_FirstTime == 1 && r3dGetTime()>ExpireFirstTime)
	{
		m_FirstTime = 0;
	}

	if (m_IsOnTerrain == false && m_Item.itemID == 'ARDR')
	{
		AirDropPos.y-=0.03f;
		SetPosition(AirDropPos);

		PKT_S2C_DropItemYPosition_s n;
		n.YPos = AirDropPos.y;
		n.spawnID = toP2pNetId(GetNetworkID());
		gServerLogic.p2pBroadcastToAll(&n, sizeof(n), true);

/////////////////////////////////
		R3DPROFILE_START("RayCast");
		PhysicsCallbackObject* target = NULL;
		const MaterialType *mt = 0;

		PxRaycastHit hit;
		PxSceneQueryFilterData filter(PxFilterData(COLLIDABLE_STATIC_MASK,0,0,0), PxSceneQueryFilterFlags(PxSceneQueryFilterFlag::eSTATIC|PxSceneQueryFilterFlag::eDYNAMIC));

		bool hitResult = g_pPhysicsWorld->raycastSingle(PxVec3(AirDropPos.x, AirDropPos.y + 0.5f, AirDropPos.z), PxVec3(0, -1, 0), 1.0f, PxSceneQueryFlags(PxSceneQueryFlag::eIMPACT), hit, filter);
		if( hitResult )
		{
			if( hit.shape && (target = static_cast<PhysicsCallbackObject*>(hit.shape->getActor().userData)))
			{
				r3dMaterial* material = 0;
				GameObject *gameObj = target->isGameObject();
				if(gameObj)
				{
					if(gameObj->isObjType(OBJTYPE_Terrain))
					{
						m_IsOnTerrain = true;
						int Items[6];
						if (m_DefaultItems == 0)
						{
							int m_LootBoxID[6];
							m_LootBoxID[0] = m_LootBoxID1;
							m_LootBoxID[1] = m_LootBoxID2;
							m_LootBoxID[2] = m_LootBoxID3;
							m_LootBoxID[3] = m_LootBoxID4;
							m_LootBoxID[4] = m_LootBoxID5;
							m_LootBoxID[5] = m_LootBoxID6;

							LootBoxConfig*	m_LootBoxConfig;
							for (int i = 0;i<6;i++)
							{
								m_LootBoxConfig = const_cast<LootBoxConfig*>(g_pWeaponArmory->getLootBoxConfig(m_LootBoxID[i]));
								if(m_LootBoxConfig != NULL)
								{
									if(m_LootBoxConfig->entries.size() != 0)
									{
										wiInventoryItem entrieID = RollItem(m_LootBoxConfig, 0);
										if (entrieID.itemID != NULL || entrieID.itemID != 0)
											Items[i] = entrieID.itemID;
										else
											Items[i] = GetItemDefault(i);
									}
									else {
										Items[i] = GetItemDefault(i);
									}
								}
								else {
									Items[i] = GetItemDefault(i);
								}
							}
						}
						

						for (int i=0;i<6;i++)
						{

							if (m_DefaultItems == 1)
								Items[i] = GetItemDefault(i);

							wiInventoryItem wi;
							wi.itemID   = Items[i];
							wi.quantity = 1;
							// create network object
							r3dPoint3D PosObjects = AirDropPos+r3dPoint3D(u_GetRandom(-3,3),0,u_GetRandom(-3,3));
							obj_DroppedItem* obj = (obj_DroppedItem*)srv_CreateGameObject("obj_DroppedItem", "obj_DroppedItem", PosObjects);
							obj->SetPosition(PosObjects);
							obj->SetNetworkID(gServerLogic.GetFreeNetId());
							obj->NetworkLocal = true;
							obj->m_Item          = wi;
							obj->m_Item.quantity = 1;
						}
						wiInventoryItem Flare;
						Flare.itemID   = 'FLPS';
						Flare.quantity = 1;
						// create network object
						obj_DroppedItem* obj = (obj_DroppedItem*)srv_CreateGameObject("obj_DroppedItem", "obj_DroppedItem", AirDropPos);
						obj->SetPosition(AirDropPos);
						obj->SetNetworkID(gServerLogic.GetFreeNetId());
						obj->NetworkLocal = true;
						obj->m_Item          = Flare;
						obj->m_Item.quantity = 1;
						//r3dOutToLog("######## ha tocado suelo\n");
						setActiveFlag(0);
					}
				}
			}
		}
		R3DPROFILE_END("RayCast");
/////////////////////////////////
	}

	return parent::Update();
}

int obj_DroppedItem::GetItemDefault(int i)
{
	switch(i)
	{
	case 0:	
			return 101088;// m106
	case 1:
			return 101084;// VSS
	case 2:
			return 101002;// m16
	case 3:	
			return 101304;// medkit
	case 4:	
			return 101317;// Wood Barricade
	case 5:	
			return 20180;// macuto militar
	}
	return 0;
}

DefaultPacket* obj_DroppedItem::NetGetCreatePacket(int* out_size)
{
	if (m_Item.itemID == 'ARDR')
	{
		SetPosition(AirDropPos);
		PKT_S2C_AirDropMapUpdate_s AirDrop;
		AirDrop.location	= AirDropPos;
		AirDrop.m_time		= expireAirDrop; // 10 min to expire
		gServerLogic.p2pBroadcastToAll(&AirDrop, sizeof(AirDrop), true);
	}

		static PKT_S2C_CreateDroppedItem_s n;
		n.spawnID = toP2pNetId(GetNetworkID());
		n.pos     = GetPosition();
		n.Item    = m_Item;
		n.FirstTime = m_FirstTime;

		*out_size = sizeof(n);
		return &n;
}

void obj_DroppedItem::LoadServerObjectData()
{
	// deserialize from xml
	IServerObject::CSrvObjXmlReader xml(srvObjParams_.Var1);
	m_Item.itemID      = srvObjParams_.ItemID;
	m_Item.InventoryID = xml.xmlObj.attribute("iid").as_int64();
	m_Item.quantity    = xml.xmlObj.attribute("q").as_int();
	m_Item.Var1        = xml.xmlObj.attribute("v1").as_int();
	m_Item.Var2        = xml.xmlObj.attribute("v2").as_int();
	m_Item.Var3        = xml.xmlObj.attribute("v3").as_int();
}

void obj_DroppedItem::SaveServerObjectData()
{
	srvObjParams_.ItemID     = m_Item.itemID;

	char strInventoryID[64];
	sprintf(strInventoryID, "%I64d", m_Item.InventoryID);

	IServerObject::CSrvObjXmlWriter xml;
	xml.xmlObj.append_attribute("iid") = strInventoryID;
	xml.xmlObj.append_attribute("q")   = m_Item.quantity;
	xml.xmlObj.append_attribute("v1")  = m_Item.Var1;
	xml.xmlObj.append_attribute("v2")  = m_Item.Var2;
	xml.xmlObj.append_attribute("v3")  = m_Item.Var3;
	xml.save(srvObjParams_.Var1);
}