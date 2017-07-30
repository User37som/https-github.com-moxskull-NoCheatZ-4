#include "AimTester.h"

AimTester::AimTester() :
	BaseTesterSystem("AimTester"),
	SourceSdk::IGameEventListener002(),
	PlayerDataStructHandler<data_throwback_t>(),
	PlayerRunCommandHookListener(),
	Singleton()
{

}

AimTester::~AimTester()
{

}

void AimTester::Init()
{
	InitDataStruct();
}

void AimTester::Load()
{
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this);
}

void AimTester::Unload()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener(this);
}

bool AimTester::GotJob() const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it(&filter_class);
	// Return if we have job to do or not ...
	return it != PlayerHandler::end();
}

void AimTester::FireGameEvent(SourceSdk::IGameEvent * ev)
{

}

PlayerRunCommandRet AimTester::RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime)
{


	return PlayerRunCommandRet::CONTINUE;
}
