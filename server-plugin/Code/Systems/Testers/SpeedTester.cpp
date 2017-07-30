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

#include "SpeedTester.h"

#include "Systems/Logger.h"

SpeedTester::SpeedTester () :
	BaseTesterSystem ( "SpeedTester" ),
	OnTickListener (),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	Singleton ()
{}

SpeedTester::~SpeedTester ()
{
	Unload ();
}

void SpeedTester::Init ()
{
	InitDataStruct ();
}

void SpeedTester::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	OnTickListener::RegisterOnTickListener ( this );
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, SystemPriority::SpeedTester );
}

void SpeedTester::Unload ()
{
	OnTickListener::RemoveOnTickListener ( this );
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( this );
}

bool SpeedTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

void SpeedTester::RT_ProcessOnTick (double const & curtime )
{
	ProcessFilter::InTestsNoBot filter_class;

	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph += &filter_class )
	{
		SpeedHolderT* const pInfo ( this->GetPlayerDataStructByIndex ( ph.GetIndex () ) );
		float const tick_interval ( SourceSdk::InterfacesProxy::Call_GetTickInterval () );
		const double newTicks ( ceil ( ( curtime - pInfo->lastTest ) / tick_interval ) );
		SourceSdk::INetChannelInfo* const netchan ( ph->GetChannelInfo () );
		//if( netchan == nullptr ) return;

		const float latency ( netchan->GetLatency ( FLOW_OUTGOING ) );

		if( !pInfo->ticksLeft && fabs ( pInfo->previousLatency - latency ) <= 0.005f )
		{
			++( pInfo->detections );

			//DebugMessage ( Helpers::format ( "Player %s :  Speedhack pre-detection #%ud", ph->GetName (), pInfo->detections ) );

			if( pInfo->detections >= 30 && curtime > pInfo->lastDetectionTime + 30.0f )
			{
				pInfo->lastDetectionTime = curtime;
				ProcessDetectionAndTakeAction<Detection_SpeedHack::data_type>(Detection_SpeedHack(), pInfo, ph, this);
			}
		}
		else if( pInfo->detections )
		{
			--( pInfo->detections );
		}

		float const vtest = ceil ( ( 1.0f / tick_interval * 2.0f ) );
		if( ( pInfo->ticksLeft += (float)newTicks ) > vtest )
		{
			pInfo->ticksLeft = vtest;
		}

		pInfo->previousLatency = latency;
		pInfo->lastTest = curtime;
	}
}

PlayerRunCommandRet SpeedTester::RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void* pCmd, double const & curtime)
{
	float& tl ( this->GetPlayerDataStructByIndex ( ph.GetIndex () )->ticksLeft );

	if( !tl ) return PlayerRunCommandRet::BLOCK;

	tl -= 1.0f;

	return PlayerRunCommandRet::CONTINUE;
}

SpeedTester g_SpeedTester;

basic_string Detection_SpeedHack::GetDataDump ()
{
	return Helpers::format ( ":::: SpeedHolderT {\n"
							 ":::::::: Ticks Left : %f,\n"
							 ":::::::: Detections Count : %lu,\n"
							 ":::::::: Last Detection Time %f,\n"
							 ":::::::: Last Latency : %f,\n"
							 ":::::::: Last Test Time : %f\n"
							 ":::: }",
							 GetDataStruct ()->ticksLeft,
							 GetDataStruct ()->detections,
							 GetDataStruct ()->lastDetectionTime,
							 GetDataStruct ()->previousLatency,
							 GetDataStruct ()->lastTest );
}
