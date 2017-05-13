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

#include "ConfigManager.h"

#include <fstream>

#include "Interfaces/InterfacesProxy.h"

#include "BaseSystem.h"
#include "Systems/Logger.h"
#include "Hooks/Hook.h" // GetModuleDirectoryFromMemoryAddress

#define FILE_LINE_BUFFER 1024

#ifdef WIN32
#	define ATTRIB_POST "_windows"
#else
#	define ATTRIB_POST "_linux"
#endif

bool GetIniAttributeValue ( std::ifstream & file, basic_string const & root, basic_string const & attrib, basic_string & value )
{
	LoggerAssert ( file.is_open () );

	file.clear ( std::ios::goodbit );
	file.seekg ( 0, std::ios::beg );
	if( file )
	{
		// find root
		basic_string root_expect ( "[" );
		root_expect.append ( root ).append ( "]" ).lower ();

		basic_string buf_ref;
		char buf[ FILE_LINE_BUFFER ];
		bool root_found ( false );

		do
		{
			file.getline ( buf, FILE_LINE_BUFFER );
			buf_ref = buf;
			buf_ref.lower ();

			if( buf_ref == root_expect )
			{
				root_found = true;
				break;
			}

		}
		while( !file.eof () );

		if( root_found )
		{
			// find attribute
			basic_string attrib_expect ( attrib );
			attrib_expect.append ( "=" );
			attrib_expect.lower ();

			do
			{
				file.getline ( buf, FILE_LINE_BUFFER );
				buf_ref = buf;
				buf_ref.lower ();

				if( buf_ref.find ( attrib_expect ) == 0 )
				{
					value = ( buf_ref.c_str () + attrib_expect.size () );
					return true;
				}

			}
			while( !file.eof () );
		}
		else
		{
			Logger::GetInstance ()->Msg<MSG_ERROR> ( Helpers::format ( "ConfigManager::GetIniAttributeValue : Cannot find root %s", root.c_str ()) );
		}
	}
	else
	{
		Logger::GetInstance ()->Msg<MSG_ERROR> (  "ConfigManager::GetIniAttributeValue : Cannot read ini file" );
	}
	value = "";
	Logger::GetInstance ()->Msg<MSG_ERROR> ( Helpers::format("ConfigManager::GetIniAttributeValue : Cannot find attribute %s -> %s", root.c_str(), attrib.c_str() ));
	return false;
}

ConfigManager::ConfigManager () :
	singleton_class (),
	content_version ( 1 ),
	m_playerdataclass ( "" ),
	m_smoke_radius ( 0.0f ),
	m_innersmoke_radius_sqr ( 0.0f ),
	m_smoke_timetobang ( 0.0f ),
	m_smoke_time ( 0.0f ),
	vfid_getdatadescmap ( 0 ),
	vfid_settransmit ( 0 ),
	vfid_mhgroundentity ( 0 ),
	vfid_weaponequip ( 0 ),
	vfid_weapondrop ( 0 ),
	vfid_playerruncommand ( 0 ),
	vfid_dispatch ( 0 ),
	vfid_thinkpost ( 0 )
{

}

ConfigManager::~ConfigManager ()
{

}

bool ConfigManager::LoadConfig ()
{
	basic_string path ( GetModuleDirectoryFromMemoryAddress ( reinterpret_cast<DWORD>(&vfid_getdatadescmap) ) );
	m_root_server_path = path;
	m_root_server_path.replace ( "addons/NoCheatZ/", "" );
	m_game_name = m_root_server_path;
	m_game_name.remove ( 0, m_game_name.find_last_of ( "/\\" ) );
	
	//SourceSdk::InterfacesProxy::GetGameDir ( path );
	//basic_string gamename ( path );
	
	path.append ( "config.ini" );
	std::ifstream file ( path.c_str (), std::ios::in );
	if( file )
	{
		basic_string value;
		if( !GetIniAttributeValue ( file, "CONFIG", "config_version", value ) ) return false;
		if( atoi ( value.c_str () ) >= content_version )
		{
			if( GetIniAttributeValue ( file, "CONFIG", "override_game", value ) )
			{
				if( value != "" )
				{
					value.upper ();
					Logger::GetInstance ()->Msg<MSG_LOG> ( Helpers::format ( "ConfigManager::LoadConfig : Replaced game name %s by %s according to override_game value in config.ini", m_game_name.c_str (), value.c_str () ) );
					m_game_name = value ;
				}
			}

			// load virtual functions id
			if( !GetIniAttributeValue ( file, m_game_name, "getdatadescmap" ATTRIB_POST, value ) ) return false;
			vfid_getdatadescmap = atoi ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "settransmit" ATTRIB_POST, value ) ) return false;
			vfid_settransmit = atoi ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "mhgroundentity" ATTRIB_POST, value ) ) return false;
			vfid_mhgroundentity = atoi ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "weaponequip" ATTRIB_POST, value ) ) return false;
			vfid_weaponequip = atoi ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "weapondrop" ATTRIB_POST, value ) ) return false;
			vfid_weapondrop = atoi ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "playerruncommand" ATTRIB_POST, value ) ) return false;
			vfid_playerruncommand = atoi ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "dispatch" ATTRIB_POST, value ) ) return false;
			vfid_dispatch = atoi ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "thinkpost" ATTRIB_POST, value ) ) return false;
			vfid_thinkpost = atoi ( value.c_str () );

			// load some strings

			if( !GetIniAttributeValue ( file, m_game_name, "playerdataclass", m_playerdataclass ) ) return false;

			// load some values

			if( !GetIniAttributeValue ( file, m_game_name, "f_smoketime", value ) ) return false;
			m_smoke_time = ( float ) atof ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "f_smoke_time_to_bang", value ) ) return false;
			m_smoke_timetobang = ( float ) atof ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "f_inner_smoke_radius_sqr", value ) ) return false;
			m_innersmoke_radius_sqr = ( float ) atof ( value.c_str () );

			if( !GetIniAttributeValue ( file, m_game_name, "f_smoke_radius", value ) ) return false;
			m_smoke_radius = ( float ) atof ( value.c_str () );

			// disable systems

			if( !GetIniAttributeValue ( file, m_game_name, "disable_systems", value ) ) return false;

			CUtlVector<basic_string> systems;
			SplitString<char> ( value, ';', systems );

			for( CUtlVector<basic_string>::iterator it ( systems.begin () ); it != systems.end (); ++it )
			{
				BaseSystem* system ( BaseSystem::FindSystemByName ( it->c_str () ) );
				if( system )
				{
					system->SetDisabledByConfigIni ();
					Logger::GetInstance ()->Msg<MSG_CONSOLE> ( Helpers::format ( "ConfigManager : Disabled system %s", it->c_str () ) );
				}
				else
				{
					Logger::GetInstance ()->Msg<MSG_ERROR> ( Helpers::format ( "ConfigManager : Unable to disable system %s -> system not known", it->c_str () ) );
				}
			}

			return true;

		}
		else
		{
			Logger::GetInstance ()->Msg<MSG_ERROR> (  "ConfigManager::LoadConfig : content_version too old"  );
		}
	}
	else
	{
		Logger::GetInstance ()->Msg<MSG_ERROR> ( "ConfigManager::LoadConfig : Cannot open config.ini"  );
	}

	return false;
}
