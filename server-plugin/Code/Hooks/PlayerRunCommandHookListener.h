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

#ifndef PLAYERRUNCOMMANDHOOKLISTENER
#define PLAYERRUNCOMMANDHOOKLISTENER

#include "Interfaces/usercmd.h"

#include "Preprocessors.h"
#include "Hook.h"

inline void InertCmd ( void * const pcmd );

/////////////////////////////////////////////////////////////////////////
// CCSPlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)
/////////////////////////////////////////////////////////////////////////

class CCSPlayer;
class IMoveHelper;
class NczPlayer;

typedef void ( HOOKFN_EXT *PlayerRunCommand_t )( void* const, void * const, IMoveHelper const * const );

enum class PlayerRunCommandRet : short
{
	CONTINUE = 0,
	INERT,
	BLOCK
};

class PlayerRunCommandHookListener
{
	typedef HookListenersList<PlayerRunCommandHookListener> ListenersListT;

private:
	static ListenersListT m_listeners;

public:
	PlayerRunCommandHookListener ();
	virtual ~PlayerRunCommandHookListener ();

protected:
	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void * const cmd, double const & curtime ) = 0;

public:
	static void HookPlayerRunCommand ( PlayerHandler::iterator ph );

protected:
	static void RegisterPlayerRunCommandHookListener ( PlayerRunCommandHookListener const * const listener, size_t const priority = std::numeric_limits<size_t>::max (), SlotStatus filter = SlotStatus::PLAYER_IN_TESTS );
	static void RemovePlayerRunCommandHookListener ( PlayerRunCommandHookListener const * const listener );

private:
#ifdef GNUC
	static void HOOKFN_INT RT_nPlayerRunCommand ( void * const This, void * const pCmd, IMoveHelper const * const pMoveHelper );
#else
	static void HOOKFN_INT RT_nPlayerRunCommand ( void * const This, void * const, void * const pCmd, IMoveHelper const * const pMoveHelper );
#endif
};

extern HookGuard<PlayerRunCommandHookListener> g_HookGuardPlayerRunCommandHookListener;

#endif
