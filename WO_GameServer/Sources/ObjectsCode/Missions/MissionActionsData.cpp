//=========================================================================
//	Module: MissionActionsData.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "MissionActionsData.h"
#include "MissionActions.h"
#include "MissionCommands.h"
#include "..\obj_ServerMissionArea.h"
#include "..\obj_ServerPlayer.h"
#include "..\Zombies\sobj_Zombie.h"

//-------------------------------------------------------------------------

#ifdef MISSIONS
namespace Mission
{

ActionData::ActionData()
	: m_actionType( ACT_Undefined )
	, m_completed( false )
	, m_action( NULL )
{ }

ActionData::ActionData( const ActionData& data )
	: m_actionType( data.m_actionType )
	, m_completed( data.m_completed )
	, m_action( data.m_action )
{
	// Ensure the m_actionType is of the right type.
	r3d_assert( m_actionType == m_action->m_actionType );
}

ActionData::~ActionData()
{
}

ActionData& ActionData::operator =( const ActionData& data )
{
	m_actionType	= data.m_actionType;
	m_completed		= data.m_completed;
	m_action		= data.m_action;

	// Ensure the m_actionType is of the right type.
	r3d_assert( m_actionType == m_action->m_actionType );

	return *this;
}

void ActionData::Init( Action* action )
{
	r3d_assert( action );
	// Ensure the m_actionType is set in the derived ActionData's
	// constructor, and that the m_actionType is set in the
	// derived Action's constructor.
	r3d_assert( m_actionType == action->m_actionType );

	m_action	= action;
}

void ActionData::LoadActionData( const pugi::xml_node& xmlActionData )
{
#if _DEBUG
	r3d_assert( !xmlActionData.empty() );
#endif
	if( xmlActionData.empty() )
	{
		r3dOutToLog("Unable to load empty Action Data.\n");
		return;
	}

	int actionType = 0;
	while( actionType < ACT_MAX - 1 &&
		   _tcsnicmp( xmlActionData.attribute("type").value(),
					 ActionTypeNames[ actionType ],
					 _tcslen( ActionTypeNames[ actionType ] ) ) )
	{
		++actionType;
	}
#if _DEBUG
	r3d_assert( actionType < (int)ACT_MAX - 1 );
#endif
	if( actionType >= (int)ACT_MAX - 1 )
	{
		r3dOutToLog("Unknown action type found while loading Action Data.\n");
		return;
	};
#if _DEBUG
	r3d_assert( (EActionType)(actionType + 1) == m_action->m_actionType );
#endif
	if( (EActionType)(actionType + 1) != m_action->m_actionType )
	{
		r3dOutToLog("Action Type mismatch found while loading Action Data.\n");
		return;
	}
	m_actionType = (EActionType)(actionType + 1); // Valid enum starts at 1, not 0, so that the checkboxes will work properly in the editor.

	m_completed = xmlActionData.attribute("completed").as_bool();
}

void ActionData::SaveActionData( pugi::xml_node& xmlActionData )
{
	xmlActionData.append_attribute("type") = GetActionTypeName();
	xmlActionData.append_attribute("completed") = m_completed;
}

bool ActionData::MeetsAreaRestriction( obj_ServerPlayer* player )
{
	// Test player's location to check if it is outside an areaRestriction.
	// and return false if it is.
	if( m_action->m_areaRestriction &&
		!m_action->m_areaRestriction->Contains( player ) )
	{
			return false;
	}
	return true;
}

bool ActionData::MeetsInventoryRestriction( obj_ServerPlayer* player )
{
	if( m_action->m_inventoryRestrictions && m_action->m_inventoryRestrictions->size() > 0 )
	{
		// Player must meet all the Inventory Restrictions
		for( std::vector< Action::InventoryRestriction >::const_iterator iter = m_action->m_inventoryRestrictions->begin();
			 iter != m_action->m_inventoryRestrictions->end(); ++iter )
		{
			if( !player->IsHaveBackpackItem( iter->m_itemID, iter->m_amount ) )
				return false;
		}
	}
	return true;
}

void ActionData::ExecuteCommands( obj_ServerPlayer* player )
{
	if( m_completed && m_action->m_commands && m_action->m_commands->size() > 0 )
	{
		std::vector< Command* >* pCommandVec = m_action->m_commands;
		for( std::vector< Command* >::iterator cmdIter = pCommandVec->begin(); cmdIter != pCommandVec->end(); ++cmdIter )
		{
			if( *cmdIter && (*cmdIter)->m_commandType == CMD_Remove )
				(*cmdIter)->PerformCommand( ORDER_End, player );
		}
	}
}

const char* ActionData::GetActionTypeName() const
{
	return ActionTypeNames[ (int)m_actionType - 1 ];
}

//-------------------------------------------------------------------------

GotoActionData::GotoActionData()
{
	m_actionType = ACT_Goto;
}

GotoActionData::GotoActionData( const GotoActionData& data )
	: ActionData( (const ActionData&)data )
{
}

GotoActionData::~GotoActionData()
{
}

GotoActionData& GotoActionData::operator=( const GotoActionData& data )
{
	return (GotoActionData&)ActionData::operator =( (const ActionData&)data );
}

void GotoActionData::Init( Action* action )
{
	r3d_assert( action );

	ActionData::Init( action );
}

void GotoActionData::Update()
{
	// Nothing to do.
}

void GotoActionData::LoadActionData( const pugi::xml_node& xmlActionData )
{
#if _DEBUG
	r3d_assert( !xmlActionData.empty() );
#endif
	if( xmlActionData.empty() )
	{
		r3dOutToLog("Unable to load empty Goto Action Data.\n");
		return;
	}

	ActionData::LoadActionData( xmlActionData );
}

void GotoActionData::SaveActionData( pugi::xml_node& xmlActionData )
{
	ActionData::SaveActionData( xmlActionData );
}

bool GotoActionData::CheckPosition( obj_ServerPlayer* player )
{
	if( m_completed )
		return true;

	if( !player ||
		!MeetsInventoryRestriction( player ) )
		return false;

	m_completed = MeetsAreaRestriction( player );
	if( m_completed )
	{
		ExecuteCommands( player );
		obj_MissionArea* missionArea = (obj_MissionArea*)((GotoAction*)m_action)->m_areaRestriction;
		r3dPoint3D pos			= missionArea->GetPosition();
		r3dPoint3D playerPos	= player->GetPosition();
		r3dOutToLog( "Mission(%d): Player %s - COMPLETED Goto Action <%.2f, %.2f, %.2f> is inside (%.2f to %.2f, %.2f to %.2f, %.2f to %.2f)\n",
			m_action->m_ID,
			player->Name.c_str(),
			playerPos.x, playerPos.y, playerPos.z,
			pos.x - missionArea->m_extents.x, pos.x + missionArea->m_extents.x,
			pos.y - missionArea->m_extents.y, pos.y + missionArea->m_extents.y,
			pos.z - missionArea->m_extents.z, pos.z + missionArea->m_extents.z );
	}
	return m_completed;
}

//-------------------------------------------------------------------------

ItemActionData::ItemActionData()
	: m_count( 0 )
{
	m_actionType = ACT_Item;
}

ItemActionData::ItemActionData( const ItemActionData& data )
	: ActionData( (const ActionData&)data )
	, m_count( data.m_count )
{
}

ItemActionData::~ItemActionData()
{
}

ItemActionData& ItemActionData::operator =( const ItemActionData& data )
{
	m_count = data.m_count;
	return (ItemActionData&)ActionData::operator =( (const ActionData&)data );
}

void ItemActionData::Init( Action* action )
{
	r3d_assert( action );

	ActionData::Init( action );
}

void ItemActionData::Update()
{
	m_completed = ( m_count >= ((ItemAction*)m_action)->m_amount );
	//if( m_completed )
	//	r3dOutToLog( "Mission(%d): COMPLETED Item Perform Action (%s) %d/%d\n", m_action->m_ID, GetItemActionTypeName(), m_count, ((ItemAction*)m_action)->m_amount );
}

void ItemActionData::LoadActionData( const pugi::xml_node& xmlActionData )
{
#if _DEBUG
	r3d_assert( !xmlActionData.empty() );
#endif
	if( xmlActionData.empty() )
	{
		r3dOutToLog("Unable to load empty Item Action Data.\n");
		return;
	}

	m_count = xmlActionData.attribute("count").as_uint();

	ActionData::LoadActionData( xmlActionData );
}

void ItemActionData::SaveActionData( pugi::xml_node& xmlActionData )
{
	if( m_count > 0 )
		xmlActionData.append_attribute("count") = m_count;
	ActionData::SaveActionData( xmlActionData );
}

bool ItemActionData::DeliverItemAction( obj_ServerPlayer* player )
{
	if( !player )
		return false;

	ItemAction* action = (ItemAction*)m_action;
	if( action->m_itemAction != ITEM_Deliver ||
		( action->m_itemID > 0 && !player->IsHaveBackpackItem( action->m_itemID, action->m_amount ) ) ||
		!MeetsAreaRestriction( player ) ||
		!MeetsInventoryRestriction( player ) )
		return false;

	// Remove items from the player's backpack.
	wiInventoryItem wi;
	wi.itemID   = action->m_itemID;
	wi.quantity = (int)action->m_amount;	
	int numNotInBackpack = player->BackpackRemoveItem( wi );
	if( numNotInBackpack > 0 )
	{
		// Something happened, put the items back.
		wi.quantity -= numNotInBackpack;
		if( wi.quantity > 0 )
			player->BackpackAddItem( wi );
		return false;
	}

	m_count = action->m_amount;
	Update();
	ExecuteCommands( player );
	return true;
}

bool ItemActionData::PerformItemAction( obj_ServerPlayer* player, EItemActionType itemAction, uint32_t itemID, uint32_t hashID, EItemUseOnTarget target /*= ITEMUSEON_NotSpecified*/ )
{
	if( !player )
		return false;

	ItemAction* action = (ItemAction*)m_action;
	if( action->m_itemAction != itemAction ||
		( action->m_itemID > 0 && action->m_itemID != itemID ) ||
		( action->m_objHash > 0 && action->m_objHash != hashID ) ||
		( action->m_target > 0 && (action->m_target & target) == 0 ) ||
		!MeetsAreaRestriction( player ) ||
		!MeetsInventoryRestriction( player ) )
		return false;
	
	++m_count;
	r3dOutToLog( "Mission(%d): Player %s - Item Perform Action (%s) %d/%d\n", m_action->m_ID, player->Name.c_str(), GetItemActionTypeName(), m_count, action->m_amount );
	Update();
	ExecuteCommands( player );
	return true;
}

bool ItemActionData::RequiresSpecificItem()
{
	return ((ItemAction*)m_action)->m_objHash > 0;
}

const char* ItemActionData::GetItemActionTypeName() const
{
	return ItemActionTypeNames[ (int)(((ItemAction*)m_action)->m_itemAction) - 1 ];
}

//-------------------------------------------------------------------------

KillActionData::KillActionData()
	: m_count( 0 )
{
	m_actionType = ACT_Kill;
}

KillActionData::KillActionData( const KillActionData& data )
	: ActionData( (const KillActionData&)data )
	, m_count( data.m_count )
{
}

KillActionData::~KillActionData()
{
}

KillActionData& KillActionData::operator =( const KillActionData& data )
{
	m_count = data.m_count;
	return (KillActionData&)ActionData::operator =( (const ActionData&)data );
}

void KillActionData::Init( Action* action )
{
	r3d_assert( action );

	ActionData::Init( action );
}

void KillActionData::Update()
{
	m_completed = ( m_count >= ((KillAction*)m_action)->m_amount );
	//if( m_completed )
	//	r3dOutToLog( "Mission(%d): COMPLETED Kill Action %d/%d\n", m_action->m_ID, m_count, ((KillAction*)m_action)->m_amount );
}

void KillActionData::LoadActionData( const pugi::xml_node& xmlActionData )
{
#if _DEBUG
	r3d_assert( !xmlActionData.empty() );
#endif
	if( xmlActionData.empty() )
	{
		r3dOutToLog("Unable to load empty Kill Action Data.\n");
		return;
	}

	m_count = xmlActionData.attribute("count").as_uint();

	ActionData::LoadActionData( xmlActionData );
}

void KillActionData::SaveActionData( pugi::xml_node& xmlActionData )
{
	if( m_count > 0 )
		xmlActionData.append_attribute("count") = m_count;
	ActionData::SaveActionData( xmlActionData );
}

bool KillActionData::AddKill( obj_ServerPlayer* player, GameObject* victim, uint32_t weaponID )
{
	if( m_completed )
		return false;

	if( !player || !victim )
		return false;

	KillAction* killAction = (KillAction*)m_action;
	if( 0 < killAction->m_weaponIDRestrictions.size() &&
		killAction->m_weaponIDRestrictions.find( weaponID ) == killAction->m_weaponIDRestrictions.end() )
		return false;

	if( !victim->isObjType( killAction->m_objType ) )
		return false;

	if( 0 < killAction->m_objTypeIDRestrictions.size() )
	{
		switch( killAction->m_objType )
		{
		case OBJTYPE_Human:
			{
				obj_ServerPlayer* plr = (obj_ServerPlayer*)victim;
				if( killAction->m_objTypeIDRestrictions.find( plr->loadout_->HeroItemID ) == killAction->m_objTypeIDRestrictions.end() )
					return false;
			}
			break;

		case OBJTYPE_Zombie:
			{
				obj_Zombie* zombie = (obj_Zombie*)victim;
				if( killAction->m_objTypeIDRestrictions.find( zombie->HeroItemID ) == killAction->m_objTypeIDRestrictions.end() )
					return false;
			}
			break;

		default:
			r3d_assert( false && "unsupported objType restriction" );
			return false;
		}
	}

	if( !MeetsAreaRestriction( player ) ||
		!MeetsInventoryRestriction( player ) )
		return false;

	++m_count;
	r3dOutToLog( "Mission(%d): Player %s - Kill Action %d/%d\n", m_action->m_ID, player->Name.c_str(), m_count, ((KillAction*)m_action)->m_amount );
	Update();
	ExecuteCommands( player );
	return true;
}

//-------------------------------------------------------------------------

StateActionData::StateActionData()
{
	m_actionType = ACT_State;
}

StateActionData::StateActionData( const StateActionData& data )
	: ActionData( (const StateActionData&)data )
{
	m_stateIter = std::find( ((StateAction*)m_action)->m_stateSequence.begin(), ((StateAction*)m_action)->m_stateSequence.end(), *data.m_stateIter );
	r3d_assert( m_stateIter != ((StateAction*)m_action)->m_stateSequence.end() && "State Action Sequence must have a sequence." );
}

StateActionData::~StateActionData()
{
}

StateActionData& StateActionData::operator =( const StateActionData& data )
{
	m_stateIter = std::find( ((StateAction*)m_action)->m_stateSequence.begin(), ((StateAction*)m_action)->m_stateSequence.end(), *data.m_stateIter );
	r3d_assert( m_stateIter != ((StateAction*)m_action)->m_stateSequence.end() && "State Action Sequence must have a sequence." );
	return (StateActionData&)ActionData::operator =( (const ActionData&)data );
}

void StateActionData::Init( Action* action )
{
	r3d_assert( action );

	m_stateIter = ((StateAction*)action)->m_stateSequence.begin();
	r3d_assert( m_stateIter != ((StateAction*)action)->m_stateSequence.end() && "State Action Sequence must have a sequence." );

	ActionData::Init( action );
}

void StateActionData::Update()
{
	m_completed = ( (*m_stateIter) == ((StateAction*)m_action)->m_stateSequence.back() );
	//if( m_completed )
	//	r3dOutToLog( "Mission(%d): COMPLETED State Action (%s)\n", m_action->m_ID, GetActionStateName() );
}

void StateActionData::LoadActionData( const pugi::xml_node& xmlActionData )
{
#if _DEBUG
	r3d_assert( !xmlActionData.empty() );
#endif
	if( xmlActionData.empty() )
	{
		r3dOutToLog("Unable to load empty State Action Data.\n");
		return;
	}

	int actionState = 0;
	while( actionState < STATE_MAX - 1 &&
		   _tcsnicmp( xmlActionData.attribute("state").value(),
					 ActionStateNames[ actionState ],
					 _tcslen( ActionStateNames[ actionState ] ) ) )
	{
		++actionState;
	}
#if _DEBUG
	r3d_assert( actionState < (int)STATE_MAX - 1 );
#endif
	if( actionState >= (int)STATE_MAX - 1 )
	{
		r3dOutToLog("Unknown action state found while loading State Action Data.\n");
		return;
	}
	// Valid enum starts at 1, not 0, so that the checkboxes will work properly in the editor.
	m_stateIter = std::find(((StateAction*)m_action)->m_stateSequence.begin(), ((StateAction*)m_action)->m_stateSequence.end(), (EActionType)(actionState + 1) );
#if _DEBUG
	r3d_assert( m_stateIter != ((StateAction*)m_action)->m_stateSequence.end() );
#endif
	if( m_stateIter == ((StateAction*)m_action)->m_stateSequence.end() )
	{
		r3dOutToLog("Unable to find action state in State Action Data's sequence.\n");
		return;
	}

	ActionData::LoadActionData( xmlActionData );
}

void StateActionData::SaveActionData( pugi::xml_node& xmlActionData )
{
#if _DEBUG
	r3d_assert( m_stateIter != ((StateAction*)m_action)->m_stateSequence.end() );
#endif
	if( m_stateIter == ((StateAction*)m_action)->m_stateSequence.end() )
	{
		r3dOutToLog("State Action Data sequence iterator found in bad state.\n");
		return;
	}
	xmlActionData.append_attribute("state") = GetActionStateName();
	ActionData::SaveActionData( xmlActionData );
}

bool StateActionData::DecState( obj_ServerPlayer* player, GameObject* item )
{
	if( m_completed )
		return false;

	if( !player || !item )
		return false;

	if( item->GetHashID() != ((StateAction*)m_action)->m_objHash ||
		!MeetsAreaRestriction( player ) ||
		!MeetsInventoryRestriction( player ) )
		return false;

	if( m_stateIter == ((StateAction*)m_action)->m_stateSequence.begin() )
		return false;

	--m_stateIter;
	r3dOutToLog( "Mission(%d): Player %s - Decrement State Action (%s)\n", m_action->m_ID, player->Name.c_str(), GetActionStateName() );
	Update();
	ExecuteCommands( player );
	return true;
}

bool StateActionData::IncState( obj_ServerPlayer* player, GameObject* item )
{
	if( m_completed )
		return false;

	if( !player || !item )
		return false;

	if( item->GetHashID() != ((StateAction*)m_action)->m_objHash ||
		!MeetsAreaRestriction( player ) ||
		!MeetsInventoryRestriction( player ) )
		return false;

	if( m_stateIter == ( ((StateAction*)m_action)->m_stateSequence.end() - 1 ) )
		return false;

	++m_stateIter;
	r3dOutToLog( "Mission(%d): Player %s - Increment State Action (%s)\n", m_action->m_ID, player->Name.c_str(), GetActionStateName() );
	Update();
	ExecuteCommands( player );
	return true;
}

EActionState StateActionData::GetState( GameObject* item )
{
	if( item->GetHashID() != ((StateAction*)m_action)->m_objHash )
		return STATE_Unknown;

	return *m_stateIter;
}

const char* StateActionData::GetActionStateName() const
{
	return ActionStateNames[ (*m_stateIter) - 1];
}

//-------------------------------------------------------------------------

}// namespace Mission
#endif