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

#include "AntiSmokeBlocker.h"

#include <cmath>

#include "Interfaces/InterfacesProxy.h"

#include "Misc/MathCache.h"
#include "Misc/EntityProps.h"
#include "Players/NczPlayerManager.h"
#include "Systems/ConfigManager.h"

AntiSmokeBlocker::AntiSmokeBlocker() :
	BaseBlockerSystem("AntiSmokeBlocker"),
	IGameEventListener002(),
	OnTickListener(),
	playerdatahandler_class(),
	SetTransmitHookListener(),
	Singleton()
{
}

AntiSmokeBlocker::~AntiSmokeBlocker ()
{
	Unload ();
}

void AntiSmokeBlocker::Init ()
{
	InitDataStruct ();
}

void AntiSmokeBlocker::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "smokegrenade_detonate", true );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "round_start", true );
	OnTickListener::RegisterOnTickListener ( this );
	SetTransmitHookListener::RegisterSetTransmitHookListener ( this, SystemPriority::AntiSmokeBlocker );
}

void AntiSmokeBlocker::Unload ()
{
	SetTransmitHookListener::RemoveSetTransmitHookListener ( this );
	OnTickListener::RemoveOnTickListener ( this );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->RemoveListener ( this );

	SmokeListT::elem_t* it ( m_smokes.GetFirst () );
	while( it != nullptr )
	{
		it = m_smokes.Remove ( it );
	}
}

bool AntiSmokeBlocker::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void AntiSmokeBlocker::RT_ProcessOnTick (double const & curtime )
{
	SmokeListT::elem_t* it ( m_smokes.GetFirst () );

	// remove old smokes
	while( it != nullptr )
	{
		if( curtime - ( it->m_value.bang_time + g_ConfigManager.m_smoke_time ) > 0.0f )
			it = m_smokes.Remove ( it );
		else it = it->m_next;
	}

	ST_R_STATIC SmokeInfoT empty;
	ResetAll ( &empty );

	it = m_smokes.GetFirst ();
	if( it == nullptr ) return;

	// Test if players are immersed in smoke
	ProcessFilter::InTestsNoBot l1_filter;
	ProcessFilter::InTestsOrBot l2_filter;
	for( PlayerHandler::iterator ph ( &l1_filter ); ph != PlayerHandler::end (); ph+=&l1_filter )
	{
		SourceSdk::Vector delta, other_delta;

		MathInfo const & x_math ( g_MathCache.RT_GetCachedMaths ( ph.GetIndex () ) );

		do // At this stage, m_smokes ! empty
		{
			if( curtime - it->m_value.bang_time > g_ConfigManager.m_smoke_timetobang )
			{
				SourceSdk::vec_t dst;
				SourceSdk::VectorDistanceSqr ( x_math.m_eyepos, it->m_value.pos, delta, dst );
				if( dst < g_ConfigManager.m_innersmoke_radius_sqr )
				{
					GetPlayerDataStructByIndex ( ph.GetIndex () )->is_in_smoke = true;
				}

				/* Players can't see eachother if they are behind a smoke */

				const SourceSdk::vec_t ang_smoke ( tanf ( g_ConfigManager.m_smoke_radius / sqrtf ( dst ) ) );
				SourceSdk::VectorNorm ( delta );

				for( PlayerHandler::iterator other_ph ( &l2_filter ); other_ph != PlayerHandler::end (); other_ph+=&l2_filter )
				{
					if( ph == other_ph ) continue;

					MathInfo const & y_math ( g_MathCache.RT_GetCachedMaths ( other_ph.GetIndex () ) );

					// Is he behind the smoke against us ?

					SourceSdk::vec_t other_dst;
					SourceSdk::VectorDistanceSqr ( x_math.m_eyepos, y_math.m_abs_origin, other_delta, other_dst );
					if( dst + g_ConfigManager.m_smoke_radius < other_dst )
					{
						// Hidden by the hull of the smoke ?

						SourceSdk::VectorNorm ( other_delta );

						SourceSdk::vec_t dp;
						SourceSdk::VectorDotProduct ( other_delta, delta, dp );
						const SourceSdk::vec_t angle_player ( fabs ( acos ( dp ) ) );

						if( angle_player < ang_smoke )
						{
							GetPlayerDataStructByIndex ( ph.GetIndex () )->can_not_see_this_player[ other_ph.GetIndex () ] = true;
						}
					}
				}
			}
			it = it->m_next;
		}
		while( it != nullptr );
		it = m_smokes.GetFirst ();
	}
}

bool AntiSmokeBlocker::RT_SetTransmitCallback ( PlayerHandler::iterator sender, PlayerHandler::iterator receiver )
{
	//if(!receiver) return false;

	if( GetPlayerDataStructByIndex ( receiver.GetIndex () )->is_in_smoke )
		return true;

	if( GetPlayerDataStructByIndex ( receiver.GetIndex () )->can_not_see_this_player[ sender.GetIndex () ] == true )
		return true;

	return false;
}

void AntiSmokeBlocker::FireGameEvent ( SourceSdk::IGameEvent * ev )
{

	if( ev->GetName ()[ 0 ] == 's' ) // smokegrenade_detonate
	{
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			SourceSdk::Vector t1 ( reinterpret_cast< SourceSdk::IGameEvent_csgo* >( ev )->GetFloat ( "x" ),
								   reinterpret_cast< SourceSdk::IGameEvent_csgo* >( ev )->GetFloat ( "y" ),
								   reinterpret_cast< SourceSdk::IGameEvent_csgo* >( ev )->GetFloat ( "z" ) );
			m_smokes.Add ( t1 );
		}
		else
		{
			SourceSdk::Vector t1 ( ev->GetFloat ( "x" ), ev->GetFloat ( "y" ), ev->GetFloat ( "z" ) );
			m_smokes.Add ( t1 );
		}

		return;
	}

	// round_start

	SmokeListT::elem_t* it ( m_smokes.GetFirst () );
	while( it != nullptr )
	{
		it = m_smokes.Remove ( it );
	}
}

AntiSmokeBlocker g_AntiSmokeBlocker;
