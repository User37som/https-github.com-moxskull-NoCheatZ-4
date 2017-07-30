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
#include "Misc/temp_singleton.h"

/////////////////////////////////////////////////////////////////////////
// JumpTester
/////////////////////////////////////////////////////////////////////////

typedef struct OnGroundHolder
{
	int onGround_Tick;
	int notOnGround_Tick;

	int jumpCount;

	OnGroundHolder ()
	{
		memset ( this, 0, sizeof ( OnGroundHolder ) );
	};
	OnGroundHolder ( const OnGroundHolder& other )
	{
		memcpy ( this, &other, sizeof ( OnGroundHolder ) );
	};
} OnGroundHolderT;

typedef struct JumpCmdHolder
{
	bool lastJumpCmdState;

	int JumpDown_Tick;
	int JumpUp_Tick;

	int outsideJumpCmdCount; // Jumps made while the player doesn't touch the ground

	JumpCmdHolder ()
	{
		memset ( this, 0, sizeof ( JumpCmdHolder ) );
	};
	JumpCmdHolder ( const JumpCmdHolder& other )
	{
		memcpy ( this, &other, sizeof ( JumpCmdHolder ) );
	};
} JumpCmdHolderT;

typedef struct JumpInfo
{
	OnGroundHolderT onGroundHolder;
	JumpCmdHolderT jumpCmdHolder;
	int total_bhopCount;
	int total_outside_jump;

	int goodBhopsCount;
	float perfectBhopsPercent;
	float totaloutsidepercent;
	int perfectBhopsCount;

	bool isOnGround;
	bool prev_jump;

	JumpInfo ()
	{
		memset ( this, 0, sizeof ( JumpInfo ) );
	};
	JumpInfo ( const JumpInfo& other )
	{
		memcpy ( this, &other, sizeof ( JumpInfo ) );
	};
} JumpInfoT;

class Detection_BunnyHopScript : public LogDetection<JumpInfoT>
{
	typedef LogDetection<JumpInfoT> hClass;
public:
	Detection_BunnyHopScript () : hClass ()
	{};
	virtual ~Detection_BunnyHopScript () override
	{};

	virtual basic_string GetDataDump () final;
	virtual basic_string GetDetectionLogMessage ()
	{
		return "BunnyHop Script";
	};
};

class Detection_BunnyHopProgram : public Detection_BunnyHopScript
{
public:
	Detection_BunnyHopProgram () : Detection_BunnyHopScript ()
	{};
	virtual ~Detection_BunnyHopProgram () override final
	{};

	virtual basic_string GetDetectionLogMessage () override final
	{
		return "BunnyHop Injected";
	};
};

class JumpTester :
	public BaseTesterSystem,
	public OnGroundHookListener,
	public PlayerDataStructHandler<JumpInfoT>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<JumpInfoT> playerdata_class;

private:
	void * convar_sv_enablebunnyhopping;
	void * convar_sv_autobunnyhopping;
	bool detect_scripts;

public:
	JumpTester ();
	virtual ~JumpTester () final;

	virtual void Init () override final;

	virtual bool sys_cmd_fn(const SourceSdk::CCommand & args);

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void RT_m_hGroundEntityStateChangedCallback ( PlayerHandler::iterator ph, bool const new_isOnGround ) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void * const cmd, double const & curtime) override final;

	void OnPlayerTouchGround ( PlayerHandler::iterator ph, int game_tick );

	void OnPlayerLeaveGround ( PlayerHandler::iterator ph, int game_tick );

	void OnPlayerJumpButtonDown ( PlayerHandler::iterator ph, int game_tick );

	void OnPlayerJumpButtonUp ( PlayerHandler::iterator ph, int game_tick );
};

extern JumpTester g_JumpTester;

#endif // JUMPTESTER_H
