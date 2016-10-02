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

#include <stdio.h>

#include "Preprocessors.h"

#include "AutoAttackTester.h"
#include "Systems/Logger.h"

/*
Test each player to see if they use any script to help them fire more bullets (= RapidFire)

Some old mouses can also make multiple clicks because of an electronic issue, not because the player itself use a script.
We have to make the difference by using statistics.

This particular tester will do simples tests about the fire rate of a player, nothing more.
So it is more sensible about electronic issues of the mouse.

In a way to perform better, this tester will analyse :
- The count of clicks per seconds
- The time between last click-up and last click-down in seconds
- The average time between clicks-up and clicks-down in seconds

Detections will trigger only if :
- The count of detected clicks is almost constant by 5% ( Higher = Bad mouse ? Lesser = Cheat ? )
- The time between last click-up and last click-down is too short ( Cheat or bad mouse or using mwheel like a troll ? )
- The number of consecutive detections is high or not ( Very bad mouse or very bad cheat ? )
*/

AutoAttackTester::AutoAttackTester ( void ) :
	BaseDynamicSystem ( "AutoAttackTester" ),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	singleton_class ()
{}

AutoAttackTester::~AutoAttackTester ( void )
{
	Unload ();
}

void AutoAttackTester::Init ()
{
	InitDataStruct ();
}

void AutoAttackTester::Load ()
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, 4 );
}

void AutoAttackTester::Unload ()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( this );
}

bool AutoAttackTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::const_iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

PlayerRunCommandRet AutoAttackTester::RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void* pCmd, void* lastcmd )
{
	PlayerRunCommandRet const constexpr drop_cmd = PlayerRunCommandRet::CONTINUE;

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		int const last_buttons ( static_cast< SourceSdk::CUserCmd_csgo* >( lastcmd )->buttons );
		int const cur_buttons ( static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->buttons );

		int const attack1_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK ) );
		int const attack2_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK2 ) );

		int const gtick ( Helpers::GetGameTickCount () );

		if( attack1_button_changed )
		{
			if( (cur_buttons & IN_ATTACK ) != 0 )
			{
				OnAttack1Down ( ph, gtick );
			}
			else
			{
				OnAttack1Up ( ph, gtick );
			}
		}

		if( attack2_button_changed )
		{
			if( (cur_buttons & IN_ATTACK2 ) != 0 )
			{
				OnAttack2Down ( ph, gtick );
			}
			else
			{
				OnAttack2Up ( ph, gtick );
			}
		}
	}
	else
	{
		int const last_buttons ( static_cast< SourceSdk::CUserCmd_csgo* >( lastcmd )->buttons );
		int const cur_buttons ( static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->buttons );

		int const attack1_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK ) );
		int const attack2_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK2 ) );

		int const gtick ( Helpers::GetGameTickCount () );

		if( attack1_button_changed )
		{
			if( (cur_buttons & IN_ATTACK) != 0 )
			{
				OnAttack1Down ( ph, gtick );
			}
			else
			{
				OnAttack1Up ( ph, gtick );
			}
		}

		if( attack2_button_changed )
		{
			if( (cur_buttons & IN_ATTACK2) != 0 )
			{
				OnAttack2Down ( ph, gtick );
			}
			else
			{
				OnAttack2Up ( ph, gtick );
			}
		}
	}

	return drop_cmd;
}

void AutoAttackTester::OnAttack1Up ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack1_up_tick = game_tick;
	pdata->attack1_sustain_stats.Store ( pdata->attack1_down_tick - game_tick, pdata->attack1_down_tick );
}

void AutoAttackTester::OnAttack1Down ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack1_down_tick = game_tick;
}

void AutoAttackTester::OnAttack2Up ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack2_up_tick = game_tick;
	pdata->attack2_sustain_stats.Store ( pdata->attack2_down_tick - game_tick, pdata->attack2_down_tick );
}

void AutoAttackTester::OnAttack2Down ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack2_down_tick = game_tick;
}
