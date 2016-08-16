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
	if( args.ArgC () > 1 )
	{
		BaseSystem* it = GetFirst ();
		while( it != nullptr )
		{
			if( stricmp ( it->GetName (), args.Arg ( 1 ) ) == 0 )
			{
				if( stricmp ( "enable", args.Arg ( 2 ) ) == 0 )
				{
					if( it->IsStatic () )
					{
						printf ( "System %s is static and cannot be loaded or unloaded\n", it->GetName () );
					}
					else
					{
						it->SetConfig ( true );
						it->SetActive ( true );
					}
				}
				else if( stricmp ( "disable", args.Arg ( 2 ) ) == 0 )
				{
					if( it->IsStatic () )
					{
						printf ( "System %s is static and cannot be loaded or unloaded\n", it->GetName () );
					}
					else
					{
						it->SetConfig ( false );
						it->SetActive ( false );
					}
				}
				else if( stricmp ( "on", args.Arg ( 2 ) ) == 0 )
				{
					if( it->IsStatic () )
					{
						printf ( "System %s is static and cannot be loaded or unloaded\n", it->GetName () );
					}
					else
					{
						it->SetConfig ( true );
						it->SetActive ( true );
					}
				}
				else if( stricmp ( "off", args.Arg ( 2 ) ) == 0 )
				{
					if( it->IsStatic () )
					{
						printf ( "System %s is static and cannot be loaded or unloaded\n", it->GetName () );
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
					printf ( "action %s not found.\nTry : %s\n", args.Arg ( 2 ), it->cmd_list () );
				}

				return;
			}
			GetNext ( it );
		}
		printf ( "System %s not found.\n", args.Arg ( 1 ) );
	}
	else
	{
		printf ( "Usage: ncz system arg1 arg2 ...\n" );
		printf ( "Systems list :\n" );
		BaseSystem* it ( GetFirst () );
		while( it != nullptr )
		{
			printf ( "%s", it->GetName () );
			if( it->IsStatic () )
			{
				printf ( " (Static)\n" );
			}
			else if( it->IsActive () )
			{
				printf ( " (Running)\n" );
			}
			else if( !it->IsEnabledByConfig () )
			{
				printf ( " (Disabled manually)\n" );
			}
			else if( it->GetDisabledByConfigIni () )
			{
				printf ( " (Disabled by config.ini)\n" );
			}
			else
			{
				printf ( " (Sleeping - Waiting for players)\n" );
			}
			printf ( "\tCommands : %s\n", it->cmd_list () );
			GetNext ( it );
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
					Logger::GetInstance ()->Msg<MSG_HINT> ( Helpers::format ( "Starting %s", GetName () ) );
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
		Logger::GetInstance ()->Msg<MSG_HINT> ( Helpers::format ( "Stoping %s", GetName () ) );
		m_isActive = false;
		Unload ();
	}
}
