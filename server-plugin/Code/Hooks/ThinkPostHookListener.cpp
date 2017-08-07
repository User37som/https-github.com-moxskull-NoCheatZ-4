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

#include "ThinkPostHookListener.h"

#include "Systems/ConfigManager.h"
#include "Misc/Helpers.h"
#include "Interfaces/InterfacesProxy.h"

ThinkPostHookListener::ListenersList_t ThinkPostHookListener::m_listeners;

HookGuard<ThinkPostHookListener> g_HookGuardThinkPostHookListener;

ThinkPostHookListener::ThinkPostHookListener ()
{
}

ThinkPostHookListener::~ThinkPostHookListener ()
{
}

void ThinkPostHookListener::HookThinkPost ( SourceSdk::edict_t const * const entity )
{
	HookInfo info ( entity->m_pUnk, g_ConfigManager.vfid_thinkpost, ( DWORD ) RT_nThinkPost );
	g_HookGuardThinkPostHookListener.VirtualTableHook ( info, "CBaseEntity::Think", true );
}

void ThinkPostHookListener::RegisterThinkPostHookListener ( ThinkPostHookListener const * const listener )
{
	m_listeners.Add ( listener );
}

void ThinkPostHookListener::RemoveThinkPostHookListener ( ThinkPostHookListener const * const listener )
{
	m_listeners.Remove ( listener );
}

#ifdef GNUC
void HOOKFN_INT ThinkPostHookListener::RT_nThinkPost ( void * const baseentity )
#else
void HOOKFN_INT ThinkPostHookListener::RT_nThinkPost ( void * const baseentity, void * const )
#endif
{
	PostThink_t gpOldFn;
	*( DWORD* ) &gpOldFn = g_HookGuardThinkPostHookListener.RT_GetOldFunction ( baseentity, g_ConfigManager.vfid_thinkpost );
	gpOldFn ( baseentity );

	ListenersList_t::elem_t const * it ( m_listeners.GetFirst () );
	SourceSdk::edict_t const * const pent ( SourceSdk::InterfacesProxy::Call_BaseEntityToEdict ( baseentity ) );

	while( it != nullptr )
	{
		it->m_value.listener->RT_ThinkPostCallback ( pent );

		it = it->m_next;
	}
}
