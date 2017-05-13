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

#include "PlayerDetections.h"

#include "Systems/Testers/Detections/temp_BaseDetection.h" // BaseDetection
#include "Players/NczPlayerManager.h" // PlayerHandler::const_iterator
#include "Systems/ConfigManager.h" // root_server_path

/*
	rt_detection_elem
*/

PlayerDetections::rt_detection_elem::rt_detection_elem () :
	m_time (),
	m_detection ( nullptr )
{

}

PlayerDetections::rt_detection_elem::rt_detection_elem ( Helpers::game_tm& time, BaseDetection* detection ) :
	m_time ( time ),
	m_detection ( detection )
{

}

bool PlayerDetections::rt_detection_elem::operator==( rt_detection_elem const & other )
{
	return m_detection->GetDataHash () == other.m_detection->GetDataHash ();
}

/*
	rt_detection_list_by_udid
*/

PlayerDetections::rt_detection_list_by_udid::rt_detection_list_by_udid () : m_udid ( 0 ), m_list ()
{}

PlayerDetections::rt_detection_list_by_udid::rt_detection_list_by_udid ( uint32_t udid ) : m_udid ( udid ), m_list ()
{}

bool PlayerDetections::rt_detection_list_by_udid::operator==( PlayerDetections::rt_detection_list_by_udid const & other ) const
{
	return m_udid == other.m_udid;
}

/*
	PlayerDetections
*/

PlayerDetections::PlayerDetections ( size_t playerindex ) :
	m_playerindex ( playerindex ),
	m_dict ()
{

}

PlayerDetections::~PlayerDetections ()
{

}

void PlayerDetections::AddDetection ( BaseDetection * detection, Helpers::game_tm& time )
{
	// find list by udid
	rt_detection_list_by_udid udid ( detection->GetUDID () );

	int const dlist_index ( m_dict.Find ( udid ) );

	if( dlist_index != -1 )
	{
		rt_detection_list& dlist ( m_dict[ dlist_index ].m_list );

		// find if we have any detection with same content using content hash

		rt_detection_elem z ( time, detection );

		int const elem_index ( dlist.Find ( z ) );

		if( elem_index == -1 )
		{
			dlist.AddToTail ( std::move ( z ) );
		}
		else
		{
			if( detection->CloneWhenEqual () )
			{
				// add a pointer to the same detection
				z.m_detection = dlist[ elem_index ].m_detection;
				dlist.AddToTail ( std::move ( z ) );
			}
			delete detection; // delete this because we already have a detection with the same content
		}
	}
	else
	{
		// create new list and move informations
		rt_detection_elem x ( time, detection );
		rt_detection_list y;
		y.AddToTail ( std::move ( x ) );
		rt_detection_list_by_udid z ( detection->GetUDID () );
		z.m_list = std::move ( y );
		m_dict.AddToTail ( std::move ( z ) );
	}
}

int PlayerDetections::GetTotalDetectionsCount () const
{
	int dcount ( 0 );

	for( rt_detection_dict::const_iterator it ( m_dict.begin () ); it != m_dict.end (); ++it )
	{
		dcount += it->m_list.Count ();
	}

	return dcount;
}

basic_string PlayerDetections::GetActionMessage () const
{
	int const involved_testers_count ( m_dict.Count () );

	__assume( involved_testers_count >= 0 );

	if( involved_testers_count == 0 )
	{
		return "without detection";
	}
	else if( involved_testers_count == 1 )
	{
		// we have only one type of detection

		return Helpers::format ( "with %d detections from %s", GetTotalDetectionsCount (), m_dict[ 0 ].m_list[ 0 ].m_detection->GetTester ()->GetName () );
	}
	else
	{
		// we have multiple testers involved

		return Helpers::format ( "with %d detections from multiple testers", GetTotalDetectionsCount () );
	}
}

int _cdecl PlayerDetections::detection_list_compare ( rt_detection_elem const * a, rt_detection_elem const * b )
{
	return ( int ) ( b->m_time.m_floattime - a->m_time.m_floattime );
}

void PlayerDetections::FlushDetections ()
{
	// move detections to a single list

	rt_detection_list sorted_dlist;
	//sorted_dlist.EnsureCapacity ( GetTotalDetectionsCount () );

	for( rt_detection_dict::iterator it ( m_dict.begin () ); it != m_dict.end (); ++it )
	{
		sorted_dlist.AddVectorToTail ( std::move ( it->m_list ) );
	}

	m_dict.Purge ();

	// sort by systime
	sorted_dlist.Sort ( detection_list_compare );

	PlayerHandler::const_iterator ph ( m_playerindex );

	// open XML log file
	basic_string fname ( Helpers::format ( "logs/NoCheatZ_4_Logs/Detections/%s-%s.xml", ph->GetSteamID (), Helpers::getStrDateTime ( "%F_%X" ) ) );
	fname.replace ( ':', '-' );
	Logger::GetInstance ()->Msg<MSG_LOG> ( Helpers::format ( "Writing detections in %s", fname.c_str () ) );

	basic_string fpath ( ConfigManager::GetInstance ()->m_root_server_path );
	fpath.append ( fname );

	FILE * xml_file = fopen ( fname.c_str (), "a" );
	if( xml_file )
	{
		fprintf ( xml_file, "<?xml version=\"1.0\" encoding=\"ansi\"?>\n<!-- NoCheatZ 4 Detection File -->\n\n" );

		// write informations about the server and the plugin

		char const * mapname;
		int mapversion;

		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			SourceSdk::CGlobalVars_csgo const * gv ( reinterpret_cast< SourceSdk::CGlobalVars_csgo const * >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () ) );
			mapname = gv->mapname;
			mapversion = gv->mapversion;
		}
		else
		{
			SourceSdk::CGlobalVars const * gv ( reinterpret_cast< SourceSdk::CGlobalVars const * >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () ) );
			mapname = gv->mapname;
			mapversion = gv->mapversion;
		}

		ProcessFilter::HumanAtLeastConnecting hfilter;
		int const human_count ( NczPlayerManager::GetInstance ()->GetPlayerCount ( &hfilter ) );
		ProcessFilter::FakeClientOnly bfilter;
		int const bot_count ( NczPlayerManager::GetInstance ()->GetPlayerCount ( &bfilter ) );

		int isprivate ( 0 );
		if( *SourceSdk::InterfacesProxy::ConVar_GetString ( SourceSdk::InterfacesProxy::ICvar_FindVar ( "sv_password" ) ) != '\0' )
		{
			isprivate = 1;
		}

		fprintf ( xml_file,
				  "<plugin>\n\t"
				  "<version full=\"" NCZ_VERSION_GIT "\" short=\"" NCZ_VERSION_GIT_SHORT "\" />"
				  "</plugin>\n"
				  "<server_informations>\n\t"
				  "<game name=\"%s\" map=\"%s\" map_version=%d />\n\t"
				  "<clients human=%d bots=%d />\n\t"
				  "<status addr=\"%s:%s\" hostname=\"%s\" secure=%d private=%d />\n"
				  "</server_informations>",
				  ConfigManager::GetInstance ()->m_game_name.c_str (), mapname, mapversion,
				  human_count, bot_count,
				  SourceSdk::InterfacesProxy::ConVar_GetString ( SourceSdk::InterfacesProxy::ICvar_FindVar ( "ip" ) ), SourceSdk::InterfacesProxy::ConVar_GetString ( SourceSdk::InterfacesProxy::ICvar_FindVar ( "hostport" ) ), SourceSdk::InterfacesProxy::ConVar_GetString ( SourceSdk::InterfacesProxy::ICvar_FindVar ( "hostname" ) ), SteamGameServer_BSecure (), isprivate
		);

		// write informations about the player

		SourceSdk::IPlayerInfo * pinfo ( ph->GetPlayerInfo () );
		SourceSdk::INetChannelInfo const * netinfo ( ph->GetChannelInfo () );

		if( pinfo )
		{
			Assert ( netinfo );

			fprintf ( xml_file,
					  "<player_informations>\n\t"
					  "<absangles x=%f y=%f z=%f />\n\t"
					  "<absorigin x=%f y=%f z=%f />\n\t"
					  "<life health=%d armor=%d isdead=%d isinavehicule=%d />\n\t"
					  "<score frags=%d deaths=%d kdr=%f />\n\t"
					  "<player name=\"%s\" steamid=\"%s\" ip=\"%s\" userid=%d index=%d team=%d weapon=\"%s\" />\n\t"
					  "<net timeconnected=%f avgchoke=%f avgloss=%f avglatency=%f timeoutseconds=%f timesincelastpacketreceived=%f istimingout=%d />\n"
					  "</player_informations>\n",
					  pinfo->GetAbsAngles ().x, pinfo->GetAbsAngles ().y, pinfo->GetAbsAngles ().z,
					  pinfo->GetAbsOrigin ().x, pinfo->GetAbsOrigin ().y, pinfo->GetAbsOrigin ().z,
					  pinfo->GetHealth (), pinfo->GetArmorValue (), pinfo->IsDead (), pinfo->IsInAVehicle (),
					  pinfo->GetFragCount (), pinfo->GetDeathCount (), ( ( float ) ( pinfo->GetFragCount () ) / ( float ) ( pinfo->GetDeathCount () ) ),
					  pinfo->GetName (), pinfo->GetNetworkIDString (), netinfo->GetAddress (), pinfo->GetUserID (), m_playerindex, pinfo->GetTeamIndex (), pinfo->GetWeaponName (),
					  netinfo->GetTimeConnected (), netinfo->GetAvgChoke ( FLOW_OUTGOING ), netinfo->GetAvgLoss ( FLOW_INCOMING ), netinfo->GetAvgLatency ( FLOW_INCOMING ), netinfo->GetTimeoutSeconds (), netinfo->GetTimeSinceLastReceived (), netinfo->IsTimingOut ()
			);
		}
		else
		{
			fprintf ( xml_file, "<player_informations>unavailable</player_informations>\n" );
		}

		// write detections ...

		for( rt_detection_list::iterator it ( sorted_dlist.begin () ); it != sorted_dlist.end (); ++it )
		{
			fprintf ( xml_file,
					  "<detection title=\"%s\" tester=\"%s\" udid=%u >\n\t"
					  "<time systime=\"%s\ floattime=%f tv=%u tickrate=%u />\n\t"
					  "<data hash=%u >\n\t\t",
					  it->m_detection->GetDetectionLogMessage ().c_str (), it->m_detection->GetTester ()->GetName (), it->m_detection->GetUDID (),
					  Helpers::getStrDateTime ( "%x %X", it->m_time.m_systime ), it->m_time.m_floattime, it->m_time.m_tickrate,
					  it->m_detection->GetDataHash () );

			it->m_detection->WriteXMLOutput ( xml_file );

			fprintf ( xml_file,
					  "\n\t"
					  "</data>\n"
					  "</detection>\n" );

			it->m_detection->PerformCustomOutput ();
		}

		fclose ( xml_file );
	}
	else
	{
		Logger::GetInstance ()->Msg<MSG_ERROR> ( "Error opening detection file" );
	}

	sorted_dlist.Purge ();
}