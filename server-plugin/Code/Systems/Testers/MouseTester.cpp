#include "MouseTester.h"

#include "Misc/EntityProps.h"

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
		if ((*flags & (3 << 5)) == 0)
		{
			/// PITCH

			float pitch_delta = userangles->x - pInfo->m_prev_pitch_angle;

			if (signbit(pitch_delta) != signbit((float)md[1]) && pitch_delta != 0.0f && md[1] != 0)
			{
				// inverted mouse false detection
				DebugMessage("Pitch direction mismatch");
			}
			else if (md[1] == 0 && fabs(pitch_delta) > 0.044f * fabs(pInfo->m_prev_mdy) + 0.044f)
			{
				// sniper scope false detection
				DebugMessage("Pitch angle moved without mouse");
			}

			/// YAW

			if ((*buttons & 3 << 7) == 0) // IN_LEFT | IN_RIGHT
			{
				float yaw_delta = AngleDistance(userangles->y, pInfo->m_prev_yaw_angle); 
				if (signbit(yaw_delta) == signbit((float)md[0]) && yaw_delta != 0.0f && md[0] != 0)
				{
					DebugMessage("Yaw direction mismatch");
				}
				else if (md[0] == 0 && fabs(yaw_delta) > 0.044f * fabs(pInfo->m_prev_mdx) + 0.044f)
				{
					// sniper scope false detection
					DebugMessage("Yaw angle moved without mouse");
				}
			}
		}
	}

	pInfo->m_prev_pitch_angle = userangles->x;
	pInfo->m_prev_yaw_angle = userangles->y;
	pInfo->m_prev_mdx = ((float)md[0] + pInfo->m_prev_mdx) / 2.0f;
	pInfo->m_prev_mdy = ((float)md[1] + pInfo->m_prev_mdy) / 2.0f;
	pInfo->m_prev_set = true;

	return PlayerRunCommandRet::CONTINUE;
}

void MouseTester::OnClientSettingsChanged(PlayerHandler::iterator ph)
{
	MouseInfo * const pInfo(GetPlayerDataStructByIndex(ph.GetIndex()));

	pInfo->m_mouse_pitch = 0.022f;
	pInfo->m_mouse_yaw = 0.022f;
	pInfo->m_prev_set = false;
}

MouseTester g_MouseTester;

basic_string Detection_MouseMismatch::GetDataDump()
{
	return basic_string("not yet");
}
