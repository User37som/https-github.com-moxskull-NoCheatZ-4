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
#include "Console/convar.h"
#include "Interfaces/InterfacesProxy.h"
#include "Interfaces/iserverunknown.h"

#include "WeaponHookListener.h"
#include "Misc/Helpers.h"
#include "plugin.h"
#include "Systems/ConfigManager.h"

WeaponHookListenersListT WeaponHookListener::m_listeners;

WeaponHookListener::WeaponHookListener()
{
}

WeaponHookListener::~WeaponHookListener()
{
}

void WeaponHookListener::HookWeapon(NczPlayer* player)
{
	Assert(Helpers::isValidEdict(player->GetEdict()));
	void* unk = player->GetEdict()->m_pUnk;

	HookInfo info_equip(unk, ConfigManager::GetInstance()->GetVirtualFunctionId("weaponequip"), (DWORD)nWeapon_Equip);
	HookInfo info_drop(unk, ConfigManager::GetInstance()->GetVirtualFunctionId("weapondrop"), (DWORD)nWeapon_Drop);
	HookGuard::GetInstance()->VirtualTableHook(info_equip);
	HookGuard::GetInstance()->VirtualTableHook(info_drop);
}

/*void WeaponHookListener::UnhookWeapon()
{
	bool tricky_guess = false;

	HookList<>::elem_t* it = m_hooked_instances.GetFirst();

	while (it != nullptr)
	{
		if (!tricky_guess) VirtualTableHook(it->m_value->pInterface, ConfigManager::GetInstance()->GetVirtualFunctionId("weaponequip"), it->m_value->oldFn, (DWORD)nWeapon_Equip);
		else              VirtualTableHook(it->m_value->pInterface, ConfigManager::GetInstance()->GetVirtualFunctionId("weapondrop"), it->m_value->oldFn, (DWORD)nWeapon_Drop);

		tricky_guess = !tricky_guess;

		it = m_hooked_instances.Remove(it);
	}
}*/

#ifdef GNUC
void HOOKFN_INT WeaponHookListener::nWeapon_Equip(CBasePlayer* basePlayer, SourceSdk::CBaseEntity* weapon)
#else
void HOOKFN_INT WeaponHookListener::nWeapon_Equip(CBasePlayer* basePlayer, void*, SourceSdk::CBaseEntity* weapon)
#endif
{
	PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByBasePlayer(basePlayer);

	if(ph->status != INVALID)
	{
		SourceSdk::edict_t* const weapon_ent = SourceSdk::InterfacesProxy::Call_BaseEntityToEdict(weapon);

		Assert(Helpers::isValidEdict(weapon_ent));

		WeaponHookListenersListT::elem_t* it2 = m_listeners.GetFirst();
		while (it2 != nullptr)
		{
			it2->m_value.listener->WeaponEquipCallback(ph->playerClass, weapon_ent);

			it2 = it2->m_next;
		}
	}

	WeaponEquip_t gpOldFn;
	*(DWORD*)&(gpOldFn) = HookGuard::GetInstance()->GetOldFunction(basePlayer, ConfigManager::GetInstance()->GetVirtualFunctionId("weaponequip"));
	gpOldFn(basePlayer, weapon);
}

#ifdef GNUC
void HOOKFN_INT WeaponHookListener::nWeapon_Drop(CBasePlayer* basePlayer, SourceSdk::CBaseEntity* weapon, const SourceSdk::Vector* targetVec, const SourceSdk::Vector* velocity)
#else
void HOOKFN_INT WeaponHookListener::nWeapon_Drop(CBasePlayer* basePlayer, void*, SourceSdk::CBaseEntity* weapon, const SourceSdk::Vector* targetVec, const SourceSdk::Vector* velocity)
#endif
{
	PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByBasePlayer(basePlayer);

	if (ph->status != INVALID && weapon != nullptr)
	{
		SourceSdk::edict_t* const weapon_ent = SourceSdk::InterfacesProxy::Call_BaseEntityToEdict(weapon);

		Assert(Helpers::isValidEdict(weapon_ent));

		WeaponHookListenersListT::elem_t* it2 = m_listeners.GetFirst();
		while (it2 != nullptr)
		{
			it2->m_value.listener->WeaponDropCallback(ph->playerClass, weapon_ent);

			it2 = it2->m_next;
		}
	}

	WeaponDrop_t gpOldFn;
	*(DWORD*)&(gpOldFn) = HookGuard::GetInstance()->GetOldFunction(basePlayer, ConfigManager::GetInstance()->GetVirtualFunctionId("weapondrop"));
	gpOldFn(basePlayer, weapon, targetVec, velocity);
}

void WeaponHookListener::RegisterWeaponHookListener(WeaponHookListener* listener)
{
	m_listeners.Add(listener);
}

void WeaponHookListener::RemoveWeaponHookListener(WeaponHookListener* listener)
{
	m_listeners.Remove(listener);
}
