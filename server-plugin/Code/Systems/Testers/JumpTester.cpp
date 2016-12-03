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

#include "JumpTester.h"

#include "Systems/BanRequest.h"

/*
	Test each player to see if they use any script to help BunnyHop.

	Some players jumps just-in-time without using any script.
	We have to make the difference by using statistics.
*/

JumpTester::JumpTester () :
	BaseDynamicSystem ( "JumpTester" ),
	OnGroundHookListener (),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	singleton_class (),
	convar_sv_enablebunnyhopping ( nullptr ),
	convar_sv_autobunnyhopping ( nullptr )
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
		Logger::GetInstance ()->Msg<MSG_WARNING> ( "JumpTester::Init : Unable to locate ConVar sv_enablebunnyhopping" );
	}

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		convar_sv_autobunnyhopping = SourceSdk::InterfacesProxy::ICvar_FindVar ( "sv_autobunnyhopping" );

		if( convar_sv_enablebunnyhopping == nullptr )
		{
			Logger::GetInstance ()->Msg<MSG_WARNING> ( "JumpTester::Init : Unable to locate ConVar sv_enablebunnyhopping" );
		}
	}
}

void JumpTester::Load ()
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	OnGroundHookListener::RegisterOnGroundHookListener ( this );
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, SystemPriority::UserCmdHookListener::JumpTester );
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
	PlayerHandler::const_iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void JumpTester::RT_m_hGroundEntityStateChangedCallback ( PlayerHandler::const_iterator ph, bool new_isOnGround )
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

PlayerRunCommandRet JumpTester::RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void* pCmd, void* old_cmd )
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

	bool last_jump_button_state;
	bool cur_jump_button_state;

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		last_jump_button_state = ( static_cast< SourceSdk::CUserCmd_csgo* >( old_cmd )->buttons & IN_JUMP ) != 0;
		cur_jump_button_state = ( static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->buttons & IN_JUMP ) != 0;
	}
	else
	{
		last_jump_button_state = ( static_cast< SourceSdk::CUserCmd* >( old_cmd )->buttons & IN_JUMP ) != 0;
		cur_jump_button_state = ( static_cast< SourceSdk::CUserCmd* >( pCmd )->buttons & IN_JUMP ) != 0;
	}

	bool const jump_button_changed ( last_jump_button_state ^ cur_jump_button_state );

	if( !jump_button_changed )
	{
		LoggerAssert ( last_jump_button_state == cur_jump_button_state );
		return drop_cmd;
	}
	else
	{
		LoggerAssert ( last_jump_button_state != cur_jump_button_state );

		if( cur_jump_button_state )
		{
			OnPlayerJumpButtonDown ( ph, Helpers::GetGameTickCount () );
		}
		else
		{
			OnPlayerJumpButtonUp ( ph, Helpers::GetGameTickCount () );
		}
	}

	return drop_cmd;
}

void JumpTester::OnPlayerTouchGround ( PlayerHandler::const_iterator ph, int game_tick )
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
			TriggerDetection ( Detection_BunnyHopJumpMacro, ph, playerData );
		}
	}

	if( playerData->jumpCmdHolder.outsideJumpCmdCount == 0 && playerData->perfectBhopsCount > 5 && playerData->perfectBhopsPercent >= std::max (0, ( 100 - std::min ( 95, playerData->perfectBhopsCount * 2 ) ) ) )
	{
		TriggerDetection ( Detection_BunnyHopPerfect, ph, playerData );
	}

	playerData->jumpCmdHolder.outsideJumpCmdCount = 0;
}

void JumpTester::OnPlayerLeaveGround ( PlayerHandler::const_iterator ph, int game_tick )
{
	JumpInfoT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	playerData->onGroundHolder.notOnGround_Tick = Helpers::GetGameTickCount ();
	++playerData->onGroundHolder.jumpCount;
	playerData->isOnGround = false;
	SystemVerbose1 ( Helpers::format ( "Player %s leaved the ground.", ph->GetName () ) );
}

void JumpTester::OnPlayerJumpButtonDown ( PlayerHandler::const_iterator ph, int game_tick )
{
	JumpInfoT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	playerData->jumpCmdHolder.JumpDown_Tick = game_tick;
	playerData->jumpCmdHolder.lastJumpCmdState = true;

	int const cmd_diff ( game_tick - playerData->jumpCmdHolder.JumpUp_Tick );
	int const wd_diff ( game_tick - playerData->onGroundHolder.onGround_Tick );

	if( cmd_diff > 0 && cmd_diff <= 3 )
	{
		TriggerDetection ( Detection_BunnyHopJumpMacro, ph, playerData );
	}

	if( playerData->isOnGround )
	{
		if( wd_diff >= 0 && wd_diff < 10 )
		{
			++playerData->total_bhopCount;
			SystemVerbose1 ( Helpers::format ( "Player %s : total_bhopCount = %d\n", ph->GetName (), playerData->total_bhopCount ) );

			if( wd_diff == 0 )
			{
				++playerData->perfectBhopsCount;
				__assume ( playerData->perfectBhopsCount <= playerData->total_bhopCount );
				playerData->perfectBhopsPercent = ( int ) ( ( playerData->perfectBhopsCount / playerData->total_bhopCount ) * 100.0f );
				SystemVerbose1 ( Helpers::format ( "Player %s : perfectBhopsCount = %d\n", ph->GetName (), playerData->perfectBhopsCount ) );
			}
			else if( wd_diff < 3 )
			{
				++playerData->goodBhopsCount;
				SystemVerbose1 ( Helpers::format ( "Player %s : goodBhopsCount = %d\n", ph->GetName (), playerData->goodBhopsCount ) );
			}
		}
	}
	else
	{
		++playerData->jumpCmdHolder.outsideJumpCmdCount;
	}

	SystemVerbose1 ( Helpers::format ( "Player %s pushed the jump button.", ph->GetName () ) );
}

void JumpTester::OnPlayerJumpButtonUp ( PlayerHandler::const_iterator ph, int game_tick )
{
	JumpInfoT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	SystemVerbose1 ( Helpers::format ( "Player %s released the jump button.", ph->GetName () ) );

	playerData->jumpCmdHolder.JumpUp_Tick = game_tick;
	playerData->jumpCmdHolder.lastJumpCmdState = false;
}

const char * ConvertButton ( bool v )
{
	if( v ) return "Button Down";
	else return "Button Up";
}

void Base_Detection_BunnyHop::WriteXMLOutput ( FILE * const out ) const
{
	Assert ( out );

	fprintf ( out,
			  "<base_detection_bunnyhop>\n\t\t\t"
			  "<struct name=\"OnGroundHolderT\">\n\t\t\t\t"
			  "<value name=\"onGround_Tick\" desc=\"Tick count the player landed on ground\" unit=\"tick\">%d</value>\n\t\t\t\t\t"
			  "<value name=\"notOnGround_Tick\" desc=\"Tick count the player started flying\" unit=\"tick\">%d</value>\n\t\t\t\t\t"
			  "<value name=\"jumpCount\" desc=\"Number of times the player landed on ground\" unit=\"count\">%d</value>\n\t\t\t\t"
			  "</struct>\n\t\t\t"
			  "<struct name=\"JumpCmdHolderT\">\n\t\t\t\t"
			  "<value name=\"lastJumpCmdState\" desc=\"Latest received jump command\" unit=\"text\">%s</value>\n\t\t\t\t\t"
			  "<value name=\"JumpDown_Tick\" desc=\"Latest tick the server was aware the player hit the jump button\" unit=\"tick\">%d</value>\n\t\t\t\t\t"
			  "<value name=\"JumpUp_Tick\" desc=\"Latest tick the server was aware the player released the jump button\" unit=\"tick\">%d</value>\n\t\t\t\t\t"
			  "<value name=\"outsideJumpCmdCount\" desc=\"Number of times the jump command was received while the player was flying\" unit=\"count\">%d</value>\n\t\t\t\t\t"
			  "</struct>\n\t\t\t"
			  "<value name=\"total_bhopCount\" desc=\"Number of jumps near BunnyHopping\" unit=\"count\">%d</value>\n\t\t\t\t"
			  "<value name=\"goodBhopsCount\" desc=\"Number of jumps closer to BunnyHopping\" unit=\"count\">%d</value>\n\t\t\t\t"
			  "<value name=\"perfectBhopsPercent\" desc=\"Percent of perfect bunny hops\" unit=\"percent\">%d</value>\n\t\t\t\t"
			  "<value name=\"perfectBhopsCount\" desc=\"Number of perfect bunny hops\" unit=\"count\">%d</value>\n\t\t\t"
			  "</base_detection_bunnyhop>",
			GetDataStruct ()->onGroundHolder.onGround_Tick,
			GetDataStruct ()->onGroundHolder.notOnGround_Tick,
			GetDataStruct ()->onGroundHolder.jumpCount,
			ConvertButton ( GetDataStruct ()->jumpCmdHolder.lastJumpCmdState ),
			GetDataStruct ()->jumpCmdHolder.JumpDown_Tick,
			GetDataStruct ()->jumpCmdHolder.JumpUp_Tick,
			GetDataStruct ()->jumpCmdHolder.outsideJumpCmdCount,
			GetDataStruct ()->total_bhopCount,
			GetDataStruct ()->goodBhopsCount,
			GetDataStruct ()->perfectBhopsPercent,
			GetDataStruct ()->perfectBhopsCount
	);
}

void Detection_BunnyHopJumpMacro::TakeAction ()
{
	m_player->Kick ();
}

void Detection_BunnyHopPerfect::TakeAction ()
{
	BanRequest::GetInstance ()->AddAsyncBan ( *m_player, 0, nullptr );
}
