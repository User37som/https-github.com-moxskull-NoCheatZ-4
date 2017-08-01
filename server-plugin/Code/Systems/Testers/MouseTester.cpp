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

#include <cmath> // signbit

#include "Misc/EntityProps.h"
#include "Misc/QueryCvarProvider.h"

MouseTester::MouseTester() :
	BaseTesterSystem ("MouseTester"),
	playerdatahandler_class(),
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

float AngleDistance(float next, float cur, bool & unsure)
{
	float delta = next - cur;
	unsure = false;

	if (delta < -180.0f)
	{
		delta += 360.0f;
		unsure = true;
	}
	else if (delta > 180.0f)
	{
		delta -= 360.0f;
		unsure = true;
	}

	return delta;
}

PlayerRunCommandRet MouseTester::RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime)
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

	MouseDetectInfo * const pInfo(GetPlayerDataStructByIndex(ph.GetIndex()));
	tb_mouse & MouseFrames(pInfo->m_frames);
	SourceSdk::QAngle const * userangles(&(reinterpret_cast<SourceSdk::CUserCmd*>(cmd)->viewangles));

	if (!pInfo->m_pitch_set && !pInfo->m_pitch_asked)
	{
		g_QueryCvarProvider.StartQueryCvarValue(ph->GetEdict(), "m_pitch");
		pInfo->m_pitch_asked = true;
		return PlayerRunCommandRet::CONTINUE;
	}

	MouseFrames.Store(MouseFrame(&(userangles->x), md, buttons, ph, ((*g_EntityProps.GetPropValue<int, PROP_FLAGS>(ph->GetEdict())) & (3 << 5)) != 0), curtime);

	FindDetection(ph, pInfo);

	return PlayerRunCommandRet::CONTINUE;
}

void MouseTester::FindDetection(PlayerHandler::iterator ph, MouseDetectInfo* info)
{
	tb_mouse::inner_type mouse_graph[TB_MOUSE_MAX_HISTORY];
	size_t amount;

	info->m_frames.CopyHistory(mouse_graph, amount, TB_MOUSE_MAX_HISTORY);

	if (amount == TB_MOUSE_MAX_HISTORY) // wait for the graph to be full before trying to detect
	{
		info->c = 0;
		info->percent_detected = 0.0f;
		info->min_consecutive_x = 0;
		info->cur_consecutive_x = 0;
		info->min_consecutive_y = 0;
		info->cur_consecutive_y = 0;

		tb_mouse::inner_type* it(mouse_graph);
		tb_mouse::inner_type const * const it_end(mouse_graph + TB_MOUSE_MAX_HISTORY);
		float prev_pitch(0.0f);
		float prev_yaw(0.0f);
		bool first(true);
		bool unsure(false);
		do
		{
			if (it->v.m_status == SlotStatus::PLAYER_IN_TESTS && !it->v.m_frozen && !first)
			{
				float const delta_y(AngleDistance(it->v.m_yaw_angle, prev_yaw, unsure));
				if (fabsf(delta_y) > 0.05f && it->v.m_x_mouse != 0 && std::signbit((float)it->v.m_x_mouse) != std::signbit(delta_y) && !(it->v.m_buttons & 3 << 7))
				{
					if (!unsure)
					{
						++info->cur_consecutive_y;
						if (info->cur_consecutive_y > 1)
						{
							++info->c;
						}
						if (info->cur_consecutive_y > info->min_consecutive_y)
						{
							info->min_consecutive_y = info->cur_consecutive_y;
						}
					}
				}
				else
				{
					info->cur_consecutive_y = 0;
				}

				float const delta_x(AngleDistance(it->v.m_pitch_angle, prev_pitch, unsure));
				if (fabsf(delta_x) > 0.05f && it->v.m_y_mouse != 0 && (std::signbit((float)it->v.m_y_mouse) == (std::signbit(delta_x) ^ info->m_inverted)))
				{
					if (!unsure)
					{
						++info->cur_consecutive_x;
						if (info->cur_consecutive_x > 1)
						{
							++info->c;
						}
						if (info->cur_consecutive_x > info->min_consecutive_x)
						{
							info->min_consecutive_x = info->cur_consecutive_x;
						}
					}
				}
				else
				{
					info->cur_consecutive_x = 0;
				}
			}
			else
			{
				info->cur_consecutive_x = 0;
				info->cur_consecutive_y = 0;
			}
			prev_pitch = it->v.m_pitch_angle;
			prev_yaw = it->v.m_yaw_angle;
			first = false;
		} while (++it != it_end);

		info->percent_detected = ((float)info->c / (float)TB_MOUSE_MAX_HISTORY) * 100.0f;

		if (info->percent_detected >= 50.0f)
		{
			ProcessDetectionAndTakeAction<Detection_MouseMismatch::data_type>(Detection_MouseMismatch(), info, ph, this);
			info->m_frames.Reset();
			g_QueryCvarProvider.StartQueryCvarValue(ph->GetEdict(), "m_pitch");
			info->m_pitch_set = false;
			info->m_pitch_asked = true;
		}
	}
}

void MouseTester::ProcessPitchConVar(PlayerHandler::iterator ph, char const * val)
{
	MouseDetectInfo * const pInfo(GetPlayerDataStructByIndex(ph.GetIndex()));

	double m_pitch_val = atof(val);
	pInfo->m_inverted = std::signbit(m_pitch_val);
	pInfo->m_pitch_set = true;
	pInfo->m_pitch_asked = false;
}

MouseTester g_MouseTester;

basic_string Detection_MouseMismatch::GetDataDump()
{
	MouseDetectInfo* const p(GetDataStruct());

	basic_string m(Helpers::format(
		":::: MouseDetectInfo {\n"
		":::::::: Percent Detected : %f,\n"
		":::::::: Min consecutive X : %d,\n"
		":::::::: Min consecutive Y : %d,\n"
		":::::::: Mouse Inverted ? : %d,\n"
		":::::::: Showing 64 Frames {\n",
		p->percent_detected,
		p->min_consecutive_x,
		p->min_consecutive_y,
		p->m_inverted
	));
	
	tb_mouse::inner_type mouse_graph[TB_MOUSE_MAX_HISTORY];
	size_t amount;
	p->m_frames.CopyHistory(mouse_graph, amount, TB_MOUSE_MAX_HISTORY);

	tb_mouse::inner_type* it(mouse_graph);
	tb_mouse::inner_type const * const it_end(mouse_graph + TB_MOUSE_MAX_HISTORY);
	do
	{
		m.append(Helpers::format(
			":::::::::::: Time %f : %f Angle X, %d Mouse Y Delta, %f Angle Y, %d Mouse X Delta, 0x%X buttons, %d frozen, %d status\n",
			it->t, it->v.m_pitch_angle, it->v.m_y_mouse, it->v.m_yaw_angle, it->v.m_x_mouse, it->v.m_buttons, it->v.m_frozen, it->v.m_status
		));
	} while (++it != it_end);

	m.append(":::::::: }\n:::: }\n");

	return m;
}
