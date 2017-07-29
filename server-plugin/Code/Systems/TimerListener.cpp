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

#include "TimerListener.h"

#include "Interfaces/InterfacesProxy.h"

timer_listeners_list_t TimerListener::m_listeners;
bool TimerListener::m_undersample ( false );

TimerListener::TimerListener ()
{

}

TimerListener::~TimerListener ()
{

}

void TimerListener::RT_OnTick (double const & curtime)
{
	m_undersample = !m_undersample;

	if( m_undersample )
	{
		timer_listeners_list_t::elem_t* it1 ( m_listeners.GetFirst () );
		while( it1 != nullptr )
		{
			timer_list_t & child ( it1->m_value->m_timers );

			size_t pos ( 0 );
			size_t max ( child.Size () );
			while( pos < max )
			{
				TimerInfo& v ( child[ pos ] );

				if( v.m_last_exec + v.m_period_seconds <= curtime)
				{
					it1->m_value->RT_TimerCallback ( v.m_name );

					if( v.m_once )
					{
						child.FindAndRemove ( v );
						--max;
						continue;
					}
					else
					{
						v.m_last_exec = curtime;
					}
				}
				++pos;
			}
			it1 = it1->m_next;
		}
	}
}

void TimerListener::AddTimerListener ( TimerListener* listener )
{
	if( m_listeners.Find ( listener ) == nullptr )
		m_listeners.Add ( listener );
}

void TimerListener::RemoveTimerListener ( TimerListener* listener )
{
	m_listeners.Remove ( listener );
}

void TimerListener::AddTimer (double period_seconds, char const * const name, bool single_time )
{
	Assert ( period_seconds >= 2.0 );

	TimerInfo t ( name, period_seconds, single_time );

	if( m_timers.Find ( t ) == -1 )
		m_timers.AddToTail ( t );
}

void TimerListener::RemoveTimer ( char const * const name )
{
	m_timers.FindAndRemove ( TimerInfo ( name ) );
}

void TimerListener::ClearTimers ()
{
	m_timers.RemoveAll ();
}
