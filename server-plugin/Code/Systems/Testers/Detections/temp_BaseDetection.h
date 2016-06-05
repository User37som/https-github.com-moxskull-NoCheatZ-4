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
#include "Systems/BaseSystem.h"

class BaseDetection
{
public:
	BaseDetection(){};
	virtual ~BaseDetection(){};
};

template <typename playerDataStructT>
class SubDetection : public BaseDetection
{
public:
	SubDetection()
	{
		this->m_timestamp = 0.0;
		this->m_tick = 0;
		this->m_dataStruct = playerDataStructT();
	};
	virtual ~SubDetection() override {};

	void PrepareDetectionData(playerDataStructT* dataStruct)
	{
		this->m_timestamp = Plat_FloatTime();
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			this->m_tick = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
		}
		else
		{
			this->m_tick = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
		}
		this->m_dataStruct = *dataStruct;
	};

	playerDataStructT* GetDataStruct() const
	{
		return (playerDataStructT*)(&(this->m_dataStruct));
	};

	virtual basic_string GetDataDump(){return "";};

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
	LogDetection() : BaseClass()
	{
		this->m_testerName = nullptr;
	};
	virtual ~LogDetection() override {};

	virtual basic_string GetDetectionLogMessage(){return "";};

	virtual void Log()
	{
		basic_string msg;
		msg = Helpers::format("%s triggered a detection : %s is using a %s.", this->m_testerName, this->m_playerIdentity.c_str(), this->GetDetectionLogMessage().c_str()); 
		Helpers::writeToLogfile(msg);
		msg = Helpers::format("[" NCZ_PLUGIN_NAME "] %s\0", msg.c_str());
		SourceSdk::InterfacesProxy::Call_LogPrint(msg.c_str());
		Helpers::chatprintf(msg);

		Helpers::writeToLogfile(this->GetDataDump());

	};

	void PrepareDetectionLog(NczPlayer* player, BaseSystem* tester)
	{
		m_testerName = tester->GetName();
		
		m_playerIdentity = player->GetReadableIdentity();

	};

protected:
	const char * m_testerName;
	basic_string m_playerIdentity;
};

#endif
