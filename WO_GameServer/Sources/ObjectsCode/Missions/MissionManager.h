//=========================================================================
//	Module: MissionManager.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

#include <map>
#include <vector>

#include "MissionEnums.h"

namespace Mission
{
class Command;
class Action;
class Reward;
class Timer;
}
class GameObject;
class obj_ServerPlayer;

//-------------------------------------------------------------------------

namespace Mission
{

// These limits are imposed by MissionProgress Save/Load, but should be enough.
static const uint32_t MaxStages		= 255;
static const uint32_t MaxObjectives	= 255;
static const uint32_t MaxActions	= 255;

class Objective
{
	Objective( const Objective& objective ) { }
	Objective& operator=( const Objective& objective ) { }

public:
	Objective();
	~Objective();

	bool loadObjective( pugi::xml_node& xmlObjective );

public:
	uint32_t				m_mustCompleteNumActions;
	std::vector< Action* >	m_actions;
	std::vector< Reward* >	m_rewards;
};

//-------------------------------------------------------------------------

class Stage
{
	Stage( const Stage& stage ) { }
	Stage& operator=( const Stage& stage ) { }

public:
	Stage();
	~Stage();

	bool loadStage( pugi::xml_node& xmlStage );

public:
	uint32_t					m_triggerObjHash;
	std::vector< Objective* >	m_objectives;
};

//-------------------------------------------------------------------------

class Mission
{
	Mission( const Mission& mission ) { }
	Mission& operator=( const Mission& mission ) { }

public:
	Mission();
	~Mission();

	bool loadMission( pugi::xml_node& xmlMission );

public:
	// Needs to match the lengths of the PKT_S2C_MissionActivate_s character arrays.
	static const uint32_t	StringIDLen = 16;
	static const uint32_t	IconPathLen	= 64;

	uint32_t	m_ID;
	uint32_t	m_version;	// If this goes beyond 255, Save/Load needs to be modified.
	TCHAR		m_name[StringIDLen];
	TCHAR		m_desc[StringIDLen];
	TCHAR		m_icon[IconPathLen];
	bool		m_active;
	bool		m_disallowSave;
	uint32_t	m_triggerObjHash;

	Timer*		m_expirationTimer;

	std::map< EOrderType, std::vector< Command* >* >	m_commands;
	std::vector< Stage* >	m_stages;
	std::vector< Reward* >	m_rewards;
};

//=========================================================================

class MissionManager
{
	MissionManager( const MissionManager& missionMgr ) { }
	MissionManager& operator=( const MissionManager& missionMgr ) { }

public:
	MissionManager();
	~MissionManager();

	bool		Init();
	void		Update();
	void		RemoveInactiveMissionsFromPlayers();
	void		ResetMissionData( obj_ServerPlayer* player );

#ifdef MISSION_TRIGGERS
	uint32_t	TriggerMission( GameObject* triggerObj );
#endif

	void		GetAvailableMissionIDs( std::vector< uint32_t >& missionIDs );
	uint32_t	GetNumRequiredActions( uint32_t missionID, uint32_t stageIndex, uint32_t objectiveIndex );
	uint32_t	GetTriggerObjectHash( uint32_t missionID, uint32_t stageIndex = UINT_MAX );
	bool		IsMissionActive( uint32_t missionID );
	void		SetMissionActive( uint32_t missionID, bool active );
	bool		IsMissionSaveDisallowed( uint32_t missionID );

	void		SendActiveMissionsToPlayer( const obj_ServerPlayer& player );
public:
	std::map< uint32_t, Mission* >	m_mapMissions;
};
extern MissionManager* g_pMissionMgr;

//=========================================================================

} // namespace Mission
#endif