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

#ifndef EYEANGLESTESTER
#define EYEANGLESTESTER

#include "Systems/BaseSystem.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Players/NczFilteredPlayersList.h"
#include "Misc/temp_singleton.h"

typedef struct EyeAngle
{
	float value; // Raw value of the angle
	float abs_value; // Abs value so it's easier to test

	float lastDetectionPrintTime;
	unsigned int detectionsCount;

	EyeAngle()
	{
		value = abs_value = 0.0;
		lastDetectionPrintTime = 0.0;
		detectionsCount = 0;
	};
	EyeAngle(const EyeAngle& other)
	{
		value = other.value;
		abs_value = other.abs_value;
		lastDetectionPrintTime = other.lastDetectionPrintTime;
		detectionsCount = other.detectionsCount;
	};
} EyeAngleT;

typedef struct EyeAngleInfo
{
	unsigned int ignore_last; // Ignore values potentially not initialized by the engine

	EyeAngleT x;
	EyeAngleT y;
	EyeAngleT z;

	EyeAngleInfo()
	{
		ignore_last = 0;
		x = y = z = EyeAngle();
	};
	EyeAngleInfo(const EyeAngleInfo& other)
	{
		ignore_last = other.ignore_last;
		x = other.x;
		y = other.y;
		z = other.z;
	};
} EyeAngleInfoT;

class Detection_EyeAngle : public LogDetection<EyeAngleInfoT>
{
	typedef LogDetection<EyeAngleInfoT> hClass;
public:
	Detection_EyeAngle() : hClass() {};
	virtual ~Detection_EyeAngle(){};

	virtual basic_string GetDataDump();
};

class Detection_EyeAngleX : public Detection_EyeAngle
{
public:
	Detection_EyeAngleX(){};
	~Detection_EyeAngleX(){};

	virtual basic_string GetDetectionLogMessage();
};

class Detection_EyeAngleY : public Detection_EyeAngle
{
public:
	Detection_EyeAngleY(){};
	~Detection_EyeAngleY(){};

	virtual basic_string GetDetectionLogMessage();
};

class Detection_EyeAngleZ : public Detection_EyeAngle
{
public:
	Detection_EyeAngleZ(){};
	~Detection_EyeAngleZ(){};

	virtual basic_string GetDetectionLogMessage();
};

class EyeAnglesTester :
	public BaseSystem,
	public PlayerRunCommandHookListener,
	public PlayerDataStructHandler<EyeAngleInfoT>,
	public SourceSdk::IGameEventListener002,
	public Singleton<EyeAnglesTester>
{
	typedef Singleton<EyeAnglesTester> singleton_class;
	typedef PlayerDataStructHandler<EyeAngleInfoT> playerdata_class;

public:
	EyeAnglesTester();
	~EyeAnglesTester();

	void Init();
	void Load();
	void Unload();
	SlotStatus GetFilter();
	PlayerRunCommandRet PlayerRunCommandCallback(NczPlayer* player, void* cmd, void* old_cmd);
	void FireGameEvent(SourceSdk::IGameEvent *ev);
	void TeleportCallback(NczPlayer* player, SourceSdk::Vector const* va, SourceSdk::QAngle const* qa, SourceSdk::Vector const* vb);
};

#endif
