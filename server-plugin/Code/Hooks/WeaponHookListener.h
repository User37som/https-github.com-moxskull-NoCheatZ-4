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

#ifndef WEAPONHOOKLISTENER
#define WEAPONHOOKLISTENER

#include "Hook.h"
#include "Players/NczPlayerManager.h"
#include "Preprocessors.h"

/////////////////////////////////////////////////////////////////////////
// CCSPlayer::Weapon_Equip(CBaseCombatWeapon*)
// CBasePlayer::Weapon_Drop(CBaseCombatWeapon*, Vector const*, Vector const*)
/////////////////////////////////////////////////////////////////////////

class CBasePlayer;
class CBaseEntity;

typedef void (HOOKFN_EXT *WeaponEquip_t)(CBasePlayer *, SourceSdk::CBaseEntity*);
typedef void (HOOKFN_EXT *WeaponDrop_t)(CBasePlayer *, SourceSdk::CBaseEntity*, const SourceSdk::Vector*, const SourceSdk::Vector*);

class WeaponHookListener;
typedef HookListenersList<WeaponHookListener> WeaponHookListenersListT;

class WeaponHookListener
{
public:
	WeaponHookListener();
	~WeaponHookListener();

	/*
		Hook and unhook functions.
		FIXME : May not works well with others plugins ...
	*/
	static void HookWeapon(NczPlayer* player);

protected:
	static void RegisterWeaponHookListener(WeaponHookListener* listener);
	static void RemoveWeaponHookListener(WeaponHookListener* listener);
	
	virtual void WeaponEquipCallback(NczPlayer* player, SourceSdk::edict_t* const weapon) = 0;
	virtual void WeaponDropCallback(NczPlayer* player, SourceSdk::edict_t* const weapon) = 0;

private:
#ifdef GNUC
	static void HOOKFN_INT nWeapon_Equip(CBasePlayer* basePlayer, SourceSdk::CBaseEntity* weapon);
	static void HOOKFN_INT nWeapon_Drop(CBasePlayer* basePlayer, SourceSdk::CBaseEntity* weapon, const SourceSdk::Vector* targetVec, const SourceSdk::Vector* velocity);
#else
	static void HOOKFN_INT nWeapon_Equip(CBasePlayer* basePlayer, void*, SourceSdk::CBaseEntity* weapon);
	static void HOOKFN_INT nWeapon_Drop(CBasePlayer* basePlayer,  void*, SourceSdk::CBaseEntity* weapon, const SourceSdk::Vector* targetVec, const SourceSdk::Vector* velocity);
#endif
	
	static WeaponHookListenersListT m_listeners;
};

#endif
