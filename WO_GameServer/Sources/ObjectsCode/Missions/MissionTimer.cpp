//=========================================================================
//	Module: MissionTimer.cpp
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#include "r3dPCH.h"
#include "r3d.h"

#include "MissionTimer.h"
//#include "GameCommon.h"
#include "..\obj_ServerPlayer.h"
//#include "..\obj_ServerMissionArea.h"
//#include "..\..\AsyncFuncs.h"


//-------------------------------------------------------------------------

#ifdef MISSIONS
namespace Mission
{

time_t Timer::m_curTime;

Timer::Timer()
	: m_expirationTime( -1 )
{
}

bool find_minute_predicate(pugi::xml_attribute attr){ return stricmp(attr.name(), "minute") == 0; }
bool find_hour_predicate(pugi::xml_attribute attr)	{ return stricmp(attr.name(), "hour") == 0; }
bool find_day_predicate(pugi::xml_attribute attr)	{ return stricmp(attr.name(), "day") == 0; }
bool find_month_predicate(pugi::xml_attribute attr)	{ return stricmp(attr.name(), "month") == 0; }
bool find_year_predicate(pugi::xml_attribute attr)	{ return stricmp(attr.name(), "year") == 0; }
bool Timer::loadTimer( pugi::xml_node& xmlTimer )
{
	if( xmlTimer.empty() )
	{
		r3dOutToLog( "!!! Timer not defined!\n" );
		return false;
	}

	// Don't want default values of 0 here, so a search is performed
	// first, to make sure the attribute exists.
	if( xmlTimer.find_attribute(find_minute_predicate).empty() ||
		xmlTimer.find_attribute(find_hour_predicate).empty() ||
		xmlTimer.find_attribute(find_day_predicate).empty() ||
		xmlTimer.find_attribute(find_month_predicate).empty() ||
		xmlTimer.find_attribute(find_year_predicate).empty() )
	{
		r3dOutToLog( "!!! Timer not properly formatted!  Needs to have minute, hour, day, month and year.\n" );
		r3dOutToLog( "!!!  Minute Present: %s\n", xmlTimer.find_attribute(find_minute_predicate).as_bool() ? "true" : "false" );
		r3dOutToLog( "!!!  Hour Present: %s\n", xmlTimer.find_attribute(find_hour_predicate).as_bool() ? "true" : "false" );
		r3dOutToLog( "!!!  Day Present: %s\n", xmlTimer.find_attribute(find_day_predicate).as_bool() ? "true" : "false" );
		r3dOutToLog( "!!!  Month Present: %s\n", xmlTimer.find_attribute(find_month_predicate).as_bool() ? "true" : "false" );
		r3dOutToLog( "!!!  Year Present: %s\n", xmlTimer.find_attribute(find_year_predicate).as_bool() ? "true" : "false" );
		xml_string_writer xmlBuf;
		xmlTimer.print(xmlBuf);
		r3dOutToLog( "!!!  Timer:\n%s\n", xmlBuf.out_.c_str() );
		return false;
	}

	tm timeInfo;
	timeInfo.tm_sec		= 0;
	timeInfo.tm_min		= xmlTimer.attribute("minute").as_int();
	timeInfo.tm_hour	= xmlTimer.attribute("hour").as_int();
	timeInfo.tm_mday	= xmlTimer.attribute("day").as_int();
	timeInfo.tm_mon		= xmlTimer.attribute("month").as_int() - 1;		// localtime and gmtime return 0-based months.
	timeInfo.tm_year	= xmlTimer.attribute("year").as_int() - 1900;	// localtime and gmtime return the current year minus 1900.
	timeInfo.tm_isdst	= 0;

	//r3dOutToLog("Expiration Time: %02d/%02d/%d %02d:%02d:%02d\n", timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_year + 1900, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

	m_expirationTime = mktime( &timeInfo );

	return true;
}

bool Timer::IsExpired() const
{
	return m_expirationTime < m_curTime;
}

void Timer::SetCurrentTime()
{
	time_t t = time(0);
	tm* timeInfo = localtime( &t );
	timeInfo->tm_isdst = 0;		// zero out Daylight Savings Time, so comparison is equal.
	m_curTime = mktime( timeInfo );

	//r3dOutToLog("Time: %02d/%02d/%d %02d:%02d:%02d\n", timeInfo->tm_mon + 1, timeInfo->tm_mday, timeInfo->tm_year + 1900, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
}

}// namespace Mission
#endif