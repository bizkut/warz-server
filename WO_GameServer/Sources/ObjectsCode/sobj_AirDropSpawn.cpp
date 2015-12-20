#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "XMLHelpers.h"
#include "ServerGameLogic.h"

#include "sobj_AirDropSpawn.h"

IMPLEMENT_CLASS(obj_AirDropSpawn, "obj_AirDropSpawn", "Object");
AUTOREGISTER_CLASS(obj_AirDropSpawn);

obj_AirDropSpawn::obj_AirDropSpawn()
	: spawnRadius(20)
{
	serializeFile = SF_ServerData;
	m_LootBoxID1 = 0;
	m_LootBoxID2 = 0;
	m_LootBoxID3 = 0;
	m_LootBoxID4 = 0;
	m_LootBoxID5 = 0;
	m_LootBoxID6 = 0;
	m_DefaultItems = 1;
}

obj_AirDropSpawn::~obj_AirDropSpawn()
{
}

BOOL obj_AirDropSpawn::OnCreate()
{

	ServerGameLogic::AirDropPositions AirDrop;
	AirDrop.m_radius	= spawnRadius;
	AirDrop.m_location	= GetPosition();
	AirDrop.m_DefaultItems = m_DefaultItems;
	AirDrop.m_LootBoxID1 = m_LootBoxID1;
	AirDrop.m_LootBoxID2 = m_LootBoxID2;
	AirDrop.m_LootBoxID3 = m_LootBoxID3;
	AirDrop.m_LootBoxID4 = m_LootBoxID4;
	AirDrop.m_LootBoxID5 = m_LootBoxID5;
	AirDrop.m_LootBoxID6 = m_LootBoxID6;
	gServerLogic.SetAirDrop( AirDrop );

	return parent::OnCreate();
}

BOOL obj_AirDropSpawn::OnDestroy()
{
	return parent::OnDestroy();
}

BOOL obj_AirDropSpawn::Update()
{
	return TRUE;
}

// copy from client version
void obj_AirDropSpawn::ReadSerializedData(pugi::xml_node& node)
{
	parent::ReadSerializedData(node);
	pugi::xml_node AirDropSpawnNode = node.child("LootID_parameters");
	GetXMLVal("spawn_radius", AirDropSpawnNode, &spawnRadius);
	GetXMLVal("m_DefaultItems", AirDropSpawnNode, &m_DefaultItems);
	GetXMLVal("m_LootBoxID1", AirDropSpawnNode, &m_LootBoxID1);
	GetXMLVal("m_LootBoxID2", AirDropSpawnNode, &m_LootBoxID2);
	GetXMLVal("m_LootBoxID3", AirDropSpawnNode, &m_LootBoxID3);
	GetXMLVal("m_LootBoxID4", AirDropSpawnNode, &m_LootBoxID4);
	GetXMLVal("m_LootBoxID5", AirDropSpawnNode, &m_LootBoxID5);
	GetXMLVal("m_LootBoxID6", AirDropSpawnNode, &m_LootBoxID6);
}
