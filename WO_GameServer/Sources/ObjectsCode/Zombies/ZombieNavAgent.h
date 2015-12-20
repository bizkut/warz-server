#pragma once

#include "../../GameEngine/ai/AutodeskNav/AutodeskNavAgent.h"

class obj_Zombie;

class ZombieNavAgent : public AutodeskNavAgent
{
public:
	obj_Zombie*	m_pOwner;

public:
	friend ZombieNavAgent* CreateZombieNavAgent(obj_Zombie* zombie);
	friend void DeleteZombieNavAgent(ZombieNavAgent* a);
	
protected:
	ZombieNavAgent();
	~ZombieNavAgent();
}; 

//////////////////////////////////////////////////////////////////////////

ZombieNavAgent* CreateZombieNavAgent(obj_Zombie* zombie);
void DeleteZombieNavAgent(ZombieNavAgent* a);