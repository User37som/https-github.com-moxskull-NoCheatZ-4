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

#include "Interfaces/InterfacesProxy.h"

#include "Players/NczPlayerManager.h"
#include "Logger.h"

AutoTVRecord::AutoTVRecord () :
	BaseDynamicSystem ( "AutoTVRecord", "Enable - Disable - Verbose" ),
	singleton_class (),
	m_prefix ( "NoCheatZ-autorecords/" ),
	m_waitfortv_time ( 0.0f ),
	m_minplayers ( 1 ),
	m_spawn_once ( true )
{}

AutoTVRecord::~AutoTVRecord ()
{
	Unload ();
}

void AutoTVRecord::Init ()
{
	
}

void AutoTVRecord::Load ()
{
	m_waitfortv_time = Plat_FloatTime () + 5.0f;
}

void AutoTVRecord::Unload ()
{
	StopRecord ();
}

bool AutoTVRecord::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::const_iterator it(&filter_class);
	// Return if we have job to do or not ...
	return it != PlayerHandler::end();
}

void AutoTVRecord::StartRecord ()
{
	if( TVWatcher::GetInstance()->IsRecording() ) return;

	if( TVWatcher::GetInstance()->IsTVPresent () )
	{
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

		SourceSdk::InterfacesProxy::Call_ServerExecute ();
		
		basic_string demofile ( Helpers::format ( "%s%s-%s", "", mapname.c_str (), basic_string ( Helpers::getStrDateTime ( "%x_%X" ) ).replace ( "/\\:?\"<>|", '-' ).c_str () ) );
		
		SourceSdk::InterfacesProxy::Call_ServerCommand ( basic_string ( "tv_record " ).append ( demofile ).append ( '\n' ).c_str () );
		SourceSdk::InterfacesProxy::Call_ServerExecute ();
	}
}

void AutoTVRecord::StopRecord ()
{
	if( !TVWatcher::GetInstance()->IsRecording()) return;

	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	SourceSdk::InterfacesProxy::Call_ServerCommand ( "tv_stoprecord\n" );
	SourceSdk::InterfacesProxy::Call_ServerExecute ();
}

void AutoTVRecord::SetMinPlayers ( int min )
{
	m_minplayers = min;
}

void AutoTVRecord::SetRecordPrefix ( basic_string const & prefix )
{
	m_prefix = prefix;
}

void AutoTVRecord::SpawnTV ()
{
	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	SourceSdk::InterfacesProxy::Call_ServerCommand ( "tv_autorecord 0\n" );
	SourceSdk::InterfacesProxy::Call_ServerCommand ( "tv_enable 1\n" );

	if( !TVWatcher::GetInstance()->IsTVPresent () )
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


/////////////////////////////////////////

TVWatcher::TVWatcher() :
	BaseStaticSystem("TVWatcher"),
	ConCommandHookListener(),
	singleton_class(),
	m_demofile(""),
	m_recordtickcount(0),
	m_recording(false)
{
}

TVWatcher::~TVWatcher()
{
	ConCommandHookListener::RemoveConCommandHookListener(this);
}

void TVWatcher::Init()
{
	m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_record"));
	m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_stoprecord"));

	ConCommandHookListener::RegisterConCommandHookListener(this);
}

bool TVWatcher::RT_ConCommandCallback(PlayerHandler::const_iterator ph, void * const cmd, SourceSdk::CCommand const & args)
{
	if (args.ArgC() >= 2)
	{
		if (stricmp(args.Arg(0), "tv_record") == 0)
		{
			RecordStarted(args);
		}
	}
	else if (stricmp(args.Arg(0), "tv_stoprecord") == 0)
	{
		RecordEnded();
	}

	return false;
}

void TVWatcher::RecordStarted(SourceSdk::CCommand const & args)
{
	if (!IsRecording())
	{
		if (IsTVPresent())
		{
			m_demofile = args.Arg(1); // TODO : Test if file can be opened for writing
			m_recording = true;
			m_recordtickcount = 0;

			Logger::GetInstance()->Msg<MSG_LOG>(Helpers::format("Starting to record the game in %s", m_demofile.c_str()));
		}
	}
}

void TVWatcher::RecordEnded()
{
	if (IsRecording())
	{
		Logger::GetInstance()->Msg<MSG_LOG>(Helpers::format("TV record ended in %s with %u ticks (%f seconds)", m_demofile.c_str(), m_recordtickcount, m_recordtickcount * SourceSdk::InterfacesProxy::Call_GetTickInterval()));
		m_demofile = "";
		m_recording = false;
	}
}

void TVWatcher::RT_OnTick()
{
	++m_recordtickcount;
}

size_t TVWatcher::GetRecordTick() const
{
	return m_recordtickcount;
}

bool TVWatcher::IsRecording() const
{
	return m_recording;
}

bool TVWatcher::IsTVPresent() const
{
	ProcessFilter::TVOnly filter_class;
	return PlayerHandler::const_iterator(&filter_class) != PlayerHandler::end();
}

basic_string const & TVWatcher::GetRecordFilename() const
{
	return m_demofile;
}

void TVWatcher::SendTVChatMessage(basic_string const & msg)
{
	ProcessFilter::TVOnly filter_class;
	for (PlayerHandler::const_iterator it(&filter_class); it != PlayerHandler::end(); it += &filter_class)
	{
		Helpers::tell(it->GetEdict(), msg);
	}
}
