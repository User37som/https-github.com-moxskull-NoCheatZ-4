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

#include "NczPlayerManager.h"

#include "Interfaces/usercmd.h"
#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h" // helpers, ifaces
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Systems/Logger.h"
#include "plugin.h"
#include "Systems/BaseSystem.h"
#include "Systems/AutoTVRecord.h"

PlayerHandler NczPlayerManager::FullHandlersList[ MAX_PLAYERS + 1 ];
PlayerHandler::const_iterator PlayerHandler::invalid ( NczPlayerManager::FullHandlersList );
PlayerHandler::const_iterator PlayerHandler::first ( invalid );
PlayerHandler::const_iterator PlayerHandler::last ( invalid );


//---------------------------------------------------------------------------------
// NczPlayerManager
//---------------------------------------------------------------------------------

NczPlayerManager::NczPlayerManager () :
	singleton_class (),
	m_max_index ( 0 )
{
	PlayerHandler::invalid = FullHandlersList;
	PlayerHandler::first = PlayerHandler::invalid;
	PlayerHandler::last = PlayerHandler::invalid;
}

NczPlayerManager::~NczPlayerManager ()
{
	OnLevelInit ();
}

void NczPlayerManager::OnLevelInit ()
{
	SourceSdk::InterfacesProxy::GetGameEventManager ()->RemoveListener ( this );
	_PLAYERS_LOOP_INIT
	{
		ph->Reset ();
	}
		_END_PLAYERS_LOOP_INIT
		m_max_index = 0;
	PlayerHandler::first = PlayerHandler::invalid;
	PlayerHandler::last = PlayerHandler::invalid;
}

void NczPlayerManager::LoadPlayerManager ()
{
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_death", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_team", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_spawn", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "round_end", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "round_freeze_end", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "bot_takeover", true );

	//Helpers::FastScan_EntList();
	Helpers::m_EdictList = Helpers::PEntityOfEntIndex ( 0 );

	//if(Helpers::m_EdictList)
	//{
	//int maxcl = Helpers::GetMaxClients();

	for( PlayerHandler::const_iterator ph ( PlayerHandler::begin () ); ph != PlayerHandler::end (); ++ph )
	{
		SourceSdk::edict_t* const pEntity ( Helpers::PEntityOfEntIndex ( ph.GetIndex () ) );
		if( Helpers::isValidEdict ( pEntity ) )
		{
			void * playerinfo ( SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( pEntity ) );
			if( playerinfo )
			{
				bool isfakeclient;
				if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
				{
					isfakeclient = static_cast< SourceSdk::IPlayerInfo_csgo* >( playerinfo )->IsFakeClient ();
				}
				else
				{
					isfakeclient = static_cast< SourceSdk::IPlayerInfo* >( playerinfo )->IsFakeClient ();
				}
				if( isfakeclient )
				{
					ph.GetHandler ()->status = SlotStatus::BOT;
					ph.GetHandler ()->playerClass = new NczPlayer ( ph.GetIndex () );
					m_max_index = ph.GetIndex ();
				}
				else if( static_cast< SourceSdk::IPlayerInfo* >( playerinfo )->IsConnected () )
				{
					ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
					ph.GetHandler ()->playerClass = new NczPlayer ( ph.GetIndex () );
					ph.GetHandler ()->playerClass->OnConnect ();
					m_max_index = ph.GetIndex ();
				}
			}
		}
	}

	//}

	if( m_max_index )
	{
		PlayerHandler::first = ( &FullHandlersList[ 1 ] );
		PlayerHandler::last = ( &FullHandlersList[ m_max_index ] );
	}
	else
	{
		PlayerHandler::first = PlayerHandler::invalid;
		PlayerHandler::last = PlayerHandler::invalid;
	}

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::ClientConnect ( SourceSdk::edict_t* pEntity )
{
	const int index ( Helpers::IndexOfEdict ( pEntity ) );
	Assert ( index );
	PlayerHandler& ph ( FullHandlersList[ index ] );
	Assert ( ph.status == SlotStatus::INVALID || ph.status == SlotStatus::PLAYER_CONNECTING );
	ph.playerClass = new NczPlayer ( index );
	// Should not be here, but heh ...
	//*PlayerRunCommandHookListener::GetLastUserCmd(ph.playerClass) = SourceSdk::CUserCmd();
	ph.status = SlotStatus::PLAYER_CONNECTING;
	ph.playerClass->OnConnect ();

	if( index > m_max_index ) m_max_index = index;

	PlayerHandler::first = ( &FullHandlersList[ 1 ] );
	PlayerHandler::last = ( &FullHandlersList[ m_max_index ] );

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::ClientActive ( SourceSdk::edict_t* pEntity )
{
	const int index ( Helpers::IndexOfEdict ( pEntity ) );
	Assert ( index );
	PlayerHandler& ph ( FullHandlersList[ index ] );
	if( ph.status == SlotStatus::INVALID ) // Bots don't call ClientConnect
	{
		ph.playerClass = new NczPlayer ( index );
		__assume ( ph.playerClass != nullptr );
		ph.playerClass->m_playerinfo = ( SourceSdk::IPlayerInfo * )SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( ph.playerClass->m_edict );
		Assert ( ph.playerClass->m_playerinfo );
#undef GetClassName
		if( strcmp ( pEntity->GetClassName (), "player" ) == 0 )
			ph.status = SlotStatus::TV;
		else
			ph.status = SlotStatus::BOT;
	}
	else
	{
		Assert ( ph.status == SlotStatus::PLAYER_CONNECTING );
		ph.status = SlotStatus::PLAYER_CONNECTED;
		ph.playerClass->m_playerinfo = ( SourceSdk::IPlayerInfo * )SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( ph.playerClass->m_edict );
		Assert ( ph.playerClass->m_playerinfo );
	}

	if( index > m_max_index ) m_max_index = index;

	PlayerHandler::first = ( &FullHandlersList[ 1 ] );
	PlayerHandler::last = ( &FullHandlersList[ m_max_index ] );

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::ClientDisconnect ( SourceSdk::edict_t* pEntity )
{
	const int index ( Helpers::IndexOfEdict ( pEntity ) );
	Assert ( index );
	FullHandlersList[ index ].Reset ();

	while( m_max_index > 0 && FullHandlersList[ m_max_index ].status == SlotStatus::INVALID )
		--m_max_index;

	if( m_max_index )
	{
		PlayerHandler::first = ( &FullHandlersList[ 1 ] );
		PlayerHandler::last = ( &FullHandlersList[ m_max_index ] );
	}
	else
	{
		PlayerHandler::first = PlayerHandler::invalid;
		PlayerHandler::last = PlayerHandler::invalid;
	}

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::FireGameEvent ( SourceSdk::IGameEvent* ev )
/*
player_death
player_team
player_spawn
round_freeze_end
round_end
bot_takeover
*/
{
	const char* event_name ( ev->GetName () + 6 );
	const int maxcl ( m_max_index );

	if( *event_name == 'e' ) // round_end
	{
		for( int x ( 1 ); x <= maxcl; ++x )
		{
			PlayerHandler::const_iterator ph ( x );
			if( ph == SlotStatus::PLAYER_IN_TESTS )
			{
				ph.GetHandler()->status = SlotStatus::PLAYER_CONNECTED;
			}
			else if( ph == SlotStatus::PLAYER_IN_TESTS_TAKEOVER )
			{
				ph->GetTakeover ()->StopBotTakeover ();
				ph->StopBotTakeover ();
				ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
			}
		}

		ProcessFilter::HumanAtLeastConnected filter_class;
		if( GetPlayerCount ( &filter_class ) == 0 ) AutoTVRecord::GetInstance ()->StopRecord ();

		BaseSystem::ManageSystems ();
		Logger::GetInstance ()->Flush ();

		return;
	}
	/*else*/ if( *event_name == 'f' ) // round_freeze_end = round_start
	{
		for( int x ( 1 ); x <= maxcl; ++x )
		{
			PlayerHandler& ph ( FullHandlersList[ x ] );
			if( ph.status == SlotStatus::INVALID ) continue;
			if( ph.status == SlotStatus::PLAYER_CONNECTED )
			{
				SourceSdk::IPlayerInfo * const pinfo ( ph.playerClass->GetPlayerInfo () );
				if( pinfo )
				{
					if( pinfo->GetTeamIndex () > 1 )
					{
						ph.in_tests_time = Plat_FloatTime () + 1.0f;
					}
					else
					{
						ph.status = SlotStatus::PLAYER_CONNECTED;
						ph.in_tests_time = std::numeric_limits<float>::max ();
					}
				}
			}
		}

		BaseSystem::ManageSystems ();
		return;
	}

	PlayerHandler::const_iterator ph ( GetPlayerHandlerByUserId ( ev->GetInt ( "userid" ) ) );

	if( *event_name == 'k' ) // bot_takeover
	{
		PlayerHandler::const_iterator bh1 ( GetPlayerHandlerByUserId ( ev->GetInt ( "botid" ) ) );
		PlayerHandler::const_iterator bh2 ( GetPlayerHandlerByUserId ( ev->GetInt ( "index" ) ) );
		DebugMessage ( Helpers::format ( "Player %s taking control of bot %s or bot %s", ph->GetName (), bh1->GetName (), bh2->GetName () ));

		ph->EnterBotTakeover ( ev->GetInt ( "index" ) );
		bh2->EnterBotTakeover ( ph.GetIndex () );

		ph.GetHandler ()->status = SlotStatus::PLAYER_IN_TESTS_TAKEOVER;
		ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();

		return;
	}

	++event_name;

	if( *event_name == 's' ) // player_spawn
	{
		if( ph > SlotStatus::BOT )
		{
			SourceSdk::IPlayerInfo * const pinfo ( ph->GetPlayerInfo () );
			if( pinfo )
			{
				if( pinfo->GetTeamIndex () > 1 )
				{
					ph.GetHandler ()->in_tests_time = Plat_FloatTime () + 3.0f;
				}
				else
				{
					ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
					ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
				}
			}
		}
		else if( ph == SlotStatus::BOT )
		{
			if( ph->IsControllingBot () )
			{
				ph->GetTakeover ()->StopBotTakeover (); // release link from player to bot
				ph->GetTakeover ().GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph->GetTakeover ().GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
				ph->StopBotTakeover (); // release link from bot to player
			}
		}
		BaseSystem::ManageSystems ();
		return;
	}
	if( *event_name == 't' ) // player_team
	{
		if( ph > SlotStatus::BOT )
		{
			if( ev->GetInt ( "teamid" ) > 1 )
			{
				ph.GetHandler ()->in_tests_time = Plat_FloatTime () + 3.0f;
			}
			else
			{
				ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
			}
		}
		else if( ph == SlotStatus::BOT )
		{
			if( ph->IsControllingBot () )
			{
				ph->GetTakeover ()->StopBotTakeover (); // release link from player to bot
				ph->GetTakeover ().GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph->GetTakeover ().GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
				ph->StopBotTakeover (); // release link from bot to player
			}
		}
		BaseSystem::ManageSystems ();
		return;
	}
	//else // player_death
	//{

	if( ph == SlotStatus::BOT )
	{
		if( ph->IsControllingBot () ) // is bot controlled
		{
			ph->GetTakeover ()->StopBotTakeover (); // release link from player to bot
			ph->GetTakeover ().GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
			ph->GetTakeover ().GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
			ph->StopBotTakeover (); // release link from bot to player
		}
	}
	if( ph </*=*/ SlotStatus::PLAYER_CONNECTED ) /// fixed :  https://github.com/L-EARN/NoCheatZ-4/issues/79#issuecomment-240174457
		return;
	ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
	ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
	BaseSystem::ManageSystems ();
	//}
}

void NczPlayerManager::DeclareKickedPlayer ( int const slot )
{
	FullHandlersList[ slot ].status = SlotStatus::KICK;
}

void NczPlayerManager::RT_Think ( float const curtime )
{
	while( m_max_index > 0 && FullHandlersList[ m_max_index ].status == SlotStatus::INVALID )
		--m_max_index;

	if( m_max_index )
	{
		PlayerHandler::first = ( &FullHandlersList[ 1 ] );
		PlayerHandler::last = ( &FullHandlersList[ m_max_index ] );
	}
	else
	{
		PlayerHandler::first = PlayerHandler::invalid;
		PlayerHandler::last = PlayerHandler::invalid;
	}

	const int maxcl ( m_max_index );

	int in_tests_count = 0;
	for( int x = 1; x <= maxcl; ++x )
	{
		PlayerHandler& ph ( FullHandlersList[ x ] );
		if( ph.status <= SlotStatus::PLAYER_CONNECTING ) continue;

		if( curtime > ph.in_tests_time )
		{
			ph.status = SlotStatus::PLAYER_IN_TESTS;
			++in_tests_count;
		}
		else
		{
			ph.status = SlotStatus::PLAYER_CONNECTED;
		}
	}
	if( in_tests_count >= 1 ) AutoTVRecord::GetInstance ()->StartRecord ();
}

PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByBasePlayer ( void * const BasePlayer ) const
{
	SourceSdk::edict_t * tEdict;
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		if( it )
		{
			tEdict = it->GetEdict ();
			if( Helpers::isValidEdict ( tEdict ) )
			{
				if( tEdict->GetUnknown () == BasePlayer ) return it;
			}
		}
	}

	return PlayerHandler::end ();
}

PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerBySteamID ( const char * steamid ) const
{
	const char * tSteamId;

	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		if( it )
		{
			tSteamId = it->GetSteamID ();
			if( strcmp ( tSteamId, steamid ) == 0 ) return it;
		}
	}

	return PlayerHandler::end ();
}

PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByName ( const char * playerName ) const
{
	const char * tName;

	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		if( it )
		{
			tName = it->GetName ();
			if( strcmp ( tName, playerName ) ) return it;
		}
	}

	return PlayerHandler::end ();
}

short NczPlayerManager::GetPlayerCount ( BaseProcessFilter const * const filter ) const
{
	short count ( 0 );
	for( PlayerHandler::const_iterator it ( filter ); it != PlayerHandler::end (); it += filter )
	{
		++count;
	}
	return count;
}
