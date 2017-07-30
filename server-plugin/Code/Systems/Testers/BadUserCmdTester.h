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

#ifndef BADUSERCMDBLOCKER_H
#define BADUSERCMDBLOCKER_H

#include "Systems/BaseSystem.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Misc/temp_singleton.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"

typedef enum TickStatus : int
{
	OK = 0,
	IN_RESET,
	RESET
} TickStatus_t;

struct UserCmdInfo
{
	TickStatus_t m_tick_status;
	bool m_prev_dead;
	double m_detected_time;
	int prev_cmd;
	int prev_tick;
	int cmd_offset;

	UserCmdInfo ()
	{
		m_prev_dead = true;
		m_detected_time = 0.0;
		prev_cmd = -1;
		prev_tick = -1;
		m_tick_status = TickStatus_t::OK;
		cmd_offset = 1;
	};
	UserCmdInfo ( const UserCmdInfo& other )
	{
		memcpy(this, &other, sizeof(UserCmdInfo));
	};
};

struct BadCmdInfo
{
	UserCmdInfo m_inner;
	SourceSdk::CUserCmd_csgo m_current_cmd;

	BadCmdInfo()
	{
		// not needed
	}
	BadCmdInfo(UserCmdInfo const * inner, SourceSdk::CUserCmd_csgo const * cmd)
	{
		memcpy(&m_inner, inner, sizeof(UserCmdInfo));
		memcpy(&m_current_cmd, cmd, sizeof(SourceSdk::CUserCmd_csgo));
	}
};

class Detection_BadUserCmd :
	public LogDetection<BadCmdInfo>
{
	typedef LogDetection<BadCmdInfo> hClass;

public:
	virtual basic_string GetDetectionLogMessage() override final
	{
		return "Tempered UserCmd";
	};

	virtual basic_string GetDataDump() override final;
};

class BadUserCmdTester :
	public BaseTesterSystem,
	public PlayerDataStructHandler<UserCmdInfo>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<UserCmdInfo> playerdatahandler_class;

public:
	BadUserCmdTester();
	virtual ~BadUserCmdTester() final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void * const cmd, double const & curtime ) override final;
};

extern BadUserCmdTester g_BadUserCmdTester;

#endif // BADUSERCMDBLOCKER_H
