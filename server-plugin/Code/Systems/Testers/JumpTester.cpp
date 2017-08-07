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

#include "JumpTester.h"

#include "Preprocessors.h"
#include "Systems/Logger.h"

/*
	Test each player to see if they use any script to help BunnyHop.

	Some players jumps just-in-time without using any script.
	We have to make the difference by using statistics.
*/

JumpTester::JumpTester () :
	BaseTesterSystem ( "JumpTester", "Enable - Disable - Verbose - SetAction - DetectScripts" ),
	OnGroundHookListener (),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	Singleton (),
	convar_sv_enablebunnyhopping ( nullptr ),
	convar_sv_autobunnyhopping ( nullptr ),
	detect_scripts(false)
{}

JumpTester::~JumpTester ()
{
	Unload ();
}

void JumpTester::Init ()
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

bool JumpTester::sys_cmd_fn(const SourceSdk::CCommand & args)
{
	if (!BaseTesterSystem::sys_cmd_fn(args))
	{
		if (args.ArgC() >= 4)
		{
			if (stricmp(args.Arg(2), "detectscripts") == 0)
			{
				if (Helpers::IsArgTrue(args.Arg(3)))
				{
					detect_scripts = true;
					g_Logger.Msg<MSG_CMD_REPLY>("DetectScripts is Yes");
					return true;
				}
				else if (Helpers::IsArgFalse(args.Arg(3)))
				{
					detect_scripts = false;
					g_Logger.Msg<MSG_CMD_REPLY>("DetectScripts is No");
					return true;
				}
				else
				{
					g_Logger.Msg<MSG_CMD_REPLY>("DetectScripts Usage : Yes / No");
					return false;
				}
			}
		}
	}
	else
	{
		return true;
	}
	return false;
}

void JumpTester::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	OnGroundHookListener::RegisterOnGroundHookListener ( this );
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, SystemPriority::JumpTester );
}

void JumpTester::Unload ()
{
	OnGroundHookListener::RemoveOnGroundHookListener ( this );
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( this );
}

bool JumpTester::GotJob () const
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
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void JumpTester::RT_m_hGroundEntityStateChangedCallback ( PlayerHandler::iterator ph, bool new_isOnGround )
{
	if( new_isOnGround )
	{
		OnPlayerTouchGround ( ph, Helpers::GetGameTickCount () );
	}
	else
	{
		OnPlayerLeaveGround ( ph, Helpers::GetGameTickCount () );
	}
}

PlayerRunCommandRet JumpTester::RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void* pCmd, double const & curtime)
{
	PlayerRunCommandRet const constexpr drop_cmd ( PlayerRunCommandRet::CONTINUE );

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

	JumpInfoT * const playerData(GetPlayerDataStructByIndex(ph.GetIndex()));
	bool cur_jump_button_state;

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		cur_jump_button_state = ( static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->buttons & IN_JUMP ) != 0;
	}
	else
	{
		cur_jump_button_state = ( static_cast< SourceSdk::CUserCmd* >( pCmd )->buttons & IN_JUMP ) != 0;
	}

	bool const jump_button_changed (playerData->prev_jump ^ cur_jump_button_state );

	if( !jump_button_changed )
	{
		return drop_cmd;
	}
	else
	{
		if( cur_jump_button_state )
		{
			OnPlayerJumpButtonDown ( ph, Helpers::GetGameTickCount () );
		}
		else
		{
			OnPlayerJumpButtonUp ( ph, Helpers::GetGameTickCount () );
		}
	}

	playerData->prev_jump = cur_jump_button_state;

	return drop_cmd;
}

void JumpTester::OnPlayerTouchGround ( PlayerHandler::iterator ph, int game_tick )
{
	JumpInfoT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	playerData->onGroundHolder.onGround_Tick = game_tick;
	playerData->isOnGround = true;
	SystemVerbose1 ( Helpers::format ( "Player %s touched the ground.", ph->GetName () ) );

	// Detect bunny hop scripts

	// Compute the average number of time a second the player is pushing the jump button, detect if it's too high
	float const fly_time = ( game_tick - playerData->onGroundHolder.notOnGround_Tick ) * SourceSdk::InterfacesProxy::Call_GetTickInterval ();
	if( fly_time >= 0.25f ) // this is to prevent collision bugs to make fake detections, and also prevents divide by zero crash below.
	{
		float const avg_jmp_per_second = ( float ) ( playerData->jumpCmdHolder.outsideJumpCmdCount ) / fly_time;

		if( avg_jmp_per_second > 10.0f && playerData->total_bhopCount > 1 )
		{
			if (detect_scripts)
			{
				ProcessDetectionAndTakeAction<Detection_BunnyHopScript::data_type>(Detection_BunnyHopScript(), playerData, ph, this);
			}
		}
	}

	if( playerData->jumpCmdHolder.outsideJumpCmdCount == 0 && playerData->perfectBhopsCount > 5 && playerData->perfectBhopsPercent >= std::max (0, ( 100 - std::min ( 95, playerData->perfectBhopsCount * 2 ) ) ) )
	{
		ProcessDetectionAndTakeAction<Detection_BunnyHopProgram::data_type>(Detection_BunnyHopProgram(), playerData, ph, this);
	}

	playerData->jumpCmdHolder.outsideJumpCmdCount = 0;
}

void JumpTester::OnPlayerLeaveGround ( PlayerHandler::iterator ph, int game_tick )
{
	JumpInfoT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	playerData->onGroundHolder.notOnGround_Tick = Helpers::GetGameTickCount ();
	++playerData->onGroundHolder.jumpCount;
	playerData->isOnGround = false;
	SystemVerbose1 ( Helpers::format ( "Player %s leaved the ground.", ph->GetName () ) );
}

void JumpTester::OnPlayerJumpButtonDown ( PlayerHandler::iterator ph, int game_tick )
{
	JumpInfoT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	playerData->jumpCmdHolder.JumpDown_Tick = game_tick;
	playerData->jumpCmdHolder.lastJumpCmdState = true;

	int const cmd_diff ( game_tick - playerData->jumpCmdHolder.JumpUp_Tick );
	int const wd_diff ( game_tick - playerData->onGroundHolder.onGround_Tick );

	if( cmd_diff > 1 && cmd_diff <= 3 )
	{
		if (detect_scripts)
		{
			ProcessDetectionAndTakeAction<Detection_BunnyHopScript::data_type>(Detection_BunnyHopScript(), playerData, ph, this);
		}
	}

	if( playerData->isOnGround )
	{
		if( wd_diff >= 0 && wd_diff < 10 )
		{
			++playerData->total_bhopCount;
			SystemVerbose1 ( Helpers::format ( "Player %s : total_bhopCount = %d\n", ph->GetName (), playerData->total_bhopCount ) );

			if( wd_diff <= 1 )
			{
				++playerData->perfectBhopsCount;
				__assume ( playerData->perfectBhopsCount <= playerData->total_bhopCount );
				playerData->perfectBhopsPercent = ( (float)(playerData->perfectBhopsCount) / (float)(playerData->total_bhopCount) ) * 100.0f;
				SystemVerbose1 ( Helpers::format ( "Player %s : perfectBhopsCount = %d\n", ph->GetName (), playerData->perfectBhopsCount ) );
			}
			else if( wd_diff < 3 )
			{
				++playerData->goodBhopsCount;
				__assume (playerData->perfectBhopsCount <= playerData->total_bhopCount);
				playerData->perfectBhopsPercent = ((float)(playerData->perfectBhopsCount) / (float)(playerData->total_bhopCount)) * 100.0f;
				SystemVerbose1 ( Helpers::format ( "Player %s : goodBhopsCount = %d\n", ph->GetName (), playerData->goodBhopsCount ) );
			}
		}
	}
	else
	{
		++playerData->jumpCmdHolder.outsideJumpCmdCount;
		++playerData->total_outside_jump;
		if (playerData->total_bhopCount != 0) // Yes it can happen ...
		{
			playerData->totaloutsidepercent = ((float)(playerData->total_outside_jump) / (float)(playerData->total_bhopCount)) * 100.0f;
		}
	}

	SystemVerbose1 ( Helpers::format ( "Player %s pushed the jump button.", ph->GetName () ) );
}

void JumpTester::OnPlayerJumpButtonUp ( PlayerHandler::iterator ph, int game_tick )
{
	JumpInfoT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	SystemVerbose1 ( Helpers::format ( "Player %s released the jump button.", ph->GetName () ) );

	playerData->jumpCmdHolder.JumpUp_Tick = game_tick;
	playerData->jumpCmdHolder.lastJumpCmdState = false;
}

JumpTester g_JumpTester;

const char * ConvertButton ( bool v )
{
	if( v ) return "Button Down";
	else return "Button Up";
}

basic_string Detection_BunnyHopScript::GetDataDump ()
{
	return Helpers::format ( ":::: BunnyHopInfoT {\n"
							 ":::::::: OnGroundHolderT {\n"
							 ":::::::::::: On Ground At (Tick #) : %d,\n"
							 ":::::::::::: Leave Ground At (Tick #) : %d,\n"
							 ":::::::::::: Jump Count : %d\n"
							 ":::::::: },\n"
							 ":::::::: JumpCmdHolderT {\n"
							 ":::::::::::: Last Jump Command : %s s,\n"
							 ":::::::::::: Jump Button Down At (Tick #) : %d,\n"
							 ":::::::::::: Jump Button Up At (Tick #) : %d,\n"
							 ":::::::::::: Jump Commands Done While Flying : %d\n"
							 ":::::::: },\n"
							 ":::::::: Total Bunny Hop Count : %d,\n"
							 ":::::::: Good Bunny Hop Count : %d,\n"
							 ":::::::: Total Jump Commands Done While Flying : %d,\n"
							 ":::::::: Total Jump Commands Done While Flying Ratio : %f,\n"
							 ":::::::: Perfect Bunny Hop Ratio : %f %%,\n"
							 ":::::::: Perfect Bunny Hop Count : %d\n"
							 ":::: }",
							 GetDataStruct ()->onGroundHolder.onGround_Tick, GetDataStruct ()->onGroundHolder.notOnGround_Tick, GetDataStruct ()->onGroundHolder.jumpCount,
							 ConvertButton ( GetDataStruct ()->jumpCmdHolder.lastJumpCmdState ), GetDataStruct ()->jumpCmdHolder.JumpDown_Tick, GetDataStruct ()->jumpCmdHolder.JumpUp_Tick, GetDataStruct ()->jumpCmdHolder.outsideJumpCmdCount,
							 GetDataStruct ()->total_bhopCount,
							 GetDataStruct ()->goodBhopsCount,
							 GetDataStruct()->total_outside_jump,
							 GetDataStruct()->totaloutsidepercent,
							 GetDataStruct ()->perfectBhopsPercent,
							 GetDataStruct ()->perfectBhopsCount );
}
