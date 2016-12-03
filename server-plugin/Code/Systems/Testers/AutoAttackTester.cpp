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

#include "AutoAttackTester.h"

#include "Systems/BanRequest.h"

AutoAttackTester::AutoAttackTester ( void ) :
	BaseDynamicSystem ( "AutoAttackTester" ),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	singleton_class ()
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
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, SystemPriority::UserCmdHookListener::AutoAttackTester );
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
	PlayerHandler::const_iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

PlayerRunCommandRet AutoAttackTester::RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void* pCmd, void* lastcmd )
{
	PlayerRunCommandRet const constexpr drop_cmd = PlayerRunCommandRet::CONTINUE;

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		int const last_buttons ( static_cast< SourceSdk::CUserCmd_csgo* >( lastcmd )->buttons );
		int const cur_buttons ( static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->buttons );

		int const attack1_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK ) );
		int const attack2_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK2 ) );

		int const gtick ( Helpers::GetGameTickCount () );

		if( attack1_button_changed )
		{
			if( (cur_buttons & IN_ATTACK ) != 0 )
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
			if( (cur_buttons & IN_ATTACK2 ) != 0 )
			{
				OnAttack2Down ( ph, gtick );
			}
			else
			{
				OnAttack2Up ( ph, gtick );
			}
		}
	}
	else
	{
		int const last_buttons ( static_cast< SourceSdk::CUserCmd* >( lastcmd )->buttons );
		int const cur_buttons ( static_cast< SourceSdk::CUserCmd* >( pCmd )->buttons );

		int const attack1_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK ) );
		int const attack2_button_changed ( ( last_buttons ^ cur_buttons ) & ( IN_ATTACK2 ) );

		int const gtick ( Helpers::GetGameTickCount () );

		if( attack1_button_changed )
		{
			if( (cur_buttons & IN_ATTACK) != 0 )
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
			if( (cur_buttons & IN_ATTACK2) != 0 )
			{
				OnAttack2Down ( ph, gtick );
			}
			else
			{
				OnAttack2Up ( ph, gtick );
			}
		}
	}

	return drop_cmd;
}

void AutoAttackTester::OnAttack1Up ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack1_up_tick = game_tick;
	pdata->attack1_sustain_stats.Store ( game_tick - pdata->attack1_down_tick , pdata->attack1_down_tick );

	FindDetection ( ph, &( pdata->attack1_sustain_stats ) );
}

void AutoAttackTester::OnAttack1Down ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack1_down_tick = game_tick;
}

void AutoAttackTester::OnAttack2Up ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack2_up_tick = game_tick;
	pdata->attack2_sustain_stats.Store ( game_tick - pdata->attack2_down_tick , pdata->attack2_down_tick );

	FindDetection ( ph, &( pdata->attack2_sustain_stats ) );
}

void AutoAttackTester::OnAttack2Down ( PlayerHandler::const_iterator ph, int game_tick )
{
	AttackTriggerStats * const pdata ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	pdata->attack2_down_tick = game_tick;
}

void AutoAttackTester::FindDetection ( PlayerHandler::const_iterator ph, tb_int* graph )
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

				TriggerDetection ( Detection_AutoAttack, ph, &info );

				// reset the graph ( let others detections be incoming without adding error or flooding )

				graph->Reset ();
			}
		}
	}
}

void Detection_AutoAttack::TakeAction ()
{
	BanRequest::GetInstance ()->AddAsyncBan ( *(this->m_player), 0, nullptr );
}

void Detection_AutoAttack::WriteXMLOutput ( FILE * const out ) const
{
	Assert ( out );

	float const ti_s ( SourceSdk::InterfacesProxy::Call_GetTickInterval () );
	int const short_time_ticks = ( int ) ( ( SHORT_TIME / ( ti_s ) ) - 0.5f );

	fprintf ( out,
			  "<detection_autoattack>\n\t\t\t"
			  "<value name=\"short_time_ticks\" desc=\"Detection Threshold\" unit=\"tick_count\">%d</value>\n\t\t\t"
			  "<value name=\"average\" desc=\"Average Attack Button Sustain\" unit=\"tick_count\">%f</value>\n\t\t\t"
			  "<value name=\"min\" desc=\"Min Attack Button Sustain\" unit=\"tick_count\">%d</value>\n\t\t\t"
			  "<value name=\"max\" desc=\"Max Attack Button Sustain\" unit=\"tick_count\">%d</value>\n\t\t\t"
			  "<value name=\"detection_percent\" desc=\"Detection Percent\" unit=\"percent\">%f</value>\n\t\t\t"
			  "<array name=\"history\" desc=\"Shot History\">",
			  short_time_ticks,
			  m_dataStruct.average,
			  m_dataStruct.min,
			  m_dataStruct.max,
			  m_dataStruct.detection_percent
	);

	tb_int::inner_type const * it ( m_dataStruct.history + TB_MAX_HISTORY - 1 );
	tb_int::inner_type const * const it_end ( m_dataStruct.history - 1);
	size_t shot_id ( 0 );

	do
	{
		fprintf ( out,
				  "\n\t\t\t\t<attack id=%d>\n\t\t\t\t\t"
				  "<value name=\"t\" desc=\"Attack Start\" unit=\"tick_count\">%d</value>\n\t\t\t\t\t"
				  "<value name=\"v\" desc=\"Attack Sustain\" unit=\"tick_count\">%d</value>\n\t\t\t\t\t"
				  "<value desc=\"Attack End\" unit=\"tick_count\">%d</value>\n\t\t\t\t\t"
				  "<value desc=\"Is Detected\" unit=\"boolean\">%d</value>\n\t\t\t\t"
				  "</attack>",
				  ++shot_id,
				  it->t,
				  it->v,
				  it->t + it->v,
				  ( it->v < short_time_ticks )
		 );
	}
	while( --it != it_end );

	fprintf ( out, "\n\t\t\t</array>\n\t\t</detection_autoattack>" );
}
