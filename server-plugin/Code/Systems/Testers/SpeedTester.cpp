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

#include "SpeedTester.h"

#include "Systems/Logger.h"

SpeedTester::SpeedTester() :
	BaseSystem("SpeedTester"),
	OnTickListener(),
	PlayerRunCommandHookListener(),
	playerdata_class(),
	singleton_class()
{
}

SpeedTester::~SpeedTester()
{
	Unload();
}

void SpeedTester::Init()
{
	InitDataStruct();
}

void SpeedTester::Load()
{
	OnTickListener::RegisterOnTickListener(this);
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, 1);
}

void SpeedTester::Unload()
{
	OnTickListener::RemoveOnTickListener(this);
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);

	PLAYERS_LOOP_RUNTIME
	{
		ResetPlayerDataStruct(ph->playerClass);
	}
	END_PLAYERS_LOOP
}

void SpeedTester::ProcessPlayerTestOnTick(NczPlayer* player)
{
	const float game_time = Plat_FloatTime();
	SpeedHolderT* const pInfo = this->GetPlayerDataStruct(player);
	float const tick_interval = SourceSdk::InterfacesProxy::Call_GetTickInterval();
	const float newTicks = ceil((game_time - pInfo->lastTest) / tick_interval);
	SourceSdk::INetChannelInfo* const netchan = player->GetChannelInfo();
	if(netchan == nullptr) return;

	const float latency = netchan->GetLatency(FLOW_OUTGOING);

	if (!pInfo->ticksLeft && fabs(pInfo->previousLatency - latency) <= 0.005f)
	{
		++(pInfo->detections);

		SystemVerbose1(Helpers::format("Player %s :  Speedhack pre-detection #%ud", player->GetName(), pInfo->detections));

		if (pInfo->detections >= 30 && game_time > pInfo->lastDetectionTime + 30.0f)
		{
			Detection_SpeedHack pDetection = Detection_SpeedHack();
			pDetection.PrepareDetectionData(pInfo);
			pDetection.PrepareDetectionLog(player, this);
			pDetection.Log();

			pInfo->lastDetectionTime = game_time;
		}
	}
	else if(pInfo->detections)
	{
		--(pInfo->detections);
	}

	float const vtest = ceil((1.0f / tick_interval * 2.0f));
	if ((pInfo->ticksLeft += newTicks) > vtest)
	{
		pInfo->ticksLeft = vtest;
	}
			
	pInfo->previousLatency = latency;
	pInfo->lastTest = game_time;
}

PlayerRunCommandRet SpeedTester::PlayerRunCommandCallback(NczPlayer* player, SourceSdk::CUserCmd* pCmd, const SourceSdk::CUserCmd& old_cmd)
{
	float& tl = this->GetPlayerDataStruct(player)->ticksLeft;

	if(!tl) return BLOCK;

	tl -= 1.0f;
	
	return CONTINUE;
}

basic_string Detection_SpeedHack::GetDataDump()
{
	return Helpers::format( ":::: SpeedHolderT {\n"
							":::::::: Ticks Left : %lu,\n"
							":::::::: Detections Count : %lu,\n"
							":::::::: Last Detection Time %f,\n"
							":::::::: Last Latency : %f,\n"
							":::::::: Last Test Time : %f\n"
							":::: }",
						GetDataStruct()->ticksLeft,
						GetDataStruct()->detections,
						GetDataStruct()->lastDetectionTime,
						GetDataStruct()->previousLatency,
						GetDataStruct()->lastTest).c_str();					
}
