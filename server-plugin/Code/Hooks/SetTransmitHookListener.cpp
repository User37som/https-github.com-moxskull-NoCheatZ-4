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

#include "SetTransmitHookListener.h"

#include "Console/convar.h"

#include "Misc/Helpers.h"
#include "plugin.h"
#include "Systems/Logger.h"
#include "Systems/ConfigManager.h"
#include "Systems/AutoTVRecord.h"

TransmitListenersListT SetTransmitHookListener::m_listeners;

SetTransmitHookListener::SetTransmitHookListener()
{
}

SetTransmitHookListener::~SetTransmitHookListener()
{
}

void SetTransmitHookListener::HookSetTransmit(SourceSdk::edict_t* ent)
{
	Assert(Helpers::isValidEdict(ent));
	void* unk = ent->m_pUnk;

	HookInfo info(unk, ConfigManager::GetInstance()->GetVirtualFunctionId("settransmit"), (DWORD)nSetTransmit);
	HookGuard::GetInstance()->VirtualTableHook(info);

	//DebugMessage(basic_string("Hooked SetTransmit of entity classname ").append(ent->GetClassName()));
}

/*void SetTransmitHookListener::UnhookSetTransmit()
{
	InstancesListT::elem_t* it = m_hooked_instances.GetFirst();
	while (it != nullptr)
	{
		VirtualTableHook(it->m_value->pInterface, ConfigManager::GetInstance()->GetVirtualFunctionId("settransmit"), it->m_value->oldFn, (DWORD)nSetTransmit);
		m_hooked_instances.Remove(it);
		it = m_hooked_instances.GetFirst();
	}	
}*/

#ifdef GNUC
void HOOKFN_INT SetTransmitHookListener::nSetTransmit(SourceSdk::CBaseEntity* This, SourceSdk::CCheckTransmitInfo* pInfo, bool bAlways)
#else
void HOOKFN_INT SetTransmitHookListener::nSetTransmit(SourceSdk::CBaseEntity* This, void*, SourceSdk::CCheckTransmitInfo* pInfo, bool bAlways)
#endif
{
	PlayerHandler* pplayer = NczPlayerManager::GetInstance()->GetPlayerHandlerByBasePlayer(This);
	if (pplayer->status > INVALID)
	{
		SourceSdk::edict_t* const pEdict_sender = pplayer->playerClass->GetEdict();
		//Assert(Helpers::isValidEdict(pEdict_sender));

		SourceSdk::edict_t* const pEdict_receiver = *pInfo;
		Assert(Helpers::isValidEdict(pEdict_receiver));

		if (Helpers::IndexOfEdict(pEdict_receiver) != AutoTVRecord::GetInstance()->GetSlot())
		{

			if (pEdict_sender != pEdict_receiver)
			{
				TransmitListenersListT::elem_t* it = m_listeners.GetFirst();

				int maxclients;
				if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
				{
					maxclients = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
				}
				else
				{
					maxclients = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
				}

				if (Helpers::IndexOfEdict(pEdict_sender) <= maxclients)
				{
					while (it != nullptr)
					{
						if (it->m_value.listener->SetTransmitCallback(pEdict_sender, pEdict_receiver))
						{
							return;
						}
						it = it->m_next;
					}
				}
				else /*if(sender_type == WEAPON)*/
				{
					while (it != nullptr)
					{
						if (it->m_value.listener->SetTransmitWeaponCallback(pEdict_sender, pEdict_receiver))
						{
							return;
						}
						it = it->m_next;
					}
				}
			}
		}
	}
	SetTransmit_t gpOldFn;
	*(uint32_t*)&(gpOldFn) = HookGuard::GetInstance()->GetOldFunction(This, ConfigManager::GetInstance()->GetVirtualFunctionId("settransmit"));
	gpOldFn(This, pInfo, bAlways);
}

void SetTransmitHookListener::RegisterSetTransmitHookListener(SetTransmitHookListener* listener, size_t priority)
{
	m_listeners.Add(listener, priority);
}

void SetTransmitHookListener::RemoveSetTransmitHookListener(SetTransmitHookListener* listener)
{
	m_listeners.Remove(listener);
}
