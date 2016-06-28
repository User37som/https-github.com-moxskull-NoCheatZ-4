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
	playerdata_class(),
	PlayerRunCommandHookListener(),
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
	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		ResetPlayerDataStructByIndex(it.GetIndex());
	}

	OnTickListener::RegisterOnTickListener(this);
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, 1);
}

void SpeedTester::Unload()
{
	OnTickListener::RemoveOnTickListener(this);
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);
}

void SpeedTester::ProcessPlayerTestOnTick(PlayerHandler::const_iterator ph, float const curtime)
{
	SpeedHolderT* const pInfo = this->GetPlayerDataStructByIndex(ph.GetIndex());
	float const tick_interval = SourceSdk::InterfacesProxy::Call_GetTickInterval();
	const float newTicks = ceil((curtime - pInfo->lastTest) / tick_interval);
	SourceSdk::INetChannelInfo* const netchan = ph->GetChannelInfo();
	if(netchan == nullptr) return;

	const float latency = netchan->GetLatency(FLOW_OUTGOING);

	if (!pInfo->ticksLeft && fabs(pInfo->previousLatency - latency) <= 0.005f)
	{
		++(pInfo->detections);

		DebugMessage(Helpers::format("Player %s :  Speedhack pre-detection #%ud", ph->GetName(), pInfo->detections));

		if (pInfo->detections >= 30 && curtime > pInfo->lastDetectionTime + 30.0f)
		{
			Detection_SpeedHack pDetection = Detection_SpeedHack();
			pDetection.PrepareDetectionData(pInfo);
			pDetection.PrepareDetectionLog(ph, this);
			pDetection.Log();

			pInfo->lastDetectionTime = curtime;

			ph->Ban();
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
	pInfo->lastTest = curtime;
}

PlayerRunCommandRet SpeedTester::PlayerRunCommandCallback(PlayerHandler::const_iterator ph, void* pCmd, void* old_cmd)
{
	float& tl = this->GetPlayerDataStructByIndex(ph.GetIndex())->ticksLeft;

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
						GetDataStruct()->lastTest);					
}
