//=========================================================================
//	Module: MissionManager.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "MissionManager.h"
#include "MissionCommands.h"
#include "MissionActions.h"
#include "MissionRewards.h"
#include "MissionTimer.h"
#include "MissionProgress.h"
#include "..\obj_ServerPlayer.h"
#include "GameCode\UserRewards.h"
#include "GameLevel.h"
#include "..\..\AsyncFuncs.h"

extern ServerGameLogic gServerLogic;

//-------------------------------------------------------------------------

#ifdef MISSIONS
namespace Mission
{

Objective::Objective()
	: m_mustCompleteNumActions( 0 )
{ }

Objective::~Objective()
{
	for(std::vector< Action* >::iterator iter = m_actions.begin();
		iter != m_actions.end(); ++iter )
	{
		delete *iter;
	}
	m_actions.clear();

	for(std::vector< Reward* >::iterator iter = m_rewards.begin();
		iter != m_rewards.end(); ++iter )
	{
		delete *iter;
	}
	m_rewards.clear();
}

bool Objective::loadObjective( pugi::xml_node& xmlObjective )
{
	uint32_t actionsCount = 0;
	uint32_t rewardsCount = 0;
	pugi::xml_node xmlNode = xmlObjective.first_child();
	while( !xmlNode.empty() )
	{
		if( actionsCount > MaxObjectives )
		{
			r3dOutToLog( "!!!  Too many actions, MissionProgress Save/Load is only built to handle 255 Actions per Objective.\n");
			return false;
		}

		Action* action = NULL;
		Reward* reward = NULL;
		if( _tcsncmp( _T("GotoAction"), xmlNode.name(), 10 ) == 0 )
			action = (Action*)game_new GotoAction;
		else if( _tcsncmp( _T("ItemAction"),  xmlNode.name(), 10 ) == 0 )
			action = (Action*)game_new ItemAction;
		else if( _tcsncmp( _T("KillAction"), xmlNode.name(), 10 ) == 0 )
			action = (Action*)game_new KillAction;
		else if( _tcsncmp( _T("StateAction"), xmlNode.name(), 11 ) == 0 )
			action = (Action*)game_new StateAction;
		else if( _tcsncmp( _T("ItemReward"), xmlNode.name(), 10 ) == 0 )
			reward = (Reward*)game_new ItemReward;
		else if( _tcsncmp( _T("StatsReward"), xmlNode.name(), 8 ) == 0 )
			reward = (Reward*)game_new StatsReward;
		else
		{
			r3dOutToLog( "!!! Unsupported action type found in objectives.\n" );
			return false;
		}

		if( NULL != action )
		{
			++actionsCount;
			if( !action->loadAction( xmlNode ) )
			{
				r3dOutToLog("!!! Failed to load action %d\n", actionsCount);
				return false;
			}
			m_actions.push_back( action );
		}
		if( NULL != reward )
		{
			++rewardsCount;
			if( !reward->loadReward( xmlNode ) )
			{
				r3dOutToLog("!!! Failed to load objective reward %d\n", rewardsCount);
				return false;
			}
			m_rewards.push_back( reward );
		}

		xmlNode = xmlNode.next_sibling();
	}
	if( 0 >= m_actions.size() )
	{
		r3dOutToLog( "!!! No actions found for objective.\n" );
		return false;
	}

	m_mustCompleteNumActions = xmlObjective.attribute("mustComplete").as_uint();
	// If there wasn't a valid value for the attribute, then require all actions.
	if( m_mustCompleteNumActions == 0 )
		m_mustCompleteNumActions = m_actions.size();

	return true;
}

//-------------------------------------------------------------------------

Stage::Stage()
	: m_triggerObjHash( 0 )
{ }

Stage::~Stage()
{
	for( std::vector< Objective* >::iterator iter = m_objectives.begin();
		 iter != m_objectives.end(); ++iter )
	{
		delete *iter;
	}
	m_objectives.clear();
}

bool Stage::loadStage( pugi::xml_node& xmlStage )
{
	if( xmlStage.empty() )
	{
		r3dOutToLog( "!!! Stage not defined!\n" );
		return false;
	}

	uint32_t objectivesCount = 0;
	pugi::xml_node xmlObjective = xmlStage.child("Objective");
	while( !xmlObjective.empty() )
	{
		++objectivesCount;
		if( objectivesCount > MaxObjectives )
		{
			r3dOutToLog( "!!! Too many objectives, MissionProgress Save/Load is only built to handle 255 Objectives per Stage.\n");
			return false;
		}

		Objective* objective = game_new Objective;
		if( !objective->loadObjective( xmlObjective ) )
		{
			r3dOutToLog("!!! Failed to load objective %d\n", objectivesCount);
			return false;
		}
		m_objectives.push_back( objective );

		xmlObjective = xmlObjective.next_sibling("Objective");
	}
	if( 0 >= m_objectives.size() )
	{
		r3dOutToLog( "!!! No objectives found for stage!\n" );
		return false;
	}

	m_triggerObjHash = xmlStage.attribute("triggerItemHash").as_uint();

	return true;
}

//-------------------------------------------------------------------------

Mission::Mission() 
	: m_ID( 0 )
	, m_active( false )
	, m_disallowSave( true )
	, m_triggerObjHash( 0 )
	, m_expirationTimer( NULL )
{
	memset( m_name, '\0', StringIDLen );
	memset( m_desc, '\0', StringIDLen );
	memset( m_icon, '\0', IconPathLen );
}

Mission::~Mission()
{
	if( m_expirationTimer )
	{
		delete m_expirationTimer;
		m_expirationTimer = NULL;
	}
	for( std::map< EOrderType, std::vector< Command* >* >::iterator mapIter = m_commands.begin();
		 mapIter != m_commands.end(); ++mapIter )
	{
		std::vector< Command* >* commandVec = mapIter->second;
		if( !commandVec )
			continue;

			for( std::vector< Command* >::iterator iter = commandVec->begin();
				 iter != commandVec->end(); ++iter )
			{
				delete *iter;
				*iter = NULL;
			}
			commandVec->clear();

		delete mapIter->second;
		mapIter->second = NULL;
	}
	m_commands.clear();

	for( std::vector< Stage* >::iterator iter = m_stages.begin();
		 iter != m_stages.end(); ++iter )
	{
		delete *iter;
	}
	m_stages.clear();

	for(std::vector< Reward* >::iterator iter = m_rewards.begin();
		iter != m_rewards.end(); ++iter )
	{
		delete *iter;
	}
	m_rewards.clear();
}

bool Mission::loadMission( pugi::xml_node& xmlMission )
{
	m_ID = xmlMission.attribute("missionID").as_uint();
	m_version = xmlMission.attribute("version").as_uint();
	_tcsncpy_s(m_name, StringIDLen, xmlMission.attribute("name").value(), StringIDLen - 1);
	_tcsncpy_s(m_desc, StringIDLen, xmlMission.attribute("desc").value(), StringIDLen - 1);
	_tcsncpy_s(m_icon, IconPathLen, xmlMission.attribute("icon").value(), IconPathLen - 1);
	m_active = xmlMission.attribute("active").as_bool();
	m_disallowSave = xmlMission.attribute("disallowsave").as_bool();
	m_triggerObjHash = xmlMission.attribute("triggerItemHash").as_uint();

	// TODO: allow inactive missions to load for the Level Editor,
	//		 but not the game; commented out for now.
	// No need to load inactive missions and utilize resources for them.
	//if( !m_active )
	//	return;

	// Expiration Timer is not required, but if it is present and there
	// is a problem loading it, the mission will not be loaded.
	pugi::xml_node xmlTimer = xmlMission.child("Expires");
	if( !xmlTimer.empty() )
	{
		m_expirationTimer = game_new Timer();
		if( !m_expirationTimer->loadTimer( xmlTimer ) )
		{
			r3dOutToLog( "!!! Failed to load timer for mission %d\n", m_ID);
			delete m_expirationTimer;
			m_expirationTimer = NULL;
			return false;
		}
	}

	uint32_t stageCount = 0;
	pugi::xml_node xmlStage = xmlMission.child("Stage");
	while( !xmlStage.empty() )
	{
		++stageCount;
		if( stageCount > MaxStages )
		{
			r3dOutToLog( "!!! Too many stages, MissionProgress Save/Load is only built to handle 255 Stages per Mission.\n" );
			return false;
		}

		Stage* stage = game_new Stage;
		if( !stage->loadStage( xmlStage ) )
		{
			r3dOutToLog("!!! Failed to load stage %d for mission %d\n", stageCount, m_ID);
			delete stage;
			return false;
		}
		m_stages.push_back( stage );

		xmlStage = xmlStage.next_sibling("Stage");
	}
	if( 0 >= m_stages.size() )
	{
		r3dOutToLog( "!!! No stages found for mission!\n" );
		return false;
	}

	uint32_t commandCount = 0;
	pugi::xml_node xmlCommand = xmlMission.child("Command");
	while( !xmlCommand.empty() )
	{
		++commandCount;
		Command* command = (Command*)game_new Command;
		if( !command->loadCommand( xmlCommand ) )
		{
			r3dOutToLog("!!! Failed to load command %d for mission %d\n", commandCount, m_ID);
			delete command;
			return false;
		}
		std::map< EOrderType, std::vector< Command* >* >::iterator mapIter = m_commands.find( command->m_orderType );
		if( mapIter == m_commands.end() )
		{
			std::vector< Command*>* pCommandVec = game_new std::vector< Command* >;
			if( !pCommandVec )
			{
				r3dOutToLog( "!!! Failed to allocate memory for Command!\n" );
				delete command;
				return false;
			}
			pCommandVec->push_back( command );
			m_commands[ command->m_orderType ] = pCommandVec;
		}
		else
		{
			std::vector< Command* >* pCommandVec = mapIter->second;
			if( !pCommandVec )
				pCommandVec = game_new std::vector< Command* >;
			pCommandVec->push_back( command );
		}

		xmlCommand = xmlCommand.next_sibling("Command");
	}

	uint32_t itemRewardCount = 0;
	pugi::xml_node xmlReward = xmlMission.child("ItemReward");
	while( !xmlReward.empty() )
	{
		++itemRewardCount;
		Reward* reward = (Reward*)game_new ItemReward;
		if( !reward->loadReward( xmlReward ) )
		{
			r3dOutToLog("!!! Failed to load item reward %d for mission %d\n", itemRewardCount, m_ID);
			delete reward;
			return false;
		}
		m_rewards.push_back( reward );

		xmlReward = xmlReward.next_sibling("ItemReward");
	}
	uint32_t statsRewardCount = 0;
	xmlReward = xmlMission.child("StatsReward");
	while( !xmlReward.empty() )
	{
		++statsRewardCount;
		Reward* reward = (Reward*)game_new StatsReward;
		if( !reward->loadReward( xmlReward ) )
		{
			r3dOutToLog("!!! Failed to load stats reward %d for mission %d\n", statsRewardCount, m_ID);
			delete reward;
			return false;
		}
		m_rewards.push_back( reward );

		xmlReward = xmlReward.next_sibling("StatsReward");
	}

	return true;
}

//=========================================================================

MissionManager::MissionManager()
{
}

MissionManager::~MissionManager()
{
	for( std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.begin();
		 mapIter != m_mapMissions.end(); ++mapIter )
	{
		Mission* mission = mapIter->second;
		if( !mission )
			continue;

		delete mapIter->second;
		mapIter->second = NULL;
	}
	m_mapMissions.clear();
}

bool MissionManager::Init()
{
	r3d_assert( m_mapMissions.size() == 0 );

	// load Mission Map
	{
		char fname[MAX_PATH];
		sprintf(fname, "%s\\Missions.xml", r3dGameLevel::GetHomeDir());
		
		r3dFile* f = r3d_open(fname, "rb");
		if ( !f )
		{
			r3dOutToLog("!!! ERROR: Failed to open %s\n", fname);
			return false;
		}

		char* fileBuffer = game_new char[f->size + 1];
		r3d_assert(fileBuffer);
		fread(fileBuffer, f->size, 1, f);
		fileBuffer[f->size] = 0;
		pugi::xml_document xmlFile;
		pugi::xml_parse_result parseResult = xmlFile.load_buffer_inplace(fileBuffer, f->size);
		fclose(f);
		if(!parseResult)
		{
			r3dOutToLog("!!! Failed to parse XML, error: %s\n", parseResult.description());
			return false;
		}
		pugi::xml_node xmlDB = xmlFile.child("MissionDB");
		{
			Timer::SetCurrentTime();
			pugi::xml_node xmlMission = xmlDB.child("Mission");
			while(!xmlMission.empty())
			{
				Mission* mission = game_new Mission;
				if( !mission->loadMission( xmlMission ) )
				{
					xmlMission = xmlMission.next_sibling();
					continue;
				}
				xmlMission = xmlMission.next_sibling();


				// TODO: allow inactive missions to load for the Level Editor,
				//		 but not the game; commented out for now.
				//if( mission->m_active )
					m_mapMissions[ mission->m_ID ] = mission;
				//else
				//	delete mission;

				// Check for expired missions.
				if( mission->m_expirationTimer && mission->m_expirationTimer->IsExpired() )
				{
					mission->m_active = false;
				}
			}
		}
		delete [] fileBuffer;
	}

	return true;
}

void MissionManager::Update()
{
	// Update all the player's missions, and hand out any rewards
	Timer::SetCurrentTime();
	for( std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.begin();
		 mapIter != m_mapMissions.end(); ++mapIter )
	{
		Mission* mission = mapIter->second;
		if( !mission || !mission->m_active )
			continue;

		// Update the player's mission data
		for(int i=0; i < gServerLogic.curPlayers_; i++)
		{
			obj_ServerPlayer* player = gServerLogic.plrList_[i];
			if( player->loadout_->Alive == 0 )
				continue;

			uint32_t outStage;
			uint32_t outObjective;
			if( player->m_MissionsProgress && player->m_MissionsProgress->Update( mapIter->first, outStage, outObjective ) )
			{
				// Give Objective rewards to the player
				std::vector< Reward* >& objectiveRewards = mission->m_stages[ outStage ]->m_objectives[ outObjective ]->m_rewards;
				for( std::vector< Reward* >::iterator iter = objectiveRewards.begin(); iter != objectiveRewards.end(); ++iter )
				{
					(*iter)->Award( player, RWD_ObjectiveComplete );
				}

				// Give Mission rewards to the player, if they've completed the last stage.
				if( outStage == mission->m_stages.size() - 1 &&
					outObjective == mission->m_stages[ outStage ]->m_objectives.size() - 1 )
				{
					std::vector< Reward* >& missionRewards = mission->m_rewards;
					for( std::vector< Reward* >::iterator iter = missionRewards.begin(); iter != missionRewards.end(); ++iter )
					{
						(*iter)->Award( player, RWD_MissionComplete );
					}
					player->loadout_->missionsProgress->SetMissionCompleted( mapIter->first, true );
					r3dOutToLog("Mission(%d): COMPLETED Mission, Player %s\n", mapIter->first, player->Name);

					// Inform the client the Mission is Complete
					PKT_S2C_MissionComplete_s n;
					n.missionID = mapIter->first;
					gServerLogic.p2pSendRawToPeer( player->peerId_, &n, sizeof( n ), true );

					// Remove the completed mission from the player's list.
					player->m_MissionsProgress->RemoveMission( mapIter->first, RMV_MissionCompleted );

					// Update the character's saved mission progress data.
					g_AsyncApiMgr->AddJob( new CJobUpdateMissionsData( player ) );
				}
			}

			// Check for Mission Expirations.
			if( mission->m_expirationTimer && mission->m_expirationTimer->IsExpired() )
			{
				// Deactivate the mission.
				mission->m_active = false;

				if( player->m_MissionsProgress->RemoveMission( mapIter->first, RMV_MissionExpired ) )
				{
					r3dOutToLog("Mission(%d): EXPIRED Removing it from player, '%'.\n", mission->m_ID, player->Name);
					g_AsyncApiMgr->AddJob(new CJobUpdateMissionsData( player ));
				}
			}
		}
	}
}

void MissionManager::RemoveInactiveMissionsFromPlayers()
{
	// Update all the player's missions, removing any inactive missions
	for( std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.begin();
		 mapIter != m_mapMissions.end(); ++mapIter )
	{
		Mission* mission = mapIter->second;
		if( !mission )
			continue;

		if( !mission->m_active )
		{
			// Remove the mission from the player's mission data
			for(int i=0; i < gServerLogic.curPlayers_; i++)
			{
				obj_ServerPlayer* player = gServerLogic.plrList_[i];
				if( !player->m_MissionsProgress )
					continue;

				player->m_MissionsProgress->RemoveMission( mapIter->first, RMV_Administration );
			}
		}
	}
}

void MissionManager::ResetMissionData( obj_ServerPlayer* player )
{
	if( !player || player->loadout_->Alive == 0 || player->userName[0] == '\0' )
		return;

	// Reset a player's mission data by player name.
	r3dOutToLog("Resetting %s's Mission Data.\n", player->userName);
	player->loadout_->missionsProgress->ResetMissionData();
	SendActiveMissionsToPlayer( *player );
}

#ifdef MISSION_TRIGGERS
uint32_t MissionManager::TriggerMission( GameObject* triggerObj )
{
	for( std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.begin();
		 mapIter != m_mapMissions.end(); ++mapIter )
	{
		if( mapIter->second->m_active && mapIter->second->m_triggerObjHash == triggerObj->GetHashID() )
			return mapIter->first;
	}
	return UINT_MAX;
}
#endif

void MissionManager::GetAvailableMissionIDs( std::vector< uint32_t >& missionIDs )
{
	missionIDs.clear();
	for( std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.begin();
		 mapIter != m_mapMissions.end(); ++mapIter )
	{
		if( mapIter->second->m_active )
			missionIDs.push_back( mapIter->first );
	}
}

uint32_t MissionManager::GetNumRequiredActions( uint32_t missionID, uint32_t stageIndex, uint32_t objectiveIndex )
{
	std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.find( missionID );
	if( mapIter == m_mapMissions.end() || mapIter->second->m_stages.size() <= stageIndex )
	{
		r3dOutToLog( "Mission(%d): MissionManager should not be getting invalid missionIDs\n", missionID );
		return 0;
	}
	if( stageIndex != UINT_MAX && mapIter->second->m_stages.size() <= stageIndex )
	{
		r3dOutToLog( "Mission(%d): Stage Index must be smaller than the size of the list of stages", missionID );
		return 0;
	}
	Stage* stage = mapIter->second->m_stages[ stageIndex ];
	r3d_assert( stage );
	if( !stage || stage->m_objectives.size() <= objectiveIndex )
	{
		r3dOutToLog( "Mission(%d): Objective Index must be smaller than the size or the list of objectives", missionID );
		return 0;
	}
	Objective* objective = stage->m_objectives[ objectiveIndex ];
	r3d_assert( objective );
	if( !objective )
		return 0;

	return objective->m_mustCompleteNumActions;
}

uint32_t MissionManager::GetTriggerObjectHash( uint32_t missionID, uint32_t stageIndex /* = UINT_MAX */ )
{
	std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.find( missionID );
	if( mapIter == m_mapMissions.end() )
	{
		r3dOutToLog( "Mission(%d): MissionManager should not be getting invalid missionIDs\n", missionID );
		return 0;
	}
	if( stageIndex != UINT_MAX && mapIter->second->m_stages.size() <= stageIndex )
	{
		r3dOutToLog( "Mission(%d): Stage Index must be smaller than the size of the list of stages", missionID );
		return 0;
	}
	if( stageIndex == UINT_MAX )
		return mapIter->second->m_triggerObjHash;
	Stage* stage = mapIter->second->m_stages[ stageIndex ];
	r3d_assert( stage );
	if( !stage )
		return 0;

	return stage->m_triggerObjHash;
}

bool MissionManager::IsMissionActive( uint32_t missionID )
{
	std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.find( missionID );
	if( mapIter == m_mapMissions.end() )
	{
		r3dOutToLog( "Mission(%d): MissionManager should not be getting invalid missionIDs\n", missionID );
		return false;
	}
	return mapIter->second->m_active;
}

void MissionManager::SetMissionActive( uint32_t missionID, bool active )
{
	std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.find( missionID );
	if( mapIter == m_mapMissions.end() )
	{
		r3dOutToLog( "Mission(%d): MissionManager should not be getting invalid missionIDs\n", missionID );
		return;
	}
	mapIter->second->m_active = active;

	
	for(DWORD i=0; i < gServerLogic.MAX_PEERS_COUNT; ++i) 
	{
		if(gServerLogic.peers_[ i ].status_ >= gServerLogic.PEER_PLAYING) 
		{

			PKT_S2C_MissionActivate_s n;
			n.missionID		= missionID;
			n.data			= ((active) ? 1 : 0) |
				((gServerLogic.peers_[ i ].player->loadout_->missionsProgress->IsMissionAccepted( missionID )) ? (1<<1) : 0) |
				(IsMissionSaveDisallowed( missionID ) ? (1<<2) : 0);
			r3dscpy( n.missionNameStringID, mapIter->second->m_name );
			r3dscpy( n.missionDescStringID, mapIter->second->m_desc );
			r3dscpy( n.missionIconPath, mapIter->second->m_icon );

			gServerLogic.p2pSendRawToPeer( i, &n, sizeof( n ), true );
		}
	}
}

bool MissionManager::IsMissionSaveDisallowed( uint32_t missionID )
{
	std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.find( missionID );
	if( mapIter == m_mapMissions.end() )
	{
		r3dOutToLog( "Mission(%d): MissionManager should not be getting invalid missionIDs\n", missionID );
		return true;
	}

	return mapIter->second->m_disallowSave;
}

void MissionManager::SendActiveMissionsToPlayer( const obj_ServerPlayer& player )
{
	// send list of active Missions
	std::vector< uint32_t > availMissions;
	GetAvailableMissionIDs( availMissions );
	for( std::vector< uint32_t >::const_iterator iter = availMissions.begin(); iter != availMissions.end(); ++iter )
	{
		std::map<uint32_t, Mission*>::iterator mapIter = m_mapMissions.find( *iter );
		
		// Don't send completed missions.
		if( !player.loadout_->missionsProgress->IsMissionCompleted( *iter ) || d_enable_mission_repeat->GetBool() )
		{
			PKT_S2C_MissionActivate_s n;
			n.missionID		= mapIter->second->m_ID;
			n.data			= 1 |
				((player.loadout_->missionsProgress->IsMissionAccepted( mapIter->second->m_ID )) ? (1<<1) : 0) |
				(IsMissionSaveDisallowed( mapIter->second->m_ID ) ? (1<<2) : 0);
			r3dscpy( n.missionNameStringID, mapIter->second->m_name );
			r3dscpy( n.missionDescStringID, mapIter->second->m_desc );
			r3dscpy( n.missionIconPath, mapIter->second->m_icon );

			gServerLogic.p2pSendRawToPeer( player.peerId_, &n, sizeof( n ), true );
		}
	}
	availMissions.clear();
}

MissionManager* g_pMissionMgr = NULL;

//-------------------------------------------------------------------------

}// namespace Mission
#endif
