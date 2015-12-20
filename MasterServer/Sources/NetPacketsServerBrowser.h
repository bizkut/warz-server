#pragma once

#include "r3dNetwork.h"
#include "../../ServerNetPackets/NetPacketsGameInfo.h"
#include "../../ServerNetPackets/NetPacketsWeaponInfo.h"

enum ESBServerType
{
	SBNET_SERVER_Gameworld  = 0x1,
	SBNET_SERVER_Stronghold = 0x2,
	SBNET_SERVER_ANY        = 0x3,
};

namespace NetPacketsServerBrowser
{
#pragma pack(push)
#pragma pack(1)

#define SBNET_VERSION		(0x0000120f + GBWEAPINFO_VERSION + GBGAMEINFO_VERSION)
#define SBNET_KEY1		0x1CD67412
#define SBNET_KEY2		0x2763E612

//
// Server Browser Packet IDs
// 
enum sbpktType_e
{
  SBPKT_ValidateConnectingPeer = r3dNetwork::FIRST_FREE_PACKET_ID,
  
  // supervisor <-> master
  SBPKT_S2M_RegisterMachine,
  SBPKT_M2S_RegisterAns,
  SBPKT_M2S_StartGameReq,
  SBPKT_S2M_ShutdownNote,
  SBPKT_S2M_DisableSlot,
  SBPKT_S2M_GameDisconnect,		// game disconnected on supervisor
  
  // game <-> master (relayed thru supervisor now)
  // MAKE VERY SURE that first DWORD of that packet is gameId
  SBPKT_G2M_RegisterGame,		// register game session in master server
  SBPKT_M2G_ShutdownNote,
  SBPKT_M2G_KillGame,			// game need to be closed because of master error
  SBPKT_M2G_KickPlayer,
  SBPKT_M2G_SetGameFlags,
  SBPKT_G2M_AddPlayer,			// update info about game session
  SBPKT_G2M_RemovePlayer,		// update info about game session
  SBPKT_G2M_FinishGame,			// game is finished
  SBPKT_G2M_CloseGame,			// close session

  SBPKT_M2G_SendIsPVE,
  
  #ifdef ENABLE_ARMORY_UPDATE
  // data update packets
  SBPKT_G2M_DataUpdateReq,
  SBPKT_M2G_UpdateWeaponData,
  SBPKT_M2G_UpdateGearData,
  SBPKT_M2G_UpdateItemData,
  SBPKT_M2G_UpdateDataEnd,
  #endif
  
  SBPKT_LAST_PACKET_ID
};

#if SBPKT_LAST_PACKET_ID > 255
  #error Shit happens, more that 255 packet ids
#endif

#ifndef CREATE_PACKET
#define CREATE_PACKET(PKT_ID, VAR) PKT_ID##_s VAR
#endif

struct SBPKT_ValidateConnectingPeer_s : public r3dNetPacketMixin<SBPKT_ValidateConnectingPeer>
{
	DWORD		version;
	DWORD		key1;
	DWORD		key2;
};

struct SBPKT_S2M_RegisterMachine_s  : public r3dNetPacketMixin<SBPKT_S2M_RegisterMachine>
{
	BYTE		region;		// GBNET_REGION_Unknown
	BYTE		serverType;	// ESBServerType
	char		serverName[64];
	BYTE		mapId;		// EMapId or 0xFF for all games
	WORD		maxGames;
	WORD		maxPlayers;
	WORD		portStart;
	DWORD		externalIpAddr;
};

struct SBPKT_M2S_RegisterAns_s : public r3dNetPacketMixin<SBPKT_M2S_RegisterAns>
{
	DWORD		id;
};

struct SBPKT_M2S_StartGameReq_s : public r3dNetPacketMixin<SBPKT_M2S_StartGameReq>
{
	DWORD		gameId;
	__int64		sessionId;
	WORD		port;
	uint32_t	creatorID;
	GBGameInfo	ginfo;
};

struct SBPKT_S2M_ShutdownNote_s : public r3dNetPacketMixin<SBPKT_S2M_ShutdownNote>
{
};

struct SBPKT_S2M_DisableSlot_s : public r3dNetPacketMixin<SBPKT_S2M_DisableSlot>
{
	WORD		slot;
};

#define DEFINE_GAME_RELAYING_PACKET(xx) \
	DWORD		gameId;		/* relayed gameId - MAKE VERY SURE that first DWORD of that packet is gameId */ \
	private: ##xx() {}		/* hide constructor without gameid */ \
	public:  ##xx(DWORD in_gameId) { \
	  gameId = in_gameId; \
	}
	
struct SBPKT_S2M_GameDisconnect_s : public r3dNetPacketMixin<SBPKT_S2M_GameDisconnect>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_S2M_GameDisconnect_s)
};

struct SBPKT_G2M_RegisterGame_s : public r3dNetPacketMixin<SBPKT_G2M_RegisterGame>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_G2M_RegisterGame_s)
};

struct SBPKT_M2G_KillGame_s : public r3dNetPacketMixin<SBPKT_M2G_KillGame>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_M2G_KillGame_s)
	BYTE		reason;
};

struct SBPKT_M2G_ShutdownNote_s: public r3dNetPacketMixin<SBPKT_M2G_ShutdownNote>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_M2G_ShutdownNote_s)
	BYTE		reason;		// 1: master server shutdown, 2: supervisor shutdown
	float		timeLeft;
};

struct SBPKT_M2G_KickPlayer_s : public r3dNetPacketMixin<SBPKT_M2G_KickPlayer>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_M2G_KickPlayer_s)
	DWORD		CharID;
};

struct SBPKT_M2G_SetGameFlags_s : public r3dNetPacketMixin<SBPKT_M2G_SetGameFlags>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_M2G_SetGameFlags_s)
	BYTE		flags;
	int		gametimeLimit;
};

struct SBPKT_G2M_AddPlayer_s : public r3dNetPacketMixin<SBPKT_G2M_AddPlayer>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_G2M_AddPlayer_s)
	WORD		playerIdx;
	DWORD		CustomerID;
	DWORD		CharID;
	char		gamertag[32*2];
	int		reputation;
	int		XP;
};

struct SBPKT_G2M_RemovePlayer_s : public r3dNetPacketMixin<SBPKT_G2M_RemovePlayer>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_G2M_RemovePlayer_s)
	WORD		playerIdx;
};

struct SBPKT_G2M_FinishGame_s : public r3dNetPacketMixin<SBPKT_G2M_FinishGame>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_G2M_FinishGame_s)
};

struct SBPKT_G2M_CloseGame_s : public r3dNetPacketMixin<SBPKT_G2M_CloseGame>
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_G2M_CloseGame_s)
};

struct SBPKT_M2G_SendIsPVE_s : public r3dNetPacketMixin<SBPKT_M2G_SendIsPVE> // for PVE maps
{
	DEFINE_GAME_RELAYING_PACKET(SBPKT_M2G_SendIsPVE_s)
	DWORD isPVE;
};
#ifdef ENABLE_ARMORY_UPDATE
struct SBPKT_G2M_DataUpdateReq_s : public r3dNetPacketMixin<SBPKT_G2M_DataUpdateReq>
{
	DEFINE_GAME_RELAYING_PACKET
};

struct SBPKT_M2G_UpdateWeaponData_s : public r3dNetPacketMixin<SBPKT_M2G_UpdateWeaponData>
{
	DEFINE_GAME_RELAYING_PACKET
	DWORD		itemId;
	GBWeaponInfo	wi;
};

struct SBPKT_M2G_UpdateGearData_s : public r3dNetPacketMixin<SBPKT_M2G_UpdateGearData>
{
	DEFINE_GAME_RELAYING_PACKET
	DWORD		itemId;
	GBGearInfo	gi;
};

struct SBPKT_M2G_UpdateItemData_s : public r3dNetPacketMixin<SBPKT_M2G_UpdateItemData>
{
	DEFINE_GAME_RELAYING_PACKET
	DWORD		itemId;
	// for now it'll be used only for lootbox RequiredLevel update
	BYTE		LevelRequired;
};

struct SBPKT_M2G_UpdateDataEnd_s : public r3dNetPacketMixin<SBPKT_M2G_UpdateDataEnd>
{
	DEFINE_GAME_RELAYING_PACKET
};
#endif

#undef DEFINE_GAME_RELAYING_PACKET

#pragma pack(pop)

}; // namespace NetPacketsServerBrowser
