//=========================================================================
//	Module: MissionRewards.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "MissionRewards.h"
#include "..\obj_ServerPlayer.h"
#include "..\sobj_DroppedItem.h"
#include "ObjectsCode\Weapons\WeaponArmory.h"

extern ServerGameLogic gServerLogic;
extern wiInventoryItem RollItem(const class LootBoxConfig* lootCfg, int depth);

//-------------------------------------------------------------------------

#ifdef MISSIONS
namespace Mission
{
//-------------------------------------------------------------------------
Reward::Reward()
	: m_minReputation( INT_MIN )
	, m_maxReputation( INT_MAX )
{
}

bool find_minRep_predicate(pugi::xml_attribute attr) { return stricmp(attr.name(), "minRep") == 0; }
bool find_maxRep_predicate(pugi::xml_attribute attr) { return stricmp(attr.name(), "maxRep") == 0; }
bool Reward::loadReward( pugi::xml_node& xmlReward )
{
	if( xmlReward.empty() )
	{
		r3dOutToLog( "!!! Reward not defined!\n" );
		return false;
	}

	// Don't want default values of 0 here, so a search is performed
	// first, to make sure the attribute exists.
	if( !xmlReward.find_attribute(find_minRep_predicate).empty() )
		m_minReputation = xmlReward.attribute("minRep").as_int();
	if( !xmlReward.find_attribute(find_maxRep_predicate).empty() )
		m_maxReputation	= xmlReward.attribute("maxRep").as_int();

	if( m_minReputation > m_maxReputation )
	{
		int temp = m_minReputation;
		m_minReputation = m_maxReputation;
		m_maxReputation = temp;
	}

	return true;
}

bool Reward::MeetsReputationRestrictions( obj_ServerPlayer* player )
{
	if( m_minReputation == INT_MIN && m_maxReputation == INT_MAX )
		return true;

	if( m_minReputation > INT_MIN && m_maxReputation == INT_MAX )
	{
		return player->loadout_->Stats.Reputation >= m_minReputation;
	}
	else if( m_minReputation == INT_MIN && m_maxReputation < INT_MAX )
	{
		return player->loadout_->Stats.Reputation <= m_maxReputation;
	}

	return (player->loadout_->Stats.Reputation >= m_minReputation &&
			player->loadout_->Stats.Reputation <= m_maxReputation);
}


//-------------------------------------------------------------------------


ItemReward::ItemReward()
	: m_itemID( 0 )
	, m_amount( 0 )
{ }

ItemReward::~ItemReward()
{
}

void ItemReward::Award( obj_ServerPlayer* player, int rewardID )
{
	r3d_assert( player );
	if( !player ) return;

	if( !MeetsReputationRestrictions( player ) )
	{
		r3dOutToLog("Mission: Reward, Player %s with %d rep. doesn't meet the reputation restrictions %d/%d\n", player->Name.c_str(), player->loadout_->Stats.Reputation, m_minReputation, m_maxReputation );
		return;
	}

	wiInventoryItem wi;

	// If the rewardID is a Loot table, roll the reward, otherwise, give them the item.
	LootBoxConfig* cfg = const_cast<LootBoxConfig*>(g_pWeaponArmory->getLootBoxConfig( m_itemID ));
	if( cfg )
		wi = RollItem( cfg, 0 );
	else
		wi.itemID   = m_itemID;
	wi.quantity = (int)m_amount;
	if( wi.itemID == 0 )
	{
		r3dOutToLog("Mission: Reward, Player %s got NOTHING, because the rolled itemID was 0.\n", player->Name.c_str());
		return;
	}
	if( !player->BackpackAddItem(wi) )
	{
		// create network object
		r3dPoint3D pos = player->GetRandomPosForItemDrop();
		obj_DroppedItem* obj = (obj_DroppedItem*)srv_CreateGameObject("obj_DroppedItem", "obj_DroppedItem", pos);
		player->SetupPlayerNetworkItem(obj);
		// vars
		obj->m_Item = wi;
		r3dOutToLog("Mission: Reward, Not enough room in Player %s's backpack, itemID(%d) placed on ground at <%0.2f, %0.2f, %0.2f> \n", player->Name.c_str(), wi.itemID, pos.x, pos.y, pos.z);
	}
	r3dOutToLog("Mission: Reward, Player %s got itemID(%d)\n", player->Name.c_str(), wi.itemID);
}

bool ItemReward::loadReward( pugi::xml_node& xmlReward )
{
	if( xmlReward.empty() )
	{
		r3dOutToLog( "!!! Item Reward not defined!\n" );
		return false;
	}

	m_itemID	= xmlReward.attribute("itemID").as_uint();
	m_amount	= xmlReward.attribute("amount").as_uint();

	if( 0 >= m_itemID )
	{
		r3dOutToLog( "!!! Item ID must be specified for an ItemReward!\n" );
		return false;
	}
	if( 0 >= m_amount )
	{
		r3dOutToLog( "!!! An amount of the item must be specified for an ItemReward!\n" );
		return false;
	}

	return Reward::loadReward( xmlReward );
}

//-------------------------------------------------------------------------

StatsReward::StatsReward()
	: m_amountXP( 0 )
	, m_amountGD( 0 )
	, m_amountGP( 0 )
	, m_amountRep( 0 )
{
}

StatsReward::~StatsReward()
{
}

void StatsReward::Award( obj_ServerPlayer* player, int rewardID )
{
	r3d_assert( player );
	if( !player ) return;

	if( !MeetsReputationRestrictions( player ) )
	{
		r3dOutToLog("Mission: Reward, Player %s with %d rep. doesn't meet the reputation restrictions %d/%d\n", player->Name.c_str(), player->loadout_->Stats.Reputation, m_minReputation, m_maxReputation );
		return;
	}

	// Give the player the experience, game dollars, game points and reputation.
	wiStatsTracking reward;
	reward.RewardID	= rewardID;
	reward.XP		= (int)m_amountXP;
	reward.GD		= (int)m_amountGD;
	reward.GP		= (int)m_amountGP;
	reward = player->AddReward( reward );  // AddReward handles GP, which is not sent to the client.
	player->loadout_->Stats.Reputation += m_amountRep;

	// Update the client.
	PKT_S2C_AddScore_s n;
	n.ID = (WORD)rewardID;
	n.XP = R3D_CLAMP((int)m_amountXP, 0, 30000);;
	n.GD = (WORD)reward.GD;
	n.Rep = R3D_CLAMP(m_amountRep, SHRT_MIN, SHRT_MAX);
	gServerLogic.p2pSendToPeer( player->peerId_, player, &n, sizeof(n) );

	r3dOutToLog("Mission: Reward, Player %s got %dxp %dgp %dgd %drep RWD_ID %d\n", player->Name.c_str(), m_amountXP, m_amountGP, m_amountGD, m_amountRep, rewardID);
}

bool StatsReward::loadReward( pugi::xml_node& xmlReward )
{
	if( xmlReward.empty() )
	{
		r3dOutToLog( "!!! Reward not defined!\n" );
		return false;
	}

	m_amountXP = xmlReward.attribute("xp").as_uint();
	m_amountGD = xmlReward.attribute("gd").as_uint();
	m_amountGP = xmlReward.attribute("gp").as_uint();
	m_amountRep = xmlReward.attribute("rep").as_int();

	// Make sure something is being awarded.
	if( 0 >= m_amountXP && 0 >= m_amountGD && 0 >= m_amountGP && 0 == m_amountRep )
	{
		r3dOutToLog( "!!! At least one stat must be specified for a StatsReward!\n" );
		return false;
	}

	return Reward::loadReward( xmlReward );
}

//-------------------------------------------------------------------------

}// namespace Mission
#endif