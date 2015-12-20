//=========================================================================
//	Module: MissionProgress.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

#include <map>
#include <vector>

#include "MissionEnums.h"

namespace Mission
{
class ActionData;
}

class obj_ServerPlayer;
class GameObject;

//-------------------------------------------------------------------------

namespace Mission
{

// MissionsProgress is a per player set of MissionStateData that tracks the
// player's progress through each of the missions.

typedef std::vector< ActionData* > ActionDataVec;
typedef std::vector< ActionDataVec* > ObjectiveDataVec;
typedef std::vector< ObjectiveDataVec* > StageDataVec;
struct MissionStateData
{
	uint32_t		m_missionID;
	uint32_t		m_stageIndex;
	uint32_t		m_objectiveIndex;
	bool			m_stageTriggered;
	StageDataVec	m_stageDataVec;
};

//-------------------------------------------------------------------------

class MissionsProgress
{
protected:
	MissionStateData* FindMission( uint32_t missionID );

public:
	MissionsProgress();
	MissionsProgress( const MissionsProgress& data );
	~MissionsProgress();

	MissionsProgress& operator=( const MissionsProgress& data );

	void	ParseMissionProgress( pugi::xml_node& xmlItem );
	void	SaveMissionProgress( pugi::xml_node& xmlItem ) const;

	// The player is not available when the MissionProgress is loaded,
	// so we must sync remote progress once the player is created.
	void	SyncRemoteMissionsProgress();

	//------------------------------
	// Applies to tracking data
	//------------------------------
	static uint32_t	GetMinMissionID();
	static uint32_t	GetMaxMissionID();
	void	GetMissionMask( uint32_t missionID, uint32_t& offset, uint32_t& mask );
	bool	IsMissionAccepted( uint32_t missionID );
	bool	IsMissionCompleted( uint32_t missionID );
	void	SetMissionAccepted( uint32_t missionID, bool accepted );
	void	SetMissionCompleted( uint32_t missionID, bool completed );
	void	ResetMissionData();

	//------------------------------
	// Applies to All Missions
	//------------------------------
	void	RemoveAllMissions();

	// GotoActionData Functions
	void	CheckPosition();

	// ItemActionData Functions
	void	DeliverItemAction();
	void	PerformItemAction( EItemActionType itemAction, uint32_t itemID, uint32_t hashID, EItemUseOnTarget target = ITEMUSEON_NotSpecified );

	// KillActionData Functions
	void	AddKill( GameObject* victim, uint32_t weaponID );

	// StateActionData Functions
	void	DecState( GameObject* item );
	void	IncState( GameObject* item );

	// UseActionData Functions
	void	UseItem( uint32_t itemID );

	//------------------------------
	// Applies to Specific Mission
	//------------------------------
	bool	Update( uint32_t missionID, uint32_t& completedStage, uint32_t& completedObjective );
	void	UpdateRemoteMissionAction( uint32_t missionID, const ActionData& action, bool remove );
	void	SetRemoteMapIcons( MissionStateData& stateData, bool active );

	bool	AddMission( uint32_t missionID, bool isFromLoad = false );
	bool	RemoveMission( uint32_t missionID, ERemoveReason reason );

	bool	IsProgressCompleted( uint32_t missionID );
	bool	TriggerStage( uint32_t missionID, GameObject* item );

	// GotoActionData Functions
	bool	CheckPosition( uint32_t missionID );

	// ItemActionData Functions
	bool	DeliverItemAction( uint32_t missionID );
	bool	PerformItemAction( uint32_t missionID, EItemActionType itemAction, uint32_t itemID, uint32_t hashID, EItemUseOnTarget target = ITEMUSEON_NotSpecified );

	// KillActionData Functions
	bool	AddKill( uint32_t missionID, GameObject* victim, uint32_t weaponID );

	// StateActionData Functions
	bool	DecState( uint32_t missionID, GameObject* item );
	bool	IncState( uint32_t missionID, GameObject* item );
	EActionState GetState( uint32_t missionID, GameObject* item );

protected:
	void	LoadProgress( const pugi::xml_node& xmlCharData );
	void	SaveProgress( pugi::xml_node& xmlCharData ) const;

public:
	static const uint32_t	MaxAllowedMissions	= 3;
	static const uint32_t	MaxArrSize			= 32;	// Can track 1024 missions with 32 uint32_t's.
	static const uint32_t	NumBits				= 32;	// Number of bits per array's storage type (i.e. uint32_t)
	static const uint32_t	BaseID				= 10000;
	static const uint32_t	Version				= 1;

	obj_ServerPlayer* m_player;

	// Tracking Data
	uint32_t	Accepted[ MaxArrSize ];
	uint32_t	Completed[ MaxArrSize ];

	// Progress
	std::map< uint32_t, MissionStateData* >	m_data;
};

//=========================================================================

} // namespace Mission
#endif