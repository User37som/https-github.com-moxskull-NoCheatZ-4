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

#include "AntiFlashbangBlocker.h"

#include "Misc/EntityProps.h"
#include "Players/NczPlayerManager.h"

AntiFlashbangBlocker::AntiFlashbangBlocker() :
	BaseSystem("AntiFlashbangBlocker", PLAYER_CONNECTED, PLAYER_CONNECTING, STATUS_EQUAL_OR_BETTER),
	IGameEventListener002(),
	playerdatahandler_class(),
	SetTransmitHookListener(),
	singleton_class()
{
	METRICS_ADD_TIMER("AntiFlashbangBlocker::SetTransmitCallback", 10.0);
	METRICS_ADD_TIMER("AntiFlashbangBlocker::FireGameEvent", 2.0);
}

AntiFlashbangBlocker::~AntiFlashbangBlocker()
{
	Unload();
}

void AntiFlashbangBlocker::Init()
{
	InitDataStruct();
}
	
void AntiFlashbangBlocker::Load()
{
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "player_blind", true);
	SetTransmitHookListener::RegisterSetTransmitHookListener(this, 0);
}

void AntiFlashbangBlocker::Unload()
{
	SetTransmitHookListener::RemoveSetTransmitHookListener(this);
	SourceSdk::InterfacesProxy::GetGameEventManager()->RemoveListener(this);

	PLAYERS_LOOP_RUNTIME
	{
		ResetPlayerDataStruct(ph->playerClass);
	}
	END_PLAYERS_LOOP
}

bool AntiFlashbangBlocker::SetTransmitCallback(SourceSdk::edict_t* const ea, SourceSdk::edict_t* const eb)
{
	METRICS_ENTER_SECTION("AntiFlashbangBlocker::SetTransmitCallback");
	if(IsActive())
	{
		if(NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(eb)->status == INVALID)
		{
			METRICS_LEAVE_SECTION("AntiFlashbangBlocker::SetTransmitCallback");
			return false;
		}

		NczPlayer* const pPlayer = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(eb)->playerClass;
		void* const player_info = pPlayer->GetPlayerInfo();
		if(!player_info) return false;
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			if (static_cast<SourceSdk::IPlayerInfo_csgo*>(player_info)->IsFakeClient())
			{
				METRICS_LEAVE_SECTION("AntiFlashbangBlocker::SetTransmitCallback");
				return false;
			}
		}
		else
		{
			if (static_cast<SourceSdk::IPlayerInfo*>(player_info)->IsFakeClient())
			{
				METRICS_LEAVE_SECTION("AntiFlashbangBlocker::SetTransmitCallback");
				return false;
			}
		}

		FlashInfoT* const pInfo = GetPlayerDataStruct(pPlayer);

		if (pInfo->flash_end_time != 0.0)
		{
			if (pInfo->flash_end_time > Plat_FloatTime())
			{
				METRICS_LEAVE_SECTION("AntiFlashbangBlocker::SetTransmitCallback");
				return true;
			}
		
			Helpers::FadeUser(eb, 0);
			ResetPlayerDataStruct(eb);
		}
	}

	METRICS_LEAVE_SECTION("AntiFlashbangBlocker::SetTransmitCallback");
	return false;
}

void AntiFlashbangBlocker::FireGameEvent(SourceSdk::IGameEvent* ev) // player_blind
{
	METRICS_ENTER_SECTION("AntiFlashbangBlocker::FireGameEvent");
	if(!IsActive()) return;

	PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByUserId(ev->GetInt("userid", 0));
	if(ph->status == INVALID)
	{
		METRICS_LEAVE_SECTION("AntiFlashbangBlocker::FireGameEvent");
		return;
	}

	if(ph->status >= PLAYER_CONNECTED)
	{
		FlashInfoT* const pInfo = GetPlayerDataStruct(ph->playerClass);
		const float flash_alpha = *EntityProps::GetInstance()->GetPropValue<float, PROP_FLASH_MAX_ALPHA>(ph->playerClass->GetEdict());
		const float flash_duration = *EntityProps::GetInstance()->GetPropValue<float, PROP_FLASH_DURATION>(ph->playerClass->GetEdict());
		
		if (flash_alpha < 255.0)
		{
			ResetPlayerDataStruct(ph->playerClass);
			METRICS_LEAVE_SECTION("AntiFlashbangBlocker::FireGameEvent");
			return;
		}
		
		if (flash_duration > 2.9)
		{
			pInfo->flash_end_time = Plat_FloatTime() + flash_duration - 2.9f;
		}
		else
		{
			pInfo->flash_end_time = Plat_FloatTime() + flash_duration / 10.0f;
		}
		
		Helpers::FadeUser(ph->playerClass->GetEdict(), (short)floorf(flash_duration * 1000.0f));
	}
	METRICS_LEAVE_SECTION("AntiFlashbangBlocker::FireGameEvent");
}
