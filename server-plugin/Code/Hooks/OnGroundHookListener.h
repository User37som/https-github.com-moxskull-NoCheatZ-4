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

#ifndef ONGROUNDHOOKLISTENER
#define ONGROUNDHOOKLISTENER

#include "Hook.h"
#include "Players/NczPlayerManager.h"
#include "Preprocessors.h"

/////////////////////////////////////////////////////////////////////////
// CBasePlayer::NetworkStateChanged_m_hGroundEntity(void*)
/////////////////////////////////////////////////////////////////////////

class CBasePlayer;

// The function declaration we will call
typedef void (HOOKFN_EXT *GroundEntity_t)(CBasePlayer *, int*);

class OnGroundHookListener;

typedef HookListenersList<OnGroundHookListener> OnGroundListenersListT;

/*
	The base class used to listen for the ground state of human players
*/
class OnGroundHookListener
{
public:
	OnGroundHookListener();
	virtual ~OnGroundHookListener();

	/*
		Hook and unhook functions.
		FIXME : May not works well with others plugins ...
	*/

	/*
		Hook function.
		Needs a valid player in game, could be a bot.

		This is a single instance hook. The plugin calls this when there is at least one player in game.
	*/
	static void HookOnGround(NczPlayer* player);

protected:
	static void RegisterOnGroundHookListener(OnGroundHookListener* listener);
	static void RemoveOnGroundHookListener(OnGroundHookListener* listener);
	
	virtual void m_hGroundEntityStateChangedCallback(NczPlayer* player, bool new_isOnGround) = 0;

private:
#ifdef GNUC
	static void HOOKFN_INT nNetworkStateChanged_m_hGroundEntity(CBasePlayer* basePlayer, int * new_m_hGroundEntity);
#else
	static void HOOKFN_INT nNetworkStateChanged_m_hGroundEntity(CBasePlayer* basePlayer, void*, int * new_m_hGroundEntity);
#endif
	
	static OnGroundListenersListT m_listeners;
};

#endif
