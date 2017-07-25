#include "MouseTester.h"

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
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it(&filter_class);
	// Return if we have job to do or not ...
	return it != PlayerHandler::end();
}

PlayerRunCommandRet MouseTester::RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, void * const old_cmd)
{
	if (!IsActive())
		return PlayerRunCommandRet::CONTINUE;

	short * md(nullptr);
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		md = &(reinterpret_cast<SourceSdk::CUserCmd_csgo*>(cmd)->mousedx);
	}
	else
	{
		md = &(reinterpret_cast<SourceSdk::CUserCmd*>(cmd)->mousedx);
	}

	MouseInfo * const pInfo(GetPlayerDataStructByIndex(ph.GetIndex()));
	SourceSdk::QAngle const * userangles(&(reinterpret_cast<SourceSdk::CUserCmd*>(cmd)->viewangles));

	if (pInfo->m_prev_set && ph == SlotStatus::PLAYER_IN_TESTS)
	{
		/// PITCH

		float pitch_delta = userangles->x - pInfo->m_prev_pitch_angle;

		if (signbit(pitch_delta) != signbit((float)md[1]) && pitch_delta != 0.0f && md[1] != 0)
		{
			// frozen mouse move false detection
			// inverted mouse false detection
			DebugMessage("DETECTED");
		}
		else if (md[1] == 0 && fabs(pitch_delta) > 0.044f * fabs(pInfo->m_prev_mdy) + 0.044f)
		{
			// sniper scope false detection
 			DebugMessage("DETECTED");
		}

		/// YAW
	}

	pInfo->m_prev_pitch_angle = userangles->x;
	pInfo->m_prev_yaw_angle = userangles->y;
	pInfo->m_prev_mdx = md[0];
	pInfo->m_prev_mdy = md[1];
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
