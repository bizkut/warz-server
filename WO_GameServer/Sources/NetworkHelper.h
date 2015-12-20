#pragma once

#include "GameCommon.h"
#include "ServerGameLogic.h"

// persistent parameters for server object
class IServerObject
{
  public:
	// helpers
	class CSrvObjXmlReader
	{
	public:
		pugi::xml_document	xmlFile;
		pugi::xml_node		xmlObj;

		CSrvObjXmlReader(const std::string& in_str);
	};

	class CSrvObjXmlWriter
	{
	public:
		pugi::xml_document	xmlFile;
		pugi::xml_node		xmlObj;

		CSrvObjXmlWriter();
		void		save(std::string& out_str);
	};

  public:
	// create part, filled in CJobAddServerObject::CJobAddServerObject
	std::string	ObjClassName;
	r3dPoint3D	CreatePos;
	r3dPoint3D	CreateRot;
	int		ObjType;	// 0- permanent, 1- temporary (hibernated)

	// common part
	DWORD		ServerObjectID;
	float		CreateTime;
	float		ExpireTime;
	DWORD		ItemID;
	DWORD		CustomerID;
	DWORD		CharID;
	std::string	Var1;
	std::string	Var2;
	std::string	Var3;
	
	bool		IsDirty;
  
	IServerObject()
	{
		CreatePos      = r3dPoint3D(0, 0, 0);
		CreateRot      = r3dPoint3D(0, 0, 0);

		ServerObjectID = 0;
		ObjType        = 0;
		CreateTime     = r3dGetTime();
		ExpireTime     = r3dGetTime() + 10 * 60; // 10 min by default
		ItemID         = 0;
		CustomerID     = 0;
		CharID         = 0;
		
		IsDirty        = false;
	}
};

class INetworkHelper
{
  public:
	BYTE		PeerVisStatus[ServerGameLogic::MAX_PEERS_COUNT];

	float		distToCreateSq;
	float		distToDeleteSq;
	
	IServerObject	srvObjParams_;

  public:
	INetworkHelper();

	virtual DefaultPacket*	NetGetCreatePacket(int* out_size) = NULL;

	bool		GetVisibility(DWORD peerId) const
	{
		r3d_assert(peerId < ServerGameLogic::MAX_PEERS_COUNT);
		return PeerVisStatus[peerId] ? true : false;
	}
	
	// persistent objects functions
	virtual int	GetServerObjectSerializationType() { return 0; }
	virtual void	LoadServerObjectData() = NULL;	// fill objects data from srvObjParams_
	virtual void	SaveServerObjectData() = NULL;  // fill srvObjParams_ with current objects data
};

// for now it'll be placed here, need to move somewhere to utils
class xml_string_writer : public pugi::xml_writer
{
  public:
	std::string	out_;
	size_t		limit_;

  public:
	xml_string_writer(size_t limit = 8000)
	{
		limit_ = limit;
		out_.reserve(8000);	// 8000 is sql limit for varchar, xml_string_writer is used mostly for it. but also can be used to store varbinary
	}
	
	virtual void write(const void* data, size_t size)
	{
		if(out_.size() + size >= limit_)
			r3dError("xml_string_writer overflow");

		out_.append((char*)data, size);
	}
};

