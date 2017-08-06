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
PlayerHandler::iterator PlayerHandler::invalid ( NczPlayerManager::FullHandlersList );
PlayerHandler::iterator PlayerHandler::first ( invalid );
PlayerHandler::iterator PlayerHandler::last ( invalid );


//---------------------------------------------------------------------------------
// NczPlayerManager
//---------------------------------------------------------------------------------

void NczPlayerManager::ResetRange()
{
	while (m_max_index > 0 && FullHandlersList[m_max_index].status == SlotStatus::INVALID)
		--m_max_index;

	if (m_max_index)
	{
		size_t id_start(1);

		while (FullHandlersList[id_start].status != SlotStatus::INVALID)
			++id_start;

		PlayerHandler::first = (&FullHandlersList[id_start]);
		PlayerHandler::last = (&FullHandlersList[m_max_index]);
	}
	else
	{
		PlayerHandler::first = PlayerHandler::invalid;
		PlayerHandler::last = PlayerHandler::invalid;
	}
}

NczPlayerManager::NczPlayerManager () :
	Singleton (),
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
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_connect", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_disconnect", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "round_end", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "round_freeze_end", true );
	//SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "bot_takeover", true );

	Helpers::m_EdictList = Helpers::PEntityOfEntIndex ( 0 );

	if (Helpers::m_EdictList != nullptr)
	{
		int index(1);
		do
		{
			SourceSdk::edict_t * pedict = Helpers::PEntityOfEntIndex(index);
			PlayerHandler& ph(FullHandlersList[index]);

#undef GetClassName
			if (Helpers::isValidEdict(pedict) && pedict->GetClassName() && strcmp(pedict->GetClassName(), "player") == 0)
			{
				SourceSdk::IPlayerInfo * pinfo = (SourceSdk::IPlayerInfo *)SourceSdk::InterfacesProxy::Call_GetPlayerInfo(pedict);
				if (pinfo)
				{
					if (pinfo->IsConnected())
					{
						m_max_index = index;
						ph.playerClass = new NczPlayer(index);
						ph.playerClass->m_playerinfo = pinfo;
						if (pinfo->IsFakeClient())
						{
							if (pinfo->IsHLTV())
							{
								ph.status = SlotStatus::TV;
							}
							else
							{
								ph.status = SlotStatus::BOT;
							}
						}
						else
						{
							ph.playerClass->OnConnect();
							ph.status = SlotStatus::PLAYER_CONNECTED;
							if (!pinfo->IsDead())
							{
								ph.in_tests_time = Tier0::Plat_FloatTime() + 1.0;
							}
							else
							{
								ph.in_tests_time = std::numeric_limits<double>::max();
							}
						}
					}
				}
			}
		} while (++index <= MAX_PLAYERS);
	}

	ResetRange();

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::ClientConnect ( SourceSdk::edict_t* pEntity )
{
	const int index ( Helpers::IndexOfEdict ( pEntity ) );
	LoggerAssert ( index );
	PlayerHandler& ph ( FullHandlersList[ index ] );
	LoggerAssert ( ph.status == SlotStatus::INVALID || ph.status == SlotStatus::PLAYER_CONNECTING );
	if (ph.playerClass)
		ph.Reset();
	ph.playerClass = new NczPlayer ( index );
	ph.status = SlotStatus::PLAYER_CONNECTING;
	ph.playerClass->OnConnect ();

	if( index > m_max_index ) m_max_index = index;

	ResetRange();

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::ClientActive ( SourceSdk::edict_t* pEntity )
{
	const int index ( Helpers::IndexOfEdict ( pEntity ) );
	LoggerAssert ( index );
	PlayerHandler& ph ( FullHandlersList[ index ] );
	if( ph.status == SlotStatus::INVALID ) // Bots don't call ClientConnect
	{
		ph.playerClass = new NczPlayer ( index );
		__assume ( ph.playerClass != nullptr );
		ph.playerClass->m_playerinfo = ( SourceSdk::IPlayerInfo * )SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( ph.playerClass->m_edict );
		LoggerAssert ( ph.playerClass->m_playerinfo );
#undef GetClassName
		if( strcmp ( pEntity->GetClassName (), "player" ) == 0 )
			ph.status = SlotStatus::TV;
		else
			ph.status = SlotStatus::BOT;
	}
	else
	{
		LoggerAssert ( ph.status == SlotStatus::PLAYER_CONNECTING );
		ph.status = SlotStatus::PLAYER_CONNECTED;
		ph.playerClass->m_playerinfo = ( SourceSdk::IPlayerInfo * )SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( ph.playerClass->m_edict );
		LoggerAssert ( ph.playerClass->m_playerinfo );
	}

	if( index > m_max_index ) m_max_index = index;

	ResetRange();

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::ClientDisconnect ( SourceSdk::edict_t* pEntity )
{
	const int index ( Helpers::IndexOfEdict ( pEntity ) );
	LoggerAssert ( index );

	FullHandlersList[ index ].Reset ();

	ResetRange();

	BaseSystem::ManageSystems ();
}

void NczPlayerManager::FireGameEvent ( SourceSdk::IGameEvent* ev )
/*
player_death
player_team
player_spawn
player_connect
player_disconnect
round_freeze_end
round_end
bot_takeover
*/
{
	const char* event_name ( ev->GetName () + 6 );
	const int maxcl ( m_max_index );
	double const curtime(Tier0::Plat_FloatTime());

	if( *event_name == 'e' ) // round_end
	{
		DebugMessage("event round_end");
		for( int x ( 1 ); x <= maxcl; ++x )
		{
			PlayerHandler::iterator ph ( x );
			if( ph == SlotStatus::PLAYER_IN_TESTS )
			{
				ph.GetHandler()->status = SlotStatus::PLAYER_CONNECTED;
				ph.GetHandler()->in_tests_time = std::numeric_limits<float>::max ();
				DebugMessage(Helpers::format("Players %s : Status changed from PLAYER_IN_TESTS to PLAYER_CONNECTED", ph->GetName()));
			}
			/*else if( ph == SlotStatus::PLAYER_IN_TESTS_TAKEOVER )
			{
				ph->GetTakeover ()->StopBotTakeover ();
				ph->StopBotTakeover ();
				ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
			}*/
		}

		//ProcessFilter::HumanAtLeastConnected filter_class;
		//if( GetPlayerCount ( &filter_class ) == 0 ) g_AutoTVRecord.StopRecord ();

		BaseSystem::ManageSystems ();

		g_AutoTVRecord.OnRoundStart();

		g_Logger.Flush ();

		return;
	}
	/*else*/ if( *event_name == 'f' ) // round_freeze_end = round_start
	{
		DebugMessage("event round_freeze_end");
		for( int x ( 1 ); x <= maxcl; ++x )
		{
			PlayerHandler& ph ( FullHandlersList[ x ] );
			if( ph.status == SlotStatus::INVALID ) continue;
			if( ph.status >= SlotStatus::PLAYER_CONNECTED )
			{
				ph.status = SlotStatus::PLAYER_CONNECTED;
				SourceSdk::IPlayerInfo * const pinfo ( ph.playerClass->GetPlayerInfo () );
				if( pinfo )
				{
					if( pinfo->GetTeamIndex () > 1 )
					{
						DebugMessage(Helpers::format("Players %s : Will enter in status PLAYER_IN_TESTS in 1 second", ph.playerClass->GetName()));
						ph.in_tests_time = curtime + 1.0f;
					}
					else
					{
						ph.in_tests_time = std::numeric_limits<float>::max ();
					}
				}
			}
		}

		BaseSystem::ManageSystems ();
		return;
	}

	PlayerHandler::iterator ph ( GetPlayerHandlerByUserId ( ev->GetInt ( "userid" ) ) );

	if( *event_name == 'k' ) // bot_takeover
	{
		DebugMessage(Helpers::format("event bot_takeover : %s -> %s", ph->GetName (), GetPlayerHandlerByUserId ( ev->GetInt ( "botid" ) )->GetName ()));
		//PlayerHandler::iterator bh1 ( GetPlayerHandlerByUserId ( ev->GetInt ( "botid" ) ) );
		//PlayerHandler::iterator bh2 ( GetPlayerHandlerByUserId (  ) );
		//DebugMessage ( Helpers::format ( "Player %s taking control of bot %s or bot %d", ph->GetName (), bh1->GetName (), ev->GetInt ( "index" ) ));

		//ph->EnterBotTakeover ( ev->GetInt ( "index" ) );
		//bh2->EnterBotTakeover ( ph.GetIndex () );

		//ph.GetHandler ()->status = SlotStatus::PLAYER_IN_TESTS_TAKEOVER;
		//ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();

		return;
	}

	++event_name;

	if( *event_name == 'c' ) // player_connect
	{
		if( ev->GetBool ( "bot" ) == false )
		{
			if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
			{
				SourceSdk::IGameEvent_csgo* rev ( reinterpret_cast< SourceSdk::IGameEvent_csgo* >( ev ) );
				g_Logger.Msg<MSG_LOG> ( Helpers::format ( "Player connect : %s [%s]", rev->GetString ( "name", "unknown-name" ), rev->GetString ( "networkid", "unknown-networkid" ) ) );
			}
			else
			{
				g_Logger.Msg<MSG_LOG> ( Helpers::format ( "Player connect : %s [%s - %s]", ev->GetString ( "name", "unknown-name" ), ev->GetString ( "networkid", "unknown-networkid" ), ev->GetString ( "address", "unknown-address" ) ) );
			}
		}

		return;
	}
	if( *(event_name + 1) == 'i' ) // player_disconnect
	{
		if( ev->GetBool ( "bot" ) == false )
		{
			if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
			{
				SourceSdk::IGameEvent_csgo* rev ( reinterpret_cast< SourceSdk::IGameEvent_csgo* >( ev ) );
				if( !rev->IsEmpty ( "name" ) )
				{
					g_Logger.Msg<MSG_LOG> ( Helpers::format ( "Player disconnect : %s [%s] -> Reason : %s", rev->GetString ( "name", "unknown-name" ), rev->GetString ( "networkid", "unknown-networkid" ), rev->GetString ( "reason", "unknown-reason" ) ) );
				}
			}
			else
			{
				g_Logger.Msg<MSG_LOG> ( Helpers::format ( "Player disconnect : %s [%s - %s] -> Reason : %s", ev->GetString ( "name", "unknown-name" ), ev->GetString ( "networkid", "unknown-networkid" ), ev->GetString ( "address", "unknown-address" ), ev->GetString ( "reason", "unknown-reason" ) ) );
			}
		}

		return;
	}
	if( *event_name == 's' ) // player_spawn(ed)
	{
		DebugMessage(Helpers::format("event player_spawn : %s", ph->GetName()));
		if( ph > SlotStatus::BOT )
		{
			SourceSdk::IPlayerInfo * const pinfo ( ph->GetPlayerInfo () );
			if( pinfo )
			{
				if( pinfo->GetTeamIndex () > 1 )
				{
					DebugMessage(Helpers::format("Players %s : Will enter in status PLAYER_IN_TESTS in 3 seconds", ph->GetName()));
					ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
					ph.GetHandler ()->in_tests_time = curtime + 3.0f;
				}
				else
				{
					DebugMessage(Helpers::format("Players %s : Status forced to PLAYER_CONNECTED", ph->GetName()));
					ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
					ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
				}
			}
		}
		else if( ph == SlotStatus::BOT )
		{
			/*if( ph->IsControllingBot () )
			{
				ph->GetTakeover ()->StopBotTakeover (); // release link from player to bot
				ph->GetTakeover ().GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph->GetTakeover ().GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
				ph->StopBotTakeover (); // release link from bot to player
			}*/
		}
		BaseSystem::ManageSystems ();
		return;
	}
	if( *event_name == 't' ) // player_team
	{
		DebugMessage("event player_team");
		if( ph > SlotStatus::BOT )
		{
			if( ev->GetInt ( "teamid" ) > 1 )
			{
				DebugMessage(Helpers::format("Players %s : Will enter in status PLAYER_IN_TESTS in 3 seconds", ph->GetName()));
				ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph.GetHandler ()->in_tests_time = curtime + 3.0f;
			}
			else
			{
				DebugMessage(Helpers::format("Players %s : Status forced to PLAYER_CONNECTED", ph->GetName()));
				ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
			}
		}
		else if( ph == SlotStatus::BOT )
		{
			/*if( ph->IsControllingBot () )
			{
				ph->GetTakeover ()->StopBotTakeover (); // release link from player to bot
				ph->GetTakeover ().GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
				ph->GetTakeover ().GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
				ph->StopBotTakeover (); // release link from bot to player
			}*/
		}
		BaseSystem::ManageSystems ();
		return;
	}
	//else // player_death
	//{

	DebugMessage(Helpers::format("event player_death : %s", ph->GetName()));
	if( ph == SlotStatus::BOT )
	{
		/*if( ph->IsControllingBot () ) // is bot controlled
		{
			ph->GetTakeover ()->StopBotTakeover (); // release link from player to bot
			ph->GetTakeover ().GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
			ph->GetTakeover ().GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
			ph->StopBotTakeover (); // release link from bot to player
		}*/
	}
	if( ph < SlotStatus::PLAYER_CONNECTED ) /// fixed : https://github.com/L-EARN/NoCheatZ-4/issues/79#issuecomment-240174457	
		return;
	DebugMessage(Helpers::format("Players %s : Status forced to PLAYER_CONNECTED", ph->GetName()));
	ph.GetHandler ()->status = SlotStatus::PLAYER_CONNECTED;
	ph.GetHandler ()->in_tests_time = std::numeric_limits<float>::max ();
	BaseSystem::ManageSystems ();
	//}
}

void NczPlayerManager::DeclareKickedPlayer ( int const slot )
{
	if(slot)
		FullHandlersList[ slot ].status = SlotStatus::KICK;
}

void NczPlayerManager::RT_Think (double const & curtime )
{
	ResetRange();

	const int maxcl ( m_max_index );

	int in_tests_count = 0;
	for( int x = 1; x <= maxcl; ++x )
	{
		PlayerHandler& ph ( FullHandlersList[ x ] );
		if( ph.status <= SlotStatus::PLAYER_CONNECTING ) continue;
		
		if( ph.status == SlotStatus::PLAYER_IN_TESTS )
		{
			if( curtime < ph.in_tests_time )
			{
				DebugMessage(Helpers::format("Players %s : Status changed from PLAYER_IN_TESTS to PLAYER_CONNECTED", ph.playerClass->GetName()));
				ph.status = SlotStatus::PLAYER_CONNECTED;
			}
			else
			{
				++in_tests_count;
			}
		}
		else if( curtime > ph.in_tests_time )
		{
			DebugMessage(Helpers::format("Players %s : Status changed from PLAYER_CONNECTED to PLAYER_IN_TESTS", ph.playerClass->GetName()));
			ph.status = SlotStatus::PLAYER_IN_TESTS;
			++in_tests_count;
		}
	}
	if( in_tests_count >= g_AutoTVRecord.GetMinPlayers()) g_AutoTVRecord.StartRecord ();
}

PlayerHandler::iterator NczPlayerManager::GetPlayerHandlerBySteamID ( const char * steamid ) const
{
	const char * tSteamId;

	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		if( it )
		{
			tSteamId = it->GetSteamID ();
			if( strcmp ( tSteamId, steamid ) == 0 ) return it;
		}
	}

	return PlayerHandler::end ();
}

PlayerHandler::iterator NczPlayerManager::GetPlayerHandlerByName ( const char * playerName ) const
{
	const char * tName;

	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
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
	for( PlayerHandler::iterator it ( filter ); it != PlayerHandler::end (); it += filter )
	{
		++count;
	}
	return count;
}

NczPlayerManager g_NczPlayerManager;
