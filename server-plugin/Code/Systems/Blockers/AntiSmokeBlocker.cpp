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
	BaseSystem("AntiSmokeBlocker", PLAYER_CONNECTED, PLAYER_CONNECTING, STATUS_EQUAL_OR_BETTER),
	IGameEventListener002(),
	OnTickListener(),
	playerdatahandler_class(),
	SetTransmitHookListener(),
	singleton_class()
{
	METRICS_ADD_TIMER("AntiSmokeBlocker::OnFrame", 10.0);
}

AntiSmokeBlocker::~AntiSmokeBlocker()
{
	Unload();
}

void AntiSmokeBlocker::Init()
{
	InitDataStruct();
}
	
void AntiSmokeBlocker::Load()
{
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "smokegrenade_detonate", true);
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "round_start", true);
	OnTickListener::RegisterOnTickListener(this);
	SetTransmitHookListener::RegisterSetTransmitHookListener(this, 1);
}

void AntiSmokeBlocker::Unload()
{
	SetTransmitHookListener::RemoveSetTransmitHookListener(this);
	OnTickListener::RemoveOnTickListener(this);
	SourceSdk::InterfacesProxy::GetGameEventManager()->RemoveListener(this);

	SmokeListT::elem_t* it = m_smokes.GetFirst();
	while (it != nullptr)
	{
		it = m_smokes.Remove(it);
	}

	PLAYERS_LOOP_RUNTIME
	{
		ResetPlayerDataStruct(ph->playerClass);
	}
	END_PLAYERS_LOOP
}

void AntiSmokeBlocker::ProcessOnTick(float const curtime)
{
	METRICS_ENTER_SECTION("AntiSmokeBlocker::OnFrame");
	if(!IsActive())
	{	
		METRICS_LEAVE_SECTION("AntiSmokeBlocker::OnFrame");
		return;
	}

	SmokeListT::elem_t* it = m_smokes.GetFirst();

	// remove old smokes
	while(it != nullptr)
	{
		if(curtime - (it->m_value.bang_time + ConfigManager::GetInstance()->m_smoke_time) > 0.0f)
			it = m_smokes.Remove(it);
		else it = it->m_next;
	}

	ST_R_STATIC SmokeInfoT empty;
	ResetAll(&empty);

	it = m_smokes.GetFirst();
	if (it == nullptr) return;

	// Test if players are immersed in smoke
	PLAYERS_LOOP_RUNTIME
	{
		if(ph->status != PLAYER_IN_TESTS) continue;

		SourceSdk::Vector delta,other_delta;

		MathInfo const & x_math = MathCache::GetInstance()->GetCachedMaths(x);
		
		do // At this stage, m_smokes ! empty
		{
			if(curtime - it->m_value.bang_time > ConfigManager::GetInstance()->m_smoke_timetobang)
			{
				SourceSdk::vec_t dst;
				SourceSdk::VectorDistanceSqr(x_math.m_eyepos, it->m_value.pos, delta, dst);
				if(dst < ConfigManager::GetInstance()->m_innersmoke_radius_sqr)
				{
					GetPlayerDataStruct(x)->is_in_smoke = true;
				}

				/* Players can't see eachother if they are behind a smoke */

				const SourceSdk::vec_t ang_smoke = tanf(ConfigManager::GetInstance()->m_smoke_radius / sqrtf(dst));
				SourceSdk::VectorNorm(delta);

				for (int y = 1; y <= maxcl; ++y)
				{
					if (x == y) continue;

					PlayerHandler const * const other_ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(y);
					if (other_ph->status == INVALID) continue;

					MathInfo const & y_math = MathCache::GetInstance()->GetCachedMaths(y);

					// Is he behind the smoke against us ?

					SourceSdk::vec_t other_dst;
					SourceSdk::VectorDistanceSqr(x_math.m_eyepos, y_math.m_abs_origin, other_delta, other_dst);
					if (dst + ConfigManager::GetInstance()->m_smoke_radius < other_dst)
					{
						// Hidden by the hull of the smoke ?

						SourceSdk::VectorNorm(other_delta);

						SourceSdk::vec_t dp;
						SourceSdk::VectorDotProduct(other_delta, delta, dp);
						const SourceSdk::vec_t angle_player = fabs(acos(dp));

						if (angle_player < ang_smoke)
						{
							GetPlayerDataStruct(x)->can_not_see_this_player[y] = true;
						}
					}
				}
			}
			it = it->m_next;
		} while(it != nullptr);
		it = m_smokes.GetFirst();
	}
	END_PLAYERS_LOOP

	METRICS_LEAVE_SECTION("AntiSmokeBlocker::OnFrame");
}

bool AntiSmokeBlocker::SetTransmitCallback(SourceSdk::edict_t const * const ea, SourceSdk::edict_t const * const eb)
{
	if(IsActive() && ea != eb)
	{
		if(NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(eb)->status == INVALID) return false;

		NczPlayer* const pPlayer_b = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(eb)->playerClass;

		if(GetPlayerDataStruct(pPlayer_b)->is_in_smoke)
			return true;

		if(GetPlayerDataStruct(pPlayer_b)->can_not_see_this_player[Helpers::IndexOfEdict(ea)] == true)
			return true;
	}
	return false;
}

void AntiSmokeBlocker::FireGameEvent(SourceSdk::IGameEvent * ev)
{
	if(!IsActive()) return;

	if(ev->GetName()[0] == 's') // smokegrenade_detonate
	{
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			SourceSdk::Vector t1(reinterpret_cast<SourceSdk::IGameEvent_csgo*>(ev)->GetFloat("x"), 
								reinterpret_cast<SourceSdk::IGameEvent_csgo*>(ev)->GetFloat("y"), 
								reinterpret_cast<SourceSdk::IGameEvent_csgo*>(ev)->GetFloat("z"));
			SmokeEntityS t2(t1);
			m_smokes.Add(t2);
		}
		else
		{
			SourceSdk::Vector t1(ev->GetFloat("x"), ev->GetFloat("y"), ev->GetFloat("z"));
			SmokeEntityS t2(t1);
			m_smokes.Add(t2);
		}
		
		return;
	}

	// round_start

	SmokeListT::elem_t* it = m_smokes.GetFirst();
	while (it != nullptr)
	{
		it = m_smokes.Remove(it);
	}

	ST_R_STATIC SmokeInfoT default_smoke = SmokeInfoT();
	ResetAll(&default_smoke);
}
