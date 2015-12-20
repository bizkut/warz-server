#pragma once

#include "GameCommon.h"
#include "multiplayer/NetCellMover.h"
#include "NetworkHelper.h"
#include "r3dInterpolator.h"

class obj_ZombieSpawn;
class obj_ServerGrenade;
class obj_ServerPlayer;
class ServerWeapon;

#include "ZombieNavAgent.h"

class obj_Zombie : public GameObject, INetworkHelper
{
	DECLARE_CLASS(obj_Zombie, GameObject)

	// Data saved for the end of a zombie's turn.
	struct TurnUserData
	{
		gobjid_t	targetID;
		int			nextState;
	};
	TurnUserData turnUserData;

public:
	static std::list<obj_Zombie*> s_ListOfAllActiveZombies;
	bool isSuperZombieForced;

	uint32_t ZombieGroup;

	obj_ZombieSpawn* spawnObject;
	CNetCellMover	netMover;
	r3dSlerpInterpolator turnInterp;

	bool		HalloweenZombie;
	int		HeroItemID;
	int		HeadIdx;
	int		BodyIdx;
	int		LegsIdx;

	uint32_t ZombieUniqueIDForFF;

	int			ZombieDisabled;	// zombie can't move, stuck
	void		DisableZombie();
	
	int		ZombieState;
	float		StateStartTime;
	float		StateTimer;
	float		HealTimer;
	float		AvoidanceIdleStartTime;
	void		SwitchToState(int in_state);

	float		ZombieHealth;

	int			FastZombie;
	float		WalkSpeed;
	float		RunSpeed;
	bool		CanSprint;
	float		SprintSpeed;
	float		SprintTimer;
	float		SprintFalloffTimer;
	float		SprintCooldownTimer;
	ZombieNavAgent* navAgent;
	int			patrolPntIdx;
	float		moveWatchTime;
	r3dPoint3D	moveWatchPos;
	float		moveStartTime;
	r3dPoint3D	moveStartPos;
	float		moveAvoidTime;
	r3dPoint3D	moveAvoidPos;
	r3dPoint3D	moveTargetPos;
	int			moveFrameCount;

	bool		CanCallForHelp;

	void		CreateNavAgent();
	void		StopNavAgent();
	void		SetNavAgentSpeed(float speed);
	bool		MoveNavAgent(const r3dPoint3D& pos, float maxAstarRange);
	void		FaceVector(const r3dPoint3D& v);
	r3dVector	GetFacingVector() const;
	r3dVector	GetRightVector() const;
	int			CheckMoveStatus();
	int			CheckMoveWatchdog();
	bool		CheckForBarricadeBlock();
	GameObject*	FindDoor();
	GameObject*	FindBarricade();

	void		CancelSprint();
	void		UpdateSprint(const GameObject& target, const float& distToTarget);
	void		UpdateSprintSpeed();

	float		staggerTime;
	int		animState;

	float		DetectRadius;		// smell detection radius
	gobjid_t	hardObjLock;
	gobjid_t	prevObjLock;
	r3dPoint3D	lastTargetPos;
	float		nextDetectTime;
	float		attackTimer;
	int			isFirstAttack;
	int			attackCounter;
	int			superAttackDir;
	int			idleFidget;
	GameObject*	ScanForTarget(bool immediate = false);
	void		SetTarget(const gobjid_t& objSafeID);
	bool		CheckViewToPlayer(const GameObject* obj);
	GameObject*	GetClosestPlayerBySenses();
	bool		IsPlayerDetectable(const obj_ServerPlayer* plr, float distSq);
	
	bool		SenseItemSound(const obj_ServerPlayer* plr, int ItemID);
	bool		SenseWeaponFire(const obj_ServerPlayer* plr, const ServerWeapon* wpn);
	bool		SenseGrenadeSound(const obj_ServerGrenade* grenade, bool isExplosion);
#ifdef VEHICLES_ENABLED
	void		SenseVehicleExplosion(const obj_Vehicle* vehicle);
	GameObject* GetClosestTargetBySenses();
	bool		IsVehicleDetectable(obj_Vehicle* vehicle, float distance);
	bool		CheckViewToVehicle(obj_Vehicle* vehicle);
	void		HandleVehicleAttack(obj_Vehicle* vehicle);
#endif

	bool		CallForHelp(const GameObject* trg);
	bool		CycleIdleFidget();
	bool		StartAttack(const GameObject* trg);
	void		StopAttack();
	bool		DoAttack(const GameObject* trg);
	bool		CanAttack(const GameObject* trg);// returns true if zombie can attack player (player is reachable to zombie)
	
	void		DoDeath(bool fakeDeath = false);
	
	void		AILog(int level, const char* fmt, ...);
	void		SendAIStateToNet();
	void		DebugSingleZombie();

	bool		IsSuperZombie();
	bool		IsDogZombie();
	void		PassiveHeal();
	void		ResetPassiveHealTimer();

public:
	obj_Zombie();
	~obj_Zombie();
	
	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();
	
	INetworkHelper*	GetNetworkHelper() { return dynamic_cast<INetworkHelper*>(this); }
	DefaultPacket*	INetworkHelper::NetGetCreatePacket(int* out_size);
	void		INetworkHelper::LoadServerObjectData() { r3dError("not implemented"); }
	void		INetworkHelper::SaveServerObjectData() { r3dError("not implemented"); }

virtual	BOOL		OnNetReceive(DWORD EventID, const void* packetData, int packetSize);
	
	bool		ApplyDamage(GameObject* fromObj, float damage, int bodyPart, STORE_CATEGORIES damageSource);
};
