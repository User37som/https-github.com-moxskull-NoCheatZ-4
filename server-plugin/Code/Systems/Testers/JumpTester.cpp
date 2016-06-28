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

JumpTester::JumpTester() :
	BaseSystem("JumpTester"),
	OnGroundHookListener(),
	playerdata_class(),
	PlayerRunCommandHookListener(),
	singleton_class()
{
}

JumpTester::~JumpTester()
{
	Unload();
}

void JumpTester::Init()
{
	InitDataStruct();
}

void JumpTester::Load()
{
	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		ResetPlayerDataStructByIndex(it.GetIndex());
	}

	OnGroundHookListener::RegisterOnGroundHookListener(this);
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, 3);
}

void JumpTester::Unload()
{
	OnGroundHookListener::RemoveOnGroundHookListener(this);
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);
}

int GetGameTickCount()
{
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		return static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
	}
	else
	{
		return static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
	}
}

void JumpTester::m_hGroundEntityStateChangedCallback(PlayerHandler::const_iterator ph, bool new_isOnGround)
{
	JumpInfoT* playerData = GetPlayerDataStructByIndex(ph.GetIndex());

	if(new_isOnGround)
	{
		playerData->onGroundHolder.onGround_Tick = GetGameTickCount();
		playerData->isOnGround = true;
		SystemVerbose1(Helpers::format("Player %s touched the ground.", ph->GetName()));
		if(playerData->jumpCmdHolder.outsideJumpCmdCount > 10) // Il serait plus judicieux d'utiliser le RMS
		{
			Detection_BunnyHopScript pDetection = Detection_BunnyHopScript();
			pDetection.PrepareDetectionData(playerData);
			pDetection.PrepareDetectionLog(ph, this);
			pDetection.Log();
			ph->Kick("You have to turn off your BunnyHop Script to play on this server.");
		}
		else if(playerData->jumpCmdHolder.outsideJumpCmdCount == 0 && playerData->perfectBhopsCount > 5)
		{
			Detection_BunnyHopProgram pDetection = Detection_BunnyHopProgram();
			pDetection.PrepareDetectionData(playerData);
			pDetection.PrepareDetectionLog(ph, this);
			pDetection.Log();

			ph->Ban("[NoCheatZ 4] You have been banned for using BunnyHop on this server.");
		}
		playerData->jumpCmdHolder.outsideJumpCmdCount = 0;
	}
	else
	{
		playerData->onGroundHolder.notOnGround_Tick = GetGameTickCount();
		++playerData->onGroundHolder.jumpCount;
		playerData->isOnGround = false;
		SystemVerbose1(Helpers::format("Player %s leaved the ground.", ph->GetName()));
	}
}

PlayerRunCommandRet JumpTester::PlayerRunCommandCallback(PlayerHandler::const_iterator ph, void* pCmd, void* old_cmd)
{
	PlayerRunCommandRet drop_cmd = CONTINUE;

	JumpInfoT* playerData = GetPlayerDataStructByIndex(ph.GetIndex());

	bool cur_in_jump;

	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		cur_in_jump = (static_cast<SourceSdk::CUserCmd_csgo*>(pCmd)->buttons & IN_JUMP) != 0;
	}
	else
	{
		cur_in_jump = (static_cast<SourceSdk::CUserCmd*>(pCmd)->buttons & IN_JUMP) != 0;
	}

	if((playerData->jumpCmdHolder.lastJumpCmdState == false) && cur_in_jump)
	{
		playerData->jumpCmdHolder.JumpDown_Tick = GetGameTickCount();
		if(playerData->isOnGround)
		{
			int diff = abs(playerData->jumpCmdHolder.JumpDown_Tick - playerData->onGroundHolder.onGround_Tick);
			if(diff < 10)
			{
				++playerData->total_bhopCount;
#				ifdef DEBUG
					printf("Player %s : total_bhopCount = %d\n", ph->GetName(), playerData->total_bhopCount);
#				endif
				if(diff < 3 && diff > 0)
				{
					++playerData->goodBhopsCount;
#					ifdef DEBUG
						printf("Player %s : goodBhopsCount = %d\n", ph->GetName(), playerData->goodBhopsCount);
#					endif
					drop_cmd = INERT;
				}
				if(diff == 0)
				{
					++playerData->perfectBhopsCount;
#					ifdef DEBUG
						printf("Player %s : perfectBhopsCount = %d\n", ph->GetName(), playerData->perfectBhopsCount);
#					endif
					drop_cmd = INERT;
				}
			}

			SystemVerbose1(Helpers::format("Player %s pushed the jump button.", ph->GetName()));
		}
		else
		{
			++playerData->jumpCmdHolder.outsideJumpCmdCount;
			SystemVerbose1(Helpers::format("Player %s pushed the jump button while flying.", ph->GetName()));
		}
		playerData->jumpCmdHolder.lastJumpCmdState = true;
	}
	else if((playerData->jumpCmdHolder.lastJumpCmdState == true) && !cur_in_jump)
	{
		playerData->jumpCmdHolder.lastJumpCmdState = false;
		playerData->jumpCmdHolder.JumpUp_Tick = GetGameTickCount();
		SystemVerbose1(Helpers::format("Player %s released the jump button.", ph->GetName()));
	}
	return drop_cmd;
}

const char * ConvertButton(bool v)
{
	if(v) return "Button Down";
	else return "Button Up";
}

basic_string Detection_BunnyHopScript::GetDataDump()
{
	return Helpers::format( ":::: BunnyHopInfoT {\n"
							":::::::: OnGroundHolderT {\n"
							":::::::::::: On Ground At (Tick #) : %d,\n"
							":::::::::::: Leave Ground At (Tick #) : %d,\n"
							":::::::::::: Jump Count : %d\n"
							":::::::: },\n"
							":::::::: JumpCmdHolderT {\n"
							":::::::::::: Last Jump Command : %s,\n"
							":::::::::::: Jump Button Down At (Tick #) : %d,\n"
							":::::::::::: Jump Button Up At (Tick #) : %d,\n"
							":::::::::::: Jump Commands Done While Flying : %d\n"
							":::::::: },\n"
							":::::::: Total Bunny Hop Count : %d,\n"
							":::::::: Good Bunny Hop Count : %d,\n"
							":::::::: Perfect Bunny Hop Ratio : %d %%,\n"
							":::::::: Perfect Bunny Hop Count : %d\n"
							":::: }",
							GetDataStruct()->onGroundHolder.onGround_Tick, GetDataStruct()->onGroundHolder.notOnGround_Tick, GetDataStruct()->onGroundHolder.jumpCount,
							ConvertButton(GetDataStruct()->jumpCmdHolder.lastJumpCmdState), GetDataStruct()->jumpCmdHolder.JumpDown_Tick, GetDataStruct()->jumpCmdHolder.JumpUp_Tick, GetDataStruct()->jumpCmdHolder.outsideJumpCmdCount,
							GetDataStruct()->total_bhopCount,
							GetDataStruct()->goodBhopsCount,
							GetDataStruct()->perfectBhopsPercent,
							GetDataStruct()->perfectBhopsCount);
}
