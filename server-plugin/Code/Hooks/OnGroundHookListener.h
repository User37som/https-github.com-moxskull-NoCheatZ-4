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

#include "Preprocessors.h"
#include "Hook.h"

/////////////////////////////////////////////////////////////////////////
// CBasePlayer::NetworkStateChanged_m_hGroundEntity(void*)
/////////////////////////////////////////////////////////////////////////

class CBasePlayer;
class NczPlayer;

// The function declaration we will call
typedef void (HOOKFN_EXT *GroundEntity_t)(void * const, int const * const);

class OnGroundHookListener;

/*
	The base class used to listen for the ground state of human players
*/
class OnGroundHookListener
{
	typedef HookListenersList<OnGroundHookListener> OnGroundListenersListT;

private:
	static OnGroundListenersListT m_listeners;

public:
	OnGroundHookListener();
	virtual ~OnGroundHookListener();

protected:
	virtual void RT_m_hGroundEntityStateChangedCallback(PlayerHandler::iterator ph, bool const new_isOnGround) = 0;

public:

	/*
		Hook and unhook functions.
		FIXME : May not works well with others plugins ...
	*/

	/*
		Hook function.
		Needs a valid player in game, could be a bot.

		This is a single instance hook. The plugin calls this when there is at least one player in game.
	*/
	static void HookOnGround(PlayerHandler::iterator ph);

protected:
	static void RegisterOnGroundHookListener(OnGroundHookListener const * const listener);
	static void RemoveOnGroundHookListener(OnGroundHookListener const * const listener);

private:
#ifdef GNUC
	static void HOOKFN_INT RT_nNetworkStateChanged_m_hGroundEntity(void * const basePlayer, int const * const new_m_hGroundEntity);
#else
	static void HOOKFN_INT RT_nNetworkStateChanged_m_hGroundEntity(void * const basePlayer, void* const, int const * const new_m_hGroundEntity);
#endif
	
};

extern HookGuard<OnGroundHookListener> g_HookGuardOnGroundHookListener;

#endif
