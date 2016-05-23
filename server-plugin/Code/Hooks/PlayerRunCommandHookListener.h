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

#include "Preprocessors.h"
#include "Hook.h"

#include "SdkPreprocessors.h"
#include "Interfaces/usercmd.h"

inline void InertCmd(void* pcmd);

/////////////////////////////////////////////////////////////////////////
// CCSPlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)
/////////////////////////////////////////////////////////////////////////

class CCSPlayer;
class IMoveHelper;
class NczPlayer;

typedef void (HOOKFN_EXT *PlayerRunCommand_t)(void*, void*, IMoveHelper*);

enum PlayerRunCommandRet
{
	CONTINUE = 0,
	INERT,
	BLOCK
};

class PlayerRunCommandHookListener;

typedef HookListenersList<PlayerRunCommandHookListener> ListenersListT;

class ALIGN8 PlayerRunCommandHookListener
{
public:
	PlayerRunCommandHookListener();
	~PlayerRunCommandHookListener();

	static void HookPlayerRunCommand(NczPlayer* player);

	static void UnhookPlayerRunCommand();
	static void* GetLastUserCmd(NczPlayer* player);

protected:
	static void RegisterPlayerRunCommandHookListener(PlayerRunCommandHookListener* listener, size_t priority = std::numeric_limits<size_t>::max(), SlotStatus filter = PLAYER_IN_TESTS);
	static void RemovePlayerRunCommandHookListener(PlayerRunCommandHookListener* listener);

	virtual PlayerRunCommandRet PlayerRunCommandCallback(NczPlayer* player, void* cmd, void* old_cmd) = 0;

private:
#ifdef GNUC
	static void HOOKFN_INT nPlayerRunCommand(void* This, void* pCmd, IMoveHelper* pMoveHelper);
#else
	static void HOOKFN_INT nPlayerRunCommand(void* This, void*, void* pCmd, IMoveHelper* pMoveHelper);
#endif
	static ListenersListT m_listeners;
	static SourceSdk::CUserCmd_csgo m_lastCUserCmd[MAX_PLAYERS];
	static PlayerRunCommand_t gpOldPlayerRunCommand;
	static DWORD* pdwInterface;
} ALIGN8_POST;

#endif
