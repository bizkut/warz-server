#pragma once

#include "GameCommon.h"

class obj_Zombie;
class LootBoxConfig;
class obj_ZombieSpawn : public GameObject
{
	DECLARE_CLASS(obj_ZombieSpawn, GameObject)

public:
	static obj_ZombieSpawn* GetClosestToPosition(r3dPoint3D pos);
	static r3dgameVector(obj_ZombieSpawn*) ZombieSpawners;	

	bool		initialSpawning;
	int		numSpawnedZombies;
	std::vector<obj_Zombie*> zombies;
	float		timeSinceLastSpawn;

	float		spawnRadius;
	size_t		maxZombieCount;
	float		zombieSpawnDelay; // Zombie spawn rate defined as how OFTEN zombies are spawned
	float		sleepersRate;

	std::vector<uint32_t> ZombieSpawnSelection; // which zombies should be spawned
	std::vector<uint32_t> ZombieBrainSelection; // which brains should be used

	float		minDetectionRadius;
	float		maxDetectionRadius;

	float		fastZombieChance;
	float		speedVariation;
	
	int		lootBoxID;
	const LootBoxConfig* lootBoxCfg;

	struct navPnt_s
	{
	  r3dPoint3D	pos;
	  int		marked;
	};
	std::vector<navPnt_s> navPoints;

	float		m_SprintersSpawnPerc;
	float		m_SprintPerc;
	float		m_SprintRadius;
	float		m_SprintMaxSpeed;
	float		m_SprintMaxTime;
	float		m_SprintSlope;
	float		m_SprintCooldownTime;
	float		m_SprintFromFarPerc;

	// Call For Help (CFH)
	uint32_t	m_CFHZombiesSpawned;
	uint32_t	m_CFHMaxZombies;
	uint32_t	m_CFHMinAdjZombies;
	float		m_CFHPerc;
	float		m_CFHSpawnPerc;
	r3dPoint3D	m_CFHExtents;

	void		GenerateNavPoints();
	int		GetFreeNavPointIdx(r3dPoint3D* out_pos, bool mark, float maxDist = -1, const r3dPoint3D* cur_pos = NULL);
	void		ReleaseNavPoint(int idx);

	bool		GetZombieSpawnPosition(r3dPoint3D* out_pos);
	bool		GetZombieRandomNavMeshPosition(r3dPoint3D* out_pos);
	
	bool		SpawnZombie();

	int		CheckForZombieAtPos(const r3dPoint3D& pos, float distSq);

public:
	obj_ZombieSpawn();
	~obj_ZombieSpawn();
	
	virtual BOOL	OnCreate();
	virtual BOOL	OnDestroy();
	virtual BOOL	Update();
	virtual	void	ReadSerializedData(pugi::xml_node& node);
	
	void		OnZombieKilled(const obj_Zombie* z);
};
