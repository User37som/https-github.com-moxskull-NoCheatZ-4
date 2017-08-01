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
	BaseDynamicSystem ( "AutoTVRecord", "Enable - Disable - Verbose - SetDemoPrefix - SetMinPlayers - SplitDemoBy" ),
	Singleton (),
	m_prefix ( "NoCheatZ-autorecords/" ),
	m_waitfortv_time ( 0.0f ),
	m_minplayers ( 1 ),
	m_splitrule ( demo_split_t::SPLIT_BY_MAP ),
	m_round_id (0),
	m_max_rounds (0),
	m_splittimer_seconds (0.0f),
	m_current_detected_players(0),
	m_spawn_once ( true )
{}

AutoTVRecord::~AutoTVRecord ()
{
	Unload ();
}

void AutoTVRecord::Init ()
{
	
}

void AutoTVRecord::OnRoundStart()
{
	if (IsActive())
	{
		if (m_splitrule == demo_split_t::SPLIT_BY_ROUNDS)
		{
			if (++m_round_id > m_max_rounds)
			{
				SplitRecord();
				m_round_id = 0;
			}
		}
	}
}

bool AutoTVRecord::sys_cmd_fn(const SourceSdk::CCommand & args)
{
	if (stricmp("setdemoprefix", args.Arg(2)) == 0)
	{
		if (args.ArgC() > 3)
		{
			basic_string prefix(args.Arg(3));
			prefix.replace(":?\"<>|%", '-');
			/// TODO : Sanitize the user input to avoid breakin attempts (top-level directory access)
			/// TODO : Test if we can write a test-file with this prefix before validating the user input
			/// TODO : Eventually move all the sanitize stuff inside SetRecordPrefix
			SetRecordPrefix(prefix);
			g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Prefix is \"%s\"", prefix.c_str()));
			return true;
		}
		g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Unable to set prefix. Current prefix is \"%s\"", m_prefix.c_str()));
		return false;
	}
	else if (stricmp("setminplayers", args.Arg(2)) == 0)
	{
		if (args.ArgC() > 3)
		{
			int min(atoi(args.Arg(3)));
			if (min > 0)
			{
				SetMinPlayers(min);
				g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("MinPlayers is \"%d\"", m_minplayers));
				return true;
			}
		}
		g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Missing agrument. Current minimum human players required to start a record is \"%d\"", m_minplayers));
		return false;
	}
	else if (stricmp("splitdemoby", args.Arg(2)) == 0)
	{
		if (args.ArgC() > 3)
		{
			if (stricmp("map", args.Arg(3)) == 0)
			{
				m_splitrule = demo_split_t::SPLIT_BY_MAP;
				g_Logger.Msg<MSG_CMD_REPLY>("Will split demos by map");
				RemoveTimer("autotv");
				TimerListener::RemoveTimerListener(this);
				return true;
			}
			else if (stricmp("rounds", args.Arg(3)) == 0)
			{
				m_splitrule = demo_split_t::SPLIT_BY_ROUNDS;
				if (args.ArgC() > 4)
				{
					m_max_rounds = atoi(args.Arg(4));
					if (m_max_rounds < 1) m_max_rounds = 1;
				}
				else
				{
					m_max_rounds = 1;
				}
				if (m_max_rounds > 1)
				{
					g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Will split demos every %d rounds", m_max_rounds));
				}
				else
				{
					g_Logger.Msg<MSG_CMD_REPLY>("Will split demos every round");
				}
				m_round_id = 0;
				RemoveTimer("autotv");
				TimerListener::RemoveTimerListener(this);
				return true;
			}
			else if (stricmp("time", args.Arg(3)) == 0)
			{
				if (args.ArgC() > 4)
				{
					m_splittimer_seconds = (float)atof(args.Arg(4));
					if (m_splittimer_seconds < 60.0f) m_splittimer_seconds = 60.0f;
				}
				else
				{
					g_Logger.Msg<MSG_CMD_REPLY>("Missing float argument");
					return false;
				}
				m_splitrule = demo_split_t::SPLIT_BY_TIMER_SECONDS;
				g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Will split demos every %f seconds", m_splittimer_seconds));
				RemoveTimer("autotv");
				TimerListener::AddTimerListener(this);
				AddTimer(m_splittimer_seconds, "autotv");
				return true;
			}
			/*else if (stricmp("detection", args.Arg(3)) == 0)
			{
				m_splitrule = demo_split_t::SPLIT_BY_DETECTION;
				g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Will record only when at least one detected player is in game", m_splittimer_seconds));
				RemoveTimer("autotv");
				TimerListener::RemoveTimerListener(this);
				if (m_current_detected_players)
				{
					StartRecord();
				}
				return true;
			}*/
		}
		g_Logger.Msg<MSG_CMD_REPLY>("SplitDemoBy Usage : Map / Detection / Rounds [optional number] / Time [seconds]");
		return false;
	}
	return false;
}

void AutoTVRecord::Load ()
{
	m_waitfortv_time = Tier0::Plat_FloatTime () + 5.0f;
	m_round_id = 0;
	if (m_splitrule == demo_split_t::SPLIT_BY_TIMER_SECONDS)
	{
		RemoveTimer("autotv");
		TimerListener::AddTimerListener(this);
		AddTimer(m_splittimer_seconds, "autotv");
	}
}

void AutoTVRecord::Unload ()
{
	StopRecord ();
	TimerListener::RemoveTimerListener(this);
}

bool AutoTVRecord::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;

	size_t condition_count(0);

	for (PlayerHandler::iterator it(&filter_class); it != PlayerHandler::end(); it+=&filter_class)
	{
		SourceSdk::IPlayerInfo * pInfo(it->GetPlayerInfo());
		if (pInfo)
		{
			if (pInfo->GetTeamIndex() > 1)
			{
				++condition_count;
			}
		}
	}

	return condition_count != 0;
}

void AutoTVRecord::RT_TimerCallback(char const * const timer_name)
{
	SplitRecord();
}

void AutoTVRecord::StartRecord ()
{
	if( g_TVWatcher.IsRecording() ) return;

	if( g_TVWatcher.IsTVPresent () && IsActive())
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

		mapname.replace ( ":?\"<>|%", '-' );

		SourceSdk::InterfacesProxy::Call_ServerExecute ();
		
		basic_string demofile ( Helpers::format ( "%s%s-%s", m_prefix.c_str(), mapname.c_str (), basic_string ( Helpers::getStrDateTime ( "%x_%X" ) ).replace ( "/\\:?\"<>|", '-' ).c_str () ) );
		
		SourceSdk::InterfacesProxy::Call_ServerCommand ( basic_string ( "tv_record " ).append ( demofile ).append ( '\n' ).c_str () );
		SourceSdk::InterfacesProxy::Call_ServerExecute ();
	}
}

void AutoTVRecord::StopRecord ()
{
	if( !g_TVWatcher.IsRecording() || ! IsActive()) return;

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
	if (IsActive())
	{
		SourceSdk::InterfacesProxy::Call_ServerExecute();

		SourceSdk::InterfacesProxy::Call_ServerCommand("tv_autorecord 0\n");
		SourceSdk::InterfacesProxy::Call_ServerCommand("tv_enable 1\n");

		if (!g_TVWatcher.IsTVPresent())
		{
			if (m_spawn_once)
			{
				g_Logger.Msg<MSG_LOG>("TV not detected. Reloading the map ...");

				basic_string mapname;
				if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
				{
					mapname = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
					size_t const strip(mapname.find_last_of("/\\"));
					if (strip != basic_string::npos) mapname = mapname.c_str() + strip + 1;
					SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("map %s\n", mapname.c_str()));
				}
				else
				{
					mapname = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
					SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("changelevel %s\n", mapname.c_str()));
				}

				m_spawn_once = false;
			}
			else
			{
				g_Logger.Msg<MSG_ERROR>("Was unable to spawn the TV.");
			}
		}
	}
}

AutoTVRecord g_AutoTVRecord;


/////////////////////////////////////////

TVWatcher::TVWatcher() :
	BaseStaticSystem("TVWatcher"),
	ConCommandHookListener(),
	Singleton(),
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

bool TVWatcher::RT_ConCommandCallback(PlayerHandler::iterator ph, void * const cmd, SourceSdk::CCommand const & args)
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

			g_Logger.Msg<MSG_LOG>(Helpers::format("Starting to record the game in %s", m_demofile.c_str()));
		}
	}
}

void TVWatcher::RecordEnded()
{
	if (IsRecording())
	{
		g_Logger.Msg<MSG_LOG>(Helpers::format("TV record ended in %s with %u ticks (%f seconds)", m_demofile.c_str(), m_recordtickcount, m_recordtickcount * SourceSdk::InterfacesProxy::Call_GetTickInterval()));
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
	return PlayerHandler::iterator(&filter_class) != PlayerHandler::end();
}

basic_string const & TVWatcher::GetRecordFilename() const
{
	return m_demofile;
}

void TVWatcher::SendTVChatMessage(basic_string const & msg)
{
	ProcessFilter::TVOnly filter_class;
	for (PlayerHandler::iterator it(&filter_class); it != PlayerHandler::end(); it += &filter_class)
	{
		Helpers::tell(it->GetEdict(), msg);
	}
}

TVWatcher g_TVWatcher;
