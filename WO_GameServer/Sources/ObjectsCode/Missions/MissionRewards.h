//=========================================================================
//	Module: MissionRewards.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

class obj_ServerPlayer;

//-------------------------------------------------------------------------

namespace Mission
{
// Rewards are given to players when they complete Objectives or Missions.

class Reward
{
	Reward( const Reward& reward ) { }
	Reward& operator=( const Reward& reward ) { }

public:
	Reward();
	virtual ~Reward() { }

	virtual void Award( obj_ServerPlayer* player, int rewardID ) = 0;

	virtual bool loadReward( pugi::xml_node& xmlReward );

	bool MeetsReputationRestrictions( obj_ServerPlayer* player );

public:
	int32_t		m_minReputation;
	int32_t		m_maxReputation;
};

//-------------------------------------------------------------------------

class ItemReward : public Reward
{
	ItemReward( const ItemReward& itemReward ) { }
	ItemReward& operator=( const ItemReward& itemReward ) { }

public:
	ItemReward();
	virtual ~ItemReward();

	virtual void Award( obj_ServerPlayer* player, int rewardID );

	virtual bool loadReward( pugi::xml_node& xmlReward );

public:
	uint32_t	m_itemID;
	uint32_t	m_amount;
};

//-------------------------------------------------------------------------

class StatsReward : public Reward
{
	StatsReward( const StatsReward& StatsReward ) { }
	StatsReward& operator=( const StatsReward& StatsReward ) { }

public:
	StatsReward();
	virtual ~StatsReward();

	virtual void Award( obj_ServerPlayer* player, int rewardID );

	virtual bool loadReward( pugi::xml_node& xmlReward );

public:
	uint32_t	m_amountXP;
	uint32_t	m_amountGD;
	uint32_t	m_amountGP;
	int32_t		m_amountRep;
};

//-------------------------------------------------------------------------

} // namespace Mission
#endif
