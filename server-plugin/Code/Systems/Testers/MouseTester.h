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

#ifndef MOUSETESTER_H
#define MOUSETESTER_H

#include "Systems/BaseSystem.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Misc/temp_singleton.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"

struct MouseInfo
{
	int prev_mdx;
	int prev_mdy;
	int mdx;
	int mdy;

	MouseInfo() :
		prev_mdx(0),
		prev_mdy(0),
		mdx(0),
		mdy(0)
	{
	}

	MouseInfo(const MouseInfo& other) :
		prev_mdx(other.prev_mdx),
		prev_mdy(other.prev_mdy),
		mdx(other.mdx),
		mdy(other.mdy)
	{
	}
};

class Detection_MouseMismatch :
	public LogDetection<MouseInfo>
{
	typedef LogDetection<MouseInfo> hClass;

public:
	virtual basic_string GetDetectionLogMessage() override final
	{
		return "mouse aimbot";
	};

	virtual basic_string GetDataDump() override final;
};

class MouseTester :
	public BaseTesterSystem,
	public PlayerDataStructHandler<MouseInfo>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<MouseInfo> playerdatahandler_class;

public:
	MouseTester();
	virtual ~MouseTester() final;

	virtual void Init() override final;

	virtual void Load() override final;

	virtual void Unload() override final;

	virtual bool GotJob() const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, void * const old_cmd) override final;
};

extern MouseTester g_MouseTester;

#endif // MOUSETESTER_H
