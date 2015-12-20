#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "multiplayer/P2PMessages.h"

#include "ServerGameLogic.h"
#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponArmory.h"

#include "ObjectsCode/obj_ServerItemSpawnPoint.h"
#include "ObjectsCode/sobj_SpawnedItem.h"

IMPLEMENT_CLASS(obj_ServerHackerItemSpawnPoint, "obj_HackerItemSpawnPoint", "Object");
AUTOREGISTER_CLASS(obj_ServerHackerItemSpawnPoint);

std::vector<obj_ServerItemSpawnPoint*> obj_ServerItemSpawnPoint::m_allSpawns;

obj_ServerHackerItemSpawnPoint::obj_ServerHackerItemSpawnPoint():obj_ServerItemSpawnPoint()
{
	m_isHackerDecoySpawnPoint = true;
}

obj_ServerHackerItemSpawnPoint::~obj_ServerHackerItemSpawnPoint()
{
}


IMPLEMENT_CLASS(obj_ServerItemSpawnPoint, "obj_ItemSpawnPoint", "Object");
AUTOREGISTER_CLASS(obj_ServerItemSpawnPoint);

obj_ServerItemSpawnPoint::obj_ServerItemSpawnPoint()
{
	m_spawnAllItemsAtTime = 0;
	m_nextUpdateTime = 0;
	m_LootBoxConfig = NULL;
	m_isHackerDecoySpawnPoint = false;
	ObjTypeFlags |= OBJTYPE_ItemSpawnPoint;
	
	m_allSpawns.push_back(this);
}

obj_ServerItemSpawnPoint::~obj_ServerItemSpawnPoint()
{
	m_allSpawns.erase(std::find(m_allSpawns.begin(), m_allSpawns.end(), this));
}

BOOL obj_ServerItemSpawnPoint::Load(const char *fname)
{
	// skip mesh loading
	if(!GameObject::Load(fname)) 
		return FALSE;

	return TRUE;
}

BOOL obj_ServerItemSpawnPoint::OnCreate()
{
	// item spawn point can be empty. i don't see any reason for that, but anyway...
	if(m_LootBoxID == 0)
		return FALSE;

	if(!m_OneItemSpawn)
	{
		m_LootBoxConfig = const_cast<LootBoxConfig*>(g_pWeaponArmory->getLootBoxConfig(m_LootBoxID));
		if(m_LootBoxConfig == NULL)
		{
			r3dOutToLog("LootBox: !!!! Failed to get lootbox with ID: %d. Server/Client desync??\n", m_LootBoxID);
			return FALSE;
		}

		if(m_LootBoxConfig->entries.size() == 0)
		{
			r3dOutToLog("LootBox: !!!! m_LootBoxID %d does NOT have items inside\n", m_LootBoxID);
			// fall thru, as content can be modified while server is running
		}

		r3dOutToLog("LootBox: %p with ItemID %d, %d items, tick: %.0f\n", this, m_LootBoxID, m_LootBoxConfig->entries.size(), m_TickPeriod);
	}
	else
	{
		const BaseItemConfig* bic = g_pWeaponArmory->getConfig(m_LootBoxID);
		if(bic == NULL)
		{
			r3dOutToLog("LootBox: !!!! Failed to get item with ID: %d. Server/Client desync??\n", m_LootBoxID);
			return FALSE;
		}

		r3dOutToLog("LootBox: %p with ItemID %d, tick: %.0f\n", this, m_LootBoxID, m_TickPeriod);
	}

	m_nextUpdateTime = r3dGetTime() + m_TickPeriod;

	m_spawnAllItemsAtTime = r3dGetTime() + 5.0f;

	return parent::OnCreate();
}

BOOL obj_ServerItemSpawnPoint::Update()
{
	if(gServerLogic.net_mapLoaded_LastNetID > 0) // do not spawn items until server has fully loaded
	{
		extern int cfg_uploadLogs;
		if(!cfg_uploadLogs) // dev environment
		{
			if(r3dGetTime() > m_spawnAllItemsAtTime && m_spawnAllItemsAtTime>0 && !m_UniqueSpawnSystem)
			{
				m_spawnAllItemsAtTime = 0;
				// sergey's request. spawn all items at server start up
				for(size_t i=0; i<m_SpawnPointsV.size(); ++i)
				{
					if(m_SpawnPointsV[i].itemID == 0)
						SpawnItem((int)i);
				}
			}
		}

		if(r3dGetTime() > m_nextUpdateTime)
		{
			m_nextUpdateTime = r3dGetTime() + m_TickPeriod;

			if(!m_OneItemSpawn)
				if(m_LootBoxConfig->entries.size() == 0)
					return parent::Update();

			if(!m_UniqueSpawnSystem)
			{
				for(size_t i=0; i<m_SpawnPointsV.size(); ++i)
				{
					ItemSpawn& spawn = m_SpawnPointsV[i];
					if(spawn.itemID == 0 && spawn.cooldown < r3dGetTime()) 
					{
						SpawnItem((int)i);
						break; // only one item spawn per tickPeriod
					}
				}
			}
			else
			{
				bool SpawnSystem_Ready = true;
				for(size_t i=0; i<m_SpawnPointsV.size(); ++i)
				{
					ItemSpawn& spawn = m_SpawnPointsV[i];
					if(spawn.itemID || spawn.cooldown > r3dGetTime())
					{
						SpawnSystem_Ready = false;
						break;
					}
				}
				if(SpawnSystem_Ready && m_SpawnPointsV.size()>0)
				{
					// select random point and spawn item there
					int p = (int)u_GetRandom(0.0f, float(m_SpawnPointsV.size()-1));
					SpawnItem(p);
				}
			}
		}
	}

	return parent::Update();
}

obj_SpawnedItem* obj_ServerItemSpawnPoint::CreateSpawnedItem(int spawnIndex, const wiInventoryItem& wi)
{
	ItemSpawn& spawn = m_SpawnPointsV[spawnIndex];

	// create network object
	obj_SpawnedItem* obj = (obj_SpawnedItem*)srv_CreateGameObject("obj_SpawnedItem", "obj_SpawnedItem", spawn.pos);
	obj->SetNetworkID(gServerLogic.GetFreeNetId());
	obj->NetworkLocal = true;
	obj->m_isHackerDecoy = m_isHackerDecoySpawnPoint;
	// vars
	if(m_DestroyItemTimer > 0.0f)
		obj->m_DestroyIn = r3dGetTime() + m_DestroyItemTimer;
	obj->m_SpawnObj   = GetSafeID();
	obj->m_SpawnIdx   = spawnIndex;
	obj->m_Item       = wi;
	obj->m_SpawnLootCrateOnClient = m_SpawnLootCrate;

	spawn.itemID       = wi.itemID;
	spawn.spawnedObjID = obj->GetSafeID();
	
	return obj;
}

void obj_ServerItemSpawnPoint::SpawnItem(int spawnIndex)
{
	//r3dOutToLog("obj_ServerItemSpawnPoint %p rolling on %d\n", this, spawnIndex); CLOG_INDENT;
	ItemSpawn& spawn = m_SpawnPointsV[spawnIndex];
	
	// roll item and mark it inside spawn
	wiInventoryItem wi;
	if(!m_OneItemSpawn)
	{
		wi = RollItem(m_LootBoxConfig, 0);
	}
	else
	{
		wi.itemID   = m_LootBoxID;
		wi.quantity = 1;
	}

	// check if somehow roll wasn't successful
	if(wi.itemID == 0)
	{
		spawn.cooldown = m_Cooldown + r3dGetTime(); // mark that spawn point not to spawn anything until next cooldown
		return;
	}

	CreateSpawnedItem(spawnIndex, wi);
	
	return;
}

void obj_ServerItemSpawnPoint::PickUpObject(int spawnIndex, gobjid_t spawnedObjID)
{
	ItemSpawn& spawn = m_SpawnPointsV[spawnIndex];

	// this will happen if delayed destruction of obj_SpawnedItem called after new item is created in same slot in LoadServerObjectData
	if(spawn.spawnedObjID != spawnedObjID)
	{
		//r3dOutToLog("item replaced in slot %d\n", spawnIndex);
		return;
	}
	
	spawn.itemID   = 0;
	spawn.cooldown = m_Cooldown + r3dGetTime();
	spawn.spawnedObjID = invalidGameObjectID;
}

void obj_ServerItemSpawnPoint::LoadServerObjectData(const pugi::xml_node xmlNode)
{
	m_spawnAllItemsAtTime = 0;
	
	// despawn all items if they was already created
	for(size_t i=0; i<m_SpawnPointsV.size(); ++i)
	{
		ItemSpawn& spawn = m_SpawnPointsV[i];
		obj_SpawnedItem* obj = (obj_SpawnedItem*)GameWorld().GetObject(spawn.spawnedObjID);
		if(!obj) 
			continue;

		obj->setActiveFlag(0);

		spawn.itemID       = 0;
		spawn.cooldown     = m_Cooldown + r3dGetTime();
		spawn.spawnedObjID = invalidGameObjectID;
	}
	
	// recreate spawned items
	for(size_t i=0; i<m_SpawnPointsV.size(); ++i)
	{
		ItemSpawn& spawn = m_SpawnPointsV[i];

		char buf[128];
		sprintf(buf, "s%d", i);

		pugi::xml_node xmlSpawn = xmlNode.child(buf);

		wiInventoryItem wi;
		wi.itemID   = xmlSpawn.attribute("id").as_uint();
		wi.quantity = xmlSpawn.attribute("q").as_int();
		if(wi.itemID == 0)
			continue;
			
		CreateSpawnedItem(i, wi);
	}
}

void obj_ServerItemSpawnPoint::SaveServerObjectData(pugi::xml_node& xmlNode)
{
	for(size_t i=0; i<m_SpawnPointsV.size(); ++i)
	{
		ItemSpawn& spawn = m_SpawnPointsV[i];
		if(spawn.itemID == 0)
			continue;
		obj_SpawnedItem* obj = (obj_SpawnedItem*)GameWorld().GetObject(spawn.spawnedObjID);
		if(!obj) 
			continue;
		
		char buf[128];
		sprintf(buf, "s%d", i);
		pugi::xml_node xmlSpawn = xmlNode.append_child();
		xmlSpawn.set_name(buf);

		xmlSpawn.append_attribute("id") = obj->m_Item.itemID;
		xmlSpawn.append_attribute("q")  = obj->m_Item.quantity;
	}
}
