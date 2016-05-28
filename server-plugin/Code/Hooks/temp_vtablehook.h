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

#ifndef TEMP_VTABLEHOOK_H
#define TEMP_VTABLEHOOK_H

#include <functional>

#include "SdkPreprocessors.h"

#include "Hook.h"
#include "Systems/ConfigManager.h"

typedef enum
{
	ConCommandHook,
	OnGroundHook,
	PlayerRunCommandHook,
	SetTransmitHook,
	WeaponDropHook,
	WeaponEquipHook
} HookId_t ;

class CBasePlayer;
class IMoveHelper;

template <HookId_t hook_id>
struct HookSpecialized
{
	typedef void (HOOKFN_EXT *HookSpecialized<ConCommandHook>::ext_fn_t)(void* cmd, SourceSdk::CCommand const & args);
	typedef void (HOOKFN_EXT *HookSpecialized<OnGroundHook>::ext_fn_t)(CBasePlayer *, int*);
	typedef void (HOOKFN_EXT *HookSpecialized<PlayerRunCommandHook>::ext_fn_t)(void*, void*, IMoveHelper*);
	typedef void (HOOKFN_EXT *HookSpecialized<SetTransmitHook>::ext_fn_t)(void*, SourceSdk::CCheckTransmitInfo*, bool);
	typedef void (HOOKFN_EXT *HookSpecialized<WeaponEquipHook>::ext_fn_t)(CBasePlayer *, SourceSdk::CBaseEntity*);
	typedef void (HOOKFN_EXT *HookSpecialized<WeaponDropHook>::ext_fn_t)(CBasePlayer *, SourceSdk::CBaseEntity*, const SourceSdk::Vector*, const SourceSdk::Vector*);

	static constexpr char const * const vfn_config_name;

	static constexpr bool cache_fn;
};

template<>
char const * const HookSpecialized<ConCommandHook>::vfn_config_name = "dispatch";

template<>
char const * const HookSpecialized<OnGroundHook>::vfn_config_name = "mhgroundentity";

template<>
char const * const HookSpecialized<PlayerRunCommandHook>::vfn_config_name = "playerruncommand";

template<>
char const * const HookSpecialized<SetTransmitHook>::vfn_config_name = "settransmit";

template<>
char const * const HookSpecialized<WeaponDropHook>::vfn_config_name = "weapondrop";

template<>
char const * const HookSpecialized<WeaponEquipHook>::vfn_config_name = "weaponequip";

template <HookId_t hook_id, class C = HookSpecialized<hook_id>>
class HookListener
{
private:
	typedef HookListener<hook_id> H;

	static C::ext_fn_t m_cached_fn;

	static HookListenersList<H> m_listeners;

protected:
	HookListener()
	{
		
	}

	virtual ~HookListener()
	{

	}

	static void RegisterListener(H * listener, size_t priority = 0, SlotFilter filter = SlotFilter(PLAYER_IN_TESTS, PLAYER_IN_TESTS, STATUS_EQUAL_OR_BETTER))
	{
		if (m_listeners.FindByListener(listener) == nullptr)
		{
			m_listeners.Add(listener, priority, filter.GetTargetSlotStatus(), filter.GetFilterBehavior());
		}
	}

	static void RemoveListener(H * listener)
	{
		m_listeners.Remove(listener);
	}

public:

	inline static void ReCacheOriginalFunction(void* instance)
	{
		*(DWORD*)&(m_cached_fn) = HookGuard::GetInstance()->GetOldFunction(instance, ConfigManager::GetInstance()->GetVirtualFunctionId(C::vfn_config_name));
	}

	static void Hook(void* instance)
	{

	}
};

#endif
