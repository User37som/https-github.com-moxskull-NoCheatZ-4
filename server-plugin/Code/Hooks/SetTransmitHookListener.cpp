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

#include "Interfaces/InterfacesProxy.h"

#include "Systems/ConfigManager.h"
#include "Players/NczPlayerManager.h"
#include "Hooks/temp_HookGuard.h"
#include "Systems/AutoTVRecord.h"

SetTransmitHookListener::TransmitListenersListT SetTransmitHookListener::m_listeners;

SetTransmitHookListener::SetTransmitHookListener ()
{
	HookGuard<SetTransmitHookListener>::Required ();
}

SetTransmitHookListener::~SetTransmitHookListener ()
{
	if( HookGuard<SetTransmitHookListener>::IsCreated () )
	{
		HookGuard<SetTransmitHookListener>::GetInstance ()->UnhookAll ();
		HookGuard<SetTransmitHookListener>::DestroyInstance ();
	}
}

void SetTransmitHookListener::HookSetTransmit ( SourceSdk::edict_t const * const ent )
{
	LoggerAssert ( Helpers::isValidEdict ( ent ) );
	void* unk ( ent->m_pUnk );

	HookInfo info ( unk, ConfigManager::GetInstance ()->vfid_settransmit, ( DWORD ) RT_nSetTransmit );
	HookGuard<SetTransmitHookListener>::GetInstance ()->VirtualTableHook ( info );
}

#ifdef GNUC
void HOOKFN_INT SetTransmitHookListener::RT_nSetTransmit ( void * const This, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways )
#else
void HOOKFN_INT SetTransmitHookListener::RT_nSetTransmit ( void * const This, void * const, SourceSdk::CCheckTransmitInfo * pInfo, bool bAlways )
#endif
{
	PlayerHandler::const_iterator receiver_assumed_player ( Helpers::IndexOfEdict ( *pInfo ) );

	if( !bAlways && !( m_listeners.GetFirst () == nullptr ) && receiver_assumed_player > SlotStatus::PLAYER_CONNECTING )
	{
		NczPlayerManager const * const inst ( NczPlayerManager::GetInstance () );
		PlayerHandler::const_iterator sender_assumed_client ( inst->GetPlayerHandlerByBasePlayer ( This ) );

		if( sender_assumed_client )
		{
			if( sender_assumed_client != receiver_assumed_player && sender_assumed_client != SlotStatus::PLAYER_CONNECTING && sender_assumed_client >= SlotStatus::BOT )
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
		}
		else // is not a player
		{
			SourceSdk::edict_t const * const pEdict_sender ( Helpers::edictOfUnknown ( This ) );
			TransmitListenersListT::elem_t* it ( m_listeners.GetFirst () );

			LoggerAssert ( Helpers::IndexOfEdict ( pEdict_sender ) > inst->GetMaxIndex () );

			while( it != nullptr )
			{
				if( it->m_value.listener->RT_SetTransmitWeaponCallback ( pEdict_sender, receiver_assumed_player ) )
				{
					return;
				}
				it = it->m_next;
			}
		}
	}

	SetTransmit_t gpOldFn;
	*( uint32_t* )&( gpOldFn ) = HookGuard<SetTransmitHookListener>::GetInstance ()->RT_GetOldFunction ( This, ConfigManager::GetInstance ()->vfid_settransmit );
	gpOldFn ( This, pInfo, bAlways );
}

void SetTransmitHookListener::RegisterSetTransmitHookListener ( SetTransmitHookListener const * const listener, size_t const priority )
{
	m_listeners.Add ( listener, priority );
}

void SetTransmitHookListener::RemoveSetTransmitHookListener ( SetTransmitHookListener const * const listener )
{
	m_listeners.Remove ( listener );
}
