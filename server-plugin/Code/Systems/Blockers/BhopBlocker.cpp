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

#include "BhopBlocker.h"

#include <cmath>

#include "Interfaces/InterfacesProxy.h"

#include "Systems/Logger.h"

BhopBlocker::BhopBlocker () :
	BaseBlockerSystem( "BhopBlocker" ),
	playerdatahandler_class (),
	PlayerRunCommandHookListener (),
	Singleton (),
	convar_sv_enablebunnyhopping(nullptr),
	convar_sv_autobunnyhopping(nullptr)
{
}

BhopBlocker::~BhopBlocker ()
{
	Unload ();
}

void BhopBlocker::Init ()
{
	InitDataStruct ();

	convar_sv_enablebunnyhopping = SourceSdk::InterfacesProxy::ICvar_FindVar ( "sv_enablebunnyhopping" );

	if( convar_sv_enablebunnyhopping == nullptr )
	{
		g_Logger.Msg<MSG_WARNING> ( "JumpTester::Init : Unable to locate ConVar sv_enablebunnyhopping" );
	}

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		convar_sv_autobunnyhopping = SourceSdk::InterfacesProxy::ICvar_FindVar ( "sv_autobunnyhopping" );

		if( convar_sv_enablebunnyhopping == nullptr )
		{
			g_Logger.Msg<MSG_WARNING> ( "JumpTester::Init : Unable to locate ConVar sv_enablebunnyhopping" );
		}
	}
}

void BhopBlocker::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	RegisterPlayerRunCommandHookListener ( this, SystemPriority::BhopBlocker, SlotStatus::PLAYER_IN_TESTS );
	RegisterOnGroundHookListener ( this );
}

void BhopBlocker::Unload ()
{
	RemovePlayerRunCommandHookListener ( this );
	RemoveOnGroundHookListener ( this );
}

bool BhopBlocker::GotJob () const
{
	if( convar_sv_enablebunnyhopping != nullptr )
	{
		if( SourceSdk::InterfacesProxy::ConVar_GetBool ( convar_sv_enablebunnyhopping ) )
		{
			return false;
		}
	}
	if( convar_sv_autobunnyhopping != nullptr )
	{
		if( SourceSdk::InterfacesProxy::ConVar_GetBool ( convar_sv_autobunnyhopping ) )
		{
			return false;
		}
	}

	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

PlayerRunCommandRet BhopBlocker::RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void* pCmd, double const & curtime)
{
	if( convar_sv_enablebunnyhopping != nullptr )
	{
		if( SourceSdk::InterfacesProxy::ConVar_GetBool ( convar_sv_enablebunnyhopping ) )
		{
			SetActive ( false );
			return PlayerRunCommandRet::CONTINUE;
		}
	}
	if( convar_sv_autobunnyhopping != nullptr )
	{
		if( SourceSdk::InterfacesProxy::ConVar_GetBool ( convar_sv_autobunnyhopping ) )
		{
			SetActive ( false );
			return PlayerRunCommandRet::CONTINUE;
		}
	}

	/*
		Prevents jumping right after the end of another jump cmd.
		If a jump has been blocked, resend it after the delay.
	*/

	static float const blocking_delay_seconds ( 0.15f );

	float const tick_interval ( SourceSdk::InterfacesProxy::Call_GetTickInterval () );
	__assume ( tick_interval > 0.0f && tick_interval < 1.0f );

	int const blocking_delay_ticks ( (int)floorf ( blocking_delay_seconds / tick_interval ) );

	int const gtick ( Helpers::GetGameTickCount () );

	int * buttons;
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		buttons = &((((SourceSdk::CUserCmd_csgo * const)pCmd))->buttons);
	}
	else
	{
		buttons = &((((SourceSdk::CUserCmd * const)pCmd))->buttons);
	}

	JmpInfo * pdata(GetPlayerDataStructByIndex(ph.GetIndex()));

	int const jmp_changed ( ( *buttons ^ pdata->old_buttons ) & IN_JUMP );

	if( jmp_changed )
	{
		int const new_jmp ( *buttons & IN_JUMP );

		if( new_jmp != 0 )
		{
			// button down				

			if( gtick - pdata->on_ground_tick < blocking_delay_ticks )
			{
				// block it

				*buttons &= ~IN_JUMP;
				pdata->has_been_blocked = true;
			}
			else
			{
				pdata->has_been_blocked = false;
			}
		}
		else
		{
			pdata->has_been_blocked = false;
		}
	}
	else if( pdata->has_been_blocked == true )
	{
		if( gtick - pdata->on_ground_tick >= blocking_delay_ticks )
		{
			// resend

			*buttons |= IN_JUMP;
			pdata->has_been_blocked = false;
		}
	}

	pdata->old_buttons = *buttons;

	return PlayerRunCommandRet::CONTINUE;
}

void BhopBlocker::RT_m_hGroundEntityStateChangedCallback ( PlayerHandler::iterator ph, bool const new_isOnGround )
{
	if( new_isOnGround )
	{
		JmpInfo * pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );
		pdata->on_ground_tick = Helpers::GetGameTickCount ();
	}
}

BhopBlocker g_BhopBlocker;
