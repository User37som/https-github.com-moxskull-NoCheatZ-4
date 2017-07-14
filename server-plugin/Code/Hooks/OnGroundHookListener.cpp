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
#include "Interfaces/edict.h"
#include "Console/convar.h"

#include "plugin.h"
#include "Systems/ConfigManager.h"

OnGroundHookListener::OnGroundListenersListT OnGroundHookListener::m_listeners;

OnGroundHookListener::OnGroundHookListener ()
{
	HookGuard<OnGroundHookListener>::Required ();
}

OnGroundHookListener::~OnGroundHookListener ()
{
	if( HookGuard<OnGroundHookListener>::IsCreated () )
	{
		HookGuard<OnGroundHookListener>::GetInstance ()->UnhookAll ();
		HookGuard<OnGroundHookListener>::DestroyInstance ();
	}
}

void OnGroundHookListener::HookOnGround ( PlayerHandler::iterator ph )
{
	LoggerAssert ( Helpers::isValidEdict ( ph->GetEdict () ) );
	void* unk ( ph->GetEdict ()->m_pUnk );

	HookInfo info ( unk, ConfigManager::GetInstance ()->vfid_mhgroundentity, ( DWORD ) RT_nNetworkStateChanged_m_hGroundEntity );
	HookGuard<OnGroundHookListener>::GetInstance ()->VirtualTableHook ( info, "CBasePlayer::NetworkStateChanged_m_hGroundEntity" );
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
void HOOKFN_INT OnGroundHookListener::RT_nNetworkStateChanged_m_hGroundEntity ( void * const basePlayer, int const * const new_m_hGroundEntity )
#else
void HOOKFN_INT OnGroundHookListener::RT_nNetworkStateChanged_m_hGroundEntity ( void * const basePlayer, void * const, int const * const new_m_hGroundEntity )
#endif
{
	PlayerHandler::iterator  ph ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByBasePlayer ( basePlayer ) );
	bool new_isOnground ( true );

	if( ph >= SlotStatus::PLAYER_CONNECTED )
	{
		if( *new_m_hGroundEntity != -1 ) new_isOnground = false;

		OnGroundListenersListT::elem_t* it ( m_listeners.GetFirst () );
		while( it != nullptr )
		{
			it->m_value.listener->RT_m_hGroundEntityStateChangedCallback ( ph, new_isOnground );
			it = it->m_next;
		}
	}

	ST_W_STATIC GroundEntity_t gpOldFn;
	*( DWORD* )&( gpOldFn ) = HookGuard<OnGroundHookListener>::GetInstance ()->RT_GetOldFunction ( basePlayer, ConfigManager::GetInstance ()->vfid_mhgroundentity );
	gpOldFn ( basePlayer, new_m_hGroundEntity );
}

void OnGroundHookListener::RegisterOnGroundHookListener ( OnGroundHookListener const * const listener )
{
	m_listeners.Add ( listener );
}

void OnGroundHookListener::RemoveOnGroundHookListener ( OnGroundHookListener const * const listener )
{
	m_listeners.Remove ( listener );
}
