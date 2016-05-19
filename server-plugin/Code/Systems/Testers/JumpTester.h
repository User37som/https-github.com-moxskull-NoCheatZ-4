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

#ifndef JUMPTESTER_H
#define JUMPTESTER_H

#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Systems/BaseSystem.h"
#include "Hooks/OnGroundHookListener.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"

/////////////////////////////////////////////////////////////////////////
// JumpTester
/////////////////////////////////////////////////////////////////////////

typedef struct OnGroundHolder
{
	int onGround_Tick;
	int notOnGround_Tick;

	int jumpCount;

	OnGroundHolder()
	{
		onGround_Tick = notOnGround_Tick = jumpCount = 0;
	};
	OnGroundHolder(const OnGroundHolder& other)
	{
		onGround_Tick = other.onGround_Tick;
		notOnGround_Tick = other.notOnGround_Tick;
		jumpCount = other.jumpCount;
	};
} OnGroundHolderT;

typedef struct JumpCmdHolder
{
	bool lastJumpCmdState;

	int JumpDown_Tick;
	int JumpUp_Tick;

	int outsideJumpCmdCount; // Jumps made while the player doesn't touch the ground

	JumpCmdHolder()
	{
		lastJumpCmdState = false;
		JumpDown_Tick = JumpUp_Tick = outsideJumpCmdCount = 0;
	};
	JumpCmdHolder(const JumpCmdHolder& other)
	{
		lastJumpCmdState = other.lastJumpCmdState;
		JumpDown_Tick = other.JumpDown_Tick;
		JumpUp_Tick = other.JumpUp_Tick;
		outsideJumpCmdCount = other.outsideJumpCmdCount;
	};
} JumpCmdHolderT;

typedef struct JumpInfo
{
	OnGroundHolderT onGroundHolder;
	JumpCmdHolderT jumpCmdHolder;
	int total_bhopCount;

	int goodBhopsCount;
	int perfectBhopsPercent;
	int perfectBhopsCount;

	bool isOnGround;

	JumpInfo()
	{
		onGroundHolder = OnGroundHolder();
		jumpCmdHolder = JumpCmdHolder();
		total_bhopCount = goodBhopsCount = perfectBhopsPercent = perfectBhopsCount = 0;
		isOnGround = false;
	};
	JumpInfo(const JumpInfo& other)
	{
		onGroundHolder = other.onGroundHolder;
		jumpCmdHolder = other.jumpCmdHolder;
		total_bhopCount = other.total_bhopCount;
		goodBhopsCount = other.goodBhopsCount;
		perfectBhopsPercent = other.perfectBhopsPercent;
		perfectBhopsCount = other.perfectBhopsCount;
		isOnGround = other.isOnGround;
	};
} JumpInfoT;

class Detection_BunnyHopScript : public LogDetection<JumpInfoT>
{
	typedef LogDetection<JumpInfoT> hClass;
public:
	Detection_BunnyHopScript() : hClass() {};
	~Detection_BunnyHopScript(){};

	virtual basic_string GetDataDump();
	virtual basic_string GetDetectionLogMessage()
	{
		return "BunnyHop Script";
	};
};

class Detection_BunnyHopProgram : public Detection_BunnyHopScript
{
public:
	Detection_BunnyHopProgram() : Detection_BunnyHopScript() {};
	~Detection_BunnyHopProgram(){};

	basic_string GetDetectionLogMessage()
	{
		return "BunnyHop Program";
	};
};

class JumpTester :
	public BaseSystem,
	public OnGroundHookListener,
	public PlayerRunCommandHookListener,
	public PlayerDataStructHandler<JumpInfoT>
{
public:
	JumpTester();
	~JumpTester();

	void Init();
	void Load();
	void Unload();
	PlayerRunCommandRet PlayerRunCommandCallback(NczPlayer* player, SourceSdk::CUserCmd* cmd, const SourceSdk::CUserCmd& old_cmd);
	void m_hGroundEntityStateChangedCallback(NczPlayer* player, bool new_isOnGround);
};

extern JumpTester g_JumpTester;

#endif // JUMPTESTER_H
