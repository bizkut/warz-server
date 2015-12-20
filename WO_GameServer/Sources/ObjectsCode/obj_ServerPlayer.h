#pragma once

#include "GameCommon.h"
#include "Backend/ServerUserProfile.h"
#include "multiplayer/P2PMessages.h"
#include "multiplayer/NetCellMover.h"
#include "NetworkHelper.h"
#include "Missions/MissionProgress.h"

#define MAX_UAV_TARGETS 8

class Gear;
class ServerWeapon;
class WeaponConfig;
class CJobBuyItem;
class CJobBackpackFromInventory;
class CJobBackpackToInventory;
class obj_ServerLockbox;

enum Playerstate_e
{
	PLAYER_INVALID = -1,

	PLAYER_IDLE = 0,
	PLAYER_IDLEAIM,

	PLAYER_MOVE_CROUCH,
	PLAYER_MOVE_CROUCH_AIM,
	PLAYER_MOVE_WALK_AIM,
	PLAYER_MOVE_RUN,
	PLAYER_MOVE_SPRINT,
	PLAYER_MOVE_PRONE,
	PLAYER_MOVE_PRONE_AIM,
	PLAYER_PRONE_UP,
	PLAYER_PRONE_DOWN,
	PLAYER_PRONE_IDLE,

	PLAYER_SWIM_IDLE,
	PLAYER_SWIM_SLOW,
	PLAYER_SWIM,
	PLAYER_SWIM_FAST,
#ifdef VEHICLES_ENABLED
	PLAYER_VEHICLE_DRIVER,
	PLAYER_VEHICLE_PASSENGER,
#endif
	PLAYER_DIE,

	PLAYER_NUM_STATES,
};

class obj_ServerPlayer : public GameObject, INetworkHelper
{
  public:
	DECLARE_CLASS(obj_ServerPlayer, GameObject)

	// info about player
	DWORD		peerId_;

	bool		wasDeleted;

	bool		wasDisconnected_;

	bool		uavRequested_;
	gobjid_t	uavId_;

	float		startPlayTime_;
	float		m_PlayerRotation;
	int			m_PlayerState; // comes from client
	float		m_PlayerFlyingAntiCheatTimer;
	float		m_PlayerUndergroundAntiCheatTimer;
	int			m_PlayerSuperJumpDetection;
	char		userName[64];

	bool		m_DevPlayerHide;

	float		m_Stamina;
	float		m_lastTimeUsedConsumable;
	float		m_currentConsumableCooldownTime;

	float		m_AggressionTimeUntil; // time until player has aggresion timer on him (means anyone can kill him without reputation penalty)

	bool		m_isAdmin_GodMode; // god mode, no one can hurt admin

	float		m_SpawnProtectedUntil;

	float		m_LeaveGroupAtTime;

#ifdef MISSIONS
	Mission::MissionsProgress*	m_MissionsProgress;
#endif

 //
 // equipment and loadouts
 //
	enum ESlot
	{
	  SLOT_Armor = 0,
	  SLOT_Headgear,
	  //SLOT_Char, NOT USED FOR NOW
	  SLOT_Max,
	};
	Gear*		gears_[SLOT_Max];

	ServerWeapon*	m_WeaponArray[NUM_WEAPONS_ON_PLAYER];
	int		m_SelectedWeapon;
	bool		m_isFlashlightOn;
	bool		m_clipAttmChanged;	// if last attachment change was a clip one
	int		m_dbg_PreviousWeapon[NUM_WEAPONS_ON_PLAYER];
	
	wiNetWeaponAttm	GetWeaponNetAttachment(int wid);
	
	struct bullets_s
	{
	  gobjid_t	localId;
	  int		fireSeqNo;
	  int		wid;
	  int		ItemID;	// ItemID of fired weapon. used to check if weapon was swapped while firing
	};
	std::vector<bullets_s> bullets_;

	bool		FireWeapon(int wid, int fireSeqNo, gobjid_t localId); // should be called only for FIRE event
	bool		 AdjustWeaponDurability(ServerWeapon* wpn); // returns false if weapon is no longer valid (destroyed due to durability)
	ServerWeapon*	OnBulletHit(gobjid_t localId, const char* pktName, bool hitSomethiing = true);
	float		weapDataReqSent;	// time when we sent PKT_C2S_PlayerWeapDataReq
	float		weapDataReqExp;		// time wnen we expect PKT_C2S_PlayerWeapDataAns answer or -1 if not active
	bool		weapCheatReported;

	void		SetLoadoutData();
	void		 SetWeaponSlot(int wslot, uint32_t weapId, const wiWeaponAttachment& attm);
	void		 SetGearSlot(int gslot, uint32_t gearId);

	bool		isNVGEquipped() const;

	void ReviveFast();
	
	int		FireHitCount; // counts how many FIRE and HIT events received. They should be equal. If not - cheating detected
	float		deathTime;
	int            GodMode;
	char		aggressor[64];
	uint8_t		killedBy;

	bool		isGameHardcore; //gamehardcore

	void		DoDeath();
	
	float		lastTimeHit;
	int		lastHitBodyPart;
	uint32_t lastTimeHitItemID;
	float		ApplyDamage(float damage, GameObject* fromObj, int bodyPart, STORE_CATEGORIES damageSource, uint32_t dmgItemID, bool canApplyBleeding = true);
	float		 ReduceDamageByGear(int bodyPart, float damage);
	
	// some old things
	float		Height;
	
	// precalculated boost vars
	float		boostXPBonus_;
	float		boostWPBonus_;

	float		lastPickupNotifyTime;

	bool		security_utcGameTimeSent;
	bool		security_GameTimeSent;
	bool		security_NVGSent;
	float		security_screenshotRequestSentAt;

	float		m_ZombieRepelentTime;

	// for aim bot detection (fair fight)
	r3dPoint3D	lastCamPos;
	r3dPoint3D  lastCamDir;

	// group info
	uint32_t	groupID; // not zero if player is in group
	bool		isGroupLeader;
	struct GroupInviteStruct
	{
		uint32_t fromID;
		float	timeOfExpire; // time when this invite will expire
	};
	std::vector<GroupInviteStruct> groupInvitesFrom; // to keep track who invited that player and when

	// stats
	CServerUserProfile profile_;
	wiCharDataFull*	   loadout_;
	wiCharDataFull	savedLoadout_;	// saved loadout copy after last update
	int		savedGameDollars_; // saved value from last character update
	void		SetProfile(const CServerUserProfile& in_profile);
	
	bool		inventoryOpActive_;
	void		StartInventoryOp();
	
	obj_ServerLockbox* GetAccessToLockbox(gp2pnetid_t lockboxID, const char* AccessCodeS);
	
	void		OnBuyItemCallback(const CJobBuyItem* job);
	void		OnFromInventoryCallback(const CJobBackpackFromInventory* job);
	void		OnToInventoryCallback(const CJobBackpackToInventory* job);

	int		GetBackpackSlotForItem(const wiInventoryItem& itm);
	void		AddItemToBackpackSlot(int slot, const wiInventoryItem& itm);
	void		AdjustBackpackSlotQuantity(int slot, int quantity, bool isAttachmentReplyReq = true);
	bool		IsHaveBackpackItem(uint32_t itemID, int quantity, bool remove = false);
	void		AddItemToInventory(__int64 InventoryID, const wiInventoryItem& itm);

	int		haveBadBackpack_;
	void		ValidateBackpack();
	void		ValidateAttachments();

	PKT_S2C_SetPlayerVitals_s lastVitals_;
	
	float		lastCharUpdateTime_;
	float		lastWorldUpdateTime_;
	DWORD		lastWorldFlags_;
	float		lastVisUpdateTime_;
	void		UpdateGameWorldFlags();
	
	r3dPoint3D	GetRandomPosForItemDrop();
	bool		BackpackAddItem(const wiInventoryItem& wi1);
	void		BackpackDropItem(int slot);
	int			BackpackRemoveItem(const wiInventoryItem& wi1);
	void		OnBackpackChanged(int slot);
	void		OnLoadoutChanged();
	void		OnAttachmentChanged(int wid, int atype);
	void		OnChangeBackpackSuccess(const std::vector<wiInventoryItem>& droppedItems);
	void		OnRemoveAttachments(int idx);

	wiStatsTracking	AddReward(const wiStatsTracking& rwd);

	bool		UseItem_Barricade(const r3dPoint3D& pos, float rotX, uint32_t itemID);
	bool		UseItem_ZombieRepelent(uint32_t itemID);
	bool		UseItem_FarmBlock(const r3dPoint3D& pos, float rotX, uint32_t itemID);
	bool		UseItem_Lockbox(const r3dPoint3D& pos, float rotX, uint32_t itemID);
	bool		UseItem_ApplyEffect(const PKT_C2C_PlayerUseItem_s& n, uint32_t itemID);
	bool		UseItem_UAV(const r3dPoint3D& pos, float rotX);
	bool		UseItem_CreateNote(const PKT_C2S_CreateNote_s& n);

	float		lastChatTime_;
	int		numChatMessages_;

	enum { MAX_TRADE_SIZE = 16, };
	gp2pnetid_t	tradeRequestTo;
	gp2pnetid_t	tradeTargetId; // with whom player is trading currently
	int		tradeStatus;
	int		numOfUAVHits;
	float	tradeLastChangeTime; 
	struct tradeSlot_s
	{
	  int		  SlotFrom;
	  wiInventoryItem Item;	// item to move
	};
	tradeSlot_s	tradeSlots[MAX_TRADE_SIZE];
	
	void		Trade_Request(const PKT_C2C_TradeRequest_s& n);
	void		Trade_Answer(const PKT_C2C_TradeRequest_s& n);
	void		Trade_Confirm();
	void		Trade_Close();
	int		Trade_CheckCanPlace(obj_ServerPlayer* target);
	void		Trade_Commit(obj_ServerPlayer* target);
	
	void		RepairItemWithKit(int slot, bool isPremium);
	void		RepairItemWithGD(int slot, bool repailAllRequest);
	
	CNetCellMover	netMover;
	float		moveAccumDist[5]; // regular speed, sprint speed, prone speed, crouch speed, vehicle speed
	float		moveAccumTime[5];
	bool		moveInited;
	bool		CheckForFastMove();
	
	float		lastPlayerAction_;	// used for AFK checks

	float		lastCallForHelp;
	r3dPoint3D	lastCallForHelpLocation;
	char		CallForHelp_distress[1024];
	char		CallForHelp_reward[512];

	int AntiAbuseclickNSG;
	int AntiAbuseclickNSZ;

	// packet sequences, used to skip late packets after logic reset
	// for example teleports, game state reset, etc
	DWORD		myPacketSequence;	// expected packets sequence ID
	DWORD		clientPacketSequence;	// received packets sequence ID
	const char*	packetBarrierReason;
	void		SetLatePacketsBarrier(const char* reason);
	
	void		SetupPlayerNetworkItem(GameObject* obj);	// helper for player-created network items

#ifdef VEHICLES_ENABLED
	DWORD		currentVehicleId;
	bool		isDriver;
	bool		isInVehicle;
	int			currentVehicleType;
	int			seatPosition;

	int			GetVehicleType();
	bool		IsInVehicle();

	void		EnterVehicle(obj_Vehicle* vehicle);
	void		ExitVehicle(bool sendPacket = false, bool isForced = false, bool isDisconnecting = false);
#endif

private: // disable access to SetPosition directly, use TeleportPlayer
	void		SetPosition(const r3dPoint3D& pos)
	{
		__super::SetPosition(pos);
	}

	bool		IsSwimming();
	//bool		IsOverWater(float& waterDepth);

public:
	void		TeleportPlayer(const r3dPoint3D& pos);
	
	void		OnNetPacket(const PKT_C2C_PacketBarrier_s& n);
	void		OnNetPacket(const PKT_C2C_MoveSetCell_s& n);
	void		OnNetPacket(const PKT_C2C_MoveRel_s& n);
	void		OnNetPacket(const PKT_C2S_MoveCameraLocation_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerJump_s& n);
	void		OnNetPacket(const PKT_C2S_PlayerEquipAttachment_s& n);
	void		OnNetPacket(const PKT_C2S_PlayerRemoveAttachment_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerSwitchWeapon_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerSwitchFlashlight_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerUseItem_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerReload_s& n);
	void		OnNetPacket(const PKT_C2S_PlayerUnloadClip_s& n);
	void		OnNetPacket(const PKT_C2S_PlayerCombineClip_s& n);
	void		OnNetPacket(PKT_C2C_PlayerFired_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerHitNothing_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerHitStatic_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerHitStaticPierced_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerHitDynamic_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerHitResource_s& n);
	void		OnNetPacket(const PKT_C2C_PlayerReadyGrenade_s& n);
	void		OnNetPacket(PKT_C2C_PlayerThrewGrenade_s& n);
	void		OnNetPacket(const PKT_C2S_PlayerChangeBackpack_s& n);
	void		OnNetPacket(const PKT_C2S_BackpackDrop_s& n);
	void		OnNetPacket(const PKT_C2S_BackpackSwap_s& n);
	void		OnNetPacket(const PKT_C2S_BackpackJoin_s& n);
	void		OnNetPacket(const PKT_C2S_BackpackDisassembleItem_s& n);
	void		OnNetPacket(const PKT_C2S_ShopBuyReq_s& n);
	void		OnNetPacket(const PKT_C2S_FromInventoryReq_s& n);
	void		OnNetPacket(const PKT_C2S_ToInventoryReq_s& n);
	void		OnNetPacket(const PKT_C2S_RepairItemReq_s& n);
	void		OnNetPacket(const PKT_C2S_DisconnectReq_s& n);
	void		OnNetPacket(const PKT_C2S_ReviveFast_s& n);
	void		OnNetPacket(const PKT_C2S_FallingDamage_s& n);
	void		OnNetPacket(const PKT_C2S_GroupInvitePlayer_s& n);
	void		OnNetPacket(const PKT_C2S_GroupAcceptInvite_s& n);
	void		OnNetPacket(const PKT_C2S_GroupLeaveGroup_s& n);
	void		OnNetPacket(const PKT_C2S_GroupKickPlayer_s& n);
	void		OnNetPacket(const PKT_C2S_CallForHelpReq_s& n);
	void		OnNetPacket(const PKT_C2S_LockboxItemLockboxToBackpack_s& n);
	void		OnNetPacket(const PKT_C2S_LockboxItemBackpackToLockbox_s& n);
	void		OnNetPacket(const PKT_C2S_LockboxPickup_s& n);
	void		OnNetPacket(const PKT_C2C_TradeRequest_s& n);
	void		OnNetPacket(const PKT_C2C_TradeItem_s& n);
	void		OnNetPacket(const PKT_C2S_LearnRecipe_s& n);
	void		OnNetPacket(const PKT_C2S_CraftItem_s& n);
	
	void		OnNetPacket(const PKT_C2S_PlayerWeapDataRepAns_s& n);

#ifdef VEHICLES_ENABLED
	void		OnNetPacket(const PKT_C2S_VehicleRepair_s& n);
	void		OnNetPacket(const PKT_C2S_VehicleRefuel_s& n);
#endif

	void		RelayPacket(const DefaultPacket* packetData, int packetSize, bool guaranteedAndOrdered = true);


  public:
	obj_ServerPlayer();
	virtual ~obj_ServerPlayer();
	
virtual	BOOL		Load(const char *name);
virtual	BOOL		OnCreate();			// ObjMan: called before objman take control of this object
virtual	BOOL		OnDestroy();			// ObjMan: just before destructor called

virtual	void		RecalcBoundBox();

virtual	BOOL		Update();			// ObjMan: tick update

virtual	BOOL	 	OnCollide(GameObject *obj, CollisionInfo &trace);

	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);
	void		INetworkHelper::LoadServerObjectData() { r3dError("not implemented"); }
	void		INetworkHelper::SaveServerObjectData() { r3dError("not implemented"); }

virtual	BOOL		OnNetReceive(DWORD EventID, const void* packetData, int packetSize);
};

