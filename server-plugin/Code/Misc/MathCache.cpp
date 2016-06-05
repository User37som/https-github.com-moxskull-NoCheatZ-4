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

#include "MathCache.h"

#include "EntityProps.h"
#include "Interfaces/InterfacesProxy.h"
#include "Helpers.h"
#include "Players/NczPlayerManager.h"
#include "Hooks/PlayerRunCommandHookListener.h"

MathCache::MathCache()
{
	SetCacheExpired();
}

MathCache::~MathCache()
{
	SetCacheExpired();
}

MathInfo& MathCache::GetCachedMaths(int const player_index, bool const force_update /* = false */)
{
	Assert(player_index > 0 && player_index <= 66);

	CacheInfo& item = m_cache[player_index];
	MathInfo& info = item.m_info;
	if ( (!item.m_is_not_expired) || force_update)
	{
		PlayerHandler const * const ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(player_index);

		Assert(ph->status > INVALID);

		void * const playerinfo = ph->playerClass->GetPlayerInfo();

		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			SourceSdk::IPlayerInfo_csgo* const pinfo = static_cast<SourceSdk::IPlayerInfo_csgo*>(playerinfo);

			SourceSdk::VectorCopy(pinfo->GetPlayerMins(), info.m_mins);
			SourceSdk::VectorCopy(pinfo->GetPlayerMaxs(), info.m_maxs);
			SourceSdk::VectorCopy(pinfo->GetAbsOrigin(), info.m_abs_origin);
			if (pinfo->IsFakeClient())
			{
				SourceSdk::VectorCopy(pinfo->GetAbsAngles(), info.m_eyeangles);
			}
			else
			{
				SourceSdk::VectorCopy(static_cast<SourceSdk::CUserCmd_csgo*>(PlayerRunCommandHookListener::GetLastUserCmd(ph->playerClass))->viewangles, info.m_eyeangles);
			}
		}
		else
		{
			SourceSdk::IPlayerInfo* const pinfo = static_cast<SourceSdk::IPlayerInfo*>(playerinfo);

			SourceSdk::VectorCopy(pinfo->GetPlayerMins(), info.m_mins);
			SourceSdk::VectorCopy(pinfo->GetPlayerMaxs(), info.m_maxs);
			SourceSdk::VectorCopy(pinfo->GetAbsOrigin(), info.m_abs_origin);
			if (pinfo->IsFakeClient())
			{
				SourceSdk::VectorCopy(pinfo->GetAbsAngles(), info.m_eyeangles);
			}
			else
			{
				SourceSdk::VectorCopy(static_cast<SourceSdk::CUserCmd_csgo*>(PlayerRunCommandHookListener::GetLastUserCmd(ph->playerClass))->viewangles, info.m_eyeangles);
			}
		}

		SourceSdk::edict_t* const pedict = Helpers::PEntityOfEntIndex(player_index);
		SourceSdk::VectorCopy(*EntityProps::GetInstance()->GetPropValue<SourceSdk::Vector, PROP_ABS_VELOCITY>(pedict, true), info.m_velocity);
		SourceSdk::VectorMultiply(info.m_velocity, 0.01f);
		SourceSdk::VectorAbs(info.m_velocity, info.m_abs_velocity);
		SourceSdk::InterfacesProxy::Call_ClientEarPosition(pedict, &info.m_eyepos);
		
	}
	return info;
}
