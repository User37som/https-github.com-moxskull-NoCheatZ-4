/*
Copyright 2012 - Le Padellec Sylvain

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "MouseTester.h"

#include "Misc/EntityProps.h"
#include "Misc/QueryCvarProvider.h"

MouseTester::MouseTester() :
	BaseTesterSystem ("MouseTester_BETA"),
	PlayerDataStructHandler<MouseInfo>(),
	PlayerRunCommandHookListener(),
	Singleton()
{
}

MouseTester::~MouseTester()
{
	Unload();
}

void MouseTester::Init()
{
	InitDataStruct();
}

void MouseTester::Load()
{
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, SystemPriority::MouseTester, SlotStatus::PLAYER_CONNECTING);
}

void MouseTester::Unload()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);
}

bool MouseTester::GotJob() const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it(&filter_class);
	// Return if we have job to do or not ...
	return it != PlayerHandler::end();
}

float AngleDistance(float next, float cur)
{
	float delta = next - cur;

	if (delta < -180.0f)
		delta += 360.0f;
	else if (delta > 180.0f)
		delta -= 360.0f;

	return delta;
}

PlayerRunCommandRet MouseTester::RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, void * const old_cmd)
{
	if (!IsActive())
		return PlayerRunCommandRet::CONTINUE;

	short * md(nullptr);
	int* buttons(nullptr);
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		md = &(reinterpret_cast<SourceSdk::CUserCmd_csgo*>(cmd)->mousedx);
		buttons = &(reinterpret_cast<SourceSdk::CUserCmd_csgo*>(cmd)->buttons);
	}
	else
	{
		md = &(reinterpret_cast<SourceSdk::CUserCmd*>(cmd)->mousedx);
		buttons = &(reinterpret_cast<SourceSdk::CUserCmd*>(cmd)->buttons);
	}

	MouseInfo * const pInfo(GetPlayerDataStructByIndex(ph.GetIndex()));
	SourceSdk::QAngle const * userangles(&(reinterpret_cast<SourceSdk::CUserCmd*>(cmd)->viewangles));

	if (pInfo->m_prev_set && ph == SlotStatus::PLAYER_IN_TESTS)
	{
		int const * const flags(g_EntityProps.GetPropValue<int, PROP_FLAGS>(ph->GetEdict()));
		if ((*flags & (3 << 5)) == 0) // FL_FROZEN | FL_ATCONTROLS
		{
			/// PITCH

			float const pitch_delta = AngleDistance(userangles->x, pInfo->m_prev_pitch_angle);
			float const yaw_delta = AngleDistance(userangles->y, pInfo->m_prev_yaw_angle);

			if (pInfo->m_pitch_set && md[1] != 0 && ((signbit(pitch_delta) == (signbit((float)md[1]))) ^ !pInfo->m_mouse_pitch_inverted) && pitch_delta != 0.0f)
			{
				if (++(pInfo->m_pitch_dir_detect_row) > 1)
				{
					MouseDetectInfo detect(*pInfo, md, buttons, userangles, pitch_delta, yaw_delta);
					ProcessDetectionAndTakeAction<MouseDetectInfo>(Detection_MouseMismatch(), &detect, ph, this);

					pInfo->m_pitch_dir_detect_row = 0;
				}
			}
			else if (!pInfo->m_pitch_set && !pInfo->m_asked_m_pitch)
			{
				g_QueryCvarProvider.StartQueryCvarValue(ph->GetEdict(), "m_pitch");
				pInfo->m_asked_m_pitch = true;
			}
			else
			{
				pInfo->m_pitch_dir_detect_row = 0;
			}

			/*else

			This detection is not safe at all because the client game process mouse on frame rather than on tick.
			A client with higher FPS than tickrate will send ligitimate 0 mouse values because mouse accum are processed
			and 0ed between ticks in this case.

			{
				
				DebugMessage(Helpers::format("md={%d, %d} prev={%f, %f}", md[0], md[1], pInfo->m_prev_mdx, pInfo->m_prev_mdy));
				if (fabs(pitch_delta) > 0.022f)
				{
					if (--(pInfo->m_pitch_dir_detect_row) < 10)
					{
						// sniper scope false detection
						DebugMessage("Pitch angle moved without mouse");
					}
				}
				else
				{
					pInfo->m_pitch_dir_detect_row = 0;
				}
			}*/

			/// YAW

			if (md[0] != 0 && ((*buttons & 3 << 7) == 0)) // IN_LEFT | IN_RIGHT
			{
				if (signbit(yaw_delta) == signbit((float)md[0]) && yaw_delta != 0.0f)
				{
					if (++(pInfo->m_yaw_dir_detect_row) > 1)
					{
						MouseDetectInfo detect(*pInfo, md, buttons, userangles, pitch_delta, yaw_delta);
						ProcessDetectionAndTakeAction<MouseDetectInfo>(Detection_MouseMismatch(), &detect, ph, this);

						pInfo->m_yaw_dir_detect_row = 0;
					}
				}
				else
				{
					pInfo->m_yaw_dir_detect_row = 0;
				}
			}
			else
			{
				pInfo->m_yaw_dir_detect_row = 0;
			}
		}
	}

	pInfo->m_prev_pitch_angle = userangles->x;
	pInfo->m_prev_yaw_angle = userangles->y;
	//pInfo->m_prev_mdx = ((float)md[0] + pInfo->m_prev_mdx) / 1.0625f; // average for multiple frames
	//pInfo->m_prev_mdy = ((float)md[1] + pInfo->m_prev_mdy) / 1.0625f;
	pInfo->m_prev_set = true;

	return PlayerRunCommandRet::CONTINUE;
}

void MouseTester::ProcessPitchConVar(PlayerHandler::iterator ph, char const * val)
{
	MouseInfo * const pInfo(GetPlayerDataStructByIndex(ph.GetIndex()));

	double m_pitch_val = atof(val);
	pInfo->m_mouse_pitch_inverted = signbit(m_pitch_val);
	pInfo->m_prev_set = false;
	pInfo->m_pitch_set = true;
	//g_QueryCvarProvider.StartQueryCvarValue(ph->GetEdict(), "m_pitch");
}

MouseTester g_MouseTester;

basic_string Detection_MouseMismatch::GetDataDump()
{
	MouseDetectInfo* const p(GetDataStruct());

	return basic_string(Helpers::format(
		":::: MouseDetectInfo {\n"
		":::::::: Previous Pitch Angle : %f,\n"
		":::::::: Current Pitch Angle : %f (Delta %f),\n"
		":::::::: Is Mouse Inverted ? : %d,\n"
		":::::::: Mouse Delta Y : %d,\n"
		":::::::: Previous Yaw Angle : %f,\n"
		":::::::: Current Yaw Angle : %f (Delta %f),\n"
		":::::::: Mouse Delta X : %d,\n"
		":::::::: CMD Buttons : 0x%X,\n"
		"::::}",
		p->m_prev_pitch_angle,
		p->m_cur_pitch_angle, p->m_pitch_delta,
		p->m_mouse_pitch_inverted,
		p->m_mouse_delta_y,
		p->m_prev_yaw_angle,
		p->m_cur_yaw_angle, p->m_yaw_delta,
		p->m_mouse_delta_x,
		p->m_buttons
	));
}
