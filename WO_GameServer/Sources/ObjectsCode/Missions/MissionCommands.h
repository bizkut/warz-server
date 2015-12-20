//=========================================================================
//	Module: MissionCommands.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

#include "MissionEnums.h"

class GameObject;
class obj_ServerPlayer;

//-------------------------------------------------------------------------

namespace Mission
{
// Commands are used to prepare mission requirements and clean up at the end.

class Command
{
	Command( const Command& cmd ) { }
	Command& operator=( const Command& cmd ) { }

public:
	Command();
	virtual ~Command();

	virtual bool		loadCommand( pugi::xml_node& xmlCmd );
	virtual GameObject*	PerformCommand( EOrderType orderType, obj_ServerPlayer* player );

protected:
	GameObject*			Spawn();
	void				Destroy();
	GameObject*			Give( obj_ServerPlayer* player );
	void				Remove( obj_ServerPlayer* player );

public:
	ECommandType	m_commandType;
	EOrderType		m_orderType;
	uint32_t		m_itemID;
	uint32_t		m_hashID;
	uint32_t		m_count;
};

//-------------------------------------------------------------------------

} // namespace Mission
#endif