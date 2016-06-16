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

#include "NczPlayerManager.h"

#include "Interfaces/usercmd.h"
#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h" // helpers, ifaces
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Systems/Logger.h"
#include "plugin.h"
#include "Systems/BaseSystem.h"

PlayerHandler::const_iterator PlayerHandler::invalid = nullptr;
PlayerHandler::const_iterator PlayerHandler::first = PlayerHandler::invalid;
PlayerHandler::const_iterator PlayerHandler::last = PlayerHandler::invalid;


//---------------------------------------------------------------------------------
// NczPlayerManager
//---------------------------------------------------------------------------------

NczPlayerManager::NczPlayerManager() : singleton_class()
{
	PlayerHandler::invalid = FullHandlersList;
	PlayerHandler::first = PlayerHandler::invalid;
	PlayerHandler::last = PlayerHandler::invalid;

	m_max_index = 0;
}

NczPlayerManager::~NczPlayerManager()
{
	OnLevelInit();
}

void NczPlayerManager::OnLevelInit()
{
	SourceSdk::InterfacesProxy::GetGameEventManager()->RemoveListener(this);
	_PLAYERS_LOOP_INIT
	{
		ph->Reset();
	}
	_END_PLAYERS_LOOP_INIT
	m_max_index = 0;
	PlayerHandler::first = PlayerHandler::invalid;
	PlayerHandler::last = PlayerHandler::invalid;
}

void NczPlayerManager::LoadPlayerManager()
{
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "player_death", true);
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "player_team", true);
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "player_spawn", true);
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "round_end", true);
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "round_freeze_end", true);

	//Helpers::FastScan_EntList();
	Helpers::m_EdictList = Helpers::PEntityOfEntIndex(0);

	//if(Helpers::m_EdictList)
	//{
		//int maxcl = Helpers::GetMaxClients();
		
		for(PlayerHandler::const_iterator ph = PlayerHandler::begin(); ph != PlayerHandler::end(); ++ph)
		{
			SourceSdk::edict_t* const pEntity = Helpers::PEntityOfEntIndex(ph.GetIndex());
			if(Helpers::isValidEdict(pEntity))
			{
				void * playerinfo = SourceSdk::InterfacesProxy::Call_GetPlayerInfo(pEntity);
				if(playerinfo)
				{	
					bool isfakeclient;
					if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
					{
						isfakeclient = static_cast<SourceSdk::IPlayerInfo_csgo*>(playerinfo)->IsFakeClient();
					}
					else
					{
						isfakeclient = static_cast<SourceSdk::IPlayerInfo*>(playerinfo)->IsFakeClient();
					}
					if(isfakeclient)
					{
						ph.GetHandler()->status = BOT;
						ph.GetHandler()->playerClass = new NczPlayer(ph.GetIndex());
						m_max_index = ph.GetIndex();
					}
					else if (static_cast<SourceSdk::IPlayerInfo*>(playerinfo)->IsConnected())
					{
						ph.GetHandler()->status = PLAYER_CONNECTED;
						ph.GetHandler()->playerClass = new NczPlayer(ph.GetIndex());
						ph.GetHandler()->playerClass->OnConnect();
						m_max_index = ph.GetIndex();
					}
				}
			}
		}

	//}

	if (m_max_index)
	{
		PlayerHandler::first = (&FullHandlersList[1]);
		PlayerHandler::last = (&FullHandlersList[m_max_index]);
	}
	else
	{
		PlayerHandler::first = PlayerHandler::invalid;
		PlayerHandler::last = PlayerHandler::invalid;
	}

	BaseSystem::ManageSystems();
}

void NczPlayerManager::ClientConnect(SourceSdk::edict_t* pEntity)
{
	const int index = Helpers::IndexOfEdict(pEntity);
	Assert(index);
	PlayerHandler& ph = FullHandlersList[index];
	Assert(ph.status == INVALID || ph.status == PLAYER_CONNECTING);
	ph.playerClass = new NczPlayer(index);
	// Should not be here, but heh ...
	//*PlayerRunCommandHookListener::GetLastUserCmd(ph.playerClass) = SourceSdk::CUserCmd();
	ph.status = PLAYER_CONNECTING;
	ph.playerClass->OnConnect();

	if(index > m_max_index) m_max_index = index;

	PlayerHandler::first = (&FullHandlersList[1]);
	PlayerHandler::last = (&FullHandlersList[m_max_index]);

	BaseSystem::ManageSystems();
}

void NczPlayerManager::ClientActive(SourceSdk::edict_t* pEntity)
{
	const int index = Helpers::IndexOfEdict(pEntity);
	Assert(index);
	PlayerHandler& ph = FullHandlersList[index];
	if(ph.status == INVALID) // Bots don't call ClientConnect
	{
		ph.playerClass = new NczPlayer(index);
		ph.status = BOT;
	}
	else
	{
		Assert(ph.status == PLAYER_CONNECTING);
		ph.status = PLAYER_CONNECTED;
	}

	if(index > m_max_index) m_max_index = index;

	PlayerHandler::first = (&FullHandlersList[1]);
	PlayerHandler::last = (&FullHandlersList[m_max_index]);

	BaseSystem::ManageSystems();
}

void NczPlayerManager::ClientDisconnect(SourceSdk::edict_t* pEntity)
{
	const int index = Helpers::IndexOfEdict(pEntity);
	Assert(index);
	FullHandlersList[index].Reset();

	while(m_max_index > 0 && FullHandlersList[m_max_index].status == INVALID)
		--m_max_index;

	if (m_max_index)
	{
		PlayerHandler::first = (&FullHandlersList[1]);
		PlayerHandler::last = (&FullHandlersList[m_max_index]);
	}
	else
	{
		PlayerHandler::first = PlayerHandler::invalid;
		PlayerHandler::last = PlayerHandler::invalid;
	}

	BaseSystem::ManageSystems();
}

void NczPlayerManager::FireGameEvent(SourceSdk::IGameEvent* ev)
	/*
		player_death
		player_team
		player_spawn
		round_freeze_end
		round_end
	*/
{
	const char* event_name = ev->GetName()+6;
	const int maxcl = m_max_index;

	if(*event_name == 'e') // round_end
	{
		for(int x = 1; x <= maxcl; ++x)
		{
			SlotStatus& pstat = FullHandlersList[x].status;
			if(pstat == PLAYER_IN_TESTS)
				pstat = PLAYER_CONNECTED;
		}
		BaseSystem::ManageSystems();
		Logger::GetInstance()->Flush();
		return;
	}
	/*else*/ if(*event_name == 'f') // round_freeze_end = round_start
	{
		for(int x = 1; x <= maxcl; ++x)
		{
			PlayerHandler& ph = FullHandlersList[x];
			if(ph.status == INVALID) continue;
			if(ph.status == PLAYER_CONNECTED)
			{
				SourceSdk::IPlayerInfo * const pinfo = ph.playerClass->GetPlayerInfo();
				if(pinfo)
				{
					if (pinfo->GetTeamIndex() > 1)
					{
						ph.in_tests_time = Plat_FloatTime() + 1.0f;
					}
					else
					{
						ph.status = PLAYER_CONNECTED;
						ph.in_tests_time = std::numeric_limits<float>::max();
					}
				}
			}
		}
		BaseSystem::ManageSystems();
		return;
	}

	PlayerHandler * ph = &(FullHandlersList[Helpers::getIndexFromUserID(ev->GetInt("userid"))]);

	++event_name;

	if(*event_name == 's') // player_spawn
	{
		if(ph->status > BOT)
		{
			SourceSdk::IPlayerInfo * const pinfo = ph->playerClass->GetPlayerInfo();
			if(pinfo)
			{
				if (pinfo->GetTeamIndex() > 1)
				{
					ph->in_tests_time = Plat_FloatTime() + 3.0f;
				}
				else
				{
					ph->status = PLAYER_CONNECTED;
					ph->in_tests_time = std::numeric_limits<float>::max();
				}
			}
		}
		BaseSystem::ManageSystems();
		return;
	}
	if(*event_name == 't') // player_team
	{
		if(ph->status > BOT)
		{
			if (ev->GetInt("teamid") > 1)
			{
				ph->in_tests_time = Plat_FloatTime() + 3.0f;
			}
			else
			{
				ph->status = PLAYER_CONNECTED;
				ph->in_tests_time = std::numeric_limits<float>::max();
			}
		}
		BaseSystem::ManageSystems();
		return;
	}
	//else // player_death
	//{
	
	if(ph->status <= PLAYER_CONNECTED)
		return;
	ph->status = PLAYER_CONNECTED;
	ph->in_tests_time = std::numeric_limits<float>::max();
	BaseSystem::ManageSystems();
	//}
}

void NczPlayerManager::DeclareKickedPlayer(int const slot)
{
	FullHandlersList[slot].status = KICK;
}

void NczPlayerManager::Think()
{
	while (m_max_index > 0 && FullHandlersList[m_max_index].status == INVALID)
		--m_max_index;

	if (m_max_index)
	{
		PlayerHandler::first = (&FullHandlersList[1]);
		PlayerHandler::last = (&FullHandlersList[m_max_index]);
	}
	else
	{
		PlayerHandler::first = PlayerHandler::invalid;
		PlayerHandler::last = PlayerHandler::invalid;
	}

	const int maxcl = m_max_index;
	const float gametime = Plat_FloatTime();
	for(int x = 1; x <= maxcl; ++x)
	{
		PlayerHandler& ph = FullHandlersList[x];
		if(ph.status <= PLAYER_CONNECTING) continue;

		if(gametime > ph.in_tests_time)
		{
			ph.status = PLAYER_IN_TESTS;
		}
		else
		{
			ph.status = PLAYER_CONNECTED;
		}
	}
}

PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByBasePlayer(void * const BasePlayer) const
{
	SourceSdk::edict_t const * const ent = SourceSdk::InterfacesProxy::Call_BaseEntityToEdict(BasePlayer);
	
	return Helpers::isValidEdict(ent) ? &FullHandlersList[Helpers::IndexOfEdict(ent)] : PlayerHandler::end();
}

PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerBySteamID(const char * steamid) const
{
	const char * tSteamId;

	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if (it)
		{
			tSteamId = it->GetSteamID();
			if (Helpers::bStrEq(tSteamId, steamid)) return it;
		}
	}

	return PlayerHandler::end();
}

PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByName(const char * playerName) const
{
	const char * tName;

	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if (it)
		{
			tName = it->GetName();
			if (Helpers::bStrEq(tName, playerName)) return it;
		}
	}

	return PlayerHandler::end();
}

short NczPlayerManager::GetPlayerCount(SlotStatus filter, SlotFilterBehavior strict) const
{
	short count = 0;
	const int maxcl = m_max_index;
	int x = 0;

	switch (strict)
	{
	case STATUS_STRICT:
		{
			do
			{
				if (FullHandlersList[x].status == filter) ++count;
			} while (++x <= maxcl);
		}
		break;
	case STATUS_BETTER:
		{
			do
			{
				if (FullHandlersList[x].status > filter) ++count;
			}while (++x <= maxcl);
		}
		break;
	case STATUS_EQUAL_OR_BETTER:
		{
			do
			{
				if (FullHandlersList[x].status >= filter) ++count;
			}while (++x <= maxcl);
		}
		break;
	};

	return count;
}
