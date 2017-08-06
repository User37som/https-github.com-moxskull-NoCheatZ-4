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

HookGuard<SetTransmitHookListener> g_HookGuardSetTransmitHookListener;
HookGuard<SetTransmitHookListenerWeapon> g_HookGuardSetTransmitHookListenerWeapon;

SetTransmitHookListener::SetTransmitHookListener ()
{
}

SetTransmitHookListener::~SetTransmitHookListener ()
{
}

void SetTransmitHookListener::HookSetTransmit ( SourceSdk::edict_t const * const ent, bool isplayer )
{
	LoggerAssert ( Helpers::isValidEdict ( ent ) );
	void* unk ( ent->m_pUnk );

	if (isplayer)
	{
		HookInfo info(unk, g_ConfigManager.vfid_settransmit, (DWORD)RT_nSetTransmit);
		g_HookGuardSetTransmitHookListener.VirtualTableHook(info, "CBaseEntity::SetTransmit");
	}
	else
	{
		HookInfo info(unk, g_ConfigManager.vfid_settransmit, (DWORD)RT_nSetTransmitWeapon);
		g_HookGuardSetTransmitHookListenerWeapon.VirtualTableHook(info, "CBaseEntity::SetTransmit (Weapon)");
	}
}

#ifdef GNUC
void HOOKFN_INT SetTransmitHookListener::RT_nSetTransmit ( void * const This, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways )
#else
void HOOKFN_INT SetTransmitHookListener::RT_nSetTransmit ( void * const This, void * const, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways )
#endif
{
	SetTransmit_t gpOldFn;
	PlayerHandler::iterator receiver_assumed_player ( Helpers::IndexOfEdict ( *pInfo ) );
	PlayerHandler::iterator sender_assumed_client(g_NczPlayerManager.GetPlayerHandlerByBasePlayer(This));

	if( sender_assumed_client != receiver_assumed_player && receiver_assumed_player > SlotStatus::PLAYER_CONNECTING && sender_assumed_client >= SlotStatus::BOT )
	{
		TransmitListenersListT::elem_t* it ( m_listeners.GetFirst () );

		while( it != nullptr )
		{
			if( it->m_value.listener->RT_SetTransmitCallback ( sender_assumed_client, receiver_assumed_player ) )
			{
				return;
			}
			it = it->m_next;
		}
	}

	*(uint32_t*)&(gpOldFn) = g_HookGuardSetTransmitHookListener.RT_GetOldFunction(This, g_ConfigManager.vfid_settransmit);
	gpOldFn ( This, pInfo, bAlways );
}

#ifdef GNUC
void HOOKFN_INT SetTransmitHookListener::RT_nSetTransmitWeapon(void * const This, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways)
#else
void HOOKFN_INT SetTransmitHookListener::RT_nSetTransmitWeapon(void * const This, void * const, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways)
#endif
{
	SetTransmit_t gpOldFn;
	PlayerHandler::iterator receiver_assumed_player(Helpers::IndexOfEdict(*pInfo));

	if (!(m_listeners.GetFirst() == nullptr) && receiver_assumed_player > SlotStatus::PLAYER_CONNECTING)
	{
		SourceSdk::edict_t const * const pEdict_sender(Helpers::edictOfUnknown(This));
		TransmitListenersListT::elem_t* it(m_listeners.GetFirst());

		while (it != nullptr)
		{
			if (it->m_value.listener->RT_SetTransmitWeaponCallback(pEdict_sender, receiver_assumed_player))
			{
				return;
			}
			it = it->m_next;
		}
	}

	*(uint32_t*)&(gpOldFn) = g_HookGuardSetTransmitHookListenerWeapon.RT_GetOldFunction(This, g_ConfigManager.vfid_settransmit);
	gpOldFn(This, pInfo, bAlways);
}

void SetTransmitHookListener::RegisterSetTransmitHookListener ( SetTransmitHookListener const * const listener, size_t const priority )
{
	m_listeners.Add ( listener, priority );
}

void SetTransmitHookListener::RemoveSetTransmitHookListener ( SetTransmitHookListener const * const listener )
{
	m_listeners.Remove ( listener );
}
