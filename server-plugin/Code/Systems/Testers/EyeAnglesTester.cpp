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
#include <cmath>

#include "EyeAnglesTester.h"

#include "Misc/EntityProps.h"
#include "Systems/BanRequest.h"

EyeAnglesTester::EyeAnglesTester ( void ) :
	BaseSystem ( "EyeAnglesTester" ),
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
	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener ( this, 2 );
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

	if( playerData->x.abs_value > 89.0f || playerData->z.abs_value > 0.0f || playerData->y.abs_value > 180.0f )
	{
#		ifdef DEBUG
		printf ( "Player %s : Bad Eye Angles %f, %f, %f\n", ph->GetName (), playerData->x.value, playerData->y.value, playerData->z.value );
#		endif
		if( playerData->ignore_last ) --( playerData->ignore_last );
		else drop_cmd = PlayerRunCommandRet::INERT;
	}

	if( drop_cmd > PlayerRunCommandRet::CONTINUE )
	{
#		ifdef DEBUG
		printf ( "Player %s : Droping command #%d\n", ph->GetName (), static_cast< SourceSdk::CUserCmd_csgo* >( pCmd )->command_number );
#		endif
		if( playerData->x.abs_value > 89.0f )
		{
			++playerData->x.detectionsCount;
			if( playerData->x.lastDetectionPrintTime + 10.0 < Plat_FloatTime () )
			{
				playerData->x.lastDetectionPrintTime = Plat_FloatTime ();

				Detection_EyeAngleX pDetection;
				pDetection.PrepareDetectionData ( playerData );
				pDetection.PrepareDetectionLog ( ph, this );
				pDetection.Log ();
			}
		}
		if( playerData->y.abs_value > 180.0f )
		{
			++playerData->y.detectionsCount;
			if( playerData->y.lastDetectionPrintTime + 10.0 < Plat_FloatTime () )
			{
				playerData->y.lastDetectionPrintTime = Plat_FloatTime ();

				Detection_EyeAngleY pDetection;
				pDetection.PrepareDetectionData ( playerData );
				pDetection.PrepareDetectionLog ( ph, this );
				pDetection.Log ();
			}
		}
		if( playerData->z.abs_value > 0.0f )
		{
			++playerData->z.detectionsCount;
			if( playerData->z.lastDetectionPrintTime + 10.0 < Plat_FloatTime () )
			{
				playerData->z.lastDetectionPrintTime = Plat_FloatTime ();

				Detection_EyeAngleZ pDetection;
				pDetection.PrepareDetectionData ( playerData );
				pDetection.PrepareDetectionLog ( ph, this );
				pDetection.Log ();
			}
		}

		BanRequest::GetInstance ()->AddAsyncBan ( ph, 0, "Banned by NoCheatZ 4" );
	}
	return drop_cmd;
}

void EyeAnglesTester::FireGameEvent ( SourceSdk::IGameEvent *ev ) // round_end
{
	for( PlayerHandler::const_iterator ph ( PlayerHandler::begin () ); ph != PlayerHandler::end (); ++ph )
	{
		if( ph >= SlotStatus::PLAYER_CONNECTED )
		{
			++( GetPlayerDataStructByIndex ( ph.GetIndex () )->ignore_last );
		}
	}
}

basic_string Detection_EyeAngle::GetDataDump ()
{
	return Helpers::format ( ":::: EyeAngleInfo {\n:::::::: EyeAngleX {\n:::::::::::: Angle : %f,\n:::::::::::: Detections Count : %ud\n:::::::: },\n:::::::: EyeAngleY {\n:::::::::::: Angle : %f,\n:::::::::::: Detections Count : %ud\n:::::::: },\n:::::::: EyeAngleZ {\n:::::::::::: Angle : %f,\n:::::::::::: Detections Count : %ud\n:::::::: }\n:::: }",
							 GetDataStruct ()->x.value, GetDataStruct ()->x.detectionsCount,
							 GetDataStruct ()->y.value, GetDataStruct ()->y.detectionsCount,
							 GetDataStruct ()->z.value, GetDataStruct ()->z.detectionsCount );
}

basic_string Detection_EyeAngleX::GetDetectionLogMessage ()
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

basic_string Detection_EyeAngleY::GetDetectionLogMessage ()
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

basic_string Detection_EyeAngleZ::GetDetectionLogMessage ()
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
