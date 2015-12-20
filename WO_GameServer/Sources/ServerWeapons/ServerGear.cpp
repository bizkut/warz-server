#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "ServerGear.h"

Gear::Gear(const GearConfig* conf) : m_pConfig(conf)
{
	Reset();
}

Gear::~Gear()
{
}

void Gear::Reset()
{
}
