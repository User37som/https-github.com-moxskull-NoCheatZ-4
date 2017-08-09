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

#include <cmath>

#include "WallhackBlocker.h"

#include "Interfaces/InterfacesProxy.h"
#include "Interfaces/bspflags.h"
#include "Interfaces/iserverunknown.h"

#include "Misc/EntityProps.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Misc/MathCache.h"

WallhackBlocker::WallhackBlocker () :
	BaseBlockerSystem( "WallhackBlocker", "Enable - Disable - Verbose - SetFFAMode" ),
	OnTickListener (),
	playerdatahandler_class (),
	SetTransmitHookListener (),
	Singleton (),
	WeaponHookListener (),
	m_weapon_owner (),
	m_viscache (),
	m_disable_shadows ( nullptr ),
	m_shadow_direction ( nullptr ),
	m_shadow_maxdist ( nullptr ),
	m_ffamode(false)
{
}

WallhackBlocker::~WallhackBlocker ()
{
	Unload ();
}

void WallhackBlocker::Init ()
{}

void WallhackBlocker::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	m_viscache.Invalidate ();
	memset ( m_weapon_owner, 0, MAX_EDICTS * sizeof ( NczPlayer* ) );
	WeaponHookListener::RegisterWeaponHookListener ( this );
	SetTransmitHookListener::RegisterSetTransmitHookListener ( this, SystemPriority::WallhackBlocker );
	OnTickListener::RegisterOnTickListener ( this );
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, SystemPriority::WallhackBlocker, SlotStatus::PLAYER_CONNECTED);

	SourceSdk::InterfacesProxy::Call_ServerCommand("sv_occlude_players 0\n");
}

void WallhackBlocker::Unload ()
{
	SetTransmitHookListener::RemoveSetTransmitHookListener ( this );
	WeaponHookListener::RemoveWeaponHookListener ( this );
	OnTickListener::RemoveOnTickListener ( this );
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);

	memset ( m_weapon_owner, 0, MAX_EDICTS * sizeof ( NczPlayer* ) );
	m_viscache.Invalidate ();
}

bool WallhackBlocker::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void WallhackBlocker::OnMapStart ()
{
	m_disable_shadows = nullptr;
	m_shadow_direction = nullptr;
	m_shadow_maxdist = nullptr;
	if( !GetDisabledByConfigIni () )
	{
		for( int x ( 0 ); x < MAX_EDICTS; ++x )
		{
			SourceSdk::edict_t * const ent ( Helpers::PEntityOfEntIndex ( x ) );
			if( Helpers::isValidEdict ( ent ) )
			{
				if( ent->GetClassName () != nullptr )
				{
#undef GetClassName
					if( basic_string ( "shadow_control" ).operator==( ent->GetClassName () ) )
					{
						m_disable_shadows = g_EntityProps.GetPropValue<bool, PROP_DISABLE_SHADOW> ( ent );
						m_shadow_direction = g_EntityProps.GetPropValue<SourceSdk::Vector, PROP_SHADOW_DIRECTION> ( ent );
						m_shadow_maxdist = g_EntityProps.GetPropValue<float, PROP_SHADOW_MAX_DIST> ( ent );
						break;
					}
				}
			}
		}
	}
}

bool WallhackBlocker::sys_cmd_fn(const SourceSdk::CCommand & args)
{
	if (stricmp("SetFFAMode", args.Arg(2)) == 0)
	{
		if (args.ArgC() >= 4)
		{
			if (Helpers::IsArgTrue(args.Arg(3)))
			{
				m_ffamode = true;
				g_Logger.Msg<MSG_CMD_REPLY>("FFAMode is Yes");
				return true;
			}
			else if (Helpers::IsArgFalse(args.Arg(3)))
			{
				m_ffamode = false;
				g_Logger.Msg<MSG_CMD_REPLY>("FFAMode is No");
				return true;
			}
		}
		g_Logger.Msg<MSG_CMD_REPLY>("FFAMode expected Yes or No");
		return true;
	}
	return false;
}

bool WallhackBlocker::RT_SetTransmitCallback ( PlayerHandler::iterator sender_player, PlayerHandler::iterator receiver_player )
{
	/*
		@sender : could be BOT, PLAYER_CONNECTED or PLAYER_IN_TESTS.
		@receiver : could be PLAYER_CONNECTED or PLAYER_IN_TESTS.
	*/

	VisCache& cache ( g_WallhackBlocker.m_viscache );

	if( cache.IsValid ( sender_player.GetIndex (), receiver_player.GetIndex () ) )
	{
		return !cache.IsVisible ( sender_player.GetIndex (), receiver_player.GetIndex () );
	}

	SourceSdk::IPlayerInfo * const pinfo_receiver = receiver_player->GetPlayerInfo ();
	LoggerAssert ( pinfo_receiver );
	SourceSdk::IPlayerInfo * const pinfo_sender = sender_player->GetPlayerInfo ();
	LoggerAssert ( pinfo_sender );

	if ( !pinfo_receiver->IsDead () )
	{
		if( !pinfo_sender->IsDead () && (pinfo_receiver->GetTeamIndex() != pinfo_sender->GetTeamIndex() || m_ffamode) )
		{
			return cache.SetVisibility_GetNotVisible(sender_player.GetIndex (), receiver_player.GetIndex (), RT_IsAbleToSee ( sender_player, receiver_player ) );
		}
	}
	else if (!pinfo_sender->IsDead())
	{
		SpectatorMode const receiver_spec(*g_EntityProps.GetPropValue<SpectatorMode, PROP_OBSERVER_MODE>(receiver_player->GetEdict(), false));
		if (receiver_spec == OBS_MODE_IN_EYE)
		{
			SourceSdk::CBaseHandle const * const bh(g_EntityProps.GetPropValue<SourceSdk::CBaseHandle, PROP_OBSERVER_TARGET>(receiver_player->GetEdict(), false));
			if (bh->IsValid())
			{
				/*
					The handle can still be invalid now https://github.com/L-EARN/NoCheatZ-4/issues/67#issuecomment-232063885
					Perform more checks.
				*/

				int const bh_index(bh->GetEntryIndex());
				if (bh_index > 0 && bh_index <= g_NczPlayerManager.GetMaxIndex())
				{
					PlayerHandler::iterator spec_player(bh_index);

					if (spec_player && sender_player != spec_player)
					{
						if (!cache.IsValid(sender_player.GetIndex(), spec_player.GetIndex()))
						{
							cache.SetVisibility(sender_player.GetIndex(), spec_player.GetIndex(), RT_IsAbleToSee(sender_player, spec_player));
						}
						return cache.SetVisibility_GetNotVisible(sender_player.GetIndex(), receiver_player.GetIndex(), cache.IsVisible(sender_player.GetIndex(), spec_player.GetIndex()));
					}
				}
			}
		}
	}

	return cache.SetVisibility_GetNotVisible( sender_player.GetIndex (), receiver_player.GetIndex (), true );
}

bool WallhackBlocker::RT_SetTransmitWeaponCallback ( SourceSdk::edict_t const * const sender, PlayerHandler::iterator receiver )
{
	const int weapon_index ( Helpers::IndexOfEdict ( sender ) );
	NczPlayer const * const owner_player ( m_weapon_owner[ weapon_index ] );
	if( !owner_player ) return false;

	if( owner_player == *receiver ) return false;

	PlayerHandler::iterator owner_ph ( owner_player->GetIndex () );

	if( receiver > SlotStatus::PLAYER_CONNECTING && owner_ph >= SlotStatus::BOT && owner_ph != SlotStatus::PLAYER_CONNECTING )
	{
		return RT_SetTransmitCallback ( owner_player->GetIndex (), receiver );
	}
	else
	{
		return false;
	}
}

void WallhackBlocker::RT_WeaponEquipCallback ( PlayerHandler::iterator ph, SourceSdk::edict_t const * const weapon )
{
	const int weapon_index ( Helpers::IndexOfEdict ( weapon ) );
	m_weapon_owner[ weapon_index ] = *ph;
	SetTransmitHookListener::HookSetTransmit ( weapon, false );
}

void WallhackBlocker::RT_WeaponDropCallback ( PlayerHandler::iterator ph, SourceSdk::edict_t const * const weapon )
{
	const int weapon_index ( Helpers::IndexOfEdict ( weapon ) );
	m_weapon_owner[ weapon_index ] = nullptr;
}

void WallhackBlocker::RT_ProcessOnTick (double const & curtime )
{
	m_viscache.Invalidate ();

	ST_W_STATIC SourceSdk::CTraceFilterWorldAndPropsOnly itracefilter;
	ST_R_STATIC SourceSdk::Vector hull_min ( -5.0f, -5.0f, -5.0f );
	ST_R_STATIC SourceSdk::Vector hull_max ( 5.0f, 5.0f, 5.0f );

	ProcessFilter::HumanAtLeastConnectedOrBot filter_class;
	float const tick_interval(SourceSdk::InterfacesProxy::Call_GetTickInterval());
	float const inv_tick_interval(1.0f / tick_interval);
	int const tick_count(Helpers::GetGameTickCount());

	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph += &filter_class )
	{
		SourceSdk::IPlayerInfo * const playerinfo ( ph->GetPlayerInfo () );
		int const ph_index(ph.GetIndex());
		if( playerinfo != nullptr )
		{
			if (playerinfo->IsDead() || playerinfo->IsObserver())
			{
				continue;
			}

			switch( ph.operator SlotStatus() )
			{
				case SlotStatus::BOT: // predict without tracing hull
					{
						MathInfo const & player_maths ( g_MathCache.RT_GetCachedMaths (ph_index) );

						ClientDataS* const pData ( GetPlayerDataStructByIndex (ph_index) );

						SourceSdk::VectorCopy ( player_maths.m_mins, pData->bbox_min );
						SourceSdk::VectorCopy ( player_maths.m_maxs, pData->bbox_max );
						SourceSdk::VectorCopy ( player_maths.m_abs_origin, pData->abs_origin );
						SourceSdk::VectorCopy ( player_maths.m_eyepos, pData->ear_pos );

						SourceSdk::vec_t& bmax2 ( pData->bbox_max.z );
						bmax2 *= 0.5f;
						pData->bbox_min.z -= bmax2;
						pData->abs_origin.z += bmax2;

						if (!SourceSdk::VectorIsZero(player_maths.m_velocity))
						{
							ST_W_STATIC SourceSdk::Vector predicted_pos;
							
							SourceSdk::VectorCopy(player_maths.m_velocity, predicted_pos);
							SourceSdk::VectorMultiply(predicted_pos, tick_interval);
							SourceSdk::VectorAdd(player_maths.m_abs_origin, predicted_pos);

							SourceSdk::vec_t const & vx(player_maths.m_abs_velocity.x);
							SourceSdk::vec_t const & vy(player_maths.m_abs_velocity.y);
							SourceSdk::vec_t const & vz(player_maths.m_abs_velocity.z);
							if (vx > 1.0f)
							{
								pData->bbox_min.x *= vx;
								pData->bbox_max.x *= vx;
							}
							if (vy > 1.0f)
							{
								pData->bbox_min.y *= vy;
								pData->bbox_max.y *= vy;
							}
							if (vz > 1.0f)
							{
								pData->bbox_min.z *= vz;
								pData->bbox_max.z *= vz;
							}
						}

						break;
					}
				default:
					{
						SourceSdk::INetChannelInfo* const netchan ( ph->GetChannelInfo () );
						if( netchan != nullptr )
						{
							MathInfo const & player_maths ( g_MathCache.RT_GetCachedMaths ( ph.GetIndex () ) );

							ClientDataS* const pData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

							SourceSdk::VectorCopy(player_maths.m_mins, pData->bbox_min);
							SourceSdk::VectorCopy(player_maths.m_maxs, pData->bbox_max);
							SourceSdk::VectorCopy(player_maths.m_abs_origin, pData->abs_origin);
							SourceSdk::VectorCopy(player_maths.m_eyepos, pData->ear_pos);

							SourceSdk::vec_t& bmax2(pData->bbox_max.z);
							bmax2 *= 0.5f;
							pData->bbox_min.z -= bmax2;
							pData->abs_origin.z += bmax2;

							if (!SourceSdk::VectorIsZero(player_maths.m_velocity))
							{
								ST_W_STATIC SourceSdk::Vector predicted_pos;
								ST_W_STATIC SourceSdk::Vector vel_scale;
								
								float const lerptime = (*g_EntityProps.GetPropValue<float, PROP_LERP_TIME>(ph->GetEdict(), true));
								int const lerpticks = (int)((lerptime * inv_tick_interval) + 0.5f);
								float fCorrect = fabsf(fmodf(netchan->GetLatency(FLOW_OUTGOING) + lerptime, 1.0f));

								int tick = pData->cmd_tickcount - lerpticks;

								if (fabsf(fCorrect - (tick_count - tick) * tick_interval) > 0.2f)
								{
									tick = tick_count - (int)((fCorrect * inv_tick_interval) + 0.5f);
								}

								SourceSdk::VectorCopy(player_maths.m_velocity, vel_scale);
								SourceSdk::VectorMultiply(vel_scale, tick_interval * (tick_count - tick));
								SourceSdk::VectorCopy(vel_scale, predicted_pos);
								SourceSdk::VectorAdd(player_maths.m_abs_origin, predicted_pos);

								if (!SourceSdk::trace_hull_fn(predicted_pos, hull_min, hull_max, MASK_PLAYERSOLID_BRUSHONLY, &itracefilter))
								{
									SourceSdk::VectorCopy(predicted_pos, pData->abs_origin);
									SourceSdk::VectorAdd(vel_scale, pData->ear_pos);
								}

								{
									SourceSdk::vec_t const & vx(player_maths.m_abs_velocity.x);
									SourceSdk::vec_t const & vy(player_maths.m_abs_velocity.y);
									SourceSdk::vec_t const & vz(player_maths.m_abs_velocity.z);
									if (vx > 1.0f)
									{
										pData->bbox_min.x *= vx;
										pData->bbox_max.x *= vx;
									}
									if (vy > 1.0f)
									{
										pData->bbox_min.y *= vy;
										pData->bbox_max.y *= vy;
									}
									if (vz > 1.0f)
									{
										pData->bbox_min.z *= vz;
										pData->bbox_max.z *= vz;
									}
								}
							}
						}

						break;
					}
			}
		}
	}
}

PlayerRunCommandRet WallhackBlocker::RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime)
{
	auto pdata(GetPlayerDataStructByIndex(ph.GetIndex()));

	pdata->cmd_tickcount = ((SourceSdk::CUserCmd const * const)cmd)->tick_count;

	return PlayerRunCommandRet::CONTINUE;
}

void WallhackBlocker::ClientDisconnect ( PlayerHandler::iterator ph )
{
	for( int x ( 0 ); x < MAX_EDICTS; ++x )
	{
		if( m_weapon_owner[ x ] == *ph )
			m_weapon_owner[ x ] = nullptr;
	}
	m_viscache.Invalidate ();
}

inline bool WallhackBlocker::RT_IsInFOV ( const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target )
{
	ST_W_STATIC SourceSdk::Vector norm, plane;
	ST_W_STATIC SourceSdk::vec_t dp;
	SourceSdk::VectorSub ( target, origin, plane );
	SourceSdk::VectorNorm ( plane );
	SourceSdk::AngleVectors ( dir, &norm );
	SourceSdk::VectorDotProduct ( plane, norm, dp );
	return dp > 0.0;
}

// Point
inline bool WallhackBlocker::RT_IsVisible ( const SourceSdk::Vector& origin, const SourceSdk::Vector& target )
{
	ST_W_STATIC SourceSdk::CTraceFilterWorldAndPropsOnly itracefilter;

	return SourceSdk::trace_ray_fn ( origin, target, MASK_VISIBLE, &itracefilter );
}

// Forward Vector
bool WallhackBlocker::RT_IsVisible ( const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target )
{
	ST_W_STATIC SourceSdk::Vector forward;
	SourceSdk::AngleVectors ( dir, &forward );
	SourceSdk::VectorMultiply ( forward, 50.0f );
	SourceSdk::VectorAdd ( target, forward );
	return RT_IsVisible ( origin, forward );
}

// Rectangle
bool WallhackBlocker::RT_IsVisible ( const SourceSdk::Vector& origin, const SourceSdk::Vector& target, const SourceSdk::Vector& mins, const SourceSdk::Vector& maxs, const SourceSdk::vec_t scale )
{
	const SourceSdk::vec_t ZpozOffset ( maxs.z * scale );
	const SourceSdk::vec_t ZnegOffset ( mins.z * scale );
	const SourceSdk::vec_t WideOffset ( ( ( maxs.x - mins.x ) + ( maxs.y - mins.y ) ) / ( 4.0f / scale ) );

	if( ZpozOffset == 0.0f && ZnegOffset == 0.0f && WideOffset == 0.0f )
	{
		return RT_IsVisible ( origin, target );
	}

	ST_W_STATIC SourceSdk::QAngle rotate;
	ST_W_STATIC SourceSdk::Vector forward, right;

	SourceSdk::VectorSub ( origin, target, forward );
	SourceSdk::VectorNorm ( forward );

	SourceSdk::VectorAngles ( forward, rotate );
	SourceSdk::AngleVectors ( rotate, &forward, &right );

	ST_W_STATIC SourceSdk::Vector rectangle[ 4 ], temp, temp2;

	if( fabs ( forward.z ) <= 0.7071 )
	{
		SourceSdk::VectorMultiply ( right, WideOffset );

		SourceSdk::VectorCopy ( target, temp );
		temp.z += ZpozOffset;
		SourceSdk::VectorAdd ( temp, right, rectangle[ 0 ] );
		SourceSdk::VectorSub ( temp, right, rectangle[ 1 ] );

		temp.z = target.z + ZnegOffset;
		SourceSdk::VectorAdd ( temp, right, rectangle[ 2 ] );
		SourceSdk::VectorSub ( temp, right, rectangle[ 3 ] );
	}
	else if( forward.z > 0.0 )
	{
		{
			forward.z = 0.0f;
			SourceSdk::VectorNorm ( forward );
			SourceSdk::VectorMultiply ( forward, WideOffset * scale );
			SourceSdk::VectorMultiply ( right, WideOffset );
		}
		{
			VectorCopy ( target, temp2 );
			temp2.z += ZpozOffset;

			SourceSdk::VectorAdd ( temp2, right, temp );
			SourceSdk::VectorSub ( temp, forward, rectangle[ 0 ] );

			SourceSdk::VectorSub ( temp2, right, temp );
			SourceSdk::VectorSub ( temp, forward, rectangle[ 1 ] );

			temp2.z = target.z + ZnegOffset;

			SourceSdk::VectorAdd ( temp2, right, temp );
			SourceSdk::VectorAdd ( temp, forward, rectangle[ 2 ] );

			SourceSdk::VectorSub ( temp2, right, temp );
			SourceSdk::VectorAdd ( temp, forward, rectangle[ 3 ] );
		}
	}
	else
	{
		{
			forward.z = 0.0;
			SourceSdk::VectorNorm ( forward );
			SourceSdk::VectorMultiply ( forward, WideOffset * scale );
			SourceSdk::VectorMultiply ( right, WideOffset );
		}
		{
			SourceSdk::VectorCopy ( target, temp2 );
			temp2.z += ZpozOffset;

			SourceSdk::VectorAdd ( temp2, right, temp );
			SourceSdk::VectorAdd ( temp, forward, rectangle[ 0 ] );

			SourceSdk::VectorSub ( temp2, right, temp );
			SourceSdk::VectorAdd ( temp, forward, rectangle[ 1 ] );

			temp2.z = target.z + ZnegOffset;

			SourceSdk::VectorAdd ( temp2, right, temp );
			SourceSdk::VectorSub ( temp, forward, rectangle[ 2 ] );

			SourceSdk::VectorSub ( temp2, right, temp );
			SourceSdk::VectorSub ( temp, forward, rectangle[ 3 ] );
		}
	}

	for( int i ( 0 ); i < 4; i++ )
		if( RT_IsVisible ( origin, rectangle[ i ] ) )
			return true;

	return false;
}

bool WallhackBlocker::RT_IsAbleToSee ( PlayerHandler::iterator sender, PlayerHandler::iterator receiver )
{
	const ClientDataS* const sender_data ( GetPlayerDataStructByIndex ( sender.GetIndex () ) );
	const ClientDataS* const receiver_data ( GetPlayerDataStructByIndex ( receiver.GetIndex () ) );

	const SourceSdk::Vector& receiver_ear_pos ( receiver_data->ear_pos );
	const SourceSdk::Vector& sender_origin ( sender_data->abs_origin );

	if( RT_IsInFOV ( receiver_ear_pos, g_MathCache.RT_GetCachedMaths ( receiver.GetIndex () ).m_eyeangles, sender_origin ) )
	{
		if( RT_IsVisible ( receiver_ear_pos, sender_origin ) )
			return true;

		if( RT_IsVisible ( receiver_ear_pos, g_MathCache.RT_GetCachedMaths ( sender->GetIndex () ).m_eyeangles, sender_data->ear_pos ) )
			return true;

		// Test shadow, if any
		if( m_disable_shadows != nullptr /* https://github.com/L-EARN/NoCheatZ-4/issues/65 */
			&& !*m_disable_shadows )
		{
			ST_W_STATIC SourceSdk::Vector shadow_trace_end;
			SourceSdk::VectorCopy ( m_shadow_direction, &shadow_trace_end );
			SourceSdk::VectorMultiply ( shadow_trace_end, *m_shadow_maxdist );
			SourceSdk::VectorAdd ( sender_data->ear_pos, shadow_trace_end );

			if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
			{
				SourceSdk::Ray_t_csgo ray;
				SourceSdk::CGameTrace_csgo trace;
				ray.Init ( sender_data->ear_pos, shadow_trace_end );
				SourceSdk::InterfacesProxy::Call_ClipRayToEntity ( &ray, MASK_PLAYERSOLID_BRUSHONLY, Helpers::PEntityOfEntIndex ( 0 )->m_pUnk, &trace );
				SourceSdk::VectorAdd ( sender_data->ear_pos, ray.m_Delta, shadow_trace_end );

				if( RT_IsVisible ( receiver_ear_pos, shadow_trace_end ) )
					return true;
			}
			else
			{
				SourceSdk::Ray_t ray;
				SourceSdk::CGameTrace trace;
				ray.Init ( sender_data->ear_pos, shadow_trace_end );
				SourceSdk::InterfacesProxy::Call_ClipRayToEntity ( &ray, MASK_PLAYERSOLID_BRUSHONLY, Helpers::PEntityOfEntIndex ( 0 )->m_pUnk, &trace );
				SourceSdk::VectorMultiply ( ray.m_Delta, 0.998f );
				SourceSdk::VectorAdd ( sender_data->ear_pos, ray.m_Delta, shadow_trace_end );

				if( RT_IsVisible ( receiver_ear_pos, shadow_trace_end ) )
				{
					return true;
				}
			}
		}

		{
			const SourceSdk::Vector& sender_maxs ( sender_data->bbox_max );
			const SourceSdk::Vector& sender_mins ( sender_data->bbox_min );

			if( RT_IsVisible ( receiver_ear_pos, sender_origin, sender_mins, sender_maxs, 1.30f ) )
				return true;

			if( RT_IsVisible ( receiver_ear_pos, sender_origin, sender_mins, sender_maxs, 0.65f ) )
				return true;
		}
	}

	return false;
}

WallhackBlocker g_WallhackBlocker;
