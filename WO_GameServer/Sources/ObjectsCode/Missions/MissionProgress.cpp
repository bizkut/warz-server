//=========================================================================
//	Module: MissionProgress.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "MissionProgress.h"
#include "MissionManager.h"
#include "MissionCommands.h"
#include "MissionActions.h"
#include "MissionActionsData.h"
#include "GameCommon.h"
#include "..\obj_ServerPlayer.h"
#include "..\obj_ServerMissionArea.h"
#include "..\..\AsyncFuncs.h"

//-------------------------------------------------------------------------

#ifdef MISSIONS
namespace Mission
{

MissionStateData* MissionsProgress::FindMission( uint32_t missionID )
{
	// Find the Mission.
	std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.find( missionID );
	if( missionIter == m_data.end() )
		return NULL; 

	return missionIter->second;
}

MissionsProgress::MissionsProgress()
	: m_player( NULL )
{
	memset( Accepted, 0, sizeof( uint32_t ) * MaxArrSize );
	memset( Completed, 0, sizeof( uint32_t ) * MaxArrSize );
}

MissionsProgress::MissionsProgress( const MissionsProgress& data )
	: m_player( data.m_player )
{
	*this = data;
}

MissionsProgress::~MissionsProgress()
{
	RemoveAllMissions();
	m_player = NULL;
}

MissionsProgress& MissionsProgress::operator =( const MissionsProgress& data )
{
	//// Clear out any previous data.
	//RemoveAllMissions();

	memcpy( Accepted, data.Accepted, sizeof( uint32_t ) * MaxArrSize );
	memcpy( Completed, data.Completed, sizeof( uint32_t ) * MaxArrSize );

	m_player = data.m_player;
	for( std::map< uint32_t, MissionStateData* >::const_iterator prevMissionIter = data.m_data.begin();
		 prevMissionIter != data.m_data.end(); ++prevMissionIter )
	{
		MissionStateData* prevStateData = prevMissionIter->second;

		// Get the mission for displaying the mission name, and to verify it's a valid mission.
		std::map<uint32_t, Mission*>::const_iterator missionIter = g_pMissionMgr->m_mapMissions.find( prevStateData->m_missionID );
		if( missionIter == g_pMissionMgr->m_mapMissions.end() )
			continue;
		Mission* mission = missionIter->second;

		MissionStateData* stateData = FindMission( prevStateData->m_missionID );
		if( stateData )
		{
			// Overwrite the mission state data.
			stateData->m_missionID		= prevStateData->m_missionID;
			stateData->m_stageIndex		= prevStateData->m_stageIndex;
			stateData->m_objectiveIndex	= prevStateData->m_objectiveIndex;
			stateData->m_stageTriggered	= prevStateData->m_stageTriggered;

			r3d_assert( prevStateData->m_stageDataVec.size() == stateData->m_stageDataVec.size() );

			// Copy the Stages
			StageDataVec::iterator stageIter = stateData->m_stageDataVec.begin();
			for( StageDataVec::const_iterator prevStageIter = prevStateData->m_stageDataVec.begin();
				 prevStageIter != prevStateData->m_stageDataVec.end(); ++prevStageIter, ++stageIter )
			{
				r3d_assert( stageIter != stateData->m_stageDataVec.end() );
				r3d_assert( (*prevStageIter)->size() == (*stageIter)->size() );

				// Copy the Objectives
				ObjectiveDataVec::iterator objectiveIter = (*stageIter)->begin();
				for( ObjectiveDataVec::const_iterator prevObjectiveIter = (*prevStageIter)->begin();
					 prevObjectiveIter != (*prevStageIter)->end(); ++prevObjectiveIter, ++objectiveIter )
				{
					r3d_assert( objectiveIter != (*stageIter)->end() );
					r3d_assert( (*prevObjectiveIter)->size() == (*objectiveIter)->size() );

					// Copy the Actions
					ActionDataVec::iterator actionIter = (*objectiveIter)->begin();
					for( ActionDataVec::const_iterator prevActionIter = (*prevObjectiveIter)->begin();
						 prevActionIter != (*prevObjectiveIter)->end(); ++prevActionIter, ++actionIter )
					{
						r3d_assert( actionIter != (*objectiveIter)->end() );
						r3d_assert( (*prevActionIter)->m_actionType == (*actionIter)->m_actionType );

						switch( (*prevActionIter)->m_actionType )
						{
						case ACT_Goto:		*(GotoActionData*)(*actionIter) = *(GotoActionData*)(*prevActionIter); break;
						case ACT_Item:		*(ItemActionData*)(*actionIter) = *(ItemActionData*)(*prevActionIter); break;
						case ACT_Kill:		*(KillActionData*)(*actionIter) = *(KillActionData*)(*prevActionIter); break;
						case ACT_State:		*(StateActionData*)(*actionIter) = *(StateActionData*)(*prevActionIter); break;
						default:
							r3d_assert(false && "Unsupported Action Data Type" );
							break;
						}
					}
				}
			}

		}
		else
		{
			// Create and copy the mission state data.
			stateData					= game_new MissionStateData;
			stateData->m_missionID		= prevStateData->m_missionID;
			stateData->m_stageIndex		= prevStateData->m_stageIndex;
			stateData->m_objectiveIndex	= prevStateData->m_objectiveIndex;
			stateData->m_stageTriggered	= prevStateData->m_stageTriggered;

			// Copy the Stages
			for( StageDataVec::const_iterator prevStageIter = prevStateData->m_stageDataVec.begin();
				 prevStageIter != prevStateData->m_stageDataVec.end(); ++prevStageIter )
			{
				// Copy the Objectives
				ObjectiveDataVec* objectiveDataVec = game_new ObjectiveDataVec;
				for( ObjectiveDataVec::const_iterator prevObjectiveIter = (*prevStageIter)->begin();
					 prevObjectiveIter != (*prevStageIter)->end(); ++prevObjectiveIter )
				{
					// Copy the Actions
					ActionDataVec* actionDataVec = game_new ActionDataVec;
					for( ActionDataVec::const_iterator prevActionIter = (*prevObjectiveIter)->begin();
						 prevActionIter != (*prevObjectiveIter)->end(); ++prevActionIter )
					{
						ActionData* actionData = NULL;
						switch( (*prevActionIter)->m_actionType )
						{
						case ACT_Goto:		actionData = (ActionData*)game_new GotoActionData( *(GotoActionData*)(*prevActionIter) ); break;
						case ACT_Item:		actionData = (ActionData*)game_new ItemActionData( *(ItemActionData*)(*prevActionIter) ); break;
						case ACT_Kill:		actionData = (ActionData*)game_new KillActionData( *(KillActionData*)(*prevActionIter) ); break;
						case ACT_State:		actionData = (ActionData*)game_new StateActionData( *(StateActionData*)(*prevActionIter) ); break;
						default:
							r3d_assert(false && "Unsupported Action Data Type" );
							break;
						}
						if( actionData )
						{
							actionDataVec->push_back( actionData );
						}
					}
					objectiveDataVec->push_back( actionDataVec );
				}
				stateData->m_stageDataVec.push_back( objectiveDataVec );
			}
		}
		m_data[ stateData->m_missionID ] = stateData;
		//r3dOutToLog( "Mission(%d): Copied Mission '%s' to Player '%s'\n", mission->m_ID, mission->m_name, (m_player) ? m_player->Name : "Unknown" );
	}

	return *this;
}

void MissionsProgress::ParseMissionProgress( pugi::xml_node& xmlItem )
{
	// Mission accepted and completed data is quantized into the bits of its arrays.
	pugi::xml_node& xmlMissionsContainer = xmlItem.child("MissionsData");
	if( !xmlMissionsContainer.empty() )
	{
//#ifdef _DEBUG
//		xml_string_writer xmlBuf;
//		xmlItem.print(xmlBuf);
//		r3dOutToLog( "Loading Missions Progress:\n%s\n", xmlBuf.out_.c_str() );
//#endif
		pugi::xml_node& xmlMissionsHeader = xmlMissionsContainer.child("MissionsHeader");
		if( !xmlMissionsHeader.empty() )
		{
			uint32_t ver = xmlMissionsHeader.attribute("version").as_uint();
			if( ver != Version )
			{
				r3dOutToLog("!!! Mission: Encountered version (%d : %d) mismatch while trying to load MissionsProgress. Data will not be loaded.\n", ver, Version);
				return;
			}

			pugi::xml_node& xmlMissionData = xmlMissionsContainer.child("MissionsArrays");
			if( !xmlMissionData.empty() )
			{
				for(int i = 0; i < MaxArrSize; ++i)
				{
					char val[ 4 ] = { 0, 0, 0, 0 };
					sprintf( val, "a%d", i );
					Accepted[ i ]	= xmlMissionData.attribute(val).as_uint();
					sprintf( val, "c%d", i );
					Completed[ i ]	= xmlMissionData.attribute(val).as_uint();
				}
			}
			// Mission Progress is stored per mission.
			LoadProgress( xmlMissionsContainer );
		}
	}
}
void MissionsProgress::SaveMissionProgress( pugi::xml_node& xmlItem ) const
{
	pugi::xml_node xmlMissionHeader = xmlItem.append_child();
	xmlMissionHeader.set_name("MissionsHeader");
	xmlMissionHeader.append_attribute("version") = Version;

	// Mission accepted and completed data is quantized into the bits of its arrays.
	pugi::xml_node xmlMissionData = xmlItem.append_child();
	xmlMissionData.set_name("MissionsArrays");

	for(int i = 0; i < MaxArrSize; ++i)
	{
		char val[ 4 ] = { 0, 0, 0, 0 };
		sprintf( val, "a%d", i );
		if( Accepted[ i ] > 0 )
			xmlMissionData.append_attribute(val)	= Accepted[ i ];
		sprintf( val, "c%d", i );
		if( Completed[ i ] > 0 )
			xmlMissionData.append_attribute(val)	= Completed[ i ];
	}
	SaveProgress( xmlItem );
//#ifdef _DEBUG
//	xml_string_writer xmlBuf;
//	xmlMissions.print(xmlBuf);
//	r3dOutToLog( "Saving Missions Progress:\n%s\n", xmlBuf.out_.c_str() );
//#endif
}

void MissionsProgress::SyncRemoteMissionsProgress()
{
	if( !m_player )
	{
		r3dOutToLog("!!! Tried to Sync Remote Player Mission Progress when the player was not set!\n");
		return;
	}

	// Iterate through the missions
	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
	 missionIter != m_data.end(); ++missionIter )
	{
		MissionStateData* stateData = missionIter->second;

		// Turn on Mission Map Icons for the active stage/objective
		SetRemoteMapIcons(*stateData, true );

		// Iterate through the Stages
		for( StageDataVec::iterator stageIter = stateData->m_stageDataVec.begin();
			 stageIter != stateData->m_stageDataVec.end(); ++stageIter )
		{
			// Iterate through the Objectives
			for( ObjectiveDataVec::iterator objectiveIter = (*stageIter)->begin();
				 objectiveIter != (*stageIter)->end(); ++objectiveIter )
			{
				// Iterate through the Actions
				for( ActionDataVec::iterator actionIter = (*objectiveIter)->begin();
					 actionIter != (*objectiveIter)->end(); ++actionIter )
				{
					// Update the remote mission action data.
					//r3dOutToLog("Syncing Mission(%d), Action(%d)\n", missionIter->first, (*actionIter)->m_action->m_ID);
					UpdateRemoteMissionAction( missionIter->first, *(*actionIter), false );
				}
			}
		}
	}

}


// Applies to tracking data
uint32_t MissionsProgress::GetMinMissionID()
{
	return BaseID + 1;
}

uint32_t MissionsProgress::GetMaxMissionID()
{
	return BaseID + NumBits * MaxArrSize;
}

void MissionsProgress::GetMissionMask( uint32_t missionID, uint32_t& offset, uint32_t& mask )
{
	// MissionIDs start at (BaseID + 1), and are less than or equal to (BaseID + NumBits * MaxArrSize)
	r3d_assert( missionID >= GetMinMissionID() && "MissionIDs start at 10001" ); 
	r3d_assert( missionID <= GetMaxMissionID() && "MissionIDs can't go above 11024" );

	uint32_t index	= missionID - GetMinMissionID();
	offset			= index / NumBits;
	mask			= (1 << (index % NumBits));
}

bool MissionsProgress::IsMissionAccepted( uint32_t missionID )
{
	uint32_t offset;
	uint32_t mask;
	GetMissionMask( missionID, offset, mask );
	return ( ( Accepted[ offset ] & mask ) > 0 );
}
bool MissionsProgress::IsMissionCompleted( uint32_t missionID )
{
	uint32_t offset;
	uint32_t mask;
	GetMissionMask( missionID, offset, mask );
	return ( ( Completed[ offset ] & mask ) > 0 );
}

void MissionsProgress::SetMissionAccepted( uint32_t missionID, bool accepted )
{
	uint32_t offset;
	uint32_t mask;
	GetMissionMask( missionID, offset, mask );
	if( accepted )
		Accepted[ offset ] |= mask;
	else
		Accepted[ offset ] &= ~mask;
}
void MissionsProgress::SetMissionCompleted( uint32_t missionID, bool completed )
{
	if( completed && g_pMissionMgr->IsMissionSaveDisallowed( missionID ) )
		return;

	uint32_t offset;
	uint32_t mask;
	GetMissionMask( missionID, offset, mask );
	if( completed )
		Completed[ offset ] |= mask;
	else
		Completed[ offset ] &= ~mask;
}

void MissionsProgress::ResetMissionData()
{
	// Remove the missions locally and at the client.
	// NOTE: Two-step process because we can't remove them
	//		 from the same map we iterate over the first time.
	std::vector<uint32_t> missionIDs;
	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		MissionStateData* stateData = missionIter->second;
		missionIDs.push_back( stateData->m_missionID );
	}
	for( std::vector< uint32_t >::iterator iter = missionIDs.begin();
		 iter != missionIDs.end(); ++iter )
	{
		RemoveMission( *iter, RMV_Administration );
	}

	// Make sure all Completed missions are reset, and any
	// erroneous Accepted missions are cleared out.
	memset( &Accepted, 0, sizeof( uint32_t ) * MaxArrSize );
	memset( &Completed, 0, sizeof( uint32_t ) * MaxArrSize );

	// Save the data.
	g_AsyncApiMgr->AddJob(new CJobUpdateMissionsData( m_player ));
}

// Applies to All Missions
void MissionsProgress::RemoveAllMissions()
{
	// Destroy the Missions
	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		MissionStateData* stateData = missionIter->second;

		stateData->m_missionID		= 0;
		stateData->m_stageIndex		= 0;
		stateData->m_objectiveIndex	= 0;

		// Destroy the Stages
		for( StageDataVec::iterator stageIter = stateData->m_stageDataVec.begin();
			 stageIter != stateData->m_stageDataVec.end(); ++stageIter )
		{
			// Destroy the Objectives
			for( ObjectiveDataVec::iterator objectiveIter = (*stageIter)->begin();
				 objectiveIter != (*stageIter)->end(); ++objectiveIter )
			{
				// Destroy the Actions
				for( ActionDataVec::iterator actionIter = (*objectiveIter)->begin();
					 actionIter != (*objectiveIter)->end(); ++actionIter )
				{
					delete (*actionIter);
					*actionIter = NULL;
				}
				(*objectiveIter)->clear();
				delete (*objectiveIter);
				*objectiveIter = NULL;
			}
			(*stageIter)->clear();
			delete (*stageIter);
			*stageIter = NULL;
		}
		stateData->m_stageDataVec.clear();
		delete stateData;
		stateData = NULL;
	}
	m_data.clear();
}

void MissionsProgress::CheckPosition()
{
	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		CheckPosition( missionIter->first );
	}
}

void MissionsProgress::DeliverItemAction()
{
	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		DeliverItemAction( missionIter->first );
	}
}

void MissionsProgress::PerformItemAction( EItemActionType itemAction, uint32_t itemID, uint32_t hashID, EItemUseOnTarget target /*= ITEMUSEON_NotSpecified*/ )
{
	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		PerformItemAction( missionIter->first, itemAction, itemID, hashID, target );
	}
}

void MissionsProgress::AddKill( GameObject* victim, uint32_t weaponID )
{
	r3d_assert( victim );
	if( !victim )
		return;

	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		AddKill( missionIter->first, victim, weaponID );
	}
}

void MissionsProgress::DecState( GameObject* item )
{
	r3d_assert( item );
	if( !item )
		return;

	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		DecState( missionIter->first, item );
	}
}

void MissionsProgress::IncState( GameObject* item )
{
	r3d_assert( item );
	if( !item )
		return;

	for( std::map< uint32_t, MissionStateData* >::iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		IncState( missionIter->first, item );
	}
}

// Applies to Specific Mission
bool MissionsProgress::Update( uint32_t missionID, uint32_t& completedStage, uint32_t& completedObjective )
{
	completedStage = UINT_MAX;
	completedObjective = UINT_MAX;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false;

	// Passive Mission Checks
	CheckPosition( missionID );
	DeliverItemAction( missionID );

	// Check the progress of all the actions in each Objective in
	// the current Stage to determine if the player should progress
	// to the next Objective or Stage.
	bool bComplete = true;
	uint32_t completeCount = 0;
	StageDataVec&		stageDataVec		= stateData->m_stageDataVec;
	ObjectiveDataVec&	objectivesDataVec	= *stageDataVec[ stateData->m_stageIndex ];
	ActionDataVec* actionsVec = objectivesDataVec[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		completeCount += ( (*actionIter)->m_completed ) ? 1 : 0;
		bComplete &= (*actionIter)->m_completed;
	}
	if( bComplete || completeCount >= g_pMissionMgr->GetNumRequiredActions( missionID, stateData->m_stageIndex, stateData->m_objectiveIndex ) )
	{
		// Stage is not really complete, unless all the Objectives
		// in the stage have been completed, however it is passed
		// out to allow access to the objectives information.
		completedStage		= stateData->m_stageIndex;
		completedObjective	= stateData->m_objectiveIndex;

		if( stateData->m_objectiveIndex < objectivesDataVec.size() - 1 )
		{
			SetRemoteMapIcons( *stateData, false );

			r3dOutToLog("Mission(%d): COMPLETED Objective [%d]!\n", missionID, stateData->m_objectiveIndex);
			++stateData->m_objectiveIndex;

			SetRemoteMapIcons( *stateData, true );

			g_AsyncApiMgr->AddJob( new CJobUpdateMissionsData( m_player ) );
		}
		else if( stateData->m_stageIndex < stageDataVec.size() - 1 )
		{
			SetRemoteMapIcons( *stateData, false );

			r3dOutToLog("Mission(%d): COMPLETED Stage [%d]!\n", missionID, stateData->m_stageIndex);
			++stateData->m_stageIndex;
			stateData->m_objectiveIndex = 0;
			stateData->m_stageTriggered = ( 0 == g_pMissionMgr->GetTriggerObjectHash( missionID, stateData->m_stageIndex ) );

			SetRemoteMapIcons( *stateData, true );

			g_AsyncApiMgr->AddJob( new CJobUpdateMissionsData( m_player ) );
		}
		return true;
	}
	return false;
}

void MissionsProgress::UpdateRemoteMissionAction( uint32_t missionID, const ActionData& actionData, bool remove )
{
	PKT_S2C_MissionActionUpdate_s n;
	n.missionID		= missionID;
	n.actionID		= actionData.m_action->m_ID;
	n.actionType	= (uint32_t)actionData.m_actionType;
	switch( actionData.m_actionType )
	{
	case ACT_Goto:
		n.amount	= 0;
		n.count		= 0;
		n.itemID	= 0;
		n.objType	= 0;
		break;
	case ACT_Item:
		n.actionType |= ((uint32_t)((ItemAction*)actionData.m_action)->m_itemAction) << 16;
		n.amount	= ((ItemAction*)(actionData.m_action))->m_amount;
		n.count		= ((ItemActionData&)actionData).m_count;
		n.itemID	= ((ItemAction*)actionData.m_action)->m_itemID;
		n.objType	= 0;
		break;
	case ACT_Kill:
		n.amount	= ((KillAction*)(actionData.m_action))->m_amount;
		n.count		= ((KillActionData&)actionData).m_count;
		n.objType	= ((KillAction*)(actionData.m_action))->m_objType;
		switch( ((KillAction*)actionData.m_action)->m_weaponIDRestrictions.size() )
		{
		case 0:		// No Weapon Restrictions
			n.itemID = 0;
			break;
		case 1:		// One Weapon Restriction
			n.itemID = *(((KillAction*)(actionData.m_action))->m_weaponIDRestrictions.begin());
			break;
		default:	// Multiple Weapon Restrictions
			n.itemID = UINT_MAX;
			break;
		}
		break;
	case ACT_State:
		n.actionType |= ((uint32_t)((StateAction*)actionData.m_action)->m_stateSequence.back()) << 16;
		n.amount	= 0;
		n.count		= (uint32_t)*(((StateActionData&)actionData).m_stateIter);
		n.itemID	= ((StateAction*)actionData.m_action)->m_objHash;
		n.objType	= 0;
		break;
	}
	if( actionData.m_action->m_areaRestriction )
	{
		n.position	= actionData.m_action->m_areaRestriction->GetPosition();
		n.extents	= actionData.m_action->m_areaRestriction->m_extents;
	}
	else
	{
		n.position	= r3dPoint3D( -9999, -9999, -9999 );
		n.extents	= r3dVector( 0, 0, 0 );
	}
	n.completed		= actionData.m_completed;
	r3dscpy( n.missionName, g_pMissionMgr->m_mapMissions[ missionID ]->m_name );

	gServerLogic.p2pSendRawToPeer( m_player->peerId_, &n, sizeof(n), true );
}
void MissionsProgress::SetRemoteMapIcons( MissionStateData& stateData, bool active )
{
	StageDataVec&		stageDataVec		= stateData.m_stageDataVec;
	ObjectiveDataVec&	objectivesDataVec	= *stageDataVec[ stateData.m_stageIndex ];
	ActionDataVec&		actionDataVec		= *objectivesDataVec[ stateData.m_objectiveIndex ];

	for( ActionDataVec::iterator actionIter = actionDataVec.begin();
		 actionIter != actionDataVec.end(); ++actionIter )
	{
		Action* action = (*actionIter)->m_action;
		if( action->m_mapIcon != ACTICON_NotSet )
		{
			// If the action is a StateAction, override the position with the MissionStateObject's
			// position, otherwise use the MissionAreaRestriction (if it exists, else no icon).
			StateAction* stateAct = (action->m_actionType == ACT_State) ? ((StateAction*)action) : NULL;
			GameObject* obj = (stateAct && stateAct->m_objHash > 0) ? GameWorld().GetObjectByHash( stateAct->m_objHash ) : action->m_areaRestriction;
			if( obj )
			{
				// Make sure the player is still attached to the server before sending him messages.
				const ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(m_player->peerId_);
				if(peer.status_ == ServerGameLogic::PEER_PLAYING)
				{
					PKT_S2C_MissionMapUpdate_s n;
					n.actionID		= action->m_ID;
					n.actionIcon	= (uint32_t)action->m_mapIcon;
					n.location		= obj->GetPosition();
					n.active		= active;

					//r3dOutToLog("!!!SetRemoteMapIcons: %s\n", (active) ? "active" : "inactive");
					gServerLogic.p2pSendRawToPeer( m_player->peerId_, &n, sizeof(n), true );
				}
			}
		}
	}
}

bool MissionsProgress::AddMission( uint32_t missionID, bool isFromLoad /* = false */ )
{
	// Make sure the player can have another mission.
	if( MaxAllowedMissions <= m_data.size() )
		return false;

	// Make sure the player doesn't already have the mission.
	MissionStateData* stateData = FindMission( missionID );
	if( stateData )
		return false;

	// Make sure the player hasn't already completed the mission.
	if( IsMissionCompleted( missionID ) && !d_enable_mission_repeat->GetBool() )
		return false;

	std::map<uint32_t, Mission*>::iterator missionIter = g_pMissionMgr->m_mapMissions.find( missionID );
	if( missionIter == g_pMissionMgr->m_mapMissions.end() )
		return false;

	// Prepare the Mission Action Data
	Mission* mission = missionIter->second;

	stateData					= game_new MissionStateData;
	stateData->m_missionID		= missionID;
	stateData->m_stageIndex		= 0;
	stateData->m_objectiveIndex	= 0;
	stateData->m_stageTriggered	= ( 0 == mission->m_stages[ 0 ]->m_triggerObjHash );

	// Don't want to allow duping based on accept mission, saving, exiting and re-loading
	if( !isFromLoad )
	{
		// Execute any Mission::Command's with the order type of ORDER_Start.
		std::map< EOrderType, std::vector< Command* >* >::iterator mapCmdIter = mission->m_commands.find( ORDER_Start );
		if( mapCmdIter != mission->m_commands.end() )
		{
			if( mapCmdIter->second )
			{
				std::vector< Command* >* pCommandVec = mapCmdIter->second;
				for( std::vector< Command* >::iterator cmdIter = pCommandVec->begin(); cmdIter != pCommandVec->end(); ++cmdIter )
				{
					if( *cmdIter )
						(*cmdIter)->PerformCommand( mapCmdIter->first, m_player );
				}
			}
		}
	}

	// Prepare the Stages
	for( std::vector< Stage* >::iterator stageIter = mission->m_stages.begin();
		 stageIter != mission->m_stages.end(); ++stageIter )
	{
		// Prepare the Objectives
		ObjectiveDataVec* objectiveDataVec = game_new ObjectiveDataVec;
		for( std::vector< Objective* >::iterator objectiveIter = (*stageIter)->m_objectives.begin();
			 objectiveIter != (*stageIter)->m_objectives.end(); ++objectiveIter )
		{
			// Prepare the Actions
			ActionDataVec* actionDataVec = game_new ActionDataVec;
			for( std::vector< Action* >::iterator actionIter = (*objectiveIter)->m_actions.begin();
				 actionIter != (*objectiveIter)->m_actions.end(); ++actionIter )
			{
				ActionData* actionData = NULL;
				switch( (*actionIter)->m_actionType )
				{
				case ACT_Goto:		actionData = (ActionData*)game_new GotoActionData; break;
				case ACT_Item:		actionData = (ActionData*)game_new ItemActionData; break;
				case ACT_Kill:		actionData = (ActionData*)game_new KillActionData; break;
				case ACT_State:		actionData = (ActionData*)game_new StateActionData; break;
				default:
					r3d_assert(false && "Unsupported Action Data Type" );
					break;
				}
				if( actionData )
				{
					actionData->Init( (*actionIter) );
					actionDataVec->push_back( actionData );

					// Nobody to update during loading
					if( !isFromLoad )
						UpdateRemoteMissionAction( missionID, *actionData, false );
				}
			}
			objectiveDataVec->push_back( actionDataVec );
		}
		stateData->m_stageDataVec.push_back( objectiveDataVec );
	}
	m_data[ missionID ] = stateData;
	SetMissionAccepted( missionID, true );
	// Nobody to update during loading
	if( !isFromLoad )
		SetRemoteMapIcons( *stateData, true );
	r3dOutToLog( "Mission(%d): Added Mission '%s' to Player '%s'\n", missionID, mission->m_name, (m_player) ? m_player->Name : "Unknown" );
	return true;
}

bool MissionsProgress::RemoveMission( uint32_t missionID, ERemoveReason reason )
{
	MissionStateData* stateData = FindMission( missionID );
	if( !stateData )
		return false;

	// Tell the client to remove the mission
	PKT_S2C_MissionRemove_s n;
	n.missionID = missionID;
	n.reason	= (int)reason;
	gServerLogic.p2pSendToPeer(m_player->peerId_, m_player, &n, sizeof(n));

	// Turn off any Mission Map Icons
	SetRemoteMapIcons( *stateData, false );

	// Execute any Destroy or Remove Mission::Command's with the order type of ORDER_End.
	std::map<uint32_t, Mission*>::const_iterator missionIter = g_pMissionMgr->m_mapMissions.find( missionID );
	if( missionIter != g_pMissionMgr->m_mapMissions.end() )
	{
		Mission* mission = missionIter->second;
		std::map< EOrderType, std::vector< Command* >* >::iterator mapCmdIter = mission->m_commands.find( ORDER_End );
		if( mapCmdIter != mission->m_commands.end() )
		{
			if( mapCmdIter->second )
			{
				std::vector< Command* >* pCommandVec = mapCmdIter->second;
				for( std::vector< Command* >::iterator cmdIter = pCommandVec->begin(); cmdIter != pCommandVec->end(); ++cmdIter )
				{
					if( *cmdIter && ((*cmdIter)->m_commandType == CMD_Destroy || (*cmdIter)->m_commandType == CMD_Remove) )
						(*cmdIter)->PerformCommand( mapCmdIter->first, m_player );
				}
			}
		}
	}

	stateData->m_missionID		= 0;
	stateData->m_stageIndex		= 0;
	stateData->m_objectiveIndex	= 0;

	// Destroy the Stages
	for( StageDataVec::iterator stageIter = stateData->m_stageDataVec.begin();
		 stageIter != stateData->m_stageDataVec.end(); ++stageIter )
	{
		// Destroy the Objectives
		for( ObjectiveDataVec::iterator objectiveIter = (*stageIter)->begin();
			 objectiveIter != (*stageIter)->end(); ++objectiveIter )
		{
			// Destroy the Actions
			for( ActionDataVec::iterator actionIter = (*objectiveIter)->begin();
				 actionIter != (*objectiveIter)->end(); ++actionIter )
			{
				delete (*actionIter);
				*actionIter = NULL;
			}
			(*objectiveIter)->clear();
			delete (*objectiveIter);
			*objectiveIter = NULL;
		}
		(*stageIter)->clear();
		delete (*stageIter);
		*stageIter = NULL;
	}
	stateData->m_stageDataVec.clear();
	delete stateData;
	stateData = NULL;

	m_data.erase( missionID );
	SetMissionAccepted( missionID, false );
	r3dOutToLog("Mission(%d): Removed Mission from Player %s\n", missionID, (m_player) ? m_player->Name : "Unknown");

	return true;
}

bool MissionsProgress::IsProgressCompleted( uint32_t missionID )
{
	if( IsMissionCompleted( missionID ) )
		return true;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false; 

	// If we haven't even reached the last stage or the last objective in a stage
	// then the mission can't have been completed.
	StageDataVec&		stageDataVec		= stateData->m_stageDataVec;
	ObjectiveDataVec&	objectivesDataVec	= *stageDataVec[ stateData->m_stageIndex ];
	if( stateData->m_stageIndex < stageDataVec.size() - 1 ||
		stateData->m_objectiveIndex < objectivesDataVec.size() - 1 )
		return false;

	// Check all the Actions Data in the last Objective to see if they are completed.
	bool bReturn = true;
	ActionDataVec* actionsVec = objectivesDataVec[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		bReturn &= (*actionIter)->m_completed;
	}

	return bReturn;
}

bool MissionsProgress::TriggerStage( uint32_t missionID, GameObject* item )
{
	r3d_assert( item );
	if( !item )
		return false;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData )
		return false; 

	// If the stage has already been triggered, then it can't be triggered again.
	if( stateData->m_stageTriggered )
		return false;

	// If the item's hash matches, then the stage is triggered, otherwise no.
	stateData->m_stageTriggered = ( item->GetHashID() == g_pMissionMgr->GetTriggerObjectHash( missionID, stateData->m_stageIndex ) );
	if( stateData->m_stageTriggered )
		r3dOutToLog("Mission(%d): Stage Triggered [%d] by Player %s\n", missionID, stateData->m_stageIndex, (m_player) ? m_player->Name : "Unknown");
	return stateData->m_stageTriggered;
}

bool MissionsProgress::CheckPosition( uint32_t missionID )
{
	r3d_assert( m_player );
	if( !m_player )
		return false;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false; 

	bool bReturn = false;
	ActionDataVec* actionsVec = (*stateData->m_stageDataVec[ stateData->m_stageIndex ])[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		if( ACT_Goto == (*actionIter)->m_actionType )
		{
			GotoActionData* actionData = (GotoActionData*)*actionIter;
			bool bPriorCompletion = actionData->m_completed;
			if( actionData->CheckPosition( m_player ) )
			{
				bReturn = true;
				if( !bPriorCompletion )
					UpdateRemoteMissionAction( missionID, *actionData, false );
			}

			// The player may be inside two different areas that overlap, so
			// early termination is not possible here.
		}
	}

	return bReturn;
}

bool MissionsProgress::DeliverItemAction( uint32_t missionID )
{
	r3d_assert( m_player );
	if( !m_player )
		return false;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false; 

	bool bReturn = false;
	ActionDataVec* actionsVec = (*stateData->m_stageDataVec[ stateData->m_stageIndex ])[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		if( ACT_Item == (*actionIter)->m_actionType )
		{
			ItemActionData* actionData = (ItemActionData*)*actionIter;

			if( ITEM_Deliver ==((ItemAction*)actionData->m_action)->m_itemAction &&
				actionData->DeliverItemAction( m_player ) )
			{
				// Arguments can be made for or against the delivery of an item
				// satisfying more than one Collect Action.  For the moment it
				// is allowed, so early termination is not used.
				bReturn = true;
				UpdateRemoteMissionAction( missionID, *actionData, false );
			}
		}
	}

	return bReturn;
}

bool MissionsProgress::PerformItemAction( uint32_t missionID, EItemActionType itemAction, uint32_t itemID, uint32_t hashID, EItemUseOnTarget target /*= ITEMUSEON_NotSpecified*/ )
{
	r3d_assert( m_player );
	if( !m_player )
		return false;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false; 

	bool bReturn = false;
	ActionDataVec* actionsVec = (*stateData->m_stageDataVec[ stateData->m_stageIndex ])[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		if( ACT_Item == (*actionIter)->m_actionType )
		{
			ItemActionData* actionData = (ItemActionData*)*actionIter;

			if( actionData->RequiresSpecificItem() )
			{
				// There should only ever be the ability to collect or use a specific
				// item once during an objective.  So we can do an early termination
				// here and return to the caller.
				bReturn = actionData->PerformItemAction( m_player, itemAction, itemID, hashID, target );
				if( bReturn )
				{
					UpdateRemoteMissionAction( missionID, *actionData, false );
					return bReturn;
				}
			}
			else
			{
				// Arguments can be made for or against the collection or use of
				// an item satisfying more than one Collect Action.  For the moment
				// it is allowed, so early termination is not used.
				if( actionData->PerformItemAction( m_player, itemAction, itemID, hashID, target ) )
				{
					bReturn = true;
					UpdateRemoteMissionAction( missionID, *actionData, false );
				}
			}
		}
	}

	return bReturn;
}

bool MissionsProgress::AddKill( uint32_t missionID, GameObject* victim, uint32_t weaponID )
{
	r3d_assert( m_player );
	r3d_assert( victim );
	if( !m_player || !victim )
		return false;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false; 

	bool bReturn = false;
	ActionDataVec* actionsVec = (*stateData->m_stageDataVec[ stateData->m_stageIndex ])[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		if( ACT_Kill == (*actionIter)->m_actionType )
		{
			KillActionData* actionData = (KillActionData*)*actionIter;
			if( actionData->AddKill( m_player, victim, weaponID ) )
			{
				bReturn = true;
				UpdateRemoteMissionAction( missionID, *actionData, false );
			}

			// Kills may satisfy more than one Kill Action, so early termination
			// is not possible here.
		}
	}

	return bReturn;
}

bool MissionsProgress::DecState( uint32_t missionID, GameObject* item )
{
	r3d_assert( m_player );
	r3d_assert( item );
	if( !m_player || !item )
		return false;

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false; 

	bool bReturn = false;
	ActionDataVec* actionsVec = (*stateData->m_stageDataVec[ stateData->m_stageIndex ])[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		if( ACT_State == (*actionIter)->m_actionType )
		{
			StateActionData* actionData = (StateActionData*)*actionIter;
			bReturn = actionData->DecState( m_player, item );

			// State Actions on the same item have to be exclusive within an objective,
			// because they can't both be satisfied at the same time if they differ. If
			// the two states are the same on the same item, then there's no point in
			// keeping two copies of the same state.  So we can do an early termination
			// here, and return to the caller.
			if( bReturn )
			{
				UpdateRemoteMissionAction( missionID, *actionData, false );
				return bReturn;
			}
		}
	}

	return bReturn;
}

bool MissionsProgress::IncState( uint32_t missionID, GameObject* item )
{
	r3d_assert( m_player );
	r3d_assert( item );
	if( !m_player || !item )
		return false;

	TriggerStage( missionID, item );

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData || !stateData->m_stageTriggered )
		return false; 

	bool bReturn = false;
	ActionDataVec* actionsVec = (*stateData->m_stageDataVec[ stateData->m_stageIndex ])[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		if( ACT_State == (*actionIter)->m_actionType )
		{
			StateActionData* actionData = (StateActionData*)*actionIter;
			bReturn = actionData->IncState( m_player, item );

			// State Actions on the same item have to be exclusive within an objective,
			// because they can't both be satisfied at the same time if they differ. If
			// the two states are the same on the same item, then there's no point in
			// keeping two copies of the same state.  So we can do an early termination
			// here, and return to the caller.
			if( bReturn )
			{
				UpdateRemoteMissionAction( missionID, *actionData, false );
				return bReturn;
			}
		}
	}

	return bReturn;
}

EActionState MissionsProgress::GetState( uint32_t missionID, GameObject* item )
{
	r3d_assert( item );

	MissionStateData* stateData = FindMission( missionID );
	if( !stateData )
		return STATE_Unknown; 

	EActionState eReturn = STATE_Unknown;
	ActionDataVec* actionsVec = (*stateData->m_stageDataVec[ stateData->m_stageIndex ])[ stateData->m_objectiveIndex ];
	for( ActionDataVec::iterator actionIter = actionsVec->begin();
		 actionIter != actionsVec->end(); ++actionIter )
	{
		if( ACT_State == (*actionIter)->m_actionType )
		{
			StateActionData* actionData = (StateActionData*)*actionIter;
			eReturn = actionData->GetState( item );
			if( STATE_Unknown != eReturn )
			{
				UpdateRemoteMissionAction( missionID, *actionData, false );
				return eReturn;
			}
		}
	}

	return eReturn;
}

void MissionsProgress::LoadProgress( const pugi::xml_node& xmlCharData )
{
	pugi::xml_node& xmlStateData = xmlCharData.child("MissionProgress");
	while( !xmlStateData.empty() )
	{
		uint32_t missionID = xmlStateData.attribute("missionID").as_uint();
		// Check if the mission is still active, or has expired.
		if( !g_pMissionMgr->IsMissionActive( missionID ) )
		{
			r3dOutToLog("Mission ID(%d) is no longer available, removing it from player '%s' during loading of mission progress!\n", missionID, (m_player) ? m_player->Name : "Unknown" );
			xmlStateData = xmlCharData.next_sibling("MissionProgress");
			continue;
		}

		// Create the MissionActionData and connect the MissionActions.
		if( !AddMission( missionID, true ) )
		{
			r3dOutToLog("Player '%s' failed to add saved mission ID(%d) during loading of mission progress!\n", (m_player) ? m_player->Name : "Unknown", missionID );
			xmlStateData = xmlCharData.next_sibling("MissionProgress");
			continue;
		}
		// Update the MissionActionData states.
		MissionStateData* stateData = FindMission( missionID );
		r3d_assert( stateData );
		if( stateData )
		{
			// Check the mission version.
			uint32_t vsos = xmlStateData.attribute("vsos").as_uint();
			uint32_t version = vsos >> 24;
			if( version != g_pMissionMgr->m_mapMissions[ missionID ]->m_version )
			{
				// If they do not match, then the player will have to start over,
				// otherwise we risk mismatching data.  (The mission has already
				// been found and added to their list of active missions.)
				r3dOutToLog("!!! Mission(%d) version (%d : %d) mis-match found while loading progress.\n", missionID, version, g_pMissionMgr->m_mapMissions[ missionID ]->m_version );
				break;;
			}

			// Version match found, set the mission progress data.
			stateData->m_stageIndex		= (vsos >> 16) & 0x000000FF;
			stateData->m_objectiveIndex	= (vsos >> 8) & 0x000000FF;
			stateData->m_stageTriggered	= (vsos & 0x000000FF) != 0 || 
				( g_pMissionMgr->m_mapMissions[ missionID ]->m_disallowSave && 0 == g_pMissionMgr->m_mapMissions[ missionID ]->m_stages[ 0 ]->m_triggerObjHash );

			pugi::xml_node& xmlActionData = xmlStateData.child("Action");
			while( !xmlActionData.empty() )
			{
				uint32_t soa = xmlActionData.attribute("soa").as_uint();
				uint32_t stageIndex		= (soa >> 16) & 0x000000FF;
				uint32_t objectiveIndex	= (soa >> 8) & 0x000000FF;
				uint32_t actionIndex	= soa & 0x000000FF;
#if _DEBUG
				r3d_assert( stageIndex < stateData->m_stageDataVec.size() );
				r3d_assert( objectiveIndex < stateData->m_stageDataVec[ stageIndex ]->size() );
				r3d_assert( actionIndex < (*stateData->m_stageDataVec[ stageIndex ])[ objectiveIndex ]->size() );
#endif
				if( stageIndex >= stateData->m_stageDataVec.size() ||
					objectiveIndex >= stateData->m_stageDataVec[ stageIndex ]->size() ||
					actionIndex >= (*stateData->m_stageDataVec[ stageIndex ])[ objectiveIndex ]->size() )
				{
					r3dOutToLog("Player '%s' failed to load mission progress for mission ID(%d): Stage %d of %d, Objective %d of %d, Action %d of %d\n",
						(m_player) ? m_player->Name : "Unknown", missionID, stageIndex, stateData->m_stageDataVec.size(),
						objectiveIndex, stateData->m_stageDataVec[ stageIndex ]->size(),
						actionIndex, (*stateData->m_stageDataVec[ stageIndex ])[ objectiveIndex ]->size() );
					xmlActionData = xmlActionData.next_sibling("Action");
					continue;
				}

				ActionData* actionData = (*(*stateData->m_stageDataVec[ stageIndex ])[ objectiveIndex ])[ actionIndex ];
#if _DEBUG
				r3d_assert( actionData );
#endif
				if( !actionData )
				{
					r3dOutToLog("Found unexpected NULL while trying to load mission progress for Player '%s' at mission ID(%d): Stage %d of %d, Objective %d of %d, Action %d of %d\n",
						(m_player) ? m_player->Name : "Unknown", missionID, stageIndex, stateData->m_stageDataVec.size(),
						objectiveIndex, stateData->m_stageDataVec[ stageIndex ]->size(),
						actionIndex, (*stateData->m_stageDataVec[ stageIndex ])[ objectiveIndex ]->size() );
					xmlActionData = xmlActionData.next_sibling("Action");
					continue;
				}

				actionData->LoadActionData( xmlActionData );

				xmlActionData = xmlActionData.next_sibling("Action");
			}
		}

		xmlStateData = xmlStateData.next_sibling("MissionProgress");
	}
}

void MissionsProgress::SaveProgress( pugi::xml_node& xmlCharData ) const
{
	for( std::map< uint32_t, MissionStateData* >::const_iterator missionIter = m_data.begin();
		 missionIter != m_data.end(); ++missionIter )
	{
		const MissionStateData* stateData = missionIter->second;

		pugi::xml_node xmlStateData = xmlCharData.append_child();
		xmlStateData.set_name("MissionProgress");

		// Save the mission progress data.
		xmlStateData.append_attribute("missionID") = missionIter->first;

		// Have to allow saving of the Mission ID, so that missions that aren't
		// allowed to save will be initialized properly on re-load.
		if( g_pMissionMgr->IsMissionSaveDisallowed( stateData->m_missionID ) )
			continue;

		uint32_t vsos = (uint32_t)g_pMissionMgr->m_mapMissions[ missionIter->first ]->m_version << 24 |
						stateData->m_stageIndex << 16 |
						stateData->m_objectiveIndex << 8 |
						(uint32_t)stateData->m_stageTriggered;
		if( vsos > 0 )
			xmlStateData.append_attribute("vsos") = vsos;

		// Save the Stages
		uint32_t stageIndex = 0;
		for( StageDataVec::const_iterator stageIter = stateData->m_stageDataVec.begin();
			 stageIter != stateData->m_stageDataVec.end(); ++stageIter )
		{
			// Save the Objectives
			uint32_t objectiveIndex = 0;
			for( ObjectiveDataVec::const_iterator objectiveIter = (*stageIter)->begin();
				 objectiveIter != (*stageIter)->end(); ++objectiveIter )
			{
				// Don't save in-complete objectives.
				if( stateData->m_objectiveIndex <= objectiveIndex )
					break;

				// Save the Actions
				uint32_t actionIndex = 0;
				for( ActionDataVec::const_iterator actionIter = (*objectiveIter)->begin();
					 actionIter != (*objectiveIter)->end(); ++actionIter )
				{

					pugi::xml_node xmlActionData = xmlStateData.append_child();
					xmlActionData.set_name("Action");

					(*actionIter)->SaveActionData( xmlActionData );

					uint32_t soa  = stageIndex << 16 |
									objectiveIndex << 8 |
									actionIndex;
					if( soa > 0 )
						xmlActionData.append_attribute("soa") = soa;

					++actionIndex;
				}
				++objectiveIndex;
			}
			++stageIndex;
		}
	}
}

//-------------------------------------------------------------------------

}// namespace Mission
#endif