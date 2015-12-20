#include "r3dPCH.h"
#include "r3d.h"

#include "ZombieNavAgent.h"
#include "sobj_Zombie.h"
#include "../../GameEngine/ai/AI_Brain.h"
#include "../../GameEngine/ai/AutodeskNav/AutodeskNavMesh.h"
#include "../../GameEngine/gameobjects/PhysXWorld.h"
#include "../../GameEngine/gameobjects/PhysObj.h"

ZombieNavAgent::ZombieNavAgent()
	: m_pOwner( NULL )
{
}

ZombieNavAgent::~ZombieNavAgent()
{
}

ZombieNavAgent* CreateZombieNavAgent(obj_Zombie* zombie)
{
	r3d_assert(NULL != zombie);
	r3d_assert(gAutodeskNavMesh.GetWorld());

	ZombieNavAgent* a = new ZombieNavAgent();
	a->Init(gAutodeskNavMesh.GetWorld(), zombie->GetPosition(), (zombie->IsSuperZombie()) ? AutodeskNavAgent::PoiSuperZombie : AutodeskNavAgent::PoiZombie);
	a->m_pOwner = zombie;
#ifdef NEW_AI
	a->m_brain->m_TacticParams.m_AIHealth = &zombie->ZombieHealth;
#endif // NEW_AI
	gAutodeskNavMesh.AddNavAgent(a);

	return a;
}

void DeleteZombieNavAgent(ZombieNavAgent* a)
{
	gAutodeskNavMesh.DeleteNavAgent(a);
}
