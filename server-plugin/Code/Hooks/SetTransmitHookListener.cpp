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
#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h"
#include "plugin.h"
#include "Systems/Logger.h"
#include "Systems/ConfigManager.h"
#include "Systems/AutoTVRecord.h"

SetTransmitHookListener::TransmitListenersListT SetTransmitHookListener::m_listeners;

SetTransmitHookListener::SetTransmitHookListener()
{
}

SetTransmitHookListener::~SetTransmitHookListener()
{
}

void SetTransmitHookListener::HookSetTransmit(SourceSdk::edict_t const * const ent)
{
	Assert(Helpers::isValidEdict(ent));
	void* unk = ent->m_pUnk;

	HookInfo info(unk, ConfigManager::GetInstance()->vfid_settransmit, (DWORD)nSetTransmit);
	HookGuard::GetInstance()->VirtualTableHook(info);

	//DebugMessage(basic_string("Hooked SetTransmit of entity classname ").append(ent->GetClassName()));
}

#ifdef GNUC
void HOOKFN_INT SetTransmitHookListener::nSetTransmit(void * const This, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways)
#else
void HOOKFN_INT SetTransmitHookListener::nSetTransmit(void * const This, void * const, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways)
#endif
{
	PlayerHandler::const_iterator receiver_assumed_player(Helpers::IndexOfEdict(*pInfo));

	if (!bAlways && !(m_listeners.GetFirst() == nullptr) && receiver_assumed_player > PLAYER_CONNECTING)
	{
		NczPlayerManager const * const inst(NczPlayerManager::GetInstance());
		PlayerHandler::const_iterator sender_assumed_client(inst->GetPlayerHandlerByBasePlayer(This));

		if (sender_assumed_client)
		{
			if (sender_assumed_client != receiver_assumed_player)
			{
				TransmitListenersListT::elem_t* it = m_listeners.GetFirst();

				while (it != nullptr)
				{
					if (it->m_value.listener->SetTransmitCallback(sender_assumed_client, receiver_assumed_player))
					{
						return;
					}
					it = it->m_next;
				}
			}
		}
		else // is not a player
		{
			SourceSdk::edict_t const * const pEdict_sender = Helpers::edictOfUnknown(This);
			TransmitListenersListT::elem_t* it = m_listeners.GetFirst();

			Assert(Helpers::IndexOfEdict(pEdict_sender) > inst->GetMaxIndex());

			if (receiver_assumed_player > PLAYER_CONNECTING)
			{
				while (it != nullptr)
				{
					if (it->m_value.listener->SetTransmitWeaponCallback(pEdict_sender, receiver_assumed_player))
					{
						return;
					}
					it = it->m_next;
				}
			}
		}
	}

	SetTransmit_t gpOldFn;
	*(uint32_t*)&(gpOldFn) = HookGuard::GetInstance()->GetOldFunction(This, ConfigManager::GetInstance()->vfid_settransmit);
	gpOldFn(This, pInfo, bAlways);
}

void SetTransmitHookListener::RegisterSetTransmitHookListener(SetTransmitHookListener const * const listener, size_t const priority)
{
	m_listeners.Add(listener, priority);
}

void SetTransmitHookListener::RemoveSetTransmitHookListener(SetTransmitHookListener const * const listener)
{
	m_listeners.Remove(listener);
}
