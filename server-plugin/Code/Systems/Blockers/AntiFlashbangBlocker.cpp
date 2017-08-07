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

#include "AntiFlashbangBlocker.h"

#include <cmath>

#include "Interfaces/InterfacesProxy.h"

#include "Misc/EntityProps.h"
#include "Players/NczPlayerManager.h"
#include "Systems/Logger.h"
#include "Misc/Helpers.h"

AntiFlashbangBlocker::AntiFlashbangBlocker () :
	BaseBlockerSystem( "AntiFlashbangBlocker" ),
	IGameEventListener002 (),
	playerdatahandler_class (),
	SetTransmitHookListener (),
	Singleton ()
{
}

AntiFlashbangBlocker::~AntiFlashbangBlocker ()
{
	Unload ();
}

void AntiFlashbangBlocker::Init ()
{
	InitDataStruct ();
}

void AntiFlashbangBlocker::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_blind", true );
	SetTransmitHookListener::RegisterSetTransmitHookListener ( this, SystemPriority::AntiFlashbangBlocker );

	if (SourceSdk::InterfacesProxy::m_game != SourceSdk::CounterStrikeGlobalOffensive)
	{
		OnTickListener::RegisterOnTickListener(this);
	}
}

void AntiFlashbangBlocker::Unload ()
{
	SetTransmitHookListener::RemoveSetTransmitHookListener ( this );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->RemoveListener ( this );

	if (SourceSdk::InterfacesProxy::m_game != SourceSdk::CounterStrikeGlobalOffensive)
	{
		OnTickListener::RemoveOnTickListener(this);
	}
}

bool AntiFlashbangBlocker::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

bool AntiFlashbangBlocker::RT_SetTransmitCallback ( PlayerHandler::iterator sender, PlayerHandler::iterator receiver )
{
	SourceSdk::IPlayerInfo * const player_info ( receiver->GetPlayerInfo () );
	if( !player_info ) return false;

	if( player_info->IsFakeClient () || player_info->IsDead() )
	{
		return false;
	}

	FlashInfoT* const pInfo ( GetPlayerDataStruct ( *receiver ) );

	if( pInfo->flash_end_time != 0.0 )
	{
		if( pInfo->flash_end_time > Tier0::Plat_FloatTime () )
		{
			return true;
		}

		if (SourceSdk::InterfacesProxy::m_game != SourceSdk::CounterStrikeGlobalOffensive)
		{
			Helpers::FadeUser(receiver->GetEdict(), 0);
		}
		ResetPlayerDataStruct ( *receiver );
	}

	return false;
}

void AntiFlashbangBlocker::FireGameEvent ( SourceSdk::IGameEvent* ev ) // player_blind
{
	PlayerHandler::iterator ph ( g_NczPlayerManager.GetPlayerHandlerByUserId ( ev->GetInt ( "userid", 0 ) ) );
	if( !ph )
	{
		return;
	}

	if( ph >= SlotStatus::PLAYER_IN_TESTS )
	{
		FlashInfoT* const pInfo ( GetPlayerDataStruct ( *ph ) );
		const float flash_alpha ( *g_EntityProps.GetPropValue<float, PROP_FLASH_MAX_ALPHA> ( ph->GetEdict () ) );
		const float flash_duration ( *g_EntityProps.GetPropValue<float, PROP_FLASH_DURATION> ( ph->GetEdict () ) );

		DebugMessage ( Helpers::format ( "Player %s flash alpha %f, duration %f", ph->GetName(), flash_alpha, flash_duration ) );

		if( flash_alpha < 255.0f )
		{
			if (SourceSdk::InterfacesProxy::m_game != SourceSdk::CounterStrikeGlobalOffensive)
			{
				Helpers::FadeUser(ph->GetEdict(), 0);
			}
			ResetPlayerDataStruct ( *ph );
			return;
		}

		if( flash_duration > 2.9f )
		{
			pInfo->flash_end_time = Tier0::Plat_FloatTime () + flash_duration - 2.9f;
		}
		else
		{
			pInfo->flash_end_time = Tier0::Plat_FloatTime () + flash_duration / 10.0f;
		}

		if (SourceSdk::InterfacesProxy::m_game != SourceSdk::CounterStrikeGlobalOffensive)
		{
			Helpers::FadeUser(ph->GetEdict(), (short)floorf(flash_duration * 1000.0f));
		}
	}
}

void AntiFlashbangBlocker::RT_ProcessOnTick (double const & curtime )
{
	// not called with CSGO

	ProcessFilter::HumanAtLeastConnected filter_class;
	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph += &filter_class )
	{
		FlashInfoT* const pInfo ( GetPlayerDataStruct ( *ph ) );

		if( pInfo->flash_end_time != 0.0 )
		{
			if( pInfo->flash_end_time > curtime)
			{
				Helpers::FadeUser ( ph->GetEdict (), 0 );
			}
		}
	}
}

AntiFlashbangBlocker g_AntiFlashbangBlocker;
