#include "r3dPCH.h"
#include "r3d.h"

#include "NetworkHelper.h"

INetworkHelper::INetworkHelper()
{
	memset(PeerVisStatus, 0, sizeof(PeerVisStatus));

#if 0
	distToCreateSq = 20 * 20;
	distToDeleteSq = 30 * 30;
#else
	distToCreateSq = 500 * 500;
	distToDeleteSq = 600 * 600;
#endif
}

IServerObject::CSrvObjXmlReader::CSrvObjXmlReader(const std::string& in_str)
{
	xmlFile.load_buffer(in_str.c_str(), in_str.size());
	xmlObj = xmlFile.first_child();
}

IServerObject::CSrvObjXmlWriter::CSrvObjXmlWriter()
{
	xmlObj = xmlFile.append_child();
	xmlObj.set_name("obj");
}

void IServerObject::CSrvObjXmlWriter::save(std::string& out_str)
{
	xml_string_writer xmlBuf;
	xmlFile.save(xmlBuf, "", pugi::format_raw);

	out_str = xmlBuf.out_;
}
