/*
	Copyright 2017 -	SM91337 @ fragdeluxe.com
						Le Padellec Sylvain

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

#include "AutoStrafeTester.h"

#include "Misc/EntityProps.h"

AutoStrafeTester::AutoStrafeTester() :
	BaseTesterSystem("AutoStrafeTester"),
	playerdatahandler_class(),
	PlayerRunCommandHookListener(),
	Singleton()
{
}

AutoStrafeTester::~AutoStrafeTester()
{
	Unload();
}

void AutoStrafeTester::Init()
{
	InitDataStruct();
}

void AutoStrafeTester::Load()
{
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, SystemPriority::AutoStrafeTester, SlotStatus::PLAYER_IN_TESTS);
}

void AutoStrafeTester::Unload()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);
}

bool AutoStrafeTester::GotJob() const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it(&filter_class);
	// Return if we have job to do or not ...
	return it != PlayerHandler::end();
}

PlayerRunCommandRet AutoStrafeTester::RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime)
{
	if(((*g_EntityProps.GetPropValue<int, PROP_FLAGS>(ph->GetEdict())) & (3 << 5)) == 0)
	{
		SourceSdk::Vector vel;
		int const * buttons;

		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			vel.x = ((SourceSdk::CUserCmd_csgo const *)cmd)->forwardmove;
			vel.y = ((SourceSdk::CUserCmd_csgo const *)cmd)->sidemove;
			//vel.z = ((SourceSdk::CUserCmd_csgo const *)cmd)->upmove;
			buttons = &(((SourceSdk::CUserCmd_csgo const *)cmd)->buttons);
		}
		else
		{
			vel.x = ((SourceSdk::CUserCmd const *)cmd)->forwardmove;
			vel.y = ((SourceSdk::CUserCmd const *)cmd)->sidemove;
			//vel.z = ((SourceSdk::CUserCmd const *)cmd)->upmove;
			buttons = &(((SourceSdk::CUserCmd const *)cmd)->buttons);
		}

		bool detect(false);
		if (vel.x < 0.0f && !(*buttons & (1 << 4))) // IN_BACK
		{
			detect = true;
		}
		else if (vel.x > 0.0f && !(*buttons & (1 << 3))) // IN_FORWARD
		{
			detect = true;
		}

		if (vel.y < 0.0f && !(*buttons & (1 << 9))) // IN_MOVELEFT
		{
			detect = true;
		}
		else if (vel.y > 0.0f && !(*buttons & (1 << 10))) // IN_MOVERIGHT
		{
			detect = true;
		}

		if (detect)
		{
			StrafeDetectInfo * const pdata(GetPlayerDataStructByIndex(ph.GetIndex()));
			++(pdata->m_detect_count);

			if (pdata->m_next_detect_time > curtime)
			{
				pdata->m_buttons = *buttons;
				pdata->m_next_detect_time = curtime + 5.0;
				SourceSdk::VectorCopy(vel, pdata->m_vel);

				ProcessDetectionAndTakeAction<AutoStrafeDetection::data_type>(AutoStrafeDetection(), pdata, ph, this);
			}
		}
	}

	return PlayerRunCommandRet::CONTINUE;
}

basic_string AutoStrafeDetection::GetDataDump()
{
	return basic_string(Helpers::format(
		":::: StrafeDetectInfo {\n"
		":::::::: Wanted velocity X : %f\n"
		":::::::: Wanted velocity Y : %f\n"
		":::::::: Buttons : %X\n"
		":::::::: Detect count : %u\n"
		":::: }"
		,
		GetDataStruct()->m_vel.x,
		GetDataStruct()->m_vel.y,
		GetDataStruct()->m_buttons,
		GetDataStruct()->m_detect_count
	));
}

AutoStrafeTester g_AutoStrafeTester;
