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

#include "Interfaces/IEngineTrace/IEngineTrace.h"
#include "Interfaces/bspflags.h"
#include "Interfaces/iserverunknown.h"

#include "Misc/EntityProps.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Misc/MathCache.h"

WallhackBlocker::WallhackBlocker() :
	BaseSystem("WallhackBlocker", PLAYER_CONNECTED, PLAYER_CONNECTING, STATUS_EQUAL_OR_BETTER),
	SetTransmitHookListener(),
	WeaponHookListener(),
	OnTickListener(),
	singleton_class()
{
	METRICS_ADD_TIMER("WallhackBlocker::SetTransmitCallback", 1.0);
	METRICS_ADD_TIMER("WallhackBlocker::OnFrame", 10.0);
	METRICS_ADD_TIMER("WallhackBlocker TraceRay Call", 2.0);
	METRICS_ADD_TIMER("WallhackBlocker TraceHull Call", 2.0);
	METRICS_ADD_COUNTER("WallhackBlocker TraceRay Count Per Tick", 1024);
}

WallhackBlocker::~WallhackBlocker()
{
	Unload();
}

void WallhackBlocker::Init()
{
}

void WallhackBlocker::Load()
{
	m_viscache.Invalidate();
	memset(m_weapon_owner, 0, MAX_EDICTS*sizeof(NczPlayer*));
	WeaponHookListener::RegisterWeaponHookListener(this);
	SetTransmitHookListener::RegisterSetTransmitHookListener(this, 2);
	OnTickListener::RegisterOnTickListener(this);
}

void WallhackBlocker::Unload()
{
	SetTransmitHookListener::RemoveSetTransmitHookListener(this);
	WeaponHookListener::RemoveWeaponHookListener(this);
	OnTickListener::RemoveOnTickListener(this);

	memset(m_weapon_owner, 0, MAX_EDICTS*sizeof(NczPlayer*));
	m_viscache.Invalidate();
}

bool WallhackBlocker::SetTransmitCallback(SourceSdk::edict_t* const sender, SourceSdk::edict_t* const receiver)
{
	METRICS_ENTER_SECTION("WallhackBlocker::SetTransmitCallback");
	PlayerHandler* const sender_player = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(sender);
	PlayerHandler* const receiver_player = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(receiver);

	if(sender_player->status == INVALID || receiver_player->status < PLAYER_IN_TESTS)
	{
		METRICS_LEAVE_SECTION("WallhackBlocker::SetTransmitCallback");
		return false;
	}

	SpectatorMode receiver_spec = *EntityProps::GetInstance()->GetPropValue<SpectatorMode, PROP_OBSERVER_MODE>(receiver_player->playerClass->GetEdict(), false);

	VisCache& cache = WallhackBlocker::GetInstance()->m_viscache;

	if(receiver_spec == OBS_MODE_IN_EYE)
	{
		SourceSdk::CBaseHandle &bh = *EntityProps::GetInstance()->GetPropValue<SourceSdk::CBaseHandle, PROP_OBSERVER_TARGET>(receiver_player->playerClass->GetEdict(), false);
		PlayerHandler* const spec_player = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(bh.GetEntryIndex());

		Assert(spec_player->status > INVALID);

		if(spec_player->status == BOT)
		{
			METRICS_LEAVE_SECTION("WallhackBlocker::SetTransmitCallback");
			return false;
		}

		if(!cache.IsValid(sender_player->playerClass, spec_player->playerClass))
		{
			cache.SetVisibility(sender_player->playerClass, spec_player->playerClass, IsAbleToSee(sender_player->playerClass, spec_player->playerClass));
		}

		bool rt = !cache.IsVisible(sender_player->playerClass, spec_player->playerClass);
		METRICS_LEAVE_SECTION("WallhackBlocker::SetTransmitCallback");
		SystemVerbose2(Helpers::format("%s can see %s ? : %s", spec_player->playerClass->GetName(), sender_player->playerClass->GetName(), Helpers::boolToString(!rt)));
		return rt;
	}

	
	if(!cache.IsValid(sender_player->playerClass, receiver_player->playerClass))
	{
		cache.SetVisibility(sender_player->playerClass, receiver_player->playerClass, IsAbleToSee(sender_player->playerClass, receiver_player->playerClass));
	}
	bool rt = !cache.IsVisible(sender_player->playerClass, receiver_player->playerClass);
	METRICS_LEAVE_SECTION("WallhackBlocker::SetTransmitCallback");
	return rt;
}

bool WallhackBlocker::SetTransmitWeaponCallback(SourceSdk::edict_t* const sender, SourceSdk::edict_t* const receiver)
{
	const int weapon_index = Helpers::IndexOfEdict(sender);
	NczPlayer* const owner_player = WallhackBlocker::GetInstance()->m_weapon_owner[weapon_index];
	if(!owner_player) return false;

	PlayerHandler* const receiver_player = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(receiver);

	if(owner_player == receiver_player->playerClass) return false;

	return SetTransmitCallback(owner_player->GetEdict(), receiver);
}

void WallhackBlocker::WeaponEquipCallback(NczPlayer* player, SourceSdk::edict_t* const weapon)
{
	const int weapon_index = Helpers::IndexOfEdict(weapon);
	WallhackBlocker::GetInstance()->m_weapon_owner[weapon_index] = player;
	SetTransmitHookListener::HookSetTransmit(weapon);
}

void WallhackBlocker::WeaponDropCallback(NczPlayer* player, SourceSdk::edict_t* const weapon)
{
	const int weapon_index = Helpers::IndexOfEdict(weapon);
	WallhackBlocker::GetInstance()->m_weapon_owner[weapon_index] = nullptr;
}

void WallhackBlocker::ProcessOnTick(float const curtime)
{
	METRICS_LEAVE_SECTION("WallhackBlocker TraceRay Count Per Tick");
	METRICS_ENTER_SECTION("WallhackBlocker::OnFrame");
	if (!IsActive())
	{
		METRICS_LEAVE_SECTION("WallhackBlocker::OnFrame");
		return;
	}

	m_viscache.Invalidate();

	int game_tick;
	ST_W_STATIC float tick_interval;

	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		game_tick = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
		tick_interval = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->interval_per_tick;
	}
	else
	{
		game_tick = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
		tick_interval = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->interval_per_tick;
	}

	ST_W_STATIC SourceSdk::CTraceFilterWorldAndPropsOnly itracefilter;
	ST_R_STATIC SourceSdk::Vector hull_min( -5.0f, -5.0f, -5.0f );
	ST_R_STATIC SourceSdk::Vector hull_max( 5.0f, 5.0f, 5.0f );

	PLAYERS_LOOP_RUNTIME(x)
	{
		if(x_ph->status != BOT && x_ph->status != PLAYER_IN_TESTS) continue;

		NczPlayer* const pPlayer = x_ph->playerClass;
		void* playerinfo = pPlayer->GetPlayerInfo();
		if (playerinfo == nullptr) continue;
		SourceSdk::INetChannelInfo* const netchan = pPlayer->GetChannelInfo();
		if (netchan == nullptr && x_ph->status == PLAYER_IN_TESTS) continue;

		MathInfo const & player_maths = MathCache::GetInstance()->GetCachedMaths(x);

		SourceSdk::edict_t* const playeredict = pPlayer->GetEdict();
		ClientDataS* const pData = GetPlayerDataStruct(pPlayer);

		SourceSdk::VectorCopy(player_maths.m_mins, pData->bbox_min);
		SourceSdk::VectorCopy(player_maths.m_maxs, pData->bbox_max);
		SourceSdk::VectorCopy(player_maths.m_abs_origin, pData->abs_origin);
		SourceSdk::VectorCopy(player_maths.m_eyepos, pData->ear_pos);

		{
			SourceSdk::vec_t& bmax2 = pData->bbox_max.z;
			bmax2 *= 0.5f;
			pData->bbox_min.z -= bmax2;
			pData->abs_origin.z += bmax2;
		}
		
		if(!SourceSdk::VectorIsZero(player_maths.m_velocity, 0.0001f))
		{
			ST_W_STATIC float diff_time;
			ST_W_STATIC int target_tick;

			if(x_ph->status == BOT)
			{
				target_tick = game_tick - 1;
				diff_time = tick_interval;
			}
			else
			{
				const int lerp_ticks = (int)( 0.5f + *EntityProps::GetInstance()->GetPropValue<float, PROP_LERP_TIME>(playeredict, true) / tick_interval );
				const float fCorrect = netchan->GetLatency(FLOW_OUTGOING) + fmodf(lerp_ticks * tick_interval, 1.0f);

				target_tick = static_cast<SourceSdk::CUserCmd_csgo*>(PlayerRunCommandHookListener::GetLastUserCmd(pPlayer))->tick_count - lerp_ticks;

				diff_time = (game_tick - target_tick) * tick_interval;

				if (fabs(fCorrect - diff_time) > 0.2f)
				{
					target_tick = (int)ceil((float)game_tick - fCorrect / tick_interval);
					diff_time = (float)(game_tick - target_tick) * tick_interval;
				}
			}

			ST_W_STATIC SourceSdk::Vector predicted_pos;

			{
				SourceSdk::VectorCopy(player_maths.m_velocity, predicted_pos);
				SourceSdk::VectorMultiply(predicted_pos, diff_time);
				SourceSdk::VectorAdd(pData->abs_origin, predicted_pos);
			}

			if (SourceSdk::trace_hull_fn(predicted_pos, hull_min, hull_max, MASK_PLAYERSOLID_BRUSHONLY, &itracefilter))
			{
				SourceSdk::VectorCopy(predicted_pos, pData->abs_origin);
				SourceSdk::VectorAdd(player_maths.m_velocity, pData->ear_pos);
			}

			{
				SourceSdk::vec_t const vx = player_maths.m_abs_velocity.x;
				SourceSdk::vec_t const vy = player_maths.m_abs_velocity.y;
				SourceSdk::vec_t const vz = player_maths.m_abs_velocity.z;
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
	END_PLAYERS_LOOP

	METRICS_LEAVE_SECTION("WallhackBlocker::OnFrame");
}

void WallhackBlocker::ClientDisconnect(SourceSdk::edict_t* const client)
{
	NczPlayer* pPlayer = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(client)->playerClass;
	Assert(pPlayer);
	for(int x = 0; x < MAX_EDICTS; ++x)
	{
		if(m_weapon_owner[x] == pPlayer) 
			m_weapon_owner[x] = nullptr;
	}
	m_viscache.Invalidate();
}

inline bool WallhackBlocker::IsInFOV(const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target)
{
	ST_W_STATIC SourceSdk::Vector norm, plane;
	ST_W_STATIC SourceSdk::vec_t dp;
	SourceSdk::VectorSub(target, origin, plane);
	SourceSdk::VectorNorm(plane);
	SourceSdk::AngleVectors(dir, &norm);
	SourceSdk::VectorDotProduct(plane, norm, dp);
	return dp > 0.0;
}

// Point
inline bool WallhackBlocker::IsVisible(const SourceSdk::Vector& origin, const SourceSdk::Vector& target)
{
	ST_W_STATIC SourceSdk::CTraceFilterWorldAndPropsOnly itracefilter;

	return SourceSdk::trace_ray_fn(origin, target, MASK_VISIBLE, &itracefilter);
}

// Forward Vector
bool WallhackBlocker::IsVisible(const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target)
{
	ST_W_STATIC SourceSdk::Vector forward;
	SourceSdk::AngleVectors(dir, &forward);
	SourceSdk::VectorMultiply(forward, 50.0f);
	SourceSdk::VectorAdd(target, forward);
	return IsVisible(origin, forward);
}

// Rectangle
bool WallhackBlocker::IsVisible(const SourceSdk::Vector& origin, const SourceSdk::Vector& target, const SourceSdk::Vector& mins, const SourceSdk::Vector& maxs, const SourceSdk::vec_t scale)
{
	const SourceSdk::vec_t ZpozOffset = maxs.z * scale;
	const SourceSdk::vec_t ZnegOffset = mins.z * scale;
	const SourceSdk::vec_t WideOffset = ((maxs.x - mins.x) + (maxs.y - mins.y)) / (4.0f / scale);

	if (ZpozOffset == 0.0f && ZnegOffset == 0.0f && WideOffset == 0.0f)
	{
		return IsVisible(origin, target);
	}
	
	ST_W_STATIC SourceSdk::QAngle rotate;
	ST_W_STATIC SourceSdk::Vector forward, right;

	SourceSdk::VectorSub(origin, target, forward);
	SourceSdk::VectorNorm(forward);

	SourceSdk::VectorAngles(forward, rotate);
	SourceSdk::AngleVectors(rotate, &forward, &right);

	ST_W_STATIC SourceSdk::Vector rectangle[4], temp, temp2;

	if (fabs(forward.z) <= 0.7071)
	{
		SourceSdk::VectorMultiply(right, WideOffset);
		
		SourceSdk::VectorCopy(target, temp);
		temp.z += ZpozOffset;
		SourceSdk::VectorAdd(temp, right, rectangle[0]);
		SourceSdk::VectorSub(temp, right, rectangle[1]);
		
		temp.z = target.z + ZnegOffset;
		SourceSdk::VectorAdd(temp, right, rectangle[2]);
		SourceSdk::VectorSub(temp, right, rectangle[3]);
	}
	else if (forward.z > 0.0)
	{
		{
			forward.z = 0.0f;
			SourceSdk::VectorNorm(forward);
			SourceSdk::VectorMultiply(forward, WideOffset * scale);
			SourceSdk::VectorMultiply(right, WideOffset);
		}
		{
			VectorCopy(target, temp2);
			temp2.z += ZpozOffset;

			SourceSdk::VectorAdd(temp2, right, temp);
			SourceSdk::VectorSub(temp, forward, rectangle[0]);

			SourceSdk::VectorSub(temp2, right, temp);
			SourceSdk::VectorSub(temp, forward, rectangle[1]);

			temp2.z = target.z + ZnegOffset;

			SourceSdk::VectorAdd(temp2, right, temp);
			SourceSdk::VectorAdd(temp, forward, rectangle[2]);

			SourceSdk::VectorSub(temp2, right, temp);
			SourceSdk::VectorAdd(temp, forward, rectangle[3]);
		}
	}
	else
	{
		{
			forward.z = 0.0;
			SourceSdk::VectorNorm(forward);
			SourceSdk::VectorMultiply(forward, WideOffset * scale);
			SourceSdk::VectorMultiply(right, WideOffset);
		}
		{
			SourceSdk::VectorCopy(target, temp2);
			temp2.z += ZpozOffset;

			SourceSdk::VectorAdd(temp2, right, temp);
			SourceSdk::VectorAdd(temp, forward, rectangle[0]);

			SourceSdk::VectorSub(temp2, right, temp);
			SourceSdk::VectorAdd(temp, forward, rectangle[1]);

			temp2.z = target.z + ZnegOffset;

			SourceSdk::VectorAdd(temp2, right, temp);
			SourceSdk::VectorSub(temp, forward, rectangle[2]);

			SourceSdk::VectorSub(temp2, right, temp);
			SourceSdk::VectorSub(temp, forward, rectangle[3]);
		}
	}

	for (int i = 0; i < 4; i++)
		if (IsVisible(origin, rectangle[i]))
			return true;

	return false;
}

bool WallhackBlocker::IsAbleToSee(NczPlayer* const sender, NczPlayer* const receiver)
{
	const ClientDataS* const sender_data = GetPlayerDataStruct(sender);
	const ClientDataS* const receiver_data = GetPlayerDataStruct(receiver);

	const SourceSdk::Vector& receiver_ear_pos = receiver_data->ear_pos;
	const SourceSdk::Vector& sender_origin = sender_data->abs_origin;

	if (IsInFOV(receiver_ear_pos, MathCache::GetInstance()->GetCachedMaths(receiver->GetIndex()).m_eyeangles, sender_origin))
	{
		if (IsVisible(receiver_ear_pos, sender_origin))
			return true;
		
		// Only test weapon tip if it's not in the wall
		if(IsVisible(sender_data->ear_pos, MathCache::GetInstance()->GetCachedMaths(sender->GetIndex()).m_eyeangles, sender_data->ear_pos))
		{
			if (IsVisible(receiver_ear_pos, MathCache::GetInstance()->GetCachedMaths(sender->GetIndex()).m_eyeangles, sender_data->ear_pos))
				return true;
		}
		
		{
			const SourceSdk::Vector& sender_maxs = sender_data->bbox_max;
			const SourceSdk::Vector& sender_mins = sender_data->bbox_min;

			if (IsVisible(receiver_ear_pos, sender_origin, sender_mins, sender_maxs, 1.30f))
				return true;

			if (IsVisible(receiver_ear_pos, sender_origin, sender_mins, sender_maxs, 0.65f))
				return true;
		}
	}
	
	return false;
}
