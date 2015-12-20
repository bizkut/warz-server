#pragma once

#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponConfig.h"

class Ammo;
class GameObject;
class obj_ServerPlayer;

class ServerWeapon
{
public:
	enum WeaponAttachmentStatsEnum
	{
		WPN_ATTM_STAT_DAMAGE=0, // %
		WPN_ATTM_STAT_RANGE, // %
		WPN_ATTM_STAT_FIRERATE, // %
		WPN_ATTM_STAT_RECOIL, // %
		WPN_ATTM_STAT_SPREAD, // %

		WPN_ATTM_STAT_MAX
	};
	float		m_WeaponAttachmentStats[WPN_ATTM_STAT_MAX];
	const WeaponAttachmentConfig* m_Attachments[WPN_ATTM_MAX]; 
	void		recalcAttachmentsStats();

	const WeaponConfig* m_pConfig;
	
	obj_ServerPlayer* m_Owner;
	int		m_BackpackIdx;

	const WeaponAttachmentConfig* getClipConfig();
	wiInventoryItem& getPlayerItem();
	
public:
	ServerWeapon(const WeaponConfig* conf, obj_ServerPlayer* owner, int backpackIdx, const wiWeaponAttachment& attm);
	~ServerWeapon();
	
	float		calcDamage(float dist) const; // return 0 if weapon isn't immediate

	STORE_CATEGORIES getCategory() const { return m_pConfig->category; }
	const WeaponConfig* getConfig() const { return m_pConfig; }
};
