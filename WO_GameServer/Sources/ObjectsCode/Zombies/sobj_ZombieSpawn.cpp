#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "XMLHelpers.h"
#include "ServerGameLogic.h"
#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponArmory.h"

#include "sobj_ZombieSpawn.h"
#include "sobj_Zombie.h"

IMPLEMENT_CLASS(obj_ZombieSpawn, "obj_ZombieSpawn", "Object");
AUTOREGISTER_CLASS(obj_ZombieSpawn);

r3dgameVector(obj_ZombieSpawn*) obj_ZombieSpawn::ZombieSpawners;

obj_ZombieSpawn* obj_ZombieSpawn::GetClosestToPosition(r3dPoint3D pos)
{
	for (r3dgameVector(obj_ZombieSpawn*)::iterator it = ZombieSpawners.begin(); it != ZombieSpawners.end(); ++it)
	{
		if (!*it)
			continue;

		return *it;
	}

	return NULL;
}

obj_ZombieSpawn::obj_ZombieSpawn()
	: initialSpawning(true)
	, numSpawnedZombies(0)
	, timeSinceLastSpawn(0)
	, spawnRadius(20)
	, maxZombieCount(5)
	, zombieSpawnDelay(15)
	, sleepersRate(10)
	, minDetectionRadius(0)
	, maxDetectionRadius(5)
	, fastZombieChance(50.0)	// percent in 0-100, they will be convered to 0-1 at ReadSerializedData
	, speedVariation(10.0)
	, lootBoxID(0)
	, lootBoxCfg(NULL)
	, m_SprintersSpawnPerc(0.0f)
	, m_SprintPerc(0.0f)
	, m_SprintRadius(0.0f)
	, m_SprintMaxSpeed(0.0f)
	, m_SprintMaxTime(0.0f)
	, m_SprintSlope(0.0f)
	, m_SprintCooldownTime(0.0f)
	, m_SprintFromFarPerc(0.0f)
	, m_CFHZombiesSpawned(0)
	, m_CFHMaxZombies(0)
	, m_CFHMinAdjZombies(10)
	, m_CFHPerc(0.0f)
	, m_CFHSpawnPerc(0.0f)
	, m_CFHExtents( 5.0f, 2.0f, 5.0f )
{
	serializeFile = SF_ServerData;

	ZombieSpawners.push_back(this);
}

obj_ZombieSpawn::~obj_ZombieSpawn()
{
	for (r3dgameVector(obj_ZombieSpawn*)::iterator it = ZombieSpawners.begin(); it < ZombieSpawners.end(); ++it)
	{
		if (*it == this)
		{
			ZombieSpawners.erase(it);
			break;
		}
	}
}

BOOL obj_ZombieSpawn::OnCreate()
{
	lootBoxCfg = g_pWeaponArmory->getLootBoxConfig(lootBoxID);

	/*if((GetPosition() - r3dPoint3D(7048, 41, 6931)).Length() > 5)
	{
		r3dOutToLog("obj_ZombieSpawn skipped\n");
		return FALSE;
	}*/

	r3dOutToLog("obj_ZombieSpawn %p, %d zombies in %.0f meters, delay %.0f, r:%.1f-%.1f\n", 
		this,
		maxZombieCount, spawnRadius, zombieSpawnDelay, 
		minDetectionRadius, maxDetectionRadius);
	CLOG_INDENT;

	extern float _zai_MaxSpawnDelay;
	if(zombieSpawnDelay > _zai_MaxSpawnDelay)
		zombieSpawnDelay = _zai_MaxSpawnDelay;
		
	GenerateNavPoints();

	return parent::OnCreate();
}

BOOL obj_ZombieSpawn::OnDestroy()
{
	return parent::OnDestroy();
}

BOOL obj_ZombieSpawn::Update()
{
	if(gServerLogic.net_mapLoaded_LastNetID == 0) // do not spawn zombies until server has fully loaded
		return TRUE;

	parent::Update();
	
	// spawn all zombies at server start
	if(initialSpawning)
	{
		bool spawned = false;
		for(int i=0; i<10; i++)
		{
			if(SpawnZombie())
			{
				timeSinceLastSpawn = r3dGetTime();
				spawned = true;
				break;
			}
		}
		
		// if we have enough zombies or spawn failed then stop.
		if(zombies.size() >= maxZombieCount || !spawned)
		{
			r3dOutToLog("obj_ZombieSpawn %p initialSpawn %d/%d zombies\n", this, zombies.size(), maxZombieCount);
			initialSpawning = false;
		}
		return TRUE;
	}
	
	// check if need to spawn zombie
	if(r3dGetTime() < timeSinceLastSpawn + zombieSpawnDelay)
		return TRUE;
	timeSinceLastSpawn = r3dGetTime();

	// if we have enough zombies	
	if(zombies.size() < maxZombieCount)
	{
		SpawnZombie();
	}
	
	return TRUE;
}

bool obj_ZombieSpawn::SpawnZombie()
{
	r3dPoint3D pos;
	if(!GetZombieSpawnPosition(&pos))
	{
		return false;
	}
			
	char name[28];
	sprintf(name, "Zombie_%d_%p", numSpawnedZombies++, this);

	obj_Zombie* z = (obj_Zombie*)srv_CreateGameObject("obj_Zombie", name, pos);
	z->SetNetworkID(gServerLogic.GetFreeNetId());
	z->NetworkLocal = true;
	z->spawnObject  = this;
	z->DetectRadius = u_GetRandom(minDetectionRadius, maxDetectionRadius);
	z->SetRotationVector(r3dVector(u_GetRandom(0, 360), 0, 0));
	z->CanCallForHelp = ( m_CFHZombiesSpawned < m_CFHMaxZombies && m_CFHSpawnPerc >= u_GetRandom( 0.0f, 1.0f ) ) ? true : false;
	if( z->CanCallForHelp )
		++m_CFHZombiesSpawned;

	zombies.push_back(z);
	return true;
}

int obj_ZombieSpawn::CheckForZombieAtPos(const r3dPoint3D& pos, float distSq)
{
	int num = 0;
	for(size_t i=0, e=zombies.size(); i<e; i++)
	{
		if((zombies[i]->GetPosition() - pos).LengthSq() < distSq)
			num++;
	}
	
	return num;
}

void obj_ZombieSpawn::GenerateNavPoints()
{
	r3d_assert(navPoints.size() == 0);
	const float NAV_GRID_SIZE = 3.0f; // generate every 3m
	
	int cells = (int)((spawnRadius + spawnRadius) / NAV_GRID_SIZE) + 1;
	navPoints.reserve(cells * cells);
	
	float x1 = GetPosition().x - spawnRadius + NAV_GRID_SIZE / 2;
	float x2 = GetPosition().x + spawnRadius;
	float z1 = GetPosition().z - spawnRadius + NAV_GRID_SIZE / 2;
	float z2 = GetPosition().z + spawnRadius;
	
	r3dPoint3D pos(x1, GetPosition().y, z1);
	for(pos.x = x1; pos.x < x2; pos.x += NAV_GRID_SIZE)
	{
		for(pos.z = z1; pos.z < z2; pos.z += NAV_GRID_SIZE)
		{
			r3dPoint3D npos = pos;
			if(! gAutodeskNavMesh.AdjustNavPointHeight(npos, 20.0f))
				continue;

			// check that we are not trying to spawn zombie on top of the building (sergey's request)
			// NOT NEEDED now, as zombies will spawn only on navmesh
			/*if(Terrain)
			{
				float terrHeight = Terrain->GetHeight(npos);
				if((npos.y - terrHeight) > 2.0f) {
					continue;
				}
			}*/
			
			navPnt_s npnt;
			npnt.pos    = npos;
			npnt.marked = 0;
			
			navPoints.push_back(npnt);
		}
	}
	
	// shuffle positions around
	if(navPoints.size() > 0)
		std::random_shuffle(navPoints.begin(), navPoints.end());
	
	r3dOutToLog("%d/%d valid navpoints for %d zombies at %f %f %f\n", navPoints.size(), cells * cells, maxZombieCount, GetPosition().x, GetPosition().y, GetPosition().z);
	if(navPoints.size() == 0)
		r3dOutToLog("!!! no valid navpoints at %f %f %f\n", GetPosition().x, GetPosition().y, GetPosition().z);
}

int obj_ZombieSpawn::GetFreeNavPointIdx(r3dPoint3D* out_pos, bool mark, float maxDist, const r3dPoint3D* cur_pos)
{
	if(navPoints.size() == 0)
		return -1;
		
	float maxDistSq = maxDist * maxDist;

	// start from random position in array and scan for first free point
        int found = 0;
	int size  = (int)navPoints.size();
	int start = u_random(size);
	int idx   = start;
	do
	{
		if(maxDist > 0)
		{
			if((navPoints[idx].pos - *cur_pos).LengthSq() > maxDistSq)
			{
				idx = (idx + 1) % size;
				continue;
			}
		}
		
		if(navPoints[idx].marked == 0)
		{
			if(mark)
			{
				navPoints[idx].marked = mark;
				//r3dOutToLog("SpawnPoint snav%p_%d marked\n", this, idx);
			}
			*out_pos = navPoints[idx].pos;
			return idx;
		}
		else
		{
			found++;
		}

		idx = (idx + 1) % size;

	} while(idx != start);
	
	//usually that happens whem zombies was attacking someone and went too far from available spawn points
	//r3dOutToLog("SpawnPoint%p had no free navpoints (%d/%d) at %f %f %f with radius %f\n", this, found, navPoints.size(), GetPosition().x, GetPosition().y, GetPosition().z, maxDist);
	
	return -1;
}

void obj_ZombieSpawn::ReleaseNavPoint(int idx)
{
	r3d_assert(idx < (int)navPoints.size());
	if(navPoints[idx].marked == 0)
		r3dOutToLog("!!! SpawnPoint releasing not used snav%p_%d\n", this, idx);

	//r3dOutToLog("SpawnPoint snav%p_%d released\n", this, idx);
	navPoints[idx].marked = 0;
}

bool obj_ZombieSpawn::GetZombieSpawnPosition(r3dPoint3D* out_pos)
{
	if(!GetFreeNavPointIdx(out_pos, false))
		return false;

	int MAX_SPAWN_TRIES = 5;
	for(int i=0; i<MAX_SPAWN_TRIES; i++)
	{
		// try to not have players around us
		extern float _zai_PlayerSpawnProtect;
		if(gServerLogic.CheckForPlayersAround(*out_pos, _zai_PlayerSpawnProtect))
			continue;
		
		// try not to have zombies around us
		if(CheckForZombieAtPos(*out_pos, 1.0f * 1.0f) > 1)
			continue;
		
		// all ok
		return true;
	}

	return false;
}

/*
bool obj_ZombieSpawn::GetZombieRandomNavMeshPosition(r3dPoint3D* out_pos)
{
	r3dPoint3D pos = GetPosition();
	
	// try few times to spawn zombie
	int MAX_SPAWN_NAVMESH_TRIES = 20;
	for(int i=0; i<MAX_SPAWN_NAVMESH_TRIES; i++)
	{
		// Generate random pos within spawn radius
		float rX = rand() / static_cast<float>(RAND_MAX);
		float rZ = rand() / static_cast<float>(RAND_MAX);

		r3dPoint3D offset(rX, 0, rZ);
		offset = (offset - 0.5f) * 2;
		offset.y = 0;
		offset.Normalize();
		offset *= rand() / static_cast<float>(RAND_MAX) * spawnRadius;
		pos += offset;

		// check that it's valid in navmesh
		if(gAutodeskNavMesh.AdjustNavPointHeight(pos, 20.0f))
		{
			*out_pos = pos;
			return true;
		}
	}
#ifdef _DEBUG
	r3dOutToLog("!!! obj_ZombieSpawn_NavFail at %f %f %f.\n", GetPosition().x, GetPosition().y, GetPosition().z);
#endif
	
	return false;
}
*/

// copy from client version
void obj_ZombieSpawn::ReadSerializedData(pugi::xml_node& node)
{
	parent::ReadSerializedData(node);
	pugi::xml_node zombieSpawnNode = node.child("spawn_parameters");
	GetXMLVal("spawn_radius", zombieSpawnNode, &spawnRadius);
	int mzc = maxZombieCount;
	GetXMLVal("max_zombies", zombieSpawnNode, &mzc);
	maxZombieCount = mzc;
	GetXMLVal("sleepers_rate", zombieSpawnNode, &sleepersRate);
	GetXMLVal("zombies_spawn_delay", zombieSpawnNode, &zombieSpawnDelay);
	GetXMLVal("min_detection_radius", zombieSpawnNode, &minDetectionRadius);
	GetXMLVal("max_detection_radius", zombieSpawnNode, &maxDetectionRadius);
	GetXMLVal("lootBoxID", zombieSpawnNode, &lootBoxID);
	GetXMLVal("fastZombieChance", zombieSpawnNode, &fastZombieChance);
	GetXMLVal("speedVariation", zombieSpawnNode, &speedVariation);
	GetXMLVal("SprintersSpawnPerc", zombieSpawnNode, &m_SprintersSpawnPerc);
	GetXMLVal("SprintPerc", zombieSpawnNode, &m_SprintPerc);
	GetXMLVal("SprintRadius", zombieSpawnNode, &m_SprintRadius);
	GetXMLVal("SprintMaxSpeed", zombieSpawnNode, &m_SprintMaxSpeed);
	GetXMLVal("SprintMaxTime", zombieSpawnNode, &m_SprintMaxTime);
	GetXMLVal("SprintSlope", zombieSpawnNode, &m_SprintSlope);
	GetXMLVal("SprintCooldownTime", zombieSpawnNode, &m_SprintCooldownTime);
	GetXMLVal("SprintFromFarPerc", zombieSpawnNode, &m_SprintFromFarPerc);
	GetXMLVal("CallForHelpMaxZombies", zombieSpawnNode, &m_CFHMaxZombies);
	GetXMLVal("CallForHelpMinAdjZombies", zombieSpawnNode, &m_CFHMinAdjZombies);
	GetXMLVal("CallForHelpPerc", zombieSpawnNode, &m_CFHPerc);
	GetXMLVal("CallForHelpSpawnPerc", zombieSpawnNode, &m_CFHSpawnPerc);
	GetXMLVal("CallForHelpExtents", zombieSpawnNode, &m_CFHExtents);
	
	uint32_t numZombies = zombieSpawnNode.attribute("numZombieSelected").as_uint();
	for(uint32_t i=0; i<numZombies; ++i)
	{
		char tempStr[32];
		sprintf(tempStr, "z%d", i);
		pugi::xml_node zNode = zombieSpawnNode.child(tempStr);
		r3d_assert(!zNode.empty());
		ZombieSpawnSelection.push_back(zNode.attribute("id").as_uint());
	}

	uint32_t numBrains = zombieSpawnNode.attribute("numBrainsSelected").as_uint();
	for(uint32_t i=0; i<numBrains; ++i)
	{
		char tempStr[32];
		sprintf(tempStr, "b%d", i);
		pugi::xml_node zNode = zombieSpawnNode.child(tempStr);
		r3d_assert(!zNode.empty());
		ZombieBrainSelection.push_back(zNode.attribute("id").as_uint());
	}

	// client have it in 0-100 range
	sleepersRate     /= 100.0f;
	speedVariation   /= 100.0f;
	fastZombieChance /= 100.0f;
}

void obj_ZombieSpawn::OnZombieKilled(const obj_Zombie* z)
{
	for(std::vector<obj_Zombie*>::iterator it = zombies.begin(); it != zombies.end(); ++it)
	{
		if(*it == z)
		{
			zombies.erase(it);
			return;
		}
	}

	--m_CFHZombiesSpawned;
	
	r3dError("zombie %p isn't found on spawn", z);
}
