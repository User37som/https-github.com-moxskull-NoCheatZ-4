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
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this, SystemPriority::JumpTester);
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
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		SourceSdk::CUserCmd_csgo* test_cmd((SourceSdk::CUserCmd_csgo*)cmd);

		if (test_cmd->mousedx || test_cmd->mousedy)
		{
			DebugMessage("mouse ban");
		}
	}
	else
	{
		// eat more maths here ...
	}
}

basic_string Detection_MouseMismatch::GetDataDump()
{
	return basic_string("not yet");
}
