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

#include <stdio.h>

#include "BaseSystem.h"

#include "Players/NczPlayerManager.h"
#include "Logger.h"

#include "vtabledumphelper.h"


/////////////////////////////////////////////////////////////////////////
// BaseSystem
/////////////////////////////////////////////////////////////////////////


BaseSystem::BaseSystem ( char const * const name, char const * const commands ) :
	ListMeClass (),
	m_name ( name ),
	m_cmd_list ( commands ),
	m_verbose ( false )
{

}

BaseSystem::~BaseSystem ()
{}

void BaseSystem::UnloadAllSystems ()
{
	BaseSystem* it ( GetFirst () );
	while( it != nullptr )
	{
		it->SetActive ( false );
		GetNext ( it );
	}
}

void BaseSystem::TryUnloadSystems ()
{
	BaseSystem* it ( GetFirst () );
	while( it != nullptr )
	{
		if( it->IsDynamic () )
		{
			if( !it->GotJob () ) it->SetActive ( false );
		}
		GetNext ( it );
	}
}

void BaseSystem::TryLoadSystems ()
{
	BaseSystem* it ( GetFirst () );
	while( it != nullptr )
	{
		if( it->IsDynamic () )
		{
			it->SetActive ( true );
		}
		GetNext ( it );
	}
}

void BaseSystem::InitSystems ()
{
	BaseSystem* it ( GetFirst () );
	while( it != nullptr )
	{
		it->Init ();
		GetNext ( it );
	}
}

BaseSystem * BaseSystem::FindSystemByName ( const char * name )
{
	BaseSystem* it ( GetFirst () );
	while( it != nullptr )
	{
		if( stricmp ( it->GetName (), name ) == 0 )
			return it;
		GetNext ( it );
	}
	return nullptr;
}

void BaseSystem::ncz_cmd_fn ( const SourceSdk::CCommand &args )
{
#ifdef DEBUG
	if (stricmp(args.Arg(1), "vtbl") == 0)
	{
		HelpMeeee();

		return;
	}
#endif
	if( args.ArgC () > 1 )
	{
		BaseSystem* it = GetFirst ();
		while( it != nullptr )
		{
			if( stricmp ( it->GetName (), args.Arg ( 1 ) ) == 0 )
			{
				if( Helpers::IsArgTrue(args.Arg(2)) )
				{
					if( it->IsStatic () )
					{
						g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "System %s is static and cannot be loaded or unloaded", it->GetName () ) );
					}
					else
					{
						it->SetConfig ( true );
						it->SetActive ( true );
					}
				}
				else if(Helpers::IsArgFalse(args.Arg(2)))
				{
					if( it->IsStatic () )
					{
						g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "System %s is static and cannot be loaded or unloaded", it->GetName () ) );
					}
					else
					{
						it->SetConfig ( false );
						it->SetActive ( false );
					}
				}
				else if( stricmp ( "verbose", args.Arg ( 2 ) ) == 0 )
				{
					if( args.ArgC () > 2 )
					{
						it->m_verbose = atoi ( args.Arg ( 3 ) );
						g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "System %s verbose level is now %d", it->GetName (), it->m_verbose ) );
					}
					return;
				}
#ifdef NCZ_USE_METRICS
				else if( Helpers::bStriEq ( "printmetrics", args.Arg ( 2 ) ) )
				{
					( *it )->GetMetrics ().PrintAll ();
				}
				else if( Helpers::bStriEq ( "resetmetrics", args.Arg ( 2 ) ) )
				{
					( *it )->GetMetrics ().ResetAll ();
				}
#endif
				else if( !it->sys_cmd_fn ( args ) )
				{
					g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "action %s not found.\nTry : %s", args.Arg ( 2 ), it->cmd_list () ) );
				}

				return;
			}
			GetNext ( it );
		}
		g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "System %s not found.", args.Arg ( 1 ) ) );
	}
	else
	{
		g_Logger.Msg<MSG_CMD_REPLY> ( "Usage: ncz system arg1 arg2 ...\nSystems list :");

		BaseSystem* it ( GetFirst () );
		while( it != nullptr )
		{
			basic_string prepared_message(it->GetName());
			if( it->IsStatic () )
			{
				prepared_message.append ( " (Static)\n" );
			}
			else if( it->IsActive () )
			{
				prepared_message.append ( " (Running)\n" );
			}
			else if( !it->IsEnabledByConfig () )
			{
				prepared_message.append ( " (Disabled manually)\n" );
			}
			else if( it->GetDisabledByConfigIni () )
			{
				prepared_message.append ( " (Disabled by config.ini)\n" );
			}
			else
			{
				prepared_message.append ( " (No task)\n" );
			}
			prepared_message.append ( Helpers::format( "\tCommands : %s", it->cmd_list () ) );
			GetNext ( it );
			g_Logger.Msg<MSG_CMD_REPLY>(prepared_message.c_str());
		}	
	}
}

/////////////////////////////////////////////////////////////////////////
// BaseStaticSystem
/////////////////////////////////////////////////////////////////////////

BaseStaticSystem::BaseStaticSystem ( char const * const name, char const * const commands ) :
	BaseSystem ( name, commands )
{}

BaseStaticSystem::~BaseStaticSystem ()
{}

/////////////////////////////////////////////////////////////////////////
// BaseDynamicSystem
/////////////////////////////////////////////////////////////////////////

BaseDynamicSystem::BaseDynamicSystem ( char const * const name, char const * const commands ) :
	BaseSystem ( name, commands ),
	m_isActive ( false ),
	m_isDisabled ( false ),
	m_configState ( true )
{}

BaseDynamicSystem::~BaseDynamicSystem ()
{}

void BaseDynamicSystem::SetActive ( bool active )
{
	if( IsActive () == active ) return;
	else if( active )
	{
		if( !GetDisabledByConfigIni () )
		{
			if( IsEnabledByConfig () )
			{
				// Should load
				if( this->GotJob () )
				{
					g_Logger.Msg<MSG_HINT> ( Helpers::format ( "Starting %s", GetName () ) );
					Load ();
					m_isActive = true;
				}
				else
				{
					//DebugMessage ( Helpers::format ( "System %s has nothing to do and will not be loaded", GetName () ) ); // That spam ...
				}
			}
			else
			{
				SystemVerbose1 ( Helpers::format ( "Wont start system %s : Disabled by server configuration", GetName () ) );
			}
		}
	}
	else
	{
		g_Logger.Msg<MSG_HINT> ( Helpers::format ( "Stoping %s", GetName () ) );
		Unload ();
		m_isActive = false;
	}
}

BaseTesterSystem::BaseTesterSystem(char const * const name, char const * const commands) :
	BaseDynamicSystem(name, commands),
	m_action_on_detection(DetectionAction_t::BAN_ASYNC)
{
}

BaseTesterSystem::~BaseTesterSystem()
{
}

bool BaseTesterSystem::sys_cmd_fn(const SourceSdk::CCommand & args)
{
	if (args.ArgC() >= 3)
	{
		if (stricmp(args.Arg(2), "setaction") == 0)
		{
			if (SetAction(args.Arg(3)))
			{
				basic_string current_action;
				GetAction(current_action);
				g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("Action on detection is %s", current_action.c_str()));
				return true;
			}
			else
			{
				g_Logger.Msg<MSG_CMD_REPLY>("SetAction Usage : BAN_ASYNC / BAN_NOW / KICK / LOG");
			}
		}
	}
	return false;
}

BaseBlockerSystem::BaseBlockerSystem(char const * const name, char const * const commands) :
	BaseDynamicSystem(name, commands)
{
}

BaseBlockerSystem::~BaseBlockerSystem()
{
}
