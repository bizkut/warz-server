#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"
#include "ServerGameLogic.h"

#include "sobj_SpawnedItem.h"
#include "obj_ServerItemSpawnPoint.h"
#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponArmory.h"

#ifdef ENABLE_GAMEBLOCKS
#include "GBClient/Inc/GBClient.h"
#include "GBClient/Inc/GBReservedEvents.h"

extern GameBlocks::GBClient* g_GameBlocks_Client;
extern GameBlocks::GBPublicSourceId g_GameBlocks_ServerID;
#endif //ENABLE_GAMEBLOCKS

IMPLEMENT_CLASS(obj_SpawnedItem, "obj_SpawnedItem", "Object");
AUTOREGISTER_CLASS(obj_SpawnedItem);

obj_SpawnedItem::obj_SpawnedItem()
{
	m_SpawnIdx = -1;
	m_DestroyIn = 0.0f;
	m_isHackerDecoy = false;
	m_SpawnLootCrateOnClient = false;
}

obj_SpawnedItem::~obj_SpawnedItem()
{
}

void obj_SpawnedItem::AdjustDroppedWeaponAmmo()
{
	if(m_Item.Var1 >= 0)
		return;
	const WeaponConfig* wcfg = g_pWeaponArmory->getWeaponConfig(m_Item.itemID);
	if(wcfg == NULL)
		return;
	const WeaponAttachmentConfig* clipCfg = g_pWeaponArmory->getAttachmentConfig(wcfg->FPSDefaultID[WPN_ATTM_CLIP]);
	if(!clipCfg)
		return;
	
	float clipSize = (float)clipCfg->m_Clipsize;
	if(int(u_GetRandom(0.0f, 10.0f))<3) // 30% chance to get brand new
	{
		//wi.Var1 = clipCfg->m_Clipsize;
		//wi.Var2 = clipCfg->m_itemID;
		return;
	}
	else
	{
		m_Item.Var1 = R3D_MAX(1, (int)u_GetRandom(clipSize * 0.25f, clipSize * 1.0f));
		m_Item.Var2 = clipCfg->m_itemID;
		return;
	}
	
	return;
}

BOOL obj_SpawnedItem::OnCreate()
{
	//r3dOutToLog("obj_SpawnedItem %p created. %dx%d\n", this, m_Item.itemID, m_Item.quantity);
	
	r3d_assert(NetworkLocal);
	r3d_assert(GetNetworkID());
	r3d_assert(m_SpawnObj.get() > 0);
	r3d_assert(m_SpawnIdx >= 0);
	r3d_assert(m_Item.itemID);
	
	m_Item.ResetClipIfFull();
	AdjustDroppedWeaponAmmo();

	// raycast down to earth in case world was changed
	r3dPoint3D pos = gServerLogic.AdjustPositionToFloor(GetPosition());
	SetPosition(pos);

	// overwrite object network visibility
	distToCreateSq = 130 * 130;
	distToDeleteSq = 150 * 150;
	
	gServerLogic.NetRegisterObjectToPeers(this);
	
	obj_ServerItemSpawnPoint* owner = (obj_ServerItemSpawnPoint*)GameWorld().GetObject(m_SpawnObj);

#ifdef ENABLE_GAMEBLOCKS
	if(g_GameBlocks_Client && g_GameBlocks_Client->Connected() && owner)
	{
		g_GameBlocks_Client->PrepareEventForSending("SpawnedItem", g_GameBlocks_ServerID, 0);
		g_GameBlocks_Client->AddKeyValueInt("ItemID", m_Item.itemID);
		g_GameBlocks_Client->AddKeyValueInt("LootBoxID", owner->m_LootBoxID);
		g_GameBlocks_Client->AddKeyValueVector3D("Position", pos.x, pos.y, pos.z);
		g_GameBlocks_Client->SendEvent();
	}
#endif


	return parent::OnCreate();
}

BOOL obj_SpawnedItem::OnDestroy()
{
	//r3dOutToLog("obj_SpawnedItem %p destroyed\n", this);

	obj_ServerItemSpawnPoint* owner = (obj_ServerItemSpawnPoint*)GameWorld().GetObject(m_SpawnObj);
	r3d_assert(owner);
	owner->PickUpObject(m_SpawnIdx, GetSafeID());
	
	PKT_S2C_DestroyNetObject_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	gServerLogic.p2pBroadcastToActive(this, &n, sizeof(n));

	return parent::OnDestroy();
}

BOOL obj_SpawnedItem::Update()
{
	if(m_DestroyIn > 0 && r3dGetTime() > m_DestroyIn)
		return FALSE; // force item de-spawn

	return parent::Update();
}

DefaultPacket* obj_SpawnedItem::NetGetCreatePacket(int* out_size)
{
	static PKT_S2C_CreateDroppedItem_s n;
	n.spawnID = toP2pNetId(GetNetworkID());
	n.pos     = GetPosition();
	n.Item    = m_Item;
	n.FirstTime = 0;

	if(m_SpawnLootCrateOnClient)
	{
		n.Item.itemID = 'LOOT';
		n.Item.quantity = 1;
	}

	*out_size = sizeof(n);
	return &n;
}