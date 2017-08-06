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

#include "Players/NczPlayerManager.h"
#include "Systems/AutoTVRecord.h"
#include "Systems/ConfigManager.h"

void Logger::SetBypassServerConsoleMsg(bool b)
{
	m_bypass_msg = b;
}

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

	if( g_TVWatcher.IsRecording () )
	{
		basic_string move_msg;
		move_msg.reserve ( 255 );
		move_msg.append ( Helpers::getStrDateTime ( "%x %X " ) );
		move_msg.append ( Helpers::format ( "[ Server Tick #%d, SourceTV:%s.dem : Tick #%d ] ", server_tick, g_TVWatcher.GetRecordFilename ().c_str (), g_TVWatcher.GetRecordTick () ) );
		move_msg.append ( copy_msg );
		m_current_memory_used += move_msg.capacity ();
		m_msg.AddToTail ( std::move( move_msg ) );
	}
	else
	{
		basic_string move_msg;
		move_msg.reserve ( 255 );
		move_msg.append ( Helpers::getStrDateTime ( "%x %X " ) );
		move_msg.append ( Helpers::format ( "[ Server Tick #%d] ", server_tick ) );
		move_msg.append ( copy_msg );
		m_current_memory_used += move_msg.capacity ();
		m_msg.AddToTail ( std::move ( move_msg ) );
	}

	if( m_always_flush || m_current_memory_used >= LOGGER_FORCE_FLUSH_MAX_MEMORY )
	{
		Flush ();
	}
	else
	{
		ProcessFilter::HumanAtLeastConnected filter_class;

		if( g_NczPlayerManager.GetPlayerCount ( &filter_class ) == 0 )
		{
			// We can flush right now.

			Flush ();
		}
	}
}

template <>
void Logger::Msg<MSG_CONSOLE> ( const char * msg, int verbose /*= 0*/ )
{
	if (!m_bypass_msg)
	{
		Tier0::Msg("%s%f %s\n", log_prolog.c_str(), Tier0::Plat_FloatTime(), msg);
	}
}

template <>
void Logger::Msg<MSG_CMD_REPLY> ( const char * msg, int verbose /*= 0*/ )
{
	if (!m_bypass_msg)
	{
		Tier0::Msg("%s\n", msg);
	}
}

template <>
void Logger::Msg<MSG_CHAT_ADMIN>(const char * msg, int verbose /*= 0*/)
{
	basic_string m(chat_prolog.c_str(), msg);

	if (!m_sm_chat || m_allow_chat == logger_chat_t::ADMIN_IDS)
	{
		int maxclients;
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			maxclients = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
		}
		else
		{
			maxclients = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
		}
		for (int i(1); i <= maxclients; i++)
		{
			SourceSdk::edict_t * ent_id(Helpers::PEntityOfEntIndex(i));
			SourceSdk::IPlayerInfo * const player(static_cast<SourceSdk::IPlayerInfo *>(SourceSdk::InterfacesProxy::Call_GetPlayerInfo(ent_id)));

			if (player)
			{
				if (player->IsConnected() && g_ConfigManager.IsAdmin(player->GetNetworkIDString()))
				{
					Helpers::tell(ent_id, m);
				}
			}
		}
	}
	else
	{
		m.reserve(m.size() + 9);
		m.insert_at_start("sm_chat ");
		m.append('\n');
		SourceSdk::InterfacesProxy::Call_ServerCommand(m.c_str());
	}
}

template <>
void Logger::Msg<MSG_CHAT> ( const char * msg, int verbose /*= 0*/ )
{
	//Msg<MSG_CONSOLE> ( msg );
	if (m_allow_chat == logger_chat_t::ON)
	{
		Helpers::chatprintf(basic_string(chat_prolog.c_str(), msg).c_str());
	}
	else if (m_allow_chat == logger_chat_t::ADMIN_IDS || m_allow_chat == logger_chat_t::ADMIN_AUTO)
	{
		Msg<MSG_CHAT_ADMIN>(msg);
	}
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
	Msg<MSG_CHAT>(msg);
}

template <>
void Logger::Msg<MSG_WARNING> ( const char * msg, int verbose /*= 0*/ )
{
	basic_string m("WARNING : ", msg);
	Push ( m.c_str () );

	Msg<MSG_CONSOLE> ( m.c_str() );
}

template <>
void Logger::Msg<MSG_ERROR> ( const char * msg, int verbose /*= 0*/ )
{
	basic_string m("ERROR : ", msg);
	Push(m.c_str());

	Msg<MSG_CONSOLE>(m.c_str());
}

template <>
void Logger::Msg<MSG_HINT> ( const char * msg, int verbose /*= 0*/ )
{
	//std::cerr << prolog.c_str () << Plat_FloatTime () << " : " << msg << '\n';
	Msg<MSG_CONSOLE> ( msg, verbose );
}

template <>
void Logger::Msg<MSG_VERBOSE1> ( const char * msg, int verbose /*= 0*/ )
{
	if( verbose == 1 )
	{
		Msg<MSG_CONSOLE> ( basic_string ( "VERBOSE1 : ", msg ).c_str() );
	}
}

template <>
void Logger::Msg<MSG_VERBOSE2> ( const char * msg, int verbose /*= 0*/ )
{
	if( verbose == 2 )
	{
		Msg<MSG_CONSOLE>(basic_string("VERBOSE2 : ", msg).c_str());
	}
}

template <>
void Logger::Msg<MSG_DEBUG> ( const char * msg, int verbose /*= 0*/ )
{
	Msg<MSG_CONSOLE> ( basic_string ( "DEBUG : ", msg ).c_str() );
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
		basic_string m1 ( log_prolog, Helpers::format("Can't write to logfile at %s ... Please check write access and if the directory exists.\n", path.c_str()));
		Msg<MSG_CONSOLE> ( m1.c_str() );
		SourceSdk::InterfacesProxy::Call_LogPrint ( m1.c_str () );
		size_t pos ( 0 );
		size_t const max ( m_msg.Size () );
		do
		{
			m1.reserve(log_prolog.size() + m_msg[pos].size());
			m1 = log_prolog;
			SourceSdk::InterfacesProxy::Call_LogPrint ( m1.append ( m_msg[ pos ] ).c_str () );
		}
		while( ++pos != max );
	}

	m_msg.RemoveAll ();
	m_current_memory_used = 0;
}

bool Logger::sys_cmd_fn ( const SourceSdk::CCommand &args )
{
	if( stricmp ( "alwaysflush", args.Arg ( 2 ) ) == 0 )
	{
		if( stricmp ( "on", args.Arg ( 3 ) ) == 0)
		{
			SetAlwaysFlush ( true );
			Msg<MSG_CMD_REPLY> ( "Logger AlwaysFlush is on" );
			return true;
		}
		else if( stricmp ( "off", args.Arg ( 3 ) ) == 0)
		{
			SetAlwaysFlush ( false );
			Msg<MSG_CMD_REPLY> ( "Logger AlwaysFlush is off" );
			return true;
		}
		else
		{
			Msg<MSG_CMD_REPLY> ( "Usage : On / Off" );
			return false;
		}
	}
	else if (stricmp("allowchat", args.Arg(2)) == 0 )
	{
		if (stricmp("on", args.Arg(3)) == 0)
		{
			m_allow_chat = logger_chat_t::ON;
			Msg<MSG_CMD_REPLY>("Logger AllowChat is on");
			return true;
		}
		else if (stricmp("off", args.Arg(3)) == 0)
		{
			m_allow_chat = logger_chat_t::OFF;
			Msg<MSG_CMD_REPLY>("Logger AllowChat is off");
			return true;
		}
		else if (stricmp("admin", args.Arg(3)) == 0)
		{
			m_allow_chat = logger_chat_t::ADMIN_AUTO;
			Msg<MSG_CMD_REPLY>("Logger AllowChat is admin");
			return true;
		}
		else if (stricmp("admin_ids", args.Arg(3)) == 0)
		{
			m_allow_chat = logger_chat_t::ADMIN_IDS;
			Msg<MSG_CMD_REPLY>("Logger AllowChat is admin_ids");
			return true;
		}
		else
		{
			Msg<MSG_CMD_REPLY>("Usage : On / Admin / Off");
			return false;
		}
	}
	else 
	{
		return false;
	}
}

void Logger::OnLevelInit()
{
	m_sm_chat = SourceSdk::InterfacesProxy::ICvar_FindCommand("sm_chat");
}

void Logger::SpewAssert ( char const * expr, char const * file, unsigned int line )
{
	basic_string msg ( Helpers::format ( "ASSERTION FAILED in %s:%u : %s", file, line, expr ) );
	g_Logger.m_always_flush = true;
	g_Logger.Msg<MSG_ERROR> ( msg.c_str(), 3 );
}

void Helpers::writeToLogfile ( const basic_string &text )
{
	g_Logger.Push ( text.c_str () );
}

Logger g_Logger;
