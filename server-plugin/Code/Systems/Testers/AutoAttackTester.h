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

#define SHORT_TIME (float)(0.055) // sec

#define TB_MAX_HISTORY 15

typedef Throwback_Arithmetic<int, int, TB_MAX_HISTORY> tb_int;

struct AttackTriggerStats
{
	int prev_buttons;
	int attack1_down_tick;
	int attack2_down_tick;
	int attack1_up_tick;
	int attack2_up_tick;
	tb_int attack1_sustain_stats;
	tb_int attack2_sustain_stats;

	AttackTriggerStats () :
		prev_buttons(0),
		attack1_down_tick(0),
		attack2_down_tick(0),
		attack1_up_tick(0),
		attack2_up_tick(0),
		attack1_sustain_stats(),
		attack2_sustain_stats()
	{
	};
	AttackTriggerStats ( const AttackTriggerStats& other ) :
		prev_buttons(other.prev_buttons),
		attack1_down_tick ( other.attack1_down_tick ),
		attack2_down_tick ( other.attack2_down_tick ),
		attack1_up_tick ( other.attack1_up_tick ),
		attack2_up_tick ( other.attack2_up_tick ),
		attack1_sustain_stats ( other.attack1_sustain_stats ),
		attack2_sustain_stats ( other.attack2_sustain_stats )
	{
	};
};

class AutoAttackTester :
	public BaseTesterSystem,
	public PlayerDataStructHandler<AttackTriggerStats>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<AttackTriggerStats> playerdata_class;

public:
	AutoAttackTester ();

	virtual ~AutoAttackTester () final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void * const pCmd, double const & curtime) override final;

	void OnAttack1Up ( PlayerHandler::iterator ph, int game_tick );

	void OnAttack1Down ( PlayerHandler::iterator ph, int game_tick );

	void OnAttack2Up ( PlayerHandler::iterator ph, int game_tick );

	void OnAttack2Down ( PlayerHandler::iterator ph, int game_tick );

	void FindDetection ( PlayerHandler::iterator ph, tb_int* graph );
};

extern AutoAttackTester g_AutoAttackTester;

/*
	This is a temporary hack until I implement the html logs and xml detections
*/
struct detection_info
{
	tb_int::inner_type history[ TB_MAX_HISTORY];
	size_t detection_count;
	float detection_percent;
	size_t time_span;
	float average;
	int min;
	int max;

	detection_info () :
		history(),
		detection_count(0),
		detection_percent(0.0f),
		time_span(0),
		average(0.0f),
		min(0),
		max(0)
	{

	}

	detection_info ( detection_info const & other )
	{
		memcpy ( this, &other, sizeof ( detection_info ) );
	}

	detection_info& operator= ( detection_info const & other )
	{
		LoggerAssert ( this != &other );
		memcpy ( this, &other, sizeof ( detection_info ) );
		return *this;
	}
};

class Detection_AutoAttack : public LogDetection<detection_info>
{
	typedef LogDetection<detection_info> hClass;
public:
	Detection_AutoAttack () : hClass ()
	{};
	virtual ~Detection_AutoAttack () override final
	{};

	virtual basic_string GetDataDump () override final;
	virtual basic_string GetDetectionLogMessage () final
	{
		return "AutoAttack";
	};
};

#endif // AUTOATTACKTESTER_H
