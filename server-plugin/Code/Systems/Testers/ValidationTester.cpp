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

#include <iostream>

#include "ValidationTester.h"

#include "Misc/Helpers.h"

#include "Systems/Logger.h"

/////////////////////////////////////////////////////////////////////////
// ValidationTester
/////////////////////////////////////////////////////////////////////////

ValidationTester::ValidationTester() : 
	BaseSystem("ValidationTester", PLAYER_CONNECTED, INVALID, STATUS_EQUAL_OR_BETTER),
	NczFilteredPlayersList(),
	OnTickListener(),
	playerdata_class(),
	singleton_class()
{
}

ValidationTester::~ValidationTester()
{
	Unload();
}

void ValidationTester::Init()
{
	InitDataStruct();
}

void ValidationTester::SetValidated(NczPlayer* player)
{
	GetPlayerDataStruct(player)->b = true;
	std::cout << Plat_FloatTime() << " : " << player->GetName() << " SteamID validated\n";
	SystemVerbose1(("%s SteamID validated", player->GetName()));
}

void ValidationTester::ProcessPlayerTestOnTick(NczPlayer* player)
{
	if(!IsActive() || !SteamGameServer_BSecure()) return;
	if(GetPlayerDataStruct(player)->b)
		return;

	SystemVerbose1(Helpers::format("%s SteamID not validated, validation time left : %f", player->GetName(), 20.0f - player->GetTimeConnected()));

	if(player->GetTimeConnected() < 20.0f) return;
	player->Kick();
}

void ValidationTester::Load()
{
	OnTickListener::RegisterOnTickListener(this);
}

void ValidationTester::Unload()
{
	OnTickListener::RemoveOnTickListener(this);

	PLAYERS_LOOP_RUNTIME
	{
		ResetPlayerDataStruct(ph->playerClass);
	}
	END_PLAYERS_LOOP
}

bool ValidationTester::JoinCallback(NczPlayer* const player)
{
	if(IsActive() && SteamGameServer_BSecure())
	{
		if(!GetPlayerDataStruct(player)->b)
		{
			Helpers::tell(player->GetEdict(), "You can't join the game now because your Steam ID is not validated yet.\nRetry with the F9 button.");
			return true;
		}
	}
	return false;
}

void ValidationTester::AddPendingValidation(const char* steamid)
{
	m_pending_validations.Add(steamid);
}

void ValidationTester::ProcessOnTick()
{
	PendingValidationsT::elem_t* it = m_pending_validations.GetFirst();
	while (it != nullptr)
	{
		PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerBySteamID(it->m_value);

		if (ph->status == INVALID)
		{
			it = it->m_next;
			continue;
		}

		SetValidated(ph->playerClass);
		it = m_pending_validations.Remove(it);
	}
}
