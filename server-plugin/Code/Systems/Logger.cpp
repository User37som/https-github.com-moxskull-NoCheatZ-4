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

#include "Logger.h"

#include <fstream>
#include <iostream>

#include "Misc/include_windows_headers.h"
#include "Misc/Helpers.h"
#include "Misc/temp_Metrics.h"
#include "Players/NczPlayerManager.h"
#include "Systems/AutoTVRecord.h"

void Logger::Push ( const char * msg )
{
	/*
		We need to make a copy of msg because most of functions calling Push are using Helpers::format while Push is also using Helpers::format.
		But because Helpers::format is using a static buffer, msg also points to this static buffer. Hence, calling Helpers::format here will just overwrite the message ...
	*/
	basic_string copy_msg ( msg );

	int server_tick;
	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		server_tick = static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
	}
	else
	{
		server_tick = static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
	}

	if( AutoTVRecord::GetInstance ()->IsRecording () )
	{
		basic_string move_msg;
		move_msg.reserve ( 255 );
		move_msg.append ( Helpers::getStrDateTime ( "%x %X " ) );
		move_msg.append ( Helpers::format ( "[ Server Tick #%d, SourceTV:%s.dem : Tick #%d ] ", server_tick, AutoTVRecord::GetInstance ()->GetRecordFilename ().c_str (), AutoTVRecord::GetInstance ()->GetRecordTick () ) );
		move_msg.append ( copy_msg );
		m_msg.AddToTail ( std::move( move_msg ) );
	}
	else
	{
		basic_string move_msg;
		move_msg.reserve ( 255 );
		move_msg.append ( Helpers::getStrDateTime ( "%x %X " ) );
		move_msg.append ( Helpers::format ( "[ Server Tick #%d] ", server_tick ) );
		move_msg.append ( copy_msg );
		m_msg.AddToTail ( std::move ( move_msg ) );
	}

	ProcessFilter::HumanAtLeastConnected filter_class;

	if( NczPlayerManager::GetInstance ()->GetPlayerCount ( &filter_class ) == 0 )
	{
		// We can flush right now.

		Flush ();
	}
}

template <>
void Logger::Msg<MSG_CONSOLE> ( const char * msg, int verbose /*= 0*/ )
{
	std::cout << prolog.c_str () << ' ' << Plat_FloatTime() << ' ' << msg << '\n';
#ifdef WIN32
	OutputDebugStringA ( prolog.c_str () );
	OutputDebugStringA ( msg );
	OutputDebugStringA ( "\n" );
#endif
}

template <>
void Logger::Msg<MSG_CHAT> ( const char * msg, int verbose /*= 0*/ )
{
	Msg<MSG_CONSOLE> ( msg );
	basic_string m ( prolog );
	Helpers::chatprintf ( m.append ( msg ).c_str () );
}

template <>
void Logger::Msg<MSG_LOG> ( const char * msg, int verbose /*= 0*/ )
{
	Msg<MSG_CONSOLE> ( msg );
	Push ( msg );
}

template <>
void Logger::Msg<MSG_LOG_CHAT> ( const char * msg, int verbose /*= 0*/ )
{
	Msg<MSG_LOG> ( msg );
	basic_string m ( prolog );
	Helpers::chatprintf ( m.append ( msg ).c_str () );
}

template <>
void Logger::Msg<MSG_WARNING> ( const char * msg, int verbose /*= 0*/ )
{
	basic_string m1 ( "WARNING : " );
	m1.append ( msg );
	Push ( m1.c_str () );

	basic_string m2 ( prolog );
	m2.append ( m1 ).append ( '\n' );
	std::cout << m2.c_str ();
#ifdef WIN32
	OutputDebugStringA ( m2.c_str () );
#endif
}

template <>
void Logger::Msg<MSG_ERROR> ( const char * msg, int verbose /*= 0*/ )
{
	basic_string m1 ( "ERROR : " );
	m1.append ( msg );
	Push ( m1.c_str () );

	basic_string m2 ( prolog );
	m2.append ( m1 ).append ( '\n' );
	std::cout << m2.c_str ();
#ifdef WIN32
	OutputDebugStringA ( m2.c_str () );
#endif
}

template <>
void Logger::Msg<MSG_HINT> ( const char * msg, int verbose /*= 0*/ )
{
	std::cerr << prolog.c_str () << Plat_FloatTime () << " : " << msg << '\n';
}

template <>
void Logger::Msg<MSG_VERBOSE1> ( const char * msg, int verbose /*= 0*/ )
{
	if( verbose == 1 )
	{
		Msg<MSG_CONSOLE> ( basic_string ( "VERBOSE1 : " ).append ( msg ) );
	}
}

template <>
void Logger::Msg<MSG_VERBOSE2> ( const char * msg, int verbose /*= 0*/ )
{
	if( verbose == 2 )
	{
		Msg<MSG_CONSOLE> ( basic_string ( "VERBOSE2 : " ).append ( msg ) );
	}
}

template <>
void Logger::Msg<MSG_DEBUG> ( const char * msg, int verbose /*= 0*/ )
{
	if( verbose > 2 )
	{
		Msg<MSG_CONSOLE> ( basic_string ( "DEBUG : " ).append ( msg ) );
	}
}

void Logger::Flush ()
{
	if( m_msg.IsEmpty () ) return;

	basic_string path;
	SourceSdk::InterfacesProxy::GetGameDir ( path );

	path.append ( Helpers::getStrDateTime ( "/logs/NoCheatZ_4_Logs/NoCheatZ-%d-%b-%Y.log" ) );
	std::ofstream fichier ( path.c_str (), std::ios::out | std::ios::app );
	if( fichier )
	{
		size_t pos ( 0 );
		size_t const max ( m_msg.Size () );
		do
		{
			fichier << m_msg[ pos ].c_str () << std::endl;
		}
		while( ++pos != max );
	}
	else
	{
		basic_string m1 ( prolog );
		m1.append ( Helpers::format ( "Can't write to logfile at %s ... Please check write access and if the directory exists.\n", path.c_str () ) );
		Msg<MSG_CONSOLE> ( m1 );
		SourceSdk::InterfacesProxy::Call_LogPrint ( m1.c_str () );
		m1 = prolog;
		size_t pos ( 0 );
		size_t const max ( m_msg.Size () );
		do
		{
			SourceSdk::InterfacesProxy::Call_LogPrint ( m1.append ( m_msg[ pos ] ).c_str () );
		}
		while( ++pos != max );
	}


	m_msg.RemoveAll ();
	m_msg.EnsureCapacity ( 256 );
}

void Helpers::writeToLogfile ( const basic_string &text )
{
	Logger::GetInstance ()->Push ( text.c_str () );
}
