//
// pbsdk.cpp
//
// PunkBuster / Game Integration SDK
//
// © Copyright 2003-2004 Even Balance, Inc. All Rights Reserved.
//
// This Software Development Kit (SDK) is proprietary and confidential. It may not be used,
// transferred, displayed or otherwise distributed in any manner except by express written 
// consent of Even Balance, Inc.
//
// created MAR 27 2003 by T.Ray @ Even Balance
//

/*SDK-geninfo

	General Notes for PB Integration:

	This PunkBuster SDK is a set of small source modules making up the "Integration Code" 
	component of the PunkBuster Anti-Cheat system. The other component is known as the 
	"Detection Code" component and is implemented as a set of DLL files which are 
	provided by Even Balance in pre-built form.

    The code below contains many empty/template functions. These are provided so that the SDK builds
	by itself. The empty functions are designed to be removed during integration with the game.

	PB will call functions in this source module. The game should not call any functions in this source module.
	This source module should be compiled and linked to exactly one component (i.e. exe or dll) of the game.

	The SDK consists of nine (9) files:

	pbsdk.cpp - this file which is heavily modified during integration of the SDK into the game; only
					built into one game component; functions can be moved out into game source or left in this module;

	 * the following files should not be modified if at all possible *
	pbsdk.h
	pbcommon.h
	pbcl.h
	pbcl.cpp
	pbsv.h
	pbsv.cpp
	pbmd5.h
	pbmd5.cpp

	SDK-relevent documentation is contained inside /*SDK-code ... * / blocks throughout this file

	For support issues, please email sdk@evenbalance.com.

*/


/*SDK-pbcommon

	1) The #include "pbsdk.h" line must be included in all game source modules that are 
		modified during PB integration - this handles declarations for all PB-related constant, 
		structure, and function definitions

*/

#include "PunkBuster.h"

#ifdef __WITH_PB__

#define DEFINE_PbSdk	/* this define is used once in all game modules (usually where the main() function is defined) */
#include "pbsdk.h"

/*SDK-getcvarvalue
*/
#ifdef __WITH_PB__
//
// PBsdk_GetCvarValue
//
extern "C" char *PBsdk_GetCvarValue ( char *var_name , int origination ) //0=client 1=server
{
	static char result[PB_Q_MAXRESULTLEN+1] = "" ;
	*result = 0;

	// Game version string
	if ( !_strnicmp ( var_name, "version", 7 ) )
		sprintf ( result, "%d", P2PNET_VERSION );
	// Player name
	else if ( !_strnicmp ( var_name, "name", 4 ) )
	   sprintf ( result, "%s", "" );
	// Server IP:Port
	else if ( !_strnicmp( var_name, "server", 11 ) )
	{
		DWORD serverIpAddress = gServerLogic.GetExternalIP() ;
		sprintf ( result, "%s:%d", inet_ntoa ( *(in_addr*)&serverIpAddress ), 0 );
	}
	// Server name
	else if ( !_strnicmp( var_name, "sv_hostname", 11 ) )
		sprintf ( result, "%s", gServerLogic.ginfo_.name );
	// Current map name
	else if ( !_strnicmp( var_name, "mapname", 7 ) )
		sprintf ( result, "%d", gServerLogic.ginfo_.mapId );
	// Game/Mod name
	else if ( !_strnicmp( var_name, "gamename", 8 ) )
	    sprintf ( result, "%s", "");
	// Extra queries here
	else if ( !_strnicmp ( var_name, "extra", 5 ) ) {
	}
	// Implement generic cvar query
	else {
        }	

	result[PB_Q_MAXRESULTLEN] = 0;

	return result ;
}
#endif

/*SDK-setclpunkbuster

	This function sets the cl_punkbuster game variable/setting which should be read-only for the user

	1) Remove the empty function
	2) Replace the sample forward declaration and the sample function call with your code

*/
#ifdef __WITH_PB__
//
// PBsdk_SetClPunkBuster
//
extern "C" void PBsdk_SetClPunkBuster ( char *value )
{

	return ;

}
#endif



/*SDK-setsvpunkbuster
*/
#ifdef __WITH_PB__
//
// PBsdk_SetSvPunkBuster
//
extern "C" void PBsdk_SetSvPunkBuster ( char *value )
{
	return ;

}
#endif

/*SDK-getmaxclients
*/
#ifdef __WITH_PB__
//
// PBsdk_GetMaxClients
//
extern "C" int PBsdk_GetMaxClients ( void )
{
	return MAX_NUM_PLAYERS;
}
#endif

/*SDK-getbasepath

PB's install directory (at least read access) - may or may not be same as homepath below

*/
#ifdef __WITH_PB__
//
// PBsdk_getBasePath
//
extern "C" char *PBsdk_getBasePath ( char *path , int maxlen )
{
	*path = 0 ; //game home path goes here (may be different from install path)

	// Using the current working directory
	_getcwd ( path, maxlen ) ;
	path[maxlen-1] = 0 ;

	return path ;
}
#endif



/*SDK-gethomepath

PB's home directory (with read/write access) - may or may not be same as basepath above

*/
#ifdef __WITH_PB__
extern int cfg_hostPort;
//
// PBsdk_getHomePath
//
extern "C" char *PBsdk_getHomePath ( char *path , int maxlen )
{
	char cwd[MAX_PATH];
	_getcwd(cwd, sizeof(cwd));

	// set a path based on the current working directory plus "pb" plus the server name
	// need wzsrv/ so we can delete all pb data if needed with rmdir
	char homePath[MAX_PATH];
	sprintf_s(homePath, sizeof(homePath), "%s/pb/wzsrv/%d", cwd, cfg_hostPort);
	// copy homepath to output. note: we should not truncate path, better exit
	if(strlen(homePath) + 1 >= (size_t)maxlen)
		r3dError("PB homepath is too big for PBsdk_getHomePath");
	strcpy_s(path, maxlen, homePath);
	
	// create directory structure (actual working dir will be 'pb' in homepath)
	char dir[MAX_PATH];
	sprintf_s(dir, sizeof(dir), "%s/pb/wzsrv", cwd);			_mkdir(dir);
	sprintf_s(dir, sizeof(dir), "%s/pb/wzsrv/%d", cwd, cfg_hostPort);	_mkdir(dir);
	sprintf_s(dir, sizeof(dir), "%s/pb/wzsrv/%d/pb", cwd, cfg_hostPort);	_mkdir(dir);

	// Copy "pbsvgame.cfg" to the working dir
	char fromFn[MAX_PATH], toFn[MAX_PATH];
	sprintf_s(fromFn, sizeof(fromFn), "%s/pb/pbsvgame.cfg", cwd);
	sprintf_s(toFn, sizeof(toFn), "%s/pbsvgame.cfg", dir);

	FILE *f = fopen ( toFn , "rb" ) ;
	if ( f != NULL ) {
		fclose ( f ) ;
	} else {
		PbCopyFile ( fromFn , toFn ) ;
	}

	path[maxlen-1] = 0 ;

	return path ;
}
#endif




/*SDK-getclientinfo
*/
#ifdef __WITH_PB__
//
// PBsdk_GetClientInfo
//
extern "C" int PBsdk_GetClientInfo ( int svsIndex , stPb_Sv_Client *c )
{
	// Zero set the client structure
	memset ( c , 0 , sizeof ( *c ) ) ;

	// Check to make sure we don't exceed the current/max number of players
	if ( svsIndex < 0 || svsIndex > MAX_NUM_PLAYERS )
		return 0 ;

	// Grab the player object
	obj_ServerPlayer* pl = gServerLogic.GetPlayerByIdx(svsIndex);
	if ( !pl )
		return 0 ;

	// ptumik: do not send admins to PB, as some of them use cheats for testing\work + using dev client
	if(pl->profile_.ProfileData.isDevAccount > 0)
		return 0;

	// Set the IP
	DWORD ipAddress = gServerLogic.net_->GetPeerIp ( pl->peerId_ ) ;
	sprintf_s( c->ip, PB_NAMELEN, "%s:%d", inet_ntoa ( *(in_addr*)&ipAddress ), ntohs ( gServerLogic.net_->GetPeerPort ( pl->peerId_ ) ) ) ;

	// Set the Player Name
	memcpy ( c->name, pl->loadout_->Gamertag, PB_NAMELEN ) ;

    // Set the Player's GUID	
	sprintf_s ( c->guid, PB_GUIDLEN, "%d", pl->profile_.CustomerID );

	return 1;
}
#endif




/*SDK-getclientstats
*/
#ifdef __WITH_PB__
//
// PBsdk_GetClientStats
//
extern "C" int PBsdk_GetClientStats ( int svsIndex , char *data )
{

	*data = 0;

	// Populate string with client stats
	sprintf ( data, "team=%d score=%d deaths=%d tks=%ld spawns=%d", 0, 0, 0, 0, 0 );

	return 1;
}
#endif



/*SDK-getserveraddr
*/
#ifdef __WITH_PB__
//
// PBsdk_GetServerAddr
//
extern "C" char *PBsdk_GetServerAddr ( void )
{

	static char saddr[128] = "" ;

	strncpy ( saddr, "", 128 );
	saddr[127] = 0;

	return saddr ;
}
#endif




/*SDK-sendupdpacket
*/
#ifdef __WITH_PB__
//
// PBsdk_SendUdpPacket
//
extern "C" void PBsdk_SendUdpPacket ( char *addr , unsigned short port , int datalen , char *data, bool fromServer = false )
{

	gServerLogic.net_->SendToAddress ( data, datalen, addr, port ) ;
	return ;
	
}
#endif




/*SDK-sendclpacket
*/
#ifdef __WITH_PB__
//
// PBsdk_SendClPacket
//
extern "C" void PBsdk_SendClPacket( int datalen , char *data ) {

	// This is the server.  No cl packets to send.
	return ;

}
#endif



/*SDK-sendsvpacket
*/
#ifdef __WITH_PB__
//
// PBsdk_SendSvPacket
//
extern "C" void PBsdk_SendSvPacket( int datalen , char *data , int index ) {

	// Check to make sure we don't exceed the current/max number of players
	if ( index < 0 || index > MAX_NUM_PLAYERS )
		return;

	// Make sure we don't overflow
	if ( datalen+4 > PB_MAXPKTLEN ) {
		return;
	}
	memmove( data+4, data, datalen );
	data[0] = '\xff';
	data[1] = '\xff';
	data[2] = '\xff';
	data[3] = '\xff';

	obj_ServerPlayer* pl = gServerLogic.GetPlayerByIdx(index);
	if ( !pl )
		return ;

    // assemble packet and send it to host
	int   pktsize = sizeof(PKT_S2C_PunkBuster_s) + datalen + 4;
	char* pktdata = game_new char[pktsize + 1];

	PKT_S2C_PunkBuster_s n;
	n.errorCode = 0;
	n.dataSize  = (WORD)datalen+4;
	memcpy(pktdata, &n, sizeof(n));
	memcpy(pktdata + sizeof(n), data, n.dataSize);
	gServerLogic.p2pSendRawToPeer ( pl->peerId_, (DefaultPacket*)pktdata, pktsize, false ) ; 

	delete[] pktdata;

}
#endif



/*SDK-execcmd

	This function executes a game command just like if the game engine read the command
	from a script or received console input from the user

	1) Remove the empty function
	2) Replace the sample forward declaration and the sample function call with your code

*/
#ifdef __WITH_PB__
//
// PBsdk_ExecCmd
//
extern "C" void PBsdk_ExecCmd ( const char *cmd )
{

	// Execute a command on the engine's command processor
	return ;

}
#endif

/*SDK-out
*/
#ifdef __WITH_PB__
//
// PBsdk_Out
//
extern "C" void PBsdk_Out( char *msg , unsigned long color ) {

	char	*cp = msg;
	int	skipnotify = 0 ;

	// Send it to the backend (perhaps not visible) console always
	r3dOutToLog(msg);

}
#endif




/*SDK-getserverinfo
*/
#ifdef __WITH_PB__
//
// PBsdk_GetServerInfo
//
extern char *PB_Q_Serverinfo ( void ) ;
char *PB_Q_Serverinfo ( void ){ return "" ;}
//
extern "C" char *PBsdk_GetServerInfo ( void )
{
	return PB_Q_Serverinfo() ;
}
#endif





/*SDK-dropclient
*/
#ifdef __WITH_PB__
//
// PBsdk_DropClient
//
extern "C" void PBsdk_DropClient ( int clientIndex , char *reason )
{
	// Grab the player object
	obj_ServerPlayer* pl = gServerLogic.GetPlayerByIdx(clientIndex);
	if ( !pl )
		return ;

	// Send the kick reason to the player prior to disconnect
	PKT_S2C_CustomKickMsg_s n;
	strncpy_s( n.msg, sizeof(n.msg), reason, _TRUNCATE) ;
	gServerLogic.p2pSendRawToPeer ( pl->peerId_, &n, sizeof(n) ) ;

	// Kick a player from the server with the given reason
	gServerLogic.LogCheat(pl->peerId_, PKT_S2C_CheatWarning_s::CHEAT_PunkBusterKick, true, "kick", reason);
}
#endif



/*SDK-forceteam
*/
#ifdef __WITH_PB__
//
// PBsdk_ForceTeam
//
extern "C" void PBsdk_ForceTeam ( int clientIndex, int team )
{
        // Switch a player to the given team index
}
#endif



/*SDK-glquery
*/
#ifdef __WITH_PB__
//
// PBsdk_GlQuery
//
extern "C" char *PBsdk_GlQuery ( int queryType )
{
	switch ( queryType ) {
	case PB_GL_READPIXELS: return (char *) PBsdk_GlQuery ;//function pointer
	case PB_GL_WIDTH: return (char *) 640 ;
	case PB_GL_HEIGHT: return (char *) 480 ;
	case PB_GL_RGB: return (char *) 0 ;
	case PB_GL_UB: return (char *) 0 ;
	}
	return NULL ;
}
#endif


/*SDK-getkeyname
*/
#ifdef __WITH_PB__
//
// PBsdk_GetKeyName
//
//extern char *Key_KeynumToString( int keynum ) ;
char *Key_KeynumToString( int keynum ) {return "";}
//
extern "C" char *PBsdk_GetKeyName ( int keynum )
{
	return (char *) Key_KeynumToString ( keynum ) ;
}
#endif



/*SDK-getkeybinding
*/
#ifdef __WITH_PB__
//
// PBsdk_GetKeyBinding
//
//extern char *Key_GetBinding ( int keynum ) ;
char *Key_GetBinding ( int keynum ) {return "";}
//
extern "C" char *PBsdk_GetKeyBinding ( int keynum )
{
	return (char *) Key_GetBinding ( keynum ) ;
}
#endif



/*SDK-getkeyvalue
*/
#ifdef __WITH_PB__
//
// PBsdk_GetKeyValue
//
extern char *Info_ValueForKey( const char *s, const char *key ) ;
char *Info_ValueForKey ( const char *s , const char *key ) { return "" ; }
//
extern "C" char *PBsdk_GetKeyValue ( char *s , char *k )
{
	return Info_ValueForKey ( s , k ) ;
}
#endif




/*SDK-getmaxkeys
*/
#ifdef __WITH_PB__
//
// PBsdk_GetMaxKeys
//
extern int PbMaxKeys ( void ) ;
int PbMaxKeys ( void ) {return 0;}
//
extern "C" int PBsdk_GetMaxKeys ( void )
{
	return PbMaxKeys() ;
}
#endif



/*SDK-cvarset

	This function sets a game variable/setting (aka cvar)

	1) Remove the empty function
	2) Replace the sample forward declaration and the sample function call with your code

*/
#ifdef __WITH_PB__
//
// PBsdk_CvarSet
//
extern void Cvar_Set ( const char *cvar , const char *value ) ;		//sample forward declaration
void Cvar_Set ( const char *cvar , const char *value ){}			//empty function - remove
//
extern "C" void PBsdk_CvarSet ( const char *varName , const char *value )
{
	Cvar_Set ( varName , value ) ;									//sample function call
}
#endif




/*SDK-dllhandle

	This function returns the handle to the open game DLL matching the provided DLL filename
	Not Applicable in some games

	If Applicable to the game:
		1) Remove the empty function
		2) Replace the sample forward declaration and the sample function call with your code

*/
#ifdef __WITH_PB__
//
// PBsdk_DllHandle
//
extern void *PbDllHandle ( const char *modname ) ;					//sample forward declaration
void *PbDllHandle ( const char *modname ){return NULL;}		//empty function - remove if necessary
//
extern "C" void *PBsdk_DllHandle ( const char *modname )
{
	return PbDllHandle ( modname ) ;								//sample function call
}
#endif



/*SDK-cvarvalidate

	This function examines all game variables/settings (aka cvars) to determine if any have been hacked.
	Normally this is done by comparing the binary value stored with each variable to the char/string
		representation to make sure they match
	The passed function parameter (buf) must be at least PB_Q_MAXRESULTLEN+1 in size
	This function sets buf to an empty string if all cvars are OK
	Otherwise, the variable name is copied into buf (the function must ensure that only the first
		PB_Q_MAXRESULTLEN chars of the variable name are copied)
	The return value is always buf

	1) Remove the empty function
	2) Replace the sample forward declaration and the sample function call with your code

*/
#ifdef __WITH_PB__
//
// PBsdk_CvarValidate
//
extern char *PbCvarValidate ( char *buf ) ;							//sample forward declaration
char *PbCvarValidate ( char *buf ){*buf=0; return buf;}				//empty function - remove
//
extern "C" char *PBsdk_CvarValidate ( char *buf )
{
	return PbCvarValidate ( buf ) ;									//sample function call
}
#endif



/*SDK-cvarwalk

	This function provides a mechanism whereby PB can 'walk' through the game's list of 
		variables/settings (cvars) one at a time.
	The first call to this function sets *name, *string, flags and *resetString to appropriate
		values for the first variable in the game's list of variables and then returns 1
	Each subsequent call sets the pointers to appropriate values for the next variable in the list
		and then returns 1
	When there are no more game variables in the list, the function returns 0 and prepares so that 
		the next call will start the whole process over with the first variable in the list

	1) Remove the empty function
	2) Replace the sample forward declaration and the sample function call with your code

*/
#ifdef __WITH_PB__
//
// PBsdk_CvarWalk
//
extern int PbCvarWalk ( char **name , char **string , int *flags , char **resetString ) ;	//sample forward declaration
int PbCvarWalk ( char **name , char **string , int *flags , char **resetString ){return 0;}	//empty function - remove
//
extern "C" int PBsdk_CvarWalk ( char **name , char **string , int *flags , char **resetString )
{
	return PbCvarWalk ( name , string , flags , resetString ) ;								//sample function call
}
#endif

#ifdef __WITH_PB__
//
// PBsdk_GetFileMD5
//
// assumes pointers point to buffers large enough to hold data
//
void PBsdk_GetFileMD5 ( int index, char *fn, unsigned char *md5, long *sz )
{
	PbGetFileMD5 ( index, fn, md5, sz ) ;
}
#endif




/*SDK-setpointers

To be modified and linked into game code module (exe or dll)

*/
#ifdef __WITH_PB__

extern "C" char *PBsdk_getBasePath ( char *path , int maxlen ) ;
extern "C" char *PBsdk_getHomePath ( char *path , int maxlen ) ;
extern "C" void  PBsdk_CvarSet ( const char *varName , const char *value ) ;
extern "C" void  PBsdk_SetClPunkBuster ( char *value ) ;
extern "C" void  PBsdk_ExecCmd ( const char *cmd ) ;
extern "C" void *PBsdk_DllHandle ( const char *modname ) ;
extern "C" char *PBsdk_CvarValidate ( char *buf ) ;
extern "C" int   PBsdk_CvarWalk ( char **name , char **string , int *flags , char **resetString ) ;
extern "C" char *PBsdk_GetKeyName ( int keynum ) ;
extern "C" char *PBsdk_GetKeyBinding ( int keynum ) ;
extern "C" int   PBsdk_GetMaxKeys ( void ) ;
extern "C" char *PBsdk_GetServerAddr ( void ) ;
extern "C" char *PBsdk_GetKeyValue ( char *s , char *k ) ;
extern "C" char *PBsdk_GetServerInfo ( void ) ;
extern "C" char *PBsdk_GetCvarValue ( char *var_name , int origination ) ;
extern "C" void  PBsdk_Out ( char *msg , unsigned long color ) ;
extern "C" void  PBsdk_SendClPacket ( int datalen , char *data ) ;
extern "C" void  PBsdk_SendUdpPacket ( char *addr , unsigned short port , int datalen , char *data, bool fromServer ) ;
extern "C" char *PBsdk_GlQuery ( int queryType ) ;
extern "C" void  PBsdk_SetSvPunkBuster ( char *val ) ;
extern "C" void  PBsdk_DropClient ( int clientIndex , char *reason ) ;
extern "C" void  PBsdk_ForceTeam ( int clientIndex , int team) ;
extern "C" int   PBsdk_GetMaxClients ( void ) ;
extern "C" int   PBsdk_GetClientInfo ( int index , stPb_Sv_Client *c ) ;
extern "C" int   PBsdk_GetClientStats ( int index , char *data ) ;
extern "C" void  PBsdk_SendSvPacket ( int datalen , char *data , int index ) ;

//
// PBsdk_SetPointers
//
extern "C" void PBsdk_SetPointers ( void )
{
	if ( pbsdk != NULL ) return ;	//already accomplished

	pbsdk = &PbSdkInstance ;

	//uncomment lines from the following section that are to be defined in "this" game module
	pbsdk->m_CvarSet = PBsdk_CvarSet ;
	pbsdk->m_CvarValidate = PBsdk_CvarValidate ;
	pbsdk->m_CvarWalk = PBsdk_CvarWalk ;
	pbsdk->m_GetFileMD5 = PBsdk_GetFileMD5 ;
	pbsdk->m_DllHandle = PBsdk_DllHandle ;
	pbsdk->m_DropClient = PBsdk_DropClient ;
	pbsdk->m_ForceTeam = PBsdk_ForceTeam ;
	pbsdk->m_ExecCmd = PBsdk_ExecCmd ;
	pbsdk->m_getBasePath = PBsdk_getBasePath ;
	pbsdk->m_GetClientInfo = PBsdk_GetClientInfo ;
	pbsdk->m_GetClientStats = PBsdk_GetClientStats ;
	pbsdk->m_GetCvarValue = PBsdk_GetCvarValue ;
	pbsdk->m_getHomePath = PBsdk_getHomePath ;
	pbsdk->m_GetKeyBinding = PBsdk_GetKeyBinding ;
	pbsdk->m_GetKeyName = PBsdk_GetKeyName ;
	pbsdk->m_GetKeyValue = PBsdk_GetKeyValue ;
	pbsdk->m_GetMaxClients = PBsdk_GetMaxClients ;
	pbsdk->m_GetMaxKeys = PBsdk_GetMaxKeys ;
	pbsdk->m_GetServerAddr = PBsdk_GetServerAddr ;
	pbsdk->m_GetServerInfo = PBsdk_GetServerInfo ;
	pbsdk->m_GlQuery = PBsdk_GlQuery ;
	pbsdk->m_Out = PBsdk_Out ;
	pbsdk->m_SendClPacket = PBsdk_SendClPacket ;
	pbsdk->m_SendSvPacket = PBsdk_SendSvPacket ;
	pbsdk->m_SendUdpPacket = PBsdk_SendUdpPacket ;
	pbsdk->m_SetClPunkBuster = PBsdk_SetClPunkBuster ;
	pbsdk->m_SetSvPunkBuster = PBsdk_SetSvPunkBuster ;

	pbsdk->pbcl.m_GlQuery = PBsdk_GlQuery ;

}
#endif
#endif
