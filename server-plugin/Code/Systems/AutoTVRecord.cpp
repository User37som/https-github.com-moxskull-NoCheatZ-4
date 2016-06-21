#include "AutoTVRecord.h"

#include "Interfaces/InterfacesProxy.h"

#include "Players/NczPlayerManager.h"
#include "Logger.h"

AutoTVRecord::AutoTVRecord() :
	BaseSystem("AutoTVRecord", PLAYER_CONNECTED, INVALID, STATUS_EQUAL_OR_BETTER, "Enable - Disable - Verbose - SetMinPlayers - SetPrefix"),
	ConCommandHookListener(),
	singleton_class(),
	m_demofile(""),
	m_prefix("NoCheatZ-autorecords/"),
	m_recordtickcount(0),
	m_waitfortv_time(0.0f),
	m_minplayers(1),
	m_recording(false),
	m_expectedtvconfigchange(false),
	m_spawn_once(true)
{
}

AutoTVRecord::~AutoTVRecord()
{
	Unload();
}

void AutoTVRecord::Init()
{
}

void AutoTVRecord::Load()
{
	m_demofile = "";
	m_recording = false;

	m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_record"));
	m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_stoprecord"));
	//m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_delay"));
	ConCommandHookListener::RegisterConCommandHookListener(this);

	m_waitfortv_time = Plat_FloatTime() + 5.0f;
}

void AutoTVRecord::Unload()
{
	StopRecord();

	m_demofile = "";
	m_recording = false;

	ConCommandHookListener::RemoveConCommandHookListener(this);
}

void AutoTVRecord::StartRecord()
{
	if (m_recording) return;

	if (IsTVPresent())
	{
		m_recordtickcount = 0;
		basic_string mapname;
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			mapname = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
		}
		else
		{
			mapname = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
		}

		size_t const strip = mapname.find_last_of("/\\");
		if (strip != basic_string::npos) mapname = mapname.c_str() + strip + 1;

		mapname.replace(":?\"<>|", '-');

		SourceSdk::InterfacesProxy::Call_ServerExecute();

		m_expectedtvconfigchange = true;

		m_demofile = Helpers::format("%s%s-%s", m_prefix.c_str(), mapname.c_str(), basic_string(Helpers::getStrDateTime("%x_%X")).replace("/\\:?\"<>|", '-').c_str());
		Logger::GetInstance()->Msg<MSG_LOG>(Helpers::format("Starting to record the game in %s.dem", m_demofile.c_str()));

		SourceSdk::InterfacesProxy::Call_ServerCommand(basic_string("tv_record ").append(m_demofile).append('\n').c_str());
		SourceSdk::InterfacesProxy::Call_ServerExecute();
		m_recording = true;

		m_expectedtvconfigchange = false;
	}
}

void AutoTVRecord::StopRecord()
{
	if (!m_recording) return;

	SourceSdk::InterfacesProxy::Call_ServerExecute();

	m_expectedtvconfigchange = true;

	SourceSdk::InterfacesProxy::Call_ServerCommand("tv_stoprecord\n");
	SourceSdk::InterfacesProxy::Call_ServerExecute();
	m_recording = false;
	Logger::GetInstance()->Msg<MSG_LOG>(Helpers::format("TV record ended in %s.dem with %u ticks (%f seconds)", m_demofile.c_str(), m_recordtickcount, m_recordtickcount * SourceSdk::InterfacesProxy::Call_GetTickInterval()));

	m_expectedtvconfigchange = false;
}

void AutoTVRecord::OnTick()
{
	++m_recordtickcount;
}

void AutoTVRecord::SetMinPlayers(int min)
{
	m_minplayers = min;
}

void AutoTVRecord::SetRecordPrefix(basic_string const & prefix)
{
	m_prefix = prefix;
}

size_t AutoTVRecord::GetRecordTick() const
{
	return m_recordtickcount;
}

bool AutoTVRecord::IsRecording() const
{
	return m_recording;
}

bool AutoTVRecord::IsTVPresent() const
{
	for(PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if (it == BOT)
		{
			if (it->GetPlayerInfo()->IsHLTV())
				return true;
		}
	}
	return false;
}

void AutoTVRecord::SpawnTV()
{
	SourceSdk::InterfacesProxy::Call_ServerExecute();

	m_expectedtvconfigchange = true;

	SourceSdk::InterfacesProxy::Call_ServerCommand("tv_autorecord 0\n");
	SourceSdk::InterfacesProxy::Call_ServerCommand("tv_enable 1\n");

	m_expectedtvconfigchange = false;

	if (!IsTVPresent())
	{
		if (m_spawn_once)
		{
			Logger::GetInstance()->Msg<MSG_LOG>("TV not detected. Reloading the map ...");

			basic_string mapname;
			if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
			{
				mapname = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
				size_t const strip = mapname.find_last_of("/\\");
				if (strip != basic_string::npos) mapname = mapname.c_str() + strip + 1;
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("map %s\n", mapname.c_str()));
			}
			else
			{
				mapname = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("changelevel %s\n", mapname.c_str()));
			}

			m_spawn_once = false;
		}
		else
		{
			Logger::GetInstance()->Msg<MSG_ERROR>("Was unable to spawn the TV.");
		}
	}
}

basic_string const & AutoTVRecord::GetRecordFilename() const
{
	return m_demofile;
}

void AutoTVRecord::SendTVChatMessage(basic_string const & msg)
{
	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if (it == BOT)
		{
			if (it->GetPlayerInfo()->IsHLTV())
				Helpers::tell(it->GetEdict(), msg);
			// We don't break because it seems we can have multiple GOTV instances ... I don't know if it means multiple entities though ...
		}
	}
}

bool AutoTVRecord::ConCommandCallback(PlayerHandler::const_iterator ph, void * cmd, const SourceSdk::CCommand & args)
{
	if (!m_expectedtvconfigchange || ph != PlayerHandler::end())
	{
		Logger::GetInstance()->Msg<MSG_LOG>("Intercepted unexpected TV configuration change.");
		return true;
	}
	return false;
}
