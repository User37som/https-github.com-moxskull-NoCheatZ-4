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

#include "ShotTester.h"

#include "Systems/Logger.h"

/*
	Test each player to see if they use any script to help them fire more bullets (= RapidFire)

	Some old mouses can also make multiple clicks because of an electronic issue, not because the player itself use a script.
	We have to make the difference by using statistics.
*/

ShotTester::ShotTester ( void ) :
	BaseDynamicSystem ( "ShotTester" ),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	singleton_class ()
{}

ShotTester::~ShotTester ( void )
{
	Unload ();
}

void ShotTester::Init ()
{
	InitDataStruct ();
}

void ShotTester::Load ()
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, 4 );
}

void ShotTester::Unload ()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( this );
}

bool ShotTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::const_iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void TriggerStat ( ShotStatHandlerT* handler, float up_time, float down_time, size_t clicks )
{
	++( handler->n );
	handler->avg_time = ( handler->avg_time + ( up_time - down_time ) / ( float ) ( handler->n ) );
	handler->ratio = ( ( float ) ( handler->n ) / ( float ) ( clicks ) ) * 100.0f;
}

void OutputStat ( ShotStatHandlerT* handler )
{
	printf ( "{ count %u, ratio %3.2f, avg %3.2f }", handler->n, handler->ratio, handler->avg_time );
}

PlayerRunCommandRet ShotTester::RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void* pCmd, void* lastcmd )
{
	PlayerRunCommandRet drop_cmd = PlayerRunCommandRet::CONTINUE;

	ShotStatsT * const playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );

	float const curtime ( Plat_FloatTime () );

	bool cur_in_attack;
	bool past_in_attack;

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		cur_in_attack = ( static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->buttons & IN_ATTACK ) != 0;
		past_in_attack = ( static_cast< SourceSdk::CUserCmd_csgo* >( lastcmd )->buttons & IN_ATTACK ) != 0;
	}
	else
	{
		cur_in_attack = ( static_cast< SourceSdk::CUserCmd* >( pCmd )->buttons & IN_ATTACK ) != 0;
		past_in_attack = ( static_cast< SourceSdk::CUserCmd* >( lastcmd )->buttons & IN_ATTACK ) != 0;
	}

	if( cur_in_attack && !past_in_attack )
	{
		//SystemVerbose1(Helpers::format("Player %s : IN_ATTACK button down.", ph->GetName()));
		playerData->down_time = curtime;
	}
	else if( past_in_attack && !cur_in_attack )
	{
		playerData->up_time = curtime;
		TriggerStat ( &( playerData->clicks ), playerData->up_time, playerData->down_time, playerData->clicks.n );
		//SystemVerbose1(Helpers::format("Player %s : IN_ATTACK button up.", ph->GetName()));

		if( playerData->up_time - playerData->down_time <= SHORT_TIME )
		{
			if( curtime - playerData->last_detection > 1.0 )
			{
				if( playerData->row == 1 )
				{
					if( playerData->on_target.ratio > 25.0 && playerData->clicks.n > 50 )
					{
						Detection_TriggerBot pDetection;
						pDetection.PrepareDetectionData ( playerData );
						pDetection.PrepareDetectionLog ( *ph, this );
						pDetection.Log ();

						ph->Ban ();
					}
				}
				playerData->row = 0;
			}
			if( playerData->row > 10 )
			{
				if( playerData->short_clicks.ratio > 40.0 && playerData->clicks.n > 50 )
				{
					Detection_AutoPistol pDetection;
					pDetection.PrepareDetectionData ( playerData );
					pDetection.PrepareDetectionLog ( *ph, this );
					pDetection.Log ();

					ph->Ban ();
				}
			}
			playerData->last_detection = curtime;
			++( playerData->row );

			TriggerStat ( &( playerData->short_clicks ), playerData->up_time, playerData->down_time, playerData->clicks.n );
			if( ph->GetWpnShotType () == HAND ) TriggerStat ( &( playerData->with_hand ), playerData->up_time, playerData->down_time, playerData->clicks.n );
			else if( ph->GetWpnShotType () == PISTOL ) TriggerStat ( &( playerData->with_pistol ), playerData->up_time, playerData->down_time, playerData->clicks.n );
			else TriggerStat ( &( playerData->with_auto ), playerData->up_time, playerData->down_time, playerData->clicks.n );
			if( ph->aimingAt () > 0 ) TriggerStat ( &( playerData->on_target ), playerData->up_time, playerData->down_time, playerData->clicks.n );
			drop_cmd = PlayerRunCommandRet::INERT;

			if( this->m_verbose )
			{
				printf ( "%f - clicks ", curtime );
				OutputStat ( &( playerData->clicks ) );
				printf ( ", short_clicks " );
				OutputStat ( &( playerData->short_clicks ) );
				printf ( ", with_hand " );
				OutputStat ( &( playerData->with_hand ) );
				printf ( ", with_pistol " );
				OutputStat ( &( playerData->with_pistol ) );
				printf ( ", with_auto " );
				OutputStat ( &( playerData->with_auto ) );
				printf ( ", on_target " );
				OutputStat ( &( playerData->on_target ) );
				printf ( ", row %u\n", playerData->row );
			}
		}
	}
	return drop_cmd;
}

basic_string ShotDetection::GetDataDump ()
{
	return Helpers::format ( ":::: ShotStatsT {\n"
							 ":::::::: Attack1 Button Up At (Time) : %f,\n"
							 ":::::::: Attack1 Button Down At (Time) : %f,\n"
							 ":::::::: Clicks {\n"
							 ":::::::::::: Clicks Count : %lu,\n"
							 ":::::::::::: Detection Ratio : %f,\n"
							 ":::::::::::: Average Button Down Hold Time : %f s\n"
							 ":::::::: },\n"
							 ":::::::: ShortClicks {\n"
							 ":::::::::::: Clicks Count : %lu,\n"
							 ":::::::::::: Detection Ratio : %f,\n"
							 ":::::::::::: Average Button Down Hold Time : %f s\n"
							 ":::::::: },\n"
							 ":::::::: WithHand {\n"
							 ":::::::::::: Clicks Count : %lu,\n"
							 ":::::::::::: Detection Ratio : %f,\n"
							 ":::::::::::: Average Button Down Hold Time : %f s\n"
							 ":::::::: },\n"
							 ":::::::: WithPistol {\n"
							 ":::::::::::: Clicks Count : %lu,\n"
							 ":::::::::::: Detection Ratio : %f,\n"
							 ":::::::::::: Average Button Down Hold Time : %f s\n"
							 ":::::::: },\n"
							 ":::::::: WithAuto {\n"
							 ":::::::::::: Clicks Count : %lu,\n"
							 ":::::::::::: Detection Ratio : %f,\n"
							 ":::::::::::: Average Button Down Hold Time : %f s\n"
							 ":::::::: },\n"
							 ":::::::: OnTarget {\n"
							 ":::::::::::: Clicks Count : %lu,\n"
							 ":::::::::::: Detection Ratio : %f,\n"
							 ":::::::::::: Average Button Down Hold Time : %f s\n"
							 ":::::::: },\n"
							 ":::::::: Consecutive Detections Count : %lu,\n"
							 ":::::::: Last Detection Time : %f\n"
							 ":::: }",
							 GetDataStruct ()->up_time, GetDataStruct ()->down_time,
							 GetDataStruct ()->clicks.n, GetDataStruct ()->clicks.ratio, GetDataStruct ()->clicks.avg_time,
							 GetDataStruct ()->short_clicks.n, GetDataStruct ()->short_clicks.ratio, GetDataStruct ()->short_clicks.avg_time,
							 GetDataStruct ()->with_hand.n, GetDataStruct ()->with_hand.ratio, GetDataStruct ()->with_hand.avg_time,
							 GetDataStruct ()->with_pistol.n, GetDataStruct ()->with_pistol.ratio, GetDataStruct ()->with_pistol.avg_time,
							 GetDataStruct ()->with_auto.n, GetDataStruct ()->with_auto.ratio, GetDataStruct ()->with_auto.avg_time,
							 GetDataStruct ()->on_target.n, GetDataStruct ()->on_target.ratio, GetDataStruct ()->on_target.avg_time,
							 GetDataStruct ()->row, GetDataStruct ()->last_detection );
}
