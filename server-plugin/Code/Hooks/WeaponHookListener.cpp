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

#include "WeaponHookListener.h"

#include "Console/convar.h"
#include "Interfaces/InterfacesProxy.h"
#include "Interfaces/iserverunknown.h"

#include "Misc/Helpers.h"
#include "plugin.h"
#include "Systems/ConfigManager.h"

WeaponHookListener::WeaponHookListenersListT WeaponHookListener::m_listeners;

HookGuard<WeaponHookListener> g_HookGuardWeaponHookListener;

WeaponHookListener::WeaponHookListener ()
{
}

WeaponHookListener::~WeaponHookListener ()
{
}

void WeaponHookListener::HookWeapon ( PlayerHandler::iterator ph )
{
	LoggerAssert ( Helpers::isValidEdict ( ph->GetEdict () ) );
	void* unk ( ph->GetEdict ()->m_pUnk );

	HookInfo info_equip ( unk, g_ConfigManager.vfid_weaponequip, ( DWORD ) RT_nWeapon_Equip );
	HookInfo info_drop ( unk, g_ConfigManager.vfid_weapondrop, ( DWORD ) RT_nWeapon_Drop );
	g_HookGuardWeaponHookListener.VirtualTableHook ( info_equip, "CBaseCombatPlayer::WeaponEquip" );
	g_HookGuardWeaponHookListener.VirtualTableHook ( info_drop, "CBaseCombatPlayer::WeaponDrop");
}

#ifdef GNUC
void HOOKFN_INT WeaponHookListener::RT_nWeapon_Equip ( void * const basePlayer, void * const weapon )
#else
void HOOKFN_INT WeaponHookListener::RT_nWeapon_Equip ( void * const basePlayer, void * const, void * weapon )
#endif
{
	WeaponEquip_t gpOldFn;
	*( DWORD* )&( gpOldFn ) = g_HookGuardWeaponHookListener.RT_GetOldFunction ( basePlayer, g_ConfigManager.vfid_weaponequip );
	if (gpOldFn)
	{
		gpOldFn(basePlayer, weapon);

		PlayerHandler::iterator ph(g_NczPlayerManager.GetPlayerHandlerByBasePlayer(basePlayer));

		if (ph != SlotStatus::INVALID)
		{
			SourceSdk::edict_t const * const weapon_ent(SourceSdk::InterfacesProxy::Call_BaseEntityToEdict(weapon));

			if (Helpers::isValidEdict(weapon_ent))
			{
				//DebugMessage(Helpers::format("WeaponHookListener::RT_nWeapon_Equip : entity name %s", weapon_ent->GetClassName()));

				WeaponHookListenersListT::elem_t* it2(m_listeners.GetFirst());
				while (it2 != nullptr)
				{
					it2->m_value.listener->RT_WeaponEquipCallback(ph, weapon_ent);

					it2 = it2->m_next;
				}
			}
		}
	}
}

#ifdef GNUC
void HOOKFN_INT WeaponHookListener::RT_nWeapon_Drop ( void * const basePlayer, void * const weapon, SourceSdk::Vector const * const targetVec, SourceSdk::Vector const * const velocity )
#else
void HOOKFN_INT WeaponHookListener::RT_nWeapon_Drop ( void * const basePlayer, void * const, void * const weapon, SourceSdk::Vector const * const targetVec, SourceSdk::Vector const * const velocity )
#endif
{
	PlayerHandler::iterator ph ( g_NczPlayerManager.GetPlayerHandlerByBasePlayer ( basePlayer ) );

	if( ph != SlotStatus::INVALID && weapon != nullptr )
	{
		SourceSdk::edict_t* const weapon_ent ( SourceSdk::InterfacesProxy::Call_BaseEntityToEdict ( weapon ) );

		LoggerAssert ( Helpers::isValidEdict ( weapon_ent ) );

		WeaponHookListenersListT::elem_t* it2 ( m_listeners.GetFirst () );
		while( it2 != nullptr )
		{
			it2->m_value.listener->RT_WeaponDropCallback ( ph, weapon_ent );

			it2 = it2->m_next;
		}
	}

	WeaponDrop_t gpOldFn;
	*( DWORD* )&( gpOldFn ) = g_HookGuardWeaponHookListener.RT_GetOldFunction ( basePlayer, g_ConfigManager.vfid_weapondrop );
	gpOldFn ( basePlayer, weapon, targetVec, velocity );
}

void WeaponHookListener::RegisterWeaponHookListener ( WeaponHookListener const * const listener )
{
	m_listeners.Add ( listener );
}

void WeaponHookListener::RemoveWeaponHookListener ( WeaponHookListener const * const listener )
{
	m_listeners.Remove ( listener );
}
