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

#include "Preprocessors.h"
#include "Hook.h"

/////////////////////////////////////////////////////////////////////////
// CCSPlayer::Weapon_Equip(CBaseCombatWeapon*)
// CBasePlayer::Weapon_Drop(CBaseCombatWeapon*, Vector const*, Vector const*)
/////////////////////////////////////////////////////////////////////////

class CBasePlayer;
class NczPlayer;

namespace SourceSdk
{
	struct Vector;
	class CBaseEntity;
}

typedef void ( HOOKFN_EXT *WeaponEquip_t )( void * const, void * const );
typedef void ( HOOKFN_EXT *WeaponDrop_t )( void * const, void * const, SourceSdk::Vector const * const, SourceSdk::Vector const * const );

class WeaponHookListener
{
	typedef HookListenersList<WeaponHookListener> WeaponHookListenersListT;

private:
	static WeaponHookListenersListT m_listeners;

public:
	WeaponHookListener ();
	virtual ~WeaponHookListener ();

	/*
		Hook and unhook functions.
		FIXME : May not works well with others plugins ...
	*/
	static void HookWeapon ( PlayerHandler::iterator ph );

protected:
	static void RegisterWeaponHookListener ( WeaponHookListener const * const listener );
	static void RemoveWeaponHookListener ( WeaponHookListener const * const listener );

	virtual void RT_WeaponEquipCallback ( PlayerHandler::iterator ph, SourceSdk::edict_t const * const weapon ) = 0;
	virtual void RT_WeaponDropCallback ( PlayerHandler::iterator ph, SourceSdk::edict_t const * const weapon ) = 0;

private:
#ifdef GNUC
	static void HOOKFN_INT RT_nWeapon_Equip ( void * const basePlayer, void * const weapon );
	static void HOOKFN_INT RT_nWeapon_Drop ( void * const basePlayer, void * const weapon, SourceSdk::Vector const * const targetVec, SourceSdk::Vector const * const velocity );
#else
	static void HOOKFN_INT RT_nWeapon_Equip ( void * const basePlayer, void * const, void * const weapon );
	static void HOOKFN_INT RT_nWeapon_Drop ( void * const basePlayer, void * const, void * const weapon, SourceSdk::Vector const * const targetVec, SourceSdk::Vector const * const velocity );
#endif
};

extern HookGuard<WeaponHookListener> g_HookGuardWeaponHookListener;

#endif
