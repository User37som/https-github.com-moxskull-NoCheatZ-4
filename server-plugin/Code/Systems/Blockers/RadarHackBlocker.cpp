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

#include "RadarHackBlocker.h"

#include "Interfaces/InterfacesProxy.h"

#include "Misc/EntityProps.h"
#include "Misc/UserMsg.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Misc/MathCache.h"
#include "Systems/AutoTVRecord.h"

RadarHackBlocker::RadarHackBlocker () :
	BaseBlockerSystem( "RadarHackBlocker" ),
	OnTickListener (),
	playerdatahandler_class (),
	Singleton (),
	ThinkPostHookListener (),
	UserMessageHookListener (),
	m_players_spotted ( nullptr ),
	m_bomb_spotted ( nullptr ),
	m_cvar_forcecamera ( nullptr ),
	m_next_process ( 0.0f )
{

}

RadarHackBlocker::~RadarHackBlocker ()
{

}

void RadarHackBlocker::OnMapStart ()
{
	if( !GetDisabledByConfigIni () )
	{
		m_players_spotted = nullptr;
		m_bomb_spotted = nullptr;

		for( int x ( 0 ); x < MAX_EDICTS; ++x )
		{
			SourceSdk::edict_t * const ent ( Helpers::PEntityOfEntIndex ( x ) );
			if( Helpers::isValidEdict ( ent ) )
			{
				if( ent->GetClassName () != nullptr )
				{
#undef GetClassName
					if( basic_string ( "cs_player_manager" ).operator==( ent->GetClassName () ) )
					{
						m_players_spotted = g_EntityProps.GetPropValue<bool, PROP_PLAYER_SPOTTED> ( ent );
						m_bomb_spotted = g_EntityProps.GetPropValue<bool, PROP_BOMB_SPOTTED> ( ent );
						ThinkPostHookListener::HookThinkPost ( ent );
						break;
					}
				}
			}
		}
	}
}

void RadarHackBlocker::Init ()
{
	m_next_process = 0.0;
	for( int x ( 0 ); x < MAX_PLAYERS; ++x )
		m_dataStruct[ x ] = ClientRadarData ( x );
}

void RadarHackBlocker::Load ()
{
	m_cvar_forcecamera = SourceSdk::InterfacesProxy::ICvar_FindVar ( "mp_forcecamera" );

	ThinkPostHookListener::RegisterThinkPostHookListener ( this );
	UserMessageHookListener::RegisterUserMessageHookListener ( this );
	OnTickListener::RegisterOnTickListener ( this );
}

void RadarHackBlocker::Unload ()
{
	OnTickListener::RemoveOnTickListener ( this );
	UserMessageHookListener::RemoveUserMessageHookListener ( this );
	ThinkPostHookListener::RemoveThinkPostHookListener ( this );
}

bool RadarHackBlocker::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void RadarHackBlocker::RT_ThinkPostCallback ( SourceSdk::edict_t const * const pent )
{
	ProcessFilter::HumanAtLeastConnectedOrBot const filter_class;

	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph += &filter_class )
	{
		int const index ( ph.GetIndex () );
		ClientRadarData * pData ( GetPlayerDataStructByIndex ( index ) );
		if( pData->m_last_spotted_status != m_players_spotted[ index ] )
		{
			pData->m_last_spotted_status = m_players_spotted[ index ];

			if( pData->m_last_spotted_status )
			{
				RT_UpdatePlayerData ( *ph );
				RT_ProcessEntity ( ph->GetEdict () );
			}
			else
			{
				//pData->m_next_update = curtime + 1.0f;
				//UpdatePlayerData(ph->playerClass);
				//ProcessEntity(Helpers::PEntityOfEntIndex(x));
			}
		}
	}
}

bool RadarHackBlocker::RT_SendUserMessageCallback ( SourceSdk::IRecipientFilter const &, int const message_id, google::protobuf::Message const & )
{
	return ( message_id == CS_UM_ProcessSpottedEntityUpdate );
}

bool RadarHackBlocker::RT_UserMessageBeginCallback ( SourceSdk::IRecipientFilter const * const, int const message_id )
{
	return ( message_id == UpdateRadar );
}

void RadarHackBlocker::RT_MessageEndCallback(SourceSdk::IRecipientFilter const * const, int const, SourceSdk::bf_write * buffer)
{
	
}

void RadarHackBlocker::RT_SendApproximativeRadarUpdate ( MRecipientFilter & filter, ClientRadarData const * pData ) const
{
	MathInfo const & player_maths ( g_MathCache.RT_GetCachedMaths ( pData->m_origin_index ) );

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate* pBuffer = ( CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate * ) g_Cstrike15UsermessageHelpers.GetPrototype ( CS_UM_ProcessSpottedEntityUpdate )->New ();

		pBuffer->set_entity_idx ( pData->m_origin_index );
		pBuffer->set_class_id ( 35 );
		pBuffer->set_origin_x ( ( int32_t ) ( player_maths.m_abs_origin.x * 0.25f ) );
		pBuffer->set_origin_y ( ( int32_t ) ( player_maths.m_abs_origin.y * 0.25f ) );
		pBuffer->set_origin_z ( 0 );
		pBuffer->set_angle_y ( ( int32_t ) player_maths.m_eyeangles.y );
		//pBuffer->set_defuser();
		//pBuffer->set_player_has_defuser();
		//pBuffer->set_player_has_c4();

		SourceSdk::InterfacesProxy::Call_SendUserMessage ( &( filter ), CS_UM_ProcessSpottedEntityUpdate, *pBuffer );
		delete pBuffer;
	}
	else
	{
		SourceSdk::bf_write *pBuffer ( SourceSdk::InterfacesProxy::Call_UserMessageBegin ( &( filter ), /*eUserMsg::UpdateRadar*/ 28 ) );

		SourceSdk::BfWriteByte ( pBuffer, pData->m_origin_index );
		SourceSdk::BfWriteNBits<int32_t, 13> ( pBuffer, ( int32_t ) ( std::round ( player_maths.m_abs_origin.x * 0.25f ) ) );
		SourceSdk::BfWriteNBits<int32_t, 13> ( pBuffer, ( int32_t ) ( std::round ( player_maths.m_abs_origin.y * 0.25f ) ) );
		SourceSdk::BfWriteNBits<int32_t, 13> ( pBuffer, 0 );
		SourceSdk::BfWriteNBits<int32_t, 9> ( pBuffer, ( int32_t ) ( std::round ( player_maths.m_eyeangles.y ) ) );

		SourceSdk::BfWriteByte ( pBuffer, 0 );
		SourceSdk::InterfacesProxy::Call_MessageEnd ();
	}
}

void RadarHackBlocker::RT_SendRandomRadarUpdate ( MRecipientFilter & filter, ClientRadarData const * pData ) const
{
	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate* pBuffer ( ( CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate * ) g_Cstrike15UsermessageHelpers.GetPrototype ( CS_UM_ProcessSpottedEntityUpdate )->New () );

		pBuffer->set_entity_idx ( pData->m_origin_index );
		pBuffer->set_class_id ( 35 );
		pBuffer->set_origin_x ( ( int32_t ) std::round ( ( float ) std::rand () ) );
		pBuffer->set_origin_y ( ( int32_t ) std::round ( ( float ) std::rand () ) );
		pBuffer->set_origin_z ( ( int32_t ) std::round ( ( float ) std::rand () ) );
		pBuffer->set_angle_y ( ( int32_t ) std::round ( ( float ) std::rand () ) );
		pBuffer->set_defuser ( false );
		pBuffer->set_player_has_defuser ( false );
		pBuffer->set_player_has_c4 ( false );

		SourceSdk::InterfacesProxy::Call_SendUserMessage ( &( filter ), CS_UM_ProcessSpottedEntityUpdate, *pBuffer );
		delete pBuffer;
	}
	else
	{
		SourceSdk::bf_write *pBuffer ( SourceSdk::InterfacesProxy::Call_UserMessageBegin ( &( filter ), /*eUserMsg::UpdateRadar*/ 28 ) );

		SourceSdk::BfWriteByte ( pBuffer, pData->m_origin_index );
		SourceSdk::BfWriteNBits<int32_t, 13> ( pBuffer, ( int32_t ) ( std::round ( ( float ) std::rand () ) ) );
		SourceSdk::BfWriteNBits<int32_t, 13> ( pBuffer, ( int32_t ) ( std::round ( ( float ) std::rand () ) ) );
		SourceSdk::BfWriteNBits<int32_t, 13> ( pBuffer, ( int32_t ) ( std::round ( ( float ) std::rand () ) ) );
		SourceSdk::BfWriteNBits<int32_t, 9> ( pBuffer, ( int32_t ) ( std::round ( ( float ) std::rand () ) ) );

		SourceSdk::BfWriteByte ( pBuffer, 0 );
		SourceSdk::InterfacesProxy::Call_MessageEnd ();
	}
}

void RadarHackBlocker::RT_ProcessEntity ( SourceSdk::edict_t const * const pent )
{
	ClientRadarData * pData ( GetPlayerDataStructByIndex ( Helpers::IndexOfEdict ( pent ) ) );

	static MRecipientFilter filter;
	filter.RemoveAll ();

	filter.SetReliable ( false );

	if( pData->m_last_spotted_status == true )
	{
		/*
			Entity is spotted -> send approximative data to all players
		*/

		filter.AddAllPlayers ( g_NczPlayerManager.GetMaxIndex () );

		RT_SendApproximativeRadarUpdate ( filter, pData );
	}
	else
	{
		/*
			Entity is not spotted -> send approximative data to teammates and spectators, random for others.
		*/


		filter.AddTeam ( pData->m_team );
		filter.AddTeam ( TEAM_SPECTATOR );
		filter.AddTeam ( TEAM_NONE );

		filter.RemoveRecipient ( pData->m_origin_index );

		RT_SendApproximativeRadarUpdate ( filter, pData );

		filter.RemoveAll ();

		if( pData->m_team == TEAM_1 )
		{
			filter.AddTeam ( TEAM_2 );
		}
		else if( pData->m_team == TEAM_2 )
		{
			filter.AddTeam ( TEAM_1 );
		}
		else
		{
			return;
		}

		RT_SendRandomRadarUpdate ( filter, pData );
	}
}

void RadarHackBlocker::RT_UpdatePlayerData ( NczPlayer* pPlayer )
{
	ClientRadarData * pData ( GetPlayerDataStruct ( pPlayer ) );

	SourceSdk::IPlayerInfo * const playerinfo ( pPlayer->GetPlayerInfo () );
	if( playerinfo == nullptr ) return;

	pData->m_origin_index = pPlayer->GetIndex ();
	pData->m_team = playerinfo->GetTeamIndex ();
}

void RadarHackBlocker::RT_ProcessOnTick ( float const & curtime )
{
	ProcessFilter::HumanAtLeastConnectedOrBot const filter_class;

	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph += &filter_class )
	{
		int const index ( ph.GetIndex () );

		ClientRadarData * pData ( GetPlayerDataStructByIndex ( index ) );

		if( pData->m_last_spotted_status )
		{
			RT_UpdatePlayerData ( *ph );
			RT_ProcessEntity ( ph->GetEdict () );
		}
		else
		{
			if( curtime > m_next_process )
			{
				RT_UpdatePlayerData ( *ph );
				RT_ProcessEntity ( ph->GetEdict () );
			}
		}
	}

	if( curtime > m_next_process )
	{
		m_next_process = curtime + 2.0f;
	}
}

RadarHackBlocker g_RadarHackBlocker;
