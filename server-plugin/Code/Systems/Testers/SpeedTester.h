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

#ifndef SPEEDTESTER_H
#define SPEEDTESTER_H

#include <cmath>

#include "Interfaces/InterfacesProxy.h"

#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Systems/BaseSystem.h"
#include "Systems/OnTickListener.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Misc/temp_singleton.h"

/////////////////////////////////////////////////////////////////////////
// SpeedTester
/////////////////////////////////////////////////////////////////////////

typedef struct SpeedHolder
{
	float ticksLeft;
	size_t detections;
	float lastDetectionTime;
	float previousLatency;
	float lastTest;

	SpeedHolder()
	{
		if (SourceSdk::InterfacesProxy::_vfptr_GetTickInterval)
			ticksLeft = ceil((1.0f / SourceSdk::InterfacesProxy::Call_GetTickInterval()) * 2.0f);
		else
			ticksLeft = 0;
		detections = 0;
		lastDetectionTime = previousLatency = lastTest = 0.0;
	};
	SpeedHolder(const SpeedHolder& other)
	{
		memset(this, 0, sizeof(SpeedHolder));
	};
} SpeedHolderT;

class Detection_SpeedHack : public LogDetection<SpeedHolderT>
{
	typedef LogDetection<SpeedHolderT> hClass;
public:
	Detection_SpeedHack() : hClass() {};
	~Detection_SpeedHack(){};

	virtual basic_string GetDataDump();
	virtual basic_string GetDetectionLogMessage()
	{
		return "SpeedHack";
	};
};

class SpeedTester :
	public BaseSystem,
	public OnTickListener,
	public PlayerRunCommandHookListener,
	public PlayerDataStructHandler<SpeedHolderT>,
	public Singleton<SpeedTester>
{
	typedef PlayerDataStructHandler<SpeedHolderT> playerdata_class;
	typedef Singleton<SpeedTester> singleton_class;

public:
	SpeedTester();
	~SpeedTester();

	void Init();
	void Load();
	void Unload();
	PlayerRunCommandRet PlayerRunCommandCallback(NczPlayer* player, void* cmd, void* old_cmd);
	void ProcessPlayerTestOnTick(NczPlayer* player);
	void ProcessOnTick(){};
};

#endif // SPEEDTESTER_H
