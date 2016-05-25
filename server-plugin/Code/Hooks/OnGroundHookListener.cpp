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

#include "OnGroundHookListener.h"

#include "Interfaces/iserverunknown.h"
#include "Console/convar.h"

#include "plugin.h"
#include "Systems/ConfigManager.h"

OnGroundListenersListT OnGroundHookListener::m_listeners;

OnGroundHookListener::OnGroundHookListener()
{
}

OnGroundHookListener::~OnGroundHookListener()
{
}

void OnGroundHookListener::HookOnGround(NczPlayer* player)
{
	Assert(Helpers::isValidEdict(player->GetEdict()));
	void* unk = player->GetEdict()->m_pUnk;

	HookInfo info(unk, ConfigManager::GetInstance()->GetVirtualFunctionId("mhgroundentity"), (DWORD)nNetworkStateChanged_m_hGroundEntity);
	HookGuard::GetInstance()->VirtualTableHook(info);
}

/*void OnGroundHookListener::UnhookOnGround()
{
	if(pdwInterface && gpOldGroundFn)
	{
		VirtualTableHook(pdwInterface, ConfigManager::GetInstance()->GetVirtualFunctionId("mhgroundentity"), (DWORD)gpOldGroundFn, (DWORD)nNetworkStateChanged_m_hGroundEntity);
		pdwInterface = nullptr;
		gpOldGroundFn = nullptr;
	}
}*/

#ifdef GNUC
void HOOKFN_INT OnGroundHookListener::nNetworkStateChanged_m_hGroundEntity(CBasePlayer* basePlayer, int * new_m_hGroundEntity)
#else
void HOOKFN_INT OnGroundHookListener::nNetworkStateChanged_m_hGroundEntity(CBasePlayer* basePlayer, void*, int * new_m_hGroundEntity)
#endif
{
	PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByBasePlayer(basePlayer);
	bool new_isOnground = true;

	if(ph->status >= PLAYER_CONNECTED)
	{
		if(*new_m_hGroundEntity != -1) new_isOnground = false;

		OnGroundListenersListT::elem_t* it = m_listeners.GetFirst();
		while (it != nullptr)
		{
			it->m_value.listener->m_hGroundEntityStateChangedCallback(ph->playerClass, new_isOnground);
			it = it->m_next;
		}
	}

	ST_W_STATIC GroundEntity_t gpOldFn;
	*(DWORD*)&(gpOldFn) = HookGuard::GetInstance()->GetOldFunction(basePlayer);
	gpOldFn(basePlayer, new_m_hGroundEntity);
}

void OnGroundHookListener::RegisterOnGroundHookListener(OnGroundHookListener* listener)
{
	m_listeners.Add(listener);
}

void OnGroundHookListener::RemoveOnGroundHookListener(OnGroundHookListener* listener)
{
	m_listeners.Remove(listener);
}
