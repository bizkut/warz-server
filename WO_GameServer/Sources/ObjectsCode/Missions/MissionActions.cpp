//=========================================================================
//	Module: MissionActions.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================


#include "r3dPCH.h"
#include "r3d.h"

#include "MissionActions.h"
#include "MissionCommands.h"
#include "GameCommon.h"

//-------------------------------------------------------------------------

#ifdef MISSIONS
namespace Mission
{

static uint32_t ActionIDs = 0;

//-------------------------------------------------------------------------

Action::Action()
	: m_ID( ++ActionIDs )
	, m_mapIcon( ACTICON_NotSet )
	, m_areaRestriction( NULL )
	, m_inventoryRestrictions( NULL )
	, m_commands( NULL )
	, m_actionType( ACT_Undefined )
{ }

Action::~Action()
{
	if( m_inventoryRestrictions )
	{
		m_inventoryRestrictions->clear();
		delete m_inventoryRestrictions;
		m_inventoryRestrictions = NULL;
	}
	if( m_commands )
	{
		for( std::vector< Command* >::iterator iter = m_commands->begin();
			 iter != m_commands->end(); ++iter )
		{
			delete *iter;
			*iter = NULL;
		}
		m_commands->clear();
	}
}

bool Action::loadAction( pugi::xml_node& xmlAction )
{
	if( xmlAction.empty() )
	{
		r3dOutToLog("!!! Action not defined!\n");
		return false;
	}

	int icon = 1;
	while( icon < ACTICON_MAX &&
		   _tcsnicmp( xmlAction.attribute("mapIcon").value(),
					 ActionIconNames[ icon - 1 ],
					 _tcslen( ActionIconNames[ icon - 1 ] ) ) )
	{
		++icon;
	}
	if( icon == ACTICON_MAX )
		icon = 0;
	m_mapIcon = (EActionIcon)icon;

	uint32_t areaHash = xmlAction.attribute("areaHash").as_uint();
	if( areaHash > 0 )
	{
		m_areaRestriction = (obj_MissionArea*)GameWorld().GetObjectByHash( areaHash );
		if( !m_areaRestriction )
		{
			r3dOutToLog( "!!! Couldn't locate mission area restriction by hashID.\n" );
			return false;
		}
	}

	pugi::xml_node inventoryRestrictNode = xmlAction.child("InventoryRestriction");
	while( !inventoryRestrictNode.empty() )
	{
		if( !m_inventoryRestrictions )
			m_inventoryRestrictions = game_new std::vector< InventoryRestriction >;

		InventoryRestriction restriction;
		restriction.m_itemID = inventoryRestrictNode.attribute("itemID").as_uint();
		restriction.m_amount = inventoryRestrictNode.attribute("amount").as_uint();
		if( restriction.m_amount == 0 )
		{
			r3dOutToLog("!!! Action Inventory Restriction amount must be greater than zero!\n");
			return false;
		}

		m_inventoryRestrictions->push_back( restriction );
		inventoryRestrictNode = inventoryRestrictNode.next_sibling("InventoryRestriction");
	}

	uint32_t commandCount = 0;
	pugi::xml_node xmlCommand = xmlAction.child("Command");
	while( !xmlCommand.empty() )
	{
		if( !m_commands )
			m_commands = game_new std::vector< Command* >;

		++commandCount;
		Command* command = (Command*)game_new Command;
		if( !command->loadCommand( xmlCommand ) )
		{
			r3dOutToLog("!!! Failed to load command %d for '%s' action.\n", commandCount, ActionTypeNames[ m_actionType - 1 ]);
			delete command;
			return false;
		}
		if( command->m_commandType != CMD_Remove )
		{
			r3dOutToLog("!!! Failed to load command %d for '%s' action.  Only '%s' command types are supported for actions.\n", commandCount, ActionTypeNames[ m_actionType - 1 ], CommandTypeNames[ CMD_Remove - 1 ]);
			delete command;
			return false;
		}
		if( command->m_orderType != ORDER_End )
		{
			r3dOutToLog("!!! Failed to load command %d for '%s' action.  Only '%s' ordered commands are supported for actions.\n", commandCount, ActionTypeNames[ m_actionType - 1 ], OrderTypeNames[ ORDER_End - 1 ]);
			delete command;
			return false;
		}
		m_commands->push_back( command );

		xmlCommand = xmlCommand.next_sibling("Command");
	}

	return true;
}

//-------------------------------------------------------------------------

GotoAction::GotoAction()
{
	m_actionType = ACT_Goto;
}

GotoAction::~GotoAction()
{
}

bool GotoAction::loadAction( pugi::xml_node& xmlAction )
{
	if( xmlAction.empty() )
	{
		r3dOutToLog("!!! Goto Action not defined!\n");
		return false;
	}

	bool bRet = Action::loadAction( xmlAction );
	if( bRet && !m_areaRestriction )
	{
		r3dOutToLog( "!!! Goto Action requires a mission area to be specified!\n" );
		return false;
	}
	return bRet;
}

//-------------------------------------------------------------------------

ItemAction::ItemAction()
	: m_itemID( 0 )
	, m_objHash( 0 )
	, m_amount( 0 )
	, m_target( 0 )
{
	m_actionType = ACT_Item;
}

ItemAction::~ItemAction()
{
}

bool ItemAction::loadAction( pugi::xml_node& xmlAction )
{
	if( xmlAction.empty() )
	{
		r3dOutToLog("!!! Item Action not defined!\n");
		return false;
	}

	m_itemID = xmlAction.attribute("itemID").as_uint();
	m_objHash = xmlAction.attribute("itemHash").as_uint();
	m_amount = xmlAction.attribute("amount").as_uint();

	// If an amount isn't specified, one is assumed.
	if( m_amount <= 0 )
		m_amount = 1;

	int action = 0;
	while( action < ITEM_MAX - 1 &&
		   _tcsnicmp( xmlAction.attribute("action").value(),
					 ItemActionTypeNames[ action ],
					 _tcslen( ItemActionTypeNames[ action ] ) ) )
	{
		++action;
	}
	if( action >= (int)ITEM_MAX - 1 )
	{
		r3dOutToLog( "!!! Unknown item action.\n" );
		return false;
	}
	m_itemAction = (EItemActionType)(action + 1); // Enum starts at 1, not 0, so that the checkboxes will work properly in the editor.

	// Targeting an Item Action is currently only supported by the "Use".
	if( m_itemAction == ITEM_Use )
	{
		int target = 1;
		while( target < ITEMUSEON_MAX &&
			   _tcsnicmp( xmlAction.attribute("target").value(),
						 ItemUseOnTargetNames[ target - 1 ],
						 _tcslen( ItemUseOnTargetNames[ target - 1 ] ) ) )
		{
			++target;
		}
		if( target == ITEMUSEON_MAX )
			target = 0;
		m_target = (EItemUseOnTarget)target;
	}
	else if( m_itemAction == ITEM_Deliver )
	{
		if( m_itemID <= 0 )
		{
			r3dOutToLog("!!! Deliver ItemAction requires an itemID!\n");
			return false;
		}
	}

	return Action::loadAction( xmlAction );
}

//-------------------------------------------------------------------------

KillAction::KillAction()
	: m_objType( 0 )
	, m_amount( 0 )

{
	m_actionType = ACT_Kill;
}

KillAction::~KillAction()
{
	m_weaponIDRestrictions.clear();
	m_objTypeIDRestrictions.clear();
}

bool KillAction::loadAction( pugi::xml_node& xmlAction )
{
	if( xmlAction.empty() )
	{
		r3dOutToLog("!!! Kill Action not defined!\n");
		return false;
	}

	m_amount	= xmlAction.attribute("amount").as_uint();

	int objType = 0;
	while( objType < KILL_OBJTYPE_MAX &&
		   _tcsnicmp( xmlAction.attribute("objType").value(),
					 KillObjectTypeNames[ objType ],
					 _tcslen( KillObjectTypeNames[ objType ] ) ) )
	{
		++objType;
	}
	if( objType == KILL_OBJTYPE_MAX )
		objType = 0;
	m_objType	= KillObjectType[ objType ];

	if( m_objType <= 0 )
	{
		r3dOutToLog( "!!! Need a valid object type to kill.\n" );
		return false;
	}
	if( m_amount <= 0 )
	{
		r3dOutToLog( "!!! Need to know how many kills are required.\n" );
		return false;
	}

	pugi::xml_node weaponRestrictNode = xmlAction.child("WeaponRestriction");
	while( !weaponRestrictNode.empty() )
	{
		m_weaponIDRestrictions.insert( weaponRestrictNode.attribute("weaponID").as_uint() );
		weaponRestrictNode = weaponRestrictNode.next_sibling("WeaponRestriction");
	}

	pugi::xml_node objTypeRestrictNode = xmlAction.child("ObjTypeRestriction");
	while( !objTypeRestrictNode.empty() )
	{
		m_objTypeIDRestrictions.insert( objTypeRestrictNode.attribute("id").as_uint() );
		objTypeRestrictNode = objTypeRestrictNode.next_sibling("ObjTypeRestriction");
	}
	
	return Action::loadAction( xmlAction );
}

//-------------------------------------------------------------------------

StateAction::StateAction()
	: m_objHash( 0 )
{
	m_actionType = ACT_State;
}

StateAction::~StateAction()
{
	m_stateSequence.clear();
}

bool StateAction::loadAction( pugi::xml_node& xmlAction )
{
	if( xmlAction.empty() )
	{
		r3dOutToLog("!!! State Action not defined!\n");
		return false;
	}

	m_objHash = xmlAction.attribute("itemHash").as_uint();

	pugi::xml_node sequenceNode = xmlAction.child("Sequence");
	while( !sequenceNode.empty() )
	{
		int state = 0;
		while( state < STATE_MAX - 1 &&
			   _tcsnicmp( sequenceNode.attribute("state").value(),
						 ActionStateNames[ state ],
						 _tcslen( ActionStateNames[ state ] ) ) )
		{
			++state;
		}
		if( state >= (int)STATE_MAX - 1 )
		{
			r3dOutToLog( "!!! Unknown state found in sequence.\n" );
			return false;
		}
		m_stateSequence.push_back( (EActionState)(state + 1) ); // Valid enums start at 1, not 0, so that the checkboxes will work properly in the editor.
		sequenceNode = sequenceNode.next_sibling("Sequence");
	}
	if( m_stateSequence.size() <= 1 )
	{
		r3dOutToLog( "!!! StateAction sequence must have at least a start and an end state.\n" );
		return false;
	}

	return Action::loadAction( xmlAction );
}

//-------------------------------------------------------------------------

}// namespace Mission
#endif