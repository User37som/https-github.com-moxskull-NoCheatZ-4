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

#include <stdio.h>

#include "Preprocessors.h"

#include "AutoAttackTester.h"
#include "Systems/Logger.h"
#include "Systems/BanRequest.h"

AutoAttackTester::AutoAttackTester ( void ) :
	BaseTesterSystem( "AutoAttackTester" ),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	Singleton ()
{}

AutoAttackTester::~AutoAttackTester ( void )
{
	Unload ();
}

void AutoAttackTester::Init ()
{
	InitDataStruct ();
}

void AutoAttackTester::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, SystemPriority::AutoAttackTester );
}

void AutoAttackTester::Unload ()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( this );
}

bool AutoAttackTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

PlayerRunCommandRet AutoAttackTester::RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void* pCmd, double const & curtime )
{
	PlayerRunCommandRet const constexpr drop_cmd = PlayerRunCommandRet::CONTINUE;

	int * buttons;
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		buttons = &((((SourceSdk::CUserCmd_csgo * const)pCmd))->buttons);
	}
	else
	{
		buttons = &((((SourceSdk::CUserCmd * const)pCmd))->buttons);
	}
	AttackTriggerStats * const pdata(GetPlayerDataStructByIndex(ph.GetIndex()));

	int const attack1_button_changed ( ( pdata->prev_buttons ^ *buttons ) & ( IN_ATTACK ) );
	int const attack2_button_changed ( ( pdata->prev_buttons ^ *buttons ) & ( IN_ATTACK2 ) );

	int const gtick ( Helpers::GetGameTickCount () );

	if( attack1_button_changed )
	{
		if( (*buttons & IN_ATTACK ) != 0 )
		{
			OnAttack1Down ( ph, gtick );
		}
		else
		{
			OnAttack1Up ( ph, gtick );
		}
	}

	if( attack2_button_changed )
	{
		if( (*buttons & IN_ATTACK2 ) != 0 )
		{
			OnAttack2Down ( ph, gtick );
		}
		else
		{
			OnAttack2Up ( ph, gtick );
		}
	}

	pdata->prev_buttons = *buttons;

	return drop_cmd;
}

void AutoAttackTester::OnAttack1Up ( PlayerHandler::iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack1_up_tick = game_tick;
	pdata->attack1_sustain_stats.Store ( game_tick - pdata->attack1_down_tick , pdata->attack1_down_tick );

	FindDetection ( ph, &( pdata->attack1_sustain_stats ) );
}

void AutoAttackTester::OnAttack1Down ( PlayerHandler::iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack1_down_tick = game_tick;
}

void AutoAttackTester::OnAttack2Up ( PlayerHandler::iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack2_up_tick = game_tick;
	pdata->attack2_sustain_stats.Store ( game_tick - pdata->attack2_down_tick , pdata->attack2_down_tick );

	FindDetection ( ph, &( pdata->attack2_sustain_stats ) );
}

void AutoAttackTester::OnAttack2Down ( PlayerHandler::iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack2_down_tick = game_tick;
}

void AutoAttackTester::FindDetection ( PlayerHandler::iterator ph, tb_int* graph )
{
	// get trigger graph

	tb_int::inner_type attack_graph[ TB_MAX_HISTORY ];
	size_t amount;

	graph->CopyHistory ( attack_graph, amount, TB_MAX_HISTORY );

	if( amount == TB_MAX_HISTORY ) // wait for the graph to be full before trying to detect
	{
		float const ti ( SourceSdk::InterfacesProxy::Call_GetTickInterval () );
		__assume ( ti > 0.0f && ti < 1.0f );

		float const average_sustain_ticks ( graph->Average (0, ( int ) ( ( 0.5 / ti ) - 0.5f ) ) );
		int const max_sustain_ticks ( graph->Max () );
		int const min_sustain_ticks ( graph->Min () );
		
		int const short_time_ticks = (int)((SHORT_TIME / ti)-0.5f);

		if( average_sustain_ticks < float ( short_time_ticks ) )
		{
			// walk the graph and get the amount of detections

			size_t c ( 0 );
			tb_int::inner_type* it ( attack_graph );
			tb_int::inner_type const * const it_end ( attack_graph + TB_MAX_HISTORY );
			do
			{
				if( it->v < short_time_ticks )
				{
					++c;
				}
			}
			while( ++it != it_end );

			// how much percent detected ?

			float const percent ( ( (float)(c) / (float)(TB_MAX_HISTORY) ) * 100.0f );

			if( percent >= 70.0f )
			{
				// construct detection info

				detection_info info;
				info.average = average_sustain_ticks;
				info.detection_count = c;
				info.detection_percent = percent;
				memcpy ( info.history, attack_graph, sizeof ( tb_int::inner_type ) * TB_MAX_HISTORY );
				info.max = max_sustain_ticks;
				info.min = min_sustain_ticks;
				info.time_span = graph->TimeSpan ();

				// push detection

				ProcessDetectionAndTakeAction<Detection_AutoAttack::data_type>(Detection_AutoAttack(), &info, ph, this);

				// reset the graph ( let others detections be incoming without adding error or flooding )

				graph->Reset ();
			}
		}
	}
}

AutoAttackTester g_AutoAttackTester;

basic_string Detection_AutoAttack::GetDataDump ()
{
	float const ti_s ( SourceSdk::InterfacesProxy::Call_GetTickInterval () );
	float const ti_ms ( ti_s * 1000.0f );
	int const short_time_ticks = ( int ) ( ( SHORT_TIME / ( ti_s ) ) - 0.5f );

	basic_string m ( Helpers::format (
		":::: Important Server-Related Informations {\n"
		":::::::: Tick Interval : %f seconds -> %f ms,\n"
		":::::::: Ticks per second : %f,\n"
		":::::::: Short Attack Detection Threshold : %f seconds -> %d ticks\n"
		":::: }\n"
		":::: detection_info {\n"
		":::::::: Average Attack Button Sustain : %f ticks -> %f ms,\n"
		":::::::: Min Attack Button Sustain : %d ticks -> %f ms,\n"
		":::::::: Max Attack Button Sustain : %d ticks -> %f ms,\n"
		":::::::: Attack-Sustain Too Short Detected Count : %d ( %f %% of history ),\n"
		":::::::: Attacks History {\n",
		ti_s, ti_ms,
		1.0f / ti_s,
		short_time_ticks * ti_s, short_time_ticks,
		m_dataStruct.average, m_dataStruct.average * ti_ms,
		m_dataStruct.min, m_dataStruct.min * ti_ms,
		m_dataStruct.max, m_dataStruct.max * ti_ms,
		m_dataStruct.detection_count, m_dataStruct.detection_percent ));

	tb_int::inner_type const * it ( m_dataStruct.history );
	tb_int::inner_type const * const it_end ( m_dataStruct.history + TB_MAX_HISTORY );
	size_t shot_id ( 0 );

	do
	{
		m.append( Helpers::format ( 
			":::::::::::: Attack %u {\n"
			":::::::::::::::: Start Tick : %d\n"
			":::::::::::::::: Sustain Time (ticks) : %d ( Ends at tick # %d )\n"
			":::::::::::::::: Sustain Time (ms) : %f\n"
			":::::::::::::::: Is detected ? : %s\n"
			":::::::::::: }\n",
			++shot_id,
			it->t,
			it->v, it->t + it->v,
			it->v * ti_ms,
			Helpers::boolToString( it->v < short_time_ticks )
			));
	}
	while( ++it != it_end );

	m.append ( ":::::::: }\n:::: }\n" );

	return m;
}
