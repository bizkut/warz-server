//=========================================================================
//	Module: MissionCommands.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "MissionCommands.h"
#include "GameCommon.h"
#include "..\obj_ServerItemSpawnPoint.h"
#include "..\sobj_SpawnedItem.h"
#include "..\obj_ServerPlayer.h"
#include "..\sobj_DroppedItem.h"

//-------------------------------------------------------------------------

#ifdef MISSIONS
namespace Mission
{

Command::Command()
	: m_commandType( CMD_Undefined )
	, m_orderType( ORDER_Unknown )
	, m_itemID( 0 )
	, m_hashID( 0 )
	, m_count( 0 )
{
}

Command::~Command()
{
}

bool Command::loadCommand( pugi::xml_node& xmlCmd )
{
	if( xmlCmd.empty() )
	{
		r3dOutToLog( "!!! Command not defined!\n" );
		return false;
	}

	// Command
	int cmd = 0;
	while( cmd < CMD_MAX - 1 &&
		   _tcsnicmp( xmlCmd.attribute("cmd").value(),
					 CommandTypeNames[ cmd ],
					 _tcslen( CommandTypeNames[ cmd ] ) ) )
	{
		++cmd;
	}
	if( cmd >= (int)CMD_MAX - 1 )
	{
		r3dOutToLog( "!!! Unknown command.\n" );
		return false;
	}
	m_commandType = (ECommandType)(cmd + 1); // Valid enum starts at 1, not 0, so that the checkboxes will work properly in the editor.
	
	// Order
	int order = 0;
	while( order < ORDER_MAX - 1 &&
		   _tcsnicmp( xmlCmd.attribute("when").value(),
					 OrderTypeNames[ order ],
					 _tcslen( OrderTypeNames[ order ] ) ) )
	{
		++order;
	}
	if( order >= (int)ORDER_MAX - 1 )
	{
		r3dOutToLog( "!!! When to execute command is unknown.\n" );
		return false;
	}
	m_orderType = (EOrderType)(order + 1); // Valid enum starts at 1, not 0, so that the checkboxes will work properly in the editor.

	m_itemID = xmlCmd.attribute("itemID").as_uint();
	if( 0 >= m_itemID )
	{
		r3dOutToLog( "!!! Item ID must be specified for Commands.\n" );
		return false;
	}
	m_count = xmlCmd.attribute("count").as_uint();
	if( m_count == 0 )
		m_count = 1;

	switch( m_commandType )
	{
	case CMD_Spawn:
		m_hashID = xmlCmd.attribute("spawnerHash").as_uint();
		if( 0 >= m_hashID )
		{
			r3dOutToLog( "!!! A Server Spawn Point Hash ID must be specified for a Spawn Command.\n" );
			return false;
		}
		break;

	default:
	case CMD_Destroy:
		break;
	}
	return true;
}

GameObject* Command::PerformCommand( EOrderType orderType, obj_ServerPlayer* player )
{
	// Sanity Check
	r3d_assert( orderType == m_orderType );
	if( orderType != m_orderType )
		return NULL;

	switch( m_commandType )
	{
	case CMD_Spawn:		return Spawn();
	case CMD_Destroy:	Destroy();				break;
	case CMD_Give:		return Give( player );	break;
	case CMD_Remove:	Remove( player );		break;
	default:
		r3d_assert(false && "Unsupported Action Data Type" );
		break;
	}
	return NULL;
}

GameObject* Command::Spawn()
{
	GameObject* obj = GameWorld().GetObjectByHash( m_hashID );
	r3d_assert( obj && obj->isObjType( OBJTYPE_ItemSpawnPoint ) && "Hash ID must correspond to a Server Item Spawn Point." );
	if( !obj || !obj->isObjType( OBJTYPE_ItemSpawnPoint ) )
		return NULL;

	obj_ServerItemSpawnPoint* spawner = (obj_ServerItemSpawnPoint*)obj;
	wiInventoryItem wi;
	wi.itemID   = m_itemID;
	wi.quantity = m_count;	

	// TODO: There needs to be a way to restrict an item to a player,
	//		 or a group (for future group missions), so that other
	//		 players cannot interfere with a mission.

	obj_SpawnedItem* item = spawner->CreateSpawnedItem( 0, wi );

	r3d_assert( item );
	r3dOutToLog("Mission: Command Spawned %d instances of item (%d), '%s'\n", m_count, m_itemID, item->Name.c_str());

	// TODO: If other commands are to be auto-generated to handle
	//		 this particular item, then this is where to create them.
	//		 There will need to be a way to pass the command back to
	//		 the MissionManager, so that it can be inserted in the
	//		 proper Command set.

	return item;
}

void Command::Destroy()
{
	// TODO: Destroy the object in the immediate area surrounding the player.
	// NOTE: This may not be useful, since a player could always immediately
	//		 pick up an item again, right after Dropping it (to satisfy a
	//		 'Drop' ItemAction, or have a friend pick it up, so we don't just
	//		 go through their backpack and remove it.
}

GameObject* Command::Give( obj_ServerPlayer* player )
{
	r3d_assert( player );
	if( !player ) return NULL;

	// Place items in the player's backpack.
	wiInventoryItem wi;
	wi.itemID   = m_itemID;
	wi.quantity = (int)m_count;	
	if( !player->BackpackAddItem(wi) )
	{
		// create network object
		r3dPoint3D pos = player->GetRandomPosForItemDrop();
		obj_DroppedItem* obj = (obj_DroppedItem*)srv_CreateGameObject("obj_DroppedItem", "obj_DroppedItem", pos);
		player->SetupPlayerNetworkItem(obj);
		// vars
		obj->m_Item = wi;

		r3dOutToLog("Mission: Command Give, Not enough room in Player %s's backpack, itemID(%d) placed on ground at <%0.2f, %0.2f, %0.2f> \n", player->Name.c_str(), m_itemID, pos.x, pos.y, pos.z);
		return obj;
	}
	r3dOutToLog("Mission: Command Give, Player %s got itemID(%d) * %d\n", player->Name.c_str(), m_itemID, m_count);
	return NULL;
}

void Command::Remove( obj_ServerPlayer* player )
{
	r3d_assert( player );
	if( !player ) return;

	// Remove items from the player's backpack.
	wiInventoryItem wi;
	wi.itemID   = m_itemID;
	wi.quantity = (int)m_count;	
	int numNotInBackpack = player->BackpackRemoveItem( wi );
	if( numNotInBackpack > 0 )
	{
		// TODO: If it does not exist then maybe search the
		//		 immediate area and destroy them if found.
	}
	r3dOutToLog("Mission: Command Remove, itemID(%d) * %d from Player %s\n", m_itemID, m_count - numNotInBackpack, player->Name.c_str());
}

//-------------------------------------------------------------------------

}// namespace Mission
#endif