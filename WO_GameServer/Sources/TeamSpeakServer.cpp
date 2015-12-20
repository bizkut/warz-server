#include "r3dPCH.h"
#include "r3d.h"

#define ENABLE_TEAMSPEAK 1

#ifndef ENABLE_TEAMSPEAK
void TSServer_Start(int port) {}
void TSServer_Stop() {}
DWORD TSServer_GetPassword() {}
#endif

#ifdef ENABLE_TEAMSPEAK
// team speak sdk includes
#include <public_definitions.h>
#include <public_errors.h>
#include <serverlib_publicdefinitions.h>
#include <serverlib.h>

#pragma comment(lib, "../../../src/external/ts3_sdk_3/lib/ts3server_win32.lib")

#include "TeamSpeakServer.h"
#include "ServerGameLogic.h"
#include "AsyncFuncs.h"

static CTeamSpeakServer	gTeamSpeakServer;
static unsigned int	error;		// last error

#define PROVISIONING_SLOTS_BUFFER	32	// we expect that many clients connecting in single game frame 

#define CHECK_ERROR(x) if((error = x) != ERROR_ok) { goto on_error; }

//#define USE_CUSTOM_ENCRYPTION
//#define CUSTOM_CRYPT_KEY 0x54

static int TSHandleError(const char* func = "")
{
	char* errormsg;
	if(ts3server_getGlobalErrorMessage(error, &errormsg) == ERROR_ok)
	{
		r3dOutToLog("!!! VoipError: %s %x %s\n", func, error, errormsg);
		ts3server_freeMemory(errormsg);
	}
	else
	{
		r3dOutToLog("!!! VoipError: %s %x\n", func, error);
	}
	return 0;
}

static int readKeyPairFromFile(const char *fileName, char *keyPair)
{
	FILE *file;

	file = fopen(fileName, "r");
	if(file == NULL) {
		r3dOutToLog("Could not open file '%s' for reading keypair\n", fileName);
		return -1;
	}

	fgets(keyPair, BUFSIZ, file);
	if(ferror(file) != 0) {
		fclose (file);
		r3dOutToLog("Error reading keypair from file '%s'.\n", fileName);
		return -1;
	}
	fclose (file);

	r3dOutToLog("Read keypair '%s' from file '%s'.\n", keyPair, fileName);
	return 0;
}

static int writeKeyPairToFile(const char *fileName, const char* keyPair)
{
	FILE *file;

	file = fopen(fileName, "w");
	if(file == NULL) {
		r3dOutToLog("Could not open file '%s' for writing keypair\n", fileName);
		return -1;
	}
    
	fputs(keyPair, file);
	if(ferror(file) != 0) {
		fclose (file);
		r3dOutToLog("Error writing keypair to file '%s'.\n", fileName);
		return -1;
	}
	fclose (file);

	r3dOutToLog("Wrote keypair '%s' to file '%s'.\n", keyPair, fileName);
	return 0;
}

/*
 * Callback when client has connected.
 *
 * Parameter:
 *   serverID  - Virtual server ID
 *   clientID  - ID of connected client
 *   channelID - ID of channel the client joined
 */
static void onClientConnected(uint64 serverID, anyID clientID, uint64 channelID, unsigned int* removeClientError)
{
	// Query client nickname
	char* clientName;
	if((error = ts3server_getClientVariableAsString(serverID, clientID, CLIENT_NICKNAME, &clientName)) != ERROR_ok)
	{
		// we wasn't able to get client nickname, abort
		*removeClientError = ERROR_client_not_logged_in;
		return;
	}

	// validate client, nickname must be a voiceId associated with peer
	DWORD voiceId = 0;
	sscanf(clientName, "%u", &voiceId);
	
	char newNick[64] = "";
	int peerId = -1;
	for(peerId=0; peerId<ServerGameLogic::MAX_PEERS_COUNT; peerId++)
	{
		ServerGameLogic::peerInfo_s& peer = gServerLogic.GetPeer(peerId);
		if(peer.status_ >= ServerGameLogic::PEER_LOADING && peer.voicePeerId == voiceId)
		{
			peer.voiceClientId = clientID;
			peer.voicePeerId   = 0;
			sprintf(newNick, "i%d", peerId);
			break;
		}
	}
	
	if(newNick[0] == 0)
	{
		// possible cheat - connecting without credentials
		r3dOutToLog("!!! voip client '%s' had BAD credential 0x%x\n", clientName, voiceId);
		*removeClientError = ERROR_client_not_logged_in;
		ts3server_freeMemory(clientName);
		return;
	}

	// replace client nickname with peerid - does not work by some reason
	/*
	if((error = ts3server_setClientVariableAsString(serverID, clientID, CLIENT_NICKNAME, newNick)) != ERROR_ok)
		TSHandleError("replacing nick");
	if((error = ts3server_flushClientVariable(serverID, clientID)) != ERROR_ok)
		TSHandleError("flushing nick");
	*/
	
	r3dOutToLog("voip: Client %u '%s' peer%02d, playerIdx:%d joined channel %llu on virtual server %llu\n", clientID, clientName, peerId, voiceId & 0xFFFF, (unsigned long long) channelID, (unsigned long long)serverID);
	ts3server_freeMemory(clientName);
	
	gTeamSpeakServer.AddClient(clientID, peerId);
}

/*
 * Callback when client has disconnected.
 *
 * Parameter:
 *   serverID  - Virtual server ID
 *   clientID  - ID of disconnected client
 *   channelID - ID of channel the client left
 */
static void onClientDisconnected(uint64 serverID, anyID clientID, uint64 channelID)
{
	r3dOutToLog("voip: Client %u left channel %llu on virtual server %llu\n", clientID, (unsigned long long)channelID, (unsigned long long)serverID);
	gTeamSpeakServer.RemoveClient(clientID);
}

/*
 * Callback when client has moved.
 *
 * Parameter:
 *   serverID     - Virtual server ID
 *   clientID     - ID of moved client
 *   oldChannelID - ID of old channel the client left
 *   newChannelID - ID of new channel the client joined
 */
static void onClientMoved(uint64 serverID, anyID clientID, uint64 oldChannelID, uint64 newChannelID)
{
	r3dOutToLog("voip: !!!!!! Client %u moved from channel %llu to channel %llu on virtual server %llu\n", clientID, (unsigned long long)oldChannelID, (unsigned long long)newChannelID, (unsigned long long)serverID);
}

/*
 * Callback when channel has been created.
 *
 * Parameter:
 *   serverID        - Virtual server ID
 *   invokerClientID - ID of the client who created the channel
 *   channelID       - ID of the created channel
 */
static void onChannelCreated(uint64 serverID, anyID invokerClientID, uint64 channelID)
{
	r3dOutToLog("voip: !!!!!! Channel %llu created by %u on virtual server %llu\n", (unsigned long long)channelID, invokerClientID, (unsigned long long)serverID);
	if(invokerClientID == 0)
		return;
		
	char* name = NULL;
	if((error = ts3server_getChannelVariableAsString(serverID, channelID, CHANNEL_NAME, &name)) != ERROR_ok)
		TSHandleError("getChannelName");

	DWORD CustomerID = gTeamSpeakServer.GetCustomerID(invokerClientID);
	AsyncJobAddServerLogInfo(PKT_S2C_CheatWarning_s::CHEAT_TeamSpeak, CustomerID, 0,
		"onChannelCreated", "name: %s", 
		name ? name : "unknown");
	
	if(name) ts3server_freeMemory(name);
}

/*
 * Callback when channel has been edited.
 *
 * Parameter:
 *   serverID        - Virtual server ID
 *   invokerClientID - ID of the client who edited the channel
 *   channelID       - ID of the edited channel
 */
static void onChannelEdited(uint64 serverID, anyID invokerClientID, uint64 channelID)
{
	r3dOutToLog("voip: !!!!!! Channel %llu edited by %u on virtual server %llu\n", (unsigned long long)channelID, invokerClientID, (unsigned long long)serverID);

	if(invokerClientID == 0)
		return;

	DWORD CustomerID = gTeamSpeakServer.GetCustomerID(invokerClientID);
	AsyncJobAddServerLogInfo(PKT_S2C_CheatWarning_s::CHEAT_TeamSpeak, CustomerID, 0,
		"onChannelEdited", "");
}

/*
 * Callback when channel has been deleted.
 *
 * Parameter:
 *   serverID        - Virtual server ID
 *   invokerClientID - ID of the client who deleted the channel
 *   channelID       - ID of the deleted channel
 */
static void onChannelDeleted(uint64 serverID, anyID invokerClientID, uint64 channelID)
{
	r3dOutToLog("voip: Channel %llu deleted by %u on virtual server %llu\n", (unsigned long long)channelID, invokerClientID, (unsigned long long)serverID);

	if(invokerClientID == 0)
		return;

	DWORD CustomerID = gTeamSpeakServer.GetCustomerID(invokerClientID);
	AsyncJobAddServerLogInfo(PKT_S2C_CheatWarning_s::CHEAT_TeamSpeak, CustomerID, 0,
		"onChannelDeleted", "%llu", 
		channelID);
}

/*
 * Callback when a server text message has been received.
 * Note that only server and channel chats are received, private client messages are not caught due to privacy reasons.
 *
 * Parameter:
 *   serverID        - Virtual server ID
 *   invokerClientID - ID of the client who sent the text message
 *   textMessage     - Message text
 */
static void onServerTextMessageEvent(uint64 serverID, anyID invokerClientID, const char* textMessage)
{
}

/*
 * Callback when a channel text message has been received.
 * Note that only server and channel chats are received, private client messages are not caught due to privacy reasons.
 *
 * Parameter:
 *   serverID        - Virtual server ID
 *   invokerClientID - ID of the client who sent the text message
 *   targetChannelID - ID of the channel in which the chat was sent
 *   textMessage     - Message text
 */
static void onChannelTextMessageEvent(uint64 serverID, anyID invokerClientID, uint64 targetChannelID, const char* textMessage)
{
}

/*
 * Callback for user-defined logging.
 *
 * Parameter:
 *   logMessage        - Log message text
 *   logLevel          - Severity of log message
 *   logChannel        - Custom text to categorize the message channel
 *   logID             - Virtual server ID giving the virtual server source of the log event
 *   logTime           - String with the date and time the log entry occured
 *   completeLogString - Verbose log message including all previous parameters for convinience
 */
static void onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString)
{
	r3dOutToLog("VOICE:[%d] %s\n", logLevel, completeLogString);
}

/*
 * Callback triggered when the specified client starts talking.
 *
 * Parameters:
 *   serverID - ID of the server sending the callback
 *   clientID - ID of the client which started talking
 */
static void onClientStartTalkingEvent(uint64 serverID, anyID clientID)
{
	//r3dOutToLog("voip: onClientStartTalkingEvent serverID=%llu, clientID=%u\n", (unsigned long long)serverID, clientID);
}

/*
 * Callback triggered when the specified client stops talking.
 *
 * Parameters:
 *   serverID - ID of the server sending the callback
 *   clientID - ID of the client which stopped talking
 */
static void onClientStopTalkingEvent(uint64 serverID, anyID clientID)
{
	//r3dOutToLog("voip: onClientStopTalkingEvent serverID=%llu, clientID=%u\n", (unsigned long long)serverID, clientID);
}

/*
 * Callback triggered when a license error occurs.
 *
 * Parameters:
 *   serverID  - ID of the virtual server on which the license error occured. This virtual server will be automatically
 *               shutdown. If the parameter is zero, all virtual servers are affected and have been shutdown.
 *   errorCode - Code of the occured error. Use ts3server_getGlobalErrorMessage() to convert to a message string
 */
static void onAccountingErrorEvent(uint64 serverID, unsigned int errorCode)
{
	char* errorMessage = NULL;
	if(ts3server_getGlobalErrorMessage(errorCode, &errorMessage) != ERROR_ok)
		TSHandleError("onAccountingErrorEvent error");

	r3dOutToLog("voip: !!!!!! onAccountingErrorEvent serverID=%llu, errorCode=%u: %s\n", (unsigned long long)serverID, errorCode, errorMessage ? errorMessage : "unknown");

	AsyncJobAddServerLogInfo(PKT_S2C_CheatWarning_s::CHEAT_TeamSpeak, 0, 0,
		"onAccountingErrorEvent", "error: %d %s", 
		errorCode, errorMessage ? errorMessage : "unknown");
	
	if(errorMessage)
		ts3server_freeMemory(errorMessage);

	// Your custom handling here. In a real application, you wouldn't stop the whole process because the TS3 server part went down.
	// The whole idea of this callback is to let you gracefully handle the TS3 server failing to start and to continue your application. */
}

/*
 * Callback allowing to apply custom encryption to outgoing packets.
 * Using this function is optional. Do not implement if you want to handle the TeamSpeak 3 SDK encryption.
 *
 * Parameters:
 *   dataToSend - Pointer to an array with the outgoing data to be encrypted
 *   sizeOfData - Pointer to an integer value containing the size of the data array
 *
 * Apply your custom encryption to the data array. If the encrypted data is smaller than sizeOfData, write
 * your encrypted data into the existing memory of dataToSend. If your encrypted data is larger, you need
 * to allocate memory and redirect the pointer dataToSend. You need to take care of freeing your own allocated
 * memory yourself. The memory allocated by the SDK, to which dataToSend is originally pointing to, must not
 * be freed.
 * 
 */
static void onCustomPacketEncryptEvent(char** dataToSend, unsigned int* sizeOfData)
{
#ifdef USE_CUSTOM_ENCRYPTION
	for(unsigned int i = 0; i < *sizeOfData; i++) {
		(*dataToSend)[i] ^= CUSTOM_CRYPT_KEY;
	}
#endif
}

/*
 * Callback allowing to apply custom encryption to incoming packets
 * Using this function is optional. Do not implement if you want to handle the TeamSpeak 3 SDK encryption.
 *
 * Parameters:
 *   dataToSend - Pointer to an array with the received data to be decrypted
 *   sizeOfData - Pointer to an integer value containing the size of the data array
 *
 * Apply your custom decryption to the data array. If the decrypted data is smaller than dataReceivedSize, write
 * your decrypted data into the existing memory of dataReceived. If your decrypted data is larger, you need
 * to allocate memory and redirect the pointer dataReceived. You need to take care of freeing your own allocated
 * memory yourself. The memory allocated by the SDK, to which dataReceived is originally pointing to, must not
 * be freed.
 */
static void onCustomPacketDecryptEvent(char** dataReceived, unsigned int* dataReceivedSize)
{
#ifdef USE_CUSTOM_ENCRYPTION
	for(unsigned int i = 0; i < *dataReceivedSize; i++) {
		(*dataReceived)[i] ^= CUSTOM_CRYPT_KEY;
	}
#endif
}

uint64 CTeamSpeakServer::CreateChannel(const char* name)
{
	uint64 newChannelID;

	// Set data of new channel. Use channelID of 0 for creating channels.
	CHECK_ERROR(ts3server_setChannelVariableAsString(serverID, 0, CHANNEL_NAME,                name));
	CHECK_ERROR(ts3server_setChannelVariableAsString(serverID, 0, CHANNEL_TOPIC,               ""));
	CHECK_ERROR(ts3server_setChannelVariableAsString(serverID, 0, CHANNEL_DESCRIPTION,         ""));
	CHECK_ERROR(ts3server_setChannelVariableAsInt   (serverID, 0, CHANNEL_FLAG_PERMANENT,      0));
	CHECK_ERROR(ts3server_setChannelVariableAsInt   (serverID, 0, CHANNEL_FLAG_SEMI_PERMANENT, 1));

	/* Flush changes to server */
	CHECK_ERROR(ts3server_flushChannelCreation(serverID, 0, &newChannelID));  /* Create as top-level channel */

	r3dOutToLog("Created channel '%s': %llu\n", name, (unsigned long long)newChannelID);
	return newChannelID;

on_error:
	r3dOutToLog("Error creating channel: %d\n", error);
	return 0;
}

CTeamSpeakServer::CTeamSpeakServer()
{
	serverID      = 0;
	m_serverPwd   = 0;
	m_serverSlots = 0;
	m_curClients  = 0;
	m_stopRequest = 0;
	m_hThread     = 0;
	m_serverPort  = 0;
}

CTeamSpeakServer::~CTeamSpeakServer()
{
}

int CTeamSpeakServer::Startup()
{
	r3dOutToLog("Starting voice\n"); CLOG_INDENT;
	r3d_assert(m_serverPort > 0);

	// Query and print server lib version
	char* version;
	if((error = ts3server_getServerLibVersion(&version)) != ERROR_ok)
		return TSHandleError("ts3server_getServerLibVersion");
	r3dOutToLog("Server lib version: %s\n", version);
	ts3server_freeMemory(version);

	// Create struct for callback function pointers
	struct ServerLibFunctions funcs;
	memset(&funcs, 0, sizeof(struct ServerLibFunctions));

	// Now assign the used callback function pointers
	funcs.onClientConnected          = onClientConnected;
	funcs.onClientDisconnected       = onClientDisconnected;
	funcs.onClientMoved              = onClientMoved;
	funcs.onChannelCreated           = onChannelCreated;
	funcs.onChannelEdited            = onChannelEdited;
	funcs.onChannelDeleted           = onChannelDeleted;
	funcs.onServerTextMessageEvent   = onServerTextMessageEvent;
	funcs.onChannelTextMessageEvent  = onChannelTextMessageEvent;
	funcs.onUserLoggingMessageEvent  = onUserLoggingMessageEvent;
	funcs.onClientStartTalkingEvent  = onClientStartTalkingEvent;
	funcs.onClientStopTalkingEvent   = onClientStopTalkingEvent;
	funcs.onAccountingErrorEvent     = onAccountingErrorEvent;
	funcs.onCustomPacketEncryptEvent = onCustomPacketEncryptEvent;
	funcs.onCustomPacketDecryptEvent = onCustomPacketDecryptEvent;

	// Initialize server lib with callbacks
	if((error = ts3server_initServerLib(&funcs, LogType_USERLOGGING, NULL)) != ERROR_ok)
		return TSHandleError("ts3server_initServerLib");

	// Create a virtual server
	serverID = CreateVirtualServer("WarZ", m_serverPort, PROVISIONING_SLOTS_BUFFER);
	if(!serverID)
		return 0;

	// Set welcome message
	ts3server_setVirtualServerVariableAsString(serverID, VIRTUALSERVER_WELCOMEMESSAGE, "WarZ");
	
	// generate and set server password
	m_serverPwd = u_random(0xFFFFFFFE);
	char pwd[32]; sprintf(pwd, "%u", m_serverPwd);
	ts3server_setVirtualServerVariableAsString(serverID, VIRTUALSERVER_PASSWORD, pwd);

	// Flush above two changes to server
	if((error = ts3server_flushVirtualServerVariable(serverID)) != ERROR_ok)
		TSHandleError("ts3server_flushVirtualServerVariable on init\n");

	r3dOutToLog("Server running\n");
	
	if((error = ts3server_setChannelVariableAsInt(serverID, 1, CHANNEL_CODEC, 0)) != ERROR_ok) // Narrowband 
		TSHandleError("CHANNEL_CODEC");
	if((error = ts3server_setChannelVariableAsInt(serverID, 1, CHANNEL_CODEC_QUALITY, 3)) != ERROR_ok) // Narrowband 8 kHz
		TSHandleError("CHANNEL_CODEC_QUALITY");
	if((error = ts3server_flushChannelVariable(serverID, 1)) != ERROR_ok) // Narrowband 8 kHz
		TSHandleError("ts3server_flushChannelVariable");
	
	return 1;
}

int CTeamSpeakServer::Shutdown()
{
	r3dOutToLog("Shutting down voice\n"); CLOG_INDENT;
	
	// Stop virtual servers to make sure connected clients are notified instead of dropped
	uint64* ids;
	if((error = ts3server_getVirtualServerList(&ids)) != ERROR_ok)
	{
		if(error == ERROR_serverlibrary_not_initialised)
			return 1;
		return TSHandleError("ts3server_getVirtualServerList");
	}

	for(int i=0; ids[i]; i++) 
	{
		if((error = ts3server_stopVirtualServer(ids[i])) != ERROR_ok)
			TSHandleError("ts3server_stopVirtualServer");
	}
	ts3server_freeMemory(ids);

	// Shutdown server lib
	if((error = ts3server_destroyServerLib()) != ERROR_ok)
		TSHandleError("ts3server_destroyServerLib");
	
	return 1;
}

void CTeamSpeakServer::Tick()
{
	if(m_curClients + PROVISIONING_SLOTS_BUFFER != m_serverSlots)
	{
		m_serverSlots = m_curClients + PROVISIONING_SLOTS_BUFFER;
		if((error = ts3server_setVirtualServerVariableAsInt(serverID, VIRTUALSERVER_MAXCLIENTS, m_serverSlots)) != ERROR_ok)
			TSHandleError("VIRTUALSERVER_MAXCLIENTS");
		if((error = ts3server_flushVirtualServerVariable(serverID)) != ERROR_ok)
			TSHandleError("VIRTUALSERVER_MAXCLIENTS flush");
	}
}

uint64 CTeamSpeakServer::CreateVirtualServer(const char* name, int port, int maxClients)
{
	char buffer[BUFSIZ] = { 0 };
	char filename[BUFSIZ];
	char port_str[20];
	char *keyPair;
	uint64 serverID;

	/* Assemble filename: keypair_<port>.txt */
	strcpy(filename, "ts_keypair_");
	sprintf(port_str, "%d", port);
	strcat(filename, port_str);
	strcat(filename, ".txt");

	/* Try reading keyPair from file */
	if(readKeyPairFromFile(filename, buffer) == 0) {
		keyPair = buffer;  /* Id read from file */
	} else {
		keyPair = "";  /* No Id saved, start virtual server with empty keyPair string */
	}

	/* Create the virtual server with specified port, name, keyPair and max clients */
	r3dOutToLog("Creating virtual server with %d slots\n", maxClients);
	if((error = ts3server_createVirtualServer(port, "127.0.0.1", name, keyPair, maxClients, &serverID)) != ERROR_ok)
		return TSHandleError("ts3server_createVirtualServer");

	/* If we didn't load the keyPair before, query it from virtual server and save to file */
	if(!*buffer)
	{
		if((error = ts3server_getVirtualServerKeyPair(serverID, &keyPair)) != ERROR_ok)
		{
			return TSHandleError("ts3server_getVirtualServerKeyPair");
		}

		/* Save keyPair to file "keypair_<port>.txt"*/
		if(writeKeyPairToFile(filename, keyPair) != 0)
		{
			r3dOutToLog("!!! was unable to write keypair\n");
		}
		ts3server_freeMemory(keyPair);
	}
	
	m_serverSlots = maxClients;

	return serverID;
}

void CTeamSpeakServer::AddClient(int clientID, int peerId)
{
	// we should not have that client
	r3d_assert(m_clients.find(clientID) == m_clients.end());

	client_s client;
	client.peerID     = peerId;
	client.CustomerID = gServerLogic.GetPeer(peerId).CustomerID;
	m_clients.insert(std::pair<int, client_s>(clientID, client));
	m_curClients++;
}

void CTeamSpeakServer::RemoveClient(int clientID)
{
	m_clients.erase(clientID);
	m_curClients--;
}

DWORD CTeamSpeakServer::GetCustomerID(int clientID)
{
	stdext::hash_map<int, client_s>::iterator it = m_clients.find(clientID);
	r3d_assert(it != m_clients.end());
	
	return it->second.CustomerID;
}


static unsigned int WINAPI TSServer_Thread(void* in_ptr)
{
	r3dRandInitInTread rand_in_thread;
	
	gTeamSpeakServer.Startup();
	
	while(gTeamSpeakServer.m_stopRequest == 0)
	{
		::Sleep(200);
		gTeamSpeakServer.Tick();
	}
	
	gTeamSpeakServer.Shutdown();
	return 0;
}


void TSServer_Start(int port)
{
	//
	// note: team speak server is communicating with his license server in main thread, so we must run it in separate thread
	//
	gTeamSpeakServer.m_serverPort = port;
	gTeamSpeakServer.m_hThread    = (HANDLE)_beginthreadex(NULL, 0, TSServer_Thread, &gTeamSpeakServer, 0, NULL);
}

void TSServer_Stop()
{
	r3d_assert(gTeamSpeakServer.m_hThread);
	gTeamSpeakServer.m_stopRequest = 1;
	WaitForSingleObject(gTeamSpeakServer.m_hThread, 2000);
}


DWORD TSServer_GetPassword()
{
	return gTeamSpeakServer.m_serverPwd;
}

#endif // #ifdef ENABLE_TEAMSPEAK
