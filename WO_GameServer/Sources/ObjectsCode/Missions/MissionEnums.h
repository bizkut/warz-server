//=========================================================================
//	Module: MissionActions.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

//-------------------------------------------------------------------------
#ifdef MISSIONS

namespace Mission
{

enum EActionState
{
	 STATE_Unknown = 0
	,STATE_Inactive
	,STATE_Active

	, STATE_MAX	// This must be last.
};
static const char* ActionStateNames[] = {
	 "Inactive"
	,"Active"
};

enum EActionType
{
	 ACT_Undefined = 0
	,ACT_Goto
	,ACT_Item
	,ACT_Kill
	,ACT_State

	,ACT_MAX	// This must be last.
};
static const char* ActionTypeNames[] = {
	 "Goto"
	,"Item"
	,"Kill"
	,"State"
};

enum EItemActionType
{
	 ITEM_Collect = 1
	,ITEM_Drop
	,ITEM_Use
	,ITEM_Craft
	,ITEM_Deliver

	,ITEM_MAX	// This must be last.
};
static const char* ItemActionTypeNames[] = {
	 "Collect"
	,"Drop"
	,"Use"
	,"Craft"
	,"Deliver"
};

enum EItemUseOnTarget
{
	 ITEMUSEON_NotSpecified = 0
	,ITEMUSEON_Self			= 1
	,ITEMUSEON_OtherPlayer	= 2
	,ITEMUSEON_Anyone		= 3	// Made up of Self(0x01) and OtherPlayer(0x10)

	,ITEMUSEON_MAX	// This must be last.
};
static const char* ItemUseOnTargetNames[] = {
	 "Self"
	,"Other"
	,"Anyone"
};

enum EActionIcon
{
	 ACTICON_NotSet = 0
	,ACTICON_Goto
	,ACTICON_Kill
	,ACTICON_ItemCollect
	,ACTICON_ItemDrop
	,ACTICON_ItemUse
	,ACTICON_StateOn
	,ACTICON_StateOff

	,ACTICON_MAX
};
// These must match the ActionIconNames in HUDPause.cpp on the client.
static const char* ActionIconNames[] = {
	 "Goto"
	,"Kill"
	,"Collect"
	,"Drop"
	,"Use"
	,"On"
	,"Off"
};

// These values must match the enumeration values in GameObj.h
static int KillObjectType[] = {
	 (1<<5)		// OBJTYPE_Human
	,(1<<17)	// OBJTYPE_Zombie
	,(1<<27)	// OBJTYPE_Deer
};
static int KILL_OBJTYPE_MAX = 3;	// This must specify the number of matched entries in KillObjectType and KillObjectTypeNames
static const char* KillObjectTypeNames[] = {
	 "Player"
	,"Zombie"
	,"Deer"
};

enum ECommandType
{
	 CMD_Undefined = 0
	,CMD_Spawn
	,CMD_Destroy
	,CMD_Give
	,CMD_Remove

	,CMD_MAX	// This must be last.
};
static const char* CommandTypeNames[] = {
	 "Spawn"
	,"Destroy"
	,"Give"
	,"Remove"
};

enum EOrderType
{
	 ORDER_Unknown = 0
	,ORDER_Start
	,ORDER_End

	,ORDER_MAX	// This must be last.
};
static const char* OrderTypeNames[] = {
	 "Start"
	,"End"
};

enum ERemoveReason
{
	 RMV_Administration = 0
	,RMV_MissionCompleted
	,RMV_MissionAbandoned
	,RMV_MissionExpired
};

} // namespace Mission
#endif