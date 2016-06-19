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

#include "BadUserCmdBlocker.h"

#include "Interfaces/InterfacesProxy.h"

BadUserCmdBlocker::BadUserCmdBlocker() :
	BaseSystem("BadUserCmdBlocker", PLAYER_CONNECTED, PLAYER_CONNECTING, STATUS_EQUAL_OR_BETTER),
	playerdatahandler_class(),
	PlayerRunCommandHookListener(),
	singleton_class()
{
	METRICS_ADD_TIMER("BadUserCmdBlocker::PlayerRunCommandCallback", 2.0);
}

BadUserCmdBlocker::~BadUserCmdBlocker()
{
	Unload();
}

void BadUserCmdBlocker::Init()
{
	InitDataStruct();
}

void BadUserCmdBlocker::Load()
{
	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if (it)
			ResetPlayerDataStruct(*it);
	}

	RegisterPlayerRunCommandHookListener(this, 0, PLAYER_CONNECTED);
}

void BadUserCmdBlocker::Unload()
{
	RemovePlayerRunCommandHookListener(this);
}

PlayerRunCommandRet BadUserCmdBlocker::PlayerRunCommandCallback(PlayerHandler::const_iterator ph, void* pCmd, void* old_cmd)
{
	METRICS_ENTER_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");

	if(static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->command_number <= 0)
	{
		METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
		return BLOCK;
	}

	UserCmdInfo* const pInfo = GetPlayerDataStruct(ph);

	if (static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->tick_count <= 0)
		pInfo->m_tick_status = IN_RESET;

	bool isDead = true;
	SourceSdk::IPlayerInfo * const player_info = ph->GetPlayerInfo();
	if (player_info)
	{
		isDead = player_info->IsDead();
	}

	if (isDead || pInfo->m_prev_dead || Plat_FloatTime() <= pInfo->m_detected_time)
	{
		pInfo->m_prev_dead = isDead;
		
		if (static_cast<SourceSdk::CUserCmd_csgo*>(old_cmd)->command_number >= static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->command_number)
		{
			if (pInfo->m_tick_status == IN_RESET)
				pInfo->m_tick_status = RESET;
		}
		else
		{
			if (pInfo->m_tick_status == RESET)
				pInfo->m_tick_status = OK;
		}
		
		METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
		return CONTINUE;
	}

	if (static_cast<SourceSdk::CUserCmd_csgo*>(old_cmd)->command_number > static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->command_number)
	{
		if (pInfo->m_tick_status != OK)
		{
			pInfo->m_tick_status = RESET;
			METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
			return BLOCK;
		}
	
		pInfo->m_detected_time = Plat_FloatTime() + 10.0f;
		
		// Push detection for reusing command

		METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
		return BLOCK;
	}

	if (static_cast<SourceSdk::CUserCmd_csgo*>(old_cmd)->command_number == static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->command_number)
	{
		if (pInfo->m_tick_status != OK)
		{
			pInfo->m_tick_status = RESET;

			METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
			return BLOCK;
		}
	
		if (static_cast<SourceSdk::CUserCmd_csgo*>(old_cmd)->tick_count+1 != static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->tick_count)
		{
			pInfo->m_detected_time = Plat_FloatTime() + 10.0f;
			
			// Push detection, reusing command
			
			METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
			return BLOCK;
		}
	}

	int z;
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		z = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
	}
	else
	{
		z = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
	}
	z += 8;

	if(static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->tick_count > z)
	{
		METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
		return INERT;
	}

	METRICS_LEAVE_SECTION("BadUserCmdBlocker::PlayerRunCommandCallback");
	return CONTINUE;
}
