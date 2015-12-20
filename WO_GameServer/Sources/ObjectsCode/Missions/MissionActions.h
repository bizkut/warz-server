//=========================================================================
//	Module: MissionActions.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

#include <vector>
#include <unordered_set>

#include "MissionEnums.h"

class GameObject;
class obj_MissionArea;
namespace Mission { class Command; }

//-------------------------------------------------------------------------

namespace Mission
{
// Actions are used to build a player's ActionData, as well as compare the
// ActionData against to check if the player has completed the Action.

class Action
{
	Action( const Action& action ) : m_ID( 0 ) { }
	Action& operator=( const Action& action ) { }

public:
	Action();
	virtual ~Action();

	virtual bool loadAction( pugi::xml_node& xmlAction );

public:
	struct InventoryRestriction
	{
		uint32_t	m_amount;
		uint32_t	m_itemID;
	};

public:
	const uint32_t		m_ID;

	EActionIcon			m_mapIcon;
	obj_MissionArea*	m_areaRestriction;
	std::vector< InventoryRestriction >*	m_inventoryRestrictions;
	std::vector< Command* >*				m_commands;

	EActionType	m_actionType;
};

//-------------------------------------------------------------------------

class GotoAction : public Action
{
	GotoAction( const GotoAction& action ) { }
	GotoAction& operator=( const GotoAction& action ) { }

public:
	GotoAction();
	virtual ~GotoAction();

	virtual bool loadAction( pugi::xml_node& xmlAction );
};

//-------------------------------------------------------------------------

class ItemAction : public Action
{
	ItemAction( const ItemAction& action ) { }
	ItemAction& operator=( const ItemAction& action ) { }

public:
	ItemAction();
	virtual ~ItemAction();

	virtual bool loadAction( pugi::xml_node& xmlAction );

public:
	uint32_t		m_itemID;
	uint32_t		m_objHash;
	uint32_t		m_amount;
	uint32_t		m_target;
	EItemActionType	m_itemAction;
};

//-------------------------------------------------------------------------

class KillAction : public Action
{
	KillAction( const KillAction& action ) { }
	KillAction& operator=( const KillAction& action ) { }

public:
	KillAction();
	virtual ~KillAction();

	virtual bool loadAction( pugi::xml_node& xmlAction );

public:
	uint32_t	m_objType;
	uint32_t	m_amount;
	std::tr1::unordered_set< uint32_t >	m_weaponIDRestrictions;
	std::tr1::unordered_set< uint32_t >	m_objTypeIDRestrictions;
};

//-------------------------------------------------------------------------

class StateAction : public Action
{
	StateAction( const StateAction& action ) { }
	StateAction& operator=( const StateAction& action ) { }

public:
	StateAction();
	virtual ~StateAction();

	virtual bool loadAction( pugi::xml_node& xmlAction );

public:
	uint32_t					m_objHash;
	std::vector< EActionState >	m_stateSequence;
};

//-------------------------------------------------------------------------

} // namespace Mission
#endif