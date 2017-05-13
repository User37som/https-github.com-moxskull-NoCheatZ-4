/*
	Copyright 2012 - Le Padellec Sylvain

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef TIMERLISTENER_H
#define TIMERLISTENER_H

#include "Misc/temp_basiclist.h"
#include <cstring> // memcpy

#include "../Preprocessors.h"
#include "../Misc/Helpers.h"
#include "Misc/temp_Metrics.h"

#define TIMERINFO_CHARCOUNT 55

class TimerListener;

struct ALIGN4 TimerInfo
{
	char m_name[ TIMERINFO_CHARCOUNT ];
	float m_period_seconds;
	float m_last_exec;
	bool m_once;

	TimerInfo ()
	{}
	~TimerInfo ()
	{}
	TimerInfo ( TimerInfo const & other )
	{
		memcpy ( this, &other, sizeof ( TimerInfo ) );
	}
	TimerInfo ( char const * const name )
	{
		strncpy ( m_name, name, TIMERINFO_CHARCOUNT - 1 );
		m_name[ TIMERINFO_CHARCOUNT - 1 ] = '\0';
	}
	TimerInfo ( char const * const name, float period, bool once )
	{
		strncpy ( m_name, name, TIMERINFO_CHARCOUNT - 1 );
		m_name[ TIMERINFO_CHARCOUNT - 1 ] = '\0';
		m_period_seconds = period;
		m_last_exec = Plat_FloatTime ();
		m_once = once;
	}

	bool operator==( TimerInfo const & other ) const
	{
		return strcmp ( m_name, other.m_name ) == 0;
	}

} ALIGN4_POST;

typedef CUtlVector<TimerInfo> timer_list_t;
typedef basic_slist<TimerListener*> timer_listeners_list_t;

class TimerListener
{
private:
	timer_list_t m_timers;
	static timer_listeners_list_t m_listeners;
	static bool m_undersample;

protected:

	static void AddTimerListener ( TimerListener* listener );
	static void RemoveTimerListener ( TimerListener* listener );

	TimerListener ();
	virtual ~TimerListener ();

	void AddTimer ( float period_seconds, char const * const name, bool single_time = false );

	void RemoveTimer ( char const * const name );

	void ClearTimers ();

	virtual void RT_TimerCallback ( char const * const timer_name /* Pointer can be invalid past the function */ ) = 0;

public:
	static void RT_OnTick (float const curtime);

};

#endif // TIMERLISTENER_H
