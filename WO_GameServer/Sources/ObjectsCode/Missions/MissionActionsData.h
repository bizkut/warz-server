//=========================================================================
//	Module: MissionActionsData.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

#include <vector>

#include "MissionEnums.h"

namespace Mission
{
class Action;
}
class obj_ServerPlayer;
class GameObject;

//-------------------------------------------------------------------------

namespace Mission
{
// ActionData is per player mission state data.

class ActionData
{
public:
	ActionData();
	ActionData( const ActionData& data );
	virtual ~ActionData();

	virtual ActionData& operator=( const ActionData& data );

	virtual void Init( Action* action );
	virtual void Update() = 0;

	virtual void LoadActionData( const pugi::xml_node& xmlActionData );
	virtual void SaveActionData( pugi::xml_node& xmlActionData );

	bool MeetsAreaRestriction( obj_ServerPlayer* player );
	bool MeetsInventoryRestriction( obj_ServerPlayer* player );

	void ExecuteCommands( obj_ServerPlayer* player );

	const char*	GetActionTypeName() const;

public:
	EActionType			m_actionType;
	bool				m_completed;
	Action*				m_action;
};

//-------------------------------------------------------------------------

class GotoActionData : public ActionData
{
public:
	GotoActionData();
	GotoActionData( const GotoActionData& data );
	virtual ~GotoActionData();

	virtual GotoActionData& operator=( const GotoActionData& data );

	virtual void Init( Action* action );
	virtual void Update();

	virtual void LoadActionData( const pugi::xml_node& xmlActionData );
	virtual void SaveActionData( pugi::xml_node& xmlActionData );

	bool CheckPosition( obj_ServerPlayer* player );
};

//-------------------------------------------------------------------------

class ItemActionData : public ActionData
{
public:
	ItemActionData();
	ItemActionData( const ItemActionData& data );
	virtual ~ItemActionData();

	virtual ItemActionData& operator=( const ItemActionData& data );

	virtual void Init( Action* action );
	virtual void Update();

	virtual void LoadActionData( const pugi::xml_node& xmlActionData );
	virtual void SaveActionData( pugi::xml_node& xmlActionData );

	bool DeliverItemAction( obj_ServerPlayer* player );
	bool PerformItemAction( obj_ServerPlayer* player, EItemActionType itemAction, uint32_t itemID, uint32_t hashID, EItemUseOnTarget target = ITEMUSEON_NotSpecified );

	bool RequiresSpecificItem();

	const char* GetItemActionTypeName() const;

public:
	uint32_t	m_count;
};

//-------------------------------------------------------------------------

class KillActionData : public ActionData
{
public:
	KillActionData();
	KillActionData( const KillActionData& data );
	virtual ~KillActionData();

	virtual KillActionData& operator=( const KillActionData& data );

	virtual void Init( Action* action );
	virtual void Update();

	virtual void LoadActionData( const pugi::xml_node& xmlActionData );
	virtual void SaveActionData( pugi::xml_node& xmlActionData );

	bool AddKill( obj_ServerPlayer* player, GameObject* victim, uint32_t weaponID );

public:
	uint32_t	m_count;
};

//-------------------------------------------------------------------------

class StateActionData : public ActionData
{
public:
	StateActionData();
	StateActionData( const StateActionData& data );
	virtual ~StateActionData();

	virtual StateActionData& operator=( const StateActionData& data );

	virtual void Init( Action* action );
	virtual void Update();

	virtual void LoadActionData( const pugi::xml_node& xmlActionData );
	virtual void SaveActionData( pugi::xml_node& xmlActionData );

	bool DecState( obj_ServerPlayer* player, GameObject* item );
	bool IncState( obj_ServerPlayer* player, GameObject* item );

	EActionState GetState( GameObject* item );

	const char*	GetActionStateName() const;

public:
	std::vector< EActionState >::iterator	m_stateIter;
};

//-------------------------------------------------------------------------

} // namespace Mission
#endif