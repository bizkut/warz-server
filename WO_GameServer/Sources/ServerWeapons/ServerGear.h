#pragma once

#include "../EclipseStudio/Sources/ObjectsCode/weapons/GearConfig.h"

class Gear
{
	friend class WeaponArmory;
public:
	Gear(const GearConfig* conf);
	~Gear();

	void Reset();

	const GearConfig* getConfig() const { return m_pConfig; }
	STORE_CATEGORIES getCategory() const { return m_pConfig->category; }

private:
	const GearConfig* m_pConfig;
};
