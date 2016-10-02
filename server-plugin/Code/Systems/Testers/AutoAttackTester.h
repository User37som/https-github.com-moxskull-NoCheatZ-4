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

#ifndef AUTOATTACKTESTER_H
#define AUTOATTACKTESTER_H

#include "Systems/BaseSystem.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Misc/temp_singleton.h"
#include "Misc/temp_Throwback.h"

#define SHORT_TIME 0.09 // sec

typedef Throwback_Arithmetic<int, int, 15> tb_int;

struct AttackTriggerStats
{
	int attack1_down_tick;
	int attack2_down_tick;
	int attack1_up_tick;
	int attack2_up_tick;
	tb_int attack1_sustain_stats;
	tb_int attack2_sustain_stats;

	AttackTriggerStats () : attack1_sustain_stats(), attack2_sustain_stats()
	{
	};
	AttackTriggerStats ( const AttackTriggerStats& other )
	{
		memcpy ( this, &other, sizeof ( AttackTriggerStats ) );
	};
};

class AutoAttackTester :
	public BaseDynamicSystem,
	public PlayerDataStructHandler<AttackTriggerStats>,
	public PlayerRunCommandHookListener,
	public Singleton<AutoAttackTester>
{
	typedef PlayerDataStructHandler<AttackTriggerStats> playerdata_class;
	typedef Singleton<AutoAttackTester> singleton_class;

public:
	AutoAttackTester ();

	virtual ~AutoAttackTester () final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const pCmd, void * const old_cmd ) override final;

	void OnAttack1Up ( PlayerHandler::const_iterator ph, int game_tick );

	void OnAttack1Down ( PlayerHandler::const_iterator ph, int game_tick );

	void OnAttack2Up ( PlayerHandler::const_iterator ph, int game_tick );

	void OnAttack2Down ( PlayerHandler::const_iterator ph, int game_tick );
};

class Detection_AutoPistol : public BaseDetection
{
public:
	Detection_AutoPistol () : BaseDetection ()
	{};
	virtual ~Detection_AutoPistol () override final
	{};

	virtual basic_string GetDetectionLogMessage () final
	{
		return "AutoPistol";
	};
};

#endif // AUTOATTACKTESTER_H
