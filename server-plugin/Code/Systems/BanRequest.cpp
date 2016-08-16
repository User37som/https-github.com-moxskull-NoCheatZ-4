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

#include "BanRequest.h"

#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h"
#include "Misc/temp_Metrics.h"
#include "Players/NczPlayerManager.h"


BanRequest::BanRequest () :
	BaseStaticSystem ( "BanRequest", "Verbose - CanKick - CanBan" ),
	singleton_class (),
	TimerListener (),
	m_wait_time ( 10.0 ),
	m_do_writeid ( false ),
	cmd_gb_ban ( nullptr ),
	cmd_sm_ban ( nullptr ),
	m_requests ()
{

}

void BanRequest::Init ()
{
	TimerListener::AddTimerListener ( this );
}

void BanRequest::OnLevelInit ()
{
	cmd_gb_ban = SourceSdk::InterfacesProxy::ICvar_FindCommand ( "gb_externalBanUser" );
	cmd_sm_ban = SourceSdk::InterfacesProxy::ICvar_FindCommand ( "sm_ban" );
}

BanRequest::~BanRequest ()
{
	TimerListener::RemoveTimerListener ( this );
}

void BanRequest::SetWaitTime ( float wait_time )
{
	m_wait_time = wait_time;
}

void BanRequest::AddAsyncBan ( NczPlayer* player, int ban_time, const char * kick_message )
{
	PlayerBanRequestT req;
	req.userid = SourceSdk::InterfacesProxy::Call_GetPlayerUserid ( player->GetEdict () );

	if( m_requests.Find ( req ) == nullptr )
	{
		req.ban_time = ban_time;
		req.request_time = Plat_FloatTime ();
		req.kick_message = kick_message;
		strcpy_s ( req.player_name, 24, player->GetName () );
		strcpy_s ( req.steamid, 24, player->GetSteamID () );
		strcpy_s ( req.ip, 24, player->GetIPAddress () );
		strcpy_s ( req.identity, 64, player->GetReadableIdentity ().c_str () );
		m_requests.Add ( req );
	}

	AddTimer ( m_wait_time, player->GetName (), true );
}

void BanRequest::BanInternal ( int ban_time, char const * steam_id, int userid, char const * kick_message, char const * ip )
{
	if( cmd_gb_ban )
	{
	//	SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("gb_externalBanUser \"%s\" \"%s\" \"%s\" %d minutes \"%s\"\n", gb_admin_id.c_str(), SteamID, gb_reason_id.c_str(), minutes, this->getName()));
	}
	if( cmd_sm_ban )
	{
		if( userid == -1 )
		{
			Logger::GetInstance ()->Msg<MSG_ERROR> ( "BanRequest::BanInternal : Bad userid -> Cannot forward to sm_ban command." );
		}
		else
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "sm_ban %d %d \"%s\"\n", userid, ban_time, kick_message ) );
		}
	}
	/*
		Commenting because https://github.com/L-EARN/NoCheatZ-4/issues/64 :
			This code do sm_ban by userid but userid can be not ready sometimes.
			So I enforce my code to be sure the player is kicked if ever sourcemod fails.
	*/
	//else // 
	//{

		SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "kickid %d [NoCheatZ 4] %s\n", userid, kick_message ) );
		if( SteamGameServer_BSecure () && steam_id != nullptr )
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "banid %d %s\n", ban_time, steam_id ) );
		}
		if( strcmp ( "127.0.0.1", ip ) )
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "addip 1440 \"%s\"\n", ip ) );
		}

		m_do_writeid = true;
	//}
}

void BanRequest::BanNow ( NczPlayer * const player, int ban_time, const char * kick_message )
{
	// Remove player from process list until he entirely gets removed from the server

	NczPlayerManager::GetInstance ()->DeclareKickedPlayer ( player->GetIndex () );

	// Ban

	BanInternal ( ban_time, player->GetSteamID (), SourceSdk::InterfacesProxy::Call_GetPlayerUserid ( player->GetEdict () ), kick_message, player->GetIPAddress () );

	// Remove from async requests if any

	BanRequestListT::elem_t* it ( m_requests.Find ( SourceSdk::InterfacesProxy::Call_GetPlayerUserid ( player->GetEdict () ) ) );
	if( it != nullptr )
	{
		m_requests.Remove ( it );
	}
}

void BanRequest::RT_TimerCallback ( char const * const timer_name )
{
	BanRequestListT::elem_t* it ( m_requests.GetFirst () );
	float const curtime ( Plat_FloatTime () );
	while( it != nullptr )
	{
		PlayerBanRequestT const & v ( it->m_value );

		if( v.request_time + m_wait_time < curtime )
		{
			PlayerHandler::const_iterator ph = SteamGameServer_BSecure () ? NczPlayerManager::GetInstance ()->GetPlayerHandlerBySteamID ( v.steamid ) : NczPlayerManager::GetInstance ()->GetPlayerHandlerByUserId ( v.userid );

			if( ph > SlotStatus::KICK ) // Still connected
			{
				NczPlayerManager::GetInstance ()->DeclareKickedPlayer ( ph->GetIndex () );
			}

			BanInternal ( v.ban_time, v.steamid, v.userid, v.kick_message, v.ip );

			it = m_requests.Remove ( it );
		}
		else
		{
			it = it->m_next;
		}
	}
}

void BanRequest::WriteBansIfNeeded ()
{
	if( m_do_writeid )
	{
		if( SteamGameServer_BSecure () )
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand ( "writeid\n" );
		}
		SourceSdk::InterfacesProxy::Call_ServerCommand ( "writeip\n" );

		m_do_writeid = false;
	}
}
