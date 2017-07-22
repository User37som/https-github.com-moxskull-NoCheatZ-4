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

#ifndef SETTRANSMITHOOKLISTENER
#define SETTRANSMITHOOKLISTENER

#include "Interfaces/edict.h"

#include "Preprocessors.h"
#include "Hook.h"

/////////////////////////////////////////////////////////////////////////
// CBaseCombatCharacter::SetTransmit(CCheckTransmitInfo*, bool)
/////////////////////////////////////////////////////////////////////////

typedef void ( HOOKFN_EXT *SetTransmit_t )( void * const, SourceSdk::CCheckTransmitInfo *, bool );

class SetTransmitHookListener
{
	typedef HookListenersList<SetTransmitHookListener> TransmitListenersListT;

private:
	static TransmitListenersListT m_listeners;

public:
	SetTransmitHookListener ();
	virtual ~SetTransmitHookListener ();

	static void HookSetTransmit ( SourceSdk::edict_t const * const ent, bool isplayer );

protected:
	static void RegisterSetTransmitHookListener ( SetTransmitHookListener const * const listener, size_t const priority );
	static void RemoveSetTransmitHookListener ( SetTransmitHookListener const * const listener );

	virtual bool RT_SetTransmitCallback ( PlayerHandler::iterator sender, PlayerHandler::iterator receiver ) = 0;
	virtual bool RT_SetTransmitWeaponCallback ( SourceSdk::edict_t const *  const, PlayerHandler::iterator )
	{
		return false;
	};

private:
#ifdef GNUC
	static void HOOKFN_INT RT_nSetTransmit ( void * const This, SourceSdk::CCheckTransmitInfo *, bool );
	static void HOOKFN_INT RT_nSetTransmitWeapon(void * const This, SourceSdk::CCheckTransmitInfo *, bool);
#else
	static void HOOKFN_INT RT_nSetTransmit ( void * const This, void* const, SourceSdk::CCheckTransmitInfo *, bool );
	static void HOOKFN_INT RT_nSetTransmitWeapon(void * const This, void* const, SourceSdk::CCheckTransmitInfo *, bool);
#endif
};

class Fool
{

};

typedef Fool SetTransmitHookListenerWeapon;

extern HookGuard<SetTransmitHookListener> g_HookGuardSetTransmitHookListener;
extern HookGuard<SetTransmitHookListenerWeapon> g_HookGuardSetTransmitHookListenerWeapon;

#endif // SETTRANSMITHOOKLISTENER
