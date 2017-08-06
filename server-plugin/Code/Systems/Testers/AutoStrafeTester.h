/*
	Copyright 2017 -	SM91337 @ fragdeluxe.com
						Le Padellec Sylvain

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

#ifndef AUTOSTRAFETESTER_H
#define AUTOSTRAFETESTER_H

#include "Systems/BaseSystem.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"

struct StrafeDetectInfo
{
	double m_next_detect_time;
	SourceSdk::Vector m_vel;
	int m_buttons;
	size_t m_detect_count;

	StrafeDetectInfo() :
		m_next_detect_time(0.0),
		m_vel(),
		m_buttons(0),
		m_detect_count(0)
	{}

	StrafeDetectInfo(StrafeDetectInfo const & other)
	{
		m_next_detect_time = other.m_next_detect_time;
		SourceSdk::VectorCopy(other.m_vel, m_vel);
		m_buttons = other.m_buttons;
		m_detect_count = other.m_detect_count;
	}
};

class AutoStrafeDetection :
	public LogDetection<StrafeDetectInfo>
{
	typedef LogDetection<StrafeDetectInfo> hClass;

public:
	virtual basic_string GetDetectionLogMessage() override final
	{
		return "auto strafe";
	};

	virtual basic_string GetDataDump() override final;
};

class AutoStrafeTester :
	public BaseTesterSystem,
	public PlayerDataStructHandler<StrafeDetectInfo>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<StrafeDetectInfo> playerdatahandler_class;

public:
	AutoStrafeTester();
	virtual ~AutoStrafeTester() final;

	virtual void Init() override final;

	virtual void Load() override final;

	virtual void Unload() override final;

	virtual bool GotJob() const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime) override final;
};

extern AutoStrafeTester g_AutoStrafeTester;

#endif // AUTOSTRAFETESTER_H

