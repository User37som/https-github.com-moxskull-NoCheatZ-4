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

#include <stdio.h>

#include "SpamChangeNameTester.h"

#include "Players/NczPlayerManager.h"
#include "Misc/Helpers.h"
#include "Systems/BanRequest.h"

SpamChangeNameTester::SpamChangeNameTester() :
	BaseSystem("SpamChangeNameTester", PLAYER_CONNECTING, PLAYER_CONNECTING, STATUS_EQUAL_OR_BETTER),
	playerdata_class(),
	IGameEventListener002(),
	OnTickListener(),
	singleton_class()
{
}

SpamChangeNameTester::~SpamChangeNameTester()
{
	Unload();
}

void SpamChangeNameTester::Init()
{
	InitDataStruct();
}

void SpamChangeNameTester::Load()
{
	PLAYERS_LOOP_RUNTIME_UNROLL_NOPH(x)
	{
		ResetPlayerDataStruct(x_index);
	}
	END_PLAYERS_LOOP_UNROLL(x)
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "player_changename", true);
	OnTickListener::RegisterOnTickListener(this);
}

void SpamChangeNameTester::Unload()
{
	OnTickListener::RemoveOnTickListener(this);
	SourceSdk::InterfacesProxy::GetGameEventManager()->RemoveListener(this);
	PLAYERS_LOOP_RUNTIME_UNROLL_NOPH(x)
	{
		ResetPlayerDataStruct(x_index);
	}
	END_PLAYERS_LOOP_UNROLL(x)
}

bool IsNameValid(const char* const o_name)
{
	const char* name = o_name;

	const size_t len = strlen(name);

	if (len < 1 || *name == '&' || Helpers::IsCharSpace(name) || Helpers::IsCharSpace(name+len-1))
		return false;

	do
	{
		const size_t bytes = Helpers::GetUTF8Bytes(name);

		if(bytes > 1)
		{
			if(!Helpers::IsValidUTF8Char(name, bytes))
				return false;

			name += bytes - 1;
			continue;
		}

		if (*name < 32 || *name == '%' || *name == 0x7F)
			return false;
	}
	while((size_t)(++name - o_name) < len);
	
	return true;
}

void SpamChangeNameTester::FireGameEvent(SourceSdk::IGameEvent* ev)
{
	if(!IsActive()) return;

	PlayerHandler* const ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByUserId(ev->GetInt("userid", 0));

	if(ph->status < PLAYER_CONNECTED) return;

	ChangeNameInfo* const pInfo = GetPlayerDataStruct(ph->playerClass);

	++(pInfo->namechange_count);
}

void SpamChangeNameTester::ProcessOnTick(float const curtime)
{
	if(!IsActive()) return;

	PLAYERS_LOOP_RUNTIME_UNROLL(x)
	{
		if (x_ph->status >= PLAYER_CONNECTED)
		{
			ChangeNameInfo* const pInfo = GetPlayerDataStruct(x_ph->playerClass);

			if (pInfo->next_namechange_test < curtime)
			{
				if (pInfo->namechange_count >= 5)
				{
					x_ph->playerClass->Ban("Banned for namechange spamming", 10);
				}
				ResetPlayerDataStruct(x_ph->playerClass);
			}
		}
	}
	END_PLAYERS_LOOP_UNROLL(x)
}

void SpamChangeNameTester::ClientConnect( bool *bAllowConnect, SourceSdk::edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	if(!IsActive()) return;
	if(!*bAllowConnect) return;

	if(!IsNameValid(pszName))
	{
		*bAllowConnect = false;
		strncpy(reject, "Your name is not valid", 23);
	}
}
