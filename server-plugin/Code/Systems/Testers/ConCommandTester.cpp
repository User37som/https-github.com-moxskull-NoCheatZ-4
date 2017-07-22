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

#include "ConCommandTester.h"

#include "Misc/Helpers.h"
#include "Systems/BanRequest.h"
#include "Systems/Logger.h"

ConCommandTester::ConCommandTester () :
	BaseTesterSystem ( "ConCommandTester" ),
	ConCommandHookListener (),
	playerdatahandler_class (),
	Singleton (),
	m_commands_list ()
{}

ConCommandTester::~ConCommandTester ()
{
	Unload ();
}

void ConCommandTester::Init ()
{
	InitDataStruct ();
}

void ConCommandTester::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	AddCommandInfo ( "ai_test_los", false );
	AddCommandInfo ( "changelevel", false, true );
	AddCommandInfo ( "cl_fullupdate", false );
	AddCommandInfo ( "dbghist_addline", false );
	AddCommandInfo ( "dbghist_dump", false );
	AddCommandInfo ( "drawcross", false );
	AddCommandInfo ( "drawline", false );
	AddCommandInfo ( "dump_entity_sizes", false );
	AddCommandInfo ( "dump_globals", false );
	AddCommandInfo ( "dump_panels", false );
	AddCommandInfo ( "dump_terrain", false );
	AddCommandInfo ( "dumpcountedstrings", false );
	AddCommandInfo ( "dumpentityfactories", false );
	AddCommandInfo ( "dumpeventqueue", false );
	AddCommandInfo ( "dumpgamestringtable", false );
	AddCommandInfo ( "editdemo", false );
	AddCommandInfo ( "endround", false );
	AddCommandInfo ( "groundlist", false );
	AddCommandInfo ( "listdeaths", false );
	AddCommandInfo ( "listmodels", false );
	AddCommandInfo ( "map_showspawnpoints", false );
	AddCommandInfo ( "mem_dump", false );
	AddCommandInfo ( "mp_dump_timers", false );
	AddCommandInfo ( "npc_ammo_deplete", false );
	AddCommandInfo ( "npc_heal", false );
	AddCommandInfo ( "npc_speakall", false );
	AddCommandInfo ( "npc_thinknow", false );
	AddCommandInfo ( "physics_budget", false );
	AddCommandInfo ( "physics_debug_entity", false );
	AddCommandInfo ( "physics_highlight_active", false );
	AddCommandInfo ( "physics_report_active", false );
	AddCommandInfo ( "physics_select", false );
	AddCommandInfo ( "q_sndrcn", false, true );
	AddCommandInfo ( "report_entities", false );
	AddCommandInfo ( "report_touchlinks", false );
	AddCommandInfo ( "report_simthinklist", false );
	AddCommandInfo ( "respawn_entities", false );
	AddCommandInfo ( "rr_reloadresponsesystems", false );
	AddCommandInfo ( "scene_flush", false );
	AddCommandInfo ( "send_me_rcon", false, true );
	AddCommandInfo ( "snd_digital_surround", false );
	AddCommandInfo ( "snd_restart", false );
	AddCommandInfo ( "soundlist", false );
	AddCommandInfo ( "soundscape_flush", false );
	AddCommandInfo ( "speed.toggle", false, true );
	AddCommandInfo ( "sv_benchmark_force_start", false );
	AddCommandInfo ( "sv_findsoundname", false );
	AddCommandInfo ( "sv_soundemitter_filecheck", false );
	AddCommandInfo ( "sv_soundemitter_flush", false );
	AddCommandInfo ( "sv_soundscape_printdebuginfo", false );
	AddCommandInfo ( "wc_update_entity", false );

	AddCommandInfo ( "buy", true );
	AddCommandInfo ( "buyammo1", true );
	AddCommandInfo ( "buyammo2", true );
	AddCommandInfo ( "spec_mode", true );
	AddCommandInfo ( "spec_next", true );
	AddCommandInfo ( "spec_prev", true );
	AddCommandInfo ( "use", true );
	AddCommandInfo ( "vmodenable", true );
	AddCommandInfo ( "vban", true );

	m_mycommands.AddToTail ( SourceSdk::InterfacesProxy::ICvar_FindCommand ( "ent_create" ) );
	m_mycommands.AddToTail ( SourceSdk::InterfacesProxy::ICvar_FindCommand ( "ent_fire" ) );
	m_mycommands.AddToTail ( SourceSdk::InterfacesProxy::ICvar_FindCommand ( "say" ) );
	m_mycommands.AddToTail ( SourceSdk::InterfacesProxy::ICvar_FindCommand ( "say_team" ) );

	ConCommandHookListener::RegisterConCommandHookListener ( this );
}

void ConCommandTester::Unload ()
{
	m_commands_list.RemoveAll ();
	ConCommandHookListener::RemoveConCommandHookListener ( this );
}

bool ConCommandTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void ConCommandTester::RT_AddPlayerCommand ( PlayerHandler::iterator ph, const basic_string& command )
{
	LastPlayerCommandsT* playerData ( this->GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	playerData->AddCmd ( command, false );

	const size_t cmd_count ( playerData->GetSpamCmdCount () );

	if( cmd_count > 32 )
	{
		ProcessDetectionAndTakeAction<Detection_CmdFlood::data_type>(Detection_CmdFlood(), playerData, ph, this);
	}
}

inline bool StriCmpOffset ( const char * s1, const char* s2, size_t offset )
{
	if( strlen ( s1 ) - offset < strlen ( s2 ) ) return false;

	for( s1 += offset; *s2; ++s1, ++s2 )
	{
		char k = *s1;
		if( k >= 'A' && k <= 'Z' ) k += 0x20;
		if( k != *s2 ) return false;
	}
	return true;
}

void ConCommandTester::AddCommandInfo ( const basic_string& name, const bool ignore/* = false*/, const bool ban/* = false*/ )
{
	m_commands_list.AddToTail ( CommandInfoS ( name, ignore, ban ) );
}

void ConCommandTester::RemoveCommandInfo ( const basic_string& name )
{
	CommandInfoS temp;
	temp.command_name = name;
	m_commands_list.FindAndRemove ( temp );
}

bool ConCommandTester::RT_TestPlayerCommand ( PlayerHandler::iterator ph, const basic_string& command )
{
	if( IsActive () )
	{
		// To lower
		basic_string lower_cmd ( command );
		//if( basic_string::IsValidMultibyteString ( lower_cmd ) )
		//{
			lower_cmd.lower ();

			size_t id ( 0 );
			size_t const max ( m_commands_list.Size () );
			for( CommandInfoT* cmd_test ( &m_commands_list[ id ] ); id != max; cmd_test = &m_commands_list[ ++id ] )
			{
				for( size_t x ( 0 ); x < lower_cmd.size (); ++x )
				{
					if( StriCmpOffset ( lower_cmd.c_str (), cmd_test->command_name.c_str (), x ) )
					{
						// Ignored cmds are always at the end of the set
						if( cmd_test->ignore ) return false;

						RT_AddPlayerCommand ( ph, command );

						ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), GetPlayerDataStructByIndex(ph.GetIndex()), ph, this);

						if( cmd_test->ban ) ph->Ban ( "ConCommand exploit" );
						else ph->Kick ( "ConCommand exploit" );
						return true;
					}
				}
			}
			RT_AddPlayerCommand ( ph, command );
		//}
		//else
		//{
		//	g_Logger.Msg<MSG_LOG> ( Helpers::format("Dropped invalid command from %s", ph->GetName () ));
		//	ph->Kick ( "Invalid ConCommand" );
		//	return true;
		//}
	}
	return false;
}

bool ConCommandTester::RT_TestPlayerCommand_Anon ( PlayerHandler::iterator ph, const basic_string& command )
{
	if( IsActive () )
	{
		// To lower
		basic_string lower_cmd ( command );
		//if( basic_string::IsValidMultibyteString ( lower_cmd ) )
		//{
			lower_cmd.lower ();

			size_t id ( 0 );
			size_t const max ( m_commands_list.Size () );
			for( CommandInfoT* cmd_test ( &m_commands_list[ id ] ); id != max; cmd_test = &m_commands_list[ ++id ] )
			{
				for( size_t x ( 0 ); x < lower_cmd.size (); ++x )
				{
					if( StriCmpOffset ( lower_cmd.c_str (), cmd_test->command_name.c_str (), x ) )
					{
						// Ignored cmds are always at the end of the set
						if( cmd_test->ignore ) return false;

						//RT_AddPlayerCommand ( ph, command );
						//Detection_CmdViolation pDetection;
						//pDetection.PrepareDetectionData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );
						//pDetection.PrepareDetectionLog ( *ph, this );
						//pDetection.Log ();

						//if( cmd_test->ban ) ph->Ban ( "ConCommand exploit" );
						//else ph->Kick ( "ConCommand exploit" );
						return true;
					}
				}
			}
			//RT_AddPlayerCommand ( ph, command );
		//}
		//else
		//{
			//g_Logger.Msg<MSG_LOG> ( Helpers::format ( "Dropped invalid command from %s", ph->GetName () ) );
			//ph->Kick ( "Invalid ConCommand" );
		//	return true;
		//}
	}
	return false;
}

bool ConCommandTester::RT_HookEntCallback ( PlayerHandler::iterator ph, const void* const command, const SourceSdk::CCommand & args )
{
	char cmd_str[ 512 ];
	strcpy_s ( cmd_str, 512 * sizeof ( char ), args.GetCommandString () );
	cmd_str[ 511 ] = '\0';
	const size_t cmd_len ( strlen ( cmd_str ) );
	char const * command_name ( SourceSdk::InterfacesProxy::ConCommand_GetName ( command ) );

	if( cmd_len > 500 )
	{
		g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
		ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
		return true;
	}

	for( size_t x ( 0 ); x < cmd_len - 3; ++x )
	{
		if( StriCmpOffset ( cmd_str, "point_", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "quit", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "exit", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "restart", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "rcon", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "mp_", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "taketimer", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "logic_", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
		if( StriCmpOffset ( cmd_str, "sv_", x ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
	}

	g_ConCommandTester.RT_AddPlayerCommand ( ph, cmd_str );
	return false;
}

bool ConCommandTester::RT_HookSayCallback ( PlayerHandler::iterator ph, const void* const command, const SourceSdk::CCommand & args )
{
	char cmd_str[ 256 ];
	strcpy_s ( cmd_str, 256 * sizeof ( char ), args.GetCommandString () );
	cmd_str[ 255 ] = '\0';
	const size_t cmd_len ( strlen ( cmd_str ) );
	char const * command_name ( SourceSdk::InterfacesProxy::ConCommand_GetName ( command ) );

	if( cmd_len > 250 )
	{
		g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
		ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
		return true;
	}

	size_t spacenum ( 0 );
	for( size_t x ( 0 ); x < 251; ++x )
	{
		const char k ( cmd_str[ x ] );
		if( k == '\0' ) break;

		if( k == ' ' )
		{
			if( ++spacenum > 64 )
			{
				g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
				ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
				return true;
			}
		}
		if( k < 0x20 && !( Helpers::GetUTF8Bytes ( &k ) > 1 ) )
		{
			g_ConCommandTester.RT_AddPlayerCommand ( ph, command_name );
			ProcessDetectionAndTakeAction<Detection_CmdViolation::data_type>(Detection_CmdViolation(), g_ConCommandTester.GetPlayerDataStructByIndex(ph.GetIndex()), ph, &g_ConCommandTester);
			return true;
		}
	}

	g_ConCommandTester.RT_AddPlayerCommand ( ph, cmd_str );
	return false;
}

bool ConCommandTester::RT_ConCommandCallback ( PlayerHandler::iterator ph, void* cmd, const SourceSdk::CCommand & args )
{
	char const * const command_name ( SourceSdk::InterfacesProxy::ConCommand_GetName ( cmd ) );

	if (!IsActive()) return false;

	if( ph != SlotStatus::INVALID ) /// https://github.com/L-EARN/NoCheatZ-4/issues/16#issuecomment-225543330
	{
		if( stricmp ( command_name, "ent_create" ) == 0 || stricmp ( command_name, "ent_fire" ) == 0 )
		{
			return RT_HookEntCallback ( ph, cmd, args );
		}
		else if( stricmp ( command_name, "say" ) == 0 || stricmp ( command_name, "say_team" ) == 0 )
		{
			return RT_HookSayCallback ( ph, cmd, args );
		}
		else
		{
			return false;
		}
	}
	else if( ph.GetIndex () == 0 ) // server : https://github.com/L-EARN/NoCheatZ-4/issues/98
	{
		return false;
	}
	else
	{
		g_Logger.Msg<MSG_ERROR> ( Helpers::format ( "ConCommandTester::ConCommandCallback : Intercepted a ConCommand because player class is not ready. Command name : %s\n", command_name ) );
		return true; // Always block the command if the plugin is not ready to test this player (player class not already created)
	}
}

ConCommandTester g_ConCommandTester;

basic_string Detection_CmdFlood::GetDataDump ()
{
	basic_string ret ( ":::: List of commands entered last second {" );
	size_t id ( 0 );
	size_t const max ( GetDataStruct ()->commands.Size () );
	for( PlayerConCommandS* it ( &( GetDataStruct ()->commands[ id ] ) ); id != max; it = &( GetDataStruct ()->commands[ ++id ] ) )
	{
		ret.append ( Helpers::format ( "\n:::::::: PlayerConCommandS {\n:::::::::::: ConCommand String : %s,\n:::::::::::: Is ConCommand Spam Ignored : %s,\n:::::::::::: Insertion Time %f\n::::::::}", it->cmd.c_str (), Helpers::boolToString ( it->isSpamIgnored ), it->time ) );
	}
	ret.append ( "\n:::: }" );
	return ret;
}
