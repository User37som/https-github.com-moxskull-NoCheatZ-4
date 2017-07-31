/*
	Copyright 2012 - Le Padellec Sylvain

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

#ifndef BASEDETECTION
#define BASEDETECTION

#include "Misc/temp_basicstring.h"

#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h" // + ifaces + preprocessors
#include "Players/NczPlayerManager.h"
#include "Systems/BanRequest.h"
#include "Systems/AutoTVRecord.h"

class BaseDetection :
	public HeapMemoryManager::OverrideNew<16>
{
public:
	BaseDetection ()
	{};
	virtual ~BaseDetection ()
	{};
};

template <typename playerDataStructT>
class SubDetection : public BaseDetection
{
public:
	SubDetection ()
	{
		this->m_timestamp = 0.0;
		this->m_tick = 0;
		this->m_dataStruct = playerDataStructT ();
	};
	virtual ~SubDetection () override
	{};

	void PrepareDetectionData ( playerDataStructT const * const dataStruct )
	{
		static_assert ( std::is_copy_assignable<playerDataStructT>::value, "Data must be copy-assignable" );

		this->m_timestamp = Tier0::Plat_FloatTime ();
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			this->m_tick = static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
		}
		else
		{
			this->m_tick = static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
		}
		this->m_dataStruct = *dataStruct;
	};

	playerDataStructT* GetDataStruct () const
	{
		return ( playerDataStructT* ) ( &( this->m_dataStruct ) );
	};

	virtual basic_string GetDataDump ()
	{
		return "";
	};

protected:
	double m_timestamp;
	int m_tick;
	playerDataStructT m_dataStruct;
};

template <typename playerDataStructT>
class LogDetection : public SubDetection<playerDataStructT>
{
	typedef SubDetection<playerDataStructT> BaseClass;
public:
	typedef playerDataStructT data_type;

	LogDetection () : BaseClass ()
	{
		this->m_testerName = nullptr;
	};
	virtual ~LogDetection () override
	{};

	virtual basic_string GetDetectionLogMessage ()
	{
		return "";
	};

	virtual void Log ()
	{
		basic_string msg ( Helpers::format ( "%s : %s detected", this->m_playerIdentity.c_str (), this->GetDetectionLogMessage ().c_str () ) );
		g_Logger.Msg<MSG_LOG>(msg.c_str());
		g_Logger.Msg<MSG_CHAT>(msg.c_str());

		Helpers::writeToLogfile ( NCZ_VERSION_GIT );
		Helpers::writeToLogfile ( this->GetDataDump () );
	};

	void PrepareDetectionLog ( NczPlayer const * const player, BaseSystem const * const tester )
	{
		m_testerName = tester->GetName ();

		m_playerIdentity = player->GetReadableIdentity ();

	};

protected:
	const char * m_testerName;
	basic_string m_playerIdentity;
};

template <class playerDataStructT>
inline void ProcessDetectionAndTakeAction(LogDetection<playerDataStructT> && info, playerDataStructT const * const pData, PlayerHandler::iterator player, BaseTesterSystem const * const pSystem)
{
	player->SetDetected(true);

	g_AutoTVRecord.DeclareDetectedPlayer();

	info.PrepareDetectionData(pData);
	info.PrepareDetectionLog(*player, pSystem);
	info.Log();

	// https://github.com/L-EARN/NoCheatZ-4/issues/151
	// calling Helpers::format to set string X with string X also in arguments
	basic_string kick_message(Helpers::format("Banned by NoCheatZ 4 : %s", info.GetDetectionLogMessage().c_str()));

	if (pSystem->GetAction() == BaseTesterSystem::DetectionAction_t::LOG)
	{
		return;
	}
	else if (pSystem->GetAction() == BaseTesterSystem::DetectionAction_t::BAN_ASYNC)
	{
		g_BanRequest.AddAsyncBan(*player, 0, kick_message.c_str());
	}
	else if (pSystem->GetAction() == BaseTesterSystem::DetectionAction_t::BAN_NOW)
	{
		g_BanRequest.BanNow(*player, 0, kick_message.c_str());
	}
	else
	{
		kick_message = Helpers::format("Kicked by NoCheatZ 4 : %s", info.GetDetectionLogMessage().c_str());
		g_BanRequest.KickNow(*player, kick_message.c_str());
	}
}

#endif
