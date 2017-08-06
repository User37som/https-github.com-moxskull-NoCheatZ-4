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

#include <cstdlib> // rand
#include <ctime>

#include "PlayerRunCommandHookListener.h"

#include "Interfaces/InterfacesProxy.h"
#include "Interfaces/iserverunknown.h"
#include "Console/convar.h"

#include "plugin.h"
#include "Systems/ConfigManager.h"
#include "Systems/Logger.h"

/////////////////////////////////////////////////////////////////////////
// PlayerRunCommandHookListener
/////////////////////////////////////////////////////////////////////////

PlayerRunCommandHookListener::ListenersListT PlayerRunCommandHookListener::m_listeners;

HookGuard<PlayerRunCommandHookListener> g_HookGuardPlayerRunCommandHookListener;

PlayerRunCommandHookListener::PlayerRunCommandHookListener ()
{
	//for(int x = 1; x < MAX_PLAYERS; ++x) m_lastCUserCmd[x] = SourceSdk::CUserCmd();
	std::srand ( ( unsigned int ) ( std::time ( 0 ) ) );
}

PlayerRunCommandHookListener::~PlayerRunCommandHookListener ()
{
}

void PlayerRunCommandHookListener::HookPlayerRunCommand ( PlayerHandler::iterator ph )
{
	LoggerAssert ( Helpers::isValidEdict ( ph->GetEdict () ) );
	SourceSdk::IServerUnknown* unk ( ph->GetEdict ()->m_pUnk );

	HookInfo info ( unk, g_ConfigManager.vfid_playerruncommand, ( DWORD ) RT_nPlayerRunCommand );
	g_HookGuardPlayerRunCommandHookListener.VirtualTableHook ( info, "CBasePlayer::PlayerRunCommand" );
}

/*void PlayerRunCommandHookListener::UnhookPlayerRunCommand()
{
	if(pdwInterface && gpOldPlayerRunCommand)
	{
		VirtualTableHook(pdwInterface, g_ConfigManager.GetVirtualFunctionId("playerruncommand"), (DWORD)gpOldPlayerRunCommand, (DWORD)nPlayerRunCommand);
		pdwInterface = nullptr;
		gpOldPlayerRunCommand = nullptr;
	}
}*/

#ifdef GNUC
void PlayerRunCommandHookListener::RT_nPlayerRunCommand ( void * const This, void * const pCmd, IMoveHelper const * const pMoveHelper )
#else
void HOOKFN_INT PlayerRunCommandHookListener::RT_nPlayerRunCommand ( void* This, void*, void * const pCmd, IMoveHelper const * const pMoveHelper )
#endif
{
	PlayerHandler::iterator ph ( g_NczPlayerManager.GetPlayerHandlerByBasePlayer ( This ) );
	PlayerRunCommandRet ret ( PlayerRunCommandRet::CONTINUE );

	if (ph)
	{
		ph->SetEyes(reinterpret_cast<SourceSdk::CUserCmd const *>(pCmd)->viewangles);
	}

	if( ph > SlotStatus::PLAYER_CONNECTING )
	{
		double const curtime = Tier0::Plat_FloatTime();

		ListenersListT::elem_t* it ( m_listeners.GetFirst () );
		while( it != nullptr )
		{
			if( ph >= it->m_value.filter )
			{
				ret = it->m_value.listener->RT_PlayerRunCommandCallback ( ph, pCmd, curtime);

				//if( ret > PlayerRunCommandRet::CONTINUE ) break;
			}
			it = it->m_next;
		}
	}

	if( ret < PlayerRunCommandRet::BLOCK )
	{
		if( ret == PlayerRunCommandRet::INERT ) InertCmd ( pCmd );

		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			//static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->random_seed = ( int ) std::rand ();
		}
		else
		{
			static_cast< SourceSdk::CUserCmd* >( pCmd )->random_seed = ( int ) std::rand ();
		}

		ST_W_STATIC PlayerRunCommand_t gpOldFn;
		*( DWORD* )&( gpOldFn ) = g_HookGuardPlayerRunCommandHookListener.RT_GetOldFunction ( This, g_ConfigManager.vfid_playerruncommand );
		gpOldFn ( This, pCmd, pMoveHelper );
	}
	else
	{
		//DebugMessage ( "Blocked CUserCmd" );
	}
}

void PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( PlayerRunCommandHookListener const * const listener, size_t priority, SlotStatus filter )
{
	m_listeners.Add ( listener, priority, filter );
}

void PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( PlayerRunCommandHookListener const * const listener )
{
	m_listeners.Remove ( listener );
}

inline void InertCmd ( void* pcmd )
{
	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		SourceSdk::CUserCmd_csgo* z = static_cast< SourceSdk::CUserCmd_csgo* >( pcmd );
		VectorCopy ( SourceSdk::QAngle ( 0.0f, 0.0f, 0.0f ), z->viewangles );
		z->buttons = 0;
		//z->tick_count = z->tick_count - 50;
	}
	else
	{
		SourceSdk::CUserCmd* z = static_cast< SourceSdk::CUserCmd* >( pcmd );
		VectorCopy ( SourceSdk::QAngle ( 0.0f, 0.0f, 0.0f ), z->viewangles );
		z->buttons = 0;
		//z->tick_count = z->tick_count - 50;
	}
}
