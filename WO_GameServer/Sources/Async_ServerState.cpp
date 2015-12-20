#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"

#include "ServerGameLogic.h"
#include "../../EclipseStudio/Sources/backend/WOBackendAPI.h"

#include "Async_ServerState.h"
#include "NetworkHelper.h"
#include "ObjectsCode/obj_ServerItemSpawnPoint.h"

extern	char*	g_ServerApiKey;

int CJobGetSavedServerState::Exec()
{
	CWOBackendReq req("api_SrvSavedState.aspx");
	req.AddParam("skey1",      g_ServerApiKey);
	
	req.AddParam("func",       "get");
	req.AddParam("GameSID",    gServerLogic.ginfo_.gameServerId);

	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobGetSavedServerState failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	pugi::xml_parse_result parseResult = state.load_buffer(req.bodyStr_, req.bodyLen_);
	if(!parseResult) 
	{
		r3dOutToLog("!!!! CJobGetSavedServerState - Failed to parse XML, error: %s\n", parseResult.description());
		return 6;
	}

	return 0;
}

void CJobGetSavedServerState::LoadItemSpawns(const pugi::xml_node& xmlNode)
{
	int numSpawns = 0;
	
	pugi::xml_node xmlObj = xmlNode.first_child();
	while(!xmlObj.empty())
	{
		uint32_t hash = xmlObj.attribute("hash").as_uint();
		GameObject* obj = GameWorld().GetObjectByHash(hash);
		if(!obj)
		{
			r3dOutToLog("there is no object with hash %d\n", hash);
			xmlObj = xmlObj.next_sibling();
			continue;
		}
		if(obj->Class->Name != "obj_ItemSpawnPoint" && obj->Class->Name != "obj_HackerItemSpawnPoint")
		{
			r3dOutToLog("object with hash %d %s is not a spawn point\n", hash, obj->Class->Name.c_str());
			xmlObj = xmlObj.next_sibling();
			continue;
		}
		
		//r3dOutToLog("ItemSpawnPoint %d restored\n", hash); CLOG_INDENT;
		obj_ServerItemSpawnPoint* spawn = (obj_ServerItemSpawnPoint*)obj;
		spawn->LoadServerObjectData(xmlObj);
		numSpawns++;
		
		xmlObj = xmlObj.next_sibling();
	}

	r3dOutToLog("%d ItemSpawnPoints restored\n", numSpawns);
}

void CJobGetSavedServerState::OnSuccess()
{
	pugi::xml_node xmlState = state.first_child();
	
	r3dOutToLog("CJobGetSavedServerState, restoring state\n"); CLOG_INDENT;
	LoadItemSpawns(xmlState.child("ItemSpawns"));
}

void CJobSetSavedServerState::SaveItemSpawns(pugi::xml_node& xmlNode)
{
	for(size_t i=0; i<obj_ServerItemSpawnPoint::m_allSpawns.size(); i++)
	{
		obj_ServerItemSpawnPoint* spawn = obj_ServerItemSpawnPoint::m_allSpawns[i];

		pugi::xml_node xmlObj = xmlNode.append_child();
		xmlObj.set_name("i");
		xmlObj.append_attribute("hash") = spawn->GetHashID();
		spawn->SaveServerObjectData(xmlObj);
	}
}

CJobSetSavedServerState::CJobSetSavedServerState()
{
	sprintf(desc, "CJobSetSavedServerState %p", this);
	
	pugi::xml_document xmlFile;
	pugi::xml_node xmlState = xmlFile.append_child();
	xmlState.set_name("ServerState");
	
	pugi::xml_node xmlSpawns = xmlState.append_child();
	xmlSpawns.set_name("ItemSpawns");
	SaveItemSpawns(xmlSpawns);

	xml_string_writer xmlBuf(240000);
	xmlFile.save(xmlBuf, "", pugi::format_raw);
	state = xmlBuf.out_;
}

int CJobSetSavedServerState::Exec()
{
	CWOBackendReq req("api_SrvSavedState.aspx");
	req.AddParam("skey1",      g_ServerApiKey);
	
	req.AddParam("func",       "set");
	req.AddParam("GameSID",    gServerLogic.ginfo_.gameServerId);
	req.AddParam("state",      state.c_str());

	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobSetSavedServerState failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}

	return 0;
}
