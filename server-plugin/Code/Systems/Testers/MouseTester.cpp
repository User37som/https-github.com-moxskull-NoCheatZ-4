#include "MouseTester.h"

MouseTester::MouseTester() :
	BaseTesterSystem ("MouseTester"),
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
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, SystemPriority::MouseTester);
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

	if (pInfo->m_prev_set)
	{

		/// Almost fully reverse MouseMove.
		// This is simple actually.
		// All we have to do is to backward-calc MouseMove and the result should be equal to previous angles. If not, detect.

		/* We start by an engine error ...

		CInput::ApplyMouse   cmd->mousedx = (int)mouse_x; cmd->mousedy = (int)mouse_y; // truncated, already introducing 0/+1 error range. Thanks valve.

		*/
		
		SourceSdk::QAngle expect_angle_min;
		SourceSdk::QAngle expect_angle_max;

		float mdx_min = md[0];
		float mdx_max = (float)md[0] + copysign(1.01f, mdx_min);
		float mdy_min = md[1];
		float mdy_max = (float)md[1] + copysign(1.01f, mdy_min);

		// CInput::ApplyMouse viewangles[PITCH] += m_pitch->GetFloat() * mouse_y;

		float pitch_min = pInfo->m_prev_pitch_angle + pInfo->m_mouse_pitch * mdy_min;
		float pitch_max = pInfo->m_prev_pitch_angle + pInfo->m_mouse_pitch * mdy_max;
		//float pitch_delta = fabsf(userangles->x - pInfo->m_prev_pitch_angle);

		if (signbit(mdx_min))
		{
			std::swap(pitch_min, pitch_max);
		}

		if (userangles->x < pitch_min)
		{
			DebugMessage(Helpers::format("PITCH DETECTED MIN %f", (pitch_min - userangles->x) / 0.022f));
		}

		if (userangles->x > pitch_max)
		{
			DebugMessage(Helpers::format("PITCH DETECTED MAX %f", (userangles->x - pitch_max) / 0.022f));
		}

		// CInput::ApplyMouse  viewangles[YAW] -= m_yaw.GetFloat() * mouse_x;

		expect_angle_min.y = userangles->y + pInfo->m_mouse_yaw * mdx_min;
		expect_angle_max.y = userangles->y + pInfo->m_mouse_yaw * mdx_max;

		fmodf(expect_angle_min.y, 180.0f);
		fmodf(expect_angle_max.y, 180.0f);

		float y_delta = userangles->y - pInfo->m_prev_yaw_angle;
		float expect_delta_min = userangles->y - expect_angle_min.y;
		float expect_delta_max = userangles->y - expect_angle_max.y;
		if (y_delta >= expect_delta_max || y_delta < expect_delta_min)
		{
			//DebugMessage("YAW DETECTED");
		}
	}

	pInfo->m_prev_pitch_angle = userangles->x;
	pInfo->m_prev_yaw_angle = userangles->y;
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
