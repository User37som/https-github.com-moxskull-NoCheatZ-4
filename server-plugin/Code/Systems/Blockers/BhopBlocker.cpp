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

#include "Interfaces/InterfacesProxy.h"

#include "Systems/Logger.h"

BhopBlocker::BhopBlocker () :
	BaseDynamicSystem ( "BhopBlocker" ),
	playerdatahandler_class (),
	PlayerRunCommandHookListener (),
	singleton_class ()
{
	METRICS_ADD_TIMER ( "BhopBlocker::PlayerRunCommandCallback", 2.0 );
}

BhopBlocker::~BhopBlocker ()
{
	Unload ();
}

void BhopBlocker::Init ()
{
	InitDataStruct ();
}

void BhopBlocker::Load ()
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	RegisterPlayerRunCommandHookListener ( this, SystemPriority::UserCmdHookListener::BhopBlocker, SlotStatus::PLAYER_IN_TESTS );
}

void BhopBlocker::Unload ()
{
	RemovePlayerRunCommandHookListener ( this );
}

bool BhopBlocker::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::const_iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

PlayerRunCommandRet BhopBlocker::RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void* pCmd, void* old_cmd )
{
	METRICS_ENTER_SECTION ( "BhopBlocker::PlayerRunCommandCallback" );


	/*
		Prevents jumping right after the end of another jump cmd.
		If a jump has been blocked, resend it after the delay.
	*/

	static float const blocking_delay_seconds ( 0.15f );

	float const tick_interval ( SourceSdk::InterfacesProxy::Call_GetTickInterval () );
	__assume ( tick_interval > 0.0f && tick_interval < 1.0f );

	int const blocking_delay_ticks ( std::floorf ( blocking_delay_seconds / tick_interval ) );

	int const gtick ( Helpers::GetGameTickCount () );

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		SourceSdk::CUserCmd_csgo const * const k_oldcmd ( ( SourceSdk::CUserCmd_csgo const * const )old_cmd );
		SourceSdk::CUserCmd_csgo * const k_newcmd ( ( SourceSdk::CUserCmd_csgo * const )pCmd );

		int const jmp_changed ( ( k_oldcmd->buttons ^ k_newcmd->buttons ) & IN_JUMP );

		JmpInfo * pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

		if( jmp_changed )
		{
			int const new_jmp ( k_newcmd->buttons & IN_JUMP );

			if( new_jmp != 0 )
			{
				// button down				

				if( gtick - pdata->jmp_up_tick < blocking_delay_ticks )
				{
					// block it

					k_newcmd->buttons &= ~IN_JUMP;
					pdata->has_been_blocked = true;
				}
				else
				{
					pdata->has_been_blocked = false;
				}
			}
			else
			{
				// button up

				pdata->jmp_up_tick = gtick;
			}
		}
		else if( pdata->has_been_blocked == true )
		{
			if( gtick - pdata->jmp_up_tick >= blocking_delay_ticks )
			{
				// resend

				k_newcmd->buttons |= IN_JUMP;
				pdata->has_been_blocked = false;
			}
		}
	}
	else
	{
		SourceSdk::CUserCmd const * const k_oldcmd ( ( SourceSdk::CUserCmd const * const )old_cmd );
		SourceSdk::CUserCmd * const k_newcmd ( ( SourceSdk::CUserCmd * const )pCmd );

		int const jmp_changed ( ( k_oldcmd->buttons ^ k_newcmd->buttons ) & IN_JUMP );

		JmpInfo * pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

		if( jmp_changed )
		{
			int const new_jmp ( k_newcmd->buttons & IN_JUMP );

			if( new_jmp != 0 )
			{
				// button down				

				if( gtick - pdata->jmp_up_tick < blocking_delay_ticks )
				{
					// block it

					k_newcmd->buttons &= ~IN_JUMP;
					pdata->has_been_blocked = true;
				}
				else
				{
					pdata->has_been_blocked = false;
				}
			}
			else
			{
				// button up

				pdata->jmp_up_tick = gtick;
			}
		}
		else if( pdata->has_been_blocked == true )
		{
			if( gtick - pdata->jmp_up_tick >= blocking_delay_ticks )
			{
				// resend

				k_newcmd->buttons |= IN_JUMP;
				pdata->has_been_blocked = false;
			}
		}
	}

	METRICS_LEAVE_SECTION ( "BhopBlocker::PlayerRunCommandCallback" );
	return PlayerRunCommandRet::CONTINUE;
}
