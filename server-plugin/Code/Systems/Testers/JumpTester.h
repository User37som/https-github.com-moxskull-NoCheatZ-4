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

#include "Systems/Testers/Detections/temp_BaseDetection.h" // + basic_string + memset/cpy + logger + basesystem + singleton + helpers + cutlvector
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

typedef struct JumpInfo : 
	public Helpers::CRC32_Specialize
{
	OnGroundHolderT onGroundHolder;
	JumpCmdHolderT jumpCmdHolder;
	int total_bhopCount;

	int goodBhopsCount;
	int perfectBhopsPercent;
	int perfectBhopsCount;

	bool isOnGround;

	JumpInfo ()
	{
		memset ( this, 0, sizeof ( JumpInfo ) );
	};
	JumpInfo ( const JumpInfo& other )
	{
		memcpy ( this, &other, sizeof ( JumpInfo ) );
	};

	virtual uint32_t Hash_CRC32 () const
	{
		return 0; // No way to differentiate 
	}
} JumpInfoT;

class Base_Detection_BunnyHop : public LogDetection<JumpInfoT>
{
protected:
	typedef LogDetection<JumpInfoT> hClass;
public:
	Base_Detection_BunnyHop ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, uint32_t udid, hClass::data_t const * data ) :
		hClass ( player, tester, udid, data )
	{};
	virtual ~Base_Detection_BunnyHop ()
	{};

	virtual void TakeAction () = 0;

	virtual void WriteXMLOutput ( FILE * const ) const final;

	virtual bool CloneWhenEqual () const final
	{
		return false; // Hash always returns 0
	}

	virtual basic_string GetDetectionLogMessage () const = 0;
};

class Detection_BunnyHopJumpMacro : public Base_Detection_BunnyHop
{
public:
	Detection_BunnyHopJumpMacro ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, hClass::data_t const * data ) :
		Base_Detection_BunnyHop ( player, tester, UniqueDetectionID::BUNNYHOP_JUMPMACRO, data )
	{};
	virtual ~Detection_BunnyHopJumpMacro () override final
	{};

	virtual void TakeAction () override final;

	virtual basic_string GetDetectionLogMessage () const override final
	{
		return "BunnyHop Jump Macro";
	};
};

class Detection_BunnyHopPerfect : public Base_Detection_BunnyHop
{
public:
	Detection_BunnyHopPerfect ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, hClass::data_t const * data ) :
		Base_Detection_BunnyHop ( player, tester, UniqueDetectionID::BUNNYHOP_PERFECT, data )
	{};
	virtual ~Detection_BunnyHopPerfect () override final
	{};

	virtual void TakeAction () override final;

	virtual basic_string GetDetectionLogMessage () const override final
	{
		return "BunnyHop Cheat";
	};
};

class JumpTester :
	public BaseDynamicSystem,
	public OnGroundHookListener,
	public PlayerDataStructHandler<JumpInfoT>,
	public PlayerRunCommandHookListener,
	public Singleton<JumpTester>
{
	typedef Singleton<JumpTester> singleton_class;
	typedef PlayerDataStructHandler<JumpInfoT> playerdata_class;

private:
	void * convar_sv_enablebunnyhopping;
	void * convar_sv_autobunnyhopping;

public:
	JumpTester ();
	virtual ~JumpTester () final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void RT_m_hGroundEntityStateChangedCallback ( PlayerHandler::const_iterator ph, bool const new_isOnGround ) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const cmd, void * const old_cmd ) override final;

	void OnPlayerTouchGround ( PlayerHandler::const_iterator ph, int game_tick );

	void OnPlayerLeaveGround ( PlayerHandler::const_iterator ph, int game_tick );

	void OnPlayerJumpButtonDown ( PlayerHandler::const_iterator ph, int game_tick );

	void OnPlayerJumpButtonUp ( PlayerHandler::const_iterator ph, int game_tick );
};

#endif // JUMPTESTER_H
