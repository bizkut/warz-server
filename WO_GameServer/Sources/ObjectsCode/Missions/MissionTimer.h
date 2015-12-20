//=========================================================================
//	Module: MissionTimer.h
//	Copyright (C) Online Warmongers Group Inc. 2013.
//=========================================================================

#pragma once

#ifdef MISSIONS

//-------------------------------------------------------------------------

namespace Mission
{
// Timers are used to make Missions expire (future use is to make objectives expire (i.e. pass/fail mission based on time since objective started))

class Timer
{
	Timer( const Timer& Timer ) { }
	Timer& operator=( const Timer& Timer ) { }

public:
	Timer();
	virtual ~Timer() { }

	virtual bool loadTimer( pugi::xml_node& xmlTimer );

	bool IsExpired() const;

	static void SetCurrentTime();

public:
	time_t			m_expirationTime;

	static time_t	m_curTime;
};

} // namespace Mission
#endif
