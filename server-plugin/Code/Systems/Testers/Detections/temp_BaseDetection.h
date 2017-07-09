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

		this->m_timestamp = Plat_FloatTime ();
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
	float m_timestamp;
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
		basic_string msg ( Helpers::format ( "%s triggered a detection : %s is using a %s.", this->m_testerName, this->m_playerIdentity.c_str (), this->GetDetectionLogMessage ().c_str () ) );
		Helpers::writeToLogfile ( msg );
		char const * text2 ( Helpers::format ( "[" NCZ_PLUGIN_NAME "] %s", msg.c_str () ) );
		SourceSdk::InterfacesProxy::Call_LogPrint ( text2 );

		Logger::GetInstance()->Msg<MSG_CHAT>(text2);

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
inline void ProcessDetectionAndTakeAction(LogDetection<playerDataStructT> && info, playerDataStructT const * const pData, PlayerHandler::iterator const player, BaseTesterSystem const * const pSystem)
{
	info.PrepareDetectionData(pData);
	info.PrepareDetectionLog(*player, pSystem);
	info.Log();

	if (pSystem->GetAction() == BaseTesterSystem::DetectionAction_t::LOG)
	{
		return;
	}
	else if (pSystem->GetAction() == BaseTesterSystem::DetectionAction_t::BAN_ASYNC)
	{
		BanRequest::GetInstance()->AddAsyncBan(*player, 0, Helpers::format("Banned by NoCheatZ 4 : %s detection with %s", info.GetDetectionLogMessage().c_str(), pSystem->GetName()));
	}
	else if (pSystem->GetAction() == BaseTesterSystem::DetectionAction_t::BAN_NOW)
	{
		BanRequest::GetInstance()->BanNow(*player, 0, Helpers::format("Banned by NoCheatZ 4 : %s detection with %s", info.GetDetectionLogMessage().c_str(), pSystem->GetName()));
	}
	else
	{
		BanRequest::GetInstance()->KickNow(*player, Helpers::format("Kicked by NoCheatZ 4 : %s detection with %s", info.GetDetectionLogMessage().c_str(), pSystem->GetName()));
	}
}

#endif
