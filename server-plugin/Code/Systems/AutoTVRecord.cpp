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

#include "AutoTVRecord.h"

AutoTVRecord::AutoTVRecord () :
	BaseDynamicSystem ( "AutoTVRecord", "Enable - Disable - Verbose" ),
	ConCommandHookListener (),
	singleton_class (),
	m_demofile ( "" ),
	m_prefix ( "NoCheatZ-autorecords/" ),
	m_recordtickcount ( 0 ),
	m_waitfortv_time ( 0.0f ),
	m_minplayers ( 1 ),
	m_recording ( false ),
	m_expectedtvconfigchange ( false ),
	m_spawn_once ( true ),
	m_started_current_record ( false )
{}

AutoTVRecord::~AutoTVRecord ()
{
	Unload ();
}

void AutoTVRecord::Init ()
{}

void AutoTVRecord::Load ()
{
	m_demofile = "";
	m_recording = false;

	m_mycommands.AddToTail ( SourceSdk::InterfacesProxy::ICvar_FindCommand ( "tv_record" ) );
	m_mycommands.AddToTail ( SourceSdk::InterfacesProxy::ICvar_FindCommand ( "tv_stoprecord" ) );
	m_mycommands.AddToTail ( SourceSdk::InterfacesProxy::ICvar_FindCommand ( "tv_stop" ) );
	//m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_delay"));
	ConCommandHookListener::RegisterConCommandHookListener ( this );

	m_waitfortv_time = Plat_FloatTime () + 5.0f;
}

void AutoTVRecord::Unload ()
{
	StopRecord ();

	m_demofile = "";
	m_recording = false;

	ConCommandHookListener::RemoveConCommandHookListener ( this );
}

bool AutoTVRecord::GotJob () const
{
	return true;
}

void AutoTVRecord::StartRecord ()
{
	if( m_recording ) return;
	if( !IsActive () ) return;

	if( IsTVPresent () )
	{
		m_recordtickcount = 0;
		basic_string mapname;
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			mapname = static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->mapname;
		}
		else
		{
			mapname = static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->mapname;
		}

		size_t const strip ( mapname.find_last_of ( "/\\" ) );
		if( strip != basic_string::npos ) mapname = mapname.c_str () + strip + 1;

		mapname.replace ( ":?\"<>|", '-' );

		m_expectedtvconfigchange = true;

		SourceSdk::InterfacesProxy::Call_ServerCommand ( basic_string ( "tv_record " ).append ( m_demofile ).append ( '\n' ).c_str () );
	}
}

void AutoTVRecord::OnStartRecord ( char const * filename )
{
	if( !m_recording )
	{
		m_demofile = filename;
		m_recording = true;
		m_recordtickcount = 0;

		if( m_expectedtvconfigchange )
		{
			m_started_current_record = true;
			m_expectedtvconfigchange = false;
			Logger::GetInstance ()->Msg<MSG_LOG> ( Helpers::format ( "Sent tv_record : Starting to record the game in %s.dem", filename ) );
		}
		else
		{
			m_started_current_record = false;
			Logger::GetInstance ()->Msg<MSG_LOG> ( Helpers::format ( "Caught tv_record : Starting to record the game in %s.dem", filename ) );
		}
	}
	else
	{
		DebugMessage ( "Caught tv_record, but already recording" );
	}
}

void AutoTVRecord::StopRecord ()
{
	if( !m_recording ) return;
	if( !m_started_current_record || !IsActive() ) return;

	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	m_expectedtvconfigchange = true;

	SourceSdk::InterfacesProxy::Call_ServerCommand ( "tv_stoprecord\n" );
	SourceSdk::InterfacesProxy::Call_ServerExecute ();
}

void AutoTVRecord::OnStopRecord ()
{
	if( m_recording )
	{
		m_recording = false;
		m_expectedtvconfigchange = false;
		m_started_current_record = false;
		Logger::GetInstance ()->Msg<MSG_LOG> ( Helpers::format ( "TV record ended in %s.dem with %u ticks (%f seconds)", m_demofile.c_str (), m_recordtickcount, m_recordtickcount * SourceSdk::InterfacesProxy::Call_GetTickInterval () ) );
	}
	else
	{
		DebugMessage ( "Caught tv_stoprecord or tv_stop, but not currently recording" );
	}
}

void AutoTVRecord::RT_OnTick ()
{
	++m_recordtickcount;
}

void AutoTVRecord::SetMinPlayers ( int min )
{
	m_minplayers = min;
}

void AutoTVRecord::SetRecordPrefix ( basic_string const & prefix )
{
	m_prefix = prefix;
}

size_t AutoTVRecord::GetRecordTick () const
{
	return m_recordtickcount;
}

bool AutoTVRecord::IsRecording () const
{
	return m_recording;
}

bool AutoTVRecord::IsTVPresent () const
{
	ProcessFilter::TVOnly filter_class;
	return PlayerHandler::const_iterator ( &filter_class ) != PlayerHandler::end ();
}

void AutoTVRecord::SpawnTV ()
{
	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	m_expectedtvconfigchange = true;

	SourceSdk::InterfacesProxy::Call_ServerCommand ( "tv_autorecord 0\n" );
	SourceSdk::InterfacesProxy::Call_ServerCommand ( "tv_enable 1\n" );

	m_expectedtvconfigchange = false;

	if( !IsTVPresent () )
	{
		if( m_spawn_once )
		{
			Logger::GetInstance ()->Msg<MSG_LOG> ( "TV not detected. Reloading the map ..." );

			basic_string mapname;
			if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
			{
				mapname = static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->mapname;
				size_t const strip ( mapname.find_last_of ( "/\\" ) );
				if( strip != basic_string::npos ) mapname = mapname.c_str () + strip + 1;
				SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "map %s\n", mapname.c_str () ) );
			}
			else
			{
				mapname = static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->mapname;
				SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "changelevel %s\n", mapname.c_str () ) );
			}

			m_spawn_once = false;
		}
		else
		{
			Logger::GetInstance ()->Msg<MSG_ERROR> ( "Was unable to spawn the TV." );
		}
	}
}

basic_string const & AutoTVRecord::GetRecordFilename () const
{
	return m_demofile;
}

void AutoTVRecord::SendTVChatMessage ( basic_string const & msg )
{
	ProcessFilter::TVOnly filter_class;
	for( PlayerHandler::const_iterator it ( &filter_class ); it != PlayerHandler::end (); it += &filter_class )
	{
		Helpers::tell ( it->GetEdict (), msg );
	}
}

bool AutoTVRecord::RT_ConCommandCallback ( PlayerHandler::const_iterator ph, void * cmd, const SourceSdk::CCommand & args )
{
	/*if( !m_expectedtvconfigchange || ph != PlayerHandler::end () )
	{
		Logger::GetInstance ()->Msg<MSG_LOG> ( "Intercepted unexpected TV configuration change." );
		return true;
	}*/

	char const * command_name ( SourceSdk::InterfacesProxy::ConCommand_GetName ( cmd ) + 3);

	if( *command_name == 'r' ) // tv_record
	{
		Assert ( strcmp ( command_name - 3, "tv_record" ) == 0 );

		if( args.ArgC >= 2 )
		{
			OnStartRecord ( args.Arg ( 1 ) );
		}
	}
	else
	{
		Assert ( strcmp ( command_name - 3, "tv_stoprecord" ) == 0 || strcmp ( command_name - 3, "tv_stop" ) == 0 );

		OnStopRecord ();
	}

	return false;
}
