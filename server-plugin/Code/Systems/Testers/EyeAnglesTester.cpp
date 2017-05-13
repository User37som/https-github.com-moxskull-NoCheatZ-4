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

#include "EyeAnglesTester.h"

#include "Misc/EntityProps.h"
#include "Systems/BanRequest.h"

EyeAnglesTester::EyeAnglesTester ( void ) :
	BaseDynamicSystem ( "EyeAnglesTester" ),
	SourceSdk::IGameEventListener002 (),
	playerdata_class (),
	PlayerRunCommandHookListener (),
	singleton_class ()
{}

EyeAnglesTester::~EyeAnglesTester ( void )
{
	Unload ();
}

void EyeAnglesTester::Init ()
{
	InitDataStruct ();
}

void EyeAnglesTester::Load ()
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "round_end", true );
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, SystemPriority::UserCmdHookListener::EyeAnglesTester );
}

void EyeAnglesTester::Unload ()
{
	PlayerRunCommandHookListener::RemovePlayerRunCommandHookListener ( this );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->RemoveListener ( this );
}

bool EyeAnglesTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnected const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::const_iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

PlayerRunCommandRet EyeAnglesTester::RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const pCmd, void * const old_cmd )
{
	int const * const flags ( EntityProps::GetInstance ()->GetPropValue<int, PROP_FLAGS> ( ph->GetEdict () ) );

	/*
		FL_FROZEN			(1 << 5)
		FL_ATCONTROLS		(1 << 6)
	*/
	if( *flags & ( 3 << 5 ) ) return PlayerRunCommandRet::CONTINUE;

	PlayerRunCommandRet drop_cmd ( PlayerRunCommandRet::CONTINUE );

	EyeAngleInfoT* playerData ( GetPlayerDataStructByIndex ( ph.GetIndex () ) );
	playerData->x.abs_value = fabs ( playerData->x.value = static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->viewangles.x );
	playerData->y.abs_value = fabs ( playerData->y.value = static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->viewangles.y );
	playerData->z.abs_value = fabs ( playerData->z.value = static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->viewangles.z );

	if( playerData->x.abs_value > 89.0f || playerData->z.abs_value > 1.0f || playerData->y.abs_value > 180.0f )
	{
		if( playerData->ignore_last ) --( playerData->ignore_last );
		else drop_cmd = PlayerRunCommandRet::INERT;
	}

	if( drop_cmd > PlayerRunCommandRet::CONTINUE )
	{
		if( playerData->x.abs_value > 89.0f )
		{
			++playerData->detectionsCount;
			if( playerData->x.lastDetectionPrintTime + ANTIFLOOD_LOGGING_TIME < Plat_FloatTime () )
			{
				playerData->x.lastDetectionPrintTime = Plat_FloatTime ();

				TriggerDetection ( Detection_EyeAngleX, ph, playerData );
			}
		}
		if( playerData->y.abs_value > 180.0f )
		{
			++playerData->detectionsCount;
			if( playerData->y.lastDetectionPrintTime + ANTIFLOOD_LOGGING_TIME < Plat_FloatTime () )
			{
				playerData->y.lastDetectionPrintTime = Plat_FloatTime ();

				TriggerDetection ( Detection_EyeAngleY, ph, playerData );
			}
		}
		if( playerData->z.abs_value > 0.0f )
		{
			++playerData->detectionsCount;
			if( playerData->z.lastDetectionPrintTime + ANTIFLOOD_LOGGING_TIME < Plat_FloatTime () )
			{
				playerData->z.lastDetectionPrintTime = Plat_FloatTime ();

				TriggerDetection ( Detection_EyeAngleZ, ph, playerData );
			}
		}
	}
	return drop_cmd;
}

void EyeAnglesTester::FireGameEvent ( SourceSdk::IGameEvent *ev ) // round_end
{
	ProcessFilter::HumanAtLeastConnected filter_class;
	for( PlayerHandler::const_iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph+=&filter_class )
	{
		++( GetPlayerDataStructByIndex ( ph.GetIndex () )->ignore_last );
	}
}

void Base_Detection_EyeAngle::TakeAction ()
{
	BanRequest::GetInstance ()->AddAsyncBan ( *m_player, 0, nullptr );
}

void Base_Detection_EyeAngle::WriteXMLOutput ( FILE * const out ) const
{
	Assert ( out );

	fprintf ( out,
			  "<base_detection_eyeangle>\n\t\t\t"
			  "<struct name=\"EyeAngleInfo\">\n\t\t\t\t"
			  "<value name=\"x.value\" desc=\"EyeAngle X value\" unit=\"deg\">%f</value>\n\t\t\t\t\t"
			  "<value name=\"y.value\" desc=\"EyeAngle Y value\" unit=\"deg\">%f</value>\n\t\t\t\t\t"
			  "<value name=\"z.value\" desc=\"EyeAngle Z value\" unit=\"deg\">%f</value>\n\t\t\t\t\t"
			  "<value name=\"detectionsCount\" desc=\"Number of times one value exceeded range\" unit=\"count\">%u</value>\n\t\t\t\t\t"
			  "</struct>\n\t\t\t"
			  "</base_detection_eyeangle>",
			GetDataStruct ()->x.value,
			 GetDataStruct ()->x.detectionsCount,
			GetDataStruct ()->y.value,
			 GetDataStruct ()->y.detectionsCount,
			GetDataStruct ()->z.value,
			  GetDataStruct ()->z.detectionsCount
	);
}

basic_string Detection_EyeAngleX::GetDetectionLogMessage () const
{
	if( Helpers::IsInt ( GetDataStruct ()->x.value ) )
	{
		return "Anti-Aim";
	}
	else
	{
		return "No recoil";
	}
}

basic_string Detection_EyeAngleY::GetDetectionLogMessage () const
{
	if( Helpers::IsInt ( GetDataStruct ()->y.value ) )
	{
		return "Anti-Aim";
	}
	else
	{
		return "No recoil";
	}
}

basic_string Detection_EyeAngleZ::GetDetectionLogMessage () const
{
	if( Helpers::IsInt ( GetDataStruct ()->z.value ) )
	{
		return "Anti-Aim";
	}
	else
	{
		return "No recoil";
	}
}
