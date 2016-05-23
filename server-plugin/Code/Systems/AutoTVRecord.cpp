#include "AutoTVRecord.h"

#include "Players/NczPlayerManager.h"
#include "Logger.h"

AutoTVRecord::AutoTVRecord() :
	BaseSystem("AutoTVRecord", PLAYER_CONNECTED, INVALID, STATUS_EQUAL_OR_BETTER, "Enable - Disable - Verbose - SetMinPlayers - SetPrefix"),
	ConCommandHookListener(),
	singleton_class()
{
	m_expectedtvconfigchange = false;
}

AutoTVRecord::~AutoTVRecord()
{
	Unload();
}

void AutoTVRecord::Init()
{
	m_prefix = "NoCheatZ-autorecord";
	m_demofile = "";
	m_tvslot = 0;
	m_minplayers = 1;
	m_recording = false;
}

void AutoTVRecord::Load()
{
	m_demofile = "";
	m_tvslot = 0;
	m_recording = false;

	m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_record"));
	m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_stoprecord"));
	//m_mycommands.AddToTail(SourceSdk::InterfacesProxy::ICvar_FindCommand("tv_delay"));
	ConCommandHookListener::RegisterConCommandHookListener(this);

	m_waitfortv_time = Plat_FloatTime() + 5.0f;

	//SpawnTV();
}

void AutoTVRecord::Unload()
{
	m_demofile = "";
	m_tvslot = 0;
	m_recording = false;

	ConCommandHookListener::RemoveConCommandHookListener(this);
}

void AutoTVRecord::StartRecord()
{
	if (m_recording) return;

	if (GetSlot() == 0)
	{
		SpawnTV();
	}
	else
	{
		m_recordtickcount = 0;
		const char * mapname;
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			mapname = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
		}
		else
		{
			mapname = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
		}

		SourceSdk::InterfacesProxy::Call_ServerExecute();

		m_expectedtvconfigchange = true;

		m_demofile = Helpers::format("%s-%s-%s", m_prefix.c_str(), mapname, Helpers::getStrDateTime("%x_%X").replace('/', '-').c_str());
		Logger::GetInstance()->Msg<MSG_LOG>(Helpers::format("Starting to record the game in %s.dem", m_demofile));

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
	if (GetSlot() == 0)
	{
		if (Plat_FloatTime() > m_waitfortv_time)
		{
			SpawnTV();
			m_waitfortv_time = std::numeric_limits<float>::max();
		}
	}
	else
	{
		int player_count = 0;
		int x = 1;
		int maxcl = NczPlayerManager::GetInstance()->GetMaxIndex();
		PlayerHandler * ph;
		do
		{
			ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(x);
			if (ph->status >= PLAYER_CONNECTED)
			{
				SourceSdk::IPlayerInfo* pinfo = static_cast<SourceSdk::IPlayerInfo*>(ph->playerClass->GetPlayerInfo());
				if (pinfo != nullptr)
				{
					if (pinfo->GetTeamIndex() > 1)
					{
						++player_count;
					}
				}
			}
		} while (++x <= maxcl);

		if (player_count >= m_minplayers)
		{
			if (!m_recording)
			{
				StartRecord();
			}
			else
			{
				++m_recordtickcount;
			}
		}
		else
		{
			if (m_recording)
			{
				StopRecord();
			}
		}
	}
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

int AutoTVRecord::GetSlot()
{
	if (m_tvslot == 0)
	{
		size_t const maxcl = NczPlayerManager::GetInstance()->GetMaxIndex();
		for (size_t index = 1; index <= maxcl; ++index)
		{
			SourceSdk::edict_t * pEntity = Helpers::PEntityOfEntIndex(index);
			if (Helpers::isValidEdict(pEntity))
			{
				void* player = SourceSdk::InterfacesProxy::Call_GetPlayerInfo(pEntity);
				if (player)
				{
					bool ishltv;
					if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
					{
						ishltv = static_cast<SourceSdk::IPlayerInfo_csgo*>(player)->IsHLTV();
					}
					else
					{
						ishltv = static_cast<SourceSdk::IPlayerInfo*>(player)->IsHLTV();
					}
					if (ishltv)
					{
						m_tvslot = index;
						break;
					}
				}
			}
		}
	}
	return m_tvslot;
}

void AutoTVRecord::SpawnTV()
{
	SourceSdk::InterfacesProxy::Call_ServerExecute();

	m_expectedtvconfigchange = true;

	SourceSdk::InterfacesProxy::Call_ServerCommand("tv_autorecord 0\n");
	SourceSdk::InterfacesProxy::Call_ServerCommand("tv_enable 1\n");

	SourceSdk::InterfacesProxy::Call_ServerExecute();

	m_expectedtvconfigchange = false;

	if (GetSlot() == 0)
	{
		Logger::GetInstance()->Msg<MSG_LOG>("SourceTV not detected. Reloading the map ...");
		
		const char * mapname;
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			mapname = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
		}
		else
		{
			mapname = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->mapname;
		}

		SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("changelevel %s\n", mapname).c_str());
	}
}

basic_string const & AutoTVRecord::GetRecordFilename() const
{
	return m_demofile;
}

void AutoTVRecord::SendTVChatMessage(basic_string const & msg)
{
	if (GetSlot() > 0)
	{
		Helpers::tell(Helpers::PEntityOfEntIndex(m_tvslot), msg);
	}
}

bool AutoTVRecord::ConCommandCallback(NczPlayer * player, void * cmd, const SourceSdk::CCommand & args)
{
	if (!m_expectedtvconfigchange || player != nullptr)
	{
		Logger::GetInstance()->Msg<MSG_LOG>("Intercepted unexpected SourceTV configuration change.");
		return true;
	}
	return false;
}
