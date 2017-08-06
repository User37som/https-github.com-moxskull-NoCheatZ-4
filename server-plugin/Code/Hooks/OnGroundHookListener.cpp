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

HookGuard<OnGroundHookListener> g_HookGuardOnGroundHookListener;

OnGroundHookListener::OnGroundHookListener ()
{
}

OnGroundHookListener::~OnGroundHookListener ()
{
}

void OnGroundHookListener::HookOnGround ( PlayerHandler::iterator ph )
{
	LoggerAssert ( Helpers::isValidEdict ( ph->GetEdict () ) );
	void* unk ( ph->GetEdict ()->m_pUnk );

	HookInfo info ( unk, g_ConfigManager.vfid_mhgroundentity, ( DWORD ) RT_nNetworkStateChanged_m_hGroundEntity );
	g_HookGuardOnGroundHookListener.VirtualTableHook ( info, "CBasePlayer::NetworkStateChanged_m_hGroundEntity" );
}

/*void OnGroundHookListener::UnhookOnGround()
{
	if(pdwInterface && gpOldGroundFn)
	{
		VirtualTableHook(pdwInterface, g_ConfigManager.GetVirtualFunctionId("mhgroundentity"), (DWORD)gpOldGroundFn, (DWORD)nNetworkStateChanged_m_hGroundEntity);
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
	PlayerHandler::iterator  ph ( g_NczPlayerManager.GetPlayerHandlerByBasePlayer ( basePlayer ) );
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
	*( DWORD* )&( gpOldFn ) = g_HookGuardOnGroundHookListener.RT_GetOldFunction ( basePlayer, g_ConfigManager.vfid_mhgroundentity );
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
