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

#include "Players/NczPlayerManager.h"


BanRequest::BanRequest () :
	BaseStaticSystem ( "BanRequest", "Verbose - CanKick - CanBan - AllowWriteBans" ),
	Singleton (),
	TimerListener (),
	m_wait_time ( 10.0 ),
	m_do_writeid ( false ),
	m_can_kick ( true ),
	m_can_ban ( true ),
	m_can_write_ids (true),
	cmd_gb_ban ( nullptr ),
	cmd_sm_ban ( nullptr ),
	m_requests ()
{

}

bool BanRequest::sys_cmd_fn ( const SourceSdk::CCommand &args )
{
	if( stricmp ( "CanKick", args.Arg ( 2 ) ) == 0 )
	{
		if(Helpers::IsArgTrue(args.Arg(3)))
		{
			m_can_kick = true;
			g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "NoCheatZ is: Able to kick (%s), Able to ban (%s)", Helpers::boolToString ( m_can_kick ), Helpers::boolToString ( m_can_ban ) ) );
		}
		else if (Helpers::IsArgFalse(args.Arg(3)))
		{
			m_can_kick = false;
			m_can_ban = false;
			g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "NoCheatZ is: Able to kick (%s), Able to ban (%s)", Helpers::boolToString ( m_can_kick ), Helpers::boolToString ( m_can_ban ) ) );
		}
		else
		{
			g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "Available arguments for \"ncz BanRequest CanKick\" : Yes - No" ) );
		}

		g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("NoCheatZ is: Able to kick (%s), Able to ban (%s)", Helpers::boolToString(m_can_kick), Helpers::boolToString(m_can_ban)));
		return true;
	}
	else if( stricmp ( "CanBan", args.Arg ( 2 ) ) == 0 )
	{
		if(Helpers::IsArgTrue(args.Arg(3)))
		{
			m_can_kick = true;
			m_can_ban = true;
			g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "NoCheatZ is: Able to kick (%s), Able to ban (%s)", Helpers::boolToString ( m_can_kick ), Helpers::boolToString ( m_can_ban ) ) );
		}
		else if(Helpers::IsArgFalse(args.Arg(3)))
		{
			m_can_ban = false;
			g_Logger.Msg<MSG_CMD_REPLY> ( Helpers::format ( "NoCheatZ is: Able to kick (%s), Able to ban (%s)", Helpers::boolToString ( m_can_kick ), Helpers::boolToString ( m_can_ban ) ) );
		}
		else
		{
			g_Logger.Msg<MSG_CMD_REPLY> ( "Available arguments for \"ncz BanRequest CanBan\" : Yes - No" );
		}
		g_Logger.Msg<MSG_CMD_REPLY>(Helpers::format("NoCheatZ is: Able to kick (%s), Able to ban (%s)", Helpers::boolToString(m_can_kick), Helpers::boolToString(m_can_ban)));
		return true;
	}
	else if (stricmp("SetBanTime", args.Arg(2)) == 0)
	{
		if (args.ArgC() >= 4)
		{
			m_ban_time = atoi(args.Arg(3));
		}
		else
		{
			g_Logger.Msg<MSG_CMD_REPLY>("SetBanTime expected int argument (ban time minutes - 0 = permanent)");
		}
		return true;
	}
	else if (stricmp("AllowWriteBans", args.Arg(2)) == 0)
	{
		if (args.ArgC() >= 4)
		{
			if (Helpers::IsArgTrue(args.Arg(3)))
			{
				m_can_write_ids = true;
				g_Logger.Msg<MSG_CMD_REPLY>("AllowWriteBans is Yes");
				return true;
			}
			else if (Helpers::IsArgFalse(args.Arg(3)))
			{
				m_can_write_ids = false;
				g_Logger.Msg<MSG_CMD_REPLY>("AllowWriteBans is No");
				return true;
			}
		}
		g_Logger.Msg<MSG_CMD_REPLY>("AllowWriteBans expected Yes or No");
		return true;
	}
	else
	{
		return false;
	}
}

void BanRequest::Init ()
{
	TimerListener::AddTimerListener ( this );
}

void BanRequest::OnLevelInit ()
{
	cmd_gb_ban = SourceSdk::InterfacesProxy::ICvar_FindCommand ( "gb_externalBanUser" );
	cmd_sm_ban = SourceSdk::InterfacesProxy::ICvar_FindCommand ( "sm_ban" );

	if (m_can_write_ids)
	{
		SourceSdk::InterfacesProxy::Call_ServerCommand("exec banned_user.cfg\n");
		SourceSdk::InterfacesProxy::Call_ServerCommand("exec banned_ip.cfg\n");
	}
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
	if( CanKick () )
	{
		PlayerBanRequestT req;
		req.userid = SourceSdk::InterfacesProxy::Call_GetPlayerUserid ( player->GetEdict () );

		if( m_requests.Find ( req ) == nullptr )
		{
			req.ban_time = ban_time;
			req.request_time = Tier0::Plat_FloatTime ();
			req.kick_message = kick_message;
			strcpy_s ( req.player_name, 24, player->GetName () );
			strcpy_s ( req.steamid, 24, player->GetSteamID () );
			strcpy_s ( req.ip, 24, player->GetIPAddress () );
			strcpy_s ( req.identity, 64, player->GetReadableIdentity ().c_str () );
			m_requests.Add ( req );
		}

		AddTimer ( m_wait_time, player->GetName (), true );
	}
}

void BanRequest::BanInternal ( int ban_time, char const * steam_id, int userid, char const * kick_message, char const * ip )
{
	if( CanBan () )
	{
		if( cmd_gb_ban )
		{
		//	SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("gb_externalBanUser \"%s\" \"%s\" \"%s\" %d minutes \"%s\"\n", gb_admin_id.c_str(), SteamID, gb_reason_id.c_str(), minutes, this->getName()));
		}
		if( cmd_sm_ban )
		{
			if( userid == -1 )
			{
				g_Logger.Msg<MSG_WARNING> ( "BanRequest::BanInternal : Bad userid -> Cannot forward to sm_ban command." );
				g_Logger.Msg<MSG_HINT> ( "BanRequest::BanInternal : Using sm_addban ..." );
				SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "sm_addban %d \"%s\" \"%s\"\n", m_ban_time, steam_id, kick_message ) );
			}
			else
			{
				SourceSdk::InterfacesProxy::Call_ServerCommand ( Helpers::format ( "sm_ban #%d %d \"%s\"\n", userid, m_ban_time, kick_message ) );
			}
			SourceSdk::InterfacesProxy::Call_ServerExecute();
			KickNow(userid, kick_message);
		}
		else
		{
			KickNow(userid, kick_message);
			if (SteamGameServer_BSecure() && steam_id != nullptr)
			{
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("banid %d %s\n", m_ban_time, steam_id));
			}

			basic_string ip_stripped(ip);
			ip_stripped.replace(':', '\0');

			if (ip_stripped != "0" && ip_stripped != "127.0.0.1" && ip_stripped != "localhost" && ip_stripped[0] != '=')
			{
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("addip 1440 \"%s\"\n", ip_stripped.c_str()));
			}
		}

		m_do_writeid = true;
	//}
	}
	else if( CanKick () )
	{
		g_Logger.Msg<MSG_WARNING> ( "BanRequest::BanInternal : Plugin is set to not ban. Player will be kicked instead." );
		KickNow ( userid, kick_message );
	}
}

void BanRequest::KickNow (int userid, const char * kick_message ) const
{
	if( CanKick () )
	{
		if( userid == -1 )
		{
			g_Logger.Msg<MSG_ERROR> ( "BanRequest::KickNow : Bad userid" );
		}
		else
		{
			// test again the validity, because SM is supposed to kick him before

			PlayerHandler::iterator ph(g_NczPlayerManager.GetPlayerHandlerByUserId(userid));
			if (ph)
			{
				g_NczPlayerManager.DeclareKickedPlayer(g_NczPlayerManager.GetPlayerHandlerByUserId(userid).GetIndex());
				SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("kickid %d [NoCheatZ 4] %s\n", userid, kick_message));
			}
		}
	}
}

bool BanRequest::IsRejected ( char const * ip )
{
	if( m_rejects.IsEmpty () )
	{
		return false;
	}
	else
	{
		time_t const curtime ( time ( nullptr ) );

		// purge old rejects
		for( RejectListT::iterator it = m_rejects.begin (); it != m_rejects.end (); )
		{
			if( it->m_reject_until <= curtime )
			{
				m_rejects.FindAndRemove ( *it );
			}
			else
			{
				++it;
			}
		}

		if( m_rejects.IsEmpty () )
		{
			return false;
		}
		else
		{
			RejectStored info;
			info.m_ip_hash = Helpers::HashString ( ip );

			int const index ( m_rejects.Find ( info ) );
			if( index == -1 )
			{
				return false;
			}
			else
			{
				return true;
			}
		}
	}
}

void BanRequest::AddReject ( size_t duration_seconds, char const * ip )
{
	time_t const curtime ( time ( nullptr ) );

	RejectStored info;
	info.m_reject_until = curtime + duration_seconds;
	info.m_ip_hash = Helpers::HashString ( ip );

	if( m_rejects.Find ( info ) == -1 )
	{
		m_rejects.AddToHead ( info );
	}

	// purge old rejects
	for( RejectListT::iterator it = m_rejects.begin (); it != m_rejects.end (); )
	{
		if( it->m_reject_until <= curtime )
		{
			m_rejects.FindAndRemove ( *it );
		}
		else
		{
			++it;
		}
	}
}

void BanRequest::BanNow ( NczPlayer * const player, int ban_time, const char * kick_message )
{
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
	double const curtime (Tier0::Plat_FloatTime () );
	while( it != nullptr )
	{
		PlayerBanRequestT const & v ( it->m_value );

		if( v.request_time + m_wait_time < curtime )
		{
			PlayerHandler::iterator ph = SteamGameServer_BSecure () ? g_NczPlayerManager.GetPlayerHandlerBySteamID ( v.steamid ) : g_NczPlayerManager.GetPlayerHandlerByUserId ( v.userid );

			if( CanKick () )
			{
				if( ph > SlotStatus::KICK ) // Still connected
				{
					g_NczPlayerManager.DeclareKickedPlayer ( ph->GetIndex () );
				}

				BanInternal ( v.ban_time, v.steamid, v.userid, v.kick_message.c_str(), v.ip );
			}
			
			it = m_requests.Remove ( it );
		}
		else
		{
			it = it->m_next;
		}
	}

	if( !CanKick () )
	{
		g_Logger.Msg<MSG_WARNING> ( "BanRequest::RT_TimerCallback : Cannot process async ban because plugin is set to not kick players." );
	}
}

void BanRequest::WriteBansIfNeeded ()
{
	if( m_do_writeid && m_can_write_ids)
	{
		if( SteamGameServer_BSecure () )
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand ( "writeid\n" );
		}
		SourceSdk::InterfacesProxy::Call_ServerCommand ( "writeip\n" );

		m_do_writeid = false;
	}
}

BanRequest g_BanRequest;
