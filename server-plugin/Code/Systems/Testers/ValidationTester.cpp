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

#include "ValidationTester.h"

/////////////////////////////////////////////////////////////////////////
// ValidationTester
/////////////////////////////////////////////////////////////////////////

ValidationTester::ValidationTester () :
	BaseDynamicSystem ( "ValidationTester" ),
	SourceSdk::IGameEventListener002 (),
	OnTickListener (),
	playerdata_class (),
	singleton_class ()
{}

ValidationTester::~ValidationTester ()
{
	Unload ();
}

void ValidationTester::Init ()
{
	InitDataStruct ();
}

bool ValidationTester::GotJob () const
{
	return true;
}

void ValidationTester::SetValidated ( PlayerHandler::const_iterator ph )
{
	GetPlayerDataStructByIndex ( ph.GetIndex () )->b = true;
	DebugMessage ( Helpers::format ( "%s SteamID validated\n", ph->GetName () ) );

	LoggerAssert ( ph->GetPlayerInfo () );

	ValidatedIdsT::elem_t const * const it ( m_validated_ids.Find ( ValidatedInfo ( ph->GetSteamID () ) ) );

	if( it == nullptr ) m_validated_ids.Add ( ValidatedInfo ( ph->GetSteamID (), ph->GetIPAddress () ) );
}

void ValidationTester::RT_ProcessPlayerTestOnTick ( PlayerHandler::const_iterator ph, float const curtime )
{
	if( !SteamGameServer_BSecure () ) return;
	if( GetPlayerDataStructByIndex ( ph.GetIndex () )->b )
		return;

	SystemVerbose1 ( Helpers::format ( "%s SteamID not validated, validation time left : %f", ph->GetName (), 20.0f - ph->GetTimeConnected () ) );

	if( ph->GetTimeConnected () < 20.0f ) return;
	ph->Kick ();
}

void ValidationTester::Load ()
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	OnTickListener::RegisterOnTickListener ( this );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_disconnect", true );
}

void ValidationTester::Unload ()
{
	OnTickListener::RemoveOnTickListener ( this );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->RemoveListener ( this );
}

bool ValidationTester::JoinCallback ( PlayerHandler::const_iterator ph )
{
	if( IsActive () && SteamGameServer_BSecure () )
	{
		if( !GetPlayerDataStruct ( *ph )->b )
		{
			if( !WasPreviouslyValidated ( ph ) )
			{
				Helpers::tell ( ph->GetEdict (), "You can't join the game now because your Steam ID is not validated yet.\nRetry with the F9 button or activate Steam and restart the game." );
			}
			else
			{
				DebugMessage ( Helpers::format ( "%s SteamID was previously validated with the same IP. Plugin will flag this player as validated.\n", ph->GetName () ) );
				SetValidated ( ph );
			}
			return true;
		}
	}
	return false;
}

void ValidationTester::AddPendingValidation ( const char *pszUserName, const char* steamid )
{
	m_pending_validations.Add ( steamid );
}

bool ValidationTester::WasPreviouslyValidated ( PlayerHandler::const_iterator ph )
{
	LoggerAssert ( ph->GetPlayerInfo () );

	ValidatedIdsT::elem_t const * const it ( m_validated_ids.Find ( ValidatedInfo ( ph->GetSteamID () ) ) );

	if( it == nullptr ) return false;
	else if( strcmp ( it->m_value.m_ipaddress, ph->GetIPAddress () ) == 0 ) return true;
	else return false;
}

void ValidationTester::RT_ProcessOnTick ( float const curtime )
{
	PendingValidationsT::elem_t* it ( m_pending_validations.GetFirst () );
	while( it != nullptr )
	{
		PlayerHandler::const_iterator ph ( NczPlayerManager::GetInstance ()->GetPlayerHandlerBySteamID ( it->m_value ) );

		if( ph == SlotStatus::INVALID || ph->GetPlayerInfo () == nullptr )
		{
			it = it->m_next;
			continue;
		}

		SetValidated ( ph );
		it = m_pending_validations.Remove ( it );
	}

	ProcessFilter::HumanOnlyConnected filter_class;
	for( PlayerHandler::const_iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph += &filter_class )
	{
		RT_ProcessPlayerTestOnTick ( ph, curtime );
	}
}

void ValidationTester::FireGameEvent ( SourceSdk::IGameEvent * ev )
{
	//DebugMessage(Helpers::format("Received player_disconnect event : \n\t%d\n\t%s\n\t%s\n\t%s\n\t%d", ev->GetInt("userid"), ev->GetString("reason"), ev->GetString("name"), ev->GetString("networkid"), ev->GetBool("bot")).c_str());
	m_validated_ids.Remove ( ValidatedInfo ( ev->GetString ( "networkid" ) ) );
}
