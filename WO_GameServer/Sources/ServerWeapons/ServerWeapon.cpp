#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "ServerWeapon.h"
#include "../ObjectsCode/obj_ServerPlayer.h"
#include "ServerGameLogic.h"
#include "../EclipseStudio/Sources/ObjectsCode/weapons/WeaponArmory.h"

ServerWeapon::ServerWeapon(const WeaponConfig* conf, obj_ServerPlayer* owner, int backpackIdx, const wiWeaponAttachment& attm)
{
	m_pConfig     = conf;
	m_Owner       = owner;
	m_BackpackIdx = backpackIdx;

	memset(m_Attachments, 0, sizeof(m_Attachments));
	memset(m_WeaponAttachmentStats, 0, sizeof(m_WeaponAttachmentStats));
	
	// init default attachments
	for(int i=0; i<WPN_ATTM_MAX; i++)
	{
		uint32_t id = attm.attachments[i] ? attm.attachments[i] : m_pConfig->FPSDefaultID[i];
		const WeaponAttachmentConfig* wac = g_pWeaponArmory->getAttachmentConfig(id);
		
		// verify attachment is legit
		if(!m_pConfig->isAttachmentValid(wac))
			wac = NULL;
		
		m_Attachments[i] = wac;
	}
	recalcAttachmentsStats();

	wiInventoryItem& wi = getPlayerItem();
	const WeaponAttachmentConfig* clipCfg = getClipConfig();
	// check if we need to modify starting ammo. (SERVER CODE SYNC POINT)
	if(wi.Var1 < 0 && clipCfg) 
	{
		wi.Var1 = clipCfg->m_Clipsize;
	}
	// set itemid of clip (SERVER CODE SYNC POINT)
	if(wi.Var2 < 0 && clipCfg)
	{
		wi.Var2 = clipCfg->m_itemID;
	}
	// initialize durability (SERVER_SYNC_POINT DUR)
	if(wi.Var3 < 0)
	{
		wi.Var3 = wiInventoryItem::MAX_DURABILITY;
	}
}

ServerWeapon::~ServerWeapon()
{
}

const WeaponAttachmentConfig* ServerWeapon::getClipConfig()
{
	// (SERVER CODE SYNC POINT)
	return m_Attachments[WPN_ATTM_CLIP];
}

wiInventoryItem& ServerWeapon::getPlayerItem()
{
	if(m_pConfig->m_itemID == WeaponConfig::ITEMID_UnarmedMelee) {
		static wiInventoryItem tempMeleeItem;
		return tempMeleeItem;
	}

	wiInventoryItem& wi = m_Owner->loadout_->Items[m_BackpackIdx];
	r3d_assert(wi.itemID == m_pConfig->m_itemID);	// make sure it wasn't changed
	return wi;
}

void ServerWeapon::recalcAttachmentsStats()
{
	memset(m_WeaponAttachmentStats, 0, sizeof(m_WeaponAttachmentStats));

	// recalculate stats
	for(int i=0; i<WPN_ATTM_MAX; ++i)
	{
		if(m_Attachments[i]!=NULL)
		{
			m_WeaponAttachmentStats[WPN_ATTM_STAT_DAMAGE]   += m_Attachments[i]->m_Damage;
			m_WeaponAttachmentStats[WPN_ATTM_STAT_RANGE]    += m_Attachments[i]->m_Range;
			m_WeaponAttachmentStats[WPN_ATTM_STAT_FIRERATE] += m_Attachments[i]->m_Firerate;
			m_WeaponAttachmentStats[WPN_ATTM_STAT_RECOIL]   += m_Attachments[i]->m_Recoil;
			m_WeaponAttachmentStats[WPN_ATTM_STAT_SPREAD]   += m_Attachments[i]->m_Spread;
		}
	}

	// convert stats to percents (from -100..100 range to -1..1)
	m_WeaponAttachmentStats[WPN_ATTM_STAT_DAMAGE] *= 0.01f; 
	m_WeaponAttachmentStats[WPN_ATTM_STAT_RANGE] *= 0.01f; 
	m_WeaponAttachmentStats[WPN_ATTM_STAT_FIRERATE] *= 0.01f; 
	m_WeaponAttachmentStats[WPN_ATTM_STAT_RECOIL] *= 0.01f; 
	m_WeaponAttachmentStats[WPN_ATTM_STAT_SPREAD] *= 0.01f;
}

// dist = distance to target
float ServerWeapon::calcDamage(float dist) const
{
	const obj_ServerPlayer* plr = (obj_ServerPlayer*)m_Owner;
	if(plr == NULL) 
	{
		r3dOutToLog("!!! weapon owner is null\n");
		return 0;
	}

	float range = m_pConfig->m_AmmoDecay;
	if(range <= 0)
	{
		gServerLogic.LogInfo(plr->peerId_, "!!! m_AmmoDecay is <= 0", "weapon ID: %d", m_pConfig->m_itemID);
		range = 1;
	}
	range = range * (1.0f+m_WeaponAttachmentStats[WPN_ATTM_STAT_RANGE]);

	float ammoDmg = m_pConfig->m_AmmoDamage;
	ammoDmg = ammoDmg * (1.0f + m_WeaponAttachmentStats[WPN_ATTM_STAT_DAMAGE]);

	// damage adjustment by durability
	wiInventoryItem& wi = ((ServerWeapon*)this)->getPlayerItem(); // override const here
	if(wi.Var3 >= 0)
	{
		int dur = wi.Var3 / 100;
		if(dur >= 40)      ammoDmg *= 1.0f;
		else if(dur >= 15) ammoDmg *= 0.85f;
		else if(dur >=  1) ammoDmg *= 0.7f;
		else               ammoDmg  = 0.0f;
	}

	if(m_pConfig->category == storecat_MELEE)
		return ammoDmg;

	// new formula
	// if < 100% range = damage 100%
	// if >= range && < (range*2) - damage goes from 100 to 50%
	// if >= range*2 && < (range*3) - damage goes from 50 to 0%
	// else - damage 0% (to fight with shotguns with cheat firing across the map)
	if(dist < range)
		return ammoDmg;
	else if(dist >= range && dist < (range*2))
	{
		float mul = 1.0f-(((dist-range)/range)*0.5f); // from 1.0f to 0.5f
		return ammoDmg*mul;
	}
	else if(dist >= (range*2) && dist < (range*3))
	{
		float mul = (1.0f-((dist-(range*2))/range))*0.5f; // from .5f to 0.0f
		return ammoDmg*mul;
	}		
	else
		return 0;
}
