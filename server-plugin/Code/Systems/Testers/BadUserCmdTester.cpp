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

#include "BadUserCmdTester.h"

#include "Interfaces/InterfacesProxy.h"

#include "Systems/Logger.h"

BadUserCmdTester::BadUserCmdTester() :
	BaseTesterSystem ( "BadUserCmdTester" ),
	playerdatahandler_class (),
	PlayerRunCommandHookListener (),
	Singleton ()
{
}

BadUserCmdTester::~BadUserCmdTester()
{
	Unload ();
}

void BadUserCmdTester::Init ()
{
	InitDataStruct ();
}

void BadUserCmdTester::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin ()); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, SystemPriority::BadUserCmdTester, SlotStatus::PLAYER_CONNECTING );
}

void BadUserCmdTester::Unload ()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( this );
}

bool BadUserCmdTester::GotJob () const
{
	/*// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();*/

	return true;
}

PlayerRunCommandRet BadUserCmdTester::RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void* pCmd, double const & curtime)
{
	SourceSdk::CUserCmd_csgo const * const k_newcmd ( ( SourceSdk::CUserCmd_csgo const * const )pCmd);

	if( k_newcmd->command_number <= 0 ) // Null cmd.
	{
		//DebugMessage(Helpers::format("Droped CUserCmd from %s (null command number)", ph->GetName()));
		return PlayerRunCommandRet::BLOCK;
	}

	UserCmdInfo* const pInfo ( GetPlayerDataStruct ( *ph ));

	if( k_newcmd->tick_count <= 0 )
		pInfo->m_tick_status = TickStatus_t::IN_RESET;

	bool isDead ( true);
	SourceSdk::IPlayerInfo * const player_info ( ph->GetPlayerInfo ());
	if( player_info )
	{
		isDead = player_info->IsDead ();
	}

	if( ( isDead | pInfo->m_prev_dead ) || curtime <= pInfo->m_detected_time )
	{
		pInfo->m_prev_dead = isDead;

		if(pInfo->prev_cmd >= k_newcmd->command_number )
		{
			if( pInfo->m_tick_status == TickStatus_t::IN_RESET )
				pInfo->m_tick_status = TickStatus_t::RESET;
		
			++(pInfo->cmd_offset);
		}
		else
		{
			if( pInfo->m_tick_status == TickStatus_t::RESET )
				pInfo->m_tick_status = TickStatus_t::OK;

			pInfo->prev_cmd = k_newcmd->command_number;
			pInfo->cmd_offset = 1;
		}

		pInfo->prev_tick = k_newcmd->tick_count;

		return PlayerRunCommandRet::CONTINUE;
	}
	
	if( pInfo->prev_cmd > k_newcmd->command_number )
	{
		if( pInfo->m_tick_status != TickStatus_t::OK )
		{
			pInfo->m_tick_status = TickStatus_t::RESET;
			DebugMessage(Helpers::format("Droped CUserCmd from %s (tick reset but command number changed)", ph->GetName()));
			return PlayerRunCommandRet::BLOCK;
		}

		pInfo->m_detected_time = curtime + 10.0f;

		// Push detection

		BadCmdInfo detect_info(pInfo, k_newcmd);
		ProcessDetectionAndTakeAction<BadCmdInfo>(Detection_BadUserCmd(), &detect_info, ph, this);

		return PlayerRunCommandRet::BLOCK;
	}
	else if( pInfo->prev_cmd == k_newcmd->command_number )
	{
		if( pInfo->m_tick_status != TickStatus_t::OK )
		{
			pInfo->m_tick_status = TickStatus_t::RESET;

			return PlayerRunCommandRet::BLOCK;
		}

		bool const b1(SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive);
		bool const b2(pInfo->prev_tick != k_newcmd->tick_count);
		bool const b3(pInfo->prev_tick + 1 != k_newcmd->tick_count);
		if ( b3 && ((b1 && b2) || (!b1)) )
		{
			pInfo->m_detected_time = curtime + 10.0f;

			// Push detection

			BadCmdInfo detect_info(pInfo, k_newcmd);
			ProcessDetectionAndTakeAction<BadCmdInfo>(Detection_BadUserCmd(), &detect_info, ph, this);

			return PlayerRunCommandRet::BLOCK;
		}
		
		++(pInfo->cmd_offset);
	}
	else
	{
		pInfo->cmd_offset = 1;
	}

	pInfo->prev_cmd = k_newcmd->command_number;
	pInfo->prev_tick = k_newcmd->tick_count;

	if( pInfo->m_tick_status == TickStatus_t::RESET )
		pInfo->m_tick_status = TickStatus_t::OK;

	return PlayerRunCommandRet::CONTINUE;
}

BadUserCmdTester g_BadUserCmdTester;

basic_string Detection_BadUserCmd::GetDataDump()
{
	return Helpers::format(
		":::: BadCmdInfo {\n"
		":::::::: UserCmdInfo {\n"
		":::::::::::: Tick count status %d,\n"
		":::::::::::: Previous command number %d (expected offset %d),\n"
		":::::::::::: Previous tick count %d,\n"
		":::::::: }\n"
		":::::::: CUserCmd {\n"
		":::::::::::: Command number %d,\n"
		":::::::::::: Tick count %d,\n"
		":::::::: }\n"
		":::: }\n"
		,
		GetDataStruct()->m_inner.m_tick_status,
		GetDataStruct()->m_inner.prev_cmd,
		GetDataStruct()->m_inner.prev_tick,
		GetDataStruct() ->m_current_cmd.command_number,
		GetDataStruct()->m_current_cmd.tick_count);
}
