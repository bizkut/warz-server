#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"

#include "ServerGameLogic.h"
#include "../../EclipseStudio/Sources/backend/WOBackendAPI.h"

#include "Async_ServerObjects.h"

extern	char*	g_ServerApiKey;

void CJobGetServerObjects::parseObjects(pugi::xml_node xmlObj)
{
	AllObjects.clear();

	// enter into items list
	xmlObj = xmlObj.first_child();
	while(!xmlObj.empty())
	{
		IServerObject prm;

		prm.ObjClassName   = xmlObj.attribute("ObjClassName").value();
		prm.ObjType        = xmlObj.attribute("ObjType").as_uint();
		prm.CreatePos.x    = xmlObj.attribute("px").as_float();
		prm.CreatePos.y    = xmlObj.attribute("py").as_float();
		prm.CreatePos.z    = xmlObj.attribute("pz").as_float();
		prm.CreateRot.x    = xmlObj.attribute("rx").as_float();
		prm.CreateRot.y    = xmlObj.attribute("ry").as_float();
		prm.CreateRot.z    = xmlObj.attribute("rz").as_float();

		// common part
		prm.ServerObjectID = xmlObj.attribute("ServerObjectID").as_uint();
		prm.CreateTime     = r3dGetTime();
		prm.ExpireTime     = r3dGetTime() + xmlObj.attribute("ExpireMins").as_float() * 60;
		prm.ItemID         = xmlObj.attribute("ItemID").as_uint();
		prm.CustomerID     = xmlObj.attribute("CustomerID").as_uint();
		prm.CharID         = xmlObj.attribute("CharID").as_uint();
		prm.Var1           = xmlObj.attribute("Var1").value();
		prm.Var2           = xmlObj.attribute("Var2").value();
		prm.Var3           = xmlObj.attribute("Var3").value();
		
		AllObjects.push_back(prm);
		xmlObj = xmlObj.next_sibling();
	}
	
	return;
}

int CJobGetServerObjects::Exec()
{
	CWOBackendReq req("api_SrvObjects.aspx");
	req.AddParam("skey1", g_ServerApiKey);
	
	req.AddParam("func",     "get");
	req.AddParam("GameSID",  gServerLogic.ginfo_.gameServerId);
	
	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobGetServerObjects failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	pugi::xml_document xmlFile;
	req.ParseXML(xmlFile);
	parseObjects(xmlFile.child("sobjects"));

	return 0;
}

void CJobGetServerObjects::OnSuccess()
{
	const float curTime = r3dGetTime();
	
	r3dOutToLog("Job %s: Creating %d objects\n", desc, AllObjects.size()); CLOG_INDENT;
	for(size_t i=0; i<AllObjects.size(); ++i)
	{
		const IServerObject& prm = AllObjects[i];
		
		if(curTime > prm.ExpireTime)
			continue;

		// create network object
		GameObject* obj = srv_CreateGameObject(prm.ObjClassName.c_str(), prm.ObjClassName.c_str(), prm.CreatePos);
		obj->SetNetworkID(gServerLogic.GetFreeNetId());
		obj->NetworkLocal = true;

		// set it parameters
		obj->SetRotationVector(prm.CreateRot);
		INetworkHelper* nh = obj->GetNetworkHelper();
		nh->srvObjParams_ = prm;
		nh->LoadServerObjectData();
		obj->OnCreate();
	}
}

CJobAddServerObject::CJobAddServerObject(GameObject* obj) : CAsyncApiJob()
{
	sprintf(desc, "CJobAddServerObject[%s] %p", obj->Class->Name.c_str(), this);
	
	r3d_assert(obj->GetNetworkID() > 0);
	GameObjID = obj->GetSafeID();

	// fill common server object parameters - class/position
	INetworkHelper* nh = obj->GetNetworkHelper();
	nh->SaveServerObjectData();
	prm              = nh->srvObjParams_;
	prm.CreatePos    = obj->GetPosition();
	prm.CreateRot    = obj->GetRotationVector();
	prm.ObjClassName = obj->Class->Name.c_str();
	prm.ObjType      = nh->GetServerObjectSerializationType();
	
	// set customerid from object creator, so it'll be put in player queue
	CustomerID       = prm.CustomerID;
}

int CJobAddServerObject::Exec()
{
	CWOBackendReq req("api_SrvObjects.aspx");
	req.AddParam("s_id",       prm.CustomerID);
	req.AddParam("CharID",     prm.CharID);
	req.AddParam("skey1",      g_ServerApiKey);
	
	req.AddParam("func",       "add");
	req.AddParam("GameSID",    gServerLogic.ginfo_.gameServerId);
        req.AddParam("ExpireMins", (int)(prm.ExpireTime - prm.CreateTime) / 60);
        req.AddParam("ObjType",    prm.ObjType);
        req.AddParam("ObjClass",   prm.ObjClassName.c_str());
        req.AddParam("ItemID",     prm.ItemID);
        req.AddParamF("px",        prm.CreatePos.x);
        req.AddParamF("py",        prm.CreatePos.y);
        req.AddParamF("pz",        prm.CreatePos.z);
        req.AddParamF("rx",        prm.CreateRot.x);
        req.AddParamF("ry",        prm.CreateRot.y);
        req.AddParamF("rz",        prm.CreateRot.z);
        req.AddParam("Var1",       prm.Var1.c_str());
        req.AddParam("Var2",       prm.Var2.c_str());
        req.AddParam("Var3",       prm.Var3.c_str());
	
	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobAddServerObject failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	// parse returned ObjectID
	int nargs = sscanf(req.bodyStr_, "%d", &out_ObjectID);
	if(nargs != 1) 
	{
		r3dOutToLog("!!!! CJobAddServerObject failed - bad answer %s\n", req.bodyStr_);
		return 1;
	}

	return 0;
}

void CJobAddServerObject::OnSuccess()
{
	GameObject* obj = GameWorld().GetObject(GameObjID);
	if(!obj)
	{
		r3dOutToLog("!!! server object %d was somehow destroyed\n", prm.ItemID);
		AsyncJobAddServerLogInfo(
			PKT_S2C_CheatWarning_s::CHEAT_Jobs,
			prm.CustomerID, 
			prm.CharID, 
			"CreateNoObj", 
			"object %d [%d] destroyed", 
			out_ObjectID, prm.ItemID);
		return;
	}
	INetworkHelper* nh = obj->GetNetworkHelper();

	r3dOutToLog("Job %s : Object %d created\n", desc, out_ObjectID);
	nh->srvObjParams_.ServerObjectID = out_ObjectID;
	return;
}

CJobUpdateServerObject::CJobUpdateServerObject(GameObject* obj) : CAsyncApiJob()
{
	sprintf(desc, "CJobUpdateServerObject[%s] %p", obj->Class->Name.c_str(), this);
	
	r3d_assert(obj->GetNetworkID() > 0);

	// fill common server object parameters - class/position
	INetworkHelper* nh = obj->GetNetworkHelper();
	nh->SaveServerObjectData();
	prm              = nh->srvObjParams_;

	// set customerid from object creator, so it'll be put in player queue
	CustomerID       = prm.CustomerID;
}

int CJobUpdateServerObject::Exec()
{
	CWOBackendReq req("api_SrvObjects.aspx");
	req.AddParam("skey1",      g_ServerApiKey);
	
	req.AddParam("func",       "update");
	req.AddParam("ObjectID",   prm.ServerObjectID);
        req.AddParam("Var1",       prm.Var1.c_str());
        req.AddParam("Var2",       prm.Var2.c_str());
        req.AddParam("Var3",       prm.Var3.c_str());
        req.AddParam("ExpireMins", (int)(prm.ExpireTime - prm.CreateTime) / 60);
	
	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobUpdateServerObject failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	return 0;
}

CJobDeleteServerObject::CJobDeleteServerObject(GameObject* obj) : CAsyncApiJob()
{
	sprintf(desc, "CJobDeleteServerObject %p", this);

	r3d_assert(obj->GetNetworkID() > 0);

	INetworkHelper* nh = obj->GetNetworkHelper();
	ServerObjectID = nh->srvObjParams_.ServerObjectID;
	// set customerid from object creator, so it'll be put in player queue
	CustomerID     = nh->srvObjParams_.CustomerID;

	// object must have ServerObjectID at this point, need to pend delete at higher game level logic
	r3d_assert(ServerObjectID > 0);
}

int CJobDeleteServerObject::Exec()
{
	CWOBackendReq req("api_SrvObjects.aspx");
	req.AddParam("skey1",      g_ServerApiKey);
	
	req.AddParam("func",       "del");
	req.AddParam("ObjectID",   ServerObjectID);

	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobDeleteServerObject failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	return 0;
}

int CJobHibernate::ApiStartHibernate()
{
	CWOBackendReq req("api_SrvObjects.aspx");
	req.AddParam("skey1",      g_ServerApiKey);
	
	req.AddParam("func",       "hstart");
	req.AddParam("GameSID",    gServerLogic.ginfo_.gameServerId);

	// issue
	if(!req.Issue())
	{
		r3dOutToLog("!!!! CJobStartHibernate failed, code: %d\n", req.resultCode_);
		return req.resultCode_;
	}
	
	return 0;
}

CJobHibernate::~CJobHibernate()
{
	for(size_t i=0; i<jobs.size(); i++)
		delete jobs[i];
	jobs.clear();
}

int CJobHibernate::Exec()
{
	// start hibernate, if fails, do not do anything
	int apiCode = ApiStartHibernate();
	if(apiCode != 0)
	{
		return apiCode;
	}

	// hibernate all objects, ignoring errors
	for(size_t i=0; i<jobs.size(); i++)
	{
		r3dOutToLog("CJobHibernate %d of %d - %s\n", i, jobs.size(), jobs[i]->desc);
		if(gServerLogic.curPeersConnected > 0)
		{
			// some player connected, abort
			return 2;
		}

		jobs[i]->Exec();
	}
	
	return 0;
}

void CJobHibernate::OnFailure()
{
	r3dOutToLog("!!!! Hibernate failed: %d\n", ResultCode);
	gServerLogic.hibernateStarted_   = false;
	gServerLogic.secsWithoutPlayers_ = 0; // reset timer so game will try to hibernate after some time again
}

void CJobHibernate::OnSuccess()
{
	r3dOutToLog("Hibernate success\n");
	gServerLogic.gameFinished_ = true;
}
