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
#include "Interfaces/iserverunknown.h"

#include "Preprocessors.h"
#include "Hook.h"
#include "Players/NczPlayer.h"

/////////////////////////////////////////////////////////////////////////
// CBaseCombatCharacter::SetTransmit(CCheckTransmitInfo*, bool)
/////////////////////////////////////////////////////////////////////////

typedef void (HOOKFN_EXT *SetTransmit_t)(void*, SourceSdk::CCheckTransmitInfo*, bool);

class SetTransmitHookListener;

typedef HookListenersList<SetTransmitHookListener> TransmitListenersListT;

class SetTransmitHookListener
{
public:
	SetTransmitHookListener();
	~SetTransmitHookListener();

	static void HookSetTransmit(SourceSdk::edict_t* ent);

protected:
	static void RegisterSetTransmitHookListener(SetTransmitHookListener* listener, size_t priority);
	static void RemoveSetTransmitHookListener(SetTransmitHookListener* listener);

	virtual bool SetTransmitCallback(SourceSdk::edict_t* const, SourceSdk::edict_t* const) = 0;
	virtual bool SetTransmitWeaponCallback(SourceSdk::edict_t* const, SourceSdk::edict_t* const) {return false;};

private:
#ifdef GNUC
	static void HOOKFN_INT nSetTransmit(SourceSdk::CBaseEntity* This, SourceSdk::CCheckTransmitInfo*, bool);
#else
	static void HOOKFN_INT nSetTransmit(SourceSdk::CBaseEntity* This, void*, SourceSdk::CCheckTransmitInfo*, bool);
#endif
	static TransmitListenersListT m_listeners;
};

#endif // SETTRANSMITHOOKLISTENER
