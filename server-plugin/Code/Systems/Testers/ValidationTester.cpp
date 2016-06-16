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

#include "ValidationTester.h"

#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h"
#include "Systems/Logger.h"

/////////////////////////////////////////////////////////////////////////
// ValidationTester
/////////////////////////////////////////////////////////////////////////

ValidationTester::ValidationTester() : 
	BaseSystem("ValidationTester", PLAYER_CONNECTED, INVALID, STATUS_EQUAL_OR_BETTER),
	SourceSdk::IGameEventListener002(),
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

void ValidationTester::SetValidated(NczPlayer const * const player)
{
	GetPlayerDataStruct(player)->b = true;
	DebugMessage(Helpers::format("%s SteamID validated\n", player->GetName()));
	SystemVerbose1(Helpers::format("%s SteamID validated", player->GetName()));

	Assert(player->GetPlayerInfo());

	ValidatedIdsT::elem_t const * const it = m_validated_ids.Find(ValidatedInfo(player->GetSteamID()));

	if (it == nullptr) m_validated_ids.Add(ValidatedInfo(player->GetSteamID(), player->GetIPAddress()));
}

void ValidationTester::ProcessPlayerTestOnTick(NczPlayer* const player, float const curtime)
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
	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if (it)
			ResetPlayerDataStruct(*it);
	}

	OnTickListener::RegisterOnTickListener(this);
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "player_disconnect", true);
}

void ValidationTester::Unload()
{
	OnTickListener::RemoveOnTickListener(this);
	SourceSdk::InterfacesProxy::GetGameEventManager()->RemoveListener(this);
}

bool ValidationTester::JoinCallback(NczPlayer const * const player)
{
	if(IsActive() && SteamGameServer_BSecure())
	{
		if(!GetPlayerDataStruct(player)->b)
		{
			if (!WasPreviouslyValidated(player))
			{
				Helpers::tell(player->GetEdict(), "You can't join the game now because your Steam ID is not validated yet.\nRetry with the F9 button or activate Steam and restart the game.");
			}
			else
			{
				DebugMessage(Helpers::format("%s SteamID was previously validated with the same IP. Plugin will flag this player as validated.\n", player->GetName()));
				SetValidated(player);
			}
			return true;
		}
	}
	return false;
}

void ValidationTester::AddPendingValidation(const char *pszUserName, const char* steamid)
{
	m_pending_validations.Add(steamid);
}

bool ValidationTester::WasPreviouslyValidated(NczPlayer const * const player)
{
	Assert(player->GetPlayerInfo());

	ValidatedIdsT::elem_t const * const it = m_validated_ids.Find(ValidatedInfo(player->GetSteamID()));

	if (it == nullptr) return false;
	else if (strcmp(it->m_value.m_ipaddress, player->GetIPAddress()) == 0) return true;
	else return false;
}

void ValidationTester::ProcessOnTick(float const curtime)
{
	PendingValidationsT::elem_t* it = m_pending_validations.GetFirst();
	while (it != nullptr)
	{
		PlayerHandler::const_iterator ph = NczPlayerManager::GetInstance()->GetPlayerHandlerBySteamID(it->m_value);

		if (ph == INVALID || ph->GetPlayerInfo() == nullptr)
		{
			it = it->m_next;
			continue;
		}

		SetValidated(ph);
		it = m_pending_validations.Remove(it);
	}
}

void ValidationTester::FireGameEvent(SourceSdk::IGameEvent * ev)
{
	//DebugMessage(Helpers::format("Received player_disconnect event : \n\t%d\n\t%s\n\t%s\n\t%s\n\t%d", ev->GetInt("userid"), ev->GetString("reason"), ev->GetString("name"), ev->GetString("networkid"), ev->GetBool("bot")).c_str());
	m_validated_ids.Remove(ValidatedInfo(ev->GetString("networkid")));
}
