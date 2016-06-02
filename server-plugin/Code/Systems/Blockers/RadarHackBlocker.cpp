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

#include "Misc/EntityProps.h"
#include "Misc/UserMsg.h"
#include "Hooks/PlayerRunCommandHookListener.h"

RadarHackBlocker::RadarHackBlocker() :
	BaseSystem("RadarHackBlocker", PLAYER_CONNECTED, PLAYER_CONNECTING, STATUS_EQUAL_OR_BETTER),
	playerdatahandler_class(),
	ThinkPostHookListener(),
	UserMessageHookListener(),
	singleton_class()
{

}

RadarHackBlocker::~RadarHackBlocker()
{

}

void RadarHackBlocker::OnMapStart()
{
	if (!GetDisabledByConfigIni())
	{
		m_players_spotted = nullptr;
		m_bomb_spotted = nullptr;

		for (int x = 0; x < MAX_EDICTS; ++x)
		{
			SourceSdk::edict_t * const ent = Helpers::PEntityOfEntIndex(x);
			if (Helpers::isValidEdict(ent))
			{
				if (ent->GetClassName() != nullptr)
				{
#undef GetClassName
					if (basic_string("cs_player_manager").operator==(ent->GetClassName()))
					{
						m_players_spotted = EntityProps::GetInstance()->GetPropValue<bool, PROP_PLAYER_SPOTTED>(ent);
						m_bomb_spotted = EntityProps::GetInstance()->GetPropValue<bool, PROP_BOMB_SPOTTED>(ent);
						ThinkPostHookListener::HookThinkPost(ent);
						break;
					}
				}
			}
		}
	}
}

void RadarHackBlocker::Init()
{
	InitDataStruct();
}

void RadarHackBlocker::Load()
{
	ThinkPostHookListener::RegisterThinkPostHookListener(this);
	UserMessageHookListener::RegisterUserMessageHookListener(this);
	OnTickListener::RegisterOnTickListener(this);
}

void RadarHackBlocker::Unload()
{
	OnTickListener::RemoveOnTickListener(this);
	UserMessageHookListener::RemoveUserMessageHookListener(this);
	ThinkPostHookListener::RemoveThinkPostHookListener(this);
}

void RadarHackBlocker::ThinkPostCallback(SourceSdk::edict_t const * const pent)
{
	float const curtime = Plat_FloatTime();
	PLAYERS_LOOP_RUNTIME
	{
		ClientRadarData * pData = GetPlayerDataStruct(x);
		if (pData->m_last_spotted_status != m_players_spotted[x])
		{
			pData->m_last_spotted_status = m_players_spotted[x];

			if (pData->m_last_spotted_status)
			{
				UpdatePlayerData(ph->playerClass);
				ProcessEntity(Helpers::PEntityOfEntIndex(x));
			}
			else
			{
				pData->m_next_update = curtime + 1.0f;
				UpdatePlayerData(ph->playerClass);
				ProcessEntity(Helpers::PEntityOfEntIndex(x));
			}
		}
	}
	END_PLAYERS_LOOP
}

bool RadarHackBlocker::SendUserMessageCallback(SourceSdk::IRecipientFilter &, int message_id, google::protobuf::Message const &)
{
	return (message_id == CS_UM_ProcessSpottedEntityUpdate);
}

bool RadarHackBlocker::UserMessageBeginCallback(SourceSdk::IRecipientFilter *, int message_id)
{
	return (message_id == UpdateRadar);
}

void RadarHackBlocker::SendApproximativeRadarUpdate(MRecipientFilter & filter, ClientRadarData const * pData) const
{
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate* pBuffer = (CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate *)g_Cstrike15UsermessageHelpers.GetPrototype(CS_UM_ProcessSpottedEntityUpdate)->New();

		pBuffer->set_entity_idx(pData->m_origin_index);
		pBuffer->set_class_id(35);
		pBuffer->set_origin_x((int32_t)pData->m_origin.x);
		pBuffer->set_origin_y((int32_t)pData->m_origin.y);
		pBuffer->set_origin_z((int32_t)pData->m_origin.z);
		pBuffer->set_angle_y((int32_t)pData->m_yawangle);
		//pBuffer->set_defuser();
		//pBuffer->set_player_has_defuser();
		//pBuffer->set_player_has_c4();

		SourceSdk::InterfacesProxy::Call_SendUserMessage(&(filter), CS_UM_ProcessSpottedEntityUpdate, *pBuffer);
		SourceSdk::InterfacesProxy::Call_MessageEnd();
		delete pBuffer;
	}
	else
	{
		SourceSdk::bf_write *pBuffer = SourceSdk::InterfacesProxy::Call_UserMessageBegin(&(filter), /*eUserMsg::UpdateRadar*/ 28);

		SourceSdk::BfWriteByte(pBuffer, pData->m_origin_index);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round(pData->m_origin.x * 0.25f), 13);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round(pData->m_origin.y * 0.25f), 13);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round((float)std::rand()), 13);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round(pData->m_yawangle), 9);

		SourceSdk::BfWriteByte(pBuffer, 0);
		SourceSdk::InterfacesProxy::Call_MessageEnd();
	}
}

void RadarHackBlocker::SendRandomRadarUpdate(MRecipientFilter & filter, ClientRadarData const * pData) const
{
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate* pBuffer = (CCSUsrMsg_ProcessSpottedEntityUpdate_SpottedEntityUpdate *)g_Cstrike15UsermessageHelpers.GetPrototype(CS_UM_ProcessSpottedEntityUpdate)->New();

		pBuffer->set_entity_idx(pData->m_origin_index);
		pBuffer->set_class_id(35);
		pBuffer->set_origin_x((int32_t)pData->m_origin.x);
		pBuffer->set_origin_y((int32_t)pData->m_origin.y);
		pBuffer->set_origin_z((int32_t)pData->m_origin.z);
		pBuffer->set_angle_y((int32_t)pData->m_yawangle);
		pBuffer->set_defuser(false);
		pBuffer->set_player_has_defuser(false);
		pBuffer->set_player_has_c4(false);

		SourceSdk::InterfacesProxy::Call_SendUserMessage(&(filter), CS_UM_ProcessSpottedEntityUpdate, *pBuffer);
		SourceSdk::InterfacesProxy::Call_MessageEnd();
		delete pBuffer;
	}
	else
	{
		SourceSdk::bf_write *pBuffer = SourceSdk::InterfacesProxy::Call_UserMessageBegin(&(filter), /*eUserMsg::UpdateRadar*/ 28);

		SourceSdk::BfWriteByte(pBuffer, pData->m_origin_index);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round((float)std::rand()), 13);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round((float)std::rand()), 13);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round((float)std::rand()), 13);
		SourceSdk::BfWriteSBitLong(pBuffer, (long)std::round((float)std::rand()), 9);

		SourceSdk::BfWriteByte(pBuffer, 0);
		SourceSdk::InterfacesProxy::Call_MessageEnd();
	}
}

void RadarHackBlocker::ProcessEntity(SourceSdk::edict_t const * const pent)
{
	ClientRadarData * pData = GetPlayerDataStruct(Helpers::IndexOfEdict(pent));

	MRecipientFilter filter;
	filter.SetReliable(false);

	if (pData->m_last_spotted_status == true)
	{
		/*
			Player is spotted -> send approximative data to all players
		*/

		filter.AddAllPlayers(NczPlayerManager::GetInstance()->GetMaxIndex());

		SendApproximativeRadarUpdate(filter, pData);
	}
	else
	{
		/*
			Player is not spotted -> send approximative data to teammates and spectators, random for others.
		*/

		filter.AddTeam(pData->m_team);
		filter.AddTeam(TEAM_SPECTATOR);
		filter.AddTeam(TEAM_NONE);

		SendApproximativeRadarUpdate(filter, pData);

		filter.RemoveAll();

		if (pData->m_team == TEAM_1)
		{
			filter.AddTeam(TEAM_2);
		}
		else if(pData->m_team == TEAM_2)
		{
			filter.AddTeam(TEAM_1);
		}
		else
		{
			return;
		}

		filter.RemoveRecipient(pData->m_origin_index);

		SendRandomRadarUpdate(filter, pData);
	}
}

void RadarHackBlocker::UpdatePlayerData(NczPlayer* pPlayer)
{
	ClientRadarData * pData = GetPlayerDataStruct(pPlayer);

	void* playerinfo = pPlayer->GetPlayerInfo();
	if (playerinfo == nullptr) return;

	pData->m_origin_index = pPlayer->GetIndex();

	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		SourceSdk::VectorCopy(static_cast<SourceSdk::IPlayerInfo_csgo*>(playerinfo)->GetAbsOrigin(), pData->m_origin);
		pData->m_yawangle = static_cast<SourceSdk::CUserCmd_csgo*>(PlayerRunCommandHookListener::GetLastUserCmd(pPlayer))->viewangles.y;
		pData->m_team = static_cast<SourceSdk::IPlayerInfo_csgo*>(playerinfo)->GetTeamIndex();
	}
	else
	{
		SourceSdk::VectorCopy(static_cast<SourceSdk::IPlayerInfo*>(playerinfo)->GetAbsOrigin(), pData->m_origin);
		pData->m_yawangle = static_cast<SourceSdk::CUserCmd*>(PlayerRunCommandHookListener::GetLastUserCmd(pPlayer))->viewangles.y;
		pData->m_team = static_cast<SourceSdk::IPlayerInfo*>(playerinfo)->GetTeamIndex();
	}
}

void RadarHackBlocker::ProcessOnTick(float const curtime)
{
	float const curtime = Plat_FloatTime();
	ClientRadarData * pData = GetPlayerDataStruct(player);

	if (pData->m_last_spotted_status)
	{
		UpdatePlayerData(player);
		ProcessEntity(player->GetEdict());
	}
	else
	{
		if (curtime > pData->m_next_update)
		{
			pData->m_next_update = curtime + 1.0f;
			UpdatePlayerData(player);
			ProcessEntity(player->GetEdict());
		}
	}
}
